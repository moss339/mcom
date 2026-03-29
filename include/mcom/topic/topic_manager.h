#pragma once
#include "publisher.h"
#include "subscriber.h"
#include <string>
#include <memory>
#include <unordered_map>
#include "mdds/domain_participant.h"

namespace mcom {
namespace topic {

class TopicManager {
public:
    static TopicManager& instance();

    template<typename T>
    PublisherPtr<T> create_publisher(const std::string& topic_name) {
        auto participant = get_participant();
        if (!participant) {
            return nullptr;
        }
        auto mdds_pub = participant->create_publisher<T>(topic_name);
        return std::make_shared<Publisher<T>>(mdds_pub);
    }

    template<typename T>
    SubscriberPtr<T> create_subscriber(const std::string& topic_name,
                                       typename Subscriber<T>::DataCallback callback) {
        auto participant = get_participant();
        if (!participant) {
            return nullptr;
        }
        auto mdds_sub = participant->create_subscriber<T>(topic_name, callback);
        return std::make_shared<Subscriber<T>>(mdds_sub);
    }

    std::shared_ptr<mdds::DomainParticipant> get_participant() {
        if (!participant_) {
            participant_ = mdds::DomainParticipant::create(0);
            participant_->start();
        }
        return participant_;
    }

    void set_participant(std::shared_ptr<mdds::DomainParticipant> participant) {
        participant_ = participant;
    }

private:
    TopicManager() = default;
    std::shared_ptr<mdds::DomainParticipant> participant_;
};

} // namespace topic
} // namespace mcom
