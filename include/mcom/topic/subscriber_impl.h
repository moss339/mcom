#pragma once
#include "subscriber.h"
#include "mdds/subscriber.h"  // Include actual mdds header

namespace moss {
namespace mcom {
namespace topic {

template<typename T>
class SubscriberImpl {
public:
    using DataCallback = std::function<void(const T&, uint64_t)>;

    explicit SubscriberImpl(std::shared_ptr<moss::mdds::Subscriber<T>> inner)
        : inner_(std::move(inner)) {}

    void set_callback(DataCallback callback) {
        callback_ = std::move(callback);
        if (inner_) {
            inner_->set_callback(
                [this](const void* data, size_t size, uint64_t timestamp) {
                    if (callback_) {
                        T obj = T::deserialize(static_cast<const uint8_t*>(data), size);
                        callback_(obj, timestamp);
                    }
                });
        }
    }

    bool read(T& data, uint64_t* timestamp = nullptr) {
        if (!inner_) return false;
        return inner_->read(data, timestamp);
    }

    bool has_data() const {
        return inner_ ? inner_->has_data() : false;
    }

    const std::string& get_topic_name() const {
        static std::string empty;
        return inner_ ? inner_->get_topic_name() : empty;
    }

    std::shared_ptr<moss::mdds::Subscriber<T>> get_inner() const { return inner_; }

private:
    std::shared_ptr<moss::mdds::Subscriber<T>> inner_;
    DataCallback callback_;
};

}  // namespace topic
}  // namespace mcom
}  // namespace moss
