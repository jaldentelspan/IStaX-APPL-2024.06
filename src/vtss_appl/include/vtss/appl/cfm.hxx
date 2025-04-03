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
 * \brief Public Connectivity Fault Management API
 * \details This header file describes CFM control/status functions and types.
 * Unless otherwise stated, references - mostly stated in parentheses - are to
 * clauses in IEEE802.1Q-2018.
 *
 * About Shared MD/MEG Levels (MELs):
 * Serval-1 exhibits shared MEL, which means that an enabled Port MEP always
 * performs level filtering no matter which VID the flow gets classified to -
 * unless the same port has a VLAN MEP on the VID in question. So if you have a
 * Port MEP in VID X and a VLAN MEP in VID Y, an OAM frame arriving on the port
 * and gets classified to VID X or VID Z will be handled/level-filtered by the
 * Port MEP, whereas an OAM frame arriving on the port in VID Y will be handled
 * by the VLAN MEP.
 * Likewise, if the switch has a Port MEP on VID X on Port X and an OAM frame
 * arrives on VID Y on Port Y, it is subject to level filtering before egressing
 * Port X, unless Port X also has a VLAN MEP on VID Y, in which case the VLAN
 * MEP will take care of the level filtering.
 *
 * This means that on Serval-1, all Port MEPs will have to be instantiated on
 * the same MEL, and any VLAN MEP will have to have a MEL higher than any Port
 * MEP.
 *
 * All other platforms exhibit independent MEL, so that Port MEPs only will
 * level filter if the frame is classified to the port MEP's VLAN.
 */

#ifndef _VTSS_APPL_CFM_HXX_
#define _VTSS_APPL_CFM_HXX_

#include <vtss/appl/interface.h>  /* For vtss_ifindex_t        */
#include <vtss/appl/module_id.h>  /* For MODULER_ERROR_START() */
#include <string>                 /* For std::string           */

/**
 * CFM error codes (mesa_rc)
 */
enum {
    VTSS_APPL_CFM_RC_INVALID_ARGUMENT = MODULE_ERROR_START(VTSS_MODULE_ID_CFM),   /**< Invalid argument (typically a NULL pointer) passed to a function          */
    VTSS_APPL_CFM_RC_INTERNAL_ERROR,                                              /**< Internal error. Requires code update                                      */
    VTSS_APPL_CFM_RC_OUT_OF_MEMORY,                                               /**< Out of memory                                                             */
    VTSS_APPL_CFM_RC_END_OF_LIST,                                                 /**< End of iterator list reached                                              */
    VTSS_APPL_CFM_RC_INVALID_NAME_KEY_LENGTH,                                     /**< The length of the requested name (used as key) is invalid                 */
    VTSS_APPL_CFM_RC_INVALID_NAME_KEY_CONTENTS,                                   /**< The contents of the requested name (used as key) is invalid               */
    VTSS_APPL_CFM_RC_INVALID_NAME_KEY_CONTENTS_COLON,                             /**< The contents of the requested name (used as key) must not contain a ':'   */
    VTSS_APPL_CFM_RC_INVALID_NAME_KEY_CONTENTS_ALL,                               /**< The contents of the requested name (used as key) must not be 'all'        */
    VTSS_APPL_CFM_RC_NO_SUCH_INSTANCE,                                            /**< The requested MD/MA/MEP/RMEP doesn't exist                                */
    VTSS_APPL_CFM_RC_INVALID_SENDER_ID_TLV_OPTION,                                /**< The requested Sender ID TLV option is not valid at this level             */
    VTSS_APPL_CFM_RC_INVALID_PORT_STATUS_TLV_OPTION,                              /**< The requested Port Status TLV option is not valid at this level           */
    VTSS_APPL_CFM_RC_INVALID_INTERFACE_STATUS_TLV_OPTION,                         /**< The requested Interface Status TLV option is not valid at this level      */
    VTSS_APPL_CFM_RC_INVALID_ORGANIZATION_SPECIFIC_TLV_OPTION,                    /**< The requested Organization-Specific TLV option is not valid at this level */
    VTSS_APPL_CFM_RC_ORG_SPEC_TLV_VAL_TOO_LONG,                                   /**< The length of the organization-specific TLV value is too long             */
    VTSS_APPL_CFM_RC_MD_UNSUPPORTED_FORMAT,                                       /**< Uhsupported MD format                                                     */
    VTSS_APPL_CFM_RC_MD_INVALID_NAME_LENGTH,                                      /**< Invalid MD name length (when format is string)                            */
    VTSS_APPL_CFM_RC_MD_INVALID_NAME_CONTENTS,                                    /**< Invalid MD name (when format is string)                                   */
    VTSS_APPL_CFM_RC_MD_INVALID_LEVEL,                                            /**< Invalid MD level                                                          */
    VTSS_APPL_CFM_RC_MD_Y1731_FORMAT,                                             /**< Changing/Creating an MD impossible because an MA requires Y.1731          */
    VTSS_APPL_CFM_RC_MD_MAID_TOO_LONG,                                            /**< Changing/Creating an MD causes the MAID to become too long                */
    VTSS_APPL_CFM_RC_MD_LIMIT_REACHED,                                            /**< The maximum number of MDs has already been created                        */
    VTSS_APPL_CFM_RC_MD_ALL_PORT_MEPS_MUST_HAVE_SAME_LEVEL,                       /**< Changing MD level will result in port MEPs of different levels            */
    VTSS_APPL_CFM_RC_MD_PORT_MEP_MUST_HAVE_LOWER_LEVEL_THAN_ANY_VLAN_MEP,         /**< Changing MD level will result in port MEPs at >= level than VLAN MEPs     */
    VTSS_APPL_CFM_RC_MD_VLAN_MEP_MUST_HAVE_HIGHER_LEVEL_THAN_PORT_MEPS,           /**< Changing MD level will result in VLAN MEPs at <= level than Port MEPs     */
    VTSS_APPL_CFM_RC_MD_LEVEL_OF_PORT_MEP_MUST_BE_LOWER_THAN_LEVEL_OF_VLAN_MEP,   /**< Changing MD level will result in P-MEP.lvl >= V-MEP.lvl                   */
    VTSS_APPL_CFM_RC_MD_LEVEL_OF_VLAN_MEP_MUST_BE_HIGHER_THAN_LEVEL_OF_PORT_MEP,  /**< Changing MD level will result in V-MEP.lvl <= P-MEP.lvl                   */
    VTSS_APPL_CFM_RC_MA_UNSUPPORTED_FORMAT,                                       /**< Unsupported MA format                                                     */
    VTSS_APPL_CFM_RC_MA_INVALID_NAME_STRING_LENGTH,                               /**< Invalid MA name length (when format is string)                            */
    VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_LENGTH,                                  /**< Invalid MA name length (when format is ICC)                               */
    VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_LENGTH,                               /**< Invalid MA name length (when format is ICC-CC)                            */
    VTSS_APPL_CFM_RC_MA_INVALID_NAME_STRING_CONTENTS,                             /**< Invalid MA name (when format is string)                                   */
    VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CONTENTS,                                /**< Invalid MA name (when format is ICC)                                      */
    VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_CONTENTS_FIRST,                       /**< Invalid MA name (when format is ICC-CC, CC-part)                          */
    VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_CONTENTS_SLASH,                       /**< Invalid MA name (when format is ICC-CC, number of '/')                    */
    VTSS_APPL_CFM_RC_MA_INVALID_NAME_ICC_CC_CONTENTS_LAST,                        /**< Invalid MA name (when format is ICC-CC, ICC-part)                         */
    VTSS_APPL_CFM_RC_MA_INVALID_VLAN,                                             /**< Invalid MA VID                                                            */
    VTSS_APPL_CFM_RC_MA_INVALID_CCM_INTERVAL,                                     /**< Invalid MA CCM interval                                                   */
    VTSS_APPL_CFM_RC_MA_CCM_INTERVAL_NOT_SUPPORTED,                               /**< CCM interval not supported on this platform                               */
    VTSS_APPL_CFM_RC_MA_Y1731_FORMAT,                                             /**< Y.1731 format of MA is not allowed, because MD's format is !none          */
    VTSS_APPL_CFM_RC_MA_MAID_TOO_LONG,                                            /**< Changing/Creating an MA causes the MAID to become too long                */
    VTSS_APPL_CFM_RC_MA_PORT_LIMIT_REACHED,                                       /**< The maximum number of port MEPs has already been created                  */
    VTSS_APPL_CFM_RC_MA_SERVICE_LIMIT_REACHED,                                    /**< The maximum number of service MEPs has already been created               */
    VTSS_APPL_CFM_RC_MA_LIMIT_REACHED,                                            /**< The maximum number of MAs inside this domain has already been created     */
    VTSS_APPL_CFM_RC_MA_ONLY_ONE_PORT_MEP_PER_PORT,                               /**< Changing MA type will result in more than one Port MEP per port           */
    VTSS_APPL_CFM_RC_MA_ONLY_ONE_VLAN_MEP_PER_PORT_PER_VLAN,                      /**< Changing MA type will result in more than 1 VLAN MEP in same <port, vid>  */
    VTSS_APPL_CFM_RC_MA_LEVEL_OF_PORT_MEP_MUST_BE_LOWER_THAN_LEVEL_OF_VLAN_MEP,   /**< Changing MA type to port MEP will result in P-MEP.lvl >= V-MEP.lvl        */
    VTSS_APPL_CFM_RC_MA_LEVEL_OF_VLAN_MEP_MUST_BE_HIGHER_THAN_LEVEL_OF_PORT_MEP,  /**< Changing MA type to VLAN MEP will result in V-MEP.lvl <= P-MEP.lvl        */
    VTSS_APPL_CFM_RC_MA_ALL_PORT_MEPS_MUST_HAVE_SAME_LEVEL,                       /**< Changing MA type will result in port MEPs of different levels             */
    VTSS_APPL_CFM_RC_MA_PORT_MEP_MUST_HAVE_LOWER_LEVEL_THAN_ANY_VLAN_MEP,         /**< Changing MA type will result in port MEPs at >= level than VLAN MEPs      */
    VTSS_APPL_CFM_RC_MA_VLAN_MEP_MUST_HAVE_HIGHER_LEVEL_THAN_PORT_MEPS,           /**< Changing MA type will result in VLAN MEPs at <= level than Port MEPs      */
    VTSS_APPL_CFM_RC_MA_VLAN_MEPS_NOT_SUPPORTED,                                  /**< VLAN MEPs are not supported on this platform (only Port MEPs)             */
    VTSS_APPL_CFM_RC_MEP_INVALID_MEPID,                                           /**< Invalid MEP ID. It must be in range [1; 8191]                             */
    VTSS_APPL_CFM_RC_MEP_INVALID_IFINDEX,                                         /**< Invalid ifindex. It must represent a port                                 */
    VTSS_APPL_CFM_RC_MEP_INVALID_DIRECTION,                                       /**< Invalid direction                                                         */
    VTSS_APPL_CFM_RC_MEP_DIRECTION_NOT_SUPPORTED_ON_THIS_PLATFORM,                /**< Direction is not supported on this platform                               */
    VTSS_APPL_CFM_RC_MEP_INVALID_VLAN,                                            /**< Invalid MEP VID                                                           */
    VTSS_APPL_CFM_RC_MEP_INVALID_PCP,                                             /**< Invalid PCP value                                                         */
    VTSS_APPL_CFM_RC_MEP_INVALID_SMAC,                                            /**< Source MAC address must be a unicast address                              */
    VTSS_APPL_CFM_RC_MEP_INVALID_ALARM_LEVEL,                                     /**< Invalid Alarm Level                                                       */
    VTSS_APPL_CFM_RC_MEP_INVALID_ALARM_TIME_PRESENT,                              /**< Invalid Alarm Present time (in ms)                                        */
    VTSS_APPL_CFM_RC_MEP_INVALID_ALARM_TIME_ABSENT,                               /**< Invalid Alarm Absent time (in ms)                                         */
    VTSS_APPL_CFM_RC_MEP_ALL_MEPS_IN_MA_MUST_HAVE_SAME_DIRECTION,                 /**< All MEPs in same MA must have same direction                              */
    VTSS_APPL_CFM_RC_MEP_ONLY_ONE_PORT_MEP_PER_PORT,                              /**< There can only be one port MEP per port                                   */
    VTSS_APPL_CFM_RC_MEP_ONLY_ONE_VLAN_MEP_PER_PORT_PER_VLAN,                     /**< Only one VLAN MEP on same <ifindex, vlan, direction>                      */
    VTSS_APPL_CFM_RC_MEP_LEVEL_OF_PORT_MEP_MUST_BE_LOWER_THAN_LEVEL_OF_VLAN_MEP,  /**< A VLAN MEP on same <port, vid> already exists with level < level of this  */
    VTSS_APPL_CFM_RC_MEP_LEVEL_OF_VLAN_MEP_MUST_BE_HIGHER_THAN_LEVEL_OF_PORT_MEP, /**< A Port MEP on same <port, vid> already exists with level > level of this  */
    VTSS_APPL_CFM_RC_MEP_ALL_PORT_MEPS_MUST_HAVE_SAME_LEVEL,                      /**< All port MEPs must have same level on Serval1                             */
    VTSS_APPL_CFM_RC_MEP_PORT_MEP_MUST_HAVE_LOWER_LEVEL_THAN_ANY_VLAN_MEP,        /**< A port MEP must have lower level than any VLAN MEP                        */
    VTSS_APPL_CFM_RC_MEP_VLAN_MEP_MUST_HAVE_HIGHER_LEVEL_THAN_PORT_MEPS,          /**< A VLAN MEP must have higher level than port MEPs                          */
    VTSS_APPL_CFM_RC_MEP_PORT_LIMIT_REACHED,                                      /**< The maximum number of port MEPs has already been created                  */
    VTSS_APPL_CFM_RC_MEP_SERVICE_LIMIT_REACHED,                                   /**< The maximum number of service MEPs has already been created               */
    VTSS_APPL_CFM_RC_RMEP_RMEPID_SAME_AS_MEPID,                                   /**< RMEPID identical to MEPID                                                 */
    VTSS_APPL_CFM_RC_RMEP_LIMIT_REACHED,                                          /**< The maximum number of RMEPs has already been created                      */
    VTSS_APPL_CFM_RC_HW_RESOURCES,                                                /**< Out of H/W resources                                                      */
};

/**
 * CCM rates. The enumeration values correspond to those used in the CCM PDUs
 * interval field (802.1Q-2018, Table 21-15).
 */
typedef enum {
    VTSS_APPL_CFM_CCM_INTERVAL_INVALID = 0, /**< No CCMs are sent      */
    VTSS_APPL_CFM_CCM_INTERVAL_300HZ   = 1, /**< 300 frames per second */
    VTSS_APPL_CFM_CCM_INTERVAL_10MS    = 2, /**< 100 frames per second */
    VTSS_APPL_CFM_CCM_INTERVAL_100MS   = 3, /**<  10 frames per second */
    VTSS_APPL_CFM_CCM_INTERVAL_1S      = 4, /**<   1 frame  per second */
    VTSS_APPL_CFM_CCM_INTERVAL_10S     = 5, /**<   6 frames per minute */
    VTSS_APPL_CFM_CCM_INTERVAL_1MIN    = 6, /**<   1 frame  per minute */
    VTSS_APPL_CFM_CCM_INTERVAL_10MIN   = 7  /**<   6 frames per hour   */
} vtss_appl_cfm_ccm_interval_t;

/**
 * Capabilities on this particular platform.
 */
typedef struct {
    /**
     * Maximum number of Maintenance Domains that can be created.
     */
    uint32_t md_cnt_max;

    /**
     * Maximum number of Maintenance Associations that can be created inside a
     * maintenance domain.
     */
    uint32_t ma_cnt_max;

    /**
     * Maximum number of Port MEPs (untagged) that can be created.
     */
    uint32_t mep_cnt_port_max;

    /**
     * Maximum number of Service MEPs (on VLANs) that can be created.
     */
    uint32_t mep_cnt_service_max;

    /**
     * Maximum number of Remote MEPs that can be monitored per MEP.
     */
    uint32_t rmep_cnt_max;

    /**
     * Not all platforms support the 300 CCM frames per second that the standard
     * indicates. This member indicates the minimum value you may assign to
     * vtss_appl_cfm_ma_conf_t::ccm_interval on this platform. It will contain
     * VTSS_APPL_CFM_CCM_INTERVAL_300HZ if the fastest standardized rate is
     * supported.
     */
    vtss_appl_cfm_ccm_interval_t ccm_interval_min;

    /**
     * Not all platforms support the slowest CCM frames that the standard
     * otherwise dictates. This member indicates the maximum value you may
     * assign to vtss_appl_cfm_ma_conf_t::ccm_interval on this platform. It will
     * contain VTSS_APPL_CFM_CCM_INTERVAL_10MIN if the slowest standardized rate
     * is supported.
     */
    vtss_appl_cfm_ccm_interval_t ccm_interval_max;

    /**
     * If this member is false, this platform only supports MEPs.
     * If true, it supports both MEPs and MIPs.
     */
    mesa_bool_t has_mips;

    /**
     * If this member is false, this platform only supports down-MEPs.
     * If true, it supports both up- and down-MEPs.
     */
    mesa_bool_t has_up_meps;

    /**
     * If this member is true, this platform uses shared MEG level, which means
     * that all Port MEPs must run in the same level, and any VLAN MEP must run
     * on a higher level than port MEPs.
     *
     * If this member is false, this platform uses independent MEG level, which
     * means that a VLAN MEP can run on a lower MEG level than a port MEP,
     * provided they run in different VLANs.
     */
    mesa_bool_t has_shared_meg_level;

    /**
     * If this member is true, this platform supports VLAN MEPs.
     *
     * If this member is false, only port MEPs can be created.
     */
    mesa_bool_t has_vlan_meps;

} vtss_appl_cfm_capabilities_t;

/**
 * Get the capabilities for this platform.
 *
 * \param cap [OUT] This module's capabilities.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_capabilities_get(vtss_appl_cfm_capabilities_t *cap);

/**
 * Make it easier to identify MEPIDs in this API.
 * A MEPID is an integer in range [1; 8191].
 */
typedef uint32_t vtss_appl_cfm_mepid_t;

/**
 * Sender ID options (12.14.6.1.3).
 */
typedef enum {
    VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DISABLE,        /**< Do not put Sender ID TLVs in PDUs (default for top level)                                                       */
    VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_CHASSIS,        /**< Put Chassis ID (MAC Address, Type 4) in Sender ID TLVs                                                          */
    VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_MANAGE,         /**< Put management address (IPv4) in Sender ID TLV                                                                  */
    VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_CHASSIS_MANAGE, /**< Put Chassis ID and management address in Sender ID TLV                                                          */
    VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER           /**< Let the higher level determine the value of the Sender ID TLV (default for lower levels, invalid for top level) */
} vtss_appl_cfm_sender_id_tlv_option_t;

/**
 * Port Status TLV and Interface Status TLV options
 */
typedef enum {
    VTSS_APPL_CFM_TLV_OPTION_DISABLE, /**< Do not put Port or Interface Status TLVs in PDUs (default for top level)                                                         */
    VTSS_APPL_CFM_TLV_OPTION_ENABLE,  /**< Put Port or Interface Status TLVs in PDUs                                                                                        */
    VTSS_APPL_CFM_TLV_OPTION_DEFER    /**< Let the higher level determine the value of the Port or Interface Status TLVs  (default for lower levels, invalid for top level) */
} vtss_appl_cfm_tlv_option_t;

/**
 * Structure containing the fields of an Organization-Specific TLV.
 * The TLV is defined in clause 21.5.2.
 * Used both when transmitting CCMs and when receiving CCMs. See enclosing
 * structures for ways to find out whether the information in this structure is
 * valid or not.
 */
typedef struct {
    /**
     * This is the three-bytes OUI transmitted or received with the
     * Organization-Specific TLV.
     * There is no check on contents.
     */
    uint8_t oui[3];

    /**
     * This is the subtype transmitted or received with the
     * Organization-Specific TLV.
     * Can be any value in range [0; 255].
     */
    uint8_t subtype;

    /**
     * Since there are no constraints on the contents of the
     * organization-specific TLV value (could be binary), a length must be
     * given. It could be 0 if needed.
     *
     * When this structure is used for CCM transmission, #value_len must not
     * exceed sizeof(#value).
     *
     * If this structure is used for CCM reception, value_len may exceed
     * sizeof(#value), which means that the value in the CCM PDU was longer than
     * what we can hold in #value.
     *
     * Notice that if #value is a plain ASCII string, the contents may not be
     * NULL-terminated (if #value_len == sizeof(#value)).
     */
    uint16_t value_len;

    /**
     * The value transmitted in the Organization-Specific TLV.
     *
     * There are no constraints on the contents of this array, so #value_len
     * tells the (minimum) number of valid bytes in the array. See #value_len
     * for a more detailed description.
     */
    uint8_t value[64];
} vtss_appl_cfm_organization_specific_tlv_t;

/**
 * Global configuration
 */
typedef struct {
    /**
     * Choose whether and what to use as Sender ID for PDUs generated by this
     * switch.
     *
     * The value VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER is invalid at this
     * global level.
     *
     * Can be overridden by MD and MA.
     * Default is VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DISABLE.
     */
    vtss_appl_cfm_sender_id_tlv_option_t sender_id_tlv_option;

    /**
     * Choose whether to send Port Status TLVs in CCMs generated by this switch.
     *
     * The value VTSS_APPL_CFM_TLV_OPTION_DEFER is invalid at this global level.
     *
     * Can be overridden by MD and MA.
     * Default is VTSS_APPL_CFM_TLV_OPTION_DISABLE.
     */
    vtss_appl_cfm_tlv_option_t port_status_tlv_option;

    /**
     * Choose whether to send Interface Status TLVs in CCMs generated by this
     * switch.
     *
     * The value VTSS_APPL_CFM_TLV_OPTION_DEFER is invalid at this global level.
     *
     * Can be overridden by MD and MA.
     * Default is VTSS_APPL_CFM_TLV_OPTION_DISABLE.
     */
    vtss_appl_cfm_tlv_option_t interface_status_tlv_option;

    /**
     * Choose whether to send Organization-Specific TLV in CFM PDUs generated by
     * this switch.
     *
     * The value VTSS_APPL_CFM_TLV_OPTION_DEFER is invalid at this global level.
     *
     * Can also be disabled at the MD and MA levels.
     * Default is VTSS_APPL_CFM_TLV_OPTION_DISABLE.
     *
     * If enabled, the contents of #organization_specific_tlv must be valid.
     */
    vtss_appl_cfm_tlv_option_t organization_specific_tlv_option;

    /**
     * If sending organization-specific TLVs along with CFM PDUs generated by
     * this switch (see #organization_specific_tlv_option), this structure must
     * be filled in.
     */
    vtss_appl_cfm_organization_specific_tlv_t organization_specific_tlv;
} vtss_appl_cfm_global_conf_t;

/**
 * Get default global configuration.
 *
 * \param conf [OUT] Pointer to structure receiving default global configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_global_conf_default_get(vtss_appl_cfm_global_conf_t *conf);

/**
 * Get the global configuration.
 *
 * \param conf [OUT] Pointer to structure receiving the global configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_global_conf_get(vtss_appl_cfm_global_conf_t *conf);

/**
 * Change the global configuration.
 *
 * \param conf [IN] Pointer to structure with new global configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_global_conf_set(const vtss_appl_cfm_global_conf_t *conf);

/**
 * Maximum length of friendly Domain and Service names.
 */
#define VTSS_APPL_CFM_KEY_LEN_MAX 15

/**
 * Domains and maintenance associations are identified by a friendly name, which
 * has certain restrictions:
 * strlen(key) must be [1; VTSS_APPL_CFM_KEY_LEN_MAX].
 * key[0] must be in range [a-zA-Z] (isalpha()).
 * key[1]..key[strlen(key) - 1] must be in range [33; 126] except for 58
 * (isgraph(), but ':' is reserved).
 * key must not be 'all' (case-insensitive), since this is a reserved keyword
 * for CLI when deleting all domains or services.
 */
typedef std::string vtss_appl_cfm_name_key_t;

/**
 * MDs are accessed by this key.
 */
typedef struct {
    /**
     * MD name
     */
    vtss_appl_cfm_name_key_t md;
} vtss_appl_cfm_md_key_t;

/**
 * Specialized function to output contents of an MD key to a stream
 *
 * \param o      [OUT] Stream
 * \param md_key [IN]  Key to print
 *
 * \return The stream itself
 */
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_cfm_md_key_t &md_key);

/**
 * Specialized fmt() function usable in trace commands.
 *
 * Suppose you want to trace an MD key. You *could* do it like this:
 *   T_I("Domain = %s", key.md.c_str());
 *
 * The presence of the following function ensures that you can do it like this:
 *   T_I("Domain = %s", key);
 *
 * You do not need to know about the parameters, so they won't be documented
 * very well.
 *
 * \param o      [OUT] Stream
 * \param fmt    [IN]  Format
 * \param md_key [IN]  Key to print
 *
 * \return Number of bytes written.
 */
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_cfm_md_key_t *md_key);

/**
 * Supported Maintenance Domain formats to be used in CCM PDUs.
 * The enumeration values correspond to the values used in the PDUs.
 */
typedef enum {
    VTSS_APPL_CFM_MD_FORMAT_NONE   =  1, /**< No MD name present (typically used in ITU) */
    VTSS_APPL_CFM_MD_FORMAT_STRING =  4, /**< Use a string                               */
} vtss_appl_cfm_md_format_t;

/**
 * Maintenance Domain (MD) configuration.
 */
typedef struct {
    /**
     * Select the MD name format.
     * To mimic Y.1731 MEG IDs, use type VTSS_APPL_CFM_MD_FORMAT_NONE.
     */
    vtss_appl_cfm_md_format_t format;

    /**
     * Maintenance Domain Name.
     *
     * The contents of this member depends on the value of the #format member.
     *
     * If #format is VTSS_APPL_CFM_MD_FORMAT_NONE:
     *   #name is not used, but will be set to all-zeros behind the scenes.
     *   This format is typically used by Y.1731-kind-of-PDUs.
     *
     * If #format is VTSS_APPL_CFM_MD_FORMAT_STRING:
     *   #name must contain a string from 1 to 43 characters long plus a
     *   terminating NULL. It allows characters in range [32; 126] (isprint()).
     */
    char name[44];

    /**
     * MD/MEG Level (0-7).
     */
    uint8_t level;

    /**
     * Choose whether and what to use as Sender ID for PDUs generate in this MD.
     * Can be overridden by MA configuration.
     *
     * Default is VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER, which means: Let the
     * global configuration decide.
     */
    vtss_appl_cfm_sender_id_tlv_option_t sender_id_tlv_option;

    /**
     * Choose whether to send Port Status TLVs in CCMs generated by this switch.
     * Can be overridden by MA configuration.
     *
     * Default is VTSS_APPL_CFM_TLV_OPTION_DEFER, which means: Let the global
     * configuration decide.
     */
    vtss_appl_cfm_tlv_option_t port_status_tlv_option;

    /**
     * Choose whether to send Interface Status TLVs in CCMs generated by this
     * switch. Can be overridden by MA configuration.
     *
     * Default is VTSS_APPL_CFM_TLV_OPTION_DEFER, which means: Let the global
     * configuration decide.
     */
    vtss_appl_cfm_tlv_option_t interface_status_tlv_option;

    /**
     * Choose whether to send Organization-Specific TLV in CFM PDUs generated by
     * this switch. Can be overridden by MA configuration.
     *
     * The value VTSS_APPL_CFM_TLV_OPTION_ENABLE is invalid at this level.
     *
     * Default is VTSS_APPL_CFM_TLV_OPTION_DEFER, which means: Let the global
     * configuration decide.
     */
    vtss_appl_cfm_tlv_option_t organization_specific_tlv_option;
} vtss_appl_cfm_md_conf_t;

/**
 * Get a default Maintenance Domain configuration.
 *
 * \param conf [OUT] Pointer to structure receiving default MD configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_md_conf_default_get(vtss_appl_cfm_md_conf_t *conf);

/**
 * Get the configuration of an existing Maintenance Domain.
 *
 * \param key  [IN]  MD name.
 * \param conf [OUT] Pointer to structure receiving \p key's configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_md_conf_get(const vtss_appl_cfm_md_key_t &key, vtss_appl_cfm_md_conf_t *conf);

/**
 * Create a new or change an existing Maintenance Domain.
 *
 * \param key  [IN] MD name.
 * \param conf [IN] Pointer to structure with \p key's new configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_md_conf_set(const vtss_appl_cfm_md_key_t &key, const vtss_appl_cfm_md_conf_t *conf);

/**
 * Delete an existing Maintenance Domain and all its MAs and all the MA's MEPs.
 *
 * \param key [IN] MD name to delete along with its MAs and the MAs' MEPs.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_md_conf_del(const vtss_appl_cfm_md_key_t &key);

/**
 * Iterate across all defined maintenance domains.
 *
 * Use this function to iterate through all created domains.
 *
 * To get started, set prev_key to NULL and pass a valid pointer in
 * next_key. Prior to subsequent iterations, set prev_key = next_key.
 *
 * \param prev_key [IN]  Previous configured MD name.
 * \param next_key [OUT] Next configured MD name.
 *
 * \return VTSS_RC_OK as long as next_key is OK,
 * VTSS_APPL_CFM_RC_END_OF_LIST when at the end and any other return code on
 * a real error.
 */
mesa_rc vtss_appl_cfm_md_itr(const vtss_appl_cfm_md_key_t *prev_key, vtss_appl_cfm_md_key_t *next_key);

/**
 * MAs are accessed by this key.
 * It inherits from vtss_appl_cfm_md_key_t, so besides the MEPID, it also
 * contains the MD name.
 */
struct vtss_appl_cfm_ma_key_t : public vtss_appl_cfm_md_key_t {
    /**
     * MA name
     */
    vtss_appl_cfm_name_key_t ma;
};

/**
 * Specialized function to output contents of an MA key to a stream
 *
 * \param o      [OUT] Stream
 * \param ma_key [IN]  Key to print
 *
 * \return The stream itself
 */
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_cfm_ma_key_t &ma_key);

/**
 * Specialized fmt() function usable in trace commands.
 *
 * Suppose you want to trace an MA key. You *could* do it like this:
 *   T_I("Domain::Service = %s::%s", key.md.c_str(), key.ma.c_str());
 *
 * The presence of the following function ensures that you can do it like this:
 *   T_I("Domain::Service = %s", key);
 *
 * You do not need to know about the parameters, so they won't be documented
 * very well.
 *
 * \param o      [OUT] Stream
 * \param fmt    [IN]  Format
 * \param ma_key [IN]  Key to print
 *
 * \return Number of bytes written.
 */
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_cfm_ma_key_t *ma_key);

/**
 * Supported short Maintenance Name formats to be used in CCM PDUs.
 * The enumeration values correspond to the values used in the PDUs, except for
 * VTSS_APPL_CFM_MA_FORMAT_PRIMARY_VID, which uses that of
 * VTSS_APPL_CFM_MA_FORMAT_TWO_OCTET_INTEGER. The enumeration value for Primary
 * VID is chosen in such a way that it can be put directly into a byte and still
 * get the correct value.
 */
typedef enum {
    VTSS_APPL_CFM_MA_FORMAT_STRING            =   2,     /**< Use a string                                    */
    VTSS_APPL_CFM_MA_FORMAT_TWO_OCTET_INTEGER =   3,     /**< Use a 2-byte integer                            */
    VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC         =  32,     /**< Use the Y.1731 ICC-based format                 */
    VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC_CC      =  33,     /**< Use the Y.1731 ICC- and CC-based format         */
    VTSS_APPL_CFM_MA_FORMAT_PRIMARY_VID       = 256 + 3, /**< Use 2-byte integer format with MA's primary VID */
} vtss_appl_cfm_ma_format_t;

/**
 * Maintenance Association (MA) configuration.
 */
typedef struct {
    /**
     * Select the short MA name format.
     *
     * To mimic Y.1731 MEG IDs, create an MD with an empty name and use
     * VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC or
     * VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC_CC.
     */
    vtss_appl_cfm_ma_format_t format;

    /**
     * Maintenance Association Name.
     *
     * The contents of this member depends on the value of the #format member.
     *
     * Besides the limitations explained for each of them, the following
     * applies in general:
     *   If the MD format is VTSS_APPL_CFM_MD_FORMAT_NONE, the size of this
     *   cannot exceed 45 bytes.
     *
     *   If the MD format is not VTSS_APPL_CFM_MD_FORMAT_NONE, the size of this
     *   cannot exceed 44 - strlen(MD name).
     *
     * If #format is VTSS_APPL_CFM_MA_FORMAT_STRING, the following applies:
     *   strlen(name) must be in range [1; 45].
     *   The string must be NULL-terminated.
     *   Contents must be in range [32; 126] (isprint()).
     *
     * If #format is VTSS_APPL_CFM_MA_FORMAT_TWO_OCTET_INTEGER, the
     * following applies:
     *   name[0] and name[1] will both be interpreted as unsigned 8-bit integers
     *   (allowing a range of [0; 255]).
     *   name[0] will be placed in the PDU before name[1].
     *   The remaining available bytes in name[] will not be used.
     *
     * If #format is VTSS_APPL_CFM_MA_FORMAT_PRIMARY_VID, the format will be
     * that of VTSS_APPL_CFM_MA_FORMAT_TWO_OCTET_INTEGER, but the contents will
     * follow the MA's primary VID.
     *
     * If #format is VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC, the following
     * applies:
     *   strlen(name) must be 13.
     *   The string must be NULL-terminated.
     *   Contents must be in range [a-zA-Z0-9] (isalnum()).
     *
     *   Y.1731 specifies that it is a concatenation of ICC (ITU Carrier Code)
     *   and UMC (Unique MEG ID Code):
     *     ICC: 1-6 bytes
     *     UMC: 7-12 bytes
     *
     *   In principle UMC can be any value in range [1; 127], but this API does
     *   not allow for specifying length of ICC, so the underlying code doesn't
     *   know where ICC ends and UMC starts.
     *
     *   When using this, the MD format must be VTSS_APPL_CFM_MD_FORMAT_NONE.
     *
     * If #format is VTSS_APPL_CFM_MA_FORMAT_Y1731_ICC_CC, the following
     * applies:
     *   strlen(name) must be 15.
     *   First 2 chars   (CC):  Must be amongst [A-Z] (isupper()).
     *   Next 1-6 chars  (ICC): Must be amongst [a-zA-Z0-9] (isalnum()).
     *   Next 7-12 chars (UMC): Must be amongst [a-zA-Z0-9] (isalnum()).
     *   There may be ONE "/" present in name[3-7].
     *
     *   In principle UMC can be any value in range [1; 127], but this API does
     *   not allow for specifying length of ICC, so the underlying code doesn't
     *   know where ICC ends and UMC starts.
     *
     *   When using this, the MD format must be VTSS_APPL_CFM_MD_FORMAT_NONE.
     */
    char name[46];

    /**
     * The MA's primary VID.
     *
     * A primary VID of 0 means that all MEPs created within this MA will be
     * created as port MEPs (interface MEPs). There can only be one port MEP
     * per interface. A given port MEP may still be created with tags, if that
     * MEP's VLAN is non-zero - even if it is made on the port's PVID and that
     * VID is untagged.
     *
     * A non-zero primary VID means that all MEPs created within this MA will
     * be created as VLAN MEPs. A given MEP may be configured with another VLAN
     * than the MA's primary VID. If the resulting VID is in the port's untagged
     * set (e.g. if PVID is transmitted untagged), the frames will become
     * untagged on egress.
     */
    mesa_vid_t vlan;

    /**
     * The CCM rate of all MEPs bound to this MA.
     *
     * Must not be VTSS_APPL_CFM_CCM_INTERVAL_INVALID.
     *
     * Also, not all rates are supported on all platforms (see
     * vtss_appl_cfm_capabilities_t::ccm_interval_min and ccm_interval_max).
     */
    vtss_appl_cfm_ccm_interval_t ccm_interval;

    /**
     * Choose whether and what to use as Sender ID for PDUs generate in this MA.
     *
     * Default is VTSS_APPL_CFM_SENDER_ID_TLV_OPTION_DEFER, which means: Let the
     * MD configuration decide.
     */
    vtss_appl_cfm_sender_id_tlv_option_t sender_id_tlv_option;

    /**
     * Choose whether to send Port Status TLVs in CCMs generated by this switch.
     *
     * Default is VTSS_APPL_CFM_TLV_OPTION_DEFER, which means: Let the MD
     * configuration decide.
     */
    vtss_appl_cfm_tlv_option_t port_status_tlv_option;

    /**
     * Choose whether to send Interface Status TLVs in CCMs generated by this
     * switch.
     *
     * Default is VTSS_APPL_CFM_TLV_OPTION_DEFER, which means: Let the MD
     * configuration decide.
     */
    vtss_appl_cfm_tlv_option_t interface_status_tlv_option;

    /**
     * Choose whether to send Organization-Specific TLV in CFM PDUs generated by
     * this switch.
     *
     * The value VTSS_APPL_CFM_TLV_OPTION_ENABLE is invalid at this level.
     *
     * Default is VTSS_APPL_CFM_TLV_OPTION_DEFER, which means: Let the MD
     * configuration decide.
     */
    vtss_appl_cfm_tlv_option_t organization_specific_tlv_option;
} vtss_appl_cfm_ma_conf_t;

/**
 * Get a default Maintenance Association configuration.
 *
 * \param conf [OUT] Pointer to structure receiving default MA configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_ma_conf_default_get(vtss_appl_cfm_ma_conf_t *conf);

/**
 * Get the configuration of an existing Maintenance Association.
 *
 * \param key  [IN]  Key identifying this MA.
 * \param conf [OUT] Pointer to structure receiving MA's configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_ma_conf_get(const vtss_appl_cfm_ma_key_t &key, vtss_appl_cfm_ma_conf_t *conf);

/**
 * Create a new or change an existing Maintenance Association.
 *
 * \param key  [IN] Key identifying the MA
 * \param conf [IN] Pointer to structure with MA's new configuration
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_ma_conf_set(const vtss_appl_cfm_ma_key_t &key, const vtss_appl_cfm_ma_conf_t *conf);

/**
 * Delete an existing Maintenance Association and all its MEPs.
 *
 * \param key [IN] Key identifyint the MA to delete along with all its MEPs
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_ma_conf_del(const vtss_appl_cfm_ma_key_t &key);

/**
 * Iterate across MAs.
 *
 * To iterate across all MAs in all MDs, set prev_key to NULL.
 * Prior to subsequent iterations, set prev_key = next_key.
 *
 * To iterate across all MAs within a given MD, start by setting
 *   prev_key.md to the MD
 *   prev_key.ma = ""
 * Then iterate with \p stay_in_this_md set to true. This will cause the
 * function to return VTSS_APPL_CFM_RC_END_OF_LIST before entering another MD.
 *
 * \param prev_key        [IN]  Previous MA
 * \param next_key        [OUT] Next MA
 * \param stay_in_this_md [IN]  See text above
 *
 * \return VTSS_RC_OK as long as next_key is OK,
 * VTSS_APPL_CFM_RC_END_OF_LIST when at the end and any other return code on
 * a real error.
 */
mesa_rc vtss_appl_cfm_ma_itr(const vtss_appl_cfm_ma_key_t *prev_key, vtss_appl_cfm_ma_key_t *next_key, bool stay_in_this_md = false);

/**
 * MEP configuration and MEP status are accessed by this key.
 * It inherits from vtss_appl_cfm_ma_key_t, so besides the MEPID, it also
 * contains the MD and MA names.
 */
struct vtss_appl_cfm_mep_key_t : public vtss_appl_cfm_ma_key_t {
    /**
     * MEPID
     */
    vtss_appl_cfm_mepid_t mepid;
};

/**
 * Specialized function to output contents of a MEP key to a stream
 *
 * \param o       [OUT] Stream
 * \param mep_key [IN]  Key to print
 *
 * \return The stream itself
 */
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_cfm_mep_key_t &mep_key);

/**
 * Specialized fmt() function usable in trace commands.
 *
 * Suppose you want to trace a MEP key. You *could* do it like this:
 *   T_I("Domain::Service::mepid = %s::%s::%u", key.md.c_str(), key.ma.c_str(), key.mepid);
 *
 * The presence of the following function ensures that you can do it like this:
 *   T_I("Domain::Service::mepid = %s", key);
 *
 * You do not need to know about the parameters, so they won't be documented
 * very well.
 *
 * \param o       [OUT] Stream
 * \param fmt     [IN]  Format
 * \param mep_key [IN]  Key to print
 *
 * \return Number of bytes written.
 */
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_cfm_mep_key_t *mep_key);

/**
 * Specialized operator< for vtss_appl_cfm_mep_key_t in order to be able to use
 * it as a key in a vtss::Map (useful for the notification serializer, for
 * instance).
 *
 * The implementation of this operator ensures that a MEP key is first sorted by
 * md, then by ma, and finally by mepid.
 *
 * \param lhs [IN] left-hand-side of operator <
 * \param rhs [IN] right-hand-side of operator <
 *
 * \return true if lhs < rhs, false otherwise.
 */
bool operator<(const vtss_appl_cfm_mep_key_t &lhs, const vtss_appl_cfm_mep_key_t &rhs);

/**
 * Specialized operator== for vtss_appl_cfm_mep_key_t in order to be able to use
 * it in conditional statements to check if two MEP keys are the same.
 *
 * \param lhs [IN] left-hand-side of operator==
 * \param rhs [IN] right-hand-side of operator==
 *
 * \return true if lhs != rhs, false otherwise.
 */
bool operator==(const vtss_appl_cfm_mep_key_t &lhs, const vtss_appl_cfm_mep_key_t &rhs);

/**
 * Specialized operator!= for vtss_appl_cfm_mep_key_t in order to be able to use
 * it in conditional statements to check if two MEP keys are different.
 *
 * \param lhs [IN] left-hand-side of operator!=
 * \param rhs [IN] right-hand-side of operator!=
 *
 * \return true if lhs != rhs, false otherwise.
 */
bool operator!=(const vtss_appl_cfm_mep_key_t &lhs, const vtss_appl_cfm_mep_key_t &rhs);

/**
 * Direction of a particular MIP/MEP.
 * Its enumeration values match those of the CFM MIB.
 */
typedef enum {
    VTSS_APPL_CFM_DIRECTION_DOWN = 1, /**< Instance is a down-MEP/MIP */
    VTSS_APPL_CFM_DIRECTION_UP   = 2, /**< Instance is an up-MEP/MIP  */
} vtss_appl_cfm_direction_t;

/**
 * This structure defines a Maintenance Association Endpoint (MEP).
 */
typedef struct {
    /**
     * Determines whether this is an Up- or a Down-MEP.
     * Notice, that not all platforms support Up-MEPs (see
     * vtss_appl_cfm_capabilities_t::has_up_meps).
     */
    vtss_appl_cfm_direction_t direction;

    /**
     * Port on which this MEP resides.
     *
     * At the time of writing, only interface indices that indicate a port are
     * allowed (so no aggregation support).
     */
    vtss_ifindex_t ifindex;

    /**
     * VLAN ID.
     *
     * A value of 0 indicates that this MEP must use the Primary VID of the
     * encompassing MA (vtss_appl_cfm_ma_conf_t::vlan). If the MA's Primary
     * VID is 0, a port MEP will be created. If the MA's Primary VID is
     * non-zero, a VLAN MEP will be created.
     *
     * See vtss_appl_cfm_ma_conf_t::vlan for tagging of PDUs.
     *
     * See also \p pcp.
     * PDUs are always transmitted with DEI = 0.
     */
    mesa_vid_t vlan;

    /**
     * PCP (priority) (default 0).
     * The PCP value used in the VLAN tag unless the MEP is untagged.
     * Must be a value in range [0; 7].
     *
     * See also \p vlan.
     * Frames are always sent with DEI = 0.
     */
    mesa_pcp_t pcp;

    /**
     * Source MAC address used in all PDUs originating at this MEP.
     *
     * Must be a unicast address.
     * If all-zeros, the switch port's MAC address will be used instead.
     *
     * In 802.1Q, this is a status varaible and not a configuration variable,
     * but in order to meet customers' needs, it has been chosen to make it a
     * configuration. It can, however, also be found in the MEP's status.
     */
    mesa_mac_t smac;

    /**
     * Start or stop generation of CCMs (default false).
     * Actual generation will only be started if both this member is true AND
     * vtss_appl_cfm_ma_conf_t::ccm_interval is not
     * VTSS_APPL_CFM_CCM_INTERVAL_INVALID.
     */
    mesa_bool_t ccm_enable;

    /**
     * The lowest priority defect that is allowed to generate a fault alarm.
     * See clause 20.9.5, LowestAlarmPri.
     *
     * Valid range is [1; 6] with 1 indicating that any defect will cause a
     * fault alarm and 6 indicating that no defect can cause a fault alarm.
     *
     * It follows the CFM MIB's definition closely:
     *  1: someRDIdefect, someMACstatusDefect, someRMEPCCMdefect, errorCCMdefect, xconCCMdefect
     *  2:                someMACstatusDefect, someRMEPCCMdefect, errorCCMdefect, xconCCMdefect
     *  3:                                     someRMEPCCMdefect, errorCCMdefect, xconCCMdefect
     *  4:                                                        errorCCMdefect, xconCCMdefect
     *  5:                                                                        xconCCMdefect
     *  6: No defects are to be reported
     *
     * Default is 2.
     */
    uint32_t alarm_level;

    /**
     * The time that defects must be present before a fault alarm is issued.
     * See clause 20.33.3 (fngAlarmTime).
     *
     * Valid values are in range 2500-10000 milliseconds (the MIB specifies it
     * in 0.01s intervals, which gives a range of 250-1000).
     *
     * Default is 2500 milliseconds.
     */
    uint32_t alarm_time_present_ms;

    /**
     * The time that defects must be absent before a fault alarm is cleared.
     * See clause 20.33.4 (fngResetTime).
     *
     * Valid values are in range 2500-10000 milliseconds (the MIB specifies it
     * in 0.01s intervals, which gives a range of 250-1000).
     *
     * Default is 10000 milliseconds.
     */
    uint32_t alarm_time_absent_ms;

    /**
     * The administrative state of this MEP (default false).
     * Set to true to make it function normally and false to make it cease
     * functioning.
     * When false, the MEP is torn down, so it will not respond to CFM PDUs
     * requiring a response, and it will not generate CFM PDUs.
     */
    mesa_bool_t admin_active;
} vtss_appl_cfm_mep_conf_t;

/**
 * Get a default MEP configuration.
 *
 * \param conf [OUT] Pointer to structure receiving default MEP configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_mep_conf_default_get(vtss_appl_cfm_mep_conf_t *conf);

/**
 * Get the configuration of an existing MEP.
 *
 * \param key  [IN]  Key identifying the MEP
 * \param conf [OUT] Pointer to structure receiving MEP's configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_mep_conf_get(const vtss_appl_cfm_mep_key_t &key, vtss_appl_cfm_mep_conf_t *conf);

/**
 * Create a new or change an existing MEP.
 *
 * After successful creation, check its operation state with a call to
 * vtss_appl_cfm_mep_status_get().
 *
 * \param key  [IN]  Key identifying the MEP
 * \param conf [OUT] Pointer to structure with MEP's new configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_mep_conf_set(const vtss_appl_cfm_mep_key_t &key, const vtss_appl_cfm_mep_conf_t *conf);

/**
 * Delete an existing MEP.
 *
 * \param key [IN] Key identifying the MEP to delete
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_mep_conf_del(const vtss_appl_cfm_mep_key_t &key);

/**
 * Iterate across MEPs.
 *
 * To iterate across all MEPs, set prev_key to NULL and pass a valid pointer
 * in next_key. Prior to subsequent iterations, set prev_key = next_key.
 *
 * To iterate across MEPs within a given MA, start by setting
 *   prev_key.md to the MD
 *   prev_key.ma to the MA
 *   prev_key.mepid = 0.
 * Then iterate with \p stay_in_this_ma set to true. This will cause the
 * function to return VTSS_APPL_CFM_RC_END_OF_LIST before entering another MD
 * or MA.
 *
 * \param prev_key        [IN]  Previous MEP
 * \param next_key        [OUT] Next MEP
 * \param stay_in_this_ma [IN]  See text above
 *
 * \return VTSS_RC_OK as long as next_key is OK,
 * VTSS_APPL_CFM_RC_END_OF_LIST when at the end and any other return code on
 * a real error.
 */
mesa_rc vtss_appl_cfm_mep_itr(const vtss_appl_cfm_mep_key_t *prev_key, vtss_appl_cfm_mep_key_t *next_key, bool stay_in_this_ma = false);

/**
 * Remote MEP configuration and status are accessed by this key.
 * It inherits from vtss_appl_cfm_mep_key_t, so besides the rmepid, it also
 * contains the MD and MA names and the MEP's mepid.
 */
struct vtss_appl_cfm_rmep_key_t : public vtss_appl_cfm_mep_key_t {
    /**
     * Remote MEPID
     */
    vtss_appl_cfm_mepid_t rmepid;
};

/**
 * Specialized function to output contents of an RMEP key to a stream
 *
 * \param o        [OUT] Stream
 * \param rmep_key [IN]  Key to print
 *
 * \return The stream itself
 */
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_cfm_rmep_key_t &rmep_key);

/**
 * Specialized fmt() function usable in trace commands.
 *
 * Suppose you want to trace an RMEP key. You *could* do it like this:
 *   T_I("Domain::Service::mepid::rmepid = %s::%s::%u:%u", key.md.c_str(), key.ma.c_str(), key.mepid, key.rmepid);
 *
 * The presence of the following function ensures that you can do it like this:
 *   T_I("Domain::Service::mepid::rmepid = %s", key);
 *
 * You do not need to know about the parameters, so they won't be documented
 * very well.
 *
 * \param stream [OUT] Stream
 * \param fmt    [IN]  Format
 * \param key    [IN]  Key to print
 *
 * \return Number of bytes written.
 */
size_t fmt(vtss::ostream &stream, const vtss::Fmt &fmt, const vtss_appl_cfm_rmep_key_t *key);

/**
 * This structure defines a remote MEP.
 * In the functions using this structure, it is indexed by the remote MEP's
 * MEPID, which must be an integer in range [1; 8191] and must not be the same
 * as other MEPs in this MA's MEPID.
 */
typedef struct {
    /**
     * As of now, it doesn't hold any data, but since that's not allowed, there
     * is a dummy value.
     */
    uint32_t unused;
} vtss_appl_cfm_rmep_conf_t;

/**
 * Get a default Remote MEP configuration.
 *
 * \param conf [OUT] Pointer to structure receiving default Remote MEP configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_rmep_conf_default_get(vtss_appl_cfm_rmep_conf_t *conf);

/**
 * Get the configuration of an existing Remote MEP.
 *
 * \param key  [IN]  Key identifying the Remote MEP
 * \param conf [OUT] Pointer to structure receiving Remote MEP's configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_rmep_conf_get(const vtss_appl_cfm_rmep_key_t &key, vtss_appl_cfm_rmep_conf_t *conf);

/**
 * Create a new or change an existing Remote MEP.
 *
 * \param key  [IN] Key identifying the Remote MEP
 * \param conf [IN] Pointer to structure with Remote MEP's new configuration.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_rmep_conf_set(const vtss_appl_cfm_rmep_key_t &key, const vtss_appl_cfm_rmep_conf_t *conf);

/**
 * Delete the monitoring of an existing Remote MEP.
 *
 * \param key    [IN]  Key identifying the Remote MEP to delete
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_rmep_conf_del(const vtss_appl_cfm_rmep_key_t &key);

/**
 * Iterate across Remote MEPs.
 *
 * To iterate across all Remote MEPs, set prev_key to NULL and pass a valid
 * pointer in next_key. Prior to subsequent iterations, set prev_key = next_key.
 *
 * To iterate across Remote MEPs within a given MEP, start by setting
 *   prev_key.md to the MD
 *   prev_key.ma to the MA
 *   prev_key.mepid = to the MEP's ID.
 * Then iterate with \p stay_in_this_mep set to true. This will cause the
 * function to return VTSS_APPL_CFM_RC_END_OF_LIST before entering another MD,
 * MA, or MEP.
 *
 * \param prev_key         [IN]  Previous configured Remote MEP
 * \param next_key         [OUT] Next configured Remote MEP
 * \param stay_in_this_mep [IN]  See text above
 *
 * \return VTSS_RC_OK as long as next_key is OK,
 * VTSS_APPL_CFM_RC_END_OF_LIST when at the end and any other return code on
 * a real error.
 */
mesa_rc vtss_appl_cfm_rmep_itr(const vtss_appl_cfm_rmep_key_t *prev_key, vtss_appl_cfm_rmep_key_t *next_key, bool stay_in_this_mep = false);

/**
 * Highest-priority defect that has been present since the MEP Fault
 * Notification Generator state machine was last in FNG_RESET state.
 * Enumerated values match those from the CFM MIB.
 */
typedef enum {
    VTSS_APPL_CFM_MEP_DEFECT_NONE       = 0, /**< No defects since FNG_RESET (DefNone/none)        */
    VTSS_APPL_CFM_MEP_DEFECT_RDI_CCM    = 1, /**< Remote Defect Indication   (someRDIdefect)       */
    VTSS_APPL_CFM_MEP_DEFECT_MAC_STATUS = 2, /**< MAC Status                 (someMACstatusDefect) */
    VTSS_APPL_CFM_MEP_DEFECT_REMOTE_CCM = 3, /**< Remote CCM                 (someRMEPCCMdefect)   */
    VTSS_APPL_CFM_MEP_DEFECT_ERROR_CCM  = 4, /**< Error CCM Recvd            (errorCCMdefect)      */
    VTSS_APPL_CFM_MEP_DEFECT_XCON_CCM   = 5, /**< Cross Connect CCM Recvd    (xconCCMdefect)       */
} vtss_appl_cfm_mep_defect_t;

// VTSS_APPL_CFM_MEP_DEFECT_MASK_xxx macros.
#define VTSS_APPL_CFM_MEP_DEFECT_MASK_RDI_CCM    (VTSS_BIT(VTSS_APPL_CFM_MEP_DEFECT_RDI_CCM    - 1)) /**< Mask value to test for RDI CCM     defects in vtss_appl_cfm_mep_status_t::defects */
#define VTSS_APPL_CFM_MEP_DEFECT_MASK_MAC_STATUS (VTSS_BIT(VTSS_APPL_CFM_MEP_DEFECT_MAC_STATUS - 1)) /**< Mask value to test for MAC Status  defects in vtss_appl_cfm_mep_status_t::defects */
#define VTSS_APPL_CFM_MEP_DEFECT_MASK_REMOTE_CCM (VTSS_BIT(VTSS_APPL_CFM_MEP_DEFECT_REMOTE_CCM - 1)) /**< Mask value to test for Remote CCM  defects in vtss_appl_cfm_mep_status_t::defects */
#define VTSS_APPL_CFM_MEP_DEFECT_MASK_ERROR_CCM  (VTSS_BIT(VTSS_APPL_CFM_MEP_DEFECT_ERROR_CCM  - 1)) /**< Mask value to test for Error CCM   defects in vtss_appl_cfm_mep_status_t::defects */
#define VTSS_APPL_CFM_MEP_DEFECT_MASK_XCON_CCM   (VTSS_BIT(VTSS_APPL_CFM_MEP_DEFECT_XCON_CCM   - 1)) /**< Mask value to test for Xcon CCM    defects in vtss_appl_cfm_mep_status_t::defects */

/**
 * This structure's members are booleans indicating various configuration
 * errors.
 */
typedef struct {
    /**
     * If this member is true, all of the following members are false:
     *   - #no_rmeps
     *   - #port_up_mep
     *   - #multiple_rmeps_ccm_interval
     *   - #is_mirror_port
     *   - #is_npi_port
     *   - #internal_error
     *   - #hw_resources
     *
     * Basically, it means that the configuration conditions for creating a MEP
     * are fulfilled.
     */
    mesa_bool_t mep_creatable;

    /**
     * If this member is true, all of the following members are false:
     *   - #no_link
     *   - #vlan_unaware
     *   - #vlan_membership
     *   - #stp_blocked
     *   - #mstp_blocked
     *
     * Basically, it means that if it is true, data can pass into the port
     * managed by this MEP, that is, it is not blocked by (M)STP or VLAN
     * configuration errors.
     *
     * If it is false, all RMEPs on this MEP will be held in
     * VTSS_APPL_CFM_RMEP_STATE_START in accordance with 802.1Q-2018, 20.20.
     *
     * The MEP may or may not be created. See #mep_creatable.
     *
     * This value is documented in 802.1Q, clause 20.9.2 as follows:
     *    enableRmepDefect = true  => Port Status TLV = psUp
     *    enableRmepDefect = false => Port Status TLV = psBlocked
     * If enableRmepDefect is true, the Remote MEP State Machines run normally.
     * If enableRmepDefect is false, the Remote MEP State Machines are disabled.
     */
    mesa_bool_t enableRmepDefect;

    /**
     * If this member is true, the MEP doesn't have any RMEPs.
     */
    mesa_bool_t no_rmeps;

    /**
     * If this member is true, the MEP's MA dictates port MEPs, which cannot be
     * created as Up-MEPs, only Down-MEPs.
     */
    mesa_bool_t port_up_mep;

    /**
     * If this member is true, the MEP has more than one RMEP, but the MA's
     * CCM interval is faster than VTSS_APPL_CFM_CCM_INTERVAL_1S.
     */
    mesa_bool_t multiple_rmeps_ccm_interval;

    /**
     * If this member is true, a H/W resource allocation failed while attempting
     * to create the MEP.
     */
    mesa_bool_t hw_resources;

    /**
     * If this member is true, an internal error has occurred while attempting
     * to create the MEP. Usually this requires a code-update.
     * Check console or crashlog for details.
     */
    mesa_bool_t internal_error;

    /**
     * If this member is true, the MEP's residence port is used as a mirror
     * destination port.
     *
     * Note that this will not be detected automatically if mirroring
     * configuration changes after a MEP is created.
     */
    mesa_bool_t is_mirror_port;

    /**
     * If this member is true, the MEP's residence port is configured as an NPI
     * port and is therefore not suitable for MEPs.
     *
     * Note that this will not be detected automatically when NPI configuration
     * changes after a MEP is created.
     */
    mesa_bool_t is_npi_port;

    /**
     * If this member is true, the MEP's residence port has no link.
     * This is known as Server Signal Fail (SSF) in Y.1731.
     */
    mesa_bool_t no_link;

    /**
     * If this member is true, the MEP is a VLAN MEP or a tagged port MEP
     * configured on a VLAN unaware port.
     */
    mesa_bool_t vlan_unaware;

    /**
     * If this member is true, the MEP's residence interface has ingress
     * filtering enabled and the interface is not a member of the MEP's
     * classified VLAN (which is the residence port's PVID if the MEP is
     * untagged)
     * #vlan_membership cannot be true if ingress filtering is disabled, since
     * any frame is allowed to enter the port in that case.
     */
    mesa_bool_t vlan_membership;

    /**
     * If this member is true, the MEP's residence port is not configured as
     * forwarding by the Spanning Tree protocol.
     */
    mesa_bool_t stp_blocked;

    /**
     * If this member is true, the MEP's residence port and classified VLAN is
     * not configured as forwarding by the MSTP protocol.
     */
    mesa_bool_t mstp_blocked;
} vtss_appl_cfm_mep_errors_t;

/**
 * State of Fault Notification Generator State Machine.
 * Enums are in accordance with the CFM MIB.
 */
typedef enum {
    VTSS_APPL_CFM_FNG_STATE_RESET           = 1, /**< No defect has been present since reset timer expired or the SM was last reset (fngReset,          FNG_RESET)           */
    VTSS_APPL_CFM_FNG_STATE_DEFECT          = 2, /**< A defect is present, but not for a long enough time to be reported            (fngDefect,         FNG_DEFECT)          */
    VTSS_APPL_CFM_FNG_STATE_REPORT_DEFECT   = 3, /**< A transient state during which the defect is reported                         (fngReportDefect,   FNG_REPORT_DEFECT)   */
    VTSS_APPL_CFM_FNG_STATE_DEFECT_REPORTED = 4, /**< A defect is present, and some defect has been reported                        (fngDefectReported, FNG_DEFECT_REPORTED) */
    VTSS_APPL_CFM_FNG_STATE_DEFECT_CLEARING = 5, /**< No defect is present, but the ResetTime timer has not yet expired             (fngDefectClearing, FNG_DEFECT_CLEARING) */
} vtss_appl_cfm_fng_state_t;

/**
 * Status of a given MEP. See also Table 17-11.
 */
typedef struct {
    /**
     * Operational state of the MEP. If it is adminstratively up and
     * successfully created, this member is true, false otherwise.
     * That is, it is true if
     *    vtss_appl_cfm_mep_conf_t::admin_active            == true &&
     *    vtss_appl_cfm_mep_status_t::errors::mep_creatable == true
     *
     * Not part of standard CFM MIB.
     */
    mesa_bool_t mep_active;

    /**
     * Holds the current state of the Fault Notification Generator State Machine
     * See enum for possible values.
     *
     * Called dot1agCfmMepFngState in the CFM MIB.
     * See 12.14.7.1.3:f and 20.37.
     */
    vtss_appl_cfm_fng_state_t fng_state;

    /**
     * This MEP's MAC address.
     *
     * Called dot1agCfmMepMacAddress in the CFM MIB.
     * See 12.14.7.1.3:i
     */
    mesa_mac_t smac;

    /**
     * Highest priority defect that has been present since the MEP's fault
     * notification generator state machine was last in the FNG_RESET state.
     *
     * This may in turn give rise to a notification, if this defect is higher
     * than what is indicated by vtss_appl_cfm_mep_conf_t::alarm_level.
     *
     * If in FNG_STATE_DEFECT, the highest defect has not yet been reported in
     * the notification, but it is indeed reported here, so the two values don't
     * always follow each other.
     */
    vtss_appl_cfm_mep_defect_t highest_defect;

    /**
     * A MEP can detect and report a number of defects, and multiple defects
     * can be present at the same time.
     *
     * This is a mask of defects, where:
     * +-----------------------------+
     * | Bit # | Description         |
     * |-------|---------------------|
     * |     0 | someRDIdefect       |
     * |     1 | someMACstatusDefect |
     * |     2 | someRMEPCCMdefect   |
     * |     3 | errorCCMdefect      |
     * |     4 | xconCCMdefect       |
     * +-----------------------------+
     *
     * You may use the VTSS_APPL_CFM_MEP_DEFECT_MASK_xxx macros to look up specific
     * defects.
     *
     * Called dot1agCfmMepDefects in CFM MIB.
     * See:
     *   12.14.7.1.3:o and 20.35.7 (someRDIdefect)
     *   12.14.7.1.3.p and 20.35.6 (someMACstatusDefect)
     *   12.14.7.1.3.q and 20.35.5 (someRMEPCCMdefect)
     *   12.14.7.1.3.r and 20.21.3 (errorCCMdefect)
     *   12.14.7.1.3.s and 20.23.3 (xconCCMdefect)
     */
    uint8_t defects;

    /**
     * Indicates whether this MEP sends CCMs with the RDI bit set.
     * It is not part of the standard to publish, because it can be determined
     * from the defects, like this:
     *   present_rdi (presentRDI in 802.1Q) is true if and only if one or more
     *   of the variables
     *     - someRMEPCCMdefect
     *     - someMACstatusDefect
     *     - errorCCMdefect
     *     - xconCCMdefect
     *   is true, and if the corresponding priority of that variable is greater
     *   than or equal to the value of the lowestAlarmPri (see
     *   vtss_appl_cfm_mep_conf_t::alarm_level).
     *
     * It is documented in clause 20.9.6 of 802.1Q-2018.
     */
    mesa_bool_t present_rdi;

    /**
     * Total number of CCMs that hit this MEP and passed the validation test
     * (20.51.4.2 and 20.51.4.3).
     *
     * Not part of the standard.
     */
    uint64_t ccm_rx_valid_cnt;

    /**
     * Total number of CCMs that hit this MEP and didn't pass the validation
     * test (20.51.4.2 and 20.51.4.3).
     *
     * Not part of the standard.
     */
    uint64_t ccm_rx_invalid_cnt;

    /**
     * Total number of out-of-sequence errors seen from RMEPs.
     *
     * Called dot1agCfmMepCcmSequenceErrors in the CFM MIB.
     * See 12.14.7.1.3:v and 20.16.12, CCMsequenceErrors.
     */
    uint64_t ccm_rx_sequence_error_cnt;

    /**
     * Total number of CCM PDUs transmitted by this MEP.
     *
     * Called dot1agCfmMepCciSentCcms in the CFM MIB.
     * See 12.14.7.1.3:w and 20.10.2, CCIsentCCMs.
     */
    uint64_t ccm_tx_cnt;

    /**
     * This structure holds information as to whether the MEP can be created
     * given the current switch configuration and whether CCM PDUs transmitted
     * by a remote MEP can enter the MEP's residence port.
     */
    vtss_appl_cfm_mep_errors_t errors;
} vtss_appl_cfm_mep_status_t;

/**
 * Get the status of an existing MEP.
 *
 * \param key    [IN]  Key identifying the MEP
 * \param status [OUT] Pointer to structure receiving MEP's status
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_mep_status_get(const vtss_appl_cfm_mep_key_t &key, vtss_appl_cfm_mep_status_t *status);

/**
 * Clear the counters of an existing MEP.
 *
 * This command clears the following counters of a MEP:
 *   vtss_appl_cfm_mep_status_t::ccm_rx_valid_cnt
 *   vtss_appl_cfm_mep_status_t::ccm_rx_invalid_cnt
 *   vtss_appl_cfm_mep_status_t::ccm_rx_sequence_error_cnt
 *   vtss_appl_cfm_mep_status_t::ccm_tx_cnt
 *
 * \param key    [IN]  Key identifying the MEP
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_mep_statistics_clear(const vtss_appl_cfm_mep_key_t &key);

/**
 * Notification Status of a given MEP.
 */
typedef struct {
    /**
     * Fault alarm notification.
     * If a defect with a priority higher than the configured alarm level (see
     * vtss_appl_cfm_mep_conf_t::alarm_level) is detected, this object will be
     * set and generate a notification after the time indicated in
     * vtss_appl_cfm_mep_conf_t::alarm_time_present_ms has expired.
     *
     * The alarm will be cleared after no defect higher than the configured
     * alarm level has been absent for
     * vtss_appl_cfm_mep_conf_t::alarm_time_absent_ms.
     */
    vtss_appl_cfm_mep_defect_t highest_defect;

    /**
     * If true, the MEP is up and running normally, that is,
     *   - it's active, that is, administratively enabled and created
     *   - enableRmepDefect is true, that is, we have no local errors.
     *   - there are no defects from remote end.
     *
     * If false, use vtss_appl_cfm_mep_status_get() to figure out what exactly
     * is wrong.
     */
    mesa_bool_t mep_ok;
} vtss_appl_cfm_mep_notification_status_t;

/**
 * Get the notification status of a MEP.
 *
 * \param key          [IN]  Key identifying the MEP
 * \param notif_status [OUT] Pointer to structure receiving MEP's notification status.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_mep_notification_status_get(const vtss_appl_cfm_mep_key_t &key, vtss_appl_cfm_mep_notification_status_t *const notif_status);

/**
 * Operational state of a Remote MEP's state machine (20.20).
 * SM stands for State Machine.
 * Corresponds to Dot1agCfmMepDbRMepState in CFM MIB.
 */
typedef enum {
    VTSS_APPL_CFM_RMEP_STATE_IDLE    = 1, /**< RMEP SM is in RMEP_IDLE state (transient) */
    VTSS_APPL_CFM_RMEP_STATE_START   = 2, /**< RMEP SM is in RMEP_START state            */
    VTSS_APPL_CFM_RMEP_STATE_FAILED  = 3, /**< RMEP SM is in RMEP_FAILED state           */
    VTSS_APPL_CFM_RMEP_STATE_OK      = 4, /**< RMEP SM is in RMEP_OK state               */
} vtss_appl_cfm_rmep_state_t;

/**
 * Value of Port Status TLV.
 * Enumeration values match those of 21.5.4, except for 0 and 100, which are
 * our own inventions.
 */
typedef enum {
    VTSS_APPL_CFM_PORT_STATUS_NOT_RECEIVED =   0, /**< Port Status TLV not present in CCM from RMEP (psNoPortStateTLV) */
    VTSS_APPL_CFM_PORT_STATUS_BLOCKED      =   1, /**< Port Status TLV present with value psBlocked                    */
    VTSS_APPL_CFM_PORT_STATUS_UP           =   2, /**< Port Status TLV present with value psUp                         */
} vtss_appl_cfm_port_status_t;

/**
 * Value of Interface Status TLV.
 * Enumeration values match those of 21.5.5, except for 0 and 100, which are
 */
typedef enum {
    VTSS_APPL_CFM_INTERFACE_STATUS_NOT_RECEIVED     =   0, /**< Interface Status TLV not present in CCM from RMEP (isNoInterfaceStatusTLV) */
    VTSS_APPL_CFM_INTERFACE_STATUS_UP               =   1, /**< Interface Status TLV present with value isUp                               */
    VTSS_APPL_CFM_INTERFACE_STATUS_DOWN             =   2, /**< Interface Status TLV present with value isDown                             */
    VTSS_APPL_CFM_INTERFACE_STATUS_TESTING          =   3, /**< Interface Status TLV present with value isTesting                          */
    VTSS_APPL_CFM_INTERFACE_STATUS_UNKNOWN          =   4, /**< Interface Status TLV present with value isUnknown                          */
    VTSS_APPL_CFM_INTERFACE_STATUS_DORMANT          =   5, /**< Interface Status TLV present with value isDormant                          */
    VTSS_APPL_CFM_INTERFACE_STATUS_NOT_PRESENT      =   6, /**< Interface Status TLV present with value isNotPresent                       */
    VTSS_APPL_CFM_INTERFACE_STATUS_LOWER_LAYER_DOWN =   7, /**< Interface Status TLV present with value isLowerLayerDown                   */
} vtss_appl_cfm_interface_status_t;

/**
 * Enumeration of allowed Chassis ID subtypes.
 * They match those of LLDP-MIB 2005 for the LldpChassisIdSubtype textual
 * convention.
 */
typedef enum {
    VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_NOT_RECEIVED      =   0, /**< No Sender ID TLV with chassis ID is received in CCM from RMEP */
    VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_CHASSIS_COMPONENT =   1, /**< Sender ID TLV present with value chassisComponent             */
    VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_INTERFACE_ALIAS   =   2, /**< Sender ID TLV present with value interfaceAlias               */
    VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_PORT_COMPONENT    =   3, /**< Sender ID TLV present with value portComponent                */
    VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_MAC_ADDRESS       =   4, /**< Sender ID TLV present with value macAddress                   */
    VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_NETWORK_ADDRESS   =   5, /**< Sender ID TLV present with value networkAddress               */
    VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_INTERFACE_NAME    =   6, /**< Sender ID TLV present with value interfaceName                */
    VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_LOCAL             =   7, /**< Sender ID TLV present with value local                        */
} vtss_appl_cfm_chassis_id_subtype_t;

/**
 * Structure containing the fields of a Sender ID TLV (21.5.3)
 * Used in received CCMs.
 */
typedef struct {
    /**
     * Contains the Sender ID TLV's (21.5.3) Chassis ID Subtype of the last
     * received CCM from this RMEP.
     *
     * If the CCM did not contain a Sender ID TLV or if the TLV's Chassis ID
     * is not present, then this is
     * VTSS_APPL_CFM_CHASSIS_ID_SUBTYPE_NOT_RECEIVED.
     *
     * The currently known subtypes are listed in the enum. If it contains a
     * subtype that is not listed, it is not an error. It must be handled
     * gracefully by the management interface.
     *
     * See 12.14.7.6.3:h and 20.19.5.
     * Called dot1agCfmMepDbChassisIdSubType in CFM MIB.
     *
     * Its syntax is specified in 802.1AB-2005 clause 9.5.2.2 and in
     * LLDP-MIB.txt's LldpChassisIdSubtype textual convention.
     */
    vtss_appl_cfm_chassis_id_subtype_t chassis_id_subtype;

    /**
     * Length of the #chassis_id field.
     * If 0, either the sender ID TLV is not present in the last CCM received
     * from this RMEP or the Chassis ID is not present in the TLV.
     */
    uint8_t chassis_id_len;

    /**
     * Contains the Sender ID TLV's Chassis ID.
     * Usually, this is a NULL-terminated string, but its exact length is given
     * by #chassis_id_len.
     *
     * See 12.14.7.6.3:h and 20.19.5.
     * Called dot1agCfmMepDbChassisId in CFM MIB.
     *
     * Its syntax is specified in 802.1AB-2005 clause 9.5.2.2 and in
     * LLDP-MIB.txt's LldpChassisId textual convention.
     */
    char chassis_id[256];

    /**
     * Length of the #mgmt_addr_domain field.
     * If 0, either the sender ID TLV is not present in the last CCM received
     * from this RMEP or the Management Address Domain field not present in the
     * TLV.
     * If > sizeof(mgmt_addr_domain), the contents was truncated to
     * sizeof(mgmt_addr_domain).
     */
    uint8_t mgmt_addr_domain_len;

    /**
     * Contains the Sender ID TLV's Management Address Domain from the last CCM
     * received from this RMEP.
     *
     * This is binary data of length #mgmt_addr_domain_len.
     *
     * The contents is to be interpreted as specified in ITU-T X.690 clause
     * 8.19, that is, as a BER-encoded OID, where:
     *    mgmt_addr_domain[0] must be 0x06 (indicating an Absolute OID).
     *    mgmt_addr_domain[1] contains the number of bytes coming after.
     *
     * To decode it to an OID, try out the following page:
     *     https://misc.daniel-marschall.de/asn.1/oid-converter/online.php
     * with Decode Hex-Notation chosen.
     *
     * See also https://en.wikipedia.org/wiki/X.690
     *
     * This software does not attempt to check the validity of the contents of
     * the management address domain.
     *
     * Example:
     *   snmpUDPDomain:
     *     OID = 1.3.6.1.6.1.1
     *     BER = 06 06 2B 06 01 06 01 01
     *     With this domain, the mgmt_addr is encoded as snmpUDPDomain (see
     *     SNMPv2-TM MIB) like: Byte 1-4 = IP address, Byte 5-6 = UDP-port.
     *
     * See 12.14.7.6.3:h and 20.19.5.
     * Called dot1agCfmMepDbManAddressDomain in the CFM MIB and is of type
     * TDomain.
     * TDomain is from the SNMPv2-TC MIB and is of OBJECT IDENTIFIER syntax.
     *
     * Possibly values are listed in the SNMPv2-TM MIB.
     */
    uint8_t mgmt_addr_domain[64];

    /**
     * Length of the #mgmt_addr field.
     * If 0, either the sender ID TLV is not present in the last CCM received
     * from this RMEP or the Management Address field not present in the TLV.
     */
    uint8_t mgmt_addr_len;

    /**
     * Contains the Sender ID TLV's Management Address.
     *
     * The number of bytes received from the CCM PDU's Sender ID TLV into this
     * field is given by #mgmt_addr_len.
     *
     * The standard says, that the number of valid bytes in this field
     * implicitly is given by the OID encoded in #mgmt_addr_domain. However, the
     * software does not attempt to do any validity check on this field.
     *
     * If for instance the BER-encoded OID in #mgmt_addr_domain represents
     * the snmpUDPDomain domain, the value in #mgmt_addr is an SnmpUDPAddress,
     * whose textual convention can be found in the SNMPv2-TM MIB:
     *   Length is 6, where first 4 bytes are the IPv4 address in network-order
     *   and the last 2 bytes are the UDP port in network-order.
     *
     * Called dot1agCfmMepDbManAddress in the CFM MIB and is of type TAddress.
     * TAddress if from the SNMPv2-TC MIB and is of type OCTET STRING.
     * The contents of a TAddress is defined in the SNMPv2-TM MIB (e.g.
     * snmpUDPAddress).
     */
    uint8_t mgmt_addr[256];
} vtss_appl_cfm_sender_id_t;

/**
 * Status of a given RMEP.
 * In the CFM MIB, the table is called dot1agCfmMepDbTable and an entry is
 * called Dot1agCfmMepDbEntry.
 */
typedef struct {
    /**
     * Indication of the operational state of this RMEP's Remote MEP state
     * machine (20.20).
     *
     * See 12.14.7.6.3:b and 20.20.
     * Called dot1agCfmMepDbRMepState in CFM MIB.
     */
    vtss_appl_cfm_rmep_state_t state;

    /**
     * The time in seconds (Sysuptime) at which the RMEP last entered
     * RMEP_FAILED or RMEP_OK state.
     * 0 if it hasn't yet entered any of these states.
     *
     * See also 12.14.7.6.3:c.
     * Called dot1agCfmMepDbRMepFailedOkTime in CFM MIB.
     */
    uint64_t failed_ok_time;

    /**
     * Source MAC address of last received CCM from this RMEP or all-zeros if
     * no CCM was received.
     *
     * See 12.14.7.6.3:d and 20.19.7, rMEPmacAddress.
     * Called dot1agCfmMepDbMacAddress in CFM MIB.
     */
    mesa_mac_t smac;

    /**
     * RDI bit contained in last CCM received from this RMEP.
     *
     * See 12.14.7.6.3:e and 20.19.2, rMEPlastRDI.
     * Called dot1agCfmMepDbRdi in CFM MIB.
     */
    mesa_bool_t rdi;

    /**
     * Contents of the Port Status TLV (21.5.4) of the last received CCM from
     * this RMEP.
     *
     * The possible values of this one are encoded with enums. If it contains an
     * unlisted enum, the frame will be discarded in accordance with the spec
     * 20.51.4.3 and 21.5.4).
     *
     * Valid values are from Table 21-9 in clause 21.5.4.
     *
     * If the last received CCM did not contain a Port Status TLV,
     * #port_status will be set to VTSS_APPL_CFM_PORT_STATUS_NOT_RECEIVED.
     *
     * See 12.14.7.6.3:f and 20.19.3, rMEPlastPortState.
     * Called dot1agCfmMepDbPortStatusTlv in CFM MIB.
     */
    vtss_appl_cfm_port_status_t port_status;

    /**
     * Contents of the Interface Status TLV (21.5.5) of the last received CCM
     * from this RMEP.
     *
     * The possible values of this one are encoded with enums. If it contains an
     * unlisted enum, the frame will be discarded in accordance with the spec
     * (20.51.4.3 and 21.5.5).
     *
     * Valid values are from Table 21-11 in clause 21.5.5.
     *
     * If the last received CCM did not contain an Interface Status TLV,
     * #interface_status will be set to
     * VTSS_APPL_CFM_INTERFACE_STATUS_NOT_RECEIVED.
     *
     * See 12.14.7.6.3:g and 20.19.4, rMEPlastInterfaceStatus.
     * Called dot1agCfmMepDbInterfaceStatusTlv in CFM MIB.
     */
    vtss_appl_cfm_interface_status_t interface_status;

    /**
     * See contents of vtss_appl_cfm_sender_id_t for a description.
     */
    vtss_appl_cfm_sender_id_t sender_id;

    /**
     * If the last received CCM from this RMEP contains an Organization-Specific
     * TLV (21.5.2), this field is true, and #organization_specific_tlv contains
     * valid data.
     *
     * If the last received CCM from this RMEP did not contain an Organization-
     * specific TLV, this field is false, and #organization_specific_tlv
     * does not contain valid data.
     */
    mesa_bool_t organization_specific_tlv_present;

    /**
     * See contents of vtss_appl_cfm_organization_specific_tlv_t for a
     * description.
     */
    vtss_appl_cfm_organization_specific_tlv_t organization_specific_tlv;
} vtss_appl_cfm_rmep_status_t;

/**
 * Get the status of an existing RMEP.
 *
 * \param key    [IN]  Key identifying the Remote MEP
 * \param status [OUT] Pointer to structure receiving RMEP's status.
 * \return Error code.
 */
mesa_rc vtss_appl_cfm_rmep_status_get(const vtss_appl_cfm_rmep_key_t &key, vtss_appl_cfm_rmep_status_t *status);

#endif /* _VTSS_APPL_CFM_HXX_ */

