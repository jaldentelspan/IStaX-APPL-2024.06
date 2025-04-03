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

#ifndef VTSS_SYNCE_BOARD_H
#define VTSS_SYNCE_BOARD_H

#include "synce_types.h"
#include "synce_constants.h"

#if defined(VTSS_SW_OPTION_PTP)
#include "ptp_constants.h"
#else
#define PTP_CLOCK_INSTANCES 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

mesa_rc synce_board_init();

// returns what recovered clock output the PHY must use, for the "board" to
// accommodate the request (source, port).
int synce_get_phy_recovered_clock(int source, int port);
int synce_get_switch_recovered_clock(int source, int port);

// need to ask Arne
// (should) return the input number on the DPLL for a given source,port.
int synce_get_selector_ref_no(int source, int port);

// return the frequency provided by the PHY/Switch, and which eventually is
// connected as an input to the DPLL.
meba_synce_clock_frequency_t synce_get_rcvrd_clock_frequency(int source, int port, mesa_port_speed_t port_speed);

// return the frequency expected by a DPLL input for a given combination
// of source and port
meba_synce_clock_frequency_t synce_get_dpll_input_frequency(int source, int port, mesa_port_speed_t port_speed);

// return the frequency expected at the output of the PHY for a given combination
// of source and port
meba_synce_clock_frequency_t synce_get_phy_output_frequency(int source, int port, mesa_port_speed_t port_speed);

// need to ask Arne
// Ask the board API if 'port' can be connected/nominated to a given clock
// 'source'
bool synce_get_source_port(int source, int port);

// Tell the board API what to answer... Seems wrong...
// Either the board API detect what modules the board has been equipped with, or
// the information should be provided as an argument when initializing the board
// API.
void synce_set_source_port(int source, int port, int val);

// Ask the board API how "the" mux must be configured.
u32 synce_get_mux_selector(int source, int port);

// Ask the board API how "the" mux must be configured (keep attributes stored in most significant bits)
u32 synce_get_mux_selector_w_attr(int source, int port);

// need to ask Arne
mesa_synce_divider_t synce_get_switch_clock_divider(int source, int port, mesa_port_speed_t port_speed);

// Configure a "mux" to choose a given input (where mux is actually refering to
// source)
void synce_mux_set(u32 mux, int port);

#ifdef __cplusplus
}
#endif

#endif // VTSS_SYNCE_BOARD_H
