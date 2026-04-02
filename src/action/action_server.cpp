#include "mcom/action/action_server.h"
#include "mcom/topic/topic_manager.h"
#include <mdds/mdds.h>
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

std::string ActionServer::get_goal_topic() const {
    return "action_" + std::to_string(config_.service_id) + "_" +
           std::to_string(config_.instance_id) + "_goal";
}

std::string ActionServer::get_cancel_topic() const {
    return "action_" + std::to_string(config_.service_id) + "_" +
           std::to_string(config_.instance_id) + "_cancel";
}

std::string ActionServer::get_result_topic() const {
    return "action_" + std::to_string(config_.service_id) + "_" +
           std::to_string(config_.instance_id) + "_result";
}

std::string ActionServer::get_feedback_topic() const {
    return "action_" + std::to_string(config_.service_id) + "_" +
           std::to_string(config_.instance_id) + "_feedback";
}

void ActionServer::init() {
    if (initialized_.load()) {
        return;
    }

    auto participant = topic::TopicManager::instance().get_participant();
    if (!participant) {
        return;
    }

    goal_subscriber_ = participant->create_subscriber<ActionGoalMessage>(
        get_goal_topic(),
        [this](const ActionGoalMessage& msg, uint64_t) {
            if (msg.service_id == config_.service_id &&
                msg.instance_id == config_.instance_id) {
                on_send_goal(msg.goal_id, msg.goal_payload);
            }
        });

    cancel_subscriber_ = participant->create_subscriber<ActionCancelMessage>(
        get_cancel_topic(),
        [this](const ActionCancelMessage& msg, uint64_t) {
            (void)msg;
            on_cancel_goal(msg.goal_id);
        });

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
    goal_subscriber_.reset();
    cancel_subscriber_.reset();
}

void ActionServer::publish_feedback(uint32_t goal_id, float progress,
                                     const std::vector<uint8_t>& feedback_data) {
    auto participant = topic::TopicManager::instance().get_participant();
    if (!participant) {
        return;
    }

    static thread_local auto publisher = participant->create_publisher<ActionFeedbackMessage>(
        get_feedback_topic());

    ActionFeedbackMessage feedback_msg;
    feedback_msg.goal_id = goal_id;
    feedback_msg.timestamp_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    feedback_msg.progress = progress;
    feedback_msg.feedback_payload = feedback_data;

    std::lock_guard<std::mutex> lock(goal_handles_mutex_);
    auto it = goal_handles_.find(goal_id);
    if (it != goal_handles_.end()) {
        feedback_msg.status = it->second->get_status();
    } else {
        feedback_msg.status = ActionStatus::EXECUTING;
    }

    publisher->write(feedback_msg);
}

void ActionServer::publish_status_update(uint32_t goal_id, ActionStatus status) {
    (void)goal_id;
    (void)status;
}

void ActionServer::send_result(uint32_t goal_id, ActionStatus status,
                                const std::vector<uint8_t>& result_data) {
    auto participant = topic::TopicManager::instance().get_participant();
    if (!participant) {
        return;
    }

    static thread_local auto publisher = participant->create_publisher<ActionResultMessage>(
        get_result_topic());

    ActionResultMessage result_msg;
    result_msg.goal_id = goal_id;
    result_msg.timestamp_us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    result_msg.terminal_status = status;
    result_msg.result_payload = result_data;

    publisher->write(result_msg);

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