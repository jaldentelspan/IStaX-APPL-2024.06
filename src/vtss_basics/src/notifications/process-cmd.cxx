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

#include "vtss/basics/trace_basics.hxx"
#include <vtss/basics/notifications/process.hxx>
#include <vtss/basics/notifications/subject-runner.hxx>
#include "vtss/basics/notifications/process-cmd.hxx"

#define TRACE(X) VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_PROCESS, X)

namespace vtss {
namespace notifications {

// Declaring an event handler, which can monitor the process state
struct ProcessCmdHandler : public EventHandler {
    ProcessCmdHandler(SubjectRunner &sr, std::string *buf_out_,
                      std::string *buf_err_, size_t max_cap_,
                      size_t max_out_cap_, size_t max_err_cap_)
        : EventHandler(&sr),
          p(&sr, "process-cmd"),
          e_(this),
          e_out(this),
          e_err(this),
          buf_out(buf_out_),
          buf_err(buf_err_),
          max_cap(max_cap_),
          max_out_cap(max_out_cap_),
          max_err_cap(max_err_cap_) {
        if (buf_out) buf_out->clear();
        if (buf_err) buf_err->clear();
    }

    // Callback method for "normal" events
    void execute(Event *e) {
        auto st = p.status(e_);
        //TRACE(INFO) << "Pid: " << st.pid() << " is " << st.state();

        // When the process is running, then we can attach the file descriptors
        if (st.state() == ProcessState::running) {
            if (buf_out) {
                e_out.assign(p.fd_out_release());
                sr->event_fd_add(e_out, EventFd::READ);
            }

            if (buf_err) {
                e_err.assign(p.fd_err_release());
                sr->event_fd_add(e_err, EventFd::READ);
            }
        }

        if (st.exited()) {
            return_code = st.exit_value();
            sr->return_when_out_of_work();
        }
    }

    // Callback method for events on one of the file-descriptors
    void execute(EventFd *e) {
        if (e == &e_out)
            read(e, buf_out, max_out_cap);
        else if (e == &e_err)
            read(e, buf_err, max_err_cap);
    }

    void read(EventFd *e, std::string *buf, size_t m) {
        char b[128];
        int res = ::read(e->raw(), b, 128);
        TRACE(INFO) << "res: " << res;

        if (res > 0) {
            sr->event_fd_add(*e, EventFd::READ);
            if (buf) {
                size_t s = res;
                if (max_cap) {
                    s = min(max_cap - buf->size(), s);
                }
                if (m) {
                    s = min(m - buf->size(), s);
                }
                buf->append(b, s);
            }
        } else {
            // Close the file descriptor and delete if from the epoll group
            e->close();
            sr->event_fd_del(*e);
        }
    }

    Process p;
    int return_code = -1;
    Event e_;
    EventFd e_out;
    EventFd e_err;
    std::string *buf_out;
    std::string *buf_err;
    size_t max_cap, max_out_cap, max_err_cap;
};

int process_cmd(const char *cmd, std::string *out, std::string *err,
                bool exit_when_finished,
                size_t max_cap, size_t max_out_cap, size_t max_err_cap) {
    SubjectRunner sr("process-cmd", VTSS_MODULE_ID_BASICS, true);
    ProcessCmdHandler handler(sr, out, err, max_cap, max_out_cap, max_err_cap);

    handler.p.executable = "/bin/sh";
    handler.p.arguments.push_back("-c");
    handler.p.arguments.push_back(cmd);
    handler.p.status(handler.e_);
    if (out) handler.p.fd_out_capture = true;
    if (err) handler.p.fd_err_capture = true;
    handler.p.run();
    sr.run(exit_when_finished);

    return handler.return_code;
}


}  // namespace notifications
}  // namespace vtss
