include_directories(${vtss_basics_SOURCE_DIR}/test)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../vtss_appl/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../vtss_api/include)
include_directories(..)

add_library(frr ../frr_daemon.cxx ../frr_ip_route.cxx ../ospf/frr_ospf_access.cxx ../rip/frr_rip_access.cxx ../router/frr_router_access.cxx)
target_link_libraries(frr vtss_basics)

add_executable(frr_tests
               ${vtss_basics_SOURCE_DIR}/test/catch.cxx
               frr_access_test.cxx frr_ospf_access_test.cxx frr_rip_access_test.cxx frr_router_access_test.cxx)

target_link_libraries(frr_tests ${CMAKE_THREAD_LIBS_INIT} supc++ vtss_basics frr)
add_test(NAME frr_tests COMMAND frr_tests)

