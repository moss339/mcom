#pragma once
#include <string>
#include <memory>
#include "mcom/types.h"
#include "mcom/topic/publisher_impl.h"

namespace moss {
namespace mcom {
namespace topic {

template<typename T>
class Publisher : public std::enable_shared_from_this<Publisher<T>> {
public:
    using DataType = T;

    Publisher() = default;

    explicit Publisher(std::shared_ptr<PublisherImpl<T>> impl)
        : impl_(std::move(impl)) {
        if (impl_) {
            topic_name_ = impl_->get_topic_name();
        }
    }

    ~Publisher() = default;

    void publish(const T& data);
    void publish(const T& data, uint64_t timestamp);

    const std::string& get_topic_name() const;

    explicit operator bool() const { return impl_ != nullptr; }

private:
    std::shared_ptr<PublisherImpl<T>> impl_;
    std::string topic_name_;
};

template<typename T>
using PublisherPtr = std::shared_ptr<Publisher<T>>;

} // namespace topic
}  // namespace mcom
}  // namespace moss
