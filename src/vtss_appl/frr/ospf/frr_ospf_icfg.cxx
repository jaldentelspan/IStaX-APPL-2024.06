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
#include "frr_ospf_icfg.hxx"
#include "frr_ospf_api.hxx"
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
static mesa_rc frr_ospf_vlan_intf_conf_mode(const vtss_icfg_query_request_t *req,
                                            vtss_icfg_query_result_t *result)
{
    vtss_ifindex_t ifindex;
    vtss_appl_ospf_intf_conf_t conf, def_conf;

    VTSS_RC(vtss_ifindex_from_vlan(req->instance_id.vlan, &ifindex));
    if (vtss_appl_ospf_intf_conf_get(ifindex, &conf) != VTSS_RC_OK) {
        return VTSS_RC_OK;  // Quit silently when the entry does not exists
    }

    VTSS_RC(frr_ospf_intf_conf_def(&ifindex, &def_conf));

    /* Commands: ip ospf priority <0-255> */
    if (req->all_defaults || conf.priority != def_conf.priority) {
        VTSS_RC(vtss_icfg_printf(result, " ip ospf priority %d\n", conf.priority));
    }

    /* Commands: ip ospf cost <1-65535>
     * Notice that the parameter 'cost' is significant only when
     * parameter 'is_specific_cost' is true.
     */
    if (req->all_defaults || conf.is_specific_cost != def_conf.is_specific_cost ||
        conf.cost != def_conf.cost) {
        if (conf.is_specific_cost) {
            VTSS_RC(vtss_icfg_printf(result, " ip ospf cost %u\n", conf.cost));
        } else {
            VTSS_RC(vtss_icfg_printf(result, " no ip ospf cost\n"));
        }
    }

    // [no] ip ospf mtu-ignore
    if (req->all_defaults || conf.mtu_ignore) {
        VTSS_RC(vtss_icfg_printf(result, " %sip ospf mtu-ignore\n", conf.mtu_ignore ? ""  : "no "));
    }

    /* Commands:
     * ip ospf dead-interval { <1-65535> | minimal hello-multiplier <2-20> } */
    /* We output either 'ip ospf dead-interval <1-65535>' or
       'ip ospf dead-interval minimal hello-multiplier <2-20>'.
       It depends on the 'conf.is_fast_hello_enabled' value. */
    if (conf.is_fast_hello_enabled == false) {
        if (req->all_defaults || conf.dead_interval != def_conf.dead_interval) {
            VTSS_RC(vtss_icfg_printf(result, " ip ospf dead-interval %d\n",
                                     conf.dead_interval));
        }
    } else {
        if (req->all_defaults ||
            conf.is_fast_hello_enabled != def_conf.is_fast_hello_enabled) {
            VTSS_RC(vtss_icfg_printf(
                        result,
                        " ip ospf dead-interval minimal hello-multiplier %d\n",
                        conf.fast_hello_packets));
        }
    }

    /* Commands: ip ospf hello-interval <1-65535> */
    if (req->all_defaults || conf.hello_interval != def_conf.hello_interval) {
        VTSS_RC(vtss_icfg_printf(result, " ip ospf hello-interval %d\n",
                                 conf.hello_interval));
    }

    /* Commands: ip ospf retransmit-interval <1-65535> */
    if (req->all_defaults ||
        conf.retransmit_interval != def_conf.retransmit_interval) {
        VTSS_RC(vtss_icfg_printf(result, " ip ospf retransmit-interval %d\n",
                                 conf.retransmit_interval));
    }

    /* Command: ip ospf authentication [ <null|message-digest> ] */
    if (conf.auth_type != def_conf.auth_type) {
        VTSS_RC(vtss_icfg_printf(
                    result, " ip ospf authentication%s\n",
                    conf.auth_type == VTSS_APPL_OSPF_AUTH_TYPE_NULL
                    ? " null"
                    : conf.auth_type == VTSS_APPL_OSPF_AUTH_TYPE_MD5
                    ? " message-digest"
                    : ""));
    } else if (req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " no ip ospf authentication\n"));
    }

    /* Command: ip ospf authentication-key { unencrypted <word1-8> | encrypted
     * <word128> } */
    if (strlen(conf.auth_key)) {
        VTSS_RC(vtss_icfg_printf(result,
                                 " ip ospf authentication-key encrypted %s\n",
                                 conf.auth_key));
    } else if (req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, " no ip ospf authentication-key\n"));
    }

    /* Command: ip ospf message-digest-key  <1-255> md5 { unencrypted <word1-16>
     * | encrypted <word128> } */
    // Iternate the digest key configuration.
    vtss_ifindex_t next_ifindex;
    uint32_t next_pre_id, *current_pre_id = NULL;
    vtss_appl_ospf_md_key_id_t next_key_id;
    while (vtss_appl_ospf_intf_md_key_precedence_itr(
               &ifindex, &next_ifindex, current_pre_id, &next_pre_id) ==
           VTSS_RC_OK) {
        vtss_appl_ospf_auth_digest_key_t conf;
        if (next_ifindex != ifindex) {
            break;
        }

        if (!current_pre_id) {
            current_pre_id = &next_pre_id;
        }

        auto rc = vtss_appl_ospf_intf_md_key_precedence_get(
                      ifindex, next_pre_id, &next_key_id);
        if (rc != VTSS_RC_OK) {
            continue;
        }

        (void)vtss_appl_ospf_intf_auth_digest_key_get(ifindex, next_key_id,
                                                      &conf);
        VTSS_RC(vtss_icfg_printf(
                    result, " ip ospf message-digest-key %d md5 encrypted %s\n",
                    next_key_id, conf.digest_key));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: route redistribution */
const vtss::Vector<std::string> redist_protocol_type = {"connected", "static",
                                                        "rip"
                                                       };
static mesa_rc frr_ospf_router_conf_router_redistribution(
    const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    /* Commands:
     * redistribute { static | connected | rip }
     *              [ metric <0-16777214> ][ metric-type { 1 | 2 } ]
     * no redistribute { static | connected | rip }
     */

    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_router_conf_t def_conf, conf;
    StringStream str_buf;

    VTSS_RC(frr_ospf_router_conf_def(&id, &def_conf));
    VTSS_RC(vtss_appl_ospf_router_conf_get(id, &conf));

    for (u32 idx = 0; idx < VTSS_APPL_OSPF_REDIST_PROTOCOL_COUNT; ++idx) {
        bool is_changed =
            (conf.redist_conf[idx].type != def_conf.redist_conf[idx].type ||
             conf.redist_conf[idx].metric != def_conf.redist_conf[idx].metric);
        if (req->all_defaults || is_changed) {
            str_buf.clear();
            if (conf.redist_conf[idx].type ==
                VTSS_APPL_OSPF_REDIST_METRIC_TYPE_NONE) {
                str_buf << " no";
            }

            str_buf << " redistribute " << redist_protocol_type[idx];
            if (conf.redist_conf[idx].is_specific_metric) {
                str_buf << " metric " << conf.redist_conf[idx].metric;
            }

            if (conf.redist_conf[idx].type ==
                VTSS_APPL_OSPF_REDIST_METRIC_TYPE_1) {
                str_buf << " metric-type 1";
            }

            VTSS_RC(vtss_icfg_printf(result, "%s\n", str_buf.cstring()));
        }
    }

    return VTSS_RC_OK;
}

/* Router configured mode: default route redistribution */
static mesa_rc frr_ospf_router_conf_def_route(const vtss_icfg_query_request_t *req,
                                              vtss_icfg_query_result_t *result)
{
    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_router_conf_t def_conf, conf;
    StringStream str_buf;

    VTSS_RC(frr_ospf_router_conf_def(&id, &def_conf));
    VTSS_RC(vtss_appl_ospf_router_conf_get(id, &conf));

    bool is_changed =
        (conf.def_route_conf.type != def_conf.def_route_conf.type ||
         conf.def_route_conf.is_always != def_conf.def_route_conf.is_always ||
         conf.def_route_conf.is_specific_metric !=
         def_conf.def_route_conf.is_specific_metric ||
         conf.def_route_conf.metric != def_conf.def_route_conf.metric);
    if (req->all_defaults || is_changed) {
        str_buf.clear();
        if (conf.def_route_conf.type == VTSS_APPL_OSPF_REDIST_METRIC_TYPE_NONE) {
            str_buf << " no";
        }

        str_buf << " default-information originate";
        if (conf.def_route_conf.is_always) {
            str_buf << " always";
        }

        if (conf.def_route_conf.is_specific_metric) {
            str_buf << " metric " << conf.def_route_conf.metric;
        }

        if (conf.def_route_conf.type == VTSS_APPL_OSPF_REDIST_METRIC_TYPE_1) {
            str_buf << " metric-type 1";
        }

        VTSS_RC(vtss_icfg_printf(result, "%s\n", str_buf.cstring()));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: default metric */
static mesa_rc frr_ospf_router_conf_def_metric(const vtss_icfg_query_request_t *req,
                                               vtss_icfg_query_result_t *result)
{
    /* Commands:
     * default-metric <0-16777214>
     * no redistribute default-metric
     */

    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_router_conf_t def_conf, conf;

    VTSS_RC(frr_ospf_router_conf_def(&id, &def_conf));
    VTSS_RC(vtss_appl_ospf_router_conf_get(id, &conf));

    bool is_changed =
        (conf.is_specific_def_metric != def_conf.is_specific_def_metric ||
         conf.def_metric != def_conf.def_metric);
    if (req->all_defaults || is_changed) {
        if (conf.is_specific_def_metric) {
            VTSS_RC(vtss_icfg_printf(result, " default-metric %u\n",
                                     conf.def_metric));
        } else {
            VTSS_RC(vtss_icfg_printf(result, " no default-metric\n"));
        }
    }

    return VTSS_RC_OK;
}
/* Router configured mode: administrative distance */
static mesa_rc frr_ospf_router_conf_admin_distance(
    const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    /* Commands:
     * distance <1-255>
     * no distance
     */

    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_router_conf_t def_conf, conf;

    VTSS_RC(frr_ospf_router_conf_def(&id, &def_conf));
    VTSS_RC(vtss_appl_ospf_router_conf_get(id, &conf));

    bool is_changed = (conf.admin_distance != def_conf.admin_distance);
    if (req->all_defaults || is_changed) {
        VTSS_RC(vtss_icfg_printf(result, " distance %u\n", conf.admin_distance));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: stub router */
static mesa_rc frr_ospf_router_conf_stub_router(
    const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    /* Commands:
     * max-metric router-lsa {[on-startup <5-86400>] | [on-shutdown <5-100>] |
     * [administrative]} * 1
     * no max-metric router-lsa [on-startup] [on-shutdown] [administrative]
     */

    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_router_conf_t def_conf, conf;

    VTSS_RC(frr_ospf_router_conf_def(&id, &def_conf));
    VTSS_RC(vtss_appl_ospf_router_conf_get(id, &conf));

    bool is_changed = ((conf.stub_router.is_on_startup !=
                        def_conf.stub_router.is_on_startup ||
                        (conf.stub_router.is_on_startup &&
                         conf.stub_router.on_startup_interval !=
                         def_conf.stub_router.on_startup_interval)) ||
                       (conf.stub_router.is_on_shutdown !=
                        def_conf.stub_router.is_on_shutdown ||
                        (conf.stub_router.is_on_shutdown &&
                         conf.stub_router.on_shutdown_interval !=
                         def_conf.stub_router.on_shutdown_interval)) ||
                       conf.stub_router.is_administrative !=
                       def_conf.stub_router.is_administrative);
    if (req->all_defaults || is_changed) {
        if (req->all_defaults && !conf.stub_router.is_on_startup &&
            !conf.stub_router.is_on_shutdown &&
            !conf.stub_router.is_administrative) {
            VTSS_RC(vtss_icfg_printf(result, " no max-metric router-lsa\n"));
        } else {
            if (conf.stub_router.is_on_startup) {
                VTSS_RC(vtss_icfg_printf(
                            result, " max-metric router-lsa on-startup %d\n",
                            conf.stub_router.on_startup_interval));
            } else if (req->all_defaults) {
                VTSS_RC(vtss_icfg_printf(result,
                                         " no max-metric on-startup\n"));
            }

            if (conf.stub_router.is_on_shutdown) {
                VTSS_RC(vtss_icfg_printf(
                            result, " max-metric router-lsa on-shutdown %d\n",
                            conf.stub_router.on_shutdown_interval));
            } else if (req->all_defaults) {
                VTSS_RC(vtss_icfg_printf(result,
                                         " no max-metric on-shutdown\n"));
            }

            if (conf.stub_router.is_administrative) {
                VTSS_RC(vtss_icfg_printf(
                            result, " max-metric router-lsa administrative\n"));
            } else if (req->all_defaults) {
                VTSS_RC(vtss_icfg_printf(result,
                                         " no max-metric administrative\n"));
            }
        }
    }

    return VTSS_RC_OK;
}

/* Router configured mode: router ID */
static mesa_rc frr_ospf_router_conf_router_id(const vtss_icfg_query_request_t *req,
                                              vtss_icfg_query_result_t *result)
{
    /* Commands:
     * router-id <router-id>
     * no router-id
     */

    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_router_conf_t def_router_conf, router_conf;
    BufStream<SBuf32> str_buf;

    VTSS_RC(frr_ospf_router_conf_def(&id, &def_router_conf));
    VTSS_RC(vtss_appl_ospf_router_conf_get(id, &router_conf));

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

/* Router configured mode: passive-interface */
static mesa_rc frr_ospf_router_conf_passive_intf(
    const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    /* Commands:
     * [no] passive-interface default
     * passive-interface vlan <vlan_list>
     */

    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_router_conf_t def_router_conf, router_conf;
    vtss_appl_ospf_id_t next_id;
    vtss_ifindex_t *cur_ifindex_p = NULL;  // Get-First operation
    vtss_ifindex_t cur_ifindex, next_ifindex;
    vtss_appl_ospf_router_intf_conf_t intf_conf;
    vtss_ifindex_elm_t ife;

    VTSS_RC(frr_ospf_router_conf_def(&id, &def_router_conf));
    VTSS_RC(vtss_appl_ospf_router_conf_get(id, &router_conf));

    /* Command: [no] passive-interface default */
    bool is_changed = (router_conf.default_passive_interface !=
                       def_router_conf.default_passive_interface);
    if (req->all_defaults || is_changed) {
        VTSS_RC(vtss_icfg_printf(
                    result, "%s passive-interface default\n",
                    router_conf.default_passive_interface ? "" : " no"));
    }

    vtss::Set<mesa_vid_t> vid_list;

    /* Command: passive-interface vlan <vlan_list> */
    while (vtss_appl_ospf_router_intf_conf_itr(&id, &next_id, cur_ifindex_p,
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

        if (vtss_appl_ospf_router_intf_conf_get(id, cur_ifindex, &intf_conf) !=
            VTSS_RC_OK ||
            vtss_ifindex_decompose(cur_ifindex, &ife) != VTSS_RC_OK) {
            continue;
        }

        /* Notice that the output text should refer to the setting of defalut
         * mode.
         * When the defalut passive-interface mode is set, we should show the
         * 'no form' interfaces. */
        bool is_shown = router_conf.default_passive_interface
                        ? !intf_conf.passive_enabled
                        : intf_conf.passive_enabled;
        if (is_shown) {
            VTSS_TRACE(DEBUG) << "vid = " << ife.ordinal;
            vid_list.insert((mesa_vid_t)ife.ordinal);
        }
    }

    if (!vid_list.empty()) {
        VTSS_RC(vtss_icfg_printf(result, "%s passive-interface vlan %s\n",
                                 router_conf.default_passive_interface ? " no" : "",
                                 mgmt_vid_list_to_txt(vid_list).c_str()));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: network area */
static mesa_rc frr_ospf_router_conf_network_area(
    const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    /* Command: network <ipv4_addr> <wildcard_mask> area { <ipv4_addr> |
     * <0-4294967295> } */

    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_id_t next_id;
    mesa_ipv4_network_t *cur_network_key_p = NULL;  // Get-First operation
    mesa_ipv4_network_t cur_network_key, next_network_key;
    vtss_appl_ospf_area_id_t area_id;
    mesa_ipv4_t network_mask;
    BufStream<SBuf32> str_buf;

    /* Iterate through all existing entries. */
    while (vtss_appl_ospf_area_conf_itr(&id, &next_id, cur_network_key_p,
                                        &next_network_key) == VTSS_RC_OK) {
        if (next_id != id) {
            break;  // Only current instance ID is required here
        }

        // Switch to current data for next loop
        if (!cur_network_key_p) {  // Get-First operation
            cur_network_key_p =
                &cur_network_key;  // Switch to Get-Next operation
        }

        id = next_id;
        cur_network_key = next_network_key;

        if (vtss_appl_ospf_area_conf_get(id, cur_network_key_p, &area_id) != VTSS_RC_OK) {
            continue;
        }

        network_mask = vtss_ipv4_prefix_to_mask(cur_network_key_p->prefix_size);

        str_buf.clear();
        str_buf << AsIpv4(cur_network_key_p->address);
        VTSS_RC(vtss_icfg_printf(result, " network %s", str_buf.cstring()));

        network_mask = ~network_mask;
        str_buf.clear();
        str_buf << AsIpv4(network_mask);
        VTSS_RC(vtss_icfg_printf(result, " %s", str_buf.cstring()));

        str_buf.clear();
        str_buf << AsIpv4(area_id);
        VTSS_RC(vtss_icfg_printf(result, " area %s\n", str_buf.cstring()));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: area authentication */
static mesa_rc frr_ospf_router_conf_area_auth(const vtss_icfg_query_request_t *req,
                                              vtss_icfg_query_result_t *result)
{
    /* Command:
     * area { <ipv4_addr> | <0-4294967295> } authentication [message-digest] */

    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_id_t next_id;
    vtss_appl_ospf_area_id_t *cur_area_id_p = NULL, cur_area_id, next_area_id;
    vtss_appl_ospf_auth_type_t auth_type;
    StringStream str_buf;

    /* Iterate through all existing entries. */
    while (vtss_appl_ospf_area_auth_conf_itr(&id, &next_id, cur_area_id_p,
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

        if (vtss_appl_ospf_area_auth_conf_get(id, cur_area_id, &auth_type) !=
            VTSS_RC_OK) {
            continue;
        }

        str_buf.clear();
        str_buf << "area " << AsIpv4(cur_area_id) << " authentication";
        if (auth_type == VTSS_APPL_OSPF_AUTH_TYPE_MD5) {
            str_buf << " message-digest";
        }

        VTSS_RC(vtss_icfg_printf(result, " %s\n", str_buf.cstring()));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: area range */
static mesa_rc frr_ospf_router_conf_area_range(const vtss_icfg_query_request_t *req,
                                               vtss_icfg_query_result_t *result)
{
    /* Command: area { <ipv4_addr> | <0-4294967295> } range <ipv4_addr>
     * <ipv4_netmask> [advertise| not-advertise] [cost <0-16777215>] */

    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_id_t next_id;
    vtss_appl_ospf_area_id_t *cur_area_id_p = NULL, cur_area_id, next_area_id;
    mesa_ipv4_network_t *cur_network_p = NULL, cur_network, next_network;
    vtss_appl_ospf_area_range_conf_t conf, def_conf;
    mesa_ipv4_t network_mask;
    BufStream<SBuf32> str_buf;

    /* Iterate through all existing entries. */
    while (vtss_appl_ospf_area_range_conf_itr(&id, &next_id, cur_area_id_p,
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

        if (frr_ospf_area_range_conf_def(&id, &cur_area_id, &cur_network, &def_conf) != VTSS_RC_OK ||
            vtss_appl_ospf_area_range_conf_get(id, cur_area_id, cur_network, &conf)  != VTSS_RC_OK) {
            continue;
        }

        network_mask = vtss_ipv4_prefix_to_mask(cur_network.prefix_size);
        str_buf.clear();
        str_buf << AsIpv4(cur_area_id);
        VTSS_RC(vtss_icfg_printf(result, " area %s", str_buf.cstring()));

        str_buf.clear();
        str_buf << AsIpv4(cur_network.address) << " " << AsIpv4(network_mask);
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
static mesa_rc frr_ospf_router_conf_stub_area(const vtss_icfg_query_request_t *req,
                                              vtss_icfg_query_result_t *result)
{
    /* Command: area { <ipv4_addr> | <0-4294967295> } stub [ no-summary ] */

    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_id_t next_id;
    vtss_appl_ospf_area_id_t *cur_area_id_p = NULL, cur_area_id, next_area_id;
    vtss_appl_ospf_stub_area_conf_t conf, def_conf;
    StringStream str_buf;

    /* Iterate through all existing entries. */
    while (vtss_appl_ospf_stub_area_conf_itr(&id, &next_id, cur_area_id_p,
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

        if (frr_ospf_stub_area_conf_def(&id, &cur_area_id, &def_conf) != VTSS_RC_OK ||
            vtss_appl_ospf_stub_area_conf_get(id, cur_area_id, &conf) != VTSS_RC_OK) {
            continue;
        }

        str_buf.clear();

        /* There's no default configuation for stub area, so all configurations
         * are shown in running-config.
         */
        str_buf << " area " << AsIpv4(cur_area_id) << " "
                << (conf.is_nssa == true ? "nssa" : "stub");
        if (conf.no_summary) {
            str_buf << " no-summary";
        }

        if (conf.is_nssa &&
            conf.nssa_translator_role !=
            VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_CANDIDATE) {
            str_buf << "\n area " << AsIpv4(cur_area_id)
                    << " nssa translate type7 "
                    << (conf.nssa_translator_role ==
                        VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_NEVER
                        ? "never"
                        : "always");
        }

        VTSS_RC(vtss_icfg_printf(result, "%s\n", str_buf.cstring()));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: virtual link message digest key */
static mesa_rc frr_ospf_router_conf_vlink_md_key(
    const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result,
    vtss_appl_ospf_id_t id, vtss_appl_ospf_area_id_t area_id,
    vtss_appl_ospf_router_id_t router_id)
{
    /* Command:
     * area <area_id> virtual-link <ipv4_addr>
     *       message-digest-key <1-255> md5 { unencrypted <word1-16> |
     *                                        encrypted <word128> }
     */
    vtss_appl_ospf_id_t next_id;
    vtss_appl_ospf_area_id_t next_area_id;
    vtss_appl_ospf_router_id_t next_router_id;
    uint32_t *precedence_p = NULL, precedence, next_precedence;
    StringStream str_buf;

    /* Iterate through all existing entries. */
    while (vtss_appl_ospf_vlink_md_key_precedence_itr(
               &id, &next_id, &area_id, &next_area_id, &router_id,
               &next_router_id, precedence_p, &next_precedence) == VTSS_RC_OK) {
        if (next_id != id || area_id != next_area_id ||
            router_id != next_router_id) {
            break;  // Only process "precedence" here
        }

        // Switch to current data for next loop
        if (!precedence_p) {             // Get-First operation
            precedence_p = &precedence;  // Switch to Get-Next operation
        }

        id = next_id;
        area_id = next_area_id;
        router_id = next_router_id;
        precedence = next_precedence;

        vtss_appl_ospf_md_key_id_t md_key_id;
        mesa_rc rc = vtss_appl_ospf_vlink_md_key_precedence_get(
                         id, area_id, router_id, precedence, &md_key_id);
        if (rc != VTSS_RC_OK) {
            continue;
        }

        vtss_appl_ospf_auth_digest_key_t md_key;
        rc = vtss_appl_ospf_vlink_md_key_conf_get(id, area_id, router_id,
                                                  md_key_id, &md_key);
        if (rc != VTSS_RC_OK) {
            continue;
        }

        str_buf.clear();
        str_buf << " area " << AsIpv4(area_id) << " virtual-link "
                << AsIpv4(router_id) << " message-digest-key " << md_key_id
                << " md5 encrypted";
        VTSS_RC(vtss_icfg_printf(result, "%s %s\n", str_buf.cstring(),
                                 md_key.digest_key));
    }

    return VTSS_RC_OK;
}

/* Router configured mode: virtual link */
static mesa_rc frr_ospf_router_conf_vlink(const vtss_icfg_query_request_t *req,
                                          vtss_icfg_query_result_t *result)
{
    /* Command:
     * area <area_id> virtual-link <ipv4_addr> [hello-interval <1-65535>]
     *                                         [retransmit-interval <1-65535>]
     *                                         [dead-interval <1-65535>]
     * area <area_id> virtual-link <ipv4_addr>
     *      authentication [ <null|message-digest> ]
     * area <area_id> virtual-link <ipv4_addr>
     *      authentication-key { unencrypted <word1-8> | encrypted <word128> }
     * area <area_id> virtual-link <ipv4_addr>
     *       message-digest-key <1-255> md5 { unencrypted <word1-16> |
     *                                        encrypted <word128> }
     */

    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;
    vtss_appl_ospf_id_t next_id;
    vtss_appl_ospf_area_id_t *cur_area_id_p = NULL, cur_area_id, next_area_id;
    vtss_appl_ospf_router_id_t *cur_router_id_p = NULL, cur_router_id,
                                next_router_id;
    vtss_appl_ospf_vlink_conf_t conf, def_conf;
    StringStream str_buf;

    /* Iterate through all existing entries. */
    while (vtss_appl_ospf_vlink_itr(&id, &next_id, cur_area_id_p, &next_area_id,
                                    cur_router_id_p, &next_router_id) == VTSS_RC_OK) {
        if (next_id != id) {
            break;  // Only current instance ID is required here
        }

        // Switch to current data for next loop
        if (!cur_area_id_p) {              // Get-First operation
            cur_area_id_p = &cur_area_id;  // Switch to Get-Next operation
        }

        if (!cur_router_id_p) {                // Get-First operation
            cur_router_id_p = &cur_router_id;  // Switch to Get-Next operation
        }

        id = next_id;
        cur_area_id = next_area_id;
        cur_router_id = next_router_id;

        if (frr_ospf_vlink_conf_def(&id, &cur_area_id, &cur_router_id,
                                    &def_conf) != VTSS_RC_OK ||
            vtss_appl_ospf_vlink_conf_get(id, cur_area_id, cur_router_id,
                                          &conf) != VTSS_RC_OK) {
            continue;
        }

        str_buf.clear();
        str_buf << AsIpv4(cur_area_id);
        VTSS_RC(vtss_icfg_printf(result, " area %s", str_buf.cstring()));

        str_buf.clear();
        str_buf << AsIpv4(cur_router_id);
        VTSS_RC(vtss_icfg_printf(result, " virtual-link %s", str_buf.cstring()));

        // Hello interval
        if (req->all_defaults || def_conf.hello_interval != conf.hello_interval) {
            VTSS_RC(vtss_icfg_printf(result, " hello-interval %u",
                                     conf.hello_interval));
        }

        // Retransmit interval
        if (req->all_defaults ||
            def_conf.retransmit_interval != conf.retransmit_interval) {
            VTSS_RC(vtss_icfg_printf(result, " retransmit-interval %u",
                                     conf.retransmit_interval));
        }

        // Dead interval
        if (req->all_defaults || def_conf.dead_interval != conf.dead_interval) {
            VTSS_RC(vtss_icfg_printf(result, " dead-interval %u",
                                     conf.dead_interval));
        }

        // End of common parameters
        VTSS_RC(vtss_icfg_printf(result, "\n"));

        // Authentication type
        if (req->all_defaults || def_conf.auth_type != conf.auth_type) {
            str_buf.clear();
            if (conf.auth_type == VTSS_APPL_OSPF_AUTH_TYPE_AREA_CFG) {
                str_buf << " no";
            }

            str_buf << " area " << AsIpv4(cur_area_id) << " virtual-link "
                    << AsIpv4(cur_router_id) << " authentication";
            VTSS_RC(vtss_icfg_printf(result, "%s", str_buf.cstring()));

            if (conf.auth_type == VTSS_APPL_OSPF_AUTH_TYPE_MD5) {
                VTSS_RC(vtss_icfg_printf(result, " message-digest"));
            } else if (conf.auth_type == VTSS_APPL_OSPF_AUTH_TYPE_NULL) {
                VTSS_RC(vtss_icfg_printf(result, " null"));
            }

            VTSS_RC(vtss_icfg_printf(result, "\n"));
        }

        // Authentication simple password (always output the encrypted password)
        if (strlen(conf.simple_pwd)) {
            str_buf.clear();
            str_buf << "area " << AsIpv4(cur_area_id) << " virtual-link "
                    << AsIpv4(cur_router_id) << " authentication-key encrypted";
            VTSS_RC(vtss_icfg_printf(result, " %s %s\n", str_buf.cstring(),
                                     conf.simple_pwd));
        }

        // Authentication message digest key (always output the encrypted
        // password)
        (void)frr_ospf_router_conf_vlink_md_key(req, result, id, cur_area_id,
                                                cur_router_id);
    }

    return VTSS_RC_OK;
}

/* ICFG callback functions for OSPF router mode.
 * Notice that the OSPF configured router mode is a CLI submode.
 * We should prefix a space character before any command text output.
 * For example,
 * vtss_icfg_printf(result, " submode_cmd ...")
 */
static mesa_rc frr_ospf_router_conf_mode(const vtss_icfg_query_request_t *req,
                                         vtss_icfg_query_result_t *result)
{
    // TODO, require change
    // When multiple OSPF instances is supported, use variable
    // 'req->instance_id.generic_u32' instead of
    // FRR_OSPF_DEFAULT_INSTANCE_ID
    vtss_appl_ospf_id_t id = FRR_OSPF_DEFAULT_INSTANCE_ID;

    if (vtss_appl_ospf_get(id) != VTSS_RC_OK) {
        // Nothing to do if the OSPF instance ID is not existing
        return VTSS_RC_OK;
    }

    /* Important!!!
     * All commands under 'OSPF router configured mode' MUST below this line.
     *
     * The configured commands sequence MUST be considered.
     *
     * General rules:
     * 1. Specific router ID assignment MUST be applied first.
     * 2. General configuration should be applied before network area
     *    configuration since the OSPF process is starting after network
     *    area commands is applied.
     * 3. Default metric and route configuration should be applied after
     *    network area configuration since default routes should be advertised
     *    after normal routes are formed.
     *
     * Current sequence:
     * 1. router id  (MUST be first)
     * 2. stub router
     * 3. area authentication
     * 4. area range
     * 5. virtual link
     * 6. stub area
     * 7. route redistribution
     * 8. passive-interface
     * 9. network area
     * 10. default route redistribution
     * 11. default metric
     * 12. administrative distance
     */

    /* 1. Router configured mode: router ID */
    (void)frr_ospf_router_conf_router_id(req, result);

    /* 2. Router configured mode: stub router */
    (void)frr_ospf_router_conf_stub_router(req, result);

    /* 3. Router configured mode: area authentication */
    (void)frr_ospf_router_conf_area_auth(req, result);

    /* 4. Router configured mode: area range */
    (void)frr_ospf_router_conf_area_range(req, result);

    /* 5. Router configured mode: virtual link */
    (void)frr_ospf_router_conf_vlink(req, result);

    /* 6. Router configured mode: stub area */
    (void)frr_ospf_router_conf_stub_area(req, result);

    /* 7. Router configured mode: route redistribution */
    (void)frr_ospf_router_conf_router_redistribution(req, result);

    /* 8. Router configured mode: passive-interface */
    (void)frr_ospf_router_conf_passive_intf(req, result);

    /* 9. Router configured mode: network area */
    (void)frr_ospf_router_conf_network_area(req, result);

    /* 10. Router configured mode: default route redistribution */
    (void)frr_ospf_router_conf_def_route(req, result);

    /* 11. Router configured mode: default metric */
    (void)frr_ospf_router_conf_def_metric(req, result);

    /* 12. Router configured mode: administrative distance */
    (void)frr_ospf_router_conf_admin_distance(req, result);
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
mesa_rc frr_ospf_icfg_init(void)
{
    /* Register callback functions to ICFG module.
     *  The configuration divided into two groups for this module.
     *  1. Router mode configuration
     *  2. VLAN interface mode configuration
     */
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_OSPF_ROUTER_CONF,    "ospf", frr_ospf_router_conf_mode));
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_INTERFACE_VLAN_OSPF, "ospf", frr_ospf_vlan_intf_conf_mode));

    return VTSS_RC_OK;
}

