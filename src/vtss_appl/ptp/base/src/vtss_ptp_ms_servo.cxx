/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss_ptp_ms_servo.h"
#include "vtss_ptp_os.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_ptp_api.h"
#include "vtss_tod_api.h"
#include "misc_api.h"

#if defined(VTSS_SW_OPTION_SYNCE)
#include "synce_ptp_if.h"
#endif

#include "synce_custom_clock_api.h"
#if defined(VTSS_SW_OPTION_SYNCE)
#endif

#include "zl_3038x_api_pdv_api.h"

extern zl303xx_ParamsS *zl303xx_Params[5];

int ptp_ms_servo::activate_count = 0;

/* This function is used to update sub nano second part of time interval into the correction field. */
static void updateSubNanoToCorr(mesa_timestamp_t send, mesa_timestamp_t recv, mesa_timeinterval_t * const corr)
{
    *corr -= (recv.nanosecondsfrac - send.nanosecondsfrac);
}

void ptp_ms_servo::displayStats(const mesa_timeinterval_t *meanPathDelay, const mesa_timeinterval_t *offsetFromMaster,
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

        len += sprintf(sbuf + len,
                       ", %d" ", %d",
                       observedParentClockPhaseChangeRate,
                       observedParentOffsetScaledLogVariance);

        len += sprintf(sbuf + len,"\n");
        printf("%s", sbuf);
    }
}

void ptp_ms_servo::process_alternate_timestamp(ptp_clock_t *ptp_clk, uint32_t domain, u16 instance, const mesa_timestamp_t *send_time, const mesa_timestamp_t *recv_time, mesa_timeinterval_t correction, i8 logMsgIntv, bool virtual_port)
{
    vtss_zl_30380_process_timestamp_t ts;

    T_I("alternate server virtual port %d", virtual_port);
    if(!zl_30380_packet_rate_set(domain, instance, logMsgIntv, TRUE, virtual_port)) {
        T_W("alternate server packet_rate_set failed");
    }
    ts.tx_ts = *send_time;
    ts.rx_ts = *recv_time;
    ts.corr = correction;
    ts.fwd_path = TRUE;
    ts.peer_delay = virtual_port ? TRUE : FALSE;
    ts.virtual_port = virtual_port;
    if(!zl_30380_process_timestamp(domain, instance, &ts)) {
        T_W("zl_30380_process_timestamp failed");
    }
}

void ptp_ms_servo::vtss_1588_process_timestamp(uint32_t domain, u16 serverId, const mesa_timestamp_t *send_time, const mesa_timestamp_t *recv_time, mesa_timeinterval_t correction, i8 logMsgIntv, bool fwd_path, bool peer_delay, bool virtual_port)
{
    vtss_zl_30380_process_timestamp_t ts;
    T_D("domain %d, serverId %d, fwd_path %d, activate_count %d", domain, serverId, fwd_path, activate_count);
    if (activate_count > 0) {
        if(!zl_30380_packet_rate_set(domain, serverId, logMsgIntv, fwd_path, virtual_port)) {
            T_W("zl_3034x_packet_rate_set failed");
        }
        ts.tx_ts = *send_time;
        ts.rx_ts = *recv_time;
        ts.corr = correction;
        ts.fwd_path = fwd_path;
        ts.peer_delay = peer_delay;
        ts.virtual_port = virtual_port;
        if(!zl_30380_process_timestamp(domain, serverId, &ts)) {
            T_W("zl_30380_process_timestamp failed");
        }
    } else {
        T_D("process_timestamp called while no 30380 instance present.");
    }
}

void ptp_ms_servo::vtss_1588_pdv_status_get(uint32_t domain, u16 serverId, vtss_slave_clock_state_t *pdv_clock_state, i32 *freq_offset)
{
    if (activate_count > 0) {
        if(!zl_30380_pdv_status_get(domain, serverId, pdv_clock_state, freq_offset)) {
            T_W("zl_30380_packet_pdv_status_get failed");
        }
    } else {
        T_D("pdv_status called while no 30380 instance present.");
    }
}

/**
 * \brief Create a Microsemi PTP offset filter instance.
 * Create an instance of the Microsemi PTP offset filter
 *
 * \param of [IN]  pointer to a structure containing the default parameters for
 *                the offset filter
 * \param s [IN]  pointer to a structure containing the default parameters for
 *                the servo
 * \return (opaque) instance data reference or NULL.
 */
ptp_ms_servo::ptp_ms_servo(int inst, const vtss_appl_ptp_clock_servo_config_t *s, const vtss_appl_ptp_clock_config_default_ds_t *clock_config, int port_count)
 : my_port_count(port_count+1)
{
    int i;
    vtss_ptp_filters::vtss_lowpass_filter_t * f;
    owd_filt = (vtss_ptp_filters::vtss_lowpass_filter_t *) vtss_ptp_calloc(port_count+1, sizeof(vtss_ptp_filters::vtss_lowpass_filter_t));
    for (i = 0; i <= port_count; i++) {
        f = new (owd_filt+i) vtss_ptp_filters::vtss_lowpass_filter_t ("PDelayFilt", filter_const);

        T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "delay_filter %d adr = %p size %u", i, f, (u32)sizeof(vtss_ptp_filters::vtss_lowpass_filter_t));
    }
    clock_inst = inst;
    servo_conf = s;
    default_ds_conf = clock_config;
}

ptp_ms_servo::~ptp_ms_servo()
{
    if (owd_filt) vtss_ptp_free(owd_filt);
}

mesa_rc ptp_ms_servo::switch_to_packet_mode(uint32_t domain)
{
    mesa_rc rc = VTSS_RC_ERROR;
    
    if (activate_count > 0) {
        if ((rc = zl_30380_apr_switch_to_packet_mode(domain, clock_inst)) == VTSS_RC_OK) {
            T_I("APR switched to packet mode.");
        } else {
            T_W("Could not switch APR to packet mode.");
        }
    } else {
        T_D("Tried to switch to packet mode while no 30380 instance present.");
    }

    return rc;
}

mesa_rc ptp_ms_servo::switch_to_hybrid_mode(uint32_t domain)
{
    mesa_rc rc = VTSS_RC_ERROR;
    
    if (activate_count > 0) {
        if ((rc = zl_30380_apr_switch_to_hybrid_mode(domain, clock_inst)) == VTSS_RC_OK) {
            T_I("APR switched to hybrid mode.");
        } else {
            T_W("Could not switch APR to hybrid mode.");
        }
    } else {
        T_D("Tried to switch to hybrid mode while no 30380 instance present.");
    }

    return rc;
}

bool ptp_ms_servo::mode_is_hybrid_mode(uint32_t domain)
{
    BOOL state;

    if (activate_count > 0) {
        if (zl_30380_apr_in_hybrid_mode(domain, clock_inst, &state) == VTSS_RC_OK) {
            return (bool)state;
        } else {
            T_W("Could not get hybrid status from APR.");
            return false;
        }
    } else {
        T_D("Request for hybrid status while no 30380 instance present .");
        return false;
    }
}

mesa_rc ptp_ms_servo::set_active_ref(uint32_t domain, int stream)
{
    mesa_rc rc = VTSS_RC_ERROR;
    
    if (activate_count > 0) {
        if ((rc = zl_30380_apr_set_active_ref(domain, stream)) != VTSS_RC_OK) {
            T_W("Could not set active reference to stream %d", stream);
        } else {
            T_I("APR set active reference");
        }
    } else {
        T_D("Tried to change active reference while no 30380 instance present.");
    }

    return rc;
}

mesa_rc ptp_ms_servo::set_active_electrical_ref(uint32_t domain, int input)
{
    mesa_rc rc = VTSS_RC_ERROR;
    
    //if (activate_count > 0) {
        //if ((rc = zl30380_apr_set_active_elec_ref(zl303xx_Params[domain], input)) != VTSS_RC_OK) {
        //    T_W("Could not set electrical reference mode");
        //} else {
        //    T_I("APR set electrical reference mode");
        //}
    //} else {
    //    T_D("Tried to set electrical reference mode while no 30380 instance present.");
    //}

    return rc;
}

mesa_rc ptp_ms_servo::force_holdover_set(BOOL enable)
{
    mesa_rc rc = VTSS_RC_ERROR;
    
    if (activate_count > 0) {
        if ((rc = zl_30380_apr_force_holdover_set(clock_inst, enable)) == MESA_RC_OK) {
            force_holdover_state = enable;
            T_I("APR holdover mode set = %s.", enable ? "TRUE" : "FALSE");
        } else {
            T_W("Could not set APR holdover mode = %s.", enable ? "TRUE" : "FALSE");
        }
    } else {
        T_D("Tried to set/clear APR holdover mode while no 30380 instance present.");
    }

    return rc;
}

mesa_rc ptp_ms_servo::force_holdover_get(BOOL *enable)
{
    *enable = force_holdover_state;

    return VTSS_RC_OK;
}

/**
 * \brief Filter execution function.
 */
int ptp_ms_servo::delay_filter(vtss_ptp_offset_filter_param_t *delay, i8 logMsgInterval, int port)
{
    VTSS_ASSERT(port < my_port_count);
    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "delay_filter (port %d)", port);

    /* low pass type delay filter */
    owd_filt[port].filter(&delay->offsetFromMaster);
    return 1;
}

/**
 * \brief Filter reset function.
 */
void ptp_ms_servo::delay_filter_reset(int port)
{
    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "delay_filter_reset member of ptp_ms_servo was called.");
    owd_filt[port].reset();
}

/**
 * \brief Filter execution function.
 *
 * Find the min offset from master within a period (defined in filter configuration)
 * At the end of the period, the min offset and the corresponding timestamp is returned.
 */
int ptp_ms_servo::offset_filter(vtss_ptp_offset_filter_param_t *offset, i8 logMsgInterval)
{
    T_EG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "delay_filter member of ptp_ms_servo should never be called.");
    return 0;
}

/**
 * \brief Filter reset function.
 */
void ptp_ms_servo::offset_filter_reset()
{
    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "offset_filter_reset member of ptp_ms_servo was called.");
}

void ptp_ms_servo::delay_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, mesa_timeinterval_t correction, i8 logMsgIntv)
{
    ptp_slave_t *const slave = &ptpClock[clock_inst]->ssm;
    uint32_t domain = ptpClock[clock_inst]->clock_init->cfg.clock_domain;
    char str[50];
    mesa_timeinterval_t t3mt2;
    mesa_timeinterval_t t4mt1;
    mesa_timeinterval_t ms_corr = correction;
    T_RG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"ptp_delay_calc");

    /* calc 'slave_to_master_delay' */
    vtss_tod_sub_TimeInterval(&slave->slave_to_master_delay, &recv_time, &send_time);
    slave->slave_to_master_delay -= correction;
    slave->master_to_slave_delay_valid = true;
    if (abs(slave->slave_to_master_delay) > (abs(slave->master_to_slave_delay) * 2) && slave->twoStepFlag && (MESA_CHIP_FAMILY_LAN966X == fast_cap(MESA_CAP_MISC_CHIP_FAMILY))){
        slave->master_to_slave_delay_valid = false;
    }
    if (slave->statistics.enable) {
        slave_to_master_delay_stati(slave);
    }

    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"correction %s", vtss_tod_TimeInterval_To_String(&correction, str, 0));
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
        T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"B,%05u,%010d,%09d,%5d,%010d,%09d,%5d,%c" VPRI64Fd("010") "\n",slave->last_delay_req_event_sequence_number, recv_time.seconds, recv_time.nanoseconds, recv_time.nanosecondsfrac, send_time.seconds, send_time.nanoseconds, recv_time.nanosecondsfrac,
               correction >= 0 ? '+' : '-', correction>>16);
        if ((slave->clock->currentDS.meanPathDelay < 0) || (t3mt2 > (mesa_timeinterval_t)10000000*(1LL<<16)) || (t4mt1 > (mesa_timeinterval_t)10000000*(1LL<<16)))
        {
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"t1: %s", TimeStampToString (&slave->sync_tx_time, str));
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"t2: %s", TimeStampToString (&slave->sync_receive_time, str));
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"t3: %s", TimeStampToString (&send_time, str));
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"t4: %s", TimeStampToString (&recv_time, str));
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"slave_to_master_delay %s", vtss_tod_TimeInterval_To_String (&slave->slave_to_master_delay, str,0));
            T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"master_to_slave_delay %s", vtss_tod_TimeInterval_To_String (&slave->master_to_slave_delay, str,0));
        }
        /* MS Servo does not support sub nano second in its API. Hence, it is added to correction field. */
        updateSubNanoToCorr(send_time, recv_time, &ms_corr);
        if (slave->debugMode == 3 || slave->debugMode == 4 || slave->debugMode == 7 || slave->debugMode == 8) {
            if (!slave->activeDebug) {
                slave->activeDebug = true;
                debug_log_header_2_print(slave->logFile);
                T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"enter debug mode, keep_control %d", slave->keep_control);
                //if (!slave->keep_control) {
                //    if (zl_30380_holdover_set(domain, slave->localClockId, TRUE) != VTSS_RC_OK) T_EG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"zl_30380_holdover_set returned error code");
                //}
            }
            char dir = 'B';
            char subn_str[12];
            subn_str[0] = '\0';
            if (slave->debugMode == 7 || slave->debugMode == 8) {
                dir = 'Y';
                if (ms_corr & 0xFFFF) {
                    sprintf(subn_str, "%c%03d",'.', VTSS_INTERVAL_PICO(ms_corr));
                }
            }
            fprintf(slave->logFile, "%c,%05u,%010d,%09d,%010d,%09d," VPRI64Fd("+011") "%s\n", dir, slave->last_delay_req_event_sequence_number, recv_time.seconds, recv_time.nanoseconds, send_time.seconds, send_time.nanoseconds,
                                                                                            ms_corr>>16, subn_str);
        }
        //if (slave->slave_port != NULL) {
        if (slave->keep_control || slave->debugMode == 0) {
            vtss_1588_process_timestamp(domain, slave->localClockId, &send_time, &recv_time, ms_corr, logMsgIntv, false, false, false);
        }
        slave->clock->currentDS.delayOk = true;
        //}
        //else {
        //    slave->clock->currentDS.delayOk = false;
        //}
    } else {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"Not ready to delay measurements");
        slave->clock->currentDS.delayOk = false;
        slave->clock->currentDS.meanPathDelay = 0LL;
        slave->servo->delay_filter_reset(0);  /* clears one-way E2E delay filter */
    }

    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "delay_calc member of ptp_ms_servo was called.");
}

/* returns 0 if adjustment has not been done, because we are waiting for a delay measurement */
/* returns 1 if time has been set, i.e. the offset is too large to adjust => statechange to UNCL*/
/* returns 2 if adjustment has been done  => state change to SLVE */
int ptp_ms_servo::offset_calc(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, const mesa_timestamp_t& send_time, const mesa_timestamp_t& recv_time, mesa_timeinterval_t correction, i8 logMsgIntv, u16 sequenceId, bool peer_delay, bool virtual_port)
{
    ptp_slave_t *const slave = &ptpClock[clock_inst]->ssm;
    uint32_t domain = ptpClock[clock_inst]->clock_init->cfg.clock_domain;
    char str[50];
    i32 adj = 0;
    bool offset_result = false;
    vtss_slave_clock_state_t pdv_clock_state;
    i32 freq_offset;
    mesa_timeinterval_t ms_corr = correction;

    T_RG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"ptp_offset_calc, clock_state %s", clock_state_to_string (slave->clock_state));
    T_DG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"F,%05u,%010d,%09d,%05d,%010d,%09d,%05d,%c" VPRI64Fd("010") "\n",sequenceId, send_time.seconds, send_time.nanoseconds, send_time.nanosecondsfrac, recv_time.seconds, recv_time.nanoseconds, recv_time.nanosecondsfrac,
           correction >= 0 ? '+' : '-', correction>>16);
    /* this hack is done to dump the offset from master measurements, because the Calnex tester fails */
    /* the correct solution is to save the dump in memory and then get it by a debug command */
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
            //if (!slave->keep_control) {
            //    if (zl_30380_holdover_set(domain, slave->localClockId, TRUE) != VTSS_RC_OK) T_EG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"zl_30380_holdover_set returned error code");
            //}

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
    /* MS Servo does not support sub nano second in its API. Hence, it is added to correction field. */
    updateSubNanoToCorr(send_time, recv_time, &ms_corr);
    if (slave->debugMode == 2 || slave->debugMode == 4 || slave->debugMode == 6 || slave->debugMode == 8) {
        if (!slave->activeDebug) {
            slave->activeDebug = true;
            debug_log_header_2_print(slave->logFile);
            T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"enter debug mode, keep_control %d", slave->keep_control);
            //if (!slave->keep_control) {
            //    if (zl_30380_holdover_set(domain, slave->localClockId, TRUE) != VTSS_RC_OK) T_EG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"zl_30380_holdover_set returned error code");
            //}
        }
        char dir = 'F';
        char subn_str[12];
        subn_str[0] = '\0';
        if (slave->debugMode == 6 || slave->debugMode == 8) {
            dir = 'X';
            if (ms_corr & 0xFFFF) {
                sprintf(subn_str, "%c%03d",'.', VTSS_INTERVAL_PICO(ms_corr));
            }
        }
        fprintf(slave->logFile, "%c,%05u,%010d,%09d,%010d,%09d," VPRI64Fd("+011") "%s\n", dir, sequenceId, send_time.seconds, send_time.nanoseconds, recv_time.seconds, recv_time.nanoseconds,
                                                                                        ms_corr>>16, subn_str);
    }
    if (slave->debugMode == 0) {
        if (slave->activeDebug) {
            T_IG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"clear debug mode, keep_control %d", slave->keep_control);
            slave->activeDebug = false;
            // clear holdover mode
            //if (!slave->keep_control) {
            //    if (zl_30380_holdover_set(domain, slave->localClockId, FALSE) != VTSS_RC_OK) T_EG(VTSS_TRACE_GRP_PTP_BASE_CLK_STATE,"zl_30380_holdover_set returned error code");
            //}
        }
    }
//    // Check if time difference to best master is more than 2 seconds. If so then we must adjust the time.
//    if (vtss_ptp_servo_get_best_master() == slave->localClockId) {
//        mesa_timestamp_t now_time;
//        u32 dummy;
//        i64 now_time_nanosecs;
//        i64 send_time_nanosecs;
//        i64 time_difference_nanosecs;
//
//        vtss_local_clock_time_get(&now_time, slave->localClockId, &dummy);
//
//        now_time_nanosecs = ((((i64)now_time.sec_msb) << 32) + (i64)now_time.seconds) * 1000000000LL + ((i64)now_time.nanoseconds);
//        send_time_nanosecs = ((((i64)send_time.sec_msb) << 32) + (i64)send_time.seconds) * 1000000000LL + ((i64)send_time.nanoseconds);
//        time_difference_nanosecs = now_time_nanosecs - send_time_nanosecs;
//
//        if (llabs(time_difference_nanosecs) > 2LL * 1000000000LL) {
//            (void)zl_3034x_apr_set_time(time_difference_nanosecs);
//            T_WG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Time was adjusted since it was more than 2 secs off compared to best master.");
//        }
//    }
    vtss_tod_sub_TimeInterval(&slave->master_to_slave_delay, &recv_time, &send_time);
    slave->master_to_slave_delay = slave->master_to_slave_delay - correction;
    slave->master_to_slave_delay_valid = true;
    if (abs(slave->slave_to_master_delay) > (abs(slave->master_to_slave_delay) * 2) && slave->twoStepFlag && (MESA_CHIP_FAMILY_LAN966X == fast_cap(MESA_CAP_MISC_CHIP_FAMILY))){
        slave->master_to_slave_delay_valid = false;
    }
    if (slave->keep_control || slave->debugMode == 0) {
        vtss_1588_process_timestamp(domain, slave->localClockId, &send_time, &recv_time, ms_corr, logMsgIntv, true, peer_delay, virtual_port);
    }
    slave->clock->currentDS.offsetFromMaster = slave->master_to_slave_delay - slave->clock->currentDS.meanPathDelay;
    offset_result = true;
    vtss_1588_pdv_status_get(domain, slave->localClockId, &pdv_clock_state, &freq_offset);

    if (pdv_clock_state == VTSS_PTP_SLAVE_CLOCK_HOLDOVER) {     // If the MS-PDV reports VTSS_PTP_SLAVE_CLOCK_HOLDOVER, we report the status as VTSS_PTP_SLAVE_CLOCK_RECOVERING since in
        slave->clock_state = VTSS_PTP_SLAVE_CLOCK_RECOVERING;   // this state the servo, although in holdover mode, is trying to recover the lock from holdover.
    } else {
        slave->clock_state = pdv_clock_state;
    }

    slave->clock->parentDS.observedParentClockPhaseChangeRate = freq_offset/1000;
    slave->ptsf_unusable = (pdv_clock_state == VTSS_PTP_SLAVE_CLOCK_FREQ_LOCKED || pdv_clock_state == VTSS_PTP_SLAVE_CLOCK_PHASE_LOCKED) ? false : true;
    #if defined(VTSS_SW_OPTION_SYNCE)
    vtss_ptp_ptsf_state_set(slave->localClockId);
    #endif

    if (slave->statistics.enable) {
        master_to_slave_delay_stati(slave);
    }
    if (offset_result) {
        displayStats(&slave->clock->currentDS.meanPathDelay, &slave->clock->currentDS.offsetFromMaster,
                     slave->clock->parentDS.observedParentClockPhaseChangeRate,
                     slave->clock->parentDS.observedParentOffsetScaledLogVariance, adj);
    }

    // Make sure slave port's portState is VTSS_APPL_PTP_SLAVE
    if (slave->slave_port != 0) {
        if (slave->slave_port->portDS.status.portState != VTSS_APPL_PTP_SLAVE) {
            vtss_ptp_state_set(VTSS_APPL_PTP_SLAVE, slave->clock, slave->slave_port);
        }
    }

    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "offset_calc member of ptp_ms_servo was called.");
    return 0;
}

/**
 * \brief Clock Servo reset function.
 *
 */
void ptp_ms_servo::clock_servo_reset(vtss_ptp_set_vcxo_freq setVCXOfreq)
{
    T_DG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "clock_servo_reset member of ptp_ms_servo was called.");
}

void ptp_ms_servo::clock_servo_status(uint32_t domain, vtss_ptp_servo_status_t *status)
{
    zl_3038x_srvr_holdover_param_get(domain, clock_inst, &status->holdover_ok, &status->holdover_adj);
}

bool ptp_ms_servo::display_stats()
{
    return servo_conf->display_stats;
}

mesa_rc ptp_ms_servo::activate(uint32_t domain, bool hybrid_init)
{
    vtss_appl_ptp_config_port_ds_t port_cfg = {};

    vtss_ptp_apply_profile_defaults_to_port_ds(&port_cfg, default_ds_conf->profile);
    /* Add a Timing Server (PTP protocol stream) to the CGU */
    force_holdover_state = FALSE;
    if (apr_stream_create(domain, clock_inst, default_ds_conf->filter_type, hybrid_init, port_cfg.logSyncInterval) != MESA_RC_OK) {
        T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Activation of servo failed");
        return VTSS_RC_ERROR;
    }
    T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "activate of MICROSEMI servo domain %d, clock_inst %d, filter_type %d", domain, clock_inst, default_ds_conf->filter_type);

    // Increment activate count (must take place before call to clock_adjtimer_enable_request as in some cases this will otherwise not work correctly).
    activate_count++;

    /* Call function to show some parameters of the server 1Hz config to CLI */
    //apr_show_server_1hz_config(zl303xx_Params[domain], clock_inst);

    T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "activate of MICROSEMI servo was called - activate count is now %d", activate_count);
    return VTSS_RC_OK;
}

void ptp_ms_servo::deactivate(uint32_t domain)
{
    // If the last PTP stream is deleted then make sure the DPLL is put in electrical mode
    if (activate_count == 1) {
        //bool grant_enable;
        //if (zl30380_apr_set_active_elec_ref(0) != VTSS_RC_OK) {
        //    T_W("Could not set electrical reference mode");
        //} else {
        //    T_W("APR set electrical reference mode");
        //}
    }

    // Decrement activate count (must take place after call to clock_adjtimer_enable_request as in some cases this will otherwise not work correctly).
    activate_count--;

    // Remove the timing server for this PTP clock instance
    apr_stream_remove(domain, clock_inst);

    T_IG(VTSS_TRACE_GRP_PTP_BASE_FILTER, "Clock inst %d, deactivate of MICROSEMI servo was called - activate count is now %d", clock_inst, activate_count);
}

mesa_rc ptp_ms_servo::set_hybrid_transient(uint32_t domain, vtss_ptp_hybrid_transient state)
{
    if (activate_count > 0) {
        zl_30380_apr_set_hybrid_transient(domain, clock_inst, state);
    } else {
        T_D("Tried to signal a hybrid transient while no 3038x instance present.");
    }

    return VTSS_RC_OK;
}

mesa_rc ptp_ms_servo::switch_1pps_virtual_ref(ptp_clock_t *ptp_clk, uint16_t instance, uint32_t domain, bool enable, bool warm_start)
{
    if (activate_count > 0) {
        zl_30380_apr_switch_1pps_virtual_reference(instance, domain, enable, warm_start);
    } else {
        T_D(" no 3038x instance present");
    }
    return VTSS_RC_OK;
}

mesa_rc ptp_ms_servo::set_device_1pps_virtual_ref(ptp_clock_t *ptp_clk, uint16_t inst, uint32_t domain, bool enable)
{
    return zl_30380_virtual_port_device_set(inst, domain, enable);
}
#endif // defined(VTSS_SW_OPTION_ZLS30387)
