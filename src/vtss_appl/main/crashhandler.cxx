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

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <mutex>
#include <vtss/basics/vector-static.hxx>
#include "types.hxx"
#include "crashhandler.hxx"
#include "subject.hxx"
#include <vtss/basics/notifications/process-daemon.hxx>
#include "backtrace.hxx"
#include "misc_api.h"
#include "control_api.h"

static vtss_mutex_t backtrace_mutex;

static vtss::VectorStatic<int, 500> threads_killed_so_far;
static vtss::VectorStatic<int, 500> threads_backtraced_so_far;

static FILE *crashfile;

static void force_reboot(void)
{
    control_system_reset_no_cb(MESA_RESTART_COOL);
}

static void crashfile_open(void)
{
    if (!crashfile) {
        static bool crashfile_nuked;
        const char *mode = crashfile_nuked ? "a" : "w";

        // If this is the first time, we open the crashfile in this session,
        // re-create it. Otherwise append to it.
        crashfile = fopen(CRASHFILE_PATH CRASHFILE, mode);
        crashfile_nuked = true;
    }
}

int crashfile_printf(const char *fmt, ...)
{
    va_list ap;
    int     rc=-1;

    crashfile_open();

    if (crashfile) {
        // Print to crashfile...
        va_start(ap, fmt);
        rc = vfprintf(crashfile, fmt, ap);
        va_end(ap);
    }

    return rc;
}

void crashfile_close(void)
{
    if (crashfile) {
        fclose(crashfile);
        crashfile = NULL;
    }
}

static int crashfile_and_console_printf(const char *fmt, ...)
{
    va_list ap;
    int     rc;

    crashfile_open();

    // Print to crashfile...
    va_start(ap, fmt);
    rc = vfprintf(crashfile, fmt, ap);
    va_end(ap);

    // ...and to console
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    return rc;
}

static void print_threads_name(int (*pr)(const char *fmt, ...), int sig)
{
    int pid = getpid();
    int tid = syscall(SYS_gettid);
    vtss_thread_info_t info;

    (void)vtss_thread_info_get(vtss_thread_self(), &info);
    pr("\n<%s> PID=%d, TID=%d", info.name, pid, tid);

    if (sig) {
        pr(" Received signal %d (%s)", sig, strsignal(sig) ? strsignal(sig) : "Unknown");
        pr(" ");
    }

    pr("\n");
}

static void crashhandler_backtrace_signal(int sig)
{
    print_threads_name(crashfile_and_console_printf, sig);
    vtss_backtrace(crashfile_and_console_printf, 0 /* current thread */);
}

static void crashhandler_kill_and_backtrace_all_threads(int sig, void *ctx_)
{
    bool first_thread_to_die = false;

    // Get the thread ID of the current context and add it to the vector
    // to avoid killing it again.
    {
        vtss_mutex_lock(&backtrace_mutex);

        pid_t tid = syscall(SYS_gettid);

        // We should not send a new kill signal to this thread as it is already
        // dead!
        if (threads_killed_so_far.size() == 0) {
            first_thread_to_die = true;
            threads_killed_so_far.push_back(tid);
        }

#ifdef __mips__
        /* Dump the mips register values */
        ucontext_t *ctx = (ucontext_t *)ctx_;
        if (first_thread_to_die) {
            misc_code_version_print(crashfile_and_console_printf);
            crashfile_and_console_printf("Exception %d (%s) caught at PC 0x" VPRI64Fx("08") "\n",
                   sig, strsignal(sig) ? strsignal(sig) : "Unknown", ctx->uc_mcontext.pc);
            crashfile_and_console_printf(".at    " VPRI64Fx("08") "   .v0-v1 " VPRI64Fx("08") " " VPRI64Fx("08") "\n",
                   ctx->uc_mcontext.gregs[ 1], ctx->uc_mcontext.gregs[ 2], ctx->uc_mcontext.gregs[ 3]);
            crashfile_and_console_printf(".a0-a3 " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") "\n",
                   ctx->uc_mcontext.gregs[ 4], ctx->uc_mcontext.gregs[ 5], ctx->uc_mcontext.gregs[ 6],
                   ctx->uc_mcontext.gregs[ 7]);
            crashfile_and_console_printf(".t0-t7 " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") "\n",
                   ctx->uc_mcontext.gregs[ 8], ctx->uc_mcontext.gregs[ 9], ctx->uc_mcontext.gregs[10],
                   ctx->uc_mcontext.gregs[11], ctx->uc_mcontext.gregs[12], ctx->uc_mcontext.gregs[13],
                   ctx->uc_mcontext.gregs[14], ctx->uc_mcontext.gregs[15]);
            crashfile_and_console_printf(".s0-s7 " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") " " VPRI64Fx("08") "\n",
                   ctx->uc_mcontext.gregs[16], ctx->uc_mcontext.gregs[17], ctx->uc_mcontext.gregs[18],
                   ctx->uc_mcontext.gregs[19], ctx->uc_mcontext.gregs[20], ctx->uc_mcontext.gregs[21],
                   ctx->uc_mcontext.gregs[22], ctx->uc_mcontext.gregs[23]);
            crashfile_and_console_printf(".t8-t9 " VPRI64Fx("08") " " VPRI64Fx("08") "  .fp/.s8 " VPRI64Fx("08") "   .k0-k1 " VPRI64Fx("08") " " VPRI64Fx("08") "\n",
                   ctx->uc_mcontext.gregs[24], ctx->uc_mcontext.gregs[25], ctx->uc_mcontext.gregs[30],
                   ctx->uc_mcontext.gregs[26], ctx->uc_mcontext.gregs[27]);
            crashfile_and_console_printf(".gp    " VPRI64Fx("08") "      .sp " VPRI64Fx("08") "      .ra " VPRI64Fx("08") "\n",
                   ctx->uc_mcontext.gregs[28], ctx->uc_mcontext.gregs[29], ctx->uc_mcontext.gregs[31]);
        }
#elif defined __x86_64__
        /* Dump the program counter only */
        crashfile_and_console_printf("Exception %d (%s) caught at PC %p\n", sig, strsignal(sig) ? (strsignal(sig)) : "Unknown", (char *)__builtin_return_address(0));
#else
        /* Not handled! */
#endif

        // Backtrace the current thread
        crashhandler_backtrace_signal(sig);

        // This thread has now been backtraced - if this was the last thread
        // then we are ready do reboot.
        threads_backtraced_so_far.push_back(tid);

        vtss_mutex_unlock(&backtrace_mutex);
    }

    // Scan through all the threads and kill all new threads that are
    // discovered.
    {

        char buf[255];
        pid_t pid = getpid();
        pid_t tid = syscall(SYS_gettid);
        struct dirent *dir_entry;

        vtss_mutex_lock(&backtrace_mutex);

        snprintf(buf, 255, "/proc/%d/task/", pid);

        DIR *d = opendir(buf);
        if (!d) {
            crashfile_and_console_printf("Cannot open Input directory %s", buf);
            force_reboot();
        }

        while (1) {
            int i;

            dir_entry = readdir(d);
            if (!dir_entry) break;
            if (sscanf(dir_entry->d_name, "%d", &i) != 1) continue;
            if (i == tid) continue;

            // Check if the thread has already been killed
            auto itr = find(threads_killed_so_far.begin(),
                            threads_killed_so_far.end(), i);

            // Not killed yet - kill it and take note of it
            if (itr == threads_killed_so_far.end()) {
                int res = syscall(SYS_tgkill, pid, i, SIGABRT);
                if (res == 0) threads_killed_so_far.push_back(i);
            }
        }

        (void)closedir(d);
        vtss_mutex_unlock(&backtrace_mutex);
    }

    // The first thread to die will wait for all other threads to finish
    // backtrace before it will reboot the target
    if (first_thread_to_die) {
        int cnt = 0;
        FILE *fp;

        while (cnt < 20) {  // never wait more than 20 seconds before rebooting!
            vtss_mutex_lock(&backtrace_mutex);

            if (threads_killed_so_far.size() == threads_backtraced_so_far.size()) {
                vtss_mutex_unlock(&backtrace_mutex);
                break;
            }

            vtss_mutex_unlock(&backtrace_mutex);
            sleep(1);
            cnt++;
        }

        crashfile_and_console_printf("\n");

        if ((fp = fopen("/proc/vc3fdma","r")) != NULL) {
            crashfile_and_console_printf("Opening /proc/vc3fdma\n");
            int c = 0;
            unsigned int i = 0;
            for (c = fgetc(fp); c != EOF; c = fgetc(fp), i++) {
                crashfile_and_console_printf("%c", (char)c);
            }

            fclose(fp);
        }

        if ((fp = fopen("/proc/vtss_if_mux_filter","r")) != NULL) {
            crashfile_and_console_printf("Opening /proc/vtss_if_mux_filter\n");
            int c = 0;
            unsigned int i = 0;
            for (c = fgetc(fp); c != EOF; c = fgetc(fp), i++) {
                crashfile_and_console_printf("%c", (char)c);
            }

            fclose(fp);
        }

        if ((fp = fopen("/proc/vtss_if_mux_ifh","r")) != NULL) {
            crashfile_and_console_printf("Opening /proc/vtss_if_mux_ifh\n");
            int c = 0;
            unsigned int i = 0;
            for (c = fgetc(fp); c != EOF; c = fgetc(fp), i++) {
                crashfile_and_console_printf("%c", (char)c);
            }

            fclose(fp);
        }

        if ((fp = fopen("/proc/vtss_if_mux_port_conf","r")) != NULL) {
            crashfile_and_console_printf("Opening /proc/vtss_if_mux_port_conf\n");
            int c = 0;
            unsigned int i = 0;
            for (c = fgetc(fp); c != EOF; c = fgetc(fp), i++) {
                crashfile_and_console_printf("%c", (char)c);
            }

            fclose(fp);
        }

        crashfile_close();

        sleep(3);
#ifdef VTSS_NO_TURNKEY
        // Not going to reboot, but ignore "abort" signal and call abort to be
        // sure that the process terminates.
        signal(SIGABRT, SIG_DFL);
        abort();
#else
        // Reboot on internal CPU, not on PCIe
        force_reboot();
#endif
    }
}

#define GDB_PORT_NUMBER_DEFAULT 2345
#if defined(VTSS_SW_OPTION_DEBUG)
static int gdb_port_number = GDB_PORT_NUMBER_DEFAULT;
#else
static int gdb_port_number = 0;
#endif /* VTSS_SW_OPTION_DEBUG */

void vtss_crashhandler_gdbserver(int port)
{
    gdb_port_number = port;
}

static void gdb_server_start_stop(bool start, char *custom)
{
    // Start the GDB server in the background so that we can work with ICLI while it's running (in case
    // it's started from there).
    static vtss::notifications::ProcessDaemon gdbserver(&vtss::notifications::subject_main_thread, "gdbserver");

    bool alive = gdbserver.status().alive();

    if (start) {
        if (!alive) {
            gdbserver.executable = "/usr/bin/gdbserver";
            gdbserver.environment.emplace("LD_PRELOAD","");
            gdbserver.arguments.clear();
            gdbserver.arguments.push_back("--attach");
            gdbserver.arguments.push_back(custom ? custom : std::string("0.0.0.0:") + std::to_string(gdb_port_number));
            gdbserver.arguments.push_back(std::to_string(getpid()));
            gdbserver.max_restart_cnt(1);
            gdbserver.kill_policy(true);
        }

        printf("%s:\n%s", alive ? "GDB server is already started with" : "Starting GDB server using command", gdbserver.executable.c_str());
        for (int i = 0; i < gdbserver.arguments.size(); i++) {
            printf(" %s", gdbserver.arguments[i].c_str());
        }

        printf("\n");

        fflush(stdout);

        if (!alive) {
           gdbserver.adminMode(vtss::notifications::ProcessDaemon::ENABLE);
        }
    } else {
        if (alive) {
            gdbserver.adminMode(vtss::notifications::ProcessDaemon::DISABLE);
            gdbserver.executable = "";
            gdbserver.arguments.clear();
            printf("GDB server stopped\n");
        } else {
            printf("%% GDB server not started\n");
        }
    }
}

static void crashhandler_gdbserver(int sig, char *gdb_env)
{
    // Do a backtrace of the current thread -  but do not start killing other threads

    printf("\n\n");
    if (sig) {
        printf("Caught signal %d\n", sig);
    }

    // Backtrace the current thread before starting GDB. We cannot backtrace
    // all threads here as this will kill them and thereby destroy the state
    // that we might want to inspect with GDB.
    crashhandler_backtrace_signal(sig);

    gdb_server_start_stop(true, gdb_port_number ? nullptr : gdb_env);

    // Stop the thread here to give GDB a fair change to call ptrace before we
    // terminate.
    while (1) {
        sleep(1);
    }
}

void vtss_gdb_server_start(void)
{
    if (!gdb_port_number) {
        gdb_port_number = GDB_PORT_NUMBER_DEFAULT;
    }

    gdb_server_start_stop(true, nullptr);
}

void vtss_gdb_server_stop(void)
{
    gdb_server_start_stop(false, nullptr);
}

static void crashhandler_signal(int sig, siginfo_t *siginfo, void *ctx)
{
    char *gdb_env = getenv("VTSS_GDBSERVER_LISTEN");
    if (gdb_port_number || gdb_env) {
        crashhandler_gdbserver(sig, gdb_env);
    } else {
        crashhandler_kill_and_backtrace_all_threads(sig, ctx);
    }
}

void vtss_crashhandler_setup(void)
{
    struct sigaction sa;

    vtss_mutex_init(&backtrace_mutex);

    memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = crashhandler_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;

    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);

     // Set number of seconds the kernel waits before rebooting on a panic.
    int fd = open("/proc/sys/kernel/panic", O_WRONLY);
    static const char *panic_val = "3\n";
    if (fd != -1) {
        write(fd, panic_val, strlen(panic_val));
        close(fd);
    }
}

