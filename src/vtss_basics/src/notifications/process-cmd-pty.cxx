/*

 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss/basics/trace_basics.hxx"
#include <vtss/basics/notifications/process.hxx>
#include <vtss/basics/notifications/subject-runner.hxx>
#include "vtss/basics/notifications/process-cmd-pty.hxx"

#define TRACE(X) VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_PROCESS, X)

namespace vtss {
namespace notifications {

// Declaring an event handler, which can monitor the process state
struct ProcessCmdPtyHandler : public EventHandler {
    ProcessCmdPtyHandler(SubjectRunner &sr, int fd)
        : EventHandler(&sr),
          p(&sr, "process-cmd-pty"),
          e(this),
          fd_(fd),
          e_fd_(this),
          e_pty_(this) {
    }

    Process p;
    Event e;
    int return_code = -1;

    ~ProcessCmdPtyHandler() {
        unsubscribe();
    }

private:
    void unsubscribe() {
        // Delete both EventFds since none of them are of any good after program
        // exiting or an error occurred on one of them.

        // Remove the Primary PTY event from the subject runner and close the
        // file descriptor afterwards.
        sr->event_fd_del(e_pty_);

        // Also remove the (CLI) event from the subject runner, but do not close
        // the file descriptor afterwards, since we've just borrowed it from the
        // caller of this class.
        // In order to avoid closing it, we must call release() on EventFd, which
        // returns an Fd, which we must also call release() on, or it would be
        // closed when going out of scope. The Fd::release() returns the int
        // representing the file descriptor.
        e_fd_.release().release();
    }

    void copy(EventFd *in, EventFd *out) {
        char b[128];
        int res = ::read(in->raw(), b, 128);
        TRACE(INFO) << "res: " << res << " from fd = " << in->raw();

        if (res > 0) {
            sr->event_fd_add(*in, EventFd::READ);
            write(out->raw(), b, res);
        } else {
            if (res < 0) {
                TRACE(INFO) << "errno = " << errno << " = " << strerror(errno);
            }

            unsubscribe();
        }
    }

    // Callback method for events on one of the file-descriptors
    void execute(EventFd *e) {
        if (e == &e_fd_)
            copy(e, &e_pty_);
        else if (e == &e_pty_)
            copy(e, &e_fd_);
    }

    // Callback method for state change events
    void execute(Event *e_) {
        ProcessStatus st = p.status(e);
        TRACE(INFO) << "Pid: " << st.pid() << " is " << st.state();

        // When the process is running, we can attach the file descriptors.
        // Read from fd_ and write into primary PTY (returned by p.fd_pty_release()).
        // Read from fd_pty_ and write into fd_.

        if (st.state() == ProcessState::running) {
            e_pty_.assign(p.fd_pty_release());
            sr->event_fd_add(e_pty_, EventFd::READ);

            e_fd_.assign(fd_);
            sr->event_fd_add(e_fd_, EventFd::READ);
        }

        if (st.exited()) {
            return_code = st.exit_value();
            sr->return_when_out_of_work();
        }
    }

    int fd_;
    EventFd e_fd_;
    EventFd e_pty_;
};

int process_cmd_pty(int fd, const char *cmd) {
    SubjectRunner sr("process-cmd-pty", VTSS_MODULE_ID_BASICS, true);

    TRACE(DEBUG) << "Opening PTY for fd = " << fd << " with cmd = \"" << cmd << "\"";
    ProcessCmdPtyHandler handler(sr, fd);

    handler.p.pty = true;
    handler.p.executable = "/bin/sh";
    handler.p.arguments.push_back("-c");
    handler.p.arguments.push_back(cmd);
    handler.p.status(handler.e);
    handler.p.environment.emplace("LD_PRELOAD","");
    handler.p.run();
    sr.run();

    return handler.return_code;
}

}  // namespace notifications
}  // namespace vtss
