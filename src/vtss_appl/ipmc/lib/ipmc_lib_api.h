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

#ifndef _IPMC_LIB_API_H_
#define _IPMC_LIB_API_H_

#include <vtss/appl/ipmc_lib.h>

#ifdef VTSS_SW_OPTION_SMB_IPMC
// Both IGMP & MLD
#define IPMC_LIB_PROTOCOL_CNT 2
#else
// IGMP, only
#define IPMC_LIB_PROTOCOL_CNT 1
#endif

// The following functions are used by ipmc.cxx and mvr.cxx
void ipmc_lib_capabilities_set(bool is_mvr, const vtss_appl_ipmc_capabilities_t &cap);
void ipmc_lib_global_default_conf_set(   const vtss_appl_ipmc_lib_key_t &key, const vtss_appl_ipmc_lib_global_conf_t    &global_conf);
void ipmc_lib_port_default_conf_set(     const vtss_appl_ipmc_lib_key_t &key, const vtss_appl_ipmc_lib_port_conf_t      &port_conf);
void ipmc_lib_vlan_default_conf_set(     const vtss_appl_ipmc_lib_key_t &key, const vtss_appl_ipmc_lib_vlan_conf_t      &vlan_conf);
void ipmc_lib_vlan_port_default_conf_set(const vtss_appl_ipmc_lib_key_t &key, const vtss_appl_ipmc_lib_vlan_port_conf_t &vlan_port_conf);
mesa_rc ipmc_lib_vlan_check(mesa_vid_t vid);

// Interface for IPMC and MVR to create a VLAN as they see fit.
// IPMC may set called_back to true. MVR may never.
// called_back == true means that IPMC_LIB has invoked IPMC to see if a
// particular VLAN interfaces indeed is up.
mesa_rc ipmc_lib_vlan_create(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, bool called_back);

// Interface for IPMC and MVR to remove a VLAN as they see fit.
mesa_rc ipmc_lib_vlan_remove(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key);

// The following functions are used by main.cxx
mesa_rc ipmc_lib_init(vtss_init_data_t *data);
const char *ipmc_lib_error_txt(mesa_rc rc);

#endif /* _IPMC_LIB_API_H_ */

