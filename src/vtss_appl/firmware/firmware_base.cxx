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

#include "main.h"
#include "firmware.h"
#include "msg_api.h"
#include "conf_api.h"

#include "vtss_tftp_api.h"

#include "vtss_os_wrapper.h"
#include "vtss_hostaddr.h"
#include "vtss_uboot.hxx"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "standalone_api.h"

extern vtss_appl_firmware_control_image_upload_t g_image_upload;

static std::string firmware_status = "idle";

static vtss_trace_reg_t trace_reg =
{
    VTSS_TRACE_MODULE_ID, "firmware", "Firmware Update"
};

static vtss_trace_grp_t trace_grps[] =
{
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_INFO] = {
        "info",
        "Info",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/*
 * Return the filename of the indicated stage 2 .mfi image file in NOR flash
 */
mesa_rc firmware_get_stage2_nor_filename(const char *fis_name,
                                         char *buffer,
                                         size_t buflen)
{
    mesa_rc rc = VTSS_RC_ERROR;
    vtss_mtd_t mtd;

    off_t off;
    int n_tlvs = 0;
    size_t length;
    size_t off_max;

    buffer[0] = '\0';    // Be sure we always have valid output
    
    if ((rc = vtss_mtd_open(&mtd, fis_name)) != VTSS_RC_OK) {
        return rc;
    }

    mscc_firmware_vimage_t fw;
    if ((rc = mscc_firmware_fw_vimage_get(&mtd, &fw)) != VTSS_RC_OK) {
        goto EXIT;
    }

    const char *msg;
    if (mscc_vimage_hdrcheck(&fw, &msg) != 0) {
        T_D("Invalid image: %s", msg);
        rc = FIRMWARE_ERROR_INVALID;
        goto EXIT;
    }

    off = fw.imglen;
    off_max = mtd.info.size;
    length = off_max - off;

    T_D("Process stage2 FD, off %zd, len %zd, max %zd", off, length, off_max);

    mscc_firmware_vimage_stage2_tlv_t tlvhdr;

    rc = VTSS_RC_ERROR;

    while (off < off_max) {
        T_D("At offset %zd, index %d", off, n_tlvs);

        if (pread(mtd.fd, &tlvhdr, sizeof(tlvhdr), off) != sizeof(tlvhdr)) {
            T_W("Read error: size %u at 0x%x - %s", (unsigned)sizeof(tlvhdr),
                 (unsigned)off, strerror(errno));
            rc = FIRMWARE_ERROR_INVALID;
            goto EXIT;
        }

        if (!mscc_vimage_stage2_check_tlv(&tlvhdr, length, false)) {
            T_D("Stage2 end at 0x%lx", (long)off);
            break;
        }

        T_D("TLV header %d read, magic:%08x, type:%u", n_tlvs, tlvhdr.magic1, tlvhdr.type);
        
        if (tlvhdr.type == MSCC_STAGE2_TLV_FILENAME) {
            if (tlvhdr.data_len < buflen) {
                if (pread(mtd.fd, buffer, tlvhdr.data_len, off + sizeof(mscc_firmware_vimage_stage2_tlv_t)) != tlvhdr.data_len) {
                    T_W("Read error: size %u at 0x%x - %s", (unsigned)sizeof(tlvhdr),
                         (unsigned)off, strerror(errno));
                    rc = FIRMWARE_ERROR_INVALID;
                    break;
                }

                buffer[tlvhdr.data_len] = '\0';
                rc = VTSS_RC_OK;
            } else {
                T_W("TLV filename buffer too small, needs %d, has %d", tlvhdr.data_len, buflen);
            }
            break;
        }

        length -= tlvhdr.tlv_len;
        n_tlvs++;
        off += tlvhdr.tlv_len;
    }

EXIT:
    (void)vtss_mtd_close(&mtd);
    return rc;
}

mesa_rc firmware_sideband_by_type_get(const char *fis_name,
                                      mscc_firmware_image_sideband_tlv_t type,
                                      char *buffer, size_t buflen)
{
    mesa_rc rc = VTSS_RC_ERROR;
    mesa_rc rc2;
    vtss_mtd_t mtd;
    if ((rc2 = vtss_mtd_open(&mtd, fis_name)) == VTSS_RC_OK) {
        off_t base = mscc_firmware_sideband_get_offset(&mtd);
        if (base) {
            mscc_firmware_sideband_t *sb = mscc_vimage_sideband_read(&mtd, base);
            if (sb) {
                mscc_firmware_vimage_tlv_t tlv;
                const char *name;
                if ((name = (const char *) mscc_vimage_sideband_find_tlv(sb, &tlv, type))) {
                     // Data is NULL terminated
                    memcpy(buffer, name, vtss::min(tlv.data_len, (mscc_le_u32) buflen));
                    // Terminate to be safe
                    buffer[buflen-1] = '\0';
                    rc = VTSS_RC_OK;
                }
                VTSS_FREE(sb);
            }
        }
        vtss_mtd_close(&mtd);
    } else {
        rc = rc2;
    }
    if (rc != VTSS_RC_OK) {
        buffer[0] = '\0';    // Be sure we have valid output
    }
    return rc;
}


mesa_rc firmware_image_stage2_name_get(const char *fis_name, char *buffer, size_t buflen)
{
    return firmware_sideband_by_type_get(fis_name,
                                         MSCC_FIRMWARE_SIDEBAND_STAGE2_FILENAME,
                                         buffer, buflen);
}

mesa_rc firmware_image_name_get(const char *fis_name, char *buffer, size_t buflen)
{
    if (firmware_is_mmc() || firmware_is_nand_only()) {
        auto filename = vtss_uboot_get_env(fis_name);
        if (filename.valid()) {
            strncpy(buffer, filename.get().c_str(), buflen);
            return VTSS_RC_OK;
        }
        return VTSS_RC_ERROR;
    } else if (firmware_is_nor_only()) {
        if (firmware_is_mfi_based()) {
            return firmware_get_stage2_nor_filename(fis_name, buffer, buflen);
        } else {
            vtss_mtd_t mtd;
            if (VTSS_RC_OK != vtss_mtd_open(&mtd, fis_name)) {
                return VTSS_RC_ERROR;
            }
            auto filename = vtss_uboot_get_env(mtd.dev);
            vtss_mtd_close(&mtd);
            if (filename.valid()) {
                strncpy(buffer, filename.get().c_str(), buflen);
                return VTSS_RC_OK;
            }
            return VTSS_RC_ERROR;
        }
    } else {
        return firmware_sideband_by_type_get(fis_name,
                                             MSCC_FIRMWARE_SIDEBAND_FILENAME,
                                             buffer, buflen);
    }
}

mesa_rc firmware_image_name_set(const char *fis_name, const char *buffer)
{
    mesa_rc rc = VTSS_RC_ERROR;
    vtss_mtd_t mtd;

    if (firmware_is_nor_only()) {
        /*
         * Modifying the flash filename is not supported for NOR flash images at
         * the moment as implementing this is rather cumbersome. As the filename
         * TLV in question cannot be guaranteed to be aligned on a Flash erase
         * block boundary we will have to handle erasing part of the last part
         * of the preceding block and re-storing that.
         *
         * And since this function is a debug command with unclear usage scenario
         * I choose not to deal with this for now.
         */
        return MESA_RC_NOT_IMPLEMENTED;
    }

    if ((rc = vtss_mtd_open(&mtd, fis_name)) == VTSS_RC_OK) {
        off_t base = mscc_firmware_sideband_get_offset(&mtd);
        if (base) {
            mscc_firmware_sideband_t *sb = mscc_vimage_sideband_read(&mtd, base);
            if (!sb) {
                sb = mscc_vimage_sideband_create();
            }
            if (sb &&
                (sb = mscc_vimage_sideband_set_tlv(sb, (u8 *)buffer, strlen(buffer)+1, MSCC_FIRMWARE_SIDEBAND_FILENAME))) {
                mscc_vimage_sideband_update_crc(sb);
                mscc_vimage_sideband_write(&mtd, sb, base);
                VTSS_FREE(sb);
            }
        }
        vtss_mtd_close(&mtd);
    }
    return rc;
}

const char *firmware_status_get(void)
{
    return firmware_status.c_str();
}

void firmware_status_set(const char *status)
{
    firmware_status = status ? status : "idle";
    if (status) {
        fprintf(stderr, "%s\n", status);
    }
    T_I("Firmware update status: %s", firmware_status.c_str());
}

void firmware_status_str_set(const std::string sstatus)
{
    firmware_status_set(sstatus.c_str());
}

/*
 * firmware_error_txt()
 */
const char *firmware_error_txt(mesa_rc rc)
{
  switch(rc) {
    case FIRMWARE_ERROR_IN_PROGRESS:             return "Firmware update in progress";
    case FIRMWARE_ERROR_IP:                      return "IP Setup error";
    case FIRMWARE_ERROR_TFTP:                    return "TFTP error";
    case FIRMWARE_ERROR_BUSY:                    return "Already updating";
    case FIRMWARE_ERROR_MALLOC:                  return "Memory allocation error";
    case FIRMWARE_ERROR_INVALID:                 return "Invalid image";
    case FIRMWARE_ERROR_FLASH_PROGRAM:           return "Flash write error";
    case FIRMWARE_ERROR_SAME:                    return "Flash is already updated with this image";
    case FIRMWARE_ERROR_CURRENT_UNKNOWN:         return "The currently loaded image is unknown";
    case FIRMWARE_ERROR_CURRENT_NOT_FOUND:       return "The image that we're currently running was not found in flash";
    case FIRMWARE_ERROR_UPDATE_NOT_FOUND:        return "The required flash entry was not found";
    case FIRMWARE_ERROR_CRC:                     return "The entry has invalid CRC";
    case FIRMWARE_ERROR_SIZE:                    return "The size of the firmware image is too big to fit into the flash";
    case FIRMWARE_ERROR_FLASH_ERASE:             return "An error occurred while attempting to erase the flash";
    case FIRMWARE_ERROR_INCOMPATIBLE_TARGET:     return "Incompatible target system";
    case FIRMWARE_ERROR_IMAGE_NOT_FOUND:         return "Image not found";
    case FIRMWARE_ERROR_IMAGE_MFI_TLV_LEN:       return "Invalid string length found in MFI Image";
    case FIRMWARE_ERROR_IMAGE_MFI_TLV_TOO_SMALL: return "MFI TLV too small";
    case FIRMWARE_ERROR_IMAGE_MFI_DECOMPRESS:    return "Decompression error";
    case FIRMWARE_ERROR_IMAGE_TYPE_UNKNOWN:      return "Image type unknown";
    case FIRMWARE_ERROR_SIGNATURE:               return "Signature missing";
    case FIRMWARE_ERROR_AUTHENTICATION:          return "Authentication failure";
    case FIRMWARE_ERROR_NO_CODE:                 return "No code in firmware image";
    case FIRMWARE_ERROR_NO_STAGE2:               return "No stage2 section in firmware image";
    case FIRMWARE_ERROR_PROTOCOL:                return "Invalid protocol";
    case FIRMWARE_ERROR_WRONG_ARCH:              return "Firmware not compatible with this system type";
    case FIRMWARE_ERROR_NO_USERPW:               return "No username and/or password";
    case FIRMWARE_ERROR_CHUNK_OOO:               return "Received chunk out-of-order";
    case FIRMWARE_ERROR_EXPECTING_MFI_IMAGE:     return "Invalid image, mfi image expected";
    case FIRMWARE_ERROR_EXPECTING_ITB_IMAGE:     return "Invalid image, itb image expected";
    case FIRMWARE_ERROR_EXPECTING_EXT4_GZ_IMAGE: return "Invalid image, ext4.gz image expected";
    case FIRMWARE_ERROR_EXPECTING_UBIFS_IMAGE:   return "Invalid image, ubifs image expected";
    default:                                     return "Unknown firmware error code";
  }
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void firmware_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_firmware_json_init(void);
#endif
extern "C" int firmware_icli_cmd_register();

mesa_rc firmware_init(vtss_init_data_t *data)
{
    // OS specific handling
    firmware_init_os(data);

    switch (data->cmd) {
    case INIT_CMD_INIT:
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        firmware_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_firmware_json_init();
#endif
        firmware_icli_cmd_register();
        break;
    default:
        break;
    }
    return 0;
}

/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\firmware.h

==============================================================================
*/
/**
 * \brief Iterate function of firmware image table
 *
 * To get first or get next image ID.
 *
 * \param prev_image_id [IN]  previous image ID.
 * \param next_image_id [OUT] next image ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_status_image_entry_itr(
    const u32       *const prev_image_id,
    u32             *const next_image_id
)
{
    /* check parameter */
    if ( next_image_id == NULL ) {
        T_D("next_image_id == NULL\n");
        return VTSS_RC_ERROR;
    }

    if ( prev_image_id ) {
        if ( *prev_image_id >= VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE ) {
            return VTSS_RC_ERROR;
        }
        *next_image_id = *prev_image_id + 1;
    } else {
        *next_image_id = VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER;
    }
    return VTSS_RC_OK;
}

/**
 * \brief Iterate function of switch firmware table
 *
 * To get first or get next switch ID.
 *
 * \param prev_switch_id [IN]  previous switch ID.
 * \param next_switch_id [OUT] next switch ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_status_switch_entry_itr(
    const u32       *const prev_switch_id,
    u32             *const next_switch_id
)
{
    vtss_usid_t         usid;
    msg_switch_info_t   info;

    if (prev_switch_id) {
        usid = *prev_switch_id + 1;
    } else {
        usid = VTSS_USID_START;
    }

    for (; usid < VTSS_USID_END; ++usid) {
        if (msg_switch_info_get(topo_usid2isid(usid), &info) == VTSS_RC_OK) {
            *next_switch_id = (u32)usid;
            return VTSS_RC_OK;
        }
    }

    return VTSS_RC_ERROR;
}
