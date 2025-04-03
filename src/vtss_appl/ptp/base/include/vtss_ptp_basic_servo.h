/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted but only in
 connection with products utilizing the Microsemi switch and PHY products.
 Permission is also granted for you to integrate into other products, disclose,
 transmit and distribute the software only in an absolute machine readable format
 (e.g. HEX file) and only in or with products utilizing the Microsemi switch and
 PHY products.  The source code of the software may not be disclosed, transmitted
 or distributed without the prior written permission of Microsemi.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software.  Microsemi retains all ownership,
 copyright, trade secret and proprietary rights in the software and its source code,
 including all modifications thereto.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL WARRANTIES
 OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES ARE EXPRESS,
 IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION, WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND NON-INFRINGEMENT.
*/

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifdef SW_OPTION_BASIC_PTP_SERVO

#ifndef _VTSS_PTP_BASIC_SERVO_H_
#define _VTSS_PTP_BASIC_SERVO_H_

#include "vtss_ptp_servo.h"

typedef struct ptp_slave_s ptp_slave_t;  // Note: This is a forward declaration to solve a circular dependency.

struct ptp_basic_servo : ptp_servo {
    /* data members */
    vtss_appl_ptp_clock_filter_config_t *filt_conf;
    const vtss_appl_ptp_clock_servo_config_t *servo_conf;

    //i64 stable_offset_threshold = 100LL<<16;
    //u32 stable_cnt_threshold = 10;
    //u32 unstable_cnt_threshold = 20;
    u32 stable_cnt;
    u32 unstable_cnt;
    bool stable_offset;
    //i64 offset_ok_threshold = 200LL<<16;
    //i64 offset_fail_threshold = 1000LL<16;
    bool _offset_ok = false;
    vtss_timeinterval_t act_min_delay;
    vtss_lowpass_filter_s *owd_filt;
    vtss_wl_delay_filter_t *owd_wl_filt;

    vtss_lowpass_filter_s offset_lp_filt;
    vtss_timeinterval_t act_min_offset = VTSS_MAX_TIMEINTERVAL;
    vtss_timeinterval_t act_max_offset;
    vtss_timeinterval_t act_mean_offset;
    mesa_timestamp_t act_min_ts;
    u32 actual_period = 0;
    u32 actual_dist;
    i64 prev_offset;
    i64 freq_off_filtered = 0;
    i64 integral;
    bool prev_offset_valid;
    mesa_timestamp_t prev_ts;
    bool prev_ts_valid;
    i64 prop;
    i64 diff;
    i64 delta_t;
    i64 adj_avefix;               /* 'fixed' point (integer with 5 decimals) filtered average adjustment used to avoid rounding errors */
    i64 adj_average = 0LL;        /* filtered average adjustment value to be used as holdover frequency */
    i64 adj_old;
    bool adj_stable = false;      /* true indicates that the adjustment is stable */
    //bool holdover_ok = false;     /* true indicates that the holdover stability period has expired */
    i32 holdover_count = 0;       /* counter to count the stability period */
    bool phase_lock = false;
    bool prc_locked_state = false;
    int my_port_count;
    int active_ref = -1;
    struct ptp_basic_servo *vp_servo = NULL; // virtual port servo.
    struct ptp_basic_servo *ep_servo = NULL; // ethernet port servo.
    bool is_virt_port = false;               // true for virtual port servo.
    bool softClock = false;

    /* member functions */
    ptp_basic_servo(int inst, vtss_appl_ptp_clock_filter_config_t *of, const vtss_appl_ptp_clock_servo_config_t *s, int port_count);
    virtual ~ptp_basic_servo();
    virtual int servo_type() { return 0; };
    void displayStats(const vtss_timeinterval_t *meanPathDelay, const vtss_timeinterval_t *offsetFromMaster, i32 observedParentClockPhaseChangeRate, u16 observedParentOffsetScaledLogVariance, i32 adj);
    virtual mesa_rc switch_to_packet_mode(uint32_t domain) { const_cast<vtss_appl_ptp_clock_servo_config_t *>(servo_conf)->srv_option = VTSS_APPL_PTP_CLOCK_FREE; return VTSS_RC_OK; }
    virtual mesa_rc switch_to_hybrid_mode(uint32_t domain) { const_cast<vtss_appl_ptp_clock_servo_config_t *>(servo_conf)->srv_option = VTSS_APPL_PTP_CLOCK_SYNCE; return VTSS_RC_OK; }
    virtual bool mode_is_hybrid_mode(uint32_t domain) { return servo_conf->srv_option == VTSS_APPL_PTP_CLOCK_SYNCE; }
    virtual mesa_rc set_active_ref(uint32_t domain, int stream) { active_ref = stream; return VTSS_RC_OK; }
    virtual mesa_rc set_active_electrical_ref(uint32_t domain, int input) { active_ref = -1; return VTSS_RC_OK; }
    virtual mesa_rc set_hybrid_transient(uint32_t domain, vtss_ptp_hybrid_transient state) { return VTSS_RC_OK; }
    virtual mesa_rc force_holdover_set(BOOL enable);
    virtual mesa_rc force_holdover_get(BOOL *enable);
    virtual int delay_filter(vtss_ptp_offset_filter_param_t *delay, i8 logMsgInterval, int port);
    virtual void delay_filter_reset(int port);
    virtual int offset_filter(vtss_ptp_offset_filter_param_t *offset, i8 logMsgInterval);
    virtual void offset_filter_reset();
    virtual void delay_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, vtss_timeinterval_t correction, i8 logMsgIntv);
    virtual int offset_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, vtss_timeinterval_t correction, i8 logMsgIntv, u16 sequenceId, bool peer_delay, bool virtual_port);
    i32 clock_servo(vtss_ptp_offset_filter_param_t *offset, i32 *observedParentClockPhaseChangeRate, int localClockId, int phase_lock);
    virtual void clock_servo_reset(vtss_ptp_set_vcxo_freq setVCXOfreq);
    virtual void clock_servo_status(uint32_t domain, vtss_ptp_servo_status_t *status);
    virtual bool display_stats();
    virtual int display_parm(char* buf);
    virtual mesa_rc activate(uint32_t domain, bool hybrid_init);
    virtual void deactivate(uint32_t domain);
    virtual mesa_rc switch_1pps_virtual_ref(ptp_clock_t *ptp_clk, uint16_t instance, uint32_t domain, bool enable, bool warm_start);
    virtual mesa_rc set_device_1pps_virtual_ref(ptp_clock_t *ptp_clk, uint16_t instance, uint32_t domain, bool enable);
    virtual void process_alternate_timestamp(ptp_clock_t *ptp_clk, uint32_t domain, u16 instance, const mesa_timestamp_t *send_time, const mesa_timestamp_t *recv_time, mesa_timeinterval_t correction, i8 logMsgIntv, bool virtual_port);
    virtual void seedFreqSet(ptp_clock_t *ptpClock, mesa_timestamp_t *txTs, mesa_timestamp_t *rxTs, mesa_timeinterval_t corr, double freq, ptp_follow_up_tlv_info_t *followUpInfo, vtss_appl_ptp_802_1as_current_ds_t *current1AsDs);
    bool stable_offset_calc(ptp_slave_t *slave, vtss_timeinterval_t off);
    void stable_offset_clear();
    bool offset_ok(ptp_slave_t *slave);
};

#endif // _VTSS_PTP_BASIC_SERVO_H_

#endif // SW_OPTION_BASIC_PTP_SERVO

