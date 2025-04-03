/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_MSTP_CALLOUT_H_
#define _VTSS_MSTP_CALLOUT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file vtss_mstp_callout.h
 * \brief MSTP host interface header file
 *
 * This file contain the definitions of functions provided \e by the
 * host system \e to the MSTP/RSTP protocol entity. Thus, a given
 * system must provide implementations of these functions according
 * the specified interface.
 *
 * \author Lars Povlsen <lpovlsen@vitesse.com>
 *
 * \date 15-01-2009
 */

/**
 * Forwarding state - used by vtss_mstp_port_setstate()
 */
typedef enum mstp_fwdstate {
    MSTP_FWDSTATE_BLOCKING,     /*!< Port is blocking */
    MSTP_FWDSTATE_LEARNING,     /*!< Port is learning */
    MSTP_FWDSTATE_FORWARDING,   /*!< Port is learning and forwarding */
} mstp_fwdstate_t;

/**
 * MSTP Trap event flags
 */
typedef enum {
    MSTP_TRAP_NEW_ROOT,
    MSTP_TRAP_TOPOLOGY_CHANGE,
} mstp_trap_event_t;

/**
 * BPPDU transmit.
 *
 * \param portnum The physical port on which to send the BPDU.
 *
 * \param buffer The BPDU to transmit.
 *
 * \param size The length of the BPDU buffer.
 *
 * \note The implementation must fill in the Source MAC address
 * (offset \e 6 in transmit buffer) for the port, and may have to pad
 * the transmit operation to physical link requirements.
 */
void vtss_mstp_tx(uint portnum, void *buffer, size_t size);

/**
 * Switch interface access - set forwarding state.
 *
 * \param portnum The physical port to control
 *
 * \param msti The port instance to control
 *
 * \param state The state to set
 */
void vtss_mstp_port_setstate(uint portnum, u8 msti, mstp_fwdstate_t state);

/**
 * Switch interface access - set forwarding state.
 *
 * \param portnum The physical port
 *
 * \param msti The port instance
 *
 * \param old_role The old port role
 *
 * \param old_role The new port role
 *
 * \note This function can be overridden/implemented by a MVRP module
 * that need to be notified of role transitions. During the execution
 * of this "hook" funtion the MSTP API is *locked*, so care should be
 * taken to not create deadlocks.
 */
void vtss_mstp_port_setrole(uint portnum, u8 msti, vtss_mstp_portrole_t old_role, vtss_mstp_portrole_t new_role);

/**
 * Switch interface access - \e MAC table.
 *
 * \param portnum The physical port to control
 *
 * \param msti The port instance to control
 */
void vtss_mstp_port_flush(uint portnum, u8 msti);

/**
 * VLAN interface access - determine port MSTI membership
 *
 * \param portnum The physical port to query
 *
 * \param msti The MSTI instance to query for membership
 *
 * \return TRUE if the port is a member of the MSTI.
 */
BOOL vtss_mstp_port_member(uint portnum, mstp_msti_t msti);

/**
 * Time interface - get current time
 *
 * \return the current time of day in seconds (relative to an
 * arbitrary absolute time)
 */
u32 vtss_mstp_current_time(void);

/**
 * Trap support - event occurred
 *
 * \param msti The bridge instance of the event
 *
 * \param event The event occurring
 */
void vtss_mstp_trap(u8 msti, mstp_trap_event_t event);

/**
 * Allocate memory.
 *
 * \param sz - number of bytes to allocate.
 *
 * \return Pointer to allocated memory.
 */
void *vtss_mstp_malloc(size_t sz);

/**
 * Free memory previously allocated with vtss_mstp_malloc().
 *
 * \param ptr - pointer to free.
 */
void vtss_mstp_free(void *ptr);

/**
 * Logging support - event occurred
 *
 * \param message - message format
 *
 * \param port - Port number (zero if non-port)
 */
void vtss_mstp_log(const char *message, u32 port);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_MSTP_CALLOUT_H_ */

