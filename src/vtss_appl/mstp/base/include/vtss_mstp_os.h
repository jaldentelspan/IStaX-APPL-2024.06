/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_MSTP_OS_H_
#define _VTSS_MSTP_OS_H_

#include <stdlib.h>             /* abort() */
#include <stdio.h>              /* For snprintf() */
#include <string.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_RSTP

#include "vtss_trace_lvl_api.h"
#include "vtss_trace_api.h"

#define VTSS_ABORT() abort()

#elif defined(__LINUX__)

#include <stdlib.h>
#include <stdio.h>              /* For snprintf() */
#include <string.h>

typedef enum {
    MSTP_TRACE_RACKET,
    MSTP_TRACE_NOISE,
    MSTP_TRACE_DEBUG,
    MSTP_TRACE_INFO,
    MSTP_TRACE_WARNING,
    MSTP_TRACE_ERROR,
} mstp_errlevel_t;

extern mstp_errlevel_t trace_level;

#define T_E(fmt, ...) MSTP_TRACE(MSTP_TRACE_ERROR,   fmt, ##__VA_ARGS__)
#define T_W(fmt, ...) MSTP_TRACE(MSTP_TRACE_WARNING, fmt, ##__VA_ARGS__)
#define T_I(fmt, ...) MSTP_TRACE(MSTP_TRACE_INFO,    fmt, ##__VA_ARGS__)
#define T_D(fmt, ...) MSTP_TRACE(MSTP_TRACE_DEBUG,   fmt, ##__VA_ARGS__)
#define T_N(fmt, ...) MSTP_TRACE(MSTP_TRACE_NOISE,   fmt, ##__VA_ARGS__)
#define T_R(fmt, ...) MSTP_TRACE(MSTP_TRACE_RACKET,  fmt, ##__VA_ARGS__)
#define TRACE_UNLIKELY(c) __builtin_expect(c,0)
#define TRACE_LIKELY(c)   __builtin_expect(c,1)
/*lint -emacro(522, MSTP_TRACE) */
#define MSTP_TRACE(lvl, fmt, ...)                                       \
    ({                                                                  \
        /*lint -e{506, 730} */                                          \
        if(TRACE_UNLIKELY(lvl >= trace_level))                          \
            mstp_trace(lvl,  __FILE__, __LINE__, fmt, ##__VA_ARGS__);   \
     })

void mstp_trace(mstp_errlevel_t lvl, const char *location, int line_no, const char *fmt, ...) __attribute__ ((format (printf, 4, 5)));

#define VTSS_ABORT()   abort()
#define VTSS_ASSERT(x) do { if(!(x)) { T_E("Assertion failed: %s", #x); VTSS_ABORT(); } } while(0)

#endif /* _VTSS_MSTP_OS_H_ */

