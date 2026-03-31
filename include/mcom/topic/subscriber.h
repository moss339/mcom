#pragma once
#include <string>
#include <memory>
#include <functional>
#include "mcom/types.h"
#include "mcom/topic/subscriber_impl.h"

namespace moss {
namespace mcom {
namespace topic {

template<typename T>
class Subscriber : public std::enable_shared_from_this<Subscriber<T>> {
public:
    using DataType = T;
    using DataCallback = std::function<void(const T& data, uint64_t timestamp)>;

    Subscriber() = default;

    explicit Subscriber(std::shared_ptr<SubscriberImpl<T>> impl)
        : impl_(std::move(impl)) {
        if (impl_) {
            topic_name_ = impl_->get_topic_name();
        }
    }

    ~Subscriber() = default;

    void subscribe();
    void unsubscribe();

    void set_callback(DataCallback callback);

    bool read(T& data, uint64_t* timestamp = nullptr);
    bool has_data() const;

    const std::string& get_topic_name() const;

    explicit operator bool() const { return impl_ != nullptr; }

private:
    std::shared_ptr<SubscriberImpl<T>> impl_;
    std::string topic_name_;
    DataCallback callback_;
};

template<typename T>
using SubscriberPtr = std::shared_ptr<Subscriber<T>>;

} // namespace topic
}  // namespace mcom
}  // namespace moss
