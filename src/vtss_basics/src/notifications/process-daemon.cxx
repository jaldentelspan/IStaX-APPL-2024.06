/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss/basics/predefs.hxx"
#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/notifications/subject-runner.hxx"
#include "vtss/basics/notifications/process-daemon.hxx"

#define TRACE(X) VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_PROCESS, X)

namespace vtss {
namespace notifications {

ProcessDaemon::ProcessDaemon(SubjectRunner *r)
    : EventHandler(r),
      Process(r),
      mode_event_(this),
      max_restart_cnt_event_(this),
      restart_cnt_event_(this),
      process_status_event_(this),
      e_out(this),
      e_err(this),
      restart_sleep_time_timer_(this),
      reset_restart_cnt_timer_(this) {
    init();
}

ProcessDaemon::ProcessDaemon(SubjectRunner *r, const std::string &name)
    : EventHandler(r),
      Process(r, name),
      mode_event_(this),
      max_restart_cnt_event_(this),
      restart_cnt_event_(this),
      process_status_event_(this),
      e_out(this),
      e_err(this),
      restart_sleep_time_timer_(this),
      reset_restart_cnt_timer_(this) {
    init();
}

ProcessDaemon::ProcessDaemon(SubjectRunner *r, const std::string &name,
                             const std::string &executable)
    : EventHandler(r),
      Process(r, name, executable),
      mode_event_(this),
      max_restart_cnt_event_(this),
      restart_cnt_event_(this),
      process_status_event_(this),
      e_out(this),
      e_err(this),
      restart_sleep_time_timer_(this),
      reset_restart_cnt_timer_(this) {
    init();
}

void ProcessDaemon::init() {
    max_restart_cnt_.set(300);
    restart_cnt_.set(0);
    mode_.set(DISABLE);

    mode_.attach(mode_event_);
    max_restart_cnt_.attach(max_restart_cnt_event_);
    restart_cnt_.attach(restart_cnt_event_);
    (void)status(process_status_event_);
}

#ifndef VTSS_BASICS_STANDALONE
void ProcessDaemon::trace_stdout_conf(int module, int group, int level) {
    trace_stdout_module_ = module;
    trace_stdout_group_ = group;
    trace_stdout_level_ = level;
}
#endif

#ifndef VTSS_BASICS_STANDALONE
void ProcessDaemon::trace_stderr_conf(int module, int group, int level) {
    trace_stderr_module_ = module;
    trace_stderr_group_ = group;
    trace_stderr_level_ = level;
}
#endif

void ProcessDaemon::adminMode(Mode m) {
    if (m == ENABLE) {
        TRACE(DEBUG) << "Process " << name() << " ENABLE";
    } else if (m == DISABLE) {
        TRACE(DEBUG) << "Process " << name() << " DISABLE";
    }

    mode_.set(m);
}

void ProcessDaemon::execute(Event *e) {
    auto m = mode_.get(e, mode_event_);
    auto max_restart = max_restart_cnt_.get(e, max_restart_cnt_event_);
    auto restart = restart_cnt_.get(e, restart_cnt_event_);
    auto st = status(e, process_status_event_);

    pid = st.pid();

    if (!fd_attached && st.alive()) {
        // Attach the EventFd's if configured

        fd_attached = true;
#ifndef VTSS_BASICS_STANDALONE
        if (trace_stdout_module_ != -1)
#endif
        {
            TRACE(DEBUG) << "Process " << name() << " attach to stdout";
            e_out.assign(fd_out_release());
            sr->event_fd_add(e_out, EventFd::READ);
        }

#ifndef VTSS_BASICS_STANDALONE
        if (trace_stderr_module_ != -1)
#endif
        {
            TRACE(DEBUG) << "Process " << name() << " attach to stderr";
            e_err.assign(fd_err_release());
            sr->event_fd_add(e_err, EventFd::READ);
        }
    }

    if (m == ENABLE && st.alive()) {
        TRACE(NOISE) << "Process " << name()
                     << " is running and we want it to be running";

    } else if (m == ENABLE && !st.alive()) {
        TRACE(NOISE) << "Process " << name()
                     << " is not running but we want it to be running";
        // Process is not running, but we want it to be running (unless...)

        // We are of luck...
        if (max_restart != -1 && restart >= max_restart) {
            TRACE(WARNING) << "Process " << name() << " restarts too often";
            return;
        }

        // If the restart_sleep_time_timer_ is pending, then we must wait for it
        // to timeout before we can continue
        if (restart_sleep_time_ != nanoseconds(0) &&
            restart_sleep_time_timer_.is_linked()) {
            TRACE(DEBUG) << "Sleeping before restarting. Process: " << name();
            return;
        }

// Capture the streams if we are connecting them to the tracing
// system
#ifndef VTSS_BASICS_STANDALONE
        Process::fd_out_capture = (trace_stdout_module_ != -1);
        Process::fd_err_capture = (trace_stderr_module_ != -1);
#else
        Process::fd_out_capture = true;
        Process::fd_err_capture = true;
#endif

        // Start the restart_sleep_time_timer_ timer if it is configured
        if (restart_sleep_time_ != nanoseconds(0)) {
            sr->timer_add(restart_sleep_time_timer_, TimeUnitMilliseconds(restart_sleep_time_.raw() / 100000));
        }

        // Start the reset_restart_cnt_timer_ timer if it is configured
        if (reset_restart_cnt_ != nanoseconds(0)) {
            sr->timer_add(reset_restart_cnt_timer_, TimeUnitMilliseconds(reset_restart_cnt_.raw() / 1000000));
        }

        TRACE(DEBUG) << "Starting process: " << name() << " " << restart << "/"
                     << max_restart;
        restart_cnt_.set(restart + 1);
        term_signal_delivered = false;
        fd_attached = false;
        Process::run();

    } else if (m == DISABLE && st.alive()) {
        TRACE(NOISE) << "Process " << name()
                     << " is running but we do not want it to be running";
        // Process is running but we do not want it to be running
        // stop_process();
        if (!term_signal_delivered) {
            TRACE(DEBUG) << "Sending SIGTERM to process: " << name();
            term_signal_delivered = true;
            sr->timer_del(reset_restart_cnt_timer_);
            sr->timer_del(restart_sleep_time_timer_);
            Process::stop(kill_policy_);
        }

    } else if (m == DISABLE && !st.alive()) {
        TRACE(NOISE) << "Process " << name()
                     << " is not running and we do not want it to be running";
    }
}

#ifndef VTSS_BASICS_STANDALONE
static void trace_process(int m, int g, int l, const char *prefix,
                          const char *name, int pid, char *buf, int size,
                          int max_size) {
    // Test if trace is active
    if (m == -1) return;
    if (l < vtss_trace_global_module_lvl_get(m, g)) return;

    while (size && (buf[size - 1] == '\r' || buf[size - 1] == '\n')) size--;

    // Ensure that we have null termination
    if (size >= max_size)
        buf[max_size - 1] = 0;
    else
        buf[size] = 0;

    vtss_trace_printf(m, g, l, __FILE__, __LINE__, "%s-%d %s%s", name, pid,
                      prefix, buf);
}
#endif

void ProcessDaemon::execute(EventFd *e) {
    const int buf_size = 128;
    char buf[buf_size];

    if (e == &e_out) {
        int res = ::read(e_out.raw(), buf, buf_size - 1);

        if (res > 0) {
#ifndef VTSS_BASICS_STANDALONE
            trace_process(trace_stdout_module_, trace_stdout_group_,
                          trace_stdout_level_, "STDOUT> ", name().c_str(), pid,
                          buf, res, buf_size);

#else
            while (res && (buf[res - 1] == '\r' || buf[res - 1] == '\n')) res--;
            TRACE(INFO) << str(buf, buf + res);
#endif
            sr->event_fd_add(e_out, EventFd::READ);

        } else if (res == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
            TRACE(DEBUG) << "Process " << name()
                         << " closing e_out due to error " << strerror(errno);
            sr->event_fd_del(e_out);
            e_out.close();
        }
    }

    if (e == &e_err) {
        int res = ::read(e_err.raw(), buf, buf_size);

        if (res > 0) {
#ifndef VTSS_BASICS_STANDALONE
            trace_process(trace_stderr_module_, trace_stderr_group_,
                          trace_stderr_level_, "STDERR> ", name().c_str(), pid,
                          buf, res, buf_size);

#else
            while (res && (buf[res - 1] == '\r' || buf[res - 1] == '\n')) res--;
            TRACE(INFO) << str(buf, buf + res);
#endif
            sr->event_fd_add(e_err, EventFd::READ);

        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            TRACE(DEBUG) << "Process " << name()
                         << " closing e_err due to error " << strerror(errno);
            sr->event_fd_del(e_err);
            e_err.close();
        }
    }
}

void ProcessDaemon::execute(Timer *e) {
    if (e == &restart_sleep_time_timer_) {
        TRACE(DEBUG) << "restart_sleep_time_timer";
        // Re-evaluate the state
        execute((Event *)nullptr);

    } else if (e == &reset_restart_cnt_timer_) {
        TRACE(DEBUG) << "reset_restart_cnt_timer_";
        auto st = status();
        if (st.alive()) {
            TRACE(DEBUG) << "Reset the restart counter";
            restart_cnt_.set(0);
        }
    }
}

}  // namespace notifications
}  // namespace vtss
