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
#include <vtss/basics/trace.hxx>
#include <vtss/basics/notifications/process.hxx>
#include <vtss/basics/notifications/subject-runner.hxx>

using namespace vtss;
using namespace vtss::notifications;

// An instance of the subject runner - use 'subject_main_thread' in SMBStaX
vtss::notifications::SubjectRunner main_thread("main", 0, true);

// Instantiate the process - 'test' is just the name used in traces
Process p(&main_thread, "test");

// Declaring an event handler, which can monitor the process state
struct Handler : public EventHandler {
    Handler() : EventHandler(&main_thread), e_(this), out(this) {}

    // Callback method for "normal" events
    void execute(Event *e) {
        auto st = p.status(e_);
        VTSS_TRACE(INFO) << "Pid: " << st.pid() << " is " << st.state();

        // When the process is running, then we can attach the file descriptors
        if (st.state() == ProcessState::running) {
            out.assign(p.fd_out_release());
            sr->event_fd_add(out, EventFd::READ);
        }
    }

    // Callback method for events on one of the file-descriptors
    void execute(EventFd *e) {
        char buf[128];
        int res = read(out.raw(), buf, 128);

        if (res > 0) {
            VTSS_TRACE(INFO) << "OUT: " << str(buf, buf + res);

            // The filedescriptor must be re-armed
            sr->event_fd_add(out, EventFd::READ);
        } else {
            // Close the file descriptor and delete if from the epoll group
            out.close();
            sr->event_fd_del(out);
        }
    }

    Event e_;
    EventFd out;
} handler;

int main(int argc, char *argv[]) {
    // Associate the executable
    p.executable = "/bin/echo";

    // Add arguments - if needed
    p.arguments.push_back("Hello world");

    // Add environment - if needed
    p.environment.emplace("FOO", "bar");

    // Attach the event handler to the process state
    p.status(handler.e_);

    // Ask it to capture the stdout stream
    p.fd_out_capture = true;

    // Run the process - will not block
    p.run();

    // Start the subject runner - not needed for SMBStaX
    main_thread.run(true);

    return 0;
}
