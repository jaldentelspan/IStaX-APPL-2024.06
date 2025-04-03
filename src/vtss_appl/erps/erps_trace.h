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

#ifndef _ERPS_TRACE_H_
#define _ERPS_TRACE_H_

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_ERPS

#define VTSS_TRACE_GRP_DEFAULT  0
#define ERPS_TRACE_GRP_BASE     1
#define ERPS_TRACE_GRP_TIMER    2
#define ERPS_TRACE_GRP_CALLBACK 3
#define ERPS_TRACE_GRP_FRAME_RX 4
#define ERPS_TRACE_GRP_FRAME_TX 5
#define ERPS_TRACE_GRP_ICLI     6
#define ERPS_TRACE_GRP_API      7
#define ERPS_TRACE_GRP_CFM      8
#define ERPS_TRACE_GRP_ACL      9
#define ERPS_TRACE_GRP_HIST    10

#include <vtss_trace_api.h>

#endif /* _ERPS_TRACE_H_ */
