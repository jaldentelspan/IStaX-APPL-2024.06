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

#include "main.h"

#if defined(CYGPKG_FS_RAM) && defined(VTSS_SW_OPTION_ICFG)
#include "os_file_api.h"
#include "critd_api.h"

#include "vtss_trace_api.h"
#include "conf_api.h"
#include "misc_api.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#if defined(CYGPKG_FS_RAM)
#include <sys/ioctl.h>
#include <sys/sockio.h>
#else
#include "vtss_os_wrapper.h"
#endif /* CYGPKG_FS_RAM */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SYSTEM /* Pick one... */
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SYSTEM /* Pick one... */

// Maximum size for our conf block in bytes. Since actual conf flash writing is
// async relative to writing the conf RAM block we can't know if the sum of
// all system conf data will fit in flash until after we've committed it
// to conf. If data doesn't fit in flash we get inconsistencies between conf RAM
// and conf flash. So this number has a heuristic base that's intended to leave
// enough flash space for other conf users.
// (The point is that all files written via ICFG are compressed in advance,
// meaning that the sum of those files is a pretty good approximation of the
// actual flash consumption.)
#define MAX_CONF_BLOCK_SIZE     (384*1024)

typedef struct {
    char   name[NAME_MAX];
    size_t size;
    time_t mtime;
} file_meta_t;

typedef struct {
    unsigned long version;      /* Block version */
#define OSFILE_CONF_VERSION 3

    /* Metadata for r/w files */
    file_meta_t meta[OS_FILE_FILES_MAX];
    u8 data[];
} file_blk_flash_t;

typedef struct {
    unsigned long version;      /* Block version */

    /* Metadata for r/w files */
    file_meta_t meta[4];
    u8 data[];
} file_blk_flashv2_t;

static vtss_mutex_t         osf_mutex;  /* Global module/API protection */

#define FILE_LOCK()         (void) vtss_mutex_lock(&osf_mutex)
#define FILE_UNLOCK()       vtss_mutex_unlock(&osf_mutex)

static void os_file_readdata(file_blk_flash_t *flash, int nfiles)
{
    int i;
    u8 *ptr = &flash->data[0];
    for (i = 0; i < nfiles; i++) {
        int fd = open(flash->meta[i].name, O_RDONLY, 0);
        ssize_t act_read;
        if (fd >= 0) {
            if ((act_read = read(fd, ptr, flash->meta[i].size)) >= 0) { /* In one scoop */
                ptr += act_read;
                if (flash->meta[i].size < (size_t) act_read) {
                    T_W("%s: File shrunk from %zd to %ld", flash->meta[i].name, flash->meta[i].size, act_read);
                    flash->meta[i].size = act_read;
                }
            }
            (void) close(fd);
        } else {
            T_W("%s: %s - skipping file", flash->meta[i].name, strerror(errno));
            memset(&flash->meta[i], 0, sizeof(flash->meta[i]));
        }
    }
}

static void os_file_writefile(const char *name, const u8 *buf, size_t fsize, size_t mtime)
{
    int fd;
    const char *str;

    /* skip any path in filename */
    if ((str = strrchr(name, '/')) != NULL) {
        name = (str + 1);
    }

    fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    if (fd < 0 ) {
        T_E("%s: %s", name, strerror(errno));
        return;
    }

    T_I("%s: %zd bytes", name, fsize);

    if (fsize) {
        ssize_t wrote = write(fd, buf, fsize);
        if (wrote < 0) {
            T_E("%s: %s (%zd bytes) ", name, strerror(errno), fsize);
        } else {
            if (wrote != fsize) {
                T_E("%s: Requested %zd, wrote %lu", name, fsize, wrote);
            }
        }
    }

#if defined(CYGPKG_FS_RAM)
    /* Hack In absence of utime() */
    if (ioctl(fd, SIOCSHIWAT, mtime) < 0) {
        T_E("%s: %s", name, strerror(errno));
    }
#endif /* CYGPKG_FS_RAM */

    if (close(fd) < 0) {
        T_E("%s: %s", name, strerror(errno));
    }
}

/*
 * Externally visible API
 */

mesa_rc os_file_fs2flash(void)
{
    const char *dirname = VTSS_ICFG_PATH;
    int err, files;
    DIR *dirp;
    struct dirent *entry;
    file_blk_flash_t *flash = (file_blk_flash_t *)VTSS_MALLOC(sizeof(*flash)), *nflash;
    size_t size = sizeof(*flash);
    mesa_rc rc = VTSS_RC_ERROR;

    if (!flash) {
        T_E("Unable to persist data struct, size %zd", size);
        goto out;
    }

    T_D("Reading directory %s", dirname);

    dirp = opendir(dirname);
    if (dirp == NULL) {
        T_E("opendir(%s): %s", dirname, strerror(errno));
        goto out;
    }

    memset(flash, 0, sizeof(*flash));
    flash->version = OSFILE_CONF_VERSION;

    for (files = 0; files < OS_FILE_FILES_MAX && (entry = readdir(dirp)) != NULL; ) {
        struct stat sbuf;
        err = stat(entry->d_name, &sbuf);
        if (err < 0) {
            if (errno == ENOSYS) {
                T_E("%s: <no status available>", entry->d_name);
            } else {
                T_E("%s: %s", entry->d_name, strerror(errno));
            }
        } else {
            // Write regular file, unless it's 'default-config' / 'crashfile'
            if (S_ISREG(sbuf.st_mode) &&
                strcmp(entry->d_name, "default-config") != 0 &&
                strcmp(entry->d_name, "crashfile") != 0) {
                T_D("File %s: %lu bytes", entry->d_name, sbuf.st_size);
                misc_strncpyz(flash->meta[files].name, entry->d_name, sizeof(flash->meta[files].name));
                flash->meta[files].size = sbuf.st_size;
                flash->meta[files].mtime = sbuf.st_mtime;
                size += sbuf.st_size;
                files++;
            }
        }
    }


    err = closedir(dirp);
    if (err < 0) {
        T_W("closedir: %s", strerror(errno));
    }

    if (size > MAX_CONF_BLOCK_SIZE) {
        T_I("Sum of files too large for conf block; not updating flash");
        goto out;
    }

    FILE_LOCK();
    if ((nflash = (file_blk_flash_t *)conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_OS_FILE_CONF, size))) {
        /* Save metadata, version */
        memcpy(nflash, flash, sizeof(*flash));

        /* Read file contents to data section */
        os_file_readdata(nflash, files);

        /* Sync data to flash (compressed); flush initiates immediate sync. */
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_OS_FILE_CONF);
        conf_flush();
        rc = VTSS_RC_OK;
    } else {
        T_E("Unable to store persistent filedata, %zd bytes", size);
    }
    FILE_UNLOCK();

out:
    if (flash) {
        VTSS_FREE(flash);
    }

    return rc;
}

#ifndef CYGPKG_FS_RAM
static BOOL
_os_file_fs_file_copy(
    const char      *const dst,
    const char      *const src
)
{
    int         ifd, ofd;
    ssize_t     nrd;
    struct stat fst;
    char        buf[1024];
    BOOL        ret = FALSE;

    if (!dst || !src || stat(src, &fst) < 0) {
        return ret;
    }

    if ((ifd = open(src, O_RDONLY)) < 0) {
        T_E("Open %s: %s", src, strerror(errno));
        return ret;
    }
    if ((ofd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH))) < 0) {
        T_E("Open %s: %s", dst, strerror(errno));

        if (close(ifd) < 0) {
            T_E("Close %s: %s", src, strerror(errno));
        }

        return ret;
    }

    ret = TRUE;
    while ((nrd = read(ifd, buf, sizeof(buf))) > 0) {
        if (write(ofd, buf, nrd) != nrd) {
            T_E("Write %s: %s", dst, strerror(errno));
            ret = FALSE;
            break;
        }
    }

    if (ret && nrd < 0) {
        T_E("Read %s: %s", src, strerror(errno));
        ret = FALSE;
    }

    if (close(ifd) < 0) {
        T_E("Close %s: %s", src, strerror(errno));
        ret = FALSE;
    }
    if (close(ofd) < 0) {
        T_E("Close %s: %s", dst, strerror(errno));
        ret = FALSE;
    }

    return ret;
}
#endif /* !CYGPKG_FS_RAM */

void os_file_flash2fs(void)
{
    u32 *blkptr;
    ulong size;
    FILE_LOCK();
    if ((blkptr = (u32 *) conf_sec_open(CONF_SEC_GLOBAL, CONF_BLK_OS_FILE_CONF, &size))) {
        switch (blkptr[0]) {
        case OSFILE_CONF_VERSION: {
            file_blk_flash_t *blk = (file_blk_flash_t *) blkptr;
            uint i;
            u8 *ptr = &blk->data[0];
            T_I("Config block v3 %u bytes", size);
            for (i = 0; i < ARRSZ(blk->meta); i++)  {
                if (blk->meta[i].name[0]) {
                    os_file_writefile(blk->meta[i].name, ptr, blk->meta[i].size, blk->meta[i].mtime);
                    ptr += blk->meta[i].size;
                } else {
                    T_D("%d: Null entry", i);
                }
            }
        }
        break;
        case 2: { /* Older config version - migrate it */
            file_blk_flashv2_t *blk = (file_blk_flashv2_t *) blkptr;
            uint i;
            u8 *ptr = &blk->data[0];
            T_I("Config block v2 %u bytes", size);
            for (i = 0; i < ARRSZ(blk->meta); i++)  {
                if (blk->meta[i].name[0]) {
                    os_file_writefile(blk->meta[i].name, ptr, blk->meta[i].size, blk->meta[i].mtime);
                    ptr += blk->meta[i].size;
                } else {
                    T_D("%d: Null entry", i);
                }
            }
        }
        break;
        default:
            T_E("%d: Unrecognized file data block - ignored", blkptr[0]);
        }
#if defined(CYGPKG_FS_RAM)
    }
#else
    } else
    {
        size_t              new_sz = sizeof(file_blk_flash_t);
        file_blk_flash_t    *new_ptr = (file_blk_flash_t *)VTSS_MALLOC(new_sz);

        if (new_ptr != NULL) {
            char    p[VTSS_ICONF_FILE_NAME_LEN_MAX], q[VTSS_ICONF_FILE_NAME_LEN_MAX], *dirname = VTSS_FS_FILE_DIR;
            DIR     *dirp;

            memset(new_ptr, 0x0, new_sz);
            new_ptr->version = OSFILE_CONF_VERSION;

            memset(p, 0x0, sizeof(p));
            if (snprintf(p, sizeof(p), "%s%s%s",
                         VTSS_FS_FILE_DIR, VTSS_ICONF_FILE_NAME_PREFIX, "startup-config") < 1) {
                T_W("snprintf(Startup): %s", strerror(errno));
            }
            memset(q, 0x0, sizeof(q));
            if (snprintf(q, sizeof(q), "%s%s%s",
                         VTSS_FS_FILE_DIR, VTSS_ICONF_FILE_NAME_PREFIX, "default-config") < 1) {
                T_W("snprintf(Default): %s", strerror(errno));
            }

            if (_os_file_fs_file_copy(p, q) &&
                (dirp = opendir(dirname)) != NULL) {
                int             files, cmp_len = strlen(VTSS_ICONF_FILE_NAME_PREFIX);
                struct dirent   *entry;
                char            fullname[VTSS_ICONF_FILE_NAME_LEN_MAX];
                struct stat     sbuf;

                for (files = 0; files < OS_FILE_FILES_MAX && (entry = readdir(dirp)) != NULL; ) {
                    if (strncmp(entry->d_name, VTSS_ICONF_FILE_NAME_PREFIX, cmp_len)) {
                        continue;
                    }

                    memset(fullname, 0x0, sizeof(fullname));
                    strcpy(fullname, dirname);
                    strcat(fullname, entry->d_name);
                    if (stat(fullname, &sbuf) < 0) {
                        if (errno == ENOSYS) {
                            T_E("%s: <no status available>", fullname);
                        } else {
                            T_E("Stat %s: %s", fullname, strerror(errno));
                        }
                    } else {
                        // Write regular file, unless it's 'default-config' / 'crashfile'
                        if (S_ISREG(sbuf.st_mode) &&
                            strcmp(&entry->d_name[cmp_len], "default-config") != 0 &&
                            strcmp(&entry->d_name[cmp_len], "crashfile") != 0) {
                            misc_strncpyz(new_ptr->meta[files].name, fullname, sizeof(new_ptr->meta[files].name));
                            new_ptr->meta[files].size = sbuf.st_size;
                            new_ptr->meta[files].mtime = sbuf.st_mtime;
                            new_sz += sbuf.st_size;
                            files++;
                        }
                    }
                }

                if (closedir(dirp) < 0) {
                    T_W("closedir: %s", strerror(errno));
                } else {
                    if ((blkptr = (u32 *)conf_sec_create(CONF_SEC_GLOBAL, CONF_BLK_OS_FILE_CONF, new_sz)) != NULL) {
                        memset((u8 *)blkptr, 0x0, new_sz);
                        memcpy((u8 *)blkptr, (u8 *)new_ptr, sizeof(file_blk_flash_t));

                        /* Read file contents to data section */
                        os_file_readdata((file_blk_flash_t *)blkptr, files);

                        /* Sync data to flash (compressed); flush initiates immediate sync. */
                        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_OS_FILE_CONF);
                        conf_flush();
                        blkptr = NULL;
                    } else {
                        T_E("Failed to access CONF_BLK_OS_FILE_CONF!");
                    }
                }
            } else {
                T_E("Unable to prepare startup-config");
            }

            VTSS_FREE(new_ptr);
        } else {
            T_E("Unable to persist data struct, size %zd", size);
        }
    }

    if (blkptr)
    {
        conf_sec_close(CONF_SEC_GLOBAL, CONF_BLK_OS_FILE_CONF);
    }
#endif /* CYGPKG_FS_RAM */
    FILE_UNLOCK();
}

mesa_rc os_file_init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_INIT:
        vtss_mutex_init(&osf_mutex);
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        os_file_flash2fs();
        break;

    case INIT_CMD_CONF_DEF:
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}
#endif  /* CYGPKG_FS_RAM && VTSS_SW_OPTION_ICFG */

