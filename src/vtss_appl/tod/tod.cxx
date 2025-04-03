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

#include "main.h"
#include "main_types.h"
#include "tod.h"
#include "tod_api.h"
#include "vtss_tod_api.h"
#include "critd_api.h"
#include "interrupt_api.h"      /* interrupt handling */
#include "misc_api.h"
#include "port_api.h"
#include "vtss_tod_mod_man.h"
#include "vtss_timer_api.h"
#include "port_iter.hxx"

//#define CALC_TEST

#ifdef CALC_TEST
#include "vtss_ptp_types.h"
#endif

#if defined(VTSS_SW_OPTION_PHY)
#include "phy_api.h"
#endif
#include "main.h"
#define API_INST_DEFAULT PHY_INST 
#define PHY_PORT_NO_START 0

#include "vtss_os_wrapper.h"

/* i2c static functions */
/* unshifted address for device address 0xe0(1110 0000)*/
#define MAX_MAL_5338_I2C_LEN 8
#define MAX_I2C_LEN 3 

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "tod", "Time of day for PTP and OAM etc."
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default (TOD core)",
        VTSS_TRACE_LVL_WARNING,
        VTSS_TRACE_FLAGS_USEC
    },
    [VTSS_TRACE_GRP_CLOCK] = {
        "clock",
        "TOD time functions",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_MOD_MAN] = {
        "mod_man",
        "PHY Timestamp module manager",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PHY_TS] = {
        "phy_ts",
        "PHY Timestamp feature",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PHY_ENG] = {
        "phy_eng",
        "PHY Engine allocation feature",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

static struct {
    BOOL ready;                 /* TOD Initited  */
    critd_t datamutex;          /* Global data protection */
} tod_global;

/* ================================================================= *
 *  Configuration definitions
 * ================================================================= */

typedef struct tod_config_t {
    mesa_packet_internal_tc_mode_t     int_mode;       /* internal timestamping mode */
    mepa_ts_clock_freq_t               freq;           /* timestamping reference clock frequency */
    CapArray <BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT> phy_ts_enable;  /* enable PHY timestamping mode */
} tod_config_t;

static tod_config_t config_data ;
static bool mepa_phy_ts_capability = false;
static bool board_phy_ts_disable = false;

/*
 * Propagate the PTP (module) configuration to the PTP core
 * library.
 */
static void
tod_conf_propagate(void)
{
    T_I("TOD Configuration has been reset, you need to reboot to activate the changed conf." );
}


/**
 * Read the PTP configuration.
 * \param create indicates a new default configuration block should be created.
 *
 */
static void
tod_conf_read(BOOL create)
{
    BOOL            tod_conf_changed = FALSE;

    TOD_DATA_LOCK();
    /* Check if configuration changed */
        if (config_data.int_mode != MESA_PACKET_INTERNAL_TC_MODE_30BIT) tod_conf_changed = TRUE;
    /* initialize run-time options to reasonable values */
    config_data.int_mode = MESA_PACKET_INTERNAL_TC_MODE_30BIT;
    if (fast_cap(MEBA_CAP_1588_REF_CLK_SEL)) {
        config_data.freq = MEPA_TS_CLOCK_FREQ_250M; /* 250 MHz is default on the PCB107 board */
    }
    if (tod_conf_changed) {
        T_I("TOD Configuration has been reset, you need to reboot to activate the changed conf." );
    }

    TOD_DATA_UNLOCK();

}
/****************************************************************************
 * Configuration API
 ****************************************************************************/

BOOL tod_tc_mode_get(mesa_packet_internal_tc_mode_t *mode)
{
    BOOL ok = FALSE;
    TOD_DATA_LOCK();
    ok = TRUE;
    *mode = config_data.int_mode;
    TOD_DATA_UNLOCK();
    return ok;
}
BOOL tod_tc_mode_set(mesa_packet_internal_tc_mode_t *mode)
{
    BOOL ok = FALSE;
    TOD_DATA_LOCK();
    if (*mode < MESA_PACKET_INTERNAL_TC_MODE_MAX) {
        ok = TRUE;
        config_data.int_mode = *mode;
    }
    TOD_DATA_UNLOCK();
    return ok;
}

BOOL tod_port_phy_ts_get(BOOL *ts, mesa_port_no_t portnum)
{
    BOOL ok = FALSE;
    *ts = FALSE;

    if (mepa_phy_ts_cap()) {
        TOD_DATA_LOCK();
        if (portnum < port_count_max()) {
            *ts = config_data.phy_ts_enable[portnum];
            ok = TRUE;
        }
        TOD_DATA_UNLOCK();
    }
    return ok;
}

BOOL tod_port_phy_ts_set(BOOL *ts, mesa_port_no_t portnum)
{
    BOOL ok = FALSE;

    if (mepa_phy_ts_cap()) {
        TOD_DATA_LOCK();
        if (portnum < port_count_max()) {
            config_data.phy_ts_enable[portnum] = *ts;
            ok = TRUE;
        }
        TOD_DATA_UNLOCK();
    }
    return ok;
}

BOOL tod_ref_clock_freg_get(mepa_ts_clock_freq_t *freq)
{
    BOOL ok = FALSE;
    TOD_DATA_LOCK();
    ok = TRUE;
    *freq = config_data.freq;
    TOD_DATA_UNLOCK();
    return ok;
}

BOOL tod_ref_clock_freg_set(mepa_ts_clock_freq_t *freq)
{
    BOOL ok = FALSE;
    TOD_DATA_LOCK();
    if (fast_cap(MEBA_CAP_1588_REF_CLK_SEL)) {
        if (*freq < MEPA_TS_CLOCK_FREQ_MAX) {
            ok = TRUE;
            config_data.freq = *freq;
            if (!tod_global.ready) {
                T_I("The 1588 reference clock is set");
            } else {
                T_W("The 1588 reference clock has been changed, please save configuration and reboot to apply the change");
            }
        }
    } else {
        T_W("The 1588 reference clock freq set is not supported");
    }
    TOD_DATA_UNLOCK();
    return ok;
}

BOOL tod_ready(void)
{
    return tod_global.ready;
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/
#ifdef CALC_TEST

#define TC(t,c) ((t<<16) + c)
static void calcTest(void)
{
    u32 r, x, y;
    mesa_timeinterval_t t;
    char str1 [30];

/* sub */    
    x = TC(43,4321);
    y = TC(41,1234); 
    vtss_tod_ts_cnt_sub(&r, x, y); /* no wrap */
    T_W("%ld,%ld - %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(43,4321);
    y = TC(41,4321); 
    vtss_tod_ts_cnt_sub(&r, x, y); /* no wrap */
    T_W("%ld,%ld - %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,4321);
    y = TC(41,1234); 
    vtss_tod_ts_cnt_sub(&r, x, y); /* no wrap */
    T_W("%ld,%ld - %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,4321);
    y = TC(42,1234); 
    vtss_tod_ts_cnt_sub(&r, x, y); /* tick wrap */
    T_W("%ld,%ld - %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(43,4321);
    y = TC(41,4322); 
    vtss_tod_ts_cnt_sub(&r, x, y); /* clk wrap */
    T_W("%ld,%ld - %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );

/* add */    
    x = TC(41,1234);
    y = TC(43,4321); 
    vtss_tod_ts_cnt_add(&r, x, y); /* no wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,1234);
    y = TC(43,8781); 
    vtss_tod_ts_cnt_add(&r, x, y); /* no wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,1234);
    y = TC(43,8782); 
    vtss_tod_ts_cnt_add(&r, x, y); /* clk wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,1234);
    y = TC(43,8783); 
    vtss_tod_ts_cnt_add(&r, x, y); /* clk wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(41,1234);
    y = TC(58,4321); 
    vtss_tod_ts_cnt_add(&r, x, y); /* no wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );
    x = TC(48,1234);
    y = TC(52,4321); 
    vtss_tod_ts_cnt_add(&r, x, y); /* tick wrap */
    T_W("%ld,%ld + %ld,%ld = %ld,%ld", x>>16,x & 0xffff,y>>16,y & 0xffff,r>>16,r & 0xffff );


/* timeinterval to cnt */
    t = VTSS_SEC_NS_INTERVAL(0,123456);
    vtss_tod_timeinterval_to_ts_cnt(&r, t);
    T_W("TimeInterval: %s => cnt %ld,%ld", TimeIntervalToString (&t, str1, '.'),r>>16,r & 0xffff );
    t = VTSS_SEC_NS_INTERVAL(0,123456789);
    vtss_tod_timeinterval_to_ts_cnt(&r, t);
    T_W("TimeInterval: %s => cnt %ld,%ld", TimeIntervalToString (&t, str1, '.'),r>>16,r & 0xffff );
    t = VTSS_SEC_NS_INTERVAL(0,999999000);
    vtss_tod_timeinterval_to_ts_cnt(&r, t);
    T_W("TimeInterval: %s => cnt %ld,%ld", TimeIntervalToString (&t, str1, '.'),r>>16,r & 0xffff );
    t = VTSS_SEC_NS_INTERVAL(0,999999999);
    vtss_tod_timeinterval_to_ts_cnt(&r, t);
    T_W("TimeInterval: %s => cnt %ld,%ld", TimeIntervalToString (&t, str1, '.'),r>>16,r & 0xffff );
    t = VTSS_SEC_NS_INTERVAL(1,0);
    vtss_tod_timeinterval_to_ts_cnt(&r, t);
    T_W("TimeInterval: %s => cnt %ld,%ld", TimeIntervalToString (&t, str1, '.'),r>>16,r & 0xffff );

/* cnt to timeinterval */
    x = TC(48,1234);
    vtss_tod_ts_cnt_to_timeinterval(&t, x);
    T_W("cnt %ld,%ld => TimeInterval: %s",r>>16,r & 0xffff , TimeIntervalToString (&t, str1, '.'));
    
}
#endif

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

static mesa_ts_internal_fmt_t serval_internal(void)
{
    mesa_packet_internal_tc_mode_t mode;
    if (tod_tc_mode_get(&mode)) {
        T_D("Internal mode: %d", mode);
        switch (mode) {
            case MESA_PACKET_INTERNAL_TC_MODE_30BIT: return MESA_TS_INTERNAL_FMT_RESERVED_LEN_30BIT;
            case MESA_PACKET_INTERNAL_TC_MODE_32BIT: return MESA_TS_INTERNAL_FMT_RESERVED_LEN_32BIT;
            case MESA_PACKET_INTERNAL_TC_MODE_44BIT: return MESA_TS_INTERNAL_FMT_SUB_ADD_LEN_44BIT_CF62;
            case MESA_PACKET_INTERNAL_TC_MODE_48BIT: return MESA_TS_INTERNAL_FMT_RESERVED_LEN_48BIT_CF;
            default: return MESA_TS_INTERNAL_FMT_NONE;
        }
    } else {
        return MESA_TS_INTERNAL_FMT_NONE;
    }
}

static void one_pps_pulse_interrupt_handler(meba_event_t source_id, u32 instance_id);

static u32 ns_sample_max(int count)
{
    if (count >= fast_cap(MESA_CAP_TOD_SAMPLES_PR_SEC)) {
        return 1000000000;
    } else {
        return count * (1000000000 / fast_cap(MESA_CAP_TOD_SAMPLES_PR_SEC)) + (1000000000 / fast_cap(MESA_CAP_TOD_SAMPLES_PR_SEC) / 2);
    }
}

/* 
 * sample_count:
 * Jaguar2: The timeofday is sampled more than once pr sec, in order to be able to convert 30 bit timestamps to timeofday
 *          sample_count = 0 => one_sec function and module man are called, and the timer is started
 *          sample_count = 1 => one_sec function is called, and interrupt is enabled
 *          sample_count = 2 =>  interrupt is enabled, this step is caused by the extra interupt that Jaguar2 generates then the one-sec function is called
 */
static uint sample_count;

void tod_one_sec_timer_expired(vtss::Timer *timer)
{
    BOOL ongoing_adj;
    mesa_timestamp_t ts;
    uint64_t         tc;
    TOD_DATA_LOCK();

    if (sample_count < fast_cap(MESA_CAP_TOD_SAMPLES_PR_SEC)) {
        TOD_RC(mesa_ts_adjtimer_one_sec(0, &ongoing_adj))
        T_D("ongoing_adj %u", ongoing_adj);
        if (mepa_phy_ts_cap()) {
            if (sample_count == 0) {
                TOD_RC(tod_mod_man_trig(ongoing_adj));
            }
        }
    }
    TOD_RC(mesa_ts_timeofday_get(NULL, &ts, &tc));
    if (fast_cap(MESA_CAP_TOD_SAMPLES_PR_SEC) > 1) {
        if (ts.nanoseconds > ns_sample_max(sample_count)) {
            T_I("Too slow timeofday sampling: sample_count %u, current time (ns) %u, must be less than %u", sample_count, ts.nanoseconds, ns_sample_max(sample_count));
        }
    }
    T_D("sample_count %u, current time (ns) %u", sample_count, ts.nanoseconds);
    if (++sample_count < fast_cap(MESA_CAP_TOD_SAMPLES_PR_SEC))
    {
        timer->set_period(vtss::microseconds(1000000 / fast_cap(MESA_CAP_TOD_SAMPLES_PR_SEC)));
        if (vtss_timer_start(timer) != VTSS_RC_OK) {
            T_EG(VTSS_TRACE_GRP_CLOCK, "Unable to start tod timer");
        }
    } else {
        if (sample_count > fast_cap(MESA_CAP_TOD_SAMPLES_PR_SEC) || fast_cap(MESA_CAP_TOD_SAMPLES_PR_SEC) == 1) sample_count = 0;
        TOD_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_TOD,
                                              one_pps_pulse_interrupt_handler,
                                              (meba_event_t)fast_cap(MEBA_CAP_ONE_PPS_INT_ID),
                                              INTERRUPT_PRIORITY_NORMAL));
    }
    TOD_DATA_UNLOCK();
}

/*
 * Time stamp age timer expired.
 */
void tod_time_stamp_age_timer_expired(vtss::Timer *timer)
{
    TOD_RC(mesa_timestamp_age(NULL));
}

/*
 * timer used to delay the One sec action
 */
/*lint -e{459} ... 'one_pps_timer' only used in the init thread and the Interrupt thread */
static vtss::Timer one_pps_timer(VTSS_THREAD_PRIO_HIGH);

static void init_one_pps_timer(void) {
    // Create one pps timer

    one_pps_timer.set_repeat(FALSE);
    one_pps_timer.set_period(vtss::milliseconds(2));     /* 2 msec */
    one_pps_timer.callback    = tod_one_sec_timer_expired;
    one_pps_timer.modid       = VTSS_MODULE_ID_TOD;
    T_IG(VTSS_TRACE_GRP_CLOCK, "one_pps_timer initialized");
}

static vtss::Timer age_timer;
/*
 * timer used for calling Time stamp age function
 */
static void init_time_stamp_age_timer(void) {
    age_timer.set_repeat(TRUE);
    age_timer.set_period(vtss::milliseconds(10));     /* 10 msec */
    age_timer.callback    = tod_time_stamp_age_timer_expired;
    age_timer.modid       = VTSS_MODULE_ID_TOD;
    T_IG(VTSS_TRACE_GRP_CLOCK, "Age timer initialized");
    TOD_RC(vtss_timer_start(&age_timer));
    T_IG(VTSS_TRACE_GRP_CLOCK, "Age timer started");
}

/*
 * TOD Synchronization 1 pps pulse update handler
 */
/*lint -esym(459, one_pps_pulse_interrupt_handler) */
static void one_pps_pulse_interrupt_handler(meba_event_t     source_id,
        u32                         instance_id)
{
    T_N("One sec pulse event: source_id %d, instance_id %u", source_id, instance_id);
    if (fast_cap(MESA_CAP_TOD_SAMPLES_PR_SEC) > 1) {
        /* trig ModuleManager */
        /* start 1 ms timer */
        TOD_DATA_LOCK();
        one_pps_timer.set_period(vtss::microseconds(2));     /* 2 usec */
        if (vtss_timer_start(&one_pps_timer) != VTSS_RC_OK) {
            T_EG(VTSS_TRACE_GRP_CLOCK, "Unable to start tod timer");
        }
        TOD_DATA_UNLOCK();
    } else {
        BOOL ongoing_adj;
        TOD_RC(mesa_ts_adjtimer_one_sec(0, &ongoing_adj));
        /* trig ModuleManager */
        if (mepa_phy_ts_cap()) {
            TOD_RC(tod_mod_man_trig(ongoing_adj));
        }
        TOD_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_TOD,
                                              one_pps_pulse_interrupt_handler,
                                              (meba_event_t)fast_cap(MEBA_CAP_ONE_PPS_INT_ID),
                                              INTERRUPT_PRIORITY_NORMAL));
    }
}

/*
 * PTP Timestamp update handler
 */
static void timestamp_interrupt_handler(meba_event_t     source_id,
        u32                         instance_id)
{

    T_D("New timestamps detected: source_id %d, instance_id %u", source_id, instance_id);
    TOD_RC(mesa_tx_timestamp_update(0));


    TOD_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_TOD,
                timestamp_interrupt_handler,
                MEBA_EVENT_CLK_TSTAMP,
                INTERRUPT_PRIORITY_NORMAL));
}

static bool mepa_phy_ts_single_cap(mepa_device_t *dev)
{
    mesa_rc rc;
    mepa_phy_info_t info;

    rc = mepa_phy_info_get(dev, &info);
    if (rc != MESA_RC_OK) {
        return false;
    }

    if (info.cap & (MEPA_CAP_TS_MASK_GEN_1 | MEPA_CAP_TS_MASK_GEN_2 | MEPA_CAP_TS_MASK_GEN_3)) {
        return true;
    }

    return false;
}

static void mepa_driver_phy_ts_cap_detect()
{
    port_iter_t     pit;
    mepa_device_t *phy_dev;

    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        phy_dev = board_instance->phy_devices[pit.iport];
        if (mepa_phy_ts_single_cap(phy_dev)) {
            mepa_phy_ts_capability = true;
            break;
        }
    }
}

bool mepa_phy_ts_cap()
{
    return mepa_phy_ts_capability;
}

// Set timestamping source for all the PHY ports on the board.
// if 'phy_ts_dis' is true => phy timestamping is disabled on the board.
// If this function is called during start-up config, then phy timestamping
// is disabled and tod_mod_man_init is also not executed.
bool tod_board_phy_ts_dis_set(BOOL phy_ts_dis)
{
    port_iter_t pit;
    mepa_device_t *phy_dev;
    bool configured = false;

    port_iter_init_local(&pit);
    while (port_iter_getnext(&pit)) {
        phy_dev = board_instance->phy_devices[pit.iport];
        if (mepa_phy_ts_single_cap(phy_dev)) {
            board_phy_ts_disable = phy_ts_dis ? true : false;
            mepa_phy_ts_capability = phy_ts_dis ? false : true;
            configured = true;
            break;
        }
    }

    return configured;
}

// true => phy timestamping is disabled.
bool tod_board_phy_ts_dis_get()
{
    return board_phy_ts_disable;
}

// After phy devices are hooked up, this function need to be called for initialising PHY timestamping. Without having valid phy device pointers, it is not possible to find whether board has phy timestamping capability.
void tod_phy_ts_init()
{
    //Detect if board has phy timestamping capability.
    mepa_driver_phy_ts_cap_detect();
}

extern "C" int tod_icli_cmd_register();

mesa_rc tod_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;
    uint32_t chip;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        tod_global.ready = FALSE;
        critd_init(&tod_global.datamutex, "tod.data", VTSS_MODULE_ID_TOD, CRITD_TYPE_MUTEX);

        tod_icli_cmd_register();
        T_I("INIT_CMD_INIT TOD" );
        break;

    case INIT_CMD_START:
        T_I("INIT_CMD_START TOD");
        break;

    case INIT_CMD_CONF_DEF:
        tod_conf_read(TRUE);
        tod_conf_propagate();
        T_I("INIT_CMD_CONF_DEF TOD" );
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_I("INIT_CMD_ICFG_LOADING_PRE ");
        tod_conf_read(FALSE);
#ifdef CALC_TEST
        calcTest();
#endif
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        if (isid == VTSS_ISID_START) {
            if (fast_cap(MESA_CAP_TS_INTERNAL_MODE_SUPPORTED)) {
                mesa_ts_internal_mode_t mode;
                mode.int_fmt = serval_internal();
                T_D("Internal mode.int_fmt: %d", mode.int_fmt);
                TOD_RC(mesa_ts_internal_mode_set(NULL, &mode));
            }
            // If phy_ts is disabled, call tod_mod_man_pre_init anyways in case phy_ts is re-enabled.
            if (mepa_phy_ts_cap() || tod_board_phy_ts_dis_get()) {
                // If phy timestamping capability is available, then initialize tod_mod_man
                TOD_RC(tod_mod_man_pre_init());

                if (fast_cap(MEBA_CAP_1588_REF_CLK_SEL)) {
                    // TBD: Need to update synce clock conf for 10G phys
                } else {
                    /* tbd if configurable for Jaguar2 boards */
                    T_I("The 1588 reference clock can not be set in this architecture");
                }
            }
            
            init_one_pps_timer();
            if (mepa_phy_ts_cap()) {
                TOD_RC(tod_mod_man_init());
            }

            if (fast_cap(MESA_CAP_TS)) {
                TOD_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_TOD,
                                                      one_pps_pulse_interrupt_handler,
                                                      (meba_event_t)fast_cap(MEBA_CAP_ONE_PPS_INT_ID),
                                                      INTERRUPT_PRIORITY_NORMAL));
            }

            if (!fast_cap(MESA_CAP_TS_MISSING_TX_INTERRUPT)) {
                T_I("Enabling timestamp interrupt");
                TOD_RC(vtss_interrupt_source_hook_set(VTSS_MODULE_ID_TOD,
                            timestamp_interrupt_handler,
                            MEBA_EVENT_CLK_TSTAMP,
                            INTERRUPT_PRIORITY_NORMAL));
            } else if (fast_cap(MESA_CAP_TS)) {
                init_time_stamp_age_timer();
            }

            tod_global.ready = TRUE;

            chip = fast_cap(MESA_CAP_MISC_CHIP_FAMILY);
            if ((MESA_CHIP_FAMILY_SPARX5 == chip) ||
                (MESA_CHIP_FAMILY_LAN966X == chip) ||
                (MESA_CHIP_FAMILY_LAN969X == chip)) {
                tod_capability_sub_nano_set(true);
            }
        } else {
            T_E("INIT_CMD_ICFG_LOADING_POST - unknown ISID %u", isid);
        }

        T_I("INIT_CMD_ICFG_LOADING_POST - ISID %u", isid);
        break;

    default:
        break;
    }

    return 0;
}

