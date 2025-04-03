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

#include <signal.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "vtss/basics/notifications/event.hxx"
#include "vtss/basics/notifications/subject-runner.hxx"
#include "vtss/basics/trace_basics.hxx"

#define TRACE(X) VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_NOTIFICATIONS, X)

namespace vtss {
namespace notifications {

template <>
TimeUnitMilliseconds get_relative_time<TimeUnitMilliseconds>(
        LinuxClock::time_point now, LinuxClock::time_point abs_time) {
    return LinuxClock::to_milliseconds(now - abs_time).raw();
}

template <>
TimeUnitSeconds get_relative_time<TimeUnitSeconds>(
        LinuxClock::time_point now, LinuxClock::time_point abs_time) {
    return LinuxClock::to_seconds(now - abs_time).raw();
}

template <>
constexpr TimeUnitSeconds get_refresh_rate() {
    return TimeUnitSeconds{86400};
}

template <>
constexpr TimeUnitMilliseconds get_refresh_rate() {
    return TimeUnitMilliseconds{86400000};
}

SubjectRunnerEvent::SubjectRunnerEvent()
    : event_mark(nullptr) {
    epoll_event_fd.assign(eventfd(0, EPOLL_CLOEXEC | EFD_NONBLOCK));
    absolute_time = LinuxClock::now();
    if (epoll_event_fd.raw() == -1) {
        TRACE(ERROR) << strerror(errno);
        VTSS_ASSERT("Failed to create eventfd instance");
    }
}

bool SubjectRunnerEvent::timer_del_unprotected(Timer &t,
                                               intrusive::List<Timer> &container) {
    typedef typename intrusive::List<Timer>::iterator I;
    for (I i = container.begin(); i != container.end(); ++i) {
        if (&(*i) == &t) {
            container.unlink(t);
            request_service();
            return true;
        }
    }
    return false;
}

bool SubjectRunnerEvent::timer_del_unprotected(Timer &t) {
    if (!t.is_linked()) {
        return true;
    }

    return timer_del_unprotected(t, timer_queue_msec) ||
           timer_del_unprotected(t, timer_queue_sec);
}

void SubjectRunnerEvent::update_absolute_time(LinuxClock::time_point time_now) {
    if (TimeUnitMilliseconds(
                LinuxClock::to_milliseconds(time_now - absolute_time).raw()) >
        get_refresh_rate<TimeUnitMilliseconds>()) {
        absolute_time = time_now;  // Update absolute time
        LockGlobalSubject lock(__FILE__, __LINE__);

        // Update timers relative to the new absolute time
        for (auto &t : timer_queue_msec) {
            if (!(TimeUnitMilliseconds(t.timeout().get_value()) <
                  get_refresh_rate<TimeUnitMilliseconds>()))
                t.timeout(TimeUnitMilliseconds(t.timeout().get_value()) -
                          get_refresh_rate<TimeUnitMilliseconds>());
            else
                t.timeout(TimeUnitMilliseconds(0));
        }

        // Update timers relative to the new absolute times
        for (auto &t : timer_queue_sec) {
            if (!(TimeUnitSeconds(t.timeout().get_value()) <
                  get_refresh_rate<TimeUnitSeconds>()))
                t.timeout(TimeUnitSeconds(t.timeout().get_value()) -
                          get_refresh_rate<TimeUnitSeconds>());
            else
                t.timeout(TimeUnitSeconds(0));
        }
    }
}

bool SubjectRunnerEvent::timer_del(Timer &t) {
    LockGlobalSubject lock(__FILE__, __LINE__);
    return timer_del_unprotected(t);
}

void SubjectRunnerEvent::timer_add(Timer &t, TimeUnit timeout) {
    switch (timeout.get_unit()) {
    case TimeUnit::Unit::seconds:
        timer_add(t, TimeUnitSeconds(timeout.get_value()), timer_queue_sec);
        break;
    case TimeUnit::Unit::milliseconds:
        timer_add(t, TimeUnitMilliseconds(timeout.get_value()), timer_queue_msec);
        break;
    }
}

void SubjectRunnerEvent::timer_inc(Timer &t, TimeUnit timeout) {
    switch (timeout.get_unit()) {
    case TimeUnit::Unit::seconds:
        timer_inc(t, TimeUnitSeconds(timeout.get_value()), timer_queue_sec);
        break;
    case TimeUnit::Unit::milliseconds:
        timer_inc(t, TimeUnitMilliseconds(timeout.get_value()), timer_queue_msec);
        break;
    }
}

// returns nullptr if empty
Timer *SubjectRunnerEvent::timer_pop_msec(LinuxClock::time_point now) {
    return timer_pop<TimeUnitMilliseconds>(now, timer_queue_msec);
}

Timer *SubjectRunnerEvent::timer_pop_sec(LinuxClock::time_point now) {
    return timer_pop<TimeUnitSeconds>(now, timer_queue_sec);
}

TimeUnitMilliseconds SubjectRunnerEvent::timer_timeout() {
    LockGlobalSubject lock(__FILE__, __LINE__);

    if (!timer_queue_msec.empty() && !timer_queue_sec.empty()) {
        return TimeUnitMilliseconds(
                min(timer_queue_msec.begin()->timeout().get_value(),
                    timer_queue_sec.begin()->timeout().get_value() * 1000));
    }
    if (!timer_queue_msec.empty() && timer_queue_sec.empty()) {
        return TimeUnitMilliseconds(
                timer_queue_msec.begin()->timeout().get_value());
    }
    if (timer_queue_msec.empty() && !timer_queue_sec.empty()) {
        return TimeUnitMilliseconds(
                timer_queue_sec.begin()->timeout().get_value() * 1000);
    }

    return TimeUnitMilliseconds{0};
}

bool SubjectRunnerEvent::event_del(Event &t) {
    LockGlobalSubject lock(__FILE__, __LINE__);

    typedef typename intrusive::List<Event>::iterator I;
    for (I i = event_queue.begin(); i != event_queue.end(); ++i) {
        if (&(*i) == &t) {
            if (&t == event_mark) {
                event_mark = nullptr;
            }
            event_queue.unlink(t);
            request_service();
            return true;
        }
    }

    return false;
}

void SubjectRunnerEvent::event_add(Event &t) {
    LockGlobalSubject lock(__FILE__, __LINE__);

    VTSS_ASSERT(!t.is_linked());
    TRACE(NOISE) << "Event add: " << &t;
    event_queue.push_back(t);
    request_service();
}

void SubjectRunnerEvent::event_set_mark() {
    LockGlobalSubject lock(__FILE__, __LINE__);

    if (!event_queue.empty()) {
        auto last = event_queue.end();
        --last;
        event_mark = &(*last);
    } else {
        event_mark = nullptr;
    }
}

// returns nullptr if empty or mark is reached
Event *SubjectRunnerEvent::event_pop() {
    LockGlobalSubject lock(__FILE__, __LINE__);

    if (event_queue.empty() || event_mark == nullptr) return nullptr;

    auto t = event_queue.begin();
    if (&(*t) == event_mark) {
        event_mark = nullptr;
    }

    event_queue.pop_front();

    return &(*t);
}

bool SubjectRunnerEvent::event_queue_empty() {
    LockGlobalSubject lock(__FILE__, __LINE__);

    return event_queue.empty();
}

mesa_rc SubjectRunnerEvent::event_fd_add(EventFd &efd, EventFd::E flags) {
    LockGlobalSubject lock(__FILE__, __LINE__);

    int res = 0;
    bool rearm = false;
    epoll_event e = {};
    e.events = EPOLLONESHOT;
    e.data.fd = efd.fd_.raw();
    if (flags & EventFd::READ) e.events |= EPOLLIN;
    if (flags & EventFd::WRITE) e.events |= EPOLLOUT;

    // The file-descriptor is already part of this epoll group, it just needs to
    // be re-activated.
    if (efd.sr_ == this) {
        TRACE(NOISE) << "Fd: " << efd.fd_.raw() << " rearm";
        res = epoll_ctl(epoll_fd.raw(), EPOLL_CTL_MOD, efd.fd_.raw(), &e);
        rearm = true;

        if (res == -1) {
            TRACE(ERROR) << "epoll_ctl error: " << strerror(errno) << "("
                         << errno << ")";
            return MESA_RC_ERROR;
        }
    }

    // The file-descriptor is part of another epoll group - it must be
    // removed from that group at first.
    if (efd.sr_ != this && efd.sr_ != nullptr) {
        efd.sr_->event_fd_del(efd);
    }

    // The file-descriptor is not part of any other epoll instance - add it
    // to
    // this subject-runner (it might just have been deleted).
    if (efd.sr_ == nullptr) {
        TRACE(NOISE) << "Fd: " << efd.fd_.raw() << " add";
        res = epoll_ctl(epoll_fd.raw(), EPOLL_CTL_ADD, efd.fd_.raw(), &e);

        if (res == -1) {
            TRACE(INFO) << "epoll_ctl error: " << strerror(errno) << "("
                        << errno << ")";
            return MESA_RC_ERROR;
        }

        // Register the event in the map - back-out on failure
        auto r = event_fd_queue.emplace(efd.fd_.raw(), &efd);
        if (!r.second) {
            TRACE(INFO) << "Failed to insert in map";
            res = epoll_ctl(epoll_fd.raw(), EPOLL_CTL_DEL, efd.fd_.raw(),
                            nullptr);

            if (res == -1) {
                TRACE(ERROR) << "clean up failed: epoll_ctl: " << strerror(errno)
                             << "(" << errno << ")";
            }

            return MESA_RC_ERROR;

        } else {
            efd.sr_ = this;
        }
    }

    if (!rearm) fd_cnt++;
    return MESA_RC_OK;
}

mesa_rc SubjectRunnerEvent::event_fd_del(EventFd &efd) {
    // Not registered
    if (efd.sr_ == nullptr) {
        return true;
    }

    if (efd.sr_ == this) {
        LockGlobalSubject lock(__FILE__, __LINE__);

        bool found = false;
        for (auto i = event_fd_queue.begin(); i != event_fd_queue.end();) {
            if (i->second == &efd) {
                event_fd_queue.erase(i++);
                found = true;
                break;
            } else {
                ++i;
            }
        }

        // Try to delete it regardless of if it is found or not.
        TRACE(NOISE) << "Fd: " << efd.fd_.raw() << " del";
        int res =
                epoll_ctl(epoll_fd.raw(), EPOLL_CTL_DEL, efd.fd_.raw(), nullptr);

        if (res == -1)
            TRACE(DEBUG) << "epoll_ctl: " << strerror(errno) << "(" << errno
                         << ")";

        // No need to come back to this subject runner
        efd.sr_ = nullptr;

        if (found) {
            fd_cnt--;
            return MESA_RC_OK;
        } else {
            TRACE(ERROR) << "Registered here, but not found here...";
            return MESA_RC_ERROR;
        }

    } else {
        // Belongs to a different subject runner.
        return efd.sr_->event_fd_del(efd);
    }

    // Not reachable
    return MESA_RC_ERROR;
}

EventFd *SubjectRunnerEvent::event_fd_pop(int fd) {
    LockGlobalSubject lock(__FILE__, __LINE__);
    auto itr = event_fd_queue.find(fd);

    if (itr == event_fd_queue.end()) {
        TRACE(NOISE) << "EventFd skipped! " << fd;
        return nullptr;
    }

    return itr->second;
}

vtss::milliseconds SubjectRunnerEvent::get_remaining(Timer *t) const {
    vtss::milliseconds timeout = vtss::milliseconds{t->timeout().get_value()};
    return timeout +
           vtss::milliseconds(LinuxClock::time_point_to_raw(absolute_time)) -
           vtss::milliseconds(LinuxClock::time_point_to_raw(LinuxClock::now()));
}

void SubjectRunnerEvent::request_service() {
    // TRACE-NOT-ALLOWED!!!
    //
    // We can enter this method in a signaling context, meaning that all
    // "normal" invariants on mutexes will not hold.
    uint64_t d = 1;
    write(epoll_event_fd.raw(), &d, 8);
}

}  // namespace notifications
}  // namespace vtss
