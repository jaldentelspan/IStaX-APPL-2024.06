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

/* Implement the 1pps synchronization feature */
/*lint -esym(766, vtss/api/options.h) */

#include "main.h"
#include "ptp.h"
#include "ptp_1pps_sync.h"
#include "ptp_api.h"
#include "vtss_ptp_sys_timer.h"
#include "vtss_tod_mod_man.h"
#include "vtss_tod_api.h"
#include "misc_api.h"

#include "critd_api.h"

#ifdef VTSS_SW_OPTION_PHY
#include "phy_api.h"
#endif

#define API_INST_DEFAULT PHY_INST
#define PTP_1PPS_DEFAULT_PPS_WIDTH 1000

#define VTSS_PTP_1PPS_MEASURE_TIME 10   /* the number of 1 sec periods for measuring the ri_time */
#define VTSS_PTP_1PPS_SETTLE_TIME   1   /* the number of 1 sec periods to pause the measurement after setting the to_main */

#define VTSS_PTP_1PPS_TIMEINTERVAL 32 /* unit is 1/128 SEC */
/*
 * Serial TOD mode.
 */
typedef enum {
    VTSS_PTP_1PPS_STATE_PASSIVE,        /* Statemachine is passive */
    VTSS_PTP_1PPS_STATE_OUTPUT_ONLY,    /* Statemachine is in output only mode */
    VTSS_PTP_1PPS_MEASURE,              /* Main output mode measuring cable delay */
    VTSS_PTP_1PPS_SETTLE,               /* Main output mode settlng after 1pps output phase adjustment */
    VTSS_PTP_1PPS_SLAVE,                /* Sub mode main state */
} one_pps_state_t;

/* 1pps phy port configuration data
 */
typedef struct {
    /* private data */
    vtss_1pps_sync_conf_t conf;
    one_pps_state_t  my_state;
    i32 to_main;            /* calculated or manually entered  1pps output phase (negative value  => 1PPS is output before the 1 sec wrap*/
    u32 pps_width_adj;
    i32 cable_asy;          /* manually entered cable asymmetry */
    vtss_1pps_ser_tod_mode_t serial_tod;
    u32 repeat;
    i32 ri_main;            /* negative value => ri goes high before the 1 sec wrap */
    /* timer for transmission of Sync messages*/
    vtss_ptp_sys_timer_t pps_timer;
    int port;
    mepa_timestamp_t phy_ts;
    u32 saved_timeout;
    
} one_pps_sync_entry_t;

static CapArray<one_pps_sync_entry_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> one_pps_sync_data;

#define PTP_1PPS_LOCK()        critd_enter(&datamutex, __FILE__, __LINE__)
#define PTP_1PPS_UNLOCK()      critd_exit (&datamutex, __FILE__, __LINE__)
static critd_t datamutex;          /* Global data protection */

static void ptp_1pps_sync_timer(vtss_ptp_sys_timer_t *timer, void *t);

static void phy_ts_2_ptp_ts(mesa_timestamp_t *ts, const mepa_timestamp_t *phy_ts)
{
    ts->sec_msb = phy_ts->seconds.high;
    ts->seconds = phy_ts->seconds.low;
    ts->nanoseconds = phy_ts->nanoseconds;
}

static void mepa_to_vtss_phy_ts(const mepa_timestamp_t *mp_ts, vtss_phy_timestamp_t *const ms_ts)
{
    ms_ts->seconds.high = mp_ts->seconds.high;
    ms_ts->seconds.low = mp_ts->seconds.low;
    ms_ts->nanoseconds = mp_ts->nanoseconds;
}
// Convert state to text
static const char *state_txt(one_pps_state_t s)
{
    switch (s)
    {
        case VTSS_PTP_1PPS_STATE_PASSIVE:       return("PASSIVE");
        case VTSS_PTP_1PPS_STATE_OUTPUT_ONLY:   return("OUTPUT_ONLY");
        case VTSS_PTP_1PPS_MEASURE:             return("MEASURE");
        case VTSS_PTP_1PPS_SETTLE:              return("SETTLE");
        case VTSS_PTP_1PPS_SLAVE:               return("SLAVE");
        default:                                return("Unknown state");
    }
}

/******************************************************************************/
// ptp_1pps_sync_init()
// Initialize the internal data structure for handling the 1pps synchronization feature
// 
/******************************************************************************/
mesa_rc ptp_1pps_sync_init(void)
{
    mesa_rc rc = VTSS_RC_OK;
    mesa_port_no_t port_no;
    for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
        memset(&one_pps_sync_data[port_no], 0, sizeof(one_pps_sync_entry_t));
        one_pps_sync_data[port_no].conf.mode = VTSS_PTP_1PPS_SYNC_DISABLE;
    }

    critd_init(&datamutex, "ptp.1pps_sync", VTSS_MODULE_ID_TOD, CRITD_TYPE_MUTEX);
    T_IG(VTSS_TRACE_GRP_PHY_1PPS,"ptp_1pps_sync initialization");
    return rc;
}

mesa_rc ptp_1pps_sync_mode_get(mesa_port_no_t port_no, vtss_1pps_sync_conf_t *conf)
{
    mesa_rc rc = VTSS_RC_OK;
    PTP_1PPS_LOCK();
    *conf = one_pps_sync_data[port_no].conf;
    PTP_1PPS_UNLOCK();
    return rc;
}


/******************************************************************************/
// ptp_1pps_sync_mode_set()
// Set the mode for a 1PPS synchronization on a PHY Gen2 port
// If the feature is supported by  port, then the statemachine is enabled
// Else return an error code
// If mode == MAIN_MAN State = OUTPUT_ONLY
// If mode == MAIN_AUTO State = MEASURE
// If mode == SUB State = SLAVE
// 
/******************************************************************************/
mesa_rc ptp_1pps_sync_mode_set(mesa_port_no_t port_no, const vtss_1pps_sync_conf_t *conf)
{
    mesa_rc rc = VTSS_RC_OK;
    mesa_rc get_time_rc = VTSS_RC_OK;
    vtss_tod_ts_phy_topo_t phy_topo;
    tod_mod_man_port_data_get(port_no, &phy_topo);
    vtss_phy_ts_alt_clock_mode_t phy_alt_clock_mode;
    mepa_ts_pps_conf_t phy_pps_config;
    vtss_phy_ts_sertod_conf_t sertod_conf; /* enable/disable serial TOD input/output */
    one_pps_sync_entry_t *sync_entry;
    mepa_timestamp_t phy_ts = {{0,0},0};

    sertod_conf.ip_enable = false;
    sertod_conf.op_enable = false;
    sertod_conf.ls_inv = false;
    
    PTP_1PPS_LOCK();
    T_IG(VTSS_TRACE_GRP_PHY_1PPS,"port_no %d, mode %d, pulse_delay %d, cable_asy %d, serial_tod %d ", iport2uport(port_no), conf->mode, conf->pulse_delay, conf->cable_asy, conf->serial_tod);
    if (phy_topo.ts_gen == VTSS_PTP_TS_GEN_2) {
        /* default loopback conf: all disabled */
        phy_alt_clock_mode.pps_ls_lpbk = false;
        phy_alt_clock_mode.ls_lpbk = false;
        phy_alt_clock_mode.ls_pps_lpbk = false;

        sync_entry = &one_pps_sync_data[port_no];
        sync_entry->port = port_no;
        sync_entry->conf = *conf;
        sync_entry->pps_width_adj = PTP_1PPS_DEFAULT_PPS_WIDTH;
        phy_pps_config.pps_width_adj = sync_entry->pps_width_adj;
        phy_pps_config.pps_offset = sync_entry->to_main;
        /* initialize timer  */
        vtss_ptp_timer_init(&sync_entry->pps_timer, "pps", port_no, ptp_1pps_sync_timer, sync_entry);
        
        switch (conf->mode) {
            case VTSS_PTP_1PPS_SYNC_MAIN_MAN:
                sync_entry->to_main = conf->pulse_delay;
                phy_pps_config.pps_offset = sync_entry->to_main;
                sync_entry->my_state = VTSS_PTP_1PPS_STATE_OUTPUT_ONLY;
                /* set up HW configuration */
                sertod_conf.op_enable = conf->serial_tod != VTSS_PTP_1PPS_SER_TOD_DISABLE ? true : false;
                break;
            case VTSS_PTP_1PPS_SYNC_MAIN_AUTO:
                sync_entry->cable_asy = conf->cable_asy;
                sync_entry->repeat = 0;
                sync_entry->ri_main = 0;

                sync_entry->my_state = VTSS_PTP_1PPS_MEASURE;
                vtss_ptp_timer_start(&sync_entry->pps_timer, VTSS_PTP_1PPS_TIMEINTERVAL, true);
                /* set up HW configuration */
                sertod_conf.op_enable = conf->serial_tod != VTSS_PTP_1PPS_SER_TOD_DISABLE ? true : false;
                phy_pps_config.pps_offset = 0;
                break;
            case VTSS_PTP_1PPS_SYNC_SUB:
                sync_entry->serial_tod = conf->serial_tod;
                sync_entry->my_state = VTSS_PTP_1PPS_SLAVE;
                vtss_ptp_timer_start(&sync_entry->pps_timer, VTSS_PTP_1PPS_TIMEINTERVAL, true);
                /* set up HW configuration */
                sertod_conf.ip_enable = conf->serial_tod != VTSS_PTP_1PPS_SER_TOD_DISABLE ? true : false;
                phy_alt_clock_mode.ls_pps_lpbk = true;
                /* take the PHY out os the 1PPS synchronization process */
                rc = tod_mod_man_force_slave_state(port_no, false);
                if (VTSS_RC_OK != (get_time_rc = meba_phy_ts_ltc_get(board_instance, port_no, &sync_entry->phy_ts))) T_NG(VTSS_TRACE_GRP_PHY_1PPS,"%s", error_txt(get_time_rc));
                PTP_RC(meba_phy_ts_ltc_ls_en(board_instance, port_no, MEPA_TS_CMD_SAVE));
                sync_entry->saved_timeout = 0;
                /* enable the save timeofday */
                PTP_RC(meba_phy_ts_ltc_ls_en(board_instance, port_no, MEPA_TS_CMD_SAVE));
                T_DG(VTSS_TRACE_GRP_PHY_1PPS,"port_no %d, new time %d sec", iport2uport(port_no), sync_entry->phy_ts.seconds.low);
                if (conf->serial_tod == VTSS_PTP_1PPS_SER_TOD_AUTO) {
                    /* enable auto load, i.e. serial TOD is loaded */
                    PTP_RC(meba_phy_ts_ltc_set(board_instance, port_no, &phy_ts));
                } else {
                    /* clear load enable, i.e. serial TOD is not loaded */
                    PTP_RC(meba_phy_ts_ltc_ls_en(board_instance, port_no, MEPA_TS_ADJ_CMD_CLEAR));
                }
                
                break;
            case VTSS_PTP_1PPS_SYNC_DISABLE:
                sync_entry->my_state = VTSS_PTP_1PPS_STATE_PASSIVE;
                /* set up HW configuration */
                /* take the PHY back to the 1PPS synchronization process */
                rc = tod_mod_man_force_slave_state(port_no, true);
                break;
            default: rc = PTP_RC_UNSUPPORTED_1PPS_OPERATION_MODE;
                break;
        }
        if (VTSS_RC_OK == rc) rc = vtss_phy_ts_alt_clock_mode_set(API_INST_DEFAULT, port_no, &phy_alt_clock_mode);
        if (VTSS_RC_OK == rc) rc = meba_phy_ts_pps_conf_set(board_instance, port_no, &phy_pps_config);
        if (VTSS_RC_OK == rc) rc = vtss_phy_ts_sertod_set(API_INST_DEFAULT, port_no, &sertod_conf);
        T_IG(VTSS_TRACE_GRP_PHY_1PPS,"state %s, rc %s", state_txt(sync_entry->my_state), error_txt(rc));
    } else {
        rc = PTP_RC_MISSING_PHY_TIMESTAMP_RESOURCE;
    }
    PTP_1PPS_UNLOCK();
    return rc;
}

/******************************************************************************/
// ptp_1pps_sync_timer()
// Handle the 1PPS sync every sec.
// Traverse all active 1PPS functions, and do update accordingly
// Each function has a statemachine:
// State        Action
// PASSIVE      Do nothing
// OUTPUT_ONLY  Do nothing
// MEASURE      read ri_main; if (repeat++ > 10) {calculate average; write to to_main; State = SETTLE}
// SETTLE       if (repeat++ > 1) State = MEASURE
// SLAVE        if (SER_TOD_MAN mode) t2 = read saved time;  t1 = TOD time; Call PTP slave function.
//              if (SER_TOD_DISABLE mode) t2 =read saved time; t1.sec = current sec; t1.ns = 0; Call PTP slave function.
//              if (SER_TOD_AUTO mode) read saved time; Calculate freq. offset; make asjustment.
//
// t1 = serial_tod ? 
// 
/******************************************************************************/
/*lint -esym(459, ptp_1pps_sync_timer) */
static void ptp_1pps_sync_timer(vtss_ptp_sys_timer_t *timer, void *t)
{
    int port_no;
    u32 tmp;
    i32 ri_reg;
    mesa_rc rc = VTSS_RC_OK;
    one_pps_sync_entry_t *sync_entry = (one_pps_sync_entry_t *)t;
    mepa_ts_pps_conf_t phy_pps_config;
    vtss_ptp_timestamps_t ts = {};
    mepa_timestamp_t phy_ts = {};
    char str0 [50], str1[50];
    mesa_timeinterval_t offset;
    
    PTP_1PPS_LOCK();
    port_no = sync_entry->port;
    T_NG(VTSS_TRACE_GRP_PHY_1PPS,"ptp_1pps_sync timer, state = %s", state_txt(sync_entry->my_state));
    if (sync_entry->my_state != VTSS_PTP_1PPS_STATE_PASSIVE) {
        switch (sync_entry->my_state) {
            case VTSS_PTP_1PPS_MEASURE:              /* Main output mode measuring cable delay */
                /* read ri_main, and convert to signed (e.g. 999999994 => -6)
                   this is needed to be able to calculate an average,because the value can vary around 0 */
                if (VTSS_RC_OK == vtss_phy_ts_alt_clock_saved_get(API_INST_DEFAULT, port_no, &tmp)) {
                    ri_reg = (tmp > VTSS_ONE_MIA/2) ? tmp - VTSS_ONE_MIA : tmp;
                    sync_entry->ri_main += ri_reg;
                    T_DG(VTSS_TRACE_GRP_PHY_1PPS,"ri_reg %d", ri_reg);
                    if (++sync_entry->repeat >= VTSS_PTP_1PPS_MEASURE_TIME) {
                        sync_entry->repeat = 0;
                        sync_entry->my_state = VTSS_PTP_1PPS_SETTLE;
                        sync_entry->to_main = (sync_entry->ri_main/VTSS_PTP_1PPS_MEASURE_TIME + sync_entry->to_main)/2
                                               + sync_entry->cable_asy;
                        phy_pps_config.pps_width_adj = sync_entry->pps_width_adj;
                        phy_pps_config.pps_offset = -sync_entry->to_main + (sync_entry->to_main > 0 ? VTSS_ONE_MIA : 0);
                        rc = meba_phy_ts_pps_conf_set(board_instance, port_no, &phy_pps_config);
                        T_IG(VTSS_TRACE_GRP_PHY_1PPS,"to_main %d, ri_main %d, pps_offset %d, rc %s", sync_entry->to_main,
                             sync_entry->ri_main/VTSS_PTP_1PPS_MEASURE_TIME, phy_pps_config.pps_offset, error_txt(rc));
                    }
                }
                break;
            case VTSS_PTP_1PPS_SETTLE:               /* Main output mode settlng after 1pps output phase adjustment */
                /* */
                if (VTSS_RC_OK == vtss_phy_ts_alt_clock_saved_get(API_INST_DEFAULT, port_no, &tmp)) {
                    /* the saved clock is not used, but we have to wait until the register has been updated before starting on a new measurement */
                    if (++sync_entry->repeat >= VTSS_PTP_1PPS_SETTLE_TIME) {
                        sync_entry->repeat = 0;
                        sync_entry->ri_main = 0;
                        sync_entry->my_state = VTSS_PTP_1PPS_MEASURE;
                        T_IG(VTSS_TRACE_GRP_PHY_1PPS,"port_no %d end of settle", iport2uport(port_no));
                    }
                }
                break;
            case VTSS_PTP_1PPS_SLAVE:                /* Sub mode main state */
                ++sync_entry->saved_timeout;
                if (VTSS_RC_OK == meba_phy_ts_ltc_get(board_instance, port_no, &phy_ts)) {
                    T_NG(VTSS_TRACE_GRP_PHY_1PPS,"slave");
                    phy_ts_2_ptp_ts(&ts.rx_ts, &phy_ts);
                    T_DG(VTSS_TRACE_GRP_PHY_1PPS,"port_no %d, t2 %s", iport2uport(port_no), TimeStampToString(&ts.rx_ts, str1));
                    PTP_RC(meba_phy_ts_ltc_ls_en(board_instance, port_no, MEPA_TS_CMD_SAVE));
                    if (phy_ts.seconds.low != sync_entry->phy_ts.seconds.low) {
                        sync_entry->saved_timeout = 0;
                        sync_entry->phy_ts = phy_ts;
                        phy_ts_2_ptp_ts(&ts.rx_ts, &phy_ts);
                        switch (sync_entry->serial_tod) {
                            case VTSS_PTP_1PPS_SER_TOD_MAN:
                                /* read saved time and transmitted time */
                                vtss_phy_timestamp_t ms_ts;
                                mepa_to_vtss_phy_ts(&phy_ts, &ms_ts);
                                if (VTSS_RC_OK == vtss_phy_ts_load_ptptime_get(API_INST_DEFAULT, port_no, &ms_ts)) {
                                    phy_ts_2_ptp_ts(&ts.tx_ts, &phy_ts);
                                    /* the time from Serial TOD is 1 sec ahead, because it is the time to be loaded at next 1PPS */
                                    ts.corr = 0LL;
                                    T_IG(VTSS_TRACE_GRP_PHY_1PPS,"port_no %d, received TOD %s, Saved LTC %s", iport2uport(port_no), TimeStampToString(&ts.tx_ts, str0), TimeStampToString(&ts.rx_ts, str1));
                                    //ptp_1pps_ptp_slave_t1_t2_rx(port_no, &ts);
                                }
                                break;
                            case VTSS_PTP_1PPS_SER_TOD_AUTO:
                                /* read saved time and transmitted time */
                                //if (VTSS_RC_OK == meba_phy_ts_load_ptptime_get(board_instance, port_no, &phy_ts)) {
                                //    phy_ts_2_ptp_ts(&ts.tx_ts, &phy_ts);
                                T_IG(VTSS_TRACE_GRP_PHY_1PPS,"port_no %d, rx_ts %s", iport2uport(port_no), TimeStampToString(&ts.rx_ts, str1));
                                    //ptp_1pps_ptp_slave_t1_t2_rx(port_no, &ts);
                                //}
                                break;
                            case VTSS_PTP_1PPS_SER_TOD_DISABLE:
                                /* read saved time, transmitted time.sec = rx time, .nanosec = 0 */
                                T_DG(VTSS_TRACE_GRP_PHY_1PPS,"port_no %d, new time %d sec, old time %d", iport2uport(port_no), phy_ts.seconds.low, sync_entry->phy_ts.seconds.low);
                                ts.tx_ts.sec_msb = ts.rx_ts.sec_msb;
                                ts.tx_ts.seconds = ts.rx_ts.seconds;
                                ts.tx_ts.nanoseconds = 0;
                                ts.corr = 0LL;
                                T_IG(VTSS_TRACE_GRP_PHY_1PPS,"port_no %d, rx_ts %s", iport2uport(port_no), TimeStampToString(&ts.rx_ts, str1));
                                T_DG(VTSS_TRACE_GRP_PHY_1PPS,"port_no %d, t1 %s, t2 %s", iport2uport(port_no), TimeStampToString(&ts.tx_ts, str0), TimeStampToString(&ts.rx_ts, str1));
                                vtss_tod_sub_TimeInterval(&offset, &ts.rx_ts, &ts.tx_ts);
                                T_DG(VTSS_TRACE_GRP_PHY_1PPS,"port_no %d, offset %s", iport2uport(port_no), vtss_tod_TimeInterval_To_String(&offset, str0,'.'));
                                
                                PTP_RC(vtss_phy_ts_timeofday_offset_set(API_INST_DEFAULT, port_no, (i32)(offset>>16)));
                                //ptp_1pps_ptp_slave_t1_t2_rx(port_no, &ts);
                                
                                break;
                            default:
                                break;
                        }
                    }
                } else {
                    T_NG(VTSS_TRACE_GRP_PHY_1PPS,"fail");
                }
                if (sync_entry->saved_timeout > 10) {
                    /* saved time timeout */
                    sync_entry->saved_timeout = 0;
                    PTP_RC(meba_phy_ts_ltc_ls_en(board_instance, port_no, MEPA_TS_CMD_SAVE));
                }
                break;
            default:
                break;
        }
    }
    PTP_1PPS_UNLOCK();
}
