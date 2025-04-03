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

/**
 * This file contains the macros for the LACP protocol module
 * that must be adapted for the operating environment.
 *
 * This is the version for the switch_app environment.
 */

#ifndef _VTSS_LACP_OS_H_
#define _VTSS_LACP_OS_H_ 1

#include <stdio.h>
#include <assert.h>

#include "vtss_common_os.h"

#define VTSS_LACP_MAX_PORTS             (2) /* Number of port in switch */

#define VTSS_LACP_AUTOKEY               ((vtss_lacp_key_t)0) /* Generate key from speed */

#define VTSS_LACP_TRACE(LVL, ARGS) \
do { \
    vtss_common_savetrace(LVL, __FUNCTION__, __LINE__); \
    vtss_common_trace ARGS; \
} while (0)
#define VTSS_LACP_TRLVL_ERROR           0
#define VTSS_LACP_TRLVL_WARNING         1
#define VTSS_LACP_TRLVL_DEBUG           2
#define VTSS_LACP_TRLVL_NOISE           3

#define VTSS_LACP_ASSERT(EXPR)          assert(EXPR)

extern const vtss_common_macaddr_t vtss_lacp_slowmac;

#define VTSS_LACP_LACPMAC               (vtss_lacp_slowmac.macaddr)

#endif /* _VTSS_LACP_OS_H__ */
