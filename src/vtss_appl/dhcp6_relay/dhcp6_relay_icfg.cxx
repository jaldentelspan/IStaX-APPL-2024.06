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

#include "icfg_api.h"
#include "misc_api.h"
#include "icli_api.h"
#include "topo_api.h"
#include "ip_utils.hxx"
#include "dhcp6_relay.h"
#include "dhcp6_relay_icfg.h"
#include "vtss/appl/dhcp6_relay.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_DHCP6_RELAY

//******************************************************************************
// ICFG callback functions
//******************************************************************************

static mesa_rc dhcp6_relay_icfg_vlan_interface_conf(const vtss_icfg_query_request_t *req,
                                      vtss_icfg_query_result_t *result) {
    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    mesa_vid_t vid = (mesa_vid_t)req->instance_id.vlan;
    vtss_ifindex_t ifidx;
    (void) vtss_ifindex_from_vlan(vid, &ifidx);

    vtss_ifindex_t itr = ifidx;
    vtss_ifindex_t relay = VTSS_IFINDEX_NONE;
    vtss_appl_dhcpv6_relay_vlan_t conf;
    for (auto rc = dhcp6_relay_vlan_configuration.get_next(&itr,&relay, &conf);
         rc == VTSS_RC_OK && itr == ifidx;
         rc = dhcp6_relay_vlan_configuration.get_next(&itr,&relay, &conf)) {
        VTSS_RC(vtss_icfg_printf(result,
                                 " ipv6 dhcp relay"));

        if (memcmp(&conf.relay_destination, &ipv6_all_dhcp_servers,sizeof(mesa_ipv6_t) != 0)) {
            char ipv6addr[100];
            inet_ntop(AF_INET6, &(conf.relay_destination.addr),
                      ipv6addr, sizeof(ipv6addr));

            VTSS_RC(vtss_icfg_printf(result, " destination %s", ipv6addr));
        }

        VTSS_RC(vtss_icfg_printf(result, " interface vlan %d\n", idx2vid(relay)));
    }

    return VTSS_RC_OK;
}

//******************************************************************************
//   Public functions
//******************************************************************************

mesa_rc dhcp6_relay_icfg_init(void) {
    mesa_rc rc;

    /* Register callback functions to ICFG module. */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_DHCP6_RELAY_INTERFACE,
                                       "dhcp6_relay",
                                       dhcp6_relay_icfg_vlan_interface_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return VTSS_RC_OK;
}
