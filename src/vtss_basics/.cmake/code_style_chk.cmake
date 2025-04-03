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
set(STYLE_EXECUTABLE "${PROJECT_SOURCE_DIR}/.cmake/cpplint.py")

set(code_style_chk_cnt "0" CACHE INTERNAL "")

if(NOT TARGET code_style_chk)
    add_custom_target(code_style_chk)
endif()

add_custom_target(code_style_chk_${PROJECT_NAME})
add_dependencies(code_style_chk code_style_chk_${PROJECT_NAME})

function(add_style_chk)
    MATH(EXPR tmp "${code_style_chk_cnt} + 1" )
    set(code_style_chk_cnt "${tmp}" CACHE INTERNAL "")
    set(TARGET "${PROJECT_NAME}_code_style_chk_${code_style_chk_cnt}")
    add_custom_target(${TARGET}
        COMMAND ${STYLE_EXECUTABLE} ${STYLE_USER_FLAGS} ${ARGN})
    add_dependencies(code_style_chk_${PROJECT_NAME} ${TARGET})
endfunction(add_style_chk)

function(add_style_chk_glob)
    set(files "")
    foreach(exp ${ARGN})
        file(GLOB f ${exp})
        list(APPEND files ${f})
    endforeach()
    add_style_chk(${files})
endfunction()

function(add_style_chk_globr)
    set(files "")
    foreach(exp ${ARGN})
        file(GLOB_RECURSE f ${exp})
        list(APPEND files ${f})
    endforeach()
    add_style_chk(${files})
endfunction()

