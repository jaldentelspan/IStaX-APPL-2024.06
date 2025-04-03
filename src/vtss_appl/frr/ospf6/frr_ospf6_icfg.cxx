/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "frr_ospf6_icfg.hxx"
#include "frr_ospf6_api.hxx"
#include "icfg_api.h"
#include "icli_porting_util.h"
#include "ip_utils.hxx" // For the operator of mesa_ipv4_network_t, vtss_ipv4_prefix_to_mask()
#include "mgmt_api.h"   // For mgmt_vid_list_to_txt()

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
/* ICFG callback functions for VLAN interface mode */
static mesa_rc frr_ospf6_vlan_intf_conf_mode(const vtss_icfg_query_request_t *req,
                                             vtss_icfg_query_result_t *result)
{
    vtss_ifindex_t ifindex;
    vtss_appl_ospf6_intf_conf_t conf, def_conf;

    VTSS_RC(vtss_ifindex_from_vlan(req->instance_id.vlan, &ifindex));
    if (vtss_appl_ospf6_intf_conf_get(ifindex, &conf) != VTSS_RC_OK) {
        return VTSS_RC_OK;  // Quit silently when the entry does not exists
    }

    VTSS_RC(frr_ospf6_intf_conf_def(&ifindex, &def_conf));

    /* Commands: ipv6 ospf priority <0-255> */
    if (req->all_defaults || conf.priority != def_conf.priority) {
        VTSS_RC(vtss_icfg_printf(result, " ipv6 ospf priority %d\n", conf.priority));
    }

    /* Commands: ipv6 ospf cost <1-65535>
     * Notice that the parameter 'cost' is significant only when
     * parameter 'is_specific_cost' is true.
     */
    if (req->all_defaults || conf.is_specific_cost != def_conf.is_specific_cost ||
        conf.cost != def_conf.cost) {
        if (conf.is_specific_cost) {
            VTSS_RC(vtss_icfg_printf(result, " ipv6 ospf cost %u\n", conf.cost));
        } else {
            VTSS_RC(vtss_icfg_printf(result, " no ip ospf cost\n"));
        }
    }

    // [no] ipv6 ospf mtu-ignore
    if (req->all_defaults || conf.mtu_ignore) {
        VTSS_RC(vtss_icfg_printf(result, " %sipv6 ospf mtu-ignore\n", conf.mtu_ignore ? ""  : "no "));
    }

    if (req->all_defaults || conf.dead_interval != def_conf.dead_interval) {
        VTSS_RC(vtss_icfg_printf(result, " ipv6 ospf dead-interval %d\n",
                                 conf.dead_interval));
    }
    /* Commands: ipv6 ospf hello-interval <1-65535> */
    if (req->all_defaults || conf.hello_interval != def_conf.hello_interval) {
        VTSS_RC(vtss_icfg_printf(result, " ipv6 ospf hello-interval %d\n",
                                 conf.hello_interval));
    }

    /* Commands: ipv6 ospf retransmit-interval <1-65535> */
    if (req->all_defaults ||
        conf.retransmit_interval != def_conf.retransmit_interval) {
        VTSS_RC(vtss_icfg_printf(result, " ipv6 ospf retransmit-interval %d\n",
                                 conf.retransmit_interval));
    }

    /* Commands: ipv6 ospf retransmit-interval <1-65535> */
    if (req->all_defaults ||
        conf.is_passive != def_conf.is_passive) {
        if (conf.is_passive == true) {
            VTSS_RC(vtss_icfg_printf(result, " ipv6 ospf passive\n"));
        } else {
            VTSS_RC(vtss_icfg_printf(result, " no ipv6 ospf passive\n"));
        }
    }


    /* Commands: ipv6 ospf transmit-delay <1-3600> */
    if (req->all_defaults ||
        conf.transmit_delay != def_conf.transmit_delay) {
        VTSS_RC(vtss_icfg_printf(result, " ipv6 ospf transmit-delay %d\n",
                                 conf.transmit_delay));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: route redistribution */
const vtss::Vector<std::string> redist_protocol_type = {"connected", "static",
                                                        "rip"
                                                       };
static mesa_rc frr_ospf6_router_conf_router_redistribution(
    const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    /* Commands:
     * redistribute { static | connected }
     * no redistribute { static | connected }
     */

    vtss_appl_ospf6_id_t id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf6_router_conf_t def_conf, conf;
    StringStream str_buf;

    VTSS_RC(frr_ospf6_router_conf_def(&id, &def_conf));
    VTSS_RC(vtss_appl_ospf6_router_conf_get(id, &conf));

    for (u32 idx = 0; idx < VTSS_APPL_OSPF6_REDIST_PROTOCOL_COUNT; ++idx) {
        if (req->all_defaults || conf.redist_conf[idx].is_redist_enable != def_conf.redist_conf[idx].is_redist_enable) {
            str_buf.clear();
            if (conf.redist_conf[idx].is_redist_enable == false) {
                str_buf << " no";
            }

            str_buf << " redistribute " << redist_protocol_type[idx];

            VTSS_RC(vtss_icfg_printf(result, "%s\n", str_buf.cstring()));
        }
    }

    return VTSS_RC_OK;
}

/* Router configured mode: default route redistribution */
static mesa_rc frr_ospf6_router_conf_def_route(const vtss_icfg_query_request_t *req,
                                               vtss_icfg_query_result_t *result)
{
    return VTSS_RC_OK;
}

/* Router configured mode: default metric */
static mesa_rc frr_ospf6_router_conf_def_metric(const vtss_icfg_query_request_t *req,
                                                vtss_icfg_query_result_t *result)
{
    /* Commands:
     * default-metric <0-16777214>
     * no redistribute default-metric
     */

    vtss_appl_ospf6_id_t id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf6_router_conf_t def_conf, conf;

    VTSS_RC(frr_ospf6_router_conf_def(&id, &def_conf));
    VTSS_RC(vtss_appl_ospf6_router_conf_get(id, &conf));

    return VTSS_RC_OK;
}
/* Router configured mode: administrative distance */
static mesa_rc frr_ospf6_router_conf_admin_distance(
    const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    /* Commands:
     * distance <1-255>
     * no distance
     */

    vtss_appl_ospf6_id_t id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf6_router_conf_t def_conf, conf;

    VTSS_RC(frr_ospf6_router_conf_def(&id, &def_conf));
    VTSS_RC(vtss_appl_ospf6_router_conf_get(id, &conf));

    bool is_changed = (conf.admin_distance != def_conf.admin_distance);
    if (req->all_defaults || is_changed) {
        VTSS_RC(vtss_icfg_printf(result, " distance %u\n", conf.admin_distance));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: router ID */
static mesa_rc frr_ospf6_router_conf_router_id(const vtss_icfg_query_request_t *req,
                                               vtss_icfg_query_result_t *result)
{
    /* Commands:
     * router-id <router-id>
     * no router-id
     */

    vtss_appl_ospf6_id_t id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf6_router_conf_t def_router_conf, router_conf;
    BufStream<SBuf32> str_buf;

    VTSS_RC(frr_ospf6_router_conf_def(&id, &def_router_conf));
    VTSS_RC(vtss_appl_ospf6_router_conf_get(id, &router_conf));

    bool is_changed = (router_conf.router_id.is_specific_id !=
                       def_router_conf.router_id.is_specific_id ||
                       router_conf.router_id.id != def_router_conf.router_id.id);
    if (req->all_defaults || is_changed) {
        if (router_conf.router_id.is_specific_id) {
            str_buf.clear();
            str_buf << AsIpv4(router_conf.router_id.id);
            VTSS_RC(vtss_icfg_printf(result, " router-id %s\n",
                                     str_buf.cstring()));
        } else {
            VTSS_RC(vtss_icfg_printf(result, " no router-id\n"));
        }
    }

    return VTSS_RC_OK;
}

/* Router configured mode: interface-area */
static mesa_rc frr_ospf6_router_conf_interface_area(
    const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    /* Commands:
     * [no] interface vlan <vlan_list> area <area_id>
     */

    vtss_appl_ospf6_id_t id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf6_id_t next_id;
    vtss_ifindex_t *cur_ifindex_p = NULL;  // Get-First operation
    vtss_ifindex_t cur_ifindex, next_ifindex;
    vtss_appl_ospf6_router_intf_conf_t intf_conf;
    vtss_ifindex_elm_t ife;
    BufStream<SBuf32> str_buf;

    vtss::Set<mesa_vid_t> vid_list;

    /* Command: interface vlan <vlan_list> area <area_id>*/
    while (vtss_appl_ospf6_router_intf_conf_itr(&id, &next_id, cur_ifindex_p,
                                                &next_ifindex) == VTSS_RC_OK) {
        if (next_id != id) {
            break;  // Only current instance ID is required here
        }

        // Switch to current data for next loop
        if (!cur_ifindex_p) {              // Get-First operation
            cur_ifindex_p = &cur_ifindex;  // Switch to Get-Next operation
        }

        id = next_id;
        cur_ifindex = next_ifindex;

        if (vtss_appl_ospf6_router_intf_conf_get(id, cur_ifindex, &intf_conf) !=
            VTSS_RC_OK ||
            vtss_ifindex_decompose(cur_ifindex, &ife) != VTSS_RC_OK) {
            continue;
        }
        if (intf_conf.area_id.is_specific_id == true) {
            str_buf.clear();
            str_buf << AsIpv4(intf_conf.area_id.id);
            VTSS_RC(vtss_icfg_printf(result, " interface vlan %u area %s\n", ife.ordinal,
                                     str_buf.cstring()));
        }

    }

    return VTSS_RC_OK;
}

/* Router configured mode: area range */
static mesa_rc frr_ospf6_router_conf_area_range(const vtss_icfg_query_request_t *req,
                                                vtss_icfg_query_result_t *result)
{
    /* Command: area { <ipv4_addr> | <0-4294967295> } range <ipv4_addr>
     * <ipv4_netmask> [advertise| not-advertise] [cost <0-16777215>] */

    vtss_appl_ospf6_id_t id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf6_id_t next_id;
    vtss_appl_ospf6_area_id_t *cur_area_id_p = NULL, cur_area_id, next_area_id;
    mesa_ipv6_network_t *cur_network_p = NULL, cur_network, next_network;
    vtss_appl_ospf6_area_range_conf_t conf, def_conf;
    mesa_ipv6_t network_mask;
    BufStream<SBuf32> str_buf;

    /* Iterate through all existing entries. */
    while (vtss_appl_ospf6_area_range_conf_itr(&id, &next_id, cur_area_id_p,
                                               &next_area_id, cur_network_p,
                                               &next_network) == VTSS_RC_OK) {
        if (next_id != id) {
            break;  // Only current instance ID is required here
        }

        // Switch to current data for next loop
        if (!cur_area_id_p) {              // Get-First operation
            cur_area_id_p = &cur_area_id;  // Switch to Get-Next operation
        }

        if (!cur_network_p) {              // Get-First operation
            cur_network_p = &cur_network;  // Switch to Get-Next operation
        }

        id = next_id;
        cur_area_id = next_area_id;
        cur_network = next_network;

        if (frr_ospf6_area_range_conf_def(&id, &cur_area_id, &cur_network, &def_conf) != VTSS_RC_OK ||
            vtss_appl_ospf6_area_range_conf_get(id, cur_area_id, cur_network, &conf)  != VTSS_RC_OK) {
            continue;
        }

        vtss_conv_prefix_to_ipv6mask(cur_network.prefix_size, &network_mask);
        str_buf.clear();
        str_buf << AsIpv4(cur_area_id);
        VTSS_RC(vtss_icfg_printf(result, " area %s", str_buf.cstring()));

        str_buf.clear();
        str_buf << cur_network;
        VTSS_RC(vtss_icfg_printf(result, " range %s", str_buf.cstring()));

        if (req->all_defaults || def_conf.is_advertised != conf.is_advertised) {
            VTSS_RC(vtss_icfg_printf(
                        result, " %s",
                        conf.is_advertised ? "advertise" : "not-advertise"));
        }

        /* Commands: [cost <0-16777215>]
         * Notice that the parameter 'cost' is significant only when
         * parameter 'is_specific_cost' is true.
         */
        if (req->all_defaults || conf.is_specific_cost ||
            conf.cost != def_conf.cost) {
            VTSS_RC(vtss_icfg_printf(result, " cost %u\n", conf.cost));
        }

        VTSS_RC(vtss_icfg_printf(result, "\n"));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: stub area */
static mesa_rc frr_ospf6_router_conf_stub_area(const vtss_icfg_query_request_t *req,
                                               vtss_icfg_query_result_t *result)
{
    /* Command: area { <ipv4_addr> | <0-4294967295> } stub [ no-summary ] */

    vtss_appl_ospf6_id_t id = FRR_OSPF6_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf6_id_t next_id;
    vtss_appl_ospf6_area_id_t *cur_area_id_p = NULL, cur_area_id, next_area_id;
    vtss_appl_ospf6_stub_area_conf_t conf;
    StringStream str_buf;

    /* Iterate through all existing entries. */
    while (vtss_appl_ospf6_stub_area_conf_itr(&id, &next_id, cur_area_id_p,
                                              &next_area_id) == VTSS_RC_OK) {
        if (next_id != id) {
            break;  // Only current instance ID is required here
        }

        // Switch to current data for next loop
        if (!cur_area_id_p) {              // Get-First operation
            cur_area_id_p = &cur_area_id;  // Switch to Get-Next operation
        }

        id = next_id;
        cur_area_id = next_area_id;

        if (vtss_appl_ospf6_stub_area_conf_get(id, cur_area_id, &conf) != VTSS_RC_OK) {
            VTSS_TRACE(ERROR) << "vtss_appl_ospf6_stub_area_conf_get : failed";
            continue;
        }

        str_buf.clear();

        /* There's no default configuation for stub area, so all configurations
         * are shown in running-config.
         */
        str_buf << " area " << AsIpv4(cur_area_id) << " stub";
        if (conf.no_summary) {
            str_buf << " no-summary";
        }

        VTSS_RC(vtss_icfg_printf(result, "%s\n", str_buf.cstring()));
    }

    return VTSS_RC_OK;
}

/* ICFG callback functions for OSPF6 router mode.
 * Notice that the OSPF6 configured router mode is a CLI submode.
 * We should prefix a space character before any command text output.
 * For example,
 * vtss_icfg_printf(result, " submode_cmd ...")
 */
static mesa_rc frr_ospf6_router_conf_mode(const vtss_icfg_query_request_t *req,
                                          vtss_icfg_query_result_t *result)
{
    // TODO, require change
    // When multiple OSPF6 instances is supported, use variable
    // 'req->instance_id.generic_u32' instead of
    // FRR_OSPF6_DEFAULT_INSTANCE_ID
    vtss_appl_ospf6_id_t id = FRR_OSPF6_DEFAULT_INSTANCE_ID;

    if (vtss_appl_ospf6_get(id) != VTSS_RC_OK) {
        // Nothing to do if the OSPF6 instance ID is not existing
        return VTSS_RC_OK;
    }

    /* Important!!!
     * All commands under 'OSPF6 router configured mode' MUST below this line.
     *
     * The configured commands sequence MUST be considered.
     *
     * General rules:
     * 1. Specific router ID assignment MUST be applied first.
     * 2. General configuration should be applied before network area
     *    configuration since the OSPF6 process is starting after network
     *    area commands is applied.
     * 3. Default metric and route configuration should be applied after
     *    network area configuration since default routes should be advertised
     *    after normal routes are formed.
     *
     * Current sequence:
     *  router id  (MUST be first)
     *  area range
     *  stub area
     *  route redistribution
     *  interface area
     *  network area
     *  default route redistribution
     *  default metric
     *  administrative distance
     */

    /*  Router configured mode: router ID */
    (void)frr_ospf6_router_conf_router_id(req, result);

    /*  Router configured mode: area range */
    (void)frr_ospf6_router_conf_area_range(req, result);

    /*  Router configured mode: stub area */
    (void)frr_ospf6_router_conf_stub_area(req, result);

    /*  Router configured mode: route redistribution */
    (void)frr_ospf6_router_conf_router_redistribution(req, result);

    /*  Router configured mode: interface area */
    (void)frr_ospf6_router_conf_interface_area(req, result);

    /*  Router configured mode: default route redistribution */
    (void)frr_ospf6_router_conf_def_route(req, result);

    /*  Router configured mode: default metric */
    (void)frr_ospf6_router_conf_def_metric(req, result);

    /*  Router configured mode: administrative distance */
    (void)frr_ospf6_router_conf_admin_distance(req, result);
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
mesa_rc frr_ospf6_icfg_init(void)
{
    /* Register callback functions to ICFG module.
     *  The configuration divided into two groups for this module.
     *  1. Router mode configuration
     *  2. VLAN interface mode configuration
     */
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_OSPF6_ROUTER_CONF,    "ospf6", frr_ospf6_router_conf_mode));
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_INTERFACE_VLAN_OSPF6, "ospf6", frr_ospf6_vlan_intf_conf_mode));

    return VTSS_RC_OK;
}

