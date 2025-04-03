project(alarm_test)

cmake_minimum_required(VERSION 2.8)
include(./.cmake/flags.cmake)

enable_testing()
include(CTest)

# Do not build vtss_basics tests
option(BUILD_TESTS "Build tests" off)

# In order to enable debug output from tests do one of:
# VTSS_TRACE_LEVEL=INFO ./alarm_tests
# VTSS_TRACE_LEVEL=DEBUG ./alarm_tests
# VTSS_TRACE_LEVEL=NOISE ./alarm_tests
add_definitions(-DVTSS_BASICS_STANDALONE -DVTSS_TRACE_MODULE_ID=1)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../vtss_basics/
                 ${CMAKE_CURRENT_BINARY_DIR}/vtss_basics)

include_directories(${vtss_basics_SOURCE_DIR}/test)
include_directories(${vtss_basics_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/vtss_basics/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/vtss_basics/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../vtss_appl/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../vtss_api/include)
include_directories(..)

add_library(alarm
            ../alarm-element.cxx
            ../alarm-expose.cxx
            ../alarm-expression.cxx
            ../alarm-expression/any.cxx
            ../alarm-expression/json-parse.cxx
            ../alarm-expression/shunting-yard.cxx
            ../alarm-expression/token.cxx
            ../alarm-expression/tree-element-var.cxx
            ../alarm-expression/tree-element.cxx
            ../alarm-leaf.cxx
            ../alarm-node.cxx)
target_link_libraries(alarm vtss_basics)

add_executable(alarm_tests
               ${vtss_basics_SOURCE_DIR}/test/catch.cxx
               test1_pfh.cxx
#               alarm_expose_test.cxx
               alarm_leaf_test.cxx)

target_link_libraries(alarm_tests ${CMAKE_THREAD_LIBS_INIT} supc++ vtss_basics alarm)
add_test(NAME alarm_tests COMMAND alarm_tests)

