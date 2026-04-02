#include "mcom/action/maction.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cstring>

using namespace moss::mcom::action;

std::vector<uint8_t> serialize_target(float x, float y, float z) {
    std::vector<uint8_t> data;
    data.resize(12);
    std::memcpy(data.data(), &x, sizeof(float));
    std::memcpy(data.data() + 4, &y, sizeof(float));
    std::memcpy(data.data() + 8, &z, sizeof(float));
    return data;
}

int main() {
    std::cout << "MAction Client Example\n";

    ActionClientConfig config;
    config.service_id = 0x5678;
    config.instance_id = 0x0001;
    config.method_id_base = 0x0001;

    auto client = std::make_shared<ActionClient>(config);

    client->subscribe_feedback(
        [](const FeedbackInfo& fb) {
            std::cout << "Feedback - Goal ID: " << fb.goal_id
                      << ", Progress: " << (fb.progress * 100) << "%\n";
        });

    client->init();

    std::cout << "Sending goal...\n";
    auto goal_handle = client->send_goal(serialize_target(1.0f, 2.0f, 3.0f));

    if (!goal_handle) {
        std::cerr << "Goal was rejected\n";
        return 1;
    }

    std::cout << "Goal ID: " << goal_handle->get_goal_id() << " accepted, waiting for completion...\n";

    auto start = std::chrono::steady_clock::now();
    while (!goal_handle->is_terminal()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed > 10) {
            std::cout << "Timeout waiting for goal completion\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (goal_handle->has_result()) {
        auto result = goal_handle->get_result();
        std::cout << "Goal completed with status: " << static_cast<int>(result.status) << "\n";
    }

    std::cout << "Done.\n";
    return 0;
}
