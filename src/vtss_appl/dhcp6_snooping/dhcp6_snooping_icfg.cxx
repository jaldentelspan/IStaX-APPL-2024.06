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
#include "vtss/appl/dhcp6_snooping.h"
#include "dhcp6_snooping_api.h"
#include "dhcp6_snooping_icfg.h"
#include "misc_api.h"   //uport2iport(), iport2uport()


/*
******************************************************************************

    Static Function

******************************************************************************
*/

/* ICFG callback functions */
static mesa_rc DHCP6_SNOOPING_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                               vtss_icfg_query_result_t *result)
{
    mesa_rc                         rc = VTSS_RC_OK;
    vtss_appl_dhcp6_snooping_conf_t conf, def_conf;
    int                             conf_changed = 0;

    if ((rc = vtss_appl_dhcp6_snooping_conf_get(&conf)) != VTSS_RC_OK) {
        return rc;
    }

    vtss_appl_dhcp6_snooping_conf_get_default(&def_conf);
    conf_changed = vtss_appl_dhcp6_snooping_conf_changed(&def_conf, &conf);

    if (req->all_defaults || conf_changed) {
        rc = vtss_icfg_printf(result, "%s%s\n",
                              conf.snooping_mode == DHCP6_SNOOPING_MODE_ENABLED ? "" : DHCP6_SNOOPING_NO_FORM_TEXT,
                              DHCP6_SNOOPING_GLOBAL_MODE_ENABLE_TEXT);
    }

    return rc;
}

static mesa_rc DHCP6_SNOOPING_ICFG_port_conf(const vtss_icfg_query_request_t *req,
                                             vtss_icfg_query_result_t *result)
{
    mesa_rc                                 rc = VTSS_RC_OK;
    vtss_appl_dhcp6_snooping_port_conf_t    conf, def_conf;
    bool                                    conf_changed = false;
    vtss_ifindex_t                          ifindex;

    if ((rc = vtss_ifindex_from_port(req->instance_id.port.isid, req->instance_id.port.begin_iport, &ifindex)) != VTSS_RC_OK) {
        return rc;
    }

    if ((rc = vtss_appl_dhcp6_snooping_port_conf_get(ifindex, &conf)) != VTSS_RC_OK) {
        return rc;
    }

    vtss_appl_dhcp6_snooping_port_conf_get_default(&def_conf);

    conf_changed = vtss_appl_dhcp6_snooping_port_conf_changed(&def_conf, &conf);
    if (req->all_defaults ||
        (conf_changed && def_conf.trust_mode != conf.trust_mode)) {
        rc = vtss_icfg_printf(result, " %s%s\n",
                              conf.trust_mode == DHCP6_SNOOPING_PORT_MODE_TRUSTED ? "" : DHCP6_SNOOPING_NO_FORM_TEXT,
                              DHCP6_SNOOPING_PORT_MODE_TRUST_TEXT);
    }

    return rc;
}


/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc dhcp6_snooping_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_DHCP6_SNOOPING_GLOBAL_CONF, "dhcp6-snooping", DHCP6_SNOOPING_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    rc = vtss_icfg_query_register(VTSS_ICFG_DHCP6_SNOOPING_PORT_CONF, "dhcp6-snooping", DHCP6_SNOOPING_ICFG_port_conf);

    return rc;
}
