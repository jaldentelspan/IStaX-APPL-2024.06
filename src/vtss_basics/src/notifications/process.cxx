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

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/notifications/process.hxx"
#include "vtss/basics/notifications/subject-runner.hxx"
#include "vtss/basics/synchronized.hxx"

extern char **environ;

#define TRACE(X) VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_PROCESS, X)

namespace vtss {
namespace notifications {

SynchronizedGlobal<Vector<Process *>> __attribute__((init_priority(400)))
LIST_OF_PROCESSSES;


ostream &operator<<(ostream &o, ProcessState s) {
    switch (s) {
    case ProcessState::not_running:
        o << "not_running";
        break;
    case ProcessState::running:
        o << "running";
        break;
    case ProcessState::stopped:
        o << "stopped";
        break;
    case ProcessState::dead:
        o << "dead";
        break;
    default:
        o << "<UNKNOWN:" << (int)s << ">";
        break;
    }

    return o;
}

// The process needs any subject runner to synchronize
// signals to thread context. The subject runner must
// be long-lived and is set by Process::persistent_subject_runner_set.
static SubjectRunner *persistent_subject_runner;

void process_child_monitor(int sig) {
    // printf("I %d/%s#%d Got signal %d\n", syscall(SYS_gettid), __FILE__, __LINE__, sig);

    // Signals are annoying, messy and complicated!!!
    //
    // The code can be signaled in any possible context, and when this happens
    // it will work as a context switch - but the scheduler will not switch back
    // to the original context before the signal handler returns.
    //
    // The code called in this context is therefore not allowed to take _any_
    // mutex!
    if (persistent_subject_runner) {
        persistent_subject_runner->sigchild();
    } else {
        // Cannot use trace here, because it uses mutexes.
        printf("E %d/%s#%d Error: No persistent subject runner set\n", (int)syscall(SYS_gettid), __FILE__, __LINE__);
    }
}


void process_sigchild() {
    while (1) {
        int s;
        pid_t p;

        {
            // Must be a critical section to avoid be able to to a blocking
            // "stop" in the process class.
            notifications::LockGlobalSubject lock__(__FILE__, __LINE__);
            p = ::waitpid(-1, &s, WNOHANG | WUNTRACED | WCONTINUED);
        }

        if (p == 0 || p == -1) return;

        TRACE(DEBUG) << "waitpid -> " << p;

        if (WIFEXITED(s))
            TRACE(INFO) << "PID: " << p
                        << " exited with return value: " << WEXITSTATUS(s);

        if (WIFSIGNALED(s))
            TRACE(INFO) << "PID: " << p << " killed by signal "
                        << strsignal(WTERMSIG(s)) << "(" << WTERMSIG(s) << ")";

        if (WIFSTOPPED(s))
            TRACE(INFO) << "PID: " << p
                        << " stopped by signal: " << strsignal(WSTOPSIG(s))
                        << "(" << WSTOPSIG(s) << ")";

        if (WIFCONTINUED(s)) TRACE(INFO) << "PID: " << p << " continued";

        SYNCHRONIZED(LIST_OF_PROCESSSES) {
            for (auto i : LIST_OF_PROCESSSES) {
                auto st = i->get();
                if (st.pid() == p) {
                    TRACE(DEBUG) << "Update status: Process-name: " << i->name()
                                 << " PID: " << p << " status: " << s;
                    i->update_status(s);
                }
            }
        }
    }
}

int ProcessStatus::exit_value() const {
    if (state_ != ProcessState::dead) return 0;
    return WEXITSTATUS(waitpid_status_);
}

int ProcessStatus::pid() const { return pid_; }
ProcessState ProcessStatus::state() const { return state_; }
bool ProcessStatus::exited() const { return state_ == ProcessState::dead; }

bool ProcessStatus::alive() const {
    return state_ == ProcessState::running || state_ == ProcessState::stopped;
}

bool operator!=(const ProcessStatus &a, const ProcessStatus &b) {
    if (a.state_ != b.state_) return true;
    if (a.pid_ != b.pid_) return true;
    if (a.waitpid_status_ != b.waitpid_status_) return true;

    return false;
}

Process::Process(SubjectRunner *r) {
    // Fall back to using this subject runner to handle signals if no persistent
    // subject runner is set.
    if (!persistent_subject_runner) persistent_subject_runner = r;
    SYNCHRONIZED(LIST_OF_PROCESSSES) { LIST_OF_PROCESSSES.push_back(this); }
}

Process::Process(SubjectRunner *r, const std::string &n) : name_(n) {
    if (!persistent_subject_runner) persistent_subject_runner = r;
    SYNCHRONIZED(LIST_OF_PROCESSSES) { LIST_OF_PROCESSSES.push_back(this); }
}

Process::Process(SubjectRunner *r, const std::string &n, const std::string &e)
    : executable(e), name_(n) {
    if (!persistent_subject_runner) persistent_subject_runner = r;
    SYNCHRONIZED(LIST_OF_PROCESSSES) { LIST_OF_PROCESSSES.push_back(this); }
}

static bool is_executable_file(const char *n) {
    struct stat st;

    if (access(n, X_OK) != 0) return false;
    if (stat(n, &st) != 0) return false;
    if (!S_ISREG(st.st_mode)) return false;

    return true;
}

static const char *path_[] = {"/usr/local/sbin", "/usr/local/bin", "/usr/sbin",
                              "/usr/bin", "/sbin", "/bin", nullptr};

mesa_rc Process::run() {
    std::string exec_;
    notifications::LockGlobalSubject lock__(__FILE__, __LINE__);

    pid_t p;
    int pipe_stdin[2] = {-1, -1};
    int pipe_stdout[2] = {-1, -1};
    int pipe_stderr[2] = {-1, -1};
    int fd_pty_secondary = -1, fd_pty_primary = -1;
    int nice_value = nice_value_;
    bool pty_ = pty;
    auto environment_(environment);

    TRACE(DEBUG) << "Process: " << name_ << " run(pty = " << pty << ")";

    if (!executable.size()) {
        TRACE(ERROR) << "Process: " << name_ << " no executable!";
        return MESA_RC_ERROR;
    }

    if (executable[0] == '/') {
        if (!is_executable_file(executable.c_str())) {
            TRACE(ERROR) << "Process: " << name_
                         << " file: " << executable.c_str()
                         << " is not an executable";
            return MESA_RC_ERROR;
        } else {
            exec_ = executable;
        }
    } else {
        bool got_one = false;
        for (const char **p = path_; *p; p++) {
            exec_ = *p;
            exec_.append("/");
            exec_.append(executable);

            if (is_executable_file(exec_.c_str())) {
                TRACE(INFO) << "Process: " << name_
                            << " using: " << exec_.c_str();
                got_one = true;
                break;
            }
        }

        if (!got_one) {
            TRACE(ERROR) << "Process: " << name_
                         << " file: " << executable.c_str()
                         << " not found in path";
            return MESA_RC_ERROR;
        }
    }

    auto st = get();
    if (st.state() == ProcessState::running ||
        st.state() == ProcessState::stopped) {
        TRACE(INFO) << "Process: " << name_ << " Refuse to run()";
        return MESA_RC_ERROR;
    }

    if (pty_) {
        if ((fd_pty_primary = posix_openpt(O_RDWR)) < 0)                    goto ERROR;
        if (grantpt(fd_pty_primary) != 0)                                   goto ERROR;
        if (unlockpt(fd_pty_primary) != 0)                                  goto ERROR;
        if ((fd_pty_secondary = open(ptsname(fd_pty_primary), O_RDWR)) < 0) goto ERROR;
    } else {
        // Close potential 'old' file descriptors
        fd_in_.close();
        fd_out_.close();
        fd_err_.close();

        // Create pipes if needed
        if (fd_in_capture && pipe(pipe_stdin) == -1)   goto ERROR;
        if (fd_out_capture && pipe(pipe_stdout) == -1) goto ERROR;
        if (fd_err_capture && pipe(pipe_stderr) == -1) goto ERROR;
    }

    // The child inherits the parents' environment unless asked not to.
    if (environ && !clean_environment) {
        int i = 0;

        while (environ[i]) {
            // Split it by "=", if "=" exists.
            char *e = environ[i];
            char *v = strchr(e, '=');
            int count = v ? v - e : -1;
            std::string e_as_str(e);
            std::string key(v ? e_as_str.substr(0, count) : e);
            std::string val(v ? e_as_str.substr(count + 1) : "");

            auto itr = environment.find(key);
            if (itr == environment.end()) {
                // It's not in the user's requested environment.
                // Add it. Notice that we check his original, but
                // modify the local version.
                environment_.set(key, val);
            }

            i++;
        }
    }

    // Set-up signal handler that gets invoked when child exits.
    ::signal(SIGCHLD, process_child_monitor);

    p = vfork();  // Notice the use of 'vfork' instead of 'fork' - be aware!!!

    if (p == -1) {
        TRACE(ERROR) << "Process: " << name_ << " Fork failed!";
        goto ERROR;
    }

    if (p == 0) {
        // Child code //////////////////////////////////////////////////////////
        int fd_max;
        char **argv = nullptr;
        char **envv = nullptr;
        char **envi = nullptr;

        // Setup default signals and unblock them
        for (int i = 1; i < 32; ++i) ::signal(i, SIG_DFL);
        ::sigset_t set;
        ::sigfillset(&set);
        ::sigprocmask(SIG_UNBLOCK, &set, NULL);

        // The child process must be its own group leader
        if (::setsid() == -1) {
            perror("setsid: ");
        }

        if (pty_) {
            // Setup file descriptors for stdin/stdout/stderr
            if (fd_pty_secondary != STDIN_FILENO) {
                if (dup2(fd_pty_secondary, STDIN_FILENO) == -1) {
                    perror("Error PTY dup2(stdin): ");
                    goto CHILD_ERROR;
                }
            }

            if (fd_pty_secondary != STDOUT_FILENO) {
                if (dup2(fd_pty_secondary, STDOUT_FILENO) == -1) {
                    perror("Error PTY dup2(stdout): ");
                    goto CHILD_ERROR;
                }
            }

            if (fd_pty_secondary != STDERR_FILENO) {
                if (dup2(fd_pty_secondary, STDERR_FILENO) == -1) {
                    perror("Error PTY dup2(stderr): ");
                    goto CHILD_ERROR;
                }
            }

            // Set controlling terminal to be the secondary side of the PTY
            if (ioctl(0, TIOCSCTTY, 1) < 0) {
                perror("Ioctl(TIOCSCTTY) failed: ");
                goto CHILD_ERROR;
            }
        } else {
            // Setup file descriptors for stdin/stdout/stderr if requested
            if (fd_in_capture) {
                if (dup2(pipe_stdin[0], STDIN_FILENO) == -1) {
                    perror("Error dup2(stdin): ");
                    goto CHILD_ERROR;
                }
            }

            if (fd_out_capture) {
                if (dup2(pipe_stdout[1], STDOUT_FILENO) == -1) {
                    perror("Error dup2(stdout): ");
                    goto CHILD_ERROR;
                }
            }

            if (fd_err_capture) {
                if (dup2(pipe_stderr[1], STDERR_FILENO) == -1) {
                    perror("Error dup2(stderr): ");
                    goto CHILD_ERROR;
                }
            }
        }

        // Close all non-needed file descriptors
        fd_max = sysconf(_SC_OPEN_MAX);
        if (fd_max == -1) fd_max = 1024;
        for (int i = 0; i < fd_max; ++i) {
            // Close all file handles, except for stdin, stdout, and stderr if captured.
            if ((pty_ || fd_in_capture)  && i == STDIN_FILENO)  continue;
            if ((pty_ || fd_out_capture) && i == STDOUT_FILENO) continue;
            if ((pty_ || fd_err_capture) && i == STDERR_FILENO) continue;

            close(i);
        }

        // Create argv array - Adding space for name of executable and null
        // terminator at the end.
        argv = (char **)calloc((arguments.size() + 2), sizeof(char *));
        if (argv == 0) goto CHILD_ERROR;
        argv[0] = strndup(executable.c_str(), executable.size());
        for (size_t i = 0; i < arguments.size(); ++i) {
            argv[i + 1] = strndup(arguments[i].c_str(), arguments[i].size());
            if (!argv[i + 1]) goto CHILD_ERROR;
        }

        // Create envv array - adding space for null termination at the end
        envv = (char **)calloc((environment_.size() + 2), sizeof(char *));
        if (envv == 0) goto CHILD_ERROR;
        envi = envv;
        for (const auto &e : environment_) {
            // make room for: "<key>=<val>\0";
            *envi = (char *)malloc(e.first.size() + e.second.size() + 2);
            if (!(*envi)) goto CHILD_ERROR;

            char *ep = *envi++;
            memcpy(ep, e.first.c_str(), e.first.size());
            ep += e.first.size();

            *ep++ = '=';

            memcpy(ep, e.second.c_str(), e.second.size());
            ep += e.second.size();

            *ep++ = '\0';
        }

        nice(nice_value);

        // Replace the forked application with the desired application
        (void)execve(exec_.c_str(), argv, envv);

        // If execve is failing then try to give some hitns on what went wrong
        perror("execve failed: ");
        fprintf(stderr, "file: %s\n", exec_.c_str());
        fprintf(stderr, "Argv: %d\n", (int)arguments.size() + 1);
        for (int i = 0; i < (int)arguments.size() + 1; ++i) {
            fprintf(stderr, "  [%d] = %s\n", i, argv[i]);
        }

        fprintf(stderr, "\n");

    CHILD_ERROR:
        fprintf(stderr, "ERROR\n");
        // Release allocated resources
        if (pty_) {
            close(fd_pty_secondary);
        } else {
            close(pipe_stdin[1]);
            close(pipe_stdout[0]);
            close(pipe_stderr[0]);
        }

        if (argv) {
            for (size_t i = 0; i < arguments.size(); ++i)
                if (argv[i]) free(argv[i]);
            free(argv);
        }

        if (envv) {
            for (size_t i = 0; i < environment_.size(); ++i)
                if (envv[i]) free(envv[i]);
            free(envv);
        }

        // Notice using "_exit" and not "exit" because we are using "vfork"
        // instead of "fork".
        _exit(EXIT_FAILURE);

        // Child code end here
        // /////////////////////////////////////////////////

    } else {
        // Parent code
        // Close the 'non-used' end of the pipes.
        if (pty_) {
            close(fd_pty_secondary);  // Parent must close secondary side of the PTY
            fd_pty_.assign(fd_pty_primary);
        } else {
            if (fd_in_capture) {
                close(pipe_stdin[0]);  // close read end of pipe
                fd_in_.assign(pipe_stdin[1]);
            }

            if (fd_out_capture) {
                close(pipe_stdout[1]);  // close write end of pipe
                fd_out_.assign(pipe_stdout[0]);
            }

            if (fd_err_capture) {
                close(pipe_stderr[1]);  // close write end of pipe
                fd_err_.assign(pipe_stderr[0]);
            }
        }

        // Update the process state (it might be dead already).
        TRACE(INFO) << "Process: " << name_ << " is now running as pid: " << p;
        ProcessStatus s;
        s.pid_ = p;
        s.state_ = ProcessState::running;
        s.waitpid_status_ = 0;
        set(s);

        return MESA_RC_OK;
    }

ERROR:
    TRACE(ERROR) << "Process: " << name_ << " run() " << strerror(errno) << "("
                 << errno << ")";

    if (pty_) {
        if (fd_pty_primary >= 0) {
            close(fd_pty_primary);
        }
        if (fd_pty_secondary >= 0) {
            close(fd_pty_secondary);
        }
    } else {
        // Try to clean up as much as possible
        close(pipe_stdin[0]);
        close(pipe_stdin[1]);
        close(pipe_stdout[0]);
        close(pipe_stdout[1]);
        close(pipe_stderr[0]);
        close(pipe_stderr[1]);
    }

    return MESA_RC_ERROR;
}

mesa_rc Process::do_stop(bool force) const {
    notifications::LockGlobalSubject lock__(__FILE__, __LINE__);

    auto st = get();
    if (st.state() == ProcessState::not_running ||
        st.state() == ProcessState::dead) {
        TRACE(INFO) << "Process: " << name_ << " Refuse to stop()";
        return MESA_RC_ERROR;
    }

    if (force) {
        if (::kill(st.pid(), SIGKILL) != 0) return MESA_RC_ERROR;
    } else {
        if (::kill(st.pid(), SIGTERM) != 0) return MESA_RC_ERROR;
    }

    return MESA_RC_OK;
}

mesa_rc Process::stop(bool force) const {
    notifications::LockGlobalSubject lock__(__FILE__, __LINE__);

    return do_stop(force);
}

mesa_rc Process::stop_and_wait(bool force) {
    notifications::LockGlobalSubject lock__(__FILE__, __LINE__);

    // Waitpid will wait for ever if process is not running
    mesa_rc rc = do_stop(force);
    if (rc != MESA_RC_OK) {
        return rc;
    }

    auto st = get();
    int s;
    int p = ::waitpid(st.pid(), &s, 0);
    if (p == st.pid()) update_status(s);

    return MESA_RC_OK;
}

void Process::be_nice(unsigned int nice_value) {
    nice_value_ = nice_value;
}

int Process::hup() const {
    notifications::LockGlobalSubject lock__(__FILE__, __LINE__);

    auto st = get();
    if (st.state() == ProcessState::not_running ||
        st.state() == ProcessState::dead) {
        TRACE(INFO) << "Process: " << name_ << " Refuse to hup()";
        return MESA_RC_ERROR;
    }

    if (::kill(st.pid(), SIGHUP) != 0) return MESA_RC_ERROR;

    return MESA_RC_OK;
}

Process::~Process() {
    SYNCHRONIZED(LIST_OF_PROCESSSES) {
        auto i = LIST_OF_PROCESSSES.begin();
        auto e = LIST_OF_PROCESSSES.end();

        while (i != e) {
            if (*i == this) {
                LIST_OF_PROCESSSES.erase(i);
                break;
            } else {
                ++i;
            }
        }
    }
}

void Process::update_status(int s) {
    notifications::LockGlobalSubject lock__(__FILE__, __LINE__);

    auto st = get();

    TRACE(DEBUG) << "Process: " << name_
                 << " waitpid status: " << st.waitpid_status_ << " -> " << s;
    st.waitpid_status_ = s;

    if (WIFEXITED(s)) {
        TRACE(INFO) << "Process: " << name_ << " state: " << st.state_ << " -> "
                    << ProcessState::dead;
        st.state_ = ProcessState::dead;

    } else if (WIFSIGNALED(s)) {
        TRACE(INFO) << "Process: " << name_ << " state: " << st.state_ << " -> "
                    << ProcessState::dead;
        st.state_ = ProcessState::dead;

    } else if (WIFSTOPPED(s)) {
        TRACE(INFO) << "Process: " << name_ << " state: " << st.state_ << " -> "
                    << ProcessState::stopped;
        st.state_ = ProcessState::stopped;

    } else if (WIFCONTINUED(s)) {
        TRACE(INFO) << "Process: " << name_ << " state: " << st.state_ << " -> "
                    << ProcessState::running;
        st.state_ = ProcessState::running;
    }

    set(st);
}

void Process::persistent_subject_runner_set(SubjectRunner &sr)
{
    persistent_subject_runner = &sr;
}

}  // namespace notifications
}  // namespace vtss
