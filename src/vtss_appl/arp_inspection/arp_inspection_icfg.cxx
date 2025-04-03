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
#include "arp_inspection_api.h"
#include "arp_inspection_icfg.h"
#include "misc_api.h"   //uport2iport(), iport2uport()
#include "topo_api.h"   //topo_usid2isid(), topo_isid2usid()
#include "icli_porting_util.h"
#include "vlan_api.h"

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

/* ICFG callback functions */
/*lint -esym(459, ARP_INSPECTION_ICFG_global_conf)                       */
static mesa_rc ARP_INSPECTION_ICFG_global_conf(const vtss_icfg_query_request_t *req,
                                               vtss_icfg_query_result_t *result)
{
    mesa_rc                             rc = VTSS_RC_OK;
    static arp_inspection_conf_t        def_conf;
    arp_inspection_entry_t              entry;
    arp_inspection_vlan_mode_conf_t     vlan_mode_conf;
    u32                                 mode;
    char                                buf0[80], buf1[80], buf2[80];
    int                                 idx;

    if ((rc = arp_inspection_mgmt_conf_mode_get(&mode)) != VTSS_RC_OK) {
        return rc;
    }

    arp_inspection_default_set(&def_conf);

    /* global mode */
    // example: ip arp inspection
    if (req->all_defaults || mode != def_conf.mode) {
        rc = vtss_icfg_printf(result, "%s%s\n",
                              mode == ARP_INSPECTION_MGMT_ENABLED ? "" : ARP_INSPECTION_NO_FORM_TEXT,
                              ARP_INSPECTION_GLOBAL_MODE_ENABLE_TEXT);
    }

    /* vlan */
    // example: ip arp inspection vlan <vlan_list>
    for (idx = VTSS_APPL_VLAN_ID_MIN; idx <= VTSS_APPL_VLAN_ID_MAX; idx++) {
        // get configuration
        if (arp_inspection_mgmt_conf_vlan_mode_get(idx, &vlan_mode_conf, FALSE) != VTSS_RC_OK) {
            continue;
        }

        if (vlan_mode_conf.flags & ARP_INSPECTION_VLAN_MODE) {

            rc = vtss_icfg_printf(result, "%s %d\n",
                                  ARP_INSPECTION_GLOBAL_MODE_VLAN_TEXT,
                                  idx);
        }
    }

    // example: ip arp inspection vlan <vlan_list> logging {deny|permit|all}
    for (idx = VTSS_APPL_VLAN_ID_MIN; idx <= VTSS_APPL_VLAN_ID_MAX; idx++) {
        // get configuration
        if (arp_inspection_mgmt_conf_vlan_mode_get(idx, &vlan_mode_conf, FALSE) != VTSS_RC_OK) {
            continue;
        }

        if (vlan_mode_conf.flags & ARP_INSPECTION_VLAN_LOG_DENY || vlan_mode_conf.flags & ARP_INSPECTION_VLAN_LOG_PERMIT) {

            if ((vlan_mode_conf.flags & ARP_INSPECTION_VLAN_LOG_DENY) && (vlan_mode_conf.flags & ARP_INSPECTION_VLAN_LOG_PERMIT)) {
                rc = vtss_icfg_printf(result, "%s %d %s %s\n",
                                      ARP_INSPECTION_GLOBAL_MODE_VLAN_TEXT,
                                      idx,
                                      ARP_INSPECTION_LOGGING_TEXT,
                                      ARP_INSPECTION_LOGGING_ALL_TEXT);
            } else if (vlan_mode_conf.flags & ARP_INSPECTION_VLAN_LOG_DENY) {
                rc = vtss_icfg_printf(result, "%s %d %s %s\n",
                                      ARP_INSPECTION_GLOBAL_MODE_VLAN_TEXT,
                                      idx,
                                      ARP_INSPECTION_LOGGING_TEXT,
                                      ARP_INSPECTION_LOGGING_DENY_TEXT);
            } else if (vlan_mode_conf.flags & ARP_INSPECTION_VLAN_LOG_PERMIT) {
                rc = vtss_icfg_printf(result, "%s %d %s %s\n",
                                      ARP_INSPECTION_GLOBAL_MODE_VLAN_TEXT,
                                      idx,
                                      ARP_INSPECTION_LOGGING_TEXT,
                                      ARP_INSPECTION_LOGGING_PERMIT_TEXT);
            }
        }
    }

    /* entries */
    // example: ip arp inspection entry interface <port_type> <port_list> <vlan_id> <mac_ucast> <ipv4_ucast>
    memset(&entry, 0x0, sizeof(arp_inspection_entry_t));
    if (arp_inspection_mgmt_conf_static_entry_get(&entry, FALSE) == VTSS_RC_OK) {

        rc = vtss_icfg_printf(result, "%s %s %s %d %s %s\n",
                              ARP_INSPECTION_GLOBAL_MODE_ENTRY_TEXT,
                              ARP_INSPECTION_INTERFACE_TEXT,
                              icli_port_info_txt(topo_isid2usid(entry.isid), iport2uport(entry.port_no), buf0),
                              entry.vid,
                              misc_mac_txt(entry.mac, buf1),
                              misc_ipv4_txt(entry.assigned_ip, buf2));
        while (arp_inspection_mgmt_conf_static_entry_get(&entry, TRUE) == VTSS_RC_OK) {
            rc = vtss_icfg_printf(result, "%s %s %s %d %s %s\n",
                                  ARP_INSPECTION_GLOBAL_MODE_ENTRY_TEXT,
                                  ARP_INSPECTION_INTERFACE_TEXT,
                                  icli_port_info_txt(topo_isid2usid(entry.isid), iport2uport(entry.port_no), buf0),
                                  entry.vid,
                                  misc_mac_txt(entry.mac, buf1),
                                  misc_ipv4_txt(entry.assigned_ip, buf2));
        }
    }

    return rc;
}

/*lint -esym(459, ARP_INSPECTION_ICFG_port_conf)                    */
static mesa_rc ARP_INSPECTION_ICFG_port_conf(const vtss_icfg_query_request_t *req,
                                             vtss_icfg_query_result_t *result)
{
    mesa_rc                             rc = VTSS_RC_OK;
    static arp_inspection_conf_t        def_conf;
    arp_inspection_port_mode_conf_t     conf;
    vtss_isid_t                         isid = topo_usid2isid(req->instance_id.port.usid);
    mesa_port_no_t                      iport = uport2iport(req->instance_id.port.begin_uport);

    VTSS_ASSERT(iport != VTSS_PORT_NO_NONE);
    if ((rc = arp_inspection_mgmt_conf_port_mode_get(isid, &conf)) != VTSS_RC_OK) {
        return rc;
    }

    arp_inspection_default_set(&def_conf);

    /* port mode */
    // example: ip arp inspection trust
    if (req->all_defaults || conf.mode[iport] != def_conf.port_mode_conf[isid].mode[iport]) {
        rc = vtss_icfg_printf(result, " %s%s\n",
                              conf.mode[iport] == ARP_INSPECTION_MGMT_DISABLED ? "" : ARP_INSPECTION_NO_FORM_TEXT,
                              ARP_INSPECTION_PORT_MODE_TRUST_TEXT);
    }

    /* check-vlan mode */
    // example: ip arp inspection check-vlan
    if (req->all_defaults || conf.check_VLAN[iport] != def_conf.port_mode_conf[isid].check_VLAN[iport]) {
        rc = vtss_icfg_printf(result, " %s%s\n",
                              conf.check_VLAN[iport] == ARP_INSPECTION_MGMT_VLAN_ENABLED ? "" : ARP_INSPECTION_NO_FORM_TEXT,
                              ARP_INSPECTION_PORT_MODE_CHECK_VLAN_TEXT);
    }

    /* logging */
    // example: ip arp inspection logging {deny|permit|all}
    if (req->all_defaults || conf.log_type[iport] != def_conf.port_mode_conf[isid].log_type[iport]) {
        switch (conf.log_type[iport]) {
        case ARP_INSPECTION_LOG_NONE:
            rc = vtss_icfg_printf(result, " %s%s\n",
                                  ARP_INSPECTION_NO_FORM_TEXT,
                                  ARP_INSPECTION_PORT_MODE_LOGGING_TEXT);
            break;
        case ARP_INSPECTION_LOG_DENY:
            rc = vtss_icfg_printf(result, " %s %s\n",
                                  ARP_INSPECTION_PORT_MODE_LOGGING_TEXT,
                                  ARP_INSPECTION_LOGGING_DENY_TEXT);
            break;
        case ARP_INSPECTION_LOG_PERMIT:
            rc = vtss_icfg_printf(result, " %s %s\n",
                                  ARP_INSPECTION_PORT_MODE_LOGGING_TEXT,
                                  ARP_INSPECTION_LOGGING_PERMIT_TEXT);
            break;
        case ARP_INSPECTION_LOG_ALL:
            rc = vtss_icfg_printf(result, " %s %s\n",
                                  ARP_INSPECTION_PORT_MODE_LOGGING_TEXT,
                                  ARP_INSPECTION_LOGGING_ALL_TEXT);
            break;
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
mesa_rc arp_inspection_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module.
       The configuration divided into two groups for this module.
       1. Global configuration
       2. Port configuration
    */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_ARP_INSPECTION_GLOBAL_CONF, "arp-inspection", ARP_INSPECTION_ICFG_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    rc = vtss_icfg_query_register(VTSS_ICFG_ARP_INSPECTION_PORT_CONF, "arp-inspection", ARP_INSPECTION_ICFG_port_conf);

    return rc;
}
