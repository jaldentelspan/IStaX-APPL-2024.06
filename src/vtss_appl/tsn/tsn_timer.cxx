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

#include "tsn_timer.hxx" /* For ourselves            */
#include "tsn_trace.h"   /* For TSN_TRACE_GRP_TIMER */
#include "tsn_lock.hxx"  /* For TSN_LOCK_SCOPE()    */

// Using vtss::Vector() might be slow if a lot of starts and stops happen, since
// it's a linear search to find it. Furthermore, the container allocates at
// least 4096 elements.
// Using vtss::List() is faster, but requires calls to malloc() and free() for
// each insertion and deletion.
// So we keep track of the list of active timers ourselves.
// The following's next pointer points to the first active timer or NULL if no
// active timers.
static tsn_timer_t active_timers;

// Thread variables
static vtss_handle_t TSN_TIMER_thread_handle;
static vtss_thread_t TSN_TIMER_thread_block;
static vtss_flag_t   TSN_TIMER_wait_flag;
static uint64_t      TSN_TIMER_next_timeout_ms;

/******************************************************************************/
// TSN_TIMER_link()
// Add timer to the front of list of active timers.
/******************************************************************************/
static inline void TSN_TIMER_link(tsn_timer_t &t)
{
    VTSS_ASSERT(t.prev == NULL && t.next == NULL);

    if (active_timers.next) {
        active_timers.next->prev = &t;
        t.next = active_timers.next;
    }

    t.prev = &active_timers;
    active_timers.next = &t;
}

/******************************************************************************/
// TSN_TIMER_unlink()
// Removes a timer from active list of timers
/******************************************************************************/
static inline void TSN_TIMER_unlink(tsn_timer_t &t)
{
    VTSS_ASSERT(t.prev);

    t.prev->next = t.next;

    if (t.next) {
        t.next->prev = t.prev;
    }

    t.prev = NULL;
    t.next = NULL;
}

/******************************************************************************/
// TSN_TIMER_run()
/******************************************************************************/
static uint64_t TSN_TIMER_run(uint64_t now_ms)
{
    tsn_timer_t *t;
    uint64_t    next_timeout_ms;

    T_DG(TSN_TRACE_GRP_TIMER, "Now = " VPRI64u " ms", now_ms);

    t = active_timers.next;
    while (t) {
        // In the following, there are two places that may change the list of
        // active timers:
        //   1) A non-periodic timer that times out causes the entry to be
        //      unlinked from the list.
        //   2) t->callback() may also call into the tsn_timer_XXX() functions
        //      and e.g. stop (and thereby unlink) or restart the timer.
        // That's why we keep a pointer to the next.
        tsn_timer_t *next_t = t->next;

        T_NG(TSN_TRACE_GRP_TIMER, "Considering %s#%d with timeout = " VPRI64u " ms", t->name, t->instance, t->timeout_ms);

        if (t->timeout_ms <= now_ms) {
            // Timeout
            T_DG(TSN_TRACE_GRP_TIMER, "%s#%d. Timed out. Periodic = %d", t->name, t->instance, t->periodic);

            if (t->periodic) {
                uint32_t loss_cnt = 0;

                // Make it as non-drifting as possible
                t->timeout_ms += t->period_ms;

                while (t->timeout_ms <= now_ms) {
                    loss_cnt++;
                    t->timeout_ms += t->period_ms;
                }

                if (loss_cnt) {
                    t->callback_loss_cnt += loss_cnt;
                    T_DG(TSN_TRACE_GRP_TIMER, "%s#%d. Can't keep up the pace: Lost %u callbacks with a period = " VPRI64u " ms", t->name, t->instance, loss_cnt, t->period_ms);
                }
            } else {
                // Remove timer from list of active timers.
                T_NG(TSN_TRACE_GRP_TIMER, "Removing %s#%d from list of active timers", t->name, t->instance);
                TSN_TIMER_unlink(*t);
            }

            T_NG(TSN_TRACE_GRP_TIMER, "%s#%d. Invoking callback = %p", t->name, t->instance, t->callback);

            t->callback_cnt++;
            t->callback(*t, t->context);

            T_NG(TSN_TRACE_GRP_TIMER, "%s#%d. Done invoking callback = %p", t->name, t->instance, t->callback);
        }

        t = next_t;
    }

    // Re-iterate once more (timers may have gone or come) to compute the next
    // timeout in milliseconds
    next_timeout_ms = -1;
    for (t = active_timers.next; t != NULL; t = t->next) {
        if (t->timeout_ms < next_timeout_ms) {
            next_timeout_ms = t->timeout_ms;
        }
    }

    T_DG(TSN_TRACE_GRP_TIMER, "Now = " VPRI64u " ms, next timeout = " VPRI64u " ms, i.e. in " VPRI64u " ms from now", now_ms, next_timeout_ms, next_timeout_ms - now_ms);

    return next_timeout_ms;
}

/******************************************************************************/
// TSN_TIMER_thread()
/******************************************************************************/
static void TSN_TIMER_thread(vtss_addrword_t data)
{
    vtss_flag_value_t flags;

    // Wait until kickstarted
    TSN_TIMER_next_timeout_ms = -1;

    while (1) {
        uint64_t now_ms;

        if (TSN_TIMER_next_timeout_ms != -1) {
            flags = vtss_flag_timed_wait(&TSN_TIMER_wait_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR, VTSS_OS_MSEC2TICK(TSN_TIMER_next_timeout_ms));
        } else {
            // Nothing going on right now. Wait for something to happen.
            flags = vtss_flag_wait(&TSN_TIMER_wait_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR);
        }

        now_ms = vtss::uptime_milliseconds();

        {
            TSN_LOCK_SCOPE();
            TSN_TIMER_next_timeout_ms = TSN_TIMER_run(now_ms);
            T_NG(TSN_TRACE_GRP_TIMER, "now_ms = " VPRI64u ". flags = 0x%x, next_timeout_ms = " VPRI64d, now_ms, flags, TSN_TIMER_next_timeout_ms);
        }
    }
}

/******************************************************************************/
// TSN_TIMER_kickstart()
/******************************************************************************/
static void TSN_TIMER_kickstart(const tsn_timer_t &timer)
{
    TSN_LOCK_ASSERT_LOCKED("%s#%d", timer.name, timer.instance);

    // Avoid kick-starting the thread if not necessary.
    if (timer.timeout_ms < TSN_TIMER_next_timeout_ms) {
        // The new tick timeout is before the current, so we need to signal the
        // thread to get it to timeout sooner.
        T_IG(TSN_TRACE_GRP_TIMER, "%s#%d: Kickstarting timer. Requested timeout_ms = " VPRI64u, timer.name, timer.instance, timer.timeout_ms);

        TSN_TIMER_next_timeout_ms = timer.timeout_ms;
        vtss_flag_setbits(&TSN_TIMER_wait_flag, 1);
    }
}

/******************************************************************************/
// tsn_timer_init()
/******************************************************************************/
void tsn_timer_init(tsn_timer_t &t, const char *name, int instance, tsn_timer_callback_t callback, void *context)
{
    VTSS_ASSERT(callback);

    T_DG(TSN_TRACE_GRP_TIMER, "%s#%d: callback = %p, context = %p", name, instance, callback, context);

    // Stop it if ongoing
    tsn_timer_stop(t);

    memset(&t, 0, sizeof(t));
    t.name      = name;
    t.instance  = instance;
    t.callback  = callback;
    t.context   = context;
}

/******************************************************************************/
// tsn_timer_start()
/******************************************************************************/
void tsn_timer_start(tsn_timer_t &t, uint32_t period_ms, bool repeat)
{
    uint64_t now_ms;

    if (t.callback == NULL) {
        T_EG(TSN_TRACE_GRP_TIMER, "%s#%d: Attempting to start an uninitialized timer (%p) with period = %u ms and repeat = %d", t.name, t.instance, &t, t.period_ms, repeat);
        return;
    }

    if (period_ms == 0 && repeat) {
        T_EG(TSN_TRACE_GRP_TIMER, "%s#%d: Attempting to start a periodic timer (%p) with a period of 0 ms", t.name, t.instance, &t);
        return;
    }

    now_ms = vtss::uptime_milliseconds();

    t.period_ms    = period_ms;
    t.periodic     = repeat;
    t.timeout_ms   = now_ms + period_ms;

    if (t.prev || t.next) {
        // Already in list of active timers.
        T_DG(TSN_TRACE_GRP_TIMER, "%s#%d: Restart of already active timer", t.name, t.instance);
    } else {
        // Add it to the list of active timers.
        TSN_TIMER_link(t);
    }

    T_DG(TSN_TRACE_GRP_TIMER, "%s#%d: period = %d ms, repeat = %d", t.name, t.instance, t.period_ms, repeat);

    TSN_TIMER_kickstart(t);
}

/******************************************************************************/
// tsn_timer_extend()
/******************************************************************************/
void tsn_timer_extend(tsn_timer_t &t, uint32_t timeout_ms)
{
    uint64_t now_ms, time_left_ms;

    if (!t.prev && !t.next) {
        // Timer not active. Start it non-repeating.
        tsn_timer_start(t, timeout_ms, false);
        return;
    }

    // Timer is currently active.
    if (t.periodic) {
        T_EG(TSN_TRACE_GRP_TIMER, "%s#%d: Cannot extend a periodic timer", t.name, t.instance);
        return;
    }

    now_ms = vtss::uptime_milliseconds();
    if (now_ms > t.timeout_ms) {
        time_left_ms = now_ms - t.timeout_ms;
    } else {
        time_left_ms = 0;
    }

    if (timeout_ms > time_left_ms) {
        // Restart the timer with a new timeout.
        tsn_timer_start(t, timeout_ms, false);
    }
}

/******************************************************************************/
// tsn_timer_stop()
/******************************************************************************/
void tsn_timer_stop(tsn_timer_t &t)
{
    if (t.prev || t.next) {
        // Remove from list of active timers
        T_DG(TSN_TRACE_GRP_TIMER, "Removing %s#%d from list of active timers", t.name, t.instance);
        TSN_TIMER_unlink(t);
    } else {
        T_DG(TSN_TRACE_GRP_TIMER, "%s#%d not active. Unable to remove", t.name, t.instance);
    }
}

/******************************************************************************/
// tsn_timer_active()
/******************************************************************************/
bool tsn_timer_active(tsn_timer_t &t)
{
    return t.prev || t.next;
}

/******************************************************************************/
// tsn_timer_debug_dump()
// Dumps active timers only.
/******************************************************************************/
void tsn_timer_debug_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    tsn_timer_t *t;
    uint32_t    cnt = 0;
    uint64_t    now_ms = vtss::uptime_milliseconds();

    pr(session_id, "Timer name              Inst Period [ms] Time left [ms] Callbacks  Losses     Periodic\n");
    pr(session_id, "----------------------- ---- ----------- -------------- ---------- ---------- --------\n");

    TSN_LOCK_SCOPE();

    for (t = active_timers.next; t != NULL; t = t->next) {
        pr(session_id, "%-23s %4d %11u " VPRI64Fd("14") " %10u %10u %s\n",
           t->name,
           t->instance,
           t->period_ms,
           (int64_t)t->timeout_ms - (int64_t)now_ms,
           t->callback_cnt,
           t->callback_loss_cnt,
           t->periodic ? "Yes" : "No");
        cnt++;
    }

    if (!cnt) {
        pr(session_id, "<No timers active>\n");
    }

    pr(session_id, "\n");
}

/******************************************************************************/
// tsn_timer_init()
/******************************************************************************/
void tsn_timer_init(void)
{
    // Start our thread.
    vtss_flag_init(&TSN_TIMER_wait_flag);
    vtss_thread_create(VTSS_THREAD_PRIO_HIGHER,
                       TSN_TIMER_thread,
                       0,
                       "TSN Timer",
                       nullptr,
                       0,
                       &TSN_TIMER_thread_handle,
                       &TSN_TIMER_thread_block);
}

