#
# Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.
#
# Unpublished rights reserved under the copyright laws of the United States of
# America, other countries and international treaties. Permission to use, copy,
# store and modify, the software and its source code is granted but only in
# connection with products utilizing the Microsemi switch and PHY products.
# Permission is also granted for you to integrate into other products, disclose,
# transmit and distribute the software only in an absolute machine readable
# format (e.g. HEX file) and only in or with products utilizing the Microsemi
# switch and PHY products.  The source code of the software may not be
# disclosed, transmitted or distributed without the prior written permission of
# Microsemi.
#
# This copyright notice must appear in any copy, modification, disclosure,
# transmission or distribution of the software.  Microsemi retains all
# ownership, copyright, trade secret and proprietary rights in the software and
# its source code, including all modifications thereto.
#
# THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
# WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
# ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
# NON-INFRINGEMENT.
#

if (${PROJECT_NAME} STREQUAL ${CMAKE_PROJECT_NAME})
option(ENABLE_COVERAGE "Enable coverage" off)

option(ENABLE_ADDRESS_SANATIZE "Enable address sanatizer" on)

IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

string (REPLACE " -" ";-" CXX_FLAGS        "${CMAKE_CXX_FLAGS}")
string (REPLACE " -" ";-" C_FLAGS          "${CMAKE_C_FLAGS}")
string (REPLACE " -" ";-" EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

LIST(APPEND C_FLAGS   "-Wall")
LIST(APPEND CXX_FLAGS "-Wall")
LIST(APPEND CXX_FLAGS "-Werror")

#LIST(APPEND CXX_FLAGS "-fno-rtti")
#LIST(APPEND CXX_FLAGS "-fno-exceptions")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    #LIST(APPEND CXX_FLAGS "-stdlib=libc++")
    LIST(APPEND CXX_FLAGS "-std=c++17")
    LIST(APPEND CXX_FLAGS "-stdlib=libstdc++")
    LIST(APPEND CXX_FLAGS "-Wno-invalid-constexpr")
else()
    LIST(APPEND CXX_FLAGS "-std=c++17")
endif()

if (${ENABLE_COVERAGE})
    LIST(APPEND CXX_FLAGS        "--coverage")
    LIST(APPEND C_FLAGS          "--coverage")
    LIST(APPEND EXE_LINKER_FLAGS "--coverage")
endif()

if (${ENABLE_ADDRESS_SANATIZE})
    LIST(APPEND CXX_FLAGS "-fsanitize=address")
    LIST(APPEND CXX_FLAGS "-fno-omit-frame-pointer")
    LIST(APPEND EXE_LINKER_FLAGS "-fsanitize=address")
endif()

list(REMOVE_DUPLICATES CXX_FLAGS)
list(REMOVE_DUPLICATES C_FLAGS)
list(REMOVE_DUPLICATES EXE_LINKER_FLAGS)

string (REPLACE ";-" " -" CXX_FLAGS        "${CXX_FLAGS}")
string (REPLACE ";-" " -" C_FLAGS          "${C_FLAGS}")
string (REPLACE ";-" " -" EXE_LINKER_FLAGS "${EXE_LINKER_FLAGS}")

SET(CMAKE_CXX_FLAGS        "${CXX_FLAGS}")
SET(CMAKE_C_FLAGS          "${C_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "${EXE_LINKER_FLAGS}")

message(STATUS "Project name = ${PROJECT_NAME}")
message(STATUS "  Branch     = ${${PROJECT_NAME}_BRANCH_NAME}")
message(STATUS "  Type       = ${CMAKE_BUILD_TYPE}")
message(STATUS "  cxx_flags  = ${CMAKE_CXX_FLAGS}")
message(STATUS "  c_flags    = ${CMAKE_C_FLAGS}")
endif()
