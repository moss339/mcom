#include "mcom/node/node.h"
#include <mcom/service/service_client.h>
#include <mcom/service/service_server.h>
#include <mcom/service/proto_service_client.h>
#include <mcom/service/proto_service_server.h>
#include <mcom/action/action_client.h>
#include <mcom/action/action_server.h>
#include <mcom/topic/topic_manager.h>

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

    runtime_node_ = mruntime::Node::create(config_.node_name, config_.domain_id);
    if (!runtime_node_ || !runtime_node_->init()) {
        runtime_node_.reset();
        return false;
    }

    topic::TopicManager::instance().set_participant(runtime_node_->get_participant());

    transition_state(NodeState::INITIALIZED);
    return true;
}

bool Node::start() {
    std::lock_guard<std::mutex> lock(state_mutex_);

    if (state_ != NodeState::INITIALIZED) {
        return false;
    }

    if (!runtime_node_->start()) {
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

    runtime_node_->stop();
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

    runtime_node_->destroy();
    runtime_node_.reset();
    topic::TopicManager::instance().set_participant(nullptr);
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
    auto client = std::make_shared<service::ServiceClient>(service_id, instance_id);
    client->init();
    return client;
}

service::ServiceServerPtr Node::create_service_server(
    service::ServiceId service_id, service::InstanceId instance_id) {
    auto server = std::make_shared<service::ServiceServer>(service_id, instance_id);
    server->init();
    server->offer();
    return server;
}

service::ProtoServiceClientPtr Node::create_proto_service_client(
    const std::string& service_name, uint32_t service_id) {
    auto client = std::make_shared<service::ProtoServiceClient>(service_name, service_id);
    client->init();
    return client;
}

service::ProtoServiceServerPtr Node::create_proto_service_server(
    const std::string& service_name, uint32_t service_id) {
    auto server = std::make_shared<service::ProtoServiceServer>(service_name, service_id);
    server->offer();
    return server;
}

action::ActionClientPtr Node::create_action_client(action::ActionClientConfig config) {
    return std::make_shared<action::ActionClient>(config);
}

action::ActionServerPtr Node::create_action_server(action::ActionServerConfig config) {
    return std::make_shared<action::ActionServer>(config);
}

template<typename TGoal>
action::ProtoActionClientPtr<TGoal> Node::create_action_client(
    const std::string& action_name,
    uint32_t action_id) {
    auto client = std::make_shared<action::ProtoActionClient<TGoal>>(action_name, action_id);
    client->init();
    return client;
}

template<typename TGoal, typename TResult>
action::ProtoActionServerPtr<TGoal, TResult> Node::create_action_server(
    const std::string& action_name,
    uint32_t action_id) {
    auto server = std::make_shared<action::ProtoActionServer<TGoal, TResult>>(action_name, action_id);
    server->init();
    return server;
}

template<typename T>
std::shared_ptr<moss::mdds::Publisher<T>> Node::create_publisher(
    const std::string& topic_name, const mdds::QoSConfig& qos) {

    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    auto participant = runtime_node_->get_participant();
    if (!participant) {
        return nullptr;
    }
    return participant->create_publisher<T>(topic_name, qos);
}

template<typename T>
std::shared_ptr<moss::mdds::Subscriber<T>> Node::create_subscriber(
    const std::string& topic_name,
    typename moss::mdds::Subscriber<T>::DataCallback callback,
    const mdds::QoSConfig& qos) {

    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    auto participant = runtime_node_->get_participant();
    if (!participant) {
        return nullptr;
    }
    return participant->create_subscriber<T>(topic_name, std::move(callback), qos);
}

template<typename T>
topic::ProtoPublisherPtr<T> Node::create_publisher(const std::string& topic_name) {
    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    return topic::TopicManager::instance().create_proto_publisher<T>(topic_name);
}

template<typename T>
topic::ProtoSubscriberPtr<T> Node::create_subscriber(
    const std::string& topic_name,
    typename topic::ProtoSubscriber<T>::DataCallback callback) {
    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    return topic::TopicManager::instance().create_proto_subscriber<T>(topic_name, std::move(callback));
}

template<typename T>
service::ProtoServiceClientPtr Node::create_service_client(
    const std::string& service_name) {
    std::lock_guard<std::mutex> lock(endpoints_mutex_);
    auto client = std::make_shared<service::ProtoServiceClient>(service_name, 0);
    client->init();
    return client;
}

void Node::spin() {
    while (is_running()) {
        spin_once();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Node::spin_once() {
    if (!is_running()) {
        return;
    }
}

}  // namespace mcom
}  // namespace moss
