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
* \file syslog.h
* \brief Public Syslog APIs.
* \details This is the header file of the syslog module. The switch has the
*          capabilities to create a syslog entry if certain events occurs.
*          All syslog entries are stored on the local switch and they can be
*          transmitted to a syslog server according the configured severity
*          level.
*          This header file provide the functionality to configure the address
*          of a remote syslog server to use and the facilities to inspect and
*          clear syslog entries stored in the RAM. Syslog entries with error 
*          severity (or worse) are also stored in flash. Syslog entries stored
*          in flash is not affected by the clear action - they can be cleared
*          through the flash API.
*/


#ifndef _VTSS_APPL_SYSLOG_H_
#define _VTSS_APPL_SYSLOG_H_


#include <vtss/appl/module_id.h>            // For MODULE_ERROR_START()
#include <vtss/appl/types.h>
#include <vtss/appl/sysutil.h>              // For VTSS_APPL_SYSUTIL_HOSTNAME_LEN
#include <vtss/appl/alarm.h>                // For VTSS_APPL_SYSUTIL_HOSTNAME_LEN
#include <vtss/basics/enum-descriptor.h>    // For vtss_enum_descriptor_t


#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************************
 - Error return code
***************************************************************************/
/*!
 * \brief API Error Return Codes. (mesa_rc)
 */
enum {
    VTSS_APPL_SYSLOG_ERROR_MUST_BE_PRIMARY_SWITCH = MODULE_ERROR_START(VTSS_MODULE_ID_SYSLOG), /*!< Operation is only allowed on the primary switch. */
    VTSS_APPL_SYSLOG_ERROR_ISID,                                                               /*!< isid parameter is invalid.                       */
    VTSS_APPL_SYSLOG_ERROR_INV_PARAM,                                                          /*!< Invalid parameter.                               */
    VTSS_APPL_SYSLOG_ERROR_LOG_ENTRY_NOT_EXIST                                                 /*!< Syslog entry isn't existing                      */
};


/***************************************************************************
 - Type/Macro Definitions
***************************************************************************/
/*!
 * \brief The definition for applying the Syslog control action on all switches.
 */
#define VTSS_APPL_SYSLOG_ALL_SWITCHES   0   /*!< It is used to apply control action on all switches */

/*!
 * \brief The maximum length of Syslog message text for private MIB.
 */
#define VTSS_APPL_SYSLOG_MIB_MSG_TEXT_LEN_MAX   4000 /*!< The maximum length of Syslog message text for private MIB */

#define VTSS_APPL_SYSLOG_ENTRY_NAME_SIZE 32 /*!< The maximum length of a name for a syslog notification */

/*!
 * \brief Syslog severity level. (defined by RFC 5424)
 */
typedef enum {
    VTSS_APPL_SYSLOG_LVL_ERROR     = 3, /*!< Error conditions                 */
    VTSS_APPL_SYSLOG_LVL_WARNING   = 4, /*!< Warning conditions               */
    VTSS_APPL_SYSLOG_LVL_NOTICE    = 5, /*!< Normal but significant condition */
    VTSS_APPL_SYSLOG_LVL_INFO      = 6, /*!< Informational messages           */
    VTSS_APPL_SYSLOG_LVL_ALL       = 8  /*!< Internal usage for show APIs     */
} vtss_appl_syslog_lvl_t;

extern const vtss_enum_descriptor_t syslog_lvl_txt[]; /*!< The text description of syslog severity level */

/*!
 * \brief Syslog notifications name.
 */
typedef struct {
    char notif_name[VTSS_APPL_SYSLOG_ENTRY_NAME_SIZE]; /*!< The name of a syslog notification*/
} vtss_appl_syslog_notif_name_t;

/*!
 * \brief Syslog notifications configurations.
 */
typedef struct {
    char notification[ALARM_NAME_SIZE]; /*!< The reference to the notification */
    vtss_appl_syslog_lvl_t level;       /*!< The syslog level */
} vtss_appl_syslog_notif_conf_t;

/*!
 * \brief Syslog Server configuration.
 */
typedef struct {
    mesa_bool_t                    server_mode;    /*!< Syslog server mode operation                                    */
    vtss_inet_address_t     syslog_server;  /*!< Syslog server address                                           */
    uint16_t                     udp_port;       /*!< The UDP port number which is used to connected to Syslog server */
    vtss_appl_syslog_lvl_t  syslog_level;   /*!< Indicates what level of message will send to syslog server      */
} vtss_appl_syslog_server_conf_t;

/*!
 * \brief Syslog history entry.
 */
typedef struct {
    vtss_appl_syslog_lvl_t  lvl;                                            /*!< Level      */
    uint64_t                     time;                                           /*!< Time stamp */
    char                    msg[VTSS_APPL_SYSLOG_MIB_MSG_TEXT_LEN_MAX + 1]; /*!< Message    */
} vtss_appl_syslog_history_t;

/*!
 * \brief Syslog control entry.
 */
typedef struct {
    mesa_bool_t clear_syslog;    /*!< clear syslog */
} vtss_appl_syslog_history_control_t;


/***************************************************************************
 - Syslog server configuration functions
***************************************************************************/
/*!
 * \brief  Get Syslog server configuration. It is a global configuration, the primary switch
 *         will automatic apply the same configuration to all salve switches.
 * \param  server_conf [OUT]: The syslog server configuration.
 * \return Error code. VTSS_APPL_SYSLOG_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.
 */
mesa_rc vtss_appl_syslog_server_conf_get(vtss_appl_syslog_server_conf_t *const server_conf);

/*!
 * \brief  Set Syslog server configuration. It is a global configuration, the primary switch
 *         will automatic apply the same configuration to all salve switches.
 * \param  server_conf [IN]:  The syslog server configuration.
 * \return Error code. VTSS_APPL_SYSLOG_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.
 */
mesa_rc vtss_appl_syslog_server_conf_set(const vtss_appl_syslog_server_conf_t *const server_conf);


/***************************************************************************
 - Syslog history functions
***************************************************************************/
/*!
 * \brief  Syslog history iterate function, it is used to get first and get next indexes.
 * \param  prev_swid_idx   [IN]:  previous switch ID.
 * \param  next_swid_idx   [OUT]: next switch ID.
 * \param  prev_syslog_idx [IN]:  previous syslog ID.
 * \param  next_syslog_idx [OUT]: next syslog ID.
 * \return Error code.
 */
mesa_rc vtss_appl_syslog_history_itr(
    const vtss_usid_t   *const prev_swid_idx,
    vtss_usid_t         *const next_swid_idx,
    const uint32_t           *const prev_syslog_idx,
    uint32_t                 *const next_syslog_idx
);

/*!
 * \brief  Get Syslog history entry. All syslog entries are stored on the local switch RAM.
 * \param  usid      [IN]: Switch ID for user view. (The valid value starts from 1)
 * \param  syslog_id [IN]: Syslog ID.
 * \param  history  [OUT]: The syslog history entry.
 * \return Error code. VTSS_APPL_SYSLOG_ERROR_LOG_ENTRY_NOT_EXIST means the specific entry isn't existing.
 */
mesa_rc vtss_appl_syslog_history_get(
    vtss_usid_t                 usid,
    uint32_t                         syslog_id,
    vtss_appl_syslog_history_t  *const history
);


/***************************************************************************
 - Syslog control functions
***************************************************************************/
/*!
 * \brief  Syslog history control iterate function, it is used to get first and get next indexes.
 * \param  prev_swid_idx  [IN]: previous switch ID.
 * \param  next_swid_idx [OUT]: next switch ID.
 * \param  prev_lvl_idx   [IN]: previous syslog level.
 * \param  next_lvl_idx  [OUT]: next syslog level.
 * \return Error code.
 */
mesa_rc vtss_appl_syslog_history_control_itr(
    const vtss_usid_t               *const prev_swid_idx,
    vtss_usid_t                     *const next_swid_idx,
    const vtss_appl_syslog_lvl_t    *const prev_lvl_idx,
    vtss_appl_syslog_lvl_t          *const next_lvl_idx
);

/*!
 * \brief  Set Syslog history control entry. It is used to clear the syslog history entries with specific level. Note the clear action doesn't affect on the FLASH.
 * \param  usid     [IN]: Switch ID for user view. (The value 0 means the control action will apply all switches)
 * \param  lvl      [IN]: Syslog level. Indicate which level of syslog history entries will be cleared. The value of 'VTSS_APPL_SYSLOG_LVL_ALL' is used to clear all entries.
 * \param  control  [IN]: The syslog control entry.
 * \return Error code.
 */
mesa_rc vtss_appl_syslog_history_control_set(
    vtss_usid_t                         usid,
    vtss_appl_syslog_lvl_t              lvl,
    const vtss_appl_syslog_history_control_t    *const control
);

/**
 * \brief Iterate through all syslog notifications.
 * \param in [IN]   Pointer to current syslog notification index. Provide a null
 *                  pointer to get the first syslog notification index.
 * \param out [OUT] Next syslog notification index (relative to the value provided
 *                  in 'in').
 * \return Error code. VTSS_RC_OK means that the value in out is valid,
 *                     VTSS_RC_ERROR means that no "next" alarm index exists
 *                     and the end has been reached.
 */
mesa_rc vtss_appl_syslog_notif_itr(const vtss_appl_syslog_notif_name_t *const in,
                                   vtss_appl_syslog_notif_name_t *const out);

/* Alarm functions ----------------------------------------------------- */

/**
 * \brief Get configuration for a specific alarm
 * \param nm [IN] Name of the alarm
 * \param conf [OUT] The expression defining the alarm
 * \return Error code.
 */
mesa_rc vtss_appl_syslog_notif_get(const vtss_appl_syslog_notif_name_t *const nm,
                                   vtss_appl_syslog_notif_conf_t *conf);

/**
 * \brief Create configuration for a specific alarm
 * \param nm [IN] Name of the alarm
 * \param conf [OUT] The expression defining the alarm
 * \return Error code.
 */
mesa_rc vtss_appl_syslog_notif_add(const vtss_appl_syslog_notif_name_t *const nm,
                                   const vtss_appl_syslog_notif_conf_t *const conf);

/**
 * \brief Delete configuration for a specific alarm
 * \param nm [IN] Name of the alarm
 * \return Error code.
 */
mesa_rc vtss_appl_syslog_notif_del(const vtss_appl_syslog_notif_name_t *const nm);


#ifdef __cplusplus
}
#endif


#endif  /* _VTSS_APPL_SYSLOG_H_ */
