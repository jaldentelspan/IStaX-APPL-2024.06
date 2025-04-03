/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_XXRP_CALLOUT_H_
#define _VTSS_XXRP_CALLOUT_H_

#include "vtss_xxrp_types.h"
#include "vlan_api.h"

/**
 * \file vtss_xxrp_callout.h
 * \brief XXRP host interface header file
 *
 * This file contain the definitions of functions provided by the host
 * system to the XXRP protocol entity. Thus, a given system must provide
 * implementations of these functions according the specified interface.
 *
 */
void *xxrp_sys_malloc(u32 size, const char *file, const char *function, u32 line);

mesa_rc xxrp_sys_free(void *ptr, const char *file, const char *function, u32 line);

#define XXRP_SYS_MALLOC(size) xxrp_sys_malloc(size, __FILE__, __FUNCTION__, __LINE__ );
#define XXRP_SYS_FREE(ptr)    xxrp_sys_free(ptr, __FILE__, __FUNCTION__, __LINE__);

/**
 * \brief MRP timer kick.
 *
 * This function is called by the base part in order to start the MRP timer.
 * See the description of vtss_mrp_timer_tick() for more information.
 *
 * \return Nothing.
 */
void vtss_mrp_timer_kick(void);

/**
 * \brief MRP crit enter.
 *
 * This function is called when the base part wants to enter the critical section.
 *
 * \return Nothing.
 */
void vtss_mrp_crit_enter(void);

/**
 * \brief MRP crit exit.
 *
 * This function is called when the base part wants to exit the critical section.
 *
 * \return Nothing.
 */
void vtss_mrp_crit_exit(void);

/**
 * \brief MRP crit assert locked.
 *
 * This function is called when the base part wants to verify that the critical section has been entered.
 *
 * \return Nothing.
 */
void vtss_mrp_crit_assert(void);

/**
 *  \brief Get MAD port status.
 *
 * This function is called when the base part wants the port msti status.
 *
 */
mesa_rc mrp_mstp_port_status_get(u8 msti, u32 l2port);

mesa_rc mrp_mstp_index_msti_mapping_get(u32 attr_index, u8 *msti);

/**
 * \brief MRP MRPDU tx allocate function.
 *
 * Allocates a transmit buffer.
 *
 * \param port_no [IN]  L2 port number.
 * \param length  [IN]  Length of MRPDU.
 * \param context [OUT] Pointer to transmit context.
 *
 * \return Return pointer to allocated MRPDU buffer or NULL.
 **/
void *vtss_mrp_mrpdu_tx_alloc(u32 port_no, u32 length);

/**
 * \brief MRP MRPDU tx function.
 *
 * Inserts an SMAC address based on the port_no and transmits the MRPDU.
 * The allocated buffer is automatically free'ed.
 *
 * \param port_no [IN]  L2 port number.
 * \param mrpdu   [IN]  Pointer to MRPDU.
 * \param length  [IN]  Length of MRPDU.
 *
 * \return Return FASLE if MRPDU is transmitted successfully, TRUE otherwise.
 **/
mesa_rc vtss_mrp_mrpdu_tx(u32 port_no, void *mrpdu, u32 length);

#ifdef VTSS_SW_OPTION_MVRP
mesa_rc XXRP_mvrp_vlan_port_membership_add(u32 port_no, mesa_vid_t vid);
mesa_rc XXRP_mvrp_vlan_port_membership_del(u32 port_no, mesa_vid_t vid);
u32 XXRP_mvrp_mac_flush_filtering_db(u32 port_no, mesa_vid_t vid);
BOOL XXRP_mvrp_is_vlan_registered(u32 l2port, mesa_vid_t vid);
mesa_rc vtss_mrp_port_mad_get(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_mad_t **mad);
#endif /* VTSS_SW_OPTION_MVRP */

#ifdef VTSS_SW_OPTION_GVRP
//u32 XXRP_gvrp_vlan_port_membership_add(u32 port_no, mesa_vid_t vid);
//u32 XXRP_gvrp_vlan_port_membership_del(u32 port_no, mesa_vid_t vid);
void XXRP_gvrp_vlan_port_membership_change(void);
#endif

mesa_rc xxrp_mgmt_vlan_state(u32 l2port, vlan_registration_type_t *array /* VTSS_APPL_VLAN_ID_MAX + 1 entries */);
BOOL XXRP_is_port_point2point(u32 l2port);

#endif /* _VTSS_XXRP_CALLOUT_H_ */
