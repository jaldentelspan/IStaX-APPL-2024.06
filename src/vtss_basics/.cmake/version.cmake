#
# Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.
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
execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/.cmake/version.sh" ${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)

include(${CMAKE_CURRENT_BINARY_DIR}/buildinfo_${PROJECT_NAME}.cmake)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(reruntarget_${PROJECT_NAME} ALL)
add_custom_command(OUTPUTS "file-which-does-not-exists"
                   COMMAND "touch" ${CMAKE_CURRENT_BINARY_DIR}/buildinfo_${PROJECT_NAME}.cmake
                   TARGET  reruntarget_${PROJECT_NAME})


# Offer the user the choice of overriding the installation directories
set(PROJECT_VERSION ${${PROJECT_NAME}_VERSION1})
set(PROJECT_VERSION_FULL ${${PROJECT_NAME}_VERSION})
set(PROJECT_VNAME ${PROJECT_NAME}${${PROJECT_NAME}_VERSION1})

set(INSTALL_LIB_DIR     "lib" CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR     "bin" CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR "include" CACHE PATH "Installation directory for header files")
set(INSTALL_DATA_DIR    "share/${PROJECT_VNAME}" CACHE PATH "Installation directory for data files")
set(INSTALL_CMAKE_DIR   "${INSTALL_DATA_DIR}/cmake" CACHE PATH "Installation directory for cmake files")

