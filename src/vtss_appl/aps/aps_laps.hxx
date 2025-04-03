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

#ifndef _APS_LAPS_HXX_
#define _APS_LAPS_HXX_

#include "aps_base.hxx"

mesa_rc aps_laps_tx_frame_update(aps_state_t *aps_state, bool do_tx);
void    aps_laps_tx_info_update(aps_state_t *aps_state, bool shutdown);
void    aps_laps_statistics_clear(aps_state_t *aps_state);
bool    aps_laps_rx_frame(aps_state_t *aps_state, const uint8_t *const frm, const mesa_packet_rx_info_t *const rx_info, uint8_t new_aps_rx_info[APS_LAPS_DATA_LENGTH]);
void    aps_laps_sf_sd_set(aps_state_t *aps_state, bool working, bool new_sf, bool new_sd);
void    aps_laps_state_init(aps_state_t *aps_state);

#endif /* _APS_LAPS_HXX_ */

