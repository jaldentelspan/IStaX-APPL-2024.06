/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_LACP_OS_H_
#define _VTSS_LACP_OS_H_ 1

#include <sys/types.h>
#include <sys/param.h>
#include <netinet/in.h>         /* To get ntohX()/htonX() */

#include "main.h"
#include "l2proto_api.h"
#include "vtss_common_os.h"

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

/* Number of port in switch */
#define VTSS_LACP_MAX_PORTS             (L2_MAX_PORTS)
#define VTSS_LACP_MAX_PORTS_            (L2_MAX_PORTS_)
#define VTSS_LACP_MAX_APORTS            (L2_MAX_LLAGS+L2_MAX_GLAGS) /* Number of aggregations */

#define VTSS_LACP_BUFMEM_ATTRIB         /* Attribute for network buffer memmory */
#define VTSS_LACP_DATA_ATTRIB           /* Attribute for general data */
#define VTSS_LACP_PTR_ATTRIB            /* Attribute for misc pointers */
typedef void *vtss_lacp_bufref_t;

#define VTSS_LACP_AUTOKEY               ((vtss_lacp_key_t)0) /* Generate key from speed */

/* Delay port_mac_address - all switches might not be available */
#define VTSS_LACP_DELAYED_PORTMAC_ADDRESS 1

#define VTSS_LACP_TRACE(l, a)  do {\
if(l == VTSS_TRACE_LVL_ERROR)   T_E a; \
if(l == VTSS_TRACE_LVL_WARNING) T_W a; \
if(l == VTSS_TRACE_LVL_DEBUG)   T_D a; \
else                            T_N a; \
} while (0)

#define VTSS_LACP_TRLVL_ERROR           VTSS_TRACE_LVL_ERROR
#define VTSS_LACP_TRLVL_WARNING         VTSS_TRACE_LVL_WARNING
#define VTSS_LACP_TRLVL_DEBUG           VTSS_TRACE_LVL_DEBUG
#define VTSS_LACP_TRLVL_NOISE           VTSS_TRACE_LVL_NOISE

#define VTSS_LACP_ASSERT(x)         VTSS_ASSERT(x)

extern const vtss_common_macaddr_t vtss_lacp_slowmac;

#define VTSS_LACP_LACPMAC               (vtss_lacp_slowmac.macaddr)
#else
#define VTSS_LACP_NOT_WANTED            1

#endif /* _VTSS_LACP_OS_H__ */
