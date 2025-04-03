include_directories(.)
include_directories(${SnmpTest_SOURCE_DIR}/include)

add_library(example_lib SHARED test.cxx)

add_executable(mib_gen mib_gen.cxx)
target_link_libraries(mib_gen vtss_basics example_lib)

add_executable(subagent subagent.cxx)
target_link_libraries(subagent vtss_basics example_lib)

add_executable(trace_example trace.cxx)
target_link_libraries(trace_example vtss_basics)

add_executable(list-example list-example.cxx)
target_link_libraries(list-example vtss_basics)


add_executable(vector-exercise
               vector-exercise-main.cxx
               vector-exercise-test.cxx)
target_link_libraries(vector-exercise vtss_basics)

add_definitions(-DCHILD_TEST="${CMAKE_CURRENT_BINARY_DIR}/child_test")

add_executable(process process.cxx)
target_link_libraries(process vtss_basics)

add_executable(process-daemon process-daemon.cxx)
target_link_libraries(process-daemon vtss_basics)

add_executable(process-cmd process-cmd.cxx)
target_link_libraries(process-cmd vtss_basics)

add_executable(child_test child_test.cxx)

add_executable(print_fmt print_fmt.cxx)
target_link_libraries(print_fmt vtss_basics)
