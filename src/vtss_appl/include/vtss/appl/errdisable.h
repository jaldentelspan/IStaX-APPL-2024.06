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
 * \brief Public Errdisable API
 * \details This header file describes errdisable control functions and types
 */

#ifndef _VTSS_APPL_ERRDISABLE_H_
#define _VTSS_APPL_ERRDISABLE_H_

#include <vtss/appl/types.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/interface.h>
#include <vtss/basics/enum-descriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * These are clients that may cause a port to be shut down.
 */
typedef enum {
    VTSS_APPL_ERRDISABLE_CLIENT_NONE,              /**< Not a real client          */
    VTSS_APPL_ERRDISABLE_CLIENT_LINK_FLAP,         /**< Link flap errdisable       */
    VTSS_APPL_ERRDISABLE_CLIENT_DHCP_RATE_LIMIT,   /**< DHCP rate limit errdisable */
    VTSS_APPL_ERRDISABLE_CLIENT_PSECURE_VIOLATION, /**< Port Security errdisable   */

    VTSS_APPL_ERRDISABLE_CLIENT_LAST               /** Must come last              */
} vtss_appl_errdisable_client_t;

/**
 * Definition of error return codes returned by the function in case of malfunctioning behavior.
 */
enum {
    VTSS_APPL_ERRDISABLE_ERROR_RECOVERY_TIME_OUT_OF_RANGE = MODULE_ERROR_START(VTSS_MODULE_ID_ERRDISABLE), /**< Recovery time out of range */
};

/**
 * \brief Capabilities of this module.
 */
typedef struct {
    /**
     * Minimum value for recovery timer
     */
    uint32_t recovery_time_secs_min;

    /**
     * Maximum value for recovery timer
     */
    uint32_t recovery_time_secs_max;

    /**
     * Default value for recovery timer
     */
    uint32_t recovery_time_secs_default;
} vtss_appl_errdisable_capabilities_t;

/**
 * \brief Per-client support of various functionality.
 */
typedef struct {
    /**
     * The client identifies itself in management interfaces
     * by this name. Must be NULL-terminated.
     * strlen(name) is 0 if the client is not included in this build.
     */
    char name[33];

    /**
     * TRUE if client uses the
     * vtss_appl_errdisable_client_conf_t::detect field.
     */
    mesa_bool_t detect;

    /**
     * Contains the default value of
     * vtss_appl_errdisable_client_conf_t::detect field for
     * this client.
     */
    mesa_bool_t detect_default;

    /**
     * Contains the default value of
     * vtss_appl_errdisable_client_conf_t::auto_recovery field for
     * this client.
     */
    mesa_bool_t auto_recovery_default;

    /**
     * TRUE if client uses the
     * vtss_appl_errdisable_client_conf_t::flap_cnt and
     * vtss_appl_errdisable_client_conf_t::time_secs fields.
     */
    mesa_bool_t flap_values;

    /**
     * The minimum allowed value of vtss_appl_errdisable_client_conf_t::flap_cnt.
     */
    uint32_t flap_cnt_min;

    /**
     * The maximum allowed value of vtss_appl_errdisable_client_conf_t::flap_cnt.
     */
    uint32_t flap_cnt_max;

    /**
     * The default value of vtss_appl_errdisable_client_conf_t::flap_cnt for this client.
     */
    uint32_t flap_cnt_default;

    /**
     * The minimum allowed value of vtss_appl_errdisable_client_conf_t::time_secs.
     */
    uint32_t time_secs_min;

    /**
     * The maximum allowed value of vtss_appl_errdisable_client_conf_t::time_secs.
     */
    uint32_t time_secs_max;

    /**
     * The default value of vtss_appl_errdisable_client_conf_t::time_secs for this client.
     */
    uint32_t time_secs_default;
} vtss_appl_errdisable_client_capabilities_t;

/**
 * \brief Global errdisable configuration
 */
typedef struct {
    /**
     * Controls the recovery time in seconds.
     */
    uint32_t recovery_time_secs;
} vtss_appl_errdisable_global_conf_t;

/**
 * \brief Per-client errdisable configuration.
 */
typedef struct {
    /**
     * TRUE when this client should detect violations. FALSE otherwise.
     * Notice that not all clients use this flag. Some may have its own
     * enabled configuration controlled by other means (e.g. psecure-violation).
     */
    mesa_bool_t detect;

    /**
     * TRUE when auto-recovery is enabled for this client. FALSE otherwise.
     */
    mesa_bool_t auto_recovery;

    /**
     * If client supports it, this will control the number of flaps within
     * a given time window that is required to trigger a link shutdown.
     * See time_secs for the size of the time window.
     */
    uint32_t flap_cnt;

    /**
     * The number of seconds to look for flaps. Only used if the given client
     * supports it.
     */
    uint32_t time_secs;
} vtss_appl_errdisable_client_conf_t;

/**
 * \brief errdisable notification status per interface.
 *
 * See vtss_appl_errdisable_interface_status_t for
 * other interface status. This table is separated
 * out of vtss_appl_errdisable_interface_status_t
 * because changes to it may give rise to a notification.
 * Had the recovery time left been member of the notification
 * status, a notification would be sent every one second.
 */
typedef struct {
    /**
     * TRUE if the interface is shut down, FALSE otherwise.
     */
    mesa_bool_t shut_down;

    /**
     * Holds the client that wants this interface to be brought down.
     * Only valid if \p shut_down is TRUE.
     */
    vtss_appl_errdisable_client_t client;
} vtss_appl_errdisable_interface_notification_status_t;

/**
 * \brief errdisable status per interface.
 *
 * Changes to this structure does not give rise to
 * notifications even though it holds a notif_status
 * member, which could indicate this. The reason for replicating
 * this member in this structure is that management interfaces
 * typically would like to know both whether an interface
 * is shut-down, and if so, how many seconds is left of the recovery
 * time and the client who has asked it to be shut-down.
 */
typedef struct {
    /**
     * Holds the number of seconds left before this interface
     * is taken out of its shut-down state. 0 if recovery is disabled
     * or the interface is not shut down.
     */
    uint32_t recovery_time_left_secs;

    /**
     * This is a replica of the notification status.
     */
    vtss_appl_errdisable_interface_notification_status_t notif_status;
} vtss_appl_errdisable_interface_status_t;

/**
 * \brief Get the capabilities of errdisable.
 *
 * \param cap [OUT] Points to location to store the capabilities.
 *
 * \return VTSS_RC_OK if conf contains valid data, else error code
 */
mesa_rc vtss_appl_errdisable_capabilities_get(vtss_appl_errdisable_capabilities_t *cap);

/**
 * \brief Get whether a given client supports a given configuration feature.
 *
 * \param client [IN]  Client to obtain the supported features for. Only clients in range ]VTSS_APPL_ERRDISABLE_CLIENT_NONE; VTSS_APPL_ERRDISABLE_CLIENT_LAST[ are valid.
 * \param cap    [OUT] Points to location tostore the support.
 *
 * \return VTSS_RC_OK if conf contains valid data, else error code
 */
mesa_rc vtss_appl_errdisable_client_capabilities_get(vtss_appl_errdisable_client_t client, vtss_appl_errdisable_client_capabilities_t *cap);

/**
 * \brief Iterate through all clients.
 *
 * \param prev_client [IN]  Previous client index.
 * \param next_client [OUT] Next client index.
 *
 * \return VTSS_RC_OK as long as next_client is OK, VTSS_RC_ERROR when at the end of the list.
 */
mesa_rc vtss_appl_errdisable_client_itr(const vtss_appl_errdisable_client_t *prev_client, vtss_appl_errdisable_client_t *next_client);

/**
 * \brief Get the global configuration
 *
 * \param global_conf [OUT] Points to location tostore the global configuration.
 *
 * \return VTSS_RC_OK if conf contains valid data, else error code
 */
mesa_rc vtss_appl_errdisable_global_conf_get(vtss_appl_errdisable_global_conf_t *global_conf);

/**
 * \brief Set the global configuration.
 *
 * \param global_conf [IN] Pointer to new global configuration.
 *
 * \return VTSS_RC_OK if configuration went well, else error code
 */
mesa_rc vtss_appl_errdisable_global_conf_set(const vtss_appl_errdisable_global_conf_t *global_conf);

/**
 * \brief Get the per-client configuration.
 *
 * \param client      [IN]  Client to obtain configuration for. Only clients in range ]VTSS_APPL_ERRDISABLE_CLIENT_NONE; VTSS_APPL_ERRDISABLE_CLIENT_LAST[ are valid.
 * \param client_conf [OUT] Points to location to store the client configuration.
 *
 * \return VTSS_RC_OK if conf contains valid data, else error code
 */
mesa_rc vtss_appl_errdisable_client_conf_get(vtss_appl_errdisable_client_t client, vtss_appl_errdisable_client_conf_t *client_conf);

/**
 * \brief Set the per-client configuration.
 *
 * \param client      [IN] Client to set new configuraiton for. Only clients in range ]VTSS_APPL_ERRDISABLE_CLIENT_NONE; VTSS_APPL_ERRDISABLE_CLIENT_LAST[ are valid.
 * \param client_conf [IN] Pointer to new client configuration.
 *
 * \return VTSS_RC_OK if configuration went well, else error code
 */
mesa_rc vtss_appl_errdisable_client_conf_set(vtss_appl_errdisable_client_t client, const vtss_appl_errdisable_client_conf_t *client_conf);

/**
 * \brief Get the current interface status.
 *
 * \param ifindex          [IN]  Interface index.
 * \param interface_status [OUT] Pointer to location to store the interface status.
 *
 * \return VTSS_RC_OK if 'interface_status' contains valid data, else error code
 */
mesa_rc vtss_appl_errdisable_interface_status_get(vtss_ifindex_t ifindex, vtss_appl_errdisable_interface_status_t *interface_status);

/**
 * \brief Get the current interface notification status.
 *
 * Notification status is status that, when changed, may give rise to
 * JSON notifications and/or SNMP traps.
 *
 * \param ifindex                [IN]  Interface index.
 * \param interface_notif_status [OUT] Pointer to location to store the interface notification status.
 *
 * \return VTSS_RC_OK if 'interface_notif_status' contains valid data, else error code
 */
mesa_rc vtss_appl_errdisable_interface_notification_status_get(vtss_ifindex_t ifindex, vtss_appl_errdisable_interface_notification_status_t *interface_notif_status);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_ERRDISABLE_H_ */
