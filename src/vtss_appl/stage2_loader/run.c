/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "mfi.h"

typedef enum
{
    false,
    true
} bool;

static bool is_executable_file(const char *n) {
    struct stat st;

    if (access(n, X_OK) != 0) return false;
    if (stat(n, &st) != 0) return false;
    if (!S_ISREG(st.st_mode)) return false;

    return true;
}

int try_run(const char *e) {
    static const char *exec_argv[] = {"", 0};
    static const char *exec_envp[] = {"HOME=/", "TERM=linux", "SHELL=/bin/sh",
                                      "USER=root", 0};

    debug("Try run: %s\n", e);
    if (!is_executable_file(e)) {
        debug("Try run: %s - not an executable\n", e);
        return -1;
    }

    exec_argv[0] = e;
    pid_t p = vfork();
    debug("Try run: %s - fork -> %d\n", e, p);

    if (p == -1) return -1;

    if (p == 0) {
        // Child code //////////////////////////////////////////////////////////
        int fd_max, i;

        for (i = 1; i < 32; ++i) signal(i, SIG_DFL);
        sigset_t set;
        sigfillset(&set);
        sigprocmask(SIG_UNBLOCK, &set, NULL);
        if (setsid() == -1) perror("setsid: ");

        fd_max = sysconf(_SC_OPEN_MAX);
        if (fd_max == -1) fd_max = 1024;
        for (i = 0; i < fd_max; ++i) {
            if (i == STDIN_FILENO) continue;
            if (i == STDOUT_FILENO) continue;
            if (i == STDERR_FILENO) continue;
            close(i);
        }

        (void)execve(e, (char **)exec_argv, (char **)exec_envp);
        _exit(EXIT_FAILURE);
        // Child code end here /////////////////////////////////////////////////

    } else {
        int status;
        do {
            status = 0;
            waitpid(p, &status, 0);
            if (WIFEXITED(status)) {
                warn("Process %s (%d) returned with exit code %d\n", e, p,
                     WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                warn("Process %s (%d) was killed by signal %s (%d)\n", e, p,
                     strsignal(WTERMSIG(status)), WTERMSIG(status));
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 0;
}


