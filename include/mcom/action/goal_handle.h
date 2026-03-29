#ifndef MACTION_GOAL_HANDLE_H
#define MACTION_GOAL_HANDLE_H

#include "types.h"
#include <atomic>
#include <mutex>

namespace maction {

class GoalHandle {
public:
    explicit GoalHandle(uint32_t goal_id);
    ~GoalHandle();

    GoalHandle(const GoalHandle&) = delete;
    GoalHandle& operator=(const GoalHandle&) = delete;

    uint32_t get_goal_id() const { return goal_id_; }
    ActionStatus get_status() const;
    void set_status(ActionStatus status);

    bool is_terminal() const {
        auto status = get_status();
        return status == ActionStatus::SUCCEEDED ||
               status == ActionStatus::ABORTED ||
               status == ActionStatus::CANCELED;
    }

    bool cancel_requested() const { return cancel_requested_.load(); }
    void request_cancel() { cancel_requested_.store(true); }

    void set_result(ActionStatus status, const std::vector<uint8_t>& result);
    bool has_result() const;
    ResultInfo get_result() const;

private:
    uint32_t goal_id_;
    std::atomic<ActionStatus> status_{ActionStatus::READY};
    std::atomic<bool> cancel_requested_{false};
    std::atomic<bool> has_result_{false};
    ResultInfo result_info_;
    mutable std::mutex result_mutex_;
};

}  // namespace maction

#endif  // MACTION_GOAL_HANDLE_H
