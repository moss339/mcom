#include "mcom/service/proto_service_client.h"
#include "mcom/service/service_manager.h"
#include "mcom/topic/topic_manager.h"
#include <mdds/mdds.h>

namespace moss {
namespace mcom {
namespace service {

ProtoServiceClient::ProtoServiceClient(const std::string& service_name, uint32_t service_id)
    : service_name_(service_name),
      service_id_(service_id),
      initialized_(false) {
}

ProtoServiceClient::~ProtoServiceClient() = default;

void ProtoServiceClient::init() {
    if (initialized_) {
        return;
    }

    auto& tm = topic::TopicManager::instance();
    if (!tm.get_participant()) {
        return;
    }

    auto request_topic = get_request_topic();
    auto response_topic = get_response_topic();

    request_pub_ = tm.create_proto_publisher<moss::proto::service::ServiceRequest>(
        request_topic);

    response_sub_ = tm.create_proto_subscriber<moss::proto::service::ServiceResponse>(
        response_topic,
        [this](const moss::proto::service::ServiceResponse& resp, uint64_t) {
            handle_response(resp);
        });

    initialized_ = true;
}

std::string ProtoServiceClient::get_request_topic() const {
    return "service_" + std::to_string(service_id_) + "_request";
}

std::string ProtoServiceClient::get_response_topic() const {
    return "service_" + std::to_string(service_id_) + "_response";
}

void ProtoServiceClient::handle_response(const moss::proto::service::ServiceResponse& resp) {
    std::lock_guard<std::mutex> lock(pending_mutex_);
    auto it = pending_requests_.find(resp.request_id());
    if (it != pending_requests_.end()) {
        it->second.set_value(resp);
        pending_requests_.erase(it);
    }
}

bool ProtoServiceClient::is_service_available() const {
    return initialized_;
}

const std::string& ProtoServiceClient::get_service_name() const {
    return service_name_;
}

uint32_t ProtoServiceClient::get_service_id() const {
    return service_id_;
}

}  // namespace service
}  // namespace mcom
}  // namespace moss
