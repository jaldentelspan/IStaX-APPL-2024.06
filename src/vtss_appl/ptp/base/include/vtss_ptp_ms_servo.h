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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#if defined(VTSS_SW_OPTION_ZLS30387)

#ifndef _VTSS_PTP_MS_SERVO_H_
#define _VTSS_PTP_MS_SERVO_H_

#include "vtss_ptp_servo.h"
#include "vtss_ptp_slave.h"
#include "vtss_ptp_filters.hxx"

struct ptp_ms_servo : ptp_servo {
    /* data members */
    static int activate_count;
    const vtss_appl_ptp_clock_servo_config_t *servo_conf;
    const vtss_appl_ptp_clock_config_default_ds_t *default_ds_conf;
    static const int filter_const = 16;
    BOOL force_holdover_state = FALSE;
    ptp_ms_servo(int inst, const vtss_appl_ptp_clock_servo_config_t *s, const vtss_appl_ptp_clock_config_default_ds_t *clock_config, int port_count);
    virtual ~ptp_ms_servo();
    virtual int servo_type() { return 2; };
    void displayStats(const mesa_timeinterval_t *meanPathDelay, const mesa_timeinterval_t *offsetFromMaster, i32 observedParentClockPhaseChangeRate, u16 observedParentOffsetScaledLogVariance, i32 adj);
    virtual mesa_rc switch_to_packet_mode(uint32_t domain);
    virtual mesa_rc switch_to_hybrid_mode(uint32_t domain);
    virtual bool mode_is_hybrid_mode(uint32_t domain);
    virtual mesa_rc set_active_ref(uint32_t domain, int stream);
    virtual mesa_rc set_active_electrical_ref(uint32_t domain, int input);
    virtual mesa_rc set_hybrid_transient(uint32_t domain, vtss_ptp_hybrid_transient state);
    virtual mesa_rc force_holdover_set(BOOL enable);
    virtual mesa_rc force_holdover_get(BOOL *enable);
    virtual int delay_filter(vtss_ptp_offset_filter_param_t *delay, i8 logMsgInterval, int port);
    virtual void delay_filter_reset(int port);
    virtual int offset_filter(vtss_ptp_offset_filter_param_t *offset, i8 logMsgInterval);
    virtual void offset_filter_reset();
    virtual void delay_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, mesa_timeinterval_t correction, i8 logMsgIntv);
    virtual int offset_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, mesa_timeinterval_t correction, i8 logMsgIntv, u16 sequenceId, bool peer_delay, bool virtual_port);
    virtual void clock_servo_reset(vtss_ptp_set_vcxo_freq setVCXOfreq);
    virtual void clock_servo_status(uint32_t domain, vtss_ptp_servo_status_t *status);
    virtual bool display_stats();
    virtual mesa_rc activate(uint32_t domain, bool hybrid_init);
    virtual void deactivate(uint32_t domain);
    virtual mesa_rc switch_1pps_virtual_ref(ptp_clock_t *ptp_clk, uint16_t instance, uint32_t domain, bool enable, bool warm_start);
    virtual mesa_rc set_device_1pps_virtual_ref(ptp_clock_t *ptp_clk, uint16_t instance, uint32_t domain, bool enable);
    virtual void process_alternate_timestamp(ptp_clock_t *ptp_clk, uint32_t domain, u16 instance, const mesa_timestamp_t *send_time, const mesa_timestamp_t *recv_time, mesa_timeinterval_t correction, i8 logMsgIntv, bool virtual_port);

private:
    void vtss_1588_pdv_status_get(uint32_t domain, u16 serverId, vtss_slave_clock_state_t *pdv_clock_state, i32 *freq_offset);
    void vtss_1588_process_timestamp(uint32_t domain, u16 serverId, const mesa_timestamp_t *send_time, const mesa_timestamp_t *recv_time, mesa_timeinterval_t correction, i8 logMsgIntv, bool fwd_path, bool peer_delay, bool virtual_port);
    vtss_ptp_filters::vtss_lowpass_filter_t *owd_filt;
    int my_port_count;
};

#endif // _VTSS_PTP_MS_SERVO_H_

#endif  // defined(VTSS_SW_OPTION_ZLS30387)

