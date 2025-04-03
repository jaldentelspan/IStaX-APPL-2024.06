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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reboot.h>

#include <linux/loop.h>
#include <linux/reboot.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <spawn.h>

#define LOG_ERROR 2
#define LOG_WARN  4
#define LOG_INFO  6
#define LOG_DEBUG 7
#define LOG_NOISE 8


int warn(const char *fmt, ...);
int info(const char *fmt, ...);
int debug(const char *fmt, ...);
int noise(const char *fmt, ...);

unsigned int g_log_level = LOG_INFO;

int pivot_root(const char *n, const char *o) {
    return syscall(__NR_pivot_root, n, o);
}

/*
void dump_dir(const char *path)
{
    struct dirent *de;
    DIR *d;

    if (d = opendir(path)) {
        noise("Contents of %s:", path);
        while ((de = readdir(d)) != 0) {
            if (LOG_NOISE <= g_log_level) {
                printf(" %s * ", de->d_name);
            }
        }
        if (LOG_NOISE <= g_log_level) printf("\n");
        closedir(d);
    }
}

*/

void print_time() {
    time_t rawtime;
    struct tm *timeinfo;
    struct tm timeinfo_r;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime_r(&rawtime, &timeinfo_r);
    strftime(buffer, 80, "%X", timeinfo);
    printf("%s ", buffer);
}

int info(const char *format, ...) {
    int res = 0;
    va_list args;

    if (LOG_INFO <= g_log_level) {
        print_time();
        va_start(args, format);
        res = vprintf(format, args);
        va_end(args);
        fflush(stdout);
    }
    return res;
}

int debug(const char *format, ...) {
    int res = 0;
    va_list args;

    if (LOG_DEBUG <= g_log_level) {
        print_time();
        va_start(args, format);
        res = vprintf(format, args);
        va_end(args);
    }
    return res;
}

int noise(const char *format, ...) {
    int res = 0;
    va_list args;

    if (LOG_NOISE <= g_log_level) {
        print_time();
        va_start(args, format);
        res = vprintf(format, args);
        va_end(args);
    }
    return res;
}

int warn(const char *format, ...) {
    int res = 0;
    va_list args;

    if (LOG_WARN <= g_log_level) {
        print_time();
        va_start(args, format);
        res = vprintf(format, args);
        va_end(args);
    }
    return res;
}

int warn_(int line, const char *format, ...) {
    int res;
    print_time();
    va_list args;
    va_start(args, format);
    res = vprintf(format, args);
    va_end(args);
    printf("\nLine: %d, errno: %d error: %s\n", line, errno, strerror(errno));
    return res;
}

void fatal(unsigned line, const char *format, ...) {
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
           "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!! FATAL-ERROR at line: %d\n!!!! MSG: ", line);
    va_list args;
    va_start(args, format);
    (void)vprintf(format, args);
    va_end(args);
    printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
           "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    sleep(1);
    reboot(LINUX_REBOOT_CMD_RESTART);
}

void fatal_(unsigned line, const char *format, ...) {
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
           "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!! FATAL-ERROR at line: %d, errno: %d error: %s\n!!!! MSG: ",
           line, errno, strerror(errno));
    va_list args;
    va_start(args, format);
    (void)vprintf(format, args);
    va_end(args);
    printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
           "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    sleep(1);
    reboot(LINUX_REBOOT_CMD_RESTART);
}

#define FATAL(CMD, MSG)                      \
    {                                        \
        int res = CMD;                       \
        if (res != 0) fatal_(__LINE__, MSG); \
    }

#define WARN(CMD, MSG)                      \
    {                                       \
        int res = CMD;                      \
        if (res != 0) warn_(__LINE__, MSG); \
    }

// find the last occurence of needle in haystack
char *strrstr0(const char *haystack, const char *needle) {
    char *tmp = NULL;
    char *p = NULL;

    if (!needle[0]) return (char *)haystack + strlen(haystack);

    for (;;) {
        p = strstr((char *)haystack, needle);
        if (!p) return tmp;
        tmp = p;
        haystack = p + 1;
    }
}

char *cmd_arg(const char *key, int size, char *val) {
    int fd, res;
    const char *p;
    char buf[8 * 1024] = {};

    fd = open("/proc/cmdline", O_RDONLY);
    if (fd == -1) {
        printf("Failed to open /proc/cmdline: %s\n", strerror(errno));
        return 0;
    }

    res = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (res<0) {
        return 0;
    }
    buf[res] = '\0';

    p = strrstr0(buf, key);
    if (!p) {
        return 0;
    }

    p += strlen(key);

    while (*p && *p != ' ' && *p != '\n') {
        if (size > 1) {
            *val++ = *p++;
            *val = 0;
            size --;
        } else {
            /* Overrun */
            return 0;
        }
    }

    return val;
}

static void basic_linux_system_init() {
    int fd = -1;
    int res, line;

    // Very basic stuff
    chdir("/");
    setsid();
    putenv((char *)"HOME=/");
    putenv((char *)"TERM=linux");
    putenv((char *)"SHELL=/bin/sh");
    putenv((char *)"USER=root");

#define DO(X)            \
    res = X;             \
    if (res == -1) {     \
        line = __LINE__; \
        goto ERROR;      \
    }
#define CHECK(X)         \
    if (X) {             \
        line = __LINE__; \
        goto ERROR;      \
    }

    // Mount proc and sysfs as we need this to find the NAND flash.
    DO(mount("proc", "/proc", "proc", 0, 0));
    DO(mount("sysfs", "/sys", "sysfs", 0, 0));

    // Enable sys-requests - ignore errors as this is a nice-to-have
    fd = open("/proc/sys/kernel/sysrq", O_WRONLY);
    static const char *one = "1\n";
    if (fd != -1) {
        write(fd, one, strlen(one));
    }

#undef DO
#undef CHECK
    if (fd != -1) close(fd);
    return;

ERROR:
    if (fd != -1) close(fd);
    printf("BASIC SYSTEM INIT FAILED!!!\n");
    printf("File: %s, Line: %d, code: %d\n", __FILE__, line, res);
    printf("errno: %d error: %s\n\n", errno, strerror(errno));
    fflush(stdout);
    fflush(stderr);

    reboot(LINUX_REBOOT_CMD_RESTART);
    abort();
}

static char **pp_env; // needs to be in global scope, as usage for posix_spawn requires this

static int appl_run(const char *appl, char *args[], char *ret_buf, int *p_num_bytes_returned)
{
    pid_t vs_pid;
    int out[2];
    posix_spawn_file_actions_t as;
    char outfile_buf[20];
    int i;
    int num_byt_returned;
    int status;

    pipe(out);

    posix_spawn_file_actions_init(&as);
    posix_spawn_file_actions_adddup2(&as, out[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&as, out[0]);

    if (posix_spawn(&vs_pid, appl, &as, NULL, args, pp_env) != 0) {
        info("Failed to execute application %s", appl);
        return 0;
    } else {
        do {
            if (waitpid(vs_pid, &status, 0 /*WUNTRACED | WCONTINUED*/) == -1) {
                debug("waitpid error");
            } else {
                if (WIFEXITED(status)) {
                    noise("child status: exited, status=%d\n", WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    noise("child status: killed by signal %d\n", WTERMSIG(status));
                } else if (WIFSTOPPED(status)) {
                    noise("child status: stopped by signal %d\n", WSTOPSIG(status));
                } else if (WIFCONTINUED(status)) {
                    noise("child status: continued\n");
                }
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        close(out[1]);
        num_byt_returned = read(out[0], outfile_buf, sizeof(outfile_buf));
        if (LOG_NOISE <= g_log_level) {
            noise("Appl %s executed ok. Output is %d bytes: ", appl, num_byt_returned);
            for (i = 0; i < num_byt_returned; i++) {
                printf("%c", outfile_buf[i]);
            }
            printf("\n");
        }
        if (ret_buf) {
            memcpy(ret_buf, outfile_buf, num_byt_returned);
            *p_num_bytes_returned = num_byt_returned;
        }
        close(out[0]);
        posix_spawn_file_actions_destroy(&as);
        return 1;
    }
}



void change_root(const char *dev) {
    struct dirent *de;
    int res, cnt = 0;
    int file_fd;
    int app_hash_fd, app_roothash_fd;
    int loop_device_fd;
    int loop_device_id;
    char loop_device_name[50];
    struct stat st;
    int secure_build_provided = 0;
    DIR *d;

    while (1) {
        res = stat(dev, &st);
        if (res == 0)
            break;

        if (cnt > 100000) {
            warn("Timeout. Device %s not found\n", dev);
            warn("\n");
            warn("Devices found:\n");
            d = opendir("/dev");
            if (d) {
                while ((de = readdir(d)) != NULL) {
                    printf("    %s\n", de->d_name);
                }

                closedir(d);
            }
            reboot(LINUX_REBOOT_CMD_RESTART);
        }

        usleep(100);
        cnt += 1;
    }
    usleep(500); // short break to allow mmc driver to finalize initialization

    // do initial mount of device
    info("Do initial mount of device %s\n", dev);
    FATAL(mount(dev, "/mnt", "ext4", 0, 0), "Mount failed");

    app_hash_fd = open("/mnt/app.hash", O_RDONLY);
    if (app_hash_fd > 0) {
        app_roothash_fd = open("/mnt/app.roothash", O_RDONLY);
        if (app_roothash_fd > 0) {
            int read_byt;
            char root_hash_buf[100];
            char *cmd_ver_setup[] = {  "veritysetup", "open", "/mnt/app.ext4", "dmv_app", "/mnt/app.hash", "", NULL };

            // read roothash from file provided
            read_byt = read(app_roothash_fd, root_hash_buf, sizeof(root_hash_buf));
            close(app_roothash_fd);

            if (read_byt > 0) {
                root_hash_buf[read_byt] = 0;
                // store pointer onto args list
                cmd_ver_setup[5] = root_hash_buf;
                info("Root hash file present (%d bytes). Execute veritysetup to read image\n", read_byt);

                if (appl_run("/usr/sbin/veritysetup", cmd_ver_setup, 0, 0)) {
                    info("Read image ok. Device /dev/mapper/dmv_app created\n");
                    secure_build_provided = 1;
                } else {
                    fatal(__LINE__, "Failed during execute of veritysetup 'open'");
                }
            } else {
                fatal_(__LINE__, "Root hash file present, but it's empty");
            }
        }
        close(app_hash_fd);
    } else {
        // no hash file present. Ok as this depends on the build provided.
    }

    if (secure_build_provided) {
        info("Attempt mount of device dev/mapper/dmv_app\n");
        if (mount("/dev/mapper/dmv_app", "/mnt", "ext4", MS_RDONLY, NULL) != 0) {
            info("!!! warning: unable to mount device. Incorrect signature? !!!\n");
            // reboot for now.. (TBD)
            sleep(1);
            reboot(LINUX_REBOOT_CMD_RESTART);

#if 0
            /*
                TGO: 24-03-18: This code supports swapping of primarily/backup image on emmc.
                If application fails to start (actually mount of partition fails), the partition
                is assumed to be invalid and the application (stage2 loader) needs to try an alternate
                partition. This is done by swapping the values for uboot environment variables mmc_cur/mmc_bak.
                After this is done, system is restarted and uboot will start again and attempt running on the
                other partition.
                This is - however - only valid on systems where emmc has a primarily and backup partition and this
                is controlled by uboot (Laguna/Maserati).  Further investigation is needed for testing to
                clarity if this affects systems running on non-emmc, with alternate emmc-layout, systems running
                other bootloader and/or other combinations.
                Since the code will become handy within a near future, we decided to keep it in here, but disable
                it for now.
            */
            char cur_id_buf[20];
            char bak_id_buf[20];
            char ret_buf[20];
            int ret_buf_size;
            int current_image_id, backup_image_id;
            int swap_failed = 0;

            char *cmd_get_cur[] =  {  "fw_printenv", "-c", "/tmp/fw_env_mmc.config", "-l", "/mnt", "-n", "mmc_cur", NULL };
            char *cmd_get_bak[] = {  "fw_printenv", "-c", "/tmp/fw_env_mmc.config", "-l", "/mnt", "-n", "mmc_bak", NULL };
            char *cmd_set_cur[] = {  "fw_setenv", "-c", "/tmp/fw_env_mmc.config", "-l", "/mnt", "mmc_cur", bak_id_buf, NULL };
            char *cmd_set_bak[] = {  "fw_setenv", "-c", "/tmp/fw_env_mmc.config", "-l", "/mnt", "mmc_bak", cur_id_buf, NULL };


            if (appl_run("/usr/sbin/fw_printenv", cmd_get_cur, cur_id_buf, &ret_buf_size)) {
                current_image_id = atoi(cur_id_buf);
                sprintf(cur_id_buf, "%d", current_image_id);
            }

            if (appl_run("/usr/sbin/fw_printenv", cmd_get_bak, bak_id_buf, &ret_buf_size)) {
                backup_image_id = atoi(bak_id_buf);
                sprintf(bak_id_buf, "%d", backup_image_id);
            }

            info("Current image %d; Backup image %d\n", current_image_id, backup_image_id);

            // set/swap cur+bak. Note: cmd doesn't output anything, hence a dummy return buffer
            // is used. Note: cmd_set_xxx already uses correct/swapped data/buffer
            if (!appl_run("/usr/sbin/fw_setenv", cmd_set_cur, ret_buf, &ret_buf_size)) {
                info("Unable to set 'mmc_cur' to %s", bak_id_buf);
                swap_failed |= 0x01;
            }

            if (swap_failed == 0) {
                if (!appl_run("/usr/sbin/fw_setenv", cmd_set_bak, ret_buf, &ret_buf_size)) {
                    info("Unable to set 'mmc_bak' to %s", cur_id_buf);
                    swap_failed |= 0x02;
                }
            }
            // so far, so good. Read back the values
            if (swap_failed == 0) {
                if (appl_run("/usr/sbin/fw_printenv", cmd_get_cur, ret_buf, &ret_buf_size)) {
                    debug("Read back of current: %d\n", atoi(ret_buf));
                    if (atoi(ret_buf) != backup_image_id) {
                        swap_failed |= 0x04;
                    }
                }
            }
            if (swap_failed == 0) {
                if (appl_run("/usr/sbin/fw_printenv", cmd_get_bak, ret_buf, &ret_buf_size)) {
                    debug("Read back of backup: %d\n", atoi(ret_buf));
                    if (atoi(ret_buf) != current_image_id) {
                        swap_failed |= 0x08;
                    }
                }
            }

            if (swap_failed == 0) {
                info("Images swapped. Current image %d; Backup image %d. Now rebooting\n", backup_image_id, current_image_id);
                sleep(1);
                reboot(LINUX_REBOOT_CMD_RESTART);
            } else {
                info("!!! warning: unable to swap cur/bak settings (error %08x) !!!", swap_failed);
                // reboot anyway!
                sleep(1);
                reboot(LINUX_REBOOT_CMD_RESTART);
            }
#endif
        } else {
            ; // all ok
        }
        // setup new root
        info("Setup new root mount\n");
        FATAL(pivot_root("/mnt", "/mnt/mnt"), "pivot_root failed");

    } else {
        char loop_device_name[50];
        int loop_device_fd;
        int loop_device_id;
        int file_fd;

        info("Non-secure boot provided\n");
        file_fd = open("/mnt/app.ext4", O_RDWR);
        if (file_fd) {
            // find available loop device
            for (loop_device_id = 0; loop_device_id <= 7; loop_device_id++) {
                sprintf(loop_device_name, "/dev/loop%d", loop_device_id);
                if ((loop_device_fd = open(loop_device_name, O_RDWR)) < 0) {
                    if (loop_device_id >= 7) {
                        info("Can't find available loop device\n");
                    } else {
                        // no worries (for now). Increase id and try once more
                    }
                } else {
                    if (ioctl(loop_device_fd, LOOP_SET_FD, file_fd) < 0) {
                        continue; // failed to set loop mark on loop device fd. Attempt increasing counter
                    }
                    info("Loop device %d used for /mnt/app.ext4\n", loop_device_id);
                    close(file_fd);
                    close(loop_device_fd);
                    break;
                }
            }
        } else {
            info("Unable to open application (app.ext4)\n");
        }

        if (mount(loop_device_name, "/mnt2", "ext4", 0, 0) != 0) {
            info("Could not mount %son /mnt2\n", loop_device_name);
        }
        // setup new root
        info("Setup new root mount\n");
        FATAL(pivot_root("/mnt2", "/mnt2/mnt"), "pivot_root failed");
    }

    // do additional mounting
    FATAL(mount("/mnt/dev", "/dev", NULL, MS_MOVE, NULL), "Mount move failed");
    FATAL(mount("proc", "/proc", "proc", 0, 0), "Mount failed");
    FATAL(mount("sysfs", "/sys", "sysfs", 0, 0), "Mount failed");
    FATAL(mount("ramfs", "/tmp", "ramfs", 0,
                "size=8388608,mode=1777"),
          "Mount failed");

    (void)mkdir("/dev/pts", 0755);
    FATAL(mount("devpts", "/dev/pts", "devpts", 0, 0), "Mount failed");

    (void)mkdir("/dev/shm", 0755);
    FATAL(mount("shm", "/dev/shm", "ramfs", 0, 0), "Mount failed");

}

int main(int argc, char *argv[]) {
    char buf_block[64];
    char buf_init[64];

    if ((int)getpid() == 1) {
        basic_linux_system_init();
    }

    if (!cmd_arg("root_next=", sizeof(buf_block), buf_block)) {
        fatal(__LINE__, "No root_next found!");
    }

    change_root(buf_block);

    if (cmd_arg("init_next=", sizeof(buf_init), buf_init)) {
        execl(buf_init, buf_init, 0);
    } else {
        execl("/sbin/init", "init", 0);
    }

    reboot(LINUX_REBOOT_CMD_RESTART);
    return 0;
}
