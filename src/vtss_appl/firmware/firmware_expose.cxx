/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "firmware_serializer.hxx"

vtss_enum_descriptor_t firmware_status_image_txt[] {
    {VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_BOOTLOADER,           "bootloader"},
    {VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ACTIVE_FIRMWARE,      "activeFirmware"},
    {VTSS_APPL_FIRMWARE_STATUS_IMAGE_TYPE_ALTERNATIVE_FIRMWARE, "alternativeFirmware"},
    {0, 0},
};

vtss_enum_descriptor_t firmware_upload_image_txt[] {
    {VTSS_APPL_FIRMWARE_UPLOAD_IMAGE_TYPE_BOOTLOADER,   "bootloader"},
    {VTSS_APPL_FIRMWARE_UPLOAD_IMAGE_TYPE_FIRMWARE,     "firmware"},
    {0, 0},
};

vtss_enum_descriptor_t firmware_upload_status_txt[] {
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_NONE,                             "none"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_SUCCESS,                          "success"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_IN_PROGRESS,                      "inProgress"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_IP,                         "errIvalidIp"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_TFTP,                       "errTftpFailed"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_BUSY,                       "errBusy"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_MEMORY_INSUFFICIENT,        "errMemoryInsufficient"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INVALID_IMAGE,              "errInvalidImage"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_WRITE_FLASH,                "errWriteFlash"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_SAME_IMAGE_EXISTED,         "errSameImageExisted"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_UNKNOWN_IMAGE,              "errUnknownImage"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_FLASH_IMAGE_NOT_FOUND,      "errFlashImageNotFound"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_FLASH_ENTRY_NOT_FOUND,      "errFlashEntryNotFound"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_CRC,                        "errCrc"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_IMAGE_SIZE,                 "errImageSize"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_ERASE_FLASH,                "errEraseFlash"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INCOMPATIBLE_TARGET,        "errIncompatibleTarget"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_DOWNLOAD_URL,               "errDownloadUrl"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INVALID_URL,                "errInvalidUrl"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INVALID_PATH,               "errInvalidPath"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_INVALID_FILENAME,           "errInvalidFilename"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_PROTOCOL,                   "errInvalidProtocol"},
    {VTSS_APPL_FIRMWARE_UPLOAD_STATUS_ERROR_NO_USERPW,                  "errNoUserNamePassword"},
    {0, 0},
};

