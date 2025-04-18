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

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#ifndef __SYNCE_TRACE_H__
#define __SYNCE_TRACE_H__

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SYNCE

#define VTSS_TRACE_GRP_DEFAULT 0
#define TRACE_GRP_PDU_RX       1
#define TRACE_GRP_PDU_TX       2
#define TRACE_GRP_API          3
#define TRACE_GRP_CLOCK        4
#define TRACE_GRP_CLI          5
#define TRACE_GRP_DEVELOP      6
#define TRACE_GRP_BOARD        7

#endif /* __SYNCE_TRACE_H__ */
