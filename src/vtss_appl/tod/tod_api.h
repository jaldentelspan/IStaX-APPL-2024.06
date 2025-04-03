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

#ifndef _TOD_API_H_
#define _TOD_API_H_


#ifdef __cplusplus
extern "C" {
#endif

mesa_rc tod_init(vtss_init_data_t *data);

BOOL tod_tc_mode_get(mesa_packet_internal_tc_mode_t *mode);
BOOL tod_tc_mode_set(mesa_packet_internal_tc_mode_t *mode);

BOOL tod_port_phy_ts_get(BOOL *ts, mesa_port_no_t portnum);
BOOL tod_port_phy_ts_set(BOOL *ts, mesa_port_no_t portnum);

BOOL tod_ref_clock_freg_get(mepa_ts_clock_freq_t *freq);
BOOL tod_ref_clock_freg_set(mepa_ts_clock_freq_t *freq);

BOOL tod_ready(void);

void tod_phy_ts_init();

bool mepa_phy_ts_cap();

bool tod_board_phy_ts_dis_set(BOOL phy_ts_dis);

bool tod_board_phy_ts_dis_get();
#ifdef __cplusplus
}
#endif
#endif // _TOD_API_H_

