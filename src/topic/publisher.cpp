#include "mcom/topic/publisher.h"

namespace moss {
namespace mcom {
namespace topic {

template<typename T>
void Publisher<T>::publish(const T& data) {
    if (impl_) {
        impl_->publish(data);
    }
}

template<typename T>
void Publisher<T>::publish(const T& data, uint64_t timestamp) {
    if (impl_) {
        impl_->publish(data, timestamp);
    }
}

template<typename T>
const std::string& Publisher<T>::get_topic_name() const {
    static std::string empty;
    return impl_ ? impl_->get_topic_name() : empty;
}

} // namespace topic
}  // namespace mcom
}  // namespace moss
