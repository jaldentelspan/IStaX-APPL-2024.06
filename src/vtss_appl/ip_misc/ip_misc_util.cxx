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

#include "ip_api.h"
#include "ip_misc_util.h"

// Check that source address is actually used by a NID interface
bool vtss_ip_misc_is_src_address_used(const char *src_address, mesa_ip_type_t ip_version)
{
    vtss_appl_ip_if_status_t if_status;
    mesa_ip_addr_t           check_addr;
    struct in_addr           sin_addr4;
    struct in6_addr          sin_addr6;

    if (ip_version != MESA_IP_TYPE_IPV4 && ip_version != MESA_IP_TYPE_IPV6) {
        return false;
    }

    check_addr.type = ip_version;

    if (ip_version == MESA_IP_TYPE_IPV4) {
        if (!inet_pton(AF_INET, src_address, &sin_addr4)) {
            return false;
        }

        check_addr.addr.ipv4 = ntohl(sin_addr4.s_addr);
    } else {
        if (!inet_pton(AF_INET6, src_address, &sin_addr6)) {
            return false;
        }

        memcpy(check_addr.addr.ipv6.addr, sin_addr6.s6_addr, sizeof(check_addr.addr.ipv6));
    }

    if (vtss_appl_ip_if_status_find(&check_addr, &if_status) != VTSS_RC_OK) {
        return false;
    }

    return true;
}

bool vtss_ip_misc_is_vid_ip_interface(mesa_vid_t vid)
{
    vtss_appl_ip_if_status_t    ifstat;
    u32                         ifct = 0;
    vtss_ifindex_t ifidx = vtss_ifindex_cast_from_u32(vid, VTSS_IFINDEX_TYPE_VLAN);

    if (vtss_appl_ip_if_status_get(ifidx, VTSS_APPL_IP_IF_STATUS_TYPE_ANY, 1, &ifct, &ifstat) != VTSS_RC_OK) {
        return FALSE;
    }

    if (ifct != 1) {
        return FALSE;
    }

    return TRUE;
}

