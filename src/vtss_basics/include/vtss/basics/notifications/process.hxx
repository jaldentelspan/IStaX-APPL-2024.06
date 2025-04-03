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

#ifndef __VTSS_BASICS_NOTIFICATIONS_PROCESS_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_PROCESS_HXX__

#include <vtss/basics/fd.hxx>
#include <vtss/basics/map.hxx>
#include <vtss/basics/vector.hxx>
#include <vtss/basics/stream.hxx>
#include <vtss/basics/notifications/subject-read-only.hxx>
#include <vtss/basics/notifications/synchronized-global.hxx>


namespace vtss {
namespace notifications {

// A signal handler used to wake the notification system when one of the child
// process changes its state. The signal handler is automatic configured then
// calling Process::run();
void process_child_monitor(int);

// State of the process.
//
// Here is the state machine implemented by the process.
//
// +-----------+
// |not_running|
// +-----------+
//       |
//       | run()
//       v
//   +-------+   [signals]   +-------+
//   |running|<------------->|stopped|
//   +-------+               +-------+
//      ^  |
// run()|  | [process-has-terminated]
//      |  v
//     +----+
//     |dead|
//     +----+
//
enum class ProcessState { not_running, running, stopped, dead };

ostream &operator<<(ostream &o, ProcessState s);

struct Process;

// A central registry of all the processes being monitored. This is requested by
// the signal handler to find the way back to the process class.
extern SynchronizedGlobal<Vector<Process *>> LIST_OF_PROCESSSES;


// This structure represents the status of the process.
struct ProcessStatus {
    friend struct Process;
    friend bool operator!=(const ProcessStatus &a, const ProcessStatus &b);

    // Get the exit status of the process. This function will only work then the
    // process is in the 'dead' state.
    int exit_value() const;

    // Get the aggregated state of the process.
    ProcessState state() const;

    // Test if the process is in the 'dead' state.
    bool exited() const;

    // Check if the process is alive ('running' or 'stopped')
    bool alive() const;

    // Get the pid of the OS process. If the process is dead then it will return
    // the pid of process before it terminated. If the process has not been
    // running yet then it will return -1
    int pid() const;

  private:
    ProcessState state_ = ProcessState::not_running;
    int pid_ = -1;
    int waitpid_status_ = 0;
};

bool operator!=(const ProcessStatus &a, const ProcessStatus &b);

void process_sigchild();

// This class represents an OS-process and integrates it into the notification
// system such that users can be notified when the state of a process is
// being changed.
struct Process : protected SubjectReadOnly<ProcessStatus> {
    friend void process_sigchild();
    Process() = delete;

    // Constructors
    Process(SubjectRunner *r);
    Process(SubjectRunner *r, const std::string &name);
    Process(SubjectRunner *r, const std::string &name,
            const std::string &executable);
    ~Process();

    // Executable to run. This may either be a absolute path or a file in the
    // PATH.
    std::string executable;

    // Arguments to invoke the executable with.
    Vector<std::string> arguments;

    // Environment for the executable
    Map<std::string, std::string> environment;

    // Indicates if the STDIN file descriptor should be accessible from the
    // parent process or if it should be closed.
    bool fd_in_capture = false;

    // Indicates if the STDOUT file descriptor should be accessible from the
    // parent process or if it should be closed.
    bool fd_out_capture = false;

    // Indicates if the STDERR file descriptor should be accessible from the
    // parent process or if it should be closed.
    bool fd_err_capture = false;

    // If true, using a pseudo-terminal between child and parent.
    // In this case, fd_XXX_capture are not used.
    bool pty = false;

    // If true, the environment passed to the child will only come from
    // #environment. Otherwise, it will inherit the environment from
    // the caller and on top of that get the environment from #environment.
    bool clean_environment = false;

    Fd fd_in_release() { return vtss::move(fd_in_); }
    Fd fd_out_release() { return vtss::move(fd_out_); }
    Fd fd_err_release() { return vtss::move(fd_err_); }
    Fd fd_pty_release() { return vtss::move(fd_pty_); }

    // Get status of the process
    ProcessStatus status() const { return get(); }

    // Get status of the process, and attach an event to receive updates
    ProcessStatus status(Event& t) { return get(t); }
    ProcessStatus status(Event *e, Event& t) { return get(e, t); }

    // Start the OS process. When the process is being started then the
    // 'arguments' and the 'environment' is copied into the process context -
    // these values must therefor be configured before calling 'run' to have any
    // effect (changes performed after the process is started will be effected
    // the next time the process starts).
    //
    // It is only allowed to call 'run()' when the state is 'not_running' or
    // 'dead'.
    mesa_rc run();

    // Stop the current process by sending a SIGTERM/SIGKILL depending on the
    // force argument.
    mesa_rc stop(bool force = false) const;

    // Call stop, and then wait for status
    mesa_rc stop_and_wait(bool force = false);

    // Set nice value for process
    void be_nice(unsigned int nice_value = 0);

    // Send the SIGHUP signal to the process.
    int hup() const;

    std::string name() const { return name_; }

    // The process needs any subject runner to synchronize
    // signals to thread context. The subject runner passed
    // to this function must never be deleted.
    // If this function is not called, it will not be possible
    // to detect when a child dies.
    static void persistent_subject_runner_set(SubjectRunner &r);

  private:
    mesa_rc do_stop(bool force) const;

    // Name of the process. Only used for debugging and/or tracing. The name
    // must not be updated after the class is created as it is assumed that the
    // name can be read from multiple threads.
    std::string name_;

    // Verify that the process is still running - if it is not running then
    // update the status fields.
    void check();

    // Update the status field - 's' is the status returned by 'waitpid'
    void update_status(int s);

    // The nice value to be used
    unsigned int nice_value_ = 0;

    Fd fd_in_;
    Fd fd_out_;
    Fd fd_err_;
    Fd fd_pty_;
};

}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_PROCESS_HXX__
