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

#ifndef _DHCP_SNOOPING_H_
#define _DHCP_SNOOPING_H_

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#include "dhcp_snooping_api.h"
#include "vtss_module_id.h"

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_DHCP_SNOOPING

#include <vtss_trace_api.h>

/* DHCP snooping configuration timer */
#define DHCP_SNOOPING_CONF_TIMER     1000

/* ================================================================= *
 *  DHCP snooping global structure
 * ================================================================= */

/* DHCP snooping global structure */
typedef struct {
    critd_t                     crit;
    dhcp_snooping_conf_t        conf;
    dhcp_snooping_port_conf_t   port_conf[VTSS_ISID_END];
    u32                         frame_info_cnt; /* Frame information counter */
} dhcp_snooping_global_t;

#endif /* _DHCP_SNOOPING_H_ */

