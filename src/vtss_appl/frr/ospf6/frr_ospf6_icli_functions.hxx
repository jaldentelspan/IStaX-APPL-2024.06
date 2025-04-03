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

/**
 * \file frr_ospf6_icli_functions.h
 * \brief This file contains the definitions of FRR module's ICLI API functions.
 */

#ifndef _FRR_OSPF6_ICLI_FUNCTIONS_HXX_
#define _FRR_OSPF6_ICLI_FUNCTIONS_HXX_

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include <vtss/appl/ospf6.h>
#include "vtss_icli_type.h"  // For icli_unsigned_range_t

/******************************************************************************/
/** Module ICLI request structure declaration                                 */
/******************************************************************************/
struct FrrOspf6CliReq {
    FrrOspf6CliReq(u32 &id)
    {
        session_id = id;
    }

    // CLI session ID
    u32 session_id;

    // Indicate if user has typed "no" keyword or not
    bool has_no_form = false;

    // OSPF6 process instance ID
    vtss_appl_ospf6_id_t inst_id = 0;

    // OSPF6 router ID
    vtss_appl_ospf6_router_id_t router_id = 0;

    // OSPF6 area ID
    mesa_bool_t has_area_id = false;
    vtss_appl_ospf6_area_id_t area_id = 0;
    mesa_bool_t configure_area_id = false;

    // OSPF6 IP address
    mesa_ipv6_t ip_address = {0};

    // OSPF6 IP address mask
    mesa_ipv6_t ip_address_mask = {0};

    // OSPF6 wildcard mask
    mesa_ipv6_t wildcard_mask = {0};

    // OSPF6 IPv6 address subnet
    mesa_ipv6_network_t subnet = {0};

    // OSPF6 passive-interface
    mesa_bool_t passive_enabled = false;

    // VLAN list
    icli_unsigned_range_t *vlan_list = NULL;

    // VLINK(Virtual link) list
    icli_unsigned_range_t *vlink_list = NULL;

    // OSPF6 authentication
    vtss_appl_ospf6_md_key_id_t md_key_id = false;
    mesa_bool_t has_encrypted = false;

    // OSPF6 area range advertise
    mesa_bool_t area_range_advertise = false;

    // OSPF6 cost
    mesa_bool_t has_cost = false;
    vtss_appl_ospf6_cost_t cost = 0;

    // MTU Ignore
    bool has_mtu_ignore = false;
    bool mtu_ignore = false;

    // OSPF6 stub area
    mesa_bool_t has_no_summary = false;

    // OSPF6 priority
    mesa_bool_t has_priority = false;
    vtss_appl_ospf6_priority_t priority = 0;

    // OSPF6 dead interval
    mesa_bool_t has_dead_interval = false;
    uint32_t dead_interval = 0;

    // OSPF6 passive interface
    mesa_bool_t has_passive = false;

    // OSPF6 fast hello packets
    mesa_bool_t has_fast_hello = false;

    // OSPF6 hello interval
    mesa_bool_t has_hello_interval = false;
    uint32_t hello_interval = 0;

    // OSPF6 retransmit interval
    mesa_bool_t has_retransmit_interval = false;
    uint32_t retransmit_interval = 0;

    // OSPF6 retransmit interval
    mesa_bool_t has_transmit_delay = false;
    uint32_t transmit_delay = 0;

    // OSPF6 route redistribution
    uint32_t redist_protocol = 0;
    mesa_bool_t redist_enabled = true;

    mesa_bool_t has_metric = false;
    vtss_appl_ospf6_metric_t metric = 0;

    // OSPF6 default route
    mesa_bool_t has_always = false;

    // option to show detail information
    mesa_bool_t has_detail = false;

    // OSPF6 database information
    vtss_appl_ospf6_lsdb_type_t lsdb_type = VTSS_APPL_OSPF6_LSDB_TYPE_NONE;
    mesa_bool_t has_link_state_id = false;
    mesa_ipv4_t link_state_id = 0;
    mesa_bool_t has_adv_router_id = false;
    vtss_appl_ospf6_router_id_t adv_router_id = 0;
    mesa_bool_t has_self_originate = false;

    // OSPF6 administrative distance
    uint8_t admin_distance = VTSS_APPL_OSPF6_ADMIN_DISTANCE_MIN;
};

/******************************************************************************/
/** Module ICLI APIs                                                          */
/******************************************************************************/
/* Set OSPF6 router ID */
mesa_rc FRR_ICLI_ospf6_router_id_set(const FrrOspf6CliReq &req);

/* Delete OSPF6 router ID */
mesa_rc FRR_ICLI_ospf6_router_id_del(const FrrOspf6CliReq &req);

/* Set OSPF6 area-id */
mesa_rc FRR_ICLI_ospf6_interface_area_set(const FrrOspf6CliReq &req);

//------------------------------------------------------------------------------
//** OSPF6 instance process configuration
//------------------------------------------------------------------------------
/* Delete OSPF6 instance process */
mesa_rc FRR_ICLI_ospf6_instance_process_del(const FrrOspf6CliReq &req);

//------------------------------------------------------------------------------
//** OSPF6 administrative distance
//------------------------------------------------------------------------------
/* Set OSPF6 administrative distance */
mesa_rc FRR_ICLI_ospf6_admin_distance_set(const FrrOspf6CliReq &req);

//------------------------------------------------------------------------------
//** OSPF6 route redistribution
//------------------------------------------------------------------------------
/* Set OSPF6 route redistribution */
mesa_rc FRR_ICLI_ospf6_redist_set(const FrrOspf6CliReq &req);

//------------------------------------------------------------------------------
//** OSPF6 route default route redistribution
//------------------------------------------------------------------------------
/* Set OSPF6 default route redistribution */
mesa_rc FRR_ICLI_ospf6_def_route_set(const FrrOspf6CliReq &req);

//------------------------------------------------------------------------------
//** OSPF6 default metric
//------------------------------------------------------------------------------
/* Set OSPF6 default metric */
mesa_rc FRR_ICLI_ospf6_def_metric_set(const FrrOspf6CliReq &req);

/* Set OSPF6 network area */
mesa_rc FRR_ICLI_ospf6_network_area_set(const FrrOspf6CliReq &req);

/* Delete OSPF6 network area */
mesa_rc FRR_ICLI_ospf6_network_area_del(const FrrOspf6CliReq &req);

//----------------------------------------------------------------------------
//** OSPF6 interface authentication
//----------------------------------------------------------------------------
/* Delete OSPF6 interface authentication */
mesa_rc FRR_ICLI_ospf6_intf_auth_del(const FrrOspf6CliReq &req);

/* Delete OSPF6 interface authentication simple password */
mesa_rc FRR_ICLI_ospf6_intf_auth_simple_pwd_del(const FrrOspf6CliReq &req);

//----------------------------------------------------------------------------
//** OSPF6 area range
//----------------------------------------------------------------------------
/* Set OSPF6 area range */
mesa_rc FRR_ICLI_ospf6_area_range_set(const FrrOspf6CliReq &req);

/* Delete OSPF6 area range */
mesa_rc FRR_ICLI_ospf6_area_range_del_or_restore(const FrrOspf6CliReq &req);

//----------------------------------------------------------------------------
//** OSPF6 stub area
//----------------------------------------------------------------------------
/* Set the OSPF6 stub area */
mesa_rc FRR_ICLI_ospf6_area_stub_set(const FrrOspf6CliReq &req);

/* Delete the OSPF6 stub area or set totally stub area as a stub area */
mesa_rc FRR_ICLI_ospf6_area_stub_no(const FrrOspf6CliReq &req);

//----------------------------------------------------------------------------
//** OSPF6 virtual link
//----------------------------------------------------------------------------
/* Set OSPF6 virtual link */
mesa_rc FRR_ICLI_ospf6_area_vlink_set(const FrrOspf6CliReq &req);

//----------------------------------------------------------------------------
//** OSPF6 interface parameter tuning
//----------------------------------------------------------------------------
/* Set OSPF6 vlan interface parameters */
mesa_rc FRR_ICLI_ospf6_vlan_interface_set(const FrrOspf6CliReq &req);

/* Reset OSPF6 vlan interface parameters to default */
mesa_rc FRR_ICLI_ospf6_vlan_interface_set_default(const FrrOspf6CliReq &req);

//----------------------------------------------------------------------------
//** OSPF6 show
//----------------------------------------------------------------------------
/* Show OSPF6 information */
mesa_rc FRR_ICLI_ospf6_show_info(const FrrOspf6CliReq &req);

/* Show OSPF6 interface information */
mesa_rc FRR_ICLI_ospf6_show_interface(const FrrOspf6CliReq &req);

/* Show OSPF6 neighbor information */
mesa_rc FRR_ICLI_ospf6_show_neighbor(const FrrOspf6CliReq &req);

//----------------------------------------------------------------------------
//** OSPF6 route information
//----------------------------------------------------------------------------
/* Show OSPF6 routing information */
mesa_rc FRR_ICLI_ospf6_show_route(const FrrOspf6CliReq &req);

//----------------------------------------------------------------------------
//** OSPF6 database information
//----------------------------------------------------------------------------
/* Show OSPF6 database general summary information */
mesa_rc FRR_ICLI_ospf6_show_db_general_info(const FrrOspf6CliReq &req);

/* Show OSPF6 database detail information */
mesa_rc FRR_ICLI_ospf6_show_db_detail_info(const FrrOspf6CliReq &req);

#endif /* _FRR_OSPF6_ICLI_FUNCTIONS_HXX_ */

