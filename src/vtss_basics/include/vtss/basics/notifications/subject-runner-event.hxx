/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_NOTIFICATIONS_SUBJECT_RUNNER_EVENT_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_SUBJECT_RUNNER_EVENT_HXX__

#include <sys/epoll.h>

#include <chrono>
#include <condition_variable>
#include <thread>

#include <vtss/basics/map.hxx>
#include <vtss/basics/mutex.hxx>
#include <vtss/basics/notifications/event-fd.hxx>
#include <vtss/basics/notifications/lock-global-subject.hxx>
#include <vtss/basics/notifications/timer.hxx>
#include <vtss/basics/predefs.hxx>
#include <vtss/basics/time_unit.hxx>

namespace vtss {
namespace notifications {

template <typename T>
T get_relative_time(LinuxClock::time_point now, LinuxClock::time_point abs_time);

template <typename T>
constexpr T get_refresh_rate();

// Subject runner is divided in two classes: SubjectRunnerEvent and
// SubjectRunner.

// SubjectRunnerEvent contains all the data that can be accessed from several
// threads. The global lock is used to insert and remove elements from data
// structures without jeopardizing the data structures. Elements can be taken
// in and out of data structures while a subject runner is processing an
// element, but elements should not be deleted while being processed by the
// subject runner.
struct SubjectRunnerEvent {
    SubjectRunnerEvent();

    // Return true if the timer was sucessfully removed.
    bool timer_del(Timer &t);

    // Add a timer to timer_queue.
    void timer_add(Timer &t, TimeUnit period);

    // Change timeout of a timer. However, if the timer already is
    // being executed, it will have no effect.
    void timer_inc(Timer &t, TimeUnit period);

    // Return true if the event was sucessfully removed.
    bool event_del(Event &t);

    // Add an event to event_queue.
    void event_add(Event &t);

    // Add an event_fd to event_fd_queue.
    mesa_rc event_fd_add(EventFd &e, EventFd::E flags);

    // Delete an event_fd
    mesa_rc event_fd_del(EventFd &e);

    // Get remaining time
    vtss::milliseconds get_remaining(Timer *t) const;

  protected:
    // Returns nullptr if empty
    Timer *timer_pop_msec(LinuxClock::time_point);
    Timer *timer_pop_sec(LinuxClock::time_point);
    TimeUnitMilliseconds timer_timeout();

    // Set a mark in the event queue. This allows the subject runner to iterate
    // through the event_queue without making an infinite loop.
    void event_set_mark();

    // returns nullptr if empty, mark is reached or marked event is deleted
    Event *event_pop();
    bool event_queue_empty();

    // Return pointer to the event-fd associated with fd.
    EventFd *event_fd_pop(int fd);

    // Updates absolute time and all the timeouts in container
    void update_absolute_time(LinuxClock::time_point time_now);

    int fd_cnt = 0;
    bool signaled = false;
    Fd epoll_fd;
    Fd epoll_event_fd;
    void request_service();

    LinuxClock::time_point absolute_time;

  private:
    template <typename TimeUnitT>
    void timer_add(Timer &t, TimeUnitT timeout,
                   intrusive::List<Timer> &container) {
        LockGlobalSubject lock(__FILE__, __LINE__);
        if (t.is_linked() && !timer_del_unprotected(t)) {
            VTSS_ASSERT("Timer event belongs to an alternative subject-runner");
        }

        t.timeout(timeout + get_relative_time<TimeUnitT>(LinuxClock::now(),
                                                         absolute_time));

        container.insert_sorted(t);
        if (&(*container.begin()) == &t) request_service();
    }

    template <typename TimeUnitT>
    void timer_inc(Timer &t, TimeUnitT timeout,
                   intrusive::List<Timer> &container) {
        LockGlobalSubject lock(__FILE__, __LINE__);
        if (t.is_linked() && !timer_del_unprotected(t)) {
            VTSS_ASSERT("Timer event belongs to an alternative subject-runner");
        }

        t.timeout(TimeUnitT(t.timeout().get_value()) + timeout);

        container.insert_sorted(t);
        if (&(*container.begin()) == &t) request_service();
    }

    template <typename TimeUnitT>
    Timer *timer_pop(LinuxClock::time_point now,
                     intrusive::List<Timer> &container) {
        LockGlobalSubject lock(__FILE__, __LINE__);

        if (container.empty()) return nullptr;

        auto t = container.begin();
        if (get_relative_time<TimeUnitT>(now, absolute_time) <
            TimeUnitT(t->timeout().get_value())) {
            return nullptr;
        }

        container.pop_front();
        return &(*t);
    }

    bool timer_del_unprotected(Timer &t);
    bool timer_del_unprotected(Timer &t, intrusive::List<Timer> &container);
    intrusive::List<Timer> timer_queue_msec;
    intrusive::List<Timer> timer_queue_sec;
    intrusive::List<Event> event_queue;
    Event *event_mark;
    Map<int, EventFd *> event_fd_queue;
};

}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_SUBJECT_RUNNER_EVENT_HXX__
