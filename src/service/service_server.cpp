#include "mcom/service/service_server.h"
#include <mdds/mdds.h>

namespace moss {
namespace mcom {
namespace service {

ServiceServer::ServiceServer(ServiceId service_id, InstanceId instance_id)
    : service_id_(service_id),
      instance_id_(instance_id),
      offered_(false) {
}

ServiceServer::~ServiceServer() {
    stop_offer();
}

void ServiceServer::init() {
}

std::string ServiceServer::get_request_topic() const {
    return "service_" + std::to_string(service_id_) + "_" +
           std::to_string(instance_id_) + "_request";
}

std::string ServiceServer::get_response_topic() const {
    return "service_" + std::to_string(service_id_) + "_" +
           std::to_string(instance_id_) + "_response";
}

void ServiceServer::offer() {
    if (offered_) {
        return;
    }

    auto participant = topic::TopicManager::instance().get_participant();
    if (!participant) {
        return;
    }

    auto request_topic = get_request_topic();
    auto response_topic = get_response_topic();

    auto subscriber = participant->create_subscriber<ServiceMessage>(
        request_topic,
        [this](const ServiceMessage& msg, uint64_t) {
            Request req = Request::deserialize(msg.data_.data(), msg.data_.size());
            if (req.header.service_id == service_id_ &&
                req.header.instance_id == instance_id_) {
                handle_request(req);
            }
        });

    offered_ = true;
}

void ServiceServer::stop_offer() {
    if (!offered_) {
        return;
    }
    offered_ = false;
}

void ServiceServer::handle_request(const Request& request) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);

    auto it = handlers_.find(request.header.method_id);
    if (it == handlers_.end()) {
        send_error_response(request, ServiceError::METHOD_NOT_FOUND);
        return;
    }

    try {
        Response response = it->second(request);
        send_response(request, response.payload);
    } catch (const std::exception&) {
        send_error_response(request, ServiceError::INTERNAL_ERROR);
    }
}

void ServiceServer::register_method(MethodId method_id, RequestHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_[method_id] = handler;
}

void ServiceServer::unregister_method(MethodId method_id) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_.erase(method_id);
}

void ServiceServer::send_response(const Request& request,
                                 const std::vector<uint8_t>& payload) {
    auto participant = topic::TopicManager::instance().get_participant();
    if (!participant) {
        return;
    }

    static thread_local auto publisher = participant->create_publisher<ServiceMessage>(
        get_response_topic());

    Response response;
    response.header.session_id = request.header.session_id;
    response.header.service_id = service_id_;
    response.header.instance_id = instance_id_;
    response.header.method_id = request.header.method_id;
    response.error = ServiceError::OK;
    response.payload = payload;

    ServiceMessage msg;
    msg.data_ = response.serialize();
    publisher->write(msg);
}

void ServiceServer::send_error_response(const Request& request, ServiceError error) {
    auto participant = topic::TopicManager::instance().get_participant();
    if (!participant) {
        return;
    }

    static thread_local auto publisher = participant->create_publisher<ServiceMessage>(
        get_response_topic());

    Response response;
    response.header.session_id = request.header.session_id;
    response.header.service_id = service_id_;
    response.header.instance_id = instance_id_;
    response.header.method_id = request.header.method_id;
    response.error = error;

    ServiceMessage msg;
    msg.data_ = response.serialize();
    publisher->write(msg);
}

ServiceId ServiceServer::get_service_id() const {
    return service_id_;
}

InstanceId ServiceServer::get_instance_id() const {
    return instance_id_;
}

}  // namespace service
}  // namespace mcom

}  // namespace moss
