#pragma once
#include <string>
#include <memory>
#include "mcom/types.h"
#include "mdds/publisher.h"

namespace mcom {
namespace topic {

template<typename T>
class Publisher : public std::enable_shared_from_this<Publisher<T>> {
public:
    using DataType = T;

    Publisher() = default;

    Publisher(std::shared_ptr<mdds::Publisher<T>> inner)
        : inner_(std::move(inner)) {
        if (inner_) {
            topic_name_ = inner_->get_topic_name();
        }
    }

    ~Publisher() = default;

    void publish(const T& data) {
        if (inner_) {
            inner_->write(data);
        }
    }

    void publish(const T& data, uint64_t timestamp) {
        if (inner_) {
            inner_->write(data, timestamp);
        }
    }

    const std::string& get_topic_name() const { return topic_name_; }

    std::shared_ptr<mdds::Publisher<T>> get_inner() const { return inner_; }

private:
    std::shared_ptr<mdds::Publisher<T>> inner_;
    std::string topic_name_;
};

template<typename T>
using PublisherPtr = std::shared_ptr<Publisher<T>>;

} // namespace topic
} // namespace mcom
