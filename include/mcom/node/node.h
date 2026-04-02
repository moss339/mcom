#pragma once
#include "node_state.h"
#include "node_config.h"
#include <mcom/types.h>
#include <mcom/service/service_client.h>
#include <mcom/service/service_server.h>
#include <mcom/service/proto_service_client.h>
#include <mcom/service/proto_service_server.h>
#include <mcom/action/action_client.h>
#include <mcom/action/action_server.h>
#include <mcom/action/proto_action_client.h>
#include <mcom/action/proto_action_server.h>
#include <mcom/topic/proto_publisher.h>
#include <mcom/topic/proto_subscriber.h>
#include <mruntime/node.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace moss {
namespace mcom {

class Node : public std::enable_shared_from_this<Node> {
public:
    static std::shared_ptr<Node> create(const std::string& node_name,
                                        uint8_t domain_id = 0);

    explicit Node(const NodeConfig& config);
    ~Node();

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    bool init();
    bool start();
    void stop();
    void destroy();

    bool is_running() const;
    NodeState get_state() const;
    const std::string& get_name() const;
    uint8_t get_domain_id() const;

    template<typename T>
    std::shared_ptr<moss::mdds::Publisher<T>> create_publisher(
        const std::string& topic_name,
        const mdds::QoSConfig& qos);

    template<typename T>
    std::shared_ptr<moss::mdds::Subscriber<T>> create_subscriber(
        const std::string& topic_name,
        typename moss::mdds::Subscriber<T>::DataCallback callback,
        const mdds::QoSConfig& qos);

    template<typename T>
    topic::ProtoPublisherPtr<T> create_publisher(const std::string& topic_name);

    template<typename T>
    topic::ProtoSubscriberPtr<T> create_subscriber(
        const std::string& topic_name,
        typename topic::ProtoSubscriber<T>::DataCallback callback);

    service::ServiceClientPtr create_service_client(
        service::ServiceId service_id, service::InstanceId instance_id);

    service::ServiceServerPtr create_service_server(
        service::ServiceId service_id, service::InstanceId instance_id);

    service::ProtoServiceClientPtr create_proto_service_client(
        const std::string& service_name, uint32_t service_id);

    service::ProtoServiceServerPtr create_proto_service_server(
        const std::string& service_name, uint32_t service_id);

    action::ActionClientPtr create_action_client(
        action::ActionClientConfig config);

    action::ActionServerPtr create_action_server(
        action::ActionServerConfig config);

    template<typename TGoal>
    action::ProtoActionClientPtr<TGoal> create_action_client(
        const std::string& action_name,
        uint32_t action_id);

    template<typename TGoal, typename TResult>
    action::ProtoActionServerPtr<TGoal, TResult> create_action_server(
        const std::string& action_name,
        uint32_t action_id);

    template<typename T>
    service::ProtoServiceClientPtr create_service_client(
        const std::string& service_name);

    void spin();
    void spin_once();

private:
    void transition_state(NodeState new_state);

    NodeConfig config_;
    NodeState state_{NodeState::UNINITIALIZED};
    mutable std::mutex state_mutex_;

    std::shared_ptr<mruntime::Node> runtime_node_;

    std::mutex endpoints_mutex_;
};

}  // namespace mcom
}  // namespace moss
