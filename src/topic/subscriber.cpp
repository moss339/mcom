#include "mcom/topic/subscriber.h"

namespace moss {
namespace mcom {
namespace topic {

template<typename T>
void Subscriber<T>::subscribe() {
    // Already subscribed via callback
}

template<typename T>
void Subscriber<T>::unsubscribe() {
    callback_ = nullptr;
}

template<typename T>
void Subscriber<T>::set_callback(DataCallback callback) {
    callback_ = std::move(callback);
    if (impl_) {
        impl_->set_callback(callback_);
    }
}

template<typename T>
bool Subscriber<T>::read(T& data, uint64_t* timestamp) {
    return impl_ ? impl_->read(data, timestamp) : false;
}

template<typename T>
bool Subscriber<T>::has_data() const {
    return impl_ ? impl_->has_data() : false;
}

template<typename T>
const std::string& Subscriber<T>::get_topic_name() const {
    static std::string empty;
    return impl_ ? impl_->get_topic_name() : empty;
}

} // namespace topic
}  // namespace mcom
}  // namespace moss
