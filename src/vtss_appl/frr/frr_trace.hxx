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

#ifndef _FRR_TRACE_HXX_
#define _FRR_TRACE_HXX_

/**
 * \file frr_trace.hxx
 * \brief This file contains the definitions for module trace purpose.
 *
 * When this file is included in the header file, you can use the following
 * macros.
 * C++: VTSS_TRACE(<sub_group>, <trace_level>)
 * C  : Original way, e.g. T_E(...), T_W(...), T_D(...).
 */

#include <vtss_trace_lvl_api.h>
#include "vtss/basics/trace.hxx"  // For VTSS_TRACE()
#include "vtss_module_id.h"       // For module ID

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_FRR
#define FRR_TRACE_GRP_DAEMON   1
#define FRR_TRACE_GRP_OSPF     2
#define FRR_TRACE_GRP_OSPF6    3
#define FRR_TRACE_GRP_RIP      4
#define FRR_TRACE_GRP_ROUTER   5
#define FRR_TRACE_GRP_IP_ROUTE 6
#define FRR_TRACE_GRP_ICLI     7
#define FRR_TRACE_GRP_ICFG     8
#define FRR_TRACE_GRP_SNMP     9

#endif /* _FRR_TRACE_HXX_ */

