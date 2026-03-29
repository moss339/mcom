#ifndef MACTION_ERROR_H
#define MACTION_ERROR_H

#include <cstdint>
#include <string>

namespace maction {

enum class ActionError : uint32_t {
    SUCCESS = 0,
    INVALID_GOAL_ID = 1,
    GOAL_NOT_FOUND = 2,
    GOAL_ALREADY_ACTIVE = 3,
    GOAL_EXECUTING = 4,
    GOAL_TERMINAL = 5,
    CANCEL_REJECTED = 6,
    TIMEOUT = 7,
    NETWORK_ERROR = 8,
    INVALID_STATE = 9,
};

inline const char* error_to_string(ActionError error) {
    switch (error) {
        case ActionError::SUCCESS: return "Success";
        case ActionError::INVALID_GOAL_ID: return "Invalid goal ID";
        case ActionError::GOAL_NOT_FOUND: return "Goal not found";
        case ActionError::GOAL_ALREADY_ACTIVE: return "Goal already active";
        case ActionError::GOAL_EXECUTING: return "Goal is executing";
        case ActionError::GOAL_TERMINAL: return "Goal already in terminal state";
        case ActionError::CANCEL_REJECTED: return "Cancel rejected";
        case ActionError::TIMEOUT: return "Operation timeout";
        case ActionError::NETWORK_ERROR: return "Network error";
        case ActionError::INVALID_STATE: return "Invalid state";
        default: return "Unknown error";
    }
}

}  // namespace maction

#endif  // MACTION_ERROR_H
