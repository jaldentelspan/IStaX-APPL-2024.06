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
#include "frr_rip_icli_functions.hxx"
#include "frr_rip_api.hxx"
#include "icli_api.h"
#include "icli_porting_util.h"
#include "ip_utils.hxx"  // For vtss_conv_ipv4mask_to_prefix()
#include "misc_api.h"    // For misc_ipv4_txt()
#include <vtss/appl/rip.h>
#include <vtss/basics/stream.hxx>  // For vtss::BufStream

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
static mesa_rc FRR_ICLI_rip_ipv4_wildcard_mask_to_prefix(
    const mesa_ipv4_t wildcard_mask, u32 *const prefix)
{
    /*  The IPv4 wildcard-mask contains 32 bits where 0 is a match and 1 is a
     * "do not care" bit e.g. 0.0.0.255 indicates a match in the first byte of
     * the network number.
     *
     * Although this CLI input argument allows setting 1 for any bits, but we
     * will map it's value to a network mask length. (It is required in the
     * OSPF public header. So there is limitation for the input format as
     * following.
     *
     * The 'don't care' (value 1) bits must be sequential and
     * the 'match' (value 0) bits MUST always be to the left.
     * For example, 0.0.0.255 means that the network mask length is 8.
     *
     * A simple way that we use to convernt the "wildcard_mask" value to a
     *   network mask length is below.
     * a. Convert the input value with bitwise NOT algorithm (~).
     * b. Check the converted value with the same rule as IPv4 mask.
     */
    return vtss_conv_ipv4mask_to_prefix(~wildcard_mask, prefix);
}

uint32_t FRR_ICLI_ipv4_addr_to_prefix(mesa_ipv4_t ip)
{
    /* Find the classful subnet mask for the given IPv4 address. */
    if ((ip & 0x80000000) == 0x00000000) {
        // class A  0.xx.xx.xx ~ 127.xx.xx.xx
        return 8;
    } else if ((ip & 0xC0000000) == 0x80000000) {
        // class B  128.xx.xx.xx ~ 191.xx.xx.xx
        return 16;
    } else if ((ip & 0xE0000000) == 0xC0000000) {
        // class C  192.xx.xx.xx ~ 223.xx.xx.xx
        return 24;
    }

    return 0;
}

/******************************************************************************/
/** Module ICLI APIs                                                          */
/******************************************************************************/

//----------------------------------------------------------------------------
//** RIP router configuration
//----------------------------------------------------------------------------

/* Set RIP global version */
mesa_rc FRR_ICLI_rip_global_ver_set(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_rip_router_conf_t router_conf;

    /* Get the current configuration */
    if (vtss_appl_rip_router_conf_get(&router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get RIP router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    /* Apply the new configuration */
    router_conf.version = req.global_ver;
    rc = vtss_appl_rip_router_conf_set(&router_conf);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Set RIP version configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    return ICLI_RC_OK;
}

/* Set RIP default metric */
mesa_rc FRR_ICLI_rip_redist_def_metric_set(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_rip_router_conf_t router_conf;

    /* Get the current configuration */
    if (vtss_appl_rip_router_conf_get(&router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get RIP router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    /* Apply the new configuration */
    router_conf.redist_def_metric = req.metric;
    rc = vtss_appl_rip_router_conf_set(&router_conf);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Set RIP default metric configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    return ICLI_RC_OK;
}

/* Set RIP redistribution metric */
mesa_rc FRR_ICLI_rip_redist_set(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    vtss_appl_rip_router_conf_t router_conf;

    /* Get the original configuration */
    if (vtss_appl_rip_router_conf_get(&router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get RIP router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    /* Update the new setting */
    router_conf.redist_conf[req.redist_proto_type].is_enabled = !req.has_no_form;
    router_conf.redist_conf[req.redist_proto_type].is_specific_metric =
        req.has_no_form ? false : req.has_metric;
    router_conf.redist_conf[req.redist_proto_type].metric =
        (!req.has_no_form && req.has_metric) ? req.metric : 0;

    /* Apply the new configuration */
    mesa_rc rc = vtss_appl_rip_router_conf_set(&router_conf);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Set RIP redistribution failed.\n");
        return rc;
    }

    return ICLI_RC_OK;
}

/* Set RIP all interfaces as passive-interface by default */
mesa_rc FRR_ICLI_rip_passive_intf_default(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_rip_router_conf_t router_conf;

    /* Get the original configuration */
    if (vtss_appl_rip_router_conf_get(&router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get RIP router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    /* Apply the new configuration */
    router_conf.def_passive_intf = req.passive_intf_enabled;
    rc = vtss_appl_rip_router_conf_set(&router_conf);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Set RIP passive-interface by default failed.\n");
        return ICLI_RC_ERROR;
    }

    return ICLI_RC_OK;
}

/* Set RIP passive-interface default mode */
mesa_rc FRR_ICLI_rip_passive_intf_set(const FrrRipCliReq &req)
{
    vtss_ifindex_t ifindex;
    StringStream str_buf;
    vtss_appl_rip_router_intf_conf_t conf;
    mesa_rc rc;
    u32 session_id = req.session_id;

    /* Iterate through all vlan IDs. */
    for (u32 idx = 0; idx < req.vlan_list->cnt; ++idx) {
        for (mesa_vid_t vid = req.vlan_list->range[idx].min;
             vid <= req.vlan_list->range[idx].max; ++vid) {
            if (vtss_ifindex_from_vlan(vid, &ifindex) != VTSS_RC_OK) {
                continue;
            }

            str_buf.clear();
            str_buf << ifindex;
            if ((rc = vtss_appl_rip_router_intf_conf_get(ifindex, &conf)) ==
                FRR_RC_VLAN_INTERFACE_DOES_NOT_EXIST) {
                ICLI_PRINTF(
                    "%% VLAN interface %s does not exist. Use command "
                    "'interface vlan <vid>' in global configuration mode "
                    "to "
                    "create an interface.\n",
                    str_buf.cstring());
                continue;
            } else if (rc != VTSS_RC_OK) {
                ICLI_PRINTF(
                    "%% Get current RIP passive-interface mode failed on "
                    "%s.\n",
                    str_buf.cstring());
                continue;
            }

            if (conf.passive_enabled != req.passive_intf_enabled) {
                conf.passive_enabled = req.passive_intf_enabled;
                if (vtss_appl_rip_router_intf_conf_set(ifindex, &conf) !=
                    VTSS_RC_OK) {
                    ICLI_PRINTF(
                        "%% Set RIP passive-interface mode failed on %s.\n",
                        str_buf.cstring());
                }

                continue;
            }
        }
    }

    return ICLI_RC_OK;
}

/* Set RIP default route redistribution */
mesa_rc FRR_ICLI_rip_def_route_redist_set(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_rip_router_conf_t router_conf;

    /* Get the current configuration */
    if (vtss_appl_rip_router_conf_get(&router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get RIP router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    /* Apply the new configuration */
    router_conf.def_route_redist = req.def_route_redist;
    rc = vtss_appl_rip_router_conf_set(&router_conf);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF(
            "%% Set RIP administrative distance configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    return ICLI_RC_OK;
}

/* Set RIP administrative distance */
mesa_rc FRR_ICLI_rip_admin_distance_set(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_rip_router_conf_t router_conf;

    /* Get the current configuration */
    if (vtss_appl_rip_router_conf_get(&router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get RIP router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    /* Apply the new configuration */
    router_conf.admin_distance = req.admin_distance;
    rc = vtss_appl_rip_router_conf_set(&router_conf);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF(
            "%% Set RIP administrative distance configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP times configuration
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_router_times_set(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_rip_router_conf_t router_conf;

    if (vtss_appl_rip_router_conf_get(&router_conf) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Get RIP router configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    router_conf.timers.update_timer = req.update_timer;
    router_conf.timers.invalid_timer = req.invalid_timer;
    router_conf.timers.garbage_collection_timer = req.garbage_collection_timer;

    rc = vtss_appl_rip_router_conf_set(&router_conf);
    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Set RIP timers configuration failed.\n");
        return ICLI_RC_ERROR;
    }

    return ICLI_RC_OK;
}
//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
#define wildcard_mask_help_str                                  \
    "%% Cannot map the wildcard mask value to a valid network " \
    "mask length.\n"                                            \
    "The 'don't care' (value 1) bits must be sequential and "   \
    "the 'match' (value 0) bits MUST always be to the left.\n"  \
    "For example, 0.0.0.255 means that the network mask length is 8.\n"

/* Add RIP network */
mesa_rc FRR_ICLI_rip_network_add(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    uint32_t prefix = 0;
    mesa_ipv4_network_t network;

    if (req.has_wildcard_mask) {  // specified wildcard mask
        if (FRR_ICLI_rip_ipv4_wildcard_mask_to_prefix(req.wildcard_mask,
                                                      &prefix) != VTSS_RC_OK) {
            ICLI_PRINTF(wildcard_mask_help_str);
            return ICLI_RC_ERROR;
        }
    } else {  // apply classful subnet mask
        prefix = FRR_ICLI_ipv4_addr_to_prefix(req.ip_address);
        if (prefix == 0) {
            ICLI_PRINTF(
                "Can't find the network range for the given IP address\n");
            return ICLI_RC_ERROR;
        }
    }

    network.address = req.ip_address;
    network.prefix_size = prefix;
    rc = vtss_appl_rip_network_conf_add(&network);
    if (rc != VTSS_RC_OK) {
        if (rc == FRR_RC_ENTRY_ALREADY_EXISTS) {
            ICLI_PRINTF("%% The same network already exists.\n");
        } else {
            ICLI_PRINTF("%% %s.\n", error_txt(rc));
        }
    }

    return ICLI_RC_OK;
}

/* Delete RIP network */
mesa_rc FRR_ICLI_rip_network_del(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    uint32_t prefix;
    mesa_ipv4_network_t network;

    if (req.has_wildcard_mask) {  // specified wildcard mask
        if (FRR_ICLI_rip_ipv4_wildcard_mask_to_prefix(req.wildcard_mask,
                                                      &prefix) != VTSS_RC_OK) {
            ICLI_PRINTF(wildcard_mask_help_str);
            return ICLI_RC_ERROR;
        }
    } else {  // apply classful subnet mask
        prefix = FRR_ICLI_ipv4_addr_to_prefix(req.ip_address);
        if (prefix == 0) {
            ICLI_PRINTF(
                "Can't find the network range for the given IP address\n");
            return ICLI_RC_ERROR;
        }
    }

    network.address = req.ip_address;
    network.prefix_size = prefix;
    rc = vtss_appl_rip_network_conf_del(&network);
    if (rc != VTSS_RC_OK) {
        if (rc == VTSS_APPL_FRR_RIP_ERROR_NOT_UNICAST_ADDRESS) {
            ICLI_PRINTF("%% %s.\n", error_txt(rc));
        } else {
            ICLI_PRINTF("%% Delete RIP network failed.\n");
        }
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP neighbor connection
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_neighbor_add(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    if ((rc = vtss_appl_rip_neighbor_conf_add(req.ip_address)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% %s.\n", error_txt(rc));
    }

    return ICLI_RC_OK;
}

mesa_rc FRR_ICLI_rip_neighbor_del(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    if ((rc = vtss_appl_rip_neighbor_conf_del(req.ip_address)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% %s.\n", error_txt(rc));
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP authentication
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_intf_auth_conf_set(const FrrRipCliReq &req)
{
    mesa_rc rc;
    vtss_ifindex_t ifindex;
    u32 session_id = req.session_id;
    StringStream str_buf;
    vtss_appl_rip_intf_conf_t conf = {};

    // Iterate through all vlan IDs.
    for (u32 idx = 0; idx < req.vlan_list->cnt; ++idx) {
        for (mesa_vid_t vid = req.vlan_list->range[idx].min;
             vid <= req.vlan_list->range[idx].max; ++vid) {
            if (vtss_ifindex_from_vlan(vid, &ifindex) != VTSS_RC_OK ||
                vtss_appl_rip_intf_conf_get(ifindex, &conf) != VTSS_RC_OK) {
                ICLI_PRINTF(" %% Skip the command on VLAN %d.", vid);
                ICLI_PRINTF(
                    " Get interface or interface configuration failed).\n");
                continue;
            }

            /* Update the new configuration */
            if (req.auth_type != VTSS_APPL_RIP_AUTH_TYPE_COUNT) {
                conf.auth_type = req.auth_type;
            }

            if (req.has_simple_pwd_str) {
                strncpy(conf.simple_pwd, req.simple_pwd_str,
                        sizeof(conf.simple_pwd) - 1);
                conf.simple_pwd[sizeof(conf.simple_pwd) - 1] = '\0';
                conf.is_encrypted = req.has_encrypted;
            }

            if (req.has_keychain_name) {
                strncpy(conf.md5_key_chain_name, req.keychain_name,
                        sizeof(conf.md5_key_chain_name) - 1);
                conf.md5_key_chain_name[sizeof(conf.md5_key_chain_name) - 1] =
                    '\0';
            }

            /* Apply the new configuration */
            rc = vtss_appl_rip_intf_conf_set(ifindex, &conf);
            if (rc != VTSS_RC_OK) {
                str_buf.clear();
                str_buf << ifindex;
                ICLI_PRINTF(" %% %s on %s.\n", error_txt(rc), str_buf.cstring());
            }
        }
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP interface configuration
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_intf_conf_set(const FrrRipCliReq &req)
{
    mesa_rc rc;
    vtss_ifindex_t ifindex;
    u32 session_id = req.session_id;
    StringStream str_buf;
    vtss_appl_rip_intf_conf_t conf;

    // Iterate through all vlan IDs.
    for (u32 idx = 0; idx < req.vlan_list->cnt; ++idx) {
        for (mesa_vid_t vid = req.vlan_list->range[idx].min;
             vid <= req.vlan_list->range[idx].max; ++vid) {
            if (vtss_ifindex_from_vlan(vid, &ifindex) != VTSS_RC_OK ||
                vtss_appl_rip_intf_conf_get(ifindex, &conf) != VTSS_RC_OK) {
                ICLI_PRINTF(" %% Skip the command on VLAN %d.", vid);
                ICLI_PRINTF(
                    " Get interface or interface configuration failed).\n");
                continue;
            }

            /* Update the new configuration */
            // Send vesion
            if (req.intf_send_ver != VTSS_APPL_RIP_INTF_SEND_VER_COUNT) {
                conf.send_ver = req.intf_send_ver;
            }

            // Receive vesion
            if (req.intf_recv_ver != VTSS_APPL_RIP_INTF_RECV_VER_COUNT) {
                conf.recv_ver = req.intf_recv_ver;
            }

            // Split horizon
            if (req.has_split_horizon) {
                conf.split_horizon_mode = req.split_horizon_mode;
            }

            /* Apply the new configuration */
            rc = vtss_appl_rip_intf_conf_set(ifindex, &conf);
            if (rc != VTSS_RC_OK) {
                str_buf.clear();
                str_buf << ifindex;
                ICLI_PRINTF(" %% %s on %s.\n", error_txt(rc), str_buf.cstring());
            }
        }
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP metric manipulation: Offset-list
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_offset_list_set(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id;
    mesa_rc rc;
    vtss_appl_rip_offset_entry_data_t entry = {};

    if (req.has_no_form) {  // Delete operation
        rc = vtss_appl_rip_offset_list_conf_del(req.ifindex,
                                                req.offset_direction);
        if (rc != VTSS_RC_OK) {
            ICLI_PRINTF("%% Delete offset-list entry failed.\n");
        }
    } else if (vtss_appl_rip_offset_list_conf_get(
                   req.ifindex, req.offset_direction, &entry) ==
               VTSS_RC_OK) {  // Set operation
        entry.name = req.access_list_name;
        entry.offset_metric = req.metric;
        rc = vtss_appl_rip_offset_list_conf_set(req.ifindex,
                                                req.offset_direction, &entry);
        if (rc == FRR_RC_ENTRY_ALREADY_EXISTS ||
            rc == FRR_RC_LIMIT_REACHED) {
            ICLI_PRINTF("%% %s.\n", error_txt(rc));
        } else if (rc != VTSS_RC_OK) {
            ICLI_PRINTF("%% Add offset-list entry failed.\n");
        }
    } else {  // Add operation
        entry.name = req.access_list_name;
        entry.offset_metric = req.metric;
        rc = vtss_appl_rip_offset_list_conf_add(req.ifindex,
                                                req.offset_direction, &entry);
        if (rc != VTSS_RC_OK) {
            ICLI_PRINTF("%% %s.\n", error_txt(rc));
        }
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP interface status
//----------------------------------------------------------------------------
static mesa_rc FRR_ICLI_rip_interface_print_all(u32 session_id)
{
    vtss_ifindex_t ifindex;

    /* Output table header */
    StringStream str_buf;
    char buf[128];
    str_buf << "  Interface             Send  Recv  Triggered RIP  Auth"
            << "         MD5 Key-chain\n";
    ICLI_PRINTF("%s", str_buf.cstring());

    /* Get RIP interface information. */
    vtss::Vector<vtss::Pair<vtss_ifindex_t, vtss_appl_rip_interface_status_t>>
                                                                            data = {};
    if (vtss_appl_rip_interface_status_get_all(data) != VTSS_RC_OK || !data.size()) {
        // Quit silently when the get operation failed(RIP is diabled)
        // or no entry existing.
        return ICLI_RC_OK;
    }

    /* Output single entry information */
    for (auto &itr : data) {
        ifindex = itr.first;
        // Interface name
        ICLI_PRINTF("  %-22s", vtss_ifindex2str(buf, sizeof(buf), ifindex));

        // Send version
        str_buf.clear();
        if (itr.second.send_version == VTSS_APPL_RIP_INTF_SEND_VER_1) {
            str_buf << "1";
        } else if (itr.second.send_version == VTSS_APPL_RIP_INTF_SEND_VER_2) {
            str_buf << "2";
        } else if (itr.second.send_version == VTSS_APPL_RIP_INTF_SEND_VER_BOTH) {
            str_buf << "1 2";
        } else {
            str_buf << "?";
        }

        ICLI_PRINTF("%-6s", str_buf.cstring());

        // Receive version
        str_buf.clear();
        if (itr.second.recv_version == VTSS_APPL_RIP_INTF_RECV_VER_NONE) {
            str_buf << "None";
        } else if (itr.second.recv_version == VTSS_APPL_RIP_INTF_RECV_VER_1) {
            str_buf << "1";
        } else if (itr.second.recv_version == VTSS_APPL_RIP_INTF_RECV_VER_2) {
            str_buf << "2";
        } else if (itr.second.recv_version == VTSS_APPL_RIP_INTF_RECV_VER_BOTH) {
            str_buf << "1 2";
        } else {
            str_buf << "?";
        }

        ICLI_PRINTF("%-6s", str_buf.cstring());

        // Triggered Update
        str_buf.clear();
        if (itr.second.triggered_update) {
            str_buf << "Yes";
        } else {
            str_buf << "No";
        }

        ICLI_PRINTF("%-15s", str_buf.cstring());

        // Authentication
        str_buf.clear();
        if (itr.second.auth_type == VTSS_APPL_RIP_AUTH_TYPE_NULL) {
            str_buf << "None";
        } else if (itr.second.auth_type ==
                   VTSS_APPL_RIP_AUTH_TYPE_SIMPLE_PASSWORD) {
            str_buf << "Simple Pwd";
        } else if (itr.second.auth_type == VTSS_APPL_RIP_AUTH_TYPE_MD5) {
            str_buf << "MD5";
        } else {
            str_buf << "Unknown";
        }

        ICLI_PRINTF("%-13s", str_buf.cstring());

        // Key-chain
        str_buf.clear();
        str_buf << itr.second.key_chain;
        ICLI_PRINTF("%s", str_buf.cstring());
        ICLI_PRINTF("\n");
    }

    return ICLI_RC_OK;
}

static mesa_rc FRR_ICLI_rip_passive_interface_print(u32 session_id)
{
    vtss_ifindex_t ifindex;
    char           buf[128];

    /* Output table header */
    ICLI_PRINTF("Passive Interface(s):");

    /* Get RIP interface information. */
    vtss::Vector<vtss::Pair<vtss_ifindex_t, vtss_appl_rip_interface_status_t>> data = {};
    if (vtss_appl_rip_interface_status_get_all(data) != VTSS_RC_OK || !data.size()) {
        // Quit silently when the get operation failed(RIP is diabled)
        // or no entry existing.
        return ICLI_RC_OK;
    }

    /* Output single entry information */
    for (auto &itr : data) {
        if (itr.second.is_passive_intf) {
            // Interface name
            ifindex = itr.first;
            ICLI_PRINTF("  %s\n", vtss_ifindex2str(buf, sizeof(buf), ifindex));
        }
    }

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP neighbor status
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_show_peer_info(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id; /* used for ICLI_PRINTF */
    char ip_str_buf[16];
    StringStream str_buf;
    /* Output table header */
    str_buf << "Routing Information Sources:\n";
    ICLI_PRINTF("%s", str_buf.cstring());

    /* Get peer information. */
    vtss::Vector<vtss::Pair<mesa_ipv4_t, vtss_appl_rip_peer_data_t>> database = {};
    if (vtss_appl_rip_peer_get_all(database) != VTSS_RC_OK || !database.size()) {
        // Quit silently when the get operation failed(RIP is diabled)
        // or no entry existing.
        return ICLI_RC_OK;
    }

    /* peer table */
    str_buf.clear();
    str_buf << "  Gateway        Last Update Version  Recv. Bad Packets   Recv."
            << " Bad Routes";
    ICLI_PRINTF("%s", str_buf.cstring());

    /* Output single entry information */
    for (auto &itr : database) {
        ICLI_PRINTF("\n%-15s ", misc_ipv4_txt(itr.first, ip_str_buf));
        ICLI_PRINTF("%12s", misc_time_txt(itr.second.last_update_time));
        ICLI_PRINTF("%8d ", itr.second.rip_version);

        ICLI_PRINTF("%18u ", itr.second.recv_bad_packets);
        ICLI_PRINTF("%18u ", itr.second.recv_bad_routes);
    }

    ICLI_PRINTF("\n");
    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP general status
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_show_status_info(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id; /* used for ICLI_PRINTF */

    vtss_appl_rip_general_status_t status = {};
    if (vtss_appl_rip_general_status_get(&status) != VTSS_RC_OK) {
        // Quit silently when the get operation failed.
        return ICLI_RC_OK;
    }

    if (!status.is_enabled) {
        // Quit silently when RIP is diabled.
        return ICLI_RC_OK;
    }

    StringStream str_buf;

    /* Timers information */
    str_buf << "Sending updates every " << status.timers.update_timer
            << " seconds, next due in " << status.next_update_time
            << " seconds\n"
            << "Invalid after " << status.timers.invalid_timer
            << " seconds, garbage collect after "
            << status.timers.garbage_collection_timer << " seconds\n";
    ICLI_PRINTF("%s", str_buf.cstring());

    /* Default redistribution metric */
    str_buf.clear();
    str_buf << "Default redistribution metric is " << status.default_metric
            << "\n";
    ICLI_PRINTF("%s", str_buf.cstring());

    /* Redistribute protocols */
    str_buf.clear();
    str_buf << "Redistributing:";
    if (status.redist_proto_type[VTSS_APPL_RIP_REDIST_PROTO_TYPE_CONNECTED]) {
        str_buf << " connected";
    }

    if (status.redist_proto_type[VTSS_APPL_RIP_REDIST_PROTO_TYPE_STATIC]) {
        str_buf << " static";
    }

    if (status.redist_proto_type[VTSS_APPL_RIP_REDIST_PROTO_TYPE_OSPF]) {
        str_buf << " ospf";
    }

    str_buf << "\n";
    ICLI_PRINTF("%s", str_buf.cstring());

    /* Global version */
    str_buf.clear();
    str_buf << "Default version control: ";
    if (status.version == VTSS_APPL_RIP_GLOBAL_VER_1) {
        str_buf << "send version 1, receive version 1\n";
    } else if (status.version == VTSS_APPL_RIP_GLOBAL_VER_2) {
        str_buf << "send version 2, receive version 2\n";
    } else {
        str_buf << "send version 2, receive any version\n";
    }

    ICLI_PRINTF("%s", str_buf.cstring());

    /* Interface status */
    FRR_ICLI_rip_interface_print_all(req.session_id);

    /* Network configureaion */
    str_buf.clear();
    str_buf << "Routing for Networks:\n";
    str_buf << "  Address         Wildcard-mask\n";
    ICLI_PRINTF("%s", str_buf.cstring());

    mesa_ipv4_network_t *cur_network_key_p = NULL;  // Get-First operation
    mesa_ipv4_network_t cur_network_key, next_network_key;
    mesa_ipv4_t network_mask;

    /* Iterate through all existing entries. */
    while (vtss_appl_rip_network_conf_itr(cur_network_key_p, &next_network_key) == VTSS_RC_OK) {
        // Switch to current data for next loop
        if (!cur_network_key_p) {  // Get-First operation
            cur_network_key_p = &cur_network_key;  // Switch to Get-Next operation
        }

        cur_network_key = next_network_key;

        if (vtss_appl_rip_network_conf_get(cur_network_key_p) != VTSS_RC_OK) {
            continue;
        }

        network_mask = vtss_ipv4_prefix_to_mask(cur_network_key_p->prefix_size);

        // show network
        str_buf.clear();
        str_buf << AsIpv4(cur_network_key_p->address);
        ICLI_PRINTF("  %-16s", str_buf.cstring());

        // show mask
        network_mask = ~network_mask;
        str_buf.clear();
        str_buf << AsIpv4(network_mask);
        ICLI_PRINTF("%s\n", str_buf.cstring());
    }

    /* Active passive interface */
    FRR_ICLI_rip_passive_interface_print(req.session_id);

    /* Peers information */
    FRR_ICLI_rip_show_peer_info(req);

    /* Administrative distance */
    str_buf.clear();
    str_buf << "Distance: (default is " << status.admin_distance << ")\n";
    ICLI_PRINTF("%s", str_buf.cstring());
    ICLI_PRINTF("\n");

    return ICLI_RC_OK;
}

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------

static const char *FRR_ICLI_rip_db_proto_type_str(vtss_appl_rip_db_proto_type_t type, bool is_short_name)
{
    switch (type) {
    case VTSS_APPL_RIP_DB_PROTO_TYPE_RIP:
        return is_short_name ? "R" : "rip";
    case VTSS_APPL_RIP_DB_PROTO_TYPE_CONNECTED:
        return is_short_name ? "C" : "connected";
    case VTSS_APPL_RIP_DB_PROTO_TYPE_STATIC:
        return is_short_name ? "S" : "static";
    case VTSS_APPL_RIP_DB_PROTO_TYPE_OSPF:
        return is_short_name ? "O" : "ospf";
    case VTSS_APPL_RIP_DB_PROTO_TYPE_COUNT:
        break;
    }

    return "?";
}

static const char *FRR_ICLI_rip_db_proto_subtype_str(
    vtss_appl_rip_db_proto_subtype_t subtype)
{
    switch (subtype) {
    case VTSS_APPL_RIP_DB_PROTO_SUBTYPE_STATIC:
        return "s";
    case VTSS_APPL_RIP_DB_PROTO_SUBTYPE_NORMAL:
        return "n";
    case VTSS_APPL_RIP_DB_PROTO_SUBTYPE_DEFAULT:
        return "d";
    case VTSS_APPL_RIP_DB_PROTO_SUBTYPE_REDIST:
        return "r";
    case VTSS_APPL_RIP_DB_PROTO_SUBTYPE_INTF:
        return "i";
    case VTSS_APPL_RIP_DB_PROTO_SUBTYPE_COUNT:
        break;
    }

    return "?";
}

/* Show RIP database information */
mesa_rc FRR_ICLI_rip_show_db_info(const FrrRipCliReq &req)
{
    u32 session_id = req.session_id; /* used for ICLI_PRINTF */
    /* Get OSPF database information. */
    vtss::Vector<vtss::Pair<vtss_appl_rip_db_key_t, vtss_appl_rip_db_data_t>>
                                                                           database = {};
    if (vtss_appl_rip_db_get_all(database) != VTSS_RC_OK || !database.size()) {
        // Quit silently when the get operation failed(RIP is diabled)
        // or no entry existing.
        return ICLI_RC_OK;
    }

    /* Output table header */
    StringStream str_buf;
    char ip_str_buf[16];
    str_buf << "Codes: R - RIP, C - connected, S - Static, O - OSPF\n"
            << "Sub-codes:\n"
            << "      (n) - normal, (s) - static, (d) - default, (r) - "
            "redistribute,\n"
            << "      (i) - interface\n\n"
            << "     Network            Next Hop        Metric From            "
            << "Ext. Metric   Tag Time";
    ICLI_PRINTF("%s", str_buf.cstring());

    /* Output single entry information */
    for (auto &itr : database) {
        ICLI_PRINTF("\n%s(%s) ",
                    FRR_ICLI_rip_db_proto_type_str(itr.second.type, true),
                    FRR_ICLI_rip_db_proto_subtype_str(itr.second.subtype));
        str_buf.clear();
        str_buf << itr.first.network;
        ICLI_PRINTF("%-18s ", str_buf.cstring());
        ICLI_PRINTF("%-15s ", misc_ipv4_txt(itr.first.nexthop, ip_str_buf));
        ICLI_PRINTF("%6u ", itr.second.metric);
        if (itr.second.type == VTSS_APPL_RIP_DB_PROTO_TYPE_RIP &&
            itr.second.subtype == VTSS_APPL_RIP_DB_PROTO_SUBTYPE_NORMAL) {
            ICLI_PRINTF("%-15s             ",
                        misc_ipv4_txt(itr.second.src_addr, ip_str_buf));
        } else if (itr.second.external_metric) {
            ICLI_PRINTF("self            %11u ", itr.second.external_metric);
        } else {
            ICLI_PRINTF("self                        ");
        }

        ICLI_PRINTF("%5u ", itr.second.tag);
        if (itr.second.type == VTSS_APPL_RIP_DB_PROTO_TYPE_RIP &&
            itr.second.subtype == VTSS_APPL_RIP_DB_PROTO_SUBTYPE_NORMAL) {
            ICLI_PRINTF("%s", misc_time_txt(itr.second.uptime));
        }
    }

    ICLI_PRINTF("\n");
    return ICLI_RC_OK;
}

