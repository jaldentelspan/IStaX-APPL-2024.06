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

#define SW_OPTION_BASIC_PTP_SERVO // Basic Servo needs to be encluded for the purpose of calibration

#ifdef SW_OPTION_BASIC_PTP_SERVO

#include "vtss_ptp_basic_servo.h"
#include "vtss_ptp_os.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_ptp_api.h"
#include "vtss_tod_api.h"
#include "vtss_ptp_slave.h"
#include "vtss_ptp_clock.h"
#include "misc_api.h"
#include "ptp_api.h"
#include "tod_api.h"

#if defined(VTSS_SW_OPTION_SYNCE)
#include "synce_ptp_if.h"
#include "synce_api.h"
#endif

#define LP_FILTER_0_1_HZ_CONSTANT 3

#define PPM_TO_PPT 1000000 // ppm in terms of parts per trillion

/* This function is used to update sub nano second part of time interval into the correction field. */
static void updateSubNanoToCorr(mesa_timestamp_t send, mesa_timestamp_t recv, mesa_timeinterval_t * const corr)
{
    *corr -= (recv.nanosecondsfrac - send.nanosecondsfrac);
}
vtss_lowpass_filter_s::vtss_lowpass_filter_s() : my_name("anonym")
{
    T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Filter %s ctor", my_name);
    reset();
}
vtss_lowpass_filter_s::vtss_lowpass_filter_s(const char * name) : my_name(name)
{
    T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Filter %s ctor", my_name);
    reset();
}


void vtss_lowpass_filter_s::reset(void)
{
    nsec_prev = VTSS_MAX_TIMEINTERVAL;
    y = 0;
    s_exp = 0;
    skipped = 0;
    T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Filter %s reset", my_name);
}

void vtss_lowpass_filter_s::filter(vtss_timeinterval_t *value, double period, bool min_delay_option, bool customBW)
{
    /* avoid overflowing filter: not needed after value is changed to 64 bit. */
    /* crank down filter cutoff by increasing 's_exp' */
    if (s_exp < 1)
        s_exp = 1;
    else if (s_exp < period)
        ++s_exp;
    else if (s_exp > period)
        s_exp = period;

    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "delay before filtering %d", VTSS_INTERVAL_NS(*value));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "OWD filt: nsec_prev %d, s_exp " VPRI64d ", y %d", VTSS_INTERVAL_NS(nsec_prev), s_exp, VTSS_INTERVAL_NS(y));

    /* filter 'one_way_delay' */
    /* nsec_prev is used as min delay */
    if (nsec_prev > *value) {
        nsec_prev = *value;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "new min delay %d", VTSS_INTERVAL_NS(*value));
    }
    /* optionally ignore delays > 3 * min delay */
    if (*value < nsec_prev + max_acceptable_delay_variation || !min_delay_option) {
        /* low pass filter measured delay */
        if (customBW) {
            y += period*(*value-y);
            //y += (1-period)*(*value-y);
        } else {
            y = ((s_exp-1)*y + *value)/s_exp;
        }
        T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Filtered delay " VPRI64d ", new delay " VPRI64d ", divisor " VPRI64d ", custom BW %d", y, *value, s_exp, customBW);
        skipped = 0;
    } else {  // if too many delay measurements have been skipped, then the min_delay may be too small
        if (++skipped > 5) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "too many delays skipped, old min %d, new min %d", VTSS_INTERVAL_NS(nsec_prev), VTSS_INTERVAL_NS(*value));
            nsec_prev = *value;
            s_exp = 0;
            y = *value;
        }
    }
    *value = y;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Filter %s: delay after filtering %d", my_name, VTSS_INTERVAL_NS(*value));

}


// FIXME: The definition of displayStats is equal to the one for ptp_advanced_servo. A common definition should be made in the base class.
// 
void ptp_basic_servo::displayStats(const vtss_timeinterval_t *meanPathDelay, const vtss_timeinterval_t *offsetFromMaster,
                                   i32 observedParentClockPhaseChangeRate, u16 observedParentOffsetScaledLogVariance, i32 adj)
{
    const int SCREEN_BUFSZ = 180;
    static int start = 1;
    static char sbuf[SCREEN_BUFSZ];
    unsigned int len = 0;
    char str1 [40];
    char str2 [40];

    if (display_stats()) {
        if (start) {
            start = 0;
            T_D("one way delay, offset from mst   adj  servo parameters ->");
            (void)fflush(stdout);
        }
        memset(sbuf, ' ', SCREEN_BUFSZ);
        len += sprintf(sbuf + len,
                       "%s, %s,",
                       vtss_tod_TimeInterval_To_String(meanPathDelay, str1,0),
                       vtss_tod_TimeInterval_To_String(offsetFromMaster, str2,0));

        len += sprintf(sbuf + len,
                       " %7d,", adj);

        len += display_parm(sbuf + len);

        //len += sprintf(sbuf + len,
        //               ", %d" ", %d",
        //               observedParentClockPhaseChangeRate,
        //               observedParentOffsetScaledLogVariance);

        len += sprintf(sbuf + len,"\n");
        printf("%s", sbuf);
    }
}


mesa_rc ptp_basic_servo::force_holdover_set(BOOL enable)
{
    T_I("force_holdover_set not implemented in basic_servo");
    return VTSS_RC_OK;
}

mesa_rc ptp_basic_servo::force_holdover_get(BOOL *enable)
{
    T_I("force_holdover_get not implemented in basic_servo");
    return VTSS_RC_OK;
}

/**
 * \brief Filter reset function.
 */
void ptp_basic_servo::delay_filter_reset(int port)
{
    // delay filter is not executed for virtual port. It is needed only for ptp port.
    if (is_virt_port) {// if virtual port is active, use ptp ethernet port.
        ep_servo->delay_filter_reset(port);
        return;
    }

    VTSS_ASSERT(port < my_port_count);
    if (filt_conf->delay_filter == 0) {
        /* reset 'wl' type delay filter */
        if (filt_conf->dist > 1) {
            owd_wl_filt[port].act_min_delay = VTSS_MAX_TIMEINTERVAL;
        } else {
            owd_wl_filt[port].act_min_delay = 0;
        }
        owd_wl_filt[port].actual_period = 0;
        owd_wl_filt[port].prev_delay = 0;
        owd_wl_filt[port].prev_cnt = 0;
    }
    else {
        /* reset normal type delay filter */
        owd_filt[port].reset();;
    }
}

void ptp_basic_servo::delay_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, vtss_timeinterval_t correction, i8 logMsgIntv)
{
    ptp_slave_t *const slave = &ptpClock[clock_inst]->ssm;
    char str[50];
    vtss_timeinterval_t t3mt2;
    vtss_timeinterval_t t4mt1;
    T_RG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"ptp_delay_calc");

    /* calc 'slave_to_master_delay' */
    vtss_tod_sub_TimeInterval(&slave->slave_to_master_delay, &recv_time, &send_time);
    slave->slave_to_master_delay -= correction;
    if(slave->statistics.enable) {
        slave_to_master_delay_stati(slave);
    }

    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"correction %s", vtss_tod_TimeInterval_To_String (&correction, str,0));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"slave_to_master_delay %s", vtss_tod_TimeInterval_To_String (&slave->slave_to_master_delay, str,0));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"master_to_slave_delay %s", vtss_tod_TimeInterval_To_String (&slave->master_to_slave_delay, str,0));


    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock %p, clock_state %s", slave->clock, clock_state_to_string (slave->clock_state));
    if ((slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED || slave->clock_state == VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED || slave->clock_state == VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING ||
         slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING || slave->clock_state == VTSS_PTP_SLAVE_CLOCK_RECOVERING) && slave->master_to_slave_delay_valid)
    {
        slave->clock->currentDS.meanPathDelay = (slave->master_to_slave_delay + slave->slave_to_master_delay)/2;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"meanPathDelay: %s", vtss_tod_TimeInterval_To_String (&slave->clock->currentDS.meanPathDelay, str,0));
        vtss_tod_sub_TimeInterval(&t3mt2, &send_time, &slave->sync_receive_time);
        vtss_tod_sub_TimeInterval(&t4mt1, &recv_time, &slave->sync_tx_time);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"B,%05u,%010d,%09d,%010d,%09d,%c" VPRI64Fd("010") "\n",slave->last_delay_req_event_sequence_number, recv_time.seconds, recv_time.nanoseconds, send_time.seconds, send_time.nanoseconds,
               correction >= 0 ? '+' : '-', correction>>16);
        if ((slave->clock->currentDS.meanPathDelay < 0) || (t3mt2 > (vtss_timeinterval_t)10000000*(1LL<<16)) || (t4mt1 > (vtss_timeinterval_t)10000000*(1LL<<16)))
        {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"t1: %s", TimeStampToString (&slave->sync_tx_time, str));
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"t2: %s", TimeStampToString (&slave->sync_receive_time, str));
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"t3: %s", TimeStampToString (&send_time, str));
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"t4: %s", TimeStampToString (&recv_time, str));
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"slave_to_master_delay %s", vtss_tod_TimeInterval_To_String (&slave->slave_to_master_delay, str,0));
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"master_to_slave_delay %s", vtss_tod_TimeInterval_To_String (&slave->master_to_slave_delay, str,0));
        }
        if (slave->debugMode == 3 || slave->debugMode == 4 || slave->debugMode == 7 || slave->debugMode == 8) {
            if (!slave->activeDebug) {
                slave->activeDebug = true;
                debug_log_header_2_print(slave->logFile);
                vtss_local_clock_ratio_clear(slave->localClockId);
                T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"enter debug mode, keep_control %d", slave->keep_control);
            }
            char dir = 'B';
            char subn_str[12];
            mesa_timeinterval_t ms_corr = correction;
            subn_str[0] = '\0';
            updateSubNanoToCorr(send_time, recv_time, &ms_corr);
            if (slave->debugMode == 7 || slave->debugMode == 8) {
                dir = 'Y';
                if (ms_corr & 0xFFFF) {
                    sprintf(subn_str, "%c%03d",'.', VTSS_INTERVAL_PICO(ms_corr));
                }
            }
            fprintf(slave->logFile, "%c,%05u,%010d,%09d,%010d,%09d,%c" VPRI64Fd("010") "%s\n", dir, slave->last_delay_req_event_sequence_number, recv_time.seconds, recv_time.nanoseconds, send_time.seconds, send_time.nanoseconds,
                                                                                            ms_corr >= 0 ? '+' : '-', ms_corr>>16, subn_str);
        }
        // Setup data for this sync latency calculation
        vtss_ptp_offset_filter_param_t delay;
        delay.offsetFromMaster = slave->clock->currentDS.meanPathDelay;
        delay.rcvTime = {0, 0};  // delay.rcvTime is only used by the advanced filter i.e. it is not used by basic filter and can be set to 0 (or actually not even be set at all).

        if (0 == delay_filter(&delay, logMsgIntv, 0)) {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Delay too big for filtering");
            slave->clock->currentDS.delayOk = false;
        } else {
            slave->clock->currentDS.delayOk = true;
            slave->clock->currentDS.meanPathDelay = delay.offsetFromMaster;
        }
    } else {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Not ready to delay measurements");
        slave->clock->currentDS.delayOk = false;
        slave->clock->currentDS.meanPathDelay = 0LL;
        delay_filter_reset(0);  /* clears one-way E2E delay filter */
    }
}

/**
 * \brief Filter execution function.
 */
int ptp_basic_servo::delay_filter(vtss_ptp_offset_filter_param_t *delay, i8 logMsgInterval, int port)
{
    // delay filter is not executed for virtual port. It is needed only for ptp port.
    if (is_virt_port) {// if virtual port is active, use ptp ethernet port.
        return ep_servo->delay_filter(delay, logMsgInterval, port);
    }

    VTSS_ASSERT(port < my_port_count);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "delay_filter (port %d)", port);

    if (filt_conf->delay_filter == 0) {
        /* 'wl' type delay filter */
        if (VTSS_INTERVAL_SEC(delay->offsetFromMaster)) {
            /* cannot filter with secs, clear filter */
            delay_filter_reset(port);
            return 0;
        }
        if (owd_wl_filt->actual_period == 0) {
            owd_wl_filt->act_min_delay = VTSS_MAX_TIMEINTERVAL;
            owd_wl_filt->act_max_delay = -VTSS_MAX_TIMEINTERVAL;
            owd_wl_filt->act_mean_delay = 0;
        }
    
        if (owd_wl_filt->act_min_delay > delay->offsetFromMaster) {
            owd_wl_filt->act_min_delay = delay->offsetFromMaster;
        }
        if (owd_wl_filt->act_max_delay < delay->offsetFromMaster) {
            owd_wl_filt->act_max_delay = delay->offsetFromMaster;
        }
        owd_wl_filt->act_mean_delay +=delay->offsetFromMaster;
        if (owd_wl_filt->prev_cnt == 0) {
            owd_wl_filt->prev_delay = delay->offsetFromMaster;
        }
        if (++owd_wl_filt->prev_cnt > filt_conf->dist) {
            owd_wl_filt->prev_cnt = filt_conf->dist;
        }
        if (filt_conf->dist > 1) {
            /* min delay algorithm */
            if (++owd_wl_filt->actual_period >= filt_conf->period) {
                owd_wl_filt->act_mean_delay = owd_wl_filt->act_mean_delay / filt_conf->period;
                //delay->offsetFromMaster = (owd_wl_filt->prev_delay*(owd_wl_filt->prev_cnt-1) + owd_wl_filt->act_min_delay)/(owd_wl_filt->prev_cnt);
                delay->offsetFromMaster = owd_wl_filt->act_min_delay;
                owd_wl_filt->prev_delay = delay->offsetFromMaster;
                owd_wl_filt->actual_period = 0;
                T_I("delayfilter, min %d ns, max %d ns, delay %d ns, prev_cnt %d",
                    VTSS_INTERVAL_NS(owd_wl_filt->act_min_delay), VTSS_INTERVAL_NS(owd_wl_filt->act_max_delay),
                    VTSS_INTERVAL_NS(delay->offsetFromMaster), owd_wl_filt->prev_cnt);
                return 1;
            }
        } else {
            /* mean delay algorithm */
            if (++owd_wl_filt->actual_period >= filt_conf->period) {
                owd_wl_filt->act_mean_delay = owd_wl_filt->act_mean_delay / filt_conf->period;
                delay->offsetFromMaster = owd_wl_filt->act_mean_delay;
                owd_wl_filt->actual_period = 0;
                T_I("delayfilter, min %d ns, max %d ns, delay %d ns",
                    VTSS_INTERVAL_NS(owd_wl_filt->act_min_delay), VTSS_INTERVAL_NS(owd_wl_filt->act_max_delay),
                    VTSS_INTERVAL_NS(delay->offsetFromMaster));
                return 1;
            }
        }
        delay->offsetFromMaster = owd_wl_filt->prev_delay;
        return 1;
    } else if (filt_conf->dist != 0) {
        /* 'normal' low pass type delay filter */
        owd_filt[port].filter(&delay->offsetFromMaster, 1<<filt_conf->delay_filter, true, false);
        return 1;
    } else {
        /* lowpass offset algorithm */
        double freq_bw;
        double smoothFactor;
        freq_bw = (double)filt_conf->period/100; // In order to use period to select freq_bw divide by 100 to enable 0.01 hertz resolution on bandwidths
        if (logMsgInterval <= 0) {
            // Calculate the smoothing factor using the sample frequency and filter bandwidth
            smoothFactor = (1<<(-logMsgInterval))/(2*M_PI*freq_bw+(1<<(-logMsgInterval)));
        } else {
            smoothFactor = 1/(2*M_PI*freq_bw+1);
        }
        T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"Smoothing factor %f, Filter BW %f Hz, Offset %d ns",smoothFactor,freq_bw,VTSS_INTERVAL_NS(delay->offsetFromMaster));
        owd_filt[port].filter(&delay->offsetFromMaster, smoothFactor, true, true);
        return 1;

        /* 'g8275.1' 0,1Hz lowpass type delay filter */
        /* lowpass offset algorithm */
        /*u32 period;
        if (logMsgInterval < 0) {
            period = (1<<(-logMsgInterval)) * LP_FILTER_0_1_HZ_CONSTANT; // Filter bandwitdth = 0,1Hz => period = 10/pi sec ~= 3
        } else {
            period = LP_FILTER_0_1_HZ_CONSTANT / (1<<(logMsgInterval));
        }
        if (period == 0) period = 1;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"Setting Delay filter parameter: period to %d", period);
        owd_filt[port].filter(&delay->offsetFromMaster, period, true, false);
        return 1;*/
    }
}

/**
 * \brief Filter reset function.
 */
void ptp_basic_servo::offset_filter_reset()
{
    if (filt_conf->dist > 1) {
        act_min_offset = VTSS_MAX_TIMEINTERVAL;
    } else {
        act_min_offset = 0;
    }
    act_min_ts.seconds = 0;
    act_min_ts.nanoseconds = 0;
    actual_period = 0;
    prev_offset = 0;
    prev_offset_valid = false;
    integral = 0;
    prev_ts.seconds = 0;
    prev_ts.nanoseconds = 0;
    prev_ts_valid = false;
    prop = 0;
    diff = 0;
    delta_t = 0;
}

bool ptp_basic_servo::stable_offset_calc(ptp_slave_t *slave, vtss_timeinterval_t off)
{
    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "off " VPRI64d " currentds offset " VPRI64d " stable offset thresh " VPRI64d " ", off >> 16, slave->clock->currentDS.offsetFromMaster >> 16,
                                          stable_offset_threshold >> 16);
    if (llabs(off - slave->clock->currentDS.offsetFromMaster) <= stable_offset_threshold ) {
        if (++stable_cnt >= stable_cnt_threshold) {
            stable_offset = true;
            unstable_cnt = 0;
            --stable_cnt;
        }
    } else {
        if (++unstable_cnt >= unstable_cnt_threshold) {
            stable_offset = false;
            stable_cnt = 0;
            --unstable_cnt;
        }
    }

    return stable_offset;
}

void ptp_basic_servo::stable_offset_clear()
{
    stable_offset = false;
    unstable_cnt = 0;
    stable_cnt = 0;
}

bool ptp_basic_servo::offset_ok(ptp_slave_t *slave)
{
    if (_offset_ok) {
        _offset_ok = (llabs(slave->clock->currentDS.offsetFromMaster) <= offset_fail_threshold || !slave->two_way);
    } else {
        _offset_ok = (llabs(slave->clock->currentDS.offsetFromMaster) <= offset_ok_threshold || !slave->two_way);
    }
    return _offset_ok;
}


/* returns 0 if adjustment has not been done, because we are waiting for a delay measurement */
/* returns 1 if time has been set, i.e. the offset is too large to adjust => statechange to UNCL*/
/* returns 2 if adjustment has been done  => state change to SLVE */
int ptp_basic_servo::offset_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, vtss_timeinterval_t correction, i8 logMsgIntv, u16 sequenceId, bool peer_delay, bool virtual_port)
{
    ptp_slave_t *const slave = &ptpClock[clock_inst]->ssm;
    char str [50];
    char str2 [50];
    char str3 [50];
    i32 adj = 0;
    vtss_ptp_offset_filter_param_t filt_par;
    bool offset_result = false;
    int ret_val = 0;
    bool stable_offset = false;
    uint32_t clkDomain = ptpClock[clock_inst]->clock_init->cfg.clock_domain;

    T_RG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"ptp_offset_calc, clock_state %s", clock_state_to_string (slave->clock_state));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"F,%05u,%010d,%09d,%010d,%09d,%c" VPRI64Fd("010") "\n",sequenceId, send_time.seconds, send_time.nanoseconds, recv_time.seconds, recv_time.nanoseconds,
           correction >= 0 ? '+' : '-', correction>>16);
    
    if (slave->debugMode == 1 || slave->debugMode == 5) {
        if (!slave->activeDebug) {
            slave->activeDebug = true;
            fprintf(slave->logFile, "\n#Type: Phase\n");
            fprintf(slave->logFile, "#Start: %s\n",misc_time2str(recv_time.seconds));
            u8 txt_idx = logMsgIntv+7;
            if (txt_idx >= sizeof(IntervalTxt)/sizeof(char *)) txt_idx = sizeof(IntervalTxt)/sizeof(char *)-1;
            fprintf(slave->logFile, "#Tau: %s\n",IntervalTxt[txt_idx]);
            fprintf(slave->logFile, "#Title: Test Probe/1588 Timestamp Data/Inter-packet Timestamp (4 ns clocks)\n");
            T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"enter debug mode, keep_control %d", slave->keep_control);
            vtss_local_clock_ratio_clear(slave->localClockId);
        }
        char sign = ' ';
        mesa_timeinterval_t delay = slave->master_to_slave_delay;
        if (delay < 0) {
            delay = -delay;
            sign = '-';
        }
        if (delay == VTSS_MAX_TIMEINTERVAL) {
            fprintf(slave->logFile, "NAN\n");
        } else if (slave->debugMode == 1) {
            fprintf(slave->logFile, "%c%d.%s\n", sign, VTSS_INTERVAL_SEC(delay), vtss_tod_ns2str(VTSS_INTERVAL_NS(delay), str, 0));
        } else {
            fprintf(slave->logFile, "%c%d.%s%03d\n", sign, VTSS_INTERVAL_SEC(delay), vtss_tod_ns2str(VTSS_INTERVAL_NS(delay), str, 0), VTSS_INTERVAL_PICO(delay));
        }
    }
    if (slave->debugMode == 2 || slave->debugMode == 4 || slave->debugMode == 6 || slave->debugMode == 8) {
        if (!slave->activeDebug) {
            slave->activeDebug = true;
            debug_log_header_2_print(slave->logFile);
            vtss_local_clock_ratio_clear(slave->localClockId);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"enter debug mode, keep_control %d", slave->keep_control);
        }
        char dir = 'F';
        char subn_str[12];
        mesa_timeinterval_t ms_corr = correction;
        subn_str[0] = '\0';
        updateSubNanoToCorr(send_time, recv_time, &ms_corr);
        if (slave->debugMode == 6 || slave->debugMode == 8) {
            dir = 'X';
            if (ms_corr & 0xFFFF) {
                sprintf(subn_str, "%c%03d",'.', VTSS_INTERVAL_PICO(ms_corr));
            }
        }
        fprintf(slave->logFile, "%c,%05u,%010d,%09d,%010d,%09d,%c" VPRI64Fd("010") "%s\n", dir, sequenceId, send_time.seconds, send_time.nanoseconds, recv_time.seconds, recv_time.nanoseconds,
                                                                                        ms_corr >= 0 ? '+' : '-', ms_corr>>16, subn_str);
    }
    prc_locked_state = false;
#if defined(VTSS_SW_OPTION_SYNCE)
    vtss_appl_synce_quality_level_t ql;
    mesa_rc rc;
    if (slave->slave_port != NULL) {
        if ((rc = synce_mgmt_clock_ql_get(&ql)) != VTSS_RC_OK) {
            T_WG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"synce_mgmt_clock_ql_get returned error");
        }
        T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"ql = %d", ql);
        prc_locked_state = ((slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED || slave->clock_state == VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED ||
                             slave->clock_state == VTSS_PTP_SLAVE_CLOCK_RECOVERING) && (ql == VTSS_APPL_SYNCE_QL_PRC));
    }
#endif //defined(VTSS_SW_OPTION_SYNCE)

    // Basic offset/delay-filters and servo
    /* ignore packets while the clock is settling */
    if (slave->clock_state != VTSS_PTP_SLAVE_CLOCK_F_SETTLING && slave->clock_state != VTSS_PTP_SLAVE_CLOCK_P_SETTLING && slave->clock_state != VTSS_PTP_SLAVE_CLOCK_FREQ_LOCK_INIT) {
        /* calc 'master_to_slave_delay' */
        vtss_tod_sub_TimeInterval(&slave->master_to_slave_delay, &recv_time, &send_time);
        slave->master_to_slave_delay = slave->master_to_slave_delay - correction;
        slave->master_to_slave_delay_valid = true;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"master_to_slave_delay %s", vtss_tod_TimeInterval_To_String (&slave->master_to_slave_delay, str,0));

        if (slave->debugMode == 0) {
            /* if huge delay (> 400ms sec) then set the local clock */
            if (llabs(slave->master_to_slave_delay) >= ((400000000LL)<<16)) {
                mesa_timestamp_t now;
                /* Set the clock in the slave to be better than one sec accuracy, in a later step we use the vtss_local_clock_adj_offset to set a more precise time */
                if (clkDomain >= fast_cap(MESA_CAP_TS_DOMAIN_CNT) && llabs(slave->master_to_slave_delay) <= ((1000000000LL)<<16)) {
                    mesa_timestamp_t tmp = {};
                    vtss_tod_add_TimeInterval(&now, &tmp, &slave->master_to_slave_delay);
                    vtss_local_clock_time_set_delta(&now, clkDomain, slave->master_to_slave_delay > 0 ? TRUE : FALSE);
                } else {
                    vtss_tod_add_TimeInterval(&now, &send_time, &correction);
                    vtss_local_clock_time_set(&now, clkDomain);
                }
                /* clear adjustment rate */
                clock_servo_reset(SET_VCXO_FREQ);
                /* cannot filter with secs, clear filter */
                offset_filter_reset();
                slave->master_to_slave_delay = 0;
                slave->master_to_slave_delay_valid = false;
                //ptpClock->owd_filt->delay_filter_reset(ptpClock->owd_filt,0);  /* clears one-way E2E delay filter */
                T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Huge time difference, sendt: %s recvt: %s", TimeStampToString(&send_time, str),TimeStampToString(&recv_time, str2));
                T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Huge time difference, corr: %s now: %s", vtss_tod_TimeInterval_To_String (&correction, str, '.'), TimeStampToString(&now, str2));
                slave->clock_state = VTSS_PTP_SLAVE_CLOCK_F_SETTLING;
                T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: Set time of day", clock_state_to_string (slave->clock_state));
                vtss_ptp_timer_start(&slave->settle_timer, slave->clock_settle_time*PTP_LOG_TIMEOUT(0), false);
                slave->in_settling_period = true;
                T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"timer started");
                ret_val = 1;
            } else {
                /* |Offset| <= 0,4 sec */
                //if (slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREERUN) {
                //    slave->clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING;
                //    vtss_local_clock_mode_set(VTSS_PTP_CLOCK_LOCKING);
                //    stable_offset_clear( slave);
                //    T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: start adjustment", clock_state_to_string (slave->clock_state));
                //}
                filt_par.offsetFromMaster = slave->master_to_slave_delay - slave->clock->currentDS.meanPathDelay;
                filt_par.rcvTime = recv_time;
                if (slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREERUN) {
                    /* if big offset (> 100 usec) then set the local clock offset */
                    if (llabs(filt_par.offsetFromMaster) > 100000LL<<16) {
                        /*do offset adjustment, max +/- 0.5 sec */
                        if (filt_par.offsetFromMaster > 500000000LL<<16) {
                            filt_par.offsetFromMaster = 500000000LL<<16;
                        }
                        if (filt_par.offsetFromMaster < -500000000LL<<16) {
                            filt_par.offsetFromMaster = -500000000LL<<16;
                        }
                        T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Do offset adj master_to_slave_delay %s, meanPathDelay %s ",
                            vtss_tod_TimeInterval_To_String (&slave->master_to_slave_delay, str,0),
                            vtss_tod_TimeInterval_To_String (&slave->clock->currentDS.meanPathDelay, str2,0));
                        vtss_local_clock_adj_offset((filt_par.offsetFromMaster>>16), clkDomain);
                        T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Old offset %s ns, new offset %s ns", vtss_tod_TimeInterval_To_String (&slave->clock->currentDS.offsetFromMaster, str,0), vtss_tod_TimeInterval_To_String (&filt_par.offsetFromMaster, str2,0));
                        slave->clock->currentDS.offsetFromMaster = filt_par.offsetFromMaster;
                        slave->master_to_slave_delay_valid = false;
                        T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Offset adjustment done %s ns", vtss_tod_TimeInterval_To_String (&filt_par.offsetFromMaster, str,0));
                        offset_filter_reset();
                        stable_offset_clear();
                        slave->clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCK_INIT;
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: Adjust time of day.", clock_state_to_string (slave->clock_state));
                        vtss_ptp_timer_start(&slave->settle_timer, slave->clock_settle_time*PTP_LOG_TIMEOUT(0), false);
                        slave->in_settling_period = true;
                        ret_val = 1;
                    } else {
                        slave->clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING;
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Offset within limits", clock_state_to_string (slave->clock_state));
                    }
                }
                if (slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING ||
                    slave->clock_state == VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING ||
                    slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED ||
                    slave->clock_state == VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED)
                {
                    /* check if offset is stable */
                    stable_offset = stable_offset_calc(slave, filt_par.offsetFromMaster);

                    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"stable_offset %d, new offset " VPRI64d ", cur offset " VPRI64d, stable_offset, filt_par.offsetFromMaster>>16, slave->clock->currentDS.offsetFromMaster>>16);
                    offset_result = offset_filter(&filt_par, logMsgIntv);
                    if (offset_result) {
                        slave->clock->currentDS.offsetFromMaster = filt_par.offsetFromMaster;
                    }
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"offset_valid %d new offset " VPRI64d ", cur offset " VPRI64d " master_to_slave_delay %s, meanPathDelay %s ", offset_result, filt_par.offsetFromMaster>>16, slave->clock->currentDS.offsetFromMaster>>16, vtss_tod_TimeInterval_To_String (&slave->master_to_slave_delay, str,0), vtss_tod_TimeInterval_To_String (&slave->clock->currentDS.meanPathDelay, str2,0));
                   // T_WG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Do offset adj master_to_slave_delay %s, meanPathDelay %s ",
                   //     vtss_tod_TimeInterval_To_String (&slave->master_to_slave_delay, str,0),
                   //     vtss_tod_TimeInterval_To_String (&slave->clock->currentDS.meanPathDelay, str2,0));
                }
                if (slave->clock_state == VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING || slave->clock_state == VTSS_PTP_SLAVE_CLOCK_RECOVERING) {
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"delay_ok %d,  two_way %d", slave->clock->currentDS.delayOk, slave->two_way);
                    if (slave->clock->currentDS.delayOk || !slave->two_way) {
                        if (slave->clock_state != VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING) {
                            slave->clock_state = VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING;
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Delay measurement done", clock_state_to_string (slave->clock_state));
                        }
                        if (offset_result) {
                            /* if big offset (> 100 usec) then set the local clock offset */
                            if (llabs(filt_par.offsetFromMaster) > 100000LL<<16) {
                                   if (slave->prev_psettling ) {
                                    /* clear servo if offset is not adjusted after PSETTLING state */
                                    slave->servo->clock_servo_reset(SET_VCXO_FREQ);
                                    slave->clock_state = VTSS_PTP_SLAVE_CLOCK_FREERUN;
                                    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,
                                             "Reset to freerun freq , clock servo master_to_slave_delay %s, meanPathDelay %s ",
                                       vtss_tod_TimeInterval_To_String (&slave->master_to_slave_delay, str,0),
                                       vtss_tod_TimeInterval_To_String (&slave->clock->currentDS.meanPathDelay, str2,0));
                                   } else {
                                       /*do offset adjustment, max +/- 0.5 sec */
                                       if (filt_par.offsetFromMaster > 500000000LL<<16) {
                                           filt_par.offsetFromMaster = 500000000LL<<16;
                                       }
                                       if (filt_par.offsetFromMaster < -500000000LL<<16) {
                                           filt_par.offsetFromMaster = -500000000LL<<16;
                                       }
                                       T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,
                                             "Do offset adj master_to_slave_delay %s, meanPathDelay %s ",
                                       vtss_tod_TimeInterval_To_String (&slave->master_to_slave_delay, str,0),
                                       vtss_tod_TimeInterval_To_String (&slave->clock->currentDS.meanPathDelay, str2,0));
                                       vtss_local_clock_adj_offset((filt_par.offsetFromMaster>>16), clkDomain);
                                       slave->master_to_slave_delay_valid = false;
                                       T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Offset adjustment done %s ns", vtss_tod_TimeInterval_To_String (&filt_par.offsetFromMaster, str,0));
                                       slave->servo->offset_filter_reset();
                                       stable_offset_clear();
                                       slave->clock_state = VTSS_PTP_SLAVE_CLOCK_P_SETTLING;
                                       T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: Adjust time of day", clock_state_to_string (slave->clock_state));
                                       vtss_ptp_timer_start(&slave->settle_timer, slave->clock_settle_time*PTP_LOG_TIMEOUT(0), false);
                                       slave->in_settling_period = true;
                                   }
                                   slave->prev_psettling = false;
                                   ret_val = 1;
                            } else {
                                slave->prev_psettling = false;
                                adj = clock_servo(&filt_par, &slave->clock->parentDS.observedParentClockPhaseChangeRate, slave->localClockId, 1);
                                if (stable_offset && offset_ok(slave)) {
                                    if (slave->two_way) {
                                        slave->clock_state = VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED;
                                    }
                                    else {
                                        slave->clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED;
                                    }
                                    vtss_local_clock_mode_set(VTSS_PTP_CLOCK_LOCKED);
                                    T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason. Offset within limits", clock_state_to_string (slave->clock_state));
                                    ret_val = 2;
                                }
                            }
                        }
                    }
                } else if (slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKING) {
                    /* update 'offset_from_master' */
                    if (offset_result) {
                        /* if big offset (> 1 msec) then set the local clock offset */
                        if (llabs(filt_par.offsetFromMaster) > 1000000LL<<16) {
                            /*do offset adjustment, max +/- 0.5 sec */
                            if (filt_par.offsetFromMaster > 500000000LL<<16) {
                                filt_par.offsetFromMaster = 500000000LL<<16;
                            }
                            if (filt_par.offsetFromMaster < -500000000LL<<16) {
                                filt_par.offsetFromMaster = -500000000LL<<16;
                            }
                            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Do offset adj master_to_slave_delay %s, meanPathDelay %s ",
                                 vtss_tod_TimeInterval_To_String (&slave->master_to_slave_delay, str,0),
                                 vtss_tod_TimeInterval_To_String (&slave->clock->currentDS.meanPathDelay, str2,0));
                            vtss_local_clock_adj_offset((filt_par.offsetFromMaster>>16), clkDomain);
                            slave->master_to_slave_delay_valid = false;
                            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Offset adjustment done %s ns", vtss_tod_TimeInterval_To_String (&filt_par.offsetFromMaster, str,0));
                            offset_filter_reset();
                            stable_offset_clear();
                            slave->clock_state = VTSS_PTP_SLAVE_CLOCK_F_SETTLING;
                            T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: Adjust time of day.", clock_state_to_string (slave->clock_state));
                            vtss_ptp_timer_start(&slave->settle_timer, slave->clock_settle_time*PTP_LOG_TIMEOUT(0), false);
                            slave->in_settling_period = true;
                            ret_val = 1;
                        } else {
                            if (slave->slave_port != NULL) {
                                filt_par.indyPhy = vtss_port_has_lan8814_phy(slave->portIdentity_p->portNumber);
                            }
                            adj = clock_servo(&filt_par, &slave->clock->parentDS.observedParentClockPhaseChangeRate, slave->localClockId, 0);
                            if (stable_offset) {
                                slave->clock_state = VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING;
                                stable_offset_clear();
                                T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: Offset is stable", clock_state_to_string (slave->clock_state));
                            }
                        }
                    }
                } else if (slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED || slave->clock_state == VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED) {
                    /* update 'offset_from_master' */
                    if (slave->clock->currentDS.delayOk || !slave->two_way) {
                        if (offset_result) {
                            adj = clock_servo(&filt_par, &slave->clock->parentDS.observedParentClockPhaseChangeRate, slave->localClockId, 2);
                            ret_val = 2;
                            if (!stable_offset || !offset_ok(slave)) {
                                slave->clock_state = VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING;
                                stable_offset_clear();
                                vtss_local_clock_mode_set(VTSS_PTP_CLOCK_LOCKING);
                                T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: %s", clock_state_to_string (slave->clock_state), stable_offset ? "Offset outside limits" : "Unstable Offset");
                                //ret_val = 1;
                            } else if (slave->two_way && slave->clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED) {
                                slave->clock_state = VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING;
                                stable_offset_clear();
                                vtss_local_clock_mode_set(VTSS_PTP_CLOCK_LOCKING);
                            }
                        }
                    } else {
                        slave->clock_state = VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKING;
                        stable_offset_clear();
                        vtss_local_clock_mode_set(VTSS_PTP_CLOCK_LOCKING);
                        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clock_state %s. Reason: Awaiting Delay measurements", clock_state_to_string (slave->clock_state));
                        ret_val = 1;
                    }
                }
            }
        }

        /* log delays > 5 ms */
        if (llabs(slave->master_to_slave_delay) > (((vtss_timeinterval_t)VTSS_ONE_MIA)<<16)/200) {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Great master_to_slave_delay %s", vtss_tod_TimeInterval_To_String(&slave->master_to_slave_delay, str,0));
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Sendt: %s recvt: %s, correction %s", TimeStampToString(&send_time, str), TimeStampToString(&recv_time, str2),
                vtss_tod_TimeInterval_To_String (&correction, str3,0));
        }
        if (slave->slave_port != NULL) {
            T_NG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"delayOk %d, two_way %d, peer_delay_ok%d",
                 slave->clock->currentDS.delayOk, slave->two_way, slave->slave_port->portDS.status.peer_delay_ok);
        }
    }

    if (slave->statistics.enable) {
        master_to_slave_delay_stati(slave);
    }
    if (offset_result) {
        displayStats(&slave->clock->currentDS.meanPathDelay, &slave->clock->currentDS.offsetFromMaster,
                     slave->clock->parentDS.observedParentClockPhaseChangeRate,
                     slave->clock->parentDS.observedParentOffsetScaledLogVariance, adj);
    }

    if (slave->slave_port != NULL) {
        switch (ret_val) {
            case 1:
                if ((!is_virt_port && !slave->virtual_port_select) || (is_virt_port && slave->virtual_port_select)) {
                    if (slave->slave_port->portDS.status.portState != VTSS_APPL_PTP_UNCALIBRATED && slave->slave_port->portDS.status.portState != VTSS_APPL_PTP_SLAVE) {
                        vtss_ptp_state_set(VTSS_APPL_PTP_UNCALIBRATED, slave->clock, slave->slave_port);
                    } else if (slave->slave_port->portDS.status.portState != VTSS_APPL_PTP_SLAVE) {
                        vtss_ptp_state_set(VTSS_APPL_PTP_SLAVE, slave->clock, slave->slave_port);
                    }
                }
                break;
            case 2:
                if ((!is_virt_port && !slave->virtual_port_select) || (is_virt_port && slave->virtual_port_select)) {
                    if (slave->slave_port->portDS.status.portState != VTSS_APPL_PTP_SLAVE) {
                        vtss_ptp_state_set(VTSS_APPL_PTP_SLAVE, slave->clock, slave->slave_port);
                    }
                }
                break;
            default:
                break;
        }
    }
    return 0;
}

/**
 * \brief Filter execution function.
 *
 * Find the min offset from master within a period (defined in filter configuration)
 * At the end of the period, the min offset and the corresponding timestamp is returned.
 */
int ptp_basic_servo::offset_filter(vtss_ptp_offset_filter_param_t *offset, i8 logMsgInterval)
{
#if defined(VTSS_OPT_PHY_TIMESTAMP)
    if ((logMsgInterval < -2) && (filt_conf->dist != 0)) {
        u32 adj_pr_sec = 1<<(abs(logMsgInterval+2));
        if (filt_conf->period*filt_conf->dist < adj_pr_sec) {
            filt_conf->period = adj_pr_sec/filt_conf->dist;
            T_WG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"Increasing filter parameter: period to %d", filt_conf->period);
        }
    }
#endif
    if (actual_period == 0) {
        act_min_offset = VTSS_MAX_TIMEINTERVAL;
        act_max_offset = -VTSS_MAX_TIMEINTERVAL;
        act_mean_offset = 0;
    }
    if (act_min_offset > offset->offsetFromMaster) {
        act_min_offset = offset->offsetFromMaster;
        act_min_ts = offset->rcvTime;
    }
    if (act_max_offset < offset->offsetFromMaster) {
        act_max_offset = offset->offsetFromMaster;
    }
    act_mean_offset +=offset->offsetFromMaster;
    if (filt_conf->dist > 1) {
        /* min offset algorithm */
        if (++actual_period >= filt_conf->period) {
            act_mean_offset = act_mean_offset/filt_conf->period;
            offset->offsetFromMaster = act_min_offset;
            offset->rcvTime = act_min_ts;
            actual_period = 0;
            T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"offsetfilter, min %d ns, max %d ns",VTSS_INTERVAL_NS(act_min_offset), VTSS_INTERVAL_NS(act_max_offset));
            if (++actual_dist >= filt_conf->dist) {
                actual_dist = 0;
                return 1;
            }
        }
    } else if (filt_conf->dist == 1) {
        /* mean offset algorithm */
        if (++actual_period >= filt_conf->period) {
            act_mean_offset = act_mean_offset/filt_conf->period;
            offset->offsetFromMaster = act_mean_offset;
            actual_period = 0;
            T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"offsetfilter, mean %d ns",VTSS_INTERVAL_NS(offset->offsetFromMaster));
            return 1;
        }
    } else { // filt_conf->dist == 0
        /* lowpass offset algorithm */
        double freq_bw;
        double smoothFactor;
        freq_bw = (double)filt_conf->period/100; // In order to use period to select freq_bw divide by 100 to enable 0.01 hertz resolution on bandwidths
        if (logMsgInterval <= 0) {
            /* Calculate the smoothing factor using the sample frequency and filter bandwidth */
            smoothFactor = (1<<(-logMsgInterval))/(2*M_PI*freq_bw+(1<<(-logMsgInterval)));
        } else {
            smoothFactor = 1/(2*M_PI*freq_bw+1);
        }
        T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"Smoothing factor %f, Filter BW %f Hz, Offset %d ns",smoothFactor,freq_bw,VTSS_INTERVAL_NS(offset->offsetFromMaster));
        offset_lp_filt.filter(&offset->offsetFromMaster, smoothFactor, false, true);
        return 1;
    }
    return 0;
}

/**
 * \brief Clock Servo reset function.
 *
 * Implemnts a PID algorithm, each element can be enabled or disabled.
 *
 * clockadjustment = if (p_reg) {offset/ap} +
 *                   if (i_reg) (integral(offset)*diff(t))/ai +
 *                   if (d_reg) diff(offset)/(diff(t)*ad)
 *
 * The internal calculation unit is TimeInterval, and the resulting adj is calculated in
 *  units of 0,1 ppb.
 */
void ptp_basic_servo::clock_servo_reset(vtss_ptp_set_vcxo_freq setVCXOfreq)
{
    if (setVCXOfreq == SET_VCXO_FREQ) {
        if (holdover_ok) {
            vtss_local_clock_ratio_set(-adj_average, clock_inst);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"reset to holdover freq " VPRI64d " ppb*10",(((u64)adj_average)*10)/(((u64)2)<<16));
        } else {
            vtss_local_clock_ratio_clear(clock_inst);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"reset to freerun freq");
        }
    }
}

void ptp_basic_servo::clock_servo_status(uint32_t domain, vtss_ptp_servo_status_t *status)
{
    status->holdover_ok = holdover_ok;
    status->holdover_adj = (adj_average * 1000LL) / (1LL << 16);
}

/**
 * \brief Clock Servo execution function.
 *
 * Implemnts a PID algorithm, each element can be enabled or disabled.
 *
 * clockadjustment = if (p_reg) {offset/ap} +
 *                   if (i_reg) (integral(offset)*diff(t))/ai +
 *                   if (d_reg) diff(offset)/(diff(t)*ad)
 *
 * The internal calculation unit is TimeInterval, and the resulting adj is calculated in
 *  units of 0,1 ppb.
 */
i32 ptp_basic_servo::clock_servo(vtss_ptp_offset_filter_param_t *offset, i32 *observedParentClockPhaseChangeRate, int localClockId, int phase_lock)
{
    vtss_timeinterval_t temp_t;
    i64 time_offset;
    i32 filt_div;
    i64 adj = 0;
    i64 cur_of = 0;
    i64 freq_off_est = 0;
    vtss_timeinterval_t thr = ((vtss_timeinterval_t)servo_conf->synce_threshold) << 16;

    time_offset = VTSS_INTERVAL_NS(offset->offsetFromMaster);
    time_offset = time_offset/ADJ_OFFSET_MAX;
    if (time_offset) {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"offsetfromMaster %d ns, time_offset " VPRI64d " timer ticks",VTSS_INTERVAL_NS(offset->offsetFromMaster), time_offset);
        /* if secs, reset clock or set freq adjustment to max */
        adj = offset->offsetFromMaster > 0 ? (ADJ_FREQ_MAX_LL << 16) : -(ADJ_FREQ_MAX_LL << 16);
    } else {
        /* the PID controller */
        if (prev_ts_valid) {
            vtss_tod_sub_TimeInterval(&temp_t, &offset->rcvTime, &prev_ts);  /* delta T in ns */
            delta_t = VTSS_INTERVAL_US(temp_t);  /* delta T in us */
        } else delta_t = VTSS_ONE_MILL;
        prev_ts = offset->rcvTime;
        prev_ts_valid = true;

        cur_of = offset->offsetFromMaster;
        /* the P component */
        // prop = 0;
        if (servo_conf->p_reg && servo_conf->ap != 0 && !servo_conf->i_reg && !servo_conf->d_reg) {
            freq_off_est = adj_old - ((longlong)VTSS_ONE_MILL*(cur_of - prev_offset))/delta_t;
            freq_off_filtered = freq_off_est/4 + (3*freq_off_filtered)/4;
            prop = ((cur_of*(longlong)VTSS_ONE_MILL)/delta_t)/servo_conf->ap + freq_off_filtered;
            prev_offset = cur_of;
        }
        else if (servo_conf->p_reg && servo_conf->ap != 0) {
            prop = offset->offsetFromMaster/servo_conf->ap;
        }

        if (servo_conf->i_reg && servo_conf->ai != 0 && (phase_lock > 0)) {
            /* the accumulator for the I component */
            integral += (cur_of*delta_t)/(servo_conf->ai*(longlong)VTSS_ONE_MILL);
            if (integral > (ADJ_FREQ_MAX_LL<<16))
                integral = (ADJ_FREQ_MAX_LL<<16);
            else if (integral < -(ADJ_FREQ_MAX_LL<<16))
                integral = -(ADJ_FREQ_MAX_LL<<16);
        } else {
            integral = 0;
        }
        *observedParentClockPhaseChangeRate = (integral>>16) * servo_conf->gain;

        /* the D component */
        if (servo_conf->d_reg && servo_conf->ad != 0 && delta_t != 0LL && (phase_lock > 0)) {
            u32 ad = servo_conf->ad;
            if (!prc_locked_state) {
                ad = ad * 3;
            }

            if (prev_offset_valid) diff = (cur_of - prev_offset)*(longlong)VTSS_ONE_MILL/(delta_t*ad);
            prev_offset = cur_of;
            prev_offset_valid = true;
        } else {
            diff = 0;
            prev_offset_valid = false;
        }

        adj = (prop + diff + integral) * servo_conf->gain;
        adj_old = adj;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"cur_offset " VPRI64d ", offset_est " VPRI64d ", freq_off_filtered " VPRI64d ", delta_t " VPRI64d " us, P= " VPRI64d ", I= " VPRI64d ", D= " VPRI64d ", adj = " VPRI64d, cur_of>>16, freq_off_est>>16, freq_off_filtered >> 16, delta_t, prop, integral, diff, adj>>16);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"gain %d", servo_conf->gain);

    }
    if ((servo_conf->srv_option == VTSS_APPL_PTP_CLOCK_FREE) || (offset->offsetFromMaster >= thr) || (offset->offsetFromMaster <= -thr)) {
        /* apply controller output as a clock tick rate adjustment */
        vtss_local_clock_ratio_set(-adj, localClockId);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"|offset from master| >= %d ns or free run mode, srv_option %d", servo_conf->synce_threshold, servo_conf->srv_option);
    } else {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"|offset from master| < %d ns and free run mode, srv_option %d", servo_conf->synce_threshold, servo_conf->srv_option);
        vtss_local_clock_ratio_set(0, localClockId);
        adj = adj*delta_t/VTSS_ONE_MILL;
        vtss_local_clock_fine_adj_offset(adj, ptp_instance_2_timing_domain(localClockId));
        *observedParentClockPhaseChangeRate = 0;       
        //vtss_local_clock_adj_offset(offset->offsetFromMaster > 0 ? servo_conf->synce_ap : -servo_conf->synce_ap, localClockId);
    }
    /* check adjustment stability and update holdover frequency */
    if (phase_lock == 2) {
        filt_div = holdover_count+1 < servo_conf->ho_filter ? holdover_count+1 : servo_conf->ho_filter;
        if (filt_div == 0) filt_div = 1;
        adj_avefix = (adj_avefix * ((i64)filt_div - 1) + adj * 100000LL) / filt_div;
        if (!prc_locked_state) {
            adj_average = adj_avefix / 100000;
        } else {
            adj_average = 0LL;
        }
        adj_stable = llabs(adj_average - adj) <= llabs((servo_conf->stable_adj_threshold<<16)/10LL) ? true : false;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"adj_average " VPRI64d ", adj_stable %d, adj " VPRI64d ", filt_div %d", adj_average, adj_stable, adj, filt_div);
        if (adj_stable && !holdover_ok ) {
            holdover_ok = (holdover_count++ >= servo_conf->ho_filter);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"holdover_count %d, holdover_ok %d", holdover_count, holdover_ok);
        }
    }
    if (!adj_stable || phase_lock != 2) {
        holdover_count = 0;
        holdover_ok = false;
    }

    return adj;
}

bool ptp_basic_servo::display_stats()
{
    return servo_conf->display_stats;
}

int ptp_basic_servo::display_parm(char *buf)
{
    return sprintf(buf, " " VPRI64Fd("15") ", " VPRI64Fd("15") ", " VPRI64Fd("15") ", %6d, %6d, %6d, " VPRI64Fd("15"),
                   prop, integral, diff, VTSS_INTERVAL_NS(act_min_offset), VTSS_INTERVAL_NS(act_max_offset), VTSS_INTERVAL_NS(act_mean_offset), delta_t);
}

void ptp_basic_servo::process_alternate_timestamp(ptp_clock_t *ptp_clk, uint32_t domain, u16 instance, const mesa_timestamp_t *send_time, const mesa_timestamp_t *recv_time, mesa_timeinterval_t correction, i8 logMsgIntv, bool virtual_port)
{
    mesa_timeinterval_t master_to_slave_delay, path_delay = 0;
    uint32_t slave_port = ptp_clk->slavePort;
    struct ptp_basic_servo *servo = NULL;
    mesa_timeinterval_t adj;

    if (is_virt_port && !virtual_port) {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, "entering ethernet port alternate servo");
        // invocation meant for ethernet while virtual port is active.
        servo = ep_servo;
    } else if (!is_virt_port && virtual_port) {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, "entering virtual port alternate servo");
        // invocation meant for virtual port while ethernet port is active.
        servo = vp_servo;
    }
    // Return if there is no alternate servo.
    if (servo == NULL) {
        return;
    }
    vtss_tod_sub_TimeInterval(&master_to_slave_delay, recv_time, send_time);
    master_to_slave_delay -= correction;
    if (slave_port) {
        if (ptp_clk->ptpPort[uport2iport(slave_port)].port_config->delayMechanism == VTSS_APPL_PTP_DELAY_MECH_P2P) {
            path_delay = ptp_clk->ptpPort[uport2iport(slave_port)].portDS.status.peerMeanPathDelay;
        }
    }
    T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, " alternate servo mstr_to_slv_dly " VPRI64d " path_delay " VPRI64d "", master_to_slave_delay>>16, path_delay>>16);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"%010d,%09d,%010d,%09d" VPRI64Fd("010") "", send_time->seconds, send_time->nanoseconds, recv_time->seconds, recv_time->nanoseconds, correction>>16);

    servo->prev_offset = master_to_slave_delay - path_delay;
    servo->prev_offset_valid = true;
    servo->prev_ts = *recv_time;
    servo->prev_ts_valid = true;
    adj = prop + diff + integral;

    servo->prop = servo->prev_offset/servo_conf->ap;
    servo->integral = adj - servo->prop;
    servo->diff = 0;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER,"alternate servo P= " VPRI64d ", I= " VPRI64d ", D= " VPRI64d ", adj = " VPRI64d, servo->prop, servo->integral, servo->diff, adj>>16);
}

mesa_rc ptp_basic_servo::switch_1pps_virtual_ref(ptp_clock_t *ptp_clk, uint16_t instance, uint32_t domain, bool enable, bool warm_start)
{
    if (enable && !is_virt_port) {// enable virtual port.
        ptp_clk->ssm.servo = vp_servo ? vp_servo : this;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, "switch to virtual port servo");
    } else if (!enable && is_virt_port) { // enable ethernet port
        ptp_clk->ssm.servo = ep_servo ? ep_servo : this;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, "switch to ethernet port servo");
    }

    return MESA_RC_OK;
}

mesa_rc ptp_basic_servo::set_device_1pps_virtual_ref(ptp_clock_t *ptp_clk, uint16_t inst, uint32_t domain, bool enable)
{
    vtss_appl_ptp_clock_filter_config_t *of;
    vtss_appl_ptp_clock_slave_config_t slv_cfg;

    if (enable) {
        vtss_appl_ptp_clock_slave_default_config_get(&slv_cfg);
        T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "creating virtual port servo");
        if (!vp_servo) {
            of = new vtss_appl_ptp_clock_filter_config_t;
            vtss_appl_ptp_filter_default_parameters_get(of, VTSS_APPL_PTP_PROFILE_IEEE_802_1AS);
            vp_servo = new ptp_basic_servo(inst, of, servo_conf, 1);
            vp_servo->ep_servo = this;
            vp_servo->is_virt_port = true;
            vp_servo->stable_offset_threshold = ((i64)slv_cfg.stable_offset)<<16;
            vp_servo->offset_ok_threshold = ((i64)slv_cfg.offset_ok)<<16;
            vp_servo->offset_fail_threshold = ((i64)slv_cfg.offset_fail)<<16;
            // may not be needed
            ep_servo = this;
            vp_servo->vp_servo = vp_servo;
            T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "new virtual port servo created");
        }
    } else {
        ptp_basic_servo *ptp_servo = ep_servo;
        ptp_clk->ssm.servo = ep_servo;
        delete vp_servo->filt_conf;
        delete vp_servo;
        ptp_servo->vp_servo = NULL;
        T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "virtual port servo deleted");
    }
    return MESA_RC_OK;
}

//This function calculates phase difference between rx timestamp, current time and converts it into master time difference in nano seconds.
int64_t calcGMPhaseChange(ptp_clock_t *ptpClock, mesa_timestamp_t *rxTs, int64_t adj)
{
    mesa_timestamp_t curr;
    uint64_t ret;
    mesa_timeinterval_t deltaT;

    vtss_local_clock_time_get(&curr, ptpClock->localClockId, &ret);
    vtss_tod_sub_TimeInterval(&deltaT, &curr, rxTs);
    ret = ((deltaT >> 16) * (adj >> 16))/VTSS_ONE_MIA; //convert from scaled ppb to ppb, timeinterval to ns.
    return ret;
}

// Seed the initial frequency into integral parameter of servo. Adjust phase and frequency once.
void ptp_basic_servo::seedFreqSet(ptp_clock_t *ptpClock, mesa_timestamp_t *txTs, mesa_timestamp_t *rxTs, mesa_timeinterval_t corr, double freq, ptp_follow_up_tlv_info_t *followUpInfo, vtss_appl_ptp_802_1as_current_ds_t *current1AsDs)
{
    int64_t adjPrev, adj, gmPhaseUpdate=0;
    BOOL negative = FALSE;
    char str[50];

    if (ptpClock->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_IEEE_802_1AS &&
        ptpClock->clock_init->cfg.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
        return;
    }

    // Frequency update
    adjPrev = vtss_local_clock_ratio_get(ptpClock->localClockId);
    if (followUpInfo->scaledLastGmFreqChange != 0) {
        // Update from follow-up tlv.
        adj = ((int64_t)followUpInfo->scaledLastGmFreqChange * VTSS_ONE_MIA) * (1LL << 16) / RATE_TO_U32_CONVERT_FACTOR;
        adj = adjPrev + adj;
        integral = -adj;
        if (servo_conf->gain) {
            integral = integral/servo_conf->gain;
        }
        vtss_local_clock_ratio_set(adj, ptpClock->localClockId);
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, "Updating seed frequency " VPRI64d "from lastGMFreqChange %d adjPrev " VPRI64d, (adj >> 16), followUpInfo->scaledLastGmFreqChange, (adjPrev >> 16));
    } else {
        // Update frequency from first packet frequency when there is no 'lastGMFreqChange'
        // If adjustment frequency from rate ratio is greater than current adjustmet
        // by 10ppb, then apply frequency from rate ratio.

        //convert rate ratio into units of ppb.
        adj = freq * VTSS_ONE_MIA;
        if (llabs((adjPrev>>16) - adj) > ADJ_1AS_FREQ_LOCK) {
            adj = adj << 16;
            integral = -adj;
            if (servo_conf->gain) {
                integral = integral/servo_conf->gain;
            }
            if (!softClock) {
                gmPhaseUpdate = calcGMPhaseChange(ptpClock, rxTs, (adj-adjPrev));
            }
            vtss_local_clock_ratio_set(adj, ptpClock->localClockId);
            followUpInfo->scaledLastGmFreqChange = (int32_t)(((adj - adjPrev) >> 16) * RATE_TO_U32_CONVERT_FACTOR / VTSS_ONE_MIA);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, "Updating seed frequency from rate ratio. adj = " VPRI64d " ppb scaledLastGmFreqChange 0x%x", adj, followUpInfo->scaledLastGmFreqChange);
            ptpClock->localGMUpdate = true;
        }
    }

    // Phase update
    if (followUpInfo->lastGmPhaseChange.scaled_ns_low != 0 ||
        followUpInfo->lastGmPhaseChange.scaled_ns_high != 0) {
        // Update from follow-up tlv.
        mesa_timestamp_t ts;

        vtss_tod_scaledNS_to_timestamp(&followUpInfo->lastGmPhaseChange, &ts, &negative);
        vtss_local_clock_time_set_delta(&ts, ptpClock->clock_init->cfg.clock_domain, negative);
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, "Updating seed phase %s from lastGMPhaseChange in %s direction", TimeStampToString(&ts, str), negative ? "-ve" : "+ve");
    } else {
        // first packet phase change.
        mesa_timestamp_t    delta = {};
        mesa_scaled_ns_t    phaseChange = {};

        vtss_tod_sub_scaled_time(&phaseChange, txTs, rxTs);
        vtss_tod_scaledNS_to_timestamp(&phaseChange, &delta, &negative);
        if (!softClock) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, "GM phase update " VPRI64d, gmPhaseUpdate);
            gmPhaseUpdate = gmPhaseUpdate << 16; //Convert it to timeinterval.
            if (negative) {
                gmPhaseUpdate = -gmPhaseUpdate;
            }
            vtss_tod_add_TimeInterval(&delta, &delta, &gmPhaseUpdate);
        }
        vtss_local_clock_time_set_delta(&delta, ptpClock->clock_init->cfg.clock_domain, negative);
        //Update currentDS here to ensure stable_offset succeeds in next iteration.
        ptpClock->currentDS.offsetFromMaster = 0;

        current1AsDs->lastGMPhaseChange = phaseChange;
        followUpInfo->lastGmPhaseChange = phaseChange;
        current1AsDs->timeOfLastGMPhaseChangeEvent = u32(vtss_current_time()/10);
        ptpClock->localGMUpdate = true;
        T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, "Updating seed phase %s from offset calculation in %s", TimeStampToString(&delta, str), negative ? "-ve" : "+ve");
        T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, "txTs %s", TimeStampToString(txTs, str));
        T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE, "rxTs %s", TimeStampToString(rxTs, str));
    }
    if (softClock) {
        // Software clock.
        ptpClock->ssm.clock_state = VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED;
    } else {
        // chip Ltc.
        ptpClock->ssm.clock_state = VTSS_PTP_SLAVE_CLOCK_F_SETTLING;
        ptpClock->ssm.in_settling_period = true;
        vtss_ptp_timer_start(&ptpClock->ssm.settle_timer, (ptpClock->ssm.clock_settle_time)*(PTP_LOG_TIMEOUT(0)), false);
    }
}

mesa_rc ptp_basic_servo::activate(uint32_t domain, bool hybrid_init)
{
    if (domain >= fast_cap(MESA_CAP_TS_DOMAIN_CNT)) {
        softClock = true;
    }
    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "activate of BASIC servo was called");
    return VTSS_RC_OK;
}

void ptp_basic_servo::deactivate(uint32_t domain)
{
    softClock = false;
    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "deactivate of BASIC servo was called");
}

/**
 * \brief Create a basic PTP offset filter instance.
 * Create an instance of the basic vtss_ptp offset filter
 *
 * \param of [IN]  pointer to a structure containing the default parameters for
 *                the offset filter
 * \param s [IN]  pointer to a structure containing the default parameters for
 *                the servo
 * \return (opaque) instance data reference or NULL.
 */
ptp_basic_servo::ptp_basic_servo(int inst, vtss_appl_ptp_clock_filter_config_t *of, const vtss_appl_ptp_clock_servo_config_t *s, int port_count)
 : offset_lp_filt("Offset LP filter"), my_port_count(port_count+1)
{
    int i;
    clock_inst = inst;
    filt_conf = of;
    servo_conf = s;
    vtss_lowpass_filter_s * f;
    owd_filt = (vtss_lowpass_filter_s *) vtss_ptp_calloc(port_count+1, sizeof(vtss_lowpass_filter_t));
    for (i = 0; i <= port_count; i++) {
        f = new (owd_filt+i) vtss_lowpass_filter_s("Delay LP filter");
        T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "delay_filter %d adr = %p size %u", i, f, (u32)sizeof(vtss_lowpass_filter_t));
    }
    owd_wl_filt = (vtss_wl_delay_filter_t*)vtss_ptp_calloc(port_count+1, sizeof(vtss_wl_delay_filter_t));
    T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "delay filter: delay_filter = %d", filt_conf->delay_filter);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "delay filter: period = %d dist = %d", filt_conf->period, filt_conf->dist);
    T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "offset filter: display_stats %d", servo_conf->display_stats);
}

ptp_basic_servo::~ptp_basic_servo()
{
    if (owd_filt) vtss_ptp_free(owd_filt);
    if (owd_wl_filt) vtss_ptp_free(owd_wl_filt);
}

#endif // SW_OPTION_BASIC_PTP_SERVO
