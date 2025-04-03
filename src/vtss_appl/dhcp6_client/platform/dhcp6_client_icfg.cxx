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
#if defined(VTSS_SW_OPTION_IP)
#include "ip_api.h"
#endif /* defined(VTSS_SW_OPTION_IP) */
#include "dhcp6_client_api.h"
#include "dhcp6c_conf.hxx"

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/
#define VTSS_TRACE_MODULE_ID            VTSS_MODULE_ID_DHCP6C

#define DHCP6C_ICFG_REG(w, x, y, z)     (((w) = vtss_icfg_query_register((x), (y), (z))) == VTSS_RC_OK)

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/
/* ICFG callback functions */
static mesa_rc
DHCP6C_icfg_intf_entry(
    const vtss_icfg_query_request_t     *const req,
    vtss_icfg_query_result_t            *const result)
{
    mesa_rc rc = VTSS_RC_OK;

    if (req && result) {
        mesa_vid_t                  vidx;
        Dhcp6cInterface             intf;
        BOOL                        ipif_exist;
#if defined(VTSS_SW_OPTION_IP)
        vtss_ifindex_t              ifidx;
        vtss_appl_ip_if_conf_ipv4_t ip4_conf;
        vtss_appl_ip_if_conf_ipv6_t ip6_conf;
#endif /* defined(VTSS_SW_OPTION_IP) */

        /*
            SUB-MODE: interface vlan <vlan_id>
            COMMAND = ipv6 address { autoconfig | dhcp [ rapid-commit ] }
        */
        vidx = req->instance_id.vlan;

        ipif_exist = FALSE;
#if defined(VTSS_SW_OPTION_IP)
        if (vtss_ifindex_from_vlan(vidx, &ifidx) == VTSS_RC_OK) {
            if (vtss_appl_ip_if_conf_ipv4_get(ifidx, &ip4_conf) == VTSS_RC_OK &&
                ip4_conf.enable) {
                ipif_exist = TRUE;
            }
            if (!ipif_exist &&
                vtss_appl_ip_if_conf_ipv6_get(ifidx, &ip6_conf) == VTSS_RC_OK &&
                ip6_conf.enable) {
                ipif_exist = TRUE;
            }
        }
#endif /* defined(VTSS_SW_OPTION_IP) */

        T_D("Get VID:%u with IPIF-%s", vidx, ipif_exist ? "EXIST" : "NONE");
        if (vtss::dhcp6c::dhcp6_client_interface_get(vidx, &intf) == VTSS_RC_OK) {
            BOOL    srvc;

            srvc = (intf.srvc & DHCP6C_RUN_SLAAC_ONLY) ? TRUE : FALSE;
            if (req->all_defaults || srvc != DHCP6C_CONF_DEF_SLAAC_STATE) {
                rc |= vtss_icfg_printf(result, " %sipv6 address autoconfig\n", srvc ? "" : "no ");
            }

            srvc = (intf.srvc & DHCP6C_RUN_DHCP_ONLY) ? TRUE : FALSE;
            if (req->all_defaults || srvc != DHCP6C_CONF_DEF_DHCP6_STATE) {
                rc |= vtss_icfg_printf(result, " %sipv6 address dhcp%s\n",
                                       srvc ? "" : "no ",
                                       srvc ? (intf.rapid_commit ? " rapid-commit" : "") : "");
            }
        } else if (ipif_exist && req->all_defaults) {
#if 0
            rc |= vtss_icfg_printf(result, " no ipv6 address autoconfig\n");
#endif
            rc |= vtss_icfg_printf(result, " no ipv6 address dhcp\n");
        }
    }

    return rc;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/
/**
 * \brief Initialization function.
 *
 * Call once, preferably from the INIT_CMD_INIT section of
 * the module's _init() function.
 */
mesa_rc dhcp6_client_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module. */
    if (DHCP6C_ICFG_REG(rc, VTSS_ICFG_DHCP6C_INTERFACE, "dhcp6_client_interface", DHCP6C_icfg_intf_entry)) {
        T_I("dhcp6c ICFG done");
    }

    return rc;
}
