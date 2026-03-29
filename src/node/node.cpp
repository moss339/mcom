#include "mcom/node/node.h"

namespace mcom {
namespace node {

Node::Node(const std::string& name) : name_(name) {}
Node::~Node() = default;

void Node::spin() {
    // Spin implementation placeholder
}

::mcom::topic::PublisherPtr Node::create_publisher(const std::string& topic_name) {
    (void)topic_name;
    return nullptr;
}

::mcom::topic::SubscriberPtr Node::create_subscriber(const std::string& topic_name,
                                             ::mcom::topic::MessageCallback callback) {
    (void)topic_name;
    (void)callback;
    return nullptr;
}

::mcom::service::ServiceClientPtr Node::create_service_client(const std::string& service_name) {
    (void)service_name;
    return nullptr;
}

::mcom::service::ServiceServerPtr Node::create_service_server(const std::string& service_name,
                                                              ::mcom::service::RequestHandler handler) {
    (void)service_name;
    (void)handler;
    return nullptr;
}

::mcom::action::ActionClientPtr Node::create_action_client(const std::string& action_name) {
    (void)action_name;
    return nullptr;
}

::mcom::action::ActionServerPtr Node::create_action_server(const std::string& action_name,
                                                            ::mcom::action::GoalHandler handler) {
    (void)action_name;
    (void)handler;
    return nullptr;
}

} // namespace node
} // namespace mcom
