/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <linux/reboot.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <unistd.h>
#include <libgen.h>
#include <ctype.h>
#include <mtd/ubi-user.h>

#include "tar.h"
#include "service.h"
#define MSCC_SERVICE_DIR "/etc/mscc/service/"  // do not forget the very last slash!

static int cmp_file_extension(const char *filename, const char *ext)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return -1;
    dot++;
    if (strcmp(dot, ext) == 0) {
        return 0;
    }
    return -1;
}

void sig_handler(int sig)
{
    if (sig == SIGCHLD) {
        printf("SIGCHLD is received!\n");
    }
}

// ****************************************************************************
char *attr_read_file_into_buf(const char *filename, ssize_t *size_of_buf)
{
    char *buf = NULL;
    struct stat st;

    int fd = open(filename, O_RDONLY);

    if (fd == -1) {
        perror("open");
        return NULL;
    }

    if (fstat(fd, &st) == -1) {
        perror("stat");
        goto EXIT;
    }

    *size_of_buf = st.st_size;
    buf = (char *)malloc(*size_of_buf);

    if (!buf) {
        perror("malloc");
        goto EXIT;
    }

    if (read(fd, buf, *size_of_buf) != *size_of_buf) {
        perror("read");
        goto EXIT;
    }

    close(fd);
    return buf;
EXIT:
    close(fd);
    if (buf) {
        free(buf);
    }
    return NULL;
}

size_t attr_get_line(char *buf, size_t len)
{
    size_t old_len = len;
    char *tmp = buf;

    while (len) {
        if (*tmp == '\n') {
            *tmp = '\0';
            len--;
            break;
        } else if (*tmp == '\r') {
            *tmp = '\0';
            len--;
            break;
        }
        len--;
        tmp++;
    }
    return old_len - len;
}

char *trim_head(char *s)
{
    while (*s && isspace((unsigned char)(*s)))
        s++;
    return s;
}

char *trim_tail(char *s)
{
    char *p = s + strlen(s);
    while (p > s && isspace((unsigned char)(*--p)))
        *p = '\0';
    return s;
}

char *trim_head_tail(char *s)
{
    return trim_head(trim_tail(s));
}

int attr_split_into_key_val(char *line, char **key, char **val)
{
    *key = (char *)line;
    *val = strchr(line, '=');

    if (!*val) {
        *key = NULL;
        return 0;
    }
    **val = '\0';
    (*val)++;
    *key = trim_head_tail(*key);
    *val = trim_head_tail(*val);

    return 1;
}

struct Attr *attr_construct(const struct AttrConf *conf, int conf_size, const char *key,
                            const char *val)
{
    int i;
    struct Attr *attr;

    for (i = 0; i < conf_size; ++i) {
        if (strcmp(key, conf[i].key_name) == 0) {
            attr = conf[i].create(val);
            if (attr) {
                attr->attr_type = i;
            }
            return attr;
        }
    }
    return NULL;
}

// ****************************************************************************
struct Attr *attr_name_create(const char *data)
{
    struct AttrName *tmp;

    if (strchr(data, ' ')) {
        return NULL;
    }

    if (!(tmp = (struct AttrName *)malloc(sizeof(struct AttrName)))) {
        perror("malloc");
        return NULL;
    }

    memset(tmp, 0x0, sizeof(struct AttrName));
    tmp->name = strdup(data);

    return (struct Attr *)(tmp);
}

void attr_name_free(struct Attr *data)
{
    struct AttrName *tmp = (struct AttrName *)data;
    free((void *)tmp->name);
}

struct Attr *attr_depend_create(const char *data)
{
    struct AttrDepend *tmp;

    if (strchr(data, ' ')) {
        return NULL;
    }
    if (!(tmp = (struct AttrDepend *)malloc(sizeof(struct AttrDepend)))) {
        perror("malloc");
        return NULL;
    }
    // save all its dependency names for later use
    tmp->name = strdup(data);
    // Associate all its dependencies (tmp->service) later in
    // service_all_dependency_ready() based on their names.
    // We cannot do that until that point

    return (struct Attr *)(tmp);
}

void attr_depend_free(struct Attr *attr)
{
    struct AttrDepend *tmp =  (struct AttrDepend *)attr;
    free((void *)tmp->name);
    tmp->service = NULL;
}

struct Attr *attr_cmd_create(const char *data)
{
    struct AttrCmd *tmp;

    if (!(tmp = (struct AttrCmd *)malloc(sizeof(struct AttrCmd)))) {
        perror("malloc");
        return NULL;
    }
    memset(tmp, 0x0, sizeof(struct AttrCmd));
    tmp->attr.attr_type = SERVICE_CMD;
    tmp->cmd = strdup(data);

    return (struct Attr *)tmp;
}

void attr_cmd_free(struct Attr *data)
{
    struct AttrCmd *tmp = (struct AttrCmd *)data;
    free((void *)tmp->cmd);
}


struct Attr *attr_ready_file_create(const char *data)
{
    struct AttrReadyFile *tmp;

    if (strchr(data, ' ')) {
        return NULL;
    }
    if (!(tmp = (struct AttrReadyFile *)malloc(sizeof(struct AttrReadyFile)))) {
        perror("malloc");
        return NULL;
    }
    tmp->attr.attr_type = SERVICE_READY_FILE;
    tmp->file = strdup(data);

    return (struct Attr *)tmp;
}

void attr_ready_file_free(struct Attr *data)
{
    struct AttrReadyFile *tmp = (struct AttrReadyFile *)data;
    free((void *)tmp->file);
}

struct Attr *attr_type_create(const char *data)
{
    struct AttrType *tmp;

    if (strchr(data, ' ')) {
        return NULL;
    }
    if (!(tmp = (struct AttrType *)malloc(sizeof(struct AttrType)))) {
        perror("malloc");
        return NULL;
    }
    tmp->attr.attr_type = SERVICE_TYPE;
    if (strcmp("oneshot", data) == 0) {
        tmp->type = ONESHOT;
    } else {
        tmp->type = SERVICE;
    }

    return (struct Attr *)tmp;
}

Attr_t *attr_on_error_create(const char *data)
{
    AttrOnError_t *tmp;

    if (strchr(data, ' ')) {
        return NULL;
    }

    if (!(tmp = (AttrOnError_t *)malloc(sizeof(*tmp)))) {
        perror("malloc");
        return NULL;
    }

    tmp->attr.attr_type = SERVICE_ON_ERROR;

    if (strcmp("reboot", data) == 0) {
        tmp->on_error = REBOOT;
    } else if (strcmp("ignore", data) == 0) {
        tmp->on_error = IGNORE;
    } else {
        // Default to respawn - both if the user has
        // used wrong case, written it wrongly or not
        // specified this attribute at all
        tmp->on_error = RESPAWN;
    }

    return (Attr_t *)tmp;
}

struct Attr *attr_env_create(const char *data)
{
    struct AttrEnv *tmp;
    char *tmp_key, *tmp_val;
    size_t size;

    tmp_key = (char *)data;
    if ((tmp_val = strchr((char *)data, '=')) == NULL) {
        // Equal sign not found. Take it as unexporting, that is, ignore.
        return NULL;
    }

    if (!(tmp = (struct AttrEnv *)malloc(sizeof(struct AttrEnv)))) {
        perror("malloc");
        return NULL;
    }

    tmp->attr.attr_type = SERVICE_ENV;

    // Zero-terminate key
    *(tmp_val++) = '\0';

    tmp->key = strdup(trim_head_tail(tmp_key));
    tmp->val = strdup(trim_head_tail(tmp_val));

    // "FOO=bar\0", '=', '\0' take 2 chars
    size = sizeof(char) * (strlen(tmp->key) + strlen(tmp->val) + 2);
    tmp->res = (char *)malloc(size);
    if (tmp->res == NULL) {
        free(tmp);
        return NULL;
    }
    memset(tmp->res, 0x0, size);
    sprintf(tmp->res, "%s=%s", tmp->key, tmp->val);
    tmp->res[size - 1] = '\0';

    return (struct Attr *)tmp;
}

void attr_env_free(struct Attr *data)
{
    if (data != NULL) {
        struct AttrEnv *tmp = (struct AttrEnv *)data;
        free((void *)tmp->key);
        free((void *)tmp->val);
    }
}

struct Attr *attr_profile_create(const char *data)
{
    struct AttrProfile *tmp;

    if (strchr(data, ' ')) {
        return NULL;
    }
    if (!(tmp = (struct AttrProfile *)malloc(sizeof(struct AttrProfile)))) {
        perror("malloc");
        return NULL;
    }

    memset(tmp, 0x0, sizeof(struct AttrProfile));
    tmp->attr.attr_type = SERVICE_PROFILE;
    tmp->profile = strdup(data);

    return (struct Attr *)tmp;
}

void attr_profile_free(struct Attr *data)
{
    struct AttrProfile *tmp = (struct AttrProfile *)data;
    free((void *)tmp->profile);
}

Service_t *service_construct()
{
    Service_t *tmp = (Service_t *)malloc(sizeof(Service_t));
    if (!tmp) {
        perror("malloc");
        return NULL;
    }
    memset(tmp, 0x0, sizeof(Service_t));
    tmp->pid = -1;
    return tmp;
}

Service_t *service_load(char profile[64])
{
    DIR *FD;
    struct dirent *in_file;
    char filepath[PATH_MAX];
    Service_t *curr, *elt, *head = NULL;
    int n = 0;
    char *data;
    ssize_t data_len;
    char tmp_profile[64];

    strncpy(tmp_profile, profile, sizeof(tmp_profile));

    if ((FD = opendir(MSCC_SERVICE_DIR)) == NULL) {
        fprintf(stderr, "Error: Failed to open " MSCC_SERVICE_DIR " directory [%m]\n");
        return NULL;
    }

    while ((in_file = readdir(FD))) {
        // ignore current & parent dir
        if (!strcmp(in_file->d_name, "."))
            continue;
        if (!strcmp(in_file->d_name, ".."))
            continue;
        if (cmp_file_extension(in_file->d_name, "service"))  // only parse *.service
            continue;

        strncpy(filepath, MSCC_SERVICE_DIR, sizeof(filepath));
        strncat(filepath, in_file->d_name, sizeof(filepath)-1);
        filepath[sizeof(filepath) - 1] = '\0';  // force termination

        curr = service_construct();
        data = attr_read_file_into_buf(filepath, &data_len);
        if (curr && data) {
            config_parse(data, data_len, attr_conf, SERVICE_CNT, curr->attr);
        } else {
            free(data);
            free(curr);
            continue;
        }

        if (service_validate(attr_conf, curr)) {
            // service profile check
            if (((AttrProfile_t *)curr->attr[SERVICE_PROFILE]) &&
                strncmp(((AttrProfile_t *)curr->attr[SERVICE_PROFILE])->profile,
                        tmp_profile, sizeof(tmp_profile)) != 0) {
                free(data);
                free(curr);
                continue;
            }
            if (!(((AttrProfile_t *)curr->attr[SERVICE_PROFILE])) &&
                strncmp("webstax", tmp_profile, sizeof(tmp_profile)) != 0) {
                free(data);
                free(curr);
                continue;
            }
            DL_APPEND(head, curr);
        } else {
            free(curr);
        }

        free(data);
        filepath[0] = '\0';
    }

    if (closedir(FD) == -1) {
        fprintf(stderr, "Error: Failed to closedir %s [%m]\n", MSCC_SERVICE_DIR);
    }

    DL_COUNT(head, elt, n);

    if (n == 0) {
        return NULL;
    }

    return head;
}

#if 0
static void service_free(Service_t *s, AttrConf_t *conf, ssize_t conf_size)
{
    Service_t *elt, *tmp;
    Attr_t *attr_elt, *attr_tmp;
    int i = 0;

    DL_FOREACH_SAFE(s, elt, tmp) {
        for (i = 0; i < conf_size; i++) {
            DL_FOREACH_SAFE(elt->attr[i], attr_elt, attr_tmp) {
                if (conf[i].free) {
                    conf[i].free(attr_elt);
                }
                DL_DELETE(elt->attr[i], attr_elt);
            }
        }
        DL_DELETE(s, elt);
    }
}
#endif

static const char *service_name_get(Service_t *s)
{
    AttrName_t *n = (AttrName_t *)s->attr[SERVICE_NAME];
    if (!n) return NULL;

    return n->name;
}

static const char *service_cmd_get(Service_t *s)
{
    AttrCmd_t *n = (AttrCmd_t *)s->attr[SERVICE_CMD];
    if (!n) return NULL;

    return n->cmd;
}

static enum ServiceType service_type_get(Service_t *s)
{
    AttrType_t *n = (AttrType_t *)s->attr[SERVICE_TYPE];
    if (!n) return SERVICE; // Default

    return n->type;
}

static enum OnError service_on_error_get(Service_t *s)
{
    AttrOnError_t *n = (AttrOnError_t *)s->attr[SERVICE_ON_ERROR];
    if (!n) return RESPAWN; // Default

    return n->on_error;
}

static const char *service_ready_file_get(Service_t *s)
{
    AttrReadyFile_t *n = (AttrReadyFile_t *)s->attr[SERVICE_READY_FILE];
    if (!n) return NULL; // Default

    return n->file;
}

int namecmp(Service_t *a, Service_t *b)
{
    return strcmp(((AttrName_t *)(a->attr[SERVICE_NAME]))->name,
                  ((AttrName_t *)(b->attr[SERVICE_NAME]))->name);
}

Service_t *service_by_name(Service_t *head, const char *name)
{
    Service_t *elt;

    DL_FOREACH(head, elt) {
        if (strcmp(((AttrName_t *)elt->attr[SERVICE_NAME])->name, name) == 0) {
            return elt;
        }
    }
    return NULL;
}

Service_t *service_by_pid(Service_t *head, pid_t pid)
{
    Service_t *elt;

    DL_FOREACH(head, elt) {
        if (elt->pid == pid) {
            return elt;
        }
    }
    return NULL;
}

void service_start(Service_t *s, int *forked, int use_non_blocking_console)
{
    int i, fd_max;
    Attr_t *elt;
    int num_ser_env, j;

    static const char *default_exec_envp[] = {"HOME=/", "TERM=linux",
                                              "SHELL=/bin/sh", "USER=root", 0};

    if (!s || s->pid > 0) return;

    s->pid = fork();

    if (s->pid == -1) {
        perror("fork");
        return;
    }

    *forked += 1;

    if (s->pid == 0) {
        // child
        DL_COUNT((Attr_t *)s->attr[SERVICE_ENV], elt, num_ser_env);

        int num_def_env = sizeof(default_exec_envp)/sizeof(default_exec_envp[0]);
        char *new_env[num_def_env + num_ser_env];

        for (j = 0; j < num_def_env - 1; j++) {
            new_env[j] = (char *)default_exec_envp[j];
        }

        DL_FOREACH((Attr_t *)s->attr[SERVICE_ENV], elt) {
            if (((AttrEnv_t *)elt)->res) {
                new_env[j] = ((AttrEnv_t *)elt)->res;
                j++;
            }
        }
        new_env[num_def_env + num_ser_env - 1] = 0;  // NULL terminate

        // Set up default signal handlers
        for (i = 1; i < 32; ++i) {
            signal(i, SIG_DFL);
        }
        sigset_t set;
        sigfillset(&set);
        sigprocmask(SIG_UNBLOCK, &set, NULL);

        // Make the new process session leader
        if (setsid() == -1) perror("setsid: ");

        // Close all file descriptors except for in/out/err
        fd_max = sysconf(_SC_OPEN_MAX);
        if (fd_max == -1) fd_max = 1024;
        for (i = 0; i < fd_max; ++i) {
            if (i == STDIN_FILENO) continue;
            if (i == STDOUT_FILENO) continue;
            if (i == STDERR_FILENO) continue;
            close(i);
        }

        if (use_non_blocking_console) {
            int f = fcntl(1, F_GETFL, 0);
            (void)fcntl(1, F_SETFL, f | O_NONBLOCK );
            f = fcntl(2, F_GETFL, 0);
            (void)fcntl(2, F_SETFL, f | O_NONBLOCK );
        }

        char a0[] = "/bin/sh";
        char a1[] = "-c";
        char *exec_argv[4] = {a0, a1, (char *)service_cmd_get(s), NULL};
        (void)execve("/bin/sh", (char **)exec_argv, (char **)new_env);
        printf("EXEC [%s]\n", service_cmd_get(s));
        perror("execvp: ");
        exit(EXIT_FAILURE);
    }
}

void service_running(Service_t *s)
{
    if (s->pid > 0) {
        s->running = 1;
    }
    s->running = 0;
}

int service_ready(Service_t *s)
{
    Attr_t *elt;

    if (!s) return 0;

    s->ready = 1;
    DL_FOREACH((Attr_t *)s->attr[SERVICE_READY_FILE], elt) {
        if (access(((AttrReadyFile_t *)elt)->file, F_OK) != 0) {
            s->ready = 0;
        }
    }
    return s->ready;
}

int service_all_dependency_ready(Service_t *head, Service_t *s)
{
    Attr_t *attr_elt;
    AttrDepend_t *tmp;

    assert(s);
    s->ready = 1;

    // associate all its dependencies first
    DL_FOREACH(s->attr[SERVICE_DEPEND], attr_elt) {
        tmp = ((AttrDepend_t *)attr_elt);
        tmp->service = service_by_name(head, tmp->name);
    }

    // then check if they are ready
    DL_FOREACH(s->attr[SERVICE_DEPEND], attr_elt) {
        if (service_ready(((AttrDepend_t *)attr_elt)->service) == 0) {
            s->ready = 0;
        }
    }
    return s->ready;
}

static void service_status_print(pid_t            pid,
                                 int              status,
                                 const char       *service_name,
                                 enum ServiceType service_type,
                                 enum OnError     action_taken)
{
    // We do not want to see these messages for processes spawned by others.
    if (!service_name) {
        return;
    }

    printf("\nService \"%s\" of type = \"%s\" with PID = %d ",
           service_name,
           service_type == ONESHOT ? "oneshot" :
           service_type == SERVICE ? "service" : "<Unknown>",
           pid);

    if (WIFEXITED(status)) {
        printf("exited with return value %d.", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("killed by signal %s (%d).", strsignal(WTERMSIG(status)), WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
        printf("stopped execution by signal %s (%d).", strsignal(WSTOPSIG(status)), WSTOPSIG(status));
    } else if (WIFCONTINUED(status)) {
        printf("continued execution.");
    } else {
        // What's this?
        printf("got unknown status (%d).", status);
    }

    printf("%s\n",
           action_taken == IGNORE  ? " Ignoring."   :
           action_taken == RESPAWN ? " Respawning." :
           action_taken == REBOOT  ? " Rebooting..." : "");
}

void service_update_wait_status(Service_t *head, pid_t pid, int status)
{
    Service_t        *elt;
    int              exited           = WIFEXITED(status) || WIFSIGNALED(status);
    int              exited_normally  = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    const char       *service_name    = NULL;
    const char       *dev_ubi_ctrl    = "/dev/ubi_ctrl";
    int              ubi_num          = 0;
    const char       *mount_source    = "ubi0:switch";
    const char       *mount_point     = "/switch";
    int              ubi_fd;
    int              ubi_res;
    enum ServiceType service_type     = _SERVICE_TYPE_INVALID;
    enum OnError     action_taken     = _ON_ERROR_INVALID;

    DL_FOREACH(head, elt) {
        enum OnError service_on_error;

        if (elt->pid != pid) {
            // Not this process that died.
            continue;
        }

        service_name     = service_name_get(elt);
        service_type     = service_type_get(elt);
        service_on_error = service_on_error_get(elt);

        if (!exited) {
            // Do nothing. This is where we get if the process was
            // stopped or continued, e.g. by a debugger
            action_taken = IGNORE;
            break;
        }

        if (service_type == ONESHOT && exited_normally) {
            // Signal that we're ready.
            elt->ready = 1;
            // We don't want to say "IGNORE" here, because that
            // would cause the status printing routine to write that as well.
            action_taken = _ON_ERROR_INVALID;
            break;
        }

        if (service_on_error == REBOOT || service_on_error == IGNORE) {
            // Nothing else to do but printing the state of
            // the process and possibly reboot (which we
            // do afterwards).
            action_taken = service_on_error;
            break;
        }

        // Respawn.
        elt->pid = -1;
        elt->ready = 0;
        action_taken = RESPAWN;

        if (service_type == SERVICE) {
            const char *ready_file = service_ready_file_get(elt);

            if (ready_file && access(ready_file, F_OK) == 0) {
                (void)remove(ready_file);
            }
        }

        break;
    }

    // The pid that dies isn't in our list (probably because it got forked)
    service_status_print(pid, status, service_name, service_type, action_taken);

    // Finally reboot if requested to
    if (action_taken == REBOOT) {
        info("Starting filesystem shutdown\n");
        // Sync any unsaved files and write-buffers in /switch
        sync();

        // Unmount the /switch mount point. Use (the) force as we are rebooting
        // anyway and will loose unsaved data no matter what.
        if (umount2(mount_point, MNT_FORCE) == 0) {
            debug("Unmounted %s successfully\n", mount_point);

            // Detach the UBI device to stop the UBI background thread as it may
            // otherwise complain when it is halted by the Linux reboot.
            // See http://www.linux-mtd.infradead.org/faq/ubi.html#L_bgt_thread and
            // http://lists.infradead.org/pipermail/linux-mtd/2014-November/056431.html
	    if (g_is_ubifs) {
		    ubi_fd = open(dev_ubi_ctrl, O_RDONLY);
		    if (ubi_fd != -1) {
			    ubi_res = ioctl(ubi_fd, UBI_IOCDET, &ubi_num);
			    close(ubi_fd);

			    if (ubi_res != -1) {
				    debug("Detached UBI device %d successfully\n", ubi_num);
			    } else {
				    warn("Error: Detach of UBI device (%s:%d) failed [%s]\n", dev_ubi_ctrl, ubi_num, strerror(errno));
			    }
		    } else {
			    warn("Error: Open UBI device %s failed [%d: %s]\n", dev_ubi_ctrl, errno, strerror(errno));
		    }
	    }
	} else {
		// unmount may fail, especially for NAND devices, in which case we remount as read-only
		if (g_is_ubifs) {
			if (mount(mount_source, mount_point, "ubifs", MS_REMOUNT | MS_RDONLY, 0) != 0) {
				warn("Error: Remount R/O of %s failed [%d: %s]\n", mount_point, errno, strerror(errno));
			} else {
				debug("Remounted %s read-only successfully\n", mount_point);
			}
		}
		sync();
		sync();
        }

        // Print final message and flush before rebooting
        warn("Rebooting kernel\n");
        fflush(stdout);

        reboot(LINUX_REBOOT_CMD_RESTART);
        // Unreachable
    }
}

void service_wait_all_pending(Service_t *head)
{
    int status;
    pid_t pid;

    while (1) {
        pid = waitpid(-1, &status, WNOHANG);

        if (pid == -1) {
            perror("waitpid");
        } else if (pid == 0) {
            return;
        } else {
            service_update_wait_status(head, pid, status);
        }
    }
}

// ****************************************************************************
int selfpipe[2];
void selfpipe_sigh(int n)
{
    int save_errno = errno;
    (void)write(selfpipe[1], "",1);
    errno = save_errno;
}
void selfpipe_setup(void)
{
    static struct sigaction act;
    if (pipe(selfpipe) == -1) { abort(); }

    (void)fcntl(selfpipe[0],F_SETFL,fcntl(selfpipe[0],F_GETFL)|O_NONBLOCK);
    (void)fcntl(selfpipe[1],F_SETFL,fcntl(selfpipe[1],F_GETFL)|O_NONBLOCK);
    memset(&act, 0, sizeof(act));
    act.sa_handler = selfpipe_sigh;
    sigaction(SIGCHLD, &act, NULL);
}

void service_wait_blocking(Service_t *head, unsigned int milisec)
{
    int pid;
    int status;
    static char dummy[4096];
    fd_set rfds;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = milisec * 1000;
    FD_ZERO(&rfds);
    FD_SET(selfpipe[0], &rfds);
    if (select(selfpipe[0]+1, &rfds, NULL, NULL, &tv) > 0) {
        while (read(selfpipe[0],dummy,sizeof(dummy)) > 0);
        while ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
            if (pid == 0) {
                return;
            } else {
                service_update_wait_status(head, pid, status);
            }
        }
    }
}

void service_spawn(int use_non_blocking_console)
{
    Service_t *result, *elt;
    int n;

    result = service_load(g_switch_profile);

    if (!result) {
        char debug[64] = "debug";

        // no profile matching service found, fall back to debug profile service.
        result = service_load(debug);
        if (!result) {
            fprintf(stderr, "No service conf file found!\n");
            return;
        }
    }

    DL_COUNT(result, elt, n);

    selfpipe_setup();

    while (1) {
        int forked = 0;
        DL_FOREACH(result, elt) {
            if (service_all_dependency_ready(result, elt)) {
                service_start(elt, &forked, use_non_blocking_console);
            }
        }
        if (forked) {
            service_wait_all_pending(result);
        } else {
            service_wait_blocking(result, 500);
        }
    }
}
