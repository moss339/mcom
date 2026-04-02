#pragma once
/**
 * @file proto_service_server.h
 * @brief Protobuf RPC 服务端
 *
 * 基于 ProtoSubscriber/ProtoPublisher 实现请求-响应模式。
 */

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <functional>
#include <google/protobuf/message.h>
#include "mcom/topic/proto_publisher.h"
#include "mcom/topic/proto_subscriber.h"
#include "moss/proto/service/rpc.pb.h"

namespace moss {
namespace mcom {
namespace service {

/**
 * @brief Protobuf RPC 服务端
 *
 * 用法示例：
 * @code
 * auto server = node->create_service_server("perception_service", 0x1001);
 *
 * server->register_method<moss::proto::service::PerceptionRequest,
 *                        moss::proto::service::PerceptionResponse>(
 *     1,
 *     [](const moss::proto::service::PerceptionRequest& req)
 *         -> moss::proto::service::PerceptionResponse {
 *         moss::proto::service::PerceptionResponse resp;
 *         resp.set_success(true);
 *         // ... 执行感知逻辑
 *         return resp;
 *     });
 *
 * server->offer();
 * @endcode
 */
class ProtoServiceServer : public std::enable_shared_from_this<ProtoServiceServer> {
public:
    /**
     * @brief 构造 RPC 服务端
     * @param service_name 服务名称
     * @param service_id 服务 ID
     */
    ProtoServiceServer(const std::string& service_name, uint32_t service_id);

    ~ProtoServiceServer();

    /**
     * @brief 注册方法处理函数
     * @tparam Req 请求类型（protobuf Message）
     * @tparam Resp 响应类型（protobuf Message）
     * @param method_id 方法 ID
     * @param handler 处理函数
     */
    template<typename Req, typename Resp>
    void register_method(uint32_t method_id,
                         std::function<Resp(const Req&)> handler);

    /**
     * @brief 开始提供服务
     */
    void offer();

    /**
     * @brief 停止提供服务
     */
    void stop_offer();

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
    void handle_request(const moss::proto::service::ServiceRequest& req);

    std::string service_name_;
    uint32_t service_id_;
    bool offered_;

    std::shared_ptr<topic::ProtoSubscriber<moss::proto::service::ServiceRequest>> request_sub_;
    std::shared_ptr<topic::ProtoPublisher<moss::proto::service::ServiceResponse>> response_pub_;

    using MethodHandler = std::function<std::unique_ptr<google::protobuf::Message>(const google::protobuf::Message&)>;
    std::map<uint32_t, MethodHandler> handlers_;
    std::mutex handlers_mutex_;
};

using ProtoServiceServerPtr = std::shared_ptr<ProtoServiceServer>;

// Template implementation

template<typename Req, typename Resp>
void ProtoServiceServer::register_method(uint32_t method_id,
                                         std::function<Resp(const Req&)> handler) {
    static_assert(std::is_base_of_v<google::protobuf::Message, Req>,
                  "Req must be a protobuf Message");
    static_assert(std::is_base_of_v<google::protobuf::Message, Resp>,
                  "Resp must be a protobuf Message");

    std::lock_guard<std::mutex> lock(handlers_mutex_);
    handlers_[method_id] = [handler](const google::protobuf::Message& req) {
        const Req& typed_req = static_cast<const Req&>(req);
        Resp resp = handler(typed_req);
        return std::unique_ptr<google::protobuf::Message>(resp.New());
    };
}

}  // namespace service
}  // namespace mcom
}  // namespace moss
