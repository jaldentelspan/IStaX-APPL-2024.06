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

#ifndef LLDP_H
#define LLDP_H

#include "lldp_sm.h"
#include "vtss/appl/lldp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LLDP_ETHTYPE 0x88CC
typedef int (*lldp_printf_t)(const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));


void lldp_set_port_enabled (lldp_port_t port, lldp_u8_t enabled);
void lldp_something_changed_local (void);
void sw_lldp_init(void);
void sw_lldp_port_init(lldp_port_t port);
void lldp_1sec_timer_tick (lldp_port_t port);
void lldp_frame_received(lldp_port_t port_no, const lldp_u8_t *const frame, lldp_u16_t len, mesa_glag_no_t glag_no);
void lldp_pre_port_disabled (lldp_port_t port_no);
void lldp_set_timing_changed (void);
void lldp_rx_process_frame (lldp_sm_t *sm);
void lldp_bad_lldpdu(lldp_port_t port_no);

void lldp_get_stat_counters (vtss_appl_lldp_port_counters_t *cnt_array) ;
void lldp_clr_stat_counters (vtss_appl_lldp_port_counters_t *cnt_array);

vtss_appl_lldp_counter_t lldp_get_tx_frames (lldp_port_t port);
vtss_appl_lldp_counter_t lldp_get_rx_total_frames (lldp_port_t port);
vtss_appl_lldp_counter_t lldp_get_rx_error_frames (lldp_port_t port);
vtss_appl_lldp_counter_t lldp_get_rx_discarded_frames (lldp_port_t port);
vtss_appl_lldp_counter_t lldp_get_TLVs_discarded (lldp_port_t port);
vtss_appl_lldp_counter_t lldp_get_TLVs_unrecognized (lldp_port_t port);
vtss_appl_lldp_counter_t lldp_get_TLVs_org_discarded (lldp_port_t port);
vtss_appl_lldp_counter_t lldp_get_ageouts (lldp_port_t port);
void received_bad_lldpdu(mesa_port_no_t port_no);
void lldp_admin_state_changed(const vtss_appl_lldp_port_conf_t  *current_conf, const vtss_appl_lldp_port_conf_t *new_conf) ;
void lldp_printf_frame(lldp_8_t *frame, lldp_u16_t len);
void lldp_statistics_clear (lldp_port_t port);

BOOL lldp_is_frame_preemption_supported(void);
#ifdef VTSS_SW_OPTION_POE

// From IEEE 802.3at-2009
// Under normal operation, an LLDPDU containing a Power via MDI TLV with an updated value for the "PSE
// allocated power value" field shall be sent within 10 seconds of receipt of an LLDPDU containing a Power
// via MDI TLV where the "PD requested power value" field is different from the previously communicated
// value.

// For being able to the above requirement we need to have a way to start a new LLDP transmission upon request from the
// PoE module. This is what LldpPoeFastStart is used for.
class LldpPoeFastStart
{
public:
    void start(mesa_port_no_t iport)
    {
        doFastStart[iport] = TRUE;
    }


    BOOL get(mesa_port_no_t iport)
    {
        BOOL returnVal = doFastStart[iport];
        doFastStart[iport] = FALSE;
        return returnVal;
    }

    LldpPoeFastStart(void)
    {
        doFastStart.clear_all();
    }

private:
    mesa_port_list_t doFastStart;
};

// Getting/setting if a LLDP frame should be send due to new PD detected or new LLDP information from the PD.
// IN : port_idx - Internal port number for port in question
//      get      - Set to TRUE in order to get, if LLDP frame should be transmitted. Set to FALSE in order to signal from the PoE module to the LLDP module
//                 to start a new LLDP frame transmission as soon as possible.
// Return        - If get is TRUE then signaling if a LLDP frame should be transmitted. If get is FALSE then return value can be ignored.
BOOL lldp_poe_fast_start(lldp_port_t port_idx, BOOL get);
#endif

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */
#endif



