/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#if defined(VTSS_SW_OPTION_FIRMWARE)

#include "main.h"
#include <sys/ioctl.h>

/* As a fundamental util, we need a trace module id that is registered at the
 * very beginning.
 */
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_FIRMWARE
#include "vtss_trace_api.h"

#include "firmware_api.h"
#include "vtss_mtd_api.hxx"

mesa_rc vtss_mtd_open(vtss_mtd_t *mtd, const char* name)
{
    FILE *procmtd;
    char mtdentry[128] = {0};
    char mtdname[32] = {0};

    memset(mtd, 0, sizeof(*mtd));
    mtd->devno = mtd->fd = -1;
    mtd->name = NULL;

    sprintf(mtdname, "\"%s\"", name);
    if ((procmtd = fopen("/proc/mtd", "r"))) {
        while (fgets(mtdentry, sizeof(mtdentry), procmtd)) {
            if (sscanf(mtdentry, "mtd%d:", &mtd->devno) && strstr(mtdentry, mtdname)) {
                snprintf(mtd->dev, sizeof(mtd->dev), "mtd%d", mtd->devno);
                snprintf(mtdentry, sizeof(mtdentry), "/dev/%s", mtd->dev);
                if ((mtd->fd = open(mtdentry, FLAGS)) >= 0) {
                    if (ioctl(mtd->fd, MEMGETINFO, &mtd->info) == 0) {
			mtd->name = name;
                        break;    // Open & info
                    }
                    T_E("NO MTD info from %s", mtdentry);
                    close(mtd->fd);
                }
            }
        }
        fclose(procmtd);
    }

    return (mtd->fd >= 0) ? VTSS_RC_OK : (mesa_rc) FIRMWARE_ERROR_CURRENT_NOT_FOUND;
}

mesa_rc vtss_mtd_erase(const vtss_mtd_t *mtd, size_t length)
{
    struct erase_info_user mtdEraseInfo;
    off_t offset;
    if (length > mtd->info.size) {
        return FIRMWARE_ERROR_SIZE;
    }
    for (offset = 0; offset < length; offset += mtd->info.erasesize) {
        mtdEraseInfo.start = offset;
        mtdEraseInfo.length = mtd->info.erasesize;
        (void) ioctl(mtd->fd, MEMUNLOCK, &mtdEraseInfo);
        if (ioctl (mtd->fd, MEMERASE, &mtdEraseInfo) < 0) {
            T_E("%s: Erase error at 0x%08zx: %s", mtd->dev, (size_t) offset, strerror(errno));
            return FIRMWARE_ERROR_FLASH_ERASE;
        }
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_mtd_program(const vtss_mtd_t *mtd, const u8 *buffer, size_t length)
{
    size_t written, hdrsize = (1 * 1024);

    if (length > hdrsize) {
        // Write payload first, *then* header
        T_D("%s: Write %zd (data) then %zd (header)", mtd->dev, length - hdrsize, hdrsize);
        if ((written = pwrite(mtd->fd, buffer + hdrsize, length - hdrsize, hdrsize)) == (length - hdrsize)) {
            T_D("%s: Data: Wrote len %zd", mtd->dev, written);
            written += pwrite(mtd->fd, buffer, hdrsize, 0);
            T_D("%s: Header: Wrote len %zd (total)", mtd->dev, written);
        }
    } else {
        T_D("%s: Write len %zd", mtd->dev, length);
        written = pwrite(mtd->fd, buffer, length, 0);
    }

    if (written != length) {
        T_E("%s: Write error len %zd: %s\n", mtd->dev, length, strerror(errno));
        return FIRMWARE_ERROR_FLASH_PROGRAM;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_mtd_program_pos(const vtss_mtd_t *mtd, const u8 *buffer, size_t length, off_t pos)
{
    size_t written;

    T_D("%s: Write len %zd @ %ld", mtd->dev, length, pos);
    written = pwrite(mtd->fd, buffer, length, pos);

    if (written != length) {
        T_E("%s: Write error len %zd: %s\n", mtd->dev, length, strerror(errno));
        return FIRMWARE_ERROR_FLASH_PROGRAM;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_mtd_close(vtss_mtd_t *mtd)
{
    return close(mtd->fd) ? VTSS_RC_ERROR : VTSS_RC_OK;
}

bool vtss_mtd_is_nor(const char *mtd_name)
{
    vtss::StringStream ss;
    int mfd;
    char type[16] = { 0 };
    print_fmt(ss, "/sys/class/mtd/%s/type", mtd_name);
    if ((mfd = open(ss.cstring(), O_RDONLY)) >= 0) {
        ssize_t nread = read(mfd, type, sizeof(type) - 1);
        if (nread > 0) {
            if (type[nread-1] == '\n') {
                type[nread-1] = '\0';    // terminate
            } else {
                type[nread] = '\0';    // terminate
            }
        } else if (nread == -1) {
            T_E("%s: read error: %d (%s)", mtd_name, errno, std::strerror(errno));
        }
        close(mfd);
    }
    T_D("%s = %s", ss.cstring(), type);
    return strncmp(type, "nor", sizeof(type)) == 0;
}

bool vtss_mtd_rootfs_is_nor()
{
    vtss_mtd_t mtd;
    bool ret = false;
    if (vtss_mtd_open(&mtd, "rootfs_data") == VTSS_RC_OK) {
        ret = vtss_mtd_is_nor(mtd.dev);
        vtss_mtd_close(&mtd);
    }
    T_D("%s", ret ? "nor" : "(nand)");
    return ret;
}

#endif
