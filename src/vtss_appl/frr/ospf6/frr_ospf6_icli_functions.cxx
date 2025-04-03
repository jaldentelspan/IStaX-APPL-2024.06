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
#include "frr_ospf6_icli_functions.hxx"
#include "frr_ospf6_api.hxx"
#include "icli_api.h"
#include "icli_porting_util.h"
#include "ip_utils.hxx"  // For vtss_conv_ipv4mask_to_prefix()
#include "misc_api.h"    // For misc_ipv4_txt()
#include "vtss/appl/ospf6.h"
#include "vtss/basics/stream.hxx"  // For vtss::BufStream

#include <functional>  // For std::function

/****************************************************************************/
/** Module default trace group declaration                                  */
/****************************************************************************/
#define VTSS_TRACE_DEFAULT_GROUP FRR_TRACE_GRP_ICLI
#include "frr_trace.hxx"  // For module trace group definitions

/******************************************************************************/
/** Namespaces using-declaration                                              */
/******************************************************************************/
using namespace vtss;

/******************************************************************************/
/** Internal variables and APIs                                               */
/******************************************************************************/
/*
example:
Routing Process, with ID 61.0.0.86
 Initial SPF schedule delay 0 msecs
 Minimum hold time between two consecutive SPFs 50 msecs
 Maximum wait time between two consecutive SPFs 5000 msecs
 SPF algorithm last executed 1d 21:30:24 ago
 Minimum LSA interval 5 secs
 Minimum LSA arrival 1000 msecs
 Number of external LSA 0. Checksum Sum 0x00000000
 Number of areas in this router is 1
*/
static void FRR_ICLI_ospf6_router_print(u32 session_id, vtss_appl_ospf6_id_t id)
{
    vtss_appl_ospf6_router_status_t status;
    char str_buf[32];

    if (vtss_appl_ospf6_router_status_get(id, &status) == VTSS_RC_OK) {
        ICLI_PRINTF("Routing Process, with ID %s\n",
                    misc_ipv4_txt(status.ospf6_router_id, str_buf));
        ICLI_PRINTF(" Initial SPF schedule delay %d msecs\n", status.spf_delay);
        ICLI_PRINTF(
            " Minimum hold time between two consecutive SPFs %d msecs\n",
            status.spf_holdtime);
        ICLI_PRINTF(
            " Maximum wait time between two consecutive SPFs %d msecs\n",
            status.spf_max_waittime);
        if (status.last_executed_spf_ts) {
            ICLI_PRINTF(" SPF algorithm last executed %s ago\n",
                        misc_time_txt(status.last_executed_spf_ts / 1000));
        } else {
            ICLI_PRINTF(" SPF algorithm has not been run\n");
        }

        ICLI_PRINTF(" Number of areas in this router is %d\n",
                    status.attached_area_count);
    }
}

/* Output area range status/configuraiton.
 *
 * The 'Active(<route_metric>)' term is added when it is actively advertising
 * the area range. Otherwise, the 'Passive' term is added when the area range
 * doesn't take effect, e.g. the number of active interfaces in backbone area is
 * 0 or it is not in the routing table.
 *
 * Example:
 *       Area ranges are
 *           192.168.1.0/24 Passive Advertise
 *           192.168.2.0/24 Passive DoNotAdvertise
 *           192.168.3.0/24 Active(1) Advertise
 */
static void FRR_ICLI_ospf6_area_range_print(u32 session_id, vtss_appl_ospf6_id_t id,
                                            vtss_appl_ospf6_area_id_t area_id)
{
    vtss_appl_ospf6_id_t              cur_id = id, next_id;
    vtss_appl_ospf6_area_id_t         cur_area_id = area_id, next_area_id;
    mesa_ipv6_network_t              *cur_network_p = NULL, cur_network, next_network;
    vtss_appl_ospf6_area_range_conf_t area_range_conf;
    bool                             first_entry = true;
    vtss_appl_ip_route_status_map_t  ipv4_routes;
    bool                             got_ipv4_routes = false;
    StringStream                     str_buf;

    /* Iterate through all existing area range entries. */
    while (vtss_appl_ospf6_area_range_conf_itr(&cur_id, &next_id, &cur_area_id,
                                               &next_area_id, cur_network_p,
                                               &next_network) == VTSS_RC_OK) {
        if (next_id != id || next_area_id != area_id) {
            break;  // Only current instance ID and area ID are required here
        }

        // Switch to current data for next loop
        if (!cur_network_p) {              // Get-First operation
            cur_network_p = &cur_network;  // Switch to Get-Next operation
        }

        cur_id = next_id;
        cur_area_id = next_area_id;
        cur_network = next_network;

        if (vtss_appl_ospf6_area_range_conf_get(cur_id, cur_area_id, cur_network,
                                                &area_range_conf) != VTSS_RC_OK) {
            continue;
        }

        // Get backbone area status
        vtss_appl_ospf6_area_status_t backbone_area_status;
        vtss_clear(backbone_area_status);
        (void)vtss_appl_ospf6_area_status_get(id, FRR_OSPF6_BACKBONE_AREA_ID,
                                              &backbone_area_status);

        /* Iterate through all existing route entries.
         * Notice that the iteration is processed only when the backbone area
         * active interfaces count not equals 0 */
        bool                        is_active = false;
        vtss_appl_ip_route_status_t route_status = {};

        if (area_range_conf.is_advertised && backbone_area_status.attached_intf_total_count && !got_ipv4_routes) {
            if (vtss_appl_ip_route_status_get_all(ipv4_routes, VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC, VTSS_APPL_IP_ROUTE_PROTOCOL_OSPF) == VTSS_RC_OK) {
                got_ipv4_routes = true;
            }
        }

        if (area_range_conf.is_advertised &&
            backbone_area_status.attached_intf_total_count) {
            vtss_appl_ip_route_status_key_t route_ipv4_key = {};
            route_ipv4_key.route.route.ipv6_uc.network = cur_network;
            route_ipv4_key.protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_OSPF;

            for (auto itr = ipv4_routes.greater_than_or_equal(route_ipv4_key); itr != ipv4_routes.end(); ++itr) {
                is_active = vtss_ipv6_net_include(cur_network_p, &itr->first.route.route.ipv6_uc.network.address);

                if (is_active) {
                    route_status = itr->second;
                }
            }
        }

        str_buf.clear();
        if (first_entry) {
            first_entry = false;  // This line is printed once
            ICLI_PRINTF("        Area ranges are\n");
        }

        str_buf << "            " << cur_network << (is_active ? " Active" : " Passive");
        if (is_active) {
            str_buf << "(" << route_status.metric << ")";
        }

        str_buf << (area_range_conf.is_advertised ? " Advertise" : " DoNotAdvertise");
        ICLI_PRINTF("%s\n", str_buf.cstring());
    }
}

/*
example:
    Area BACKBONE(0.0.0.0)
        Number of interfaces in this area is 3
        Area has no authentication
        SPF algorithm executed 158 times
        Number of LSA 26

    Area ID: 0.0.0.1
        Number of interfaces in this area is 1
        It is a stub area
        Area has no authentication
        SPF algorithm executed 8 times
        Number of LSA 19
        Area ranges are
            192.168.1.0/24 Passive DoNotAdvertise
*/
static void FRR_ICLI_ospf6_router_area_print(u32 session_id,
                                             vtss_appl_ospf6_id_t id,
                                             vtss_appl_ospf6_area_id_t area_id)
{
    vtss_appl_ospf6_area_status_t status;
    char buf[32];

    if (vtss_appl_ospf6_area_status_get(id, area_id, &status) == VTSS_RC_OK) {
        if (status.is_backbone) {
            ICLI_PRINTF("    Area BACKBONE(%s)\n", misc_ipv4_txt(area_id, buf));
        } else {
            ICLI_PRINTF("    Area ID: %s\n", misc_ipv4_txt(area_id, buf));
        }

        ICLI_PRINTF("        Number of interfaces in this area is %d\n",
                    status.attached_intf_total_count);
        if (status.area_type == VTSS_APPL_OSPF6_AREA_STUB) {
            ICLI_PRINTF("        It is a stub area\n");
        } else if (status.area_type == VTSS_APPL_OSPF6_AREA_TOTALLY_STUB) {
            ICLI_PRINTF("        It is a totally stub area\n");
        }

        ICLI_PRINTF("        SPF algorithm executed %d times\n",
                    status.spf_executed_count);

        ICLI_PRINTF("        Number of LSA %d\n", status.lsa_count);

        FRR_ICLI_ospf6_area_range_print(session_id, id, area_id);
    }
}

/******************************************************************************/
/** Module ICLI APIs                                                          */
/******************************************************************************/
/* Set OSPF6 router ID */
mesa_rc FRR_ICLI_ospf6_router_id_set(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_ospf6_router_conf_t router_conf;

    if (vtss_appl_ospf6_router_conf_get(req.inst_id, &router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get OSPF6 router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    router_conf.router_id.is_specific_id = true;
    router_conf.router_id.id = req.router_id;
    rc = vtss_appl_ospf6_router_conf_set(req.inst_id, &router_conf);
    if (rc == VTSS_APPL_FRR_OSPF6_ERROR_INVALID_ROUTER_ID) {
        ICLI_PRINTF("%% The OSPF6 router ID is invalid.\n");
    } else if (rc == VTSS_APPL_FRR_OSPF6_ERROR_ROUTER_ID_CHANGE_NOT_TAKE_EFFECT) {
        ICLI_PRINTF("%% %s. (Command: 'clear ipv6 ospf process')\n", error_txt(rc));
    } else if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Set OSPF6 router ID failed.\n");
        return ICLI_RC_ERROR;
    }

    return ICLI_RC_OK;
}

/* Delete OSPF6 router ID */
mesa_rc FRR_ICLI_ospf6_router_id_del(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_ospf6_router_conf_t router_conf;

    if (vtss_appl_ospf6_router_conf_get(req.inst_id, &router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get OSPF6 router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    router_conf.router_id.is_specific_id = false;
    router_conf.router_id.id = 0;
    rc = vtss_appl_ospf6_router_conf_set(req.inst_id, &router_conf);
    if (rc == VTSS_APPL_FRR_OSPF6_ERROR_ROUTER_ID_CHANGE_NOT_TAKE_EFFECT) {
        ICLI_PRINTF("%% %s. (Command: 'clear ipv6 ospf process')\n", error_txt(rc));
    } else if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Set OSPF6 router ID failed.\n");
        return ICLI_RC_ERROR;
    }

    return ICLI_RC_OK;
}

/* Set OSPF6 interface-area */
mesa_rc FRR_ICLI_ospf6_interface_area_set(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_ifindex_t ifindex;
    vtss_appl_ospf6_router_intf_conf_t conf;
    StringStream str_buf;

    /* Iterate through all vlan IDs. */
    for (u32 idx = 0; idx < req.vlan_list->cnt; ++idx) {
        for (mesa_vid_t vid = req.vlan_list->range[idx].min;
             vid <= req.vlan_list->range[idx].max; ++vid) {
            if (vtss_ifindex_from_vlan(vid, &ifindex) != VTSS_RC_OK) {
                continue;
            }

            str_buf.clear();
            str_buf << ifindex;

            /* Get the original configuration */
            rc = vtss_appl_ospf6_router_intf_conf_get(req.inst_id, ifindex, &conf);
            if (rc == FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST) {
                ICLI_PRINTF(
                    "%% VLAN interface %s does not exist. Use command "
                    "'interface vlan <vid>' in global configuration mode "
                    "to "
                    "create an interface.\n",
                    str_buf.cstring());
                continue;
            } else if (rc != VTSS_RC_OK) {
                ICLI_PRINTF(
                    "%% Get OSPF6 router interface configuration "
                    "failed on %s.\n",
                    str_buf.cstring());
                continue;
            }

            /* Apply the new configuration */
            conf.area_id.id = req.area_id;
            conf.area_id.is_specific_id = req.configure_area_id;
            rc = vtss_appl_ospf6_router_intf_conf_set(req.inst_id, ifindex, &conf);
            if (rc != VTSS_RC_OK) {
                ICLI_PRINTF("%% Set OSPF6 area-id failed on %s.\n",
                            str_buf.cstring());
                continue;
            }
        }
    }

    return ICLI_RC_OK;
}

//------------------------------------------------------------------------------
//** OSPF6 instance process configuration
//------------------------------------------------------------------------------
/* Delete OSPF6 instance process */
mesa_rc FRR_ICLI_ospf6_instance_process_del(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id;

    if (vtss_appl_ospf6_get(req.inst_id) != VTSS_RC_OK) {
        // Quit silently when the OSPF6 process is already disabled.
        return ICLI_RC_OK;
    }

    vtss_appl_ospf6_router_conf_t rconf;
    mesa_rc rc = vtss_appl_ospf6_router_conf_get(req.inst_id, &rconf);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF(
            "%% Disable OSPF6 routing process failed. Cannot get router "
            "config\n");
        return ICLI_RC_ERROR;
    }

    rc = vtss_appl_ospf6_del(req.inst_id);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Disable OSPF6 routing process failed.\n");
        return ICLI_RC_ERROR;
    }

    return ICLI_RC_OK;
}

//------------------------------------------------------------------------------
//** OSPF6 administrative distance
//------------------------------------------------------------------------------
/* Set OSPF6 administrative distance */
mesa_rc FRR_ICLI_ospf6_admin_distance_set(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id;
    vtss_appl_ospf6_router_conf_t router_conf;

    /* Get the original configuration */
    if (vtss_appl_ospf6_router_conf_get(req.inst_id, &router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get OSPF6 router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    /* Update the new setting */
    router_conf.admin_distance = req.admin_distance;

    /* Apply the new configuration */
    mesa_rc rc = vtss_appl_ospf6_router_conf_set(req.inst_id, &router_conf);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Set OSPF6 administrative distance failed.\n");
        return rc;
    }

    return ICLI_RC_OK;
}

//------------------------------------------------------------------------------
//** OSPF6 route redistribution
//------------------------------------------------------------------------------
/* Set OSPF6 route redistribution */
mesa_rc FRR_ICLI_ospf6_redist_set(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id;
    vtss_appl_ospf6_router_conf_t router_conf;

    /* Get the original configuration */
    if (vtss_appl_ospf6_router_conf_get(req.inst_id, &router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get OSPF router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    /* Update the new setting */
    router_conf.redist_conf[req.redist_protocol].is_redist_enable = req.redist_enabled;

    /* Apply the new configuration */
    mesa_rc rc = vtss_appl_ospf6_router_conf_set(req.inst_id, &router_conf);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Set OSPF route redistribution failed.\n");
        return rc;
    }


    return ICLI_RC_OK;
}

//------------------------------------------------------------------------------
//** OSPF6 default route redistribution
//------------------------------------------------------------------------------
/* Set OSPF6 default route redistribution */
mesa_rc FRR_ICLI_ospf6_def_route_set(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id;
    vtss_appl_ospf6_router_conf_t router_conf;

    /* Get the original configuration */
    if (vtss_appl_ospf6_router_conf_get(req.inst_id, &router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get OSPF6 router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    /* Apply the new configuration */
    mesa_rc rc = vtss_appl_ospf6_router_conf_set(req.inst_id, &router_conf);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Set OSPF6 route redistribution failed.\n");
        return rc;
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 area range
//----------------------------------------------------------------------------
/* Set OSPF6 area range */
mesa_rc FRR_ICLI_ospf6_area_range_set(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_ospf6_area_range_conf_t conf;
    mesa_ipv6_network_t network = req.subnet;

    /* Check if it is a new entry or not */
    bool new_entry;
    rc = vtss_appl_ospf6_area_range_conf_get(req.inst_id, req.area_id, req.subnet,
                                             &conf);
    if (rc == VTSS_RC_OK) {  // already existing
        new_entry = false;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        ICLI_PRINTF("%% %s.\n", error_txt(rc));
        return rc;
    } else {  // New entry
        new_entry = true;

        // Get the default setting
        rc = frr_ospf6_area_range_conf_def(
                 (vtss_appl_ospf6_id_t *)&req.inst_id,
                 (vtss_appl_ospf6_area_id_t *)&req.area_id, &network, &conf);
        if (rc != VTSS_RC_OK) {
            ICLI_PRINTF("%% %s.\n", error_txt(rc));
        }
    }

    /* Update the new values */
    conf.is_advertised = req.area_range_advertise;
    if (req.has_cost) {
        conf.is_specific_cost = true;
        conf.cost = req.cost;
        conf.is_advertised = true;  // setting cost also sets advertise
    }

    if (!req.area_range_advertise) {
        conf.is_specific_cost =
            false;  // setting not-advertise also sets no-cost
    }

    /* Apply the new configuration */
    if (new_entry) {
        rc = vtss_appl_ospf6_area_range_conf_add(req.inst_id, req.area_id,
                                                 req.subnet, &conf);
    } else {
        rc = vtss_appl_ospf6_area_range_conf_set(req.inst_id, req.area_id,
                                                 req.subnet, &conf);
    }

    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% %s.\n", error_txt(rc));
    }

    return ICLI_RC_OK;
}

/* Delete/Restore OSPF6 area range */
mesa_rc FRR_ICLI_ospf6_area_range_del_or_restore(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    mesa_ipv6_network_t network = req.subnet;

    /* 'no cost' is to reset cost
       'no advertise/not-advertise' will delete area entry */

    /* Restore the default setting */
    if (req.has_cost) {
        vtss_appl_ospf6_area_range_conf_t conf, def_conf;

        // Get the original configuration
        rc = vtss_appl_ospf6_area_range_conf_get(req.inst_id, req.area_id,
                                                 req.subnet, &conf);
        if (rc != VTSS_RC_OK) {
            // For the deleting operation, quit silently when it does not exists
            return ICLI_RC_OK;
        }

        // Update the default configuration
        rc = frr_ospf6_area_range_conf_def(
                 (vtss_appl_ospf6_id_t *)&req.inst_id,
                 (vtss_appl_ospf6_area_id_t *)&req.area_id, &network, &def_conf);
        conf.is_specific_cost = def_conf.is_specific_cost;
        conf.cost = def_conf.cost;

        /* Update the entry */
        rc = vtss_appl_ospf6_area_range_conf_set(req.inst_id, req.area_id,
                                                 req.subnet, &conf);
    } else {
        /* Delete the entry */
        rc = vtss_appl_ospf6_area_range_conf_del(req.inst_id, req.area_id,
                                                 req.subnet);
    }

    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% %s.\n", error_txt(rc));
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 virtual link
//----------------------------------------------------------------------------
/* Set OSPF6 virtual link */
mesa_rc FRR_ICLI_ospf6_area_vlink_set(const FrrOspf6CliReq &req)
{
    return ICLI_RC_OK;
}
//----------------------------------------------------------------------------
//** OSPF6 stub area
//----------------------------------------------------------------------------
/* Set the OSPF6 stub area */
mesa_rc FRR_ICLI_ospf6_area_stub_set(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_ospf6_stub_area_conf_t conf;

    /* Check if it is a new entry or not */
    bool new_entry;
    rc = vtss_appl_ospf6_stub_area_conf_get(req.inst_id, req.area_id, &conf);
    if (rc == VTSS_RC_OK) {  // already existing
        new_entry = false;
    } else if (rc != FRR_RC_ENTRY_NOT_FOUND) {
        ICLI_PRINTF("%% %s.\n", error_txt(rc));
        return rc;
    } else {  // New entry
        new_entry = true;

        // Get the default setting
        rc = frr_ospf6_stub_area_conf_def(
                 (vtss_appl_ospf6_id_t *)&req.inst_id,
                 (vtss_appl_ospf6_area_id_t *)&req.area_id, &conf);
        if (rc != VTSS_RC_OK) {
            ICLI_PRINTF("%% %s.\n", error_txt(rc));
        }
    }

    // no-summary
    if (req.has_no_summary) {
        conf.no_summary = req.has_no_form ? false : true;
    }

    /* Apply the new configuration */
    if (new_entry) {
        rc = vtss_appl_ospf6_stub_area_conf_add(req.inst_id, req.area_id, &conf);
    } else {
        rc = vtss_appl_ospf6_stub_area_conf_set(req.inst_id, req.area_id, &conf);
    }

    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% %s.\n", error_txt(rc));
    }

    return ICLI_RC_OK;
}

/* Delete the OSPF6 stub area or set totally stub area as a stub area */
mesa_rc FRR_ICLI_ospf6_area_stub_no(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;

    vtss_appl_ospf6_stub_area_conf_t conf;
    rc = vtss_appl_ospf6_stub_area_conf_get(req.inst_id, req.area_id, &conf);
    if (rc == FRR_RC_ENTRY_NOT_FOUND) {
        return VTSS_RC_OK;
    }

    if (req.has_no_summary) {
        if (rc == VTSS_RC_OK) {
            conf.no_summary = false;
            rc = vtss_appl_ospf6_stub_area_conf_set(req.inst_id, req.area_id,
                                                    &conf);
        }
    } else {
        rc = vtss_appl_ospf6_stub_area_conf_del(req.inst_id, req.area_id);
    }

    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% %s.\n", error_txt(rc));
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 interface parameter tuning
//----------------------------------------------------------------------------
/* Set OSPF6 vlan interface parameters */
mesa_rc FRR_ICLI_ospf6_vlan_interface_set(const FrrOspf6CliReq &req)
{
    mesa_rc rc;
    vtss_ifindex_t ifindex;
    vtss_appl_ospf6_intf_conf_t conf;
    u32 session_id = req.session_id; /* used for ICLI_PRINTF */

    // Iterate through all vlan IDs.
    for (u32 idx = 0; idx < req.vlan_list->cnt; ++idx) {
        for (mesa_vid_t vid = req.vlan_list->range[idx].min;
             vid <= req.vlan_list->range[idx].max; ++vid) {
            if (vtss_ifindex_from_vlan(vid, &ifindex) != VTSS_RC_OK ||
                vtss_appl_ospf6_intf_conf_get(ifindex, &conf) != VTSS_RC_OK) {
                continue;
            }

            /* Update new parameters */
            // priority
            if (req.has_priority) {
                conf.priority = req.priority;
            }

            // cost
            if (req.has_cost) {
                conf.is_specific_cost = true;
                conf.cost = req.cost;
            }

            // MTU Ignore
            if (req.has_mtu_ignore) {
                conf.mtu_ignore = req.mtu_ignore;
            }

            // hello interval
            if (req.has_hello_interval) {
                conf.hello_interval = req.hello_interval;
            }

            // retransmit interval
            if (req.has_retransmit_interval) {
                conf.retransmit_interval = req.retransmit_interval;
            }

            // dead interval
            if (req.has_dead_interval) {
                // if you set 'dead-interval' means 'fast hello' is disabled
                conf.dead_interval = req.dead_interval;
            }

            // passive-interface
            if (req.has_passive) {
                conf.is_passive = true;
            }

            // Transmit Delay
            if (req.has_transmit_delay) {
                conf.transmit_delay = req.transmit_delay;
            }

            /* Apply the new configuration */
            if ((rc = vtss_appl_ospf6_intf_conf_set(ifindex, &conf)) != VTSS_RC_OK) {
                ICLI_PRINTF("Set OSPF6 interface configuration failed");
                return rc;
            }
        }
    }

    return ICLI_RC_OK;
}

/* Reset OSPF6 vlan interface parameters to default */
mesa_rc FRR_ICLI_ospf6_vlan_interface_set_default(const FrrOspf6CliReq &req)
{
    mesa_rc rc;
    vtss_ifindex_t ifindex;
    vtss_appl_ospf6_intf_conf_t conf, def_conf;

    // Iterate through all vlan IDs.
    for (u32 idx = 0; idx < req.vlan_list->cnt; ++idx) {
        for (mesa_vid_t vid = req.vlan_list->range[idx].min;
             vid <= req.vlan_list->range[idx].max; ++vid) {
            if (vtss_ifindex_from_vlan(vid, &ifindex) != VTSS_RC_OK ||
                vtss_appl_ospf6_intf_conf_get(ifindex, &conf) != VTSS_RC_OK ||
                frr_ospf6_intf_conf_def(&ifindex, &def_conf) != VTSS_RC_OK) {
                continue;
            }

            /* Reset parameters */
            // priority
            if (req.has_priority) {
                conf.priority = def_conf.priority;
            }

            // cost
            if (req.has_cost) {
                conf.is_specific_cost = false;
                conf.cost = def_conf.cost;
            }

            // MTU ignore
            if (req.has_mtu_ignore) {
                conf.mtu_ignore = false;
            }

            // hello interval
            if (req.has_hello_interval) {
                conf.hello_interval = def_conf.hello_interval;
            }

            // retransmit interval
            if (req.has_retransmit_interval) {
                conf.retransmit_interval = def_conf.retransmit_interval;
            }

            // dead interval and fast hello packets
            if (req.has_dead_interval) {
                conf.dead_interval = def_conf.dead_interval;
            }

            // passive interface
            if (req.has_passive) {
                conf.is_passive = false;
            }

            // transmit delay
            if (req.has_transmit_delay) {
                conf.transmit_delay = def_conf.transmit_delay;
            }

            /* Apply the new configuration */
            if ((rc = vtss_appl_ospf6_intf_conf_set(ifindex, &conf)) != VTSS_RC_OK) {
                return rc;
            }
        }
    }

    return ICLI_RC_OK;
}

/* Show OSPF6 information */
mesa_rc FRR_ICLI_ospf6_show_info(const FrrOspf6CliReq &req)
{
    vtss_appl_ospf6_id_t key_1, *ptr_1 = NULL;
    mesa_ipv4_t key_2, *ptr_2 = NULL;
    u32 session_id = req.session_id; /* used for ICLI_PRINTF */

    while (vtss_appl_ospf6_inst_itr(ptr_1, &key_1) == VTSS_RC_OK) {
        if (!ptr_1) {
            ptr_1 = &key_1;
        }
        /* print router status */
        FRR_ICLI_ospf6_router_print(req.session_id, key_1);

        auto current_id = key_1;
        while (vtss_appl_ospf6_area_status_itr(ptr_1, &key_1, ptr_2, &key_2) ==
               VTSS_RC_OK &&
               key_1 == current_id) {
            if (!ptr_2) {
                ptr_2 = &key_2;
            } else {
                /* newline to separeate from previous area */
                ICLI_PRINTF("\n");
            }

            /* print router status */
            FRR_ICLI_ospf6_router_area_print(req.session_id, key_1, key_2);
        }
    }

    return VTSS_RC_OK;
}

/* Display single entry of OSPF6 interface status */
static mesa_rc FRR_ICLI_ospf6_interface_print(u32 session_id,
                                              vtss_ifindex_t ifindex,
                                              bool *add_new_line)
{
    vtss_appl_ospf6_interface_status_t entry;
    char buf[128];

    mesa_rc rc = vtss_appl_ospf6_interface_status_get(ifindex, &entry);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    if (*add_new_line) {
        ICLI_PRINTF("\n");
    } else {
        *add_new_line = true;
    }

    // Syntax : Vlan60 is up
    ICLI_PRINTF("%s is %s\n", vtss_ifindex2str(buf, sizeof(buf), ifindex),
                entry.status ? "up" : "down");

    // Syntax : Internet Address 60.0.0.89/8, Area 0
    ICLI_PRINTF("  Internet Address %s/%d",
                misc_ipv6_txt(&entry.network.address, buf),
                entry.network.prefix_size);
    ICLI_PRINTF(", Area %s\n", misc_ipv4_txt(entry.area_id, buf));

    // Syntax : Router ID 1.2.3.5, Cost: 1
    ICLI_PRINTF("  Router ID %s, Cost: %d\n",
                misc_ipv4_txt(entry.router_id, buf), entry.cost);

    // Syntax : Transmit Delay is 1 sec, State BDR, Priority 1
    ICLI_PRINTF("  Transmit Delay is %d sec", entry.transmit_delay);
    ICLI_PRINTF(", State ");
    if (entry.status) {
        switch (entry.state) {
        case VTSS_APPL_OSPF6_INTERFACE_DR_OTHER:
            ICLI_PRINTF("DROther");
            break;
        case VTSS_APPL_OSPF6_INTERFACE_BDR:
            ICLI_PRINTF("BDR");
            break;
        case VTSS_APPL_OSPF6_INTERFACE_DR:
            ICLI_PRINTF("DR");
            break;
        case VTSS_APPL_OSPF6_INTERFACE_POINT2POINT:
            ICLI_PRINTF("POINT_TO_POINT");
            break;
        case VTSS_APPL_OSPF6_INTERFACE_DOWN:
            ICLI_PRINTF("DOWN");
            break;
        case VTSS_APPL_OSPF6_INTERFACE_WAITING:
            ICLI_PRINTF("WAITING");
            break;
        default:
            ICLI_PRINTF("Unknown");
            break;
        }
    } else {
        ICLI_PRINTF("DOWN");
    }

    ICLI_PRINTF(", Priority %d\n", entry.priority);

    // Syntax : Designated Router (ID) 61.0.0.86
    if (entry.dr_id == 0) {
        ICLI_PRINTF("  No designated router on this network\n");
    } else {
        ICLI_PRINTF("  Designated Router (ID) %s\n",
                    misc_ipv4_txt(entry.dr_id, buf));
    }

    // Syntax : Backup Designated router (ID) 1.2.3.5
    if (entry.bdr_id == 0) {
        ICLI_PRINTF("  No backup designated router on this network\n");
    } else {
        ICLI_PRINTF("  Backup Designated router (ID) %s\n",
                    misc_ipv4_txt(entry.bdr_id, buf));
    }

    // Syntax : Timer intervals configured, Hello 10s, Dead 40s,
    // Retransmit 5
    ICLI_PRINTF(
        "  Timer intervals configured, Hello %d, Dead %d, Retransmit %d\n",
        entry.hello_time, entry.dead_time, entry.retransmit_time);

    // Syntax :   Hello due in 8.685s
    if (entry.is_passive) {
        ICLI_PRINTF("    No Hellos (Passive interface)\n");
    }
    return ICLI_RC_OK;
}

/* Display all entries of OSPF6 interface status */
static mesa_rc FRR_ICLI_ospf6_interface_print_all(u32 session_id)
{
    vtss_ifindex_t ifindex;
    vtss_appl_ospf6_interface_status_t entry;
    char buf[128];
    vtss::Map<vtss_ifindex_t, vtss_appl_ospf6_interface_status_t> interface;
    int cnt = 0;

    mesa_rc rc = vtss_appl_ospf6_interface_status_get_all(interface);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    cnt = 0;
    vtss::Map<vtss_ifindex_t, vtss_appl_ospf6_interface_status_t>::iterator itr;
    for (itr = interface.begin(); itr != interface.end(); ++itr) {
        ifindex = itr->first;
        entry = itr->second;

        if (cnt != 0) {
            ICLI_PRINTF("\n");
        }

        cnt++;

        // Syntax : Vlan60 is up
        ICLI_PRINTF("%s is %s\n", vtss_ifindex2str(buf, sizeof(buf), ifindex),
                    entry.status ? "up" : "down");

        // Syntax : Internet Address 60.0.0.89/8, Area 0
        ICLI_PRINTF("  Internet Address %s/%d",
                    misc_ipv6_txt(&entry.network.address, buf),
                    entry.network.prefix_size);
        ICLI_PRINTF(", Area %s\n", misc_ipv4_txt(entry.area_id, buf));

        // Syntax : Router ID 1.2.3.5, Cost: 1
        ICLI_PRINTF("  Router ID %s, Cost: %d\n",
                    misc_ipv4_txt(entry.router_id, buf), entry.cost);

        // Syntax : Transmit Delay is 1 sec, State BDR, Priority 1
        ICLI_PRINTF("  Transmit Delay is %d sec", entry.transmit_delay);
        ICLI_PRINTF(", State ");
        if (entry.status) {
            switch (entry.state) {
            case VTSS_APPL_OSPF6_INTERFACE_DR_OTHER:
                ICLI_PRINTF("DROther");
                break;
            case VTSS_APPL_OSPF6_INTERFACE_BDR:
                ICLI_PRINTF("BDR");
                break;
            case VTSS_APPL_OSPF6_INTERFACE_DR:
                ICLI_PRINTF("DR");
                break;
            case VTSS_APPL_OSPF6_INTERFACE_POINT2POINT:
                ICLI_PRINTF("POINT_TO_POINT");
                break;
            case VTSS_APPL_OSPF6_INTERFACE_DOWN:
                ICLI_PRINTF("DOWN");
                break;
            case VTSS_APPL_OSPF6_INTERFACE_WAITING:
                ICLI_PRINTF("WAITING");
                break;
            default:
                ICLI_PRINTF("Unknown");
                break;
            }
        } else {
            ICLI_PRINTF("DOWN");
        }

        ICLI_PRINTF(", Priority %d\n", entry.priority);

        // Syntax : Designated Router (ID) 61.0.0.86, Interface address
        // 60.0.0.86
        if (entry.dr_id == 0) {
            ICLI_PRINTF("  No designated router on this network\n");
        } else {
            ICLI_PRINTF("  Designated Router (ID) %s\n",
                        misc_ipv4_txt(entry.dr_id, buf));
        }

        // Syntax : Backup Designated router (ID) 1.2.3.5, Interface address
        // 60.0.0.89
        if (entry.bdr_id == 0) {
            ICLI_PRINTF("  No backup designated router on this network\n");
        } else {
            ICLI_PRINTF("  Backup Designated router (ID) %s\n",
                        misc_ipv4_txt(entry.bdr_id, buf));
        }

        // Syntax : Timer intervals configured, Hello 10s, Dead 40s,
        // Retransmit 5
        ICLI_PRINTF(
            "  Timer intervals configured, Hello %d, Dead %d, Retransmit %d\n",
            entry.hello_time, entry.dead_time, entry.retransmit_time);

    }

    return ICLI_RC_OK;
}

/* Show OSPF6 interface information */
mesa_rc FRR_ICLI_ospf6_show_interface(const FrrOspf6CliReq &req)
{
    mesa_rc rc;
    u32 session_id = req.session_id;
    vtss_ifindex_t ifindex;
    bool first_valid_entry = false;

    /* Is it 'show all interfaces' command? */
    if (!req.vlan_list) {
        (void)FRR_ICLI_ospf6_interface_print_all(req.session_id);
        return ICLI_RC_OK;
    }

    /* Otherwise, show the specific interfaces */
    // Iterate through all vlan IDs.
    for (u32 idx = 0; req.vlan_list && idx < req.vlan_list->cnt; ++idx) {
        for (mesa_vid_t vid = req.vlan_list->range[idx].min;
             vid <= req.vlan_list->range[idx].max; ++vid) {
            if (vtss_ifindex_from_vlan(vid, &ifindex) != VTSS_RC_OK) {
                continue;
            }

            rc = FRR_ICLI_ospf6_interface_print(req.session_id, ifindex,
                                                &first_valid_entry);
            if (rc == FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST) {
                ICLI_PRINTF(" %% VLAN interface %d does not exist.\n", vid);
            } else if (rc == FRR_RC_ENTRY_NOT_FOUND) {
                ICLI_PRINTF(" %% OSPF6 not enabled on VLAN interface %d\n", vid);
            }
        }
    }

    // Iterate through all virtual link IDs.
    for (u32 idx = 0; req.vlink_list && idx < req.vlink_list->cnt; ++idx) {
        for (u32 vlink_id = req.vlink_list->range[idx].min;
             vlink_id <= req.vlink_list->range[idx].max; ++vlink_id) {
            if (vtss_ifindex_from_frr_vlink(vlink_id, &ifindex) != VTSS_RC_OK) {
                ICLI_PRINTF(" %% VINK interface %d does not exist.\n", vlink_id);
                continue;
            }

            rc = FRR_ICLI_ospf6_interface_print(req.session_id, ifindex,
                                                &first_valid_entry);
            if (rc != VTSS_RC_OK) {
                // Unlike the OSPF6 VLAN interface, there are no such case:
                // 'OSPF6 not enabled on VLINK interface'.
                ICLI_PRINTF(" %% VLINK interface %d does not exist.\n", vlink_id);
            }
        }
    }

    return ICLI_RC_OK;
}

/* Show OSPF6 neighbor information */
mesa_rc FRR_ICLI_ospf6_show_neighbor(const FrrOspf6CliReq &req)
{
    vtss_appl_ospf6_neighbor_status_t entry;
    char buf[128];
    u32 session_id = req.session_id; /* used for ICLI_PRINTF */
    StringStream str_buf;
    bool first_entry = true;
    vtss::Vector<vtss_appl_ospf6_neighbor_data_t> neighbors;

    // return silently
    if (vtss_appl_ospf6_neighbor_status_get_all(neighbors) != VTSS_RC_OK) {
        return VTSS_RC_OK;
    }

    for (const auto &itr_status : neighbors) {
        entry = itr_status.status;

        if (!req.has_detail) {
            if (first_entry) {
                ICLI_PRINTF("%-15s  %-3s  %-18s  %-10s  %-39s  %s",
                            "Neighbor ID", "Pri", "State", "Dead Time",
                            "Address", "Interface\n");
                first_entry = false;
            }

            ICLI_PRINTF("%-15s  %-3d", misc_ipv4_txt(entry.neighbor_id, buf),
                        entry.priority);

            str_buf.clear();
            switch (entry.state) {
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_DEPENDUPON:
                str_buf << "DEPENDUPON";
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_DELETED:
                str_buf << "DELETED";
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_DOWN:
                str_buf << "DOWN";
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_ATTEMPT:
                str_buf << "ATTEMPT";
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_INIT:
                str_buf << "INIT";
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_2WAY:
                str_buf << "2WAY";
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_EXSTART:
                str_buf << "EXSTART";
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_EXCHANGE:
                str_buf << "EXCHANGE";
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_LOADING:
                str_buf << "LOADING";
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_FULL:
                str_buf << "FULL";
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_UNKNOWN:
                str_buf << "UNKNOWN";
                break;
            }

            if (entry.neighbor_id == entry.dr_id) {
                str_buf << "/DR";
            } else if (entry.neighbor_id == entry.bdr_id) {
                str_buf << "/BDR";
            } else {
                str_buf << "/DROTHER";
            }

            ICLI_PRINTF("  %-18s", str_buf.cstring());

            str_buf.clear();
            str_buf << (entry.dead_time / 1000) << "."
                    << (entry.dead_time % 1000) << "sec";

            ICLI_PRINTF("  %-10s", str_buf.cstring());

            str_buf.clear();
            str_buf << entry.ifindex;
            ICLI_PRINTF("  %-39s  %s\n", misc_ipv6_txt(&entry.ip_addr, buf),
                        str_buf.cstring());
        } else {
            // Syntax : Neighbor 61.0.0.86, interface address 60.0.0.86
            ICLI_PRINTF("Neighbor %s", misc_ipv4_txt(entry.neighbor_id, buf));
            ICLI_PRINTF(", interface address %s\n",
                        misc_ipv6_txt(&entry.ip_addr, buf));

            // Syntax :     In the area 0.0.0.0 via interface Vlan60
            str_buf.clear();
            str_buf << "    In the area " << AsIpv4(entry.area_id);

            str_buf << " via interface " << entry.ifindex;
            ICLI_PRINTF("%s\n", str_buf.cstring());

            // Syntax :     Neighbor priority is 1, State is FULL
            switch (entry.state) {
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_DEPENDUPON:
                ICLI_PRINTF(
                    "    Neighbor priority is %d, State is DEPENDUPON\n",
                    entry.priority);
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_DELETED:
                ICLI_PRINTF("    Neighbor priority is %d, State is DELETED\n",
                            entry.priority);
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_DOWN:
                ICLI_PRINTF("    Neighbor priority is %d, State is DOWN\n",
                            entry.priority);
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_ATTEMPT:
                ICLI_PRINTF("    Neighbor priority is %d, State is ATTEMPT\n",
                            entry.priority);
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_INIT:
                ICLI_PRINTF("    Neighbor priority is %d, State is INIT\n",
                            entry.priority);
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_2WAY:
                ICLI_PRINTF("    Neighbor priority is %d, State is 2WAY\n",
                            entry.priority);
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_EXSTART:
                ICLI_PRINTF("    Neighbor priority is %d, State is EXSTART\n",
                            entry.priority);
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_EXCHANGE:
                ICLI_PRINTF("    Neighbor priority is %d, State is EXCHANGE\n",
                            entry.priority);
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_LOADING:
                ICLI_PRINTF("    Neighbor priority is %d, State is LOADING\n",
                            entry.priority);
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_FULL:
                ICLI_PRINTF("    Neighbor priority is %d, State is FULL\n",
                            entry.priority);
                break;
            case VTSS_APPL_OSPF6_NEIGHBOR_STATE_UNKNOWN:
                ICLI_PRINTF("    Neighbor priority is %d, State is UNKNOWN\n",
                            entry.priority);
                break;
            }

            // Syntax :     DR ID is 60.0.0.86
            ICLI_PRINTF("    DR ID is %s", misc_ipv4_txt(entry.dr_id, buf));

            // Syntax :     BDR ID is 60.0.0.89
            ICLI_PRINTF("    BDR ID is %s", misc_ipv4_txt(entry.bdr_id, buf));

            // Syntax :     Dead timer due in 35 sec
            ICLI_PRINTF("\n    Dead timer due in %d.%03d sec\n",
                        entry.dead_time / 1000, entry.dead_time % 1000);
            ICLI_PRINTF("\n");
        }
    }

    return VTSS_RC_OK;
}

static void FRR_ICLI_ospf6_route_ipv4_table_header_print(
    const u32 session_id, const vtss_appl_ospf6_route_type_t type)
{
    switch (type) {
    case VTSS_APPL_OSPF6_ROUTE_TYPE_INTRA_AREA:
        ICLI_PRINTF("\n    Intra-area Route List\n\n");
        break;
    case VTSS_APPL_OSPF6_ROUTE_TYPE_INTER_AREA:
        ICLI_PRINTF("\n    Inter-area Route List\n\n");
        break;
    case VTSS_APPL_OSPF6_ROUTE_TYPE_BORDER_ROUTER:
        ICLI_PRINTF("\n    Router Path List\n\n");
        break;
    case VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_1:
    case VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_2:
        ICLI_PRINTF("\n    External Route List\n\n");
        break;
    case VTSS_APPL_OSPF6_ROUTE_TYPE_UNKNOWN:
        ICLI_PRINTF("\n    Unknown\n\n");
        break;
    }
}

/*
example:

            OSPF6 Router with ID (0.0.0.3)

    Codes: i - Intra-area Router Path, I - Inter-area Router Path

    Intra-area Route List

    1.0.1.0/24, Intra, cost 1, area 0.0.0.0, Connected
      via 1.0.1.3, VLAN 100
    1.0.4.0/24, Intra, cost 11, area 0.0.0.0
      via 1.0.3.4, VLAN 200
    1.2.6.0/24, Inter, cost 11, area 0.0.0.0
      via 1.0.1.2, VLAN 100
    1.3.12.0/24, Intra, cost 1, area 0.0.0.3, Connected
      via 1.3.12.3, VLAN 300

    Inter-area Route List

    1.1.7.0/24, Inter, cost 31, area 0.0.0.0
      via 1.0.3.4, VLAN 200
      via 1.0.1.2, VLAN 100
    1.5.16.0/24, Inter, cost 21, area 0.0.0.0
      via 1.0.3.4, VLAN 200

    Router Path List

i 0.0.0.1 [11] via 1.0.1.2, VLAN 100, ABR, Area 0.0.0.0
I 0.0.0.15 [31] via 1.0.3.4, VLAN 200, ASBR, Area 0.0.0.0
I 0.0.0.15 [31] via 1.0.1.2, VLAN 100, ASBR, Area 0.0.0.0
i 0.0.0.31 [1] via 1.3.12.1, VLAN 300, ASBR, Area 0.0.0.3

    External Route List

    1.99.1.0/24, Ext2, cost 20, fwd cost 31, tag 0
      via 1.0.1.2, VLAN 100
      via 1.0.3.4, VLAN 200
    192.100.1.0/24, Ext1, cost 51, fwd cost 31, tag 0
      via 1.3.12.1, VLAN 300
*/
static void FRR_ICLI_ospf6_route_ipv4_entry_print(
    const u32 session_id, const vtss_appl_ospf6_route_ipv6_data_t *const entry,
    bool is_multi_nexthop)
{
    StringStream dest_str;
    StringStream nexthop_str;  // The common output for all route type.
    mesa_ipv6_t nexthop = entry->nexthop;
    mesa_ipv4_t area = entry->area;

    // format: via 1.3.12.1, VLAN 300
    nexthop_str << "via " << (nexthop) << ", " << entry->status.ifindex;
    switch (entry->rt_type) {
    case VTSS_APPL_OSPF6_ROUTE_TYPE_INTRA_AREA:
    case VTSS_APPL_OSPF6_ROUTE_TYPE_INTER_AREA:
        // format: 1.0.1.0/24, Intra, cost 1, area 0.0.0.0, Connected
        dest_str << entry->dest << ", "
                 << (entry->rt_type == VTSS_APPL_OSPF6_ROUTE_TYPE_INTRA_AREA
                     ? "Intra"
                     : "Inter")
                 << ", cost " << entry->status.cost << ", area " << AsIpv4(area);
        if (entry->status.connected) {
            dest_str << ", "
                     << "Connected";

            // 'via 0.0.0.0, ' is substituted by whitespace for the direct
            // connected entry.
            std::string adjust_str("via 0.0.0.0, ");
            nexthop_str.clear();
            nexthop_str << WhiteSpace(adjust_str.size()) << entry->status.ifindex;
        }

        break;

    case VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_1:
    case VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_2:
        // format: 1.99.1.0/24, Ext2, cost 20, fwd cost 31
        dest_str << entry->dest << ", "
                 << (entry->rt_type == VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_1
                     ? "Ext1"
                     : "Ext2")
                 << ", cost " << entry->status.cost;
        if (entry->rt_type == VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_2) {
            dest_str << ", fwd cost " << entry->status.as_cost;
        }

        break;

    case VTSS_APPL_OSPF6_ROUTE_TYPE_BORDER_ROUTER:
        // format:  i 0.0.0.15 [31] via 1.0.3.4, Vlan200, ASBR, Area 0.0.0.0
        if (entry->status.border_router_type ==
            VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_INTER_AREA_ASBR) {
            dest_str << "I ";
        } else {
            dest_str << "i ";
        }

        {
            mesa_ipv6_t router_id = entry->dest.address;
            dest_str << (router_id) << " [" << entry->status.cost << "] ";
            dest_str << nexthop_str;
        }

        if (entry->status.border_router_type ==
            VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_ABR) {
            dest_str << ", ABR";
        } else if (entry->status.border_router_type ==
                   VTSS_APPL_OSPF6_ROUTE_BORDER_ROUTER_TYPE_ABR_ASBR) {
            dest_str << ", ABR/ASBR";
        } else {
            dest_str << ", ASBR";
        }

        dest_str << ", Area " << AsIpv4(area);
        ICLI_PRINTF("%s\n", dest_str.cstring());
        return;

    case VTSS_APPL_OSPF6_ROUTE_TYPE_UNKNOWN:
        ICLI_PRINTF("\n    Unknown\n\n");
        break;
    }

    /* 'is_multi_nexthop' is 'true' indicates the
     * 'dest_str' is the same as previous one, so
     * it isn't shown here.
    */
    if (!is_multi_nexthop) {
        ICLI_PRINTF("    %s\n", dest_str.cstring());
    }

    ICLI_PRINTF("      %s\n", nexthop_str.cstring());
}

mesa_rc FRR_ICLI_ospf6_show_route(const FrrOspf6CliReq &req)
{
    vtss_appl_ospf6_route_status_t entry;
    StringStream str_buf;
    u32 session_id = req.session_id; /* used for ICLI_PRINTF */
    vtss::Vector<vtss_appl_ospf6_route_ipv6_data_t> routes;
    vtss_appl_ospf6_route_ipv6_data_t prev_entry = {};
    bool is_multi_nb = false;

    /* Initail 'prev_entry' with an impossible entry */
    prev_entry.rt_type = VTSS_APPL_OSPF6_ROUTE_TYPE_UNKNOWN;
    prev_entry.dest.address = {0}; //TODO Is this an impossible entry?
    prev_entry.dest.prefix_size = 128;
    prev_entry.id = 0;

    /* Return silently if error or no entries. */
    if (vtss_appl_ospf6_route_ipv6_status_get_all(routes) != VTSS_RC_OK ||
        !routes.size()) {
        return VTSS_RC_OK;
    }

    for (const auto &itr_status : routes) {
        entry = itr_status.status;

        /* get the router ID and print the header */
        if (prev_entry.id != itr_status.id) {
            vtss_appl_ospf6_router_status_t status;
            vtss_appl_ospf6_router_status_get(itr_status.id, &status);
            str_buf.clear();
            str_buf << AsIpv4(status.ospf6_router_id);
            ICLI_PRINTF("\n            OSPF6 Router with ID (%s)\n\n",
                        str_buf.cstring());
            ICLI_PRINTF(
                "    Codes: i - Intra-area Router Path, I - Inter-area "
                "Router Path\n");
        }

        /* print the header,  */
        if (prev_entry.rt_type != itr_status.rt_type &&
            // External Type-1 and Type-2 are in the same table.
            prev_entry.rt_type != VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_1 &&
            prev_entry.rt_type != VTSS_APPL_OSPF6_ROUTE_TYPE_EXTERNAL_TYPE_2) {
            FRR_ICLI_ospf6_route_ipv4_table_header_print(session_id,
                                                         itr_status.rt_type);
        }

        /* For the route list type (intra-area, inter-area, and external route)
           only the first destination is shown for multiple nexthop.
           exmaple:

                Inter-area Route List

                1.1.7.0/24, Inter, cost 31, area 0.0.0.0
                  via 1.0.3.4, Vlan200
                  via 1.0.1.2, Vlan100

                External Route List

                1.99.1.0/24, Ext2, cost 20, fwd cost 31, tag 0
                  via 1.0.1.2, Vlan100
                  via 1.0.3.4, Vlan200
            But it's unnecessary for router path list
            example:
                I 0.0.0.15 [31] via 1.0.3.4, Vlan200, ASBR, Area 0.0.0.0
                I 0.0.0.15 [31] via 1.0.1.2, Vlan100, ASBR, Area 0.0.0.0
        */
        /* the router type wont' be checked because it's impossible to
           have a multiple routes with differnt path type.
        */
        if (itr_status.rt_type != VTSS_APPL_OSPF6_ROUTE_TYPE_BORDER_ROUTER &&
            prev_entry.dest == itr_status.dest &&
            prev_entry.status.cost == itr_status.status.cost) {
            is_multi_nb = true;
        }

        FRR_ICLI_ospf6_route_ipv4_entry_print(session_id, &itr_status, is_multi_nb);

        /* Update 'prev_entry' and 'is_multi_nb' */
        prev_entry = itr_status;
        is_multi_nb = false;
    }

    return VTSS_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 database information
//----------------------------------------------------------------------------
/* Mapping xxx to vtss_appl_ospf6_route_type_t */
static const char *frr_ospf6_lsdb_to_type_mapping(
    const vtss_appl_ospf6_lsdb_type_t lsdb_type)
{
    switch (lsdb_type) {
    case VTSS_APPL_OSPF6_LSDB_TYPE_ROUTER:
        return "router-LSA";

    case VTSS_APPL_OSPF6_LSDB_TYPE_NETWORK:
        return "network-LSA";

    case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX:
        return "inter-area-prefix-LSA";

    case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_ROUTER:
        return "inter-area-router-LSA";

    case VTSS_APPL_OSPF6_LSDB_TYPE_EXTERNAL:
        return "AS-external-LSA";

    case VTSS_APPL_OSPF6_LSDB_TYPE_LINK:
        return "Link-LSA";

    case VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX:
        return "intra-area-prefix-LSA";

    case VTSS_APPL_OSPF6_LSDB_TYPE_NSSA_EXTERNAL:
        return "NSSA-external-LSA";

    case VTSS_APPL_OSPF6_LSDB_TYPE_NONE:
    default:
        return "Unknown";
    }
}

/* Mapping xxx to string */
static const char *frr_ospf6_link_connected_type_mapping(const int type)
{
    switch (type) {
    case 1:
        return "another Router (point-to-point)";

    case 2:
        return "a Transit Network";

    case 3:
        return "Stub Network";

    case 4:
        return "a Virtual Link";

    default:
        return "Unknown";
    }
}

/* Show OSPF6 database general summary information */
mesa_rc FRR_ICLI_ospf6_show_db_general_info(const FrrOspf6CliReq &req)
{
    u32 session_id = req.session_id; /* used for ICLI_PRINTF */
    StringStream str_buf;
    char ip_str_buf[128];
    vtss_appl_ospf6_router_status_t ospf6_status;
    vtss::Vector<vtss_appl_ospf6_db_entry_t> db_entries;

    // Since some table header only need to display once.
    // So we record the previous variable values here.
    vtss_appl_ospf6_id_t prev_inst_id = 0;
    vtss_appl_ospf6_area_id_t prev_area_id = 0;
    vtss_appl_ospf6_lsdb_type_t prev_lsdb_type = VTSS_APPL_OSPF6_LSDB_TYPE_NONE;

    /* Get OSPF6 database information. */
    if (vtss_appl_ospf6_router_status_get(req.inst_id, &ospf6_status) != VTSS_RC_OK ||
        vtss_appl_ospf6_db_get_all(db_entries) != VTSS_RC_OK || !db_entries.size()) {
        // Quit silently when the get operation failed(OSPF6 is diabled)
        // or no entrt existing.
        return ICLI_RC_OK;
    }

    /* Output single database entry information */
    for (const auto &itr : db_entries) {
        /* Output myself router ID */
        if (prev_inst_id != itr.inst_id) {
            prev_inst_id = itr.inst_id;
            ICLI_PRINTF("\n            ");
            ICLI_PRINTF("OSPF6 Router with ID (%s)\n",
                        misc_ipv4_txt(ospf6_status.ospf6_router_id, ip_str_buf));
        }

        // Filter condition
        if (req.has_adv_router_id && itr.adv_router_id != req.adv_router_id) {
            // Unmatched advertising router ID
            continue;
        }

        if (req.has_self_originate && itr.adv_router_id != ospf6_status.ospf6_router_id) {
            // Not self originated
            continue;
        }

        /* Output link state type header (if needed) */
        auto &db = itr.db;

        if (prev_area_id != itr.area_id || prev_lsdb_type != itr.lsdb_type) {
            prev_lsdb_type = itr.lsdb_type;
            prev_area_id = itr.area_id;

            str_buf.clear();
            str_buf << "                ";
            switch (itr.lsdb_type) {
            case VTSS_APPL_OSPF6_LSDB_TYPE_ROUTER:
                str_buf << "Router";
                break;
            case VTSS_APPL_OSPF6_LSDB_TYPE_NETWORK:
                str_buf << "Net";
                break;
            case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX:
                str_buf << "Inter Area Prefix";
                break;
            case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_ROUTER:
                str_buf << "Inter Area Router";
                break;
            case VTSS_APPL_OSPF6_LSDB_TYPE_EXTERNAL:
                str_buf << "AS External";
                break;
            case VTSS_APPL_OSPF6_LSDB_TYPE_LINK:
                str_buf << "Link";
                break;
            case VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX:
                str_buf << "Intra Area Prefix";
                break;
            case VTSS_APPL_OSPF6_LSDB_TYPE_NONE:
            default:
                continue;  // Continue loop iterator
            }

            if (itr.lsdb_type == VTSS_APPL_OSPF6_LSDB_TYPE_EXTERNAL) {
                ICLI_PRINTF("\n%s Link States\n\n", str_buf.cstring());
            } else {
                ICLI_PRINTF("\n%s Link States (Area %s)\n\n", str_buf.cstring(),
                            misc_ipv4_txt(itr.area_id, ip_str_buf));
            }

            ICLI_PRINTF("Link ID         ADV Router      Age         Seq#");
            ICLI_PRINTF("\n");
        }

        /* Output general summary database information */
        ICLI_PRINTF("%-16s", misc_ipv4_txt(itr.link_state_id, ip_str_buf));
        ICLI_PRINTF("%-16s", misc_ipv4_txt(itr.adv_router_id, ip_str_buf));
        ICLI_PRINTF("%-12d", db.age);
        ICLI_PRINTF("0x%-9x", db.sequence);
        ICLI_PRINTF("\n");
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** OSPF6 database detail information
//----------------------------------------------------------------------------
/* Show OSPF6 database detail header */
template <typename TYPE>
mesa_rc FRR_ICLI_ospf6_show_db_detail_common_part_header(const FrrOspf6CliReq &req,
                                                         TYPE &entry)
{
    StringStream str_buf;
    char ip_str_buf[128];
    u32 session_id = req.session_id; /* used for ICLI_PRINTF */

    str_buf.clear();
    str_buf << "                ";
    switch (entry.lsdb_type) {
    case VTSS_APPL_OSPF6_LSDB_TYPE_ROUTER:
        str_buf << "Router";
        break;
    case VTSS_APPL_OSPF6_LSDB_TYPE_NETWORK:
        str_buf << "Net";
        break;
    case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX:
        str_buf << "Inter Area Prefix";
        break;
    case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_ROUTER:
        str_buf << "Inter Area Router";
        break;
    case VTSS_APPL_OSPF6_LSDB_TYPE_EXTERNAL:
        str_buf << "AS External";
        break;
    case VTSS_APPL_OSPF6_LSDB_TYPE_LINK:
        str_buf << "Link";
        break;
    case VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX:
        str_buf << "Intra Area Prefix";
        break;
    case VTSS_APPL_OSPF6_LSDB_TYPE_NONE:
    default:
        break;
    }

    if (entry.lsdb_type == VTSS_APPL_OSPF6_LSDB_TYPE_EXTERNAL) {
        ICLI_PRINTF("\n%s Link States\n", str_buf.cstring());
        ICLI_PRINTF("\n");
    } else {
        ICLI_PRINTF("\n%s Link States (Area %s)\n", str_buf.cstring(),
                    misc_ipv4_txt(entry.area_id, ip_str_buf));
        ICLI_PRINTF("\n");
    }
    return ICLI_RC_OK;
}

/* Show OSPF6 database detail common information */
template <typename TYPE>
mesa_rc FRR_ICLI_ospf6_show_db_detail_common_part_info(const FrrOspf6CliReq &req,
                                                       TYPE &entry)
{
    u32 session_id = req.session_id; /* used for ICLI_PRINTF */
    char ip_str_buf[128];

    ICLI_PRINTF("  LS age: %d\n", entry.data.age);

    // OSPF6 Option Bits
    // +---+---+---+---+---+---+---+---+
    // |DN |O  |DC |EA |NP |MC |E  |MT |
    // +---+---+---+---+---+---+---+---+
    // Syntax :     Options 18 *|-|-|EA|-|-|E|-
    if (entry.lsdb_type != VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX) {
        ICLI_PRINTF(
            "  Options: 0x%x %s|%s|%s|%s|%s|%s \n", entry.data.options,
            (entry.data.options & VTSS_APPL_OSPF6_OPTION_FIELD_DC) ? "DC" : "-",
            (entry.data.options & VTSS_APPL_OSPF6_OPTION_FIELD_R)  ? "R"  : "-",
            (entry.data.options & VTSS_APPL_OSPF6_OPTION_FIELD_N)  ? "N"  : "-",
            (entry.data.options & VTSS_APPL_OSPF6_OPTION_FIELD_MC)  ? "MC"  : "--",
            (entry.data.options & VTSS_APPL_OSPF6_OPTION_FIELD_E) ? "E" : "-",
            (entry.data.options & VTSS_APPL_OSPF6_OPTION_FIELD_V6) ? "V6" : "--");
    }
    ICLI_PRINTF("  LS Type: %s\n",
                frr_ospf6_lsdb_to_type_mapping(entry.lsdb_type));
    ICLI_PRINTF("  Link State ID: %-16s\n",
                misc_ipv4_txt(entry.link_state_id, ip_str_buf));
    ICLI_PRINTF("  Advertising Router: %-16s\n",
                misc_ipv4_txt(entry.adv_router_id, ip_str_buf));
    ICLI_PRINTF("  LS Seq Number: 0x%x\n", entry.data.sequence);
    ICLI_PRINTF("  Checksum: 0x%x\n", entry.data.checksum);
    ICLI_PRINTF("  Length: %d\n", entry.data.length);

    ICLI_PRINTF("\n");

    return ICLI_RC_OK;
}

template <typename TYPE>
void FRR_ICLI_ospf6_show_db_detail_specific_info(u32 session_id, TYPE const &,
                                                 char *);
template <>
void FRR_ICLI_ospf6_show_db_detail_specific_info(
    u32 session_id, vtss_appl_ospf6_db_detail_link_entry_t const &entry,
    char *ip_str_buf)
{
    int i;
    vtss_appl_ospf6_db_link_info_t data;

    // display router part information
    ICLI_PRINTF("   Number of Links: %d\n", entry.data.prefix_cnt);
    ICLI_PRINTF("\n");

    for (i = 0; i < entry.data.prefix_cnt; i++) {
        if (vtss_appl_ospf6_db_detail_link_entry_get_by_index(
                entry.inst_id, entry.area_id, entry.lsdb_type,
                entry.link_state_id, entry.adv_router_id, i,
                &data) != VTSS_RC_OK) {
            continue;
        }

        ICLI_PRINTF("    Prefix Address : %s\n",
                    misc_ipv6_txt(&data.link_prefix, ip_str_buf));
        ICLI_PRINTF("    Prefix Length: %d\n", data.link_prefix_len);
        ICLI_PRINTF("    Prefix Options: %d\n", data.prefix_options);
        ICLI_PRINTF("\n");
    }
}

template <>
void FRR_ICLI_ospf6_show_db_detail_specific_info(
    u32 session_id, vtss_appl_ospf6_db_detail_intra_area_prefix_entry_t const &entry,
    char *ip_str_buf)
{
    int i;
    vtss_appl_ospf6_db_link_info_t data;

    // display router part information
    ICLI_PRINTF("   Number of Prefixes: %d\n", entry.data.prefix_cnt);
    ICLI_PRINTF("\n");

    for (i = 0; i < entry.data.prefix_cnt; i++) {
        if (vtss_appl_ospf6_db_detail_intra_area_prefix_entry_get_by_index(
                entry.inst_id, entry.area_id, entry.lsdb_type,
                entry.link_state_id, entry.adv_router_id, i,
                &data) != VTSS_RC_OK) {
            continue;
        }

        ICLI_PRINTF("    Prefix Address : %s\n",
                    misc_ipv6_txt(&data.link_prefix, ip_str_buf));
        ICLI_PRINTF("    Prefix Length: %d\n", data.link_prefix_len);
        ICLI_PRINTF("    Prefix Options: %d\n", data.prefix_options);
        ICLI_PRINTF("\n");
    }
}

template <>
void FRR_ICLI_ospf6_show_db_detail_specific_info(
    u32 session_id, vtss_appl_ospf6_db_detail_router_entry_t const &entry,
    char *ip_str_buf)
{
    int i;
    vtss_appl_ospf6_db_router_link_info_t data;

    // display router part information
    ICLI_PRINTF("   Number of Links: %d\n", entry.data.router_link_count);
    ICLI_PRINTF("\n");

    for (i = 0; i < entry.data.router_link_count; i++) {
        if (vtss_appl_ospf6_db_detail_router_entry_get_by_index(
                entry.inst_id, entry.area_id, entry.lsdb_type,
                entry.link_state_id, entry.adv_router_id, i,
                &data) != VTSS_RC_OK) {
            continue;
        }

        ICLI_PRINTF("    Link connected to: %s\n",
                    frr_ospf6_link_connected_type_mapping(data.link_connected_to));
        ICLI_PRINTF("     (Link ID) Net: %s\n",
                    misc_ipv4_txt(data.link_id, ip_str_buf));
        ICLI_PRINTF("     (Link Data) : %s\n",
                    misc_ipv4_txt(data.link_data, ip_str_buf));
        ICLI_PRINTF("      metrics: %d\n", data.metric);
        ICLI_PRINTF("\n");
    }
}

template <>
void FRR_ICLI_ospf6_show_db_detail_specific_info(
    u32 session_id, vtss_appl_ospf6_db_detail_network_entry_t const &entry,
    char *)
{
    ICLI_PRINTF("\n");
}

template <>
void FRR_ICLI_ospf6_show_db_detail_specific_info(
    u32 session_id, vtss_appl_ospf6_db_detail_inter_area_router_entry_t const &entry,
    char *ip_str_buf)
{
    // display summary part information
    ICLI_PRINTF("        Destination id: %s\n", misc_ipv4_txt(entry.data.destination_router_id, ip_str_buf));
    ICLI_PRINTF("        Metric: %d\n", entry.data.metric);
    ICLI_PRINTF("\n");
}

template <>
void FRR_ICLI_ospf6_show_db_detail_specific_info(
    u32 session_id, vtss_appl_ospf6_db_detail_inter_area_prefix_entry_t const &entry,
    char *ip_str_buf)
{
    // display Prefix part information
    ICLI_PRINTF("        Prefix %s/%d\n", misc_ipv6_txt(&entry.data.prefix.address, ip_str_buf), entry.data.prefix.prefix_size);
    ICLI_PRINTF("        Metric: %d\n", entry.data.metric);
    ICLI_PRINTF("\n");
}

template <>
void FRR_ICLI_ospf6_show_db_detail_specific_info(
    u32 session_id, vtss_appl_ospf6_db_detail_external_entry_t const &entry,
    char *ip_str_buf)
{
    // display external part information
    ICLI_PRINTF("        Prefix: %s/%d\n", misc_ipv6_txt(&entry.data.prefix.address, ip_str_buf), entry.data.prefix.prefix_size);
    ICLI_PRINTF("        Metric: %d\n", entry.data.metric);
    ICLI_PRINTF("        Forward Address: %s\n",
                misc_ipv6_txt(&entry.data.forward_address, ip_str_buf));
    ICLI_PRINTF("\n");
}

template <typename TYPE>
using GetAllFunction = std::function<mesa_rc(vtss::Vector<TYPE> &)>;

/* Get OSPF6 database details */
template <typename TYPE>
mesa_rc FRR_ICLI_ospf6_show_db_detail_common_info(
    const FrrOspf6CliReq &req, GetAllFunction<TYPE> ospf6_db_detail_get_all)
{
    u32 session_id = req.session_id; /* used for ICLI_PRINTF */
    char ip_str_buf[128];
    vtss::Vector<TYPE> db_entries;

    // Since some table header only need to display once.
    // So we record the previous variable values here.
    vtss_appl_ospf6_id_t prev_inst_id = 0;
    vtss_appl_ospf6_area_id_t prev_area_id = 0;
    vtss_appl_ospf6_lsdb_type_t prev_lsdb_type = VTSS_APPL_OSPF6_LSDB_TYPE_NONE;

    vtss_appl_ospf6_router_status_t ospf6_status;

    /* Get OSPF6 database information. */
    if (vtss_appl_ospf6_router_status_get(req.inst_id, &ospf6_status) != VTSS_RC_OK) {
        // Quit silently when the get operation failed(OSPF6 is diabled)
        return ICLI_RC_OK;
    }

    if (ospf6_db_detail_get_all(db_entries) != VTSS_RC_OK || !db_entries.size()) {
        // Quit silently when no entry existing.
        return ICLI_RC_OK;
    }

    // print common information for router entry
    for (const auto &itr : db_entries) {
        /* Output myself router ID */
        if (prev_inst_id != itr.inst_id) {
            prev_inst_id = itr.inst_id;
            ICLI_PRINTF("\n            ");
            ICLI_PRINTF("OSPF6 Router with ID (%s)\n",
                        misc_ipv4_txt(ospf6_status.ospf6_router_id, ip_str_buf));
        }

        // Filter condition
        if (req.has_adv_router_id && itr.adv_router_id != req.adv_router_id) {
            // Unmatched advertising router ID
            continue;
        }

        if (req.has_self_originate &&
            itr.adv_router_id != ospf6_status.ospf6_router_id) {
            // Not self originated
            continue;
        }

        if (prev_area_id != itr.area_id || prev_lsdb_type != itr.lsdb_type) {
            prev_area_id = itr.area_id;
            prev_lsdb_type = itr.lsdb_type;

            // display common part header
            FRR_ICLI_ospf6_show_db_detail_common_part_header(req, itr);
        }

        // display common part information
        FRR_ICLI_ospf6_show_db_detail_common_part_info(req, itr);

        // display specific part information
        FRR_ICLI_ospf6_show_db_detail_specific_info(session_id, itr, ip_str_buf);
    }

    ICLI_PRINTF("\n");

    return ICLI_RC_OK;
}

/* Show OSPF6 database detail intra prefix information */
mesa_rc FRR_ICLI_ospf6_show_db_detail_intra_prefix_info(const FrrOspf6CliReq &req)
{
    return FRR_ICLI_ospf6_show_db_detail_common_info<vtss_appl_ospf6_db_detail_intra_area_prefix_entry_t>(
               req, vtss_appl_ospf6_db_detail_intra_area_prefix_get_all);
}

/* Show OSPF6 database detail link information */
mesa_rc FRR_ICLI_ospf6_show_db_detail_link_info(const FrrOspf6CliReq &req)
{
    return FRR_ICLI_ospf6_show_db_detail_common_info<vtss_appl_ospf6_db_detail_link_entry_t>(
               req, vtss_appl_ospf6_db_detail_link_get_all);
}

/* Show OSPF6 database detail router information */
mesa_rc FRR_ICLI_ospf6_show_db_detail_router_info(const FrrOspf6CliReq &req)
{
    return FRR_ICLI_ospf6_show_db_detail_common_info<vtss_appl_ospf6_db_detail_router_entry_t>(
               req, vtss_appl_ospf6_db_detail_router_get_all);
}

/* Show OSPF6 database detail network information */
mesa_rc FRR_ICLI_ospf6_show_db_detail_network_info(const FrrOspf6CliReq &req)
{
    return FRR_ICLI_ospf6_show_db_detail_common_info <
           vtss_appl_ospf6_db_detail_network_entry_t > (
               req, vtss_appl_ospf6_db_detail_network_get_all);
}

/* Show OSPF6 database detail summary information */
mesa_rc FRR_ICLI_ospf6_show_db_detail_summary_info(const FrrOspf6CliReq &req)
{
    return FRR_ICLI_ospf6_show_db_detail_common_info <
           vtss_appl_ospf6_db_detail_inter_area_prefix_entry_t > (
               req, vtss_appl_ospf6_db_detail_inter_area_prefix_get_all);
}

/* Show OSPF6 database detail asbr summary information */
mesa_rc FRR_ICLI_ospf6_show_db_detail_inter_area_router_info(const FrrOspf6CliReq &req)
{
    return FRR_ICLI_ospf6_show_db_detail_common_info <
           vtss_appl_ospf6_db_detail_inter_area_router_entry_t > (
               req, vtss_appl_ospf6_db_detail_inter_area_router_get_all);
}

/* Show OSPF6 database detail external information */
mesa_rc FRR_ICLI_ospf6_show_db_detail_external_info(const FrrOspf6CliReq &req)
{
    return FRR_ICLI_ospf6_show_db_detail_common_info <
           vtss_appl_ospf6_db_detail_external_entry_t > (
               req, vtss_appl_ospf6_db_detail_external_get_all);
}

/* Show OSPF6 database detail nssa external information */
mesa_rc FRR_ICLI_ospf6_show_db_detail_nssa_external_info(const FrrOspf6CliReq &req)
{
    return VTSS_RC_OK;
}

/* Show OSPF6 database detail information */
mesa_rc FRR_ICLI_ospf6_show_db_detail_info(const FrrOspf6CliReq &req)
{
    // Get all entries from OSPF6 detail database
    switch (req.lsdb_type) {
    case VTSS_APPL_OSPF6_LSDB_TYPE_LINK:
        FRR_ICLI_ospf6_show_db_detail_link_info(req);
        return ICLI_RC_OK;
    case VTSS_APPL_OSPF6_LSDB_TYPE_INTRA_AREA_PREFIX:
        FRR_ICLI_ospf6_show_db_detail_intra_prefix_info(req);
        return ICLI_RC_OK;
    case VTSS_APPL_OSPF6_LSDB_TYPE_ROUTER:
        FRR_ICLI_ospf6_show_db_detail_router_info(req);
        return ICLI_RC_OK;
    case VTSS_APPL_OSPF6_LSDB_TYPE_NETWORK:
        FRR_ICLI_ospf6_show_db_detail_network_info(req);
        return ICLI_RC_OK;
    case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_PREFIX:
        FRR_ICLI_ospf6_show_db_detail_summary_info(req);
        return ICLI_RC_OK;
    case VTSS_APPL_OSPF6_LSDB_TYPE_INTER_AREA_ROUTER:
        FRR_ICLI_ospf6_show_db_detail_inter_area_router_info(req);
        return ICLI_RC_OK;
    case VTSS_APPL_OSPF6_LSDB_TYPE_EXTERNAL:
        FRR_ICLI_ospf6_show_db_detail_external_info(req);
        return ICLI_RC_OK;
    case VTSS_APPL_OSPF6_LSDB_TYPE_NONE:
    default:
        // Quit silently when the type is none.
        return ICLI_RC_OK;
    }

    return ICLI_RC_OK;
}

