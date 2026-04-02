#include "mcom/action/action_client.h"
#include "mcom/topic/topic_manager.h"
#include <mdds/mdds.h>
#include <chrono>

namespace moss {
namespace mcom {
namespace action {

ActionClient::ActionClient(ActionClientConfig config)
    : config_(config), next_goal_id_(1) {
}

ActionClient::~ActionClient() {
}

std::string ActionClient::get_goal_topic() const {
    return "action_" + std::to_string(config_.service_id) + "_" +
           std::to_string(config_.instance_id) + "_goal";
}

std::string ActionClient::get_cancel_topic() const {
    return "action_" + std::to_string(config_.service_id) + "_" +
           std::to_string(config_.instance_id) + "_cancel";
}

std::string ActionClient::get_result_topic() const {
    return "action_" + std::to_string(config_.service_id) + "_" +
           std::to_string(config_.instance_id) + "_result";
}

std::string ActionClient::get_feedback_topic() const {
    return "action_" + std::to_string(config_.service_id) + "_" +
           std::to_string(config_.instance_id) + "_feedback";
}

void ActionClient::init() {
    if (initialized_.load()) {
        return;
    }

    auto participant = topic::TopicManager::instance().get_participant();
    if (!participant) {
        return;
    }

    result_subscriber_ = participant->create_subscriber<ActionResultMessage>(
        get_result_topic(),
        [this](const ActionResultMessage& msg, uint64_t) {
            on_goal_result(msg.goal_id, msg.terminal_status, msg.result_payload);
        });

    feedback_subscriber_ = participant->create_subscriber<ActionFeedbackMessage>(
        get_feedback_topic(),
        [this](const ActionFeedbackMessage& msg, uint64_t) {
            on_feedback(msg.goal_id, msg.progress, msg.feedback_payload);
        });

    initialized_.store(true);
}

std::shared_ptr<GoalHandle> ActionClient::send_goal(
    const std::vector<uint8_t>& goal_data,
    std::chrono::milliseconds timeout) {
    (void)timeout;

    if (!initialized_.load()) {
        init();
    }

    uint32_t goal_id;
    {
        std::lock_guard<std::mutex> lock(goal_handles_mutex_);
        goal_id = next_goal_id_++;
    }

    auto goal_handle = std::make_shared<GoalHandle>(goal_id);
    {
        std::lock_guard<std::mutex> lock(goal_handles_mutex_);
        goal_handles_[goal_id] = goal_handle;
    }

    auto participant = topic::TopicManager::instance().get_participant();
    if (!participant) {
        goal_handle->set_status(ActionStatus::ABORTED);
        return goal_handle;
    }

    auto publisher = participant->create_publisher<ActionGoalMessage>(get_goal_topic());
    if (!publisher) {
        goal_handle->set_status(ActionStatus::ABORTED);
        return goal_handle;
    }

    ActionGoalMessage goal_msg;
    goal_msg.goal_id = goal_id;
    goal_msg.timestamp_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    goal_msg.service_id = config_.service_id;
    goal_msg.instance_id = config_.instance_id;
    goal_msg.goal_payload = goal_data;

    publisher->write(goal_msg);

    goal_handle->set_status(ActionStatus::EXECUTING);
    return goal_handle;
}

std::future<std::shared_ptr<GoalHandle>> ActionClient::send_goal_async(
    const std::vector<uint8_t>& goal_data) {
    return std::async(std::launch::async, [this, goal_data]() {
        return send_goal(goal_data);
    });
}

bool ActionClient::cancel_goal(uint32_t goal_id) {
    auto participant = topic::TopicManager::instance().get_participant();
    if (!participant) {
        return false;
    }

    auto publisher = participant->create_publisher<ActionCancelMessage>(get_cancel_topic());
    if (!publisher) {
        return false;
    }

    ActionCancelMessage cancel_msg;
    cancel_msg.goal_id = goal_id;
    cancel_msg.timestamp_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    publisher->write(cancel_msg);

    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it != goal_handles_.end()) {
        it->second->request_cancel();
    }

    return true;
}

std::optional<ResultInfo> ActionClient::get_result(
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

void ActionClient::subscribe_feedback(FeedbackHandler handler) {
    std::lock_guard<std::mutex> lock(feedback_mutex_);
    feedback_handler_ = handler;
}

void ActionClient::unsubscribe_feedback() {
    std::lock_guard<std::mutex> lock(feedback_mutex_);
    feedback_handler_ = nullptr;
}

void ActionClient::on_goal_result(uint32_t goal_id, ActionStatus status,
                                   const std::vector<uint8_t>& result_data) {
    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it != goal_handles_.end()) {
        it->second->set_result(status, result_data);
    }
}

void ActionClient::on_feedback(uint32_t goal_id, float progress,
                                const std::vector<uint8_t>& feedback_data) {
    (void)progress;
    FeedbackInfo info;
    info.goal_id = goal_id;
    info.feedback_data = feedback_data;

    std::lock_guard<std::mutex> lock(feedback_mutex_);
    if (feedback_handler_) {
        feedback_handler_(info);
    }
}

}  // namespace action
}  // namespace mcom
}  // namespace moss