#include "mcom/action/maction.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace moss::mcom::action;

void test_goal_handle_creation() {
    auto handle = std::make_shared<GoalHandle>(123);
    assert(handle->get_goal_id() == 123);
    assert(handle->get_status() == ActionStatus::READY);
    assert(!handle->is_terminal());
    assert(!handle->cancel_requested());
    std::cout << "test_goal_handle_creation passed\n";
}

void test_goal_handle_cancel() {
    auto handle = std::make_shared<GoalHandle>(456);
    assert(!handle->cancel_requested());
    handle->request_cancel();
    assert(handle->cancel_requested());
    std::cout << "test_goal_handle_cancel passed\n";
}

void test_goal_handle_result() {
    auto handle = std::make_shared<GoalHandle>(789);
    assert(!handle->has_result());

    std::vector<uint8_t> result_data = {1, 2, 3, 4};
    handle->set_result(ActionStatus::SUCCEEDED, result_data);

    assert(handle->has_result());
    auto result = handle->get_result();
    assert(result.goal_id == 789);
    assert(result.status == ActionStatus::SUCCEEDED);
    assert(result.result_data == result_data);
    std::cout << "test_goal_handle_result passed\n";
}

void test_goal_handle_terminal_states() {
    auto succeeded = std::make_shared<GoalHandle>(1);
    succeeded->set_result(ActionStatus::SUCCEEDED, {});
    assert(succeeded->is_terminal());

    auto aborted = std::make_shared<GoalHandle>(2);
    aborted->set_result(ActionStatus::ABORTED, {});
    assert(aborted->is_terminal());

    auto canceled = std::make_shared<GoalHandle>(3);
    canceled->set_result(ActionStatus::CANCELED, {});
    assert(canceled->is_terminal());

    auto executing = std::make_shared<GoalHandle>(4);
    executing->set_status(ActionStatus::EXECUTING);
    assert(!executing->is_terminal());

    std::cout << "test_goal_handle_terminal_states passed\n";
}

void test_action_status_enum() {
    assert(static_cast<uint8_t>(ActionStatus::READY) == 0x08);
    assert(static_cast<uint8_t>(ActionStatus::EXECUTING) == 0x06);
    assert(static_cast<uint8_t>(ActionStatus::SUCCEEDED) == 0x01);
    assert(static_cast<uint8_t>(ActionStatus::CANCELED) == 0x02);
    std::cout << "test_action_status_enum passed\n";
}

int main() {
    test_goal_handle_creation();
    test_goal_handle_cancel();
    test_goal_handle_result();
    test_goal_handle_terminal_states();
    test_action_status_enum();

    std::cout << "All maction tests passed!\n";
    return 0;
}
