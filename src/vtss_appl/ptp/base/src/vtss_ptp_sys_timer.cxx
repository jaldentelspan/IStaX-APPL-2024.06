/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss_ptp_sys_timer.h"
#include "vtss_ptp_api.h"
#include "critd_api.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_PTP

// Using vtss::Vector() might be slow if a lot of starts and stops happen, since
// it's a linear search to find it. Furthermore, the container allocates at
// least 4096 elements.
// Using vtss::List() is faster, but requires calls to malloc() and free() for
// each insertion and deletion.
// So we keep track of the list of active timers ourselves.
// The following's next pointer points to the first active timer or NULL if no
// active timers.
static vtss_ptp_sys_timer_t active_timers;
extern u64 tick_duration_ns;
extern u64 tick_duration_us;

static critd_t ptp_timer_crit;
struct ptp_timer_scope_lock {
    ptp_timer_scope_lock(const char *func, int line) {
        critd_enter(&ptp_timer_crit, func, line);
    }
    ~ptp_timer_scope_lock(void) {
        critd_exit(&ptp_timer_crit, __FILE__, 0);
    }
};

#define PTP_TIMER_LOCK_SCOPE() ptp_timer_scope_lock __lock_guard__(__FUNCTION__, __LINE__)
#define PTP_TIMER_LOCK()       critd_enter(&ptp_timer_crit,        __FUNCTION__, __LINE__)
#define PTP_TIMER_UNLOCK()     critd_exit( &ptp_timer_crit,        __FUNCTION__, __LINE__)

/******************************************************************************/
// vtss_ptp_timer_link()
// Add timer to the front of list of active timers.
/******************************************************************************/
static inline void vtss_ptp_timer_link(vtss_ptp_sys_timer_t *t)
{
    VTSS_ASSERT(t->prev == NULL && t->next == NULL);

    if (active_timers.next) {
        active_timers.next->prev = t;
        t->next = active_timers.next;
    }

    t->prev = &active_timers;
    active_timers.next = t;
}

/******************************************************************************/
// vtss_ptp_timer_unlink()
// Removes a timer from active list of timers
/******************************************************************************/
static inline void vtss_ptp_timer_unlink(vtss_ptp_sys_timer_t *t)
{
    VTSS_ASSERT(t->prev);

    t->prev->next = t->next;

    if (t->next) {
       t->next->prev = t->prev;
    }

    t->prev         = NULL;
    t->next         = NULL;
    t->delay_unlink = false;
}

/******************************************************************************/
// vtss_ptp_timer_init()
/******************************************************************************/
void vtss_ptp_timer_init(vtss_ptp_sys_timer_t *t, const char *name, int instance, vtss_ptp_sys_timer_callout_t callout, void *context)
{
    VTSS_ASSERT(t);
    VTSS_ASSERT(callout);

    T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER, "%s#%d: callout = %p, context = %p", name, instance, callout, context);

    PTP_TIMER_LOCK_SCOPE();
    // Unlink if it is ongoing
    if (t->prev || t->next) {
        // No vtss_ptp_timer_init is part of callouts. Hence, safe to unlink.
        vtss_ptp_timer_unlink(t);
    }

    memset(t, 0, sizeof(*t));

    t->name     = name;
    t->instance = instance;
    t->callout  = callout;
    t->context  = context;
}

/******************************************************************************/
// vtss_ptp_timer_start()
/******************************************************************************/
void vtss_ptp_timer_start(vtss_ptp_sys_timer_t *t, u32 period_ticks, bool repeat)
{
   u64 now_us;

    PTP_TIMER_LOCK_SCOPE();

    VTSS_ASSERT(t);

    if (t->callout == NULL) {
        T_EG(VTSS_TRACE_GRP_PTP_BASE_TIMER,"%s#%d: Attempting to start an uninitialized timer (%p) with period = %u ticks and repeat = %d", t->name, t->instance, t, period_ticks, repeat);
        return;
    }

    if (period_ticks == 0 && repeat) {
        T_EG(VTSS_TRACE_GRP_PTP_BASE_TIMER,"%s#%d: Attempting to start a periodic timer (%p) with a period of 0 ticks", t->name, t->instance, t);
        return;
    }

    now_us = vtss::uptime_microseconds();
    // Gotta compute the period in ns, because of rounding errors if doing it in
    // us. Example: 'ptp 0 log 1 max-time 100' won't give a timeout of 100 secs
    // if using us, but 99 secs, which looks odd in ICLI when dumped with
    // 'show ptp 0 log-mode'
    t->period_us    = (period_ticks * tick_duration_ns) / 1000LLU;
    t->periodic     = repeat;
    t->timeout_us   = tick_duration_us * PTP_DIV_ROUND_UP(now_us + t->period_us, tick_duration_us);
    t->delay_unlink = false;

    if (t->prev || t->next) {
        // Already in list of active timers.
        T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER,"%s#%d: Restart of timer already in list", t->name, t->instance);
    } else {
        // Add it to the list of active timers.
        vtss_ptp_timer_link(t);
    }

    T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER,"%s#%d: period = %d ticks = " VPRI64u " us, repeat = %d", t->name, t->instance, period_ticks, t->period_us, repeat);

    void ptp_timer_tick_start(const vtss_ptp_sys_timer_t *const t);
    ptp_timer_tick_start(t);
}

/******************************************************************************/
// vtss_ptp_timer_stop()
/******************************************************************************/
void vtss_ptp_timer_stop(vtss_ptp_sys_timer_t *t)
{
    PTP_TIMER_LOCK_SCOPE();

    VTSS_ASSERT(t);

    if (t->prev || t->next) {
        // Set the timer to be ready for deletion
        T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER,"Mark %s#%d timer as to be deleted", t->name, t->instance);
        t->delay_unlink = true;
    } else {
        T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER,"%s#%d not active. Unable to remove", t->name, t->instance);
    }
}

/******************************************************************************/
// vtss_ptp_timer_tick()
/******************************************************************************/
u64 vtss_ptp_timer_tick(u64 now_us)
{
    vtss_ptp_sys_timer_t *t;
    u64                  next_timeout_us;

    T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER, "Now = " VPRI64u " us", now_us);

    PTP_TIMER_LOCK();

    t = active_timers.next;
    while (t) {
        // In the following, there are two places that may change the list of
        // active timers:
        //   1) A non-periodic timer that times out causes the entry to be
        //      unlinked from the list.
        //   2) t->callout() may also call into the vtss_ptp_timer_XXX()
        //      functions and e.g. stop (and thereby unlink) or restart the
        //      timer.
        // That's why we keep a pointer to the next.
        vtss_ptp_sys_timer_t *next_t = t->next;

        T_NG(VTSS_TRACE_GRP_PTP_BASE_TIMER, "Considering %s#%d with timeout = " VPRI64u " us", t->name, t->instance, t->timeout_us);

        if (!t->delay_unlink && t->timeout_us <= now_us) {
            // Timeout
            T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER,"%s#%d. Timed out. Periodic = %d", t->name, t->instance, t->periodic);

            if (t->periodic) {
                u32 loss_cnt = 0;

                // Make it as non-drifting as possible
                t->timeout_us += t->period_us;

                while (t->timeout_us <= now_us) {
                    loss_cnt++;
                    t->timeout_us += t->period_us;
                }

                if (loss_cnt) {
                    t->callback_loss_cnt += loss_cnt;
                    T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER,"%s#%d. Can't keep up the pace: Lost %u callbacks with a period = " VPRI64u " us", t->name, t->instance, loss_cnt, t->period_us);
                }
            } else {
                // Mark timer to be removed from list of active timers.
                t->delay_unlink = true;
            }

            T_NG(VTSS_TRACE_GRP_PTP_BASE_TIMER,"%s#%d. Invoking callout = %p", t->name, t->instance, t->callout);

            t->callback_cnt++;

            PTP_TIMER_UNLOCK();
            t->callout(t, t->context);
            PTP_TIMER_LOCK();
            T_NG(VTSS_TRACE_GRP_PTP_BASE_TIMER,"%s#%d. Done invoking callout = %p", t->name, t->instance, t->callout);
        }

        if (t->delay_unlink) {
            T_NG(VTSS_TRACE_GRP_PTP_BASE_TIMER,"Removing %s#%d from list of active timers", t->name, t->instance);
            vtss_ptp_timer_unlink(t);
        }

        t = next_t;
    }

    // Re-iterate once more (timers may have gone or come) to compute the next
    // timeout in terms of ticks.
    next_timeout_us = (u64)-1;
    for (t = active_timers.next; t != NULL; t = t->next) {
        // Ignore timers set for deletion
        if (t->delay_unlink) {
            continue;
        }

        if (t->timeout_us < next_timeout_us) {
            next_timeout_us = t->timeout_us;
        }
    }

    T_DG(VTSS_TRACE_GRP_PTP_BASE_TIMER, "Now = " VPRI64u " us, next timeout = " VPRI64u " us, i.e. in " VPRI64u " us from now", now_us, next_timeout_us, next_timeout_us - now_us);

    PTP_TIMER_UNLOCK();

    return next_timeout_us;
}

/******************************************************************************/
// vtss_ptp_timers_dump()
// Dumps active timers only.
/******************************************************************************/
void vtss_ptp_timers_dump(u32 session_id, i32 (*pr)(u32 session_id, const char *fmt, ...))
{
    vtss_ptp_sys_timer_t *t;
    u32                  cnt = 0;
    u64                  now_us = vtss::uptime_microseconds();

    pr(session_id, "Timer name              Inst Period [us] Time left [us] Callbacks  Losses     Periodic Delayed delete\n");
    pr(session_id, "----------------------- ---- ----------- -------------- ---------- ---------- -------- --------------\n");

    PTP_TIMER_LOCK_SCOPE();

    for (t = active_timers.next; t != NULL; t = t->next) {
        pr(session_id, "%-23s %4d %11u " VPRI64Fd("14") " %10u %10u %8s %14s\n",
           t->name,
           t->instance,
           t->period_us,
           (i64)t->timeout_us - (i64)now_us,
           t->callback_cnt,
           t->callback_loss_cnt,
           t->periodic ? "Yes" : "No",
           t->delay_unlink ? "true" : "false");
        cnt++;
    }

    if (!cnt) {
        pr(session_id, "<No timers active>\n");
    }

    pr(session_id, "\n");
}

/******************************************************************************/
// vtss_ptp_timer_initialize()
/******************************************************************************/
void vtss_ptp_timer_initialize(void)
{
    // We need our own mutex to protect the list of active timers.
    // The reason is that it cannot be guaranteed that the PTP_CORE_LOCK() is
    // taken at all times when we get in here. An example of this is:
    //   zl_3038x_api_pdv.cxx invokes apr_step_time_tsu(),  which invokes
    //   ptp_local_clock.cxx#vtss_local_clock_adj_offset(), which invokes
    //   ptp.cxx#ptp_time_setting_start(), which directly invokes
    //   vtss_ptp_sys_timer.cxx#vtss_ptp_timer_start(), which may fiddle with
    //   the pointers to the list of active timers.
    // Simply taking the PTP_CORE_LOCK() in ptp_time_setting_start() leads to
    // deadlocks between PTP_CORE_LOCK() and CLOCK_LOCK().
    critd_init(&ptp_timer_crit, "ptp.timer", VTSS_MODULE_ID_PTP, CRITD_TYPE_MUTEX);
}

