/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "thread_load_monitor_api.hxx"
#include "misc_api.h"
#include "critd_api.h"

#include "vtss_timer_api.h"
static vtss::Timer TLM_one_sec_timer(VTSS_THREAD_PRIO_BELOW_NORMAL);

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_THREAD_LOAD_MONITOR
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_THREAD_LOAD_MONITOR

static vtss_trace_reg_t TLM_trace_reg = {
    VTSS_TRACE_MODULE_ID, "ThreadLoadMon", "Thread Load Monitor"
};

static vtss_trace_grp_t TLM_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    }
};

VTSS_TRACE_REGISTER(&TLM_trace_reg, TLM_trace_grps);

typedef enum {
    TLM_USER_LOAD,
    TLM_USER_CONTEXT_SWITCHES,
    TLM_USER_LAST
} TLM_user_t;

typedef struct {
    u64  last_1sec;
    u64  last_10sec;
    u64  last_sample;
} TLM_sample_t;

typedef struct {
    // Updated by 1-sec tick:
    TLM_sample_t m[TLM_USER_LAST];
    bool updated;
} TLM_load_t;

static thread_load_monitor_page_faults_t TLM_page_faults;
static vtss::Map<int, TLM_load_t>        TLM_load;
static vtss::Map<int, TLM_load_t>        TLM_context_switches;
static critd_t                           TLM_crit;
static bool                              TLM_started;

struct TLM_Lock {
    TLM_Lock(int line)
    {
        critd_enter(&TLM_crit, __FILE__, line);
    }

    ~TLM_Lock()
    {
        critd_exit( &TLM_crit, __FILE__, 0);
    }
};

#define TLM_LOCK_SCOPE() TLM_Lock __tlm_lock_guard__(__LINE__)

/******************************************************************************/
// TLM_load_add()
/******************************************************************************/
static void TLM_load_add(int tid, TLM_user_t user, u64 now)
{
    auto itr = TLM_load.find(tid);
    if (itr == TLM_load.end()) {
        // Not calculated before.
        TLM_load_t t  = {};
        t.m[user].last_sample = now;
        t.updated     = true;
        TLM_load.set(tid, t);
    } else {
        itr->second.m[user].last_1sec   = now - itr->second.m[user].last_sample;
        itr->second.m[user].last_10sec  = itr->second.m[user].last_1sec + (itr->second.m[user].last_10sec * 9ULL) / 10ULL;
        itr->second.m[user].last_sample = now;
        itr->second.updated     = true;
    }
}

/******************************************************************************/
// TLM_one_sec_timeout()
/******************************************************************************/
static void TLM_one_sec_timeout(vtss::Timer *timer)
{
    vtss_handle_t      thread_handle = 0;
    vtss_proc_stat_t   stat;
    vtss_proc_vmstat_t vmstat;
    int                i;
    unsigned long      page_faults;

    T_D("Enter");

    TLM_LOCK_SCOPE();

    for (auto itr = TLM_load.begin(); itr != TLM_load.end(); ++itr) {
        itr->second.updated = false;
    }

    if (misc_proc_stat_parse(&stat) != VTSS_RC_OK) {
        T_E("misc_proc_stat_parse() failed");
        return;
    }

    T_D("Idle = %llu\n", stat.idle);

    // Store the idle time in thread ID -1.
    // We calculate the time taken by other processes and store it in thread ID 0 in thread_load_monitor_load_get().
    TLM_load_add(-1, TLM_USER_LOAD, stat.idle);

    while ((thread_handle = vtss_thread_get_next(thread_handle)) != 0) {
        vtss_thread_info_t      info;
        vtss_proc_thread_stat_t thread_stat;
        u64                     now, context_switches;

        // Handle load
        (void)vtss_thread_info_get(thread_handle, &info);
        if (misc_proc_thread_stat_parse(info.tid, &thread_stat) != VTSS_RC_OK) {
            T_E("Unable to get thread stat for TID = %d (%s)", info.tid, info.name);
            continue;
        }

        now = thread_stat.utime + thread_stat.stime;
        TLM_load_add(info.tid, TLM_USER_LOAD, now);

        // Handle context switches
        if (misc_thread_context_switches_get(info.tid, &context_switches) != VTSS_RC_OK) {
            // Error already printed
            continue;
        }

        TLM_load_add(info.tid, TLM_USER_CONTEXT_SWITCHES, context_switches);
    }

    for (auto itr = TLM_load.begin(); itr != TLM_load.end();) {
        if (!itr->second.updated) {
            TLM_load.erase(itr++);
        } else {
            ++itr;
        }
    }

    // Handle page-faults
    if (misc_proc_vmstat_parse(&vmstat) != VTSS_RC_OK) {
        T_E("misc_proc_vmstat_parse() failed");
        return;
    }

    for (i = 0; i < ARRSZ(vmstat.tlv); i++) {
        if (strcmp(vmstat.tlv[i].name, "pgfault") == 0) {
            break;
        }
    }

    if (i == ARRSZ(vmstat.tlv)) {
        T_E("No such entry in vmstat (pgfault)");
        return;
    }

    page_faults = vmstat.tlv[i].value;

    TLM_page_faults.one_sec = page_faults - TLM_page_faults.total;
    TLM_page_faults.ten_sec = TLM_page_faults.one_sec + (TLM_page_faults.ten_sec * 9ULL) / 10ULL;
    TLM_page_faults.total   = page_faults;

    T_D("Exit");
}

/******************************************************************************/
// thread_load_monitor_error_txt()
/******************************************************************************/
const char *thread_load_monitor_error_txt(mesa_rc rc)
{
    switch (rc) {
    case THREAD_LOAD_MONITOR_RC_TIMER_START:
        return "Unable to start timer";

    case THREAD_LOAD_MONITOR_RC_TIMER_STOP:
        return "Unable to stop timer";

    default:
        return "Unknown Thread Load Monitor error";
    }
}

/******************************************************************************/
// thread_load_monitor_start()
/******************************************************************************/
mesa_rc thread_load_monitor_start(void)
{
    TLM_LOCK_SCOPE();

    if (TLM_started) {
        return VTSS_RC_OK;
    }

    TLM_load.clear();
    memset(&TLM_page_faults, 0, sizeof(TLM_page_faults));

    if (vtss_timer_start(&TLM_one_sec_timer) != VTSS_RC_OK) {
        return THREAD_LOAD_MONITOR_RC_TIMER_START;
    }

    TLM_started = true;

    return VTSS_RC_OK;
}

/******************************************************************************/
// thread_load_monitor_stop()
/******************************************************************************/
mesa_rc thread_load_monitor_stop(void)
{
    TLM_LOCK_SCOPE();

    if (!TLM_started) {
        return VTSS_RC_OK;
    }

    if (vtss_timer_cancel(&TLM_one_sec_timer) != VTSS_RC_OK) {
        return THREAD_LOAD_MONITOR_RC_TIMER_STOP;
    }

    TLM_started = false;

    return VTSS_RC_OK;
}

/******************************************************************************/
// thread_load_monitor_load_get()
/******************************************************************************/
mesa_rc thread_load_monitor_load_get(vtss::Map<int, thread_load_monitor_load_t> &load, bool &started)
{
    u64                        sum_1sec = 0, sum_10sec = 0, sum1 = 0, sum10 = 0;
    const TLM_sample_t         *sample;
    thread_load_monitor_load_t l;

    T_D("Enter");

    load.clear();

    TLM_LOCK_SCOPE();

    started = TLM_started;

    for (auto itr = TLM_load.cbegin(); itr != TLM_load.cend(); ++itr) {
        sample = &itr->second.m[TLM_USER_LOAD];
        sum_1sec  += sample->last_1sec;
        sum_10sec += sample->last_10sec;
    }

    if (sum_1sec != 0 || sum_10sec != 0) {
        // Find the percentages.
        for (auto itr = TLM_load.cbegin(); itr != TLM_load.cend(); ++itr) {
            sample = &itr->second.m[TLM_USER_LOAD];
            l.one_sec_load = sum_1sec  ? (10000ULL * sample->last_1sec)  / sum_1sec  : 0;
            l.ten_sec_load = sum_10sec ? (10000ULL * sample->last_10sec) / sum_10sec : 0;
            sum1  += l.one_sec_load;
            sum10 += l.ten_sec_load;
            (void)load.set(itr->first, l);
        }
    }

    // Add <other> element into thread ID 0.
    l.one_sec_load = 10000ULL - sum1;
    l.ten_sec_load = 10000ULL - sum10;
    (void)load.set(0, l);

    T_D("Exit");

    return VTSS_RC_OK;
}

/******************************************************************************/
// thread_load_monitor_context_switches_get()
/******************************************************************************/
mesa_rc thread_load_monitor_context_switches_get(vtss::Map<int, thread_load_monitor_context_switches_t> &context_switches, bool &started)
{
    u64                                    sum_1sec = 0, sum_10sec = 0, sum_total = 0;
    const TLM_sample_t                     *sample;
    thread_load_monitor_context_switches_t c;

    T_D("Enter");

    context_switches.clear();

    TLM_LOCK_SCOPE();

    started = TLM_started;

    for (auto itr = TLM_load.cbegin(); itr != TLM_load.cend(); ++itr) {
        sample = &itr->second.m[TLM_USER_CONTEXT_SWITCHES];
        c.one_sec = sample->last_1sec;
        c.ten_sec = sample->last_10sec;
        c.total   = sample->last_sample;
        (void)context_switches.set(itr->first, c);

        sum_1sec  += sample->last_1sec;
        sum_10sec += sample->last_10sec;
        sum_total += sample->last_sample;
    }

    // Add total element into thread ID 0.
    c.one_sec = sum_1sec;
    c.ten_sec = sum_10sec;
    c.total   = sum_total;
    (void)context_switches.set(0, c);

    T_D("Exit");

    return VTSS_RC_OK;
}

/******************************************************************************/
// thread_load_monitor_page_faults_get()
/******************************************************************************/
mesa_rc thread_load_monitor_page_faults_get(thread_load_monitor_page_faults_t &page_faults, bool &started)
{
    TLM_LOCK_SCOPE();

    started = TLM_started;
    page_faults = TLM_page_faults;

    return VTSS_RC_OK;
}

/******************************************************************************/
// thread_load_monitor_init()
/******************************************************************************/
mesa_rc thread_load_monitor_init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_INIT:
        // On Linux, we must do the hard work ourselves. We need a one-second timer.
        TLM_one_sec_timer.set_repeat(true);
        TLM_one_sec_timer.set_period(vtss::seconds(1));
        TLM_one_sec_timer.callback = TLM_one_sec_timeout;
        TLM_one_sec_timer.modid = VTSS_MODULE_ID_THREAD_LOAD_MONITOR;
        // When something happens in this module, and we wish to print a trace
        // error (with T_E()), we would like to be able to do so without an
        // "ASSERTION FAILED" error just because we attempt to lock our own
        // mutex twice, because T_E() causes misc_thread_status_print() to be
        // called, which in turns causes thread_load_monitor_load_get() to be called,
        // which in turn attempts to lock our own mutex - hence recursive.
        // Alternatively, one could avoid calling T_E() in this module and all
        // external functions invoked by this module.
        critd_init(&TLM_crit, "ThreadLoadMonitor", VTSS_MODULE_ID_THREAD_LOAD_MONITOR, CRITD_TYPE_MUTEX_RECURSIVE);

        // TLM_one_sec_timeout() is called back on a low-priority thread and
        // it might take a while to traverse all threads by that function, so
        // we are allowing the mutex to be taken indefinitely.
        TLM_crit.max_lock_time = -1;

        // Enable the following line to start the thread monitor at the earliest possible stage of boot.
        // (void)thread_load_monitor_start();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

