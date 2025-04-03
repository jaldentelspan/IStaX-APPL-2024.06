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

#include "vtss/basics/notifications/subject-runner.hxx"
#include <signal.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "vtss/basics/notifications/event.hxx"
#include "vtss/basics/trace_basics.hxx"

#define TRACE(X) VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_NOTIFICATIONS, X)

namespace vtss {
namespace notifications {

SubjectRunner::SubjectRunner(const char *runner_mutex_name, uint32_t module_id,
                             bool _unlock_on_callback)
    : SubjectRunnerEvent(),
      name(runner_mutex_name),
      unlock_on_callback(_unlock_on_callback),
      runner_mutex(runner_mutex_name, module_id) {
    epoll_fd.assign(epoll_create1(EPOLL_CLOEXEC));
    if (epoll_fd.raw() == -1) {
        TRACE(ERROR) << strerror(errno);
        VTSS_ASSERT("Failed to create epoll instance");
    }

    epoll_event events = {};
    events.data.fd = epoll_event_fd.raw();
    events.events = EPOLLIN;

    if (epoll_ctl(epoll_fd.raw(), EPOLL_CTL_ADD, epoll_event_fd.raw(),
                  &events) != 0) {
        TRACE(ERROR) << strerror(errno);
        VTSS_ASSERT("Failed to add eventfd to epoll");
    }
}

void SubjectRunner::sigchild() {
    signaled = true;
    request_service();
}

int SubjectRunner::epoll_wait(epoll_event *e, int max, int timeout) {
    int res = ::epoll_wait(epoll_fd.raw(), e, max, timeout);

    // We must acknowledge that the eventfd as we are using level based trigger
    uint64_t data;
    // coverity[check_return:SUPPRESS]
    (void)::read(epoll_event_fd.raw(), &data, 8);
    // Dont care about the result, we are just using this for signaling

    return res;
}

int SubjectRunner::sleep(void *epoll_event_, int max) {
    TRACE(NOISE) << "Sleeping";
    int res = epoll_wait((epoll_event *)epoll_event_, max, -1);
    return res;
}

int SubjectRunner::sleep(void *epoll_event_, int max,
                         TimeUnitMilliseconds next_timeout) {
    auto sleep_time = TimeUnitMilliseconds{0};
    auto relative_time = get_relative_time<TimeUnitMilliseconds>(
            LinuxClock::now(), absolute_time);
    if (!(next_timeout < relative_time))
        sleep_time = next_timeout - relative_time;

    TRACE(NOISE) << "Sleping for " << sleep_time.data_;
    int res = epoll_wait((epoll_event *)epoll_event_, max, sleep_time.data_);
    return res;
}

void process_sigchild();

TimeUnitMilliseconds SubjectRunner::run_generic() {
    typename LinuxClock::time_point now = LinuxClock::now();

    ScopeLock<CritdRecursive> runner_lock(&runner_mutex, __FILE__, __LINE__);

    uint32_t trigger_cnt = 0;
    uint32_t timer_trigger_cnt = 0;

    // Ensure that timer_queue also gets attention if a trigger handler
    // keeps re-arming the trigger.
    event_set_mark();
    for (auto t = event_pop(); t != nullptr; t = event_pop()) {
        TRACE(NOISE) << "event-execute " << &(*t);
        {
            if (unlock_on_callback) {
                ScopeUnlock<CritdRecursive> runner_unlock(&runner_mutex,
                                                          __FILE__, __LINE__);
                t->execute();
            } else {
                t->execute();
            }
        }

        trigger_cnt++;
    }


    // first try to update timers in case there is needed a refresh
    update_absolute_time(now);

    // evaluate timer_queues
    for (auto t = timer_pop_msec(now); t != nullptr; t = timer_pop_msec(now)) {
        TRACE(NOISE) << "timer-event-execute " << &(*t);
        if (t->get_repeat()) {
            auto relative_time =
                    get_relative_time<TimeUnitMilliseconds>(now, absolute_time);
            if (TimeUnitMilliseconds(t->timeout().get_value() +
                                     t->get_period().get_value()) <
                relative_time) {
                timer_inc(*t, TimeUnit(relative_time.data_ -
                                               (t->timeout().get_value() -
                                                t->get_period().get_value()),
                                       TimeUnit::Unit::milliseconds));
            } else {
                timer_inc(*t, t->get_period());
            }
        }

        if (unlock_on_callback) {
            ScopeUnlock<CritdRecursive> runner_unlock(&runner_mutex, __FILE__,
                                                      __LINE__);
            t->invoke_cb();
        } else {
            t->invoke_cb();
        }

        timer_trigger_cnt++;
    }

    for (auto t = timer_pop_sec(now); t != nullptr; t = timer_pop_sec(now)) {
        TRACE(NOISE) << "timer-event-execute " << &(*t);
        if (t->get_repeat()) {
            auto relative_time =
                    get_relative_time<TimeUnitSeconds>(now, absolute_time);
            if (TimeUnitSeconds(t->timeout().get_value() +
                                t->get_period().get_value()) < relative_time) {
                timer_inc(*t, TimeUnit(relative_time.data_ -
                                               (t->timeout().get_value() -
                                                t->get_period().get_value()),
                                       TimeUnit::Unit::seconds));
            } else {
                timer_inc(*t, t->get_period());
            }
        }

        if (unlock_on_callback) {
            ScopeUnlock<CritdRecursive> runner_unlock(&runner_mutex, __FILE__,
                                                      __LINE__);
            t->invoke_cb();
        } else {
            t->invoke_cb();
        }

        timer_trigger_cnt++;
    }

    TRACE(NOISE) << "TriggerCnt: " << trigger_cnt
                 << " TimerTriggerCnt: " << timer_trigger_cnt;

    return timer_timeout();
}

void SubjectRunner::run(bool return_when_out_of_work) {
    const int epoll_event_max = 16;
    int epoll_event_cnt = 0;
    epoll_event epoll_events[epoll_event_max];
    sigset_t signal_set;

    return_when_out_of_work_ = return_when_out_of_work;

    while (1) {
        // We do not want to be signaled while invoking the callback handlers
        sigfillset(&signal_set);
        pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
        auto next_timeout = run_generic();

        // Process the epoll events
        TRACE(NOISE) << "epoll_event_cnt: " << epoll_event_cnt;
        for (int i = 0; i < epoll_event_cnt; ++i) {
            ScopeLock<CritdRecursive> runner_lock(&runner_mutex, __FILE__,
                                                  __LINE__);
            auto itr = event_fd_pop(epoll_events[i].data.fd);

            if (itr == nullptr) continue;

            itr->event_flags_ = EventFd::NONE;

            if (epoll_events[i].events & EPOLLIN)
                itr->event_flags_ |= EventFd::READ;

            if (epoll_events[i].events & EPOLLOUT)
                itr->event_flags_ |= EventFd::WRITE;

            if (epoll_events[i].events & EPOLLERR)
                itr->event_flags_ |= EventFd::EXCEPT;

            if (epoll_events[i].events & EPOLLHUP)
                itr->event_flags_ |= EventFd::EXCEPT;

            TRACE(NOISE) << "Invoke callback on fd " << epoll_events[i].data.fd;
            if (unlock_on_callback) {
                ScopeUnlock<CritdRecursive> runner_unlock(&runner_mutex,
                                                          __FILE__, __LINE__);
                itr->invoke_cb();
            } else {
                itr->invoke_cb();
            }
        }
        epoll_event_cnt = 0;

        if (signaled) {
            // We have been signaled, call waitpid()
            signaled = false;
            process_sigchild();
        }

        // If events have been received since we iterated the list, then there
        // is no reason to iterate.
        if (!event_queue_empty()) continue;

        // Open up for signaling again
        pthread_sigmask(SIG_UNBLOCK, &signal_set, NULL);

        if (next_timeout == TimeUnitMilliseconds{0}) {
            if (!fd_cnt) {
                TRACE(NOISE) << "Out of work";
                if (return_when_out_of_work_ && !signaled) {
                    TRACE(INFO) << "Subject runner: " << (void *)this
                                << " returns from run method";
                    return;
                }
            }
            epoll_event_cnt = sleep(epoll_events, epoll_event_max);

        } else {
            epoll_event_cnt = sleep(epoll_events, epoll_event_max, next_timeout);
        }

        if (epoll_event_cnt == -1) {
            switch (errno) {
            case EINTR:
            case EAGAIN:
                TRACE(NOISE) << "Wake up due to interrupt";
                break;
            default:
                TRACE(ERROR) << "Epoll error: " << strerror(errno) << "("
                             << errno << ")";
            }
            epoll_event_cnt = 0;

        } else {
            TRACE(NOISE) << "Wake up - epoll events: " << epoll_event_cnt;
        }
    }

    // Should never return
    VTSS_ASSERT(0);
}

}  // namespace notifications
}  // namespace vtss
