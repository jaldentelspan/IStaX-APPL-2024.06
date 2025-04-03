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

#include "vtss/appl/firmware.h"
#include "firmware.h"
#include "firmware_api.h"
#include "firmware_mfi_info.hxx"
#include "string"
#include "vtss_mtd_api.hxx"
#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
#include <xz.h>
#endif
#include <sys/mman.h>

size_t fd_fsize(int fd)
{
    struct stat st;

    if (!fstat(fd, &st)) {
        return st.st_size;
    }
    return 0;
}

// Same constants as found in the MFI.rb script in mscc-modular-firmware-image project.
constexpr int TLV_ROOTFS_NAME = 1;
constexpr int TLV_ROOTFS_VERSION = 2;
constexpr int TLV_ROOTFS_LICENSE = 3;
constexpr int TLV_ROOTFS_PREEXEC = 4;
constexpr int TLV_ROOTFS_CONTENT = 5;
constexpr int TLV_ROOTFS_POSTEXEC = 6;
constexpr int TLV_ROOTFS_FILENAME = 7;
constexpr int TLV_ROOTFS_SQUASHFS = 8;

// Adds a string attribute to the MFI information map
// In Attr_name : Atrribute name
//    str       : Atttibute value
//    str_len   : Length of the attribute value
//    key       : Pointer to the current key.
//    tlv_map   : Pointer to the map containing the MFI information.
static mesa_rc firmware_tlv_map_str_add(const char *attr_name, const char *str, const u16 str_len,
                                        firmware_info_key_t &key, tlv_map_t &tlv_map) {
    vtss_appl_firmware_image_status_tlv_t image_status;

    BOOL attr_name_len_err = (strlen(attr_name) > VTSS_APPL_FIRMWARE_ATTR_TLV_NAME_LEN - 1);
    BOOL attr_str_len_err  = (str_len > VTSS_APPL_FIRMWARE_ATTR_TLV_STR_LEN -1);

    if (attr_name_len_err) {
        strncpy(image_status.attr_name, "Invalid Length", VTSS_APPL_FIRMWARE_ATTR_TLV_STR_LEN);
    } else {
        strcpy(image_status.attr_name, attr_name);
    }

    if (attr_str_len_err) {
        strncpy(image_status.value.str, "Invalid Length", VTSS_APPL_FIRMWARE_ATTR_TLV_STR_LEN);
    } else {
        strncpy(image_status.value.str, str, str_len);
        image_status.value.str[str_len] = '\0';
    }

    image_status.attr_type = VTSS_APPL_FIRMWARE_STATUS_IMAGE_TLV_TYPE_STR;
    tlv_map.insert ( std::pair<firmware_info_key_t,vtss_appl_firmware_image_status_tlv_t>(key, image_status) );

    key.attribute_id++;

    if (attr_name_len_err || attr_str_len_err) {
        tlv_map[key] = image_status;
        return FIRMWARE_ERROR_IMAGE_MFI_TLV_LEN;
    }

    T_NG(VTSS_TRACE_GRP_INFO,"Adding imageId:%d, %s, %s", key.image_id, attr_name, image_status.value.str);
    T_NG(VTSS_TRACE_GRP_INFO, "ImageId:%d, Section:%d, AttrId:%d",
         key.image_id, key.section_id, key.attribute_id);

    return VTSS_RC_OK;
}

// Starting a new section for the key/map
static void incr_section_id(firmware_info_key_t &key) {
    key.section_id++;
    key.attribute_id = 0;
    T_N("New section id:%d", key.section_id);
}

// Starting a new section for the key/map
static void incr_image_id(firmware_info_key_t &key) {
    key.image_id++;
    key.section_id = 0;
    key.attribute_id = 0;
    T_N("New section id:%d", key.section_id);
}

// Extrcting TLVS from rootfs data.
// In : d - Rootfs data
//      s - Size of rootfs data
//      key - Current key.
//      tlv_map - Pointer to the map to add the rootfs informtion.
static mesa_rc rootfs_extract(const char *d, size_t s, firmware_info_key_t &key, tlv_map_t &tlv_map) {
    const char *e = d + s;
    while (d + 8 < e) {
        int type = le32toh(*((int *)d));
        int length = le32toh(*((int *)(d + 4)));
        const char *data = d + 8;
        u32 data_length = length-8;
        if (length <= 8) {
            T_EG(VTSS_TRACE_GRP_INFO,"Sub-TLV Empty or too small!!! %d", length);
            return FIRMWARE_ERROR_IMAGE_MFI_TLV_TOO_SMALL;
        }
        T_NG(VTSS_TRACE_GRP_INFO, "Type:%d, length:%d", type, length);
        switch (type) {
        case TLV_ROOTFS_NAME:
            T_NG(VTSS_TRACE_GRP_INFO,"AttrId:%d",  key.attribute_id);
            firmware_tlv_map_str_add("Section Name", data, data_length, key, tlv_map);
            break;
        case TLV_ROOTFS_VERSION:
            firmware_tlv_map_str_add("Section Version", data, data_length, key, tlv_map);
            break;
        case TLV_ROOTFS_LICENSE:
            break;
        case TLV_ROOTFS_PREEXEC:
            firmware_tlv_map_str_add("Section Pre-exec", data, data_length, key, tlv_map);
            break;
        case TLV_ROOTFS_POSTEXEC:
            firmware_tlv_map_str_add("Section Post-exec", data, data_length, key, tlv_map);
            break;
        case TLV_ROOTFS_CONTENT:
            T_DG(VTSS_TRACE_GRP_INFO,"Content: %d", data_length);
            break;
        case TLV_ROOTFS_FILENAME:
            firmware_tlv_map_str_add("Section File-name", data, data_length, key, tlv_map);
            break;
        case TLV_ROOTFS_SQUASHFS: {
            std::string squash_len_s;
            squash_len_s = std::to_string(data_length);
            firmware_tlv_map_str_add("Section SQUASHFS contents length", squash_len_s.c_str(), squash_len_s.length(), key, tlv_map);
            break;
        }
        default:
            firmware_tlv_map_str_add("Section Un-supported", data, data_length, key, tlv_map);
            T_DG(VTSS_TRACE_GRP_INFO,"Unsupported SUB-TLV: %d", type);
        }
        d += length;
    }
    incr_section_id(key);

    return VTSS_RC_OK;
}

// Getting mtd name from image id
static mesa_rc firmware_image_id2mtd_name(vtss_appl_firmware_status_image_type_t image_id, char *mtd_name, i32 max_len) {
    mesa_rc rc;
    vtss_mtd_t mtd;
    const char *fis_name;
    switch (image_id) {
    case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER:
        if ( (rc = vtss_mtd_open(&mtd, (fis_name = "RedBoot"))) != VTSS_RC_OK &&
             (rc = vtss_mtd_open(&mtd, (fis_name = "Redboot"))) != VTSS_RC_OK &&
             (rc = vtss_mtd_open(&mtd, (fis_name = "redboot"))) != VTSS_RC_OK) {
            T_DG(VTSS_TRACE_GRP_INFO,"%s (%u) cannot be located", fis_name, image_id);
            return FIRMWARE_ERROR_IMAGE_NOT_FOUND;
        }
        strncpy(mtd_name, fis_name, max_len);
        break;
    case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE:
    case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE:
        fis_name = (image_id == VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE) ? "linux" : "linux.bk";
        if ( (rc = vtss_mtd_open(&mtd, fis_name)) != VTSS_RC_OK) {
            T_DG(VTSS_TRACE_GRP_INFO,"%s (%u) cannot be located", fis_name, image_id);
            return FIRMWARE_ERROR_IMAGE_NOT_FOUND;
        }
        strncpy(mtd_name, fis_name, max_len);
        break;
    default:
        T_DG(VTSS_TRACE_GRP_INFO,"ImageId (%u) not available", image_id);
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

/*
 * Process an open stage 2 image file descriptor. The file descriptor is expected
 * to refer to a state 2 image file located in either NOR or NAND flash.
 */
static void process_stage2_fd(int fd,
                              size_t len,
                              firmware_info_key_t &key,
                              tlv_map_t &tlv_map)
{
    off_t off;
    size_t mapsize = (((len-1)/getpagesize())+1) * getpagesize();
    const uint8_t *map = (uint8_t *) mmap(NULL, mapsize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map != NULL) {
        const char *errmsg = "undefined";
        const mscc_firmware_vimage_t *fw = (mscc_firmware_vimage_t *)map;
        if (mscc_vimage_hdrcheck(fw, &errmsg) == 0) {
            off = fw->imglen;
        } else {
            off = 0;
        }

        size_t s2len = len - off;
        T_DG(VTSS_TRACE_GRP_INFO, "Stage 2 size %zd at offset %jd", s2len, (uintmax_t)off);
        const uint8_t *s2start = map + off;
        const uint8_t *s2ptr = s2start;
        const mscc_firmware_vimage_stage2_tlv_t *s2tlv;
        while(s2len > sizeof(*s2tlv)) {
            s2tlv = (const mscc_firmware_vimage_stage2_tlv_t*) s2ptr;
            T_NG(VTSS_TRACE_GRP_INFO, "Stage2 TLV at %p", s2tlv);
            if (mscc_vimage_stage2_check_tlv(s2tlv, s2len, true)) {
                // Validated
                switch (s2tlv->type) {
                    case MSCC_STAGE2_TLV_ROOTFS:
                        T_DG(VTSS_TRACE_GRP_INFO,"Got rootfs: %d", s2tlv->data_len);
                        rootfs_extract((const char*)s2tlv->value, s2tlv->data_len, key, tlv_map);
                        break;
                    case MSCC_STAGE2_TLV_FILENAME:
                        T_DG(VTSS_TRACE_GRP_INFO,"Got filename: %s", (char *)s2tlv->value);
                        break;
                    case MSCC_STAGE2_TLV_BOOTLOADER:
                    default:
                        ;    // Skip silently
                }
            } else {
                T_DG(VTSS_TRACE_GRP_INFO, "Stage2 TLV error at %p, offset %08zx", s2ptr, s2ptr - s2start);
                break;
            }
            s2ptr += s2tlv->tlv_len;
            s2len -= s2tlv->tlv_len;
        }
        munmap((void*)map, mapsize);
    }
}

/*
 * Open a stage 2 image file located in the file system on the NAND flash.
 */
static int process_stage2_file(const char *f,
                                firmware_info_key_t &key,
                                tlv_map_t &tlv_map)
{
    T_N("Process section2 file: %s", f);
    int fd = open(f, O_RDONLY);
    if (fd < 0) {
        T_E("Failed to open file:%s", f);
        return VTSS_RC_ERROR;
    }

    size_t len = fd_fsize(fd);

    // process the file descriptor
    process_stage2_fd(fd, len, key, tlv_map);

    close(fd);
    return VTSS_RC_OK;
}

/*
 * Open a stage 2 image file located in the indicated MTD on the NOR flash
 */
static mesa_rc process_stage2_mtd(vtss_mtd_t *mtd,
                                  firmware_info_key_t &key,
                                  tlv_map_t &tlv_map)
{
    char bdevname[64];
    snprintf(bdevname, sizeof(bdevname), "/dev/mtdblock%d", mtd->devno);

    int blkfd = open(bdevname, O_RDONLY | O_SYNC);
    if (blkfd < 0) {
        T_E("Unable to open '%s' block device", bdevname);
        return VTSS_RC_ERROR;
    }

    process_stage2_fd(blkfd, mtd->info.size, key, tlv_map);

    close(blkfd);
    return VTSS_RC_OK;
}

static mesa_rc stage2_fw_info_add(firmware_info_key_t &key, tlv_map_t &tlv_map, vtss_mtd_t *mtd) {
    mscc_firmware_vimage_t fw;
    VTSS_RC(mscc_firmware_fw_vimage_get(mtd, &fw));

    firmware_tlv_map_str_add("Machine", fw.machine, strlen(fw.machine), key, tlv_map);
    firmware_tlv_map_str_add("Soc Name", fw.soc_name, strlen(fw.soc_name), key, tlv_map);
    return VTSS_RC_OK;
}

static mesa_rc read_stage2_file(firmware_info_key_t &key, tlv_map_t &tlv_map) {
    mesa_rc rc = VTSS_RC_OK;
    vtss_mtd_t mtd;
    char mtd_name[FIRMWARE_IMAGE_NAME_MAX];
    firmware_image_id2mtd_name((vtss_appl_firmware_status_image_type_t)key.image_id, &mtd_name[0], FIRMWARE_IMAGE_NAME_MAX);

    VTSS_RC(vtss_mtd_open(&mtd, &mtd_name[0]));

    if (vtss_mtd_rootfs_is_nor()) {
        // Stage 2 is in NOR-flash: Open MTD as a block device and scan for information
        VTSS_RC(process_stage2_mtd(&mtd, key, tlv_map));

    } else {
        // Stage 2 is in NAND flash: Find file information in sideband TLV
        char pathname[1024];
        off_t base = mscc_firmware_sideband_get_offset(&mtd);
        VTSS_RC(stage2_fw_info_add(key, tlv_map, &mtd));
        T_DG(VTSS_TRACE_GRP_INFO, "mtd_name:%s, base:%lld", mtd_name, base);
        if (base) {
            mscc_firmware_sideband_t *sb = mscc_vimage_sideband_read(&mtd, base);
            if (sb) {
                mscc_firmware_vimage_tlv_t tlv;
                const char *name;
                if ((name = (const char *) mscc_vimage_sideband_find_tlv(sb, &tlv, MSCC_FIRMWARE_SIDEBAND_STAGE2_FILENAME))) {
                    // TLV data *is* NULL terminated
                    T_N("Name:%s", name);
                    (void) snprintf(pathname, sizeof(pathname), "/switch/stage2/%s", name);
                    rc = process_stage2_file(pathname, key, tlv_map);
                } else {
                    T_I("No stage2 filename data found"); // This is OK. Could be an old image layout with no stage2 image.
                }
                VTSS_FREE(sb);
            } else {
                T_I("No sideband data found");
            }
        } else {
            T_E("No valid firmware data found in flash, name:%s", mtd_name);
        }
    }

    vtss_mtd_close(&mtd);

    return rc;
}

// Adding image information to the MFI information map.
static mesa_rc firmware_image_entry2tlv_map(firmware_info_key_t &key, tlv_map_t &tlv_map) {
    vtss_appl_firmware_status_image_t image_entry;

    T_NG(VTSS_TRACE_GRP_INFO,"Looking for image Id:%d", key.image_id);
    if (vtss_appl_firmware_status_image_entry_get(key.image_id, &image_entry) != VTSS_RC_OK) {
        T_NG(VTSS_TRACE_GRP_INFO,"Image not found Id:%d", key.image_id);
        return VTSS_RC_OK; // No image found - This is OK, just skipping this
    }

    char name[VTSS_APPL_FIRMWARE_NAME_LEN];
    switch (image_entry.type) {
    case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER:
        strcpy(name, "Bootloader");
        break;

    case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE:
        strcpy(name, "Alternative Firmware");
        break;

    case VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE:
        strcpy(name, "Active Firmware");
        break;
    default:
        T_E("Image type:%d unknown", image_entry.type);
        return FIRMWARE_ERROR_IMAGE_TYPE_UNKNOWN;
    }

    firmware_tlv_map_str_add("Image type name", name, strlen(name), key, tlv_map);
    firmware_tlv_map_str_add("Image Name", image_entry.name, strlen(image_entry.name), key, tlv_map);
    firmware_tlv_map_str_add("Image Version", image_entry.version, strlen(image_entry.version), key, tlv_map);
    firmware_tlv_map_str_add("Image Built date", image_entry.built_date, strlen(image_entry.built_date), key, tlv_map);
    firmware_tlv_map_str_add("Image Code revision", image_entry.code_revision, strlen(image_entry.code_revision), key, tlv_map);

    incr_section_id(key);

    if (image_entry.type != VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER) {
        VTSS_RC(read_stage2_file(key, tlv_map));
    }

    incr_image_id(key);
    return VTSS_RC_OK;
}

// Map Iterator
mesa_rc vtss_appl_firmware_status_image_tlv_itr(const u32 *prev_image_id,     u32 *next_image_id,
                                                const u32 *prev_section_id,   u32 *next_section_id,
                                                const u32 *prev_attribute_id, u32 *next_attribute_id) {

    tlv_map_t tlv_map;
    firmware_image_tlv_map_get(&tlv_map);

    firmware_info_key_t key;
    key.image_id     = prev_image_id ? *prev_image_id : 0;
    key.section_id   = prev_section_id ? *prev_section_id : 0;
    key.attribute_id = prev_attribute_id ? *prev_attribute_id + 1 : 0;

    // Check if the key exist
    if (tlv_map.count(key)) {
        goto found;
    }

    key.section_id++;
    key.attribute_id = 0;

    // Check if the key exist
    if (tlv_map.count(key)) {
        goto found;
    }

    key.image_id++;
    key.section_id   = 0;
    key.attribute_id = 0;

    // Check if the key exist
    if (tlv_map.count(key)) {
        goto found;
    }

    T_RG(VTSS_TRACE_GRP_INFO, "Not found: tlv_get, section_id:%d, attribute_id:%d, image_id:%d",
         key.section_id, key.attribute_id, key.image_id);

    return VTSS_RC_ERROR; // Signaling last was reached

found:
    T_RG(VTSS_TRACE_GRP_INFO, "Found: tlv_get, section_id:%d, attribute_id:%d, image_id:%d",
         key.section_id, key.attribute_id, key.image_id);

    *next_image_id     = key.image_id;
    *next_section_id   = key.section_id;
    *next_attribute_id = key.attribute_id;

    return VTSS_RC_OK;
}

// Getting a MFI TLV.
mesa_rc vtss_appl_firmware_status_image_tlv_get(
   u32  image_id,
   u32  section_id,
   u32  attribute_id,
   vtss_appl_firmware_image_status_tlv_t *const data)
{
    T_IG(VTSS_TRACE_GRP_INFO, "tlv_get, section_id:%d, attribute_id:%d, image_id:%d", section_id, attribute_id, image_id);
    firmware_info_key_t key;
    key.image_id = image_id;
    key.section_id =  section_id;
    key.attribute_id = attribute_id;

    tlv_map_t tlv_map;
    firmware_image_tlv_map_get(&tlv_map);

    *data = tlv_map[key];
    return VTSS_RC_OK;
}


// Because it takes quite a while to pass the MFI image and decompress it, we do
// it only once and then keep the result in the below static variables.

// Used to keep track of when the MFI is extracted and ready for use.
// No semaphore protection - only set once.
static BOOL tlv_info_valid = FALSE;

static tlv_map_t tlv_map; // MAP going to contain the MFI information -
                          // OK that it is not semaphore protected, because tlv_map will not change once read.

// Analyze the image in flash and update the maps with the information. Only need to be called once.
static mesa_rc firmware_image_analyze() {
    u32 next_image_id, prev_image_id;
    u32 *prev_image_id_p = NULL; // Start from begining

    firmware_info_key_t key;

    while (vtss_appl_firmware_status_image_entry_itr(prev_image_id_p, &next_image_id) == VTSS_RC_OK) {
        key.section_id   = 0;
        key.attribute_id = 0;
        key.image_id = next_image_id;

        prev_image_id_p = &prev_image_id;
        prev_image_id = next_image_id;

        T_NG(VTSS_TRACE_GRP_INFO, "Next Image ID:%d", next_image_id);

        VTSS_RC(firmware_image_entry2tlv_map(key, tlv_map));
    }

    tlv_info_valid = TRUE;
    return VTSS_RC_OK;
};

// Getting all the MFI TLVs information in a map
// In : tlv_map_i - Pointer to where to put the result.
mesa_rc firmware_image_tlv_map_get(tlv_map_t *tlv_map_i) {
    T_RG(VTSS_TRACE_GRP_INFO, "Getting map");

    // If MFI image is not parsed yet, we need to parse the image, so the information can
    // be added to the map
    if (!tlv_info_valid) {
        T_IG(VTSS_TRACE_GRP_INFO, "Map pt empty");
        VTSS_RC(firmware_image_analyze());
    }

    *tlv_map_i = tlv_map;
    return VTSS_RC_OK;
}

