#include "mcom/action/action_client.h"
#include <iostream>

namespace mcom {
namespace action {

ActionClient::ActionClient(ActionClientConfig config)
    : config_(config), next_goal_id_(1) {
}

ActionClient::~ActionClient() {
}

void ActionClient::init() {
    if (initialized_.load()) {
        return;
    }
    initialized_.store(true);
}

std::shared_ptr<GoalHandle> ActionClient::send_goal(
    const std::vector<uint8_t>& goal_data,
    std::chrono::milliseconds timeout) {
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
    (void)goal_id;
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
    FeedbackInfo info;
    info.goal_id = goal_id;
    info.progress = progress;
    info.feedback_data = feedback_data;

    std::lock_guard<std::mutex> lock(feedback_mutex_);
    if (feedback_handler_) {
        feedback_handler_(info);
    }
}

}  // namespace action
}  // namespace mcom
