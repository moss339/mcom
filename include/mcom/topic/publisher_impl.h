#pragma once
#include "publisher.h"
#include "mdds/publisher.h"  // Include actual mdds header

namespace moss {
namespace mcom {
namespace topic {

template<typename T>
class PublisherImpl {
public:
    explicit PublisherImpl(std::shared_ptr<moss::mdds::Publisher<T>> inner)
        : inner_(std::move(inner)) {}

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

    const std::string& get_topic_name() const {
        static std::string empty;
        return inner_ ? inner_->get_topic_name() : empty;
    }

    std::shared_ptr<moss::mdds::Publisher<T>> get_inner() const { return inner_; }

private:
    std::shared_ptr<moss::mdds::Publisher<T>> inner_;
};

}  // namespace topic
}  // namespace mcom
}  // namespace moss
