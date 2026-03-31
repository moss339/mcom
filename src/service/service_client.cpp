#include "mcom/service/service_client.h"
#include "mcom/service/service_manager.h"
#include <mdds/mdds.h>
#include <chrono>

namespace moss {
namespace mcom {
namespace service {

ServiceClient::ServiceClient(ServiceId service_id, InstanceId instance_id)
    : service_id_(service_id),
      instance_id_(instance_id),
      initialized_(false) {
}

ServiceClient::~ServiceClient() = default;

void ServiceClient::init() {
    if (initialized_) {
        return;
    }
    initialized_ = true;
}

std::string ServiceClient::get_request_topic() const {
    return "service_" + std::to_string(service_id_) + "_" +
           std::to_string(instance_id_) + "_request";
}

std::string ServiceClient::get_response_topic() const {
    return "service_" + std::to_string(service_id_) + "_" +
           std::to_string(instance_id_) + "_response";
}

void ServiceClient::handle_response(const Response& response) {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    auto it = pending_requests_.find(response.header.session_id);
    if (it != pending_requests_.end()) {
        it->second.set_value(response);
        pending_requests_.erase(it);
    }
}

std::optional<Response> ServiceClient::send_request(
    MethodId method_id,
    const std::vector<uint8_t>& payload,
    std::chrono::milliseconds timeout) {

    if (!initialized_) {
        init();
    }

    auto participant = topic::TopicManager::instance().get_participant();
    if (!participant) {
        return std::nullopt;
    }

    SessionId session_id = ServiceManager::instance().generate_session_id();

    std::promise<std::optional<Response>> promise;
    auto future = promise.get_future();

    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_requests_[session_id] = std::move(promise);
    }

    Request request;
    request.header.session_id = session_id;
    request.header.service_id = service_id_;
    request.header.instance_id = instance_id_;
    request.header.method_id = method_id;
    request.payload = payload;

    auto request_topic = get_request_topic();
    auto response_topic = get_response_topic();

    auto publisher = participant->create_publisher<ServiceMessage>(request_topic);
    if (!publisher) {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_requests_.erase(session_id);
        return std::nullopt;
    }

    ServiceMessage req_msg;
    req_msg.data_ = request.serialize();

    auto subscriber = participant->create_subscriber<ServiceMessage>(
        response_topic,
        [this](const ServiceMessage& msg, uint64_t) {
            Response resp = Response::deserialize(msg.data_.data(), msg.data_.size());
            if (resp.header.session_id != 0) {
                handle_response(resp);
            }
        });

    if (!subscriber) {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_requests_.erase(session_id);
        return std::nullopt;
    }

    publisher->write(req_msg);

    auto status = future.wait_for(timeout);

    std::lock_guard<std::mutex> lock(pending_mutex_);
    auto it = pending_requests_.find(session_id);
    if (it != pending_requests_.end()) {
        pending_requests_.erase(it);
    }

    if (status == std::future_status::timeout) {
        return std::nullopt;
    }

    return future.get();
}

std::future<std::optional<Response>> ServiceClient::send_request_async(
    MethodId method_id,
    const std::vector<uint8_t>& payload,
    std::chrono::milliseconds timeout) {
    return std::async(std::launch::async, [this, method_id, payload, timeout]() {
        return send_request(method_id, payload, timeout);
    });
}

bool ServiceClient::send_request_no_return(MethodId method_id,
                                           const std::vector<uint8_t>& payload) {
    auto result = send_request(method_id, payload, std::chrono::milliseconds(100));
    return result.has_value();
}

bool ServiceClient::is_service_available() const {
    return initialized_;
}

ServiceId ServiceClient::get_service_id() const {
    return service_id_;
}

InstanceId ServiceClient::get_instance_id() const {
    return instance_id_;
}

}  // namespace service
}  // namespace mcom

}  // namespace moss
