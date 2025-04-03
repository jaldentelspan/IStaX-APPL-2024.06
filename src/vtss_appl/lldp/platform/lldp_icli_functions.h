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
 * \file
 * \brief LLDP iCLI functions
 * \details This header file describes LLDP iCLI functions
 */

#ifndef _VTSS_ICLI_LLDP_H_
#define _VTSS_ICLI_LLDP_H_

#include "icli_api.h"

#ifdef __cplusplus
extern "C" {
#endif

VTSS_BEGIN_HDR
/**
 * \brief Function for showing lldp status (chip temperature and port status)
 *
 * \param session_id      [IN] The session id.
 * \param show_neighbors  [IN] TRUE if neighbor information shall be printed
 * \param show_statistics [IN] TRUE if LLDP statistics shall be printed
 * \param show_eee        [IN] TRUE if user has specified to show EEE information
 * \param has_interface   [IN] TRUE if user has specified a specific interface
 * \param port_list       [IN]  Port list in case user has specified a specific interface.
 * \return error code
 **/
mesa_rc lldp_icli_status(i32 session_id, BOOL show_neighbors, BOOL show_statistics, BOOL show_eee, BOOL show_preempt, BOOL has_interface, icli_stack_port_range_t *list);

/**
 * \brief Function for setting lldp global configuration (hold time, interval, delay etc.)
 *
 * \param session_id         [IN] The session id for printing.
 * \param holdtime           [IN] TRUE when setting holdtime value.
 * \param timer              [IN] TRUE when setting the tx interval
 * \param reinit             [IN] TRUE when setting reinit.
 * \param transmission_delay [IN] TRUE when setting the tx delay configuration.
 * \param new_val            [IN] The new value to be set.
 * \param no                 [IN] TRUE if the no command is use
 * \return error code
 **/
mesa_rc lldp_icli_global_conf(i32 session_id, BOOL holdtime, BOOL timer, BOOL reinit, BOOL transmission_delay, u16 new_val, BOOL no);

/**
 * \brief Function for setting priority for ports
 *
 * \param prio_list [IN] Port list
 * \param prio      [IN] Priority to be set for the ports in the port list.
 * \param no        [IN] TRUE if the no command is used
 * \return error code
 **/
mesa_rc lldp_icli_prio(BOOL interface, icli_stack_port_range_t *port_list, u8 prio, BOOL no);


/**
 * \brief Function for enabling/disabling TX and RX mode.
 *
 * \param session_id [IN] The session id for printing.
 * \param port_list  [IN] Port list with ports to configure.
 * \param tx         [IN] TRUE if we shall transmit LLDP frames.
 * \param rx         [IN] TRUE if we shall add LLDP information received from neighbors into the entry table.
 * \param no         [IN] TRUE if the no command is used
 * \return error code
 **/
mesa_rc lldp_icli_mode(i32 session_id, icli_stack_port_range_t *port_list, BOOL tx, BOOL rx, BOOL no);

/**
 * \brief Function for setting when optional TLVs to transmit to the neighbors.
 *
 * \param session_id [IN] The session id for printing.
 * \param plist      [IN] Port list with ports to configure.
 * \param mgmt       [IN] TRUE when management TLV shall be transmitted to neighbors.
 * \param port       [IN] TRUE when port TLV shall be transmitted to neighbors.
 * \param sys_capa   [IN] TRUE when system capabilities TLV shall be transmitted to neighbors.
 * \param sys_des    [IN] TRUE when system description TLV shall be transmitted to neighbors.
 * \param sys_name   [IN] TRUE when system name TLV shall be transmitted to neighbors.
 * \param no         [IN] TRUE if the no command is use
 * \return error_code.
 **/
mesa_rc lldp_icli_tlv_select(i32 session_id, icli_stack_port_range_t *plist, BOOL mgmt, BOOL port, BOOL sys_capa, BOOL sys_des, BOOL sys_name, BOOL no);

/**
 * \brief Function for configuring CDP awareness
 *
 * \param session_id [IN] The session id for printing.
 * \param port_list  [IN] Port list with ports to configure.
 * \param no         [IN] TRUE if the no command is used
 * \return error code
 **/
mesa_rc lldp_icli_cdp(i32 session_id, icli_stack_port_range_t *plist, BOOL no);

/**
 * \brief Function for configuring LLDP SNMP trap
 *
 * \param session_id [IN] The session id for printing.
 * \param port_list  [IN] Port list with ports to configure.
 * \param no         [IN] TRUE if the no command is used
 * \return error code
 **/
mesa_rc lldp_icli_trap(i32 session_id, icli_stack_port_range_t *plist, BOOL no);

/**
 * \brief Function for clearing the LLDP interfacestatistic.
 * \param has_global [IN] Set to TRUE to clear global counters.
 * \param has_global [IN] Set to TRUE to clear interface counters.
 * \param port_list  [IN] Port list with ports to configure.
 * \return error code
 */
mesa_rc lldp_icli_clear_counters(BOOL has_global, BOOL has_interface, icli_stack_port_range_t *list);


mesa_rc lldp_icfg_init(void);
VTSS_END_HDR

//****************************************************************************
#ifdef __cplusplus
}
#endif

#endif /* _VTSS_ICLI_LLDP_H_ */

