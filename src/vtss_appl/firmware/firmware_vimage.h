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

#ifndef _FIRMWARE_VIMAGE_H_
#define _FIRMWARE_VIMAGE_H_

#include <stddef.h>
#include "vtss_mtd_api.hxx"

#ifdef __cplusplus
extern "C" {
#endif

// Type used for 32bit unsigned integers - little endian encoded.
typedef uint32_t mscc_le_u32;

#define MSCC_VIMAGE_MAGIC1    0xedd4d5de
#define MSCC_VIMAGE_MAGIC2    0x987b4c4d

typedef enum {
    MSCC_STAGE1_TLV_KERNEL      = 0,
    MSCC_STAGE1_TLV_SIGNATURE,
    MSCC_STAGE1_TLV_INITRD,
    MSCC_STAGE1_TLV_KERNEL_CMD,
    MSCC_STAGE1_TLV_METADATA,
    MSCC_STAGE1_TLV_LICENSES,
    MSCC_STAGE1_TLV_STAGE2,
} mscc_firmware_image_stage1_tlv_t;

/* Signature types */
typedef enum {
    MSCC_FIRMWARE_IMAGE_SIGNATURE_NULL   = 0,
    MSCC_FIRMWARE_IMAGE_SIGNATURE_MD5    = 1,
    MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA256 = 2,
    MSCC_FIRMWARE_IMAGE_SIGNATURE_SHA512 = 3,
} mscc_firmware_image_signature_t;

typedef struct mscc_firmware_vimage {
    mscc_le_u32 magic1;         // 0xedd4d5de
    mscc_le_u32 magic2;         // 0x987b4c4d
    mscc_le_u32 version;        // 0x00000001

    mscc_le_u32 hdrlen;         // Header length
    mscc_le_u32 imglen;         // Total image length (stage1)

    char machine[32];           // Machine/board name
    char soc_name[32];          // SOC family name
    mscc_le_u32 soc_no;         // SOC family number

    mscc_le_u32 img_sign_type;  // Image signature algorithm. TLV has signature data

    // After <hdrlen> bytes;
    // struct mscc_firmware_vimage_tlv tlvs[0];
} mscc_firmware_vimage_t;

typedef struct mscc_firmware_vimage_tlv {
    mscc_le_u32 type;      // TLV type (mscc_firmware_image_stage1_tlv_t)
    mscc_le_u32 tlv_len;   // Total length of TLV (hdr, data, padding)
    mscc_le_u32 data_len;  // Data length of TLV
    u8          value[0];  // Blob data
} mscc_firmware_vimage_tlv_t;

/*************************************************
 *
 * Sideband firmware trailer adds system metadata
 *
 *************************************************/

#define MSCC_VIMAGE_SB_MAGIC1    0x0e27877e
#define MSCC_VIMAGE_SB_MAGIC2    0xf28ee435

typedef enum {
    MSCC_FIRMWARE_SIDEBAND_FILENAME,
    MSCC_FIRMWARE_SIDEBAND_STAGE2_FILENAME,
} mscc_firmware_image_sideband_tlv_t;

// Optional trailer data following firmware image in NOR
// NB: 4k byte aligned!
typedef struct mscc_firmware_sideband {
    mscc_le_u32 magic1;
    mscc_le_u32 magic2;
    u8          checksum[16];    // MD5 signature - entire trailer
    mscc_le_u32 length;
    // <mscc_firmware_vimage_tlv_t/mscc_firmware_image_sideband_tlv_t> TLvs here
} mscc_firmware_sideband_t;

#define MSCC_FIRMWARE_ALIGN(sz, blk) (((sz) + (blk-1)) & ~(blk-1))
#define MSCC_FIRMWARE_TLV_ALIGN(sz) MSCC_FIRMWARE_ALIGN(sz, 4)

int mscc_vimage_hdrcheck(const mscc_firmware_vimage_t *fw, const char **errmsg);
u8 *mscc_vimage_mtd_read_tlv(const char *mtd_name,
                             mscc_firmware_vimage_t *fw,
                             mscc_firmware_vimage_tlv_t *tlv,
                             mscc_firmware_image_stage1_tlv_t type);
u8 *mscc_vimage_find_tlv(const mscc_firmware_vimage_t *fw,
                         mscc_firmware_vimage_tlv_t *tlv,
                         mscc_firmware_image_stage1_tlv_t type);
off_t mscc_vimage_mtd_find_tlv(const mscc_firmware_vimage_t *fw,
                               vtss_mtd_t *mtd,
                               mscc_firmware_vimage_tlv_t *tlv,
                               mscc_firmware_image_stage1_tlv_t type);
bool mscc_vimage_validate(const mscc_firmware_vimage_t *fw, const u8 *signature, uint32_t siglen);

/*
 * Stage2
 */

#define MSCC_VIMAGE_S2TLV_MAGIC1    0xa7b263fe

typedef enum {
    MSCC_STAGE2_TLV_BOOTLOADER = 1,
    MSCC_STAGE2_TLV_ROOTFS,
    MSCC_STAGE2_TLV_FILENAME,
    MSCC_STAGE2_TLV_LAST,    // Must be last - always
} mscc_firmware_image_stage2_tlv_t;

typedef struct mscc_firmware_vimage_s2_tlv {
    mscc_le_u32 magic1;
    mscc_le_u32 type;      // TLV type (mscc_firmware_image_stage2_tlv_t)
    mscc_le_u32 tlv_len;   // Total length of TLV (hdr, data, padding)
    mscc_le_u32 data_len;  // Data length of TLV
    mscc_le_u32 sig_type;  // Signature type (mscc_firmware_image_signature_t)
    u8          value[0];  // Blob data
} mscc_firmware_vimage_stage2_tlv_t;

bool mscc_vimage_stage2_check_tlv(const mscc_firmware_vimage_stage2_tlv_t *s2tlv,
                                  size_t s2len, bool check_sig);

mscc_firmware_vimage_stage2_tlv_t * mscc_vimage_stage2_filename_tlv_create(const char* filename, uint32_t *tlvlen);
void mscc_vimage_stage2_filename_tlv_delete(mscc_firmware_vimage_stage2_tlv_t *s2tlv);
/*
 * Sideband data
 */

int mscc_vimage_sideband_check(const mscc_firmware_sideband_t *sb, const char **errmsg);
off_t mscc_firmware_sideband_get_offset(vtss_mtd_t *mtd);
mscc_firmware_sideband_t *mscc_vimage_sideband_read(vtss_mtd_t *mtd, off_t offset);
mesa_rc mscc_vimage_sideband_write(const vtss_mtd_t *mtd,
                                   mscc_firmware_sideband_t *sb,
                                   const off_t base);
u8 *mscc_vimage_sideband_find_tlv(const mscc_firmware_sideband_t *sb,
                                  mscc_firmware_vimage_tlv_t *tlv,
                                  mscc_firmware_image_sideband_tlv_t type);
mscc_firmware_sideband_t *mscc_vimage_sideband_create(void);
mscc_firmware_sideband_t *mscc_vimage_sideband_add_tlv(mscc_firmware_sideband_t *sb,
                                                       const u8 *data,
                                                       size_t data_length,
                                                       mscc_firmware_image_sideband_tlv_t type);
mscc_firmware_sideband_t *mscc_vimage_sideband_set_tlv(mscc_firmware_sideband_t *sb,
                                                       const u8 *data,
                                                       size_t data_length,
                                                       mscc_firmware_image_sideband_tlv_t type);
void mscc_vimage_sideband_delete_tlv(mscc_firmware_sideband_t *sb,
                                     mscc_firmware_image_sideband_tlv_t type);
void mscc_vimage_sideband_update_crc(mscc_firmware_sideband_t *sb);

// Get FW image information.
// IN : mtd - MTD to read
// OUT: fw - Pointer to where to put the information.
mesa_rc mscc_firmware_fw_vimage_get(vtss_mtd_t *mtd, mscc_firmware_vimage_t *fw);
#ifdef __cplusplus
}
#endif

#endif // _VIMAGE_H_

