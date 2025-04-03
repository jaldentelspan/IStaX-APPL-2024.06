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

#ifndef _FIRMWARE_API_H_
#define _FIRMWARE_API_H_

#include "cli_io_api.h"
#include "ip_api.h"
#include "simage_api.h"         /* Snap-in */
#include "control_api.h"        /* For mesa_restart_t */
#include "firmware_vimage.h"
#include "vtss/appl/firmware.h" /* Firmware public header */

#define FIRWARE_STAGE2 VTSS_FS_FLASH_DIR "stage2"

#ifdef __cplusplus
extern "C" {
#endif

//
/*
 * Default timeout in seconds for session-based async. upgrades.
 * Timeout = 30 seconds for writing each chunk (empirically determined)
 */
#define FIRMWARE_ASYNC_TIMEOUT_SECS_DEF         (30)

extern size_t firmware_max_download;
extern const char *const fw_fis_linux;
extern const char *const fw_fis_linux_bk;
extern const char *const fw_fis_boot0;
extern const char *const fw_fis_boot1;
extern char fw_fis_mmc1[];
extern char fw_fis_mmc2[];

extern const char *const fw_nor;
extern const char *const fw_nand;
extern const char *const fw_emmc;
extern const char *const fw_nor_nand;

enum {
    FIRMWARE_ERROR_IN_PROGRESS = MODULE_ERROR_START(VTSS_MODULE_ID_FIRMWARE), /**< In progress */
    FIRMWARE_ERROR_IP,                                                        /**< IP Setup error */
    FIRMWARE_ERROR_TFTP,                                                      /**< TFTP error */
    FIRMWARE_ERROR_BUSY,                                                      /**< Already updating */
    FIRMWARE_ERROR_MALLOC,                                                    /**< Memory allocation error */
    FIRMWARE_ERROR_INVALID,                                                   /**< Image error */
    FIRMWARE_ERROR_FLASH_PROGRAM,                                             /**< FLASH write error */
    FIRMWARE_ERROR_SAME,                                                      /**< Flash is already updated with this image */
    FIRMWARE_ERROR_CURRENT_UNKNOWN,                                           /**< The currently loaded image is unknown */
    FIRMWARE_ERROR_CURRENT_NOT_FOUND,                                         /**< The image that we're currently running was not found in flash */
    FIRMWARE_ERROR_UPDATE_NOT_FOUND,                                          /**< The entry we wish to update was not found in flash */
    FIRMWARE_ERROR_CRC,                                                       /**< The entry has invalid CRC */
    FIRMWARE_ERROR_SIZE,                                                      /**< The entry we wish to update was too small to hold the new image */
    FIRMWARE_ERROR_FLASH_ERASE,                                               /**< An error occurred while attempting to erase the flash */
    FIRMWARE_ERROR_INCOMPATIBLE_TARGET,                                       /**< Incompatible target system */
    FIRMWARE_ERROR_IMAGE_NOT_FOUND,                                           /**< Image not found */
    FIRMWARE_ERROR_IMAGE_TYPE_UNKNOWN,                                        /**< Image type unknown */
    FIRMWARE_ERROR_IMAGE_MFI_TLV_LEN,                                         /**< Invalid string length found in MFI Image */
    FIRMWARE_ERROR_IMAGE_MFI_TLV_TOO_SMALL,                                   /**< MFI TLV too small */
    FIRMWARE_ERROR_IMAGE_MFI_DECOMPRESS,                                      /**< Decompression error  */
    FIRMWARE_ERROR_SIGNATURE,                                                 /**< Signature missing */
    FIRMWARE_ERROR_AUTHENTICATION,                                            /**< Authentication error */
    FIRMWARE_ERROR_NO_CODE,                                                   /**< Code section missing */
    FIRMWARE_ERROR_NO_STAGE2,                                                 /**< No stage2 section found in image*/
    FIRMWARE_ERROR_WRONG_ARCH,                                                /**< Wrong architecture */
    FIRMWARE_ERROR_PROTOCOL,                                                  /**< Unsupported protocol */
    FIRMWARE_ERROR_NO_USERPW,                                                 /**< No username and/or password given */
    FIRMWARE_ERROR_CHUNK_OOO,                                                 /**< Received chunk out-of-order */
    FIRMWARE_ERROR_EXPECTING_MFI_IMAGE,                                       /**< Invalid image, mfi image expected */
    FIRMWARE_ERROR_EXPECTING_ITB_IMAGE,                                       /**< Invalid image, itb image expected */
    FIRMWARE_ERROR_EXPECTING_EXT4_GZ_IMAGE,                                   /**< Invalid image, ext4.gz image expected */
    FIRMWARE_ERROR_EXPECTING_UBIFS_IMAGE,                                     /**< Invalid image, ubifs image expected */
};

/* Initialize module */
mesa_rc firmware_init(vtss_init_data_t *data);

/* API functions */

/* Straight file validity check */
mesa_rc firmware_check(const unsigned char *buffer, size_t length);

/* Cleanup unreferenced images */
void firmware_stage2_cleanup(void);

/* Update stage1 and sideband data */
mesa_rc firmware_update_stage1(cli_iolayer_t *io,
                               const unsigned char *buffer,
                               size_t length,
                               const char *mtd_name,
                               const char *sb_file_name,
                               const char *sb_stage2_name);

/* Update bootloader, iff bottloader TLV present */
mesa_rc firmware_update_stage2_bootloader(cli_iolayer_t *io,
                                          const unsigned char *s2ptr,
                                          size_t s2len);

/*
 * Perform an single-part async upgrade operation using an upgrade image which
 * is completely loaded in memory.
 *
 * This method is only used by the legacy web client which uploads the complete
 * image in one HTTP POST request.
 */
mesa_rc firmware_update_async(cli_iolayer_t *io,
                              const unsigned char *buffer,
                              // Pointer to overall HTTP buffer. Free this before mem-mapping the results.
                              char *buf2free,
                              size_t length,
                              const char *filename,
                              mesa_restart_t restart);

/**
 * Start a multi-part async upgrade operation using a chunked firmware upload method
 * for the given session ID.
 *
 * Only a single async upgrade operation can be active at any given time.
 *
 * This method is used by the new web client which uploads the firmware image in
 * a number of HTTP POST requests in order to minimize the overall memory usage.
 * 
 * @param session_id        The unique ID for this firmware update session
 * @param total_chunks      The total number of chunks expected
 * @param io                The IO layer used for user communication
 * @param filename          The filename
 * @return                  VTSS_RC_OK if successful and other error code if failure
 */
mesa_rc firmware_update_async_start(uint32_t session_id,
                                    uint32_t total_chunks,
                                    cli_iolayer_t *io,
                                    const char *filename);

/**
 * Write the next chunk of the firmware image for the currectly active multi-part
 * async upgrade operation with the given session ID.
 * 
 * @param session_id        The unique ID for this firmware update session
 * @param chunk_number      The chunk number for this buffer. Chunks are using 1-offset numbers.
 * @param buffer            The actual chunk data
 * @param length            The size of the chunk data in bytes
 * @param timeout_secs      The timeout in seconds to wait for new chunks
 * @return                  VTSS_RC_OK if successful and other error code if failure
 */
mesa_rc firmware_update_async_write(uint32_t session_id,
                                    uint32_t chunk_number,
                                    const unsigned char *buffer,
                                    size_t length,
                                    uint32_t timeout_secs = FIRMWARE_ASYNC_TIMEOUT_SECS_DEF);

/**
 * Return the last received chunk number for the indicated session.
 * 
 * @param session_id        The unique ID for this firmware update session
 * @param chunk_number      The number of the last received chunk
 * @return                  VTSS_RC_OK if successful and other error code if failure
 */
mesa_rc firmware_update_async_get_last_chunk(uint32_t session_id,
                                             uint32_t &chunk_number);

/**
 * Commit the currently active multi-part async upgrade operation:
 * - Check that the completed image is valid
 * - Write the image to Flash
 * - Reboot the system
 * 
 * @param session_id        The unique ID for this firmware update session
 * @param restart           The restart type to use
 * @return                  VTSS_RC_OK if successful and other error code if failure
 */
mesa_rc firmware_update_async_commit(uint32_t session_id,
                                     mesa_restart_t restart);

/**
 * Abort an ongoing multi-part async upgrade operation for the given session ID.
 * 
 * @param session_id        The unique ID for this firmware update session
 * @return                  VTSS_RC_OK if successful and other error code if failure
 */
mesa_rc firmware_update_async_abort(uint32_t session_id);


// Returns mesa_rc return value
mesa_rc firmware_swap_images(void);

// Split primary image from a single FIS - creating backup FIS
mesa_rc firmware_fis_split(const char *primary, size_t primary_size,
                           const char *backup, size_t backup_size);

// Resize fis (debug only)
mesa_rc firmware_fis_resize(const char *name, size_t new_size);

size_t firmware_section_size(const char *image_name);

#define FIRMWARE_IMAGE_NAME_MAX 128  /**< Maximum stored image name*/

/**
 * \brief Get firmware image name of stored image
 */
mesa_rc firmware_image_name_get(const char *fis_name, char *buffer, size_t buflen);

/**
 * \brief Get firmware image stage2 filename of stored image
 */
mesa_rc firmware_image_stage2_name_get(const char *fis_name, char *buffer, size_t buflen);

/**
 * \brief Set firmware image name of image
 */
mesa_rc firmware_image_name_set(const char *fis_name, const char *buffer);

/**
 * \brief Firmware error txt - converts error code to text
 */
const char *firmware_error_txt(mesa_rc rc);

// Function that converts a TFTP error code into a string.
void firmware_tftp_err2str(int err_num, char *err_str);

// Status
const char *firmware_status_get(void);
void firmware_status_set(const char *status);
void firmware_status_str_set(const std::string sstatus);
bool firmware_is_mfi_based();
bool firmware_is_nand_only();
bool firmware_is_nor_only();
bool firmware_is_mmc();
bool firmware_is_nor_nand();
mesa_rc get_mount_point_source(const char *mt_point, char *fsname, int fsname_size);
mesa_rc get_mount_point(const char *fsname, char *mt_point, int mt_point_size);



// Update firmware in MTD device
mesa_rc firmware_flash_mtd(cli_iolayer_t *io,
                           const char *mtd_name,
                           const unsigned char *buffer,
                           size_t length);

// As above, but rc is OK if no update needed
mesa_rc firmware_flash_mtd_if_needed(cli_iolayer_t *io,
                                     const char *mtd_name,
                                     const unsigned char *buffer,
                                     size_t length);

mesa_rc firmware_check_bootloader_simg(const u8 *buffer, size_t length, u32 *type);

#ifdef __cplusplus
}
#endif

#endif // _FIRMWARE_API_H_

