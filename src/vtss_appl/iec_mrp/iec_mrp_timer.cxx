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

#include "iec_mrp_timer.hxx" /* For ourselves           */
#include "iec_mrp_trace.h"   /* For MRP_TRACE_GRP_TIMER */
#include "iec_mrp_lock.hxx"  /* For MRP_LOCK_SCOPE()    */

// Using vtss::Vector() might be slow if a lot of starts and stops happen, since
// it's a linear search to find it. Furthermore, the container allocates at
// least 4096 elements.
// Using vtss::List() is faster, but requires calls to malloc() and free() for
// each insertion and deletion.
// So we keep track of the list of active timers ourselves.
// The following's next pointer points to the first active timer or NULL if no
// active timers.
static mrp_timer_t active_timers;

// Thread variables
static vtss_handle_t MRP_TIMER_thread_handle;
static vtss_thread_t MRP_TIMER_thread_block;
static vtss_flag_t   MRP_TIMER_wait_flag;
static uint64_t      MRP_TIMER_next_timeout_us;

/******************************************************************************/
// MRP_TIMER_link()
// Add timer to the front of list of active timers.
/******************************************************************************/
static inline void MRP_TIMER_link(mrp_timer_t &t)
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
// MRP_TIMER_unlink()
// Removes a timer from active list of timers
/******************************************************************************/
static inline void MRP_TIMER_unlink(mrp_timer_t &t)
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
// MRP_TIMER_run()
/******************************************************************************/
static uint64_t MRP_TIMER_run(uint64_t now_us)
{
    mrp_timer_t *t;
    uint64_t    next_timeout_us;

    T_DG(MRP_TRACE_GRP_TIMER, "Now = " VPRI64u " us", now_us);

    t = active_timers.next;
    while (t) {
        // In the following, there are two places that may change the list of
        // active timers:
        //   1) A non-periodic timer that times out causes the entry to be
        //      unlinked from the list.
        //   2) t->callback() may also call into the mrp_timer_XXX() functions
        //      and e.g. stop (and thereby unlink) or restart the timer.
        // That's why we keep a pointer to the next.
        mrp_timer_t *next_t = t->next;

        T_NG(MRP_TRACE_GRP_TIMER, "Considering %s#%d with timeout = " VPRI64u " us", t->name, t->instance, t->timeout_us);

        if (t->timeout_us <= now_us) {
            // Timeout
            T_DG(MRP_TRACE_GRP_TIMER, "%s#%d. Timed out. Periodic = %d", t->name, t->instance, t->periodic);

            if (t->periodic) {
                uint32_t loss_cnt = 0;

                // Make it as non-drifting as possible
                t->timeout_us += t->period_us;

                while (t->timeout_us <= now_us) {
                    loss_cnt++;
                    t->timeout_us += t->period_us;
                }

                if (loss_cnt) {
                    t->callback_loss_cnt += loss_cnt;
                    T_DG(MRP_TRACE_GRP_TIMER, "%s#%d. Can't keep up the pace: Lost %u callbacks with a period = " VPRI64u " us", t->name, t->instance, loss_cnt, t->period_us);
                }
            } else {
                // Remove timer from list of active timers.
                T_NG(MRP_TRACE_GRP_TIMER, "Removing %s#%d from list of active timers", t->name, t->instance);
                MRP_TIMER_unlink(*t);
            }

            T_NG(MRP_TRACE_GRP_TIMER, "%s#%d. Invoking callback = %p", t->name, t->instance, t->callback);

            t->callback_cnt++;
            t->callback(*t, t->context);

            T_NG(MRP_TRACE_GRP_TIMER, "%s#%d. Done invoking callback = %p", t->name, t->instance, t->callback);
        }

        t = next_t;
    }

    // Re-iterate once more (timers may have gone or come) to compute the next
    // timeout in microseconds
    next_timeout_us = -1;
    for (t = active_timers.next; t != NULL; t = t->next) {
        if (t->timeout_us < next_timeout_us) {
            next_timeout_us = t->timeout_us;
        }
    }

    T_DG(MRP_TRACE_GRP_TIMER, "Now = " VPRI64u " us, next timeout = " VPRI64u " us, i.e. in " VPRI64u " us from now", now_us, next_timeout_us, next_timeout_us - now_us);

    return next_timeout_us;
}

/******************************************************************************/
// MRP_TIMER_thread()
/******************************************************************************/
static void MRP_TIMER_thread(vtss_addrword_t data)
{
    vtss_flag_value_t flags;

    // Wait until kickstarted
    MRP_TIMER_next_timeout_us = -1;

    while (1) {
        uint64_t now_us;

        if (MRP_TIMER_next_timeout_us != -1) {
            flags = vtss_flag_timed_wait(&MRP_TIMER_wait_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR, VTSS_OS_MSEC2TICK(MRP_TIMER_next_timeout_us / 1000));
        } else {
            // Nothing going on right now. Wait for something to happen.
            flags = vtss_flag_wait(&MRP_TIMER_wait_flag, 0xFFFFFFFF, VTSS_FLAG_WAITMODE_OR_CLR);
        }

        now_us = vtss::uptime_microseconds();

        {
            MRP_LOCK_SCOPE();
            MRP_TIMER_next_timeout_us = MRP_TIMER_run(now_us);
            T_NG(MRP_TRACE_GRP_TIMER, "now_us = " VPRI64u ". flags = 0x%x, next_timeout_us = " VPRI64d, now_us, flags, MRP_TIMER_next_timeout_us);
        }
    }
}

/******************************************************************************/
// MRP_TIMER_kickstart()
/******************************************************************************/
static void MRP_TIMER_kickstart(const mrp_timer_t &timer)
{
    MRP_LOCK_ASSERT_LOCKED("%s#%d", timer.name, timer.instance);

    // Avoid kick-starting the thread if not necessary.
    if (timer.timeout_us < MRP_TIMER_next_timeout_us) {
        // The new tick timeout is before the current, so we need to signal the
        // thread to get it to timeout sooner.
        T_IG(MRP_TRACE_GRP_TIMER, "%s#%d: Kickstarting timer. Requested timeout_us = " VPRI64u, timer.name, timer.instance, timer.timeout_us);

        MRP_TIMER_next_timeout_us = timer.timeout_us;
        vtss_flag_setbits(&MRP_TIMER_wait_flag, 1);
    }
}

/******************************************************************************/
// mrp_timer_init()
/******************************************************************************/
void mrp_timer_init(mrp_timer_t &t, const char *name, int instance, mrp_timer_callback_t callback, struct mrp_state_s *context)
{
    VTSS_ASSERT(callback);

    T_DG(MRP_TRACE_GRP_TIMER, "%s#%d: callback = %p, context = %p", name, instance, callback, context);

    // Stop it if ongoing
    mrp_timer_stop(t);

    memset(&t, 0, sizeof(t));
    t.name      = name;
    t.instance  = instance;
    t.callback  = callback;
    t.context   = context;
}

/******************************************************************************/
// mrp_timer_start()
/******************************************************************************/
void mrp_timer_start(mrp_timer_t &t, uint32_t period_us, bool repeat)
{
    uint64_t now_us;

    if (t.callback == NULL) {
        T_EG(MRP_TRACE_GRP_TIMER, "%s#%d: Attempting to start an uninitialized timer (%p) with period = %u us and repeat = %d", t.name, t.instance, &t, t.period_us, repeat);
        return;
    }

    if (period_us == 0 && repeat) {
        T_EG(MRP_TRACE_GRP_TIMER, "%s#%d: Attempting to start a periodic timer (%p) with a period of 0 us", t.name, t.instance, &t);
        return;
    }

    now_us = vtss::uptime_microseconds();

    t.period_us    = period_us;
    t.periodic     = repeat;
    t.timeout_us   = now_us + period_us;

    if (t.prev || t.next) {
        // Already in list of active timers.
        T_DG(MRP_TRACE_GRP_TIMER, "%s#%d: Restart of already active timer", t.name, t.instance);
    } else {
        // Add it to the list of active timers.
        MRP_TIMER_link(t);
    }

    T_DG(MRP_TRACE_GRP_TIMER, "%s#%d: period = %u us, repeat = %d", t.name, t.instance, t.period_us, repeat);

    MRP_TIMER_kickstart(t);
}

/******************************************************************************/
// mrp_timer_extend()
/******************************************************************************/
void mrp_timer_extend(mrp_timer_t &t, uint32_t timeout_us)
{
    uint64_t now_us, time_left_us;

    if (!t.prev && !t.next) {
        // Timer not active. Start it non-repeating.
        mrp_timer_start(t, timeout_us, false);
        return;
    }

    // Timer is currently active.
    if (t.periodic) {
        T_EG(MRP_TRACE_GRP_TIMER, "%s#%d: Cannot extend a periodic timer", t.name, t.instance);
        return;
    }

    now_us = vtss::uptime_microseconds();
    if (now_us > t.timeout_us) {
        time_left_us = now_us - t.timeout_us;
    } else {
        time_left_us = 0;
    }

    if (timeout_us > time_left_us) {
        // Restart the timer with a new timeout.
        mrp_timer_start(t, timeout_us, false);
    }
}

/******************************************************************************/
// mrp_timer_stop()
/******************************************************************************/
void mrp_timer_stop(mrp_timer_t &t)
{
    if (t.prev || t.next) {
        // Remove from list of active timers
        T_DG(MRP_TRACE_GRP_TIMER, "Removing %s#%d from list of active timers", t.name, t.instance);
        MRP_TIMER_unlink(t);
    } else {
        T_DG(MRP_TRACE_GRP_TIMER, "%s#%d not active. Unable to remove", t.name, t.instance);
    }
}

/******************************************************************************/
// mrp_timer_active()
/******************************************************************************/
bool mrp_timer_active(mrp_timer_t &t)
{
    return t.prev || t.next;
}

/******************************************************************************/
// mrp_timer_debug_dump()
// Dumps active timers only.
/******************************************************************************/
void mrp_timer_debug_dump(uint32_t session_id, i32 (*pr)(uint32_t session_id, const char *fmt, ...))
{
    mrp_timer_t *t;
    uint32_t    cnt = 0;
    uint64_t    now_us = vtss::uptime_microseconds();

    pr(session_id, "Timer name              Inst Period [us] Time left [us] Callbacks  Losses     Periodic\n");
    pr(session_id, "----------------------- ---- ----------- -------------- ---------- ---------- --------\n");

    MRP_LOCK_SCOPE();

    for (t = active_timers.next; t != NULL; t = t->next) {
        pr(session_id, "%-23s %4d %11u " VPRI64Fd("14") " %10u %10u %s\n",
           t->name,
           t->instance,
           t->period_us,
           (int64_t)t->timeout_us - (int64_t)now_us,
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
// mrp_timer_init()
/******************************************************************************/
void mrp_timer_init(void)
{
    // Start our thread.
    vtss_flag_init(&MRP_TIMER_wait_flag);
    vtss_thread_create(VTSS_THREAD_PRIO_HIGH,
                       MRP_TIMER_thread,
                       0,
                       "MRP Timer",
                       nullptr,
                       0,
                       &MRP_TIMER_thread_handle,
                       &MRP_TIMER_thread_block);
}

