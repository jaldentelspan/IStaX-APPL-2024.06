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

#ifndef _VTSS_APPL_AUTH_H_
#define _VTSS_APPL_AUTH_H_

#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/module_id.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * \file auth.h
 *
 * \brief This API provides typedefs and functions for the
 * Authentication module.
 */

/**
 * \brief Auth error codes (mesa_rc)
 */
enum {
    VTSS_APPL_AUTH_ERROR_SERVER_REJECT = MODULE_ERROR_START(VTSS_MODULE_ID_AUTH), /**< Server request rejected */
    VTSS_APPL_AUTH_ERROR_SERVER_ERROR,            /**< Server request error */
    VTSS_APPL_AUTH_ERROR_MUST_BE_PRIMARY_SWITCH,  /**< Operation only valid on primary switch */
    VTSS_APPL_AUTH_ERROR_CFG_TIMEOUT,             /**< Invalid timeout configuration parameter */
    VTSS_APPL_AUTH_ERROR_CFG_RETRANSMIT,          /**< Invalid retransmit configuration parameter */
    VTSS_APPL_AUTH_ERROR_CFG_DEADTIME,            /**< Invalid deadtime configuration parameter */
    VTSS_APPL_AUTH_ERROR_CFG_SECRET_KEY,          /**< Invalid secret key configuration parameter */
    VTSS_APPL_AUTH_ERROR_CFG_HOST,                /**< Invalid host name or IP address */
    VTSS_APPL_AUTH_ERROR_CFG_PORT,                /**< Invalid port configuration parameter */
    VTSS_APPL_AUTH_ERROR_CFG_HOST_PORT,           /**< Invalid host and port combination */
    VTSS_APPL_AUTH_ERROR_CFG_HOST_TABLE_FULL,     /**< Host table is full */
    VTSS_APPL_AUTH_ERROR_CFG_HOST_NOT_FOUND,      /**< Host not found */
    VTSS_APPL_AUTH_ERROR_CFG_AGENT,               /**< Invalid agent */
    VTSS_APPL_AUTH_ERROR_CFG_AGENT_METHOD,        /**< Invalid agent method */
    VTSS_APPL_AUTH_ERROR_CFG_PRIV_LVL,            /**< Invalid privilege level */
    VTSS_APPL_AUTH_ERROR_CACHE_EXPIRED,           /**< The cache entry matches username and password, but is expired */
    VTSS_APPL_AUTH_ERROR_CACHE_INVALID,           /**< The cache entry is invalid */
    VTSS_APPL_AUTH_ERROR_CFG_RADIUS_NAS_IPV4,     /**< Invalid RADIUS NAS-IP-Address configuration parameter */
    VTSS_APPL_AUTH_ERROR_CFG_RADIUS_NAS_IPV6,     /**< Invalid RADIUS NAS-IPv6-Address configuration parameter */
    VTSS_APPL_AUTH_ERROR_CHECKSUM_TO_SYSLOG_ERROR /**< Error occured during submit of syslog with checksum */
};

/**
 * \brief Auth configuration
 */
#define VTSS_APPL_AUTH_NUMBER_OF_SERVERS            5 /**< We use the same number for both RADIUS and TACACS+ servers */

#define VTSS_APPL_AUTH_TIMEOUT_DEFAULT              5 /**< Seconds */
#define VTSS_APPL_AUTH_TIMEOUT_MIN                  1 /**< Seconds */
#define VTSS_APPL_AUTH_TIMEOUT_MAX               1000 /**< Seconds */

#define VTSS_APPL_AUTH_RETRANSMIT_DEFAULT           3 /**< Times */
#define VTSS_APPL_AUTH_RETRANSMIT_MIN               1 /**< Times */
#define VTSS_APPL_AUTH_RETRANSMIT_MAX            1000 /**< Times */

#define VTSS_APPL_AUTH_DEADTIME_DEFAULT             0 /**< Minutes */
#define VTSS_APPL_AUTH_DEADTIME_MIN                 0 /**< Minutes */
#define VTSS_APPL_AUTH_DEADTIME_MAX              1440 /**< Minutes */

#define VTSS_APPL_AUTH_RADIUS_AUTH_PORT_DEFAULT  1812 /**< UDP port number */
#define VTSS_APPL_AUTH_RADIUS_ACCT_PORT_DEFAULT  1813 /**< UDP port number */
#define VTSS_APPL_AUTH_TACACS_PORT_DEFAULT         49 /**< TCP port number */

#define VTSS_APPL_AUTH_HOST_LEN                   256 /**< Maximum length for a hostname (incl. NULL) */
#define VTSS_APPL_AUTH_UNENCRYPTED_KEY_INPUT_LEN   63 /**< Maximum length for an unencrypted key (without NULL) */
#define VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN         64 /**< Maximum length for an unencrypted key (incl. NULL) */
#define VTSS_APPL_AUTH_ENCRYPTED_KEY_LEN(x)         (((16 + (((x) / 16) * 16) + 32) * 2) + 1) /**< Maximum length for a encrypted key */
#define VTSS_APPL_AUTH_KEY_LEN                      VTSS_APPL_AUTH_ENCRYPTED_KEY_LEN(VTSS_APPL_AUTH_UNENCRYPTED_KEY_LEN) /**< Maximum length for a key (incl. NULL) */

#define VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX           2 /**< Maximum priority index(0-based) of authentication method, where 0 is the highest priority index */

/**
 * \brief Auth authentication, authorization and accounting agents
 */
typedef enum {
    VTSS_APPL_AUTH_AGENT_CONSOLE, /**< serial port CLI */
    VTSS_APPL_AUTH_AGENT_TELNET,  /**< telnet CLI */
    VTSS_APPL_AUTH_AGENT_SSH,     /**< SSH CLI */
    VTSS_APPL_AUTH_AGENT_HTTP,    /**< HTTP and HTTPS WEB interface */
    VTSS_APPL_AUTH_AGENT_LAST     /**< The last one - ALWAYS insert above this entry */
} vtss_appl_auth_agent_t;

/**
 * \brief Auth authentication method
 */
typedef enum {
    VTSS_APPL_AUTH_AUTHEN_METHOD_NONE,   /**< Authentication disabled - login not possible */
    VTSS_APPL_AUTH_AUTHEN_METHOD_LOCAL,  /**< Local authentication */
    VTSS_APPL_AUTH_AUTHEN_METHOD_RADIUS, /**< RADIUS authentication */
    VTSS_APPL_AUTH_AUTHEN_METHOD_TACACS, /**< TACACS+ authentication */
    VTSS_APPL_AUTH_AUTHEN_METHOD_LAST    /**< The last one - ALWAYS insert above this entry */
} vtss_appl_auth_authen_method_t;

/**
 * \brief Auth authenticating agent configuration
 */
typedef struct {
    vtss_appl_auth_authen_method_t method[VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX + 1]; /**< List of authentication methods */
} vtss_appl_auth_authen_agent_conf_t;

/**
 * \brief Get authenticating agent configuration
 *
 * \param agent [IN]  Agent
 * \param conf  [OUT] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_authen_agent_conf_get(vtss_appl_auth_agent_t agent,
                                             vtss_appl_auth_authen_agent_conf_t *const conf);

/**
 * \brief Set authenticating agent configuration
 *
 * \param agent [IN]  Agent
 * \param conf  [IN]  Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_authen_agent_conf_set(vtss_appl_auth_agent_t agent,
                                             const vtss_appl_auth_authen_agent_conf_t *const conf);

/**
 * \brief Auth authorization method
 */
typedef enum {
    VTSS_APPL_AUTH_AUTHOR_METHOD_NONE,       /**< Authorization disabled - command will always be authorized */
    VTSS_APPL_AUTH_AUTHOR_METHOD_TACACS = 3, /**< TACACS+ authorization */
    VTSS_APPL_AUTH_AUTHOR_METHOD_LAST        /**< The last one - ALWAYS insert above this entry */
} vtss_appl_auth_author_method_t;

/**
 * \brief Auth authorization agent configuration
 */
typedef struct {
    vtss_appl_auth_author_method_t method; /**< Authorization method */
    mesa_bool_t cmd_enable;                       /**< If TRUE, authorize all commands with priv_lvl = 'cmd_priv_lvl' and above */
    uint8_t   cmd_priv_lvl;                     /**< Starting command privilege level (0..15) */
    mesa_bool_t cfg_cmd_enable;                   /**< If TRUE, authorize configuration commands also */
} vtss_appl_auth_author_agent_conf_t;

/**
 * \brief Get authorization agent configuration
 *
 * \param agent [IN]  Agent
 * \param conf  [OUT] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_author_agent_conf_get(vtss_appl_auth_agent_t agent,
                                             vtss_appl_auth_author_agent_conf_t *const conf);

/**
 * \brief Set authorization agent configuration
 *
 * \param agent [IN]  Agent
 * \param conf  [IN]  Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_author_agent_conf_set(vtss_appl_auth_agent_t agent,
                                             const vtss_appl_auth_author_agent_conf_t *const conf);

/**
 * \brief Auth accounting method
 */
typedef enum {
    VTSS_APPL_AUTH_ACCT_METHOD_NONE,       /**< Accounting disabled. */
    VTSS_APPL_AUTH_ACCT_METHOD_TACACS = 3, /**< TACACS+ accounting */
    VTSS_APPL_AUTH_ACCT_METHOD_LAST        /**< The last one - ALWAYS insert above this entry */
} vtss_appl_auth_acct_method_t;

/**
 * \brief Auth accounting agent configuration
 */
typedef struct {
    vtss_appl_auth_acct_method_t method; /**< Accounting method */
    mesa_bool_t exec_enable;                    /**< Send accounting records for login/logout */
    mesa_bool_t cmd_enable;                     /**< If TRUE, send accounting records for all commands with priv_lvl = 'cmd_priv_lvl' and above */
    uint8_t   cmd_priv_lvl;                   /**< Starting command privilege level (0..15) */
} vtss_appl_auth_acct_agent_conf_t;

/**
 * \brief Get accounting agent configuration
 *
 * \param agent [IN]  Agent
 * \param conf  [OUT] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_acct_agent_conf_get(vtss_appl_auth_agent_t agent,
                                           vtss_appl_auth_acct_agent_conf_t *const conf);

/**
 * \brief Set accounting agent configuration
 *
 * \param agent [IN]  Agent
 * \param conf  [IN]  Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_acct_agent_conf_set(vtss_appl_auth_agent_t agent,
                                           const vtss_appl_auth_acct_agent_conf_t *const conf);

/**
 * Auth remote server configuration, specific host index
 */
typedef uint32_t vtss_auth_host_index_t;

#ifdef VTSS_SW_OPTION_RADIUS
/**
 * Auth RADIUS global configuration
 */
typedef struct {
    uint32_t         timeout;                                 /**< Global timeout. Can be overridden for each server. */
    uint32_t         retransmit;                              /**< Global retransmit. Can be overridden for each server. */
    uint32_t         deadtime;                                /**< Global deadtime. */
    mesa_bool_t        encrypted;                               /**< The flag indicates the secret key is encrypted or not. TRUE means the secret key is an encrypted string. FALSE means the secret key is a plain text. */
    char        key[VTSS_APPL_AUTH_KEY_LEN];             /**< Global secret key. Can be overridden for each server. */
    mesa_bool_t        nas_ip_address_enable;                   /**< Enable NAS-IP-Address if TRUE. */
    mesa_ipv4_t nas_ip_address;                          /**< Global NAS-IP-Address. */
    mesa_bool_t        nas_ipv6_address_enable;                 /**< Enable NAS-IPv6-Address if TRUE. */
    mesa_ipv6_t nas_ipv6_address;                        /**< Global NAS-IPv6-Address. */
    char        nas_identifier[VTSS_APPL_AUTH_HOST_LEN]; /**< Global NAS-Identifier. */
} vtss_appl_auth_radius_global_conf_t;

/**
 * \brief Get RADIUS configuration
 *
 * \param conf  [OUT] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_radius_global_conf_get(vtss_appl_auth_radius_global_conf_t *const conf);

/**
 * \brief Set RADIUS global configuration
 *
 * \param conf  [IN]  Configuration
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_radius_global_conf_set(const vtss_appl_auth_radius_global_conf_t *const conf);

/**
 * Auth RADIUS server configuration
 */
typedef struct {
    char host[VTSS_APPL_AUTH_HOST_LEN]; /**< IPv4, IPv6 or hostname of this server. Entry not used if zero. */
    uint32_t  auth_port;                     /**< Authentication port number to use on this server. */
    uint32_t  acct_port;                     /**< Accounting port number to use on this server. */
    uint32_t  timeout;                       /**< Seconds to wait for a response from this server. Use global timeout if zero. */
    uint32_t  retransmit;                    /**< Number of times a request is resent to an unresponding server. Use global retransmit if zero. */
    mesa_bool_t encrypted;                     /**< The flag indicates the secret key is encrypted or not. TRUE means the secret key is an encrypted string. FALSE means the secret key is a plain text. */
    char key[VTSS_APPL_AUTH_KEY_LEN];   /**< The secret key to use on this server. Use global key if zero */
} vtss_appl_auth_radius_server_conf_t;

/**
 * \brief Get a RADIUS server configuration
 *
 * \param ix    [IN]  Index (0 .. VTSS_APPL_AUTH_NUMBER_OF_SERVERS-1)
 * \param conf  [OUT] Configuration
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_radius_server_get(vtss_auth_host_index_t ix,
                                         vtss_appl_auth_radius_server_conf_t *conf);

/**
 * \brief Set a RADIUS server configuration
 *
 * An entry with an empty hostname is regarded as inactive.
 *
 * \param ix    [IN]  Index
 * \param conf  [IN]  Configuration
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_radius_server_set(vtss_auth_host_index_t ix,
                                         const vtss_appl_auth_radius_server_conf_t *conf);
#endif /* VTSS_SW_OPTION_RADIUS */

/**
 * Auth TACACS+ global configuration
 */
typedef struct {
    uint32_t         timeout;                     /**< Global timeout. Can be overridden for each server. */
    uint32_t         deadtime;                    /**< Global deadtime. */
    mesa_bool_t        encrypted;                   /**< The flag indicates the secret key is encrypted or not. TRUE means the secret key is an encrypted string. FALSE means the secret key is a plain text. */
    char        key[VTSS_APPL_AUTH_KEY_LEN]; /**< Global secret key. Can be overridden for each server. */
} vtss_appl_auth_tacacs_global_conf_t;

/**
 * \brief Get TACACS+ global configuration
 *
 * \param conf  [OUT] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_tacacs_global_conf_get(vtss_appl_auth_tacacs_global_conf_t *const conf);

/**
 * \brief Set TACACS+ global configuration
 *
 * \param conf  [IN]  Configuration
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_tacacs_global_conf_set(const vtss_appl_auth_tacacs_global_conf_t *const conf);

/**
 * Auth TACACS+ server configuration
 */
typedef struct {
    char host[VTSS_APPL_AUTH_HOST_LEN]; /**< IPv4, IPv6 or hostname of this server. Entry not used if zero. */
    uint32_t  port;                          /**< Authentication, authorization and accounting port number to use on this server. */
    uint32_t  timeout;                       /**< Seconds to wait for a response from this server. Use global timeout if zero. */
    mesa_bool_t no_single;                     /**< If TRUE, don't use a single TCP connections for multiple sessions. */
    mesa_bool_t encrypted;                     /**< The flag indicates the secret key is encrypted or not. TRUE means the secret key is an encrypted string. FALSE means the secret key is a plain text. */
    char key[VTSS_APPL_AUTH_KEY_LEN];   /**< The secret key to use on this server. Use global key if zero */
} vtss_appl_auth_tacacs_server_conf_t;

/**
 * \brief Get a TACACS+ server configuration
 *
 * \param ix    [IN]  Index (0 .. VTSS_APPL_AUTH_NUMBER_OF_SERVERS-1)
 * \param conf  [OUT] Configuration
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_tacacs_server_get(vtss_auth_host_index_t ix,
                                         vtss_appl_auth_tacacs_server_conf_t *conf);

/**
 * \brief Set a TACACS+ server configuration
 *
 * An entry with an empty hostname is regarded as inactive.
 *
 * \param ix    [IN]  Index
 * \param conf  [IN]  Configuration
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc vtss_appl_auth_tacacs_server_set(vtss_auth_host_index_t ix,
                                         const vtss_appl_auth_tacacs_server_conf_t *conf);

/**
 * \brief Convert an authentication agent to a string
 *
 * \param agent [IN]  Auth agent
 *
 * \return a printable string representing the input parameter
 */
static inline const char *vtss_appl_auth_agent_name(vtss_appl_auth_agent_t agent)
{
    switch (agent) {
    case VTSS_APPL_AUTH_AGENT_CONSOLE:
        return "console";
    case VTSS_APPL_AUTH_AGENT_TELNET:
        return "telnet";
    case VTSS_APPL_AUTH_AGENT_SSH:
        return "ssh";
    case VTSS_APPL_AUTH_AGENT_HTTP:
        return "http";
    default:
        ;
    }
    return "unknown";
}

/**
 * \brief Convert an authentication method to a string
 *
 * \param method [IN]  Authentication method
 *
 * \return a printable string representing the input parameter
 */
static inline const char *vtss_appl_auth_authen_method_name(vtss_appl_auth_authen_method_t method)
{
    switch (method) {
    case VTSS_APPL_AUTH_AUTHEN_METHOD_NONE:
        return "no";
    case VTSS_APPL_AUTH_AUTHEN_METHOD_LOCAL:
        return "local";
    case VTSS_APPL_AUTH_AUTHEN_METHOD_RADIUS:
        return "radius";
    case VTSS_APPL_AUTH_AUTHEN_METHOD_TACACS:
        return "tacacs";
    default:
        ;
    }
    return "unknown";
}

/**
 * \brief Convert an authorization method to a string
 *
 * \param method [IN]  Authorization method
 *
 * \return a printable string representing the input parameter
 */
static inline const char *vtss_appl_auth_author_method_name(vtss_appl_auth_author_method_t method)
{
    switch (method) {
    case VTSS_APPL_AUTH_AUTHOR_METHOD_NONE:
        return "no";
    case VTSS_APPL_AUTH_AUTHOR_METHOD_TACACS:
        return "tacacs";
    default:
        ;
    }
    return "unknown";
}

/**
 * \brief Convert an accounting method to a string
 *
 * \param method [IN]  Accounting method
 *
 * \return a printable string representing the input parameter
 */
static inline const char *vtss_appl_auth_acct_method_name(vtss_appl_auth_acct_method_t method)
{
    switch (method) {
    case VTSS_APPL_AUTH_ACCT_METHOD_NONE:
        return "no";
    case VTSS_APPL_AUTH_ACCT_METHOD_TACACS:
        return "tacacs";
    default:
        ;
    }
    return "unknown";
}

/**
 * \brief Return the default values for an authentication agent configuration.
 *
 * \param conf [OUT]  Structure to initialize with default configuration values.
 *
 * \return The input parameter
 */
static inline vtss_appl_auth_authen_agent_conf_t *vtss_appl_auth_authen_agent_conf_default(vtss_appl_auth_authen_agent_conf_t *conf)
{
    if (conf) {
        vtss_appl_auth_authen_agent_conf_t def_conf = {{VTSS_APPL_AUTH_AUTHEN_METHOD_LOCAL}};
        *conf = def_conf;
    }
    return conf;
}

/**
 * \brief Return the default values for an authorization agent configuration.
 *
 * \param conf [OUT]  Structure to initialize with default configuration values.
 *
 * \return The input parameter
 */
static inline vtss_appl_auth_author_agent_conf_t *vtss_appl_auth_author_agent_conf_default(vtss_appl_auth_author_agent_conf_t *conf)
{
    if (conf) {
        vtss_appl_auth_author_agent_conf_t def_conf = {VTSS_APPL_AUTH_AUTHOR_METHOD_NONE};
        *conf = def_conf;
    }
    return conf;
}

/**
 * \brief Return the default values for an accounting agent configuration.
 *
 * \param conf [OUT]  Structure to initialize with default configuration values.
 *
 * \return The input parameter
 */
static inline vtss_appl_auth_acct_agent_conf_t *vtss_appl_auth_acct_agent_conf_default(vtss_appl_auth_acct_agent_conf_t *conf)
{
    if (conf) {
        vtss_appl_auth_acct_agent_conf_t def_conf = {VTSS_APPL_AUTH_ACCT_METHOD_NONE};
        *conf = def_conf;
    }
    return conf;
}

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_AUTH_H_ */
