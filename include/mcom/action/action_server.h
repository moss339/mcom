#ifndef MACTION_ACTION_SERVER_H
#define MACTION_ACTION_SERVER_H

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

class ActionServer : public std::enable_shared_from_this<ActionServer> {
public:
    ActionServer(std::shared_ptr<msomeip::Application> app,
                 ActionServerConfig config);
    ~ActionServer();

    ActionServer(const ActionServer&) = delete;
    ActionServer& operator=(const ActionServer&) = delete;

    void init();

    void register_goal_handler(GoalHandler handler);
    void register_cancel_handler(CancelHandler handler);

    void start();
    void stop();

    void publish_feedback(uint32_t goal_id, float progress,
                          const std::vector<uint8_t>& feedback_data);
    void publish_status_update(uint32_t goal_id, ActionStatus status);
    void send_result(uint32_t goal_id, ActionStatus status,
                     const std::vector<uint8_t>& result_data);

    void accept_goal(uint32_t goal_id);
    void reject_goal(uint32_t goal_id);

    void on_send_goal(uint32_t goal_id, const std::vector<uint8_t>& goal_data);
    void on_cancel_goal(uint32_t goal_id);

private:
    std::shared_ptr<msomeip::Application> app_;
    ActionServerConfig config_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};

    GoalHandler goal_handler_;
    CancelHandler cancel_handler_;
    std::mutex handlers_mutex_;

    std::mutex goal_handles_mutex_;
    std::unordered_map<uint32_t, std::shared_ptr<GoalHandle>> goal_handles_;

    uint32_t next_goal_id_{1};
    std::mutex goal_id_mutex_;
};

}  // namespace maction

#endif  // MACTION_ACTION_SERVER_H
