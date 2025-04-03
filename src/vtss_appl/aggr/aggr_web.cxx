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

#include "web_api.h"
#ifdef VTSS_SW_OPTION_LACP
#include "l2proto_api.h"
#include "lacp_api.h"
#endif /* VTSS_SW_OPTION_LACP */
#include "aggr_api.h"
#include "port_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif
#include "icli_porting_util.h"
#define AGGR_PORT_BUF_SIZE 180

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

// #define aggr_mgmt_port_members_add(a,b,c) aggr_mgmt_port_members_add(b,c)
// #define aggr_mgmt_group_del(a,b)        aggr_mgmt_group_del(b)
// #define aggr_mgmt_aggr_mode_set(a,b)    aggr_mgmt_aggr_mode_set(b)
// #define aggr_mgmt_port_members_get(a,b,c,d) aggr_mgmt_port_members_get(b,c,d)
// #define aggr_mgmt_aggr_mode_get(a,b)    aggr_mgmt_aggr_mode_get(b)
static aggr_mgmt_group_no_t web_aggr_id2no(aggr_mgmt_group_no_t aggr_id)
{
    return (AGGR_MGMT_GROUP_NO_START + aggr_id - 1);
}

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/
static i32 handler_config_aggr_common(CYG_HTTPD_STATE *p)
{
    vtss_isid_t              sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                      ct;
    mesa_rc                  rc;
    mesa_aggr_mode_t mode;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_AGGR)) {
        return -1;
    }
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {
        size_t nlen;

        memset(&mode, 0, sizeof(mesa_aggr_mode_t));  // Gnats 5498, Jack 20061129
        if (cyg_httpd_form_varable_string(p, "src_mac", &nlen)) { // if get token "src_mac", it must be checked on browser
            mode.smac_enable = 1;
        } else {
            mode.smac_enable = 0;
        }
        if (cyg_httpd_form_varable_string(p, "det_mac", &nlen)) {
            mode.dmac_enable = 1;
        } else {
            mode.dmac_enable = 0;
        }
        if (cyg_httpd_form_varable_string(p, "mode_ip", &nlen)) {
            mode.sip_dip_enable = 1;
        } else {
            mode.sip_dip_enable = 0;
        }
        if (cyg_httpd_form_varable_string(p, "mode_port", &nlen)) {
            mode.sport_dport_enable = 1;
        } else {
            mode.sport_dport_enable = 0;
        }
        if ((rc = aggr_mgmt_aggr_mode_set(&mode)) != VTSS_RC_OK) {
            send_custom_error(p, "Aggregation Error", aggr_error_txt(rc), strlen(aggr_error_txt(rc)));
            return -1;
        }
        redirect(p, "/aggr_common.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */

        if (aggr_mgmt_aggr_mode_get(&mode) != VTSS_RC_OK) {
            T_E("aggr_mgmt_aggr_mode_get fail!!");
        }

        cyg_httpd_start_chunked("html");
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%u/%u",
                      mode.smac_enable, mode.dmac_enable, mode.sip_dip_enable, mode.sport_dport_enable, 0);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}


static i32 handler_config_aggr_groups(CYG_HTTPD_STATE *p)
{
    vtss_isid_t              sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_port_no_t           iport;
    aggr_mgmt_group_no_t     aggrgroup;
    aggr_mgmt_group_member_t aggrMember;
    aggr_mgmt_group_t        groupMember;
    int                      ct;
    mesa_rc                  rc;
    BOOL                     aggr[AGGR_LLAG_CNT_ + 1][VTSS_PORTS_];
    u32                      port_count = port_count_max();
    unsigned int             aggr_count = port_count / 2;
    int glag_count[2] = {0, 0};

    memset(aggr, 0, sizeof(BOOL) * ( AGGR_LLAG_CNT_ + 1) * VTSS_PORTS_);

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_AGGR)) {
        return -1;
    }
#endif
    if (AGGR_UPLINK == AGGR_UPLINK_NONE) {
        aggr_count++;
    }
    if (p->method == CYG_HTTPD_METHOD_POST) {
        char var_aggr[12], var_id[33];
        size_t nlen;
        const char *id;
        aggr_mgmt_group_member_t grp;
        int newval;

        for (iport = 0; iport < port_count; iport++) { // for each port, find it's group from web POST arguments
            sprintf(var_aggr, "aggr_port_%d", iport);
            id = cyg_httpd_form_varable_string(p, var_aggr, &nlen);
            for (aggrgroup = 0; aggrgroup < aggr_count; aggrgroup++) {
                sprintf(var_id, "aggr_%u_port_%d", aggrgroup, iport);
                if (nlen == strlen(var_id) && memcmp(id, var_id, nlen) == 0) {
                    aggr[aggrgroup][iport] = 1;
                } else {
                    aggr[aggrgroup][iport] = 0;
                }
            }
        }

        for (aggrgroup = 1; aggrgroup < aggr_count; aggrgroup++) { // Group 0 is Normal group, real group from 1
            aggr_mgmt_group_no_t aggr_group_id = web_aggr_id2no(aggrgroup) ;
            vtss_ifindex_t grp_ifindex;

            if ((rc = vtss_ifindex_from_llag(sid, aggr_group_id, &grp_ifindex)) != VTSS_RC_OK) {
                send_custom_error(p, "Aggregation Error (ifindex)", aggr_error_txt(rc), strlen(aggr_error_txt(rc)));
                return -1;
            }

            // get old config
            vtss_appl_aggr_group_conf_t old_grp_conf;
            if ((rc = vtss_appl_aggregation_group_get(grp_ifindex, &old_grp_conf)) != VTSS_RC_OK) {
                send_custom_error(p, "Aggregation Error (get group config)", aggr_error_txt(rc), strlen(aggr_error_txt(rc)));
                return -1;
            }

            // prepare new group config
            vtss_appl_aggr_group_conf_t grp_conf;
            memset(&grp_conf, 0, sizeof(grp_conf));

            // get new mode
            if (!cyg_httpd_form_variable_int_fmt(p, &newval, "groupmode_%d", aggrgroup)) {
                const char *errmess = "Group mode not found";
                send_custom_error(p, "Aggregation Error", errmess, strlen(errmess));
                return -1;
            }
            vtss_appl_aggr_mode_t new_mode = (vtss_appl_aggr_mode_t)newval;

            // setup new group members unless new mode is DISABLED
            if (new_mode != VTSS_APPL_AGGR_GROUP_MODE_DISABLED) {
                for (iport = 0; iport < port_count; iport++) {
                    if ( aggr[aggrgroup][iport] == 1 ) { // this port is a member of the group
                        (void)portlist_state_set(sid, iport, &grp_conf.cfg_ports.member);
                    }
                }
            }

            // check for config changes - skip if no changes detected
            if (memcmp(&grp_conf, &old_grp_conf, sizeof(grp_conf)) == 0) {
                // no changes to this group configuration
                continue;
            }

            // if we are changing from one "active" (i.e. non-disabled) mode to another "active" mode,
            // then we need to temporarily disable the group to cleanup properly.
            if (new_mode != VTSS_APPL_AGGR_GROUP_MODE_DISABLED && old_grp_conf.mode != VTSS_APPL_AGGR_GROUP_MODE_DISABLED) {
                grp_conf.mode = VTSS_APPL_AGGR_GROUP_MODE_DISABLED;
                if ((rc = vtss_appl_aggregation_group_set(grp_ifindex, &grp_conf)) != VTSS_RC_OK) {
                    send_custom_error(p, "Aggregation Error (delete group config)", aggr_error_txt(rc), strlen(aggr_error_txt(rc)));
                    return -1;
                }
            }

            // set new group configuration
            grp_conf.mode = new_mode;
            if ((rc = vtss_appl_aggregation_group_set(grp_ifindex, &grp_conf)) != VTSS_RC_OK) {
                send_custom_error(p, "Aggregation Error (set group config)", aggr_error_txt(rc), strlen(aggr_error_txt(rc)));
                return -1;
            }

            if (grp_conf.mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_ACTIVE || grp_conf.mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_PASSIVE) {
                vtss_appl_lacp_group_conf_t lacp_conf;

                if ((rc = vtss_appl_lacp_group_conf_get(aggrgroup, &lacp_conf)) != VTSS_RC_OK) {
                    send_custom_error(p, "Aggregation Error (get LACP config)", aggr_error_txt(rc), strlen(aggr_error_txt(rc)));
                    return -1;
                }

                lacp_conf.revertive = cyg_httpd_form_variable_check_fmt(p, "revertive_%d", aggrgroup);

                if (!cyg_httpd_form_variable_int_fmt(p, &newval, "maxbundle_%d", aggrgroup)) {
                    const char *errmess = "MaxBundle value not found";
                    send_custom_error(p, "Aggregation Error", errmess, strlen(errmess));
                    return -1;
                }
                lacp_conf.max_bundle = (u32)newval;
                if ((rc = vtss_appl_lacp_group_conf_set(aggrgroup, &lacp_conf)) != VTSS_RC_OK) {
                    send_custom_error(p, "Aggregation Error (set LACP config)", lacp_error_txt(rc), strlen(lacp_error_txt(rc)));
                    return 0;
                }
            }
        }
        redirect(p, "/aggr_groups.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */

        vtss_appl_aggr_mode_t mode_configs[AGGR_LLAG_CNT_];
#ifdef VTSS_SW_OPTION_LACP
        vtss_appl_lacp_group_conf_t lacp_configs[AGGR_LLAG_CNT_];
#endif


        for (aggrgroup = 1; aggrgroup < aggr_count; aggrgroup++) { // Get each group members
            int aggr_grp_idx = aggrgroup - 1;

            aggr_mgmt_group_no_t aggr_group_id = web_aggr_id2no(aggrgroup) ;

            vtss_ifindex_t grp_ifindex;
            if ((rc = vtss_ifindex_from_llag(sid, aggr_group_id, &grp_ifindex)) != VTSS_RC_OK) {
                send_custom_error(p, "Aggregation Error (ifindex)", aggr_error_txt(rc), strlen(aggr_error_txt(rc)));
                return -1;
            }

            vtss_appl_aggr_group_conf_t grp_conf;
            if ((rc = vtss_appl_aggregation_group_get(grp_ifindex, &grp_conf)) != VTSS_RC_OK) {
                send_custom_error(p, "Aggregation Error (get group config)", aggr_error_txt(rc), strlen(aggr_error_txt(rc)));
                return -1;
            }

            for (iport = VTSS_PORT_NO_START ; iport < port_count ; iport++) {
                aggr[aggrgroup][iport - VTSS_PORT_NO_START] = portlist_state_get(sid, iport, &grp_conf.cfg_ports.member);
            }

            mode_configs[aggr_grp_idx] = grp_conf.mode;

#ifdef VTSS_SW_OPTION_LACP
            if ((rc = vtss_appl_lacp_group_conf_get(aggr_group_id, &(lacp_configs[aggr_grp_idx]))) != VTSS_RC_OK) {
                send_custom_error(p, "Aggregation Error (get group config)", aggr_error_txt(rc), strlen(aggr_error_txt(rc)));
                return -1;
            }
#endif
        }

        for (iport = VTSS_PORT_NO_START; iport < port_count; iport++) {
            aggr[0][iport - VTSS_PORT_NO_START] = 1; // default belong to Normal group
            for (aggrgroup = 1; aggrgroup < aggr_count; aggrgroup++) {
                if (aggr[aggrgroup][iport - VTSS_PORT_NO_START] == 1) {
                    aggr[0][iport - VTSS_PORT_NO_START] = 0; // if this port belongs to any other aggr group, remove it from the Normal group.
                    //break;
                }
            }
        }

        cyg_httpd_start_chunked("html");

        for (aggrgroup = 0 ; aggrgroup < aggr_count; aggrgroup++) {   // send out the whole aggr[][] structure to client.
            for (iport = 0 ; iport < port_count; iport++) { // VTSS_PORTS - 2 for "Do not send stack port 25,26"
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/", aggr[aggrgroup][iport]);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            if (aggrgroup == 0) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "0/0/0");
            } else {
#ifdef VTSS_SW_OPTION_LACP
                int aggr_grp_idx = aggrgroup - 1;
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u", mode_configs[aggr_grp_idx],
                              lacp_configs[aggr_grp_idx].revertive, lacp_configs[aggr_grp_idx].max_bundle);
#else
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "0/0/0");
#endif
            }
            cyg_httpd_write_chunked(p->outbuffer, ct);

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|");
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u", glag_count[0], glag_count[1]);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}


static i32 handler_status_aggregation(CYG_HTTPD_STATE *p)
{
    vtss_isid_t              isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_rc                  rc;
    int                      ct;
    aggr_mgmt_group_no_t     aggr_no;
    vtss_ifindex_t           ifindex;
    vtss_appl_aggr_group_status_t grp_status;
    mesa_port_no_t           port_no;
    mesa_port_list_t         cfg_ports, aggr_ports;
    char                     cfgp_buf[AGGR_PORT_BUF_SIZE];
    char                     aggrp_buf[AGGR_PORT_BUF_SIZE];

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_AGGR)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    // Loop over all existing (not configurable) switches in usid order...
    for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END_; aggr_no++) {
        if ((rc = vtss_ifindex_from_llag(isid, aggr_no, &ifindex)) != VTSS_RC_OK) {
            return rc;
        }

        if (vtss_appl_aggregation_status_get(ifindex, &grp_status) != VTSS_RC_OK) {
            continue;
        }

        for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            cfg_ports[port_no]  = portlist_state_get(isid, port_no, &grp_status.cfg_ports.member);
            aggr_ports[port_no] = portlist_state_get(isid, port_no, &grp_status.aggr_ports.member);
        }

        icli_port_list_info_txt(isid, cfg_ports, cfgp_buf, FALSE);
        icli_port_list_info_txt(isid, aggr_ports, aggrp_buf, FALSE);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u|%s%u|%s|%s|%s|%s@",
                      aggr_no,
                      AGGR_MGMT_GROUP_IS_LAG(aggr_no) ? "LLAG" : "GLAG",
                      AGGR_MGMT_NO_TO_ID(aggr_no),
                      grp_status.type,
                      grp_status.speed == MESA_SPEED_10M   ? "10M" :
                      grp_status.speed == MESA_SPEED_100M  ? "100M" :
                      grp_status.speed == MESA_SPEED_1G    ? "1G" :
                      grp_status.speed == MESA_SPEED_2500M ? "2G5" :
                      grp_status.speed == MESA_SPEED_10G   ? "10G" :
                      grp_status.speed == MESA_SPEED_12G   ? "12G" : "Undefined",
                      strlen(cfgp_buf) ? cfgp_buf : "none",
                      strlen(aggrp_buf) ? aggrp_buf : "none");

        cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    cyg_httpd_write_chunked("@", 1);
    cyg_httpd_end_chunked();

    return -1;
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_aggr_common, "/config/aggr_common", handler_config_aggr_common);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_aggr_groups, "/config/aggr_groups", handler_config_aggr_groups);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_status_aggregation, "/stat/aggregation_status", handler_status_aggregation);

