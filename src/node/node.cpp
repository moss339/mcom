#include "mcom/node/node.h"
#include <mdds/mdds.h>
#include <mcom/service/service_client.h>
#include <mcom/service/service_server.h>
#include <mcom/action/action_client.h>
#include <mcom/action/action_server.h>

namespace moss {
namespace mcom {

NodeStateException::NodeStateException(NodeState current, NodeState expected)
    : NodeException("Node state error: expected " + std::string(to_string(expected)) +
                    " but current state is " + std::string(to_string(current)))
    , current_(current), expected_(expected) {
}

Node::Node(const NodeConfig& config)
    : config_(config) {
}

Node::~Node() {
    if (state_ != NodeState::DESTROYED && state_ != NodeState::UNINITIALIZED) {
        destroy();
    }
}

std::shared_ptr<Node> Node::create(const std::string& node_name, uint8_t domain_id) {
    NodeConfig config;
    config.node_name = node_name;
    config.domain_id = domain_id;
    return std::shared_ptr<Node>(new Node(config));
}

bool Node::init() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (state_ != NodeState::UNINITIALIZED) {
        return false;
    }

    participant_ = moss::mdds::DomainParticipant::create(config_.domain_id);
    if (!participant_) {
        return false;
    }

    transition_state(NodeState::INITIALIZED);
    return true;
}

bool Node::start() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (state_ != NodeState::INITIALIZED) {
        return false;
    }

    if (!participant_->start()) {
        return false;
    }

    transition_state(NodeState::RUNNING);
    return true;
}

void Node::stop() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (state_ != NodeState::RUNNING) {
        return;
    }

    participant_->stop();
    transition_state(NodeState::STOPPED);
}

void Node::destroy() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (state_ == NodeState::DESTROYED) {
        return;
    }

    if (state_ == NodeState::RUNNING) {
        stop();
    }

    participant_.reset();
    transition_state(NodeState::DESTROYED);
}

bool Node::is_running() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_ == NodeState::RUNNING;
}

NodeState Node::get_state() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return state_;
}

const std::string& Node::get_name() const {
    return config_.node_name;
}

uint8_t Node::get_domain_id() const {
    return config_.domain_id;
}

void Node::transition_state(NodeState new_state) {
    state_ = new_state;
}

service::ServiceClientPtr Node::create_service_client(
    service::ServiceId service_id, service::InstanceId instance_id) {
    return std::make_shared<service::ServiceClient>(service_id, instance_id);
}

service::ServiceServerPtr Node::create_service_server(
    service::ServiceId service_id, service::InstanceId instance_id) {
    return std::make_shared<service::ServiceServer>(service_id, instance_id);
}

action::ActionClientPtr Node::create_action_client(action::ActionClientConfig config) {
    return std::make_shared<action::ActionClient>(config);
}

action::ActionServerPtr Node::create_action_server(action::ActionServerConfig config) {
    return std::make_shared<action::ActionServer>(config);
}

// Template implementations - require full mdds definitions
template<typename T>
std::shared_ptr<moss::mdds::Publisher<T>> Node::create_publisher(
    const std::string& topic_name, const mdds::QoSConfig& qos) {

    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    return participant_->create_publisher<T>(topic_name, qos);
}

template<typename T>
std::shared_ptr<moss::mdds::Subscriber<T>> Node::create_subscriber(
    const std::string& topic_name,
    typename moss::mdds::Subscriber<T>::DataCallback callback,
    const mdds::QoSConfig& qos) {

    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    return participant_->create_subscriber<T>(topic_name, std::move(callback), qos);
}

}  // namespace mcom

}  // namespace moss
