/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_WEB_H_
#define _VTSS_WEB_H_

#include "msg_api.h"
#include "sysutil_api.h"
#include "mgmt_api.h"

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#include "misc_api.h"
#endif

#ifdef VTSS_SW_OPTION_LIBFETCH
#include "fetch.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_WEB

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB

#include <vtss_trace_api.h>
/* ============== */

#ifdef __cplusplus
extern "C" {
#endif

// Set WEB_CACHE_STATIC_FILES to 1 if you want to cache the static files in memory
// This is really not neccessary as browsers will cache the static files locally.
// Static files are currently config.js and filter.css.
#define WEB_CACHE_STATIC_FILES 0
#define WEB_BUF_LEN 1024

#define ARG_NAME_MAXLEN 64

#define HTTPD_WATCH_DOG_ENABLED 1

#define VTSS_HTTPD_HEADER_END_MARKER          "\r\n\r\n"

#ifndef MIN
#define MIN(_x_, _y_) ((_x_) < (_y_) ? (_x_) : (_y_))
#endif

#define XSTR(s) STR(s)
#define STR(s) #s

/*
 * String helpers
 */

static inline bool str_noteol(const char *start, const char *end)
{
    return start && start < end && *start && *start != '\r' && *start != '\n';
}

static inline const char *str_nextline(const char *p, const char *end)
{
    //diag_printf("%s: start\n", __FUNCTION__);
    while(p && p < end && (p = (const char *)memchr(p, '\r', end - p))) {
        if((end-p) > 1 && p[1] == '\n')
            return p+2;
        p++;                    /* Advance past \r */
    }
    return NULL;
}

mesa_rc web_init_os(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_WEB_H_ */

