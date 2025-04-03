/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include <unistd.h>
#include "vtss/basics/list.hxx"
#include "vtss/basics/map.hxx"
#include "vtss/basics/time_unit.hxx"
#include "vtss/basics/vector.hxx"
#include "vtss/basics/memory.hxx"
#include "vtss_timer_api.h"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_TIMER

/****************************************************************************/
// Trace definitions
/****************************************************************************/
#include "vtss_module_id.h"
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID   VTSS_MODULE_ID_TIMER
#define VTSS_TRACE_GRP_DEFAULT 0
#include <vtss_trace_api.h>

// Trace registration. Initialized by vtss_timer_init() */
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "timer", "Timer module"
};

#ifndef VTSS_TIMER_DEFAULT_TRACE_LVL
#define VTSS_TIMER_DEFAULT_TRACE_LVL VTSS_TRACE_LVL_ERROR
#endif

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TIMER_DEFAULT_TRACE_LVL
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/****************************************************************************/
// Vitesse timer implementation using vtss_basics
/****************************************************************************/
static  vtss::List<vtss::Timer *> TIMER_list;
typedef vtss::List<vtss::Timer *>::iterator timer_list_iterator_t;
static void TIMER_add(vtss::Timer *timer)
{
    vtss_global_lock(__FILE__, __LINE__);

    timer_list_iterator_t itr = vtss::find(TIMER_list.begin(), TIMER_list.end(), timer);

    if (itr == TIMER_list.end()) {
        TIMER_list.push_back(timer);
    }

    vtss_global_unlock(__FILE__, __LINE__);
}

void vtss::TIMER_del(vtss::Timer *timer)
{
    vtss_global_lock(__FILE__, __LINE__);

    timer_list_iterator_t itr = vtss::find(TIMER_list.begin(), TIMER_list.end(), timer);

    if (itr != TIMER_list.end()) {
        TIMER_list.erase(itr);
    }

    vtss_global_unlock(__FILE__, __LINE__);
}

#include "icli_api.h"
void timer_debug_print(u32 session_id)
{
    u32 cnt = 0;
    vtss_global_lock(__FILE__, __LINE__);

    ICLI_PRINTF("Module                  Repeat Period [ms] Count\n");
    ICLI_PRINTF("----------------------- ------ ----------- ----------\n");

    for (timer_list_iterator_t itr = TIMER_list.begin(); itr != TIMER_list.end(); ++itr) {
        ICLI_PRINTF("%-23s %-6s " VPRI64Fu("11") " %10u\n",
                    vtss_module_names[(*itr)->modid],
                    (*itr)->get_repeat() ? "Yes" : "No",
                    vtss::to_milliseconds((*itr)->get_period()).raw(),
                    (*itr)->total_cnt);
        cnt++;
    }

    if (!cnt) {
        ICLI_PRINTF("<none>\n");
    }

    vtss_global_unlock(__FILE__, __LINE__);
}

mesa_rc vtss_timer_start(vtss::Timer *timer)
{
    const char *err_buf = NULL;

    if (timer->callback == NULL) {
        err_buf = "Invalid callback";
        goto do_exit;
    }

    if (timer->get_repeat() && timer->get_period() < vtss::microseconds {1000}) {
        err_buf = "When timer is repeated, period_us must be >= 1000 us";
        goto do_exit;
    }

    // Initialize selected private fields (remaining will be
    // initialized during insertion in active list)
    timer->total_cnt = 0;
    timer->add(&timer->my_timer, timer->my_timer.get_period());
    TIMER_add(timer);

do_exit:
    if (err_buf) {
        T_E("%s (module = %s)", err_buf, vtss_module_names[timer->modid]);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_timer_cancel(vtss::Timer *timer)
{
    timer->total_cnt           = 0;
    timer->del(&(timer->my_timer));
    TIMER_del(timer);

    // Do not clear timer->this and timer->total_cnt
    return VTSS_RC_OK;
}

void timer_test_callback(vtss::Timer *timer);

struct TimerTestTimerState {
    TimerTestTimerState(uint32_t itr,
                        vtss::milliseconds d,
                        vtss::milliseconds interval,
                        vtss_thread_prio_t prio) :
        t(prio), itr_cnt(itr), duration(d), start_of_execution(itr),
        end_of_execution(itr)
    {
        t.user_data = this;
        t.set_period(interval);
        t.callback = timer_test_callback;
        t.set_repeat(true);

        (void)vtss_timer_start(&t);
    }

    vtss::Timer t;
    uint32_t itr = 0;
    uint32_t itr_cnt;
    vtss::milliseconds duration;
    vtss::Vector<vtss::LinuxClock::time_point> start_of_execution;
    vtss::Vector<vtss::LinuxClock::time_point> end_of_execution;

};

void timer_test_callback(vtss::Timer *t)
{
    TimerTestTimerState *s = static_cast<TimerTestTimerState *>(t->user_data);
    s->start_of_execution.push_back(vtss::LinuxClock::now());
    s->itr += 1;

    if (s->itr >= s->itr_cnt) {
        vtss_timer_cancel(t);
    }

    vtss::LinuxClock::sleep(s->duration);

    s->end_of_execution.push_back(vtss::LinuxClock::now());
}

struct TimerTestState {
    TimerTestState(uint32_t cnt, uint32_t itr, vtss::milliseconds duration,
                   vtss::milliseconds interval, vtss_thread_prio_t prio) :
        timers(cnt)
    {
        for (uint32_t i = 0; i < cnt; ++i) {
            timers.push_back(vtss::make_unique<TimerTestTimerState>(itr,
                                                                    duration,
                                                                    interval,
                                                                    prio));
        }
    };

    vtss::Vector<std::unique_ptr<TimerTestTimerState>> timers;
};

static uint32_t timer_test_handle = 0;
vtss::Map<uint32_t, std::unique_ptr<TimerTestState>> timer_tests;

uint32_t vtss_timer_test_start(uint32_t timer_cnt,
                               vtss::milliseconds duration,
                               vtss::milliseconds interval,
                               uint32_t itr_cnt,
                               vtss_thread_prio_t prio)
{
    uint32_t handle = timer_test_handle++;
    timer_tests[handle] = vtss::make_unique<TimerTestState>(timer_cnt,
                                                            itr_cnt,
                                                            duration,
                                                            interval,
                                                            prio);
    return handle;
}

timer_test_status vtss_timer_test_status(uint32_t handle)
{
    timer_test_status h = {};
    auto i = timer_tests.find(handle);
    if (i == timer_tests.end()) {
        h.handle = -1;
        return h;
    }

    auto p = i->second.get();
    h.handle = handle;
    h.min_itr_cnt = (uint32_t) - 1;
    h.completed = 1;

    h.period.min = (uint32_t) - 1;

    for (auto &e : p->timers) {
        if (e->itr < h.min_itr_cnt) {
            h.min_itr_cnt = e->itr;
        }

        if (e->itr == e->itr_cnt) {
            size_t end = e->start_of_execution.size();
            for (size_t i = 1; i < end; i++) {
                auto a = e->start_of_execution[i];
                auto b = e->start_of_execution[i - 1];
                auto diff = vtss::LinuxClock::to_milliseconds(a - b).raw32();

                if (diff < h.period.min) {
                    h.period.min = diff;
                }

                if (diff > h.period.max) {
                    h.period.max = diff;
                }

                h.period.cnt += 1;
                h.period.mean += diff;
            }
        } else {
            h.completed = 0;
        }

        if (e->start_of_execution.size() && e->end_of_execution.size()) {
            auto a = e->start_of_execution[0];
            auto b = e->end_of_execution[e->end_of_execution.size() - 1];
            auto diff = vtss::LinuxClock::to_milliseconds(b - a).raw32();
            if (h.test_time < diff) {
                h.test_time = diff;
            }
        }
    }

    if (h.period.cnt != 0) {
        h.period.mean = h.period.mean / h.period.cnt;
    }

    return h;
}

extern "C" int timer_icli_cmd_register();

/****************************************************************************/
// vtss_timer_init()
// Module initialization function.
/****************************************************************************/
mesa_rc vtss_timer_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        timer_icli_cmd_register();
    }

    return VTSS_RC_OK;
}

