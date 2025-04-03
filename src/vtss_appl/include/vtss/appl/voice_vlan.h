/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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
* \brief Public Voice VLAN API
* \details This header file describes Voice VLAN control functions and types.
* The Voice VLAN feature enables voice traffic forwarding on the Voice VLAN, and the
* switch can then classify and schedule network traffic.
* Voice VLAN detects configured telephony OUI (Organizationally Unique Identifier)
* sources or LLDP notifications on a specific port and then join this port as one of
* the Voice VLAN members.
* The Voice VLAN global configuration manages system wide Voice VLAN function.
* The Voice VLAN port interface configuration enables per port Voice VLAN function
* when Voice VLAN is also globally enabled.
* The Voice VLAN telephony OUI entry configuration describes the telephony OUI address
* prefix to be checked by Voice VLAN.
*/

#ifndef _VTSS_APPL_VOICE_VLAN_H_
#define _VTSS_APPL_VOICE_VLAN_H_

#include <vtss/appl/types.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/interface.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN         3                                           /**< Telephony OUI prefix length used in Voice VLAN setting. */
#define VTSS_APPL_VOICE_VLAN_STRING_MAX_LEN         32                                          /**< Maximum number of characters used for Voice VLAN string. */
#define VTSS_APPL_VOICE_VLAN_MAX_DESCRIPTION_LEN    (VTSS_APPL_VOICE_VLAN_STRING_MAX_LEN + 1)   /**< Maximum string length for OUI description. */

/*! \brief Per port Voice VLAN function administrative type. */
typedef enum {
    VTSS_APPL_VOICE_VLAN_MANAGEMENT_DISABLE = 0,    /**< Per port Voice VLAN function is disabled. */
    VTSS_APPL_VOICE_VLAN_MANAGEMENT_FORCE_MEMBER,   /**< Per port Voice VLAN function is always working by management. */
    VTSS_APPL_VOICE_VLAN_MANAGEMENT_AUTOMATIC       /**< Per port Voice VLAN function is working based on either telephony OUI setting or LLDP discovery. */
} vtss_appl_voice_vlan_management_t;

/*! \brief Per port Voice VLAN discovery protocol. */
typedef enum {
    VTSS_APPL_VOICE_VLAN_DISCOVER_BY_OUI = 0,       /**< Per port Voice VLAN discovery protocol relies on telephony OUI detection. */
    VTSS_APPL_VOICE_VLAN_DISCOVER_BY_LLDP,          /**< Per port Voice VLAN discovery protocol relies on LLDP notification. */
    VTSS_APPL_VOICE_VLAN_DISCOVER_BY_OUI_OR_LLDP    /**< Per port Voice VLAN discovery protocol relies on either telephony OUI detection or LLDP notification. */
} vtss_appl_voice_vlan_discovery_t;

/** \brief Collection of capability properties of the Voice VLAN module. */
typedef struct {
    /** The maximum number of telephony OUI entry registration. */
    uint32_t     max_oui_registration_entry_count;
    /** The maximum allowed CoS (Class of Service) value to be used in forwarding Voice VLAN traffic. */
    uint32_t     max_forwarding_traffic_class;
    /** The maximum time value in second for aging telephony OUI sources in Voice VLAN. */
    uint32_t     max_oui_learning_aging_time;
    /** The minimum time value in second for aging telephony OUI sources in Voice VLAN. */
    uint32_t     min_oui_learning_aging_time;
    /** The capability to support voice device discovery from LLDP notification. */
    mesa_bool_t    support_lldp_discovery_notification;
} vtss_appl_voice_vlan_capabilities_t;

/**
 * \brief The Voice VLAN global configuration.
 * This configuration is the global setting that can manage the Voice VLAN functions.
 */
typedef struct {
    /**
     * \brief Administrative control for system wide Voice VLAN function, TRUE is to
     * enable the Voice VLAN function and FALSE is to disable it.
     * When Voice VLAN is globally enabled, system classifies and schedules voice traffic
     * forwarding but blocks other kinds of network traffic on Voice VLAN member ports.
     * When Voice VLAN is globally disabled, system follows general frame forwarding rule
     * for all kinds of network traffic on ports.
     */
    mesa_bool_t                                admin_state;
    /**
     * \brief VLAN ID, which should be unique in the system, for Voice VLAN.
     */
    mesa_vid_t                          voice_vlan_id;
    /**
     * \brief Traffic class value used in frame CoS queuing insides Voice VLAN.
     * All kinds of traffic on Voice VLAN apply this traffic class.
     */
    mesa_prio_t                         traffic_class;
    /**
     * \brief MAC address aging time (T) for telephony OUI source registrated by Voice VLAN.
     * The actual timing in purging the specific entry ranges from T to 2T.
     */
    uint32_t                                 aging_time;
} vtss_appl_voice_vlan_global_conf_t;

/**
 * \brief The Voice VLAN port interface configuration.
 * This configuration is the per port settings that can manage the Voice VLAN functions.
 */
typedef struct {
    /**
     * \brief Management mode of the specific port in Voice VLAN.
     * VTSS_APPL_VOICE_VLAN_MANAGEMENT_DISABLE will disjoin the port from Voice VLAN.
     * VTSS_APPL_VOICE_VLAN_MANAGEMENT_FORCE_MEMBER will force the port to join Voice VLAN.
     * VTSS_APPL_VOICE_VLAN_MANAGEMENT_AUTOMATIC will join the port in Voice VLAN upon
     * detecting attached VoIP devices by using protocol parameter.
     */
    vtss_appl_voice_vlan_management_t   management;
    /**
     * \brief Specify the protocol for detecting attached VoIP devices.
     * It only works when VTSS_APPL_VOICE_VLAN_MANAGEMENT_AUTOMATIC is set in management parameter.
     * Voice VLAN will restart automatic detecting process upon changing the protocol.
     * When VTSS_APPL_VOICE_VLAN_DISCOVER_BY_OUI is given, Voice VLAN performs VoIP device detection
     * based on checking telephony OUI settings via new MAC address notification.
     * When VTSS_APPL_VOICE_VLAN_DISCOVER_BY_LLDP is given, Voice VLAN performs VoIP device detection
     * based on LLDP notifications.
     * When VTSS_APPL_VOICE_VLAN_DISCOVER_BY_OUI_OR_LLDP is given, Voice VLAN performs VoIP device
     * detection based on either new MAC address notification or LLDP notifications.  In addition,
     * the first come notification will be first served.
     */
    vtss_appl_voice_vlan_discovery_t    protocol;
    /**
     * \brief Manage the security control of this port interface in Voice VLAN.
     * When it is disabled, all the traffic in Voice VLAN will be permit.
     * When it is enabled, all non-telephonic MAC addresses in the Voice VLAN will be blocked for 10
     * seconds and thus the traffic from these senders will be deny.
     */
    mesa_bool_t                                secured;
} vtss_appl_voice_vlan_port_conf_t;

/**
 * \brief The Voice VLAN telephony OUI entry index type.
 * Address prefix of the telephony OUI.
 */
typedef struct {
    /**
     * \brief A leading 3 bytes index used to denote whether specific MAC address is presenting a voice device.
     */
    uint8_t                                  prefix[VTSS_APPL_VOICE_VLAN_OUI_PREFIX_LEN];
} vtss_appl_voice_vlan_oui_index_t;

/**
 * \brief The Voice VLAN telephony OUI entry configuration.
 * This configuration is the telephony OUI settings that will be used for Voice VLAN functions.
 */
typedef struct {
    /**
     * \brief The description for the specific telephony OUI.
     */
    char                                description[VTSS_APPL_VOICE_VLAN_MAX_DESCRIPTION_LEN];
} vtss_appl_voice_vlan_telephony_oui_conf_t;

/**
 * \brief Get the capabilities of Voice VLAN.
 *
 * \param cap       [OUT]   The capability properties of the Voice VLAN module.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_capabilities_get(vtss_appl_voice_vlan_capabilities_t *const cap);

/**
 * \brief Get Voice VLAN global default configuration.
 *
 * Get default configuration of the Voice VLAN global setting.
 *
 * \param entry     [OUT]   The default configuration of the Voice VLAN global setting.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_global_config_default(
    vtss_appl_voice_vlan_global_conf_t              *const entry
);

/**
 * \brief Get Voice VLAN global configuration.
 *
 * Get configuration of the Voice VLAN global setting.
 *
 * \param entry     [OUT]   The current configuration of the Voice VLAN global setting.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_global_config_get(
    vtss_appl_voice_vlan_global_conf_t              *const entry
);

/**
 * \brief Set/Update Voice VLAN global configuration.
 *
 * Modify configuration of the Voice VLAN global setting.
 *
 * \param entry     [IN]    The revised configuration of the Voice VLAN global setting.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_global_config_set(
    const vtss_appl_voice_vlan_global_conf_t        *const entry
);

/**
 * \brief Iterator for retrieving Voice VLAN port configuration table index.
 *
 * Retrieve the 'next' configuration index of the Voice VLAN port configuration table
 * according to the given 'prev'.
 *
 * \param prev      [IN]    Porinter of port interface index to be used for index determination.
 *
 * \param next      [OUT]   The next index should be used for the table entry.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Error code.      VTSS_RC_OK for success operation and the value in 'next' is valid,
 *                          other error code means that no "next" index and its corresponding
 *                          entry exists, and the end has been reached.
 */
mesa_rc vtss_appl_voice_vlan_port_itr(
    const vtss_ifindex_t                            *const prev,
    vtss_ifindex_t                                  *const next
);

/**
 * \brief Get Voice VLAN port interface default configuration.
 *
 * Get default configuration of the Voice VLAN port interface.
 *
 * \param ifindex   [OUT]   The logical interface index of Voice VLAN port to be used.
 * \param entry     [OUT]   The default configuration of the Voice VLAN port interface.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_port_config_default(
    vtss_ifindex_t                                  *const ifindex,
    vtss_appl_voice_vlan_port_conf_t                *const entry
);

/**
 * \brief Get Voice VLAN specific port interface configuration.
 *
 * Get configuration of the specific Voice VLAN port.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of Voice VLAN port.
 *
 * \param entry     [OUT]   The current configuration of the specific Voice VLAN port.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_port_config_get(
    const vtss_ifindex_t                            *const ifindex,
    vtss_appl_voice_vlan_port_conf_t                *const entry
);

/**
 * \brief Set/Update Voice VLAN specific port interface configuration.
 *
 * Modify configuration of the specific Voice VLAN port.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of Voice VLAN port.
 * \param entry     [IN]    The revised configuration of the specific Voice VLAN port.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_port_config_set(
    const vtss_ifindex_t                            *const ifindex,
    const vtss_appl_voice_vlan_port_conf_t          *const entry
);

/**
 * \brief Iterator for retrieving Voice VLAN telephony OUI configuration table index.
 *
 * Retrieve the 'next' configuration index of the Voice VLAN telephony OUI table
 * according to the given 'prev'.
 *
 * \param prev      [IN]    Porinter of OUI configuration index to be used for index determination.
 *
 * \param next      [OUT]   The next index should be used for the table entry.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Error code.      VTSS_RC_OK for success operation and the value in 'next' is valid,
 *                          other error code means that no "next" index and its corresponding
 *                          entry exists, and the end has been reached.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_itr(
    const vtss_appl_voice_vlan_oui_index_t          *const prev,
    vtss_appl_voice_vlan_oui_index_t                *const next
);

/**
 * \brief Get Voice VLAN telephony OUI entry's default configuration.
 *
 * Get default configuration of the Voice VLAN telephony OUI entry.
 *
 * \param ouiindex  [OUT]   Telephony OUI prefix to be used in Voice VLAN.
 * \param entry     [OUT]   The default configuration of the Voice VLAN telephony OUI entry.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_config_default(
    vtss_appl_voice_vlan_oui_index_t                *const ouiindex,
    vtss_appl_voice_vlan_telephony_oui_conf_t       *const entry
);

/**
 * \brief Get Voice VLAN specific telephony OUI entry's configuration.
 *
 * Get configuration of the Voice VLAN telephony OUI entry.
 *
 * \param ouiindex  [IN]    (key) OUI index - Telephony OUI prefix index used in Voice VLAN.
 *
 * \param entry     [OUT]   The current configuration of the specific Voice VLAN telephony OUI entry.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_config_get(
    const vtss_appl_voice_vlan_oui_index_t          *const ouiindex,
    vtss_appl_voice_vlan_telephony_oui_conf_t       *const entry
);

/**
 * \brief Add Voice VLAN specific telephony OUI entry's configuration.
 *
 * Create configuration of the Voice VLAN telephony OUI entry.
 *
 * \param ouiindex  [IN]    (key) OUI index - Telephony OUI prefix index used in Voice VLAN.
 * \param entry     [IN]    The new configuration of the specific Voice VLAN telephony OUI entry.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_config_add(
    const vtss_appl_voice_vlan_oui_index_t          *const ouiindex,
    const vtss_appl_voice_vlan_telephony_oui_conf_t *const entry
);

/**
 * \brief Set/Update Voice VLAN specific telephony OUI entry's configuration.
 *
 * Modify configuration of the Voice VLAN telephony OUI entry.
 *
 * \param ouiindex  [IN]    (key) OUI index - Telephony OUI prefix index used in Voice VLAN.
 * \param entry     [IN]    The revised configuration of the specific Voice VLAN telephony OUI entry.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_config_set(
    const vtss_appl_voice_vlan_oui_index_t          *const ouiindex,
    const vtss_appl_voice_vlan_telephony_oui_conf_t *const entry
);

/**
 * \brief Delete Voice VLAN specific telephony OUI entry's configuration.
 *
 * Remove configuration of the Voice VLAN telephony OUI entry.
 *
 * \param ouiindex  [IN]    (key) OUI index - Telephony OUI prefix index used in Voice VLAN.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_voice_vlan_telephony_oui_config_del(
    const vtss_appl_voice_vlan_oui_index_t          *const ouiindex
);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_VOICE_VLAN_H_ */
