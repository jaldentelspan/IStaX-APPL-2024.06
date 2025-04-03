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

/**
 * \file frr_rip_icli_functions.h
 * \brief This file contains the definitions of FRR module's ICLI API functions.
 */

#ifndef _FRR_RIP_ICLI_FUNCTIONS_HXX_
#define _FRR_RIP_ICLI_FUNCTIONS_HXX_

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include <vtss/appl/rip.h>
#include "vtss_icli_type.h"  // For icli_unsigned_range_t

/******************************************************************************/
/** Module ICLI request structure declaration                                 */
/******************************************************************************/
struct FrrRipCliReq {
    FrrRipCliReq(u32 &id)
    {
        session_id = id;
    }

    // CLI session ID
    u32 session_id;

    // Indicate if user has typed "no" keyword or not
    bool has_no_form = false;

    // Global version
    vtss_appl_rip_global_ver_t global_ver = VTSS_APPL_RIP_GLOBAL_VER_DEFAULT;

    // Interface send version
    vtss_appl_rip_intf_send_ver_t intf_send_ver =
        VTSS_APPL_RIP_INTF_SEND_VER_COUNT;

    // Interface receive version
    mesa_bool_t has_intf_recv_ver = false;
    vtss_appl_rip_intf_recv_ver_t intf_recv_ver =
        VTSS_APPL_RIP_INTF_RECV_VER_COUNT;

    // RIP meteric
    mesa_bool_t has_metric = false;
    vtss_appl_rip_metric_t metric = 0;

    // OSPF route redistribution
    vtss_appl_rip_redist_proto_type_t redist_proto_type =
        VTSS_APPL_RIP_REDIST_PROTOCOL_COUNT;

    // RIP default route redistribution
    bool def_route_redist = false;

    // RIP passive-interface
    mesa_bool_t passive_intf_enabled = false;

    // RIP administrative distance
    vtss_appl_rip_distance_t admin_distance = 0;

    // VLAN list
    icli_unsigned_range_t *vlan_list = NULL;

    // Perform Split horizon mode
    mesa_bool_t has_split_horizon = false;
    vtss_appl_rip_split_horizon_mode_t split_horizon_mode;

    // RIP IP address
    mesa_ipv4_t ip_address = 0;

    // RIP wildcard mask
    bool has_wildcard_mask = false;
    mesa_ipv4_t wildcard_mask = 0;

    // RIP timers
    vtss_appl_rip_timer_t update_timer = 0;
    vtss_appl_rip_timer_t invalid_timer = 0;
    vtss_appl_rip_timer_t garbage_collection_timer = 0;

    // RIP authentication
    vtss_appl_rip_auth_type_t auth_type = VTSS_APPL_RIP_AUTH_TYPE_COUNT;
    bool has_simple_pwd_str = false, has_keychain_name = false;
    bool has_encrypted = false;
    char simple_pwd_str[VTSS_APPL_RIP_AUTH_ENCRYPTED_SIMPLE_KEY_LEN + 1] =
        "";  // simple password str
    char keychain_name[VTSS_APPL_ROUTER_KEY_CHAIN_NAME_MAX_LEN + 1] =
        "";  // key chain profile name

    // RIP metric manipulation: Offset-list
    vtss_appl_router_access_list_name_t access_list_name = {};
    vtss_appl_rip_offset_direction_t offset_direction;
    vtss_ifindex_t ifindex = {0};
};

/******************************************************************************/
/** Module ICLI APIs                                                          */
/******************************************************************************/

//----------------------------------------------------------------------------
//** RIP router configuration
//----------------------------------------------------------------------------
/* Set RIP global version */
mesa_rc FRR_ICLI_rip_global_ver_set(const FrrRipCliReq &req);

/* Set RIP default metric */
mesa_rc FRR_ICLI_rip_redist_def_metric_set(const FrrRipCliReq &req);

/* Set RIP redistribution metric */
mesa_rc FRR_ICLI_rip_redist_set(const FrrRipCliReq &req);

/* Set RIP all interfaces as passive-interface by default */
mesa_rc FRR_ICLI_rip_passive_intf_default(const FrrRipCliReq &req);

/* Set RIP passive-interface default mode */
mesa_rc FRR_ICLI_rip_passive_intf_set(const FrrRipCliReq &req);

/* Set RIP default route redistribution */
mesa_rc FRR_ICLI_rip_def_route_redist_set(const FrrRipCliReq &req);

/* Set RIP administrative distance */
mesa_rc FRR_ICLI_rip_admin_distance_set(const FrrRipCliReq &req);

//----------------------------------------------------------------------------
//** RIP times configuration
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_router_times_set(const FrrRipCliReq &req);
mesa_rc FRR_ICLI_rip_router_times_del(const FrrRipCliReq &req);

//----------------------------------------------------------------------------
//** RIP network configuration
//----------------------------------------------------------------------------
/* Add RIP network */
mesa_rc FRR_ICLI_rip_network_add(const FrrRipCliReq &req);

/* Delete RIP network */
mesa_rc FRR_ICLI_rip_network_del(const FrrRipCliReq &req);

//----------------------------------------------------------------------------
//** RIP neighbor connection
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_neighbor_add(const FrrRipCliReq &req);
mesa_rc FRR_ICLI_rip_neighbor_del(const FrrRipCliReq &req);

//----------------------------------------------------------------------------
//** RIP authentication
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_intf_auth_conf_set(const FrrRipCliReq &req);

//----------------------------------------------------------------------------
//** RIP interface configuration
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_intf_conf_set(const FrrRipCliReq &req);

//----------------------------------------------------------------------------
//** RIP metric manipulation: Offset-list
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_offset_list_set(const FrrRipCliReq &req);

//----------------------------------------------------------------------------
//** RIP general status
//----------------------------------------------------------------------------
mesa_rc FRR_ICLI_rip_show_status_info(const FrrRipCliReq &req);

//----------------------------------------------------------------------------
//** RIP database
//----------------------------------------------------------------------------
/* Show RIP database information */
mesa_rc FRR_ICLI_rip_show_db_info(const FrrRipCliReq &req);

#endif /* _FRR_RIP_ICLI_FUNCTIONS_HXX_ */

