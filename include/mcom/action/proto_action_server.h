#pragma once
/**
 * @file proto_action_server.h
 * @brief Protobuf Action 服务端
 *
 * 基于 ProtoSubscriber/ProtoPublisher 实现动作的 Goal/Result/Feedback 机制。
 * ActionGoal/Feedback/Result 使用 protobuf 序列化。
 */

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <atomic>
#include <google/protobuf/message.h>
#include "mcom/topic/proto_publisher.h"
#include "mcom/topic/proto_subscriber.h"
#include "mcom/action/types.h"
#include "mcom/action/goal_handle.h"
#include "moss/proto/action/action.pb.h"

namespace moss {
namespace mcom {
namespace action {

/**
 * @brief Protobuf Action 服务端模板类
 * @tparam TGoal 用户定义的 Goal protobuf 消息类型
 * @tparam TResult 用户定义的 Result protobuf 消息类型
 *
 * 用法示例：
 * @code
 * auto action_server = node->create_action_server<moss::proto::action::NavigateToPoseGoal,
 *                                                   moss::proto::action::NavigateToPoseResult>("move_base");
 *
 * action_server->register_goal_handler(
 *     [](const GoalInfo& info, moss::proto::action::NavigateToPoseGoal& goal) {
 *         // 处理目标
 *         return std::make_tuple(true, moss::proto::action::NavigateToPoseResult());
 *     });
 *
 * action_server->start();
 * @endcode
 */
template<typename TGoal, typename TResult>
class ProtoActionServer : public std::enable_shared_from_this<ProtoActionServer<TGoal, TResult>> {
public:
    static_assert(std::is_base_of_v<google::protobuf::Message, TGoal>,
                  "TGoal must be a protobuf Message derived class");
    static_assert(std::is_base_of_v<google::protobuf::Message, TResult>,
                  "TResult must be a protobuf Message derived class");

    using GoalType = TGoal;
    using ResultType = TResult;
    using FeedbackType = moss::proto::action::ActionFeedback;

    /**
     * @brief 目标处理函数类型
     * @param goal_id 目标 ID
     * @param goal 目标消息（引用）
     * @return std::tuple<bool, TResult, float> - (是否接受, 结果, 进度)
     */
    using GoalHandler = std::function<std::tuple<bool, TResult, float>(
        uint32_t goal_id, const TGoal& goal)>;

    /**
     * @brief 取消处理函数类型
     * @param goal_id 目标 ID
     * @return 是否接受取消
     */
    using CancelHandler = std::function<bool(uint32_t goal_id)>;

    /**
     * @brief 反馈发布函数类型
     */
    using FeedbackPublisher = std::function<void(uint32_t goal_id, float progress,
                                                 const google::protobuf::Message& feedback)>;

    /**
     * @brief 构造 Action 服务端
     * @param action_name 动作名称
     * @param action_id 动作 ID
     */
    ProtoActionServer(const std::string& action_name, uint32_t action_id);

    ~ProtoActionServer();

    /**
     * @brief 初始化服务端（创建 publisher/subscriber）
     */
    void init();

    /**
     * @brief 注册目标处理函数
     * @param handler 目标处理函数
     */
    void register_goal_handler(GoalHandler handler);

    /**
     * @brief 注册取消处理函数
     * @param handler 取消处理函数
     */
    void register_cancel_handler(CancelHandler handler);

    /**
     * @brief 启动服务端
     */
    void start();

    /**
     * @brief 停止服务端
     */
    void stop();

    /**
     * @brief 发布反馈
     * @param goal_id 目标 ID
     * @param progress 进度 [0.0, 1.0]
     * @param feedback 反馈消息
     */
    template<typename TFeedback>
    void publish_feedback(uint32_t goal_id, float progress, const TFeedback& feedback);

    /**
     * @brief 发布反馈（无具体反馈数据）
     * @param goal_id 目标 ID
     * @param progress 进度
     */
    void publish_feedback(uint32_t goal_id, float progress);

    /**
     * @brief 发送结果
     * @param goal_id 目标 ID
     * @param status 状态
     * @param result 结果消息
     */
    void send_result(uint32_t goal_id, ActionStatus status, const TResult& result);

    /**
     * @brief 接受目标
     * @param goal_id 目标 ID
     */
    void accept_goal(uint32_t goal_id);

    /**
     * @brief 拒绝目标
     * @param goal_id 目标 ID
     */
    void reject_goal(uint32_t goal_id);

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

    void on_goal(const moss::proto::action::ActionGoal& msg);
    void on_cancel(const moss::proto::action::ActionCancel& msg);

    std::string action_name_;
    uint32_t action_id_;
    bool initialized_{false};
    std::atomic<bool> running_{false};

    GoalHandler goal_handler_;
    CancelHandler cancel_handler_;
    std::mutex handlers_mutex_;

    std::mutex goal_handles_mutex_;
    std::unordered_map<uint32_t, std::shared_ptr<GoalHandle>> goal_handles_;

    std::shared_ptr<topic::ProtoSubscriber<moss::proto::action::ActionGoal>> goal_sub_;
    std::shared_ptr<topic::ProtoSubscriber<moss::proto::action::ActionCancel>> cancel_sub_;
    std::shared_ptr<topic::ProtoPublisher<moss::proto::action::ActionResult>> result_pub_;
    std::shared_ptr<topic::ProtoPublisher<moss::proto::action::ActionFeedback>> feedback_pub_;
};

template<typename TGoal, typename TResult>
ProtoActionServer<TGoal, TResult>::ProtoActionServer(
    const std::string& action_name, uint32_t action_id)
    : action_name_(action_name), action_id_(action_id) {
}

template<typename TGoal, typename TResult>
ProtoActionServer<TGoal, TResult>::~ProtoActionServer() {
    stop();
}

template<typename TGoal, typename TResult>
std::string ProtoActionServer<TGoal, TResult>::get_goal_topic() const {
    return "action_" + action_name_ + "_" + std::to_string(action_id_) + "_goal";
}

template<typename TGoal, typename TResult>
std::string ProtoActionServer<TGoal, TResult>::get_cancel_topic() const {
    return "action_" + action_name_ + "_" + std::to_string(action_id_) + "_cancel";
}

template<typename TGoal, typename TResult>
std::string ProtoActionServer<TGoal, TResult>::get_result_topic() const {
    return "action_" + action_name_ + "_" + std::to_string(action_id_) + "_result";
}

template<typename TGoal, typename TResult>
std::string ProtoActionServer<TGoal, TResult>::get_feedback_topic() const {
    return "action_" + action_name_ + "_" + std::to_string(action_id_) + "_feedback";
}

template<typename TGoal, typename TResult>
void ProtoActionServer<TGoal, TResult>::init() {
    if (initialized_) {
        return;
    }

    auto& topic_mgr = topic::TopicManager::instance();

    goal_sub_ = topic_mgr.create_proto_subscriber<moss::proto::action::ActionGoal>(
        get_goal_topic(),
        [this](const moss::proto::action::ActionGoal& msg, uint64_t) {
            this->on_goal(msg);
        });

    cancel_sub_ = topic_mgr.create_proto_subscriber<moss::proto::action::ActionCancel>(
        get_cancel_topic(),
        [this](const moss::proto::action::ActionCancel& msg, uint64_t) {
            this->on_cancel(msg);
        });

    result_pub_ = topic_mgr.create_proto_publisher<moss::proto::action::ActionResult>(
        get_result_topic());

    feedback_pub_ = topic_mgr.create_proto_publisher<moss::proto::action::ActionFeedback>(
        get_feedback_topic());

    initialized_ = true;
}

template<typename TGoal, typename TResult>
void ProtoActionServer<TGoal, TResult>::register_goal_handler(GoalHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    goal_handler_ = handler;
}

template<typename TGoal, typename TResult>
void ProtoActionServer<TGoal, TResult>::register_cancel_handler(CancelHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    cancel_handler_ = handler;
}

template<typename TGoal, typename TResult>
void ProtoActionServer<TGoal, TResult>::start() {
    if (running_.load()) {
        return;
    }
    running_.store(true);
}

template<typename TGoal, typename TResult>
void ProtoActionServer<TGoal, TResult>::stop() {
    running_.store(false);
    goal_sub_.reset();
    cancel_sub_.reset();
    result_pub_.reset();
    feedback_pub_.reset();
}

template<typename TGoal, typename TResult>
void ProtoActionServer<TGoal, TResult>::on_goal(
    const moss::proto::action::ActionGoal& msg) {

    if (!running_.load()) {
        return;
    }

    uint32_t goal_id = msg.goal_id();

    TGoal goal;
    if (!goal.ParseFromString(msg.goal_payload())) {
        reject_goal(goal_id);
        return;
    }

    auto handle = std::make_shared<GoalHandle>(goal_id);
    {
        std::lock_guard<std::mutex> lock(goal_handles_mutex_);
        goal_handles_[goal_id] = handle;
    }

    std::lock_guard<std::mutex> lock(handlers_mutex_);
    if (goal_handler_) {
        auto [accepted, result, progress] = goal_handler_(goal_id, goal);
        if (accepted) {
            handle->set_status(ActionStatus::EXECUTING);
            if (progress > 0.0f) {
                publish_feedback(goal_id, progress);
            }
        } else {
            handle->set_status(ActionStatus::REJECTED);
        }
    } else {
        handle->set_status(ActionStatus::EXECUTING);
    }
}

template<typename TGoal, typename TResult>
void ProtoActionServer<TGoal, TResult>::on_cancel(
    const moss::proto::action::ActionCancel& msg) {

    uint32_t goal_id = msg.goal_id();

    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it != goal_handles_.end()) {
        if (!it->second->cancel_requested()) {
            it->second->request_cancel();
            std::lock_guard<std::mutex> lock2(handlers_mutex_);
            if (cancel_handler_) {
                cancel_handler_(goal_id);
            }
        }
    }
}

template<typename TGoal, typename TResult>
void ProtoActionServer<TGoal, TResult>::accept_goal(uint32_t goal_id) {
    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it == goal_handles_.end()) {
        auto handle = std::make_shared<GoalHandle>(goal_id);
        handle->set_status(ActionStatus::EXECUTING);
        goal_handles_[goal_id] = handle;
    } else {
        it->second->set_status(ActionStatus::EXECUTING);
    }
}

template<typename TGoal, typename TResult>
void ProtoActionServer<TGoal, TResult>::reject_goal(uint32_t goal_id) {
    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it == goal_handles_.end()) {
        auto handle = std::make_shared<GoalHandle>(goal_id);
        handle->set_status(ActionStatus::REJECTED);
        goal_handles_[goal_id] = handle;
    } else {
        it->second->set_status(ActionStatus::REJECTED);
    }
}

template<typename TGoal, typename TResult>
void ProtoActionServer<TGoal, TResult>::send_result(
    uint32_t goal_id, ActionStatus status, const TResult& result) {

    moss::proto::action::ActionResult result_msg;
    result_msg.set_goal_id(goal_id);
    result_msg.set_timestamp_us(get_current_timestamp_us());

    switch (status) {
        case ActionStatus::SUCCEEDED:
            result_msg.set_terminal_status(moss::proto::action::SUCCEEDED);
            break;
        case ActionStatus::CANCELED:
            result_msg.set_terminal_status(moss::proto::action::CANCELED);
            break;
        case ActionStatus::REJECTED:
            result_msg.set_terminal_status(moss::proto::action::REJECTED);
            break;
        case ActionStatus::ABORTED:
            result_msg.set_terminal_status(moss::proto::action::ABORTED);
            break;
        case ActionStatus::PREEMPTED:
            result_msg.set_terminal_status(moss::proto::action::PREEMPTED);
            break;
        default:
            result_msg.set_terminal_status(moss::proto::action::UNKNOWN);
            break;
    }

    if (!result.SerializeToString(result_msg.mutable_result_payload())) {
        return;
    }

    result_pub_->publish(result_msg);

    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it != goal_handles_.end()) {
        std::vector<uint8_t> result_data(result_msg.result_payload().begin(),
                                          result_msg.result_payload().end());
        it->second->set_result(status, result_data);
    }
}

template<typename TGoal, typename TResult>
template<typename TFeedback>
void ProtoActionServer<TGoal, TResult>::publish_feedback(
    uint32_t goal_id, float progress, const TFeedback& feedback) {

    (void)progress;

    moss::proto::action::ActionFeedback feedback_msg;
    feedback_msg.set_goal_id(goal_id);
    feedback_msg.set_timestamp_us(get_current_timestamp_us());

    if (!feedback.SerializeToString(feedback_msg.mutable_feedback_payload())) {
        return;
    }

    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it != goal_handles_.end()) {
        auto status = it->second->get_status();
        switch (status) {
            case ActionStatus::UNKNOWN:
                feedback_msg.set_status(moss::proto::action::UNKNOWN);
                break;
            case ActionStatus::SUCCEEDED:
                feedback_msg.set_status(moss::proto::action::SUCCEEDED);
                break;
            case ActionStatus::CANCELED:
                feedback_msg.set_status(moss::proto::action::CANCELED);
                break;
            case ActionStatus::REJECTED:
                feedback_msg.set_status(moss::proto::action::REJECTED);
                break;
            case ActionStatus::ABORTED:
                feedback_msg.set_status(moss::proto::action::ABORTED);
                break;
            case ActionStatus::PREEMPTED:
                feedback_msg.set_status(moss::proto::action::PREEMPTED);
                break;
            default:
                feedback_msg.set_status(moss::proto::action::UNKNOWN);
                break;
        }
    } else {
        feedback_msg.set_status(moss::proto::action::UNKNOWN);
    }

    feedback_pub_->publish(feedback_msg);
}

template<typename TGoal, typename TResult>
void ProtoActionServer<TGoal, TResult>::publish_feedback(uint32_t goal_id, float progress) {
    publish_feedback<moss::proto::action::ActionFeedback>(
        goal_id, progress, moss::proto::action::ActionFeedback{});
}

template<typename TGoal, typename TResult>
using ProtoActionServerPtr = std::shared_ptr<ProtoActionServer<TGoal, TResult>>;

}  // namespace action
}  // namespace mcom
}  // namespace moss
