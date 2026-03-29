cmake_minimum_required(VERSION 3.15)
project(mcom VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Threads REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mshm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../mdds/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../msomeip/include
)

# Source files
set(TOPIC_SOURCES
    src/topic/topic_manager.cpp
)

set(SERVICE_SOURCES
    src/service/service_manager.cpp
)

set(ACTION_SOURCES
    src/action/action_client.cpp
    src/action/action_server.cpp
    src/action/goal_handle.cpp
)

set(NODE_SOURCES
    src/node/node.cpp
)

add_library(mcom STATIC
    ${TOPIC_SOURCES}
    ${SERVICE_SOURCES}
    ${ACTION_SOURCES}
    ${NODE_SOURCES}
)

target_link_libraries(mcom
    Threads::Threads
    mshm
    mdds
)

target_include_directories(mcom PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS mcom
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
