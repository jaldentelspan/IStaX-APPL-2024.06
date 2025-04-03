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
 * \brief Public ICFG API
 * \details This header file describes ICFG control functions and types.
 */

#ifndef _VTSS_APPL_ICFG_H_
#define _VTSS_APPL_ICFG_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

/** The maximum length of file modified time string */
#define VTSS_APPL_ICFG_TIME_STR_LEN         39

/** The maximum length of file attribute string */
#define VTSS_APPL_ICFG_ATTR_STR_LEN         15

/** The maximum length of file path string */
#define VTSS_APPL_ICFG_FILE_STR_LEN         127

/**
 * \brief ICFG file list status
 */
typedef struct {
    uint32_t         numberOfFiles;  /*!< Number of files in flash                           */
    uint32_t         totalBytes;     /*!< Total number of bytes used by all files in flash   */
    uint32_t         flashSize;      /*!< Flash file system size in bytes */
    uint32_t         flashFree;      /*!< Flash file system free bytes */
} vtss_appl_icfg_file_statistics_t;

/**
 * \brief ICFG file list entry to get the property of each file
 */
typedef struct {
    char        fileName[VTSS_APPL_ICFG_FILE_STR_LEN + 1];      /*!< File name                                          */
    uint32_t         bytes;                                          /*!< Number of bytes of the file                        */
    char        modifiedTime[VTSS_APPL_ICFG_TIME_STR_LEN + 1];  /*!< Last modified time of the file                     */
    char        attribute[VTSS_APPL_ICFG_ATTR_STR_LEN + 1];     /*!< File attribute in the format of 'rw'               */
} vtss_appl_icfg_file_entry_t;

/**
 * \brief Types of reload default
 */
typedef enum {
    VTSS_APPL_ICFG_RELOAD_DEFAULT_NONE,     /*!< No action                                              */
    VTSS_APPL_ICFG_RELOAD_DEFAULT,          /*!< reset the whole system to default                      */
    VTSS_APPL_ICFG_RELOAD_DEFAULT_KEEP_IP   /*!< reset system to default, but keep IP address of VLAN 1 */
} vtss_appl_icfg_reload_default_t;

/**
 * \brief ICFG control
 * The control can reset system to default and also can delete file in flash
 */
typedef struct {
    vtss_appl_icfg_reload_default_t     reloadDefault;                                  /*!< reset system to default */
    char                                deleteFile[VTSS_APPL_ICFG_FILE_STR_LEN + 1];    /*!< delete file in flash, in format of flash:'filename' */
} vtss_appl_icfg_control_t;

/**
 * \brief Types of configuration file
 */
typedef enum {
    VTSS_APPL_ICFG_CONFIG_TYPE_NONE,        /*!< Configuration file     */
    VTSS_APPL_ICFG_CONFIG_TYPE_RUNNING,     /*!< Running configuration  */
    VTSS_APPL_ICFG_CONFIG_TYPE_STARTUP,     /*!< Startup configuration  */
    VTSS_APPL_ICFG_CONFIG_TYPE_FILE         /*!< Configuration file     */
} vtss_appl_icfg_config_type_t;

/**
 * \brief Status of copy operation
 */
typedef enum {
    VTSS_APPL_ICFG_COPY_STATUS_NONE,                        /*!< No copy operation                                                          */
    VTSS_APPL_ICFG_COPY_STATUS_SUCCESS,                     /*!< Copy operation is successful                                               */
    VTSS_APPL_ICFG_COPY_STATUS_IN_PROGRESS,                 /*!< Current copy operation is in progress                                      */
    VTSS_APPL_ICFG_COPY_STATUS_FAILED_OTHER_IN_PROCESSING,  /*!< Copy operation is failed due to other in processing                        */
    VTSS_APPL_ICFG_COPY_STATUS_FAILED_NO_SUCH_FILE,         /*!< Copy operation is failed due to file not existing                          */
    VTSS_APPL_ICFG_COPY_STATUS_FAILED_SAME_SRC_DST,         /*!< Copy operation is failed due to the source and destination are the same    */
    VTSS_APPL_ICFG_COPY_STATUS_FAILED_PERMISSION_DENIED,    /*!< Copy operation is failed due to the destination is not permitted to modify */
    VTSS_APPL_ICFG_COPY_STATUS_FAILED_LOAD_SRC,             /*!< Copy operation is failed due to the error to load source file              */
    VTSS_APPL_ICFG_COPY_STATUS_FAILED_SAVE_DST,             /*!< Copy operation is failed due to the error to save or commit destination    */
} vtss_appl_icfg_copy_status_t;

/**
 * \brief ICFG status of copy config
 */
typedef struct {
    vtss_appl_icfg_copy_status_t    status;     /*!< Read-only copy status                                                            */
} vtss_appl_icfg_status_copy_config_t;

/**
 * \brief Copy configuration
 * The action can download and upload configuration
 */
typedef struct {
    mesa_bool_t                            copy;               /*!< Action to copy, TRUE to do the action, FALSE to do nothing */
    vtss_appl_icfg_config_type_t    sourceConfigType;   /*!< Source configuration type. if the type is VTSS_APPL_ICFG_CONFIG_TYPE_FILE(3),
                                                           * then the configuration file name with path should be specified in
                                                           * SourceConfigFile
                                                           */
    char                            sourceConfigFile[VTSS_APPL_ICFG_FILE_STR_LEN + 1];  /*!< Specify configuration file name with path if SourceConfigType
                                                                                         * is VTSS_APPL_ICFG_CONFIG_TYPE_FILE(3)
                                                                                         */
    vtss_appl_icfg_config_type_t    destinationConfigType;  /*!< Destination configuration type. if the type is VTSS_APPL_ICFG_CONFIG_TYPE_FILE(3),
                                                               * then the configuration file name with path should be specified in
                                                               * DestinationConfigFile
                                                               */
    char                            destinationConfigFile[VTSS_APPL_ICFG_FILE_STR_LEN + 1]; /*!< Specify configuration file name with path if DestinationConfigType
                                                                                             * is VTSS_APPL_ICFG_CONFIG_TYPE_FILE(3)
                                                                                             */
    mesa_bool_t                            merge;  /*!< This flag works only if DestinationConfigType is VTSS_APPL_ICFG_CONFIG_TYPE_RUNNING(1).
                                               * TRUE is to merge the source configuration into the current running configuration.
                                               * FALSE is to replace the current running configuration with the source configuration.
                                               */

    // this is just a data, but not in MIB file
    vtss_appl_icfg_copy_status_t    copyStatus;    /*!< The status indicates the status of current copy operation.
                                                      * It is updated automatically. Modifying this flag does not
                                                      * take any effect. */
} vtss_appl_icfg_copy_config_t;

/*
==============================================================================

    Public APIs

==============================================================================
*/
/**
 * \brief Get file statistics
 *
 * To read current statistics of all fills in flash.
 *
 * \param statistics [OUT] The statistics of all files in flash
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_file_statistics_get(
    vtss_appl_icfg_file_statistics_t    *const statistics
);

/**
 * \brief Iterate function of file table
 *
 * To get first or get next index.
 *
 * \param prev_fileNo [IN]  previous file number.
 * \param next_fileNo [OUT] next file number.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_file_entry_itr(
    const uint32_t    *const prev_fileNo,
    uint32_t          *const next_fileNo
);

/**
 * \brief Get file entry
 *
 * To read status of each file in flash.
 *
 * \param fileNo    [IN]  (key) File number, starts from 1
 * \param fileEntry [OUT] The current property of the file
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_file_entry_get(
    uint32_t                             fileNo,
    vtss_appl_icfg_file_entry_t     *const fileEntry
);

/**
 * \brief Get ICFG status of copy config
 *
 * To get the status of current copy config operation.
 *
 * \param status [OUT] The status of current copy operation.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_status_copy_config_get(
    vtss_appl_icfg_status_copy_config_t     *const status
);

/**
 * \brief Set ICFG Globals Control
 *
 * To do actions on ICFG.
 *
 * \param control [IN] The actions
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_control_set(
    const vtss_appl_icfg_control_t  *const control
);

/**
 * \brief Get ICFG Globals Control
 *
 * To read default values of actions.
 *
 * \param control [OUT] The actions
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_control_get(
    vtss_appl_icfg_control_t        *const control
);

/**
 * \brief Set ICFG Copy Config Control
 *
 * To copy configuration between device and files.
 *
 * \param config [IN] The configuration to copy
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_copy_config_set(
    const vtss_appl_icfg_copy_config_t  *const config
);

/**
 * \brief Get ICFG Copy Config Control
 *
 * To get the values of copy configuration.
 *
 * \param config [OUT] The currnet values to copy configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_icfg_copy_config_get(
    vtss_appl_icfg_copy_config_t        *const config
);

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_ICFG_H_ */
