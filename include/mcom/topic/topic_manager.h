#pragma once
#include "publisher.h"
#include "subscriber.h"
#include <string>
#include <memory>

namespace mcom {
namespace topic {

class TopicManager {
public:
    TopicManager() = default;
    ~TopicManager() = default;
    
    PublisherPtr create_publisher(const std::string& topic_name);
    SubscriberPtr create_subscriber(const std::string& topic_name, 
                                    MessageCallback callback);
    
private:
    // Internal state
};

} // namespace topic
} // namespace mcom
