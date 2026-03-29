add_library(maction STATIC
    goal_handle.cpp
    action_client.cpp
    action_server.cpp
)

target_link_libraries(maction
    pthread
    /mnt/data/workspace/moss/msomeip/build/libmsomeip.a
    /mnt/data/workspace/moss/mservice/build/libmservice.so
    dl
)

target_include_directories(maction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    /mnt/data/workspace/moss/msomeip/include
    /mnt/data/workspace/moss/mservice/include
)
