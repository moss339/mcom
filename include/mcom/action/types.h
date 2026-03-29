#ifndef MACTION_TYPES_H
#define MACTION_TYPES_H

#include <cstdint>
#include <vector>
#include <functional>
#include <chrono>
#include <optional>
#include <future>

namespace maction {

enum class ActionStatus : uint8_t {
    READY = 0x00,
    EXECUTING = 0x01,
    PREEMPTING = 0x02,
    PREEMPTED = 0x03,
    SUCCEEDING = 0x04,
    SUCCEEDED = 0x05,
    ABORTING = 0x06,
    ABORTED = 0x07,
    CANCELED = 0x08,
    REJECTED = 0x09,
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
    float progress;
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

}  // namespace maction

#endif  // MACTION_TYPES_H
