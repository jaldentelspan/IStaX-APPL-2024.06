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

#ifndef _VTSS_IP_SOURCE_GUARD_H_
#define _VTSS_IP_SOURCE_GUARD_H_


/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#include "ip_source_guard_api.h"
#include "vtss_module_id.h"

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#include "acl_api.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_IP_SOURCE_GUARD

#include <vtss_trace_api.h>

/* IP_SOURCE_GUARD ACE IDs */
#define IP_SOURCE_GUARD_DEFAULT_ACE_ID(isid, port_no) (((isid) - VTSS_ISID_START) * fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + (port_no) - VTSS_PORT_NO_START + 1)

/* ================================================================= *
 *  IP_SOURCE_GUARD global structure
 * ================================================================= */

/* IP_SOURCE_GUARD global structure */
typedef struct {
    critd_t                         crit;
    ip_source_guard_conf_t          ip_source_guard_conf;
    ip_source_guard_entry_t         ip_source_guard_dynamic_entry[IP_SOURCE_GUARD_MAX_ENTRY_CNT];
    CapArray<BOOL, VTSS_APPL_CAP_ACL_ACE_CNT> id_used;
} ip_source_guard_global_t;

#endif /* _VTSS_IP_SOURCE_GUARD_H_ */

