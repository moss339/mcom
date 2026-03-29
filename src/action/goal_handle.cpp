#include "mcom/action/goal_handle.h"
#include <stdexcept>

namespace mcom {
namespace action {

GoalHandle::GoalHandle(uint32_t goal_id)
    : goal_id_(goal_id), status_(ActionStatus::READY) {
}

GoalHandle::~GoalHandle() = default;

ActionStatus GoalHandle::get_status() const {
    return status_.load();
}

void GoalHandle::set_status(ActionStatus status) {
    status_.store(status);
}

void GoalHandle::set_result(ActionStatus status, const std::vector<uint8_t>& result) {
    std::lock_guard<std::mutex> lock(result_mutex_);
    result_info_.goal_id = goal_id_;
    result_info_.status = status;
    result_info_.result_data = result;
    has_result_.store(true);
    status_.store(status);
}

bool GoalHandle::has_result() const {
    return has_result_.load();
}

ResultInfo GoalHandle::get_result() const {
    std::lock_guard<std::mutex> lock(result_mutex_);
    return result_info_;
}

}  // namespace action
}  // namespace mcom
