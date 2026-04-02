#pragma once
/**
 * @file proto_publisher_impl.h
 * @brief ProtoPublisher 内部实现
 *
 * 负责：
 * 1. protobuf Message → 字节序列化
 * 2. 调用 mdds DataWriter 传输字节
 */

#include <memory>
#include <string>
#include <cstring>
#include <cstdint>
#include <functional>
#include <google/protobuf/message.h>
#include <mdds/data_writer_raw.h>

namespace moss {
namespace mcom {
namespace topic {

namespace internal {

/**
 * @brief 序列化辅助函数（ADL 可扩展）
 *
 * 默认实现调用 protobuf 原生 SerializeToArray。
 * 如需自定义序列化，可在此命名空间添加特化版本。
 */
inline bool serialize_to_array(const google::protobuf::Message& msg,
                                uint8_t* buffer,
                                int buffer_size,
                                int* bytes_written) {
    if (!msg.SerializeToArray(buffer, buffer_size)) {
        *bytes_written = 0;
        return false;
    }
    *bytes_written = msg.ByteSizeLong();
    return true;
}

} // namespace internal

/**
 * @brief ProtoPublisher 内部实现类
 */
template<typename T>
class ProtoPublisherImpl {
public:
    using RawCallback = std::function<void(const uint8_t*, size_t, uint64_t)>;

    /**
     * @brief 构造（绑定 mdds DataWriter）
     */
    explicit ProtoPublisherImpl(std::shared_ptr<moss::mdds::DataWriterRaw> writer,
                                const std::string& topic_name)
        : writer_(std::move(writer))
        , topic_name_(topic_name) {}

    void publish(const T& msg, uint64_t timestamp_us) {
        if (!writer_) return;

        // 计算序列化后大小
        const int size = msg.ByteSizeLong();
        if (size <= 0 || size > moss::mdds::MDDS_MAX_PAYLOAD_SIZE) {
            return;
        }

        // 序列化到栈上 buffer（或使用 arena）
        uint8_t buffer[moss::mdds::MDDS_MAX_PAYLOAD_SIZE];
        int bytes_written = 0;
        if (!internal::serialize_to_array(msg, buffer, size, &bytes_written)) {
            return;
        }

        publish_raw(buffer, bytes_written, timestamp_us);
    }

    void publish_raw(const uint8_t* data, size_t size, uint64_t timestamp_us) {
        if (writer_) {
            writer_->write_raw(data, size, timestamp_us);
        }
    }

    const std::string& get_topic_name() const { return topic_name_; }
    std::shared_ptr<moss::mdds::DataWriterRaw> get_writer() const { return writer_; }

private:
    std::shared_ptr<moss::mdds::DataWriterRaw> writer_;
    std::string topic_name_;
};

} // namespace topic
}  // namespace mcom
}  // namespace moss
