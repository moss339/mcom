#include "maction/action_client.h"
#include "msomeip/application.h"
#include "msomeip/message/message.h"
#include "mservice/service_client.h"
#include <iostream>
#include <thread>

namespace maction {

ActionClient::ActionClient(std::shared_ptr<msomeip::Application> app,
                           ActionClientConfig config)
    : app_(app), config_(config), next_goal_id_(1) {
}

ActionClient::~ActionClient() {
}

void ActionClient::init() {
    if (initialized_.load()) {
        return;
    }

    auto self = shared_from_this();
    app_->register_method_handler(
        static_cast<msomeip::MethodId>(config_.method_id_base + 0x0003),
        [self](msomeip::MessagePtr msg) {
            if (msg) {
                auto payload = msg->get_payload();
                if (payload && payload->size() >= 5) {
                    const uint8_t* data = payload->data();
                    uint32_t goal_id = (static_cast<uint32_t>(data[0]) << 24) |
                                       (static_cast<uint32_t>(data[1]) << 16) |
                                       (static_cast<uint32_t>(data[2]) << 8) |
                                       static_cast<uint32_t>(data[3]);
                    ActionStatus status = static_cast<ActionStatus>(data[4]);
                    std::vector<uint8_t> result_data(data + 5, data + payload->size());
                    self->on_goal_result(goal_id, status, result_data);
                }
            }
        });

    app_->set_event_handler(
        static_cast<msomeip::EventId>(config_.method_id_base + 0x8001),
        [self](msomeip::MessagePtr msg) {
            if (msg) {
                auto payload = msg->get_payload();
                if (payload && payload->size() >= 9) {
                    const uint8_t* data = payload->data();
                    uint32_t goal_id = (static_cast<uint32_t>(data[0]) << 24) |
                                       (static_cast<uint32_t>(data[1]) << 16) |
                                       (static_cast<uint32_t>(data[2]) << 8) |
                                       static_cast<uint32_t>(data[3]);
                    float progress;
                    std::memcpy(&progress, data + 4, sizeof(float));
                    std::vector<uint8_t> feedback_data(data + 8, data + payload->size());
                    self->on_feedback(goal_id, progress, feedback_data);
                }
            }
        });

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

    std::vector<uint8_t> payload_vec;
    payload_vec.push_back(static_cast<uint8_t>(goal_id >> 24));
    payload_vec.push_back(static_cast<uint8_t>((goal_id >> 16) & 0xFF));
    payload_vec.push_back(static_cast<uint8_t>((goal_id >> 8) & 0xFF));
    payload_vec.push_back(static_cast<uint8_t>(goal_id & 0xFF));
    payload_vec.insert(payload_vec.end(), goal_data.begin(), goal_data.end());

    msomeip::PayloadData payload_data(payload_vec);
    auto future = app_->send_request(
        config_.service_id,
        config_.instance_id,
        static_cast<msomeip::MethodId>(config_.method_id_base + 0x0001),
        payload_data,
        false,
        timeout);

    if (future.wait_for(timeout) == std::future_status::ready) {
        auto response = future.get();
        if (response) {
            goal_handle->set_status(ActionStatus::EXECUTING);
        }
    } else {
        goal_handle->set_status(ActionStatus::ABORTED);
    }

    return goal_handle;
}

std::future<std::shared_ptr<GoalHandle>> ActionClient::send_goal_async(
    const std::vector<uint8_t>& goal_data) {
    return std::async(std::launch::async, [this, goal_data]() {
        return send_goal(goal_data);
    });
}

bool ActionClient::cancel_goal(uint32_t goal_id) {
    std::vector<uint8_t> payload_vec;
    payload_vec.push_back(static_cast<uint8_t>(goal_id >> 24));
    payload_vec.push_back(static_cast<uint8_t>((goal_id >> 16) & 0xFF));
    payload_vec.push_back(static_cast<uint8_t>((goal_id >> 8) & 0xFF));
    payload_vec.push_back(static_cast<uint8_t>(goal_id & 0xFF));

    msomeip::PayloadData payload_data(payload_vec);
    auto future = app_->send_request(
        config_.service_id,
        config_.instance_id,
        static_cast<msomeip::MethodId>(config_.method_id_base + 0x0002),
        payload_data,
        false,
        std::chrono::milliseconds(2000));

    auto response = future.get();
    return response != nullptr;
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
    app_->subscribe_event(
        config_.service_id,
        config_.instance_id,
        config_.method_id_base + 0x8001,
        config_.method_id_base + 0x8001,
        0);
}

void ActionClient::unsubscribe_feedback() {
    std::lock_guard<std::mutex> lock(feedback_mutex_);
    feedback_handler_ = nullptr;
    app_->unsubscribe_event(
        config_.service_id,
        config_.instance_id,
        config_.method_id_base + 0x8001);
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

}  // namespace maction
