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

/* vtss_tod_mod_man.c */
/*lint -esym(766, vtss/api/options.h) */
/*lint -esym(766, vtss_silabs_clk_api.h) */

#include "microchip/ethernet/board/api.h"
#include "main.h"

#include "vtss_os_wrapper.h"
#include "vtss_tod_mod_man.h"
#include "vtss_tod_api.h"
#include "tod.h"
#include "interrupt_api.h"
#if defined VTSS_SW_OPTION_PTP
#include "ptp_api.h"
#endif
#if defined(VTSS_SW_OPTION_PHY)
#include "phy_api.h"
#endif
#include "port_api.h"
#include "port_iter.hxx"

#include "vtss_silabs_clk_api.h"
/* include pcos interface */
#include "critd_api.h"
#include "tod_api.h"
#include <vtss/basics/ringbuf.hxx>

#define API_INST_DEFAULT PHY_INST
/* Note: The serial TOD feature in Gen2 phy's only works properly if the PHY_CONF_AUTO_CLEAR_LS = TRUE */
#define PHY_CONF_AUTO_CLEAR_LS = TRUE           /* TRUE if auto clear of Load/Save in the gen2 phy's are enabled */

/****************************************************************************
 * PTP Module Manager thread
 ****************************************************************************/

static vtss_handle_t mod_man_thread_handle;
static vtss_thread_t mod_man_thread_block;


#define CTLFLAG_PTP_MOD_MAN_1PPS           (1 << 0)
#define CTLFLAG_PTP_MOD_MAN_SETTING_TIME   (1 << 1)

static vtss_flag_t   module_man_thread_flags; /* PTP module manager thread control */

static critd_t mod_man_mutex;          /* clock internal data protection */

#define MOD_MAN_LOCK()   critd_enter(&mod_man_mutex, __FILE__, __LINE__)
#define MOD_MAN_UNLOCK() critd_exit (&mod_man_mutex, __FILE__, __LINE__)

static void tx_ts_in_sync_call_out(mesa_port_no_t port_no, BOOL in_sync);
static mesa_rc port_slave_start_all(void);

/**
 * \brief Module manager add a slave timecounter.
 * \note this function is used to manage a list of slave timecounters that are adjusted simultaneously.
 *
 * \param time_cnt [IN]  Timecounter type, currently only VTSS_TS_TIMECOUNTER_PHY is implemented.
 * \param port_no  [IN]  port number that the timecounter is assigned to.
 * \param one_pps_load_latency [IN]  The latency from master 1PPS edge until the internal timer in the PHY is
 *                              loaded (the internal latency in the Phy is 2 clock cycles.
 * \param one_pps_save_latency [IN]  The latency from master 1PPS edge until the internal timer in the PHY is
 *                              saved (the internal latency in the Phy is 1 clock cycles.
 *
 * \return Return code.
 **/
static mesa_rc port_slave_set(mesa_port_no_t port_no,
                              mesa_timeinterval_t one_pps_load_latency,
                              mesa_timeinterval_t one_pps_save_latency);


/****************************************************************************
 * Module Manager port data used to maintain the list of ports that supports
 * PHY timestamping
 ****************************************************************************/

typedef struct {
    vtss_tod_ts_phy_topo_t topo; /* Phy timestamp topology info */
    BOOL phy_init;               /* true when the PHY timestamper for the port has been initialized */
} port_data_t;

static CapArray<port_data_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_data;
static mesa_timestamp_t load_ts;
static bool set_time = false;

static const i64 clk_mhz[MEPA_TS_CLOCK_FREQ_MAX] = {
    [MEPA_TS_CLOCK_FREQ_25M] =    25000000LL,
    [MEPA_TS_CLOCK_FREQ_125M] =   125000000LL,
    [MEPA_TS_CLOCK_FREQ_15625M] = 156250000LL,
    [MEPA_TS_CLOCK_FREQ_200M] =   200000000LL,
    [MEPA_TS_CLOCK_FREQ_250M] =   250000000LL,
    [MEPA_TS_CLOCK_FREQ_500M] =   500000000LL};

static void phy_ts_internal_mode_get(mepa_ts_rx_timestamp_len_t *ts_len, mepa_ts_tc_op_mode_t *tc_mode)
{
    mesa_packet_internal_tc_mode_t mode;
    if (tod_tc_mode_get(&mode)) {
        switch (mode) {
            case MESA_PACKET_INTERNAL_TC_MODE_30BIT:
                *ts_len = MEPA_TS_RX_TIMESTAMP_LEN_30BIT;
                *tc_mode = MEPA_TS_TC_OP_MODE_A;
                break;
            case MESA_PACKET_INTERNAL_TC_MODE_32BIT:
                *ts_len = MEPA_TS_RX_TIMESTAMP_LEN_32BIT;
                *tc_mode = MEPA_TS_TC_OP_MODE_A;
                break;
            case MESA_PACKET_INTERNAL_TC_MODE_44BIT:
                *ts_len = MEPA_TS_RX_TIMESTAMP_LEN_30BIT;
                *tc_mode = MEPA_TS_TC_OP_MODE_B;
                break;
            case MESA_PACKET_INTERNAL_TC_MODE_48BIT:
                *ts_len = MEPA_TS_RX_TIMESTAMP_LEN_30BIT;
                *tc_mode = MEPA_TS_TC_OP_MODE_C;
                T_IG(VTSS_TRACE_GRP_PHY_TS, "Mode 48BIT only supported in Gen2, Gen3 PHYs");
                break;
            default:
                *ts_len = MEPA_TS_RX_TIMESTAMP_LEN_30BIT;
                *tc_mode = MEPA_TS_TC_OP_MODE_A;
                T_WG(VTSS_TRACE_GRP_PHY_TS, "Mode not supported");
                break;
        }
    }
}

static void port_data_set(mesa_port_no_t port_no)
{
    BOOL phy_ts_port = FALSE;
    mesa_rc rc = VTSS_RC_OK;
    mepa_ts_init_conf_t  phy_conf = {};
    char str1 [30];
    char str2 [30];
    mepa_timeinterval_t one_pps_load_latency = 0;
    mepa_timeinterval_t one_pps_save_latency = 0;
    mepa_ts_clock_freq_t clk_freq = {};
    mepa_ts_clock_src_t clk_src = {};
    mepa_ts_rx_timestamp_len_t ts_len = (mepa_ts_rx_timestamp_len_t)0;
    mepa_ts_tc_op_mode_t tc_mode = (mepa_ts_tc_op_mode_t)0;
    mepa_phy_info_t mepa_phy_info = {};

    rc = meba_phy_ts_init_conf_get(board_instance, port_no, &phy_conf);
    if (rc == MESA_RC_OK) {
        rc = meba_phy_info_get(board_instance, port_no, &mepa_phy_info);
        if (rc != MESA_RC_OK) {
            T_WG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, meba_phy_info_get failed, rc = %x", port_no, rc);
        }
    }
    if (mepa_phy_ts_cap()) {
        phy_ts_internal_mode_get(&ts_len, &tc_mode);
        T_IG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, ts_len %d, tc_mode %d", port_no, ts_len, tc_mode);
        // Lan8814 supports only TC mode C.
        if (mepa_phy_info.part_number == 8814) {
            tc_mode = MEPA_TS_TC_OP_MODE_C;
        }
    }


    if (rc == MESA_RC_OK) {
        // MEPA PHY TS driver hooked up.
        phy_ts_port = TRUE;
        (void)meba_tod_phy_ts_clk_info(board_instance, port_no, &clk_freq, &clk_src, &one_pps_load_latency, &one_pps_save_latency);
        if (!port_data[port_no].phy_init) {
            port_data[port_no].topo.ts_feature = VTSS_PTP_TS_PTS;

            if (mepa_phy_info.cap & MEPA_CAP_TS_MASK_GEN_3) {
                port_data[port_no].topo.ts_gen = VTSS_PTP_TS_GEN_3;
            } else if (mepa_phy_info.cap & MEPA_CAP_TS_MASK_GEN_2) {
                port_data[port_no].topo.ts_gen = VTSS_PTP_TS_GEN_2;
            } else if (mepa_phy_info.cap & MEPA_CAP_TS_MASK_GEN_1) {
                port_data[port_no].topo.ts_gen = VTSS_PTP_TS_GEN_1;
            } else {
                port_data[port_no].topo.ts_gen = VTSS_PTP_TS_GEN_NONE;
            }

            memset(&phy_conf, 0,sizeof(mepa_ts_init_conf_t));
            phy_conf.clk_freq = clk_freq;
            phy_conf.clk_src = clk_src;
            phy_conf.rx_ts_pos = MEPA_TS_RX_TIMESTAMP_POS_IN_PTP;
            phy_conf.rx_ts_len = ts_len;
            phy_conf.tx_fifo_mode = MEPA_TS_FIFO_MODE_NORMAL;
            phy_conf.tx_ts_len = MEPA_TS_FIFO_TIMESTAMP_LEN_10BYTE;
            phy_conf.tc_op_mode = tc_mode;
            if (fast_cap(MESA_CAP_TS_DELAY_REQ_AUTO_RESP)) {
                if (port_data[port_no].topo.ts_gen == VTSS_PTP_TS_GEN_1 || port_data[port_no].topo.ts_gen == VTSS_PTP_TS_GEN_2) {
                    phy_conf.dly_req_recv_10byte_ts = TRUE;
                }
                T_DG(VTSS_TRACE_GRP_PHY_TS, "port = %d delay_req ts %d ts_gen %d", port_no, phy_conf.dly_req_recv_10byte_ts, port_data[port_no].topo.ts_gen);
            }
            rc = meba_phy_ts_init_conf_set(board_instance, port_no, &phy_conf);
            port_data[port_no].phy_init = TRUE;
        }
        rc = port_slave_set(port_no, one_pps_load_latency, one_pps_save_latency);
        T_IG(VTSS_TRACE_GRP_PHY_TS, "Port_no = %d, added to slave counter list, latency = %s/%s, rc = %x", port_no,
             vtss_tod_TimeInterval_To_String(&one_pps_load_latency,str1,'.'),
             vtss_tod_TimeInterval_To_String(&one_pps_save_latency,str2,'.'), rc);
    } else {
        port_data[port_no].topo.port_ts_in_sync = TRUE;
        port_data[port_no].topo.ts_feature = VTSS_PTP_TS_NONE;
    }

    // Set-up back-plane mode in switch for ports having phy timestamping. Ref: TN1257
    // TODO: Disable for now as MESA_TS_MODE_INTERNAL is disabled in mesa.
    if (phy_ts_port && fast_cap(MESA_CAP_TS_INTERNAL_MODE_SUPPORTED)) {
        mesa_ts_operation_mode_t mode;
        mode.mode = MESA_TS_MODE_INTERNAL; /* set the backplane mode if TS PHY exists for the port*/
        mode.domain = 0;
        TOD_RC(mesa_ts_operation_mode_set(NULL, port_no, &mode));
    }
}

static void port_data_init(void)
{
    int i;
    port_iter_t       pit;
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        i = pit.iport;
        port_data[i].topo.port_ts_in_sync = TRUE;
        port_data[i].topo.ts_gen = VTSS_PTP_TS_GEN_NONE;
        port_data[i].topo.ts_feature = VTSS_PTP_TS_NONE;
        port_data[i].topo.channel_id = 0;
        port_data[i].topo.port_shared = FALSE;
        port_data[i].topo.shared_port_no = 0;
        port_data[i].topo.port_ts_in_sync = TRUE;
        port_data[i].phy_init = FALSE;
    }
}

void tod_mod_man_port_data_get(mesa_port_no_t port_no,
                         vtss_tod_ts_phy_topo_t *phy)
{
    MOD_MAN_LOCK();
    *phy = port_data[port_no].topo;
    MOD_MAN_UNLOCK();
}

/****************************************************************************
 * Module Manager state machine
 ****************************************************************************/


typedef struct {
    mesa_rc (*tod_set)    ( const meba_inst_t           inst,
                            const mepa_port_no_t        port_no,
                            const mepa_timestamp_t  *const ts) ;

    mesa_rc (*tod_set_done)(const meba_inst_t     inst,
                            const mepa_port_no_t  port_no,
                            mepa_ts_ls_type_t     ls_type);
    mesa_rc (*tod_arm)    ( const meba_inst_t     inst,
                            const mepa_port_no_t  port_no,
                            mepa_ts_ls_type_t     ls_type);
    mesa_rc (*tod_get)    ( const meba_inst_t     inst,
                            const mepa_port_no_t  port_no,
                            mepa_timestamp_t  *const ts);
    mesa_rc (*tod_adj1ns) ( const meba_inst_t     inst,
                            const mepa_port_no_t  port_no,
                            const mepa_bool_t     incr);
    mesa_rc (*tod_rateadj)( const meba_inst_t              inst,
                            const mepa_port_no_t           port_no,
                            const mepa_ts_scaled_ppb_t *const adj);
    mesa_rc (*ts_mode_set)( const meba_inst_t     inst,
                            const mepa_port_no_t  port_no,
                            const mepa_bool_t     enable);
    mesa_rc (*tod_fifo_empty)(const meba_inst_t       inst,
                            const mepa_port_no_t    port_no);

    meba_inst_t           inst;
    mesa_port_no_t        port_no;
    mod_man_state_t       my_state;
    mepa_timestamp_t  current_slave_time;
    mepa_timestamp_t  time_at_next_pps;
    mepa_timeinterval_t          one_pps_load_latency;
    mepa_timeinterval_t          one_pps_save_latency;
    BOOL                  in_sync;
    BOOL                  in_sync_reported_once;
    i64                   slave_skew;
    i64                   slave_skew_save;

    uint32_t              out_of_sync_cnt; // After attaining in-sync state once, this is the count of phy and switch LTC time going out of sync.
    i64                   oos_skew;        // Maximum skew observed between phy and switch LTC during last out of sync event.
} mod_man_slave_table_t;

#define MOD_MAN_SLAVE_TABLE_SIZE 56
#define MOD_MAN_USER_TABLE_SIZE 2
#define SLAVE_MAX_SKEW              400      /* max LTC slave skew that can be corrected by 1nsadj */

static int mod_man_thread_running = MOD_MAN_NOT_INSTALLED;
static vtss_tick_count_t mod_man_thread_init_end_time;
static mod_man_slave_table_t mod_man_slave_table[MOD_MAN_SLAVE_TABLE_SIZE];

static  vtss_module_man_in_sync_cb_t my_in_sync_cb [MOD_MAN_USER_TABLE_SIZE];

static mepa_ts_scaled_ppb_t my_adj = 0; /* actual adjustment rate */

static mesa_timeinterval_t my_1pps_latency = 0; /* actual master 1pps latency */

/* forward declarations */
static void phy_ts_interrupt_handler(meba_event_t     source_id,
        u32                         instance_id);

static void
mod_man_main_thread(vtss_addrword_t data)
{

    mepa_timestamp_t time_at_next_load;
    mepa_timestamp_t slave_time;
    mesa_timestamp_t      ts;
    mesa_timestamp_t      prev_ts;
    vtss_tick_count_t wakeup = vtss_current_time() + VTSS_OS_MSEC2TICK(6000); /* wait 6 sec before monitoring */
    vtss_flag_value_t flags;
    int slave_idx;
    int tick_cnt = 0;
#if defined VTSS_SW_OPTION_PTP
    vtss_appl_ptp_ext_clock_mode_t ext_clk_mode;
#endif
    mesa_timeinterval_t my_latency ;

    port_phy_wait_until_ready(); /* wait until the phy's are initialized */
    /* do initialization of interrupt handling */
    TOD_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_TOD,
                                          phy_ts_interrupt_handler,
                                          MEBA_EVENT_EGR_TIMESTAMP_CAPTURED,
                                          INTERRUPT_PRIORITY_NORMAL));
    TOD_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_TOD,
                                          phy_ts_interrupt_handler,
                                          MEBA_EVENT_EGR_FIFO_OVERFLOW,
                                          INTERRUPT_PRIORITY_NORMAL));
    T_NG(VTSS_TRACE_GRP_MOD_MAN, "PHY timestamp interrupt installed");

#if !defined VTSS_SW_OPTION_PTP
    /* if the PTP module is not included, then always set the 1PPS output, to be able to synchronize the PHY's */
    mesa_ts_ext_clock_mode_t   ext_clock_mode = {MESA_TS_EXT_CLOCK_MODE_ONE_PPS_OUTPUT, FALSE, 1};
    TOD_RC(mesa_ts_external_clock_mode_set(NULL, &ext_clock_mode));
    T_IG(VTSS_TRACE_GRP_MOD_MAN, "1PPS out enabled");
#endif
#if defined(VTSS_PHY_TS_SILABS_CLK_DLL)
    BOOL silabs_present = module_man_si5326();
    int tries = 0;
    T_IG(VTSS_TRACE_GRP_MOD_MAN, "PHY 250 MHz LTC timer option");
#else
    T_IG(VTSS_TRACE_GRP_MOD_MAN, "PHY 156/125 MHz LTC timer option");
#endif /* VTSS_PHY_TS_SILABS_CLK_DLL */

    wakeup = vtss_current_time() + VTSS_OS_MSEC2TICK(25000); /* wait 25 sec before start monitoring */
    mod_man_thread_init_end_time = wakeup;
    mod_man_thread_running = MOD_MAN_INSTALLED_BUT_NOT_RUNNING;
    T_IG(VTSS_TRACE_GRP_MOD_MAN, "Wait 25 sec before starting monitor");
    flags = vtss_flag_timed_wait(&module_man_thread_flags, 0xffff, VTSS_FLAG_WAITMODE_AND, wakeup);
    T_IG(VTSS_TRACE_GRP_MOD_MAN, "done, starting monitor (flags = %x)", flags);
    port_slave_start_all();
    wakeup =  vtss_current_time() + VTSS_OS_MSEC2TICK(100); /* wait 100 ms */
    mod_man_thread_running = MOD_MAN_INSTALLED_AND_RUNNING;
    
    for (;;) {

        flags = vtss_flag_timed_wait(&module_man_thread_flags, 0xffff, VTSS_FLAG_WAITMODE_OR_CLR, wakeup);

        if (flags) {
#if defined VTSS_SW_OPTION_PTP
            (void) vtss_appl_ptp_ext_clock_out_get(&ext_clk_mode); /* the monitoring is only done when the 1PPS output is enabled */
            T_NG(VTSS_TRACE_GRP_MOD_MAN, "clock mode 1pps %d", ext_clk_mode.clock_out_enable);
            if (ext_clk_mode.one_pps_mode != VTSS_APPL_PTP_ONE_PPS_DISABLE) {
#else
            {
#endif /*  VTSS_SW_OPTION_PTP */
#if defined(VTSS_PHY_TS_SILABS_CLK_DLL)
                if (!silabs_present && tries++ < 2) {
                    silabs_present = module_man_si5326();
                }
#endif /* VTSS_PHY_TS_SILABS_CLK_DLL */
                T_NG(VTSS_TRACE_GRP_MOD_MAN, "flag(s) set %x", flags);
                MOD_MAN_LOCK();
                if (flags & CTLFLAG_PTP_MOD_MAN_SETTING_TIME) {
                    T_DG(VTSS_TRACE_GRP_MOD_MAN, "SETTING_TIME signal received");
                    for (slave_idx = 0; slave_idx < MOD_MAN_SLAVE_TABLE_SIZE; slave_idx++) {
                        switch (mod_man_slave_table[slave_idx].my_state) {
                            case MOD_MAN_NOT_STARTED:
                            case MOD_MAN_DISABLED:
                            case MOD_MAN_SET_ABS_TOD:
                                break;
                            case MOD_MAN_READ_RTC_FROM_SWITCH:
                            case MOD_MAN_SET_ABS_TOD_DONE:
                            case MOD_MAN_READ_RTC_FROM_PHY_AND_COMPARE:
                                T_NG(VTSS_TRACE_GRP_MOD_MAN, "ModuleManager: force state to MOD_MAN_SET_ABD_TOD");
                                    /* disable PTP operation  ( removed because it may cause packet loss)*/
                                //TOD_RC(mod_man_slave_table[slave_idx].ts_mode_set(
                                //           mod_man_slave_table[slave_idx].inst,
                                //           mod_man_slave_table[slave_idx].port_no,
                                //           FALSE));
                                tx_ts_in_sync_call_out(
                                    mod_man_slave_table[slave_idx].port_no, FALSE);
                                mod_man_slave_table[slave_idx].in_sync = FALSE;
                                T_DG(VTSS_TRACE_GRP_MOD_MAN, "port %d now out-of-sync",mod_man_slave_table[slave_idx].port_no);
                                /* set timeofday in master timecounter ?*/
                                mod_man_slave_table[slave_idx].my_state = MOD_MAN_SET_ABS_TOD;
                                break;
                            default:
                                T_WG(VTSS_TRACE_GRP_MOD_MAN, "Undefined ModuleManager state");
                                break;
                        }
                    }
                }
                if (flags & CTLFLAG_PTP_MOD_MAN_1PPS) {
                    T_NG(VTSS_TRACE_GRP_MOD_MAN, "1 PPS signal received");
                    for (slave_idx = 0; slave_idx < MOD_MAN_SLAVE_TABLE_SIZE; slave_idx++) {
                        // Verify time is loaded or not from first port.
                        if (set_time == true) {
                            mesa_timestamp_t ts;
                            uint64_t tc;
                            TOD_RC(mesa_ts_domain_timeofday_get(NULL, 0, &ts, &tc));

                            T_IG(VTSS_TRACE_GRP_MOD_MAN, "load_ts sec:%u switch ts.seconds :%u", load_ts.seconds, ts.seconds);
                            if (ts.seconds == load_ts.seconds) {
                                set_time = false;
                                T_IG(VTSS_TRACE_GRP_MOD_MAN, "set_time done");
                            } else {
                                continue;
                            }
                        }

                        switch (mod_man_slave_table[slave_idx].my_state) {
                            case MOD_MAN_NOT_STARTED:
                            case MOD_MAN_DISABLED:
                                break;
                            case MOD_MAN_READ_RTC_FROM_SWITCH:
                                T_NG(VTSS_TRACE_GRP_MOD_MAN, "%02d: ModuleManager state = MOD_MAN_READ_RTC_FROM_SWITCH", slave_idx);
                                /* start clockskew adjustment */
                                TOD_RC(mesa_ts_timeofday_next_pps_get(NULL, &ts));
                                my_latency = mod_man_slave_table[slave_idx].one_pps_save_latency + my_1pps_latency;
                                T_NG(VTSS_TRACE_GRP_MOD_MAN, "Read ts: 0 %d:%d " VPRI64d "," VPRI64d, ts.seconds, ts.nanoseconds, my_latency, my_1pps_latency);  
                                vtss_tod_add_TimeInterval(&ts, &ts, &my_latency);
                                mod_man_slave_table[slave_idx].time_at_next_pps.seconds.high = 0;
                                mod_man_slave_table[slave_idx].time_at_next_pps.seconds.low = ts.seconds;
                                mod_man_slave_table[slave_idx].time_at_next_pps.nanoseconds = ts.nanoseconds;
                                T_NG(VTSS_TRACE_GRP_MOD_MAN, "time_at_next_pps = %d %d:%d",
                                     mod_man_slave_table[slave_idx].time_at_next_pps.seconds.high,
                                     mod_man_slave_table[slave_idx].time_at_next_pps.seconds.low,
                                     mod_man_slave_table[slave_idx].time_at_next_pps.nanoseconds);
                                if (mod_man_slave_table[slave_idx].tod_arm) {
                                    TOD_RC(mod_man_slave_table[slave_idx].tod_arm(
                                    board_instance,
                                    mod_man_slave_table[slave_idx].port_no, MEPA_TS_CMD_SAVE));
                                }
                                mod_man_slave_table[slave_idx].my_state = MOD_MAN_READ_RTC_FROM_PHY_AND_COMPARE;
                                break;
                            case MOD_MAN_SET_ABS_TOD:
                                T_NG(VTSS_TRACE_GRP_MOD_MAN, "%02d: ModuleManager state = MOD_MAN_SET_ABS_TOD", slave_idx);
                                /* get time of next 1PPS tick */
                                TOD_RC(mesa_ts_timeofday_next_pps_get(NULL, &ts));
                                my_latency = mod_man_slave_table[slave_idx].one_pps_load_latency + my_1pps_latency;
                                vtss_tod_add_TimeInterval(&ts, &ts, &my_latency);
                                time_at_next_load.seconds.high = 0;
                                time_at_next_load.seconds.low = ts.seconds;
                                time_at_next_load.nanoseconds = ts.nanoseconds;
                                /* set timeofday in all slave timecounters */
                                if (mod_man_slave_table[slave_idx].tod_set) {
                                    TOD_RC(mod_man_slave_table[slave_idx].tod_set(
                                        board_instance,
                                        mod_man_slave_table[slave_idx].port_no,
                                        &time_at_next_load));
                                }
                                mod_man_slave_table[slave_idx].my_state = MOD_MAN_SET_ABS_TOD_DONE;

                                break;
                            case MOD_MAN_SET_ABS_TOD_DONE:
                                T_NG(VTSS_TRACE_GRP_MOD_MAN, "%02d: ModuleManager state = MOD_MAN_SET_ABS_TOD_DONE", slave_idx);
                                /* enable PTP operation */
                                TOD_RC(mod_man_slave_table[slave_idx].ts_mode_set(
                                           board_instance,
                                           mod_man_slave_table[slave_idx].port_no,
                                           TRUE));
                                if (mod_man_slave_table[slave_idx].tod_set_done) {
                                    TOD_RC(mod_man_slave_table[slave_idx].tod_set_done(
                                               mod_man_slave_table[slave_idx].inst,
                                               mod_man_slave_table[slave_idx].port_no, MEPA_TS_CMD_LOAD));
                                }
                                mod_man_slave_table[slave_idx].my_state = MOD_MAN_READ_RTC_FROM_SWITCH;
                                break;
                            case MOD_MAN_READ_RTC_FROM_PHY_AND_COMPARE:
                                T_NG(VTSS_TRACE_GRP_MOD_MAN, "%02d: ModuleManager state = MOD_MAN_READ_RTC_FROM_PHY_AND_COMPARE", slave_idx);
                                /* do clockskew adjustment */
                                if (mod_man_slave_table[slave_idx].tod_get) {
                                    if (mod_man_slave_table[slave_idx].tod_get(
                                        mod_man_slave_table[slave_idx].inst,
                                        mod_man_slave_table[slave_idx].port_no,
                                        &slave_time) != VTSS_RC_OK) {
                                        T_I("Could not get TOD from phy on port %d", mod_man_slave_table[slave_idx].port_no);
                                    }
                                }
                                mod_man_slave_table[slave_idx].current_slave_time = slave_time;
                                TOD_RC(mesa_ts_timeofday_prev_pps_get(NULL, &prev_ts));
                                my_latency = mod_man_slave_table[slave_idx].one_pps_save_latency + my_1pps_latency;
                                vtss_tod_add_TimeInterval(&prev_ts, &prev_ts, &my_latency);

                                T_DG(VTSS_TRACE_GRP_MOD_MAN, "Read ts: 0 %d:%d " VPRI64d "," VPRI64d, prev_ts.seconds, prev_ts.nanoseconds, my_latency, my_1pps_latency);
                                mod_man_slave_table[slave_idx].slave_skew = (((i64)prev_ts.seconds - (i64)slave_time.seconds.low))*(i64)VTSS_ONE_MIA +
                                                 (((i64)prev_ts.nanoseconds - (i64)slave_time.nanoseconds));
                                mod_man_slave_table[slave_idx].slave_skew_save = mod_man_slave_table[slave_idx].slave_skew;
                                T_DG(VTSS_TRACE_GRP_MOD_MAN, "port: %d, mst time: %d %d:%d, slv_time: %d %d:%d, diff " VPRI64d,
                                     mod_man_slave_table[slave_idx].port_no,
                                     mod_man_slave_table[slave_idx].time_at_next_pps.seconds.high,
                                     mod_man_slave_table[slave_idx].time_at_next_pps.seconds.low,
                                     mod_man_slave_table[slave_idx].time_at_next_pps.nanoseconds,
                                     slave_time.seconds.high, slave_time.seconds.low,
                                     slave_time.nanoseconds, mod_man_slave_table[slave_idx].slave_skew);
                                if (mod_man_slave_table[slave_idx].slave_skew > SLAVE_MAX_SKEW || mod_man_slave_table[slave_idx].slave_skew < -SLAVE_MAX_SKEW) {
                                    T_IG(VTSS_TRACE_GRP_MOD_MAN, "Port %d: Skew %d out of range [-%d .. %d]",
                                         mod_man_slave_table[slave_idx].port_no,
                                         mod_man_slave_table[slave_idx].slave_skew,
                                         SLAVE_MAX_SKEW,
                                         SLAVE_MAX_SKEW);
                                    /* disable PTP operation ( removed because it may cause packet loss)*/
                                    //TOD_RC(mod_man_slave_table[slave_idx].ts_mode_set(
                                    //           mod_man_slave_table[slave_idx].inst,
                                    //           mod_man_slave_table[slave_idx].port_no,
                                    //           FALSE));
                                    if (mod_man_slave_table[slave_idx].in_sync == TRUE || mod_man_slave_table[slave_idx].in_sync_reported_once == FALSE) {
                                        tx_ts_in_sync_call_out(mod_man_slave_table[slave_idx].port_no, FALSE);
                                        T_DG(VTSS_TRACE_GRP_MOD_MAN, "port: %d, skew too high (now out-of-sync)",mod_man_slave_table[slave_idx].port_no);
                                        mod_man_slave_table[slave_idx].in_sync_reported_once = TRUE;
                                        mod_man_slave_table[slave_idx].out_of_sync_cnt++;
                                        mod_man_slave_table[slave_idx].oos_skew = mod_man_slave_table[slave_idx].slave_skew_save;
                                    }
                                    mod_man_slave_table[slave_idx].in_sync = FALSE;
                                    mod_man_slave_table[slave_idx].my_state = MOD_MAN_SET_ABS_TOD;
                                } else {
                                    // With external phys, having their own internal DPLL, it can
                                    // happen that the internal LTC is incremented at exactly the same
                                    // time as the rising edge of the PPS. When latching the time of
                                    // the 1PPS, it is therefore indeterministic whether the value
                                    // latched is the LTC value just before it is incremented or just
                                    // after it is incremented. Low pass filtering is therefore
                                    // required to avoid that the LTC is adjusted 1ns forth and back
                                    // at every PPS. One clock cycle in the PHY is 4ns, so once the slave
                                    // is in sync with the switch, skew values in the range [-4 .. 4] are allowed.
                                    i64 allowed_skew = mod_man_slave_table[slave_idx].in_sync ? 4 : 0;
                                    if (mod_man_slave_table[slave_idx].slave_skew > allowed_skew) {
                                        /* minor adjustment - adjust slave */
                                        /* increment slave timer*/
                                        T_IG(VTSS_TRACE_GRP_MOD_MAN, "port: %d, Increment slave time, skew = %d",mod_man_slave_table[slave_idx].port_no, mod_man_slave_table[slave_idx].slave_skew);
                                        TOD_RC(mod_man_slave_table[slave_idx].tod_adj1ns(
                                                    board_instance,
                                                    mod_man_slave_table[slave_idx].port_no,
                                                    TRUE));
                                    } else if ((mod_man_slave_table[slave_idx].slave_skew < -allowed_skew)) {
                                        /* minor adjustment - adjust slave */
                                        /* decrement slave timer */
                                        T_IG(VTSS_TRACE_GRP_MOD_MAN, "port: %d, Decrement slave time, skew = %d",mod_man_slave_table[slave_idx].port_no, mod_man_slave_table[slave_idx].slave_skew);
                                        TOD_RC(mod_man_slave_table[slave_idx].tod_adj1ns(
                                                    mod_man_slave_table[slave_idx].inst,
                                                    mod_man_slave_table[slave_idx].port_no,
                                                    FALSE));
                                    } else if (mod_man_slave_table[slave_idx].slave_skew == 0) {
                                        if (mod_man_slave_table[slave_idx].in_sync == FALSE || mod_man_slave_table[slave_idx].in_sync_reported_once == FALSE) {
                                            tx_ts_in_sync_call_out(mod_man_slave_table[slave_idx].port_no, TRUE);
                                            mod_man_slave_table[slave_idx].in_sync_reported_once = TRUE;
                                            T_IG(VTSS_TRACE_GRP_MOD_MAN, "port: %d, in_sync set to TRUE",mod_man_slave_table[slave_idx].port_no);
                                        }
                                        mod_man_slave_table[slave_idx].in_sync = TRUE;
                                    }
                                    mod_man_slave_table[slave_idx].my_state = MOD_MAN_READ_RTC_FROM_SWITCH;
                                }
                                break;
                            default:
                                T_WG(VTSS_TRACE_GRP_MOD_MAN, "Undefined ModuleManager state");
                                break;
                        }
                    }
                }
                MOD_MAN_UNLOCK();
            }
#if defined VTSS_SW_OPTION_PTP
            else {
                MOD_MAN_LOCK();
                T_DG(VTSS_TRACE_GRP_MOD_MAN, "1PPS disabled");
                for (slave_idx = 0; slave_idx < MOD_MAN_SLAVE_TABLE_SIZE; slave_idx++) {
                    if (mod_man_slave_table[slave_idx].in_sync) {
                        tx_ts_in_sync_call_out(
                            mod_man_slave_table[slave_idx].port_no, FALSE);
                        mod_man_slave_table[slave_idx].in_sync = FALSE;
                        /* set timeofday in master timecounter ?*/
                        mod_man_slave_table[slave_idx].my_state = MOD_MAN_SET_ABS_TOD;
                    }
                }
                MOD_MAN_UNLOCK();
            }
#endif
            tick_cnt = 0;
        } else {
            wakeup += VTSS_OS_MSEC2TICK(100); /* 100 msec */
            if (++tick_cnt > 18) {
                T_IG(VTSS_TRACE_GRP_MOD_MAN, "Missed a 1 PPS tick");
                tick_cnt = 0;
            }
        }


    }
}

mesa_rc tod_mod_man_trig(BOOL ongoing_adj)
{
/* trig ModuleManager */
    T_NG(VTSS_TRACE_GRP_MOD_MAN, "Ongoing adj = %d",ongoing_adj);

    vtss_flag_setbits(&module_man_thread_flags, ongoing_adj ? CTLFLAG_PTP_MOD_MAN_SETTING_TIME : CTLFLAG_PTP_MOD_MAN_1PPS);
    return VTSS_RC_OK;
}

mesa_rc tod_mod_man_set_time(const mesa_timestamp_t *in)
{
    mepa_timestamp_t ts = {};

    ts.seconds.high = in->sec_msb;
    if (in->seconds == 0xFFFFFFFF) {
        ts.seconds.high++;
    }
    ts.seconds.low = in->seconds + 1;
    load_ts.seconds = ts.seconds.low;
    load_ts.sec_msb = ts.seconds.high;

    for (int ix = 0; ix < MOD_MAN_SLAVE_TABLE_SIZE; ++ix) {
        /* set timeofday in all slave timecounters */
        if (mod_man_slave_table[ix].tod_set) {
            ts.nanoseconds = mod_man_slave_table[ix].one_pps_load_latency << 16;
            TOD_RC(mod_man_slave_table[ix].tod_set(
                board_instance,
                mod_man_slave_table[ix].port_no,
                &ts));
            mod_man_slave_table[ix].my_state = MOD_MAN_SET_ABS_TOD_DONE;
        }
    }
    return VTSS_RC_OK;
}

// If 1pps signal is generated between execution of switch and PHY APIs, then
// the PHY and switch LTC will not be in sync.
mesa_rc tod_mod_man_time_set(const mesa_timestamp_t *ts)
{
    T_IG(VTSS_TRACE_GRP_MOD_MAN, "TOD Set time");
    if (set_time == true) {
        T_IG(VTSS_TRACE_GRP_MOD_MAN, "LTC adjustment is in progress. Not setting time again.");
        return VTSS_RC_OK;
    }
    MOD_MAN_LOCK();
    TOD_RC(tod_mod_man_set_time(ts));
    set_time = true;
    MOD_MAN_UNLOCK();

    TOD_RC(mesa_ts_domain_timeofday_set(NULL, 0, ts));
    return VTSS_RC_OK;
}

mesa_rc tod_mod_man_time_step(const mesa_timestamp_t *in, BOOL negative)
{
    mesa_timestamp_t ts, ts1;
    uint64_t tc;

    T_IG(VTSS_TRACE_GRP_MOD_MAN, "TOD Step time");
    if (set_time == true) {
        T_IG(VTSS_TRACE_GRP_MOD_MAN, "LTC adjustment is in progress. Not setting time again.");
        return VTSS_RC_OK;
    }
    TOD_RC(mesa_ts_domain_timeofday_get(NULL, 0, &ts, &tc));
    if (negative == FALSE) {
        tod_add_timestamps(&ts, in, &ts1);
    } else {
        tod_sub_timestamps(&ts, in, &ts1);
    }

    MOD_MAN_LOCK();
    TOD_RC(tod_mod_man_set_time(&ts1));
    set_time = true;
    MOD_MAN_UNLOCK();

    TOD_RC(mesa_ts_domain_timeofday_set_delta(NULL, 0, in, negative));

    return VTSS_RC_OK;
}

mesa_rc tod_mod_man_time_fine_adj(int offset, BOOL neg)
{
    uint32_t nanoSec = labs(offset);

    if (nanoSec > 25) {
        nanoSec = 25;
    }
    MOD_MAN_LOCK();
    for(auto idx = 0; idx < MOD_MAN_SLAVE_TABLE_SIZE; idx++) {
        for (int i = 0; i < nanoSec; i++) {
            TOD_RC(mod_man_slave_table[idx].tod_adj1ns(
                   mod_man_slave_table[idx].inst,
                   mod_man_slave_table[idx].port_no,
                   neg));
        }
    }
    MOD_MAN_UNLOCK();

    return VTSS_RC_OK;
}

mesa_rc tod_mod_man_settling_time_ts_proc(uint64_t hw_time, mesa_timestamp_t *ts)
{
    uint64_t tc;
    uint32_t hw_time_ns = hw_time >> 16;

    TOD_RC(mesa_ts_domain_timeofday_get(NULL, 0, ts, &tc));
    if (load_ts.seconds == ts->seconds) {
        MOD_MAN_LOCK();
        set_time = false;
        MOD_MAN_UNLOCK();

        if ((ts->nanoseconds) < hw_time_ns) {
            ts->seconds--;
        }
    } else {
        mepa_timestamp_t phy_ts;

        meba_phy_ts_ltc_get(board_instance, 0, &phy_ts);
        ts->sec_msb = phy_ts.seconds.high;
        ts->seconds = phy_ts.seconds.low;
        if ((phy_ts.nanoseconds) < hw_time_ns) {
            ts->seconds--;
        }
    }
    ts->nanoseconds = hw_time_ns;
    ts->nanosecondsfrac = hw_time & 0xFFFF;

    return VTSS_RC_OK;
}

bool tod_mod_man_time_settling()
{
    return set_time;
}

mesa_rc tod_mod_man_pre_init(void)
{
    mesa_rc rc = VTSS_RC_OK;
    int ix;
    critd_init(&mod_man_mutex, "tod.mod_man", VTSS_MODULE_ID_TOD, CRITD_TYPE_MUTEX);

    MOD_MAN_LOCK();

    vtss_flag_init(&module_man_thread_flags);
    /* initialize moduleman table */
    for (ix = 0; ix < MOD_MAN_SLAVE_TABLE_SIZE; ++ix) {
        mod_man_slave_table[ix].tod_set = NULL;
        mod_man_slave_table[ix].tod_arm = NULL;
        mod_man_slave_table[ix].tod_set_done = NULL;
        mod_man_slave_table[ix].tod_get = NULL;
        mod_man_slave_table[ix].tod_adj1ns = NULL;
        mod_man_slave_table[ix].tod_rateadj = NULL;
        mod_man_slave_table[ix].tod_fifo_empty = NULL;
        mod_man_slave_table[ix].inst = 0;
        mod_man_slave_table[ix].port_no = 0;
        mod_man_slave_table[ix].my_state = MOD_MAN_NOT_STARTED;
        mod_man_slave_table[ix].in_sync = FALSE;
        mod_man_slave_table[ix].one_pps_load_latency = 0;
        mod_man_slave_table[ix].one_pps_save_latency = 0;
        mod_man_slave_table[ix].out_of_sync_cnt = 0;
        mod_man_slave_table[ix].oos_skew = 0;
    }

    T_RG(VTSS_TRACE_GRP_MOD_MAN, "ModuleManager initialized");

    MOD_MAN_UNLOCK();

    return rc;
}

mesa_rc tod_mod_man_init(void)
{
    mesa_port_no_t port_no;
    port_iter_t       pit;
    MOD_MAN_LOCK();
    port_data_init();
    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        port_no = pit.iport;
        port_data_set(port_no);
    }
    MOD_MAN_UNLOCK();

    vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                       mod_man_main_thread,
                       0,
                       "PTP_module_man",
                       nullptr,
                       0,
                       &mod_man_thread_handle,
                       &mod_man_thread_block);

    return VTSS_RC_OK;
}

mesa_rc tod_mod_man_gen_status_get(int *running, int *remain_time_until_start, int *max_skew)
{
    *running = mod_man_thread_running;

    if (mod_man_thread_running == MOD_MAN_NOT_INSTALLED) {
        // ok
        *remain_time_until_start = 0;
    } else if (mod_man_thread_running == MOD_MAN_INSTALLED_BUT_NOT_RUNNING) {
        *remain_time_until_start = VTSS_OS_TICK2MSEC(mod_man_thread_init_end_time - vtss_current_time());
    } else {
        *remain_time_until_start = 0;
    }
    *max_skew = SLAVE_MAX_SKEW;

    return VTSS_RC_OK;
}

mesa_rc tod_mod_man_port_status_get(mesa_port_no_t port_no, mepa_timestamp_t *slave_time, mepa_timestamp_t *next_pps, i64 *skew, bool *in_sync, mod_man_state_t *state, uint32_t *oos_cnt, int64_t *oos_skew)
{
    int ix;

    for (ix = 0; ix < MOD_MAN_SLAVE_TABLE_SIZE; ++ix) {
        if (mod_man_slave_table[ix].inst && mod_man_slave_table[ix].port_no == port_no) {
            *slave_time = mod_man_slave_table[ix].current_slave_time;
            *next_pps = mod_man_slave_table[ix].time_at_next_pps;
            *skew =  mod_man_slave_table[ix].slave_skew_save;
            *in_sync = port_data[mod_man_slave_table[ix].port_no].topo.port_ts_in_sync;
            *state = mod_man_slave_table[ix].my_state;
            *oos_cnt = mod_man_slave_table[ix].out_of_sync_cnt;
            *oos_skew = mod_man_slave_table[ix].oos_skew;
            return VTSS_RC_OK;
        }
    }
    return VTSS_RC_ERROR;
}


mesa_rc tod_mod_man_slave_freq_adjust(mepa_ts_scaled_ppb_t *adj)
{
    mesa_rc rc = VTSS_RC_OK;
    int ix;
    /* set adjustment on all active timecounters */
    MOD_MAN_LOCK();
    my_adj = *adj;
    for (ix = 0; ix < MOD_MAN_SLAVE_TABLE_SIZE; ++ix) {
        if (mod_man_slave_table[ix].tod_rateadj) {
            rc = mod_man_slave_table[ix].tod_rateadj(
                        mod_man_slave_table[ix].inst,
                        mod_man_slave_table[ix].port_no,
                        adj);
            T_DG(VTSS_TRACE_GRP_MOD_MAN, "rate adj port=%d, rc = %x, adj = " VPRI64d,
                        mod_man_slave_table[ix].port_no, rc, (*adj)>>16);
        }
    }
    MOD_MAN_UNLOCK();
    return rc;
}

static mesa_rc port_slave_set(mesa_port_no_t port_no,
                              mesa_timeinterval_t one_pps_load_latency,
                              mesa_timeinterval_t one_pps_save_latency)
{
    mesa_rc my_rc = VTSS_RC_OK;
    mepa_fifo_ts_entry_t arr[10];
    uint32_t num = 0;

    /* Add a slaveclock to the system */
    int ix = 0;
    while (ix < MOD_MAN_SLAVE_TABLE_SIZE) {
        if (mod_man_slave_table[ix].port_no == port_no) {
            T_IG(VTSS_TRACE_GRP_PHY_TS, "port already included in moduleman table, port = %d", port_no);
            break;
        }
        ++ix;
    }
    if (ix >= MOD_MAN_SLAVE_TABLE_SIZE) { /* id not already in table then find a free entry */
        ix = 0;
        while (ix < MOD_MAN_SLAVE_TABLE_SIZE && mod_man_slave_table[ix].tod_set != NULL) {
            ++ix;
        }
    }
    if (ix < MOD_MAN_SLAVE_TABLE_SIZE) {
        T_IG(VTSS_TRACE_GRP_PHY_TS, "setup for port %d (idx %d)", port_no, ix);
        mod_man_slave_table[ix].tod_set = meba_phy_ts_ltc_set;
        mod_man_slave_table[ix].tod_arm = meba_phy_ts_ltc_ls_en;
        mod_man_slave_table[ix].tod_set_done = meba_phy_ts_ltc_ls_en;
        mod_man_slave_table[ix].tod_get = meba_phy_ts_ltc_get;
        mod_man_slave_table[ix].tod_adj1ns = meba_phy_ts_clock_adj1ns;
        mod_man_slave_table[ix].tod_rateadj = meba_phy_ts_clock_rateadj_set;
        mod_man_slave_table[ix].ts_mode_set = meba_phy_ts_mode_set;
        mod_man_slave_table[ix].tod_fifo_empty = meba_phy_ts_fifo_empty;
        mod_man_slave_table[ix].inst = board_instance;
        mod_man_slave_table[ix].port_no = port_no;
        mod_man_slave_table[ix].my_state = MOD_MAN_NOT_STARTED;
        mod_man_slave_table[ix].in_sync = FALSE;
        mod_man_slave_table[ix].in_sync_reported_once = FALSE;
        mod_man_slave_table[ix].out_of_sync_cnt = 0;
        mod_man_slave_table[ix].oos_skew = 0;
        mod_man_slave_table[ix].one_pps_load_latency = one_pps_load_latency;
        mod_man_slave_table[ix].one_pps_save_latency = one_pps_save_latency;
        /* initial setting of adjustment rate */
        TOD_RC(mod_man_slave_table[ix].tod_rateadj(mod_man_slave_table[ix].inst,
                                            mod_man_slave_table[ix].port_no,
                                            &my_adj));
        // Clear FIFO.
        (void)meba_phy_ts_fifo_get(board_instance, port_no, arr, 10, &num);
        T_DG(VTSS_TRACE_GRP_MOD_MAN, "added port = %d, index = %d", port_no, ix);
    } else {
        my_rc = VTSS_RC_ERROR;
        T_IG(VTSS_TRACE_GRP_PHY_TS, "missing space in moduleman table, port = %d", port_no);
    }
    return my_rc;
}

static mesa_rc port_slave_start_all(void)
{
   int ix;

   for (ix = 0; ix < MOD_MAN_SLAVE_TABLE_SIZE; ix++) {
        if (mod_man_slave_table[ix].inst) {
            mod_man_slave_table[ix].my_state = MOD_MAN_SET_ABS_TOD;
        }
    }
    return VTSS_RC_OK;
}


mesa_rc tod_mod_man_force_slave_state(mesa_port_no_t port_no, BOOL enable)
{
    mesa_rc rc = VTSS_RC_OK;
    /* change the state for a slaveclock from the system */
    int ix = 0;
    MOD_MAN_LOCK();
    while (ix < MOD_MAN_SLAVE_TABLE_SIZE && mod_man_slave_table[ix].port_no != port_no) {
        ++ix;
    }
    if (ix < MOD_MAN_SLAVE_TABLE_SIZE) {
        if (enable && (mod_man_slave_table[ix].my_state == MOD_MAN_DISABLED)) {
            mod_man_slave_table[ix].my_state = MOD_MAN_READ_RTC_FROM_SWITCH;
        }
        if (!enable) {
            mod_man_slave_table[ix].my_state = MOD_MAN_DISABLED;
        }
        T_DG(VTSS_TRACE_GRP_MOD_MAN, "enable/disable port = %d, index = %d, enable = %d", port_no, ix, enable);
    } else {
        rc = VTSS_RC_ERROR;
        T_WG(VTSS_TRACE_GRP_MOD_MAN, "no active timecounter for port = %d", port_no);
    }
    MOD_MAN_UNLOCK();
    return rc;
}


/***************************************************************************************************
 * PHY tx timestamp fifo handling.
 ***************************************************************************************************/

typedef struct {
    uint32_t port_no;
    mepa_ts_fifo_sig_t ts_sig;
    void *context;
    void (*cb)(void *context, u32 port_no, mesa_ts_timestamp_t *ts);
} vtss_phy_ts_entry_t;

typedef vtss::RingBuf<vtss_phy_ts_entry_t, 15> ts_entry_t;
static CapArray<ts_entry_t *, MEBA_CAP_BOARD_PORT_MAP_COUNT> ts_q;

static CapArray<int, MEBA_CAP_BOARD_PORT_MAP_COUNT> ptp_phy;


mesa_rc tod_mod_man_tx_ts_queue_init(const mesa_port_no_t port)
{
    if (port_data[port].topo.ts_feature == VTSS_PTP_TS_PTS) {
        if (ts_q[port] == NULL) {
            ts_q[port] = new ts_entry_t;
            T_IG(VTSS_TRACE_GRP_PHY_TS, "created TS Q for port %d", port);
        }
        if (!ts_q[port]) {
            return MESA_RC_ERROR;
        }
    }
    return MESA_RC_OK;
}
mesa_rc tod_mod_man_tx_ts_queue_deinit(const mesa_port_no_t port)
{
    if (!ts_q[port]) {
        return MESA_RC_ERROR;
    }
    delete ts_q[port];
    T_IG(VTSS_TRACE_GRP_PHY_TS, "Deleted TS Q for port %d", port);
    ts_q[port] = NULL;
    return MESA_RC_OK;
}

/* Allocate a timestamp entry for a two step transmission */
mesa_rc tod_mod_man_tx_ts_allocate(const mesa_ts_timestamp_alloc_t *const alloc_parm,
                                        const mepa_ts_fifo_sig_t    *const ts_sig)
{
    mesa_rc rc = VTSS_RC_ERROR;
    uint64_t mask = alloc_parm->port_mask;
    vtss_phy_ts_entry_t ts_entry;

    ts_entry.ts_sig = *ts_sig;
    ts_entry.context = alloc_parm->context;
    ts_entry.cb = alloc_parm->cb;
    MOD_MAN_LOCK();
    for (int i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        if ((mask & 1) && ts_q[i]) {
            T_NG(VTSS_TRACE_GRP_PHY_TS, "port %d seq-id %d msg_type %d", i, ts_sig->sequence_id, ts_sig->msg_type);
            ts_entry.port_no = i;
            ts_q[i]->push(ts_entry);
            ptp_phy[i]++;
            rc = VTSS_RC_OK;
        }
        mask = mask >> 1;
    }
    MOD_MAN_UNLOCK();
    return rc;
}

static bool phy_ts_signature_equal(mepa_ts_fifo_sig_t *sig_a, mepa_ts_fifo_sig_t *sig_b)
{
    if (sig_a->msg_type != sig_b->msg_type ||
        sig_a->sequence_id != sig_b->sequence_id) {
        return false;
    }

    T_RG(VTSS_TRACE_GRP_PHY_TS, "has_crc crc sig A: %d %d sig B: %d %d", sig_a->has_crc_src, sig_a->crc_src_port, sig_b->has_crc_src, sig_b->crc_src_port);
    if (sig_a->has_crc_src) {
        if (sig_a->crc_src_port != sig_b->crc_src_port) {
            return false;
        }
    } else {
        if (memcmp(sig_a->src_port_identity, sig_b->src_port_identity, sizeof(sig_a->src_port_identity))) {
            return false;
        }
    }

    return true;
}
/*
 * PHY Timestamp feature interrupt handler
 * \param instance_id   IN  PHY port number that caused the interrupt.
 */
static void phy_ts_interrupt_handler(meba_event_t source_id,
                                            u32          instance_id)
{
    mepa_fifo_ts_entry_t arr[10];
    vtss_phy_ts_entry_t entry;
    uint32_t num = 0;
    mesa_ts_timestamp_t ts;
    uint32_t port = instance_id;

    ts.ts_valid = TRUE;
    if (ptp_phy[port] &&
       (source_id == MEBA_EVENT_EGR_TIMESTAMP_CAPTURED ||
        source_id == MEBA_EVENT_EGR_FIFO_OVERFLOW)) {
        if (!ts_q[port]) {
            T_WG(VTSS_TRACE_GRP_PHY_TS, "timestamp Q not initialised for port");
        }
        meba_phy_ts_fifo_get(board_instance, port, arr, 10, &num);
        for (int j = 0; j < num; j++) {
            bool valid;
            if (!ts_q[port]) {
                break;
            }
            while ((valid = ts_q[port]->pop(entry))) {
                    T_NG(VTSS_TRACE_GRP_PHY_TS, "arr msg_type %d seq %d fifo msg_type %d seq %d",
                                                arr[j].sig.msg_type, arr[j].sig.sequence_id,
                                                entry.ts_sig.msg_type, entry.ts_sig.sequence_id);
                if (phy_ts_signature_equal(&arr[j].sig, &entry.ts_sig)) {
                    (void)mesa_packet_ns_to_ts_cnt(NULL, arr[j].ts.nanoseconds, &ts.ts);
                    ts.id = entry.ts_sig.sequence_id;
                    T_DG(VTSS_TRACE_GRP_PHY_TS, "signature matched for seq-id %d", ts.id);
                    entry.cb(entry.context, port, &ts);
                    break;
                }
            }
            if (!valid) {
                break;
            }
        }
    }
    if (ptp_phy[port] <= num || !num) {
        ptp_phy[port] = 0;
    }
    T_IG(VTSS_TRACE_GRP_PHY_TS, "PHY timestamp interrupt detected: source_id %d, instance_id %u", source_id, instance_id);
    TOD_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_TOD,
                                          phy_ts_interrupt_handler,
                                          source_id,
                                          INTERRUPT_PRIORITY_NORMAL));
}




/* to avoid lint warnings for the MOD_MAN_UNLOCK() ; MOD_MAN_LOCK when calling out from the module_man */
/*lint -e{456} */
/*lint -e{455} */
/*lint -e{454} */
static void tx_ts_in_sync_call_out(mesa_port_no_t port_no, BOOL in_sync)
{
   int id;
   port_data[port_no].topo.port_ts_in_sync = in_sync;

   for (id = 0; id < MOD_MAN_USER_TABLE_SIZE; id++) {
        if (my_in_sync_cb[id] != NULL) {
            /* as the applications may call back to module_man, we unlock here to avoid deadlock */
            MOD_MAN_UNLOCK();
            my_in_sync_cb[id](port_no, in_sync);
            MOD_MAN_LOCK();
        }
    }
}

mesa_rc tod_mod_man_tx_ts_in_sync_cb_set(const vtss_module_man_in_sync_cb_t in_sync_cb)
{
    int id;
    mesa_rc rc = VTSS_RC_ERROR;
    MOD_MAN_LOCK();
    for (id = 0; id < MOD_MAN_USER_TABLE_SIZE; id++) {
        if (my_in_sync_cb[id] == in_sync_cb) {
            /* only insert same pointer once */
            rc = VTSS_RC_OK;
            break;
        }
    }
    if (rc != VTSS_RC_OK) {
        /* if not already in list, then find a free entry */
        for (id = 0; id < MOD_MAN_USER_TABLE_SIZE; id++) {
            if (my_in_sync_cb[id] == NULL) {
                my_in_sync_cb[id] = in_sync_cb;
                rc = VTSS_RC_OK;
                break;
            }
        }
    }
    MOD_MAN_UNLOCK();
    return rc;
}

