#pragma once
#include <string>
#include <functional>
#include <memory>

namespace mcom {
namespace topic {

using MessageCallback = std::function<void(const void* data, size_t size)>;

class Subscriber {
public:
    Subscriber() = default;
    virtual ~Subscriber() = default;
    
    virtual void subscribe() = 0;
    virtual void unsubscribe() = 0;
    
    const std::string& get_topic_name() const { return topic_name_; }
    
protected:
    std::string topic_name_;
};

using SubscriberPtr = std::shared_ptr<Subscriber>;

} // namespace topic
} // namespace mcom
