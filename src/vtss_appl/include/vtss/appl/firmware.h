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

/**
 * \file
 * \brief Public Firmware API
 * \details This header file describes Firmware control functions and types.
 */

#ifndef _VTSS_APPL_FIRMWARE_H_
#define _VTSS_APPL_FIRMWARE_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

/** The maximum string length of name string */
#define VTSS_APPL_FIRMWARE_NAME_LEN         127

/** The maximum string length of version string */
#define VTSS_APPL_FIRMWARE_VERSION_LEN      255

/** The maximum string length of date string */
#define VTSS_APPL_FIRMWARE_DATE_LEN         255

/** The maximum string length of URL string */
#define VTSS_APPL_FIRMWARE_URL_LEN          255

/** The maximum string length of URL string */
#define VTSS_APPL_FIRMWARE_REVISION_LEN     127

/** The maximum string length of chip ID */
#define VTSS_APPL_FIRMWARE_CHIP_ID_LEN      31

/** The maximum string length of board type */
#define VTSS_APPL_FIRMWARE_BOARD_TYPE_LEN   63

/** The maximum string length of product */
#define VTSS_APPL_FIRMWARE_PRODUCT_LEN      255

/**
 * \brief Types of Image of Firmware status
 */
typedef enum {
    VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER,            /*!< Boot Loader            */
    VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE,       /*!< Active Firmware        */
    VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE,  /*!< Alternative Firmware   */
} vtss_appl_firmware_status_image_type_t;

/**
 * \brief Firmware status to get the status of each image
 */
typedef struct {
    vtss_appl_firmware_status_image_type_t  type;                                               /*!< Image type          */
    char                                    name[VTSS_APPL_FIRMWARE_NAME_LEN + 1];              /*!< Image name          */
    char                                    version[VTSS_APPL_FIRMWARE_VERSION_LEN + 1];        /*!< Image version       */
    char                                    built_date[VTSS_APPL_FIRMWARE_DATE_LEN + 1];        /*!< Image built date    */
    char                                    code_revision[VTSS_APPL_FIRMWARE_REVISION_LEN + 1]; /*!< Image code revision */
} vtss_appl_firmware_status_image_t;

/**
 * \brief Firmware status to get the status of each switch
 */
typedef struct {
    char    chip_id[VTSS_APPL_FIRMWARE_CHIP_ID_LEN + 1];            /*!< Chip ID */
    char    board_type[VTSS_APPL_FIRMWARE_BOARD_TYPE_LEN + 1];      /*!< Board type */
    uint32_t     port_cnt;                                               /*!< Count of ports */
    char    product[VTSS_APPL_FIRMWARE_PRODUCT_LEN + 1];            /*!< Product name */
    char    version[VTSS_APPL_FIRMWARE_VERSION_LEN + 1];            /*!< Image version       */
    char    built_date[VTSS_APPL_FIRMWARE_DATE_LEN + 1];            /*!< Image built date    */
} vtss_appl_firmware_status_switch_t;

/**
 * \brief Status of firmware upload
 */
typedef enum {
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_NONE,                          /*!< No upload operation                                                            */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_SUCCESS,                       /*!< Upload operation is successful                                                 */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_IN_PROGRESS,                   /*!< Current upload operation is in progress                                        */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_IP,                      /*!< IP Setup error                                                                 */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_TFTP,                    /*!< TFTP error                                                                     */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_BUSY,                    /*!< Already updating                                                               */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_MEMORY_INSUFFICIENT,     /*!< Memory allocation error                                                        */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INVALID_IMAGE,           /*!< Invalid image                                                                  */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_WRITE_FLASH,             /*!< Flash write error                                                              */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_SAME_IMAGE_EXISTED,      /*!< Flash is already updated with this image                                       */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_UNKNOWN_IMAGE,           /*!< The currently loaded image is unknown                                          */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_FLASH_IMAGE_NOT_FOUND,   /*!< The image that we're currently running was not found in flash                  */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_FLASH_ENTRY_NOT_FOUND,   /*!< The required flash entry was not found                                         */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_CRC,                     /*!< The entry has invalid CRC                                                      */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_IMAGE_SIZE,              /*!< The size of the firmware image is too big to fit into the flash                */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_ERASE_FLASH,             /*!< An error occurred while attempting to erase the flash                          */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INCOMPATIBLE_TARGET,     /*!< Incompatible target system                                                     */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_DOWNLOAD_URL,            /*!< Failed to download URL                                                         */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INVALID_URL,             /*!< Invalid URL                                                                    */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INVALID_PATH,            /*!< The path of the image is invalid                                               */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INVALID_FILENAME,        /*!< The image name is invalid                                                      */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_PROTOCOL,                /*!< Unsupported protocol                                                           */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_NO_USERPW,               /*!< No username and/or password given                                              */
    VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_CHUNK_OOO                /*!< Received chunk out-of-order                                                    */
} vtss_appl_firmware_upload_status_t;

/**
 * \brief Firmware status to upload image
 */
typedef struct {
    vtss_appl_firmware_upload_status_t      status;     /*!< Read-only upload status                                                            */
} vtss_appl_firmware_status_image_upload_t;

/**
 * \brief Firmware control to upload image
 */
typedef struct {
    mesa_bool_t    swap_firmware;   /*!< Action to swap active and alternative firmwares, TRUE to do the action, FALSE to do nothing */
} vtss_appl_firmware_control_globals_t;

/**
 * \brief Types of Image of Firmware status
 */
typedef enum {
    VTSS_APPL_FIRMWARE_UPLOAD_IMAGE_TYPE_BOOTLOADER,    /*!< Boot Loader    */
    VTSS_APPL_FIRMWARE_UPLOAD_IMAGE_TYPE_FIRMWARE,      /*!< Firmware       */
} vtss_appl_firmware_upload_image_type_t;

/**
 * \brief Firmware control to upload image
 */
typedef struct {
    mesa_bool_t                             upload;                                 /*!< Action to upload, TRUE to do the action, FALSE to do nothing                       */
    vtss_appl_firmware_upload_image_type_t  type;                                   /*!< Image type                                                                         */
    char                                    url[VTSS_APPL_FIRMWARE_URL_LEN + 1];    /*!< URL of the image to upload, the format is [protocol]://[IP-address]/[file-path]    */
    mesa_bool_t                             ssh_save_host_keys;                     /*!< Set to true to save new SSH host keys in local cache                               */
    mesa_bool_t                             ftp_active;                             /*!< Set to true to force FTP session to use active mode (default is passive mode)      */

    // this is just a data, but not in MIB file
    vtss_appl_firmware_upload_status_t      upload_status;                          /*!< Read-only upload status                                                            */
} vtss_appl_firmware_control_image_upload_t;

/** TLV attribute name */
#define VTSS_APPL_FIRMWARE_ATTR_NAME           1

/** TLV attribute version number */
#define VTSS_APPL_FIRMWARE_ATTR_VERSION        2

/** TLV attribute size */
#define VTSS_APPL_FIRMWARE_ATTR_SIZE           3

/** TLV attribute license type */
#define VTSS_APPL_FIRMWARE_ATTR_LICENSE_TYPE   4

/** TLV attribute signature */
#define VTSS_APPL_FIRMWARE_ATTR_SIGNATURE_TYPE 5

/** TLV attribute TLV string max. length */
#define VTSS_APPL_FIRMWARE_ATTR_TLV_STR_LEN    255

/** TLV attribute TLV name max. length */
#define VTSS_APPL_FIRMWARE_ATTR_TLV_NAME_LEN   255

/**
 * \brief Types of image status TLV value
 */
typedef enum {
    VTSS_APPL_FIRMWARE_STATUS_IMAGE_TLV_TYPE_STR,   /*!< Value is a string type           */
    VTSS_APPL_FIRMWARE_STATUS_IMAGE_TLV_TYPE_INT,   /*!< Value is a integer type          */
    VTSS_APPL_FIRMWARE_STATUS_IMAGE_TLV_TYPE_UINT,  /*!< Value is a unsigned integer type */
} vtss_appl_firmware_status_image_tlv_type_t;

/**
 * \brief Containing a TLV value from the MFI image
 */
typedef struct {
    char attr_name[VTSS_APPL_FIRMWARE_ATTR_TLV_NAME_LEN];   /*!< TLV attribute Name */
    vtss_appl_firmware_status_image_tlv_type_t  attr_type;  /*!< TLV attribute Type */

    union {
        char str[VTSS_APPL_FIRMWARE_ATTR_TLV_STR_LEN]; /*!< TLV information as string     */
        int32_t  number;                              /*!< TLV information as signed number   */
        uint32_t  unumber;                             /*!< TLV information as unsigned number */
    } value;                                      /**< TLV information                    */
} vtss_appl_firmware_image_status_tlv_t;


/*
==============================================================================

    Public APIs

==============================================================================
*/

/**
 * \brief Get firmware MFI image TLV information.
 *
 * \param image_id     [IN]  (key) Image number, starts from 0 and 0 is always the boot loader
 * \param section_id   [IN]  (key) Section number for the section within the image.
 * \param attribute_id [IN]  (key) Attribute number for the TLV within the section.
 * \param data         [OUT] The TLV information.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_status_image_tlv_get(
    uint32_t  image_id,
    uint32_t  section_id,
    uint32_t  attribute_id,
    vtss_appl_firmware_image_status_tlv_t *const data
);

/**
 * \brief Iterate function of firmware image status table
 *
 * To get first or get next TLV.
 *
 * \param prev_image_id     [IN]  previous image ID.    - Setting to NULL = Use first ID.
 * \param next_image_id     [OUT] next image ID.
 * \param prev_section_id   [IN]  previous section ID.  - Setting to NULL = Use first ID.
 * \param next_section_id   [OUT] next section ID.
 * \param prev_attribute_id [IN]  previous attribute ID - Setting to NULL = Use first ID.
 * \param next_attribute_id [OUT] next section ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_status_image_tlv_itr(const uint32_t *prev_image_id,     uint32_t *next_image_id,
                                                const uint32_t *prev_section_id,   uint32_t *next_section_id,
                                                const uint32_t *prev_attribute_id, uint32_t *next_attribute_id);

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
    const uint32_t       *const prev_image_id,
    uint32_t             *const next_image_id
);


/**
 * \brief Get firmware image entry
 *
 * To read status of each firmware image.
 *
 * \param image_id    [IN]  (key) Image ID
 * \param image_entry [OUT] The current status of the image
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_status_image_entry_get(
    uint32_t                                 image_id,
    vtss_appl_firmware_status_image_t   *const image_entry
);

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
    const uint32_t       *const prev_switch_id,
    uint32_t             *const next_switch_id
);

/**
 * \brief Get firmware image entry
 *
 * To read firmware status of each switch.
 *
 * \param switch_id    [IN]  (key) Image number, starts from 1 and 1 is always the boot loader
 * \param switch_entry [OUT] The current status of the image in the switch
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_status_switch_entry_get(
    uint32_t                                 switch_id,
    vtss_appl_firmware_status_switch_t  *const switch_entry
);

/**
 * \brief Get firmware status to image upload
 *
 * To get the status of current upload operation.
 *
 * \param status [OUT] The status of current upload operation.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_status_image_upload_get(
    vtss_appl_firmware_status_image_upload_t    *const status
);

/**
 * \brief Get global parameters of firmware control
 *
 * \param globals [OUT] The global parameters.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_control_globals_get(
    vtss_appl_firmware_control_globals_t            *const globals
);

/**
 * \brief Set global parameters of firmware control
 *
 * \param globals [IN] The global parameters.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_control_globals_set(
    const vtss_appl_firmware_control_globals_t      *const globals
);

/**
 * \brief Get firmware image upload status
 *
 * To get the configuration and status of current upload operation.
 *
 * \param image [OUT] The configuration and status of current upload operation.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_control_image_upload_get(
    vtss_appl_firmware_control_image_upload_t           *const image
);

/**
 * \brief Set firmware image upload
 *
 * To set the configuration to upload image.
 *
 * \param image [IN] The configuration to upload image.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_firmware_control_image_upload_set(
    const vtss_appl_firmware_control_image_upload_t     *const image
);

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_FIRMWARE_H_ */
