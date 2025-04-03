project(main_module_conf_test)

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
add_definitions(-DVTSS_BASICS_STANDALONE -DVTSS_TRACE_MODULE_ID=1 -DMAIN_CONF_FILE="../switch.conf")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../vtss_basics/
                 ${CMAKE_CURRENT_BINARY_DIR}/vtss_basics)

include_directories(${vtss_basics_SOURCE_DIR}/test)
include_directories(${vtss_basics_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/vtss_basics/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/vtss_basics/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../vtss_appl/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../vtss_api/include)
include_directories(..)

add_library(main_conf
            ../main_conf.cxx)

target_link_libraries(main_conf vtss_basics)

add_executable(main_conf_tests
               ${vtss_basics_SOURCE_DIR}/test/catch.cxx
               main_conf_test.cxx)

target_link_libraries(main_conf_tests ${CMAKE_THREAD_LIBS_INIT} supc++ vtss_basics main_conf)
add_test(NAME main_conf_tests COMMAND main_conf_tests)

