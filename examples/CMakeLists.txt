cmake_minimum_required(VERSION 3.15)
project(mcom_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Set up mcom library
set(MCOM_LIBRARY /mnt/data/workspace/moss/mcom/build/libmcom.a)
set(MCOM_INCLUDE_DIR /mnt/data/workspace/moss/mcom/include/mcom)
add_library(mcom STATIC IMPORTED)
set_target_properties(mcom PROPERTIES IMPORTED_LOCATION ${MCOM_LIBRARY})
set_target_properties(mcom PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${MCOM_INCLUDE_DIR}")

# Set up mshm library
set(MSHM_LIBRARY /mnt/data/workspace/moss/mshm/build/src/libmshm.a)
set(MSHM_INCLUDE_DIR /mnt/data/workspace/moss/mshm/include)
add_library(mshm STATIC IMPORTED)
set_target_properties(mshm PROPERTIES IMPORTED_LOCATION ${MSHM_LIBRARY})
set_target_properties(mshm PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${MSHM_INCLUDE_DIR}")

# Set up mdds library
set(MDDS_LIBRARY /mnt/data/workspace/moss/mdds/build/lib/libmdds.a)
set(MDDS_INCLUDE_DIR /mnt/data/workspace/moss/mdds/include)
add_library(mdds STATIC IMPORTED)
set_target_properties(mdds PROPERTIES IMPORTED_LOCATION ${MDDS_LIBRARY})
set_target_properties(mdds PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${MDDS_INCLUDE_DIR}")

add_executable(example_action_server example_action_server.cpp)
target_include_directories(example_action_server PRIVATE
    ${MCOM_INCLUDE_DIR}
    ${MSHM_INCLUDE_DIR}
    ${MDDS_INCLUDE_DIR}
)
target_link_libraries(example_action_server
    mcom
    mshm
    mdds
    Threads::Threads
)

add_executable(example_action_client example_action_client.cpp)
target_include_directories(example_action_client PRIVATE
    ${MCOM_INCLUDE_DIR}
    ${MSHM_INCLUDE_DIR}
    ${MDDS_INCLUDE_DIR}
)
target_link_libraries(example_action_client
    mcom
    mshm
    mdds
    Threads::Threads
)