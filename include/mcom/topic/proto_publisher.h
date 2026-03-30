#pragma once
/**
 * @file proto_publisher.h
 * @brief Protobuf 消息发布者
 *
 * 封装 mdds DataWriter + protobuf 序列化，对外暴露类型安全 API。
 * 用户直接操作 protobuf 消息对象，无需关心底层传输细节。
 *
 * 设计原则：
 * - mdds 只负责传输 uint8_t 字节数组
 * - protobuf 序列化由 ProtoPublisher 完成
 * - Topic 层完全不依赖 T::deserialize()
 */

#include <memory>
#include <string>
#include <google/protobuf/message.h>
#include "mcom/types.h"

namespace mcom {
namespace topic {

/**
 * @brief Protobuf 发布者模板类
 * @tparam T 必须是一个 protobuf Message 派生类
 *
 * 用法示例：
 * @code
 * auto pub = node->create_publisher<moss::proto::topic::Image>("/camera/image");
 * moss::proto::topic::Image img;
 * img.set_timestamp_us(1234567890);
 * img.set_width(1920);
 * img.set_height(1080);
 * img.set_data(image_buffer);
 * pub->publish(img);
 * @endcode
 */
template<typename T>
class ProtoPublisher : public std::enable_shared_from_this<ProtoPublisher<T>> {
public:
    static_assert(std::is_base_of_v<google::protobuf::Message, T>,
                  "T must be a protobuf Message derived class");

    using DataType = T;

    ProtoPublisher() = default;

    /**
     * @brief 构造 ProtoPublisher
     * @param impl 内部实现指针（由 TopicManager 创建）
     */
    explicit ProtoPublisher(std::shared_ptr<class ProtoPublisherImpl<T>> impl)
        : impl_(std::move(impl)) {
        if (impl_) {
            topic_name_ = impl_->get_topic_name();
        }
    }

    ~ProtoPublisher() = default;

    // ========== 禁止拷贝，允许移动 ==========
    ProtoPublisher(const ProtoPublisher&) = delete;
    ProtoPublisher& operator=(const ProtoPublisher&) = delete;
    ProtoPublisher(ProtoPublisher&&) = default;
    ProtoPublisher& operator=(ProtoPublisher&&) = default;

    /**
     * @brief 发布 protobuf 消息
     * @param msg 消息对象（protobuf Message 派生类）
     */
    void publish(const T& msg) {
        publish(msg, get_current_timestamp_us());
    }

    /**
     * @brief 发布带时间戳的 protobuf 消息
     * @param msg 消息对象
     * @param timestamp_us 微秒级时间戳
     */
    void publish(const T& msg, uint64_t timestamp_us) {
        if (impl_) {
            impl_->publish(msg, timestamp_us);
        }
    }

    /**
     * @brief 发布原始字节（供内部使用）
     * @param data 字节数据
     * @param size 数据大小
     * @param timestamp_us 时间戳
     */
    void publish_raw(const uint8_t* data, size_t size, uint64_t timestamp_us) {
        if (impl_) {
            impl_->publish_raw(data, size, timestamp_us);
        }
    }

    /**
     * @brief 获取 Topic 名称
     */
    const std::string& get_topic_name() const { return topic_name_; }

    /**
     * @brief 布尔转换（检查是否有效）
     */
    explicit operator bool() const { return impl_ != nullptr; }

private:
    std::shared_ptr<class ProtoPublisherImpl<T>> impl_;
    std::string topic_name_;

    static uint64_t get_current_timestamp_us();
};

template<typename T>
uint64_t ProtoPublisher<T>::get_current_timestamp_us() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count();
}

template<typename T>
using ProtoPublisherPtr = std::shared_ptr<ProtoPublisher<T>>;

} // namespace topic
} // namespace mcom
