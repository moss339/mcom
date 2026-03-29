add_executable(test_maction test_action.cpp)

target_link_libraries(test_maction
    maction
    pthread
)

add_test(NAME maction_test COMMAND test_maction)
