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
 * \brief SNMPaccess and security management API.
 * \details This header file describes public functions applicable to SNMP management.
 *          User can configure the device to operate on SNMPv1, SNMPv2 or SNMPv3. When the device is
 *          configured to SNMPv1 or SNMPv2, SNMP users can access the device or not according to
 *          read/write community of global configuration.
 *          When the device is configured to SNMPv3, the device will give the SNMP users different
 *          read/write permission according the following 5 tables:
 *
 * Community Table - This table is used when authenticate SNMPv1/2c requests. The
 *                  community string from the SNMPv1/v2c PDU is used along with the source IP
 *                  address from the PDU, this information is match against the entries in the
 *                  table. If a match is found, the request has passed the authentication and the
 *                  processing may continue, otherwise the request is rejected. Additionally
 *                  If the request is authenticated successfully, then the community name
 *                  will be used as security name in the further processing.
 *                  The network only supports IPv4, so SNMPv2 user can't access DUT via IPv6.
 *                  If the table is empty, it means no SNMPv1/SNMPv2 user can access it.
 *                  The device will check this table for v1/v2 communities to determine the v1/v2
 *                  communities is authenticated or not.
 *
 * User Table - Thes used to authenticate incoming SNMPv3 requests, and when producing outgoing
 *                  SNMPv3 traps. There are two keys here, username and engine ID. For SNMPv3, every
 *                  SNMP agent has a engine ID, and when a user needs to access the specific device,
 *                  it must know the engine ID of the SNMP agent. If the engine ID is the device's
 *                  agent ID, it means the entry is used for SNMP client to get/set the SNMP agent.
 *                  On the other hands, if the engine ID is not the device's engine ID, it means
 *                  the entry is used for the device to send trap to SNMP trap server. The device
 *                  will check this table for v3 users to determine the v3 user is authenticated
 *                  or not.
 *
 * UserToAccessGroup Table - The table indicates which access group the user belongs to. Every users
 *                  and SNMPv1/SNMPv2 communities must be included in a specific access group.
 *                  Access Group Table will give different view level for different access group and
 *                  different secure level. This table is used for SNMPv1/v2c/v3 users.
 *
 * View Table - The table is used to configuring if a given OID tree can be access or not.
 *                  The entry is referenced by Access Group Table.This table is used for
 *                  SNMPv1/v2c/v3 users.
 *
 * Access Group Table - The table combines the UserToAccessGroup entry and view entry to determine
 *                  the access group can access the device or not. This table is used for
 *                  SNMPv1/v2c/v3 users.
 *
 * The relationship among the SNMPv3 table is shown as following:
 * \verbatim

    User  +----------------------------+    Community +-----------+
    Table | User Name | Security Level |    Table     | Community |
          +------+---------------------+              +-----+-----+
                 |                                          |
                 |                                          |
                 +-------------------------------+----------+
                                                 |
                                                \|/
                +--------------------------+-----------+---------+
                | Security Model           |           |         |
    UserTo-     |                          | User/     | Access  |
    AccessGroup | (User Table->usm)        | Community | Group   |
      Table     | (Community Table->v1/v2) |           |         |
                +-----------+--------------+-----------+---+-----+
                           A|                             B|
                            |                              |
                  +---------+------------------------------+
                  |         |
                 B|        A|
                 \|/       \|/
    Access +---------+-----------+----------+----------+-----------+
    Group  | Access  | Security  | Security |   Read   |   Write   |
    Table  |  Group  |  Model    |  Level   |   View   |   View    |
           +---------+-----------+----------+------+---+-----+-----+
                                                   |         |
                                                   |         |
                                                   +----+----+
                                                        |
        X                                              \|/
        |                                     View  +-------------+
        |  means Y is referenced by X         Table | View | type |
       \|/                                          +---+---------+
        Y


 * \endverbatim
 *
 * After configuring the above 5 tables, SNMP agent will give the access right base on the
 * following steps.
 *      1. After authentication, the device will find the access group which the user belongs to
 *         according to the SNMPv3's user name or SNMPv1/SNMPv2's community.
 *      2. SNMP agent take the access group and user's security model(v1/v2/usm) and security
 *         level(noAuth/authNoPriv/authPriv) to get the entry of Access Group Table.
 *      3. Then SNMP agent get the view entry according to the read view name and write view name
 *         of the access group entry to get
 *      4. Finally, SNMP agent determine which OID subtree can be accessed or not according to
 *         the veiw entry.
 *
 * Trap configuration:
 *      1. Trap receivers are stored in the Trap Receiver table. All receivers will get all enabled
 *         traps. This table is referencing the User Table for configured v3 trap receivers and
 *         will use auth and priv options from that user.
 *         See: vtss_appl_trap_receiver_index_t, vtss_appl_trap_receiver_conf_t
 *      2. Trap sources are given on a per event/table basis together with a subtree index_filter
 *         within that table. These can be given as include or exclude. For a trap to be sent it
 *         needs to match at least one include rule and no exclude rules for that table. An empty
 *         index_filter matches all.
 *         See: vtss_appl_trap_source_index_t, vtss_appl_trap_source_conf_t
 *
 */

#ifndef _VTSS_APPL_SNMP_H_
#define _VTSS_APPL_SNMP_H_

#include <vtss/appl/interface.h>
#include <vtss/appl/types.h>        /* For PortListStackable */

#ifdef __cplusplus
extern "C" {
#endif

/** The maximum length of SNMP community  */
#define VTSS_APPL_SNMP_COMMUNITY_LEN                        63

/** The minimum length of SNMP engine ID  */
#define VTSS_APPL_SNMP_ENGINE_ID_MIN_LEN                    5

/** The maximum length of SNMP engine ID  */
#define VTSS_APPL_SNMP_ENGINE_ID_MAX_LEN                    32

/** The maximum length of SNMPv3 name */
#define VTSS_APPL_SNMP_MAX_NAME_LEN                         32

/** The maximum length of SHA password */
#define VTSS_APPL_SNMP_MAX_SHA_PASSWORD_LEN                 40

/** The maximum length of DES password */
#define VTSS_APPL_SNMP_MAX_DES_PASSWORD_LEN                 32

/** The maximum length of subtree */
#define VTSS_APPL_SNMP_MAX_SUBTREE_STR_LEN                  64

/** SNMP max OID length */
#define VTSS_APPL_SNMP_MAX_OID_LEN                          127

/** SNMP max OID mask length */
#define VTSS_APPL_SNMP_MAX_SUBTREE_LEN                      16

/** The max length of event/table names for traps */
#define VTSS_APPL_TRAP_TABLE_NAME_SIZE                      64

/** SNMP max trap inform timeout */
#define VTSS_APPL_SNMP_TRAP_TIMEOUT_MAX                     2147 

/** SNMP max trap inform retry times */
#define VTSS_APPL_SNMP_TRAP_RETRIES_MAX                     255

/** SNMP max number of trap filters per trap source */
#define VTSS_APPL_SNMP_TRAP_FILTER_MAX                      128

/**
 * This enum identifies for SNMP version.
 */
typedef enum {
    VTSS_APPL_SNMP_VERSION_1,       /**< SNMPv1  */
    VTSS_APPL_SNMP_VERSION_2C,      /**< SNMPv2c */
    VTSS_APPL_SNMP_VERSION_3,       /**< SNMPv3 */
} vtss_appl_snmp_version_t;

/**
 * This enum identifies for SNMP security model.
 */
typedef enum {
    VTSS_APPL_SNMP_SECURITY_MODEL_ANY,       /**< Any security model accepted (v1|v2c|usm)  */
    VTSS_APPL_SNMP_SECURITY_MODEL_SNMPV1,    /**< Reserved for SNMPv1 */
    VTSS_APPL_SNMP_SECURITY_MODEL_SNMPV2C,   /**< Reserved for SNMPv2 */
    VTSS_APPL_SNMP_SECURITY_MODEL_USM        /**< User-based Security Model (USM)  */
} vtss_appl_snmp_security_model_t;

/**
 * This enum identifies for SNMP security model.
 */
typedef enum {
    VTSS_APPL_SNMP_SECURITY_LEVEL_NOAUTH = 1,   /**< No authentication and no privacy  */
    VTSS_APPL_SNMP_SECURITY_LEVEL_AUTHNOPRIV,   /**< Authentication and no privacy  */
    VTSS_APPL_SNMP_SECURITY_LEVEL_AUTHPRIV,     /**< Authentication and privacy */
} vtss_appl_snmp_security_level_t;

/**
 * This enum identifies for SNMP authentication protocol.
 */
typedef enum {
    VTSS_APPL_SNMP_AUTH_PROTOCOL_NONE,      /**< No authentication protocol  */
    VTSS_APPL_SNMP_AUTH_PROTOCOL_MD5,       /**< MD5 authentication protocol  */
    VTSS_APPL_SNMP_AUTH_PROTOCOL_SHA,       /**< SHA authentication protocol */
} vtss_appl_snmp_auth_protocol_t;

/**
 * This enum identifies for SNMP privacy protocol.
 */
typedef enum {
    VTSS_APPL_SNMP_PRIV_PROTOCOL_NONE,      /**< No privacy protocol.  */
    VTSS_APPL_SNMP_PRIV_PROTOCOL_DES,       /**< DES privacy protocol  */
    VTSS_APPL_SNMP_PRIV_PROTOCOL_AES,       /**< AES privacy protocol */
} vtss_appl_snmp_priv_protocol_t;

/**
 * This enum identifies for SNMP view type.
 */
typedef enum {
    VTSS_APPL_SNMP_VIEW_TYPE_INCLUDED,       /**< This entry is included to view  */
    VTSS_APPL_SNMP_VIEW_TYPE_EXCLUDED        /**< This entry is excluded to view  */
} vtss_appl_snmp_view_type_t;

/**
 * This enum identifies for notification type.
 *
 */
typedef enum {
    VTSS_APPL_TRAP_NOTIFY_TRAP,     /**< Trap - as defined in RFC1905 section 4.2.6 */
    VTSS_APPL_TRAP_NOTIFY_INFORM,   /**< Inform - as defined in RFC1905 section 4.2.7  */
} vtss_appl_trap_notify_type_t;

/** \brief SNMP global configuration */
typedef struct {
    /**
     * The administrative mode of SNMP
     */
    mesa_bool_t                            mode;

    /**
     * SNMPv3 engine ID. The content of engineID is hexadecimal data which length
     * is from 5 bytes to 32 bytes, but all-zeros and all-'F's are not allowed.
     * Change of the Engine ID will clear all original local users.
     */
    uint8_t                              engineid[VTSS_APPL_SNMP_ENGINE_ID_MAX_LEN];

    /**
     * SNMPv3 engine ID length
     */
    uint32_t                             engineid_len;
} vtss_appl_snmp_conf_t;

/** \brief SNMP community entry index type. */
typedef struct {
    /**
     * Name of SNMP community - encoded as a c-string.
     */
    char                    name[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];

    /**
     * The network range allowed access to the SNMP agent with this community:
     * IPv4 range.
     */
    mesa_ipv4_network_t sip;
} vtss_appl_snmp_community_index_t;

/** \brief SNMP community entry index type. */
typedef struct {
    /**
     * Name of SNMP community - encoded as a c-string.
     */
    char                    name[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];

    /**
     * The network range allowed access to the SNMP agent with this community:
     * IPv6 range.
     */
    mesa_ipv6_network_t sip_ipv6;
} vtss_appl_snmp_community6_index_t;

/** \brief SNMP community Configuration. */
typedef struct {
    /**
     * The SNMP community secret - encoded as a c-string.
     */
    char                    secret[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];
} vtss_appl_snmp_community_conf_t;

/** \brief SNMP user entry index type. */
typedef struct {
    /**
     * The engine ID of trap destination, it only effects on SNMPv3
     */
    uint8_t                                  engineid[VTSS_APPL_SNMP_ENGINE_ID_MAX_LEN];

    /**
     * The length of engine ID of trap destination, it only effects on SNMPv3
     */
    uint32_t                                 engineid_len;

    /**
     * The user name this entry should belong to.
     */
    char                                user_name[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];

} vtss_appl_snmp_user_index_t;

/** \brief SNMP user entry Configuration. */
typedef struct {

    /**
     * The security model of the user entry.
     */
    vtss_appl_snmp_security_level_t     security_level;

    /**
     * The authentication protocol.
     */
    vtss_appl_snmp_auth_protocol_t      auth_protocol;

    /**
     * The authentication password the user entry.
     */
    char                                    auth_password[VTSS_APPL_SNMP_MAX_SHA_PASSWORD_LEN + 1];

    /**
     * The privacy protocol.
     */
    vtss_appl_snmp_priv_protocol_t      priv_protocol;

    /**
     * The privacy password the user entry.
     */
    char                                    priv_password[VTSS_APPL_SNMP_MAX_DES_PASSWORD_LEN + 1];
} vtss_appl_snmp_user_conf_t;

/** \brief SNMP UserToAccessGroup entry index type. */
typedef struct {
    /**
     * The security model this entry should belong to.
     * The security model depands on the user_or_community is user or community,
     * if user_or_community is user, the model must be VTSS_APPL_SNMP_SEC_MODEL_USM,
     * otherwise, it can be VTSS_APPL_SNMP_SEC_MODEL_SNMPV1 or VTSS_APPL_SNMP_SEC_MODEL_SNMPV2C.
     */
    vtss_appl_snmp_security_model_t     security_model;

    /**
     * The security name this entry should belong to.
     * The value is defined in User Table or Community Table.
     */
    char                                    user_or_community[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];
} vtss_appl_snmp_user_to_access_group_index_t;

/** \brief SNMP UserToAccessGroup entry Configuration. */
typedef struct {
    /**
     * The access group name this entry should belong to.
     */
    char                                    access_group_name[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];
} vtss_appl_snmp_user_to_access_group_conf_t;

/** \brief SNMP view entry index type. */
typedef struct {
    /**
     * The name of the view entry.
     */
    char    view_name[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];

    // 
    /**
     * The subtree of the view entry.
     */
    char    subtree[VTSS_APPL_SNMP_MAX_SUBTREE_STR_LEN + 1];
} vtss_appl_snmp_view_index_t;

/** \brief SNMP view entry Configuration. */
typedef struct {
    /**
     * The view type of the entry.
     */
    vtss_appl_snmp_view_type_t   view_type;
} vtss_appl_snmp_view_conf_t;

/** \brief SNMP access group entry index type. */
typedef struct {
    /**
     * The access group name of the access group entry.
     * The value is defined in UserToAccessGroup Table.
     */
    char                                    access_group_name[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];

    /**
     * The security model the access group entry.
     * The value is defined in UserToAccessGroup Table.
     */
    vtss_appl_snmp_security_model_t     security_model;

    /**
     * The security model the access group entry.
     * The value is defined in User Table.
     */
    vtss_appl_snmp_security_level_t     security_level;
} vtss_appl_snmp_access_group_index_t;

/** \brief SNMP access group entry Configuration. */
typedef struct {
    /**
     * The name of the MIB view defining the MIB objects for which this request may request the
     * current values. The value is defined in View Table.
     */
    char      read_view_name[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];

    /**
     * The name of the MIB view defining the MIB objects for which this request may potentially
     * set new values. The value is defined in View Table.
     */
    char      write_view_name[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];
} vtss_appl_snmp_access_group_conf_t;

/** \brief SNMP Trap receiver entry index */
typedef struct {
    char                                name[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];                  /**< Name of trap receiver - encoded as a c-string. */
} vtss_appl_trap_receiver_index_t;

/** \brief SNMP Trap receiver entry configuration */
typedef struct {
    mesa_bool_t                            enable;                                                 /**< Enable the trap receiver */
    vtss_inet_address_t             dest_addr;                                              /**< The destination of the trap, it can be hostname, ipv4 address, or ipv6 address */
    uint16_t                             port;                                                   /**< The UDP port of the trap */
    vtss_appl_snmp_version_t        version;                                                /**< The version of the trap */
    char                                community[VTSS_APPL_SNMP_COMMUNITY_LEN + 1];            /**< The community of the trap, it only works when the version is v1 or v2c */
    vtss_appl_trap_notify_type_t    notify_type;                                            /**< The type of the notification */
    uint32_t                             timeout;                                                /**< The timeout of notification, it only effect for inform notification. */
    uint32_t                             retries;                                                /**< The retries of notification, it only effect for inform notification. */
    uint8_t                              engineid[VTSS_APPL_SNMP_ENGINE_ID_MAX_LEN];             /**< The engine ID of trap destination, only affects SNMPv3 */
    uint32_t                             engineid_len;                                           /**< The length of engine ID of trap destination, only affects SNMPv3 */
    char                                user_name[VTSS_APPL_SNMP_MAX_NAME_LEN + 1];             /**< The user name for authentication, only affects SNMPv3 */
} vtss_appl_trap_receiver_conf_t;

/**
 * \brief SNMP Trap source entry index
 *
 * A trap is sent for the given trap source if at least one filter with
 * filter_type included matches the index OID, and no filters with filter_type
 * excluded matches.
 *
 */
typedef struct {
    char                                name[VTSS_APPL_TRAP_TABLE_NAME_SIZE + 1];               /**< Event/table for this trap source */
    uint32_t                             index_filter_id;                                        /**< ID of the index filter local to this trap source */
} vtss_appl_trap_source_index_t;

/**
 * \brief SNMP Trap source entry configuration
 *
 * The filter matches an index OID if the index_filter matches the OID of the
 * index taking index_mask into account.
 *
 * Each bit of index_mask corresponds to a sub-identifier of index_filter,
 * with the most significant bit of the i-th octet of this octet string value
 * (extended if necessary) corresponding to the (8*i)-th sub-identifier.
 *
 * Each bit of index_mask specifies whether or not the corresponding
 * sub-identifiers must match when determining if an OID matches; a '1'
 * indicates that an exact match must occur; a '0' indicates 'wild card'.
 *
 * If the value of index_mask is M bits long and there are more than M
 * sub-identifiers in index_filter, then the bit mask is extended with 1's to
 * be the required length.
 *
 */
typedef struct {
    vtss_appl_snmp_view_type_t      filter_type;                                            /**< The filter type of the entry; included or excluded */
    uint32_t                             index_filter_len;                                       /**< Length of index_filter */
    uint32_t                             index_filter[VTSS_APPL_SNMP_MAX_OID_LEN];               /**< Subtree to match for this filter */
    uint32_t                             index_mask_len;                                         /**< Length of index_mask */
    uint8_t                              index_mask[VTSS_APPL_SNMP_MAX_SUBTREE_LEN];             /**< Mask for the subtree to match for this filter */
} vtss_appl_trap_source_conf_t;

/**
 * \brief Set SNMP configuration
 *
 * To Set the SNMP global configuration.
 *
 * \param conf      [IN]    The global configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_conf_set(const vtss_appl_snmp_conf_t *const conf);

/**
 * \brief Get SNMP configuration
 *
 * To Get the SNMP global configuration.
 *
 * \param conf      [OUT]    The global configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_conf_get(vtss_appl_snmp_conf_t *const conf);

/**
 * \brief Iterate function of SNMP community table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community_itr(
    const vtss_appl_snmp_community_index_t    *const prev_index,
    vtss_appl_snmp_community_index_t          *const next_index
);

/**
 * \brief Get the entry of SNMP community table
 *
 * To get the specific entry in SNMP community table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP community entry.
 * \param conf          [OUT]   The current entry of the SNMP community entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community_get(
    const vtss_appl_snmp_community_index_t   conf_index,
    vtss_appl_snmp_community_conf_t          *const conf
);

/**
 * \brief Set the entry of SNMP community table
 *
 * To modify the specific entry in SNMP community table.
 *
 * \param conf_index    [IN]    (key) Index of the SNMP community entry.
 * \param conf          [IN]    The revised the SNMP community entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community_set(
    const vtss_appl_snmp_community_index_t   conf_index,
    const vtss_appl_snmp_community_conf_t    *const conf
);

/**
 * \brief Delete the entry of SNMP community table
 *
 * To delete the specific entry in SNMP community table.
 *
 * \param conf_index    [IN]    (key) Index of the SNMP community entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community_del(
    const vtss_appl_snmp_community_index_t   conf_index
);

/**
 * \brief Add new entry of SNMP community table
 *
 * To Add new entry in SNMP community table.
 *
 * \param conf_index    [IN]    (key) Index of the SNMP community entry.
 * \param conf          [IN]    The new entry of SNMP community table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community_add(
    const vtss_appl_snmp_community_index_t   conf_index,
    const vtss_appl_snmp_community_conf_t    *const conf
);

/**
 * \brief Get default value of SNMP community table
 *
 * To add new entry in SNMP community table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP community entry.
 * \param conf          [OUT]    The new entry of SNMP community table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community_default(
    vtss_appl_snmp_community_index_t  *const conf_index,
    vtss_appl_snmp_community_conf_t   *const conf
);

/**
 * \brief Iterate function of SNMP community table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community6_itr(
    const vtss_appl_snmp_community6_index_t   *const prev_index,
    vtss_appl_snmp_community6_index_t         *const next_index
);

/**
 * \brief Get the entry of SNMP community table
 *
 * To get the specific entry in SNMP community table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP community entry.
 * \param conf          [OUT]   The current entry of the SNMP community entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community6_get(
    const vtss_appl_snmp_community6_index_t  conf_index,
    vtss_appl_snmp_community_conf_t          *const conf
);

/**
 * \brief Set the entry of SNMP community table
 *
 * To modify the specific entry in SNMP community table.
 *
 * \param conf_index    [IN]    (key) Index of the SNMP community entry.
 * \param conf          [IN]    The revised the SNMP community entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community6_set(
    const vtss_appl_snmp_community6_index_t  conf_index,
    const vtss_appl_snmp_community_conf_t    *const conf
);

/**
 * \brief Delete the entry of SNMP community table
 *
 * To delete the specific entry in SNMP community table.
 *
 * \param conf_index    [IN]    (key) Index of the SNMP community entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community6_del(
    const vtss_appl_snmp_community6_index_t  conf_index
);

/**
 * \brief Add new entry of SNMP community table
 *
 * To Add new entry in SNMP community table.
 *
 * \param conf_index    [IN]    (key) Index of the SNMP community entry.
 * \param conf          [IN]    The new entry of SNMP community table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community6_add(
    const vtss_appl_snmp_community6_index_t  conf_index,
    const vtss_appl_snmp_community_conf_t    *const conf
);

/**
 * \brief Get default value of SNMP community table
 *
 * To add new entry in SNMP community table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP community entry.
 * \param conf          [OUT]    The new entry of SNMP community table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_community6_default(
    vtss_appl_snmp_community6_index_t *const conf_index,
    vtss_appl_snmp_community_conf_t   *const conf
);

/**
 * \brief Iterate function of SNMP user table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_itr(
    const vtss_appl_snmp_user_index_t    *const prev_index,
    vtss_appl_snmp_user_index_t          *const next_index
);

/**
 * \brief Get the entry of SNMP user table
 *
 * To get the specific entry in SNMP user table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP user entry.
 * \param conf          [OUT]   The current entry of the SNMP user entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_get(
    const vtss_appl_snmp_user_index_t  conf_index,
    vtss_appl_snmp_user_conf_t         *const conf
);

/**
 * \brief Set the entry of SNMP user table
 *
 * To modify the specific entry in SNMP user table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP user entry.
 * \param conf          [IN]    The revised the SNMP user entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_set(
    const vtss_appl_snmp_user_index_t  conf_index,
    const vtss_appl_snmp_user_conf_t   *const conf
);

/**
 * \brief Delete the entry of SNMP user table
 *
 * To delete the specific entry in SNMP user table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP user entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_del(
    const vtss_appl_snmp_user_index_t  conf_index
);

/**
 * \brief Add new entry of SNMP user table
 *
 * To add new entry in SNMP user table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP user entry.
 * \param conf          [IN]    The new entry of SNMP user table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_add(
    const vtss_appl_snmp_user_index_t  conf_index,
    const vtss_appl_snmp_user_conf_t   *const conf
);

/**
 * \brief Get default value of SNMP user table
 *
 * To add new entry in SNMP user table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP user entry.
 * \param conf          [OUT]    The new entry of SNMP user table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */

mesa_rc vtss_appl_snmp_user_default(
    vtss_appl_snmp_user_index_t  *const conf_index,
    vtss_appl_snmp_user_conf_t   *const conf
);

/**
 * \brief Iterate function of SNMP UserToAccessGroup table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_to_access_group_itr(
    const vtss_appl_snmp_user_to_access_group_index_t    *const prev_index,
    vtss_appl_snmp_user_to_access_group_index_t          *const next_index
);

/**
 * \brief Get the entry of SNMP UserToAccessGroup table
 *
 * To get the specific entry in SNMP UserToAccessGroup table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP UserToAccessGroup entry.
 * \param conf          [OUT]   The current entry of the SNMP UserToAccessGroup entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_to_access_group_get(
    const vtss_appl_snmp_user_to_access_group_index_t  conf_index,
    vtss_appl_snmp_user_to_access_group_conf_t         *const conf
);

/**
 * \brief Set the entry of SNMP UserToAccessGroup table
 *
 * To modify the specific entry in SNMP UserToAccessGroup table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP UserToAccessGroup entry.
 * \param conf          [IN]    The revised the SNMP UserToAccessGroup entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_to_access_group_set(
    const vtss_appl_snmp_user_to_access_group_index_t  conf_index,
    const vtss_appl_snmp_user_to_access_group_conf_t   *const conf
);

/**
 * \brief Delete the entry of SNMP UserToAccessGroup table
 *
 * To delete the specific entry in SNMP UserToAccessGroup table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP UserToAccessGroup entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_to_access_group_del(
    const vtss_appl_snmp_user_to_access_group_index_t  conf_index
);

/**
 * \brief Add new entry of SNMP UserToAccessGroup table
 *
 * To add new entry in SNMP UserToAccessGroup table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP UserToAccessGroup entry.
 * \param conf          [IN]    The new entry of SNMP UserToAccessGroup table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_user_to_access_group_add(
    const vtss_appl_snmp_user_to_access_group_index_t  conf_index,
    const vtss_appl_snmp_user_to_access_group_conf_t   *const conf
);

/**
 * \brief Get default value of SNMP UserToAccessGroup table
 *
 * To add new entry in SNMP user table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP UserToAccessGroup entry.
 * \param conf          [OUT]    The new entry of SNMP UserToAccessGroup table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */

mesa_rc vtss_appl_snmp_user_to_access_group_default(
    vtss_appl_snmp_user_to_access_group_index_t  *const conf_index,
    vtss_appl_snmp_user_to_access_group_conf_t   *const conf
);

/**
 * \brief Iterate function of SNMP view table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_view_itr(
    const vtss_appl_snmp_view_index_t    *const prev_index,
    vtss_appl_snmp_view_index_t          *const next_index
);

/**
 * \brief Get the entry of SNMP view table
 *
 * To get the specific entry in SNMP view table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP view entry.
 * \param conf          [OUT]   The current entry of the SNMP view entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_view_get(
    const vtss_appl_snmp_view_index_t  conf_index,
    vtss_appl_snmp_view_conf_t         *const conf
);

/**
 * \brief Set the entry of SNMP view table
 *
 * To modify the specific entry in SNMP view table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP view entry.
 * \param conf          [IN]    The revised the SNMP view entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_view_set(
    const vtss_appl_snmp_view_index_t  conf_index,
    const vtss_appl_snmp_view_conf_t   *const conf
);

/**
 * \brief Delete the entry of SNMP view table
 *
 * To delete the specific entry in SNMP view table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP view entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_view_del(
    const vtss_appl_snmp_view_index_t  conf_index
);

/**
 * \brief Add new entry of SNMP view table
 *
 * To add new entry in SNMP view table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP view entry.
 * \param conf          [IN]    The new entry of SNMP view table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_view_add(
    const vtss_appl_snmp_view_index_t  conf_index,
    const vtss_appl_snmp_view_conf_t   *const conf
);

/**
 * \brief Get default value of SNMP view table
 *
 * To add new entry in SNMP view table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP view entry.
 * \param conf          [OUT]    The new entry of SNMP view table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_view_default(
    vtss_appl_snmp_view_index_t  *const conf_index,
    vtss_appl_snmp_view_conf_t   *const conf
);

/**
 * \brief Iterate function of SNMP access group table
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_access_group_itr(
    const vtss_appl_snmp_access_group_index_t    *const prev_index,
    vtss_appl_snmp_access_group_index_t          *const next_index
);

/**
 * \brief Get the entry of SNMP access group table
 *
 * To get the specific entry in SNMP access group table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP access group entry.
 * \param conf          [OUT]   The current entry of the SNMP access group entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_access_group_get(
    const vtss_appl_snmp_access_group_index_t  conf_index,
    vtss_appl_snmp_access_group_conf_t         *const conf
);

/**
 * \brief Set the entry of SNMP access group table
 *
 * To modify the specific entry in SNMP access group table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP access group entry.
 * \param conf          [IN]    The revised the SNMP access group entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_access_group_set(
    const vtss_appl_snmp_access_group_index_t  conf_index,
    const vtss_appl_snmp_access_group_conf_t   *const conf
);

/**
 * \brief Delete the entry of SNMP access group table
 *
 * To delete the specific entry in SNMP access group table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP access group entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_access_group_del(
    const vtss_appl_snmp_access_group_index_t  conf_index
);

/**
 * \brief Add new entry of SNMP access group table
 *
 * To add new entry in SNMP access group table.
 *
 * \param conf_index    [IN]    (key) Name of the SNMP access group entry.
 * \param conf          [IN]    The new entry of SNMP access group table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_access_group_add(
    const vtss_appl_snmp_access_group_index_t  conf_index,
    const vtss_appl_snmp_access_group_conf_t   *const conf
);

/**
 * \brief Get default value of SNMP access group table
 *
 * To add new entry in SNMP user table.
 *
 * \param conf_index    [OUT]    (key) Name of the SNMP access group entry.
 * \param conf          [OUT]    The new entry of SNMP access group table.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_snmp_access_group_default(
    vtss_appl_snmp_access_group_index_t  *const conf_index,
    vtss_appl_snmp_access_group_conf_t   *const conf
);

/**
 * \brief Iterate function of Trap receiver configuration
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_trap_receiver_itr(
    const vtss_appl_trap_receiver_index_t    *const prev_index,
    vtss_appl_trap_receiver_index_t          *const next_index
);

/**
 * \brief Get the entry of Trap receiver configuration table
 *
 * To get configuration of the specific entry in Trap configuration table.
 *
 * \param conf_index    [IN]    (key) Name of the trap entry.
 * \param conf          [OUT]   The current configuration of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_receiver_get(
    const vtss_appl_trap_receiver_index_t    conf_index,
    vtss_appl_trap_receiver_conf_t           *const conf
);

/**
 * \brief Set the entry of Trap receiver configuration table
 *
 * To modify configuration of the specific entry in Trap configuration table.
 *
 * \param conf_index    [IN]    (key) Name of the trap entry.
 * \param conf          [IN]    The revised configuration of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_receiver_set(
    const vtss_appl_trap_receiver_index_t    conf_index,
    const vtss_appl_trap_receiver_conf_t     *const conf
);

/**
 * \brief Delete the entry of Trap receiver configuration table
 *
 * To delete configuration of the specific entry in Trap configuration table.
 *
 * \param conf_index    [IN]    (key) Name of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_receiver_del(
    const vtss_appl_trap_receiver_index_t    conf_index
);

/**
 * \brief Add the entry of Trap receiver configuration table
 *
 * To add configuration of the specific entry in Trap configuration table.
 *
 * \param conf_index    [IN]    (key) Name of the trap entry.
 * \param conf          [IN]    The new configuration of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_receiver_add(
    const vtss_appl_trap_receiver_index_t    conf_index,
    const vtss_appl_trap_receiver_conf_t     *const conf
);

/**
 * \brief Get default value of Trap receiver configuration table
 *
 * To get default values for the new entry in Trap configuration table.
 *
 * \param conf_index    [OUT]    (key) Name of the trap entry.
 * \param conf          [OUT]    The new trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_trap_receiver_default(
    vtss_appl_trap_receiver_index_t  *const conf_index,
    vtss_appl_trap_receiver_conf_t   *const conf
);

/**
 * \brief Iterate function of Trap source configuration
 *
 * To get first and get next indexes.
 *
 * \param prev_index [IN]  previous index.
 * \param next_index [OUT] next index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_trap_source_itr(
    const vtss_appl_trap_source_index_t    *const prev_index,
    vtss_appl_trap_source_index_t          *const next_index
);

/**
 * \brief Get the entry of Trap source configuration table
 *
 * To get configuration of the specific entry in Trap configuration table.
 *
 * \param conf_index    [IN]    (key) Name of the trap entry.
 * \param conf          [OUT]   The current configuration of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_source_get(
    const vtss_appl_trap_source_index_t    conf_index,
    vtss_appl_trap_source_conf_t           *const conf
);

/**
 * \brief Set the entry of Trap source configuration table
 *
 * To modify configuration of the specific entry in Trap configuration table.
 *
 * \param conf_index    [IN]    (key) Name of the trap entry.
 * \param conf          [IN]    The revised configuration of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_source_set(
    const vtss_appl_trap_source_index_t    conf_index,
    const vtss_appl_trap_source_conf_t     *const conf
);

/**
 * \brief Delete the entry of Trap source configuration table
 *
 * To delete configuration of the specific entry in Trap configuration table.
 *
 * \param conf_index    [IN]    (key) Name of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_source_del(
    const vtss_appl_trap_source_index_t    conf_index
);

/**
 * \brief Add the entry of Trap source configuration table
 *
 * To add configuration of the specific entry in Trap configuration table.
 *
 * \param conf_index    [IN]    (key) Name of the trap entry.
 * \param conf          [IN]    The new configuration of the trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc
vtss_appl_trap_source_add(
    const vtss_appl_trap_source_index_t    conf_index,
    const vtss_appl_trap_source_conf_t     *const conf
);

/**
 * \brief Get default value of Trap source configuration table
 *
 * To get default values for the new entry in Trap configuration table.
 *
 * \param conf_index    [OUT]    (key) Name of the trap entry.
 * \param conf          [OUT]    The new trap entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_trap_source_default(
    vtss_appl_trap_source_index_t  *const conf_index,
    vtss_appl_trap_source_conf_t   *const conf
);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_SNMP_H_ */
