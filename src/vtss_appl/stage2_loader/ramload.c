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

#include <unistd.h>
#include <dirent.h>
#include <sys/mman.h>
#include <errno.h>

#include "main.h"
#include "tar.h"
#include "mfi.h"

#include "firmware_vimage.h"

static bool find_fw_dev(const char *driver, char *iodev, size_t bufsz)
{
    const char *top = "/sys/class/uio";
    DIR *dir;
    struct dirent *dent;
    char fn[PATH_MAX], devname[PATH_MAX];
    FILE *fp;
    bool found = false;

    if (!(dir = opendir (top))) {
        perror(top);
        exit (1);
    }

    while((dent = readdir(dir)) != NULL) {
        if (dent->d_name[0] != '.') {
            snprintf(fn, sizeof(fn), "%s/%s/name", top, dent->d_name);
            if ((fp = fopen(fn, "r"))) {
                const char *rrd = fgets(devname, sizeof(devname), fp);
                fclose(fp);
                if (rrd && (strstr(devname, driver))) {
                    strncpy(iodev, dent->d_name, bufsz);
                    found = true;
                    break;
                }
            }
        }
    }

    closedir(dir);

    return found;
}

static int get_uio_map_size(const char *device, int map_no)
{
    const char *top = "/sys/class/uio";
    char fn[PATH_MAX];
    FILE *fp;
    int val = -1;

    snprintf(fn, sizeof(fn), "%s/%s/maps/map%d/size", top, device, map_no);
    if ((fp = fopen(fn, "r"))) {
        if (fscanf(fp, "%i", &val) != 1) {
            T_I("Unable to read size of %s mapping %d", device, map_no);
        }
        fclose(fp);
    }

    return val;
}

int process_stage2_ramload(const char *uio_driver)
{
    int res = -1;
    int dev_fd;
    char uiodev[16], devname[64];
    void *fw_mem;
    int mmap_size = -1;

    info("No active flash image, trying ramload from '%s'\n", uio_driver);

    if(!find_fw_dev(uio_driver, uiodev, sizeof(uiodev))) {
        warn("Firmware buffer device not available.\n");
        warn("Execute 'fconfig -i' or 'fconfig linux_memmap <number>' in Redboot.\n");
        warn("Reserve 16 Mb or whatever is suitable for the firmware buffer.\n");
        return -1;
    }

    /* Open the UIO device file */
    T_D("Using UIO, found '%s' driver at %s", uio_driver, uiodev);
    snprintf(devname, sizeof(devname), "/dev/%s", uiodev);
    dev_fd = open(devname, O_RDWR);
    if (dev_fd < 0) {
        warn("Error opening %s: %s\n", devname, strerror(errno));
        return -1;
    }

    mmap_size = get_uio_map_size(uiodev, 0);
    if (mmap_size <= 0) {
        warn("%s: Unable to determine mapping size\n", uiodev);
        goto out_close;
    }

    /* mmap the UIO device */
    fw_mem = mmap(NULL, mmap_size, PROT_READ|PROT_WRITE, MAP_SHARED, dev_fd, 0);
    if(fw_mem != MAP_FAILED) {
        mscc_firmware_vimage_t *fw = (mscc_firmware_vimage_t *) fw_mem;
        const char *msg;
        if (mscc_vimage_hdrcheck(fw, &msg) == 0) {
            mscc_firmware_vimage_tlv_t tlv;
            u8 *tlvdata;
            // Authenticate
            if ((tlvdata = mscc_vimage_find_tlv(fw, &tlv, MSCC_STAGE1_TLV_SIGNATURE))) {
                if (!mscc_vimage_validate(fw, &tlv, tlvdata)) {
                    warn("Validation fails: %s\n", msg);
                    goto out_unmap;
                }
            } else {
                warn("Invalid image: Signature missing");
                goto out_unmap;
            }
            size_t s2len = mmap_size - fw->imglen;
            const char *s2ptr = fw_mem + fw->imglen;
            const mscc_firmware_vimage_stage2_tlv_t *s2tlv;
#define STAGE2_RAM "/tmp/s2ram"
            int s2fd = open(STAGE2_RAM, O_RDWR | O_CREAT | O_TRUNC, 0666);
            if (s2fd < 0) {
                warn("open(%s): %s\n", STAGE2_RAM, strerror(errno));
                goto out_unmap;
            }
            while(s2len > sizeof(*s2tlv)) {
                s2tlv = (const mscc_firmware_vimage_stage2_tlv_t*) s2ptr;
                T_D("Stage2 TLV at %p", s2tlv);
                if (mscc_vimage_stage2_check_tlv(s2tlv, s2len)) {
                    // Validated
                    switch (s2tlv->type) {
                        case MSCC_STAGE2_TLV_ROOTFS:
                            debug("Save rootfs tlv: %d\n", s2tlv->tlv_len);
                            write(s2fd, (const char*)s2tlv, s2tlv->tlv_len);
                            break;
                        default:
                            ;    // Skip silently
                    }
                } else {
                    warn("Stage2 ends at %p, offset %08zx\n", s2ptr, s2ptr - (char *)fw_mem);
                    break;
                }
                s2ptr += s2tlv->tlv_len;
                s2len -= s2tlv->tlv_len;
            }
            res = process_stage2_fd(s2fd, s2fd, 0, lseek(s2fd, 0, SEEK_CUR));
            close(s2fd);
        } else {
            warn("ramload: %s\n", msg);
        }
  out_unmap:
        munmap(fw_mem, mmap_size);
    } else {
        warn("mmap(%s,%d): %s\n", uiodev, mmap_size, strerror(errno));
    }

out_close:
    close(dev_fd);

    return res;
}
