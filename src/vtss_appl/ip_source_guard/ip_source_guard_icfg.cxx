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
#include "ip_source_guard_api.h"
#include "ip_source_guard_icfg.h"
#include "misc_api.h"   //uport2iport(), iport2uport()
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#include "icli_porting_util.h"

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

    Static Variables

******************************************************************************
*/

// Default configuration for IP Source Guard
static ip_source_guard_conf_t               ip_source_guard_def_conf;

/*
******************************************************************************

    Static Function

******************************************************************************
*/

/* ICFG callback functions */
/*lint -esym(459, IP_SOURCE_GUARD_ICFG_global_conf)                       */
static mesa_rc IP_SOURCE_GUARD_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                                vtss_icfg_query_result_t *result)
{
    mesa_rc                             rc = VTSS_RC_OK;
    ulong                               mode;
    ip_source_guard_entry_t             entry;
    char                                buf0[80], buf1[80], buf2[80];


    if ((rc = ip_source_guard_mgmt_conf_get_mode(&mode)) != VTSS_RC_OK) {
        return rc;
    }

    /* global mode */
    // example: ip verify source
    if (req->all_defaults || mode != ip_source_guard_def_conf.mode) {
        rc = vtss_icfg_printf(result, "%s%s\n",
                              mode == IP_SOURCE_GUARD_MGMT_ENABLED ? "" : IP_SOURCE_GUARD_NO_FORM_TEXT,
                              IP_SOURCE_GUARD_GLOBAL_MODE_ENABLE_TEXT);
    }

    /* entries */
    // example: ip source binding interface <port_type> <port_id> <vlan_id> <ipv4_ucast> <mac_ucast>
    if (ip_source_guard_mgmt_conf_get_first_static_entry(&entry) == VTSS_RC_OK) {

        rc = vtss_icfg_printf(result, "%s %s %s %d %s %s\n",
                              IP_SOURCE_GUARD_GLOBAL_MODE_ENTRY_TEXT,
                              IP_SOURCE_GUARD_INTERFACE_TEXT,
                              icli_port_info_txt(topo_isid2usid(entry.isid), iport2uport(entry.port_no), buf0),
                              entry.vid,
                              misc_ipv4_txt(entry.assigned_ip, buf1),
                              misc_mac_txt(entry.assigned_mac, buf2));

        while (ip_source_guard_mgmt_conf_get_next_static_entry(&entry) == VTSS_RC_OK) {
            rc = vtss_icfg_printf(result, "%s %s %s %d %s %s\n",
                                  IP_SOURCE_GUARD_GLOBAL_MODE_ENTRY_TEXT,
                                  IP_SOURCE_GUARD_INTERFACE_TEXT,
                                  icli_port_info_txt(topo_isid2usid(entry.isid), iport2uport(entry.port_no), buf0),
                                  entry.vid,
                                  misc_ipv4_txt(entry.assigned_ip, buf1),
                                  misc_mac_txt(entry.assigned_mac, buf2));
        }
    }

    return rc;
}

/*lint -esym(459, IP_SOURCE_GUARD_ICFG_port_conf)                       */
static mesa_rc IP_SOURCE_GUARD_ICFG_port_conf(const vtss_icfg_query_request_t *req,
                                              vtss_icfg_query_result_t *result)
{
    mesa_rc                                     rc = VTSS_RC_OK;
    ip_source_guard_port_mode_conf_t            conf;
    ip_source_guard_port_dynamic_entry_conf_t   entry_cnt_conf;
    vtss_isid_t                                 isid = topo_usid2isid(req->instance_id.port.usid);
    mesa_port_no_t                              iport = uport2iport(req->instance_id.port.begin_uport);

    if (req->instance_id.port.port_type == ICLI_PORT_TYPE_CPU) {
        return rc;
    }

    if ((rc = ip_source_guard_mgmt_conf_get_port_mode(isid, &conf)) != VTSS_RC_OK) {
        return rc;
    }

    /* port mode */
    // example: ip verify source
    if (req->all_defaults || conf.mode[iport] != ip_source_guard_def_conf.port_mode_conf[isid - VTSS_ISID_START].mode[iport]) {
        rc = vtss_icfg_printf(result, " %s%s\n",
                              conf.mode[iport] == IP_SOURCE_GUARD_MGMT_ENABLED ? "" : IP_SOURCE_GUARD_NO_FORM_TEXT,
                              IP_SOURCE_GUARD_PORT_MODE_ENABLE_TEXT);
    }

    if ((rc = ip_source_guard_mgmt_conf_get_port_dynamic_entry_cnt(isid, &entry_cnt_conf)) != VTSS_RC_OK) {
        return rc;
    }

    /* port limit */
    // example: ip verify source limit <0-2>
    if (req->all_defaults || entry_cnt_conf.entry_cnt[iport] != ip_source_guard_def_conf.port_dynamic_entry_conf[isid - VTSS_ISID_START].entry_cnt[iport]) {
        if (entry_cnt_conf.entry_cnt[iport] == IP_SOURCE_GUARD_DYNAMIC_UNLIMITED) {
            rc = vtss_icfg_printf(result, " %s%s\n",
                                  IP_SOURCE_GUARD_NO_FORM_TEXT,
                                  IP_SOURCE_GUARD_PORT_MODE_LIMIT_TEXT);
        } else {
            rc = vtss_icfg_printf(result, " %s " VPRIlu"\n",
                                  IP_SOURCE_GUARD_PORT_MODE_LIMIT_TEXT,
                                  entry_cnt_conf.entry_cnt[iport]);
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
mesa_rc ip_source_guard_icfg_init(void)
{
    mesa_rc rc;

    // Setup default configuration once
    ip_source_guard_default_set(&ip_source_guard_def_conf);

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_IP_SOURCE_GUARD_GLOBAL_CONF, "source-guard", IP_SOURCE_GUARD_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    rc = vtss_icfg_query_register(VTSS_ICFG_IP_SOURCE_GUARD_PORT_CONF, "source-guard", IP_SOURCE_GUARD_ICFG_port_conf);

    return rc;
}
