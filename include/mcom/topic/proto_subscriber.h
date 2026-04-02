#pragma once
/**
 * @file proto_subscriber.h
 * @brief Protobuf 消息订阅者
 *
 * 封装 mdds DataReader + protobuf 反序列化，对外暴露类型安全回调。
 * 用户注册 lambda/function 接收 protobuf 消息对象。
 *
 * 设计原则：
 * - mdds 只负责传输 uint8_t 字节数组
 * - protobuf 反序列化由 ProtoSubscriber 完成
 * - Topic 层完全不依赖 T::deserialize()
 */

#include <memory>
#include <string>
#include <functional>
#include <google/protobuf/message.h>
#include "mcom/types.h"
#include "mcom/topic/proto_subscriber_impl.h"

namespace moss {
namespace mcom {
namespace topic {

/**
 * @brief Protobuf 订阅者模板类
 * @tparam T 必须是一个 protobuf Message 派生类
 *
 * 用法示例：
 * @code
 * auto sub = node->create_subscriber<moss::proto::topic::Image>("/camera/image",
 *     [](const moss::proto::topic::Image& img) {
 *         fmt::print("收到图像: {}x{} frame={}\n", img.width(), img.height(), img.frame_id());
 *     });
 * sub->subscribe();
 * @endcode
 */
template<typename T>
class ProtoSubscriber : public std::enable_shared_from_this<ProtoSubscriber<T>> {
public:
    static_assert(std::is_base_of_v<google::protobuf::Message, T>,
                  "T must be a protobuf Message derived class");

    using DataType = T;

    /**
     * @brief 消息回调类型
     * @param msg 收到的 protobuf 消息（const 引用，不会拷贝）
     * @param timestamp_us 消息时间戳
     */
    using DataCallback = std::function<void(const T& msg, uint64_t timestamp_us)>;

    ProtoSubscriber() = default;

    /**
     * @brief 构造 ProtoSubscriber
     * @param impl 内部实现指针（由 TopicManager 创建）
     */
    explicit ProtoSubscriber(std::shared_ptr<ProtoSubscriberImpl<T>> impl)
        : impl_(std::move(impl)) {
        if (impl_) {
            topic_name_ = impl_->get_topic_name();
        }
    }

    ~ProtoSubscriber() = default;

    // ========== 禁止拷贝，允许移动 ==========
    ProtoSubscriber(const ProtoSubscriber&) = delete;
    ProtoSubscriber& operator=(const ProtoSubscriber&) = delete;
    ProtoSubscriber(ProtoSubscriber&&) = default;
    ProtoSubscriber& operator=(ProtoSubscriber&&) = default;

    /**
     * @brief 开始订阅
     */
    void subscribe() {
        if (impl_) {
            impl_->subscribe();
        }
    }

    /**
     * @brief 取消订阅
     */
    void unsubscribe() {
        if (impl_) {
            impl_->unsubscribe();
        }
    }

    /**
     * @brief 设置消息回调
     * @param callback 回调函数，签名为 void(const T&, uint64_t timestamp_us)
     */
    void set_callback(DataCallback callback) {
        callback_ = std::move(callback);
        if (impl_) {
            impl_->set_callback(
                [this](const uint8_t* data, size_t size, uint64_t timestamp_us) {
                    this->handle_raw_data(data, size, timestamp_us);
                });
        }
    }

    /**
     * @brief 主动读取下一条消息（非阻塞）
     * @param msg 输出参数，收到消息则填充
     * @param timestamp_us 输出参数，时间戳（可为 nullptr）
     * @return 是否有数据
     */
    bool read(T& msg, uint64_t* timestamp_us = nullptr) {
        if (!impl_) return false;
        uint64_t ts = 0;
        bool ok = impl_->read_raw(&ts);
        if (ok) {
            // 从 impl 缓存取最后一条原始数据进行反序列化
            ok = impl_->deserialize_last(msg);
            if (ok && timestamp_us) {
                *timestamp_us = ts;
            }
        }
        return ok;
    }

    /**
     * @brief 检查是否有待处理消息
     */
    bool has_data() const {
        return impl_ && impl_->has_data();
    }

    /**
     * @brief 获取 Topic 名称
     */
    const std::string& get_topic_name() const { return topic_name_; }

    /**
     * @brief 布尔转换
     */
    explicit operator bool() const { return impl_ != nullptr; }

private:
    void handle_raw_data(const uint8_t* data, size_t size, uint64_t timestamp_us) {
        if (!callback_) return;

        T msg;
        if (msg.ParseFromArray(data, static_cast<int>(size))) {
            callback_(msg, timestamp_us);
        }
        // 如果反序列化失败，记录警告（用户可通过 has_data() 检测）
    }

    std::shared_ptr<ProtoSubscriberImpl<T>> impl_;
    std::string topic_name_;
    DataCallback callback_;
};

template<typename T>
using ProtoSubscriberPtr = std::shared_ptr<ProtoSubscriber<T>>;

} // namespace topic
}  // namespace mcom
}  // namespace moss
