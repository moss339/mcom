#include "mcom/service/proto_service_server.h"
#include "mcom/topic/topic_manager.h"
#include <mdds/mdds.h>

namespace moss {
namespace mcom {
namespace service {

namespace {

uint64_t get_current_timestamp_us() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count();
}

}  // anonymous namespace

ProtoServiceServer::ProtoServiceServer(const std::string& service_name, uint32_t service_id)
    : service_name_(service_name),
      service_id_(service_id),
      offered_(false) {
}

ProtoServiceServer::~ProtoServiceServer() {
    stop_offer();
}

std::string ProtoServiceServer::get_request_topic() const {
    return "service_" + std::to_string(service_id_) + "_request";
}

std::string ProtoServiceServer::get_response_topic() const {
    return "service_" + std::to_string(service_id_) + "_response";
}

void ProtoServiceServer::offer() {
    if (offered_) {
        return;
    }

    auto& tm = topic::TopicManager::instance();
    if (!tm.get_participant()) {
        return;
    }

    auto request_topic = get_request_topic();
    auto response_topic = get_response_topic();

    request_sub_ = tm.create_proto_subscriber<moss::proto::service::ServiceRequest>(
        request_topic,
        [this](const moss::proto::service::ServiceRequest& req, uint64_t) {
            handle_request(req);
        });

    response_pub_ = tm.create_proto_publisher<moss::proto::service::ServiceResponse>(
        response_topic);

    offered_ = true;
}

void ProtoServiceServer::stop_offer() {
    if (!offered_) {
        return;
    }
    offered_ = false;
    request_sub_.reset();
    response_pub_.reset();
}

void ProtoServiceServer::handle_request(const moss::proto::service::ServiceRequest& req) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);

    auto it = handlers_.find(req.method_id());
    if (it == handlers_.end()) {
        moss::proto::service::ServiceResponse resp;
        resp.set_timestamp_us(get_current_timestamp_us());
        resp.set_request_id(req.request_id());
        resp.set_error(moss::proto::service::ServiceResponse::METHOD_NOT_FOUND);
        response_pub_->publish(resp);
        return;
    }

    try {
        moss::proto::service::ServiceRequest req_copy;
        req_copy.CopyFrom(req);

        auto resp_msg = it->second(req_copy);
        if (!resp_msg) {
            moss::proto::service::ServiceResponse resp;
            resp.set_timestamp_us(get_current_timestamp_us());
            resp.set_request_id(req.request_id());
            resp.set_error(moss::proto::service::ServiceResponse::INTERNAL_ERROR);
            response_pub_->publish(resp);
            return;
        }

        moss::proto::service::ServiceResponse resp;
        resp.set_timestamp_us(get_current_timestamp_us());
        resp.set_request_id(req.request_id());
        resp.set_error(moss::proto::service::ServiceResponse::OK);
        resp_msg->SerializeToString(resp.mutable_response_payload());
        response_pub_->publish(resp);

    } catch (const std::exception&) {
        moss::proto::service::ServiceResponse resp;
        resp.set_timestamp_us(get_current_timestamp_us());
        resp.set_request_id(req.request_id());
        resp.set_error(moss::proto::service::ServiceResponse::INTERNAL_ERROR);
        response_pub_->publish(resp);
    }
}

const std::string& ProtoServiceServer::get_service_name() const {
    return service_name_;
}

uint32_t ProtoServiceServer::get_service_id() const {
    return service_id_;
}

}  // namespace service
}  // namespace mcom
}  // namespace moss
