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
#include "dhcp_relay_api.h"
#include "dhcp_relay_icfg.h"


/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/
#define VTSS_TRACE_MODULE_ID            VTSS_MODULE_ID_DHCP_RELAY
#define DHCP_RELAY_ICFG_REG(x, y, z, w)    (((x) = vtss_icfg_query_register((y), (z), (w))) == VTSS_RC_OK)

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
static const char *_icfg_dhcp_relay_policy_txt(u32 dhcpr_policy)
{
    const char    *txt;

    switch ( dhcpr_policy ) {
    case DHCP_RELAY_INFO_POLICY_DROP:
        txt = "drop";
        break;
    case DHCP_RELAY_INFO_POLICY_KEEP:
        txt = "keep";
        break;
    case DHCP_RELAY_INFO_POLICY_REPLACE:
    default:
        txt = "replace";
        break;
    }

    return txt;
}

/* ICFG callback functions */
static mesa_rc dhcp_relay_icfg_global_ctrl(const vtss_icfg_query_request_t *req,
                                           vtss_icfg_query_result_t *result)
{
    mesa_rc rc = VTSS_RC_OK;

    if (req && result) {
        dhcp_relay_conf_t   dhcpr_icfg;
        BOOL                do_anyway = req->all_defaults;

        /*
            COMMAND = ip dhcp relay
            COMMAND = ip helper-address <ip:ipv4_ucast>
            COMMAND = ip dhcp relay information option
            COMMAND = ip dhcp relay information policy {drop | keep | replace}
        */
        memset(&dhcpr_icfg, 0x0, sizeof(dhcp_relay_conf_t));
        if (dhcp_relay_mgmt_conf_get(&dhcpr_icfg) == VTSS_RC_OK) {
            if (do_anyway ||
                (dhcpr_icfg.relay_mode != DHCP4R_DEF_MODE)) {
                rc |= vtss_icfg_printf(result, "%sip dhcp relay\n",
                                       (dhcpr_icfg.relay_mode == DHCP_RELAY_MGMT_DISABLED) ? "no " : "");
            }

            if (do_anyway ||
                (dhcpr_icfg.relay_server_cnt != DHCP4R_DEF_SRV_CNT)) {
                char  adrString[40];
                u32 idx = 0, exist_cnt = 0;

                for (; idx < dhcpr_icfg.relay_server_cnt; idx++) {
                    if (dhcpr_icfg.relay_server[idx] == 0) {
                        continue;
                    }
                    memset(adrString, 0x0, sizeof(adrString));
                    rc = vtss_icfg_printf(result, "ip helper-address %s\n",
                                          misc_ipv4_txt(dhcpr_icfg.relay_server[idx], adrString));
                    exist_cnt++;
                }

                if (idx == 0 || exist_cnt == 0) {
                    rc = vtss_icfg_printf(result, "no ip helper-address\n");
                }
            }

            if (do_anyway ||
                (dhcpr_icfg.relay_info_mode != DHCP4R_DEF_INFO_MODE)) {
                rc |= vtss_icfg_printf(result, "%sip dhcp relay information option\n",
                                       (dhcpr_icfg.relay_info_mode == DHCP_RELAY_MGMT_ENABLED) ? "" : "no ");
            }

            if (do_anyway ||
                (dhcpr_icfg.relay_info_policy != DHCP4R_DEF_INFO_POLICY)) {
                rc |= vtss_icfg_printf(result, "ip dhcp relay information policy %s\n",
                                       _icfg_dhcp_relay_policy_txt(dhcpr_icfg.relay_info_policy));
            }
        } else {
            if (do_anyway) {
                rc = vtss_icfg_printf(result, "no ip dhcp relay\n");
                rc = vtss_icfg_printf(result, "no ip helper-address\n");
                rc = vtss_icfg_printf(result, "no ip dhcp relay information option\n");
                rc = vtss_icfg_printf(result, "no ip dhcp relay information policy\n");
            }
        }
    }

    return rc;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/
/* Initialization function */
mesa_rc dhcp_relay_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module. */
    if (DHCP_RELAY_ICFG_REG(rc, VTSS_ICFG_DHCP_RELAY_CONFIG, "dhcp", dhcp_relay_icfg_global_ctrl)) {
        T_I("dhcp_relay ICFG done");
    }

    return rc;
}
