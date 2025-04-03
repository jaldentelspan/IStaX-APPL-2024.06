project (vtss_basics)

cmake_minimum_required(VERSION 2.8)
include(.cmake/common.cmake)

SET (VERSION_MAJOR 0)
SET (VERSION_MINOR 0)
SET (VERSION_PATCH 0)

option(BUILD_TESTS "Build tests" on)

add_definitions(-DVTSS_BASICS_STANDALONE -DVTSS_TRACE_MODULE_ID=1)
add_definitions(-DVTSS_SNMP_HEADLINE_DESC="Microsemi ENT private MIB")

# VTSS-API header source #######################################################
find_package(vtss_api QUIET)
if (${vtss_api_FOUND})
    option(VTSS_USE_API_HEADERS     "Use VTSS-Unified-API header files" on)
else()
    option(VTSS_USE_API_HEADERS     "Use VTSS-Unified-API header files" off)
endif()
option(VTSS_API_HEADERS_IN_TREE "Has VTSS-Unified-API in-tree" off)

if (${VTSS_USE_API_HEADERS})
    if (${VTSS_API_HEADERS_IN_TREE})
        MESSAGE(STATUS "BASICS: Use in-tree version of API headers")
    else()
        MESSAGE(STATUS "BASICS: Use system installation of API headers")
        find_package(vtss_api REQUIRED)
        include_directories(${vtss_api_INCLUDE_DIRS})

    endif()

else()
    MESSAGE(STATUS "BASICS: Use hardcoded representation of API types")

endif()
################################################################################



set(STYLE_USER_FLAGS "--filter=-runtime/references")
set(STYLE_USER_FLAGS "${STYLE_USER_FLAGS},-build/include_what_you_use")
set(STYLE_USER_FLAGS "${STYLE_USER_FLAGS},-build/include")


# External packets #############################
find_package(Threads REQUIRED)
################################################

# Write defines to a config file
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/include/vtss/basics/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/vtss/basics/config.h"
    ESCAPE_QUOTES)

add_style_chk_globr("./src/*.cxx" "./src/*.hxx" "./src/*.h"
                    "include/*.hxx" "include/*.h")

include_directories(include)
include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")
set(src
    src/arithmetic-overflow.cxx
    src/assert.cxx
    src/enum-utils.cxx
    src/expose/json/char-encode.cxx
    src/expose/json/enum-exporter.cxx
    src/expose/json/enum-loader.cxx
    src/expose/json/exporter.cxx
    src/expose/json/function-exporter-abstract.cxx
    src/expose/json/handler-function-exporter-serializer.cxx
    src/expose/json/json-array-ptr.cxx
    src/expose/json/json-core-type.cxx
    src/expose/json/json-encoded-data.cxx
    src/expose/json/json-value-ptr.cxx
    src/expose/json/json-value.cxx
    src/expose/json/literal.cxx
    src/expose/json/loader.cxx
    src/expose/json/method-split.cxx
    src/expose/json/namespace-node.cxx
    src/expose/json/node.cxx
    src/expose/json/notification-base.cxx
    src/expose/json/notification-row.cxx
    src/expose/json/parse-and-compare-no-qoutes.cxx
    src/expose/json/parse-and-compare.cxx
    src/expose/json/response-base.cxx
    src/expose/json/response-parser-base.cxx
    src/expose/json/root-node.cxx
    src/expose/json/serialize-enum.cxx
    src/expose/json/serialize.cxx
    src/expose/json/skip-array.cxx
    src/expose/json/skip-number.cxx
    src/expose/json/skip-object.cxx
    src/expose/json/skip-value.cxx
    src/expose/json/specification/indent.cxx
    src/expose/json/specification/inventory.cxx
    src/expose/json/specification/reflector-echo.cxx
    src/expose/json/specification/type-class.cxx
    src/expose/json/specification/walk.cxx
    src/expose/json/string-decoder-no-qoutes.cxx
    src/expose/json/string-decoder.cxx
    src/expose/json/string-encode-no-qoutes.cxx
    src/expose/json/string-encode.cxx
    src/expose/json/string-literal.cxx
    src/expose/snmp/asn-type.cxx
    src/expose/snmp/error_code.cxx
    src/expose/snmp/globals.cxx
    src/expose/snmp/handler-state.cxx
    src/expose/snmp/handlers.cxx
    src/expose/snmp/handlers/get.cxx
    src/expose/snmp/handlers/getset.cxx
    src/expose/snmp/handlers/mib_generator.cxx
    src/expose/snmp/handlers/netsnmp_pass_persist/initial_tree_walker.cxx
    src/expose/snmp/handlers/netsnmp_pass_persist/set.cxx
    src/expose/snmp/handlers/oid_handler.cxx
    src/expose/snmp/handlers/reflector.cxx
    src/expose/snmp/handlers/set.cxx
    src/expose/snmp/handlers/trap-serialize.cxx
    src/expose/snmp/handlers/trap.cxx
    src/expose/snmp/handlers_netsnmp_pass_persist.cxx
    src/expose/snmp/handlers_oid.cxx
    src/expose/snmp/iterator-oid-base.cxx
    src/expose/snmp/oid_inventory.cxx
    src/expose/snmp/oid_sequence.cxx
    src/expose/snmp/types.cxx
    src/fd.cxx
    src/http-message-stream-parser.cxx
    src/http-too-simple-client.cxx
    src/i128.cxx
    src/json-rpc-function.cxx
    src/json-rpc-server.cxx
    src/json/stream-parser.cxx
    src/list-node-base.cxx
    src/map-memory-fixed.cxx
    src/map-memory-static.cxx
    src/measure_scope.cxx
    src/mutex.cxx
    src/notifications/event-fd.cxx
    src/notifications/event-handler.cxx
    src/notifications/event-type.cxx
    src/notifications/event.cxx
    src/notifications/lock-global-subject.cxx
    src/notifications/process-cmd.cxx
    src/notifications/process-cmd-pty.cxx
    src/notifications/process-daemon.cxx
    src/notifications/process.cxx
    src/notifications/subject-base.cxx
    src/notifications/subject-runner.cxx
    src/notifications/subject-runner-event.cxx
    src/notifications/table-observer-common.cxx
    src/notifications/timer.cxx
    src/parse.cxx
    src/parse_group.cxx
    src/parser_impl.cxx
    src/print_fmt.cxx
    src/print_fmt_extra.cxx
    src/rbtree-base.cxx
    src/rbtree-stl.cxx
    src/stream.cxx
    src/string-utils.cxx
    src/string.cxx
    src/trace_linux.cxx
    src/time_unit.cxx
    src/types.cxx
    src/vector-memory.cxx
)

add_library(vtss_basics SHARED ${src})
target_link_libraries(vtss_basics rt netsnmpagent netsnmp ${CMAKE_THREAD_LIBS_INIT})

file(GLOB_RECURSE API_INCLUDE
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hxx"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
list(APPEND API_INCLUDE
    "${CMAKE_CURRENT_BINARY_DIR}/include/vtss/basics/config.h")
set_target_properties(vtss_basics PROPERTIES VERSION ${vtss_basics_VERSION1} SOVERSION ${vtss_basics_VERSION1})
set_target_properties(vtss_basics PROPERTIES PUBLIC_HEADER "${API_INCLUDE}")

install(TARGETS       vtss_basics
        EXPORT        ${PROJECT_NAME}Depends
        LIBRARY       DESTINATION "${INSTALL_LIB_DIR}"
        PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}/vtss/basics")
install(EXPORT        ${PROJECT_NAME}Depends
        DESTINATION   ${INSTALL_CMAKE_DIR} COMPONENT dev)

configure_file(".cmake/${PROJECT_NAME}-config.cmake.in"
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}-config.cmake" @ONLY)
configure_file(".cmake/${PROJECT_NAME}-config-version.cmake.in"
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}-config-version.cmake" @ONLY)
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}-config.cmake"
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${PROJECT_NAME}-config-version.cmake"
  DESTINATION ${INSTALL_CMAKE_DIR} COMPONENT dev)

if (${BUILD_TESTS})
    enable_testing()
    include(CTest)
    add_subdirectory(test)
    add_subdirectory(examples)

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../vtss_appl/frr/unittest
        ${CMAKE_CURRENT_BINARY_DIR}/frr)

endif()

