#include "maction/action_server.h"
#include "msomeip/application.h"
#include "msomeip/message/message.h"
#include <iostream>

namespace maction {

ActionServer::ActionServer(std::shared_ptr<msomeip::Application> app,
                           ActionServerConfig config)
    : app_(app), config_(config), next_goal_id_(1) {
}

ActionServer::~ActionServer() {
    stop();
}

void ActionServer::init() {
    if (initialized_.load()) {
        return;
    }

    auto self = shared_from_this();

    app_->register_method_handler(
        static_cast<msomeip::MethodId>(config_.method_id_base + 0x0001),
        [self](msomeip::MessagePtr msg) {
            if (msg) {
                auto payload = msg->get_payload();
                if (payload && payload->size() >= 4) {
                    const uint8_t* data = payload->data();
                    uint32_t goal_id = (static_cast<uint32_t>(data[0]) << 24) |
                                       (static_cast<uint32_t>(data[1]) << 16) |
                                       (static_cast<uint32_t>(data[2]) << 8) |
                                       static_cast<uint32_t>(data[3]);
                    std::vector<uint8_t> goal_data(data + 4, data + payload->size());
                    self->on_send_goal(goal_id, goal_data);
                }
            }
        });

    app_->register_method_handler(
        static_cast<msomeip::MethodId>(config_.method_id_base + 0x0002),
        [self](msomeip::MessagePtr msg) {
            if (msg) {
                auto payload = msg->get_payload();
                if (payload && payload->size() >= 4) {
                    const uint8_t* data = payload->data();
                    uint32_t goal_id = (static_cast<uint32_t>(data[0]) << 24) |
                                       (static_cast<uint32_t>(data[1]) << 16) |
                                       (static_cast<uint32_t>(data[2]) << 8) |
                                       static_cast<uint32_t>(data[3]);
                    self->on_cancel_goal(goal_id);
                }
            }
        });

    msomeip::EventConfig feedback_event_config;
    feedback_event_config.service_id = config_.service_id;
    feedback_event_config.instance_id = config_.instance_id;
    feedback_event_config.event_id = config_.method_id_base + 0x8001;
    feedback_event_config.eventgroup_id = config_.method_id_base + 0x8001;
    app_->offer_event(feedback_event_config);

    msomeip::EventConfig status_event_config;
    status_event_config.service_id = config_.service_id;
    status_event_config.instance_id = config_.instance_id;
    status_event_config.event_id = config_.method_id_base + 0x8002;
    status_event_config.eventgroup_id = config_.method_id_base + 0x8002;
    app_->offer_event(status_event_config);

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
    std::vector<uint8_t> payload_vec;
    payload_vec.push_back(static_cast<uint8_t>(goal_id >> 24));
    payload_vec.push_back(static_cast<uint8_t>((goal_id >> 16) & 0xFF));
    payload_vec.push_back(static_cast<uint8_t>((goal_id >> 8) & 0xFF));
    payload_vec.push_back(static_cast<uint8_t>(goal_id & 0xFF));

    uint8_t progress_bytes[4];
    std::memcpy(progress_bytes, &progress, sizeof(float));
    payload_vec.insert(payload_vec.end(), progress_bytes, progress_bytes + sizeof(float));
    payload_vec.insert(payload_vec.end(), feedback_data.begin(), feedback_data.end());

    msomeip::PayloadData payload_data(payload_vec);
    app_->notify_event(static_cast<msomeip::EventId>(config_.method_id_base + 0x8001),
                       payload_data);
}

void ActionServer::publish_status_update(uint32_t goal_id, ActionStatus status) {
    std::vector<uint8_t> payload_vec;
    payload_vec.push_back(static_cast<uint8_t>(goal_id >> 24));
    payload_vec.push_back(static_cast<uint8_t>((goal_id >> 16) & 0xFF));
    payload_vec.push_back(static_cast<uint8_t>((goal_id >> 8) & 0xFF));
    payload_vec.push_back(static_cast<uint8_t>(goal_id & 0xFF));
    payload_vec.push_back(static_cast<uint8_t>(status));

    msomeip::PayloadData payload_data(payload_vec);
    app_->notify_event(static_cast<msomeip::EventId>(config_.method_id_base + 0x8002),
                       payload_data);
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

}  // namespace maction
