#include "mservice/service_server.h"

namespace mservice {

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

void ServiceServer::offer() {
    if (offered_) {
        return;
    }
    offered_ = true;
}

void ServiceServer::stop_offer() {
    if (!offered_) {
        return;
    }
    offered_ = false;
}

void ServiceServer::register_method(MethodId method_id, RequestHandler handler) {
    handlers_[method_id] = handler;
}

void ServiceServer::unregister_method(MethodId method_id) {
    handlers_.erase(method_id);
}

void ServiceServer::send_response(const Request& request,
                                 const std::vector<uint8_t>& payload) {
    (void)request;
    (void)payload;
}

void ServiceServer::send_error_response(const Request& request, ServiceError error) {
    (void)request;
    (void)error;
}

ServiceId ServiceServer::get_service_id() const {
    return service_id_;
}

InstanceId ServiceServer::get_instance_id() const {
    return instance_id_;
}

}  // namespace mservice
