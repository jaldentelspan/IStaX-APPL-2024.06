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

#include <vtss/appl/tsn.h>        /* For ourselves (public header)           */
#include <vtss/appl/ptp.h>        /* For ourselves (public header)           */
#include "tsn_api.h"              /* For ourselves (semi-public header)      */
#include "tsn_trace.h"            /* For our own trace definitions           */
#include "tsn_expose.hxx"         /* For our own notifications               */
#include "critd_api.h"            /* For mutex wrapper                       */
#include "tsn_timer.hxx"
#include "port_api.h"             /* For port_change_register()              */
#include "tsn_lock.hxx"           /* For TSN_LOCK_SCOPE()                    */
#include "misc_api.h"             /* For misc_mac_txt()                      */
#include "vlan_api.h"             /* For vlan_mgmt_XXX()                     */
#include "conf_api.h"             /* For conf_mgmt_mac_addr_get()            */
#include "vtss_tod_api.h"         /* For mesa_timestamp_t operations         */
#include "l2proto_api.h"          /* For l2port2port()                       */
#include "interrupt_api.h"        /* For vtss_interrupt_source_hook_set()    */
#include "web_api.h"              /* For webCommonBufferHandler()            */
#include "psfp_api.h"             /* For psfp_ptp_ready()                    */
#include <vtss/basics/trace.hxx>  /* For VTSS_TRACE()                        */
#include "vtss_common_iterator.hxx" /* For vtss_appl_iterator_ifindex_front_port */

#if defined(VTSS_SW_OPTION_LLDP)
#include "lldp_api.h"
#endif /* VTSS_SW_OPTION_LLDP */

#define PTP_CHECK_TIMER_TIMEOUT_VALUE_MS    1000

//*****************************************************************************/
// Trace definitions
/******************************************************************************/
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "tsn", "Time Sensitive Network"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [TSN_TRACE_GRP_CALLBACK] = {
        "callback",
        "Callbacks",
        VTSS_TRACE_LVL_ERROR
    },
    [TSN_TRACE_GRP_CONFIG_FP] = {
        "configFp",
        "ConfigFp",
        VTSS_TRACE_LVL_ERROR
    },
    [TSN_TRACE_GRP_CONFIG_TAS] = {
        "configTas",
        "ConfigTas",
        VTSS_TRACE_LVL_ERROR
    },
    [TSN_TRACE_GRP_TIMER] = {
        "timers",
        "Timers",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/******************************************************************************/
// Global variables
/******************************************************************************/
critd_t TSN_crit;

typedef struct {
    tsn_timer_t             ptp_chk_timer;
    appl_tsn_start_state_t  start_state;
    uint32_t                time_passed;
    bool                    start;
} tsn_global_state_t;

uint32_t                                                                TSN_cap_port_cnt;
vtss_appl_tsn_capabilities_t                                            TSN_cap;
const vtss_appl_tsn_capabilities_t *const vtss_appl_tsn_capabilities = &TSN_cap;
static vtss_appl_tsn_global_conf_t                                      TSN_global_conf;
static tsn_global_state_t                                               TSN_global_state;
static bool                                                             TSN_started;

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/
static size_t TSN_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    unsigned int cnt = 0;
    char buff[512];

    cnt  = snprintf(buff, sizeof(buff),
                    "var configTsnHasFramePreemption = %d;\n"
                    ,
                    TSN_cap.has_queue_frame_preemption
                   );

    if (sizeof(buff) - cnt < 2) {
        T_W("config.js might be truncated (cnt:%u, size:" VPRIz").", cnt, sizeof(buff));
    } else {
        T_I("config.js seems ok (cnt:%u, size:" VPRIz").", cnt, sizeof(buff));
    }
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib_config table entry                                               */
/****************************************************************************/
web_lib_config_js_tab_entry(TSN_lib_config_js);

/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t TSN_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[512];
    (void) snprintf(buff, sizeof(buff), "%s",
                    TSN_cap.has_queue_frame_preemption ? "" : ".has_tsn_frame_preemption { display: none; }\r\n");
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(TSN_lib_filter_css);

/******************************************************************************/
// TSN_capabilities_set()
/******************************************************************************/
static void TSN_capabilities_set(void)
{

    TSN_cap.has_queue_frame_preemption = fast_cap(MESA_CAP_QOS_FRAME_PREEMPTION);
    TSN_cap.has_tas                    = fast_cap(MESA_CAP_QOS_TAS);
    TSN_cap.has_psfp                   = fast_cap(MESA_CAP_L2_PSFP);
    TSN_cap.has_frer                   = fast_cap(MESA_CAP_L2_FRER);
    TSN_cap.min_add_frag_size          = 0; // TBD: Should we have a MESA_CAP for this?
    TSN_cap.tas_mac_restrict           = fast_cap(MESA_CAP_QOS_TAS_HOLD_REL_MAC_RESTRICT);
    TSN_cap.tas_max_gce_cnt            = fast_cap(MESA_CAP_QOS_TAS_GCE_CNT);
    TSN_cap.tas_max_sdu_min            = fast_cap(MESA_CAP_QOS_TAS_MAX_SDU_MIN);
    TSN_cap.tas_max_sdu_max            = std::min(fast_cap(MESA_CAP_PORT_FRAME_LENGTH_MAX), fast_cap(MESA_CAP_QOS_TAS_MAX_SDU_MAX));
    TSN_cap.tas_ct_min                 = fast_cap(MESA_CAP_QOS_TAS_CT_MIN);
    TSN_cap.tas_ct_max                 = fast_cap(MESA_CAP_QOS_TAS_CT_MAX);
}

/******************************************************************************/
// TSN_ptr_check()
/******************************************************************************/
mesa_rc TSN_ptr_check(const void *ptr)
{
    return ptr == nullptr ? (mesa_rc)VTSS_APPL_TSN_INVALID_ARGUMENT : VTSS_RC_OK;
}

/******************************************************************************/
// TSN_ifindex_to_port()
/******************************************************************************/
mesa_rc TSN_ifindex_to_port(const vtss_ifindex_t ifindex, uint32_t *port)
{
    vtss_ifindex_elm_t ife = {};
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= TSN_cap_port_cnt) {
        T_I("Only port type interfaces are currently supported");
        return VTSS_APPL_FP_INVALID_IFINDEX;
    }
    VTSS_RC(TSN_ptr_check(port));
    *port = ife.ordinal;
    return VTSS_RC_OK;
}

/******************************************************************************/
// TSN_ptp_status_timeout()
// Timer = ptp_chk_timer
/******************************************************************************/
static void TSN_ptp_status_timeout(tsn_timer_t &timer, void *context)
{
    vtss_appl_ptp_clock_slave_ds_t ptp_status;
    appl_tsn_start_state_t         state;
    bool                           start;

    start = false;
    state = VTSS_APPL_TSN_INITIAL;

    {
        TSN_LOCK_SCOPE();

        TSN_global_state.time_passed++;
        switch (TSN_global_conf.procedure) {
        case VTSS_APPL_TSN_PROCEDURE_NONE:
            start = true;
            state = VTSS_APPL_TSN_START_IMMEDIATELY;
            T_IG(TSN_TRACE_GRP_TIMER, "Start immediately");
            break;

        case VTSS_APPL_TSN_PROCEDURE_TIME_ONLY:
            if (TSN_global_state.time_passed > TSN_global_conf.timeout) {
                start = true;
                state = VTSS_APPL_TSN_TIMED_OUT;
                T_IG(TSN_TRACE_GRP_TIMER, "Time only");
            } else {
                state = VTSS_APPL_TSN_WAITING_FOR_TIMEOUT;
            }

            break;

        case VTSS_APPL_TSN_PROCEDURE_TIME_AND_PTP:
            vtss_appl_ptp_clock_status_slave_ds_get(TSN_global_conf.ptp_port, &ptp_status);
            if (ptp_status.slave_state == VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKING ||
                ptp_status.slave_state == VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKED ) {
                T_IG(TSN_TRACE_GRP_TIMER, "Time %u and PTP sense: %s", TSN_global_state.time_passed,
                     ptp_status.slave_state == VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKING ? "Locking" : "Locked" );
                start = true;
                if (ptp_status.slave_state == VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKING) {
                    state = VTSS_APPL_TSN_PTP_LOCKING;
                } else {
                    state = VTSS_APPL_TSN_PTP_LOCKED;
                }
            } else if (TSN_global_state.time_passed > TSN_global_conf.timeout) {
                T_IG(TSN_TRACE_GRP_TIMER, "Time and PTP sense. Time %u State %d. Start due to timeout.", TSN_global_state.time_passed, ptp_status.slave_state);
                start = true;
                state = VTSS_APPL_TSN_PTP_TIMED_OUT;
            } else {
                state = VTSS_APPL_TSN_PTP_WAITING_FOR_LOCK;
            }

            break;

        default:
            T_EG(TSN_TRACE_GRP_TIMER, "Invalid procedure %u timeout %u ptp_port %u", TSN_global_conf.procedure, TSN_global_conf.timeout, TSN_global_conf.ptp_port);
            break;
        }

        TSN_global_state.start       = start;
        TSN_global_state.start_state = state;
    } // end of locked scope

    if (start) {
        TSN_tas_config_change();
        psfp_ptp_ready();
        // Done. We do not start the timer, and TSN_ptp_status_timeout will only be called after a cold reboot.
    } else {
        TSN_LOCK_SCOPE();
        tsn_timer_start(timer, PTP_CHECK_TIMER_TIMEOUT_VALUE_MS, false /* false = one shot */);
    }
}

/******************************************************************************/
// TSN_global_state_init()
/******************************************************************************/
static void TSN_global_state_init(void)
{
    TSN_LOCK_SCOPE();
    tsn_timer_init( TSN_global_state.ptp_chk_timer, "ptp check timer", -1, TSN_ptp_status_timeout, nullptr);
    tsn_timer_start(TSN_global_state.ptp_chk_timer, PTP_CHECK_TIMER_TIMEOUT_VALUE_MS, false /* don't repeat */);
    TSN_tas_init();
}

/******************************************************************************/
// TSN_default()
/******************************************************************************/
static void TSN_default(void)
{
    {
        TSN_LOCK_SCOPE();
        (void)vtss_appl_tsn_global_conf_default_get(&TSN_global_conf);
    }

    TSN_fp_default();
    TSN_tas_default();
}

/******************************************************************************/
// vtss_appl_tsn_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_tsn_capabilities_get(vtss_appl_tsn_capabilities_t *cap)
{
    VTSS_RC(TSN_ptr_check(cap));
    *cap = TSN_cap;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_tsn_global_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_tsn_global_conf_default_get(vtss_appl_tsn_global_conf_t *conf)
{
    VTSS_RC(TSN_ptr_check(conf));
    memset(conf, 0, sizeof(*conf));
    conf->procedure = VTSS_APPL_TSN_PROCEDURE_NONE;
    conf->timeout = 20;
    conf->ptp_port = 0;
    conf->clock_domain = 0;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_tsn_global_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_tsn_global_conf_get(vtss_appl_tsn_global_conf_t *conf)
{
    VTSS_RC(TSN_ptr_check(conf));
    TSN_LOCK_SCOPE();
    *conf = TSN_global_conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_tsn_global_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_tsn_global_conf_set(const vtss_appl_tsn_global_conf_t *conf)
{
    mesa_ts_conf_t tsn_global_conf;

    VTSS_RC(TSN_ptr_check(conf));

    TSN_LOCK_SCOPE();

    if (conf->timeout < 10 ) {
        return VTSS_APPL_TSN_WAIT_TIME_TOO_SMALL;
    }
    if (conf->timeout > 240 ) {
        return VTSS_APPL_TSN_WAIT_TIME_TOO_BIG;
    }
    if (conf->ptp_port > 3) {
        return VTSS_APPL_TSN_PTP_PORT_TOO_BIG;
    }
    if (conf->clock_domain > 2 ) {
        return VTSS_APPL_TSN_INVALID_CLOCK_DOMAIN;
    }

    if (conf->procedure < VTSS_APPL_TSN_PROCEDURE_NONE || conf->procedure > VTSS_APPL_TSN_PROCEDURE_TIME_AND_PTP) {
        return VTSS_APPL_TSN_INVALID_PROCEDURE;
    }

    // APPL-6091: Set tsn domain in MESA
    T_D("Set clock domain to %d", conf->clock_domain);
    tsn_global_conf.tsn_domain = conf->clock_domain;
    (void)mesa_ts_conf_set(NULL, &tsn_global_conf);

    TSN_global_conf = *conf;
    return VTSS_RC_OK;
}

/**
 * Get the global state.
 *
 * \param state [OUT] Pointer to structure receiving the global state.
 * \return Error code.
 */
mesa_rc vtss_appl_tsn_global_state_get(vtss_appl_tsn_global_state_t *state)
{
    VTSS_RC(TSN_ptr_check(state));
    TSN_LOCK_SCOPE();

    state->start_state  = TSN_global_state.start_state;
    state->time_passed  = TSN_global_state.time_passed;
    state->start        = TSN_global_state.start;

    return VTSS_RC_OK;
}

bool TSN_is_started()
{
    return TSN_started;
}

//******************************************************************************
// tsn_util_calc_cycle_time_nsec()
//******************************************************************************
uint32_t tsn_util_calc_cycle_time_nsec(uint32_t numerator, uint32_t denominator)
{
    u32 cycle_time_ns;
    if (denominator) { // Avoid division by zero
        cycle_time_ns = (u32)((numerator * 1000000000ULL) / denominator); // convert to nanoseconds
    } else {
        cycle_time_ns = 100000000; // Use 100 msec as default if denominator is zero
    }

    return cycle_time_ns;
}

/******************************************************************************/
// tsn_util_calculate_chip_base_time()
/******************************************************************************/
mesa_timestamp_t tsn_util_calculate_chip_base_time(mesa_timestamp_t admin_base_time, uint32_t chip_cycle_time)
{
    mesa_timestamp_t chip_base_time;
    mesa_timestamp_t tod;
    uint64_t         tc;

    (void)mesa_ts_timeofday_get(NULL, &tod, &tc);
    mesa_timeinterval_t two_seconds = 2 * MESA_ONE_MIA; // time as given in nano seconds
    mesa_timestamp_t earliest_start_time = tod + two_seconds;

    if (admin_base_time > earliest_start_time) {
        // base time is in the future, just start when arriving at base time
        chip_base_time = admin_base_time;
    } else {
        uint64_t N = (earliest_start_time - admin_base_time) / chip_cycle_time;
        chip_base_time = admin_base_time + N * chip_cycle_time;
    }

    return chip_base_time;
}

/******************************************************************************/
// tsn_util_current_time_get()
/******************************************************************************/
mesa_rc tsn_util_current_time_get(mesa_timestamp_t &tod)
{
    uint64_t tc;
    return mesa_ts_timeofday_get(nullptr, &tod, &tc);
}

/******************************************************************************/
// tsn_util_timestamp_to_str()
/******************************************************************************/
char *tsn_util_timestamp_to_str(char *buf, size_t sz, mesa_timestamp_t &ts)
{
    uint64_t seconds;

    if (!buf || sz < 15 /* 2^48 seconds */ + 1 /* . */ + 9 /* nanoseconds */ + 1 /* terminating NULL */) {
        return "Invalid buffer size";
    }

    seconds = ((uint64_t)ts.sec_msb << 32) | (uint64_t)ts.seconds;
    (void)snprintf(buf, sz, VPRI64u ".%09u", seconds, ts.nanoseconds);
    return buf;
}

/******************************************************************************/
// tsn_util_timestamp_to_iso8601()
// Returns a string of the following format:
// "YYYY-MM-ddTHH:mm:ss.SSSZ", e.g. "2024-01-27T17:33:16.841T". This requires
// 24 + terminating zero = 25 chars.
/******************************************************************************/
char *tsn_util_timestamp_to_iso8601(char *buf, size_t sz, mesa_timestamp_t &ts)
{
    // A time_t is 64 bits on a 64-bit platform, but only 32 bits on a 32-bit
    // platform. Moreover, a time_t is a *signed* integer, which causes only 31
    // 31 bits to be valid.
    // We make sure to support both, although 32-bit platforms can't represent
    // times after 03:14:07 UTC on 19th of January 2038.
    time_t    time = ((uint64_t)ts.sec_msb << 32) | (uint64_t)ts.seconds;
    struct tm timeinfo, *t = localtime_r(&time, &timeinfo);
    size_t    len;

    if (!buf || sz < TSN_UTIL_ISO8601_STRING_SIZE) {
        return "Invalid buffer size";
    }

    (void)strftime(buf, sz, "%Y-%m-%dT%H:%M:%S", t);

    len = strlen(buf);

    // Add milliseconds and a 'Z'
    snprintf(buf + len, sz - len, ".%03uZ", ts.nanoseconds / 1000000);

    return buf;
}

/******************************************************************************/
// tsn_error_txt()
/******************************************************************************/
const char *tsn_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_TSN_INVALID_ARGUMENT:
        return "Invalid argument (typically a NULL pointer) pass to function";
    case VTSS_APPL_TSN_PTP_TIME_NOT_READY:
        return "PTP Time not ready";
    case VTSS_APPL_TSN_WAIT_TIME_TOO_SMALL:
        return "Wait time too small";
    case VTSS_APPL_TSN_WAIT_TIME_TOO_BIG:
        return "Wait time too big";
    case VTSS_APPL_TSN_PTP_PORT_TOO_BIG:
        return "PTP port too big";
    case VTSS_APPL_TSN_INVALID_PROCEDURE:
        return "Invalid time procedure";
    case VTSS_APPL_FP_FEATURE:
        return "Frame Preemption feature not present";
    case VTSS_APPL_TAS_FEATURE:
        return "Time Aware Shaper (TAS) feature not present";
    case VTSS_APPL_FP_INVALID_IFINDEX:
        return "Invalid interface index. It must represent a port";
    case VTSS_APPL_FP_INVALID_VERIFY_TIME:
        return "Invalid verify time. It must be in range [1,128]";
    case VTSS_APPL_FP_INVALID_FRAG_SIZE:
        return "Invalid add_frag_size. It must be in range [0,3]";
    case VTSS_APPL_FP_CUT_THROUGH:
        return "It is not possible to enable both cut-through and frame preemption";
    case VTSS_APPL_FP_10G_COPPER_PORT_NOT_SUPPORTED:
        return "Frame Preemption not supported on 10G Copper port";
    case VTSS_APPL_TAS_INVALID_CYCLE_TIME:
        return "CycleTime must be equal to or greater than accumulated TimeInterval";
    case VTSS_APPL_TAS_CYCLE_TIME_TOO_SMALL:
        return "CycleTime is too small ";
    case VTSS_APPL_TAS_CYCLE_TIME_TOO_BIG:
        return "CycleTime is too big";
    case VTSS_APPL_TAS_CYCLE_TIME_EXTENSION_TOO_BIG:
        return "CycleTimeExtension is too big";
    case VTSS_APPL_TAS_CYCLE_TIME_EXTENSION_TOO_SMALL:
        return "CycleTimeExtension is too small";
    case VTSS_APPL_TAS_UNHANDLED_QUEUES:
        return "Not all queues are open during cycle.";
    case VTSS_APPL_TAS_ADMIN_CTRL_LIST_TOO_SHORT:
        return "ControlListLength must be > 0 when gate enabled and config change.";
    case VTSS_APPL_TAS_ADMIN_CTRL_LIST_TOO_LONG:
        return "ControlListLength is too long.";
    case VTSS_APPL_TAS_INVALID_ADMIN_BASE_TIME_SEC_MSB:
        return "AdminBaseTime sec msb must be 0";
    case VTSS_APPL_TAS_ADMIN_BASE_TIME_SEC_NANO_TOO_BIG:
        return "AdminBaseTime nanoseconds is too big";
    case VTSS_APPL_TAS_ADMIN_CYCLE_TIME_NUMERATOR_ZERO:
        return "CycleTimeNumerator must not be zero";
    case VTSS_APPL_TAS_ADMIN_CYCLE_TIME_DENOMINATOR_NOT_VALID:
        return "CycleTimeDenominator must either in milliseconds, microseconds or nanoseconds";
    case VTSS_APPL_TAS_ADMIN_CYCLE_TIME_DENOMINATOR_ZERO:
        return "CycleTimeDenominator must not be zero";
    case VTSS_APPL_TAS_INVALID_GATE_OPERATION:
        return "Invalid GateOperation";
    case VTSS_APPL_TAS_MAX_SDU_OUT_OF_RANGE:
        return "Maximum SDU size out of range";
    case VTSS_APPL_TAS_INVALID_QUEUE:
        return "Queue is out of range";
    case VTSS_APPL_TAS_INVALID_GCE:
        return "Gate Control Element index out of range";
    case VTSS_APPL_TAS_GCE_CONFIGURATION:
        return "Failed to configure Gate Control Element";
    case VTSS_APPL_TAS_TAS_CONFIGURATION:
        return "Failed to configure TAS";
    case VTSS_APPL_TAS_TAS_CONFIGURATION_BASETIME_NOT_FUTURE:
        return "AdminBaseTime must be in the future";
    default:
        return "TSN: Unknown error code";
    }
}

extern "C" int tsn_icli_cmd_register(void);
extern "C" int tsn_fp_icli_cmd_register(void);
extern "C" int tsn_tas_icli_cmd_register(void);

#ifdef VTSS_SW_OPTION_ICFG
extern mesa_rc tsn_icfg_init();
extern mesa_rc tsn_fp_icfg_init();
extern mesa_rc tsn_tas_icfg_init();
#endif

/******************************************************************************/
// Pre-declaration of JSON registration function.
/******************************************************************************/
#if defined(VTSS_SW_OPTION_JSON_RPC)
VTSS_PRE_DECLS void vtss_appl_tsn_json_init(void);
#endif /* defined(VTSS_SW_OPTION_PRIVATE_MIB) */

/******************************************************************************/
// tsn_init()
/******************************************************************************/
mesa_rc tsn_init(vtss_init_data_t *data)
{
    mesa_rc rc;
    switch (data->cmd) {
    case INIT_CMD_INIT:
        // Do not use MESA_CAP_(MESA_CAP_PORT_CNT), because that one will return
        // a number >= actual port count, and then some functions may fail (e.g.
        // vlan_mgmt_port_conf_get()).
        TSN_cap_port_cnt = port_count_max();

        TSN_capabilities_set();
        TSN_tas_data_init();
        (void)vtss_appl_tsn_global_conf_default_get(&TSN_global_conf);
        critd_init(&TSN_crit, "tsn", VTSS_MODULE_ID_TSN, CRITD_TYPE_MUTEX_RECURSIVE);
        tsn_icli_cmd_register();
        tsn_fp_icli_cmd_register();
        tsn_tas_icli_cmd_register();

#if defined(VTSS_SW_OPTION_JSON_RPC)
        vtss_appl_tsn_json_init();
#endif /* defined(VTSS_SW_OPTION_PRIVATE_MIB) */
        break;

    case INIT_CMD_START:
#ifdef VTSS_SW_OPTION_ICFG
        if ( VTSS_RC_OK != (rc = tsn_icfg_init())) {
            T_E("tsn_icfg_init failed");
        }
        if ( VTSS_RC_OK != (rc = tsn_fp_icfg_init())) {
            T_E("tsn_fp_icfg_init failed");
        }
        if ( VTSS_RC_OK != (rc = tsn_tas_icfg_init())) {
            T_E("tsn_tas_icfg_init failed");
        }
#endif
        tsn_timer_init(); // Timer to support delayed call of config_change in TAS and PSFP module
        if (TSN_cap.has_queue_frame_preemption) {
            /* Port change callback registration */
            if (port_change_register(VTSS_MODULE_ID_TSN, TSN_port_change_cb) != VTSS_RC_OK) {
                T_E("Unable to register port change callback");
            }
#if defined(VTSS_SW_OPTION_LLDP)
            /* LLDP change callback registration */
            lldp_mgmt_entry_updated_callback_register(TSN_lldp_change_cb);
#endif /* VTSS_SW_OPTION_LLDP */
        }

        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_GLOBAL) {
            TSN_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        // Initialize the global state
        TSN_global_state_init();
        TSN_default();
        break;

    case INIT_CMD_ICFG_LOADING_POST: {
        TSN_LOCK_SCOPE();
        if (!TSN_is_started()) {
            TSN_started = true;
            TSN_fp_port_state_set_all();
        }
        break;
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}

