cmake_minimum_required(VERSION 3.14)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

project(mcom VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

# Use local builds with relative paths
set(mshm_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../mshm" CACHE PATH "mshm root")
set(mdds_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../mdds" CACHE PATH "mdds root")
set(mruntime_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../mruntime" CACHE PATH "mruntime root")

# Proto include path
set(PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../proto" CACHE PATH "proto root")

# Import mshm library
add_library(mshm::mshm SHARED IMPORTED)
set_target_properties(mshm::mshm PROPERTIES
    IMPORTED_LOCATION "${mshm_ROOT}/build/src/libmshm.so"
    INTERFACE_INCLUDE_DIRECTORIES "${mshm_ROOT}/include"
)

# Import mdds library
add_library(mdds::mdds SHARED IMPORTED)
set_target_properties(mdds::mdds PROPERTIES
    IMPORTED_LOCATION "${mdds_ROOT}/build/src/libmdds.so"
    INTERFACE_INCLUDE_DIRECTORIES "${mdds_ROOT}/include"
)

# Import mruntime library
add_library(mruntime::mruntime SHARED IMPORTED)
set_target_properties(mruntime::mruntime PROPERTIES
    IMPORTED_LOCATION "${mruntime_ROOT}/build/src/libmruntime.so"
    INTERFACE_INCLUDE_DIRECTORIES "${mruntime_ROOT}/include"
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${mshm_ROOT}/include
    ${mdds_ROOT}/include
    ${mruntime_ROOT}/include
    ${PROTO_ROOT}/include
)

# Source files
set(TOPIC_SOURCES
    src/topic/topic_manager.cpp
)

set(SERVICE_SOURCES
    src/service/service_client.cpp
    src/service/service_server.cpp
    src/service/service_manager.cpp
    src/service/proto_service_client.cpp
    src/service/proto_service_server.cpp
)

set(ACTION_SOURCES
    src/action/action_client.cpp
    src/action/action_server.cpp
    src/action/goal_handle.cpp
)

set(NODE_SOURCES
    src/node/node.cpp
)

if(BUILD_SHARED_LIBS)
    add_library(mcom SHARED
        ${TOPIC_SOURCES}
        ${SERVICE_SOURCES}
        ${ACTION_SOURCES}
        ${NODE_SOURCES}
    )
    set_target_properties(mcom PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        POSITION_INDEPENDENT_CODE ON
    )
else()
    add_library(mcom STATIC
        ${TOPIC_SOURCES}
        ${SERVICE_SOURCES}
        ${ACTION_SOURCES}
        ${NODE_SOURCES}
    )
    set_target_properties(mcom PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
endif()

target_link_libraries(mcom
    Threads::Threads
    mshm::mshm
    mdds::mdds
    mruntime::mruntime
    protobuf::libprotobuf
)

target_include_directories(mcom PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS mcom
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION include)

# Examples
option(MCOM_BUILD_EXAMPLES "Build mcom examples" OFF)
if(MCOM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
