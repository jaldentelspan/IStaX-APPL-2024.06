/*

 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_AUTH_API_H_
#define _VTSS_AUTH_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vtss/appl/auth.h"

/**
 * \brief Identify the AUTH server address string.
 *
 * The given host address string can only be valid IPv4/IPv6 unicast address or DNS name.
 * Otherwise, the address string is treated as invalid input.
 *
 * \param srv   [IN]  Host address string
 *
 * \return VTSS_RC_OK on valid server address string, otherwise return the error code.
 */
mesa_rc vtss_appl_auth_server_address_valid(const char *const srv);

#ifdef VTSS_SW_OPTION_RADIUS
/**
 * \brief Add a RADIUS server configuration
 *
 * If host AND auth_port AND acct_port matches an existing entry, this entry is updated.
 * Otherwise the entry is added to the end of list.
 *
 * Adding an entry where host and only one of the port matches is treated as an error.
 *
 * \param conf  [IN]  Configuration
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_radius_server_add(vtss_appl_auth_radius_server_conf_t *const conf);

/**
 * \brief Delete a RADIUS server configuration
 *
 * If host AND auth_port AND acct_port matches an existing entry, this entry is deleted.
 * Otherwise an error is returned.
 *
 * \param conf  [IN]  Configuration
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_radius_server_del(vtss_appl_auth_radius_server_conf_t *const conf);
#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
/**
 * \brief Add a TACACS+ server configuration
 *
 * If host AND port matches an existing entry, this entry is updated.
 * Otherwise the entry is added to the end of list.
 *
 * \param conf  [IN]  Configuration
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_tacacs_server_add(vtss_appl_auth_tacacs_server_conf_t *const conf);

/**
 * \brief Delete a TACACS+ server configuration
 *
 * If host AND port matches an existing entry, this entry is deleted.
 * Otherwise an error is returned.
 *
 * \param conf  [IN]  Configuration
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_tacacs_server_del(vtss_appl_auth_tacacs_server_conf_t *const conf);
#endif /* VTSS_SW_OPTION_TACPLUS */

/**
 * \brief Clear HTTPD Cache
 */
void vtss_auth_mgmt_httpd_cache_expire(void);

/**
 * \brief Authenticate HTTPD requests
 *
 * \param username  [IN]  The username.
 * \param password  [IN]  The password.
 * \param priv_lvl  [OUT] The privilege level for this username/password combination.
 *
 * \return 1 : Authenticated.
 *         0 : NOT authenticated.
 */
int vtss_auth_mgmt_httpd_authenticate(char *username, char *password, int *priv_lvl);

/**
 * \brief vtss_auth_login. Called when an agent wants to login.
 *
 * \param agent     [IN]  The agent e.g. telnet or SSH.
 * \param hostname  [IN]  The hostname of the remote host (or NULL if no remote host).
 * \param username  [IN]  The username.
 * \param password  [IN]  The password.
 * \param priv_lvl  [OUT] The privilege level for this username/password combination.
 * \param agent_id  [OUT] A unique agent id is returned for each login.
 *
 * \return VTSS_RC_OK                         : Login is successful.
 *         VTSS_APPL_AUTH_ERROR_SERVER_REJECT : Login is rejected.
 */
mesa_rc vtss_auth_login(vtss_appl_auth_agent_t agent,
                        const char             *hostname,
                        const char             *username,
                        const char             *password,
                        u8                     *priv_lvl,
                        u16                    *agent_id);

/**
 * \brief vtss_auth_cmd. Called when an agent wants authorize a command.
 *
 * \param agent        [IN]  The agent e.g. telnet or SSH.
 * \param hostname     [IN]  The hostname of the remote host (or NULL if no remote host).
 * \param username     [IN]  The username.
 * \param command      [IN]  The command.
 * \param priv_lvl     [IN]  The privilege level for this session.
 * \param agent_id     [IN]  The agent id for this session.
 * \param execute      [IN]  If TRUE, agent wants to execute command. If FALSE, agent wants to check command (no accounting).
 * \param cmd_priv_lvl [IN]  The privilege level of the command.
 * \param cfg_cmd      [IN]  If TRUE, it is a configuration command.
 *
 * \return VTSS_RC_OK                         : Command is permitted.
 *         VTSS_APPL_AUTH_ERROR_SERVER_REJECT : Command is not permitted.
 *         VTSS_APPL_AUTH_ERROR_SERVER_ERROR  : General error. Fallback to default local authorization.
 */
mesa_rc vtss_auth_cmd(vtss_appl_auth_agent_t agent,
                      const char             *hostname,
                      const char             *username,
                      const char             *command,
                      u8                     priv_lvl,
                      u16                    agent_id,
                      BOOL                   execute,
                      u8                     cmd_priv_lvl,
                      BOOL                   cfg_cmd);

/**
 * \brief vtss_auth_integrity_log. Submit system logs containing checksum for app+config
 *
 * \return VTSS_RC_OK                                     : All ok
 *         VTSS_APPL_AUTH_ERROR_CHECKSUM_TO_SYSLOG_ERROR  : Something went wrong during fetch of checksum or submit of syslog
 */
mesa_rc vtss_auth_integrity_log(void);

/**
 * \brief vtss_auth_logout. Called when an agent wants to logout.
 *
 * \param agent     [IN]  The agent e.g. telnet or SSH.
 * \param hostname  [IN]  The hostname of the remote host (or NULL if no remote host).
 * \param username  [IN]  The username.
 * \param priv_lvl  [IN]  The privilege level for this session.
 * \param agent_id  [IN]  The agent id for this session.
 *
 * \return VTSS_RC_OK                        : Logout is successful.
 *         VTSS_APPL_AUTH_ERROR_SERVER_ERROR : Logout not registered.
 */
mesa_rc vtss_auth_logout(vtss_appl_auth_agent_t agent,
                         const char             *hostname,
                         const char             *username,
                         u8                     priv_lvl,
                         u16                    agent_id);

/**
 * \brief Auth error txt - converts error code to text
 */
const char *vtss_auth_error_txt(mesa_rc rc);

/**
 * \brief Initialize Auth module
 */
mesa_rc vtss_auth_init(vtss_init_data_t *data);

/* Encrypt the plain text of secret key with AES256 cryptography.
 * Or Decrypt the encrypted hex string of the secret key to plain text.
 * Return VTSS_RC_OK when encryption successfully, else error code.
 */
mesa_rc AUTH_secret_key_cryptography(BOOL is_encrypt, char *plain_txt, char *hex_str);

/* Validate the secret key is valid or not.
 * Return TRUE when the secret key is valid, otherwise FALSE
 */
BOOL AUTH_validate_secret_key(BOOL is_encrypted, const char *key);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_AUTH_API_H_ */
