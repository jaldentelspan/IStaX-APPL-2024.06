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

#include "psfp_trace.h"
#include "psfp_api.h"
#include "psfp_lock.hxx"          // For PSFP_LOCK_SCOPE()
#include "psfp_timer.hxx"         // For psfp_timer_XXX()
#include "stream_api.h"           // For stream_notif_table (expose)
#include "subject.hxx"            // For subject_main_thread
#include "vtss_tod_api.h"         // For mesa_timestamp_t::operators
#include "critd_api.h"            // For critd_t
#include <vtss/appl/psfp.h>
#include <vtss/appl/tsn.h>        // For vtss_appl_tsn_global_state_get()
#include <vtss/basics/expose/table-status.hxx>
#include <vtss/basics/notifications/event-handler.hxx>
#include <vtss/basics/memcmp-operator.hxx> // For VTSS_BASICS_MEMCMP_OPERATOR()

// API configuration can be seen with "debug api vxlat".
using namespace vtss;

//*****************************************************************************/
// Trace definitions
/******************************************************************************/
static vtss_trace_reg_t PSFP_trace_reg = {
    VTSS_TRACE_MODULE_ID, "psfp", "Per-Stream Filtering and Policing"
};

static vtss_trace_grp_t PSFP_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PSFP_TRACE_GRP_FLOW_METER] = {
        "flow-meter",
        "Flow-meter trace",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PSFP_TRACE_GRP_GATE] = {
        "gate",
        "Stream Gate trace",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PSFP_TRACE_GRP_FILTER] = {
        "filter",
        "Stream Filter trace",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PSFP_TRACE_GRP_API] =  {
        "api",
        "MESA calls",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PSFP_TRACE_GRP_STREAM] = {
        "stream",
        "Change in stream config",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PSFP_TRACE_GRP_TIMER] = {
        "timer",
        "Timer",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PSFP_TRACE_GRP_ICLI] = {
        "icli",
        "ICLI",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [PSFP_TRACE_GRP_NOTIF] = {
        "notif",
        "Notifications",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    }
};

VTSS_TRACE_REGISTER(&PSFP_trace_reg, PSFP_trace_grps);

critd_t                                 PSFP_crit;
static vtss_appl_psfp_capabilities_t    PSFP_cap;
static vtss_appl_psfp_flow_meter_conf_t PSFP_flow_meter_default_conf;
static vtss_appl_psfp_gate_conf_t       PSFP_gate_default_conf;
static vtss_appl_psfp_filter_conf_t     PSFP_filter_default_conf;
static bool                             PSFP_gate_chip_base_time_issue;
static uint64_t                         PSFP_gate_chip_base_time_secs_max;
static bool                             PSFP_ptp_ready;

// Flow meter state and map
typedef struct {
    vtss_appl_psfp_flow_meter_conf_t conf;
    mesa_dlb_policer_id_t            dlb_policer_id;
} psfp_flow_meter_state_t;

typedef vtss::Map<vtss_appl_psfp_flow_meter_id_t, psfp_flow_meter_state_t> psfp_flow_meter_map_t;
typedef psfp_flow_meter_map_t::iterator                                    psfp_flow_meter_itr_t;
static  psfp_flow_meter_map_t                                              PSFP_flow_meter_map;

// Stream Gate state and map
typedef struct {
    vtss_appl_psfp_gate_id_t     gate_id;
    vtss_appl_psfp_gate_conf_t   conf;
    vtss_appl_psfp_gate_status_t status;

    // We need a timer on some platforms, because a base-time larger than 20
    // bits away from current time cannot be coped with by the chip.
    psfp_timer_t base_time_timer;
} psfp_gate_state_t;

typedef vtss::Map<vtss_appl_psfp_gate_id_t, psfp_gate_state_t> psfp_gate_map_t;
typedef psfp_gate_map_t::iterator                              psfp_gate_itr_t;
static  psfp_gate_map_t                                        PSFP_gate_map;

// Stream Filter state and map

// This indicates whether the PSFP filter could attach to a stream or a stream
// collection and whether it exists.
typedef struct {
    // true if stream exists.
    bool exists;

    // true if we couldn't take over the stream.
    bool attach_failed;
} psfp_stream_state_t;

typedef struct {
    vtss_appl_psfp_filter_id_t     filter_id;
    vtss_appl_psfp_filter_conf_t   conf;
    vtss_appl_psfp_filter_status_t status;
    psfp_stream_state_t            stream_state;

    bool using_stream_collection(void) const
    {
        return conf.stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE;
    }
} psfp_filter_state_t;

typedef vtss::Map<vtss_appl_psfp_filter_id_t, psfp_filter_state_t> psfp_filter_map_t;
typedef psfp_filter_map_t::iterator                                psfp_filter_itr_t;
static  psfp_filter_map_t                                          PSFP_filter_map;

/******************************************************************************/
// mesa_opt_bool_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_opt_bool_t &b)
{
    o << "{enable = " << b.enable
      << ", value = " << b.value
      << "}";

    return o;
}

/******************************************************************************/
// mesa_dlb_policer_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_dlb_policer_conf_t &conf)
{
    o << "{type = "          << (conf.type == MESA_POLICER_TYPE_MEF ? "MEF" : "Single")
      << ", enable = "       << conf.enable
      << ", cm = "           << (conf.cm ? "Aware" : "Blind")
      << ", cf = "           << conf.cf
      << ", line_Rate = "    << conf.line_rate
      << ", cir = "          << conf.cir
      << ", cbs = "          << conf.cbs
      << ", eir = "          << conf.eir
      << ", ebs = "          << conf.ebs
      << ", drop_yellow = "  << conf.drop_yellow
      << ", mark_all_red = " << conf.mark_all_red // Using mesa_opt_bool_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_dlb_policer_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_dlb_policer_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_dlb_policer_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_dlb_policer_status_t &status)
{
    o << "{mark_all_red = " << status.mark_all_red << "}";

    return o;
}

/******************************************************************************/
// mesa_dlb_policer_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_dlb_policer_status_t *status)
{
    o << *status;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_psfp_flow_meter_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_psfp_flow_meter_conf_t &conf)
{
    o << "{cir = "                         << conf.cir
      << ", cbs = "                        << conf.cbs
      << ", eir = "                        << conf.eir
      << ", ebs = "                        << conf.ebs
      << ", cf = "                         << conf.cf
      << ", cm = "                         << (conf.cm == VTSS_APPL_PSFP_FLOW_METER_CM_BLIND ? "Blind" : "Aware")
      << ", drop_on_yellow = "             << conf.drop_on_yellow
      << ", mark_all_frames_red_enable = " << conf.mark_all_frames_red_enable
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_psfp_flow_meter_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_psfp_flow_meter_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_psfp_gate_gce_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_psfp_gate_gce_conf_t &conf)
{
    o << "{gate_state = "          << psfp_util_gate_state_to_str(conf.gate_state)
      << ", ipv = "                << conf.ipv
      << ", time_interval_ns = "   << conf.time_interval_ns
      << ", interval_octet_max = " << conf.interval_octet_max
      << "}";

    return o;
}

// When tracing using operator<<(), we must know the size of the gcl_conf, so we
// wrap it into its own structure, whose members can also be used when setting
// MESA configuration.
typedef struct {
    uint32_t        length;
    mesa_psfp_gce_t conf[ARRSZ(vtss_appl_psfp_gate_conf_t::gcl)];
} psfp_gcl_conf_t;

/******************************************************************************/
// mesa_opt_prio_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_opt_prio_t &p)
{
    o << "{enable = " << p.enable
      << ", value = " << p.value
      << "}";

    return o;
}

/******************************************************************************/
// mesa_timestamp_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_timestamp_t &t)
{
    o << "{sec_msb = "          << t.sec_msb
      << ", seconds = "         << t.seconds
      << ", nanoseconds = "     << t.nanoseconds
      << ", nanosecondsfrac = " << t.nanosecondsfrac
      << "}";

    return o;
}

/******************************************************************************/
// mesa_timestamp_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_timestamp_t *t)
{
    o << *t;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_psfp_gce_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_psfp_gce_t &conf)
{
    o << "{gate_open = "      << conf.gate_open
      << ", prio = "          << conf.prio          // Using mesa_opt_prio_t::operator<<()
      << ", time_interval = " << conf.time_interval
      << ", octet_max = "     << conf.octet_max
      << "}";

    return o;
}

/******************************************************************************/
// psfp_gcl_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const psfp_gcl_conf_t &conf)
{
    bool first;
    int  i;

    o << "{length = " << conf.length
      << ", conf = {";

    first = true;
    for (i = 0; i < conf.length; i++) {
        o << (first ? "" : ", ") << "[" << i << "] = " << conf.conf[i]; // Using mesa_psfp_gce_t::operator<<()
    }

    o << "}";

    return o;
}

/******************************************************************************/
// psfp_gcl_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const psfp_gcl_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_psfp_gcl_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_psfp_gcl_conf_t &conf)
{
    o << "{base_time = "       << conf.base_time // Using mesa_timestamp_t::operator<<()
      << ", cycle_time = "     << conf.cycle_time
      << ", cycle_time_ext = " << conf.cycle_time_ext
      << "}";

    return o;
}

/******************************************************************************/
// mesa_psfp_gate_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_psfp_gate_conf_t &conf)
{
    o << "{enable = "                 << conf.enable
      << ", gate_open = "             << conf.gate_open
      << ", prio = "                  << conf.prio                  // Using mesa_opt_prio_t::operator<<()
      << ", close_invalid_rx = "      << conf.close_invalid_rx      // Using mesa_opt_bool_t::operator<<()
      << ", close_octets_exceeded = " << conf.close_octets_exceeded // Using mesa_opt_bool_t::operator<<()
      << ", config_change = "         << conf.config_change
      << ", config = "                << conf.config
      << "}";

    return o;
}

/******************************************************************************/
// mesa_psfp_gate_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_psfp_gate_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_psfp_gate_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_psfp_gate_conf_t &conf)
{
    int  i;

    o << "{gate_state = "                                << psfp_util_gate_state_to_str(conf.gate_state)
      << ", ipv = "                                      << conf.ipv
      << ", close_gate_due_to_invalid_rx_enable = "      << conf.close_gate_due_to_invalid_rx_enable
      << ", close_gate_due_to_octets_exceeded_enable = " << conf.close_gate_due_to_octets_exceeded_enable
      << ", cycle_time_ns = "                            << conf.cycle_time_ns
      << ", cycle_time_extension_ns = "                  << conf.cycle_time_extension_ns
      << ", base_time = "                                << conf.base_time // Using mesa_timestamp_t::operator<<()
      << ", gcl_length = "                               << conf.gcl_length
      << ", gate_enabled = "                             << conf.gate_enabled
      << ", config_change = "                            << conf.config_change;

    for (i = 0; i < conf.gcl_length; i++) {
        o << ", gcl[" << i << "] = " << conf.gcl[i]; // Using vtss_appl_psfp_gate_gce_conf_t::operator<<()
    }

    o << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_psfp_gate_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_psfp_gate_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_psfp_gate_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_psfp_gate_status_t &s)
{
    o << "{gate_open = "              << s.gate_open
      << ", prio = "                  << s.prio               // Using mesa_opt_prio_t::operator<<()
      << ", config_change_time = "    << s.config_change_time // Using mesa_timestamp_t::operator<<()
      << ", current_time = "          << s.current_time       // Using mesa_timestamp_t::operator<<()
      << ", config_pending = "        << s.config_pending
      << ", close_invalid_rx = "      << s.close_invalid_rx
      << ", close_octets_exceeded = " << s.close_octets_exceeded
      << "}";

    return o;
}

/******************************************************************************/
// mesa_psfp_gate_status_t::fmt
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_psfp_gate_status_t *s)
{
    o << *s;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_psfp_gate_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_psfp_gate_status_t &s)
{
    o << "{oper_conf = "                           << s.oper_conf          // Using vtss_appl_psfp_gate_conf_t::operator<<()
      << ", pend_conf = "                          << s.pend_conf          // Using vtss_appl_psfp_gate_conf_t::operator<<()
      << ", config_change_time = "                 << s.config_change_time // Using mesa_timestamp_t::operator<<()
      << ", current_time = "                       << s.current_time       // Using mesa_timestamp_t::operator<<()
      << ", tick_granularity = "                   << s.tick_granularity
      << ", config_pending = "                     << s.config_pending
      << ", config_change_errors = "               << s.config_change_errors
      << ", gate_closed_due_to_invalid_rx = "      << s.gate_closed_due_to_invalid_rx
      << ", gate_closed_due_to_octets_exceeded = " << s.gate_closed_due_to_octets_exceeded
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_psfp_gate_status_t::fmt
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_psfp_gate_status_t *s)
{
    o << *s;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_psfp_gate_control_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_psfp_gate_control_t &c)
{
    o << "{clear_gate_closed_due_to_invalid_rx = "       << c.clear_gate_closed_due_to_invalid_rx
      << ", clear_gate_closed_due_to_octets_exceeded = " << c.clear_gate_closed_due_to_octets_exceeded
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_psfp_gate_control_t::fmt
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_psfp_gate_control_t *c)
{
    o << *c;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_psfp_filter_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_psfp_filter_conf_t &conf)
{
    o << "{gate_enable = "     << conf.gate_enable
      << ", gate_id = "        << conf.gate_id
      << ", max_sdu = "        << conf.max_sdu
      << ", block_oversize = " << conf.block_oversize // Using mesa_opt_bool_t::operator<<()
      << "}";

    return o;
}

/******************************************************************************/
// mesa_psfp_filter_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_psfp_filter_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_psfp_filter_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_psfp_filter_status_t &s)
{
    o << "{block_oversize = " << s.block_oversize << "}";

    return o;
}

/******************************************************************************/
// mesa_psfp_filter_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_psfp_filter_status_t *s)
{
    o << *s;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_psfp_filter_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_psfp_filter_conf_t &conf)
{
    o << "{stream_id = "                           << conf.stream_id
      << ", stream_collection_id = "               << conf.stream_collection_id
      << ", flow_meter_id = "                      << conf.flow_meter_id
      << ", gate_id = "                            << conf.gate_id
      << ", max_sdu_size = "                       << conf.max_sdu_size
      << ", block_due_to_oversize_frame_enable = " << conf.block_due_to_oversize_frame_enable
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_psfp_filter_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_psfp_filter_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_psfp_filter_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_psfp_filter_status_t &s)
{
    char buf[500];

    o << "{stream_blocked_due_to_oversize_frame = " << s.stream_blocked_due_to_oversize_frame
      << ", oper_warnings = " << psfp_util_filter_oper_warnings_to_str(buf, sizeof(buf), s.oper_warnings)
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_psfp_filter_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_psfp_filter_status_t *s)
{
    o << *s;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_psfp_filter_control_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_psfp_filter_control_t &s)
{
    o << "{clear_blocked_due_to_oversize_frame = " << s.clear_blocked_due_to_oversize_frame << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_psfp_filter_control_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_psfp_filter_control_t *s)
{
    o << *s;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_psfp_filter_statistics_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_psfp_filter_statistics_t &s)
{
    o << "{matching = "         << s.matching
      << ", passing = "         << s.passing
      << ", not_passing = "     << s.not_passing
      << ", passing_sdu = "     << s.passing_sdu
      << ", not_passing_sdu = " << s.not_passing_sdu
      << ", red_frames = "      << s.red
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_psfp_filter_statistics_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_psfp_filter_statistics_t *s)
{
    o << *s;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// PSFP_filter_oper_warnings_stream_update()
/******************************************************************************/
static void PSFP_filter_oper_warnings_stream_update(psfp_filter_state_t &filter_state)
{
    vtss_appl_stream_collection_status_t  stream_collection_status;
    vtss_appl_stream_status_t             stream_status;
    vtss_appl_psfp_filter_oper_warnings_t &w = filter_state.status.oper_warnings;
    bool                                  using_stream_collection = filter_state.using_stream_collection();
    char                                  buf[500];

    w &= ~VTSS_APPL_PSFP_FILTER_OPER_WARNING_NO_STREAM_OR_STREAM_COLLECTION;
    w &= ~VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_NOT_FOUND;
    w &= ~VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_NOT_FOUND;
    w &= ~VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_ATTACH_FAIL;
    w &= ~VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_ATTACH_FAIL;
    w &= ~VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_HAS_OPERATIONAL_WARNINGS;
    w &= ~VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_HAS_OPERATIONAL_WARNINGS;

    T_IG(PSFP_TRACE_GRP_FILTER, "%u: conf = %s", filter_state.filter_id, filter_state.conf);

    if (filter_state.conf.stream_id == VTSS_APPL_STREAM_ID_NONE && filter_state.conf.stream_collection_id == VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        w |= VTSS_APPL_PSFP_FILTER_OPER_WARNING_NO_STREAM_OR_STREAM_COLLECTION;
    } else {
        if (!filter_state.stream_state.exists) {
            w |= using_stream_collection ? VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_NOT_FOUND : VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_NOT_FOUND;
        } else if (filter_state.stream_state.attach_failed) {
            // Something failed when we attempted to attach to the stream
            // (collection).
            w |= using_stream_collection ? VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_ATTACH_FAIL : VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_ATTACH_FAIL;
        } else {
            // Stream or stream collection exists and attachment went fine.

            if (using_stream_collection) {
                // Propagate warnings from the stream collection into the filter
                if (vtss_appl_stream_collection_status_get(filter_state.conf.stream_collection_id, &stream_collection_status) == VTSS_RC_OK &&
                    stream_collection_status.oper_warnings != VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NONE) {
                    w |= VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_HAS_OPERATIONAL_WARNINGS;
                }
            } else {
                // Propagate warnings from the stream into the filter
                if (vtss_appl_stream_status_get(filter_state.conf.stream_id, &stream_status) == VTSS_RC_OK &&
                    stream_status.oper_warnings != VTSS_APPL_STREAM_OPER_WARNING_NONE) {
                    w |= VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_HAS_OPERATIONAL_WARNINGS;
                }
            }
        }
    }

    T_IG(PSFP_TRACE_GRP_FILTER, "%u: Warnings = %s", filter_state.filter_id, psfp_util_filter_oper_warnings_to_str(buf, sizeof(buf), w));
}

/******************************************************************************/
// PSFP_filter_oper_warnings_flow_meter_gate_update()
/******************************************************************************/
static void PSFP_filter_oper_warnings_flow_meter_gate_update(psfp_filter_state_t &filter_state)
{
    vtss_appl_psfp_filter_oper_warnings_t &w = filter_state.status.oper_warnings;
    psfp_gate_itr_t                       gate_itr;
    char                                  buf[500];

    w &= ~VTSS_APPL_PSFP_FILTER_OPER_WARNING_FLOW_METER_NOT_FOUND;
    w &= ~VTSS_APPL_PSFP_FILTER_OPER_WARNING_GATE_NOT_FOUND;
    w &= ~VTSS_APPL_PSFP_FILTER_OPER_WARNING_GATE_NOT_ENABLED;

    if (filter_state.conf.flow_meter_id != VTSS_APPL_PSFP_FLOW_METER_ID_NONE) {
        if (PSFP_flow_meter_map.find(filter_state.conf.flow_meter_id) == PSFP_flow_meter_map.end()) {
            w |= VTSS_APPL_PSFP_FILTER_OPER_WARNING_FLOW_METER_NOT_FOUND;
        }
    }

    if (filter_state.conf.gate_id != VTSS_APPL_PSFP_GATE_ID_NONE) {
        if ((gate_itr = PSFP_gate_map.find(filter_state.conf.gate_id)) == PSFP_gate_map.end()) {
            w |= VTSS_APPL_PSFP_FILTER_OPER_WARNING_GATE_NOT_FOUND;
        } else if (!gate_itr->second.conf.gate_enabled) {
            w |= VTSS_APPL_PSFP_FILTER_OPER_WARNING_GATE_NOT_ENABLED;
        }
    }

    T_IG(PSFP_TRACE_GRP_FILTER, "%u: Warnings = %s", filter_state.filter_id, psfp_util_filter_oper_warnings_to_str(buf, sizeof(buf), w));
}

/******************************************************************************/
// PSFP_mesa_gcl_conf_set()
/******************************************************************************/
static mesa_rc PSFP_mesa_gcl_conf_set(vtss_appl_psfp_gate_id_t gate_id, const psfp_gcl_conf_t &gcl)
{
    mesa_rc rc;

    T_IG(PSFP_TRACE_GRP_API, "mesa_psfp_gcl_conf_set(%u, %s)", gate_id, gcl);
    if ((rc = mesa_psfp_gcl_conf_set(nullptr, gate_id, gcl.length, gcl.conf)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "mesa_psfp_gcl_conf_set(%u, %s) failed: %s", gate_id, gcl, error_txt(rc));
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_mesa_gate_conf_get()
/******************************************************************************/
static mesa_rc PSFP_mesa_gate_conf_get(psfp_gate_state_t &gate_state, mesa_psfp_gate_conf_t &m_gate_conf)
{
    mesa_rc rc;

    if ((rc = mesa_psfp_gate_conf_get(nullptr, gate_state.gate_id, &m_gate_conf)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "mesa_psfp_gate_conf_get(%u) failed: %s", gate_state.gate_id, error_txt(rc));
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    T_IG(PSFP_TRACE_GRP_API, "mesa_psfp_gate_conf_get(%u) => %s", gate_state.gate_id, m_gate_conf);
    return VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_mesa_gate_conf_set()
/******************************************************************************/
static mesa_rc PSFP_mesa_gate_conf_set(vtss_appl_psfp_gate_id_t gate_id, const mesa_psfp_gate_conf_t &m_gate_conf)
{
    mesa_rc rc;

    T_IG(PSFP_TRACE_GRP_API, "mesa_psfp_gate_conf_set(%u, %s)", gate_id, m_gate_conf);
    if ((rc = mesa_psfp_gate_conf_set(nullptr, gate_id, &m_gate_conf)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "mesa_psfp_gate_conf_set(%u, %s) failed: %s", gate_id, m_gate_conf, error_txt(rc));
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_gate_base_time_timer_update()
// Returns true if the S/W-based timer is set active, false otherwise.
/******************************************************************************/
static bool PSFP_gate_base_time_timer_update(psfp_gate_state_t &gate_state, mesa_timestamp_t &current_time)
{
    mesa_timestamp_t base_time;
    uint64_t         when_secs, timeout_secs;

    if (!PSFP_gate_chip_base_time_issue) {
        // This chip doesn't suffer from the base-time issue.
        return false;
    }

    base_time = gate_state.status.pend_conf.base_time;
    if (base_time > current_time) {
        // We must apply the configuration at some point in time in the future.
        // Figure out how far into the future this is.

        // Let's compute when it happens ourselves, rather than relying on
        // vtss_tod_api.h's mesa_timestamp_t::operator-(), which returns the
        // number of nanoseconds.
        when_secs = base_time.seconds - current_time.seconds;

        if (when_secs > PSFP_gate_chip_base_time_secs_max) {
            // The base-time is more than the chip's maximum supported base-time
            // away from current time. Start a S/W-baed timer. The timer value
            // cannot exceed PSFP_gate_chip_base_time_secs_max, because the
            // H/W must be updated with new base-times whenever we get close to
            // the currently configured base-time. This is to prevent the
            // applied configuration to take effect before the real base-time.
            // We stop the timer 10 seconds before the real base time is reached
            // in order for this function to be called again, so that we can
            // re-apply the real base and let the chip itself do the work.
            timeout_secs = PSFP_gate_chip_base_time_secs_max - 10;

            T_IG(PSFP_TRACE_GRP_GATE, "%u: Starting timer. base_time = %s, current_time = %s, when = " VPRI64u " seconds, timeout = " VPRI64u " seconds", gate_state.gate_id, base_time, current_time, when_secs, timeout_secs);
            psfp_timer_start(gate_state.base_time_timer, timeout_secs * 1000 /* ms */, false /* never repeat */);
            return true;
        }
    }

    // Still here? The configured base-time is well within the chip-configurable
    // base-time. Stop a possibly active base-time timer.
    if (psfp_timer_active(gate_state.base_time_timer)) {
        T_IG(PSFP_TRACE_GRP_GATE, "%u: Stopping timer", gate_state.gate_id);
        psfp_timer_stop(gate_state.base_time_timer);
    }

    return false;
}

/******************************************************************************/
// PSFP_gate_mesa_status_get()
/******************************************************************************/
static mesa_rc PSFP_gate_mesa_status_get(psfp_gate_state_t &gate_state, mesa_psfp_gate_status_t &m_status)
{
    mesa_rc rc;

    // Get current MESA gate status
    if ((rc = mesa_psfp_gate_status_get(nullptr, gate_state.gate_id, &m_status)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "mesa_psfp_gate_status_get(%u) failed: %s", gate_state.gate_id, error_txt(rc));
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    T_IG(PSFP_TRACE_GRP_API, "mesa_psfp_gate_status_get(%u) => %s", gate_state.gate_id, m_status);
    return VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_gate_status_update()
/******************************************************************************/
static mesa_rc PSFP_gate_status_update(psfp_gate_state_t &gate_state, const mesa_psfp_gate_status_t &m_status)
{
    vtss_appl_psfp_gate_status_t &a_status    = gate_state.status;
    bool                         timer_active = psfp_timer_active(gate_state.base_time_timer);

    T_IG(PSFP_TRACE_GRP_GATE, "%u: m.config_pending = %d, a.config_pending = %d, timer_active = %d", gate_state.gate_id, m_status.config_pending, a_status.config_pending, timer_active);

    if (timer_active) {
        // Configuration applied to MESA, but with a base-time which is not the
        // real base time, because the chip can't cope with the real base time.
        if (!a_status.config_pending) {
            // a_status.config_pending must already be true.
            T_EG(PSFP_TRACE_GRP_GATE, "%u: Timer is active, but S/W-based config_pending is false", gate_state.gate_id);
        }

        a_status.config_change_time = a_status.pend_conf.base_time;
    } else {
        // The real base-time is not controlled by a S/W-based timer. When we
        // go from S/W-based config-pending to H/W-based NOT config-pending, the
        // hardware has adopted the pending configuration.
        if (a_status.config_pending && !m_status.config_pending) {
            // Transfer the pending configuration to the operational
            // configuration.
            T_IG(PSFP_TRACE_GRP_GATE, "%u: Transferring pend_conf to oper_conf", gate_state.gate_id);
            a_status.oper_conf          = a_status.pend_conf;
            a_status.oper_conf_valid    = true;
            a_status.config_pending     = false;
        } else if (!a_status.config_pending && m_status.config_pending) {
            T_EG(PSFP_TRACE_GRP_GATE, "%u: S/W thinks no configuration is pending, while H/W does", gate_state.gate_id);
        }

        a_status.config_change_time = m_status.config_change_time;
    }

    a_status.oper_ipv                           = m_status.prio.enable ? m_status.prio.value : -1;
    a_status.oper_gate_state                    = m_status.gate_open ? VTSS_APPL_PSFP_GATE_STATE_OPEN : VTSS_APPL_PSFP_GATE_STATE_CLOSED;
    a_status.current_time                       = m_status.current_time;
    a_status.tick_granularity                   = 1;
    a_status.gate_closed_due_to_invalid_rx      = m_status.close_invalid_rx;
    a_status.gate_closed_due_to_octets_exceeded = m_status.close_octets_exceeded;

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_gate_chip_base_time_calculate()
/******************************************************************************/
mesa_timestamp_t PSFP_gate_chip_base_time_calculate(psfp_gate_state_t &gate_state, const mesa_timestamp_t &current_time, bool timer_active)
{
    vtss_appl_psfp_gate_conf_t &pend_conf = gate_state.status.pend_conf;
    mesa_timestamp_t           sw_base_time, hw_base_time, earliest_start_time;
    mesa_timeinterval_t        two_seconds = 2 * MESA_ONE_MIA; // Two seconds measured in nanoseconds
    uint64_t                   n, cycle_time_ns, when_seconds;

    if (timer_active) {
        // The S/W-based timer is active, indicating that the chip cannot cope
        // with the new base-time. Configure the base time to the maximum value
        // supported by the chip. Don't use vtss_tod_api.h's
        // mesa_timestamp_t::operator+(), because it doesn't take seconds as the
        // right-hand-side operator.
        when_seconds = (uint64_t)current_time.seconds + PSFP_gate_chip_base_time_secs_max;
        vtss_clear(hw_base_time);
        hw_base_time.sec_msb = when_seconds >> 32;
        hw_base_time.seconds = when_seconds & 0xffffffff;
    } else {
        earliest_start_time = current_time + two_seconds;

        sw_base_time = pend_conf.base_time;
        if (sw_base_time > earliest_start_time) {
            // Base time is more than two seconds from now.
            hw_base_time = sw_base_time;
        } else {
            // Base time was in the past or within two seconds from now.
            // If gcl_length is zero, the cycle time may be configured as 0, in
            // case a division-by-zero would occur here. So adjust it to 1 ns.
            cycle_time_ns = pend_conf.cycle_time_ns != 0 ? pend_conf.cycle_time_ns : 1;

            n = (earliest_start_time - sw_base_time) / cycle_time_ns;
            hw_base_time = sw_base_time + n * cycle_time_ns;
        }
    }

    return hw_base_time;
}

/******************************************************************************/
// PSFP_mesa_gate_conf_del()
/******************************************************************************/
static void PSFP_mesa_gate_conf_del(vtss_appl_psfp_gate_id_t gate_id)
{
    mesa_psfp_gate_conf_t  m_conf = {};
    psfp_gcl_conf_t        m_gcl  = {};

    // We gotta go through both these calls whether the first fails or not.
    T_IG(PSFP_TRACE_GRP_GATE, "%u: Removing", gate_id);

    (void)PSFP_mesa_gcl_conf_set( gate_id, m_gcl);
    (void)PSFP_mesa_gate_conf_set(gate_id, m_conf);
}

/******************************************************************************/
// PSFP_gate_update()
/******************************************************************************/
static mesa_rc PSFP_gate_update(psfp_gate_state_t &gate_state, bool timeout = false)
{
    mesa_psfp_gate_status_t      m_status;                      // MESA
    mesa_psfp_gate_conf_t        m_conf;                        // MESA
    psfp_gcl_conf_t              m_gcl     = {};                // MESA
    vtss_appl_psfp_gate_conf_t   &a_conf   = gate_state.conf;   // Application
    vtss_appl_psfp_gate_status_t &a_status = gate_state.status; // Application
    int                          i;
    bool                         gate_was_already_enabled, timer_active;

    T_IG(PSFP_TRACE_GRP_GATE, "%u: Enter", gate_state.gate_id);

    // Get current MESA gate configuration
    VTSS_RC(PSFP_mesa_gate_conf_get(gate_state, m_conf));

    // Save whether the current configuration has the gate enabled.
    gate_was_already_enabled = m_conf.enable;

    // Update initial gate configuration
    m_conf.enable      = a_conf.gate_enabled;
    m_conf.gate_open   = a_conf.gate_state == VTSS_APPL_PSFP_GATE_STATE_OPEN ? true : false;
    m_conf.prio.enable = a_conf.ipv != -1;
    m_conf.prio.value  = a_conf.ipv == -1 ? 0 : a_conf.ipv;

    if (!a_conf.gate_enabled) {
        // When the gate gets disabled in H/W, H/W also clears e.g.
        // config_pending, so so do we and stop a possibly ongoing timer.
        psfp_timer_stop(gate_state.base_time_timer);
        a_status.oper_conf_valid = false;
        a_status.config_pending  = false;
        a_conf.config_change     = false; // Ignore such requests on a disabled gate.
    }

    // PTP must be ready in order to apply the full configuration, because
    // otherwise, we cannot trust the current time.
    if (a_conf.gate_enabled && PSFP_ptp_ready && (a_conf.config_change || timeout)) {
        // Apply full configuration (or update base-time only).

        // a_conf.config_change and timeout cannot be set simultaneously!
        // a_conf.config-change is a one-shot parameter.
        if (a_conf.config_change && timeout) {
            T_EG(PSFP_TRACE_GRP_GATE, "%u: config_change and timeout are set simultaneously", gate_state.gate_id);
        }

        // If a_conf.config_change, we transfer all the currently configured
        // parameters to status.pend_conf. This indicates the configuration to
        // be adopted by hardware when base_time is reached.
        if (a_conf.config_change) {
            // User has issued a config-change.
            a_status.pend_conf      = a_conf;
            a_status.config_pending = true;
        }

        // We need the current time before we invoke the next function, but we
        // cannot update the status yet.
        VTSS_RC(PSFP_gate_mesa_status_get(gate_state, m_status));

        // Figure out whether we need to start a S/W-based timer to overcome a
        // chip-issue where we cannot set the absolute base-time of the pending
        // configuration to a high value.
        timer_active = PSFP_gate_base_time_timer_update(gate_state, m_status.current_time);

        // Apply the pending conf (which might be the same as the currently
        // pending configuration if invoked by the base-time timer.

        // PSFP_mesa_gcl_conf_set() only updates S/W variables, which will only
        // be put into use when the stream gate gets configured further down.
        m_gcl.length = a_status.pend_conf.gcl_length;
        for (i = 0; i < m_gcl.length; i++) {
            mesa_psfp_gce_t                &m = m_gcl.conf[i];             // MESA
            vtss_appl_psfp_gate_gce_conf_t &a = a_status.pend_conf.gcl[i]; // Application

            m.gate_open     = a.gate_state == VTSS_APPL_PSFP_GATE_STATE_OPEN ? true : false;
            m.prio.enable   = a.ipv != -1;
            m.prio.value    = a.ipv == -1 ? 0 : a.ipv;
            m.time_interval = a.time_interval_ns;
            m.octet_max     = a.interval_octet_max;
        }

        PSFP_mesa_gcl_conf_set(gate_state.gate_id, m_gcl);

        // m_conf.close_invalid_rx.value and m_conf.close_octets_exceeded.value
        // are retained from the status if enabled. They can only be cleared
        // by disabling the detection or by a call to
        // vtss_appl_psfp_gate_control_set().
        m_conf.close_invalid_rx.enable      = a_status.pend_conf.close_gate_due_to_invalid_rx_enable;
        m_conf.close_invalid_rx.value       = a_status.pend_conf.close_gate_due_to_invalid_rx_enable && m_status.close_invalid_rx;
        m_conf.close_octets_exceeded.enable = a_status.pend_conf.close_gate_due_to_octets_exceeded_enable;
        m_conf.close_octets_exceeded.value  = a_status.pend_conf.close_gate_due_to_octets_exceeded_enable && m_status.close_octets_exceeded;
        m_conf.config.cycle_time            = a_status.pend_conf.cycle_time_ns;
        m_conf.config.cycle_time_ext        = a_status.pend_conf.cycle_time_extension_ns;
        m_conf.config_change                = true; // Apply the following configuration fields

        // The following function calculates the config change time as follows:
        // If the pending base_time is more than two seconds into the future, we
        // use that one, unless the S/W-based timer runs, in which case we use
        // the maximum supported value. Otherwise, we use an integral number of
        // cycle times on top of the current time plus two seconds into the
        // future.
        // We must call the function even if the requested base-time is too far
        // away from the current time for the chip to cope with, because we
        // cannot allow the chip to change operating values before we actually
        // reach the requested base-time. In that case, the remaining parameters
        // are actually the same - only base-time gets changed.
        m_conf.config.base_time = PSFP_gate_chip_base_time_calculate(gate_state, m_status.current_time, timer_active);

        T_IG(PSFP_TRACE_GRP_GATE, "%u: Applying full configuration. Invoked by timeout = %s", gate_state.gate_id, timeout ? "yes" : "no");
        VTSS_RC(PSFP_mesa_gate_conf_set(gate_state.gate_id, m_conf));

        // We also need to update config_change_errors.
        // According to 802.1Q-2022, clause 8.6.9.3.1 (SetConfigChangeTime())
        // bullet c), this is done when AdminBaseTime specifies a time in the
        // past and the current schedule is running.
        if (gate_was_already_enabled && m_conf.enable && a_status.pend_conf.base_time < m_status.current_time) {
            a_status.config_change_errors++;
        }

        // The a_conf.config_change is a one-shot parameter.
        a_conf.config_change = false;
    } else {
        // Attributes not related to gate control list are always applied.
        T_IG(PSFP_TRACE_GRP_GATE, "%u: Applying limited configuration", gate_state.gate_id);
        m_conf.config_change = false;
        VTSS_RC(PSFP_mesa_gate_conf_set(gate_state.gate_id, m_conf));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_gate_base_time_timeout()
/******************************************************************************/
static void PSFP_gate_base_time_timeout(psfp_timer_t &timer, const void *context)
{
    psfp_gate_itr_t gate_itr;

    if ((gate_itr = PSFP_gate_map.find(timer.instance)) == PSFP_gate_map.end()) {
        T_EG(PSFP_TRACE_GRP_GATE, "Unable to find gate instance #%u in map", timer.instance);
        return;
    }

    // Go and see if we can apply the configuration now.
    T_IG(PSFP_TRACE_GRP_GATE, "%u: Timeout", gate_itr->first);
    (void)PSFP_gate_update(gate_itr->second, true /* signal that we need to update the chip's base-time */);
}

/******************************************************************************/
// PSFP_mesa_filter_conf_set()
/******************************************************************************/
static mesa_rc PSFP_mesa_filter_conf_set(vtss_appl_psfp_filter_id_t filter_id, const mesa_psfp_filter_conf_t &m_filter_conf)
{
    mesa_rc rc;

    T_IG(PSFP_TRACE_GRP_API, "mesa_psfp_filter_conf_set(%u, %s)", filter_id, m_filter_conf);
    if ((rc = mesa_psfp_filter_conf_set(nullptr, filter_id, &m_filter_conf)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "mesa_psfp_filter_conf_set(%u, %s)", filter_id, m_filter_conf);
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_filter_mesa_status_get()
/******************************************************************************/
static mesa_rc PSFP_filter_mesa_status_get(vtss_appl_psfp_filter_id_t filter_id, mesa_psfp_filter_status_t &m_status)
{
    mesa_rc rc;

    if ((rc = mesa_psfp_filter_status_get(nullptr, filter_id, &m_status)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "mesa_psfp_filter_status_get(%u) failed: %s", filter_id, error_txt(rc));
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    T_IG(PSFP_TRACE_GRP_API, "mesa_psfp_filter_status_get(%u) => %s", filter_id, m_status);
    return VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_stream_or_stream_collection_detach()
/******************************************************************************/
static void PSFP_stream_or_stream_collection_detach(psfp_filter_state_t &filter_state)
{
    vtss_appl_stream_id_t            stream_id;
    vtss_appl_stream_collection_id_t stream_collection_id;
    vtss_appl_stream_action_t        action = {};
    mesa_rc                          rc;

    T_IG(PSFP_TRACE_GRP_FILTER, "%u: Enter. stream_id = %u, stream_collection_id = %u", filter_state.filter_id, filter_state.conf.stream_id, filter_state.conf.stream_collection_id);
    if (filter_state.stream_state.exists && !filter_state.stream_state.attach_failed) {
        if (filter_state.using_stream_collection()) {
            stream_collection_id = filter_state.conf.stream_collection_id;
            rc = vtss_appl_stream_collection_action_set(stream_collection_id, VTSS_APPL_STREAM_CLIENT_PSFP, &action);
            T_IG(PSFP_TRACE_GRP_FILTER, "%u: vtss_appl_stream_collection_action_set(%u, enable = false): %s", filter_state.filter_id, stream_collection_id, error_txt(rc));
        } else {
            // The stream (used) to exist and we have attached to it.
            stream_id = filter_state.conf.stream_id;
            rc = vtss_appl_stream_action_set(stream_id, VTSS_APPL_STREAM_CLIENT_PSFP, &action);
            T_IG(PSFP_TRACE_GRP_FILTER, "%u: vtss_appl_stream_action_set(%u, enable = false): %s", filter_state.filter_id, stream_id, error_txt(rc));
        }
    }

    filter_state.stream_state.exists        = false;
    filter_state.stream_state.attach_failed = false;

    // Update operational warnings based on what came out of the detachment.
    PSFP_filter_oper_warnings_stream_update(filter_state);
}

/******************************************************************************/
// PSFP_stream_action_get()
/******************************************************************************/
static void PSFP_stream_action_get(psfp_filter_state_t &filter_state, vtss_appl_stream_action_t &action)
{
    psfp_flow_meter_itr_t flow_meter_itr;
    mesa_dlb_policer_id_t dlb_policer_id;
    bool                  flow_meter_found;

    vtss_clear(action);

    action.enable    = true;
    action.client_id = filter_state.filter_id; // To identify ourselves

    flow_meter_found = false;
    if (filter_state.conf.flow_meter_id != VTSS_APPL_PSFP_FLOW_METER_ID_NONE) {
        if ((flow_meter_itr = PSFP_flow_meter_map.find(filter_state.conf.flow_meter_id)) == PSFP_flow_meter_map.end()) {
            dlb_policer_id = 0;
        } else {
            dlb_policer_id = flow_meter_itr->second.dlb_policer_id;
            flow_meter_found = true;
        }
    } else {
        dlb_policer_id = 0;
    }

    // We wish to enable cut through. The stream module arbitrates between us
    // and FRER.
    action.cut_through_override    = true;
    action.cut_through_disable     = false;
    action.psfp.dlb_enable         = flow_meter_found;
    action.psfp.dlb_id             = dlb_policer_id;
    action.psfp.psfp.filter_enable = true;
    action.psfp.psfp.filter_id     = filter_state.filter_id;
}

/******************************************************************************/
// PSFP_stream_attach()
/******************************************************************************/
static void PSFP_stream_attach(psfp_filter_state_t &filter_state)
{
    vtss_appl_stream_id_t     stream_id = filter_state.conf.stream_id;
    vtss_appl_stream_conf_t   stream_conf;
    vtss_appl_stream_status_t stream_status;
    vtss_appl_stream_action_t action;
    mesa_rc                   rc1, rc2;

    filter_state.stream_state.exists        = false;
    filter_state.stream_state.attach_failed = false;

    T_IG(PSFP_TRACE_GRP_FILTER, "%u: Enter. stream-id = %u", filter_state.filter_id, stream_id);

    if (stream_id == VTSS_APPL_STREAM_ID_NONE) {
        return;
    }

    if ((rc1 = vtss_appl_stream_conf_get(stream_id, &stream_conf)) != VTSS_RC_OK) {
        T_IG(PSFP_TRACE_GRP_FILTER, "%u: vtss_appl_stream_conf_get(%u) failed: %s", filter_state.filter_id, stream_id, error_txt(rc1));
    }

    if ((rc2 = vtss_appl_stream_status_get(stream_id, &stream_status)) != VTSS_RC_OK) {
        T_IG(PSFP_TRACE_GRP_FILTER, "%u: vtss_appl_stream_status_get(%u) failed: %s", filter_state.filter_id, stream_id, error_txt(rc2));
    }

    if (rc1 != VTSS_RC_OK || rc2 != VTSS_RC_OK) {
        // Stream doesn't exist. This is not an error, since it might not have
        // been created yet (or have been deleted).
        return;
    }

    // Stream exists
    filter_state.stream_state.exists = true;

    if (stream_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_PSFP].enable) {
        // We are already attached to the stream. Check that the ID is indeed
        // this instance's. Otherwise we have a problem, Houston, that requires
        // a code update. We shouldn't be able to get into this situation,
        // because of the checks at the bottom of
        // vtss_appl_psfp_filter_conf_set(): Two administratively enabled
        // instances cannot use the same stream ID.
        T_IG(PSFP_TRACE_GRP_FILTER, "%u: We are already attached to stream %u", filter_state.filter_id, stream_id);

        if (stream_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_PSFP].client_id != filter_state.filter_id) {
            // This requires a code update, and is not an operational warning.
            T_EG(PSFP_TRACE_GRP_FILTER, "%u: Stream (%u) attached to by us, but on another PSFP instance (%u)", filter_state.filter_id, stream_id, stream_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_PSFP].client_id);
            return;
        }
    }

    // Here, we might or might already be attached to the stream. It could be
    // we need to change actions.
    PSFP_stream_action_get(filter_state, action);

    rc1 = vtss_appl_stream_action_set(stream_id, VTSS_APPL_STREAM_CLIENT_PSFP, &action);
    T_IG(PSFP_TRACE_GRP_FILTER, "%u: vtss_appl_stream_action_set(%u, enable = true): %s", filter_state.filter_id, stream_id, error_txt(rc1));

    filter_state.stream_state.attach_failed = rc1 != VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_stream_collection_attach()
/******************************************************************************/
static void PSFP_stream_collection_attach(psfp_filter_state_t &filter_state)
{
    vtss_appl_stream_collection_id_t     stream_collection_id = filter_state.conf.stream_collection_id;
    vtss_appl_stream_collection_conf_t   stream_collection_conf;
    vtss_appl_stream_collection_status_t stream_collection_status;
    vtss_appl_stream_action_t            action;
    mesa_rc                              rc1, rc2;

    filter_state.stream_state.exists        = false;
    filter_state.stream_state.attach_failed = false;

    T_IG(PSFP_TRACE_GRP_FILTER, "%u: Enter. stream-collection-id = %u", filter_state.filter_id, stream_collection_id);

    if (stream_collection_id == VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        return;
    }

    if ((rc1 = vtss_appl_stream_collection_conf_get(stream_collection_id, &stream_collection_conf)) != VTSS_RC_OK) {
        T_IG(PSFP_TRACE_GRP_FILTER, "%u: vtss_appl_stream_collection_conf_get(%u) failed: %s", filter_state.filter_id, stream_collection_id, error_txt(rc1));
    }

    if ((rc2 = vtss_appl_stream_collection_status_get(stream_collection_id, &stream_collection_status)) != VTSS_RC_OK) {
        T_IG(PSFP_TRACE_GRP_FILTER, "%u: vtss_appl_stream_collection_status_get(%u) failed: %s", filter_state.filter_id, stream_collection_id, error_txt(rc2));
    }

    if (rc1 != VTSS_RC_OK || rc2 != VTSS_RC_OK) {
        // Stream collection doesn't exist. This is not an error, since it might
        // not have been created yet (or has been deleted).
        return;
    }

    // Stream collection exists
    filter_state.stream_state.exists = true;

    if (stream_collection_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_PSFP].enable) {
        // We are already attached to the stream collection . Check that the ID
        // is indeed this instance's. Otherwise we have a problem, Houston, that
        // requires a code update. We shouldn't be able to get into this
        // situation, because of the checks at the bottom of
        // vtss_appl_psfp_filter_conf_set(): Two administratively enabled
        // instances cannot use the same stream collection ID.
        T_IG(PSFP_TRACE_GRP_FILTER, "%u: We are already attached to stream collection %u", filter_state.filter_id, stream_collection_id);

        if (stream_collection_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_PSFP].client_id != filter_state.filter_id) {
            // This requires a code update, and is not an operational warning.
            T_EG(PSFP_TRACE_GRP_FILTER, "%u: Stream collection (%u) attached to by us, but on another PSFP instance (%u)", filter_state.filter_id, stream_collection_id, stream_collection_status.client_status.clients[VTSS_APPL_STREAM_CLIENT_PSFP].client_id);
            return;
        }
    }

    // Here, we might or might already be attached to the stream collection. It
    // could be we need to change actions.
    PSFP_stream_action_get(filter_state, action);

    rc1 = vtss_appl_stream_collection_action_set(stream_collection_id, VTSS_APPL_STREAM_CLIENT_PSFP, &action);
    T_IG(PSFP_TRACE_GRP_FILTER, "%u: vtss_appl_stream_collection_action_set(%u, enable = true): %s", filter_state.filter_id, stream_collection_id, error_txt(rc1));

    filter_state.stream_state.attach_failed = rc1 != VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_stream_or_stream_collection_attach()
/******************************************************************************/
static void PSFP_stream_or_stream_collection_attach(psfp_filter_state_t &filter_state)
{
    T_IG(PSFP_TRACE_GRP_FILTER, "%u: Enter. stream_id = %u, stream_collection_id = %u", filter_state.filter_id, filter_state.conf.stream_id, filter_state.conf.stream_collection_id);
    if (filter_state.using_stream_collection()) {
        PSFP_stream_collection_attach(filter_state);
    } else {
        PSFP_stream_attach(filter_state);
    }

    // Update operational warnings based on what came out of the attachment.
    PSFP_filter_oper_warnings_stream_update(filter_state);
}

/******************************************************************************/
// PSFP_filter_flow_meter_conf_changed()
// Invoked whenever a flow meter gets created or gets deleted.
/******************************************************************************/
static void PSFP_filter_flow_meter_conf_changed(vtss_appl_psfp_flow_meter_id_t flow_meter_id)
{
    psfp_filter_itr_t filter_itr;

    // Update all stream filters that use this flow meter.
    for (filter_itr = PSFP_filter_map.begin(); filter_itr != PSFP_filter_map.end(); ++filter_itr) {
        if (filter_itr->second.conf.flow_meter_id == flow_meter_id) {
            // Re-attach to stream to set new actions.
            PSFP_stream_or_stream_collection_attach(filter_itr->second);

            // Update operational warnings
            PSFP_filter_oper_warnings_flow_meter_gate_update(filter_itr->second);
        }
    }
}

/******************************************************************************/
// PSFP_flow_meter_conf_del()
/******************************************************************************/
static mesa_rc PSFP_flow_meter_conf_del(psfp_flow_meter_itr_t &flow_meter_itr)
{
    vtss_appl_psfp_flow_meter_id_t flow_meter_id  = flow_meter_itr->first;
    mesa_dlb_policer_id_t          dlb_policer_id = flow_meter_itr->second.dlb_policer_id;
    psfp_filter_itr_t              filter_itr;
    mesa_rc                        rc;

    // Start by erasing the flow meter from the map. This ensures that when
    // stream actions and operational warnings for stream filters that use this
    // flow meter get updated, the flow meter is no longer available.
    PSFP_flow_meter_map.erase(flow_meter_itr);

    // Then update stream filter actions and operational warnings
    PSFP_filter_flow_meter_conf_changed(flow_meter_id);

    // And finally delete the policer. This must be done after the stream
    // actions are set in order not to reference a policer that doesn't exist,
    // which would be the case right after the policer got deleted and before
    // the stream actions were set.
    T_IG(PSFP_TRACE_GRP_API, "%u: mesa_dlb_policer_free(%u)", flow_meter_id, dlb_policer_id);
    if ((rc = mesa_dlb_policer_free(nullptr, dlb_policer_id)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "%u: mesa_dlb_policer_free(%u) failed: %s", flow_meter_id, dlb_policer_id, error_txt(rc));
        rc = VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    return rc;
}

/******************************************************************************/
// PSFP_mesa_filter_conf_update()
/******************************************************************************/
static mesa_rc PSFP_mesa_filter_conf_update(psfp_filter_state_t &filter_state)
{
    vtss_appl_psfp_gate_id_t  gate_id = filter_state.conf.gate_id;
    mesa_psfp_filter_conf_t   m_filter_conf;
    mesa_psfp_filter_status_t m_status;
    bool                      gate_exists;

    gate_exists = gate_id != VTSS_APPL_PSFP_GATE_ID_NONE && PSFP_gate_map.find(gate_id) != PSFP_gate_map.end();

    vtss_clear(m_filter_conf);
    m_filter_conf.gate_enable           = gate_exists;
    m_filter_conf.gate_id               = gate_exists ? gate_id : 0;
    m_filter_conf.max_sdu               = filter_state.conf.max_sdu_size;
    m_filter_conf.block_oversize.enable = filter_state.conf.block_due_to_oversize_frame_enable;

    VTSS_RC(PSFP_filter_mesa_status_get(filter_state.filter_id, m_status));

    // m_filter_conf.block_oversize.value is retained from the status if
    // enabled. It can only be cleared by disabling the detection or by a call
    // to vtss_appl_psfp_filter_control_set().
    m_filter_conf.block_oversize.value = m_filter_conf.block_oversize.enable && m_status.block_oversize;

    return PSFP_mesa_filter_conf_set(filter_state.filter_id, m_filter_conf);
}

/******************************************************************************/
// PSFP_filter_gate_conf_changed()
// Invoked whenever a gate gets created or deleted or gate_enable changes.
// When update_filter_conf is true, it's because the gate just got created or
// deleted. If false, it's because of a change in the gate's enable state.
/******************************************************************************/
static void PSFP_filter_gate_conf_changed(vtss_appl_psfp_gate_id_t gate_id, bool update_filter_conf)
{
    psfp_filter_itr_t filter_itr;

    // Update all stream filters that use this stream gate.
    for (filter_itr = PSFP_filter_map.begin(); filter_itr != PSFP_filter_map.end(); ++filter_itr) {
        if (filter_itr->second.conf.gate_id == gate_id) {
            if (update_filter_conf) {
                // Update the filter configuration, because the gate now exists
                // or doesn't exist.
                (void)PSFP_mesa_filter_conf_update(filter_itr->second);
            }

            // Update operational warnings
            PSFP_filter_oper_warnings_flow_meter_gate_update(filter_itr->second);
        }
    }
}

/******************************************************************************/
// PSFP_gate_conf_del()
/******************************************************************************/
static void PSFP_gate_conf_del(psfp_gate_itr_t &gate_itr)
{
    vtss_appl_psfp_gate_id_t gate_id = gate_itr->first;

    // Stop a possible running base-time timer.
    psfp_timer_stop(gate_itr->second.base_time_timer);

    // Remove the gate from our map. This must be done before we update all
    // filters that use this gate, because they must know that the gate is now
    // gone.
    PSFP_gate_map.erase(gate_itr);

    // Update all filters that use the gate. This should be done before we
    // change the gate's configuration.
    PSFP_filter_gate_conf_changed(gate_id, true);

    // Clear the gate configuration.
    PSFP_mesa_gate_conf_del(gate_id);
}

/******************************************************************************/
// PSFP_filter_conf_del()
/******************************************************************************/
static void PSFP_filter_conf_del(psfp_filter_itr_t &filter_itr)
{
    mesa_psfp_filter_conf_t m_filter_conf = {};

    // Detach from stream/stream collection
    PSFP_stream_or_stream_collection_detach(filter_itr->second);

    (void)PSFP_mesa_filter_conf_set(filter_itr->first, m_filter_conf);

    PSFP_filter_map.erase(filter_itr);
}

/******************************************************************************/
// PSFP_default()
/******************************************************************************/
static void PSFP_default(void)
{
    psfp_filter_itr_t     filter_itr,     next_filter_itr;
    psfp_gate_itr_t       gate_itr,       next_gate_itr;
    psfp_flow_meter_itr_t flow_meter_itr, next_flow_meter_itr;

    PSFP_LOCK_SCOPE();

    filter_itr = PSFP_filter_map.begin();
    while (filter_itr != PSFP_filter_map.end()) {
        next_filter_itr = filter_itr;
        ++next_filter_itr;
        PSFP_filter_conf_del(filter_itr);
        filter_itr = next_filter_itr;
    }

    gate_itr = PSFP_gate_map.begin();
    while (gate_itr != PSFP_gate_map.end()) {
        next_gate_itr = gate_itr;
        ++next_gate_itr;
        PSFP_gate_conf_del(gate_itr);
        gate_itr = next_gate_itr;
    }

    flow_meter_itr = PSFP_flow_meter_map.begin();
    while (flow_meter_itr != PSFP_flow_meter_map.end()) {
        next_flow_meter_itr = flow_meter_itr;
        ++next_flow_meter_itr;
        (void)PSFP_flow_meter_conf_del(flow_meter_itr);
        flow_meter_itr = next_flow_meter_itr;
    }
}

/******************************************************************************/
// PSFP_capabilities_set()
/******************************************************************************/
static void PSFP_capabilities_set(void)
{
    vtss_appl_stream_capabilities_t            stream_cap;
    vtss_appl_stream_collection_capabilities_t stream_collection_cap;
    mesa_chip_family_t                         chip_family;
    mesa_rc                                    rc;

    PSFP_cap.psfp_supported = fast_cap(MESA_CAP_L2_PSFP);

    if (!PSFP_cap.psfp_supported) {
        return;
    }

    if ((rc = vtss_appl_stream_capabilities_get(&stream_cap)) != VTSS_RC_OK) {
        T_E("vtss_appl_stream_capabilities_get() failed: %s", error_txt(rc));
        stream_cap.inst_cnt_max = 127;
    }

    if ((rc = vtss_appl_stream_collection_capabilities_get(&stream_collection_cap)) != VTSS_RC_OK) {
        T_E("vtss_appl_stream_collection_capabilities_get() failed: %s", error_txt(rc));
        stream_collection_cap.inst_cnt_max = 63;
    }

    // Make a copy of the maximum number of streams that can be created.
    PSFP_cap.stream_id_max = stream_cap.inst_cnt_max;

    // Make a copy of the maximum stream collection ID that can be created.
    PSFP_cap.stream_collection_id_max = stream_collection_cap.inst_cnt_max;

    PSFP_cap.max_filter_instances         = fast_cap(MESA_CAP_L2_PSFP_FILTER_CNT);
    PSFP_cap.max_gate_instances           = fast_cap(MESA_CAP_L2_PSFP_GATE_CNT);
    PSFP_cap.max_flow_meter_instances     = fast_cap(MESA_CAP_L2_PSFP_FILTER_CNT); // Dynamic limitations may cause system to run out of resources sooner
    PSFP_cap.gate_control_list_length_max = VTSS_APPL_PSFP_GATE_LIST_LENGTH_MAX;   // According to Claus

    // On some chips, we cannot set the base time more than ~12 days away
    // from current time, so these chips require special handling.
    chip_family = (mesa_chip_family_t)fast_cap(MESA_CAP_MISC_CHIP_FAMILY);
    PSFP_gate_chip_base_time_issue = chip_family == MESA_CHIP_FAMILY_SPARX5 || chip_family == MESA_CHIP_FAMILY_LAN969X;

    if (PSFP_gate_chip_base_time_issue) {
        // This chip has a base-time configuration issue. The thing is that
        // the chip clock is so fast, that it was impossible to close timing
        // with a full 48-bit seconds comparison. so only 20 bits are used in
        // the comparison between configured base time and current time.
        // Setting the chip's base time more than 2^20 seconds away from current
        // time causes the chip to calculate a wrong change time and
        // config-pending flag. Therefore, we implement a software timer that
        // fires when the base-time is within a value that the chip accepts.

        // This is the maximum distance from current-time to configured
        // base-time the chip accepts. This yields 1048575 seconds, which is the
        // same as ~12 days into the future.
        PSFP_gate_chip_base_time_secs_max = (1 << 20) - 1;

        // If someone changes the maximum base time to a value that the
        // S/W-based timer cannot cope with, we must throw an assertion.
        // The S/W_based timer is started with an uint32_t measured in
        // milliseconds.
        VTSS_ASSERT(PSFP_gate_chip_base_time_secs_max < 0xFFFFFFFFUL / 1000UL);

        // We use a slack of 10 seconds in the function that computes the S/W
        // timeout, so the value must also be > 10.
        VTSS_ASSERT(PSFP_gate_chip_base_time_secs_max > 10);
    }
}

//*****************************************************************************/
// PSFP_default_conf_set()
// This is the only function that we need to change default parameters in.
/******************************************************************************/
static void PSFP_default_conf_set(void)
{
    size_t i;

    // Flow Meter defaults
    PSFP_flow_meter_default_conf.cir = 10000;
    PSFP_flow_meter_default_conf.cbs = 2048;

    // Stream Gate defaults
    PSFP_gate_default_conf.ipv = -1; // Disabled.
    for (i = 0; i < ARRSZ(PSFP_gate_default_conf.gcl); i++) {
        PSFP_gate_default_conf.gcl[i].time_interval_ns = 1;
        PSFP_gate_default_conf.gcl[i].ipv              = -1; // Disabled
    }

    // Stream Filter defaults
    PSFP_filter_default_conf.stream_id            = VTSS_APPL_STREAM_ID_NONE;
    PSFP_filter_default_conf.stream_collection_id = VTSS_APPL_STREAM_COLLECTION_ID_NONE;
    PSFP_filter_default_conf.gate_id              = VTSS_APPL_PSFP_GATE_ID_NONE;
    PSFP_filter_default_conf.flow_meter_id        = VTSS_APPL_PSFP_FLOW_METER_ID_NONE;
}

/******************************************************************************/
// PSFP_stream_id_check()
/******************************************************************************/
static mesa_rc PSFP_stream_id_check(vtss_appl_stream_id_t stream_id)
{
    // Stream IDs are numbered [1; max]
    return stream_id < 1 && stream_id > PSFP_cap.stream_id_max ? (mesa_rc)VTSS_APPL_PSFP_RC_STREAM_ID_OUT_OF_RANGE : VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_stream_collection_id_check()
/******************************************************************************/
static mesa_rc PSFP_stream_collection_id_check(vtss_appl_stream_collection_id_t stream_collection_id)
{
    // Stream collection IDs are numbered [1; max]
    return stream_collection_id < 1 && stream_collection_id > PSFP_cap.stream_collection_id_max ? (mesa_rc)VTSS_APPL_PSFP_RC_STREAM_COLLECTION_ID_OUT_OF_RANGE : VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_flow_meter_id_check()
/******************************************************************************/
static mesa_rc PSFP_flow_meter_id_check(vtss_appl_psfp_flow_meter_id_t flow_meter_id)
{
    // Flow meter instances are numbered [0; max[, which is a bit strange IMO.
    return flow_meter_id >= PSFP_cap.max_flow_meter_instances ? (mesa_rc)VTSS_APPL_PSFP_RC_FLOW_METER_ID_OUT_OF_RANGE : VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_gate_id_check()
/******************************************************************************/
static mesa_rc PSFP_gate_id_check(vtss_appl_psfp_gate_id_t gate_id)
{
    // Stream gate instances are numbered [0; max[, which is a bit strange IMO.
    return gate_id >= PSFP_cap.max_gate_instances ? (mesa_rc)VTSS_APPL_PSFP_RC_GATE_ID_OUT_OF_RANGE : VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_filter_id_check()
/******************************************************************************/
static mesa_rc PSFP_filter_id_check(vtss_appl_psfp_filter_id_t filter_id)
{
    // Stream filter instances are numbered [0; max[, which is a bit strange IMO
    return filter_id >= PSFP_cap.max_filter_instances ? (mesa_rc)VTSS_APPL_PSFP_RC_FILTER_ID_OUT_OF_RANGE : VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_mesa_dlb_policer_conf_get()
/******************************************************************************/
static mesa_rc PSFP_mesa_dlb_policer_conf_get(psfp_flow_meter_itr_t &flow_meter_itr, mesa_dlb_policer_conf_t &conf)
{
    mesa_rc rc;

    if ((rc = mesa_dlb_policer_conf_get(nullptr, flow_meter_itr->second.dlb_policer_id, 0, &conf)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "%u: mesa_dlb_policer_conf_get(%u) failed: %s", flow_meter_itr->first, flow_meter_itr->second.dlb_policer_id, error_txt(rc));
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    T_IG(PSFP_TRACE_GRP_API, "%u: mesa_dlb_policer_conf_get(%u) => %s", flow_meter_itr->first, flow_meter_itr->second.dlb_policer_id, conf);
    return VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_mesa_dlb_policer_conf_set()
/******************************************************************************/
static mesa_rc PSFP_mesa_dlb_policer_conf_set(psfp_flow_meter_itr_t &flow_meter_itr, const mesa_dlb_policer_conf_t &conf)
{
    mesa_rc rc;

    T_IG(PSFP_TRACE_GRP_API, "%u: mesa_dlb_policer_conf_set(%u, %s)", flow_meter_itr->first, flow_meter_itr->second.dlb_policer_id, conf);
    if ((rc = mesa_dlb_policer_conf_set(nullptr, flow_meter_itr->second.dlb_policer_id, 0, &conf)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "%u: mesa_dlb_policer_conf_set(%u) failed: %s", flow_meter_itr->first, flow_meter_itr->second.dlb_policer_id, error_txt(rc));
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// PSFP_mesa_dlb_policer_status_get()
/******************************************************************************/
static mesa_rc PSFP_mesa_dlb_policer_status_get(psfp_flow_meter_itr_t &flow_meter_itr, mesa_dlb_policer_status_t &dlb_policer_status)
{
    mesa_rc rc;

    if ((rc = mesa_dlb_policer_status_get(nullptr, flow_meter_itr->second.dlb_policer_id, 0, &dlb_policer_status)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "%u: mesa_dlb_policer_status_get(%u) failed: %s", flow_meter_itr->first, flow_meter_itr->second.dlb_policer_id, error_txt(rc));
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    T_IG(PSFP_TRACE_GRP_API, "%u: mesa_dlb_policer_status_get(%u) => %s", flow_meter_itr->first, flow_meter_itr->second.dlb_policer_id, dlb_policer_status);
    return VTSS_RC_OK;
}

//*****************************************************************************/
// PSFP_stream_change_notifications.
// Snoops on changes to the stream change notification structure to be able to
// react to changes in configurations of streams.
/******************************************************************************/
static struct psfp_stream_change_notifications_t : public vtss::notifications::EventHandler {
    vtss::notifications::Event     e;
    stream_notif_table_t::Observer o;

    psfp_stream_change_notifications_t() : EventHandler(&vtss::notifications::subject_main_thread), e(this)
    {
    }

    void init()
    {
        stream_notif_table.observer_new(&e);
    }

    void execute(vtss::notifications::Event *event)
    {
        psfp_filter_itr_t filter_itr;

        // The observer_get() moves all the events captured per key into #o.
        // This object contains a map, events, whose events->first is a
        // vtss_appl_stream_id_t and whose events->second is an integer
        // indicating whether event->first has been added (EventType::Add; 1),
        // modified (EventType::Modify; 2), or deleted (EventType::Delete; 3).
        // The observer runs a state machine, so that it only needs to return
        // one value per key. So if e.g. first an Add, then a Delete operation
        // was performed on a key before this execute() function got invoked,
        // the observer's event map would have EventType::None (0), which would
        // be erased from the map, so that we don't get to see it.
        stream_notif_table.observer_get(&e, o);

        for (auto i = o.events.cbegin(); i != o.events.cend(); ++i) {
            T_DG(PSFP_TRACE_GRP_STREAM, "%u: %s event", i->first, vtss::notifications::EventType::txt[vtss::notifications::EventType::E(i->second)].valueName);
        }

        PSFP_LOCK_SCOPE();

        // Loop through all stream filters
        for (filter_itr = PSFP_filter_map.begin(); filter_itr != PSFP_filter_map.end(); ++filter_itr) {
            psfp_filter_state_t &filter_state = filter_itr->second;

            if (filter_state.conf.stream_id == VTSS_APPL_STREAM_ID_NONE) {
                // No streams attached to this PSFP filter.
                continue;
            }

            if (o.events.find(filter_state.conf.stream_id) == o.events.end()) {
                // No events on this stream ID.
                continue;
            }

            // Update our attachment to the stream.
            PSFP_stream_or_stream_collection_attach(filter_itr->second);
        }
    }
} PSFP_stream_change_notifications;

//*****************************************************************************/
// PSFP_stream_collection_change_notifications.
// Snoops on changes to the stream collection change notification structure to
// be able to react to changes in configurations of streams collections.
/******************************************************************************/
static struct psfp_stream_collection_change_notifications_t : public vtss::notifications::EventHandler {
    vtss::notifications::Event     e;
    stream_notif_table_t::Observer o;

    psfp_stream_collection_change_notifications_t() : EventHandler(&vtss::notifications::subject_main_thread), e(this)
    {
    }

    void init()
    {
        stream_collection_notif_table.observer_new(&e);
    }

    void execute(vtss::notifications::Event *event)
    {
        psfp_filter_itr_t filter_itr;

        // The observer_get() moves all the events captured per key into #o.
        // This object contains a map, events, whose events->first is a
        // vtss_appl_stream_collection_id_t and whose events->second is an
        // integer indicating whether event->first has been added
        // (EventType::Add; 1), modified (EventType::Modify; 2), or deleted
        // (EventType::Delete; 3).
        // The observer runs a state machine, so that it only needs to return
        // one value per key. So if e.g. first an Add, then a Delete operation
        // was performed on a key before this execute() function got invoked,
        // the observer's event map would have EventType::None (0), which would
        // be erased from the map, so that we don't get to see it.
        stream_collection_notif_table.observer_get(&e, o);

        for (auto i = o.events.cbegin(); i != o.events.cend(); ++i) {
            T_DG(PSFP_TRACE_GRP_STREAM, "%s: %s event to stream collection", i->first, vtss::notifications::EventType::txt[vtss::notifications::EventType::E(i->second)].valueName);
        }

        PSFP_LOCK_SCOPE();

        // Loop through all stream filters
        for (filter_itr = PSFP_filter_map.begin(); filter_itr != PSFP_filter_map.end(); ++filter_itr) {
            psfp_filter_state_t &filter_state = filter_itr->second;

            if (filter_state.conf.stream_collection_id == VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
                // No streams attached to this PSFP filter.
                continue;
            }

            if (o.events.find(filter_state.conf.stream_collection_id) == o.events.end()) {
                // No events on this stream ID.
                continue;
            }

            // Update our attachment to the stream collection.
            PSFP_stream_or_stream_collection_attach(filter_itr->second);
        }
    }
} PSFP_stream_collection_change_notifications;

/******************************************************************************/
// PSFP_supported()
/******************************************************************************/
static mesa_rc PSFP_supported(void)
{
    return PSFP_cap.psfp_supported ? VTSS_RC_OK : (mesa_rc)VTSS_APPL_PSFP_RC_NOT_SUPPORTED;
}

/******************************************************************************/
// PSFP_ptr_check()
/******************************************************************************/
static mesa_rc PSFP_ptr_check(const void *ptr)
{
    return ptr == nullptr ? (mesa_rc)VTSS_APPL_PSFP_RC_INVALID_PARAMETER : VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_supported()
/******************************************************************************/
mesa_bool_t vtss_appl_psfp_supported(void)
{
    return PSFP_supported() == VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_psfp_capabilities_get(vtss_appl_psfp_capabilities_t *cap)
{
    VTSS_RC(PSFP_ptr_check(cap));

    *cap = PSFP_cap;
    return VTSS_RC_OK;
}

//------------------------------------------------------------------------------
// Public Flow Meter functions.
//------------------------------------------------------------------------------

/******************************************************************************/
// vtss_appl_psfp_flow_meter_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_psfp_flow_meter_conf_default_get(vtss_appl_psfp_flow_meter_conf_t *conf)
{
    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_ptr_check(conf));

    *conf = PSFP_flow_meter_default_conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_flow_meter_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_psfp_flow_meter_conf_get(vtss_appl_psfp_flow_meter_id_t flow_meter_id, vtss_appl_psfp_flow_meter_conf_t *conf)
{
    psfp_flow_meter_itr_t flow_meter_itr;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_flow_meter_id_check(flow_meter_id));

    PSFP_LOCK_SCOPE();

    if ((flow_meter_itr = PSFP_flow_meter_map.find(flow_meter_id)) == PSFP_flow_meter_map.end()) {
        return VTSS_APPL_PSFP_RC_FLOW_METER_NO_SUCH_INSTANCE;
    }

    *conf = flow_meter_itr->second.conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_flow_meter_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_psfp_flow_meter_conf_set(vtss_appl_psfp_flow_meter_id_t flow_meter_id, const vtss_appl_psfp_flow_meter_conf_t *conf)
{
    psfp_flow_meter_itr_t     flow_meter_itr;
    mesa_dlb_policer_id_t     dlb_policer_id;
    mesa_dlb_policer_conf_t   dlb_policer_conf;
    mesa_dlb_policer_status_t dlb_policer_status;
    bool                      new_entry;
    mesa_rc                   rc;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_flow_meter_id_check(flow_meter_id));

    PSFP_LOCK_SCOPE();

    T_IG(PSFP_TRACE_GRP_FLOW_METER, "%u: %s", flow_meter_id, *conf);

    if ((flow_meter_itr = PSFP_flow_meter_map.find(flow_meter_id)) != PSFP_flow_meter_map.end()) {
        new_entry = false;

        if (memcmp(&flow_meter_itr->second.conf, conf, sizeof(flow_meter_itr->second.conf)) == 0) {
            // No changed
            T_IG(PSFP_TRACE_GRP_FLOW_METER, "No changes");
            return VTSS_RC_OK;
        }
    } else {
        new_entry = true;

        if ((rc = mesa_dlb_policer_alloc(nullptr, 1, &dlb_policer_id)) != VTSS_RC_OK) {
            T_IG(PSFP_TRACE_GRP_API, "%u: mesa_dlb_policer_alloc() => %u failed: %s", flow_meter_id, dlb_policer_id, error_txt(rc));
            return VTSS_APPL_PSFP_RC_FLOW_METER_OUT_OF_HW_RESOURCES;
        }

        T_IG(PSFP_TRACE_GRP_API, "%u: mesa_dlb_policer_alloc() => %u", flow_meter_id, dlb_policer_id);
    }

    // Create a new or update an existing entry
    if ((flow_meter_itr = PSFP_flow_meter_map.get(flow_meter_id)) == PSFP_flow_meter_map.end()) {
        return VTSS_APPL_PSFP_RC_OUT_OF_MEMORY;
    }

    if (new_entry) {
        vtss_clear(flow_meter_itr->second);
        flow_meter_itr->second.dlb_policer_id = dlb_policer_id;
    }

    dlb_policer_conf.type                = MESA_POLICER_TYPE_MEF;
    dlb_policer_conf.enable              = true;
    dlb_policer_conf.cm                  = conf->cm == VTSS_APPL_PSFP_FLOW_METER_CM_AWARE;
    dlb_policer_conf.cf                  = conf->cf;
    dlb_policer_conf.line_rate           = false;    // Use data rate policing (as opposed to line rate)
    dlb_policer_conf.cir                 = conf->cir;
    dlb_policer_conf.cbs                 = conf->cbs;
    dlb_policer_conf.eir                 = conf->eir;
    dlb_policer_conf.ebs                 = conf->ebs;
    dlb_policer_conf.drop_yellow         = conf->drop_on_yellow;
    dlb_policer_conf.mark_all_red.enable = conf->mark_all_frames_red_enable;

    // If current status marks all red, so should new - if enabled.
    VTSS_RC(PSFP_mesa_dlb_policer_status_get(flow_meter_itr, dlb_policer_status));
    dlb_policer_conf.mark_all_red.value = dlb_policer_status.mark_all_red && dlb_policer_conf.mark_all_red.enable;

    // Set new policer configuration.
    VTSS_RC(PSFP_mesa_dlb_policer_conf_set(flow_meter_itr, dlb_policer_conf));

    // Get it again, because the configuration adjusts itself to the closest
    // supported by hardware.
    VTSS_RC(PSFP_mesa_dlb_policer_conf_get(flow_meter_itr, dlb_policer_conf));

    flow_meter_itr->second.conf     = *conf;
    flow_meter_itr->second.conf.cir = dlb_policer_conf.cir;
    flow_meter_itr->second.conf.cbs = dlb_policer_conf.cbs;
    flow_meter_itr->second.conf.eir = dlb_policer_conf.eir;
    flow_meter_itr->second.conf.ebs = dlb_policer_conf.ebs;

    if (new_entry) {
        // Update all stream filters that use this flow meter.
        PSFP_filter_flow_meter_conf_changed(flow_meter_id);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_flow_meter_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_psfp_flow_meter_conf_del(vtss_appl_psfp_flow_meter_id_t flow_meter_id)
{
    psfp_flow_meter_itr_t flow_meter_itr;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_flow_meter_id_check(flow_meter_id));

    PSFP_LOCK_SCOPE();

    if ((flow_meter_itr = PSFP_flow_meter_map.find(flow_meter_id)) == PSFP_flow_meter_map.end()) {
        return VTSS_APPL_PSFP_RC_FLOW_METER_NO_SUCH_INSTANCE;
    }

    return PSFP_flow_meter_conf_del(flow_meter_itr);
}

/******************************************************************************/
// vtss_appl_psfp_flow_meter_itr()
/******************************************************************************/
mesa_rc vtss_appl_psfp_flow_meter_itr(const vtss_appl_psfp_flow_meter_id_t *prev, vtss_appl_psfp_flow_meter_id_t *next)
{
    psfp_flow_meter_itr_t flow_meter_itr;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_ptr_check(next));

    PSFP_LOCK_SCOPE();

    if (!prev || *prev == VTSS_APPL_PSFP_FLOW_METER_ID_NONE) {
        flow_meter_itr = PSFP_flow_meter_map.begin();
    } else {
        flow_meter_itr = PSFP_flow_meter_map.greater_than(*prev);
    }

    if (flow_meter_itr != PSFP_flow_meter_map.end()) {
        *next = flow_meter_itr->first;
        return VTSS_RC_OK;
    }

    // No next.
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_psfp_flow_meter_status_get()
/******************************************************************************/
mesa_rc vtss_appl_psfp_flow_meter_status_get(vtss_appl_psfp_flow_meter_id_t flow_meter_id, vtss_appl_psfp_flow_meter_status_t *status)
{
    psfp_flow_meter_itr_t     flow_meter_itr;
    mesa_dlb_policer_status_t dlb_policer_status;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_flow_meter_id_check(flow_meter_id));
    VTSS_RC(PSFP_ptr_check(status));

    PSFP_LOCK_SCOPE();

    if ((flow_meter_itr = PSFP_flow_meter_map.find(flow_meter_id)) == PSFP_flow_meter_map.end()) {
        return VTSS_APPL_PSFP_RC_FLOW_METER_NO_SUCH_INSTANCE;
    }

    VTSS_RC(PSFP_mesa_dlb_policer_status_get(flow_meter_itr, dlb_policer_status));
    vtss_clear(*status);
    status->mark_all_frames_red = dlb_policer_status.mark_all_red;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_flow_meter_control_set()
/******************************************************************************/
mesa_rc vtss_appl_psfp_flow_meter_control_set(vtss_appl_psfp_flow_meter_id_t flow_meter_id, const vtss_appl_psfp_flow_meter_control_t *ctrl)
{
    psfp_flow_meter_itr_t   flow_meter_itr;
    mesa_dlb_policer_conf_t dlb_policer_conf;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_flow_meter_id_check(flow_meter_id));
    VTSS_RC(PSFP_ptr_check(ctrl));

    PSFP_LOCK_SCOPE();

    if ((flow_meter_itr = PSFP_flow_meter_map.find(flow_meter_id)) == PSFP_flow_meter_map.end()) {
        return VTSS_APPL_PSFP_RC_FLOW_METER_NO_SUCH_INSTANCE;
    }

    if (!ctrl->clear_mark_all_frames_red) {
        // Nothing to do.
        return VTSS_RC_OK;
    }

    VTSS_RC(PSFP_mesa_dlb_policer_conf_get(flow_meter_itr, dlb_policer_conf));
    dlb_policer_conf.mark_all_red.value = false;
    VTSS_RC(PSFP_mesa_dlb_policer_conf_set(flow_meter_itr, dlb_policer_conf));

    return VTSS_RC_OK;
}

//------------------------------------------------------------------------------
// Public Stream Gate functions.
//------------------------------------------------------------------------------

/******************************************************************************/
// vtss_appl_psfp_gate_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_psfp_gate_conf_default_get(vtss_appl_psfp_gate_conf_t *gate_conf)
{
    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_ptr_check(gate_conf));

    *gate_conf = PSFP_gate_default_conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_gate_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_psfp_gate_conf_get(vtss_appl_psfp_gate_id_t gate_id, vtss_appl_psfp_gate_conf_t *gate_conf)
{
    psfp_gate_itr_t gate_itr;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_gate_id_check(gate_id));
    VTSS_RC(PSFP_ptr_check(gate_conf));

    PSFP_LOCK_SCOPE();

    if ((gate_itr = PSFP_gate_map.find(gate_id)) == PSFP_gate_map.end()) {
        return VTSS_APPL_PSFP_RC_GATE_NO_SUCH_INSTANCE;
    }

    *gate_conf = gate_itr->second.conf;

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_gate_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_psfp_gate_conf_set(vtss_appl_psfp_gate_id_t gate_id, const vtss_appl_psfp_gate_conf_t *gate_conf)
{
    vtss_appl_psfp_gate_conf_t local_conf;
    psfp_gate_itr_t            gate_itr;
    uint32_t                   total_time_interval_ns;
    bool                       new_entry, old_gate_enabled;
    size_t                     i;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_gate_id_check(gate_id));
    VTSS_RC(PSFP_ptr_check(gate_conf));

    T_IG(PSFP_TRACE_GRP_GATE, "gate_id = %u, conf = %s", gate_id, *gate_conf);

    // Take a copy of the user's configuration, so that we can change unused
    // fields on the fly.
    local_conf = *gate_conf;

    if (local_conf.gate_state != VTSS_APPL_PSFP_GATE_STATE_CLOSED &&
        local_conf.gate_state != VTSS_APPL_PSFP_GATE_STATE_OPEN) {
        return VTSS_APPL_PSFP_RC_GATE_INVALID_GATE_STATE;
    }

    if (local_conf.ipv < -1 || local_conf.ipv > 7) {
        return VTSS_APPL_PSFP_RC_GATE_INVALID_IPV_VALUE;
    }

    if (local_conf.cycle_time_ns > 1000000000) {
        return VTSS_APPL_PSFP_RC_GATE_INVALID_CYCLE_TIME;
    }

    if (local_conf.cycle_time_extension_ns > 1000000000) {
        return VTSS_APPL_PSFP_RC_GATE_INVALID_TIME_EXTENSION;
    }

    if (local_conf.gcl_length > PSFP_cap.gate_control_list_length_max) {
        return VTSS_APPL_PSFP_RC_GATE_INVALID_GCL_LEN;
    }

    for (i = 0; i < ARRSZ(local_conf.gcl); i++) {
        vtss_appl_psfp_gate_gce_conf_t &gce_conf = local_conf.gcl[i];

        if (i < local_conf.gcl_length) {
            // In use. Verify it.
            if (gce_conf.gate_state != VTSS_APPL_PSFP_GATE_STATE_CLOSED &&
                gce_conf.gate_state != VTSS_APPL_PSFP_GATE_STATE_OPEN) {
                return VTSS_APPL_PSFP_RC_GATE_INVALID_GCE_GATE_STATE;
            }

            if (gce_conf.ipv < -1 || gce_conf.ipv > 7) {
                return VTSS_APPL_PSFP_RC_GATE_INVALID_GCE_IPV_VALUE;
            }

            if (gce_conf.time_interval_ns < 1 || gce_conf.time_interval_ns >= 1000000000) {
                return VTSS_APPL_PSFP_RC_GATE_INVALID_GCE_TIME_INTERVAL;
            }

            // I don't dare making limitations on interval_octet_max. Small
            // values can be used to discard all frames, and I'm not sure if
            // that's the intent or existing customers have saved small values
            // to their startup-config.
        } else {
            // GCE is not in use (any longer). Default it.
            gce_conf = PSFP_gate_default_conf.gcl[i];
        }
    }

    // Check cycle time vs timer_intervals.
    // We can only do that if the instance gets (or is) enabled, because the
    // user may be re-configuring the entire gate. The value of the
    // config_change flag doesn't matter.
    if (local_conf.gate_enabled) {
        total_time_interval_ns = 0;

        for (i = 0; i < local_conf.gcl_length; i++) {
            total_time_interval_ns += local_conf.gcl[i].time_interval_ns;
        }

        if (total_time_interval_ns > local_conf.cycle_time_ns) {
            return VTSS_APPL_PSFP_RC_GATE_CYCLETIME_EXCEEDED;
        }
    }

    PSFP_LOCK_SCOPE();

    if ((gate_itr = PSFP_gate_map.find(gate_id)) != PSFP_gate_map.end()) {
        if (memcmp(&gate_itr->second.conf, &local_conf, sizeof(gate_itr->second.conf)) == 0) {
            // No changed
            T_IG(PSFP_TRACE_GRP_GATE, "No changes");
            return VTSS_RC_OK;
        }

        new_entry        = false;
        old_gate_enabled = gate_itr->second.conf.gate_enabled;
    } else {
        new_entry        = true;
        old_gate_enabled = false;
    }

    // Create a new or update an existing entry
    if ((gate_itr = PSFP_gate_map.get(gate_id)) == PSFP_gate_map.end()) {
        return VTSS_APPL_PSFP_RC_OUT_OF_MEMORY;
    }

    if (new_entry) {
        vtss_clear(gate_itr->second);
        gate_itr->second.gate_id = gate_id;

        // Whether or not we have a chip-issue with base-time, we create a timer
        // that will only be used if we do have a chip-issue. If we don't have
        // a chip-issue, we may call psfp_timer_stop() for code-simplicity,
        // which is why we create it.
        psfp_timer_init(gate_itr->second.base_time_timer, "Base-Time", gate_id, PSFP_gate_base_time_timeout, nullptr);
    }

    gate_itr->second.conf = local_conf;

    VTSS_RC(PSFP_gate_update(gate_itr->second));

    if (new_entry || old_gate_enabled != local_conf.gate_enabled) {
        // Update all PSFP filters that reference this gate.
        PSFP_filter_gate_conf_changed(gate_id, new_entry);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_gate_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_psfp_gate_conf_del(vtss_appl_psfp_gate_id_t gate_id)
{
    psfp_gate_itr_t gate_itr;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_gate_id_check(gate_id));

    PSFP_LOCK_SCOPE();

    if ((gate_itr = PSFP_gate_map.find(gate_id)) == PSFP_gate_map.end()) {
        return VTSS_APPL_PSFP_RC_GATE_NO_SUCH_INSTANCE;
    }

    PSFP_gate_conf_del(gate_itr);
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_gate_itr()
/******************************************************************************/
mesa_rc vtss_appl_psfp_gate_itr(const vtss_appl_psfp_gate_id_t *prev, vtss_appl_psfp_gate_id_t *next)
{
    psfp_gate_itr_t gate_itr;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_ptr_check(next));

    if (prev) {
        T_IG(PSFP_TRACE_GRP_GATE, "prev = %u", *prev);
    } else {
        T_IG(PSFP_TRACE_GRP_GATE, "prev = nullptr");
    }

    PSFP_LOCK_SCOPE();
    if (!prev || *prev == VTSS_APPL_PSFP_GATE_ID_NONE) {
        gate_itr = PSFP_gate_map.begin();
    } else {
        gate_itr = PSFP_gate_map.greater_than(*prev);
    }

    if (gate_itr != PSFP_gate_map.end()) {
        *next = gate_itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_psfp_gate_status_get()
/******************************************************************************/
mesa_rc vtss_appl_psfp_gate_status_get(vtss_appl_psfp_gate_id_t gate_id, vtss_appl_psfp_gate_status_t *status)
{
    psfp_gate_itr_t         gate_itr;
    mesa_psfp_gate_status_t m_status;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_gate_id_check(gate_id));
    VTSS_RC(PSFP_ptr_check(status));

    PSFP_LOCK_SCOPE();

    if ((gate_itr = PSFP_gate_map.find(gate_id)) == PSFP_gate_map.end()) {
        return VTSS_APPL_PSFP_RC_GATE_NO_SUCH_INSTANCE;
    }

    VTSS_RC(PSFP_gate_mesa_status_get(gate_itr->second, m_status));
    VTSS_RC(PSFP_gate_status_update(  gate_itr->second, m_status));

    *status = gate_itr->second.status;

    T_IG(PSFP_TRACE_GRP_GATE, "gate_id = %u. status = %s", gate_id, *status);
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_gate_control_set()
/******************************************************************************/
mesa_rc vtss_appl_psfp_gate_control_set(vtss_appl_psfp_gate_id_t gate_id, const vtss_appl_psfp_gate_control_t *control)
{
    psfp_gate_itr_t       gate_itr;
    mesa_psfp_gate_conf_t gate_conf;
    mesa_rc               rc;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_gate_id_check(gate_id));
    VTSS_RC(PSFP_ptr_check(control));

    PSFP_LOCK_SCOPE();

    if ((gate_itr = PSFP_gate_map.find(gate_id)) == PSFP_gate_map.end()) {
        return VTSS_APPL_PSFP_RC_GATE_NO_SUCH_INSTANCE;
    }

    if (!control->clear_gate_closed_due_to_invalid_rx && !control->clear_gate_closed_due_to_octets_exceeded) {
        // Nothing to do.
        return VTSS_RC_OK;
    }

    T_IG(PSFP_TRACE_GRP_GATE, "%u: %s", gate_id, *control);

    T_IG(PSFP_TRACE_GRP_API, "mesa_psfp_gate_conf_get(%u)", gate_id);
    if ((rc = mesa_psfp_gate_conf_get(NULL, gate_id, &gate_conf)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "mesa_psfp_gate_conf_get(%u) failed: %s", gate_id, error_txt(rc));
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    if (control->clear_gate_closed_due_to_invalid_rx) {
        gate_conf.close_invalid_rx.value = false;
    }

    if (control->clear_gate_closed_due_to_octets_exceeded) {
        gate_conf.close_octets_exceeded.value = false;
    }

    T_IG(PSFP_TRACE_GRP_API, "mesa_psfp_gate_conf_set(%u, %s)", gate_id, gate_conf);
    if ((rc = mesa_psfp_gate_conf_set(NULL, gate_id, &gate_conf)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "mesa_psfp_gate_conf_set(%u, %s) failed: %s", gate_id, gate_conf, error_txt(rc));
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    return VTSS_RC_OK;
}

//------------------------------------------------------------------------------
// Public Stream Filter functions.
//------------------------------------------------------------------------------

/******************************************************************************/
// vtss_appl_psfp_filter_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_psfp_filter_conf_default_get(vtss_appl_psfp_filter_conf_t *filter_conf)
{
    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_ptr_check(filter_conf));

    *filter_conf = PSFP_filter_default_conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_filter_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_psfp_filter_conf_get(vtss_appl_psfp_filter_id_t filter_id, vtss_appl_psfp_filter_conf_t *filter_conf)
{
    psfp_filter_itr_t filter_itr;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_filter_id_check(filter_id));
    VTSS_RC(PSFP_ptr_check(filter_conf));

    PSFP_LOCK_SCOPE();

    if ((filter_itr = PSFP_filter_map.find(filter_id)) == PSFP_filter_map.end()) {
        return VTSS_APPL_PSFP_RC_FILTER_NO_SUCH_INSTANCE;
    }

    *filter_conf = filter_itr->second.conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_filter_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_psfp_filter_conf_set(vtss_appl_psfp_filter_id_t filter_id, const vtss_appl_psfp_filter_conf_t *filter_conf)
{
    psfp_filter_itr_t filter_itr, filter_itr2;
    bool              new_entry;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_filter_id_check(filter_id));
    VTSS_RC(PSFP_ptr_check(filter_conf));

    T_IG(PSFP_TRACE_GRP_FILTER, "%u: conf = %s", filter_id, *filter_conf);

    if (filter_conf->stream_id != VTSS_APPL_STREAM_ID_NONE && filter_conf->stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        // Cannot attach to both a stream and a stream collection at the same
        // time.
        return VTSS_APPL_PSFP_RC_FILTER_STREAM_ID_AND_COLLECTION_ID_CANNOT_BE_USED_SIMULTANEOUSLY;
    }

    if (filter_conf->stream_id != VTSS_APPL_STREAM_ID_NONE) {
        VTSS_RC(PSFP_stream_id_check(filter_conf->stream_id));
    }

    if (filter_conf->stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        VTSS_RC(PSFP_stream_collection_id_check(filter_conf->stream_collection_id));
    }

    if (filter_conf->flow_meter_id != VTSS_APPL_PSFP_FLOW_METER_ID_NONE) {
        VTSS_RC(PSFP_flow_meter_id_check(filter_conf->flow_meter_id));
    }

    if (filter_conf->gate_id != VTSS_APPL_PSFP_GATE_ID_NONE) {
        VTSS_RC(PSFP_gate_id_check(filter_conf->gate_id));
    }

    // I don't dare making limitations on sdu_size_max. Small values can be used
    // to discard all frames, and I'm not sure if that's the intent or existing
    // customers have saved small values to their startup-config.

    PSFP_LOCK_SCOPE();

    if ((filter_itr = PSFP_filter_map.find(filter_id)) != PSFP_filter_map.end()) {
        if (memcmp(&filter_itr->second.conf, filter_conf, sizeof(filter_itr->second.conf)) == 0) {
            T_IG(PSFP_TRACE_GRP_FILTER, "No changes");
            return VTSS_RC_OK;
        }

        new_entry = false;
    } else {
        new_entry = true;
    }

    // Cross filter checks
    for (filter_itr2 = PSFP_filter_map.begin(); filter_itr2 != PSFP_filter_map.end(); ++filter_itr2) {
        if (filter_itr2 == filter_itr) {
            // Don't check against our selves.
            continue;
        }

        // Two PSFP filters cannot use the same stream ID.
        if (filter_conf->stream_id != VTSS_APPL_STREAM_ID_NONE && filter_conf->stream_id == filter_itr2->second.conf.stream_id) {
            return VTSS_APPL_PSFP_RC_FILTER_ANOTHER_FILTER_USING_SAME_STREAM_ID;
        }

        if (filter_conf->stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE && filter_conf->stream_collection_id == filter_itr2->second.conf.stream_collection_id) {
            return VTSS_APPL_PSFP_RC_FILTER_ANOTHER_FILTER_USING_SAME_STREAM_COLLECTION_ID;
        }
    }

    // Create a new or update an existing entry
    if ((filter_itr = PSFP_filter_map.get(filter_id)) == PSFP_filter_map.end()) {
        return VTSS_APPL_PSFP_RC_OUT_OF_MEMORY;
    }

    if (new_entry) {
        vtss_clear(filter_itr->second);
        filter_itr->second.filter_id = filter_id;
    } else {
        // Existing entry. If the stream_id or stream_collection_id has changed,
        // detach before attaching again. Use the old configuration.
        if (filter_conf->stream_id            != filter_itr->second.conf.stream_id ||
            filter_conf->stream_collection_id != filter_itr->second.conf.stream_collection_id) {
            // This function is clever enough to not detach from a non-attached
            // stream (collection).
            PSFP_stream_or_stream_collection_detach(filter_itr->second);
        }
    }

    filter_itr->second.conf = *filter_conf;

    // Always call the following function, whether or not the stream or stream
    // collection ID has changed, because some of the action parameters used
    // when attaching may have changed.
    PSFP_stream_or_stream_collection_attach(filter_itr->second);

    // The above function updated operational warnings related to streams and
    // stream collections. We also need to update those related to flow meter
    // and gate.
    PSFP_filter_oper_warnings_flow_meter_gate_update(filter_itr->second);

    // Time to update MESA
    return PSFP_mesa_filter_conf_update(filter_itr->second);
}

/******************************************************************************/
// vtss_appl_psfp_filter_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_psfp_filter_conf_del(vtss_appl_psfp_filter_id_t filter_id)
{
    psfp_filter_itr_t filter_itr;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_filter_id_check(filter_id));

    PSFP_LOCK_SCOPE();

    if ((filter_itr = PSFP_filter_map.find(filter_id)) == PSFP_filter_map.end()) {
        return VTSS_APPL_PSFP_RC_FILTER_NO_SUCH_INSTANCE;
    }

    PSFP_filter_conf_del(filter_itr);
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_filter_itr()
/******************************************************************************/
mesa_rc vtss_appl_psfp_filter_itr(const vtss_appl_psfp_filter_id_t *prev, vtss_appl_psfp_filter_id_t *next)
{
    psfp_filter_itr_t filter_itr;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_ptr_check(next));

    if (prev) {
        T_IG(PSFP_TRACE_GRP_FILTER, "prev = %u", *prev);
    } else {
        T_IG(PSFP_TRACE_GRP_FILTER, "prev = nullptr");
    }

    PSFP_LOCK_SCOPE();
    if (!prev || *prev == VTSS_APPL_PSFP_FILTER_ID_NONE) {
        filter_itr = PSFP_filter_map.begin();
    } else {
        filter_itr = PSFP_filter_map.greater_than(*prev);
    }

    if (filter_itr != PSFP_filter_map.end()) {
        *next = filter_itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

/******************************************************************************/
// vtss_appl_psfp_filter_status_get()
/******************************************************************************/
mesa_rc vtss_appl_psfp_filter_status_get(vtss_appl_psfp_filter_id_t filter_id, vtss_appl_psfp_filter_status_t *status)
{
    psfp_filter_itr_t         filter_itr;
    mesa_psfp_filter_status_t m_status;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_filter_id_check(filter_id));
    VTSS_RC(PSFP_ptr_check(status));

    PSFP_LOCK_SCOPE();

    if ((filter_itr = PSFP_filter_map.find(filter_id)) == PSFP_filter_map.end()) {
        return VTSS_APPL_PSFP_RC_FILTER_NO_SUCH_INSTANCE;
    }

    VTSS_RC(PSFP_filter_mesa_status_get(filter_id, m_status));
    *status = filter_itr->second.status;
    status->stream_blocked_due_to_oversize_frame = m_status.block_oversize;

    T_IG(PSFP_TRACE_GRP_FILTER, "%u: status = %s", filter_id, *status);
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_filter_control_set()
/******************************************************************************/
mesa_rc vtss_appl_psfp_filter_control_set(vtss_appl_psfp_filter_id_t filter_id, const vtss_appl_psfp_filter_control_t *ctrl)
{
    mesa_psfp_filter_conf_t m_filter_conf;
    mesa_rc                 rc;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_filter_id_check(filter_id));
    VTSS_RC(PSFP_ptr_check(ctrl));

    PSFP_LOCK_SCOPE();

    T_IG(PSFP_TRACE_GRP_FILTER, "%u: %s", filter_id, *ctrl);

    if (!ctrl->clear_blocked_due_to_oversize_frame) {
        // Nothing to do.
        return VTSS_RC_OK;
    }

    T_IG(PSFP_TRACE_GRP_API, "mesa_psfp_filter_conf_get(%u)", filter_id);
    if ((rc = mesa_psfp_filter_conf_get(nullptr, filter_id, &m_filter_conf)) != VTSS_RC_OK) {
        T_EG(PSFP_TRACE_GRP_API, "mesa_psfp_filter_conf_get(%u) failed: %s", filter_id, error_txt(rc));
        return VTSS_APPL_PSFP_RC_INTERNAL_ERROR;
    }

    m_filter_conf.block_oversize.value = false;

    return PSFP_mesa_filter_conf_set(filter_id, m_filter_conf);
}

/******************************************************************************/
// vtss_appl_psfp_filter_statistics_get()
/******************************************************************************/
mesa_rc vtss_appl_psfp_filter_statistics_get(vtss_appl_psfp_filter_id_t filter_id, vtss_appl_psfp_filter_statistics_t *statistics)
{
    psfp_filter_itr_t       filter_itr;
    mesa_ingress_counters_t counters;
    mesa_rc                 rc;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_filter_id_check(filter_id));
    VTSS_RC(PSFP_ptr_check(statistics));

    PSFP_LOCK_SCOPE();

    if ((filter_itr = PSFP_filter_map.find(filter_id)) == PSFP_filter_map.end()) {
        return VTSS_APPL_PSFP_RC_FILTER_NO_SUCH_INSTANCE;
    }

    if (filter_itr->second.using_stream_collection()) {
        rc = stream_collection_util_counters_get(filter_itr->second.conf.stream_collection_id, counters);
    } else {
        rc = stream_util_counters_get(filter_itr->second.conf.stream_id, counters);
    }

    if (rc == VTSS_RC_OK) {
        statistics->matching        = counters.rx_match;
        statistics->passing         = counters.rx_gate_pass;
        statistics->not_passing     = counters.rx_gate_discard;
        statistics->passing_sdu     = counters.rx_sdu_pass;
        statistics->not_passing_sdu = counters.rx_sdu_discard;
        statistics->red             = counters.rx_red.frames;
    } else {
        // If no stream is attached, just return empty counters. Otherwise, JSON
        // will not show any rows for this filter.
        vtss_clear(*statistics);
    }

    T_IG(PSFP_TRACE_GRP_FILTER, "%u: %s", filter_id, *statistics);

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_psfp_filter_statistics_clear()
/******************************************************************************/
mesa_rc vtss_appl_psfp_filter_statistics_clear(vtss_appl_psfp_filter_id_t filter_id)
{
    psfp_filter_itr_t filter_itr;

    VTSS_RC(PSFP_supported());
    VTSS_RC(PSFP_filter_id_check(filter_id));

    PSFP_LOCK_SCOPE();

    if ((filter_itr = PSFP_filter_map.find(filter_id)) == PSFP_filter_map.end()) {
        return VTSS_APPL_PSFP_RC_FILTER_NO_SUCH_INSTANCE;
    }

    T_IG(PSFP_TRACE_GRP_FILTER, "%u", filter_id);

    // Don't check for return code of the clear functions. It's not particularly
    // an error if no stream (collection) is attached to this filter.
    if (filter_itr->second.using_stream_collection()) {
        (void)stream_collection_util_counters_clear(filter_itr->second.conf.stream_collection_id);
    } else {
        (void)stream_util_counters_clear(filter_itr->second.conf.stream_id);
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// psfp_util_time_to_num_denom()
/******************************************************************************/
void psfp_util_time_to_num_denom(uint32_t time_ns, psfp_util_time_t &psfp_time)
{
    vtss_clear(psfp_time);

    if (time_ns == 0) {
        // 0 nanoseconds
        psfp_time.unit = PSFP_UTIL_TIME_UNIT_NSEC;
        return;
    }

    if (time_ns % 1000000 == 0) {
        // It's possible to express it in an integral number of milliseconds
        psfp_time.unit = PSFP_UTIL_TIME_UNIT_MSEC;
        psfp_time.numerator = time_ns / 1000000;
    } else if (time_ns % 1000 == 0) {
        // It's possible to express it in an integral number of microseconds
        psfp_time.unit = PSFP_UTIL_TIME_UNIT_USEC;
        psfp_time.numerator = time_ns / 1000;
    } else {
        // It's not possible to express it in milli- or microseconds
        psfp_time.unit = PSFP_UTIL_TIME_UNIT_NSEC;
        psfp_time.numerator = time_ns;
    }
}

/******************************************************************************/
// psfp_util_time_from_num_denom()
/******************************************************************************/
mesa_rc psfp_util_time_from_num_denom(uint32_t &time_ns, const psfp_util_time_t &psfp_time)
{
    uint64_t val, mul;

    // The thing is that with e.g. units of msec and numerator of a great value,
    // we can get an interval that exceeds the size of an uint32_t, so let's
    // compute the time into a 64-bit integer and set its value to the maximum
    // value supported by a 32-bit integer if it exceeds that value, so that
    // a subsequent call to vtss_appl_psfp_gate_conf_set() fails with a mesa_rc.
    switch (psfp_time.unit) {
    case PSFP_UTIL_TIME_UNIT_NSEC:
        mul = 1;
        break;

    case PSFP_UTIL_TIME_UNIT_USEC:
        mul = 1000;
        break;

    case PSFP_UTIL_TIME_UNIT_MSEC:
        mul = 1000000;
        break;

    default:
        return VTSS_APPL_PSFP_RC_INVALID_TIME_UNIT;
    }

    val = (uint64_t)psfp_time.numerator * mul;
    if (val > 0xFFFFFFFF) {
        val = 0xFFFFFFFF;
    }

    time_ns = (uint32_t)val;
    return VTSS_RC_OK;
}

/******************************************************************************/
// psfp_util_time_unit_to_str()
/******************************************************************************/
const char *psfp_util_time_unit_to_str(psfp_util_time_unit_t unit)
{
    switch (unit) {
    case PSFP_UTIL_TIME_UNIT_NSEC:
        return "ns";

    case PSFP_UTIL_TIME_UNIT_USEC:
        return "us";

    case PSFP_UTIL_TIME_UNIT_MSEC:
        return "ms";

    default:
        T_E("Invalid unit (%d)", unit);
        return "Unknown!";
    }
}

/******************************************************************************/
// psfp_util_gate_state_to_str()
/******************************************************************************/
const char *psfp_util_gate_state_to_str(vtss_appl_psfp_gate_state_t gate_state, bool capitals)
{
    switch (gate_state) {
    case VTSS_APPL_PSFP_GATE_STATE_CLOSED:
        return capitals ? "Closed" : "closed";

    case VTSS_APPL_PSFP_GATE_STATE_OPEN:
        return capitals ? "Open" : "open";

    default:
        T_E("Invalid gate_state (%d)", gate_state);
        return "Unknown!";
    }
}

/******************************************************************************/
// psfp_util_filter_oper_warnings_to_str()
// Buf must be ~400 bytes long if all bits are set.
/******************************************************************************/
char *psfp_util_filter_oper_warnings_to_str(char *buf, size_t size, vtss_appl_psfp_filter_oper_warnings_t oper_warnings)
{
    int  s, res;
    bool first;

    if (!buf) {
        return buf;
    }

#define P(_str_)                                        \
    if (size - s > 0) {                                 \
        res = snprintf(buf + s, size - s, "%s", _str_); \
        if (res > 0) {                                  \
            s += res;                                   \
        }                                               \
    }

#define F(X, _name_)                                              \
    if (oper_warnings & VTSS_APPL_PSFP_FILTER_OPER_WARNING_##X) { \
        oper_warnings &= ~VTSS_APPL_PSFP_FILTER_OPER_WARNING_##X; \
        if (first) {                                              \
            first = false;                                        \
            P(_name_);                                            \
        } else {                                                  \
            P(", " _name_);                                       \
        }                                                         \
    }

    buf[0] = 0;
    s      = 0;
    first  = true;

    // Example of a field name (just so that we can search for this function):
    // VTSS_APPL_PSFP_FILTER_OPER_WARNING_STREAM_COLLECTION_NOT_FOUND

    F(NO_STREAM_OR_STREAM_COLLECTION,             "Neither a stream or a stream collection is specified");
    F(STREAM_NOT_FOUND,                           "The specified stream ID does not exist");
    F(STREAM_COLLECTION_NOT_FOUND,                "The specified stream collection ID does not exist");
    F(STREAM_ATTACH_FAIL,                         "Unable to attach to the specified stream, possibly because it is part of a stream collection");
    F(STREAM_COLLECTION_ATTACH_FAIL,              "Unable to attach to the specified stream collection");
    F(STREAM_HAS_OPERATIONAL_WARNINGS,            "The specified stream has configurational warnings");
    F(STREAM_COLLECTION_HAS_OPERATIONAL_WARNINGS, "The specified stream collection has configurational warnings");
    F(FLOW_METER_NOT_FOUND,                       "The specified flow meter ID does not exist");
    F(GATE_NOT_FOUND,                             "The specified stream gate ID does not exist");
    F(GATE_NOT_ENABLED,                           "The specified stream gate is not enabled");

    buf[MIN(size - 1, s)] = 0;
#undef F
#undef P

    if (oper_warnings != 0) {
        T_E("Not all operational warnings are handled. Missing = 0x%x", oper_warnings);
    }

    return buf;
}

/******************************************************************************/
// psfp_error_txt()
/******************************************************************************/
const char *psfp_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_PSFP_RC_INVALID_PARAMETER:
        return "Invalid parameter";

    case VTSS_APPL_PSFP_RC_NOT_SUPPORTED:
        return "PSFP is not supported on this platform";

    case VTSS_APPL_PSFP_RC_OUT_OF_MEMORY:
        return "Out of memory";

    case VTSS_APPL_PSFP_RC_INTERNAL_ERROR:
        return "Internal Error: A code-update is required. See console or crashlog for details";

    case VTSS_APPL_PSFP_RC_INVALID_TIME_UNIT:
        return "Invalid time unit";

    case VTSS_APPL_PSFP_RC_STREAM_ID_OUT_OF_RANGE:
        return "Stream ID is out of range";

    case VTSS_APPL_PSFP_RC_STREAM_COLLECTION_ID_OUT_OF_RANGE:
        return "Stream collection ID is out of range";

    case VTSS_APPL_PSFP_RC_FLOW_METER_ID_OUT_OF_RANGE:
        return "Flow meter ID is out of range";

    case VTSS_APPL_PSFP_RC_FLOW_METER_NO_SUCH_INSTANCE:
        return "No such flow meter instance";

    case VTSS_APPL_PSFP_RC_FLOW_METER_OUT_OF_HW_RESOURCES:
        return "Out of hardware resources";

    case VTSS_APPL_PSFP_RC_GATE_ID_OUT_OF_RANGE:
        return "Stream gate ID is out of range";

    case VTSS_APPL_PSFP_RC_GATE_NO_SUCH_INSTANCE:
        return "No such stream gate instance";

    case VTSS_APPL_PSFP_RC_GATE_INVALID_GATE_STATE:
        return "Invalid administrative gate state";

    case VTSS_APPL_PSFP_RC_GATE_INVALID_IPV_VALUE:
        return "Invalid priority value. Valid range is 0 to 7";

    case VTSS_APPL_PSFP_RC_GATE_INVALID_CYCLE_TIME:
        return "Invalid cycle time. Valid values are 0 to 1,000,000,000 nanoseconds";

    case VTSS_APPL_PSFP_RC_GATE_INVALID_TIME_EXTENSION:
        return "Invalid extension time. Valid values are 0 to 1,000,000,000 nanoseconds";

    case VTSS_APPL_PSFP_RC_GATE_INVALID_GCL_LEN:
        return "Invalid gate control list length";

    case VTSS_APPL_PSFP_RC_GATE_INVALID_GCE_GATE_STATE:
        return "Invalid gate control entry gate state";

    case VTSS_APPL_PSFP_RC_GATE_INVALID_GCE_IPV_VALUE:
        return "Invalid gate control entry IPV value";

    case VTSS_APPL_PSFP_RC_GATE_INVALID_GCE_TIME_INTERVAL:
        return "Invalid gate control entry time interval. Valid values are 1 to 999,999,999 nanoseconds";

    case VTSS_APPL_PSFP_RC_GATE_CYCLETIME_EXCEEDED:
        return "The sum of the enabled gate control entries' time-intervals exceeds the gate's cycle time";

    case VTSS_APPL_PSFP_RC_FILTER_ID_OUT_OF_RANGE:
        return "Stream filter ID is out of range";

    case VTSS_APPL_PSFP_RC_FILTER_NO_SUCH_INSTANCE:
        return "No such stream filter instance";

    case VTSS_APPL_PSFP_RC_FILTER_STREAM_ID_AND_COLLECTION_ID_CANNOT_BE_USED_SIMULTANEOUSLY:
        return "It is not possible to specfy a stream ID and a stream collection ID simultaneously";

    case VTSS_APPL_PSFP_RC_FILTER_ANOTHER_FILTER_USING_SAME_STREAM_ID:
        return "Another stream filter is using the same stream";

    case VTSS_APPL_PSFP_RC_FILTER_ANOTHER_FILTER_USING_SAME_STREAM_COLLECTION_ID:
        return "Another stream filter is using the same stream collection";

    default:
        T_E("Unknown error code (%u = 0x%08x)", rc, rc);
        return "PSFP: Unknown error code";
    }
}

/******************************************************************************/
// psfp_ptp_ready()
// Called by tsn.cxx whenever PTP is ready and stream gate configurations can
// be applied.
/******************************************************************************/
void psfp_ptp_ready(void)
{
    psfp_gate_itr_t gate_itr;

    if (PSFP_supported() != VTSS_RC_OK) {
        return;
    }

    PSFP_LOCK_SCOPE();

    PSFP_ptp_ready = true;
    T_IG(PSFP_TRACE_GRP_GATE, "PTP is ready. Updating gates");
    for (gate_itr = PSFP_gate_map.begin(); gate_itr != PSFP_gate_map.end(); ++gate_itr) {
        (void)PSFP_gate_update(gate_itr->second);
    }
}

extern "C" int psfp_icli_cmd_register(void);

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void psfp_mib_init(void);
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void psfp_json_init(void);
#endif

/******************************************************************************/
// psfp_init()
/******************************************************************************/
mesa_rc psfp_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_EARLY_INIT) {
        // Not used, but we don't want to get the T_I() below because we don't
        // know yet whether PSFP is supported.
        return VTSS_RC_OK;
    }

    if (data->cmd == INIT_CMD_INIT) {
        // Gotta get the capabilities before initializing anything, because
        // if PSFP is not supported on this platform, we don't do anything.
        PSFP_capabilities_set();
    }

    if (PSFP_supported() != VTSS_RC_OK) {
        // Stop here.
        T_I("PSFP is not supported. Exiting");
        return VTSS_RC_OK;
    }

    switch (data->cmd) {
    case INIT_CMD_INIT:
        PSFP_default_conf_set();

        critd_init(&PSFP_crit, "psfp", VTSS_MODULE_ID_PSFP, CRITD_TYPE_MUTEX);

        psfp_icli_cmd_register();

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        psfp_mib_init();
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC)
        psfp_json_init();
#endif /* defined(VTSS_SW_OPTION_JSON_RPC) */

        break;

    case INIT_CMD_START:
#ifdef VTSS_SW_OPTION_ICFG
        mesa_rc psfp_icfg_init(void);
        VTSS_RC(psfp_icfg_init());
#endif
        break;

    case INIT_CMD_CONF_DEF:
        PSFP_default();
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        PSFP_default();

        {
            PSFP_LOCK_SCOPE();
            psfp_timer_init();
        }

        PSFP_stream_change_notifications.init();
        PSFP_stream_collection_change_notifications.init();
        psfp_json_init();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

