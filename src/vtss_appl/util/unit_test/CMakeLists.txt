cmake_minimum_required(VERSION 2.8)

project (util_unit_test)

enable_testing()

find_package(Threads REQUIRED)
add_definitions(-std=c++17 -Wall)
add_definitions(-DVTSS_SWITCH_STANDALONE=1 -DVTSS_OPT_PORT_COUNT=12)

include_directories(../../main)
include_directories(../../../vtss_appl/include)
include_directories(../../../vtss_api/include)
include_directories(../../../vtss_api/boards)

set(VTSS_USE_API_HEADERS on CACHE STRING "Use VTSS-Unified-API header files")
set(VTSS_API_HEADERS_IN_TREE on CACHE STRING "Has VTSS-Unified-API in-tree")
add_subdirectory(../../../vtss_basics vtss_basics)
include_directories(${vtss_basics_BINARY_DIR}/include)
include_directories(${vtss_basics_SOURCE_DIR}/include)

add_executable(test_port_list port_list.cxx ../vtss_appl_types.cxx
    ../../misc/interface.c)

target_link_libraries(test_port_list gtest_main ${CMAKE_THREAD_LIBS_INIT}
                      supc++ vtss_basics)
add_test(NAME test_port_list COMMAND test_port_list)


