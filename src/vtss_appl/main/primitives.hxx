/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable
 format (e.g. HEX file) and only in or with products utilizing the Microsemi
 switch and PHY products.  The source code of the software may not be
 disclosed, transmitted or distributed without the prior written permission of
 Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all
 ownership, copyright, trade secret and proprietary rights in the software and
 its source code, including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
 WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
 ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
 NON-INFRINGEMENT.

*/

#ifndef _VTSS_APPL_MAIN_PRIMITIVES_
#define _VTSS_APPL_MAIN_PRIMITIVES_

#include <stdint.h>
#include <vtss/appl/module_id.h>

/* - Assertions ----------------------------------------------------- */
/* Allow for assertion-is-going-to-fail callback. */
typedef void (*vtss_common_assert_cb_t)(const char *file_name,
                                        const unsigned long line_num,
                                        const char *msg);
extern vtss_common_assert_cb_t vtss_common_assert_cb;
void vtss_common_assert_cb_set(vtss_common_assert_cb_t cb);
extern "C" void control_system_assert_do_reset(void) __attribute__ ((noreturn));

/* Note, that the vtss_common_assert_cb() function will be called whether or
   not the image is compiled assertions enabled. */
#define VTSS_COMMON_ASSERT_CB() {                                      \
    if(vtss_common_assert_cb)                                          \
        vtss_common_assert_cb(__FILE__, __LINE__, "Assertion failed"); \
}

/* Trying hard to avoid expr to be evaluated twice (which may cause side effects
 * if it's a function call for instance, or a register read with
 * read-side-effects). */
#ifndef VTSS_ASSERT
#define VTSS_ASSERT(expr) {                                        \
    if (!(expr)) {                                                 \
        T_EXPLICIT(VTSS_MODULE_ID_MAIN, 0, VTSS_TRACE_LVL_ERROR,   \
                   __FILE__, __LINE__, "ASSERTION FAILED");        \
        VTSS_COMMON_ASSERT_CB(); control_system_assert_do_reset(); \
    }                                                              \
}
#endif

#endif  // _VTSS_APPL_MAIN_PRIMITIVES_
