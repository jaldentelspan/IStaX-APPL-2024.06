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

#include <vtss/appl/frer.hxx>
#include <vtss/appl/stream.h>
#include <vtss/appl/vlan.h>
#include "frer_api.h"
#include "frer_base.hxx"
#include "frer_expose.hxx"  /* For frer_notification_status_t */
#include "frer_trace.h"
#include "frer_lock.hxx"
#include "l2proto_api.h"    /* For l2port2port()               */
#include "mgmt_api.h"       /* For mgmt_iport_list2txt()       */
#include "port_api.h"       /* For port_change_register()      */
#include "stream_api.h"     /* For stream_notif_table (expose) */
#include "vlan_api.h"       /* For vlan_mgmt_XXX()             */
#include "vtss_appl_formatting_tags.hxx"
#include <vtss/basics/expose/table-status.hxx>
#include <vtss/basics/notifications/event.hxx>
#include <vtss/basics/notifications/event-handler.hxx>
#include "subject.hxx"      /* For subject_main_thread         */

//*****************************************************************************/
// Trace definitions
/******************************************************************************/
static vtss_trace_reg_t FRER_trace_reg = {
    VTSS_TRACE_MODULE_ID, "frer", "Frame Replication and Elimination for Reliability"
};

static vtss_trace_grp_t FRER_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [FRER_TRACE_GRP_BASE] = {
        "base",
        "Base/MESA calls",
        VTSS_TRACE_LVL_ERROR
    },
    [FRER_TRACE_GRP_TIMER] = {
        "timers",
        "Timers",
        VTSS_TRACE_LVL_ERROR
    },
    [FRER_TRACE_GRP_CALLBACK] = {
        "callback",
        "Callbacks",
        VTSS_TRACE_LVL_ERROR
    },
    [FRER_TRACE_GRP_STREAM] = {
        "stream",
        "Change in stream config",
        VTSS_TRACE_LVL_ERROR
    },
    [FRER_TRACE_GRP_ICLI] = {
        "icli",
        "ICLI",
        VTSS_TRACE_LVL_ERROR
    },
    [FRER_TRACE_GRP_NOTIF] = {
        "notif",
        "Notifications",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&FRER_trace_reg, FRER_trace_grps);

/******************************************************************************/
// Global variables
/******************************************************************************/
critd_t                              FRER_crit;
static uint32_t                      FRER_cap_port_cnt;
static vtss_appl_frer_capabilities_t FRER_cap;
frer_map_t                           FRER_map;
static vtss_appl_frer_conf_t         FRER_default_conf;
static bool                          FRER_started;
static CapArray<frer_port_state_t, MESA_CAP_PORT_CNT> FRER_port_state;

// frer_notification_status holds the per-instance state that one can get
// notifications on, that being SNMP traps or JSON notifications. Each row in
// this table is a struct of type vtss_appl_frer_notification_status_t.
frer_notification_status_t frer_notification_status("frer_notification_status", VTSS_MODULE_ID_FRER);

/******************************************************************************/
// vtss_appl_frer_latent_error_detection_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_frer_latent_error_detection_t &l)
{
    o << "{enable = "           << l.enable
      << ", difference = "      << l.difference
      << ", period_ms = "       << l.period_ms
      << ", paths = "           << l.paths
      << ", reset_period_ms = " << l.reset_period_ms
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_frer_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_frer_conf_t &conf)
{
    char stream_buf[100], egress_port_buf[500], *p;
    int  i;

    p = stream_buf;
    for (i = 0; i < ARRSZ(conf.stream_ids); i++) {
        p += sprintf(p, "%s%u", i == 0 ? "" : ",", conf.stream_ids[i]);
    }

    o << "{mode = "                         << frer_util_mode_to_str(conf.mode)
      << ", frer_vlan = "                   << conf.frer_vlan
      << ", stream_ids = "                  << stream_buf
      << ", stream_collection_id = "        << conf.stream_collection_id
      << ", outer_tag_pop = "               << conf.outer_tag_pop
      << ", egress_port_list = "            << mgmt_iport_list2txt(conf.egress_ports, egress_port_buf)
      << ", rcvy_algorithm = "              << frer_util_rcvy_alg_to_str(conf.rcvy_algorithm)
      << ", rcvy_history_len = "            << conf.rcvy_history_len
      << ", rcvy_reset_timeout_ms = "       << conf.rcvy_reset_timeout_ms
      << ", rcvy_take_no_sequence = "       << conf.rcvy_take_no_sequence
      << ", rcvy_individual = "             << conf.rcvy_individual
      << ", rcvy_terminate = "              << conf.rcvy_terminate
      << ", rcvy_latent_error_detection = " << conf.rcvy_latent_error_detection // Using vtss_appl_frer_latent_error_detection_t::operator<<()
      << ", admin_active = "                << conf.admin_active
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_frer_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_frer_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_frer_control_t::operator<<()
// Used for tracing.
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_frer_control_t &c)
{
    o << "{reset = "               << c.reset
      << ", latent_error_clear = " << c.latent_error_clear
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_frer_control_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_frer_control_t *c)
{
    o << *c;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_frer_statistics_t::operator<<()
// Used for tracing
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_frer_statistics_t &s)
{
    o << "{rcvy_out_of_order_packets = " << s.rcvy_out_of_order_packets
      << ", rcvy_rogue_packets = "       << s.rcvy_rogue_packets
      << ", rcvy_passed_packets = "      << s.rcvy_passed_packets
      << ", rcvy_discarded_packets = "   << s.rcvy_discarded_packets
      << ", rcvy_lost_packets = "        << s.rcvy_lost_packets
      << ", rcvy_tagless_packets = "     << s.rcvy_tagless_packets
      << ", rcvy_resets = "              << s.rcvy_resets
      << ", rcvy_latent_error_resets = " << s.rcvy_latent_error_resets
      << ", gen_resets = "               << s.gen_resets
      << ", gen_matches = "              << s.gen_matches
      << "}";

    return o;
}

/******************************************************************************/
// vtss_appl_frer_statistics_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_frer_statistics_t *s)
{
    o << *s;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// FRER_capabilities_set()
/******************************************************************************/
static void FRER_capabilities_set(void)
{
    vtss_appl_stream_capabilities_t            stream_cap;
    vtss_appl_stream_collection_capabilities_t stream_collection_cap;
    uint32_t                                   chip_family = fast_cap(MESA_CAP_MISC_CHIP_FAMILY);
    mesa_rc                                    rc;

    if (!fast_cap(MESA_CAP_L2_FRER)) {
        // FRER is not supported on this platform. Leave all fields at 0.
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
    FRER_cap.stream_id_max = stream_cap.inst_cnt_max;

    // Make a copy of the maximum stream collection ID that can be created.
    FRER_cap.stream_collection_id_max = stream_collection_cap.inst_cnt_max;

    // Unfortunately, a capability for the maximum number of egress ports
    // doesn't exist, but on LAN966X, it's 4 and on other chips (thus far) it's
    // 8.
    FRER_cap.egress_port_cnt_max = chip_family == MESA_CHIP_FAMILY_LAN966X ? 4 : 8;

    if (FRER_cap.egress_port_cnt_max > VTSS_APPL_FRER_EGRESS_PORT_CNT_MAX) {
        T_E("This chip supports %u egress ports, but VTSS_APPL_FRER_EGRESS_PORT_CNT_MAX is ony %u. Please align them", FRER_cap.egress_port_cnt_max, VTSS_APPL_FRER_EGRESS_PORT_CNT_MAX);
        FRER_cap.egress_port_cnt_max = VTSS_APPL_FRER_EGRESS_PORT_CNT_MAX;
    }

    FRER_cap.rcvy_mstream_cnt_max = fast_cap(MESA_CAP_L2_FRER_MSTREAM_CNT);
    FRER_cap.rcvy_cstream_cnt_max = fast_cap(MESA_CAP_L2_FRER_CSTREAM_CNT);

    // The maximum number of instances is hard to find, because we don't need
    // mstreams and cstreams for generation functionality, but we need it for
    // recovery functionality.
    // We need at least two mstreams per cstream, so the maximum is:
    FRER_cap.inst_cnt_max = MIN(FRER_cap.rcvy_mstream_cnt_max / 2, FRER_cap.rcvy_cstream_cnt_max);

    // However, it does not make sense to make more instances than we have
    // streams, because each instance requires at least one stream, so we also
    // limit the number of instances by that:
    FRER_cap.inst_cnt_max = MIN(FRER_cap.inst_cnt_max, FRER_cap.stream_id_max);

    // Finally, it could make sense to limit the number of instances to:
    //   rcvy_mstream_cnt_max / FRER_cap.egress_port_cnt_max^2
    // FRER_cap.egress_port_cnt_max^2 is the theoretical maximum number
    // of mstreams we need for a single FRER instance in individual recovery
    // mode. It goes like this:
    //   - We can map FRER_cap.egress_port_cnt_max ingress streams to one set of
    //     member streams.
    //   - For each set of member streams, we can do individual recovery on
    //     FRER_cap.egress_port_cnt_max egress ports.
    //
    // As an example, suppose we have 1024 mstreams in H/W and
    // FRER_cap.egress_port_cnt_max (also limited by H/W) is 8.
    // In this case, we could have at most 1024 / (8 * 8) = 16 FRER instances.
    // In reality, we can also have generators that don't require member or
    // compound streams, so let's not limit it by this.
    // If the user determines to make individual recovery on more than 8
    // member streams on 8 egress ports for more than 16 instances, he will be
    // told that this is not possible (H/W resources).
    // Furthermore, the way MESA allocates mstreams is in a sequence, so that it
    // attempts to find number-of-egress-ports free sequential mstreams for
    // every call to mesa_frer_mstream_alloc(), and if it can't it fails. So the
    // list of allocated mstreams may be fragmented, so there's no guarantee
    // that if FRER instances are allocated and freed many times that it still
    // will be able to find number-of-egress-ports in sequence.

    T_I("egress_port_cnt_max = %u, mstream count = %u, cstream count = %u, inst_cnt_max = %u", FRER_cap.egress_port_cnt_max, FRER_cap.rcvy_mstream_cnt_max, FRER_cap.rcvy_cstream_cnt_max, FRER_cap.inst_cnt_max);

    // This is a bit harder, because MESA doesn't expose it. I know for a fact
    // that the prescaler is hardcoded to 1000 ticks per second and the
    // per-stream reset counter can count up to 2^12 - 1 = 4095, which means
    // that we can set it from 1 / 4095 = 0 ms to 4095 ms.
    FRER_cap.rcvy_reset_timeout_ms_min =    1;
    FRER_cap.rcvy_reset_timeout_ms_max = 4095;

    // Hard-coded in chip:
    FRER_cap.rcvy_history_len_min =  2;
    FRER_cap.rcvy_history_len_max = 32;

    // Latent error dection
    FRER_cap.rcvy_latent_error_difference_min  =          0;
    FRER_cap.rcvy_latent_error_difference_max  =   10000000;
    FRER_cap.rcvy_latent_error_period_ms_min   =       1000; // 1 second
    FRER_cap.rcvy_latent_error_period_ms_max   =   86400000; // 1 hour  (arbitrarily chosen)

    // As for the number of error paths, one could argue that we already know
    // this number from the number of ingress ports included in the ingress
    // streams. However, one could also imagine that the same ingress port could
    // receive non-recovered member streams from the same link partner. The
    // (individual) recovery function would still work in that case, but the
    // latent error detection function would detect a higher number of discarded
    // packets than was expected, if we relied only on the number of ingress
    // ports, so we make it configurable.
    // The minimum number (which is also the default) is 2.
    // The maximum number is debatable. It could be the maximum number of
    // supported member streams (8), or it could be the number of ports on the
    // device, or it could be any arbitrary higher number.
    // I suggest that we limit it to the number of H/W-supported member streams,
    // given by FRER_cap.egress_port_cnt_max.
    FRER_cap.rcvy_latent_error_paths_min     = 2;
    FRER_cap.rcvy_latent_error_paths_max     = FRER_cap.egress_port_cnt_max;

    FRER_cap.rcvy_latent_reset_period_ms_min =       1000; // 1 second
    FRER_cap.rcvy_latent_reset_period_ms_max =   86400000; // 1 hour (arbitrarily chosen)
}

//*****************************************************************************/
// FRER_default_conf_set()
/******************************************************************************/
static void FRER_default_conf_set(void)
{
    vtss_appl_frer_latent_error_detection_t &led_default_conf = FRER_default_conf.rcvy_latent_error_detection;
    int                                     i;

    vtss_clear(FRER_default_conf);

    FRER_default_conf.mode      = VTSS_APPL_FRER_MODE_GENERATION;
    FRER_default_conf.frer_vlan = VTSS_APPL_VLAN_ID_DEFAULT; // Whatever

    for (i = 0; i < ARRSZ(FRER_default_conf.stream_ids); i++) {
        FRER_default_conf.stream_ids[i] = VTSS_APPL_STREAM_ID_NONE;
    }

    FRER_default_conf.egress_ports.clear_all();
    FRER_default_conf.stream_collection_id        = VTSS_APPL_STREAM_COLLECTION_ID_NONE;
    FRER_default_conf.rcvy_algorithm              = MESA_FRER_RECOVERY_ALG_VECTOR;
    FRER_default_conf.rcvy_history_len            =     2;
    FRER_default_conf.rcvy_reset_timeout_ms       =  1000;
    FRER_default_conf.rcvy_take_no_sequence       = false;
    FRER_default_conf.rcvy_individual             = false;
    FRER_default_conf.rcvy_terminate              = false;
    led_default_conf.enable                       = false;
    led_default_conf.difference                   =   100;
    led_default_conf.period_ms                    =  2000;
    led_default_conf.paths                        =     2;
    led_default_conf.reset_period_ms              = 30000;
    FRER_default_conf.admin_active                = false;
}

/******************************************************************************/
// FRER_supported()
/******************************************************************************/
static mesa_rc FRER_supported(void)
{
    return FRER_cap.inst_cnt_max == 0 ? (mesa_rc)VTSS_APPL_FRER_RC_NOT_SUPPORTED : VTSS_RC_OK;
}

/******************************************************************************/
// FRER_ptr_check()
/******************************************************************************/
static mesa_rc FRER_ptr_check(const void *ptr)
{
    return ptr == nullptr ? (mesa_rc)VTSS_APPL_FRER_RC_INVALID_PARAMETER : VTSS_RC_OK;
}

/******************************************************************************/
// FRER_inst_check()
// Range [1-FRER_cap.inst_cnt_max]
/******************************************************************************/
static mesa_rc FRER_inst_check(uint32_t inst)
{
    if (inst < 1 || inst > FRER_cap.inst_cnt_max) {
        return VTSS_APPL_FRER_RC_INVALID_PARAMETER;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// FRER_stream_check()
/******************************************************************************/
static void FRER_stream_check(frer_state_t *frer_state)
{
    vtss_appl_stream_collection_status_t stream_collection_status;
    vtss_appl_stream_status_t            stream_status;
    vtss_appl_frer_oper_warnings_t       &w = frer_state->status.oper_warnings;
    frer_stream_state_t                  *stream_state;
    mesa_port_no_t                       port_no;
    int                                  i;
    bool                                 using_stream_collection = frer_state->using_stream_collection();

    // Assume that everything is fine.
    w &= ~VTSS_APPL_FRER_OPER_WARNING_STREAM_NOT_FOUND;
    w &= ~VTSS_APPL_FRER_OPER_WARNING_STREAM_ATTACH_FAIL;
    w &= ~VTSS_APPL_FRER_OPER_WARNING_STREAM_HAS_OPERATIONAL_WARNINGS;
    w &= ~VTSS_APPL_FRER_OPER_WARNING_INGRESS_EGRESS_OVERLAP;
    w &= ~VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_NOT_FOUND;
    w &= ~VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_ATTACH_FAIL;
    w &= ~VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_HAS_OPERATIONAL_WARNINGS;

    for (i = 0; i < ARRSZ(frer_state->stream_states); i++) {
        if (using_stream_collection) {
            // Only iterate once, because only stream_states[0] is valid,
            // because we attach to only one stream collection.
            if (i > 0) {
                break;
            }
        } else {
            if (frer_state->conf.stream_ids[i] == VTSS_APPL_STREAM_ID_NONE) {
                // No more stream IDs to check (invariant is that active stream
                // IDs come first in the array.
                break;
            }
        }

        stream_state = &frer_state->stream_states[i];

        if (!stream_state->exists) {
            w |= using_stream_collection ? VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_NOT_FOUND : VTSS_APPL_FRER_OPER_WARNING_STREAM_NOT_FOUND;

            // Don't report anything else when the stream (collection) doesn't
            // exist.
            continue;
        }

        if (stream_state->attach_failed) {
            // Something failed when we attempted to attach to the stream or
            // stream collection.
            w |= using_stream_collection ? VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_ATTACH_FAIL : VTSS_APPL_FRER_OPER_WARNING_STREAM_ATTACH_FAIL;
        }

        if (using_stream_collection) {
            // Propagate warnings from the stream collection into the FRER
            // instance.
            if (vtss_appl_stream_collection_status_get(frer_state->conf.stream_collection_id, &stream_collection_status) == VTSS_RC_OK &&
                stream_collection_status.oper_warnings != VTSS_APPL_STREAM_COLLECTION_OPER_WARNING_NONE) {
                w |= VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_HAS_OPERATIONAL_WARNINGS;
            }
        } else {
            // Propagate warnings from the stream into the filter
            if (vtss_appl_stream_status_get(frer_state->conf.stream_ids[i], &stream_status) == VTSS_RC_OK &&
                stream_status.oper_warnings != VTSS_APPL_STREAM_OPER_WARNING_NONE) {
                w |= VTSS_APPL_FRER_OPER_WARNING_STREAM_HAS_OPERATIONAL_WARNINGS;
            }
        }

        // Check whether there is an overlap between ingress and egress ports
        for (port_no = 0; port_no < FRER_cap_port_cnt; port_no++) {
            if (stream_state->ingress_ports.get(port_no)) {
                if (frer_state->conf.egress_ports.get(port_no)) {
                    w |= VTSS_APPL_FRER_OPER_WARNING_INGRESS_EGRESS_OVERLAP;
                    break;
                }
            }
        }
    }
}

/******************************************************************************/
// FRER_egress_port_check()
/******************************************************************************/
static void FRER_egress_port_check(frer_state_t *frer_state)
{
    mesa_port_no_t port_no;
    int            port_cnt;

    frer_state->status.oper_warnings &= ~VTSS_APPL_FRER_OPER_WARNING_EGRESS_PORT_CNT;

    if (frer_state->conf.mode == VTSS_APPL_FRER_MODE_RECOVERY) {
        return;
    }

    // Issue a warning if we are in generation mode and we only have one egress
    // port.
    port_cnt = 0;
    for (port_no = 0; port_no < FRER_cap_port_cnt; port_no++) {
        if (frer_state->conf.egress_ports.get(port_no)) {
            port_cnt++;
        }
    }

    if (port_cnt < 2) {
        frer_state->status.oper_warnings |= VTSS_APPL_FRER_OPER_WARNING_EGRESS_PORT_CNT;
    }
}

/******************************************************************************/
// FRER_link_check()
/******************************************************************************/
static void FRER_link_check(frer_state_t *frer_state)
{
    frer_stream_state_t *stream_state;
    mesa_port_no_t      port_no;
    int                 i;

    frer_state->status.oper_warnings &= ~VTSS_APPL_FRER_OPER_WARNING_INGRESS_NO_LINK;
    frer_state->status.oper_warnings &= ~VTSS_APPL_FRER_OPER_WARNING_EGRESS_NO_LINK;

    for (port_no = 0; port_no < FRER_cap_port_cnt; port_no++) {

        // First check ingress ports (coming from streams).
        for (i = 0; i < ARRSZ(frer_state->stream_states); i++) {
            stream_state = &frer_state->stream_states[i];

            if (!stream_state->exists) {
                continue;
            }

            if (stream_state->attach_failed) {
                continue;
            }

            if (!stream_state->ingress_ports.get(port_no)) {
                continue;
            }

            if (!FRER_port_state[port_no].link) {
                frer_state->status.oper_warnings |= VTSS_APPL_FRER_OPER_WARNING_INGRESS_NO_LINK;
            }
        }

        // Then check egress ports (coming from configuration).
        if (!frer_state->conf.egress_ports.get(port_no)) {
            continue;
        }

        if (!FRER_port_state[port_no].link) {
            frer_state->status.oper_warnings |= VTSS_APPL_FRER_OPER_WARNING_EGRESS_NO_LINK;
        }
    }
}

//*****************************************************************************/
// FRER_stream_change_notifications.
// Snoops on changes to the stream change notification structure to be able to
// react to changes in configurations of streams.
/******************************************************************************/
static struct frer_stream_change_notifications_t : public vtss::notifications::EventHandler {
    vtss::notifications::Event     e;
    stream_notif_table_t::Observer o;

    frer_stream_change_notifications_t() : EventHandler(&vtss::notifications::subject_main_thread), e(this)
    {
    }

    void init()
    {
        stream_notif_table.observer_new(&e);
    }

    void execute(vtss::notifications::Event *event)
    {
        frer_itr_t   itr;
        frer_state_t *frer_state;
        bool         oper_update;
        int          idx;

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
            T_DG(FRER_TRACE_GRP_STREAM, "%s: %s event to stream", i->first, vtss::notifications::EventType::txt[vtss::notifications::EventType::E(i->second)].valueName);
        }

        FRER_LOCK_SCOPE();

        if (!FRER_started) {
            // Defer these change notifications until we have told the base
            // about all FRER instances after boot.
            return;
        }

        // Loop through all FRER instances
        for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
            frer_state = &itr->second;

            if (frer_state->status.oper_state != VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
                // Don't care about FRER changes on this instance, since it's
                // not active.
                continue;
            }

            if (frer_state->using_stream_collection()) {
                // This FRER instance is using stream collections, so we don't
                // care about stream changes (if a stream is part of a stream
                // collection, we also get a stream collection notification).
                continue;
            }

            oper_update = false;

            for (idx = 0; idx < ARRSZ(frer_state->conf.stream_ids); idx++) {
                if (o.events.find(frer_state->conf.stream_ids[idx]) == o.events.end()) {
                    // No events on this stream ID
                    continue;
                }

                // Update the base.
                frer_base_stream_update(frer_state, idx);

                // And update the operational warnings.
                oper_update = true;
            }

            if (oper_update) {
                // Update operational warnings. Both stream- and link- may get
                // affected by a stream change.
                FRER_stream_check(frer_state);
                FRER_link_check(frer_state);
            }
        }
    }
} FRER_stream_change_notifications;

//*****************************************************************************/
// FRER_stream_collection_change_notifications.
// Snoops on changes to the stream collection change notification structure to
// be able to react to changes in configurations of streams collections.
/******************************************************************************/
static struct frer_stream_collection_change_notifications_t : public vtss::notifications::EventHandler {
    vtss::notifications::Event     e;
    stream_notif_table_t::Observer o;

    frer_stream_collection_change_notifications_t() : EventHandler(&vtss::notifications::subject_main_thread), e(this)
    {
    }

    void init()
    {
        stream_collection_notif_table.observer_new(&e);
    }

    void execute(vtss::notifications::Event *event)
    {
        frer_itr_t   itr;
        frer_state_t *frer_state;

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
            T_DG(FRER_TRACE_GRP_STREAM, "%s: %s event to stream collection", i->first, vtss::notifications::EventType::txt[vtss::notifications::EventType::E(i->second)].valueName);
        }

        FRER_LOCK_SCOPE();

        if (!FRER_started) {
            // Defer these change notifications until we have told the base
            // about all FRER instances after boot.
            return;
        }

        // Loop through all FRER instances
        for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
            frer_state = &itr->second;

            if (frer_state->status.oper_state != VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
                // Don't care about FRER changes on this instance, since it's
                // not active.
                continue;
            }

            if (!frer_state->using_stream_collection()) {
                // This FRER instance is not using stream collections, so we
                // don't care.
                continue;
            }

            if (o.events.find(frer_state->conf.stream_collection_id) == o.events.end()) {
                // No events on this stream collection
                continue;
            }

            // Update the base.
            frer_base_stream_update(frer_state, 0 /* stream collections only use index 0 */);

            // Update operational warnings. Both stream- and link- may get
            // affected by a stream change.
            FRER_stream_check(frer_state);
            FRER_link_check(frer_state);
        }
    }
} FRER_stream_collection_change_notifications;

/******************************************************************************/
// FRER_frer_vlan_membership_check()
/******************************************************************************/
static void FRER_frer_vlan_membership_check(frer_state_t *frer_state)
{
    mesa_port_no_t   port_no;
    mesa_port_list_t ports;
    mesa_vid_t       vid;
    mesa_rc          rc;

    vid = frer_state->conf.frer_vlan;

    T_N("%u: mesa_vlan_port_members_get(%u)", frer_state->inst, vid);
    if ((rc = mesa_vlan_port_members_get(nullptr, vid, &ports)) != VTSS_RC_OK) {
        T_E("%u: mesa_vlan_port_members_get(%u) failed: %s", frer_state->inst, vid, error_txt(rc));
        return;
    }

    // Check that all specified egress ports are members of the FRER VLAN.
    for (port_no = 0; port_no < FRER_cap_port_cnt; port_no++) {
        if (!frer_state->conf.egress_ports.get(port_no)) {
            continue;
        }

        if (!ports[port_no]) {
            frer_state->status.oper_warnings |= VTSS_APPL_FRER_OPER_WARNING_VLAN_MEMBERSHIP;
            return;
        }
    }

    // Mo VLAN membership warning.
    frer_state->status.oper_warnings &= ~VTSS_APPL_FRER_OPER_WARNING_VLAN_MEMBERSHIP;
}

/******************************************************************************/
// FRER_stp_state_to_str()
/******************************************************************************/
static const char *FRER_stp_state_to_str(mesa_stp_state_t stp_status)
{
    switch (stp_status) {
    case MESA_STP_STATE_DISCARDING:
        return "Discarding";

    case MESA_STP_STATE_LEARNING:
        return "Learning";

    case MESA_STP_STATE_FORWARDING:
        return "Forwarding";

    default:
        T_E("Unknown stp_status (%d)", stp_status);
        return "Unknown";
    }
}

/******************************************************************************/
// FRER_stp_check()
/******************************************************************************/
static void FRER_stp_check(frer_state_t *frer_state)
{
    frer_state->status.oper_warnings &= ~VTSS_APPL_FRER_OPER_WARNING_STP_BLOCKED;
    frer_state->status.oper_warnings &= ~VTSS_APPL_FRER_OPER_WARNING_MSTP_BLOCKED;

#ifdef VTSS_SW_OPTION_MSTP
    mesa_stp_state_t stp_status;
    mesa_port_no_t   port_no;
    mesa_msti_t      msti;
    mesa_rc          rc;

    // Convert from FRER VLAN to MSTI
    if ((rc = mesa_mstp_vlan_msti_get(nullptr, frer_state->conf.frer_vlan, &msti)) != VTSS_RC_OK) {
        T_E("%u: mesa_mstp_vlan_msti_get(%u) failed : %s", frer_state->inst, frer_state->conf.frer_vlan, error_txt(rc));
    }

    for (port_no = 0; port_no < FRER_cap_port_cnt; port_no++) {
        if (!frer_state->conf.egress_ports.get(port_no)) {
            continue;
        }

        // STP forwarding
        // Using the API directly, because using the MSTP module would require
        // two calls to figure out whether the port is discarding or disabled.
        if ((rc = mesa_stp_port_state_get(nullptr, port_no, &stp_status)) != VTSS_RC_OK) {
            T_E("%u: mesa_stp_port_state_get(%u) failed: %s", frer_state->inst, port_no, error_txt(rc));
        }

        T_I_PORT(port_no, "mesa_stp_port_state_get() => stp_status = %s", FRER_stp_state_to_str(stp_status));

        if (stp_status != MESA_STP_STATE_FORWARDING) {
            // Not forwarding because STP has marked the port as not forwarding.
            frer_state->status.oper_warnings |= VTSS_APPL_FRER_OPER_WARNING_STP_BLOCKED;
        }

        // Then ask for MSTI state on <port, VLAN>
        if ((rc = mesa_mstp_port_msti_state_get(nullptr, port_no, msti, &stp_status)) != VTSS_RC_OK) {
            T_E("%u: mesa_mstp_port_msti_state_get(%u, %u) failed: %s", frer_state->inst, port_no, msti, error_txt(rc));
        }

        T_I_PORT(port_no, "mesa_mstp_port_msti_state_get(msti = %u) => stp_status = %s", msti, FRER_stp_state_to_str(stp_status));

        if (stp_status != MESA_STP_STATE_FORWARDING) {
            frer_state->status.oper_warnings |= VTSS_APPL_FRER_OPER_WARNING_MSTP_BLOCKED;
        }
    }
#endif
}

/******************************************************************************/
// FRER_oper_warnings_update()
/******************************************************************************/
static void FRER_oper_warnings_update(frer_state_t *frer_state)
{
    T_I("%u: Enter", frer_state->inst);

    // Assume no warnings.
    frer_state->status.oper_warnings = VTSS_APPL_FRER_OPER_WARNING_NONE;

    // Is this FRER instance administratively enabled and up and running
    if (frer_state->status.oper_state != VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
        // Nope. Nothing more to do.
        return;
    }

    // Check that all streams are as they are supposed to be and whether there
    // is an overlap between ingress and egress ports.
    FRER_stream_check(frer_state);

    // Check that user has specified more than one egress port in generation
    // mode.
    FRER_egress_port_check(frer_state);

    // Check link on ingress and egress ports.
    FRER_link_check(frer_state);

    // Check that all specified egress ports are members of the FRER VLAN.
    FRER_frer_vlan_membership_check(frer_state);

    // Check for STP/MSTP on egress ports
    FRER_stp_check(frer_state);
}

/******************************************************************************/
// FRER_conf_update()
// new_conf is a nullptr if this is the first time we activate the FRER
// instance. Otherwise, we are invoked because of a configuration change.
/******************************************************************************/
static mesa_rc FRER_conf_update(frer_itr_t itr, const vtss_appl_frer_conf_t *new_conf = nullptr)
{
    frer_state_t          *frer_state = &itr->second;
    vtss_appl_frer_conf_t temp_conf;
    bool                  only_led_conf_changed = false, deactivate_old = false;
    mesa_rc               rc = VTSS_RC_OK;

    if (new_conf) {
        // If the instance is currently operationally active, we disable it if
        // the configuration change is of such a nature that we need to remove
        // all rules before applying new.

        // Currently, we support one case, where we leave all rules as they are
        // and just signal to the base that a new configuration is applied. This
        // is when *only* the Latent Error Detection configuration has changed,
        // because it doesn't require chip-changes, since it's all done in S/W.
        if (frer_state->status.oper_state == VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
            // Figure out wheter this is the case.
            // First copy the current configuration to a temporary.
            temp_conf = frer_state->conf;

            // Then overwrite the LED conf of the old configuration with the new
            // conf's LED conf.
            temp_conf.rcvy_latent_error_detection = new_conf->rcvy_latent_error_detection;

            // And compare. If new configuration and temporary configuration are
            // now identical, it's only the LED conf that has changed.
            only_led_conf_changed = memcmp(&temp_conf, new_conf, sizeof(temp_conf)) == 0;

            if (!only_led_conf_changed) {
                deactivate_old = true;
            }
        }
    }

    T_I("%u: Old state = %s, Deactivate old = %d, only-LED-conf-changed = %d, FRER_started = %d",
        frer_state->inst, frer_util_oper_state_to_str(frer_state->status.oper_state),
        deactivate_old, only_led_conf_changed, FRER_started);

    if (deactivate_old) {
        // Assume we end up being administratively disabled.
        frer_state->status.oper_state = VTSS_APPL_FRER_OPER_STATE_ADMIN_DISABLED;

        if ((rc = frer_base_deactivate(frer_state)) != VTSS_RC_OK) {
            // Base has already printed the errors to print, so we just print
            // a warning.
            T_I("%u: Unable to deactivate FRER instance: %s", frer_state->inst, error_txt(rc));
            rc = VTSS_RC_OK;
        }
    }

    if (new_conf) {
        // Save the new configuration.
        frer_state->conf = *new_conf;
    }

    if (only_led_conf_changed) {
        (void)frer_base_led_conf_change(frer_state);
    } else if (FRER_started && frer_state->conf.admin_active) {
        // Only configure the base if we are ready.
        if ((rc = frer_base_activate(frer_state)) != VTSS_RC_OK) {
            T_E("%u: Failed to activate base: %s", frer_state->inst, error_txt(rc));
            frer_state->status.oper_state = VTSS_APPL_FRER_OPER_STATE_INTERNAL_ERROR;
        } else {
            frer_state->status.oper_state = VTSS_APPL_FRER_OPER_STATE_ACTIVE;
        }
    }

    // Update operational warnings.
    FRER_oper_warnings_update(frer_state);

    return rc;
}

/******************************************************************************/
// FRER_default()
/******************************************************************************/
static void FRER_default(void)
{
    vtss_appl_frer_conf_t new_conf;
    frer_itr_t            itr;

    FRER_LOCK_SCOPE();

    // Start by setting all FRER instances inactive and call to update the
    // configuration. This will release the FRER resources in MESA, so that we
    // can erase all of it in one go afterwards.
    for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
        if (itr->second.conf.admin_active) {
            new_conf = itr->second.conf;
            new_conf.admin_active = false;
            (void)FRER_conf_update(itr, &new_conf);
        }
    }

    // Then erase all elements from the map.
    FRER_map.clear();
}

/******************************************************************************/
// FRER_port_link_state_change_callback()
/******************************************************************************/
static void FRER_port_link_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    frer_itr_t   itr;
    frer_state_t *frer_state;

    FRER_LOCK_SCOPE();

    if (FRER_port_state[port_no].link == status->link) {
        return;
    }

    FRER_port_state[port_no].link = status->link;

    // Loop across all FRER instances to see if we need to issue an operational
    // warning, because one of the ingress or egress ports no longer has link.
    for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
        frer_state = &itr->second;

        if (frer_state->status.oper_state == VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
            FRER_link_check(frer_state);
        }
    }
}

/******************************************************************************/
// FRER_vlan_membership_change_callback()
/******************************************************************************/
static void FRER_vlan_membership_change_callback(vtss_isid_t isid, mesa_vid_t vid, vlan_membership_change_t *changes)
{
    frer_itr_t   itr;
    frer_state_t *frer_state;

    FRER_LOCK_SCOPE();

    // Loop across all FRER instances to see if we need to issue an operational
    // warning, because one of the egress ports are not part of the FRER VLAN.
    for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
        frer_state = &itr->second;

        if (frer_state->status.oper_state == VTSS_APPL_FRER_OPER_STATE_ACTIVE && frer_state->conf.frer_vlan == vid) {
            FRER_frer_vlan_membership_check(frer_state);
        }
    }
}

#ifdef VTSS_SW_OPTION_MSTP
/******************************************************************************/
// FRER_port_stp_msti_state_change_callback()
/******************************************************************************/
static void FRER_port_stp_msti_state_change_callback(vtss_common_port_t l2port, u8 msti, vtss_common_stpstate_t new_state)
{
    vtss_isid_t    isid;
    mesa_port_no_t port_no;
    frer_itr_t     itr;

    // Convert l2port to isid/iport
    if (!l2port2port(l2port, &isid, &port_no)) {
        T_IG(FRER_TRACE_GRP_CALLBACK, "l2port2port(%u) failed - probably because it's an aggr not a port", l2port);
        return;
    }

    T_DG(FRER_TRACE_GRP_CALLBACK, "MSTP change on port_no %u. new_state = %d", port_no, new_state);

    FRER_LOCK_SCOPE();

    // Update port/VLAN's forwarding state for the sake of Port Status TLVs
    for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
        if (itr->second.status.oper_state == VTSS_APPL_FRER_OPER_STATE_ACTIVE && itr->second.conf.egress_ports.get(port_no)) {
            // Update the warnings for this FRER instance.
            FRER_stp_check(&itr->second);
        }
    }
}
#endif

/******************************************************************************/
// FRER_port_state_init()
/******************************************************************************/
static void FRER_port_state_init(void)
{
    frer_port_state_t *port_state;
    mesa_port_no_t    port_no;
    mesa_rc           rc;

    {
        FRER_LOCK_SCOPE();

        for (port_no = 0; port_no < FRER_cap_port_cnt; port_no++) {
            port_state            = &FRER_port_state[port_no];
            port_state->port_no   = port_no;
            port_state->link      = false;
        }
    }

    // Don't take our own mutex during callback registrations (see comment in
    // similar function in cfm.cxx)

    // Subscribe to link changes in the Port module
    if ((rc = port_change_register(VTSS_MODULE_ID_FRER, FRER_port_link_state_change_callback)) != VTSS_RC_OK) {
        T_E("port_change_register() failed: %s", error_txt(rc));
    }

    // Subscribe to VLAN port membership changes in the VLAN module
    vlan_membership_change_register(VTSS_MODULE_ID_FRER, FRER_vlan_membership_change_callback);

#ifdef VTSS_SW_OPTION_MSTP
    // Subscribe to MSTP state changes
    if ((rc = l2_stp_msti_state_change_register(FRER_port_stp_msti_state_change_callback)) != VTSS_RC_OK) {
        T_E("l2_stp_msti_state_change_register() failed: %s", error_txt(rc));
    }
#endif
}

//*****************************************************************************/
// FRER_stream_ids_sort()
//*****************************************************************************/
static int FRER_stream_ids_sort(const void *a_, const void *b_)
{
    vtss_appl_stream_id_t a = *(vtss_appl_stream_id_t *)a_, b = *(vtss_appl_stream_id_t *)b_;

    if (a == b) {
        return 0;
    }

    // Make sure that VTSS_APPL_STREAM_ID_NONE entries come last.
    if (a == VTSS_APPL_STREAM_ID_NONE) {
        return 1;
    }

    if (b == VTSS_APPL_STREAM_ID_NONE) {
        return -1;
    }

    return a < b ? -1 : 1;
}

/******************************************************************************/
// FRER_stream_ids_normalize()
/******************************************************************************/
static void FRER_stream_ids_normalize(vtss_appl_stream_id_t *arr, size_t cnt)
{
    int i, j;

    // First we need to remove duplicates from the array.
    for (i = 0; i < cnt - 1; i++) {
        if (arr[i] == VTSS_APPL_STREAM_ID_NONE) {
            continue;
        }

        for (j = i + 1; j < cnt; j++) {
            if (arr[j] == arr[i]) {
                // Duplicate. Remove it.
                arr[j] = VTSS_APPL_STREAM_ID_NONE;
            }
        }
    }

    // Then sort the array.
    qsort(arr, cnt, sizeof(arr[0]), FRER_stream_ids_sort);
}

//*****************************************************************************/
// FRER_ifindex_greater_than_or_sometimes_equal()
/******************************************************************************/
static bool FRER_ifindex_greater_than_or_sometimes_equal(frer_itr_t itr, vtss_ifindex_t &ifindex, bool or_equal, bool start_over)
{
    vtss_ifindex_elm_t ife;
    vtss_ifindex_t     old_ifindex = ifindex;
    bool               use_this_port;
    mesa_port_no_t     port_no_itr, port_no;
    bool               result;
    mesa_rc            rc;

    if (start_over) {
        port_no = VTSS_PORT_NO_START;
    } else {
        // Gotta decode the ifindex.
        if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= FRER_cap_port_cnt) {
            result = false;
            goto do_exit;
        }

        port_no = ife.ordinal;
    }

    for (port_no_itr = 0; port_no_itr < FRER_cap_port_cnt; port_no_itr++) {
        if (!itr->second.conf.egress_ports.get(port_no_itr)) {
            continue;
        }

        if (start_over) {
            use_this_port = true;
        } else if (or_equal && port_no_itr >= port_no) {
            // Accept if it's greater than or equal
            use_this_port = true;
        } else if (!or_equal && port_no_itr > port_no) {
            // Only accept if it's greater than
            use_this_port = true;
        } else {
            use_this_port = false;
        }

        if (use_this_port) {
            if ((rc = vtss_ifindex_from_port(VTSS_ISID_START, port_no_itr, &ifindex)) != VTSS_RC_OK) {
                T_E("%u: Unable to convert port_no %u to an ifindex: %s", itr->first, port_no, error_txt(rc));
                result = false;
                goto do_exit;
            }

            result = true;
            goto do_exit;
        }
    }

    result = false;

do_exit:
    T_I("inst = %u, mode = %s, ifindex = %s, or_equal = %d, start_over = %d => ifindex = %s. Returning %d", itr->first, frer_util_mode_to_str(itr->second.conf.mode), old_ifindex, or_equal, start_over, ifindex, result);
    return result;
}

//*****************************************************************************/
// FRER_stream_id_greater_than()
/******************************************************************************/
static bool FRER_stream_id_greater_than(frer_itr_t itr, vtss_appl_stream_id_t &stream_id, bool start_over)
{
    vtss_appl_stream_id_t old_stream_id = stream_id;
    int                   stream_idx;
    bool                  result;

    // If in non-individual recovery mode, we only have one set of statistics,
    // namely the compound counters in recovery mode and the generation counters
    // in generation mode, so we set stream ID to VTSS_APPL_STREAM_ID_NONE.
    if (!itr->second.individual_recovery()) {
        if (start_over) {
            stream_id = VTSS_APPL_STREAM_ID_NONE;
            result = true;
        } else {
            result = false;
        }

        goto do_exit;
    }

    // In individual recovery mode, we first iterate across the individual
    // counters (per stream IDs), and then we return
    // VTSS_APPL_STREAM_ID_NONE, which indicates the compound counters.
    // The reason for this order is that if someone uses the iterator to gather
    // all statistics before printing it, it needs to know when the last
    // counter-set for this FRER instance is reached, and it can know that by
    // looking for VTSS_APPL_STREAM_ID_NONE.
    // Notice: In individual recovery mode, we cannot use stream collections, so
    // this is not a problem.
    if (!start_over && stream_id == VTSS_APPL_STREAM_ID_NONE) {
        // The end is reached for this <inst, egress_port>.
        result = false;
        goto do_exit;
    }

    // Iterate one beyond the end of the stream_ids[] array, because we need to
    // catch the special case, where all entries are used, and we need to return
    // the VTSS_APPL_STREAM_ID_NONE entry.
    for (stream_idx = 0; stream_idx <= ARRSZ(itr->second.conf.stream_ids); stream_idx++) {
        if (stream_idx == ARRSZ(itr->second.conf.stream_ids) || itr->second.conf.stream_ids[stream_idx] == VTSS_APPL_STREAM_ID_NONE) {
            // No more stream IDs. Return the NONE stream ID as the very last.
            stream_id = VTSS_APPL_STREAM_ID_NONE;
            result = true;
            goto do_exit;
        }

        if (start_over || itr->second.conf.stream_ids[stream_idx] > stream_id) {
            stream_id = itr->second.conf.stream_ids[stream_idx];
            result = true;
            goto do_exit;
        }
    }

    result = false;

do_exit:
    T_I("inst = %u, mode = %s, stream_id = %u, start_over = %d => stream_id = %u. Returning %d", itr->first, frer_util_mode_to_str(itr->second.conf.mode), old_stream_id, start_over, stream_id, result);
    return result;
}

/******************************************************************************/
// vtss_appl_frer_capabilities_get()
/******************************************************************************/
mesa_rc vtss_appl_frer_capabilities_get(vtss_appl_frer_capabilities_t *cap)
{
    VTSS_RC(FRER_ptr_check(cap));
    *cap = FRER_cap;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_frer_supported()
/******************************************************************************/
mesa_bool_t vtss_appl_frer_supported(void)
{
    return FRER_supported() == VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_frer_conf_default_get()
/******************************************************************************/
mesa_rc vtss_appl_frer_conf_default_get(vtss_appl_frer_conf_t *conf)
{
    VTSS_RC(FRER_supported());
    VTSS_RC(FRER_ptr_check(conf));
    *conf = FRER_default_conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_appl_frer_conf_get()
/******************************************************************************/
mesa_rc vtss_appl_frer_conf_get(uint32_t inst, vtss_appl_frer_conf_t *conf)
{
    frer_itr_t itr;

    VTSS_RC(FRER_supported());
    VTSS_RC(FRER_inst_check(inst));
    VTSS_RC(FRER_ptr_check(conf));

    T_N("Enter, %u", inst);

    FRER_LOCK_SCOPE();

    if ((itr = FRER_map.find(inst)) == FRER_map.end()) {
        return VTSS_APPL_FRER_RC_NO_SUCH_INSTANCE;
    }

    *conf = itr->second.conf;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_frer_conf_set()
/******************************************************************************/
mesa_rc vtss_appl_frer_conf_set(uint32_t inst, const vtss_appl_frer_conf_t *conf)
{
    frer_itr_t            itr, itr2;
    int                   i, j, stream_cnt, port_cnt;
    bool                  instance_is_new;
    mesa_port_no_t        port_no;
    vtss_appl_frer_conf_t local_conf;
    frer_state_t          *frer_state2;
    vtss_appl_stream_id_t stream_id1, stream_id2;

    VTSS_RC(FRER_supported());
    VTSS_RC(FRER_inst_check(inst));
    VTSS_RC(FRER_ptr_check(conf));

    T_D("Enter, inst = %u, conf = %s", inst, *conf);

    // Create a local version of the user's conf, so that we can normalize e.g.
    // stream IDs and egress ports.
    local_conf = *conf;

    // Parameter checking

    // Mode
    if (conf->mode != VTSS_APPL_FRER_MODE_GENERATION && conf->mode != VTSS_APPL_FRER_MODE_RECOVERY) {
        return VTSS_APPL_FRER_RC_INVALID_MODE;
    }

    // Stream IDs.
    // Normalize list of stream IDs by first removing duplicates and then
    // sorting it, so that entries with VTSS_APPL_STREAM_ID_NONE come last.
    // We work on local_conf, which is a copy of the user's conf.
    FRER_stream_ids_normalize(local_conf.stream_ids, ARRSZ(local_conf.stream_ids));

    stream_cnt = 0;
    for (i = 0; i < ARRSZ(local_conf.stream_ids); i++) {
        if (local_conf.stream_ids[i] != VTSS_APPL_STREAM_ID_NONE && (local_conf.stream_ids[i] < 1 || local_conf.stream_ids[i] > FRER_cap.stream_id_max)) {
            return VTSS_APPL_FRER_RC_INVALID_STREAM_ID_LIST;
        }

        if (local_conf.stream_ids[i] != VTSS_APPL_STREAM_ID_NONE) {
            stream_cnt++;
        }
    }

    if (stream_cnt > 0 && conf->stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
        return VTSS_APPL_FRER_RC_STREAM_ID_AND_COLLECTION_ID_CANNOT_BE_USED_SIMULTANEOUSLY;
    }

    if (conf->stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE && (conf->stream_collection_id < 1 || conf->stream_collection_id > FRER_cap.stream_collection_id_max)) {
        return VTSS_APPL_FRER_RC_INVALID_STREAM_COLLECTION_ID;
    }

    // FRER VLAN
    if (conf->frer_vlan < VTSS_APPL_VLAN_ID_MIN || conf->frer_vlan > VTSS_APPL_VLAN_ID_MAX) {
        return VTSS_APPL_FRER_RC_INVALID_VLAN;
    }

    // Egress ports
    port_cnt = 0;
    local_conf.egress_ports.clear_all();
    for (port_no = 0; port_no < FRER_cap_port_cnt; port_no++) {
        // Don't bother about bits beyond the number of supported ports, but do
        // clear them in our normalized conf.
        if (conf->egress_ports.get(port_no)) {
            local_conf.egress_ports.set(port_no);
            port_cnt++;
        }
    }

    if (port_cnt > FRER_cap.egress_port_cnt_max) {
        return VTSS_APPL_FRER_RC_EGRESS_PORT_CNT_EXCEEDED;
    }

    // Algorithm
    if (conf->rcvy_algorithm != MESA_FRER_RECOVERY_ALG_VECTOR && conf->rcvy_algorithm != MESA_FRER_RECOVERY_ALG_MATCH) {
        return VTSS_APPL_FRER_RC_INVALID_ALGORITHM;
    }

    // History length. Check this whether or not we are in recovery mode and
    // whether or not we use the vector algorithm (this is actually a matter of
    // temper).
    if (conf->rcvy_history_len < FRER_cap.rcvy_history_len_min || conf->rcvy_history_len > FRER_cap.rcvy_history_len_max) {
        return VTSS_APPL_FRER_RC_INVALID_HISTORY_LEN;
    }

    // Reset timeout
    if (conf->rcvy_reset_timeout_ms < FRER_cap.rcvy_reset_timeout_ms_min || conf->rcvy_reset_timeout_ms > FRER_cap.rcvy_reset_timeout_ms_max) {
        return VTSS_APPL_FRER_RC_INVALID_RESET_TIMEOUT;
    }

    // Latent error difference
    if (conf->rcvy_latent_error_detection.difference < FRER_cap.rcvy_latent_error_difference_min || conf->rcvy_latent_error_detection.difference > FRER_cap.rcvy_latent_error_difference_max) {
        return VTSS_APPL_FRER_RC_INVALID_LATENT_ERROR_DIFF;
    }

    // Latent error period
    if (conf->rcvy_latent_error_detection.period_ms < FRER_cap.rcvy_latent_error_period_ms_min || conf->rcvy_latent_error_detection.period_ms > FRER_cap.rcvy_latent_error_period_ms_max) {
        return VTSS_APPL_FRER_RC_INVALID_LATENT_ERROR_PERIOD;
    }

    // Latent error paths
    if (conf->rcvy_latent_error_detection.paths < FRER_cap.rcvy_latent_error_paths_min || conf->rcvy_latent_error_detection.paths > FRER_cap.rcvy_latent_error_paths_max) {
        return VTSS_APPL_FRER_RC_INVALID_LATENT_ERROR_PATHS;
    }

    // Latent reset period
    if (conf->rcvy_latent_error_detection.reset_period_ms < FRER_cap.rcvy_latent_reset_period_ms_min || conf->rcvy_latent_error_detection.reset_period_ms > FRER_cap.rcvy_latent_reset_period_ms_max) {
        return VTSS_APPL_FRER_RC_INVALID_LATENT_RESET_PERIOD;
    }

    // Defer some of the checks until the user enables this instance. Otherwise
    // we couldn't create a default instance in the first place.
    if (conf->admin_active) {
        if (stream_cnt == 0 && conf->stream_collection_id == VTSS_APPL_STREAM_COLLECTION_ID_NONE) {
            return VTSS_APPL_FRER_RC_STREAM_CNT_ZERO_OR_NO_STREAM_COLLECTION;
        }

        if (conf->mode == VTSS_APPL_FRER_MODE_GENERATION && conf->stream_collection_id == VTSS_APPL_STREAM_COLLECTION_ID_NONE && stream_cnt != 1) {
            // In generation mode, we support only one stream ID. User must
            // specify a stream collection if he wants to match more streams.
            return VTSS_APPL_FRER_RC_STREAM_CNT_MUST_BE_ONE;
        }

        if (conf->stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE && conf->mode == VTSS_APPL_FRER_MODE_RECOVERY && conf->rcvy_individual) {
            // Cannot use individual recovery with stream collections.
            return VTSS_APPL_FRER_RC_INDIVIDUAL_RECOVERY_WITH_STREAM_COLLECTIONS_NOT_POSSIBLE;
        }

        if (stream_cnt > FRER_cap.egress_port_cnt_max) {
            return VTSS_APPL_FRER_RC_STREAM_CNT_EXCEEDED;
        }

        if (port_cnt == 0) {
            return VTSS_APPL_FRER_RC_EGRESS_PORT_CNT_ZERO;
        }
    }

    FRER_LOCK_SCOPE();

    if ((itr = FRER_map.find(inst)) != FRER_map.end()) {
        instance_is_new = false;
        if (memcmp(&local_conf, &itr->second, sizeof(local_conf)) == 0) {
            // No changes.
            return VTSS_RC_OK;
        }
    } else {
        instance_is_new = true;
    }

    // Check that we haven't created more instances than we can allow
    if (itr == FRER_map.end()) {
        if (FRER_map.size() >= FRER_cap.inst_cnt_max) {
            return VTSS_APPL_FRER_RC_LIMIT_REACHED;
        }
    }

    // Cross-FRER instance checks
    if (local_conf.admin_active) {
        for (itr2 = FRER_map.begin(); itr2 != FRER_map.end(); ++itr2) {
            if (itr2 == itr) {
                // Don't check against our selves.
                continue;
            }

            frer_state2 = &itr2->second;

            if (!frer_state2->conf.admin_active) {
                // Only check against active instances.
                continue;
            }

            // Two administratively enabled instances cannot use the same stream
            // IDs.
            for (i = 0; i < ARRSZ(local_conf.stream_ids); i++) {
                stream_id1 = local_conf.stream_ids[i];
                if (stream_id1 == VTSS_APPL_STREAM_ID_NONE) {
                    // Done
                    break;
                }

                for (j = 0; j < ARRSZ(frer_state2->conf.stream_ids); j++) {
                    stream_id2 = frer_state2->conf.stream_ids[j];
                    if (stream_id2 == VTSS_APPL_STREAM_ID_NONE) {
                        // Done
                        break;
                    }

                    if (stream_id1 == stream_id2) {
                        return VTSS_APPL_FRER_RC_ANOTHER_FRER_INSTANCE_USING_SAME_STREAM_ID;
                    }
                }
            }

            // Two administratively enabled instances cannot use the same stream
            // collection.
            if (local_conf.stream_collection_id != VTSS_APPL_STREAM_COLLECTION_ID_NONE && local_conf.stream_collection_id == frer_state2->conf.stream_collection_id) {
                return VTSS_APPL_FRER_RC_ANOTHER_FRER_INSTANCE_USING_SAME_STREAM_COLLECTION_ID;
            }
        }
    }

    // Create a new or update an existing entry
    if ((itr = FRER_map.get(inst)) == FRER_map.end()) {
        return VTSS_APPL_FRER_RC_OUT_OF_MEMORY;
    }

    if (instance_is_new) {
        // Cannot memset(), because it contains structs (e.g. mesa_vce_t), which
        // contain mesa_port_list_t, which has a constructor.
        vtss_clear(itr->second);

        // Initialize base

        // The state's #inst member is only used for tracing.
        itr->second.inst = itr->first;
        VTSS_RC(frer_base_state_init(&itr->second));
    }

    return FRER_conf_update(itr, &local_conf);
}

//*****************************************************************************/
// vtss_appl_frer_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_frer_conf_del(uint32_t inst)
{
    vtss_appl_frer_conf_t new_conf;
    frer_itr_t            itr;
    mesa_rc               rc;

    VTSS_RC(FRER_supported());
    VTSS_RC(FRER_inst_check(inst));

    T_D("Enter, %u", inst);

    FRER_LOCK_SCOPE();

    if ((itr = FRER_map.find(inst)) == FRER_map.end()) {
        return VTSS_APPL_FRER_RC_NO_SUCH_INSTANCE;
    }

    if (itr->second.conf.admin_active) {
        new_conf = itr->second.conf;
        new_conf.admin_active = false;

        // Back out of everything
        rc = FRER_conf_update(itr, &new_conf);
    } else {
        rc = VTSS_RC_OK;
    }

    // Delete FRER instance from our map
    FRER_map.erase(inst);

    return rc;
}

//*****************************************************************************/
// vtss_appl_frer_itr()
/******************************************************************************/
mesa_rc vtss_appl_frer_itr(const uint32_t *prev_inst, uint32_t *next_inst)
{
    frer_itr_t itr;

    VTSS_RC(FRER_supported());
    VTSS_RC(FRER_ptr_check(next_inst));

    FRER_LOCK_SCOPE();

    if (prev_inst) {
        // Here we have a valid prev_inst. Find the next from that one.
        itr = FRER_map.greater_than(*prev_inst);
    } else {
        // We don't have a valid prev_inst. Get the first.
        itr = FRER_map.begin();
    }

    if (itr != FRER_map.end()) {
        *next_inst = itr->first;
        return VTSS_RC_OK;
    }

    // No next
    return VTSS_RC_ERROR;
}

//*****************************************************************************/
// vtss_appl_frer_control_set()
/******************************************************************************/
mesa_rc vtss_appl_frer_control_set(uint32_t inst, const vtss_appl_frer_control_t *ctrl)
{
    frer_state_t *frer_state;
    frer_itr_t   itr;

    VTSS_RC(FRER_inst_check(inst));
    VTSS_RC(FRER_ptr_check(ctrl));

    FRER_LOCK_SCOPE();

    if ((itr = FRER_map.find(inst)) == FRER_map.end()) {
        return VTSS_APPL_FRER_RC_NO_SUCH_INSTANCE;
    }

    frer_state = &itr->second;

    T_I("%u: %s", inst, *ctrl);

    if (frer_state->status.oper_state != VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
        // Only reset active instances, but don't give an error.
        return VTSS_RC_OK;
    }

    return frer_base_control_reset(frer_state, ctrl);
}

//******************************************************************************
// vtss_appl_frer_notification_status_get()
//******************************************************************************
mesa_rc vtss_appl_frer_notification_status_get(uint32_t inst, vtss_appl_frer_notification_status_t *const notif_status)
{
    VTSS_RC(FRER_inst_check(inst));
    VTSS_RC(FRER_ptr_check(notif_status));

    T_D("%u: Enter", inst);

    // No need to lock scope, because the .get() function is guaranteed to be
    // atomic.
    return frer_notification_status.get(inst, notif_status);
}

//*****************************************************************************/
// vtss_appl_frer_status_get()
/******************************************************************************/
mesa_rc vtss_appl_frer_status_get(uint32_t inst, vtss_appl_frer_status_t *status)
{
    frer_itr_t itr;

    VTSS_RC(FRER_inst_check(inst));
    VTSS_RC(FRER_ptr_check(status));

    FRER_LOCK_SCOPE();

    if ((itr = FRER_map.find(inst)) == FRER_map.end()) {
        return VTSS_APPL_FRER_RC_NO_SUCH_INSTANCE;
    }

    *status = itr->second.status;
    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_frer_statistics_get()
/******************************************************************************/
mesa_rc vtss_appl_frer_statistics_get(uint32_t inst, vtss_ifindex_t ifindex, vtss_appl_stream_id_t stream_id, vtss_appl_frer_statistics_t *statistics)
{
    vtss_ifindex_elm_t ife;
    mesa_port_no_t     port_no;
    frer_state_t       *frer_state;
    frer_itr_t         itr;
    bool               found;
    int                idx;
    mesa_rc            rc;

    // Check arguments.
    VTSS_RC(FRER_inst_check(inst));
    VTSS_RC(FRER_ptr_check(statistics));

    T_I("inst = %u, ifindex = %s, stream_id = %d", inst, ifindex, stream_id);

    FRER_LOCK_SCOPE();

    if ((itr = FRER_map.find(inst)) == FRER_map.end()) {
        return VTSS_APPL_FRER_RC_NO_SUCH_INSTANCE;
    }

    frer_state = &itr->second;

    if (frer_state->conf.mode == VTSS_APPL_FRER_MODE_GENERATION) {
        if (ifindex != VTSS_IFINDEX_NONE) {
            // In generation mode, ifindex must be VTSS_IFINDEX_NONE.
            return VTSS_APPL_FRER_RC_IFINDEX_MUST_BE_NONE_IN_GENERATION_MODE;
        }

        if (stream_id != VTSS_APPL_STREAM_ID_NONE) {
            // In generation mode, stream_id must be
            // VTSS_APPL_STREAM_ID_NONE.
            return VTSS_APPL_FRER_RC_STREAM_ID_MUST_BE_NONE_IN_GENERATION_MODE;
        }

        port_no = VTSS_PORT_NO_NONE;
    } else {
        // ifindex checks:
        if (ifindex == VTSS_IFINDEX_NONE) {
            return VTSS_APPL_FRER_RC_IFINDEX_MUST_NOT_BE_NONE_IN_RECOVERY_MODE;
        }

        if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_PORT || ife.ordinal >= FRER_cap_port_cnt) {
            return VTSS_APPL_FRER_RC_INVALID_IFINDEX;
        }

        port_no = ife.ordinal;

        if (!frer_state->conf.egress_ports.get(port_no)) {
            // Selected port is not part of the configured egress ports.
            return VTSS_APPL_FRER_RC_NOT_PART_OF_EGRESS_PORTS;
        }

        // stream_id checks:
        if (frer_state->individual_recovery()) {
            // Stream collections cannot be used in individual recovery mode, so
            // no cow on the ice here.
            if (stream_id != VTSS_APPL_STREAM_ID_NONE && (stream_id < 1 || stream_id > FRER_cap.stream_id_max)) {
                return VTSS_APPL_FRER_RC_INVALID_STREAM_ID;
            }
        } else {
            if (stream_id != VTSS_APPL_STREAM_ID_NONE) {
                return VTSS_APPL_FRER_RC_STREAM_ID_CANNOT_BE_SPECIFIED_IN_THIS_MODE;
            }
        }

        if (stream_id != VTSS_APPL_STREAM_ID_NONE) {
            found = false;
            for (idx = 0; idx < ARRSZ(frer_state->conf.stream_ids); idx++) {
                if (frer_state->conf.stream_ids[idx] == stream_id) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                return VTSS_APPL_FRER_RC_STREAM_ID_NOT_FOUND;
            }
        }
    }

    // Start by clearing counters (also expected by base).
    vtss_clear(*statistics);

    if (frer_state->status.oper_state == VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
        rc = frer_base_statistics_get(frer_state, port_no, stream_id, statistics);
    } else {
        rc = VTSS_RC_OK;
    }

    T_I("inst = %u, ifindex = %s, stream_id = %u => statistics = %s", inst, ifindex, stream_id, *statistics);
    return rc;
}

//*****************************************************************************/
// vtss_appl_frer_statistics_clear()
//*****************************************************************************/
mesa_rc vtss_appl_frer_statistics_clear(uint32_t inst)
{
    frer_state_t *frer_state;
    frer_itr_t   itr;

    VTSS_RC(FRER_inst_check(inst));

    FRER_LOCK_SCOPE();

    if ((itr = FRER_map.find(inst)) == FRER_map.end()) {
        return VTSS_APPL_FRER_RC_NO_SUCH_INSTANCE;
    }

    frer_state = &itr->second;

    T_I("%u", inst);

    if (frer_state->status.oper_state == VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
        return frer_base_statistics_clear(frer_state);
    }

    return VTSS_RC_OK;
}

//*****************************************************************************/
// vtss_appl_frer_statistics_itr()
/******************************************************************************/
mesa_rc vtss_appl_frer_statistics_itr(const uint32_t *prev_inst, uint32_t *next_inst, const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex, const vtss_appl_stream_id_t *prev_stream_id, vtss_appl_stream_id_t *next_stream_id)
{
    frer_itr_t            itr;
    vtss_ifindex_t        ifindex;
    bool                  start_over_ifindex, start_over_stream_id;
    bool                  very_first_ifindex_lookup;

    VTSS_RC(FRER_supported());
    VTSS_RC(FRER_ptr_check(next_inst));
    VTSS_RC(FRER_ptr_check(next_ifindex));
    VTSS_RC(FRER_ptr_check(next_stream_id));

    *next_inst      = prev_inst      ? *prev_inst      : 0;
    *next_ifindex   = prev_ifindex   ? *prev_ifindex   : VTSS_IFINDEX_NONE        /* whatever */;
    *next_stream_id = prev_stream_id ? *prev_stream_id : VTSS_APPL_STREAM_ID_NONE /* whatever */;
    ifindex         = *next_ifindex;

    start_over_ifindex   = prev_ifindex   == nullptr;
    start_over_stream_id = prev_stream_id == nullptr;
    very_first_ifindex_lookup = true;

    T_I("inst = %u, ifindex = %s, stream_id = %d, start_over_ifindex = %d, start_over_stream_id = %u", *next_inst, *next_ifindex, *next_stream_id, start_over_ifindex, start_over_stream_id);

    FRER_LOCK_SCOPE();

    for (itr = FRER_map.greater_than_or_equal(*next_inst); itr != FRER_map.end(); ++itr) {
        if (itr->first != *next_inst) {
            // Start over on ifindex and stream_id
            start_over_ifindex   = true;
            start_over_stream_id = true;
        }

        if (itr->second.status.oper_state != VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
            // Don't return inactive FRER instances.
            continue;
        }

        *next_inst = itr->first;

        T_I("inst = %u, ifindex = %s, stream_id = %d, start_over_ifindex = %d, start_over_stream_id = %u", *next_inst, *next_ifindex, *next_stream_id, start_over_ifindex, start_over_stream_id);

        // If we are in generation mode, we stay with ifindex ==
        // VTSS_IFINDEX_NONE, and stream_id = VTSS_APPL_STREAM_ID_NONE.
        if (itr->second.conf.mode == VTSS_APPL_FRER_MODE_GENERATION) {
            if (start_over_ifindex) {
                *next_ifindex   = VTSS_IFINDEX_NONE;
                *next_stream_id = VTSS_APPL_STREAM_ID_NONE;
                T_I("inst = %u, ifindex = %s, stream_id = %d", *next_inst, *next_ifindex, *next_stream_id);
                return VTSS_RC_OK;
            }

            // In generation mode, we can only return one single iteration.
            continue;
        }

        // Recovery mode
        // Loop across all ifindices in this FRER instance. The very, very first
        // time, we use this function, it's OK to find an ifindex >= previous
        // ifindex, because we also might need to look for stream IDs using the
        // previous ifindex.
        while (FRER_ifindex_greater_than_or_sometimes_equal(itr, ifindex, very_first_ifindex_lookup, start_over_ifindex)) {
            if (start_over_ifindex || ifindex != *next_ifindex) {
                start_over_stream_id = true;
            }

            very_first_ifindex_lookup = false;
            start_over_ifindex        = false;
            *next_ifindex             = ifindex;

            // Search for a stream ID > *next_stream_id
            T_I("inst = %u, ifindex = %s, stream_id = %d, start_over_ifindex = %d, start_over_stream_id = %u", *next_inst, *next_ifindex, *next_stream_id, start_over_ifindex, start_over_stream_id);
            if (FRER_stream_id_greater_than(itr, *next_stream_id, start_over_stream_id)) {
                T_I("inst = %u, ifindex = %s, stream_id = %d", *next_inst, *next_ifindex, *next_stream_id);
                return VTSS_RC_OK;
            }
        }
    }

    // No next
    T_I("No next");
    return VTSS_RC_ERROR;
}

//*****************************************************************************/
// frer_util_mode_to_str()
/******************************************************************************/
const char *frer_util_mode_to_str(vtss_appl_frer_mode_t mode, bool capital)
{
    switch (mode) {
    case VTSS_APPL_FRER_MODE_GENERATION:
        return capital ? "Generation" : "generation";

    case VTSS_APPL_FRER_MODE_RECOVERY:
        return capital ? "Recovery" : "recovery";

    default:
        T_E("Invalid mode (%d)", mode);
        return capital ? "Unknown" : "unknown";
    }
}

//*****************************************************************************/
// frer_util_rcvy_alg_to_str()
/******************************************************************************/
const char *frer_util_rcvy_alg_to_str(mesa_frer_recovery_alg_t alg, bool capital)
{
    switch (alg) {
    case MESA_FRER_RECOVERY_ALG_VECTOR:
        return capital ? "Vector" : "vector";

    case MESA_FRER_RECOVERY_ALG_MATCH:
        return capital ? "Match" : "match";

    default:
        T_E("Invalid algorithm (%d)", alg);
        return "unknown";
    }
}

//*****************************************************************************/
// frer_util_yes_no_str()
/******************************************************************************/
const char *frer_util_yes_no_str(bool value)
{
    return value ? "Yes" : "No";
}

//*****************************************************************************/
// frer_util_ena_dis_str()
/******************************************************************************/
const char *frer_util_ena_dis_str(bool value)
{
    return value ? "Enabled" : "Disabled";
}

//*****************************************************************************/
// frer_util_oper_state_to_str()
/******************************************************************************/
const char *frer_util_oper_state_to_str(vtss_appl_frer_oper_state_t oper_state)
{
    switch (oper_state) {
    case VTSS_APPL_FRER_OPER_STATE_ADMIN_DISABLED:
        return "Admin disabled";

    case VTSS_APPL_FRER_OPER_STATE_ACTIVE:
        return "Active";

    case VTSS_APPL_FRER_OPER_STATE_INTERNAL_ERROR:
        return "Internal error";

    default:
        T_E("Invalid operational state (%d)", oper_state);
        return "Unknown";
    }
}

//*****************************************************************************/
// frer_util_oper_state_to_str()
/******************************************************************************/
const char *frer_util_oper_state_to_str(vtss_appl_frer_oper_state_t oper_state, vtss_appl_frer_oper_warnings_t oper_warnings)
{
    if (oper_warnings == VTSS_APPL_FRER_OPER_WARNING_NONE || oper_state != VTSS_APPL_FRER_OPER_STATE_ACTIVE) {
        return frer_util_oper_state_to_str(oper_state);
    }

    return "Active (warnings)";
}

//*****************************************************************************/
// frer_util_oper_warning_to_str()
// This function only prints one of the warnings that may be in #warnings, so
// the caller should do something to reduce it to just one flag being set prior
// to calling us.
// See frer_util_oper_warnings_to_str() for a function that can return a string
// with more than one flag set.
/******************************************************************************/
const char *frer_util_oper_warning_to_str(vtss_appl_frer_oper_warnings_t warning)
{
    switch (warning) {
    case VTSS_APPL_FRER_OPER_WARNING_NONE:
        return "None";

    case VTSS_APPL_FRER_OPER_WARNING_STREAM_NOT_FOUND:
        return "At least one of the ingress streams doesn't exist";

    case VTSS_APPL_FRER_OPER_WARNING_STREAM_ATTACH_FAIL:
        return "Unable to attach to at least one of the ingress streams, possibly because it is part of a stream collection";

    case VTSS_APPL_FRER_OPER_WARNING_STREAM_HAS_OPERATIONAL_WARNINGS:
        return "At least one of the ingress streams has configurational warnings";

    case VTSS_APPL_FRER_OPER_WARNING_INGRESS_EGRESS_OVERLAP:
        return "There is an overlap between ingress and egress ports";

    case VTSS_APPL_FRER_OPER_WARNING_EGRESS_PORT_CNT:
        return "In generation mode, at least two egress ports should be configured";

    case VTSS_APPL_FRER_OPER_WARNING_INGRESS_NO_LINK:
        return "At least one of the ingress ports doesn't have link";

    case VTSS_APPL_FRER_OPER_WARNING_EGRESS_NO_LINK:
        return "At least one of the egress ports doesn't have link";

    case VTSS_APPL_FRER_OPER_WARNING_VLAN_MEMBERSHIP:
        return "At least one of the egress ports is not member of the FRER VLAN";

    case VTSS_APPL_FRER_OPER_WARNING_STP_BLOCKED:
        return "At least one of the egress ports is blocked by STP";

    case VTSS_APPL_FRER_OPER_WARNING_MSTP_BLOCKED:
        return "At least one of the egress ports is blocked by MSTP";

    case VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_NOT_FOUND:
        return "The specified stream collection ID does not exist";

    case VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_ATTACH_FAIL:
        return "Unable to attach to the specified stream collection";

    case VTSS_APPL_FRER_OPER_WARNING_STREAM_COLLECTION_HAS_OPERATIONAL_WARNINGS:
        return "The specified stream collection has configurational warnings";


    default:
        T_E("Unknown warning (0x%x)", warning);
        return "Unknown";
    }
}

//*****************************************************************************/
// frer_util_oper_warnings_to_str()
// We don't know the size of the buffer. Caller must ensure it's big enough.
// See also frer_util_oper_warning_to_str()
/******************************************************************************/
char *frer_util_oper_warnings_to_str(vtss_appl_frer_oper_warnings_t warnings, char *buf)
{
    bool first = true;
    char *p;

    if (!buf) {
        return buf;
    }

    p = buf;
    *(p++) = '<';

#define F(X)                                           \
    if (warnings & VTSS_APPL_FRER_OPER_WARNING_##X) {  \
        p += sprintf(p, "%s%s", first ? "" : " ", #X); \
        first = false;                                 \
    }

    F(STREAM_NOT_FOUND);
    F(STREAM_ATTACH_FAIL);
    F(STREAM_HAS_OPERATIONAL_WARNINGS);
    F(INGRESS_EGRESS_OVERLAP);
    F(EGRESS_PORT_CNT);
    F(INGRESS_NO_LINK);
    F(EGRESS_NO_LINK);
    F(VLAN_MEMBERSHIP);
    F(STP_BLOCKED);
    F(MSTP_BLOCKED);
    F(STREAM_COLLECTION_NOT_FOUND);
    F(STREAM_COLLECTION_ATTACH_FAIL);
    F(STREAM_COLLECTION_HAS_OPERATIONAL_WARNINGS);
#undef F

    *(p++) = '>';
    *p     = '\0';

    return buf;
}

//*****************************************************************************/
// frer_error_txt()
//*****************************************************************************/
const char *frer_error_txt(mesa_rc error)
{
    switch (error) {
    case VTSS_APPL_FRER_RC_INVALID_PARAMETER:
        return "Invalid parameter";

    case VTSS_APPL_FRER_RC_NOT_SUPPORTED:
        return "802.1CB (FRER) is not supported on this platform";

    case VTSS_APPL_FRER_RC_INTERNAL_ERROR:
        return "Internal Error: A code-update is required. See console or crashlog for details";

    case VTSS_APPL_FRER_RC_NO_SUCH_INSTANCE:
        return "No such FRER instance";

    case VTSS_APPL_FRER_RC_HW_RESOURCES:
        return "Out of H/W resources";

    case VTSS_APPL_FRER_RC_OUT_OF_MEMORY:
        return "Out of memory";

    case VTSS_APPL_FRER_RC_INVALID_MODE:
        return "Invalid mode";

    case VTSS_APPL_FRER_RC_INVALID_STREAM_ID_LIST:
        return "At least one of the Stream IDs is invalid";

    case VTSS_APPL_FRER_RC_STREAM_ID_AND_COLLECTION_ID_CANNOT_BE_USED_SIMULTANEOUSLY:
        return "It is not possible to specify a stream ID list and a stream collection ID simultaneously";

    case VTSS_APPL_FRER_RC_INVALID_STREAM_COLLECTION_ID:
        return "Invalid stream collection ID";

    case VTSS_APPL_FRER_RC_INVALID_VLAN:
        return "Invalid FRER VLAN";

    case VTSS_APPL_FRER_RC_INVALID_STREAM_ID:
        return "Ingress Stream ID is invalid";

    case VTSS_APPL_FRER_RC_STREAM_CNT_ZERO_OR_NO_STREAM_COLLECTION:
        return "When administratively enabled, at least one Ingress Stream ID or Ingress Stream Collection must be specified";

    case VTSS_APPL_FRER_RC_STREAM_CNT_MUST_BE_ONE:
        return "In generation mode, the number of streams must be exactly one. Consider using a stream collection if more than one stream is needed";

    case VTSS_APPL_FRER_RC_STREAM_CNT_EXCEEDED:
        return "The maximum supported number of streams is exceeded";

    case VTSS_APPL_FRER_RC_INDIVIDUAL_RECOVERY_WITH_STREAM_COLLECTIONS_NOT_POSSIBLE:
        return "It is not possible to use stream collections when individual recovery is enabled. Use the stream ID list instead.";

    case VTSS_APPL_FRER_RC_EGRESS_PORT_CNT_EXCEEDED:
        return "The maximum supported number of egress ports is exceeded";

    case VTSS_APPL_FRER_RC_EGRESS_PORT_CNT_ZERO:
        return "When administratively enabled, at least one egress port must be specified";

    case VTSS_APPL_FRER_RC_INVALID_ALGORITHM:
        return "Invalid recovery algorithm";

    case VTSS_APPL_FRER_RC_INVALID_HISTORY_LEN:
        return "Invalid history length";

    case VTSS_APPL_FRER_RC_INVALID_RESET_TIMEOUT:
        return "Invalid recovery reset timeout";

    case VTSS_APPL_FRER_RC_INVALID_LATENT_ERROR_DIFF:
        return "Invalid recovery latent error difference";

    case VTSS_APPL_FRER_RC_INVALID_LATENT_ERROR_PERIOD:
        return "Invalid recovery latent error period";

    case VTSS_APPL_FRER_RC_INVALID_LATENT_ERROR_PATHS:
        return "Invalid value for number of latent error detection paths";

    case VTSS_APPL_FRER_RC_INVALID_LATENT_RESET_PERIOD:
        return "Invalid recovery latent reset period";

    case VTSS_APPL_FRER_RC_LIMIT_REACHED:
        return "The maximum number of FRER instances is reached";

    case VTSS_APPL_FRER_RC_ANOTHER_FRER_INSTANCE_USING_SAME_STREAM_ID:
        return "Another active FRER instance is using at least one of the same Stream IDs";

    case VTSS_APPL_FRER_RC_ANOTHER_FRER_INSTANCE_USING_SAME_STREAM_COLLECTION_ID:
        return "Another active FRER instance is using the same stream collection";

    case VTSS_APPL_FRER_RC_IFINDEX_MUST_BE_NONE_IN_GENERATION_MODE:
        return "Ifindex must be VTSS_IFINDEX_NONE (0) in generation mode";

    case VTSS_APPL_FRER_RC_STREAM_ID_MUST_BE_NONE_IN_GENERATION_MODE:
        return "Stream ID must be " vtss_xstr(VTSS_APPL_STREAM_ID_NONE) " in generation mode";

    case VTSS_APPL_FRER_RC_IFINDEX_MUST_NOT_BE_NONE_IN_RECOVERY_MODE:
        return "Ifindex must not be VTSS_IFINDEX_NONE (0) in recovery mode";

    case VTSS_APPL_FRER_RC_INVALID_IFINDEX:
        return "Invalid ifindex. ifindex must be of type port and be within proper port range";

    case VTSS_APPL_FRER_RC_NOT_PART_OF_EGRESS_PORTS:
        return "The specified interface is not part of the configured egress interfaces";

    case VTSS_APPL_FRER_RC_STREAM_ID_CANNOT_BE_SPECIFIED_IN_THIS_MODE:
        return "A stream ID can only be specified in individual recovery mode";

    case VTSS_APPL_FRER_RC_STREAM_ID_NOT_FOUND:
        return "Stream ID is not found";

    default:
        T_E("Unknown error code (%u)", error);
        return "FRER: Unknown error code";
    }
}

extern "C" int frer_icli_cmd_register(void);

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void frer_mib_init(void);
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void frer_json_init(void);
#endif

/******************************************************************************/
// frer_init()
/******************************************************************************/
mesa_rc frer_init(vtss_init_data_t *data)
{
    frer_itr_t itr;

    if (data->cmd == INIT_CMD_EARLY_INIT) {
        // Not used, but we don't want to get the T_I() below because we don't
        // know yet whether FRER is supported.
        return VTSS_RC_OK;
    }

    if (data->cmd == INIT_CMD_INIT) {
        // Gotta get the capabilities before initializing anything, because
        // if FRER is not supported on this platform, we don't do anything.
        FRER_capabilities_set();
    }

    if (FRER_supported() != VTSS_RC_OK) {
        // Stop here.
        T_I("FRER is not supported. Exiting");
        return VTSS_RC_OK;
    }

    switch (data->cmd) {
    case INIT_CMD_INIT:
        FRER_default_conf_set();

        critd_init(&FRER_crit, "frer", VTSS_MODULE_ID_FRER, CRITD_TYPE_MUTEX);

        frer_icli_cmd_register();

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        frer_mib_init();
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC)
        frer_json_init();
#endif /* defined(VTSS_SW_OPTION_JSON_RPC) */

        FRER_stream_change_notifications.init();
        FRER_stream_collection_change_notifications.init();
        break;

    case INIT_CMD_START:
#ifdef VTSS_SW_OPTION_ICFG
        mesa_rc frer_icfg_init(void);
        VTSS_RC(frer_icfg_init()); // ICFG initialization (show running-config)
#endif

        FRER_cap_port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_COUNT);
        break;

    case INIT_CMD_CONF_DEF:
        if (data->isid == VTSS_ISID_GLOBAL) {
            FRER_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE: {
        {
            FRER_LOCK_SCOPE();

            // Initialize the base library
            frer_base_init();
        }

        // Initialize the port-state array
        FRER_port_state_init();

        FRER_default();
        break;
    }

    case INIT_CMD_ICFG_LOADING_POST:
        // Now that ICLI has applied all configuration, start creating all the
        // FRER entries in MESA. Only do this the very first time an
        // INIT_CMD_ICFG_LOADING_POST is issued.
        if (!FRER_started) {
            FRER_LOCK_SCOPE();
            FRER_started = true;

            for (itr = FRER_map.begin(); itr != FRER_map.end(); ++itr) {
                FRER_conf_update(itr);
            }
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

