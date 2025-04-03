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

#ifndef _VTSS_TIMER_API_H_
#define _VTSS_TIMER_API_H_

#include "main_types.h" /* For vtss_init_data_t */
#include "subject.hxx"
#include "time.hxx"
#include "vtss_module_id.h" /* For vtss_module_id_t */

namespace vtss
{

void TIMER_del(struct Timer *timer);

typedef void (vtss_timer_cb_f)(struct Timer *timer);

struct Timer : public notifications::EventHandler {
    Timer(vtss_thread_prio_t prio = VTSS_THREAD_PRIO_DEFAULT)
        : notifications::EventHandler(
              &notifications::subject_runner_get(prio, false)), my_timer(this)
    {
        modid = VTSS_MODULE_ID_NONE;
    }

    void execute(vtss::notifications::Event *e)
    {
    }

    void execute(vtss::notifications::Timer *e)
    {
        if (!get_repeat()) {
            TIMER_del(this);
        }

        if (callback) {
            callback(this);
            total_cnt++;
        }
    }

    /**
     * Counts the total number of times the
     * counter has expired.
     */
    u32 total_cnt;

    // --------------------------------------
    // Public write members
    // --------------------------------------

    /**
     * The timer will trigger this many microseconds from now.
     * See restrictions under #repeat.
     */
    vtss::microseconds get_period() const
    {
        return vtss::microseconds(vtss::to_microseconds(my_timer.get_period()));
    }

    void set_period(vtss::microseconds time_unit)
    {
        my_timer.set_period(
            TimeUnit(time_unit.raw() / 1000, TimeUnit::Unit::milliseconds));
    }

    /**
     * When TRUE, the timer will trigger repeatedly every
     * #period_us microseconds.
     * When FALSE, the timer will only trigger once.
     *
     * When TRUE, #period_us must be at least 1000 microseconds.
     */
    bool get_repeat() const
    {
        return my_timer.get_repeat();
    }

    void set_repeat(bool repeat)
    {
        my_timer.set_repeat(repeat);
    }

    /**
     * Function to callback when the timer expires.
     * Once called back, the resources occupying
     * the timer should be freed if this is a
     * one-shot timer (i.e. #repeat == FALSE).
     */
    vtss_timer_cb_f *callback = nullptr;

    /**
     * Set your module's ID here, or leave it as is if your
     * module doesn't have a module ID.
     * The module ID is used for providing per-module
     * statistics, and is mainly for debugging.
     */
    vtss_module_id_t modid = 0;

    /**
     * User data. Not used by this module.
     */
    void *user_data = nullptr;

    /**
     * The timer struct defined by vtss::notifications::TimerBasic.
     */
    vtss::notifications::Timer my_timer;
};

} // namespace vtss

/**
 * \brief Start a timer.
 *
 * Once started, the timer will run and callback
 * as described in the #timer structure.
 *
 * The function must not be called from within
 * a timer-callback function if the timer is
 * repeating (unless it's a completely unrelated
 * timer).
 *
 * \param timer [IN] The timer to start.
 *
 * \return VTSS_RC_OK if the timer is valid
 * and started correctly, VTSS_RC_ERROR otherwise.
 */
mesa_rc vtss_timer_start(vtss::Timer *timer);

/**
 * \brief Cancel an on-going timer.
 *
 * Normally it only makes sense to cancel a repeating
 * timer, but once in a while it could happen that the state
 * of your module changes so that a one-shot timer needs
 * to be cancelled. In this case, the return value from
 * this function should be ignored, since the timer might
 * have fired or fire during the cancellation.
 *
 * The function may be called from within the callback function
 * in thread context if it's a repeating timer.
 *
 * \param timer [IN] The timer to cancel
 *
 * \return VTSS_RC_OK if the timer was successfully cancelled,
  * VTSS_RC_ERROR otherwise.
 */
mesa_rc vtss_timer_cancel(vtss::Timer *timer);

/**
 * \brief Module initialization function
 */
mesa_rc vtss_timer_init(vtss_init_data_t *data);

struct timer_test_stats {
    uint32_t min;
    uint32_t max;
    uint32_t mean;
    uint32_t cnt;
};

struct timer_test_status {
    uint32_t handle;
    uint32_t completed;
    uint32_t min_itr_cnt;
    uint32_t test_time;
    timer_test_stats period;
};

// Create 'timer_cnt' timer instances, each will sleep for 'duration' ms in its
// callback, each timer will be re-activated for 'itr_cnt' timers after
// 'interval' ms, and they will be running on a thread with priority 'prio'.
uint32_t vtss_timer_test_start(uint32_t timer_cnt, vtss::milliseconds duration,
                               vtss::milliseconds interval, uint32_t itr_cnt,
                               vtss_thread_prio_t prio);

timer_test_status vtss_timer_test_status(uint32_t handle);
void timer_debug_print(u32 session_id);

#endif /* _VTSS_TIMER_API_H_ */

