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
 * \brief Public Users API
 * \details This header file describes Users control functions and types.
 */

#ifndef _VTSS_APPL_USERS_H_
#define _VTSS_APPL_USERS_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

/** The maximum length of user name */
#define VTSS_APPL_USERS_NAME_LEN                    31

/** The maximum length of unencrypted password */
#define VTSS_APPL_USERS_UNENCRYPTED_PASSWORD_LEN    31

/** The maximum length of encrypted password */
#define VTSS_APPL_USERS_ENCRYPTED_PASSWORD_LEN      128

/** The maximum length of password */
#define VTSS_APPL_USERS_PASSWORD_LEN                (VTSS_APPL_USERS_UNENCRYPTED_PASSWORD_LEN > VTSS_APPL_USERS_ENCRYPTED_PASSWORD_LEN ? VTSS_APPL_USERS_UNENCRYPTED_PASSWORD_LEN : VTSS_APPL_USERS_ENCRYPTED_PASSWORD_LEN)

/** The maximum privilege level */
#define VTSS_APPL_PRIVILEGE_LEVEL_MAX               15

/**
 * \brief Data struct of user name
 *  The data type is to encapsulate char string of user name to be index.
 */
typedef struct {
    /**
     * \brief Name of user.
     */
    char    username[VTSS_APPL_USERS_NAME_LEN + 1];
} vtss_appl_users_username_t;

/**
 * \brief Users configuration
 *  The configuration is the users configuration that can manage the user
 *  account to access the system. The configuration include user name,
 *  privilege and its password.
 */
typedef struct {
    /**
     * \brief Privilege level of the user.
     */
    uint32_t     privilege;

    /**
     * \brief The flag indicates the password is encrypted or not. TRUE means
     * the password is encrypted. FALSE means the password is plain text.
     */
    mesa_bool_t    encrypted;

    /**
     * \brief Password of the user.
     */
    char    password[VTSS_APPL_USERS_PASSWORD_LEN + 1];

} vtss_appl_users_config_t;

/**
 * \brief Users information
 *  The information is used for JSON connection that return myself
 *  user name and privilege.
 */
typedef struct {
    /**
     * \brief Name of user.
     */
    char    username[VTSS_APPL_USERS_NAME_LEN + 1];

    /**
     * \brief Privilege level of the user.
     */
    uint32_t     privilege;
} vtss_appl_users_info_t;

/**
 * \brief Iterate function of Users Configuration
 *
 * To get first and get next indexes.
 *
 * \param prev_username [IN]  previous user name.
 * \param next_username [OUT] next user name.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_users_config_itr(
    const vtss_appl_users_username_t    *const prev_username,
    vtss_appl_users_username_t          *const next_username
);

/**
 * \brief Get Users Configuration
 *
 * To read configuration of Users.
 *
 * \param username [IN]  (key) User name.
 * \param conf     [OUT] The configuration of the user
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_users_config_get(
    vtss_appl_users_username_t  username,
    vtss_appl_users_config_t    *const conf
);

/**
 * \brief Set Users Configuration
 *
 * To add or modify configuration of Users.
 *
 * \param username [IN] (key) User name.
 * \param conf     [IN] The configuration of the user
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_users_config_set(
    vtss_appl_users_username_t      username,
    const vtss_appl_users_config_t  *const conf
);

/**
 * \brief Delete Users Configuration
 *
 * To delete configuration of Users.
 *
 * \param username [IN]  (key) User name.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_users_config_del(
    vtss_appl_users_username_t  username
);

/**
 * \brief Get myself username and privilege.
 *
 * \param info [OUT]: myself user information.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_users_whoami(
    vtss_appl_users_info_t    *const info
);

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_USERS_H_ */
