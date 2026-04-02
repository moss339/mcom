#ifndef MCOM_ACTION_TYPES_H
#define MCOM_ACTION_TYPES_H

#include <cstdint>
#include <vector>
#include <functional>
#include <chrono>
#include <optional>
#include <future>
#include <cstring>

namespace moss {
namespace mcom {
namespace action {

enum class ActionStatus : uint8_t {
    UNKNOWN = 0,
    SUCCEEDED = 1,
    CANCELED = 2,
    REJECTED = 3,
    ABORTED = 4,
    PREEMPTED = 5,
    EXECUTING = 6,
    PREEMPTING = 7,
    READY = 8,
};

enum class ActionGoalStatus : uint8_t {
    GOAL_WAITING = 0,
    GOAL_ACCEPTED = 1,
    GOAL_EXECUTING = 2,
    GOAL_CANCEL_REQUESTED = 3,
    GOAL_SUCCEEDED = 4,
    GOAL_CANCELED = 5,
    GOAL_ABORTED = 6,
    GOAL_REJECTED = 7,
    GOAL_PREEMPTED = 8,
};

struct GoalInfo {
    uint32_t goal_id;
    uint64_t timestamp;
    std::vector<uint8_t> goal_data;
};

struct ResultInfo {
    uint32_t goal_id;
    ActionStatus status;
    std::vector<uint8_t> result_data;
};

struct FeedbackInfo {
    uint32_t goal_id;
    ActionStatus status;
    std::vector<uint8_t> feedback_data;
};

struct ActionServerConfig {
    uint16_t service_id;
    uint16_t instance_id;
    uint16_t method_id_base;
};

struct ActionClientConfig {
    uint16_t service_id;
    uint16_t instance_id;
    uint16_t method_id_base;
};

using GoalHandler = std::function<std::tuple<bool, ActionStatus, std::vector<uint8_t>>(
    const GoalInfo& goal)>;
using FeedbackHandler = std::function<void(const FeedbackInfo& feedback)>;
using CancelHandler = std::function<bool(uint32_t goal_id)>;

struct ActionGoalMessage {
    uint64_t timestamp_us;
    uint32_t goal_id;
    uint16_t service_id;
    uint16_t instance_id;
    std::vector<uint8_t> goal_payload;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        size_t total_size = sizeof(timestamp_us) + sizeof(goal_id) +
                          sizeof(service_id) + sizeof(instance_id) +
                          sizeof(size_t) + goal_payload.size();
        buffer.resize(total_size);
        size_t offset = 0;
        std::memcpy(buffer.data() + offset, &timestamp_us, sizeof(timestamp_us));
        offset += sizeof(timestamp_us);
        std::memcpy(buffer.data() + offset, &goal_id, sizeof(goal_id));
        offset += sizeof(goal_id);
        std::memcpy(buffer.data() + offset, &service_id, sizeof(service_id));
        offset += sizeof(service_id);
        std::memcpy(buffer.data() + offset, &instance_id, sizeof(instance_id));
        offset += sizeof(instance_id);
        size_t payload_size = goal_payload.size();
        std::memcpy(buffer.data() + offset, &payload_size, sizeof(size_t));
        offset += sizeof(size_t);
        if (!goal_payload.empty()) {
            std::memcpy(buffer.data() + offset, goal_payload.data(), goal_payload.size());
        }
        return buffer;
    }

    static ActionGoalMessage deserialize(const uint8_t* buffer, size_t size) {
        ActionGoalMessage msg;
        size_t offset = 0;
        if (offset + sizeof(timestamp_us) > size) return msg;
        std::memcpy(&msg.timestamp_us, buffer + offset, sizeof(timestamp_us));
        offset += sizeof(timestamp_us);
        if (offset + sizeof(goal_id) > size) return msg;
        std::memcpy(&msg.goal_id, buffer + offset, sizeof(goal_id));
        offset += sizeof(goal_id);
        if (offset + sizeof(service_id) > size) return msg;
        std::memcpy(&msg.service_id, buffer + offset, sizeof(service_id));
        offset += sizeof(service_id);
        if (offset + sizeof(instance_id) > size) return msg;
        std::memcpy(&msg.instance_id, buffer + offset, sizeof(instance_id));
        offset += sizeof(instance_id);
        if (offset + sizeof(size_t) > size) return msg;
        size_t payload_size;
        std::memcpy(&payload_size, buffer + offset, sizeof(size_t));
        offset += sizeof(size_t);
        if (offset + payload_size > size) return msg;
        msg.goal_payload.resize(payload_size);
        if (payload_size > 0) {
            std::memcpy(msg.goal_payload.data(), buffer + offset, payload_size);
        }
        return msg;
    }
};

struct ActionCancelMessage {
    uint64_t timestamp_us;
    uint32_t goal_id;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer(sizeof(timestamp_us) + sizeof(goal_id));
        size_t offset = 0;
        std::memcpy(buffer.data() + offset, &timestamp_us, sizeof(timestamp_us));
        offset += sizeof(timestamp_us);
        std::memcpy(buffer.data() + offset, &goal_id, sizeof(goal_id));
        return buffer;
    }

    static ActionCancelMessage deserialize(const uint8_t* buffer, size_t size) {
        ActionCancelMessage msg;
        if (size < sizeof(timestamp_us) + sizeof(goal_id)) return msg;
        std::memcpy(&msg.timestamp_us, buffer, sizeof(timestamp_us));
        std::memcpy(&msg.goal_id, buffer + sizeof(timestamp_us), sizeof(goal_id));
        return msg;
    }
};

struct ActionFeedbackMessage {
    uint64_t timestamp_us;
    uint32_t goal_id;
    ActionStatus status;
    float progress;
    std::vector<uint8_t> feedback_payload;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        size_t total_size = sizeof(timestamp_us) + sizeof(goal_id) +
                          sizeof(status) + sizeof(progress) +
                          sizeof(size_t) + feedback_payload.size();
        buffer.resize(total_size);
        size_t offset = 0;
        std::memcpy(buffer.data() + offset, &timestamp_us, sizeof(timestamp_us));
        offset += sizeof(timestamp_us);
        std::memcpy(buffer.data() + offset, &goal_id, sizeof(goal_id));
        offset += sizeof(goal_id);
        std::memcpy(buffer.data() + offset, &status, sizeof(status));
        offset += sizeof(status);
        std::memcpy(buffer.data() + offset, &progress, sizeof(progress));
        offset += sizeof(progress);
        size_t payload_size = feedback_payload.size();
        std::memcpy(buffer.data() + offset, &payload_size, sizeof(size_t));
        offset += sizeof(size_t);
        if (!feedback_payload.empty()) {
            std::memcpy(buffer.data() + offset, feedback_payload.data(), feedback_payload.size());
        }
        return buffer;
    }

    static ActionFeedbackMessage deserialize(const uint8_t* buffer, size_t size) {
        ActionFeedbackMessage msg;
        size_t offset = 0;
        if (offset + sizeof(timestamp_us) > size) return msg;
        std::memcpy(&msg.timestamp_us, buffer + offset, sizeof(timestamp_us));
        offset += sizeof(timestamp_us);
        if (offset + sizeof(goal_id) > size) return msg;
        std::memcpy(&msg.goal_id, buffer + offset, sizeof(goal_id));
        offset += sizeof(goal_id);
        if (offset + sizeof(status) > size) return msg;
        std::memcpy(&msg.status, buffer + offset, sizeof(status));
        offset += sizeof(status);
        if (offset + sizeof(progress) > size) return msg;
        std::memcpy(&msg.progress, buffer + offset, sizeof(progress));
        offset += sizeof(progress);
        if (offset + sizeof(size_t) > size) return msg;
        size_t payload_size;
        std::memcpy(&payload_size, buffer + offset, sizeof(size_t));
        offset += sizeof(size_t);
        if (offset + payload_size > size) return msg;
        msg.feedback_payload.resize(payload_size);
        if (payload_size > 0) {
            std::memcpy(msg.feedback_payload.data(), buffer + offset, payload_size);
        }
        return msg;
    }
};

struct ActionResultMessage {
    uint64_t timestamp_us;
    uint32_t goal_id;
    ActionStatus terminal_status;
    std::vector<uint8_t> result_payload;

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> buffer;
        size_t total_size = sizeof(timestamp_us) + sizeof(goal_id) +
                          sizeof(terminal_status) +
                          sizeof(size_t) + result_payload.size();
        buffer.resize(total_size);
        size_t offset = 0;
        std::memcpy(buffer.data() + offset, &timestamp_us, sizeof(timestamp_us));
        offset += sizeof(timestamp_us);
        std::memcpy(buffer.data() + offset, &goal_id, sizeof(goal_id));
        offset += sizeof(goal_id);
        std::memcpy(buffer.data() + offset, &terminal_status, sizeof(terminal_status));
        offset += sizeof(terminal_status);
        size_t payload_size = result_payload.size();
        std::memcpy(buffer.data() + offset, &payload_size, sizeof(size_t));
        offset += sizeof(size_t);
        if (!result_payload.empty()) {
            std::memcpy(buffer.data() + offset, result_payload.data(), result_payload.size());
        }
        return buffer;
    }

    static ActionResultMessage deserialize(const uint8_t* buffer, size_t size) {
        ActionResultMessage msg;
        size_t offset = 0;
        if (offset + sizeof(timestamp_us) > size) return msg;
        std::memcpy(&msg.timestamp_us, buffer + offset, sizeof(timestamp_us));
        offset += sizeof(timestamp_us);
        if (offset + sizeof(goal_id) > size) return msg;
        std::memcpy(&msg.goal_id, buffer + offset, sizeof(goal_id));
        offset += sizeof(goal_id);
        if (offset + sizeof(terminal_status) > size) return msg;
        std::memcpy(&msg.terminal_status, buffer + offset, sizeof(terminal_status));
        offset += sizeof(terminal_status);
        if (offset + sizeof(size_t) > size) return msg;
        size_t payload_size;
        std::memcpy(&payload_size, buffer + offset, sizeof(size_t));
        offset += sizeof(size_t);
        if (offset + payload_size > size) return msg;
        msg.result_payload.resize(payload_size);
        if (payload_size > 0) {
            std::memcpy(msg.result_payload.data(), buffer + offset, payload_size);
        }
        return msg;
    }
};

}  // namespace action
}  // namespace mcom
}  // namespace moss
#endif  // MCOM_ACTION_TYPES_H