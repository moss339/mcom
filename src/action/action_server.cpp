#include "mcom/action/action_server.h"
#include <iostream>

namespace moss {
namespace mcom {
namespace action {

ActionServer::ActionServer(ActionServerConfig config)
    : config_(config), next_goal_id_(1) {
}

ActionServer::~ActionServer() {
    stop();
}

void ActionServer::init() {
    if (initialized_.load()) {
        return;
    }
    initialized_.store(true);
}

void ActionServer::register_goal_handler(GoalHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    goal_handler_ = handler;
}

void ActionServer::register_cancel_handler(CancelHandler handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    cancel_handler_ = handler;
}

void ActionServer::start() {
    if (running_.load()) {
        return;
    }
    running_.store(true);
}

void ActionServer::stop() {
    running_.store(false);
}

void ActionServer::publish_feedback(uint32_t goal_id, float progress,
                                     const std::vector<uint8_t>& feedback_data) {
    (void)goal_id;
    (void)progress;
    (void)feedback_data;
}

void ActionServer::publish_status_update(uint32_t goal_id, ActionStatus status) {
    (void)goal_id;
    (void)status;
}

void ActionServer::send_result(uint32_t goal_id, ActionStatus status,
                                const std::vector<uint8_t>& result_data) {
    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it != goal_handles_.end()) {
        it->second->set_result(status, result_data);
    }
}

void ActionServer::accept_goal(uint32_t goal_id) {
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

void ActionServer::reject_goal(uint32_t goal_id) {
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

void ActionServer::on_send_goal(uint32_t goal_id, const std::vector<uint8_t>& goal_data) {
    GoalInfo info;
    info.goal_id = goal_id;
    info.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    info.goal_data = goal_data;

    std::shared_ptr<GoalHandle> handle;
    {
        std::lock_guard<std::mutex> lock(goal_handles_mutex_);
        handle = std::make_shared<GoalHandle>(goal_id);
        goal_handles_[goal_id] = handle;
    }

    std::lock_guard<std::mutex> lock(handlers_mutex_);
    if (goal_handler_) {
        auto [accepted, status, result] = goal_handler_(info);
        if (accepted) {
            handle->set_status(ActionStatus::EXECUTING);
        } else {
            handle->set_status(ActionStatus::REJECTED);
        }
    }
}

void ActionServer::on_cancel_goal(uint32_t goal_id) {
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

}  // namespace action
}  // namespace mcom

}  // namespace moss
