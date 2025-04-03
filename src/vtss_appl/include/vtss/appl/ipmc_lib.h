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

/**
* \file
* \brief Public IPMC Snooping API
* \details This header file describes IPMC snooping control functions and types.
*          The types are common to both IPMC and MVR.
*/

#ifndef _VTSS_APPL_IPMC_LIB_H_
#define _VTSS_APPL_IPMC_LIB_H_

#include <vtss/appl/interface.h>
#include <vtss/appl/types.h>
#include <vtss/basics/enum_macros.hxx> /* For VTSS_ENUM_BITWISE() */

/**
 * Definition of error return codes.
 * See also ipmc_lib_error_txt() in ipmc_lib.cxx.
 */
enum {
    VTSS_APPL_IPMC_LIB_RC_INVALID_PARAMETER = MODULE_ERROR_START(VTSS_MODULE_ID_IPMC_LIB), /**< Invalid parameter                                                        */
    VTSS_APPL_IPMC_LIB_RC_INTERNAL_ERROR,                                                  /**< Internal error. Check console for details                                */
    VTSS_APPL_IPMC_LIB_RC_OUT_OF_MEMORY,                                                   /**< Out of memory                                                            */
    VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_ID,                                                 /**< Invalid VLAN ID                                                          */
    VTSS_APPL_IPMC_LIB_RC_MLD_NOT_SUPPORTED,                                               /**< MLD is not supported                                                     */
    VTSS_APPL_IPMC_LIB_RC_IPMC_NOT_SUPPORTED,                                              /**< IPMC is not supported                                                    */
    VTSS_APPL_IPMC_LIB_RC_MVR_NOT_SUPPORTED,                                               /**< MVR is not supported                                                     */
    VTSS_APPL_IPMC_LIB_RC_FUNCTION_ONLY_SUPPORTED_BY_MVR,                                  /**< Function only supported by MVR                                           */
    VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN,                                                    /**< No such VLAN                                                             */
    VTSS_APPL_IPMC_LIB_RC_VLAN_ALREADY_EXISTS,                                             /**< VLAN already exists                                                      */
    VTSS_APPL_IPMC_LIB_RC_VLAN_LIMIT_REACHED,                                              /**< The maximum number of VLANs is reached                                   */
    VTSS_APPL_IPMC_LIB_RC_IFINDEX_NOT_VLAN,                                                /**< The interface index does not represent a VLAN                            */
    VTSS_APPL_IPMC_LIB_RC_IFINDEX_NOT_PORT,                                                /**< The interface index does not represent a port                            */
    VTSS_APPL_IPMC_LIB_RC_INVALID_PORT_NUMBER,                                             /**< Invalid port number                                                      */
    VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_LENGTH,                                        /**< The length of the VLAN name is invalid                                   */
    VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_CONTENTS,                                      /**< The contents of the VLAN name is invalid                                 */
    VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_CONTENTS_COLON,                                /**< The contents of the VLAN name must not contain a ':'                     */
    VTSS_APPL_IPMC_LIB_RC_INVALID_VLAN_NAME_CONTENTS_ALL,                                  /**< The contents of the VLAN name must not be 'all'                          */
    VTSS_APPL_IPMC_LIB_RC_VLAN_NAME_NOT_SPECIFIED,                                         /**< The VLAN name is not specified                                           */
    VTSS_APPL_IPMC_LIB_RC_VLAN_NAME_NOT_FOUND,                                             /**< The VLAN name was not found                                              */
    VTSS_APPL_IPMC_LIB_RC_UNREGISTERED_FLOODING_ENABLE_CANNOT_BE_CHANGED,                  /**< Unregistered flooding enable cannot be changed by MVR                    */
    VTSS_APPL_IPMC_LIB_RC_PROXY_CANNOT_BE_CHANGED,                                         /**< Proxy enabledness cannot be changed                                      */
    VTSS_APPL_IPMC_LIB_RC_LEAVE_PROXY_CANNOT_BE_CHANGED,                                   /**< Leave Proxy enabledness cannot be changed                                */
    VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_CANNOT_BE_CHANGED,                                    /**< SSM prefix address cannot be changed                                     */
    VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_LEN_CANNOT_BE_CHANGED,                                /**< SSM prefix length cannot be changed                                      */
    VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_NOT_MC,                                               /**< SSM prefix address is not a M/C address                                  */
    VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_LEN_INVALID,                                          /**< Invalid SSM prefix length                                                */
    VTSS_APPL_IPMC_LIB_RC_SSM_PREFIX_BITS_OUTSIDE_OF_MASK_SET,                             /**< SSM prefix address has bits set out of the mask                          */
    VTSS_APPL_IPMC_LIB_RC_ROUTER_CANNOT_BE_CHANGED,                                        /**< Marking a port as a router port is not possible by MVR                   */
    VTSS_APPL_IPMC_LIB_RC_MAX_GROUP_CNT_CANNOT_BE_CHANGED,                                 /**< The maximum group count cannot be changed                                */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_KEY_CANNOT_BE_CHANGED,                                   /**< The filtering profile cannot be changed                                  */
    VTSS_APPL_IPMC_LIB_RC_MAX_GROUP_CNT_INVALID,                                           /**< Invalid maximum group count                                              */
    VTSS_APPL_IPMC_LIB_RC_QUERIER_ADDR_NOT_IPV4,                                           /**< The IGMP querier address is not specified as IPv4                        */
    VTSS_APPL_IPMC_LIB_RC_QUERIER_ADDR_NOT_UNICAST,                                        /**< The IGMP querier address is not a U/C address                            */
    VTSS_APPL_IPMC_LIB_RC_ADMIN_ACTIVE_CANNOT_BE_CHANGED,                                  /**< MVR VLANs cannot be set inactive                                         */
    VTSS_APPL_IPMC_LIB_RC_VLAN_MODE_CANNOT_BE_CHANGED,                                     /**< IPMC cannot change the VLAN mode (dynamic/compatible)                    */
    VTSS_APPL_IPMC_LIB_RC_TX_TAGGED_CANNOT_BE_CHANGED,                                     /**< The Tx Tagged property cannot be changed                                 */
    VTSS_APPL_IPMC_LIB_RC_VLAN_NAME_NOT_SUPPORTED,                                         /**< Naming of VLANs not supported (MVR feature only)                         */
    VTSS_APPL_IPMC_LIB_RC_COMPATIBILITY_CANNOT_BE_CHANGED,                                 /**< The compatibility cannot be changed                                      */
    VTSS_APPL_IPMC_LIB_RC_PCP_CANNOT_BE_CHANGED,                                           /**< PCP value cannot be changed                                              */
    VTSS_APPL_IPMC_LIB_RC_RV_CANNOT_BE_CHANGED,                                            /**< RV cannot be changed                                                     */
    VTSS_APPL_IPMC_LIB_RC_QI_CANNOT_BE_CHANGED,                                            /**< QI cannot be changed                                                     */
    VTSS_APPL_IPMC_LIB_RC_QRI_CANNOT_BE_CHANGED,                                           /**< QRI cannot be changed                                                    */
    VTSS_APPL_IPMC_LIB_RC_LMQI_CANNOT_BE_CHANGED,                                          /**< LMQI cannot be changed                                                   */
    VTSS_APPL_IPMC_LIB_RC_URI_CANNOT_BE_CHANGED,                                           /**< URI cannot be changed                                                    */
    VTSS_APPL_IPMC_LIB_RC_COMPATIBILITY,                                                   /**< Compatibility is set to an invalid value                                 */
    VTSS_APPL_IPMC_LIB_RC_COMPATIBILITY_OLD_WITH_MLD,                                      /**< Compatibility cannot be set to "old" for MLD                             */
    VTSS_APPL_IPMC_LIB_RC_INVALID_PCP,                                                     /**< Invalid PCP (priority) value                                             */
    VTSS_APPL_IPMC_LIB_RC_INVALID_ROBUSTNESS_VARIABLE,                                     /**< RV must be a value in range [2; 255]                                     */
    VTSS_APPL_IPMC_LIB_RC_INVALID_QUERY_INTERVAL,                                          /**< QI must be a value in range [1; 31744]                                   */
    VTSS_APPL_IPMC_LIB_RC_INVALID_QUERY_RESPONSE_INTERVAL,                                 /**< QRI must be a value in range [0; 31744]                                  */
    VTSS_APPL_IPMC_LIB_RC_QRI_QI_INVALID,                                                  /**< QRI <-> QI interdependency failed                                        */
    VTSS_APPL_IPMC_LIB_RC_INVALID_LMQI,                                                    /**< LMQI must be a value in range [0; 31744]                                 */
    VTSS_APPL_IPMC_LIB_RC_INVALID_URI,                                                     /**< URI must be a value in range [1; 32] seconds                             */
    VTSS_APPL_IPMC_LIB_RC_CHANNEL_PROFILES_NOT_SUPPORTED,                                  /**< Per-VLAN channel profiles are not supported by IPMC                      */
    VTSS_APPL_IPMC_LIB_RC_INVALID_PORT_ROLE,                                               /**< Invalid port role (MVR)                                                  */
    VTSS_APPL_IPMC_LIB_RC_ROLE_SOURCE_OTHER_RECEIVER,                                      /**< Cannot set port as source, as another instance configures it as receiver */
    VTSS_APPL_IPMC_LIB_RC_ROLE_RECEIVER_OTHER_SOURCE,                                      /**< Cannot set port as receiver, as another instance configures it as source */
    VTSS_APPL_IPMC_LIB_RC_NO_SUCH_GRP_ADDR,                                                /**< <VLAN ID, Group Address> not found                                       */
    VTSS_APPL_IPMC_LIB_RC_PORT_NO_ACTIVE_ON_GRP,                                           /**< Port not active on <VLAN ID, Group Address>                              */
    VTSS_APPL_IPMC_LIB_RC_NO_SUCH_SRC_ADDR,                                                /**< <VLAN ID, Group Address, Source> not found                               */
    VTSS_APPL_IPMC_LIB_RC_KEY_IS_IPV4_GRP_IS_IPV6,                                         /**< Key is IPv4, Group address is IPv6                                       */
    VTSS_APPL_IPMC_LIB_RC_KEY_IS_IPV6_GRP_IS_IPV4,                                         /**< Key is IPv6, Group address is IPv4                                       */
    VTSS_APPL_IPMC_LIB_RC_GRP_IS_IPV4_SRC_IS_IPV6,                                         /**< Group address is IPv4, Source address is IPv6                            */
    VTSS_APPL_IPMC_LIB_RC_GRP_IS_IPV6_SRC_IS_IPV4,                                         /**< Group address is IPv6, Source address is IPv4                            */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_EMPTY,                                              /**< Profile name cannot be empty                                             */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_TOO_LONG,                                           /**< Profile name too long                                                    */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_CONTENTS,                                           /**< Profile name contents is invalid                                         */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_NAME_CONTENTS_ALL,                                       /**< Profile name must not be 'all' (case-insensitively)                      */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_DSCR_TOO_LONG,                                           /**< Profile description too long                                             */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_DSCR_CONTENTS,                                           /**< Profile description contents is invalid                                  */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_NO_SUCH,                                                 /**< No such profile name                                                     */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_USED_BY_ANOTHER_MVR_VLAN,                                /**< Channel profile is used by another MVR VLAN                              */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_LIMIT_REACHED,                                           /**< Maximum number of profiles reached                                       */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_EMPTY,                                        /**< Range name cannot be empty                                               */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_TOO_LONG,                                     /**< Range name too long                                                      */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_CONTENTS,                                     /**< Range name contents is invalid                                           */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NAME_CONTENTS_ALL,                                 /**< Range name must not be 'all' (case-insensitively)                        */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_NO_SUCH,                                           /**< No such range                                                            */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_LIMIT_REACHED,                                     /**< Maximum number of ranges reached                                         */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_INVALID_TYPE,                                      /**< Invalid range type (IPv4/IPv6)                                           */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_AND_END_NOT_SAME_IP_VERSION,                 /**< Range's start IP version differs from range's end                        */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_NOT_IPV4_MC,                                 /**< Range's start IPv4 address is not a M/C address                          */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_END_NOT_IPV4_MC,                                   /**< Range's end   IPv4 address is not a M/C address                          */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_NOT_IPV6_MC,                                 /**< Range's start IPv6 address is not a M/C address                          */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_END_NOT_IPV6_MC,                                   /**< Range's end   IPv6 address is not a M/C address                          */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_GREATER_THAN_END_IPV4,                       /**< Range's IPv4 start address greater than end address                      */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_START_GREATER_THAN_END_IPV6,                       /**< Range's IPv6 start address greater than end address                      */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_RULE_NO_SUCH,                                      /**< No such rule using the specified range in the profile                    */
    VTSS_APPL_IPMC_LIB_RC_PROFILE_RANGE_INSERT_BEFORE_NO_SUCH,                             /**< The 'insert-after-this-range' key not found                              */
};

/**
 * IPMC Profile capabilities
 */
typedef struct {
    /**
     * Maximum number of IPMC profiles to create
     */
    uint32_t profile_cnt_max;

    /**
     * Maximum number of IPMC profile ranges
     */
    uint32_t range_cnt_max;
} vtss_appl_ipmc_lib_profile_capabilities_t;

/**
 * Get IPMC profile capabilities.
 *
 * \param cap [OUT] Capabilities
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_capabilities_get(vtss_appl_ipmc_lib_profile_capabilities_t *cap);

/**
 * Global Profile configuration.
 */
typedef struct {
    /**
     * Global administrative mode.
     * Set to true to enable IPMC profiles, false to disable them.
     * Default is false.
     */
    mesa_bool_t enable;
} vtss_appl_ipmc_lib_profile_global_conf_t;

/**
 * Get global default configuration.
 *
 * \param conf [OUT] Global default configuration.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_global_conf_default_get(vtss_appl_ipmc_lib_profile_global_conf_t *conf);

/**
 * Get global IPMC profile configuration.
 *
 * \param conf [OUT] Global IPMC profile configuration.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_global_conf_get(vtss_appl_ipmc_lib_profile_global_conf_t *conf);

/**
 * Set global IPMC profile configuration.
 *
 * \param conf [IN] Global IPMC profile configuration.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_global_conf_set(const vtss_appl_ipmc_lib_profile_global_conf_t *conf);

/**
 * Maximum length of profile name, excluding terminating NULL.
 */
#define VTSS_APPL_IPMC_LIB_PROFILE_NAME_LEN_MAX 16

/**
 * Maximum length of a profile's description, excluding terminating NULL.
 */
#define VTSS_APPL_IPMC_LIB_PROFILE_DSCR_LEN_MAX 64

/**
 * IPMC profiles are indexed by name held in this type.
 */
typedef struct {
    /**
     * Name of profile.
     *
     * This has certain restrictions:
     *   1) strlen(name) must be [1; VTSS_APPL_IPMC_LIB_PROFILE_NAME_LEN_MAX], so it
     *      must be NULL-terminated.
     *   2) name[0]..name[strlen(name) - 1] must be in range [33; 126]
     *      (isgraph()).
     *   3) name must not be 'all' (case-insensitive), since this is a reserved
     *      keyword for CLI when deleting all profiles.
     */
    char name[VTSS_APPL_IPMC_LIB_PROFILE_NAME_LEN_MAX + 1];
} vtss_appl_ipmc_lib_profile_key_t;

/**
 * vtss_appl_ipmc_lib_profile_key_t::operator==()
 * Makes it possible to do a "if (profile_key1 == profile_key2)"
 *
 * \param lhs [IN] Left-hand-side of comparison
 * \param rhs [IN] Right-hand-side of comparison
 *
 * \return true if lhs is the same as rhs, false otherwise.
 */
bool operator==(const vtss_appl_ipmc_lib_profile_key_t &lhs, const vtss_appl_ipmc_lib_profile_key_t &rhs);

/**
 * Configuration associated with a profile.
 */
typedef struct {
    /**
     * Description of this profile.
     */
    char dscr[VTSS_APPL_IPMC_LIB_PROFILE_DSCR_LEN_MAX + 1];
} vtss_appl_ipmc_lib_profile_conf_t;

/**
 * Get IPMC profile configuration.
 *
 * \param key  [IN]  Name of the profile (key)
 * \param conf [OUT] The current configuration of this profile
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_conf_get(const vtss_appl_ipmc_lib_profile_key_t *key, vtss_appl_ipmc_lib_profile_conf_t *conf);

/**
 * Create or change an IPMC profile configuration.
 *
 * \param key  [IN] Name of the profile (key)
 * \param conf [IN] Revised or new configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_conf_set(const vtss_appl_ipmc_lib_profile_key_t *key, const vtss_appl_ipmc_lib_profile_conf_t *conf);

/**
 * Delete IPMC profile.
 *
 * \param key [IN] Name of the profile (key)
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_conf_del(const vtss_appl_ipmc_lib_profile_key_t *key);

/**
 * Iterator for retrieving IPMC profiles
 *
 * vtss_clear(key) and use that as both \p prev and \p next to iterate across
 * all profiles.
 *
 * \param prev [IN]  Key to find next profile for.
 * \param next [OUT] Next key.

 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_itr(const vtss_appl_ipmc_lib_profile_key_t *prev, vtss_appl_ipmc_lib_profile_key_t *next);

/**
 * IPMC ranges are indexed by name held in this type.
 */
typedef struct {
    /**
     * Name of range.
     *
     * This has certain restrictions:
     *   1) strlen(name) must be [1; VTSS_APPL_IPMC_LIB_PROFILE_NAME_LEN_MAX], so it
     *      must be NULL-terminated.
     *   2) name[0]..name[strlen(name) - 1] must be in range [33; 126]
     *      (isgraph()).
     *   3) name must not be 'all' (case-insensitive), since this is a reserved
     *      keyword for CLI when deleting all profiles.
     */
    char name[VTSS_APPL_IPMC_LIB_PROFILE_NAME_LEN_MAX + 1];
} vtss_appl_ipmc_lib_profile_range_key_t;

/**
 * vtss_appl_ipmc_lib_profile_range_key_t::operator==()
 * Makes it possible to do a "if (range_key1 == range_key2)"
 *
 * \param lhs [IN] Left-hand-side of comparison
 * \param rhs [IN] Right-hand-side of comparison
 *
 * \return true if lhs is the same as rhs, false otherwise.
 */
bool operator==(const vtss_appl_ipmc_lib_profile_range_key_t &lhs, const vtss_appl_ipmc_lib_profile_range_key_t &rhs);

/**
 * A range consists of either multicast IPv4 or multicast IPv6 addresses.
 */
typedef enum {
    VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_ANY,  /**< Any range type. Only valid in calls to vtss_appl_ipmc_lib_profile_range_itr() */
    VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_IPV4, /**< Range is IPv4                                                             */
    VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_IPV6, /**< Range is IPv6. Only supported on systems with IPv6 support                */
} vtss_appl_ipmc_lib_profile_range_type_t;

/**
 * Structure capable of having both IPv4 and IPv6 addresses. A boolean indicates
 * whether it's one or the other.
 * The union is anonymous, so its members can be used without referencing the
 * union.
 */
struct vtss_appl_ipmc_lib_ip_t {
    /**
     * This is true if this structure represents an IPv4 address, false if it
     * represents an IPv6 address.
     */
    mesa_bool_t is_ipv4;

    /**
     * ipv4 is valid if is_ipv4 is true.
     * ipv6 is valid if is_ipv4 is false.
     */
    union {
        /**
         * The IPv4 address
         */
        mesa_ipv4_t ipv4;

        /**
         * The IPv6 address.
         */
        mesa_ipv6_t ipv6;
    };

    /**
     * Method for printing this IP address into \p buf.
     *
     * \param buf [IN] Buffer of at least 40 bytes.
     *
     * \return Function returns \p buf, which allows this method to be used
     *         directly in e.g. printf() statements.
     */
    char *print(char *buf) const;

    /**
     * Method returning true if the IP address represented by this structure is
     * 0.0.0.0 (for IPv4) or :: (for IPv6).
     *
     * \return Boolean.
     */
    mesa_bool_t is_zero(void) const;

    /**
     * Method returning true if the IP address represented by this structure is
     * a unicast address.
     *
     * \return Boolean.
     */
    mesa_bool_t is_uc(void) const;

    /**
     * Method returning true if the IP address represented by this structure is
     * a multicast address.
     *
     * \return Boolean.
     */
    mesa_bool_t is_mc(void) const;

    /**
     * Method for comparing two IP addresses.
     * If this->is_ipv4 != rhs.is_ipv4, a trace error is thrown.
     *
     * \param rhs [IN] Right-hand-side of expression
     *
     * \return Boolean.
     */
    bool operator==(const vtss_appl_ipmc_lib_ip_t &rhs) const;

    /**
     * Method for comparing two IP addresses.
     * If this->is_ipv4 != rhs.is_ipv4, a trace error is thrown.
     *
     * \param rhs [IN] Right-hand-side of expression
     *
     * \return Boolean.
     */
    bool operator!=(const vtss_appl_ipmc_lib_ip_t &rhs) const;

    /**
     * Method for comparing two IP addresses.
     * If this->is_ipv4 != rhs.is_ipv4, a trace error is thrown.
     *
     * \param rhs [IN] Right-hand-side of expression
     *
     * \return Boolean.
     */
    bool operator<(const vtss_appl_ipmc_lib_ip_t &rhs) const;

    /**
     * Method for bit-wise two IP addresses (typically used with prefix
     * matching.
     * If this->is_ipv4 != rhs.is_ipv4, a trace error is thrown.
     *
     * \param rhs [IN] Right-hand-side of expression
     *
     * \return A new IP address.
     */
    vtss_appl_ipmc_lib_ip_t operator&(const vtss_appl_ipmc_lib_ip_t &rhs) const;

    /**
     * Method for bit-wise negation of an IP address.
     *
     * \return A new IP address.
     */
    vtss_appl_ipmc_lib_ip_t operator~() const;

    /**
     * Add one to the IP address.
     * If IP address is all-ones, the new will be all-zeros.
     * This is a prefix operator, so use it as "++ip;".
     *
     * \return This IP address
     */
    vtss_appl_ipmc_lib_ip_t &operator++();

    /**
     * Subtract one from the IP address.
     * If IP address is 0, nothing is subtracted.
     * This is a prefix operator, so use it as "--ip;".
     *
     * \return This IP address
     */
    vtss_appl_ipmc_lib_ip_t &operator--();

    /**
     * Set the IP address to all-zeros.
     * If is_ipv4 == true,  the IP address becomes 0.0.0.0.
     * If is_ipv4 == false, the IP address becomes ::
     */
    void all_zeros_set();

    /**
     * Set the IP address to all-ones.
     * If is_ipv4 == true,  the IP address becomes 255.255.255.255.
     * If is_ipv4 == false, the IP address becomes ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff.
     * Useful to initailize some iterators.
     */
    void all_ones_set();

    /**
     * Returns true if is_ipv4 and ipv4 == 255.255.255.255 or if not is_ipv4 and
     * ipv6 == ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff.
     * Used by implementation of some iterators.
     *
     * \return true if the IP address is all ones.
     */
    bool is_all_ones() const;
};

/**
 * Specialized function to output contents of a vtss_appl_ipmc_lib_ip_t to a
 * stream
 *
 * \param o  [OUT] Stream
 * \param ip [IN]  IP address to print
 *
 * \return The stream itself
 */
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_ipmc_lib_ip_t &ip);

/**
 * Specialized fmt() function usable in trace commands.
 *
 * Suppose you want to trace an IP address. You *could* do it like this:
 *   T_I("IP address = %s", misc_ipv6_txt(ip.ipv6, buf));
 *
 * The presence of the following function ensures that you can do it like this:
 *   T_I("IP address = %s", ip);
 *
 * You do not need to know about the parameters, so they won't be documented
 * very well.
 *
 * \param o   [OUT] Stream
 * \param fmt [IN]  Format
 * \param ip  [IN]  IP address to print
 *
 * \return Number of bytes written.
 */
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_ip_t *ip);

/**
 * IPMC IP address range configuration.
 * Ranges are created independently of profiles and are assigned to a profile
 * once created. If a range is deleted, it automatically gets deleted from the
 * profile it is used in.
 */
typedef struct {
    /**
     * The first IPv4 or IPv6 address in the range.
     * Both start and end must be of the same type.
     */
    vtss_appl_ipmc_lib_ip_t start;

    /**
     * The last IPv4 or IPv6 address in the range.
     * Both start and end must be of the same type.
     */
    vtss_appl_ipmc_lib_ip_t end;
} vtss_appl_ipmc_lib_profile_range_conf_t;

/**
 * Get IPMC profile range configuration.
 *
 * \param key  [IN]  Name of the range (key)
 * \param conf [OUT] The current configuration of this range
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_range_conf_get(const vtss_appl_ipmc_lib_profile_range_key_t *key, vtss_appl_ipmc_lib_profile_range_conf_t *conf);

/**
 * Create or change an IPMC profile range configuration.
 *
 * \param key  [IN] Name of the range (key)
 * \param conf [IN] Revised or new configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_range_conf_set(const vtss_appl_ipmc_lib_profile_range_key_t *key, const vtss_appl_ipmc_lib_profile_range_conf_t *conf);

/**
 * Delete IPMC profile range.
 *
 * The range will also be removed from all profiles using it.
 *
 * \param key [IN] Name of the range (key)
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_range_conf_del(const vtss_appl_ipmc_lib_profile_range_key_t *key);

/**
 * Iterator for retrieving IPMC profile ranges.
 *
 * vtss_clear(key) and use that as both \p prev and \p next to iterate across
 * all profiles.
 * You may use the last argument to iterate across either IPv4 or IPv6 ranges.
 * Default is to iterate across both IPv4 and IPv6 ranges.
 *
 * \param prev [IN]  Key to find next range for.
 * \param next [OUT] Next key.
 * \param type [IN]  Range type.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_range_itr(const vtss_appl_ipmc_lib_profile_range_key_t *prev, vtss_appl_ipmc_lib_profile_range_key_t *next, vtss_appl_ipmc_lib_profile_range_type_t type = VTSS_APPL_IPMC_LIB_PROFILE_RANGE_TYPE_ANY);

/**
 * IPMC profile rule.
 * A given profile rule belongs to one and only one profile.
 * They are inserted into the profile in an ordered manner. The earlier in the
 * rule list they are inserted, the higher precedence.
 */
typedef struct {
    /**
     * If true, frames that hit this rule's range are denied. Otherwise they are
     * permitted.
     */
    mesa_bool_t deny;

    /**
     * When this rule is hit and the frame is denied, a syslog entry will be
     * created if this variable is true. Otherwise it will be silently ignored.
     */
    mesa_bool_t log;
} vtss_appl_ipmc_lib_profile_rule_conf_t;

/**
 * Get default IPMC profile rule configuration
 *
 * \param rule_conf [OUT] Default rule configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_rule_conf_default_get(vtss_appl_ipmc_lib_profile_rule_conf_t *rule_conf);

/**
 * Get IPMC Profile specific rule configuration.
 *
 * \param profile_key [IN]  Name of the profile (key)
 * \param range_key   [IN]  Name of the range (key)
 * \param rule_conf   [OUT] The current configuration of the rule in a specific profile.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_rule_conf_get(const vtss_appl_ipmc_lib_profile_key_t *profile_key, const vtss_appl_ipmc_lib_profile_range_key_t *range_key, vtss_appl_ipmc_lib_profile_rule_conf_t *rule_conf);

/**
 * Set IPMC Profile specific rule configuration.
 *
 * \param profile_key             [IN]  Name of the profile to add/change rule for (key)
 * \param range_key               [IN]  Name of the range to add/change rule for (key)
 * \param rule_conf               [OUT] The rule configuration.
 * \param insert_before_range_key [IN]  If nullptr the rule is appended or moved to the end of the list.
 *                                      If strlen(insert_before_range_key->name) == 0, the rule stays where it are. In this case, \p range_key must be part of this rule already.
 *                                      Otherwise, the rule is inserted just before another rule's range with this name. Error if this other range doesn't exist.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_rule_conf_set(const vtss_appl_ipmc_lib_profile_key_t *profile_key, const vtss_appl_ipmc_lib_profile_range_key_t *range_key, const vtss_appl_ipmc_lib_profile_rule_conf_t *rule_conf, const vtss_appl_ipmc_lib_profile_range_key_t *insert_before_range_key);

/**
 * Delete IPMC Profile specific rule configuration.
 *
 * \param profile_key [IN] Name of the profile (key)
 * \param range_key   [IN] Name of the range (key)
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_rule_conf_del(const vtss_appl_ipmc_lib_profile_key_t *profile_key, const vtss_appl_ipmc_lib_profile_range_key_t *range_key);

/**
 * Iterator for retrieving IPMC Profile rules
 *
 * vtss_clear(profile_key) and vtss_clear(range_key) and use them as both prev
 * and next to iterate across all profiles and rules.
 *
 * \param profile_prev         [IN]  Previous profile name
 * \param profile_next         [OUT] Next profile name
 * \param range_prev           [IN]  Prev range.
 * \param range_next           [OUT] Next range.
 * \param stay_in_this_profile [IN]  If true, the iterator stops when the next profile is reached.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_profile_rule_itr(const vtss_appl_ipmc_lib_profile_key_t       *profile_prev, vtss_appl_ipmc_lib_profile_key_t       *profile_next,
                                            const vtss_appl_ipmc_lib_profile_range_key_t *range_prev,   vtss_appl_ipmc_lib_profile_range_key_t *range_next, bool stay_in_this_profile = false);

/**
 * IPMC library capabilities
 * Common to both IPMC and MVR.
 */
typedef struct {
    /**
     * This is true if IGMP Snooping is supported by this implementation.
     */
    mesa_bool_t igmp_support;

    /**
     * This is true if MLD Snooping is supported by this implementation.
     */
    mesa_bool_t mld_support;

    /**
     * This is true if Source Specific Multicast Forwarding for IPv4 is
     * supported by the chip.
     */
    mesa_bool_t ssm_chip_support_ipv4;

    /**
     * This is true if Source Specific Multicast Forwarding for IPv6 is
     * supported by the chip.
     */
    mesa_bool_t ssm_chip_support_ipv6;

    /**
     * Maximum number of IPv4 M/C groups that can be created.
     *
     * Both IPMC and MVR eat of these resources, as do IGMP and MLD for both
     * of them.
     *
     * Notice that this is the best case maximum number, since the H/W resource
     * is shared amongst other features.
     *
     * Also notice, that IPv6 group addresses typically take twice the amount
     * of resources in the chip as does IPv4 group addresses.
     *
     * One M/C group is per VLAN ID per port.
     */
    uint32_t grp_cnt_max;

    /**
     * Maximum number of IPv4 source addresses that can be created.
     *
     * Both IPMC and MVR eat of these resources, as do IGMP and MLD for both
     * of them.
     *
     * Notice that this is the best case maximum number, since the H/W resource
     * is shared amongst other features.
     *
     * Also notice, that IPv6 source addresses typically take twice the amount
     * of resources in the chip as does IPv4 source addresses.
     *
     * One M/C group is per VLAN ID per port.
     */
    uint32_t src_cnt_max;

    /**
     * Maximum number of source addresses per M/C group.
     * If limit is reached, the remaining are silently discarded.
     */
    uint32_t src_per_grp_cnt_max;
} vtss_appl_ipmc_lib_capabilities_t;

/**
 * Get IPMC library capabilities.
 *
 * \param cap [OUT] Capabilities
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_capabilities_get(vtss_appl_ipmc_lib_capabilities_t *cap);

/**
 * IPMC capabilities. These are for either IPMC or MVR.
 * See vtss_appl_ipmc_lib_capabilities_t for capabilities common to both MVR and
 * IPMC.
 */
typedef struct {
    /**
     * Maximum number of VLANs that MVR or IPMC supports.
     *
     * Even if both IGMP and MLD are compile-time enabled, it counts as one
     * VLAN.
     */
    uint32_t vlan_cnt_max;
} vtss_appl_ipmc_capabilities_t;

/**
 * Get IPMC or MVR capabilities.
 *
 * \param is_mvr [IN]  If true, get MVR capabilities. Otherwise IPMC.
 * \param cap    [OUT] Capabilities
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_capabilities_get(bool is_mvr, vtss_appl_ipmc_capabilities_t *cap);

/**
 * Since IPMC LIB functionality covers both IPMC and MVR, we will use a key in
 * the following functions that indicate whether it is one of the other that is
 * getting accessed.
 * Also, IPMC LIB handles both IGMP and MLD, so another value is needed to
 * indicate whether one or the other is getting accessed.
 */
struct vtss_appl_ipmc_lib_key_t {
    /**
     * Set to true to access MVR-specific information. False to access IPMC.
     */
    bool is_mvr;

    /**
     * Set to true to access IGMP-specific information. False to access MLD.
     */
    bool is_ipv4;
};

/**
 * Specialized fmt() function usable in trace commands.
 *
 * Suppose you want to trace a key. You *could* do it like this:
 *   T_I("is_mvr = %d, is_ipv4 = %d", key.is_mvr, key.is_ipv4);
 *
 * The presence of the following function ensures that you can do it like this:
 *   T_I("key = %s", key);
 *
 * You do not need to know about the parameters, so they won't be documented
 * very well.
 *
 * \param o   [OUT] Stream
 * \param fmt [IN]  Format
 * \param key [IN]  Key to print
 *
 * \return Number of bytes written.
 */
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_key_t *key);

/**
 * The following key is used to access VLAN-specific information.
 * Besides <is_mvr, is_ipv4> (which is inherited), it also contains a VLAN ID.
 */
struct vtss_appl_ipmc_lib_vlan_key_t : public vtss_appl_ipmc_lib_key_t {
    /**
     * VLAN ID.
     */
    mesa_vid_t vid;
};

/**
 * Specialized fmt() function usable in trace commands.
 *
 * Suppose you want to trace a key. You *could* do it like this:
 *   T_I("is_mvr = %d, is_ipv4 = %d, vid = %u", key.is_mvr, key.is_ipv4, key.vid);
 *
 * The presence of the following function ensures that you can do it like this:
 *   T_I("key = %s", key);
 *
 * You do not need to know about the parameters, so they won't be documented
 * very well.
 *
 * \param o        [OUT] Stream
 * \param fmt      [IN]  Format
 * \param vlan_key [IN]  Key to print
 *
 * \return Number of bytes written.
 */
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_vlan_key_t *vlan_key);

/**
 * Configuration used to globally control either IPMC or MVR for both IGMP and
 * MLD snooping.
 */
typedef struct {
    /**
     * Global administrative mode. Set to true to enable IGMP/MLD snooping
     * globally, false to disable it.
     *
     * Used by both IPMC and MVR.
     */
    mesa_bool_t admin_active;

    /**
     * Set to true to enable flooding of IPv4/IPv6 multicast traffic to ports
     * with unregistered group addresses. Set to false to avoid flooding of such
     * traffic to ports with unregistered group addresses.
     *
     * Only changeable by IPMC, not MVR.
     */
    mesa_bool_t unregistered_flooding_enable;

    /**
     * Set to true to enable IGMP/MLD proxy mode, false to disable it.
     *
     * Can only be changed in SMBStaX and up.
     *
     * Only changeable by IPMC, not MVR.
     */
    mesa_bool_t proxy_enable;

    /**
     * Set to true to enable IGMP/MLD proxy for leave mode, false to disable it.
     *
     * Can only be changed in SMBStaX and up.
     *
     * Only changeable by IPMC, not MVR.
     */
    mesa_bool_t leave_proxy_enable;

    /**
     * Along with \p ssm_prefix_len, this field indicates a range of M/C
     * addresses used in the SSM service module in group registration.
     * Reception of IGMPv1/IGMPv2/MLDv1 reports with group addresses in this
     * range are ignored.
     *
     * Can only be changed in SMBStaX and up.
     *
     * IPMC: Changeable in SMBStaX and up.
     * MVR:  Not changeable and not used.
     */
    vtss_appl_ipmc_lib_ip_t ssm_prefix;

    /**
     * See \p ssm_prefix for details.
     *
     * Can only be changed in SMBStaX and up.
     *
     * Only changeable by IPMC, not MVR.
     */
    uint32_t ssm_prefix_len;
} vtss_appl_ipmc_lib_global_conf_t;

/**
 * Get a global IPMC or MVR default configuration for IGMP or MLD.
 *
 * \param key         [IN]  Key indicating which global default configuration to get.
 * \param global_conf [OUT] Global default configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_global_conf_default_get(const vtss_appl_ipmc_lib_key_t &key, vtss_appl_ipmc_lib_global_conf_t *global_conf);

/**
 * Get global IPMC or MVR configuration for IGMP or MLD.
 *
 * \param key         [IN]  Key indicating which global configuration to get.
 * \param global_conf [OUT] Current global configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_global_conf_get(const vtss_appl_ipmc_lib_key_t &key, vtss_appl_ipmc_lib_global_conf_t *global_conf);

/**
 * Set global IPMC or MVR configuration for IGMP Or MLD.
 *
 * \param key         [IN] Key indicating which global configuration to set.
 * \param global_conf [IN] Global configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_global_conf_set(const vtss_appl_ipmc_lib_key_t &key, const vtss_appl_ipmc_lib_global_conf_t *global_conf);

/**
 * Structure for holding port related configuation.
 * Some fields are used by MVR and some fields are used by IPMC, as shown in
 * their descriptions.
 */
typedef struct {
    /**
      * Ports marked statically as a router port receive all M/C data and
      * IGMP/MLD reports.
      *
      * Default is false.
      * The field is only changeable with IPMC, not MVR.
      */
    mesa_bool_t router;

    /**
      * Ports marked with fast_leave leave immediately without the switch
      * sending a query first.
      *
      * When a leave message is received on ports marked without fast_leave, the
      * switch sends a (source-and-) group-specific query on that port to ask if
      * there are other hosts that want this group address before actually
      * leaving.
      *
      * Changeable by both IPMC and MVR.
      */
    mesa_bool_t fast_leave;

    /**
      * The maximum number of groups that can be registered on a port can be
      * restricted to a certain number.
      *
      * If this number is reached while processing a report, the remaining
      * groups of the report will not be registered, and the report will not be
      * forwarded to other router ports (RBNTBD).
      *
      * This feature is by default disabled (by setting the value to 0), but
      * can be enabled by setting it to a value between 1 and 10.
      *
      * Changeable by IPMC, not MVR.
      */
    uint32_t grp_cnt_max;

    /**
      * Filtering profile for port.
      *
      * Can only be changed in SMBStaX and up.
      *
      * Changeable by IPMC, not MVR.
      */
    vtss_appl_ipmc_lib_profile_key_t profile_key;
} vtss_appl_ipmc_lib_port_conf_t;

/**
 * Get a default configuration for port (IGMP or MLD)
 *
 * \param key     [IN]  Key indicating which port default configuration to get.
 * \param conf    [OUT] Port default configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_port_conf_default_get(const vtss_appl_ipmc_lib_key_t &key, vtss_appl_ipmc_lib_port_conf_t *conf);

/**
 * Get IPMC config for single port
 *
 * Used by both IGMP and MLD
 *
 * \param key     [IN]  Key indicating which port configuration to get.
 * \param port_no [IN]  Port number requested
 * \param conf    [OUT] The current config for this port
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_port_conf_get(const vtss_appl_ipmc_lib_key_t &key, mesa_port_no_t port_no, vtss_appl_ipmc_lib_port_conf_t *conf);

/**
 * Set IPMC config for single port
 *
 * Set configuration for single port.
 * Used by both IGMP and MLD
 *
 * \param key     [IN] Key indicating which port configuration to set.
 * \param port_no [IN] Port number requested
 * \param conf    [IN] The new config for this port
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_port_conf_set(const vtss_appl_ipmc_lib_key_t &key, mesa_port_no_t port_no, const vtss_appl_ipmc_lib_port_conf_t *conf);

/**
 * Iterator for retrieving IPMC port information key/index
 *
 * To walk information (configuration and status) index of the port in IPMC snooping.
 *
 * \param prev [IN]  Interface index to be used for indexing determination.
 * \param next [OUT] The key/index should be used for the GET operation.
 *                   When IN is NULL, assign the first index.
 *                   When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_port_itr(const vtss_ifindex_t *prev, vtss_ifindex_t *next);

/**
 * IPMC version compatiblity type
 */
typedef enum {
    VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO, /**< IPMC compatibility is automatically determined      */
    VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD,  /**< IPMC only reacts on and transmits IGMPv1       PDUs */
    VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN,  /**< IPMC only reacts on and transmits IGMPv2/MLDv1 PDUs */
    VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM   /**< IPMC only reacts on and transmits IGMPv3/MLDv2 PDUs */
} vtss_appl_ipmc_lib_compatibility_t;

/**
 * Maximum length of friendly VLAN name, excluding terminating NULL.
 * Used by MVR only.
 */
#define VTSS_APPL_IPMC_LIB_VLAN_NAME_LEN_MAX 16

/**
 * Per-VLAN configuration, covering both IPMC and MVR - and for each of those
 * both IGMP and MLD.
 *
 * Some fields may not be changed by MVR, and they will always hold defaults.
 * These are marked as "MVR: Cannot be changed".
 *
 * Likewise, some fields may not be changed by IPMC, and they will always hold
 * defaults. These are mared as "IPMC: Cannot be changed":
 *
 * Some fields may not be changed in IGMP/MLD in some software flavors.
 * These are marked as "IGMP & MLD: Can only be changed in SMBStaX and up".
 */
typedef struct {
    /**
     * Set to true, to enable snooping on this VLAN, false to disable it.
     *
     * MVR: Cannot be changed.
     */
    mesa_bool_t admin_active;

    /**
     * VLANs may have an associated name for easy identification of the VLAN.
     *
     * It has certain restrictions, though:
     *   1) strlen(name) must be [0; VTSS_APPL_IPMC_LIB_VLAN_NAME_LEN_MAX], so
     *      it must be NULL-terminated.
     *   2) name[0] must be in range [a-zA-Z] (isalpha()).
     *   3) name[1]..name[strlen(name) - 1] must be in range [33; 126] except
     *      for 58 (isgraph(), but ':' is reserved).
     *   4) name must not be 'all' (case-insensitive), since this is a reserved
     *      keyword for CLI when deleting all VLANs.
     *
     * IPMC: Cannot be changed.
     */
    char name[VTSS_APPL_IPMC_LIB_VLAN_NAME_LEN_MAX + 1];

    /**
     * This option determines what happens if receiving a report on a port in
     * listener mode (see vtss_appl_ipmc_lib_vlan_port_conf_t::role).
     *
     * If false (a.k.a. dynamic mode), IGMP/MLD reports are allowed and
     * processed when received on a port marked as a source port.
     *
     * In true, (a.k.a. compatible mode), IGMP/MLD reports are discarded when
     * received on a port marked as a source port.
     *
     * Default is false.
     *
     * IPMC: Cannot be changed.
     * MVR: Changeable.
     */
    bool compatible_mode;

    /**
     * If querier is enabled, the IGMP/MLD VLAN instance joins querier election,
     * and will send querier frames if it becomes the querier (has the lowest IP
     * address of all queriers on the VLAN interface).
     *
     * Supported by both IPMC and MVR on both IGMP and MLD.
     */
    mesa_bool_t querier_enable;

    /**
     * If this is not 0.0.0.0, all queries sent will use this as Source IP
     * address. This must be a valid unicast IPv4 address, but not from
     * 127.x.x.x (loopback).
     *
     * This is only used with IGMP, not MLD.
     *
     * If 0.0.0.0, the following will happen in this order:
     *  1) the IPv4 management address of the IP interface associated with this
     *     VLAN will be used. If no such IP address, then
     *  2) the first available IPv4 management address in VLAN order will be
     *     used. If no such IP address, then
     *  3)  192.0.2.1 will be used.
     *
     * Default is 0.0.0.0.
     *
     * Supported on both MVR and IPMC on IGMP.
     */
    vtss_appl_ipmc_lib_ip_t querier_address;

    /**
     * Use this to force the VLAN instance to use a particular version, or let
     * it auto-update by setting it to VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO,
     * which is also the default.
     *
     * IPMC: Can only be changed in SMBStaX and up.
     * MVR: Cannot be changed.
     */
    vtss_appl_ipmc_lib_compatibility_t compatibility;

    /**
     * If true, IPMC control frames sent in this VLAN are transmitted tagged,
     * untagged otherwise.
     *
     * Default is true for MVR, false for IGMP/MLD.
     *
     * Only used by MVR.
     * IPMC: Cannot be changed.
     */
    mesa_bool_t tx_tagged;

    /**
     * PCP value used in possible VLAN tags of frames transmitted by MVR.
     *
     * Must be a value in range [0; 7]. Default is 0.
     *
     * IPMC: Can only be changed in SMBStaX and up.
     */
    mesa_pcp_t pcp;

    /**
     * The Querier Robustness Variable allows tuning for the expected packet
     * loss on a link. If a link is expected to be lossy, the value of \p rv
     * may be increased. IGMP/MLD is robust to (\p rv - 1) packet losses.
     *
     * Allowed range is [2; 255]. Default is 2.
     *
     * IPMC: Can only be changed in SMBStaX and up.
     * MVR: Cannot be changed from its default.
     */
    uint32_t rv;

    /**
     * The Query Interval denotes the interval between General Queries sent by
     * the Querier - measured in seconds.
     *
     * Allowed range is [1; 31744] seconds. Default is 125 seconds.
     *
     * IPMC: Can only be changed in SMBStaX and up.
     * MVR: Cannot be changed from its default.
     */
    uint32_t qi;

    /**
     * Query Response Interval is the maximum response delay inserted into the
     * periodic General Queries.
     *
     * By varying the query response interval, an administrator may tune the
     * burstiness of IGMP/MLD messages on the link. Larger values make the
     * traffic less bursty, as node responses are spread out over a larger
     * interval.
     *
     * \p qri is measured in 10ths of a second.
     * Allowed range is [0; 31744], and the default is 100 corresponding to 10
     * seconds.
     *
     * IPMC: Can only be changed in SMBStaX and up.
     * MVR: Cannot be changed from its default.
     */
    uint32_t qri;

    /**
     * The Last Member Query Interval is the Max Response Time used to calculate
     * the Max Response Code inserted into Group-Specific Queries sent in
     * response to Leave Groupm messages. It is also the Max Response Time used
     * in calculating the Max Respon Code for Group-And-Source specific Query
     * messages.
     *
     * It is measured in 10ths of a second with a default value of 10 (1
     * second).
     *
     * Must be in range [0; 31744].
     *
     * IPMC: Can only be changed in SMBStaX and up.
     * MVR: Supported.
     */
    uint32_t lmqi;

    /**
     * Unsolicited Report Interval (URI) is the time between repetitions of a
     * node's initial report of interest in a multicast address - measured in
     * seconds.
     *
     * Default is 1 second.
     * RBNTBD: THIS IS NOT USED IN THE CODE!!!!
     *
     * Must be in range [1; 31744] seconds
     *
     * IPMC: Can only be changed in SMBStaX and up.
     * MVR: Cannot be changed from its default.
     */
    uint32_t uri;

    /**
     * MVR has a profile per VLAN interface.
     *
     * IPMC: Not used.
     * MVR: Channel profile.
     */
    vtss_appl_ipmc_lib_profile_key_t channel_profile;
} vtss_appl_ipmc_lib_vlan_conf_t;

/**
 * Specialized fmt() function usable in trace commands.
 *
 * You do not need to know about the parameters, so they won't be documented
 * very well.
 *
 * \param o    [OUT] Stream
 * \param fmt  [IN]  Format
 * \param conf [IN]  Key to print
 *
 * \return Number of bytes written.
 */
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_ipmc_lib_vlan_conf_t *conf);

/**
 * Get a default configuration for VLAN (MVR/IPMC and IGMP/MLD)
 *
 * \param key       [IN]  Key indicating which VLAN default configuration to get.
 * \param vlan_conf [OUT] VLAN default configuration
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_conf_default_get(const vtss_appl_ipmc_lib_key_t &key, vtss_appl_ipmc_lib_vlan_conf_t *vlan_conf);

/**
 * Get VLAN configuration for MVR/IPMC and IGMP/MLD
 *
 * Get configuration for single VLAN id
 *
 * \param vlan_key [IN]  Key indicating which VLAN configuration to get.
 * \param conf     [OUT] The current configuration for VLAN
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_conf_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, vtss_appl_ipmc_lib_vlan_conf_t *conf);

/**
 * Set configuration for VLAN for IPMC usage
 *
 * Write/update configuration for single VLAN id
 *
 * \param vlan_key [IN]  Key indicating which VLAN configuration to set.
 * \param conf     [IN]  The current configuration for VLAN
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_conf_set(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, const vtss_appl_ipmc_lib_vlan_conf_t *conf);

/**
 * Iterator for retrieving VLANs defined for IPMC/MCR and IGMP/MLD usage.
 *
 * Get next defined VLAN greater than VLAN ID set in *prev.
 *
 * \param prev             [IN]  Key to find next VLAN for.
 * \param next             [OUT] Next key.
 * \param stay_in_this_key [IN]  If true, prev must be nun-NULL and next->is_ipv4 and next->is_mvr remain the same as prev's while returning VTSS_RC_OK.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_itr(const vtss_appl_ipmc_lib_vlan_key_t *prev, vtss_appl_ipmc_lib_vlan_key_t *next, bool stay_in_this_key = false);

/**
 * Get VLAN ID from a VLAN name (used by MVR, only).
 *
 * \param name [IN]  VLAN name to get.
 * \param vid  [OUT] The corresponding VLAN ID.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_name_to_vid(const char *name, mesa_vid_t *vid);

/**
 * Port operational role.
 * Used by MVR, only.
 */
typedef enum {
    VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE,     /**< Port is neither a source nor a receiver */
    VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE,   /**< Port acts as a source port              */
    VTSS_APPL_IPMC_LIB_PORT_ROLE_RECEIVER, /**< Port acts as a receiver port            */
} vtss_appl_ipmc_lib_port_role_t;

/**
 * Per port per VLAN configuration.
 *
 * This configuration is only used by MVR, not IPMC.
 */
typedef struct {
    /**
     * Tells whether the port is a source port (port on the switch that receives
     * M/C data traffic) or a receiver port (port on the switch on which
     * receivers of the M/C data reside) - on none of the above.
     *
     * Default: VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE.
     */
    vtss_appl_ipmc_lib_port_role_t role;
} vtss_appl_ipmc_lib_vlan_port_conf_t;

/**
 * Get a default per-port-per-VLAN configuration.
 *
 * Not used by IPMC.
 *
 * \param key  [IN]  Key indicating which per port per VLAN default configuration to get.
 * \param conf [OUT] Default per port per VLAN configuration.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_port_conf_default_get(const vtss_appl_ipmc_lib_key_t &key, vtss_appl_ipmc_lib_vlan_port_conf_t *conf);

/**
 * Get port configuration for a particular MVR VLAN.
 *
 * Not used by IPMC.
 *
 * \param vlan_key [IN]  Key indicating the MVR VLAN to get configuration for.
 * \param port_no  [IN]  Port
 * \param conf     [OUT] The current configuration of the port in the MVR VLAN.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_port_conf_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, mesa_port_no_t port_no, vtss_appl_ipmc_lib_vlan_port_conf_t *conf);

/**
 * Set port configuration for a particular MVR VLAN.
 *
 * Not used by IPMC.
 *
 * \param vlan_key [IN] Key indicating the MVR VLAN to set configuration for.
 * \param port_no  [IN] Port
 * \param conf     [IN] The new configuration of the port in the MVR VLAN.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_port_conf_set(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, mesa_port_no_t port_no, const vtss_appl_ipmc_lib_vlan_port_conf_t *conf);

/**
 * Iterator for per-port per VLAN.
 *
 * To iterate over all defined VLANs and ports do the following:
 *   vtss_clear(vlan_key);
 *   vlan_key.is_ipv4 = true;
 *   while (vtss_appl_ipmc_lib_vlan_port_itr(&vlan_key, &vlan_key, &ifindex, &ifindex) == VTSS_RC_OK) {
 *       // Do something
 *   }
 *
 * \param vlan_key_prev    [IN]  Previous VLAN key
 * \param vlan_key_next    [OUT] Next VLAN key
 * \param port_prev        [IN]  Previous port
 * \param port_next        [OUT] Next port
 * \param stay_in_this_key [IN] If true, the function returns VTSS_RC_ERROR if going from IPv4 (IGMP) to IPv6 (MLD) or from IPMC to MVR.
 *
 * \return VTSS_RC_OK as long as operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_port_itr(const vtss_appl_ipmc_lib_vlan_key_t *vlan_key_prev, vtss_appl_ipmc_lib_vlan_key_t *vlan_key_next, const mesa_port_no_t *port_prev, mesa_port_no_t *port_next, bool stay_in_this_key = false);

/**
 * Router status type.
 */
typedef enum {
    VTSS_APPL_IPMC_LIB_ROUTER_STATUS_NONE,    /* No IGMP router is determined                      */
    VTSS_APPL_IPMC_LIB_ROUTER_STATUS_STATIC,  /* Static IGMP router is configured                  */
    VTSS_APPL_IPMC_LIB_ROUTER_STATUS_DYNAMIC, /* Dynamic IGMP router is detected                   */
    VTSS_APPL_IPMC_LIB_ROUTER_STATUS_BOTH,    /* Both static and dynamic IGMP router is determined */
} vtss_appl_ipmc_lib_router_status_t;

/**
 * Structure for holding port related status.
 * Used by both IGMP and MLD.
 */
typedef struct {
    /**
      * The router status of the port indicates whether it is detected and/or
      * configured as a router port.
      */
    vtss_appl_ipmc_lib_router_status_t router_status;

    /**
     * If \p router_status is VTSS_APPL_IPMC_LIB_ROUTER_STATUS_DYNAMIC or
     * VTSS_APPL_IPMC_LIB_ROUTER_STATUS_BOTH, the following shows the time - in
     * seconds - until the dynamic router status times out.
     */
    uint32_t dynamic_router_timeout;
} vtss_appl_ipmc_lib_port_status_t;

/**
 * Get IPMC/MVR IGMP/MLD status for single port
 *
 * \param key     [IN]  Key indicating which port status to get.
 * \param port_no [IN]  Port number requested
 * \param status  [OUT] The current status for this port
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_port_status_get(const vtss_appl_ipmc_lib_key_t &key, mesa_port_no_t port_no, vtss_appl_ipmc_lib_port_status_t *status);

/**
 * IPMC Querier states
 */
typedef enum {
    VTSS_APPL_IPMC_LIB_QUERIER_STATE_DISABLED, /**< Interface Querier is in disabled state       */
    VTSS_APPL_IPMC_LIB_QUERIER_STATE_INIT,     /**< Interface Querier is in initialization state */
    VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE,     /**< Interface Querier is in inactive state       */
    VTSS_APPL_IPMC_LIB_QUERIER_STATE_ACTIVE    /**< Interface Querier is in active state         */
} vtss_appl_ipmc_lib_querier_state_t;

/**
 * This enum holds flags that indicate various (configuration) warnings.
 */
typedef enum {
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_NONE                                              = 0x000000, /**< No warnings found                                                                             */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_INTERNAL_ERROR                                    = 0x000001, /**< Internal error. Check console/crashlog for details                                            */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_IPMC_AND_MVR_BOTH_ACTIVE_IPMC                     = 0x000002, /**< Both MVR and IPMC are active on the same VLAN. For IPMC                                       */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_GLOBALLY_DISABLED_MVR                     = 0x000004, /**< IPMC profiles are globally disabled. For MVR                                                  */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_GLOBALLY_DISABLED_IPMC                    = 0x000008, /**< IPMC profiles are globally disabled. For IPMC                                                 */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_NOT_SET_MVR                               = 0x000010, /**< The MVR VLAN instance doesn't reference an IPMC Profile                                       */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_DOESNT_EXIST_MVR                          = 0x000020, /**< The referenced IPMC profile doesn't exist. For MVR                                            */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_DOESNT_EXIST_IPMC                         = 0x000040, /**< The referenced IPMC profile doesn't exist. For IPMC                                           */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_RANGES_MVR                    = 0x000080, /**< The referenced IPMC profile doesn't have any IPv4 ranges attached. For MVR                    */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_RANGES_IPMC                   = 0x000100, /**< The referenced IPMC profile doesn't have any IPv4 ranges attached. For IPMC                   */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_RANGES_MVR                    = 0x000200, /**< The referenced IPMC profile doesn't have any IPv6 ranges attached. For MVR                    */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_RANGES_IPMC                   = 0x000400, /**< The referenced IPMC profile doesn't have any IPv6 ranges attached. For IPMC                   */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_PERMIT_RULES_MVR              = 0x000800, /**< The referenced IPMC profile has no IPv4 permit rules. For MVR                                 */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV4_PERMIT_RULES_IPMC             = 0x001000, /**< The referenced IPMC profile has no IPv4 permit rules. For IPMC                                */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_PERMIT_RULES_MVR              = 0x002000, /**< The referenced IPMC profile has no IPv6 permit rules. For MVR                                 */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_HAS_NO_IPV6_PERMIT_RULES_IPMC             = 0x004000, /**< The referenced IPMC profile has no IPv6 permit rules. For IPMC                                */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV4_MVR   = 0x008000, /**< An IPv4 deny rule shadows a permit rule coming later in the profile's rule list. For MVR      */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV4_IPMC  = 0x010000, /**< An IPv4 deny rule shadows a permit rule coming later in the profile's rule list. For IPMC     */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV6_MVR   = 0x020000, /**< An IPv6 deny rule shadows a permit rule coming later in the profile's rule list. For MVR      */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV6_IPMC  = 0x040000, /**< An IPv6 deny rule shadows a permit rule coming later in the profile's rule list. For IPMC     */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_OTHER_OVERLAPS_IPV4_MVR                   = 0x080000, /**< Another MVR VLAN instance uses a profile with at least one IPv4 rule that overlaps this one's */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_PROFILE_OTHER_OVERLAPS_IPV6_MVR                   = 0x100000, /**< Another MVR VLAN instance uses a profile with at least one IPv6 rule that overlaps this one's */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_NO_SOURCE_PORTS_CONFIGURED                        = 0x200000, /**< No source ports are configured (MVR only)                                                     */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_NO_RECEIVER_PORTS_CONFIGURED                      = 0x400000, /**< No receiver ports are configured (MVR only)                                                   */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_AT_LEAST_ONE_SOURCE_PORT_MEMBER_OF_VLAN_INTERFACE = 0x800000, /**< At least one source port is member of a VLAN interface with same MVR VLAN ID. For MVR         */
} vtss_appl_ipmc_lib_vlan_oper_warnings_t;

/**
 * Operators for vtss_appl_ipmc_lib_vlan_oper_warnings_t flags.
 */
VTSS_ENUM_BITWISE(vtss_appl_ipmc_lib_vlan_oper_warnings_t);

/**
 * The operational state of the VLAN. This can only be set inactive for MVR
 * VLANs.
 * MVR VLANs must have an IPMC profile attached. IPMC profiles must be globally
 * enabled (stupid that it can be set to disabled, but what don't we do in order
 * to be backwards compatible?!?), and it must have at least one permit rule.
 */
typedef enum {
    VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ADMIN_DISABLED, /**< Instance is inactive, because it is administratively disabled (either globally or locally). */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_ACTIVE,         /**< The instance is active and up and running.                                                  */
    VTSS_APPL_IPMC_LIB_VLAN_OPER_STATE_INACTIVE,       /**< Instance is inactive, because of an operational warning.                                    */
} vtss_appl_ipmc_lib_vlan_oper_state_t;

/**
 * IPMC VLAN status
 * The status is the per-VLAN basic runtime values. Covers
 * both functionality on IGMP and MLD
 */
typedef struct {
    /**
     * The operational state of this VLAN.
     */
    vtss_appl_ipmc_lib_vlan_oper_state_t oper_state;

    /**
     * Operational warnings of this VLAN interface/instance.
     */
    vtss_appl_ipmc_lib_vlan_oper_warnings_t oper_warnings;

    /**
     * Interface Querier state is used to denote the current IGMP/MLD
     * interface's Querier state based on Querier election defined in protocol.
     */
    vtss_appl_ipmc_lib_querier_state_t querier_state;

    /**
     * The currently active Querier's IP address.
     */
    vtss_appl_ipmc_lib_ip_t active_querier_address;

    /**
     * The time - measured in seconds - that we as an active querier - have been
     * up.
     */
    uint32_t querier_uptime;

    /**
     * Time left (in seconds) until the next query is transmitted, when
     * configured as a querier and the active querier.
     * See also RFC3376 8.2 (IGMP) and RFC3810 9.2 (MLD).
     */
    uint32_t query_interval_left;

    /**
     * other_querier_expiry_time presents the "Other Querier Present Interval",
     * as stated in RFC3376 8.5 (IGMP) and RFC3810 9.5 (MLD).
     */
    uint32_t other_querier_expiry_time;

    /**
     * This will basically follow vtss_appl_ipmc_lib_vlan_conf_t::compatibility,
     * but may change only if that configuration is set to
     * VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO, so the following description is
     * based on such a configuration.
     *
     * If a query is received, that query's version is compared to the current
     * querier compatibility.
     *
     * If the received query version is smaller than the current querier
     * compatibility, the current querier compatibility is lowered to that of
     * the received query and a timer is started (IGMP has two timers - one for
     * IGMPv1 and one for IGMPv2 - and MLD has one for MLDv1; see two next
     * fields in this structure).
     *
     * If the received query version is equal to the current querier
     * compatibility and it is not IGMPv3 or MLDv2, the timer is reset.
     *
     * The querier compatibility determines the versions of the reports sent by
     * the switch towards router ports, but only if the VLAN is configured for
     * proxy mode. In non-proxy mode, received reports are simply forwarded to
     * router ports, independent of current querier compatibility and forwarded
     * report versions.
     */
    vtss_appl_ipmc_lib_compatibility_t querier_compat;

    /**
     * When vtss_appl_ipmc_lib_vlan_conf_t::compatibility is
     * VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO, the following may be non-zero if
     * an IGMPv1 query is received on the VLAN.
     * It represents the time - in seconds - until the switch returns to IGMPv2
     * or IGMPv3 mode.
     *
     * It is used used with IGMP, and not with MLD.
     */
    uint32_t older_version_querier_present_timeout_old;

    /**
     * When vtss_appl_ipmc_lib_vlan_conf_t::compatibility is
     * VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO, the following may be non-zero if
     * an IGMPv2/MLDv1 query is received on the VLAN.
     * It represents the time - in seconds - until the switch returns to IGMPv3
     * or MLDv2 mode if not in IGMPv1 mode.
     */
    uint32_t older_version_querier_present_timeout_gen;

    /**
     * This will basically follow vtss_appl_ipmc_lib_vlan_conf_t::compatibility,
     * but may change only if that configuration is set to
     * VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO, so the following description is
     * based on such a configuration.
     *
     * If a report is received for a given group, that report's version is
     * compared to the group's current host compatibility.
     *
     * If the received report version is smaller than the group's current host
     * compatibility, the group compatibility is lowered to that of the received
     * report and a timer is started (IGMP has two timers - one for IGMPv1 and
     * one for IGMPv2 - and MLD has one for MLDv1; see two next fields in this
     * structure).
     *
     * If the received report version is equal to the current group
     * compatibility and it is not IGMPv3 or MLDv2, the timer is reset.
     *
     * The group compatibility determines the versions of the queries sent by
     * the switch towards host ports for that group.
     *
     * The \p host_compat field below is the lowest of all group's group
     * compatibilities on the VLAN.
     */
    vtss_appl_ipmc_lib_compatibility_t host_compat;

    /**
     * When vtss_appl_ipmc_lib_vlan_conf_t::compatibility is
     * VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO, the following may be non-zero if
     * an IGMPv1 report is received on the VLAN.
     * It represents the time - in seconds - until the switch returns to IGMPv2
     * or IGMPv3 mode.
     *
     * As described in \p host_compat, \p host_compat is the lowest version of
     * all group's group compatibilites of the VLAN. Likewise, the
     * \p older_version_host_present_timeout_old is the largest of all group's
     * timeouts.
     *
     * It is used only with IGMP, but not with MLD.
     */
    uint32_t older_version_host_present_timeout_old;

    /**
     * When vtss_appl_ipmc_lib_vlan_conf_t::compatibility is
     * VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO, the following may be non-zero if
     * an IGMPv2/MLDv1 report is received on the VLAN.
     * It represents the time - in seconds - until the switch returns to IGMPv3
     * (if not in IGMPv1 mode, in which case \p
     * older_version_host_present_timeout_old governs) or to MLDv2 mode.
     *
     * As described in \p host_compat, \p host_compat is the lowest version of
     * all group's group compatibilites of the VLAN. Likewise, the
     * \p older_version_host_present_timeout_gen is the largest of all group's
     * timeouts.
     */
    uint32_t older_version_host_present_timeout_gen;

    /**
     * Number of registered groups on this VLAN.
     */
    uint32_t grp_cnt;
} vtss_appl_ipmc_lib_vlan_status_t;

/**
 * Get status for VLAN for IPMC usage
 *
 * Get status/runtime values for single VLAN id
 *
 * \param vlan_key [IN]  Key indicating which VLAN status to get.
 * \param status   [OUT] The current status for VLAN
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_status_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, vtss_appl_ipmc_lib_vlan_status_t *status);

/**
 * The chip may or may not support exact matching on a M/C group's IP address
 * and possibly also a source address. It supports exact IPv4 matching if
 * vtss_appl_ipmc_lib_capabilities_t::ssm_chip_support_ipv4 is true and exact
 * IPv6 matching if vtss_appl_ipmc_lib_capabilities_t::ssm_chip_support_ipv6 is
 * true.
 *
 * Exact matching utilizes a so-called TCAM (Ternary Content-
 * Addressable Memory) embedded in the chip, so the enumeration below includes
 * the word "TCAM" if the entry is installed in H/W using the TCAM.
 *
 * If the chip does not support exact matching or if all entries in the TCAM are
 * depleted, the code resorts to using the chip's MAC Address table for
 * forwarding particular MAC addresses to the ports.
 * Since one M/C MAC address comprises several M/C IP addresses, this may result
 * in more multicast data towards the listeners than what they ask for.
 *
 * This enumeration shows whether or not a particular M/C group and source
 * address is known by the chip and if so, whether it's an exact or inexact
 * match
 */
typedef enum {
    VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE,      /**< The <Group, Source> entry is not installed in the chip.                 */
    VTSS_APPL_IPMC_LIB_HW_LOCATION_TCAM,      /**< The <Group, Source> entry is installed in the chip's TCAM.              */
    VTSS_APPL_IPMC_LIB_HW_LOCATION_MAC_TABLE, /**< The <Group, Source> entry is installed in the chip's MAC address table. */
} vtss_appl_ipmc_lib_hw_location_t;

/**
 * Structure for holding per-multicast group status.
 * Used by both IGMP and MLD.
 * Notice that this is only 100% correct if there are no source addresses
 * attached to the group.
 */
typedef struct {
    /**
     * List of ports that are forwarding for this M/C group.
     * It does not include router ports, and it may not indicate the 100%
     * correct status for the following two reasons:
     * If a port is in EXCLUDE filter mode, and it has one or more source
     * addresses that are not forwarding, the \p port_list will indicate that
     * the port is forwarding (despite it's not forwarding ALL source
     * addresses).
     * Likewise, if a port is in INCLUDE filter mode, and it has one or more
     * source addresses that are forwarding, the \p port_list will indicate that
     * the port is NOT forwarding.
     */
    mesa_port_list_t port_list;

    /**
     * This indicates if and where this entry is located in the chip.
     */
    vtss_appl_ipmc_lib_hw_location_t hw_location;
} vtss_appl_ipmc_lib_grp_status_t;

/**
 * Get IGMP or MLD group status for a given <VLAN ID, M/C group address>.
 *
 * Use vtss_appl_ipmc_lib_grp_itr() to iterate through the known M/C groups.
 *
 * \param vlan_key [IN] The VLAN key to get port memberships for.
 * \param grp      [IN] The M/C IP address to get port memberships for.
 * \param status   [OUT] The current status of the registered multicast group address.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_grp_status_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, const vtss_appl_ipmc_lib_ip_t *grp, vtss_appl_ipmc_lib_grp_status_t *status);

/**
 * Iterate through all <VLAN ID, M/C groups>.
 *
 * To iterate over everything, do the following:
 *   vtss_clear(vlan_key);
 *   vtss_clear(grp);
 *   vlan_key.is_ipv4 = true;
 *   while (vtss_appl_ipmc_lib_grp_itr(&vlan_key, &vlan_key, &grp, &grp) == VTSS_RC_OK) {
 *       // Do something
 *   }
 *
 * \param vlan_key_prev    [IN]  Pointer to the previous VLAN key or nullptr if starting over.
 * \param vlan_key_next    [OUT] Pointer receiving the next VLAN key.
 * \param grp_prev         [IN]  Pointer to the previous M/C group address or nullptr if starting over on this VLAN.
 * \param grp_next         [OUT] Pointer receiving the next M/C group address.
 * \param stay_in_this_key [IN]  If true, key_next.is_mvr and key_next.is_ipv4 will be the same as the previous (which must not be a nullptr), but VID may increase. If false, IPMC will become before MVR and IPv4 will come before IPv6 addresses.
 *
 * \return VTSS_RC_OK as long as there are more groups to return.
 */
mesa_rc vtss_appl_ipmc_lib_grp_itr(const vtss_appl_ipmc_lib_vlan_key_t *vlan_key_prev, vtss_appl_ipmc_lib_vlan_key_t *vlan_key_next, const vtss_appl_ipmc_lib_ip_t *grp_prev, vtss_appl_ipmc_lib_ip_t *grp_next, bool stay_in_this_key = false);

/**
 * IPMC Group Filter Mode
 */
typedef enum {
    VTSS_APPL_IPMC_LIB_FILTER_MODE_EXCLUDE, /**< Filtering Mode is Exclude */
    VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE, /**< Filtering Mode is Include */
} vtss_appl_ipmc_lib_filter_mode_t;

/**
 * Get a source registration status.
 *
 * The status concerns a specific <G, port, S>.
 */
typedef struct {
    /**
     * The filter mode of this <G, port>.
     * If EXCLUDE, <G, S> is not forwarded to this port, but other sources are.
     * If INCLUDE, <G, S> is forwarded to this port, but other source aren't.
     */
    vtss_appl_ipmc_lib_filter_mode_t filter_mode;

    /**
     * The group timer is per <G, port> and is only used when filter_mode is
     * EXCLUDE. It contains the number of seconds until all excluded sources
     * are removed and the <G, port> moves to INCLUDE mode with all sources in
     * the "Requested List" set as forwarding. If there are no sources in
     * "Requested List", the <G, port> is removed entirely.
     */
    uint32_t grp_timeout;

    /**
     * If this source is forwarding on <G, port>, \p forwarding is true,
     * otherwise, it's false.
     */
    mesa_bool_t forwarding;

    /**
     * If filter mode is INCLUDE, source timers are non-zero.
     * If filter mode is EXCLUDE, source timers are non-zero if the entry is
     * currently forwarding (not in the exclude list).
     *
     * Otherwise, source timers are 0.
     */
    uint32_t src_timeout;

    /**
     * This indicates if and where this <G, S> is located in the chip.
     */
    vtss_appl_ipmc_lib_hw_location_t hw_location;
} vtss_appl_ipmc_lib_src_status_t;

/**
 * Get IGMP or MLD source status for a given <VLAN ID, G, port, S>.
 *
 * Use vtss_appl_ipmc_lib_src_itr() to iterate through the known sources.
 *
 * \param vlan_key [IN] The VLAN key to get source status for.
 * \param grp      [IN] The M/C IP address to get source status for.
 * \param port_no  [IN] The port number to get source status for.
 * \param src      [IN] The source to get source status for. 0.0.0.0 for ASM entry.
 * \param status   [OUT] The current status of the registered <G, port, S>.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_src_status_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, const vtss_appl_ipmc_lib_ip_t *grp, mesa_port_no_t port_no, const vtss_appl_ipmc_lib_ip_t *src, vtss_appl_ipmc_lib_src_status_t *status);

/**
 * Iterate through all <VLAN ID, M/C groups, Sources>.
 *
 * Upon successful return of the function, all source addresses come first for
 * a given <vid, grp>, and after that comes a special source address set to all-
 * ones (can be tested with src.is_all_ones()), which indicates the "catch
 * remaining" (ASM) entry. If there were no actual source addresses for <vid,
 * grp>, then the "catch remaining" entry corresponds to forwarding of all
 * sources. Otherwise the "catch remaining" entry corresponds to either
 * discarding (filter_mode == INCLUDE) or forwarding (filter_mode == EXCLUDE)
 * the source addresses not already listed for this <vid, grp, port>.
 *
 * To iterate through all sources and all ports, one may start like this:
 *   vtss_clear(key);
 *   key.is_ipv4 = true; // Needed because internally IPv4 come before IPv6.
 *   vtss_clear(grp);
 *   grp.is_ipv4 = true;
 * The initialization values of port_no and src don't matter when starting
 * over on a new key.
 *
 *   while (vtss_appl_ipmc_lib_src_itr(&key,     &key,
 *                                     &grp,     &grp,
 *                                     &port_no, &port_no,
 *                                     &src,     &src)) == VTSS_RC_OK) {
 *       // Do something
 *   }
 *
 * \param vlan_key_prev    [IN]  Pointer to the previous VLAN key or nullptr if starting over.
 * \param vlan_key_next    [OUT] Pointer receiving the next VLAN key.
 * \param grp_prev         [IN]  Pointer to the previous M/C group address or nullptr if starting over on this VLAN.
 * \param grp_next         [OUT] Pointer receiving the next M/C group address.
 * \param port_prev        [IN]  Pointer to the previous port number or nullptr if starting over.
 * \param port_next        [OUT] Pointer receiving the next port number.
 * \param src_prev         [IN]  Pointer to the previous U/C source address or nullptr if starting over on this M/C group.
 * \param src_next         [OUT] Pointer receiving the next U/C source address (0.0.0.0 if ASM entry).
 * \param stay_in_this_key [IN]  If true, key_next.is_mvr and key_next.is_ipv4 will be the same as the previous (which must not be a nullptr), otherwise IPMC will become before MVR and IPv4 will come before IPv6 addresses.
 *
 * \return VTSS_RC_OK as long as there are more source addresses to return.
 */
mesa_rc vtss_appl_ipmc_lib_src_itr(const vtss_appl_ipmc_lib_vlan_key_t *vlan_key_prev, vtss_appl_ipmc_lib_vlan_key_t *vlan_key_next,
                                   const vtss_appl_ipmc_lib_ip_t       *grp_prev,      vtss_appl_ipmc_lib_ip_t       *grp_next,
                                   const mesa_port_no_t                *port_prev,     mesa_port_no_t                *port_next,
                                   const vtss_appl_ipmc_lib_ip_t       *src_prev,      vtss_appl_ipmc_lib_ip_t       *src_next, bool stay_in_this_key = false);

/**
 * Statistics counters only pertaining to IGMP.
 *
 * The structure is used for both Rx and Tx statistics.
 */
typedef struct {
    /**
     * Number of IGMPv1 joins received or transmitted on this VLAN.
     */
    uint32_t v1_report;

    /**
     * Number of IGMPv1 (general) queries received or transmitted on this VLAN.
     */
    uint32_t v1_query;

    /**
     * Number of IGMPv2 joins received or transmitted on this VLAN.
     */
    uint32_t v2_report;

    /**
     * Number of IGMPv2 leaves received or transmitted on this VLAN.
     */
    uint32_t v2_leave;

    /**
     * Number of IGMPv2 general queries received or transmitted on this VLAN.
     */
    uint32_t v2_g_query;

    /**
     * Number of IGMPv2 group-specific queries received or transmitted on this
     * VLAN.
     */
    uint32_t v2_gs_query;

    /**
     * Number of IGMPv3 reports received or transmitted on this VLAN.
     */
    uint32_t v3_report;

    /**
     * Number of IGMPv3 general queries received or transmitted on this VLAN.
     */
    uint32_t v3_g_query;

    /**
     * Number of IGMPv3 group-specific queries received or transmitted on this
     * VLAN.
     */
    uint32_t v3_gs_query;

    /**
     * Number of IGMPv3 group-and-source-specific queries received or
     * transmitted on this VLAN.
     */
    uint32_t v3_gss_query;
} vtss_appl_ipmc_lib_igmp_vlan_statistics_t;

/**
 * Statistics counters only pertaining to MLD.
 */
typedef struct {
    /**
     * Number of MLDv1 reports received or transmitted on this VLAN.
     */
    uint32_t v1_report;

    /**
     * Number of MLDv1 dones received or transmitted on this VLAN.
     */
    uint32_t v1_done;

    /**
     * Number of MLDv1 general queries received or transmitted on this VLAN.
     */
    uint32_t v1_g_query;

    /**
     * Number of MLDv1 group-specific (multicast-address-specific) queries
     * received or transmitted on this VLAN.
     */
    uint32_t v1_gs_query;

    /**
     * Number of MLDv2 reports received or transmitted on this VLAN interface.
     */
    uint32_t v2_report;

    /**
     * Number of MLDv2 general queries received or transmitted on this VLAN.
     */
    uint32_t v2_g_query;

    /**
     * Number of MLDv2 group-specific (multicast-address-specific) queries
     * received or transmitted on this VLAN.
     */
    uint32_t v2_gs_query;

    /**
     * Number of MLDv2 group-and-source-specific (multicast-address-and-source-
     * specific) queries received or transmitted on this VLAN.
     */
    uint32_t v2_gss_query;
} vtss_appl_ipmc_lib_mld_vlan_statistics_t;

/**
 * IGMP and MLD statistics.
 */
typedef struct {
    /**
     * Number of IGMP or MLD PDUs with errors received on this VLAN.
     */
    uint32_t rx_errors;

    /**
     * Number of IGMP or MLD Tx queries.
     * This is the sum of all queries from tx.igmp or tx.mld.
     */
    uint32_t tx_query;

    /**
     * Number of IGMP or MLD Tx group- or group-specific queries.
     * This is the sum of all group- and group-specific queries from
     * tx.igmp or from tx.mld.
     */
    uint32_t tx_specific_query;

    /**
     * Number of IGMP or MLD Rx queries.
     * This is the sum of all queries from rx.igmp.utilized and
     * rx.igmp.ignored or from rx.mld.utilized and rx.mld.ignored.
     */
    uint32_t rx_query;

    /**
     * This union may represent both IGMP and MLD Rx statistics depending on
     * context it is used in.
     */
    union {
        /**
         * Counters to use when this structure is used for IGMP
         */
        struct {
            /**
             * IGMP counters for PDUs received and processed.
             */
            vtss_appl_ipmc_lib_igmp_vlan_statistics_t utilized;

            /**
             * IGMP counters for PDUs received and ignored.
             */
            vtss_appl_ipmc_lib_igmp_vlan_statistics_t ignored;
        } igmp;

        /**
         * Counters to use when this structure is used for MLD
         */
        struct {
            /**
             * MLD counters for PDUs received and utilized.
             */
            vtss_appl_ipmc_lib_mld_vlan_statistics_t utilized;

            /**
             * MLD counters for PDUs received and ignored.
             */
            vtss_appl_ipmc_lib_mld_vlan_statistics_t ignored;
        } mld;
    } rx;

    /**
     * This union may represent both IGMP and MLD Tx statistics depending on
     * context it is used in.
     */
    union {
        /**
         * IGMP Tx counters.
         */
        vtss_appl_ipmc_lib_igmp_vlan_statistics_t igmp;

        /**
         * MLD Tx counters.
         */
        vtss_appl_ipmc_lib_mld_vlan_statistics_t mld;
    } tx;
} vtss_appl_ipmc_lib_vlan_statistics_t;

/**
 * Get IPMC/MVR and IGMP/MLD statistics counters.
 *
 * \param vlan_key   [IN]  Key indicating which VLAN to get statistics for.
 * \param statistics [OUT] Object to get statistics into
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_statistics_get(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key, vtss_appl_ipmc_lib_vlan_statistics_t *statistics);

/**
 * Clear MVR/IPMC's IGMP/MLD statistics.
 *
 * \param vlan_key [IN] Key indicating which VLAN to clear statistics for.
 *
 * \return VTSS_RC_OK if operation succeeds.
 */
mesa_rc vtss_appl_ipmc_lib_vlan_statistics_clear(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key);

#endif  /* _VTSS_APPL_IPMC_LIB_H_ */

