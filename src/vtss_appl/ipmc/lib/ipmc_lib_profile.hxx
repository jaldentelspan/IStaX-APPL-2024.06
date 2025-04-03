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

#ifndef _IPMC_LIB_PROFILE_HXX_
#define _IPMC_LIB_PROFILE_HXX_

// This structure is used as input to profile matching. vid, port_no and src are
// only used if dst.ipv4/dst.ipv6 matches a rule that has logging enabled.
typedef struct {
    vtss_appl_ipmc_lib_profile_key_t profile_key;
    mesa_vid_t                       vid;
    mesa_port_no_t                   port_no;
    vtss_appl_ipmc_lib_ip_t          dst;
    vtss_appl_ipmc_lib_ip_t          src;
} ipmc_lib_profile_match_t;

// Given the profile_key check whether match.dst matches one of the profile's
// rules.
// The function returns true if the address is permitted, false if denied.
//
// The rules are as follows:
// 1) If IPMC profile functionality is globally disabled, the address is
//    permitted.
// 2) If strlen(profile_key) == 0 or the profile key doesn't exist, the address
//    is denied.
// 3) The profile rules are searched in order and the first that matches the IP
//    address tells whether to permit or deny and whether to log or not.
// 4) If no match was found, the address is denied.
//
// If a rule match was found, and it tells to log the entry, it will be logged
// to the syslog with vid, port_no, and src.
bool ipmc_lib_profile_permit(const ipmc_lib_vlan_state_t &vlan_state, const ipmc_lib_profile_match_t &match, bool log);

// Check the validity of a profile name
mesa_rc ipmc_lib_profile_key_check(const vtss_appl_ipmc_lib_profile_key_t *key, bool allow_empty);

#endif // _IPMC_LIB_PROFILE_HXX_

