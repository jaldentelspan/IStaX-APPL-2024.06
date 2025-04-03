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

#ifndef __VTSS_BASICS_NOTIFICATIONS_SUBJECT_RUNNER_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_SUBJECT_RUNNER_HXX__

#include <sys/epoll.h>

#include <thread>
#include <chrono>
#include <condition_variable>

#include <vtss/basics/map.hxx>
#include <vtss/basics/mutex.hxx>
#include <vtss/basics/predefs.hxx>
#include <vtss/basics/notifications/timer.hxx>
#include <vtss/basics/notifications/event-fd.hxx>
#include <vtss/basics/notifications/lock-global-subject.hxx>
#include <vtss/basics/notifications/subject-runner-event.hxx>
#include <vtss/basics/time_unit.hxx>

namespace vtss {
namespace notifications {

// Subject runner consists of two classes: SubjectRunnerEvent and SubjectRunner.

// SubjectRunner contains the data that only is acessed from the subject runner
// thread itself. SubjectRunner is using the mutex "runner_mutex" to guard
// execution of elements.
struct SubjectRunner : SubjectRunnerEvent {
    typedef LinuxClock clock_t;

    const char *name;
    uint32_t prio;  // Not used by the framework
    explicit SubjectRunner(const char *runner_mutex_name, uint32_t module_id,
                           bool _unlock_on_callback);

    SubjectRunner(const SubjectRunner&) = delete;  // No copies

    void lock(const char *f, unsigned l) { runner_mutex.lock(f, l); }
    void unlock(const char *f, unsigned l) { runner_mutex.unlock(f, l); }

    void run(bool return_when_out_of_work = false);
    void sigchild();

    void return_when_out_of_work() { return_when_out_of_work_ = true; }

  private:
    bool unlock_on_callback = false;
    bool return_when_out_of_work_ = false;
    CritdRecursive runner_mutex;

    TimeUnitMilliseconds run_generic();
    int sleep(void *epoll_event, int max);
    int sleep(void *epoll_event, int max, TimeUnitMilliseconds time);
    int epoll_wait(epoll_event *e, int max, int timeout);
};

}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_SUBJECT_RUNNER_HXX__
