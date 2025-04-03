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

#ifndef _IPMC_LIB_TRACE_H_
#define _IPMC_LIB_TRACE_H_

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include "ipmc_lib_base.hxx"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_IPMC_LIB

#define IPMC_LIB_TRACE_GRP_BASE      1
#define IPMC_LIB_TRACE_GRP_RX        2
#define IPMC_LIB_TRACE_GRP_TX        3
#define IPMC_LIB_TRACE_GRP_API       4
#define IPMC_LIB_TRACE_GRP_TICK      5
#define IPMC_LIB_TRACE_GRP_ICLI      6
#define IPMC_LIB_TRACE_GRP_WEB       7
#define IPMC_LIB_TRACE_GRP_QUERIER   8
#define IPMC_LIB_TRACE_GRP_CALLBACK  9
#define IPMC_LIB_TRACE_GRP_LOG      10
#define IPMC_LIB_TRACE_GRP_BIP      11
#define IPMC_LIB_TRACE_GRP_PROFILE  12

#include <vtss_trace_api.h>
#include "ipmc_lib_trace.hxx"

#endif /* _IPMC_LIB_TRACE_H_ */

