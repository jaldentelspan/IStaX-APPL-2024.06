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

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include "icfg_api.h"
#include "vtss_upnp_api.h"
#include "upnp_icfg.h"
/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/

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

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* ICFG callback functions */
static mesa_rc UPNP_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
    mesa_rc             rc = VTSS_RC_OK;
    vtss_appl_upnp_param_t conf, def_conf;

    (void) vtss_appl_upnp_system_config_get(&conf);
    (void) upnp_default_get(&def_conf);
    /* command: upnp
                upnp ttl <1-255>
                upnp advertising-duration <100-86400>
                upnp ip-addressing-mode { dynamic | static }
                upnp static interface vlan <vlan_id>
       */

    if (req->all_defaults ||
        (conf.mode != def_conf.mode)) {
        rc = vtss_icfg_printf(result, "%s\n",
                              conf.mode == UPNP_MGMT_ENABLED ? "upnp" : "no upnp");
    }
    /*
    if (req->all_defaults ||
        (conf.ttl != def_conf.ttl)) {
        rc = vtss_icfg_printf(result, "%s%u\n",
                              "upnp ttl ", (u32)conf.ttl);
    }
    */
    if (req->all_defaults ||
        (conf.adv_interval != def_conf.adv_interval) ) {
        rc = vtss_icfg_printf(result, "%s%d\n",
                              "upnp advertising-duration ", conf.adv_interval);
    }

    if (req->all_defaults ||
        (conf.ip_addressing_mode != def_conf.ip_addressing_mode) ) {
        if (VTSS_APPL_UPNP_IPADDRESSING_MODE_DYNAMIC == conf.ip_addressing_mode) {
            rc = vtss_icfg_printf(result, "%s\n", "upnp ip-addressing-mode dynamic");
        } else {
            rc = vtss_icfg_printf(result, "%s\n", "upnp ip-addressing-mode static");
        }
    }

    if (req->all_defaults ||
        (conf.static_ifindex != def_conf.static_ifindex) ) {
        rc = vtss_icfg_printf(result, "%s %d\n",
                              "upnp static interface vlan", vtss_ifindex_cast_to_u32(conf.static_ifindex));
    }

    return rc;
}

/* Initialization function */
mesa_rc upnp_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_UPNP_GLOBAL_CONF, "upnp", UPNP_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return rc;
}
