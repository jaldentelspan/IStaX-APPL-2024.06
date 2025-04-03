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

#ifndef _VTSS_TRACE_H_
#define _VTSS_TRACE_H_

#include <stdio.h>
#include <string.h>

#include "vtss_trace_lvl_api.h"
#include "vtss_trace_api.h"
#include "vtss_trace_io.h"
#include "vtss_os_wrapper.h"
#include <vtss/appl/module_id.h>

#define HAS_FLAGS(p, f) ((p->flags & (f)) == f)

/* Semaphore for IO registrations */
extern vtss_sem_t trace_io_crit;
extern vtss_sem_t trace_rb_crit;

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_TRACE

#define TRACE_GRP_DEFAULT 0

#include <vtss_trace_api.h>
/* ============== */

#ifndef TRACE_ASSERT
#define TRACE_ASSERT(expr, msg) { \
    if (!(expr)) { \
        T_E("ASSERTION FAILED"); \
        T_E msg; \
        VTSS_ASSERT(expr); \
    } \
}
#endif

#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))

/* Max size of Error message (size of buffer used to hand the message to syslog_flash_log) */
/* Make it large enough! */
#define TRACE_ERR_BUF_SIZE 4096

/******************************************************************************/
// trace_conf_load()
/******************************************************************************/
mesa_rc trace_conf_load(void);

/******************************************************************************/
// trace_conf_save()
/******************************************************************************/
mesa_rc trace_conf_save(vtss_trace_reg_t **trace_regs, size_t size);

/******************************************************************************/
// trace_conf_erase()
/******************************************************************************/
mesa_rc trace_conf_erase(void);

/******************************************************************************/
// trace_conf_apply()
/******************************************************************************/
mesa_rc trace_conf_apply(vtss_trace_reg_t **trace_regs, size_t size, vtss_module_id_t module_id = -1);

#endif /* _VTSS_TRACE_H_ */

