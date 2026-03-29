#ifndef MCOM_NODE_NODE_H
#define MCOM_NODE_NODE_H

#include "node_state.h"
#include "node_config.h"
#include <mcom/types.h>
#include <mcom/service/service_client.h>
#include <mcom/service/service_server.h>
#include <mcom/action/action_client.h>
#include <mcom/action/action_server.h>

#include <mdds/mdds.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace mcom {

class Node : public std::enable_shared_from_this<Node> {
public:
    static std::shared_ptr<Node> create(const std::string& node_name,
                                        mdds::DomainId domain_id = 0);

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
    mdds::DomainId get_domain_id() const;

    // Topic creation - returns mdds typed publisher/subscriber
    template<typename T>
    std::shared_ptr<mdds::Publisher<T>> create_publisher(
        const std::string& topic_name,
        const mdds::QoSConfig& qos = mdds::default_qos::publisher());

    template<typename T>
    std::shared_ptr<mdds::Subscriber<T>> create_subscriber(
        const std::string& topic_name,
        typename mdds::Subscriber<T>::DataCallback callback,
        const mdds::QoSConfig& qos = mdds::default_qos::subscriber());

    // Service creation
    service::ServiceClientPtr create_service_client(
        service::ServiceId service_id, service::InstanceId instance_id);

    service::ServiceServerPtr create_service_server(
        service::ServiceId service_id, service::InstanceId instance_id);

    // Action creation
    action::ActionClientPtr create_action_client(
        action::ActionClientConfig config);

    action::ActionServerPtr create_action_server(
        action::ActionServerConfig config);

private:
    void transition_state(NodeState new_state);

    NodeConfig config_;
    NodeState state_{NodeState::UNINITIALIZED};
    mutable std::mutex state_mutex_;

    std::shared_ptr<mdds::DomainParticipant> participant_;

    std::mutex endpoints_mutex_;
};

template<typename T>
std::shared_ptr<mdds::Publisher<T>> Node::create_publisher(
    const std::string& topic_name, const mdds::QoSConfig& qos) {

    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    return participant_->create_publisher<T>(topic_name, qos);
}

template<typename T>
std::shared_ptr<mdds::Subscriber<T>> Node::create_subscriber(
    const std::string& topic_name,
    typename mdds::Subscriber<T>::DataCallback callback,
    const mdds::QoSConfig& qos) {

    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    return participant_->create_subscriber<T>(topic_name, std::move(callback), qos);
}

}  // namespace mcom

#endif  // MCOM_NODE_NODE_H
