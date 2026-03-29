#include "mservice/service_client.h"
#include "mservice/service_manager.h"
#include <chrono>

namespace mservice {

ServiceClient::ServiceClient(ServiceId service_id, InstanceId instance_id)
    : service_id_(service_id),
      instance_id_(instance_id),
      initialized_(false) {
}

ServiceClient::~ServiceClient() = default;

void ServiceClient::init() {
    initialized_ = true;
}

std::optional<Response> ServiceClient::send_request(
    MethodId method_id,
    const std::vector<uint8_t>& payload,
    std::chrono::milliseconds timeout) {
    (void)method_id;
    (void)payload;
    (void)timeout;

    Response response;
    response.header.session_id = ServiceManager::instance().generate_session_id();
    response.header.service_id = service_id_;
    response.header.instance_id = instance_id_;
    response.header.method_id = method_id;
    response.error = ServiceError::OK;
    response.payload = payload;
    return response;
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
    (void)method_id;
    (void)payload;
    return true;
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

}  // namespace mservice
