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
    std::cout << "MAction Server Example\n";

    ActionServerConfig config;
    config.service_id = 0x5678;
    config.instance_id = 0x0001;
    config.method_id_base = 0x0001;

    ActionServerPtr server = std::make_shared<ActionServer>(config);

    server->register_goal_handler(
        [](const GoalInfo& goal) -> std::tuple<bool, ActionStatus, std::vector<uint8_t>> {
            std::cout << "Received goal ID: " << goal.goal_id << ", data size: " << goal.goal_data.size() << "\n";

            for (int i = 0; i <= 10; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            std::vector<uint8_t> result = serialize_target(1.0f, 2.0f, 3.0f);
            std::cout << "Goal " << goal.goal_id << " succeeded\n";
            return {true, ActionStatus::SUCCEEDED, result};
        });

    server->register_cancel_handler(
        [](uint32_t goal_id) -> bool {
            std::cout << "Cancel requested for goal: " << goal_id << "\n";
            return true;
        });

    server->init();
    server->start();

    std::cout << "Action server started. Press Ctrl+C to stop.\n";

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
