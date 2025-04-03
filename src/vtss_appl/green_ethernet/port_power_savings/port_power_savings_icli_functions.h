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
 * \brief mirror icli functions
 * \details This header file describes PHY control functions
 */


#ifndef _VTSS_ICLI_PORT_POWER_SAVINGS_H_
#define _VTSS_ICLI_PORT_POWER_SAVINGS_H_

#include "icli_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Function for showing or setting current configuration
 *
 * \param session_id [IN]  The session id.
 * \param show_eee [IN] TRUE if the caller want to show EEE status
 * \param show_perfect_reach [IN] TRUE if the caller want to show perfect_reach status
 * \param show_actiphy [IN] TRUE if the caller want to show actiphy status
 * \param mode [IN] TRUE if the caller is the ICLI enable/disable command
 * \param urgent_queues [IN] TRUE if the caller is the ICLI urgent queues command
 * \param interface [IN] TRUE if only some ports should be shown, else FALSE
 * \param port_list [IN] ICLI port list with the ports to show.
 * \param urgent_queues_list [IN] List of which queues that are urgent.
 * \param no [IN] TRUE is the command is called with "no"
 * \return Error code.
 **/
mesa_rc port_power_savings_common(i32 session_id, BOOL mode, BOOL urgent_queues, BOOL interface, BOOL actiphy, BOOL perfectreach, icli_stack_port_range_t *port_list, icli_range_t *urgent_queues_list, BOOL no, BOOL show_eee, BOOL show_actiphy, BOOL show_perfect_reach);


/**
 * \brief Function for setting the optimize for configuration
 *
 * \param optimize_for_power [IN] TRUE if EEE should be optimize for power, else optimized for latency
 *
 * \return Error code.
 **/
mesa_rc port_power_savings_eee_optimize_for_power(BOOL optimize_for_power);

/**
 * \ Debugging Function for Displaying the EEE Phy Registers
 * \return Error code.
 **/
mesa_rc port_power_savings_debug_eee_status(i32 session_id, BOOL has_phy, BOOL has_raw, BOOL has_status, icli_stack_port_range_t *v_port_type_list);

/**
 * \ Debugging Function for Displaying the EEE Wake up time or setting wake up time
 * \return Error code.
 **/
mesa_rc port_power_savings_debug_eee_wakeup_time(i32 session_id, BOOL has_tx, BOOL has_clear, u16 v_uint16, icli_stack_port_range_t *plist);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_ICLI_PORT_POWER_SAVINGS_H_ */

