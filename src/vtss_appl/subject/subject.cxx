/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SUBJECT
#define VTSS_TRACE_SUBJECT_GRP_DEFAULT 0

#include <vtss/basics/array.hxx>
#include <vtss/basics/trace.hxx>
#include <vtss/basics/notifications.hxx>
#include <vtss/basics/notifications/process.hxx>
#include "vtss_trace_api.h"

static init_cmd_t local_init_state;

namespace vtss {
namespace notifications {

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "subject", "Subject"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_SUBJECT_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

static vtss::Array<SubjectRunner *, VTSS_THREAD_PRIO_NA> sr_prio;
static vtss::Array<SubjectRunner *, VTSS_THREAD_PRIO_NA> sr_prio_unlock_on_cb;

extern "C" void subject_thread_run(vtss_addrword_t _st_ptr) {
    using namespace vtss::notifications;
    SubjectRunner *st_ptr = (SubjectRunner *)_st_ptr;
    st_ptr->run();
}

extern "C" void subject_thread_start(vtss::notifications::SubjectRunner *sr) {
    using namespace vtss::notifications;
    vtss_handle_t handle;
    vtss_thread_create((vtss_thread_prio_t)sr->prio,
                       subject_thread_run,
                       (vtss_addrword_t)sr,
                       const_cast<char *>(sr->name),
                       nullptr,
                       0,
                       &handle,
                       0);

}

const char *sr_name(vtss_thread_prio_t prio, bool _unlock_on_callback) {
#define CASE(X, Y) case X: return Y;
    if (_unlock_on_callback) {
        switch (prio) {
            CASE(VTSS_THREAD_PRIO_BELOW_NORMAL,    "SR-PR-1");
            CASE(VTSS_THREAD_PRIO_DEFAULT,         "SR-PR0");
            CASE(VTSS_THREAD_PRIO_ABOVE_NORMAL,    "SR-PR+1");
            CASE(VTSS_THREAD_PRIO_HIGH,            "SR-PR+2");
            CASE(VTSS_THREAD_PRIO_HIGHER,          "SR-PR+3");
            CASE(VTSS_THREAD_PRIO_HIGHEST,         "SR-PR+4");
            CASE(VTSS_THREAD_PRIO_BELOW_NORMAL_RT, "SR-PR-RT-1");
            CASE(VTSS_THREAD_PRIO_DEFAULT_RT,      "SR-PR-RT0");
            CASE(VTSS_THREAD_PRIO_ABOVE_NORMAL_RT, "SR-PR-RT+1");
            CASE(VTSS_THREAD_PRIO_HIGH_RT,         "SR-PR-RT+2");
            CASE(VTSS_THREAD_PRIO_HIGHER_RT,       "SR-PR-RT+3");
            CASE(VTSS_THREAD_PRIO_HIGHEST_RT,      "SR-PR-RT+4");
            case VTSS_THREAD_PRIO_NA: break;
        }
    } else {
        switch (prio) {
            CASE(VTSS_THREAD_PRIO_BELOW_NORMAL,    "SRL-PR-1");
            CASE(VTSS_THREAD_PRIO_DEFAULT,         "SRL-PR0");
            CASE(VTSS_THREAD_PRIO_ABOVE_NORMAL,    "SRL-PR+1");
            CASE(VTSS_THREAD_PRIO_HIGH,            "SRL-PR+2");
            CASE(VTSS_THREAD_PRIO_HIGHER,          "SRL-PR+3");
            CASE(VTSS_THREAD_PRIO_HIGHEST,         "SRL-PR+4");
            CASE(VTSS_THREAD_PRIO_BELOW_NORMAL_RT, "SRL-PR-RT-1");
            CASE(VTSS_THREAD_PRIO_DEFAULT_RT,      "SRL-PR-RT0");
            CASE(VTSS_THREAD_PRIO_ABOVE_NORMAL_RT, "SRL-PR-RT+1");
            CASE(VTSS_THREAD_PRIO_HIGH_RT,         "SRL-PR-RT+2");
            CASE(VTSS_THREAD_PRIO_HIGHER_RT,       "SRL-PR-RT+3");
            CASE(VTSS_THREAD_PRIO_HIGHEST_RT,      "SRL-PR-RT+4");
            case VTSS_THREAD_PRIO_NA: break;
        }
    }

    return "SR-UNKNOWN";
#undef CASE
}

SubjectRunner subject_main_thread("SubjectMainThread",     VTSS_TRACE_MODULE_ID, true);
SubjectRunner subject_locked_thread("SubjectLockedThread", VTSS_TRACE_MODULE_ID, false);

static SubjectRunner *sr_create(vtss_thread_prio_t prio,
                                bool _unlock_on_callback) {
    SubjectRunner *sr = new SubjectRunner(sr_name(prio, _unlock_on_callback),
                                          VTSS_TRACE_MODULE_ID,
                                          _unlock_on_callback);
    sr->prio = prio;
    if (local_init_state > INIT_CMD_START)
        subject_thread_start(sr);
    return sr;
}

SubjectRunner &subject_runner_get(vtss_thread_prio_t prio,
                                  bool locked_in_callback) {
    bool _unlock_on_callback = !locked_in_callback;

    if (prio == VTSS_THREAD_PRIO_DEFAULT) {
        if (_unlock_on_callback) {
            return subject_main_thread;
        } else {
            return subject_locked_thread;
        }
    }

    if (_unlock_on_callback) {
        if (sr_prio_unlock_on_cb[prio]) {
            return *sr_prio_unlock_on_cb[prio];
        }
        sr_prio_unlock_on_cb[prio] = sr_create(prio, _unlock_on_callback);
        return *sr_prio_unlock_on_cb[prio];
    } else {
        if (sr_prio[prio]) {
            return *sr_prio[prio];
        }
        sr_prio[prio] = sr_create(prio, _unlock_on_callback);
        return *sr_prio[prio];
    }
}

}  // notifications
}  // vtss

extern "C" mesa_rc vtss_subject_init(vtss_init_data_t *data)
{
    using namespace vtss::notifications;

    VTSS_TRACE(DEBUG) << "enter, cmd: " << data->cmd << ", isid: " << data->isid
                      << ", flags: " << data->flags;

    local_init_state = data->cmd;

    if (data->cmd == INIT_CMD_START) {
        VTSS_TRACE(DEBUG) << "Start";
        subject_main_thread.prio = VTSS_THREAD_PRIO_DEFAULT;
        subject_locked_thread.prio = VTSS_THREAD_PRIO_DEFAULT;
        sr_prio[VTSS_THREAD_PRIO_DEFAULT] = &subject_locked_thread;
        sr_prio_unlock_on_cb[VTSS_THREAD_PRIO_DEFAULT] = &subject_main_thread;

        // Start all the threads which as been requested so far
        for (auto e : sr_prio) if (e) subject_thread_start(e);
        for (auto e : sr_prio_unlock_on_cb) if (e) subject_thread_start(e);

        // The Process class needs any subject runner to handle signals through.
        // This must be a subject runner that isn't deleted throughout the
        // lifetime of the application.
        Process::persistent_subject_runner_set(subject_main_thread);
    }

    return VTSS_RC_OK;
}
