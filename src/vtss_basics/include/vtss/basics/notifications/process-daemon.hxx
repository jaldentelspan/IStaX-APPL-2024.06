/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_NOTIFICATIONS_PROCESS_DAEMON_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_PROCESS_DAEMON_HXX__

#include "vtss/basics/predefs.hxx"
#include "vtss/basics/notifications/timer.hxx"
#include "vtss/basics/notifications/subject.hxx"
#include "vtss/basics/notifications/process.hxx"
#include "vtss/basics/notifications/event-fd.hxx"

namespace vtss {
namespace notifications {

// Represents a daemon-process (a long-lived process that must be kept alive).
// It build upon the Process class but add facilities to keep the process
// running and to connect the stdout and stderr pipes to a configurable trace
// group.
struct ProcessDaemon : public EventHandler, protected Process {
    enum Mode { ENABLE, DISABLE };

    // Constructors/destructor
    ProcessDaemon(SubjectRunner *r);
    ProcessDaemon(SubjectRunner *r, const std::string &name);
    ProcessDaemon(SubjectRunner *r, const std::string &name,
                  const std::string &executable);

    // Providing access to selected part of the protected base class Process.
    // These members are documented in the process.hxx file.
    using Process::executable;
    using Process::arguments;
    using Process::environment;
    ProcessStatus status() { return Process::status(); }
    ProcessStatus status(Event &t) { return Process::status(t); }
    ProcessStatus status(Event *e, Event &t) { return Process::status(e, t); }
    int hup() const { return Process::hup(); }
    mesa_rc stop(bool force = false) const { return Process::stop(force || kill_policy_); }
    mesa_rc stop_and_wait(bool force = false) {
        mesa_rc rc = Process::stop_and_wait(force || kill_policy_);
        if (rc != MESA_RC_OK) return rc;
        return MESA_RC_OK;
    }
    void be_nice(unsigned int nice_value = 0) { Process::be_nice(nice_value); }

    std::string name() const { return Process::name(); }

    // Get the current mode of operation. Note that this is the desired
    // configured mode, the actual state can bee access by using 'status()'
    Mode adminMode() { return mode_.get(); };
    Mode adminMode(Event &e) { return mode_.get(e); }

    // Set the desired mode. The initial adminMode will be DISABLE, and the
    // process will not be running until this is updated to ENABLE. When the
    // adminMode is configured to ENABLE then the process will be restarted if
    // it dies.
    void adminMode(Mode m);

#ifndef VTSS_BASICS_STANDALONE
    // Configure how/if the stdout/stderr pipes should be connected to the
    // tracing system. To disable connecting the pipes to the tracing system set
    // module to -1 (this is default).
    void trace_stdout_conf(int module, int group, int level);
    void trace_stderr_conf(int module, int group, int level);
#endif

    // Get/set a maximum of restarts. If the application crashes more than 'c'
    // times then it will not be restarted.
    //
    // If configured to -1 then there will be no limit.
    //
    // The default value is 300.
    void max_restart_cnt(int c) { max_restart_cnt_.set(c); }
    int max_restart_cnt() { return max_restart_cnt_.get(); }
    int max_restart_cnt(Event &e) { return max_restart_cnt_.get(e); }

    // Get the current restart count. This value will be reset to zero if
    // calling 'reset_restart_cnt()'
    int restart_cnt() { return restart_cnt_.get(); }
    int restart_cnt(Event &e) { return restart_cnt_.get(e); }

    // Determine whether to kill process using SIGTERM or SIGKILL.
    // Default is to use SIGTERM. Setting kill_policy to true will cause
    // SIGKILL to be used
    void kill_policy(bool policy) { kill_policy_ = policy; }
    bool kill_policy() const { return kill_policy_; }
    // Do not restart the application sooner than 't' time after last start.
    // If 't' is configured to 5 seconds, and the application keep crashing
    // after 1 second - then it will wait additional 4 seconds before
    // restarting. If 't' is configured to 5 seconds, and the application runs
    // for 10 seconds, then it will be restarted imitatively after it crashes.
    //
    // The default value is 250ms.
    void restart_sleep_time(nanoseconds t) { restart_sleep_time_ = t; }

    // Manually reset the restart counter.
    void reset_restart_cnt() { restart_cnt_.set(0); }

    // Automatic reset the restart counter if the application runs without
    // crashing for 't' time.
    //
    // The default value is 10s.
    void reset_restart_cnt(nanoseconds t) { reset_restart_cnt_ = t; }

    // Used by the notification system - must not be called manually
    void execute(Event *e);
    void execute(EventFd *e);
    void execute(Timer *e);

  private:
    void init();

#ifndef VTSS_BASICS_STANDALONE
    int trace_stdout_module_ = -1;
    int trace_stderr_module_ = -1;
    int trace_stdout_group_ = -1;
    int trace_stderr_group_ = -1;
    int trace_stdout_level_ = -1;
    int trace_stderr_level_ = -1;
#endif

    Subject<Mode> mode_;
    Subject<int> max_restart_cnt_;
    Subject<int> restart_cnt_;

    int pid = -1;
    bool term_signal_delivered = false;
    bool fd_attached = false;
    bool kill_policy_ = false;

    Event mode_event_;
    Event max_restart_cnt_event_;
    Event restart_cnt_event_;
    Event process_status_event_;
    EventFd e_out;
    EventFd e_err;

    Timer restart_sleep_time_timer_;
    Timer reset_restart_cnt_timer_;

    nanoseconds restart_sleep_time_ = milliseconds(250);
    nanoseconds reset_restart_cnt_ = seconds(10);
};

}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_PROCESS_DAEMON_HXX__
