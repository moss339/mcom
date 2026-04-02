#pragma once
/**
 * @file proto_action_client.h
 * @brief Protobuf Action 客户端
 *
 * 基于 ProtoSubscriber/ProtoPublisher 实现动作的 Goal/Result/Feedback 机制。
 * ActionGoal/Feedback/Result 使用 protobuf 序列化。
 */

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <future>
#include <atomic>
#include <google/protobuf/message.h>
#include "mcom/topic/proto_publisher.h"
#include "mcom/topic/proto_subscriber.h"
#include "mcom/action/types.h"
#include "moss/proto/action/action.pb.h"

namespace moss {
namespace mcom {
namespace action {

inline uint64_t get_current_timestamp_us() {
    using namespace std::chrono;
    return duration_cast<microseconds>(
        steady_clock::now().time_since_epoch()).count();
}

/**
 * @brief Protobuf Action 客户端模板类
 * @tparam TGoal 用户定义的 Goal protobuf 消息类型
 *
 * 用法示例：
 * @code
 * auto action_client = node->create_action_client<moss::proto::action::NavigateToPoseGoal>("move_base");
 *
 * moss::proto::action::NavigateToPoseGoal goal;
 * goal.set_target_x(1.0);
 * goal.set_target_y(2.0);
 *
 * auto handle = action_client->send_goal(goal);
 * if (handle) {
 *     // 等待结果
 *     auto result = action_client->get_result(handle->get_goal_id());
 * }
 * @endcode
 */
template<typename TGoal>
class ProtoActionClient : public std::enable_shared_from_this<ProtoActionClient<TGoal>> {
public:
    static_assert(std::is_base_of_v<google::protobuf::Message, TGoal>,
                  "TGoal must be a protobuf Message derived class");

    using GoalType = TGoal;
    using FeedbackType = moss::proto::action::ActionFeedback;
    using ResultType = moss::proto::action::ActionResult;

    /**
     * @brief 构造 Action 客户端
     * @param action_name 动作名称
     * @param action_id 动作 ID
     */
    ProtoActionClient(const std::string& action_name, uint32_t action_id);

    ~ProtoActionClient();

    /**
     * @brief 初始化客户端（创建 publisher/subscriber）
     */
    void init();

    /**
     * @brief 发送目标
     * @param goal 目标消息
     * @param timeout 超时时间
     * @return 目标句柄
     */
    std::shared_ptr<GoalHandle> send_goal(const TGoal& goal,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

    /**
     * @brief 异步发送目标
     * @param goal 目标消息
     * @return 目标句柄 future
     */
    std::future<std::shared_ptr<GoalHandle>> send_goal_async(const TGoal& goal);

    /**
     * @brief 取消目标
     * @param goal_id 目标 ID
     * @return 是否成功
     */
    bool cancel_goal(uint32_t goal_id);

    /**
     * @brief 获取结果
     * @param goal_id 目标 ID
     * @param timeout 超时时间
     * @return 结果（如果超时返回 std::nullopt）
     */
    std::optional<ResultInfo> get_result(uint32_t goal_id,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

    /**
     * @brief 订阅反馈
     * @param handler 反馈处理函数
     */
    void subscribe_feedback(FeedbackHandler handler);

    /**
     * @brief 取消反馈订阅
     */
    void unsubscribe_feedback();

    /**
     * @brief 获取动作名称
     */
    const std::string& get_action_name() const { return action_name_; }

    /**
     * @brief 获取动作 ID
     */
    uint32_t get_action_id() const { return action_id_; }

private:
    std::string get_goal_topic() const;
    std::string get_cancel_topic() const;
    std::string get_result_topic() const;
    std::string get_feedback_topic() const;

    void on_goal_result(const moss::proto::action::ActionResult& msg);
    void on_feedback(const moss::proto::action::ActionFeedback& msg);

    std::string action_name_;
    uint32_t action_id_;
    bool initialized_{false};

    std::mutex goal_handles_mutex_;
    std::unordered_map<uint32_t, std::shared_ptr<GoalHandle>> goal_handles_;
    std::atomic<uint32_t> next_goal_id_{1};

    FeedbackHandler feedback_handler_;
    std::mutex feedback_mutex_;

    std::shared_ptr<topic::ProtoPublisher<moss::proto::action::ActionGoal>> goal_pub_;
    std::shared_ptr<topic::ProtoPublisher<moss::proto::action::ActionCancel>> cancel_pub_;
    std::shared_ptr<topic::ProtoSubscriber<moss::proto::action::ActionResult>> result_sub_;
    std::shared_ptr<topic::ProtoSubscriber<moss::proto::action::ActionFeedback>> feedback_sub_;
};

template<typename TGoal>
ProtoActionClient<TGoal>::ProtoActionClient(const std::string& action_name, uint32_t action_id)
    : action_name_(action_name), action_id_(action_id) {
}

template<typename TGoal>
ProtoActionClient<TGoal>::~ProtoActionClient() {
    unsubscribe_feedback();
    result_sub_.reset();
    feedback_sub_.reset();
    goal_pub_.reset();
    cancel_pub_.reset();
}

template<typename TGoal>
std::string ProtoActionClient<TGoal>::get_goal_topic() const {
    return "action_" + action_name_ + "_" + std::to_string(action_id_) + "_goal";
}

template<typename TGoal>
std::string ProtoActionClient<TGoal>::get_cancel_topic() const {
    return "action_" + action_name_ + "_" + std::to_string(action_id_) + "_cancel";
}

template<typename TGoal>
std::string ProtoActionClient<TGoal>::get_result_topic() const {
    return "action_" + action_name_ + "_" + std::to_string(action_id_) + "_result";
}

template<typename TGoal>
std::string ProtoActionClient<TGoal>::get_feedback_topic() const {
    return "action_" + action_name_ + "_" + std::to_string(action_id_) + "_feedback";
}

template<typename TGoal>
void ProtoActionClient<TGoal>::init() {
    if (initialized_) {
        return;
    }

    auto& topic_mgr = topic::TopicManager::instance();

    goal_pub_ = topic_mgr.create_proto_publisher<moss::proto::action::ActionGoal>(get_goal_topic());
    cancel_pub_ = topic_mgr.create_proto_publisher<moss::proto::action::ActionCancel>(get_cancel_topic());

    result_sub_ = topic_mgr.create_proto_subscriber<moss::proto::action::ActionResult>(
        get_result_topic(),
        [this](const moss::proto::action::ActionResult& msg, uint64_t) {
            this->on_goal_result(msg);
        });

    feedback_sub_ = topic_mgr.create_proto_subscriber<moss::proto::action::ActionFeedback>(
        get_feedback_topic(),
        [this](const moss::proto::action::ActionFeedback& msg, uint64_t) {
            this->on_feedback(msg);
        });

    initialized_ = true;
}

template<typename TGoal>
std::shared_ptr<GoalHandle> ProtoActionClient<TGoal>::send_goal(
    const TGoal& goal,
    std::chrono::milliseconds timeout) {
    (void)timeout;

    if (!initialized_) {
        init();
    }

    uint32_t goal_id = next_goal_id_++;

    auto goal_handle = std::make_shared<GoalHandle>(goal_id);
    {
        std::lock_guard<std::mutex> lock(goal_handles_mutex_);
        goal_handles_[goal_id] = goal_handle;
    }

    moss::proto::action::ActionGoal goal_msg;
    goal_msg.set_goal_id(goal_id);
    goal_msg.set_timestamp_us(get_current_timestamp_us());
    goal_msg.set_action_name(action_name_);
    goal_msg.set_action_id(action_id_);

    if (!goal.SerializeToString(goal_msg.mutable_goal_payload())) {
        goal_handle->set_status(ActionStatus::ABORTED);
        return goal_handle;
    }

    goal_pub_->publish(goal_msg);
    goal_handle->set_status(ActionStatus::EXECUTING);
    return goal_handle;
}

template<typename TGoal>
std::future<std::shared_ptr<GoalHandle>> ProtoActionClient<TGoal>::send_goal_async(
    const TGoal& goal) {
    return std::async(std::launch::async, [this, &goal]() {
        return send_goal(goal);
    });
}

template<typename TGoal>
bool ProtoActionClient<TGoal>::cancel_goal(uint32_t goal_id) {
    if (!initialized_) {
        return false;
    }

    moss::proto::action::ActionCancel cancel_msg;
    cancel_msg.set_goal_id(goal_id);
    cancel_msg.set_timestamp_us(get_current_timestamp_us());

    cancel_pub_->publish(cancel_msg);

    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it != goal_handles_.end()) {
        it->second->request_cancel();
    }

    return true;
}

template<typename TGoal>
std::optional<ResultInfo> ProtoActionClient<TGoal>::get_result(
    uint32_t goal_id,
    std::chrono::milliseconds timeout) {
    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it == goal_handles_.end()) {
        return std::nullopt;
    }

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < timeout) {
        if (it->second->has_result()) {
            return it->second->get_result();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return std::nullopt;
}

template<typename TGoal>
void ProtoActionClient<TGoal>::subscribe_feedback(FeedbackHandler handler) {
    std::lock_guard<std::mutex> lock(feedback_mutex_);
    feedback_handler_ = handler;
}

template<typename TGoal>
void ProtoActionClient<TGoal>::unsubscribe_feedback() {
    std::lock_guard<std::mutex> lock(feedback_mutex_);
    feedback_handler_ = nullptr;
}

template<typename TGoal>
void ProtoActionClient<TGoal>::on_goal_result(
    const moss::proto::action::ActionResult& msg) {

    uint32_t goal_id = msg.goal_id();
    ActionStatus status = ActionStatus::UNKNOWN;

    switch (msg.terminal_status()) {
        case moss::proto::action::SUCCEEDED:
            status = ActionStatus::SUCCEEDED;
            break;
        case moss::proto::action::CANCELED:
            status = ActionStatus::CANCELED;
            break;
        case moss::proto::action::REJECTED:
            status = ActionStatus::REJECTED;
            break;
        case moss::proto::action::ABORTED:
            status = ActionStatus::ABORTED;
            break;
        case moss::proto::action::PREEMPTED:
            status = ActionStatus::PREEMPTED;
            break;
        default:
            status = ActionStatus::UNKNOWN;
            break;
    }

    std::vector<uint8_t> result_data(msg.result_payload().begin(),
                                      msg.result_payload().end());

    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it != goal_handles_.end()) {
        it->second->set_result(status, result_data);
    }
}

template<typename TGoal>
void ProtoActionClient<TGoal>::on_feedback(
    const moss::proto::action::ActionFeedback& msg) {

    FeedbackInfo info;
    info.goal_id = msg.goal_id();

    switch (msg.status()) {
        case moss::proto::action::UNKNOWN:
            info.status = ActionStatus::UNKNOWN;
            break;
        case moss::proto::action::SUCCEEDED:
            info.status = ActionStatus::SUCCEEDED;
            break;
        case moss::proto::action::CANCELED:
            info.status = ActionStatus::CANCELED;
            break;
        case moss::proto::action::REJECTED:
            info.status = ActionStatus::REJECTED;
            break;
        case moss::proto::action::ABORTED:
            info.status = ActionStatus::ABORTED;
            break;
        case moss::proto::action::PREEMPTED:
            info.status = ActionStatus::PREEMPTED;
            break;
        default:
            info.status = ActionStatus::UNKNOWN;
            break;
    }

    info.feedback_data = std::vector<uint8_t>(msg.feedback_payload().begin(),
                                               msg.feedback_payload().end());

    std::lock_guard<std::mutex> lock(feedback_mutex_);
    if (feedback_handler_) {
        feedback_handler_(info);
    }
}

template<typename TGoal>
using ProtoActionClientPtr = std::shared_ptr<ProtoActionClient<TGoal>>;

}  // namespace action
}  // namespace mcom
}  // namespace moss
