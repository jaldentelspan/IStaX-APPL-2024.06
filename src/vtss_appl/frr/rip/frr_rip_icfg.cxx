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

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr_rip_icfg.hxx"
#include "frr_rip_api.hxx"
#include "icfg_api.h"
#include "icli_porting_util.h"
#include "ip_utils.hxx"  // For the operator of mesa_ipv4_network_t, vtss_ipv4_prefix_to_mask()
#include "mgmt_api.h"    // For mgmt_vid_list_to_txt()

/****************************************************************************/
/** Module default trace group declaration                                  */
/****************************************************************************/
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_ICFG
#include "frr_trace.hxx"  // For module trace group definitions

/******************************************************************************/
/** Namespaces using-declaration                                              */
/******************************************************************************/
using namespace vtss;

/******************************************************************************/
/** Internal variables and APIs                                               */
/******************************************************************************/
/* Router configured mode: Offset-list */
static mesa_rc frr_rip_router_conf_offset_list(const vtss_icfg_query_request_t *req,
                                               vtss_icfg_query_result_t *result)
{
    /* Command: offset-list <access_list_name> { in | out } <metric_value> [
     * vlan <vlan_id> ] */

    vtss::Vector<vtss::Pair<vtss_appl_rip_offset_entry_key_t, vtss_appl_rip_offset_entry_data_t>>
            conf;

    /* Walk through all offset-list entries */
    if (vtss_appl_rip_offset_list_conf_get_all(conf) == VTSS_RC_OK) {
        for (const auto &itr : conf) {
            vtss_ifindex_elm_t ife;
            if (!ifindex_is_none(itr.first.ifindex) &&
                vtss_ifindex_decompose(itr.first.ifindex, &ife) != VTSS_RC_OK) {
                continue;
            }

            VTSS_RC(vtss_icfg_printf(
                        result, " offset-list %s %s %u", itr.second.name.name,
                        itr.first.direction == VTSS_APPL_RIP_OFFSET_DIRECTION_IN
                        ? "in"
                        : "out",
                        itr.second.offset_metric));
            if (!ifindex_is_none(itr.first.ifindex)) {
                VTSS_RC(vtss_icfg_printf(result, " vlan %u", ife.ordinal));
            }

            VTSS_RC(vtss_icfg_printf(result, "\n"));
        }
    }

    return VTSS_RC_OK;
}

/* Router configured mode: RIP network */
static mesa_rc frr_rip_router_conf_network(const vtss_icfg_query_request_t *req,
                                           vtss_icfg_query_result_t *result)
{
    /* Command: network <ipv4_addr> <wildcard_mask> */

    mesa_ipv4_network_t *cur_network_key_p = NULL;  // Get-First operation
    mesa_ipv4_network_t cur_network_key, next_network_key;
    mesa_ipv4_t network_mask;
    BufStream<SBuf32> str_buf;

    /* Iterate through all existing entries. */
    while (vtss_appl_rip_network_conf_itr(cur_network_key_p,
                                          &next_network_key) == VTSS_RC_OK) {
        // Switch to current data for next loop
        if (!cur_network_key_p) {  // Get-First operation
            cur_network_key_p =
                &cur_network_key;  // Switch to Get-Next operation
        }

        cur_network_key = next_network_key;

        if (vtss_appl_rip_network_conf_get(cur_network_key_p) != VTSS_RC_OK) {
            continue;
        }

        network_mask = vtss_ipv4_prefix_to_mask(cur_network_key_p->prefix_size);

        str_buf.clear();
        str_buf << AsIpv4(cur_network_key_p->address);
        VTSS_RC(vtss_icfg_printf(result, " network %s", str_buf.cstring()));

        if (cur_network_key_p->prefix_size !=
            FRR_ICLI_ipv4_addr_to_prefix(cur_network_key_p->address)) {
            network_mask = ~network_mask;
            str_buf.clear();
            str_buf << AsIpv4(network_mask);
            VTSS_RC(vtss_icfg_printf(result, " %s", str_buf.cstring()));
        }

        VTSS_RC(vtss_icfg_printf(result, "\n"));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: RIP neighbor */
static mesa_rc frr_rip_router_conf_neighbor(const vtss_icfg_query_request_t *req,
                                            vtss_icfg_query_result_t *result)
{
    /* Command: neighbor <ipv4_addr> */

    mesa_ipv4_t *cur_addr = NULL, next_addr;
    BufStream<SBuf32> str_buf;

    /* Iterate through all neighbor connections. */
    while (vtss_appl_rip_neighbor_conf_itr(cur_addr, &next_addr) == VTSS_RC_OK) {
        // Because neighbor configuration does not have data,
        // vtss_appl_rip_neighbor_conf_get() isn't needed here.
        str_buf.clear();
        str_buf << AsIpv4(next_addr);
        VTSS_RC(vtss_icfg_printf(result, " neighbor %s\n", str_buf.cstring()));

        if (!cur_addr) {
            cur_addr = &next_addr;
        }
    }

    return VTSS_RC_OK;
}

/* Router configured mode: passive interface
 * This function only prints the interface that passive interface mode is
 * different from default mode. E.g., if default mode is enabled, only
 * non-passive interfaces are printed, vice versa. 'result' is the same no
 * matter the user types 'show running-config' or 'show running-config
 * all-defaults', this is because the command 'passive-interface default'
 * affects to all interfaces, if the interfaces have the same passive mode
 * as default mode, they do not need to be printed.
 */
static mesa_rc frr_rip_router_conf_passive(vtss_icfg_query_result_t *result,
                                           bool passive_default)
{
    /* Command: passive-interface vlan <vid_list> */

    vtss_ifindex_t *cur_ifindex = NULL;
    vtss_ifindex_t next_ifindex;
    vtss_appl_rip_router_intf_conf_t conf;
    vtss::Set<mesa_vid_t> vid_list;
    vtss_ifindex_elm_t ife;

    /* Iterate through all router interfaces. */
    while (vtss_appl_rip_router_intf_conf_itr(cur_ifindex, &next_ifindex) ==
           VTSS_RC_OK) {
        if (vtss_appl_rip_router_intf_conf_get(next_ifindex, &conf) != VTSS_RC_OK ||
            vtss_ifindex_decompose(next_ifindex, &ife) != VTSS_RC_OK) {
            continue;
        }

        if (conf.passive_enabled != passive_default) {
            vid_list.insert((mesa_vid_t)ife.ordinal);
        }

        if (!cur_ifindex) {
            cur_ifindex = &next_ifindex;
        }
    }

    if (!vid_list.empty()) {
        VTSS_RC(vtss_icfg_printf(result, "%s passive-interface vlan %s\n",
                                 passive_default ? " no" : "",
                                 mgmt_vid_list_to_txt(vid_list).c_str()));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: timers */
static mesa_rc frr_rip_router_conf_timers(const vtss_icfg_query_request_t *req,
                                          vtss_icfg_query_result_t *result)
{
    vtss_appl_rip_router_conf_t conf, def_conf;

    VTSS_RC(frr_rip_router_conf_def(&def_conf));
    VTSS_RC(vtss_appl_rip_router_conf_get(&conf));

    bool is_changed =
        (conf.timers.update_timer != def_conf.timers.update_timer ||
         conf.timers.invalid_timer != def_conf.timers.invalid_timer ||
         conf.timers.garbage_collection_timer !=
         def_conf.timers.garbage_collection_timer);

    if (req->all_defaults || is_changed) {
        VTSS_RC(vtss_icfg_printf(
                    result, " timers basic %u %u %u\n", conf.timers.update_timer,
                    conf.timers.invalid_timer, conf.timers.garbage_collection_timer));
    }

    return VTSS_RC_OK;
}

/* ICFG callback functions for RIP router mode.
 * Notice that the RIP configured router mode is a CLI submode.
 * We should prefix a space character before any command text output.
 * For example,
 * vtss_icfg_printf(result, " submode_cmd ...")
 */
static mesa_rc frr_rip_router_conf_mode(const vtss_icfg_query_request_t *req,
                                        vtss_icfg_query_result_t *result)
{
    mesa_rc rc;
    vtss_appl_rip_router_conf_t conf, def_conf;

    /* Get current RIP router configuration */
    if ((rc = vtss_appl_rip_router_conf_get(&conf)) != VTSS_RC_OK) {
        VTSS_TRACE(WARNING) << "Get RIP router configuration failed."
                            << " (" << error_txt(rc) << ")";
        return FALSE;
    }

    /* Terminate the process if the router mode is disabled.
     * Since all RIP router configured parameters are significant only when the
     * router mode is enabled.
     */
    if (!conf.router_mode) {
        return VTSS_RC_OK;
    }

    /* Important!!!
     * All commands under 'RIP router configured mode' MUST below this line.
     *
     * General rules:
     * 1. General configuration should be applied before network
     *    configuration since the RIP process is start to run on the
     *    interfaces after network commands is applied.
     *
     * The configured commands sequence MUST be considered.
     * Current sequence:
     * 1. version
     * 2. timers
     * 3. redistribute protocol type
     * 4. passive-interface mode
     * 5. offset-list
     * 6. network
     * 7. neighbor
     * 8. default route redistribution
     * 9. default metric
     * 10. administrative distance
     */

    /* Get the default configuration */
    VTSS_RC(frr_rip_router_conf_def(&def_conf));

    /* 1. Router configured mode: version */
    bool is_ver_changed = (conf.version != def_conf.version);
    if (req->all_defaults || is_ver_changed) {
        if (conf.version == VTSS_APPL_RIP_GLOBAL_VER_1) {
            VTSS_RC(vtss_icfg_printf(result, " version 1\n"));
        } else if (conf.version == VTSS_APPL_RIP_GLOBAL_VER_2) {
            VTSS_RC(vtss_icfg_printf(result, " version 2\n"));
        } else {
            VTSS_RC(vtss_icfg_printf(result, " no version\n"));
        }
    }

    /* 2. Router configured mode: timers */
    VTSS_RC(frr_rip_router_conf_timers(req, result));

    /* 3. Router configured mode: redistribute protocol type */
    StringStream str_buf;
    const vtss::Vector<std::string> redist_protocol_type = {"connected",
                                                            "static", "ospf"
                                                           };
    for (u32 idx = 0; idx < VTSS_APPL_RIP_REDIST_PROTOCOL_COUNT; ++idx) {
        bool is_redist_conf_changed =
            (conf.redist_conf[idx].is_enabled !=
             def_conf.redist_conf[idx].is_enabled ||
             conf.redist_conf[idx].is_specific_metric !=
             def_conf.redist_conf[idx].is_specific_metric ||
             conf.redist_conf[idx].metric != def_conf.redist_conf[idx].metric);
        if (req->all_defaults || is_redist_conf_changed) {
            str_buf.clear();
            if (!conf.redist_conf[idx].is_enabled) {
                str_buf << " no";
            }

            str_buf << " redistribute " << redist_protocol_type[idx];
            if (conf.redist_conf[idx].is_specific_metric) {
                str_buf << " metric " << conf.redist_conf[idx].metric;
            }

            VTSS_RC(vtss_icfg_printf(result, "%s\n", str_buf.cstring()));
        }
    }

    /* 4. Router configured mode: passive-interface mode */
    // Print default mode first, if default mode is enabled, only non-passive
    // interfaces are printed, vice versa.
    bool is_def_def_passive_intf_changed =
        (conf.def_passive_intf != def_conf.def_passive_intf);
    if (req->all_defaults || is_def_def_passive_intf_changed) {
        VTSS_RC(vtss_icfg_printf(result, " %spassive-interface default\n",
                                 conf.def_passive_intf ? "" : "no "));
    }

    VTSS_RC(frr_rip_router_conf_passive(result, conf.def_passive_intf));

    /* 5. Router configured mode: offset-list */
    VTSS_RC(frr_rip_router_conf_offset_list(req, result));

    /* 6. Router configured mode: network */
    VTSS_RC(frr_rip_router_conf_network(req, result));

    /* 7. Router configured mode: neighbor */
    VTSS_RC(frr_rip_router_conf_neighbor(req, result));

    /* 8. Router configured mode: default route redistribution */
    bool is_def_route_redist_changed =
        (conf.def_route_redist != def_conf.def_route_redist);
    if (req->all_defaults || is_def_route_redist_changed) {
        VTSS_RC(vtss_icfg_printf(result, " %sdefault-information originate\n",
                                 conf.def_route_redist ? "" : "no "));
    }

    /* 9. Router configured mode: default metric */
    bool is_redist_def_metric_changed =
        (conf.redist_def_metric != def_conf.redist_def_metric);
    if (req->all_defaults || is_redist_def_metric_changed) {
        VTSS_RC(vtss_icfg_printf(result, " default-metric %u\n",
                                 conf.redist_def_metric));
    }

    /* 10. Router configured mode: administrative distance */
    bool is_admin_distance_changed =
        (conf.admin_distance != def_conf.admin_distance);
    if (req->all_defaults || is_admin_distance_changed) {
        VTSS_RC(vtss_icfg_printf(result, " distance %u\n", conf.admin_distance));
    }

    return VTSS_RC_OK;
}

/* ICFG callback functions for VLAN interface mode */
static mesa_rc frr_rip_vlan_intf_conf_mode(const vtss_icfg_query_request_t *req,
                                           vtss_icfg_query_result_t *result)
{
    vtss_ifindex_t ifindex;
    vtss_appl_rip_intf_conf_t conf, def_conf;
    mesa_rc rc;

    VTSS_RC(vtss_ifindex_from_vlan(req->instance_id.vlan, &ifindex));

    /* Get RIP interface configuration */
    if ((rc = vtss_appl_rip_intf_conf_get(ifindex, &conf)) != VTSS_RC_OK) {
        VTSS_TRACE(INFO) << "Get RIP interface configuration failed." << " (" << error_txt(rc) << ")";
        return VTSS_RC_ERROR;
    }

    VTSS_RC(frr_rip_intf_conf_def(&ifindex, &def_conf));

    /* Print the configuration */
    // Send version
    vtss::Vector<std::string> ver_str = {" none", " 1", " 2", " 1 2", ""};
    if (req->all_defaults || conf.send_ver != def_conf.send_ver) {
        VTSS_RC(vtss_icfg_printf(
                    result, " %sip rip send version%s\n",
                    conf.send_ver == VTSS_APPL_RIP_INTF_SEND_VER_NOT_SPECIFIED
                    ? "no "
                    : "",
                    ver_str[static_cast<int>(conf.send_ver)].c_str()));
    }

    // Receive version
    if (req->all_defaults || conf.recv_ver != def_conf.recv_ver) {
        VTSS_RC(vtss_icfg_printf(
                    result, " %sip rip receive version%s\n",
                    conf.recv_ver == VTSS_APPL_RIP_INTF_RECV_VER_NOT_SPECIFIED
                    ? "no "
                    : "",
                    ver_str[static_cast<int>(conf.recv_ver)].c_str()));
    }

    // Split horizon
    if (req->all_defaults ||
        conf.split_horizon_mode != def_conf.split_horizon_mode) {
        VTSS_RC(vtss_icfg_printf(
                    result, " %sip rip split-horizon%s\n",
                    conf.split_horizon_mode == VTSS_APPL_RIP_SPLIT_HORIZON_MODE_DISABLED
                    ? "no "
                    : "",
                    conf.split_horizon_mode ==
                    VTSS_APPL_RIP_SPLIT_HORIZON_MODE_POISONED_REVERSE
                    ? " poisoned-reverse"
                    : ""));
    }

    // Authentication mode
    // Command: ip rip authentication { mode { text|md5 }

    if (req->all_defaults || conf.auth_type != def_conf.auth_type) {
        VTSS_RC(vtss_icfg_printf(
                    result, " ip rip authentication mode %s\n",
                    conf.auth_type == VTSS_APPL_RIP_AUTH_TYPE_SIMPLE_PASSWORD
                    ? "text"
                    : conf.auth_type == VTSS_APPL_RIP_AUTH_TYPE_MD5 ? "md5"
                    : ""));
    } else if (req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " no ip rip authentication mode\n"));
    }

    // Command: ip rip authentication key-chain <word1-31>
    if (strlen(conf.md5_key_chain_name)) {
        VTSS_RC(vtss_icfg_printf(result,
                                 " ip rip authentication key-chain %s\n",
                                 conf.md5_key_chain_name));
    } else if (req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result,
                                 " no ip rip authentication key-chain\n"));
    }

    // Command: ip rip authentication string { unencrypted <word1-15> |
    // encrypted <word128> }

    if (strlen(conf.simple_pwd)) {
        VTSS_RC(vtss_icfg_printf(result,
                                 " ip rip authentication string encrypted %s\n",
                                 conf.simple_pwd));
    } else if (req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " no ip rip authentication string\n"));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
/** Module ICFG initialization                                                */
/******************************************************************************/
/**
 * \brief FRR ICFG initialization function.
 *
 * Call once, preferably from the INIT_CMD_INIT section of the module's init()
 * function.
 */
mesa_rc frr_rip_icfg_init(void)
{
    /* Register callback functions to ICFG module.
     *  The configuration divided into two groups for this module.
     *  1. Router mode configuration
     *  2. VLAN interface mode configuration
     */
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_RIP_ROUTER_CONF,    "rip", frr_rip_router_conf_mode));
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_INTERFACE_VLAN_RIP, "rip", frr_rip_vlan_intf_conf_mode));

    return VTSS_RC_OK;
}

