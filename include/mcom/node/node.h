#pragma once
#include <string>
#include <memory>
#include "mcom/topic/publisher.h"
#include "mcom/topic/subscriber.h"
#include "mcom/service/service_client.h"
#include "mcom/service/service_server.h"
#include "mcom/service/types.h"
#include "mcom/action/action_client.h"
#include "mcom/action/action_server.h"
#include "mcom/action/types.h"

namespace mcom {
namespace node {

class Node {
public:
    explicit Node(const std::string& name);
    ~Node();
    
    // Topic
    ::mcom::topic::PublisherPtr create_publisher(const std::string& topic_name);
    ::mcom::topic::SubscriberPtr create_subscriber(const std::string& topic_name,
                                           ::mcom::topic::MessageCallback callback);
    
    // Service
    ::mcom::service::ServiceClientPtr create_service_client(const std::string& service_name);
    ::mcom::service::ServiceServerPtr create_service_server(const std::string& service_name,
                                                     ::mcom::service::RequestHandler handler);
    
    // Action
    ::mcom::action::ActionClientPtr create_action_client(const std::string& action_name);
    ::mcom::action::ActionServerPtr create_action_server(const std::string& action_name,
                                                  ::mcom::action::GoalHandler handler);
    
    const std::string& get_name() const { return name_; }
    void spin();
    
private:
    std::string name_;
};

} // namespace node
} // namespace mcom
