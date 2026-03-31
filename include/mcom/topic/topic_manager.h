#pragma once
#include "publisher.h"
#include "subscriber.h"
#include "mdds/domain_participant.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace moss {
namespace mcom {
namespace topic {

class TopicManager {
public:
    static TopicManager& instance();

    template<typename T>
    PublisherPtr<T> create_publisher(const std::string& topic_name);

    template<typename T>
    SubscriberPtr<T> create_subscriber(const std::string& topic_name,
                                       typename Subscriber<T>::DataCallback callback);

    std::shared_ptr<moss::mdds::DomainParticipant> get_participant();

    void set_participant(std::shared_ptr<moss::mdds::DomainParticipant> participant);

private:
    TopicManager() = default;
    std::shared_ptr<moss::mdds::DomainParticipant> participant_;
};

} // namespace topic
}  // namespace mcom
}  // namespace moss
