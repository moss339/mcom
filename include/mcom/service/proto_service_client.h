#pragma once
/**
 * @file proto_service_client.h
 * @brief Protobuf RPC 客户端
 *
 * 基于 ProtoSubscriber/ProtoPublisher 实现请求-响应模式。
 * ServiceRequest/Response 使用 protobuf 序列化。
 */

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <future>
#include <optional>
#include <google/protobuf/message.h>
#include "mcom/topic/proto_publisher.h"
#include "mcom/topic/proto_subscriber.h"
#include "mcom/service/service_manager.h"
#include "moss/proto/service/rpc.pb.h"

namespace moss {
namespace mcom {
namespace service {

inline uint64_t get_current_timestamp_us() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count();
}

/**
 * @brief Protobuf RPC 客户端
 *
 * 用法示例：
 * @code
 * auto client = node->create_service_client("perception_service", 0x1001);
 *
 * moss::proto::service::PerceptionRequest req;
 * req.set_capture_time_us(get_timestamp_us());
 * req.set_sensor_data(sensor_buffer);
 *
 * auto resp = client->call<moss::proto::service::PerceptionRequest,
 *                          moss::proto::service::PerceptionResponse>(
 *     1, req, std::chrono::milliseconds(2000));
 *
 * if (resp && resp->success()) {
 *     for (const auto& det : resp->detections()) {
 *         fmt::print("检测到: {} 置信度 {:.2f}\n", det.class_name(), det.confidence());
 *     }
 * }
 * @endcode
 */
class ProtoServiceClient : public std::enable_shared_from_this<ProtoServiceClient> {
public:
    /**
     * @brief 构造 RPC 客户端
     * @param service_name 服务名称
     * @param service_id 服务 ID
     */
    ProtoServiceClient(const std::string& service_name, uint32_t service_id);

    ~ProtoServiceClient();

    /**
     * @brief 初始化客户端（创建 publisher/subscriber）
     */
    void init();

    /**
     * @brief 同步调用
     * @tparam Req 请求 protobuf 消息类型
     * @tparam Resp 响应 protobuf 消息类型
     * @param method_id 方法 ID
     * @param request 请求消息
     * @param timeout 超时时间
     * @return 响应消息，如果失败返回 std::nullopt
     */
    template<typename Req, typename Resp>
    std::optional<Resp> call(uint32_t method_id,
                             const Req& request,
                             std::chrono::milliseconds timeout = std::chrono::milliseconds(1000));

    /**
     * @brief 异步调用
     * @tparam Req 请求类型
     * @tparam Resp 响应类型
     * @param method_id 方法 ID
     * @param request 请求消息
     * @return 响应 future
     */
    template<typename Req, typename Resp>
    std::future<std::optional<Resp>> call_async(uint32_t method_id, const Req& request);

    /**
     * @brief 单向调用（不等待响应）
     * @tparam Req 请求类型
     * @param method_id 方法 ID
     * @param request 请求消息
     */
    template<typename Req>
    void call_no_return(uint32_t method_id, const Req& request);

    /**
     * @brief 检查服务是否可用
     */
    bool is_service_available() const;

    /**
     * @brief 获取服务名称
     */
    const std::string& get_service_name() const;

    /**
     * @brief 获取服务 ID
     */
    uint32_t get_service_id() const;

private:
    std::string get_request_topic() const;
    std::string get_response_topic() const;
    void handle_response(const moss::proto::service::ServiceResponse& resp);

    std::string service_name_;
    uint32_t service_id_;
    bool initialized_;

    std::shared_ptr<topic::ProtoPublisher<moss::proto::service::ServiceRequest>> request_pub_;
    std::shared_ptr<topic::ProtoSubscriber<moss::proto::service::ServiceResponse>> response_sub_;

    using PendingRequest = std::promise<std::optional<moss::proto::service::ServiceResponse>>;
    std::map<uint32_t, PendingRequest> pending_requests_;
    std::mutex pending_mutex_;
};

using ProtoServiceClientPtr = std::shared_ptr<ProtoServiceClient>;

// Template implementation

template<typename Req, typename Resp>
std::optional<Resp> ProtoServiceClient::call(uint32_t method_id,
                                              const Req& request,
                                              std::chrono::milliseconds timeout) {
    static_assert(std::is_base_of_v<google::protobuf::Message, Req>,
                  "Req must be a protobuf Message");
    static_assert(std::is_base_of_v<google::protobuf::Message, Resp>,
                  "Resp must be a protobuf Message");

    if (!initialized_) {
        return std::nullopt;
    }

    uint32_t request_id = ServiceManager::instance().generate_session_id();

    std::promise<std::optional<moss::proto::service::ServiceResponse>> promise;
    auto future = promise.get_future();

    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_requests_[request_id] = std::move(promise);
    }

    moss::proto::service::ServiceRequest req;
    req.set_timestamp_us(get_current_timestamp_us());
    req.set_request_id(request_id);
    req.set_service_name(service_name_);
    req.set_service_id(service_id_);
    req.set_method_id(method_id);

    if (!request.SerializeToString(req.mutable_request_payload())) {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_requests_.erase(request_id);
        return std::nullopt;
    }

    request_pub_->publish(req);

    auto status = future.wait_for(timeout);

    std::lock_guard<std::mutex> lock(pending_mutex_);
    auto it = pending_requests_.find(request_id);
    if (it != pending_requests_.end()) {
        pending_requests_.erase(it);
    }

    if (status == std::future_status::timeout) {
        return std::nullopt;
    }

    auto resp_opt = future.get();
    if (!resp_opt) {
        return std::nullopt;
    }

    const auto& resp = *resp_opt;
    if (resp.error() != moss::proto::service::ServiceResponse::OK) {
        return std::nullopt;
    }

    Resp result;
    if (!result.ParseFromString(resp.response_payload())) {
        return std::nullopt;
    }

    return result;
}

template<typename Req, typename Resp>
std::future<std::optional<Resp>> ProtoServiceClient::call_async(uint32_t method_id,
                                                                  const Req& request) {
    return std::async(std::launch::async, [this, method_id, request]() {
        return call<Req, Resp>(method_id, request);
    });
}

template<typename Req>
void ProtoServiceClient::call_no_return(uint32_t method_id, const Req& request) {
    call<Req, moss::proto::service::ServiceResponse>(method_id, request,
        std::chrono::milliseconds(100));
}

}  // namespace service
}  // namespace mcom
}  // namespace moss
