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

/**
 * \file dhcp6_snooping_priv.h
 * \brief This file contains module-private definitions for the DHCPv6 Snooping module
 */

#ifndef _DHCP6_SNOOPING_PRIV_H_
#define _DHCP6_SNOOPING_PRIV_H_

#include "vtss_module_id.h"
#include "vtss_trace_lvl_api.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_DHCP6_SNOOPING

#define VTSS_TRACE_GRP_DEFAULT      0
#define TRACE_GRP_TXPKT             1
#define TRACE_GRP_RXPKT             2
#define TRACE_GRP_CFG               3
#define TRACE_GRP_STATE             4
#define TRACE_GRP_TIMER             5

#include <vtss_trace_api.h>

#endif /* _DHCP6_SNOOPING_PRIV_H_ */

