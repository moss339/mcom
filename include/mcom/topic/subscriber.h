#pragma once
#include <string>
#include <memory>
#include <functional>
#include "mcom/types.h"
#include "mdds/subscriber.h"

namespace mcom {
namespace topic {

template<typename T>
class Subscriber : public std::enable_shared_from_this<Subscriber<T>> {
public:
    using DataType = T;
    using DataCallback = std::function<void(const T& data, uint64_t timestamp)>;

    Subscriber() = default;

    Subscriber(std::shared_ptr<mdds::Subscriber<T>> inner)
        : inner_(std::move(inner)) {
        if (inner_) {
            topic_name_ = inner_->get_topic_name();
        }
    }

    ~Subscriber() = default;

    void subscribe() {
    }

    void unsubscribe() {
    }

    void set_callback(DataCallback callback) {
        callback_ = std::move(callback);
        if (inner_) {
            inner_->set_callback(
                [this](const T& data, uint64_t timestamp) {
                    if (callback_) {
                        callback_(data, timestamp);
                    }
                });
        }
    }

    bool read(T& data, uint64_t* timestamp = nullptr) {
        return inner_ ? inner_->read(data, timestamp) : false;
    }

    bool has_data() const {
        return inner_ ? inner_->has_data() : false;
    }

    const std::string& get_topic_name() const { return topic_name_; }

    std::shared_ptr<mdds::Subscriber<T>> get_inner() const { return inner_; }

private:
    std::shared_ptr<mdds::Subscriber<T>> inner_;
    std::string topic_name_;
    DataCallback callback_;
};

template<typename T>
using SubscriberPtr = std::shared_ptr<Subscriber<T>>;

} // namespace topic
} // namespace mcom
