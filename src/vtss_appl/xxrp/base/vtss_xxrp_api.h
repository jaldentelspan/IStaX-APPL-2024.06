/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_XXRP_API_H_
#define _VTSS_XXRP_API_H_

#include "vtss_xxrp_map.h"
#include "vtss_xxrp_types.h"
#include "vlan_api.h"
#include "vtss_xxrp_applications.h"
#include "vtss_xxrp_debug.h"
#include "vtss_xxrp_mad.h"

/**
 * \file vtss_xxrp_api.h
 * \brief XXRP main API header file
 *
 * This file contain the definitions of API functions and associated types.
 *
 */

/* ================================================================= *
 *  MRP capabilities struct (dummy needed for capabilities reference)
 * ================================================================= */
typedef struct {
} vtss_appl_mrp_capabilities_t;

/****************************************************************************
 * Capabilities
 ****************************************************************************/

/**
 * \brief Get MRP capabilities.
 *
 * \param cap [OUT]  The MRP capabilities.
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_mrp_capabilities_get(vtss_appl_mrp_capabilities_t *const cap);

/***************************************************************************************************
 * XXRP Module error defintions.
 **************************************************************************************************/
enum {
    XXRP_ERROR_INVALID_PARAMETER = MODULE_ERROR_START(VTSS_MODULE_ID_XXRP),
    XXRP_ERROR_INVALID_APPL,
    XXRP_ERROR_INVALID_L2PORT,
    XXRP_ERROR_INVALID_ATTR,
    XXRP_ERROR_NOT_ENABLED,
    XXRP_ERROR_NOT_ENABLED_PORT,
    XXRP_ERROR_NULL_PTR,
    XXRP_ERROR_ALREADY_CONFIGURED,
    XXRP_ERROR_NO_MEMORY,
    XXRP_ERROR_NOT_SUPPORTED,
    XXRP_ERROR_NO_SUFFIFIENT_MEMORY,
    XXRP_ERROR_NOT_FOUND,
    XXRP_ERROR_UNKNOWN,
    XXRP_ERROR_FRAME,
    XXRP_RC_LAST
};

/* Timer default and valid range definitions */
#define VTSS_MRP_JOIN_TIMER_DEF             20 /* CentiSeconds */
#define VTSS_MRP_JOIN_TIMER_MIN              1 /* CentiSeconds */
#define VTSS_MRP_JOIN_TIMER_MAX             20 /* CentiSeconds */
#define VTSS_MRP_LEAVE_TIMER_DEF            60 /* CentiSeconds */
#define VTSS_MRP_LEAVE_TIMER_MIN            60 /* CentiSeconds */
#define VTSS_MRP_LEAVE_TIMER_MAX           300 /* CentiSeconds */
#define VTSS_MRP_LEAVE_ALL_TIMER_DEF      1000 /* CentiSeconds */
#define VTSS_MRP_LEAVE_ALL_TIMER_MIN      1000 /* CentiSeconds */
#define VTSS_MRP_LEAVE_ALL_TIMER_MAX      5000 /* CentiSeconds */
#define VTSS_MRP_PERIODIC_TIMER_DEF        100 /* CentiSeconds */
#define VTSS_MRP_PERIODIC_TIMER_MIN        100 /* CentiSeconds */
#define VTSS_MRP_PERIODIC_TIMER_MAX        100 /* CentiSeconds */
#define VTSS_MRP_PERIODIC_TIMER_MODE_DEF FALSE /* Default disabled */

#define VTSS_APPL_MVRP_VLAN_ID_MIN 1
#define VTSS_APPL_MVRP_VLAN_ID_MAX 4094

#define MVRP_VLAN_ID_MAX                      4096
#define VTSS_MRP_VLAN_MAX_DEF                 20 /* default max number of VLANs */

#if !defined(XXRP_ATTRIBUTE_PACKED)
#define XXRP_ATTRIBUTE_PACKED __attribute__((packed)) /* GCC defined */
#endif

/***************************************************************************************************
 * XXRP API definitions.
 **************************************************************************************************/
/**
 * \brief function that handles global control event.
 *
 * \param application [IN]  Application type (as defined in vtss_mrp_appl_t)
 * \param enable      [IN]  Set this boolean variable to enable MRP application
 *                          globally and clear this to disable.
 *
 * \return Return error code.
 **/
mesa_rc vtss_mrp_global_control_conf_set(vtss_mrp_appl_t appl, BOOL enable);

/**
 * \brief function to get MRP application global status.
 *
 * \param application [IN]  Application type (as defined in vtss_mrp_appl_t)
 * \param status      [OUT] Pointer to a boolean variable. Its value
 *                          is set to TRUE if MRP application is
 *                          enabled globally, else it is set to FALSE.
 *
 * \return Return error code.
 **/
mesa_rc vtss_mrp_global_control_conf_get(vtss_mrp_appl_t appl, BOOL *const status);

/**
 * \brief function that handles port control event.
 *
 * \param application [IN]  Application type (as defined in vtss_mrp_appl_t)
 * \param port_no     [IN]  L2 port number.
 * \param enable      [IN]  Set this boolean variable to enable MRP applicaiton on this
 *                          port and clear this to disable.
 *
 * \return Return error code.
 **/
mesa_rc vtss_xxrp_port_control_conf_set(vtss_mrp_appl_t appl, u32 l2port, BOOL enable, vtss::VlanList &vls);

/**
 * \brief function to get the current port control status of a MRP application.
 *
 * \param application [IN]  Application type (as defined in vtss_mrp_appl_t)
 * \param port_no     [IN]  L2 port number.
 * \param status      [OUT] Set the value of boolean variable to enable MRP application
 *                          on this port and clear this to disable.
 *
 * \return Return error code.
 **/
mesa_rc vtss_xxrp_port_control_conf_get(vtss_mrp_appl_t appl, u32 l2port, BOOL *const status);

/**
 * \brief function that handles periodic timer control event.
 *
 * \param application [IN]  Application type (as defined in vtss_mrp_appl_t)
 * \param port_no     [IN]  L2 port number.
 * \param enable      [IN]  Set this boolean variable to enable periodic transmission
 *                          on this port and clear this to disable.
 *
 * \return Return error code.
 **/
mesa_rc vtss_xxrp_periodic_transmission_control_conf_set(vtss_mrp_appl_t application,
                                                         u32             port_no,
                                                         BOOL            enable);

/**
 * \brief function to configure MRP application timers.
 *
 * \param application [IN]  Application type (as defined in vtss_mrp_appl_t)
 * \param port_no     [IN]  L2 port number.
 * \param timers      [IN]  Timer values.
 *
 * \return Return error code.
 **/
mesa_rc vtss_xxrp_timers_conf_set(vtss_mrp_appl_t appl, u32 port, vtss_mrp_timer_conf_t *const timers);

/**
 * \brief function to handle applicant admin control event.
 *
 * \param application [IN]  Application type (as defined in vtss_mrp_appl_t)
 * \param port_no     [IN]  L2 port number.
 * \param attr_type   [IN]  Attribute type as defined in vtss_mrp_attribute_type_t enumeration.
 * \param participant [IN]  Set this boolean variable to TRUE to make attr_type a normal participant,
 *                          else set to FALSE.
 *
 * \return Return error code.
 **/
mesa_rc vtss_xxrp_applicant_admin_control_conf_set(vtss_mrp_appl_t appl, u32 l2port,
                                                   vtss_mrp_attribute_type_t attr_type,
                                                   BOOL                      participant);

/**
 * \brief MSTP port state change handler.
 *
 * \param port_no            [IN]  L2 port number.
 * \param msti               [IN]  MSTP instance
 * \param port_state_type    [IN]  port state type
 *
 * \return Return error code.
 **/
mesa_rc vtss_xxrp_mstp_port_state_change_handler(u32 l2port, u8 msti,
                                                 vtss_mrp_mstp_port_state_change_type_t  port_state_type);

/**
 * \brief MSTP port role change handler.
 *
 * \param port_no            [IN]  L2 port number.
 * \param msti               [IN]  MSTP instance
 * \param port_role_type     [IN]  port role type
 *
 * \return Return error code.
 **/
u32 vtss_mrp_mstp_port_role_change_handler(u32                                      port_no,
                                           u8                                       msti,
                                           vtss_mrp_mstp_port_role_change_type_t    port_role_type);

#ifdef VTSS_SW_OPTION_MVRP
/**
 * \brief VLAN add/delete event handler.
 *
 * \param vid                [IN]  VLAN ID.
 * \param port_no            [IN]  L2 port number.
 * \param is_add             [IN]  Boolean = TRUE for VLAN add;
 *                                         = FALSE for VLAN delete
 * \return Return error code.
 **/
mesa_rc vtss_mrp_vlan_change_handler(u32 l2port, mesa_vid_t vid, vlan_registration_type_t t);
#endif /* VTSS_SW_OPTION_MVRP */

/**
 * \brief MRP application statistics get function.
 *
 * \param application [IN]  Application type (as defined in vtss_mrp_appl_t)
 * \param port_no     [IN]  L2 port number.
 * \param stats       [IN]  Pointer to statistics structure;
 *
 * \return Return error code.
 **/
mesa_rc vtss_mrp_stats_get(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_stats_t *const stats);

/**
 * \brief MRP application statistics clear funtion.
 *
 * \param application [IN]  Application type (as defined in vtss_mrp_appl_t)
 * \param port_no     [IN]  L2 port number.
 *
 * \return Return error code.
 **/
mesa_rc vtss_mrp_stats_clear(vtss_mrp_appl_t appl, u32 l2port);

/**
 * \brief MRP timer tick handler.
 *
 * \param delay [IN] number of cs(ticks) elapsed since this handler was called last time.
 *
 * \return Return min_time.
 **/

u32 vtss_xxrp_timer_tick(u32 delay);

/**
 * \brief MRP MRPDU receive handler.
 *
 * \param port_no [IN]  L2 port number.
 * \param mrpdu   [IN]  Pointer to MRPDU.
 * \param length  [IN]  Length of MRPDU.
 *
 * \return Return TRUE if MRPDU is approved and handled, FALSE otherwise.
 **/

BOOL vtss_mrp_mrpdu_rx(u32 l2port, const u8 *mrpdu, u32 length);
void vtss_mrp_init(void);
mesa_rc vtss_mrp_port_ring_print(vtss_mrp_appl_t appl, u8 msti);
void xxrp_packet_dump(u32 port_no, const u8 *packet, BOOL packet_transmit);
mesa_rc vtss_mrp_port_mad_print(u32 l2port, u32 machine_index);
mesa_rc vtss_mrp_map_get(vtss_mrp_appl_t appl, vtss_mrp_map_t ***map_ports);
mesa_rc vtss_mrp_reg_admin_status_set(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index, u8 t);
mesa_rc vtss_mrp_reg_admin_status_get(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index, u8 *t);
#endif /* _VTSS_XXRP_API_H_ */
