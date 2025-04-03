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

#ifndef _IPMC_LIB_UTILS_HXX_
#define _IPMC_LIB_UTILS_HXX_

#include <vtss/appl/ipmc_lib.h>

const char *ipmc_lib_util_tag_type_to_str(       mesa_tag_type_t                    tag_type);
const char *ipmc_lib_util_compatibility_to_str(  vtss_appl_ipmc_lib_compatibility_t compat, bool is_ipv4, bool full = false);
const char *ipmc_lib_util_filter_mode_to_str(    vtss_appl_ipmc_lib_filter_mode_t   filter_mode);
const char *ipmc_lib_util_router_status_to_str(  vtss_appl_ipmc_lib_router_status_t router_status);
const char *ipmc_lib_util_hw_location_to_str(    vtss_appl_ipmc_lib_hw_location_t   hw_location);
const char *ipmc_lib_util_querier_state_to_str(  vtss_appl_ipmc_lib_querier_state_t querier_state);
const char *ipmc_lib_util_port_role_to_str(      vtss_appl_ipmc_lib_port_role_t     role, bool capital = false);
const char *ipmc_lib_util_compatible_mode_to_str(bool                               compatible_mode);
char       *ipmc_lib_util_vlan_oper_warnings_to_txt(char *buf, size_t size, vtss_appl_ipmc_lib_vlan_oper_warnings_t oper_warnings);

#endif /* _IPMC_LIB_UTILS_HXX_ */

