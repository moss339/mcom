#pragma once
/**
 * @file proto_subscriber_impl.h
 * @brief ProtoSubscriber 内部实现
 *
 * 负责：
 * 1. 调用 mdds DataReader 接收字节
 * 2. 提供原始字节回调（由 ProtoSubscriber 执行 protobuf 反序列化）
 */

#include "mcom/topic/proto_subscriber.h"
#include <memory>
#include <string>
#include <deque>
#include <mutex>
#include <vector>
#include <functional>
#include <google/protobuf/message.h>

namespace mdds {
class DataReaderRaw;
class Transport;
}
class QoSConfig;

namespace moss {
namespace mcom {
namespace topic {

/**
 * @brief ProtoSubscriber 内部实现类
 */
template<typename T>
class ProtoSubscriberImpl {
public:
    /**
     * @brief 原始字节回调类型
     */
    using RawCallback = std::function<void(const uint8_t* data, size_t size, uint64_t timestamp_us)>;

    explicit ProtoSubscriberImpl(std::shared_ptr<moss::mdds::DataReaderRaw> reader,
                                 const std::string& topic_name)
        : reader_(std::move(reader))
        , topic_name_(topic_name) {}

    void subscribe() {
        if (reader_) {
            reader_->set_callback(
                [this](const uint8_t* data, size_t size, uint64_t timestamp) {
                    this->on_raw_data(data, size, timestamp);
                });
        }
    }

    void unsubscribe() {
        if (reader_) {
            reader_->clear_callback();
        }
    }

    void set_callback(RawCallback cb) {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = std::move(cb);
    }

    bool read_raw(uint64_t* timestamp_us) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_.empty()) {
            return false;
        }
        if (timestamp_us) {
            *timestamp_us = pending_.front().timestamp_us;
        }
        return true;
    }

    /**
     * @brief 从缓存中反序列化最后一条消息
     */
    bool deserialize_last(T& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_.empty()) {
            return false;
        }
        const auto& item = pending_.front();
        if (!msg.ParseFromArray(item.data.data(), static_cast<int>(item.data.size()))) {
            pending_.pop_front();
            return false;
        }
        // 消费掉
        pending_.pop_front();
        return true;
    }

    bool has_data() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return !pending_.empty();
    }

    const std::string& get_topic_name() const { return topic_name_; }

private:
    struct PendingItem {
        std::vector<uint8_t> data;
        uint64_t timestamp_us;
    };

    void on_raw_data(const uint8_t* data, size_t size, uint64_t timestamp_us) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 缓存原始数据（供后续 read() 调用）
        PendingItem item;
        item.data.assign(data, data + size);
        item.timestamp_us = timestamp_us;
        pending_.push_back(std::move(item));

        // 限制队列大小（防止内存溢出）
        constexpr size_t MAX_PENDING = 64;
        while (pending_.size() > MAX_PENDING) {
            pending_.pop_front();
        }

        // 触发回调
        if (callback_) {
            callback_(data, size, timestamp_us);
        }
    }

    std::shared_ptr<moss::mdds::DataReaderRaw> reader_;
    std::string topic_name_;

    mutable std::mutex mutex_;
    RawCallback callback_;
    std::deque<PendingItem> pending_;  // 原始数据缓存
};

} // namespace topic
}  // namespace mcom
}  // namespace moss
