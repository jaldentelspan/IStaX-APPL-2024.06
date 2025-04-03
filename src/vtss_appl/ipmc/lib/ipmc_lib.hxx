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

#ifndef _IPMC_LIB_HXX_
#define _IPMC_LIB_HXX_

#include <vtss/appl/ipmc_lib.h>

// Get IPv4 address for a given VID.
// 1) Use querier if this is set and non-zero.
// 2) Otherwise use IP address of VLAN interface given by vid if that I/F is up
//    and address is non-zero.
// 3) Otherwise, use IP address of first VLAN interface that is up and has a
//    non-zero IPv4 address.
// 4) Otherwise use 192.0.2.1
vtss_appl_ipmc_lib_ip_t ipmc_lib_vlan_if_ipv4_get(mesa_vid_t vid, const vtss_appl_ipmc_lib_ip_t &querier);

typedef i32 (*ipmc_lib_icli_pr_t)(const char *fmt, ...) __attribute__((__format__(__printf__, 1, 2)));

#endif /* _IPMC_LIB_HXX_ */

