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

#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <mtd/mtd-user.h>

#include <linux/loop.h>
#include <linux/reboot.h>
#include <sys/syscall.h>
#include <mtd/ubi-user.h>

#include <dirent.h>

#include "main.h"
#include "tar.h"
#include "mfi.h"
#include "ramload.h"

#include "firmware_vimage.h"
#include "service.h"

#define TLV_ROOTFS_NAME 1
#define TLV_ROOTFS_VERSION 2
#define TLV_ROOTFS_LICENSE 3
#define TLV_ROOTFS_PREEXEC 4
#define TLV_ROOTFS_TARXZ 5
#define TLV_ROOTFS_POSTEXEC 6
#define TLV_ROOTFS_FILE_NAME 7
#define TLV_ROOTFS_SQUASH 8

static size_t bytes_read;
static uint64_t t_fs_read_ms, t_squash_mount_ms;

// Enforce SHA signed images/tlv's?
bool secure_enforce;

// initial log level, updated by cmdline
unsigned int g_log_level = LOG_INFO;
// initial profile, updated by cmdline
char g_switch_profile[64] = "webstax";
// initial image type, updated by cmdline
bool g_fit_image = true;
// Disaster recovery
bool g_nomount_switch = false;
// UBIFS/ext4 handling
int g_is_ubifs = true;

static uint64_t uptime_milliseconds(void) {
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC, &time) == 0) {
        return ((uint64_t)time.tv_sec * 1000ULL) + (time.tv_nsec / 1000000ULL);
    }

    return 0;
}

int pivot_root(const char *n, const char *o) {
    return syscall(__NR_pivot_root, n, o);
}

void print_time() {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
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
#if !defined(STANDALONE)
    reboot(LINUX_REBOOT_CMD_RESTART);
#endif
    abort();
}

#if !defined(STANDALONE)
static void error_(unsigned line, const char *format, ...) {
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
           "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("!!!! ERROR at line: %d, errno: %d error: %s\n!!!! MSG: ", line,
           errno, strerror(errno));
    va_list args;
    va_start(args, format);
    (void)vprintf(format, args);
    va_end(args);
    printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
           "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
}
#endif

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
#if !defined(STANDALONE)
    reboot(LINUX_REBOOT_CMD_RESTART);
#endif
    abort();
}

#define FATAL(CMD, MSG...)                   \
    {                                        \
        int res = CMD;                       \
        if (res != 0) fatal_(__LINE__, MSG); \
    }

#define ERR(CMD, MSG...)                     \
    ({                                       \
        int res = CMD;                       \
        if (res != 0) error_(__LINE__, MSG); \
        res;                                 \
    })

#define WARN(CMD, MSG...)                   \
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

Buf *Buf_new() {
    Buf *p = (Buf *)malloc(sizeof(Buf));
    if (p) {
        memset(p, 0x0, sizeof(Buf));
        p->size = 0;
        p->data = NULL;
    }
    return p;
}

Buf *Buf_init(const size_t s) {
    Buf *p = Buf_new();
    if (p) {
        memset(p, 0x0, sizeof(Buf));
        p->size = s;
        p->data = (char *)malloc(s);
        if (!(p->data)) {
            p->size = 0;
        }
    }
    return p;
}

void Buf_free(Buf *b) {
    if (b) {
        if (b->data) {
            free(b->data);
            b->data = NULL;
        }
        free(b);
        b = NULL;
    }
}

typedef struct CmdLine { char fis_act[64]; } CmdLine;


int cmdline_read(CmdLine *cl) {
    char buf[8 * 1024];

    // Open the kernel cmdline
    int fd = open("/proc/cmdline", O_RDONLY);
    if (fd == -1) {
        printf("Failed to open /proc/cmdline: %s\n", strerror(errno));
        return -1;
    }

    int res = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (res<0) return 0;

    buf[res] = '\0';
    const char *p, *needle = "fis_act=", *lvl = "loglevel=",
                   *profile = "serviced_profile=";

    // Parse loglevel
    if ((p = strrstr0(buf, lvl)) &&
        sscanf(p + strlen(lvl), "%u", &(g_log_level)) == 1) {
    } else {
        g_log_level = LOG_INFO;
        warn("Unable to find log level, default to 4 (info)\n");
    }

    // Parse active flash image name
    if ((p = strstr(buf, needle))) {
        const char *work = p + strlen(needle);
        char *save = cl->fis_act;
        int bufsiz = sizeof(cl->fis_act) - 1;
        while (bufsiz && *work && *work != ' ') {
            *save++ = *work++;
            bufsiz--;
        }
        *save = '\0';
        info("Active flash image is '%s'\n", cl->fis_act);
    } else {
        cl->fis_act[0] = '\0';
        warn("Unable to find active flash image\n");
    }

    // Parse ServiceD profile
    if ((p = strrstr0(buf, profile)) &&
        sscanf(p + strlen(profile), "%63s", g_switch_profile) == 1) {
    }
    info("Starting MSCC %s profile\n", g_switch_profile);

    if ((p = strstr(buf, "image=mfi"))) {
        info("Image type is 'MFI'\n");
        g_fit_image = false;
    }

    if ((p = strstr(buf, "switch=nomount"))) {
        info("Disabling mounting of switch\n");
        g_nomount_switch = true;
    }

    return 0;
};

static int mount_if_needed(const char *source, const char *target, 
                           const char *filesystemtype, unsigned long mountflags, 
                           const void *data)
{
    struct dirent *dp;
    DIR *dfd;
    int run_count = 0;

    if ((dfd = opendir(target)) == NULL)
    {
        return -1;
    }

    while ((dp = readdir(dfd)) != NULL)
    {
        run_count++;
    }
    closedir(dfd);

    if (run_count <= 2) {
        info("mounted %s at %s as %s\n", source, target, filesystemtype);
        return mount(source, target, filesystemtype, mountflags, data);
    }
    info("%s already populated\n", target);
    return 0;
}


static void basic_linux_system_init(bool do_sysmounts) {
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
    if (do_sysmounts) {
	    DO(mount("proc", "/proc", "proc", 0, 0));
	    DO(mount("sysfs", "/sys", "sysfs", 0, 0));
    }

    // Enable sys-requests - ignore errors as this is a nice-to-have
    fd = open("/proc/sys/kernel/sysrq", O_WRONLY);
    static const char *one = "1\n";
    if (fd != -1) {
        write(fd, one, strlen(one));
        close(fd);
        fd = -1;
    }

#undef DO
#undef CHECK
    return;

ERROR:
    if (fd != -1) close(fd);
    printf("BASIC SYSTEM INIT FAILED!!!\n");
    printf("Function: %s, Line: %d, code: %d\n", __FUNCTION__, line, res);
    printf("errno: %d error: %s\n\n", errno, strerror(errno));
    fflush(stdout);
    fflush(stderr);
    sleep(1);

    reboot(LINUX_REBOOT_CMD_RESTART);
    abort();
}

static bool mtd_device_by_name(const char *name, char *b, size_t l, int *mtd_idx) {
    const int buf_size = 1024 * 1024;
    char buf[buf_size];

    // Open the mtd index
    int fd = open("/proc/mtd", O_RDONLY);
    if (fd == -1) {
        printf("Failed to open /proc/mtd: %s\n", strerror(errno));
        return false;
    }

    char *p = buf;
    int s = buf_size - 1;  // make sure we have room for null termination

    // Read the entire file into the buffer
    while (s) {
        int res = read(fd, p, s);
        if (res == -1 || res == 0) {
            break;
        }

        s -= res;
        p += res;
    }
    (void)close(fd);
    fd = -1;

    // Ensure the buffer is null terminated
    *p = 0;

    // Read the buffer line by line
    char *line_start = buf;
    for (p = buf; *p; ++p) {
        // We have a complete line
        if (*p == '\n') {
            // This is the format which we are expecting:
            // mtd0: 00040000 00040000 "RedBoot"

            *p = 0;  // temporary end the string at the end of line
            int mtd = 0;
            char *s1 = 0, *s2 = 0;

            // Check the line starts with "mtd"
            char *i = line_start;
            if (*i++ != 'm') goto END;
            if (*i++ != 't') goto END;
            if (*i++ != 'd') goto END;

            // Parse the following integer
            mtd = atoi(i);

            // Find the >name< which is enclosed in quotes
            s1 = strchr(line_start, '"');
            if (!s1) goto END;
            s1++;

            s2 = strchr(s1, '"');
            if (!s2) goto END;
            *s2 = 0;

            // Check that the two strings has equal length
            if ((size_t)(s2 - s1) != strlen(name)) goto END;

            // Check if the two string has equal content
            if (memcmp(s1, name, strlen(name)) == 0) {
                // Return the block device
                snprintf(b, l, "/dev/mtd%d", mtd);
                *(b + l - 1) = 0;
                if (mtd_idx) *mtd_idx = mtd;
                return true;
            }

        END:
            // Continue parsing the next line
            *p = '\n';
            line_start = p + 1;
        }
    }

    return false;
}

bool has_part_name(FILE *fp, const char *partname)
{
	char buf[128], lookfor[128];

	snprintf(lookfor, sizeof(lookfor), "PARTNAME=%s", partname);
	while (fgets(buf, sizeof(buf), fp)) {
		buf[strlen(buf)-1] = '\0'; /* Chomp */
		if (strcmp(buf, lookfor) == 0)
			return true;
	}
	return false;
}

char *get_mmc_device(const char *partname)
{
	static char buf[128];
	char fn[128];
	FILE *fp;
	int i;

	for (i = 0; i < 10; i++) {
		snprintf(fn , sizeof(fn), "/sys/block/mmcblk0/mmcblk0p%d/uevent", i);
		if ((fp = fopen(fn, "r"))) {
			if (has_part_name(fp, partname)) {
				snprintf(buf, sizeof(buf), "/dev/mmcblk0p%d", i);
				fclose(fp);
				return buf;
			}
			fclose(fp);
		}
	}
	return NULL;
}

char *get_stage2_device(int *mtd_idx) {
    static char buf[128];

    if (mtd_device_by_name("rootfs_data", buf, 128, mtd_idx) || /* Try with 'nor' mtd name first */
	mtd_device_by_name("rootfs_nand_data", buf, 128, mtd_idx)) { /* Fallback to 'nand' specific name last */
        return buf;
    } else {
        return NULL;
    }
}

int stage2_create_mount_point(char *buf, size_t s, int idx) {
    (void)mkdir(DIR_ROOT_ELEM, 0700);

    if (snprintf(buf, s, DIR_ROOT_ELEM_IX, idx) >= s) {
        return -1;
    }

    if (mkdir(buf, 0700) == -1) {
        warn("Failed to create folder for index %d: %s", idx, strerror(errno));
        return -1;
    }

    return 0;
}

int tar_mount(const char *data, size_t length, int idx) {
#define BUF_SIZE 1024
    char buf[BUF_SIZE];

    if (stage2_create_mount_point(buf, BUF_SIZE, idx) != 0) {
        return -1;
    }

    if (mount("ramfs", buf, "ramfs", 0, 0) == -1) {
        warn("Failed to mount ramfs at %s: %s", buf, strerror(errno));
        return -1;
    }

    if (tar_xz_extract(data, length, buf) < 0) {
        warn("tar_xz_extract failed");
        return -1;
    }

    if (mount("ramfs", buf, "ramfs", MS_REMOUNT | MS_RDONLY, 0) < 0) {
        warn("Failed to remount ramfs at %s: %s", buf, strerror(errno));
        return -1;
    }

    return 0;
#undef BUF_SIZE
}

int squash_mount(int squash_fd, off_t off, size_t size, int idx) {
#define BUF_SIZE 128
    int res, ctrl_fd, loop_idx, loop_fd;
    char mnt_dir[BUF_SIZE];
    char loop_dev[BUF_SIZE];
    struct loop_info64 loop_info = {};

    loop_info.lo_offset = off;
    loop_info.lo_sizelimit = size;

    if (stage2_create_mount_point(mnt_dir, BUF_SIZE, idx) != 0) {
        return -1;
    }

    ctrl_fd = open("/dev/loop-control", O_RDWR);
    if (ctrl_fd < 0) {
        warn("Failed to open /dev/loop-control: %s", strerror(errno));
        return -1;
    }

    loop_idx = ioctl(ctrl_fd, LOOP_CTL_GET_FREE);
    close(ctrl_fd);

    snprintf(loop_dev, BUF_SIZE, "/dev/loop%d", loop_idx);
    loop_fd = open(loop_dev, O_RDWR);
    if (loop_fd < 0) {
        warn("open(%s): %s", loop_dev, strerror(errno));
        return -1;
    }

    FATAL(ioctl(loop_fd, LOOP_SET_FD, squash_fd), "ioctl(LOOP_SET_FD)");
    FATAL(ioctl(loop_fd, LOOP_SET_STATUS64, &loop_info), "ioctl(LOOP_SET_STATUS64)");
    res = mount(loop_dev, mnt_dir, "squashfs", MS_RDONLY, NULL);
    if (res != 0) {
        warn("mount(%s): squashfs tlv %d: %s", loop_dev, idx, strerror(errno));
    }

    close(loop_fd);

    return res;
#undef BUF_SIZE
}

int mount_overlay(int n_elems, const char *mnt_dir)
{
    char *p, options[256 + (n_elems * sizeof(DIR_ROOT_ELEM_IX))];
    int i;

    p = options + sprintf(options, "lowerdir=");
    for (i = n_elems-1; i >= 0; i--) {    // NB reverse order
        if(p[-1] != '=') *p++ = ':';
        p += sprintf(p, DIR_ROOT_ELEM_IX, i);
    }

    debug("mount -t overlay overlay -o %s %s\n", options, mnt_dir);

    return mount("overlay", mnt_dir, "overlay", MS_RDONLY, options);
}

int process_stage2_element_fd(int idx, int type, int fd, int fd_loop, size_t off, size_t len,
                              const char *d) {
    size_t off_max = off + len;
    uint64_t t_start;

    while (off + 8 < off_max) {
        int tlv_type = le32toh(*((int *)d));
        int tlv_length = le32toh(*((int *)(d + 4)));
        const char *data = d + 8;

        if (tlv_length <= 8) {
            debug("Sub-TLV Empty or too small!!! %d\n", tlv_length);
            return -1;
        }

        switch (tlv_type) {
        case TLV_ROOTFS_NAME:
        case TLV_ROOTFS_VERSION:
        case TLV_ROOTFS_LICENSE:
        case TLV_ROOTFS_PREEXEC:
        case TLV_ROOTFS_POSTEXEC:
        case TLV_ROOTFS_FILE_NAME:
            break;

        case TLV_ROOTFS_TARXZ:
            debug("TAR Content: %d\n", tlv_length - 8);
            if (tar_mount(data, tlv_length - 8, idx) < 0) {
                debug("TAR-Extract failed\n");
                return -1;
            }
            break;

        case TLV_ROOTFS_SQUASH:
            t_start = uptime_milliseconds();
            debug("SquashFS Content: %zu, length %d\n", off, tlv_length);
            if (squash_mount(fd_loop, off + 8, tlv_length, idx) < 0) {
                debug("SquashFS mount failed\n");
                return -1;
            }

            t_squash_mount_ms = uptime_milliseconds() - t_start;

            break;

        default:
            debug("Unsupported SUB-TLV: %d\n", tlv_type);
        }

        d += tlv_length;
        off += tlv_length;
    }

    return 0;
}

int process_stage2_fd(int fd, int fd_loop, size_t off, size_t len) {
    int n_tlvs = 0;
    size_t off_data, off_max = off + len;
    uint64_t t_start;

    debug("Process stage2 FD, off %zd, len %zd, max %zd\n", off, len, off_max);

    while (off < off_max) {
        mscc_firmware_vimage_stage2_tlv_t tlvhdr;
        mscc_firmware_vimage_stage2_tlv_t *s2tlv = NULL;

        debug("At offset %zd, index %d\n", off, n_tlvs);

        t_start = uptime_milliseconds();
        if (pread(fd, &tlvhdr, sizeof(tlvhdr), off) != sizeof(tlvhdr)) {
            warn("Read error: size %u at 0x%x - %s", (unsigned)sizeof(tlvhdr),
                 (unsigned)off, strerror(errno));
            return -1;
        }

        t_fs_read_ms += (uptime_milliseconds() - t_start);
        bytes_read += sizeof(tlvhdr);

        if (!mscc_vimage_stage2_check_tlv_basic(&tlvhdr)) {
            debug("Stage2 end at 0x%lx", (long)off);
            break;
        }

        s2tlv = (mscc_firmware_vimage_stage2_tlv_t *)malloc(tlvhdr.tlv_len);
        if (!s2tlv) {
            warn("malloc(%d) fails end at 0x%lx\n", tlvhdr.tlv_len, (long)off);
            return -1;
        }

        debug("Reading TLV of %u bytes @ 0x%08lx", tlvhdr.tlv_len, (long)off);

        t_start = uptime_milliseconds();
        if (pread(fd, s2tlv, tlvhdr.tlv_len, off) != tlvhdr.tlv_len) {
            warn("Read error: size %u at 0x%lx - %s\n", tlvhdr.tlv_len, (long)off,
                 strerror(errno));
            free(s2tlv);
            return -1;
        }

        t_fs_read_ms += (uptime_milliseconds() - t_start);
        bytes_read += tlvhdr.tlv_len;

        // Check TLV (and validate)
        if (!mscc_vimage_stage2_check_tlv(s2tlv, s2tlv->tlv_len)) {
            warn("Stage2 TLV error, extraction aborted\n");
            free(s2tlv);
            return -1;
        }

        off_data = off + offsetof(mscc_firmware_vimage_stage2_tlv_t, value);

        // Validated
        switch (s2tlv->type) {
        case MSCC_STAGE2_TLV_ROOTFS:
            debug("Got rootfs: %d\n", s2tlv->data_len);
            if (process_stage2_element_fd(n_tlvs, s2tlv->type, fd, fd_loop, off_data,
                                          s2tlv->data_len,
                                          (const char *)s2tlv->value) == 0) {
                n_tlvs++;
                info("Read and extracted stage2 TLV, count = %d\n", n_tlvs);
            } else {
                warn("Problem with stage2 TLV, offset %zd, index %d\n", off, n_tlvs);
            }
            break;

        default:;  // Skip silently
        }

        // Cleanup and advance
        free(s2tlv);
        off += tlvhdr.tlv_len;
    }

    info("Extracted %d TLVs\n", n_tlvs);

    return n_tlvs;
}

static int process_stage2_file(const char *f) {
    int fd, res;
    struct stat st;
    off_t off;
    info("Process stage2 file: %s\n", f);

    fd = open(f, O_RDONLY);
    if (fd == -1) {
        warn("Failed to open file: %s %s\n", f, strerror(errno));
        return -1;
    }

    if (fstat(fd, &st) == -1) {
        warn("fstat failed: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    off = mscc_firmware_stage1_get_imglen_fd(fd);

    res = process_stage2_fd(fd, fd, off, st.st_size - off);

    close(fd);

    return res;
}

static int vtss_mtd_blkdev_open(vtss_mtd_t *mtd, int flags) {
    char devname[64];

    snprintf(devname, sizeof(devname), "/dev/mtdblock%d", mtd->devno);

    return open(devname, flags);
}

static int process_stage2_mtd(vtss_mtd_t *mtd) {
    off_t off = mscc_firmware_stage1_get_imglen(mtd);
    int blkdev, res;

    if (!off) {
        warn("No stage1 found in '%s'\n", mtd->dev);
        return -1;
    }

    blkdev = vtss_mtd_blkdev_open(mtd, O_RDONLY | O_SYNC);
    if (blkdev < 0) {
        warn("Unable to open '%s' block device\n", mtd->dev);
        return -1;
    }

    res = process_stage2_fd(mtd->fd, blkdev, off, mtd->info.size - off);

    close(blkdev);

    return res;
}

static int locate_and_process_stage2_mtd(const char *fis_act) {
    int res = -1;
    off_t base;
    vtss_mtd_t mtd;
    char pathname[1024];
    mscc_firmware_sideband_t *sb;

    info("Locating stage2 through use of NOR flash partition '%s'\n", fis_act);
    if (vtss_mtd_open(&mtd, fis_act) != VTSS_OK) {
        fatal(__LINE__, "Failed to open MTD device\n");
    }

    base = mscc_firmware_sideband_get_offset(&mtd);
    if (!base) {
        vtss_mtd_close(&mtd);
        fatal(__LINE__, "No valid firmware data found in flash\n");
        return -1;
    }

    sb = mscc_vimage_sideband_read(&mtd, base);
    if (sb) {
        mscc_firmware_vimage_tlv_t tlv;
        const char *name = (const char *)mscc_vimage_sideband_find_tlv(
                sb, &tlv, MSCC_FIRMWARE_SIDEBAND_STAGE2_FILENAME);

        if (name) {
            // TLV data *is* NULL terminated
            warn("Loading stage2 from NAND file '%s'\n", name);
            (void)snprintf(pathname, sizeof(pathname), "/mnt/stage2/%s", name);
            res = process_stage2_file(pathname);
        } else {
            fatal(__LINE__, "No stage2 filename\n");
        }

        VTSS_FREE(sb);

    } else {
        warn("Loading stage2 from NOR flash partition '%s'\n", fis_act);
        res = process_stage2_mtd(&mtd);
    }

    vtss_mtd_close(&mtd);

    return res;
}

void setup_lo() {
    struct ifreq ifr = {};
    struct sockaddr_in *sa = (struct sockaddr_in *)&ifr.ifr_addr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        warn("socket failed\n");
        return;
    }
    strncpy(ifr.ifr_name, "lo", IFNAMSIZ);
    sa->sin_family = AF_INET;
    sa->sin_port = 0;
    sa->sin_addr.s_addr = inet_addr("127.0.0.1");
    WARN(ioctl(fd, SIOCSIFADDR, &ifr), "SIOCSIFADDR failed");
    WARN(ioctl(fd, SIOCGIFFLAGS, &ifr), "SIOCGIFFLAGS failed");
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    WARN(ioctl(fd, SIOCSIFFLAGS, &ifr), "SIOCSIFFLAGS failed");
    close(fd);
}

int ubi_attach(int mtd_dev, int *ubi_num) {
    int fd, ret;
    struct ubi_attach_req uatt;

    fd = open("/dev/ubi_ctrl", O_RDONLY);
    if (fd < 0) {
        fatal(__LINE__, "No UBI control node");
        return -1;
    }

    memset(&uatt, 0, sizeof(uatt));
    uatt.mtd_num = mtd_dev;
    uatt.ubi_num = -1;		/* Auto assign */

    ret = ioctl(fd, UBI_IOCATT, &uatt);
    if (ret == 0) {
	debug("Attached mtd%d as ubi%d\n", mtd_dev,  uatt.ubi_num);
	*ubi_num = uatt.ubi_num;
    }

    close(fd);

    return ret;
}

#if !defined(STANDALONE)
static void mount_ubifs_mfi(int *p_ubi_num) {
    int mtd_idx;
    int rc;
    char *stage2_device = get_stage2_device(&mtd_idx);
    char rootdev[128];

    if (g_nomount_switch) {
        goto MOUNT_TMP;
    }

    if (!stage2_device) {
        error_(__LINE__, "No rootfs_data partition could be found!");
        goto MOUNT_TMP;
    }

    (void)mkdir("/mnt", 0700);
    info("Using device: %s\n", stage2_device);
    if (ERR(ubi_attach(mtd_idx, p_ubi_num), "Attach of data storage %s failed", stage2_device)) {
        goto MOUNT_TMP;
    }

    // Try to mount the UBIFS rootfs_data partition in the Flash
    snprintf(rootdev, sizeof(rootdev), "ubi%d:%s", *p_ubi_num, "switch");
    rc = ERR(mount(rootdev, "/mnt", "ubifs", MS_SYNCHRONOUS | MS_SILENT, 0),
             "Mount of %s failed", rootdev);
    if (rc) {
        // print out a more helpful error message
        printf("Note: The preceding error message suggests that the UBIFS %s partition\n", stage2_device);
        printf("in the Flash has not been formatted. If you have just flashed a new binary \n");
        printf("firmware image file you need to perform the bootstrap procedure with a suitable\n");
        printf("MFI image to resolve this.\n");
        printf("\n");
        goto MOUNT_TMP;
    }

    info("Mounted %s on %s\n", stage2_device, rootdev);
    return;

MOUNT_TMP:
    FATAL(mount("ramfs", "/mnt", "ramfs", 0,
                "size=8388608,mode=1777"),
          "Failed to mount ramfs in place of ubifs");
}
#endif    // STANDALONE

int main_mfi(CmdLine *c, uint64_t *t_ubifs) {
    int      n_roots_elems = -1;
    uint64_t t_start = 0;
    int      ubi_num;
#if !defined(STANDALONE)
    t_start = uptime_milliseconds();
    mount_ubifs_mfi(&ubi_num);
    *t_ubifs = uptime_milliseconds() - t_start;

    FATAL(mount("ramfs", "/tmp", "ramfs", 0, 0), "Failed to mount /tmp");
    FATAL(mkdir(DIR_STAGE2, 0777), "Failed to create /tmp/stage2");
    FATAL(mount("ramfs", DIR_STAGE2, "ramfs", 0, 0),
          "Failed to mount /tmp/stage2 as ramdisk");

    setup_lo();

    if (strlen(c->fis_act) == 0) {
        warn("Loading stage2 from RAM\n");
        n_roots_elems = process_stage2_ramload("vcfw_uio");
    } else {
        n_roots_elems = locate_and_process_stage2_mtd(c->fis_act);
    }
#else
    g_log_level = LOG_DEBUG;
    if (argc != 2) {
        printf("Usage: %s <mfi-image>\n", argv[0]);
        exit(1);
    }

    info("Mounting MFI image '%s'\n", argv[1]);
    FATAL(mkdir(DIR_STAGE2, 0777), "Failed to create /tmp/stage2");
    FATAL(mount("ramfs", DIR_STAGE2, "ramfs", 0, 0), "Failed to mount /tmp/stage2 as ramdisk");
    n_roots_elems = process_stage2_file(argv[1]);
#endif

    if (n_roots_elems > 0) {
        info("Extracted %d root-tlv elements\n", n_roots_elems);
        FATAL(mkdir(DIR_NEW_ROOT, 0777), "Failed to create " DIR_NEW_ROOT);
        FATAL(mount_overlay(n_roots_elems, DIR_NEW_ROOT), "Failed to mount overlay on " DIR_NEW_ROOT);
    } else {
        fatal(__LINE__, "No root-tlv elements found\n");
    }

#if defined(STANDALONE)
    exit(0);
#endif

    FATAL(chdir(DIR_STAGE2), "Failed to change dir");
    FATAL(pivot_root(DIR_NEW_ROOT, DIR_NEW_ROOT "/mnt"), "pivot_root failed");
    FATAL(chdir("/"), "chdir failed");
    FATAL(mount("/mnt/dev", "/dev", NULL, MS_MOVE, NULL), "Mount move failed");
    FATAL(mount("proc", "/proc", "proc", 0, 0), "Mount failed");
    FATAL(mount("sysfs", "/sys", "sysfs", 0, 0), "Mount failed");

    FATAL(mount("ramfs", "/tmp", "ramfs", 0,
                "size=8388608,mode=1777"),
          "Mount failed");
    FATAL(mount("tmpfd", "/run", "ramfs", 0,
                "size=2097152,mode=1777"),
          "Mount failed");

    (void)mkdir("/dev/pts", 0755);
    FATAL(mount("devpts", "/dev/pts", "devpts", 0, 0), "Mount failed");

    (void)mkdir("/dev/shm", 0755);
    FATAL(mount("shm", "/dev/shm", "ramfs", 0, 0), "Mount failed");

    (void)mkdir("/switch", 0755);

    FATAL(mount("/mnt/mnt", "/switch", NULL, MS_MOVE, NULL),
          "Mount move failed");

    WARN(umount2("/mnt", MNT_DETACH), "Unmount of /mnt failed");
    WARN(mount("none", "/", "ramfs", MS_REMOUNT | MS_RDONLY, 0),
         "Remount R/O of rootfs failed");

#if !defined(STANDALONE)
    char cmd[256];
    sprintf(cmd, "ubihealthd -d /dev/ubi%d", ubi_num);
    WARN(system(cmd), "Failed starting ubihealthd");
#endif

    return 0;
}

#if !defined(STANDALONE)
static void mount_ubifs_fit(void) {
    int mtd_idx;
    int rc;
    char *stage2_device;

    if (g_nomount_switch) {
        goto MOUNT_TMP;
    }

    // Prefer MMC device
    stage2_device = get_mmc_device("Data");

    if (stage2_device) {
        // Try to mount the MMC partition as ext4
        char cmd[200];
        sprintf(cmd, "e2fsck -p %s", stage2_device);
        system(cmd);
        rc = ERR(mount(stage2_device, "/switch", "ext4", MS_NOATIME | MS_SILENT, 0),
                 stage2_device);
        if (rc) {
            // print out a more helpful error message
            printf("Note: Mounting %s as an ext4 filesystem failed.\n", stage2_device);
            printf("If the partion is not already formatted, you need to issue:\n");
            printf("	mkfs.ext4 %s\n", stage2_device);
            printf("NOTE: This will wipe all existing data off the device\n");
            printf("\n");
            goto MOUNT_TMP;
        }
        g_is_ubifs = false;
    } else {
	    char rootdev[128];
	    int ubi_num;
            char sys_filename[64];
            char mtd_type[32];
            int mtd_erasesize;
            int fd;
            char buf[64];

	    /* Try MTD device */
	    stage2_device = get_stage2_device(&mtd_idx);

	    if (!stage2_device) {
		    error_(__LINE__, "No rootfs_data partition could be found!");
		    goto MOUNT_TMP;
	    }

            sprintf(sys_filename, "/sys/block/mtdblock%d/device/type", mtd_idx);
            fd = open(sys_filename, O_RDONLY);
            read(fd, buf, sizeof(buf));
            sscanf(buf, "%s\n", mtd_type);
            close(fd);
            sprintf(sys_filename, "/sys/block/mtdblock%d/device/erasesize", mtd_idx);
            fd = open(sys_filename, O_RDONLY);
            read(fd, buf, sizeof(buf));
            close(fd);
            sscanf(buf, "%d", &mtd_erasesize);
            
            if (mtd_erasesize<16384) {
                sprintf(stage2_device, "/dev/mtdblock%d", mtd_idx);
                // UBIFS does not work with small erase block sizes, use jffs2
                info("Mounting device %s with JFFS2\n", stage2_device);
                rc = ERR(mount(stage2_device, "/switch", "jffs2", MS_SYNCHRONOUS | MS_SILENT, 0),
                         "Mount of %s failed", stage2_device);
                if (rc) {
		    // print out a more helpful error message
		    printf("Note: The preceding error message suggests that the UBIFS %s partition\n", stage2_device);
		    printf("in the Flash has not been formatted. If you have just flashed a new binary \n");
		    printf("firmware image file you need to perform the bootstrap procedure to resolve this.\n");
		    printf("\n");
		    goto MOUNT_TMP;
                }
            } else {

                info("Mounting device %s with UBIFS\n", stage2_device);
                if (ERR(ubi_attach(mtd_idx, &ubi_num), "Attach of data storage %s failed", stage2_device)) {
		    goto MOUNT_TMP;
                }

                // Try to mount the UBIFS rootfs_data partition in the Flash
                snprintf(rootdev, sizeof(rootdev), "ubi%d:%s", ubi_num, "switch");
                rc = ERR(mount(rootdev, "/switch", "ubifs", MS_SYNCHRONOUS | MS_SILENT, 0),
                         "Mount of %s failed", rootdev);
                if (rc) {
		    // print out a more helpful error message
		    printf("Note: The preceding error message suggests that the UBIFS %s partition\n", stage2_device);
		    printf("in the Flash has not been formatted. If you have just flashed a new binary \n");
		    printf("firmware image file you need to perform the bootstrap procedure to resolve this.\n");
		    printf("\n");
		    goto MOUNT_TMP;
                }
                // Start ubihealth
                char cmd[256];
                sprintf(cmd, "ubihealthd -d /dev/ubi%d", ubi_num);
                WARN(system(cmd), "Failed starting ubihealthd");
            }
    }

    info("Mounted %s\n", stage2_device);
    return;

MOUNT_TMP:
    FATAL(mount("ramfs", "/switch", "ramfs", 0,
                "size=8388608,mode=1777"),
          "Failed to mount ramfs in place of ubifs");
}
#endif    // STANDALONE

int main_fit(uint64_t *t_ubifs) {
    uint64_t t_start = 0;

    setup_lo();

    info("About to  mount file systems\n");
    FATAL(mount_if_needed("ramfs", "/tmp", "ramfs", 0,
                          "size=8388608,mode=1777"),
          "Mount failed");
    FATAL(mount("tmpfd", "/run", "ramfs", 0,
                          "size=2097152,mode=1777"),
          "Mount failed");
    (void)mkdir("/run/lock", 0755);
    (void)mkdir("/dev/pts", 0755);
    FATAL(mount_if_needed("devpts", "/dev/pts", "devpts", 0, 0), "Mount failed");
    
    (void)mkdir("/dev/shm", 0755);
    FATAL(mount_if_needed("shm", "/dev/shm", "ramfs", 0, 0), "Mount failed");

    (void)mkdir("/switch", 0755);
    t_start = uptime_milliseconds();
    mount_ubifs_fit();
    *t_ubifs = uptime_milliseconds() - t_start;

    return 0;
}

int main(int argc, char *argv[]) {
    uint64_t read_rate_kbps;
    // Measure time
    uint64_t t_overall[2], t_ubifs;
    bool do_sysmounts;
    struct stat statb;
    int use_non_blocking_console = 0;

    do_sysmounts = (stat("/proc/self", &statb) != 0);
    if (!do_sysmounts) {
	debug("Secondary boot mode detected, not mounting system FS's\n");
    }

    // Certain parts of the init steps is only relevant if we run as PID==1
    if ((int)getpid() == 1) {
        // Performs a couple of basic initializations steps that is not
        // allowed to fail.
        basic_linux_system_init(do_sysmounts);
    }

    FILE *fp;
    if ((fp = fopen("/proc/consoles", "r"))) {
	char buf[128];
	while (fgets(buf, sizeof(buf), fp)) {
            buf[sizeof(buf)-1] = '\0'; /* Chomp */
            if (strstr(buf, "ttyGS") != 0) {
                // The ttyGS driver does not provide information
                // whether a cable is connected, and if no usb cable
                // is connected, the driver will block, causing the
                // entire system to block
                use_non_blocking_console = 1;
                int f = fcntl(1, F_GETFL, 0);
                (void)fcntl(1, F_SETFL, f | O_NONBLOCK );
                f = fcntl(2, F_GETFL, 0);
                (void)fcntl(2, F_SETFL, f | O_NONBLOCK );
            }
	}
    }

    t_overall[0] = uptime_milliseconds();

    info("Stage 1 booted. Starting stage2 boot @ %" PRIu64 " ms\n", t_overall[0]);

    if (strstr(argv[0], "secure")) {
        secure_enforce = true;
        info("Secure mode - all data must be signed\n");
    }

    CmdLine c;
    if (cmdline_read(&c) == -1) {
        fatal(__LINE__, "Failed to read /proc/cmdline\n");
    }

    if (g_fit_image) {
        (void)main_fit(&t_ubifs);
    } else {
        (void)main_mfi(&c, &t_ubifs);
    }

    // Load the random seed (if we have such) - must run after we have a rw
    // filesystem.
    basic_linux_system_init_urandom("/switch/random-seed");

    info("Stage 2 environment ready\n");

    t_overall[1] = uptime_milliseconds();

    if (g_fit_image) {
        warn("Overall: %" PRIu64 " ms, ubifs = %" PRIu64 " ms\n",
             t_overall[1] - t_overall[0], t_ubifs);
    } else {
        if (t_fs_read_ms) {
            read_rate_kbps = ((uint64_t)bytes_read * 1000LLU) / (1024 * t_fs_read_ms);
        } else {
            read_rate_kbps = 0;
        }

        warn("Overall: %" PRIu64 " ms, ubifs = %" PRIu64 " ms,"
             " squash mount: %" PRIu64 " ms,"
             " rootfs %zu bytes read in %" PRIu64 " ms (%" PRIu64 " KiB/s)\n",
             t_overall[1] - t_overall[0], t_ubifs,
             t_squash_mount_ms,
             bytes_read, t_fs_read_ms, read_rate_kbps);
    }

    info("Spawn services\n");
    service_spawn(use_non_blocking_console);

    fatal(__LINE__, "Failed to exec!");

    return 0;
}
