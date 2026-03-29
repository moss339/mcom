#ifndef MACTION_ACTION_CLIENT_H
#define MACTION_ACTION_CLIENT_H

#include "types.h"
#include "goal_handle.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace msomeip {
class Application;
}

namespace maction {

class ActionClient : public std::enable_shared_from_this<ActionClient> {
public:
    ActionClient(std::shared_ptr<msomeip::Application> app,
                 ActionClientConfig config);
    ~ActionClient();

    ActionClient(const ActionClient&) = delete;
    ActionClient& operator=(const ActionClient&) = delete;

    void init();

    std::shared_ptr<GoalHandle> send_goal(
        const std::vector<uint8_t>& goal_data,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

    std::future<std::shared_ptr<GoalHandle>> send_goal_async(
        const std::vector<uint8_t>& goal_data);

    bool cancel_goal(uint32_t goal_id);

    std::optional<ResultInfo> get_result(
        uint32_t goal_id,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

    void subscribe_feedback(FeedbackHandler handler);
    void unsubscribe_feedback();

    void on_goal_result(uint32_t goal_id, ActionStatus status,
                        const std::vector<uint8_t>& result_data);
    void on_feedback(uint32_t goal_id, float progress,
                     const std::vector<uint8_t>& feedback_data);

private:
    std::shared_ptr<msomeip::Application> app_;
    ActionClientConfig config_;
    std::atomic<bool> initialized_{false};

    std::mutex goal_handles_mutex_;
    std::unordered_map<uint32_t, std::shared_ptr<GoalHandle>> goal_handles_;

    FeedbackHandler feedback_handler_;
    std::mutex feedback_mutex_;

    uint32_t next_goal_id_{1};
};

}  // namespace maction

#endif  // MACTION_ACTION_CLIENT_H
