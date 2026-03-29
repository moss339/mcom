#pragma once
#include <string>
#include <memory>

namespace mcom {
namespace node {

class Node {
public:
    explicit Node(const std::string& name);
    ~Node();
    
    // Topic
    topic::PublisherPtr create_publisher(const std::string& topic_name);
    topic::SubscriberPtr create_subscriber(const std::string& topic_name,
                                           topic::MessageCallback callback);
    
    // Service
    service::ServiceClientPtr create_service_client(const std::string& service_name);
    service::ServiceServerPtr create_service_server(const std::string& service_name,
                                                     service::ServiceHandler handler);
    
    // Action
    action::ActionClientPtr create_action_client(const std::string& action_name);
    action::ActionServerPtr create_action_server(const std::string& action_name,
                                                 action::ActionHandler handler);
    
    const std::string& get_name() const { return name_; }
    void spin();
    
private:
    std::string name_;
};

} // namespace node
} // namespace mcom
