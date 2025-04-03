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
 * \file frr_ospf_icli_functions.h
 * \brief This file contains the definitions of FRR module's ICLI API functions.
 */

#ifndef _FRR_OSPF_ICLI_FUNCTIONS_HXX_
#define _FRR_OSPF_ICLI_FUNCTIONS_HXX_

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include <vtss/appl/ospf.h>
#include "vtss_icli_type.h"  // For icli_unsigned_range_t

/******************************************************************************/
/** Module ICLI request structure declaration                                 */
/******************************************************************************/
// Maximum length of encrypted password.
#define FRR_ICLI_MAX_ENCRYPTED_PWD_LEN                            \
    (VTSS_APPL_OSPF_AUTH_ENCRYPTED_DIGEST_KEY_LEN >               \
                     VTSS_APPL_OSPF_AUTH_ENCRYPTED_SIMPLE_KEY_LEN \
             ? VTSS_APPL_OSPF_AUTH_ENCRYPTED_DIGEST_KEY_LEN       \
             : VTSS_APPL_OSPF_AUTH_ENCRYPTED_SIMPLE_KEY_LEN)

struct FrrOspfCliReq {
    FrrOspfCliReq(u32 &id)
    {
        session_id = id;
    }

    // CLI session ID
    u32 session_id;

    // Indicate if user has typed "no" keyword or not
    bool has_no_form = false;

    // OSPF process instance ID
    vtss_appl_ospf_id_t inst_id = 0;

    // OSPF router ID
    vtss_appl_ospf_router_id_t router_id = 0;

    // OSPF area ID
    mesa_bool_t has_area_id = false;
    vtss_appl_ospf_area_id_t area_id = 0;

    // OSPF IP address
    mesa_ipv4_t ip_address = 0;

    // OSPF IP address mask
    mesa_ipv4_t ip_address_mask = 0;

    // OSPF wildcard mask
    mesa_ipv4_t wildcard_mask = 0;

    // OSPF passive-interface
    mesa_bool_t passive_enabled = false;

    // VLAN list
    icli_unsigned_range_t *vlan_list = NULL;

    // VLINK(Virtual link) list
    icli_unsigned_range_t *vlink_list = NULL;

    // OSPF authentication
    vtss_appl_ospf_auth_type_t auth_type = VTSS_APPL_OSPF_AUTH_TYPE_AREA_CFG;
    vtss_appl_ospf_md_key_id_t md_key_id = false;
    mesa_bool_t has_encrypted = false;
    char password[FRR_ICLI_MAX_ENCRYPTED_PWD_LEN + 1] =
        "";  // + 1 for termination character

    // OSPF area range advertise
    mesa_bool_t area_range_advertise = false;

    // OSPF cost
    mesa_bool_t has_cost = false;
    vtss_appl_ospf_cost_t cost = 0;

    // MTU Ignore
    bool has_mtu_ignore = false;
    bool mtu_ignore = false;

    // OSPF stub area
    mesa_bool_t is_nssa = false;
    mesa_bool_t has_no_summary = false;
    mesa_bool_t has_nssa_translator_role = false;
    vtss_appl_ospf_nssa_translator_role_t nssa_translator_role =
        VTSS_APPL_OSPF_NSSA_TRANSLATOR_ROLE_CANDIDATE;

    // OSPF priority
    mesa_bool_t has_priority = false;
    vtss_appl_ospf_priority_t priority = 0;

    // OSPF dead interval
    mesa_bool_t has_dead_interval = false;
    uint32_t dead_interval = 0;

    // OSPF fast hello packets
    mesa_bool_t has_fast_hello = false;
    uint32_t fast_hello_packets = 0;

    // OSPF hello interval
    mesa_bool_t has_hello_interval = false;
    uint32_t hello_interval = 0;

    // OSPF retransmit interval
    mesa_bool_t has_retransmit_interval = false;
    uint32_t retransmit_interval = 0;

    // OSPF route redistribution
    uint32_t redist_protocol = 0;

    // OSPF metric type and value
    vtss_appl_ospf_redist_metric_type_t metric_type =
        VTSS_APPL_OSPF_REDIST_METRIC_TYPE_2;
    mesa_bool_t has_metric = false;
    vtss_appl_ospf_metric_t metric = 0;

    // OSPF default route
    mesa_bool_t has_always = false;

    // option to show detail information
    mesa_bool_t has_detail = false;

    // OSPF stub router
    mesa_bool_t has_on_startup = false;
    uint32_t on_startup_interval = 5;
    mesa_bool_t has_on_shutdown = false;
    uint32_t on_shutdown_interval = 5;
    mesa_bool_t has_administrative = false;

    // OSPF database information
    vtss_appl_ospf_lsdb_type_t lsdb_type = VTSS_APPL_OSPF_LSDB_TYPE_NONE;
    mesa_bool_t has_link_state_id = false;
    mesa_ipv4_t link_state_id = 0;
    mesa_bool_t has_adv_router_id = false;
    vtss_appl_ospf_router_id_t adv_router_id = 0;
    mesa_bool_t has_self_originate = false;

    // OSPF administrative distance
    uint8_t admin_distance = VTSS_APPL_OSPF_ADMIN_DISTANCE_MIN;
};

/******************************************************************************/
/** Module ICLI APIs                                                          */
/******************************************************************************/
/* Set OSPF router ID */
mesa_rc FRR_ICLI_ospf_router_id_set(const FrrOspfCliReq &req);

/* Delete OSPF router ID */
mesa_rc FRR_ICLI_ospf_router_id_del(const FrrOspfCliReq &req);

/* Set OSPF all interfaces as passive-interface by default */
mesa_rc FRR_ICLI_ospf_passive_interface_default(const FrrOspfCliReq &req);

/* Set OSPF passive-interface */
mesa_rc FRR_ICLI_ospf_passive_interface_set(const FrrOspfCliReq &req);

//------------------------------------------------------------------------------
//** OSPF instance process configuration
//------------------------------------------------------------------------------
/* Delete OSPF instance process */
mesa_rc FRR_ICLI_ospf_instance_process_del(const FrrOspfCliReq &req);

//------------------------------------------------------------------------------
//** OSPF administrative distance
//------------------------------------------------------------------------------
/* Set OSPF administrative distance */
mesa_rc FRR_ICLI_ospf_admin_distance_set(const FrrOspfCliReq &req);

//------------------------------------------------------------------------------
//** OSPF route redistribution
//------------------------------------------------------------------------------
/* Set OSPF route redistribution */
mesa_rc FRR_ICLI_ospf_redist_set(const FrrOspfCliReq &req);

//------------------------------------------------------------------------------
//** OSPF route default route redistribution
//------------------------------------------------------------------------------
/* Set OSPF default route redistribution */
mesa_rc FRR_ICLI_ospf_def_route_set(const FrrOspfCliReq &req);

//------------------------------------------------------------------------------
//** OSPF default metric
//------------------------------------------------------------------------------
/* Set OSPF default metric */
mesa_rc FRR_ICLI_ospf_def_metric_set(const FrrOspfCliReq &req);

/* Set OSPF network area */
mesa_rc FRR_ICLI_ospf_network_area_set(const FrrOspfCliReq &req);

/* Delete OSPF network area */
mesa_rc FRR_ICLI_ospf_network_area_del(const FrrOspfCliReq &req);

//----------------------------------------------------------------------------
//** OSPF interface authentication
//----------------------------------------------------------------------------
/* Set OSPF interface authentication */
mesa_rc FRR_ICLI_ospf_intf_auth_set(const FrrOspfCliReq &req);

/* Delete OSPF interface authentication */
mesa_rc FRR_ICLI_ospf_intf_auth_del(const FrrOspfCliReq &req);

/* Set OSPF interface authentication simple password */
mesa_rc FRR_ICLI_ospf_intf_auth_simple_pwd_set(const FrrOspfCliReq &req);

/* Delete OSPF interface authentication simple password */
mesa_rc FRR_ICLI_ospf_intf_auth_simple_pwd_del(const FrrOspfCliReq &req);

/* Set OSPF interface authentication message digest key */
mesa_rc FRR_ICLI_ospf_intf_auth_md_key_set(const FrrOspfCliReq &req);

/* Delete OSPF interface authentication message digest key */
mesa_rc FRR_ICLI_ospf_intf_auth_md_key_del(const FrrOspfCliReq &req);

//----------------------------------------------------------------------------
//** OSPF area authentication
//----------------------------------------------------------------------------
/* Set OSPF area authentication */
mesa_rc FRR_ICLI_ospf_area_auth_set(const FrrOspfCliReq &req);

/* Delete OSPF area authentication */
mesa_rc FRR_ICLI_ospf_area_auth_del(const FrrOspfCliReq &req);

//----------------------------------------------------------------------------
//** OSPF area range
//----------------------------------------------------------------------------
/* Set OSPF area range */
mesa_rc FRR_ICLI_ospf_area_range_set(const FrrOspfCliReq &req);

/* Delete OSPF area range */
mesa_rc FRR_ICLI_ospf_area_range_del_or_restore(const FrrOspfCliReq &req);

//----------------------------------------------------------------------------
//** OSPF stub area
//----------------------------------------------------------------------------
/* Set the OSPF stub area */
mesa_rc FRR_ICLI_ospf_area_stub_set(const FrrOspfCliReq &req);

/* Delete the OSPF stub area or set totally stub area as a stub area */
mesa_rc FRR_ICLI_ospf_area_stub_no(const FrrOspfCliReq &req);

//----------------------------------------------------------------------------
//** OSPF virtual link
//----------------------------------------------------------------------------
/* Set OSPF virtual link */
mesa_rc FRR_ICLI_ospf_area_vlink_set(const FrrOspfCliReq &req);

/* Restore OSPF virtual link default setting */
mesa_rc FRR_ICLI_ospf_vlink_conf_restore(const FrrOspfCliReq &req);

/* Set OSPF virtual link authentication type */
mesa_rc FRR_ICLI_ospf_vlink_auth_type_set(const FrrOspfCliReq &req);

/* Set OSPF virtual link authentication simple password */
mesa_rc FRR_ICLI_ospf_vlink_auth_simple_pwd_set(const FrrOspfCliReq &req);

/* Set OSPF virtual link authentication message digest */
mesa_rc FRR_ICLI_ospf_vlink_md_key_set(const FrrOspfCliReq &req);

/* Delete OSPF virtual link authentication message digest */
mesa_rc FRR_ICLI_ospf_vlink_md_key_del(const FrrOspfCliReq &req);

//----------------------------------------------------------------------------
//** OSPF interface parameter tuning
//----------------------------------------------------------------------------
/* Set OSPF vlan interface parameters */
mesa_rc FRR_ICLI_ospf_vlan_interface_set(const FrrOspfCliReq &req);

/* Reset OSPF vlan interface parameters to default */
mesa_rc FRR_ICLI_ospf_vlan_interface_set_default(const FrrOspfCliReq &req);

//----------------------------------------------------------------------------
//** OSPF show
//----------------------------------------------------------------------------
/* Show OSPF information */
mesa_rc FRR_ICLI_ospf_show_info(const FrrOspfCliReq &req);

/* Show OSPF interface information */
mesa_rc FRR_ICLI_ospf_show_interface(const FrrOspfCliReq &req);

/* Show OSPF neighbor information */
mesa_rc FRR_ICLI_ospf_show_neighbor(const FrrOspfCliReq &req);

//----------------------------------------------------------------------------
//** OSPF stub router
//----------------------------------------------------------------------------
/* Set OSPF stub router */
mesa_rc FRR_ICLI_ospf_stub_router_set(const FrrOspfCliReq &req);

/* Disable OSPF stub router mode */
mesa_rc FRR_ICLI_ospf_stub_router_disable(const FrrOspfCliReq &req);

//----------------------------------------------------------------------------
//** OSPF route information
//----------------------------------------------------------------------------
/* Show OSPF routing information */
mesa_rc FRR_ICLI_ospf_show_route(const FrrOspfCliReq &req);

//----------------------------------------------------------------------------
//** OSPF database information
//----------------------------------------------------------------------------
/* Show OSPF database general summary information */
mesa_rc FRR_ICLI_ospf_show_db_general_info(const FrrOspfCliReq &req);

/* Show OSPF database detail information */
mesa_rc FRR_ICLI_ospf_show_db_detail_info(const FrrOspfCliReq &req);

#endif /* _FRR_OSPF_ICLI_FUNCTIONS_HXX_ */

