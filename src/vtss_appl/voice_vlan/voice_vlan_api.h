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

#ifndef _VTSS_VOICE_VLAN_API_H_
#define _VTSS_VOICE_VLAN_API_H_

#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_t, etc. */

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \file voice_vlan_api.h
 * \brief This file defines the APIs for the Voice VLAN module
 */

#define VOICE_VLAN_CLASS_SUPPORTED

/**
 * Voice VLAN management enabled/disabled
 */
#define VOICE_VLAN_MGMT_ENABLED         (1)       /**< Enable option  */
#define VOICE_VLAN_MGMT_DISABLED        (0)       /**< Disable option */

/**
 * Voice VLAN secure learning age time
 */
#define VOICE_VLAN_MIN_AGE_TIME         (10)        /**< Minimum allowed aging period */
#define VOICE_VLAN_MAX_AGE_TIME         (10000000)  /**< Maximum allowed aging period */

/**
 * Voice VLAN traffic class
 */
#define VOICE_VLAN_MAX_TRAFFIC_CLASS    (7)

/**
 * \brief Voice VLAN port mode
 */
enum {
    VOICE_VLAN_PORT_MODE_DISABLED,  /**< Disjoin from Voice VLAN                                                                                                                           */
    VOICE_VLAN_PORT_MODE_AUTO,      /**< Enable auto detect mode. It detects whether there is VoIP phone attached on the specific port and configure the Voice VLAN members automatically. */
    VOICE_VLAN_PORT_MODE_FORCED     /**< Forced join to Voice VLAN.                                                                                                                        */
};

/**
 * \brief Voice VLAN discovery protocol
 */
enum {
    VOICE_VLAN_DISCOVERY_PROTOCOL_OUI,  /**< Detect telephony device by OUI address          */
    VOICE_VLAN_DISCOVERY_PROTOCOL_LLDP, /**< Detect telephony device by LLDP                 */
    VOICE_VLAN_DISCOVERY_PROTOCOL_BOTH  /**< Detect telephony device by OUI address and LLDP */
};

/**
 * Voice VLAN OUI check conflict configuration
 */
#define VOICE_VLAN_CHECK_CONFLICT_CONF      VOICE_VLAN_MGMT_ENABLED

/**
 * Voice VLAN OUI maximum entries counter
 */
#define VOICE_VLAN_OUI_ENTRIES_CNT      (16)  /**< Maximum allowed OUI entry number */

/**
 * Voice VLAN OUI description maximum length
 */
#define VOICE_VLAN_MAX_DESCRIPTION_LEN  (32)  /**< Maximum allowed OUI description string length */

/**
 * Default Voice VLAN configuration
 */
#define VOICE_VLAN_MGMT_DEFAULT_MODE                VOICE_VLAN_MGMT_DISABLED            /**< Default global mode        */
#define VOICE_VLAN_MGMT_DEFAULT_VID                 (1000)                              /**< Default Voice VID          */
#define VOICE_VLAN_MGMT_DEFAULT_AGE_TIME            (86400)                             /**< Default age time           */
#define VOICE_VLAN_MGMT_DEFAULT_HOLD_TIME           PSEC_HOLD_TIME_MIN                  /**< Default hold time          */
#define VOICE_VLAN_MGMT_DEFAULT_TRAFFIC_CLASS       VOICE_VLAN_MAX_TRAFFIC_CLASS        /**< Default traffic class      */

#define VOICE_VLAN_MGMT_DEFAULT_PORT_MODE           VOICE_VLAN_PORT_MODE_DISABLED       /**< Default port mode          */
#define VOICE_VLAN_MGMT_DEFAULT_SECURITY            VOICE_VLAN_MGMT_DISABLED            /**< Default security mode      */
#define VOICE_VLAN_MGMT_DEFAULT_DISCOVERY_PROTOCOL  VOICE_VLAN_DISCOVERY_PROTOCOL_OUI   /**< Default discovery protocol */


/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH = MODULE_ERROR_START(VTSS_MODULE_ID_VOICE_VLAN), /**< Operation is only allowed on the primary switch.                 */
    VOICE_VLAN_ERROR_ISID,                                                                   /**< isid parameter is invalid.                                       */
    VOICE_VLAN_ERROR_ISID_NON_EXISTING,                                                      /**< isid parameter is non-existing.                                  */
    VOICE_VLAN_ERROR_INV_PARAM,                                                              /**< Invalid parameter.                                               */
    VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_MGMT_VID,                                          /**< Voice VID is conflict with managed VID.                          */
    VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_MVR_VID,                                           /**< Voice VID is conflict with MVR VID.                              */
    VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_STATIC_VID,                                        /**< Voice VID is conflict with static VID.                           */
    VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_PVID,                                              /**< Voice VID is conflict with PVID.                                 */
    VOICE_VLAN_ERROR_IS_CONFLICT_WITH_LLDP,                                                  /**< Configure auto detect mode by LLDP but LLDP feature is disabled. */
    VOICE_VLAN_ERROR_PARM_NULL_OUI_ADDR,                                                     /**< parameter error of null OUI address.                             */
    VOICE_VLAN_ERROR_REACH_MAX_OUI_ENTRY,                                                    /**< OUI table reach max entries.                                     */
    VOICE_VLAN_ERROR_ENTRY_NOT_EXIST,                                                        /**< Table entry not exist.                                           */
    VOICE_VLAN_ERROR_ENTRY_ALREADY_EXIST                                                     /**< Table entry already exist.                                       */
};

/**
 * \brief Voice VLAN configuration.
 */
typedef struct {
    BOOL            mode;           /**< Voice VLAN global mode setting.                                                         */
    mesa_vid_t      vid;            /**< Voice VLAN ID. It should be a unique VLAN ID in the system.                             */
    u32             age_time;       /**< Voice VLAN secure learning age time.                                                    */
    mesa_prio_t     traffic_class;  /**< Voice VLAN traffic class. The switch can classifying and scheduling to network traffic. */
} voice_vlan_conf_t;

/**
 * \brief Voice VLAN port configuration.
 */
typedef struct {
    CapArray<int,  MEBA_CAP_BOARD_PORT_MAP_COUNT> port_mode;              /**< Enable auto detect mode or configure manual.                                                                                          */
    CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT> security;               /**< When security mode is enabled, all non-telephone MAC address in Voice VLAN will be removed.                                           */
    CapArray<int,  MEBA_CAP_BOARD_PORT_MAP_COUNT> discovery_protocol;     /**< Detect telephony device by the discovery protocol.                                                                                    */
} voice_vlan_port_conf_t;

inline int vtss_memcmp(const voice_vlan_port_conf_t &a, const voice_vlan_port_conf_t &b)
{
    VTSS_MEMCMP_ELEMENT_CAP(a, b, port_mode);
    VTSS_MEMCMP_ELEMENT_CAP(a, b, security);
    VTSS_MEMCMP_ELEMENT_CAP(a, b, discovery_protocol);

    return 0;
}


/**
 * \brief Voice VLAN OUI entry.
 */
typedef struct {
    BOOL    valid;                                              /**< Internal state.                                                                     */
    u8      oui_addr[3];                                        /**< An OUI address is a globally unique identifier assigned to a vendor by IEEE.        */
    char    description[VOICE_VLAN_MAX_DESCRIPTION_LEN + 1];    /**< The description of OUI address. Normaly, it descript which vendor telephony device. */
} voice_vlan_oui_entry_t;

/**
 * \brief Voice VLAN LLDP telephony MAC entry.
 */
typedef struct {
    BOOL            valid;      /**< Internal state.     */
    vtss_isid_t     isid;       /**< Internal switch ID. */
    mesa_port_no_t  port_no;    /**< Port number.        */
    u8              mac[6];     /**< MAC address.        */
} voice_vlan_lldp_telephony_mac_entry_t;


/**
  * \brief Retrieve an error string based on a return code
  *        from one of the Voice VLAN API functions.
  *
  * \param rc [IN]: Error code that must be in the VOICE_VLAN_ERROR_xxx range.
  */
const char *voice_vlan_error_txt(mesa_rc rc);

/**
  * \brief Get the global Voice VLAN configuration.
  *
  * \param glbl_cfg [OUT]: Pointer to structure that receives
  *                        the current configuration.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VOICE_VLAN_ERROR_INV_PARAM if glbl_cfg is NULL.\n
  *    VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc voice_vlan_mgmt_conf_get(voice_vlan_conf_t *glbl_cfg);

/**
  * \brief Set the global Voice VLAN configuration.
  *
  * \param glbl_cfg [IN]: Pointer to structure that contains the
  *                       global configuration to apply to the
  *                       voice VLAN module.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VOICE_VLAN_ERROR_INV_PARAM if glbl_cfg is NULL or parameters error.\n
  *    VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VOICE_VLAN_ERROR_IS_CONFLICT_WITH_LLDP if it is a conflict configuration with LLDP.\n
  *    Others value arises from sub-function.\n
  */
mesa_rc voice_vlan_mgmt_conf_set(voice_vlan_conf_t *glbl_cfg);

/**
  * \brief Get a switch's per-port configuration.
  *
  * \param isid        [IN]: The Switch ID for which to retrieve the
  *                          configuration.
  * \param switch_cfg [OUT]: Pointer to structure that receives
  *                          the switch's per-port configuration.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VOICE_VLAN_ERROR_INV_PARAM if switch_cfg is NULL.\n
  *    VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VOICE_VLAN_ERROR_ISID if called with an invalid ISID.\n
  */
mesa_rc voice_vlan_mgmt_port_conf_get(vtss_isid_t isid, voice_vlan_port_conf_t *switch_cfg);

/**
  * \brief Set a switch's per-port configuration.
  *
  * \param isid       [IN]: The switch ID for which to set the configuration.
  * \param switch_cfg [IN]: Pointer to structure that contains
  *                         the switch's per-port configuration to be applied.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VOICE_VLAN_ERROR_INV_PARAM if switch_cfg is NULL or parameters error.\n
  *    VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VOICE_VLAN_ERROR_ISID if called with an invalid ISID.\n
  *    VOICE_VLAN_ERROR_IS_CONFLICT_WITH_LLDP if it is a conflict configuration with LLDP.\n
  */
mesa_rc voice_vlan_mgmt_port_conf_set(vtss_isid_t isid, voice_vlan_port_conf_t *switch_cfg);


//
// Other public Voice VLAN functions.
//

/**
  * \brief Add or set Voice VLAN OUI entry
  *
  * \param entry [IN]: Pointer to structure that contains
  *                    the entry configuration to be applied.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VOICE_VLAN_ERROR_INV_PARAM if entry is NULL.\n
  *    VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VOICE_VLAN_ERROR_PARM_NULL_OUI_ADDR if parameter "oui_addr" is null OUI address.\n
  *    VOICE_VLAN_ERROR_REACH_MAX_OUI_ENTRY if reach maximum entries number.\n
  */
mesa_rc voice_vlan_oui_entry_add(voice_vlan_oui_entry_t *entry);

/**
  * \brief Delete Voice VLAN OUI entry
  *
  * \param entry [IN]: Pointer to structure that contains
  *                    the entry configuration to be applied.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VOICE_VLAN_ERROR_INV_PARAM if entry is NULL.\n
  *    VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VOICE_VLAN_ERROR_ENTRY_NOT_EXIST if delete entry not exist.\n
  *    Others value arises from sub-function.\n
  */
mesa_rc voice_vlan_oui_entry_del(voice_vlan_oui_entry_t *entry);

/**
  * \brief Clear Voice VLAN OUI entry
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  */
mesa_rc voice_vlan_oui_entry_clear(void);

/**
  * \brief Get Voice VLAN OUI entry
  *
  * \param entry [IN]: Pointer to structure that contains
  *                    the entry configuration to be applied.
  *                    The entry key is OUI address.
  *                    Use null OUI address to get first entry.
  * \param next  [IN]: Set 0 to get current entry.
  *                    Set 1 to get next valid entry.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VOICE_VLAN_ERROR_INV_PARAM if entry is NULL.\n
  *    VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VTSS_RC_ERROR if get or getnext operation fail.\n
  */
mesa_rc voice_vlan_oui_entry_get(voice_vlan_oui_entry_t *entry, BOOL next);

/**
  * \brief Initialize the Voice VLAN module
  *
  * \param data [IN]: Initial data point.
  *
  * \return
  *    VTSS_RC_OK.
  */
mesa_rc voice_vlan_init(vtss_init_data_t *data);

/* The API uses for checking conflicted configuration with LLDP module.
 * User cannot set LLDP port mode to disabled or TX only when Voice-VLAN
 * support LLDP discovery protocol. */
/**
  * \brief Get Voice VLAN is supported LLDP discovery protocol
  *
  * \param isid    [IN]: The switch ID for which to set the configuration.
  * \param port_no [IN]: The port number for which to set the configuration.
  *
  * \return
  *    TRUE if supported.\n
  *    FALSE if not supported.\n
  */
BOOL voice_vlan_is_supported_LLDP_discovery(vtss_isid_t isid, mesa_port_no_t port_no);

/**
  * \brief Check Voice VLAN ID is conflict with other configurations
  *
  * \param voice_vid [IN]: The Voice VLAN ID.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_MGMT_VID if Voice VID is conflict with managed VID.\n
  *    VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_MVR_VID if Voice VID is conflict with MVR VID.\n
  *    VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_STATIC_VID if Voice VID is conflict with static VID.\n
  *    VOICE_VLAN_ERROR_VID_IS_CONFLICT_WITH_PVID if Voice VID is conflict with PVID.\n
  */
mesa_rc VOICE_VLAN_is_valid_voice_vid(mesa_vid_t voice_vid);

/**
  * \brief Get Voice VLAN telephony MAC entry (It is only used for debug command)
  *
  * \param entry [IN]: Pointer to structure that contains
  *                    the entry configuration to be applied.
  *                    The entry key is MAC address.
  *                    Use MAC OUI address to get first entry.
  * \param next  [IN]: Set 0 to get current entry.
  *                    Set 1 to get next valid entry.
  *
  * \return
  *    VTSS_RC_OK on success.\n
  *    VOICE_VLAN_ERROR_INV_PARAM if entry is NULL.\n
  *    VOICE_VLAN_ERROR_MUST_BE_PRIMARY_SWITCH if called on a secondary switch.\n
  *    VTSS_RC_ERROR if get or getnext operation fail.\n
  */
mesa_rc voice_vlan_lldp_telephony_mac_entry_get(voice_vlan_lldp_telephony_mac_entry_t *entry, BOOL next);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_VOICE_VLAN_API_H_ */

