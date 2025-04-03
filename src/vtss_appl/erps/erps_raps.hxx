/*
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _ERPS_RAPS_HXX_
#define _ERPS_RAPS_HXX_

#include "erps_base.hxx"

#define ERPS_RAPS_TX_TIMEOUT_MS 5000
#define ERPS_RAPS_RX_TIMEOUT_MS ((35 * ERPS_RAPS_TX_TIMEOUT_MS) / 10)

mesa_rc erps_raps_tx_frame_update(erps_state_t *erps_state, bool do_tx);
void    erps_raps_tx_info_update(erps_state_t *erps_state, vtss_appl_erps_raps_info_t &tx_raps_info);
void    erps_raps_statistics_clear(erps_state_t *erps_state);
bool    erps_raps_rx_frame(erps_state_t *erps_state, vtss_appl_erps_ring_port_t ring_port, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info, vtss_appl_erps_raps_info_t *rx_raps_info);
void    erps_raps_state_init(erps_state_t *erps_state);
void    erps_raps_deactivate(erps_state_t *erps_state);

#endif /* _ERPS_RAPS_HXX_ */

