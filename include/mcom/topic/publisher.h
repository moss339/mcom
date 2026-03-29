#pragma once
#include <string>
#include <vector>
#include <memory>

namespace mcom {
namespace topic {

class Publisher {
public:
    Publisher() = default;
    virtual ~Publisher() = default;
    
    virtual void publish(const void* data, size_t size) = 0;
    virtual void publish(const void* data, size_t size, uint64_t timestamp) = 0;
    
    const std::string& get_topic_name() const { return topic_name_; }
    
protected:
    std::string topic_name_;
};

using PublisherPtr = std::shared_ptr<Publisher>;

} // namespace topic
} // namespace mcom
