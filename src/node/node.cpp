#include "mcom/node/node.h"

namespace mcom {
namespace node {

Node::Node(const std::string& name) : name_(name) {}
Node::~Node() = default;

void Node::spin() {
    // Spin implementation
}

topic::PublisherPtr Node::create_publisher(const std::string& topic_name) {
    return nullptr;
}

topic::SubscriberPtr Node::create_subscriber(const std::string& topic_name,
                                             topic::MessageCallback callback) {
    return nullptr;
}

service::ServiceClientPtr Node::create_service_client(const std::string& service_name) {
    return nullptr;
}

service::ServiceServerPtr Node::create_service_server(const std::string& service_name,
                                                       service::ServiceHandler handler) {
    return nullptr;
}

action::ActionClientPtr Node::create_action_client(const std::string& action_name) {
    return nullptr;
}

action::ActionServerPtr Node::create_action_server(const std::string& action_name,
                                                  action::ActionHandler handler) {
    return nullptr;
}

} // namespace node
} // namespace mcom
