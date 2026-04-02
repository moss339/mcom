#include "mcom/topic/topic_manager.h"
#include "mcom/topic/proto_publisher_impl.h"
#include "mcom/topic/proto_subscriber_impl.h"
#include "mdds/domain_participant.h"

namespace moss {
namespace mcom {
namespace topic {

TopicManager& TopicManager::instance() {
    static TopicManager instance_;
    return instance_;
}

template<typename T>
PublisherPtr<T> TopicManager::create_publisher(const std::string& topic_name) {
    auto participant = get_participant();
    if (!participant) {
        return nullptr;
    }
    auto mdds_pub = participant->create_publisher<T>(topic_name);
    if (!mdds_pub) {
        return nullptr;
    }
    auto impl = std::make_shared<PublisherImpl<T>>(mdds_pub);
    return std::make_shared<Publisher<T>>(impl);
}

template<typename T>
SubscriberPtr<T> TopicManager::create_subscriber(
    const std::string& topic_name,
    typename Subscriber<T>::DataCallback callback) {
    auto participant = get_participant();
    if (!participant) {
        return nullptr;
    }
    auto mdds_sub = participant->create_subscriber<T>(topic_name, callback);
    if (!mdds_sub) {
        return nullptr;
    }
    auto impl = std::make_shared<SubscriberImpl<T>>(mdds_sub);
    auto sub = std::make_shared<Subscriber<T>>(impl);
    sub->set_callback(std::move(callback));
    return sub;
}

template<typename T>
ProtoPublisherPtr<T> TopicManager::create_proto_publisher(const std::string& topic_name) {
    auto participant = get_participant();
    if (!participant) {
        return nullptr;
    }
    auto writer_raw = participant->create_writer_raw(topic_name);
    if (!writer_raw) {
        return nullptr;
    }
    auto impl = std::make_shared<ProtoPublisherImpl<T>>(writer_raw, topic_name);
    return std::make_shared<ProtoPublisher<T>>(impl);
}

template<typename T>
ProtoSubscriberPtr<T> TopicManager::create_proto_subscriber(
    const std::string& topic_name,
    typename ProtoSubscriber<T>::DataCallback callback) {
    auto participant = get_participant();
    if (!participant) {
        return nullptr;
    }
    auto reader_raw = participant->create_reader_raw(topic_name);
    if (!reader_raw) {
        return nullptr;
    }
    auto impl = std::make_shared<ProtoSubscriberImpl<T>>(reader_raw, topic_name);
    auto sub = std::make_shared<ProtoSubscriber<T>>(impl);
    sub->set_callback(std::move(callback));
    sub->subscribe();
    return sub;
}

std::shared_ptr<moss::mdds::DomainParticipant> TopicManager::get_participant() {
    if (!participant_) {
        participant_ = moss::mdds::DomainParticipant::create(0);
        if (participant_) {
            participant_->start();
        }
    }
    return participant_;
}

void TopicManager::set_participant(std::shared_ptr<moss::mdds::DomainParticipant> participant) {
    participant_ = std::move(participant);
}

} // namespace topic
}  // namespace mcom
}  // namespace moss
