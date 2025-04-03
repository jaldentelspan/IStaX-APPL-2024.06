/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_USERS_API_H_
#define _VTSS_USERS_API_H_

#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_t, etc. */
#include "sysutil_api.h"

/* for public APIs */
#include "vtss/appl/users.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \file vtss_users_api.h
 * \brief This file defines the APIs for the Users module
 */

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH = MODULE_ERROR_START(VTSS_MODULE_ID_USERS), /**< Operation is only allowed on the primary switch.      */
    VTSS_USERS_ERROR_ISID,                                                              /**< isid parameter is invalid.                            */
    VTSS_USERS_ERROR_INV_PARAM,                                                         /**< Invalid parameter.                                    */
    VTSS_USERS_ERROR_REJECT,                                                            /**< Username and password combination not found.          */
    VTSS_USERS_ERROR_CFG_INVALID_USERNAME,                                              /**< Invalid username.                                     */
    VTSS_USERS_ERROR_CFG_INVALID_PASSWORD,                                              /**< Invalid password.                                     */
    VTSS_USERS_ERROR_USERS_TABLE_FULL,                                                  /**< Users table full.                                     */
    VTSS_USERS_ERROR_USERS_DEL_ADMIN,                                                   /**< Cannot delete system default administrator.           */
    VTSS_USERS_ERROR_LOWER_PRIV_LVL_LOCK_YOUSELF,                                       /**< Change to lower privilege level will lock yourself out */
    VTSS_USERS_ERROR_USERNAME_NOT_EXISTING                                              /**< Username is not existing                              */
};

/**
 * Users password digest length
 */
#define VTSS_USERS_HASH_DIGEST_LEN      VTSS_SYS_HASH_DIGEST_LEN    /**< The digeset length for password */

/**
 * Users maximum password length
 */
#define VTSS_USERS_PASSWORD_MAX_LEN     VTSS_SYS_PASSWD_ARRAY_SIZE

/**
 * \brief Users module configuration.
*/
typedef struct {
    BOOL    valid;                                  /**< entry valid?                           */
    char    username[VTSS_SYS_USERNAME_LEN];        /**< Add an extra byte for null termination */
    char    password[VTSS_USERS_PASSWORD_MAX_LEN];  /**< Add an extra byte for null termination */
    int     privilege_level;                        /**< privilege level                        */
    BOOL    encrypted;                              /**< password is encrypted or not           */
} users_conf_t;

/**
 * Users maximum entries counter
 */
#define VTSS_USERS_NUMBER_OF_USERS  20      /**< Maximum allowed users entry number */

/**
 * Users minimum/maximum privilege level
 */
#define VTSS_USERS_MIN_PRIV_LEVEL   0       /**< Minimum allowed privilege level */
#define VTSS_USERS_MAX_PRIV_LEVEL   15      /**< Maximum allowed privilege level */

/**
  * \brief Retrieve an error string based on a return code
  *        from one of the Users API functions.
  *
  * \param rc [IN]: Error code that must be in the VTSS_USERS_ERROR_xxx range.
  */
const char *vtss_users_error_txt(mesa_rc rc);

/**
  * \brief Get the global Users configuration.
  *
  * \param glbl_cfg [OUT]: Pointer to structure that receives
  *                        the current configuration.
  * \param next     [IN]:  Getnext?
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VTSS_USERS_ERROR_INV_PARAM if glbl_cfg is NULL.\n
  *    VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VTSS_USERS_ERROR_REJECT if get fail.\n
  */
mesa_rc vtss_users_mgmt_conf_get(users_conf_t *glbl_cfg, BOOL next);

/**
  * \brief Vereify the clear password if valid.
  *
  * \param username [IN]: Pointer to username.
  * \param password [IN]: Pointer to password.
  *
  * \return
  *    TRUE when the username/password is valid, FALSE otherwise.
  */
BOOL vtss_users_mgmt_verify_clear_password(const char *username, const char *password);

/**
  * \brief Set the global Users configuration.
  *
  * \param glbl_cfg [IN]: Pointer to structure that contains the
  *                       global configuration to apply to the
  *                       voice VLAN module.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VTSS_USERS_ERROR_INV_PARAM if glbl_cfg is NULL or parameters error.\n
  *    VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VTSS_USERS_ERROR_CFG_INVALID_USERNAME if user name is null string.\n
  *    VTSS_USERS_ERROR_USERS_TABLE_FULL if users table is full.\n
  *    Others value arises from sub-function.\n
  */
mesa_rc vtss_users_mgmt_conf_set(users_conf_t *glbl_cfg);

/**
 * \brief Delete the Users configuration.
 *
 * \param user_name [IN] The user name
 * \return : VTSS_RC_OK or one of the following
 *  VTSS_USERS_ERROR_GEN (conf is a null pointer)
 *  VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH
 */
mesa_rc vtss_users_mgmt_conf_del(char *user_name);

/**
 * \brief Clear the Users configuration.
 *
 * \param user_name [IN] The user name
 * \return : VTSS_RC_OK or one of the following
 *  VTSS_USERS_ERROR_MUST_BE_PRIMARY_SWITCH
 */
mesa_rc vtss_users_mgmt_conf_clear(void);

/**
  * \brief Initialize the Users module
  *
  * \param data [IN]: Initial data point.
  *
  * \return
  *    VTSS_RC_OK.
  */
mesa_rc vtss_users_init(vtss_init_data_t *data);

/* Check if user name string */
BOOL vtss_users_mgmt_is_valid_username(const char *str);

BOOL vtss_users_mgmt_is_printable_string(char *encry_password);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_USERS_API_H_ */

