/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "icli_api.h"
#include "icli_porting_util.h"
#include "firmware_api.h"
#include "firmware_icli_functions.h"
#include "main.h"
#include "firmware.h"
#include "simage_api.h"
#include "firmware_vimage.h"
#include "firmware_api.h"
#include "conf_api.h"
#include "firmware_ubi.hxx"
#include "firmware_mfi_info.hxx"
#include "vtss_os_wrapper_linux.hxx"
#include <unistd.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "vtss_mtd_api.hxx"


/* Help function for printing the image information.
 * In : session_id - For printing error messages
 */
static void display_props(i32 session_id, const char *type, u32 imageNo)
{
    vtss_appl_firmware_status_image_t image_entry;
    char filename[FIRMWARE_IMAGE_NAME_MAX];
    const char *imagetype = NULL;
    mesa_rc rc;

    if ((rc = vtss_appl_firmware_status_image_entry_get(imageNo, &image_entry)) == VTSS_RC_OK) {
        switch (imageNo) {
        case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER:
            imagetype = "bootloader";
            break;
        case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE:
            imagetype = "primary";
            break;
        case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE:
            imagetype = "backup";
            break;
        default:
            imagetype = "unknown";
        }
    } else {
        imagetype = error_txt(rc);
        return;
    }

    if (imagetype) {
        icli_parm_header(session_id, type);

        ICLI_PRINTF("%-17s: %s %s\n", "Image", image_entry.name, (firmware_fis_act() && strcmp(firmware_fis_act(), image_entry.name) == 0) ? "(Active)" : "");
        ICLI_PRINTF("%-17s: %s\n", "Version", image_entry.version);
        ICLI_PRINTF("%-17s: %s\n", "Date", image_entry.built_date);
        if (imageNo != VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER &&
            firmware_image_name_get(image_entry.name,
                                    filename, sizeof(filename)) == VTSS_RC_OK) {
            ICLI_PRINTF("%-17s: %s\n", "Upload filename", filename);
        }
    }
}

//  see firmware_icli_functions.h
void firmware_icli_show_version(i32 session_id)
{
    ICLI_PRINTF("\n");
    display_props(session_id, "Bootloader", VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER);
    ICLI_PRINTF("\n");
    display_props(session_id, "Primary Image", VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE);
    ICLI_PRINTF("\n");
    display_props(session_id, "Backup Image", VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE);
}

// see firmware_icli_functions.h
void firmware_icli_swap_image(i32 session_id)
{



    mesa_restart_t restart_type = MESA_RESTART_COOL;


    control_system_flash_lock();    // Locked until reboot of fail

    if (firmware_swap_images() == VTSS_RC_OK) {
        ICLI_PRINTF("Alternate image activated, now rebooting.\n");
        /* Now restart */
        if (vtss_switch_standalone()) {
            (void) control_system_reset(TRUE, VTSS_USID_ALL, restart_type);
        } else {
            (void) control_system_reset(FALSE, VTSS_USID_ALL, restart_type);
        }
    } else {
        ICLI_PRINTF("Alternate image activation failed.\n");
        control_system_flash_unlock();    // Only unlock if we failed
    }
}

//  see firmware_icli_functions.h
void firmware_icli_upgrade(i32 session_id, const char *url, vtss::remote_file_options_t &transfer_options)
{
    auto fw = manager.get();
    if (fw.ok()) {
        fw->init(nullptr, firmware_max_download);
        fw->attach_cli(cli_get_io_handle());
        if (fw->download(url, transfer_options) == MESA_RC_OK) {
            mesa_rc rc = fw->check();
            if (rc != MESA_RC_OK) {
                ICLI_PRINTF("%s\n", firmware_error_txt(rc));
                return;
            }
            mesa_rc result;
            // This only returns if an error occurred
            if ((result = fw->update(MESA_RESTART_COOL)) != VTSS_RC_OK) {
                ICLI_PRINTF("Error: %s\n", error_txt(result));
            }
        }
    } else {
        ICLI_PRINTF("Error: Firmware update already pending\n");
    }
}

static bool is_bootstrapped(i32 session_id, const char *fis)
{
    vtss_mtd_t mtd;
    bool result = false;

    if (firmware_is_mfi_based() && vtss_mtd_open(&mtd, fis) == VTSS_RC_OK) {
        // Considered as a NOR/NAND configuration (mfi image on MIPS board)
        off_t base = mscc_firmware_sideband_get_offset(&mtd);
        if (base) {
            mscc_firmware_sideband_t *sb = mscc_vimage_sideband_read(&mtd, base);
            if (sb) {
                char pathname[PATH_MAX];
                mscc_firmware_vimage_tlv_t tlv;
                const char *name;
                if ((name = (const char *) mscc_vimage_sideband_find_tlv(sb, &tlv, MSCC_FIRMWARE_SIDEBAND_STAGE2_FILENAME))) {
                    // TLV data *is* NULL terminated
                    (void) snprintf(pathname, sizeof(pathname), "/switch/stage2/%s", name);
                    if (access(name, F_OK)) {
                        T_D("stage2 file %s FOUND", name);
                        result = true;
                    } else {
                        T_D("stage2 file %s does not exist", name);
                    }
                } else {
                    T_D("No stage2 filename tlv found in %s sideband", fis);
                }
                VTSS_FREE(sb);
            } else {
                T_D("Sideband read error for in %s, offset %08llx", fis, base);
            }
        } else {
            T_D("No sideband for in %s", fis);
        }
        vtss_mtd_close(&mtd);
    } else if (firmware_is_nand_only()) {
        char fsname[64];
        auto rc = get_mount_point_source("/", fsname, sizeof(fsname));
        if (rc == VTSS_RC_OK) {
            T_D("%s mounted on '/'", fsname);
        }
        rc = get_mount_point_source("/switch", fsname, sizeof(fsname));
        if (rc == VTSS_RC_OK) {
            T_D("%s mounted on '/switch'", fsname);
        }
    } else if (firmware_is_mmc()) {
        char fsname[64];
        auto rc = get_mount_point_source("/", fsname, sizeof(fsname));
        if (rc == VTSS_RC_OK) {
            T_D("%s mounted on '/'", fsname);
        }
        rc = get_mount_point_source("/switch", fsname, sizeof(fsname));
        if (rc == VTSS_RC_OK) {
            T_D("%s mounted on '/switch'", fsname);
        }
    } else if (!firmware_is_mfi_based()) {
        // NOR-only, not mfi
        char fsname[64];
        auto rc = get_mount_point_source("/", fsname, sizeof(fsname));
        if (rc == VTSS_RC_OK) {
            T_D("%s mounted on '/'", fsname);
        }
        rc = get_mount_point_source("/switch", fsname, sizeof(fsname));
        if (rc == VTSS_RC_OK) {
            T_D("%s mounted on '/switch'", fsname);
        }
    }

    return result;
}

#define SZ_M(n) (1024*1024*n)

/*
Bootstrapping scenarios:
MMC-ONLY:
  There are no MMC-ONLY flash images. Boards with MMC are all running UBOOT. Programming the MMC
  must be done from UBOOT

NAND-ONLY:
  There are no NAND-ONLY flash images. Boards that can run NOR-ONLY are all running UBOOT.
  Programming the NAND must be done from UBOOT

NOR-NAND:
  These systems are traditionally running REDBOOT
  Boards with a small NOR and a large NAND can boot linux from NOR, then mount the NAND and boot the
  application from NAND. Only the NOR can be programmed with a flash programmer. Bringup images are
  therefore NOR-ONLY images with a bootstrap command that can download a full application, format
  the NAND flash, and program the downloaded image partially in NOR (linux kernel) and partially in
  NAND (root file system).

NOR-ONLY:
  These systems can both be running REDBOOT and UBOOT.
  Boards with a large NOR can have the full application including rootfile system in NOR only. Depending
  on the size of the NOR there may only be room for one image in the NOR. The bringup image can
  bootstrap NOR only images by formatting the writeable part of the filesystem, download an application
  and update primary and backup image (if space for a backup image)
 */

// pre-declaration
mesa_rc firmware_icli_bootstrap_ubi(i32 session_id, unsigned int mbytes_limit);
// see firmware_icli_functions.h
void firmware_icli_bootstrap(i32 session_id, const char *url, unsigned int nandsize,
                             bool force, vtss::remote_file_options_t &transfer_options)
{
    const char *mtd_name = firmware_fis_to_update();
    size_t mtdsize;
    if (is_bootstrapped(session_id, firmware_fis_act()) && !force) {
        ICLI_PRINTF("System is already bootstrapped, please use normal firmware upgrade.\n");
    } else if ((mtdsize = firmware_section_size(mtd_name)) == 0) {
        ICLI_PRINTF("Error: MTD name '%s' not found\n", mtd_name);
    } else { // Bootstrap both nand and nor with the corresponding image part
        // Bootstrap nand flash into ubi
        if (firmware_icli_bootstrap_ubi(session_id, nandsize) != MESA_RC_OK) {
            T_I("Flash format failed\n");
        } else {
            auto fw = manager.get();
            if (fw.ok()) {
                fw->init(nullptr, firmware_max_download);
                fw->attach_cli(cli_get_io_handle());
                if (fw->download(url, transfer_options) != MESA_RC_OK) {
                    return;
                }

                mesa_rc rc = fw->check();
                if ( rc != MESA_RC_OK) {
                    ICLI_PRINTF("%s\n", firmware_error_txt(rc));
                    return;
                }

                if (fw->load_nor("linux", "primary") == MESA_RC_OK) {

                    // Determine if we need to do a main image split as well
                    if (mtdsize >= SZ_M(6) &&
                        firmware_section_size("linux.bk") == 0 &&
                        firmware_section_size("split_linux_lo") >= SZ_M(3) &&
                        firmware_section_size("split_linux_hi") >= SZ_M(3) &&
                        !firmware_is_nor_only()) {
                        if (fw->load_nor("split_linux_hi", "backup") != MESA_RC_OK) {
                            return;
                        }
                        if (firmware_fis_split("linux", firmware_section_size("split_linux_lo"),
                                               "linux.bk", firmware_section_size("split_linux_hi")) != MESA_RC_OK) {
                            ICLI_PRINTF("Error: Unable to split primary image.\n");
                            return;
                        }
                    } else {
                        if (firmware_has_alt()) {
                            if (fw->load_nor("linux.bk", "backup") != MESA_RC_OK) {
                                return;
                            }
                        } else {
                            ICLI_PRINTF("Warning: No backup partition detected.\n");
                        }
                    }
                    // reboot only if everything is ok
                    ICLI_PRINTF("Rebooting ...\n");

                    control_system_reset_sync(MESA_RESTART_COOL);
                }
            } else {
                ICLI_PRINTF("Firware update already in progress.\n");
            }
        }
    }
}

mesa_rc firmware_icli_load_image(cli_iolayer_t *io,
                                 const char *mtd_name,
                                 const char *url,
                                 mesa_rc (*checker)(cli_iolayer_t *, const u8 *, size_t),
                                 vtss::remote_file_options_t &transfer_options)
{
    mesa_rc rc = MESA_RC_ERROR;
    size_t mtdsize;
    if ((mtdsize = firmware_section_size(mtd_name)) == 0) {
        cli_io_printf(io, "Error: MTD name '%s' not found\n", mtd_name);
        rc = FIRMWARE_ERROR_UPDATE_NOT_FOUND;
    } else {
        auto fw = manager.get();
        if (fw.ok()) {
            fw->init(nullptr, mtdsize);
            fw->attach_cli(io);
            if ((rc = fw->download(url, transfer_options)) == MESA_RC_OK) {
                if (checker == NULL || ((rc = checker(io, fw->map(), fw->length())) == VTSS_RC_OK )) {
                    rc = fw->load_raw(mtd_name);
                }
            }
            fw->reset();
        } else {
            cli_io_printf(io, "Error: Firmware update already pending\n");
            rc = VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_BUSY;
        }
    }
    return rc;
}

mesa_rc check_bootloader(cli_iolayer_t *io, const u8 *buffer, size_t length)
{
    mesa_rc rc = VTSS_RC_OK;
    u32 imgtype, valid_type = SIMAGE_IMGTYPE_BOOT_LOADER;

    rc = firmware_check_bootloader_simg(buffer, length, &imgtype);
    if (rc == VTSS_RC_OK) {
        if (imgtype != valid_type) {
            cli_io_printf(io, "Image type mismatch (type %d is not bootloader type %d)\n", imgtype, valid_type);
            rc = FIRMWARE_ERROR_WRONG_ARCH;
        }
    } else {
        cli_io_printf(io, "Bad Image: %s\n", error_txt(rc));
    }
    return rc;
}

mesa_rc firmware_icli_bootstrap_ubi(i32 session_id, unsigned int mbytes_limit)
{
    mesa_rc rc = VTSS_RC_OK;
    Firmware_ubi ubi;
    u8 cnt = 0;
    const u8 max_retry = 2;

    if (mbytes_limit > 0) {
        ICLI_PRINTF("Bootstrapping UBIFS with max. %d Mbyte ...\n", mbytes_limit);
    } else {
        ICLI_PRINTF("Bootstrapping UBIFS with all available volume space ...\n");
    }

    // retry once in case of failure.
    do {
        if ((rc = ubi.ubiumount()) != VTSS_RC_OK) {
            ICLI_PRINTF("Umount ubifs failed, num: %d vol: %d.\n", ubi.ubi_num, ubi.vol_id);
            cnt++;
            continue;
        }

        if ((rc = ubi.ubidetach()) != VTSS_RC_OK) {
            ICLI_PRINTF("Detach ubi device failed, num: %d(%d) vol: %d.\n", ubi.ubi_num, ubi.data_mtd.devno, ubi.vol_id);
            cnt++;
            continue;
        }

        if ((rc = ubi.ubiformat()) != VTSS_RC_OK) {
            ICLI_PRINTF("Format ubi device failed, num: %d vol: %d.\n", ubi.ubi_num, ubi.vol_id);
            cnt++;
            continue;
        }

        if ((rc = ubi.ubiattach()) != VTSS_RC_OK) {
            ICLI_PRINTF("Attach ubi device failed, num: %d vol: %d.\n", ubi.ubi_num, ubi.vol_id);
            cnt++;
            continue;
        }

        if ((rc = ubi.ubimkvol(mbytes_limit)) != VTSS_RC_OK) {
            ICLI_PRINTF("Create ubi volume failed, num: %d vol: %d.\n", ubi.ubi_num, ubi.vol_id);
            cnt++;
            continue;
        }

        if ((rc = ubi.ubimount()) != VTSS_RC_OK) {
            ICLI_PRINTF("Mount ubi device failed, num: %d vol: %d.\n", ubi.ubi_num, ubi.vol_id);
            cnt++;
            continue;
        }
    } while ((cnt < max_retry) && (rc != VTSS_RC_OK));

    ICLI_PRINTF("Bootstrap ubi done %s.\n", (rc == VTSS_RC_OK) ? "ok" : "failed");
    return rc;
}

mesa_rc firmware_icli_bootstrap_mmc(i32 session_id, unsigned int mbytes_limit)
{
    mesa_rc rc = VTSS_RC_ERROR;
    const char *mmc_data_dev = get_mmc_device("Data");
    if (!mmc_data_dev) {
        ICLI_PRINTF("No data partition found on mmc. Please partition the mmc from uboot\n");
        return rc;
    }

    int fd = open(mmc_data_dev, O_RDONLY);
    if ( fd < 0 ) {
        ICLI_PRINTF("Did not find %s\n", mmc_data_dev);
        return rc;
    }
    close(fd);
    // Found an mmc partition to be used for application data, check whether it
    // already is mounted
    char mt_point[256];
    if ( get_mount_point(mmc_data_dev, mt_point, sizeof(mt_point)) == VTSS_RC_OK) {
        ICLI_PRINTF("%s already mounted on %s\n", mmc_data_dev, mt_point);
        return VTSS_RC_ERROR;
    }

    // mmc device not mounted, format as ext4
    char sys_cmd[256];
    sprintf(sys_cmd, "mkfs.ext4 -F %s", mmc_data_dev);
    system(sys_cmd);

    return rc;
}

void firmware_icli_bootloader(i32 session_id, const char *url, vtss::remote_file_options_t &transfer_options)
{
    if (conf_is_uboot()) {
        firmware_icli_load_image(cli_get_io_handle(), "UBoot", url, check_bootloader, transfer_options);
    } else {
        firmware_icli_load_image(cli_get_io_handle(), "RedBoot", url, check_bootloader, transfer_options);
    }
}

void firmware_icli_load_fis(i32 session_id,
                            const char *mtd_name,
                            const char *url,
                            vtss::remote_file_options_t &transfer_options)
{
    firmware_icli_load_image(cli_get_io_handle(), mtd_name, url, NULL, transfer_options);
}

static BOOL find_fw_dev(const char *driver, char *iodev, size_t bufsz)
{
    const char *top = "/sys/class/uio";
    DIR *dir;
    struct dirent *dent;
    char fn[PATH_MAX], devname[PATH_MAX];
    FILE *fp;
    BOOL found = FALSE;

    if (!(dir = opendir (top))) {
        perror(top);
        exit (1);
    }

    while ((dent = readdir(dir)) != NULL) {
        if (dent->d_name[0] != '.') {
            snprintf(fn, sizeof(fn), "%s/%s/name", top, dent->d_name);
            if ((fp = fopen(fn, "r"))) {
                const char *rrd = fgets(devname, sizeof(devname), fp);
                fclose(fp);
                if (rrd && (strstr(devname, driver))) {
                    strncpy(iodev, dent->d_name, bufsz);
                    found = TRUE;
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


void firmware_icli_load_ram(i32 session_id, const char *url, vtss::remote_file_options_t &transfer_options)
{
    int dev_fd;
    char uiodev[16], devname[64];
    const char *driver = "vcfw_uio";
    int mmap_size = -1;

    if (!find_fw_dev(driver, uiodev, sizeof(uiodev))) {
        ICLI_PRINTF("Firmware buffer device not available.\n");
        ICLI_PRINTF("Execute 'fconfig -i' or 'fconfig linux_memmap <number>' in Redboot.\n");
        ICLI_PRINTF("Reserve 20 Mb or whatever is suitable for the firmware buffer.\n");
        return;
    }

    /* Open the UIO device file */
    T_D("Using UIO, found '%s' driver at %s", driver, uiodev);
    snprintf(devname, sizeof(devname), "/dev/%s", uiodev);
    dev_fd = open(devname, O_RDWR);
    if (dev_fd < 0) {
        ICLI_PRINTF("Error opening %s: %s\n", devname, strerror(errno));
        return;
    }

    mmap_size = get_uio_map_size(uiodev, 0);
    if (mmap_size <= 0) {
        ICLI_PRINTF("%s: Unable to determine mapping size (%s)\n", uiodev, strerror(errno));
    } else {
        /* mmap the UIO device */
        u8 *fw_mem = (u8 *) mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, 0);
        if (fw_mem != MAP_FAILED) {
            T_D("Mapped register memory @ %p, size %d", fw_mem, mmap_size);
            auto fw = manager.get();
            if (fw.ok()) {
                fw->init(fw_mem, firmware_max_download);
                fw->attach_cli(cli_get_io_handle());
                if (fw->download(url, transfer_options) == MESA_RC_OK) {
                    if (fw->check() == MESA_RC_OK) {
                        CPRINTF("Image is a valid firmware image, do the following in redboot to activate:\n");
                        CPRINTF("ramload\n");
                        (void) control_system_reset(TRUE, VTSS_USID_ALL, MESA_RESTART_COOL);
                    } else {
                        CPRINTF("Image was not valid firmware image, but is kept resident in memory until system reboot.\n");
                    }
                }
            } else {
                ICLI_PRINTF("Error: Firmware update already pending\n");
            }
            (void) munmap(fw_mem, mmap_size);
        } else {
            ICLI_PRINTF("Error: mmap(%s,%d): %s\n", uiodev, mmap_size, strerror(errno));
        }
    }
    close(dev_fd);
}

// Print MFI image information
void firmware_icli_show_info(i32 session_id)
{
    tlv_map_t tlv_map; // Map going to contain the information.
    firmware_image_tlv_map_get(&tlv_map);

    char str_buf[101];
    snprintf(str_buf, sizeof(str_buf), "%-12s %-12s %-12s %-40s %-20s", "ImageId", "SectionId", "Attr.Id", "Name", "Value");
    icli_table_header(session_id, str_buf);

    for (tlv_map_t::iterator it = tlv_map.begin(); it != tlv_map.end(); ++it) {
        ICLI_PRINTF("%-12d %-12d %-12d %-40s %-20s\n",
                    it->first.image_id, it->first.section_id,
                    it->first.attribute_id, it->second.attr_name, it->second.value.str);
    }
    return;
}

