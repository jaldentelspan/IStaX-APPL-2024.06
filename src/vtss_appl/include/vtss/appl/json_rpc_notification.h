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

/**
 * \file
 * \brief Public JSON-RPC Notification API.
 * \details JSON-RPC Notifications is the JSON-RPC equivalent to SNMP traps - it
 * is a mechanism to notify "other systems" when an event occur.
 *
 * JSON-RPC Notification is a part of the JSON-RPC 1.0 standard which can be
 * found at http://json-rpc.org/wiki/specification.
 *
 * This header has two concepts:
 *  - notification destination
 *  - notification event subscription
 *
 * There is a dependent one-to-many relation between these two. A given
 * notification destination may have many notification event subscriptions. If a
 * notification destination is deleted, then all the configured event
 * subscriptions the given destination owns is deleted as well.
 *
 * More information on this can be found in the VTSS documents RS1108 and
 * AN1126.
 *
 */

#ifndef _VTSS_APPL_JSON_RPC_NOTIFICATION_H_
#define _VTSS_APPL_JSON_RPC_NOTIFICATION_H_

#include <microchip/ethernet/switch/api/types.h>
#include <vtss/basics/enum-descriptor.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Max allowed string length for the JSON-RPC Notification destination name. */
#define VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH 16

/** Max allowed string length for the JSON-RPC Notification URL. */
#define VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_URL_LENGTH 254

/** Max allowed string length for the JSON-RPC Notification username . */
#define VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_USER_LENGTH 32

/** Max allowed string length for the JSON-RPC Notification password. */
#define VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_PASS_LENGTH 32

/** Max allowed string length for the JSON-RPC Notification event name. */
#define VTSS_APPL_JSON_RPC_NOTIFICATION_EVENT_LENGTH 96

/** \brief Name of the JSON-RPC Notification destination. */
typedef struct {
    /** Name of the JSON-RPC Notification destination. */
    char name[VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH + 1];
} vtss_appl_json_rpc_notification_dest_name_t;

/** \brief Name of the JSON-RPC Notification event. */
typedef struct {
    /** Name of the JSON-RPC Notification event. */
    char name[VTSS_APPL_JSON_RPC_NOTIFICATION_EVENT_LENGTH + 1];
} vtss_appl_json_rpc_notification_event_name_t;

/** \brief Type of authentication (if any). */
typedef enum {
    /** No authentication. */
    VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_AUTH_TYPE_NONE = 0,

    /** Basic authentication. When selecting this option it will use the 'user'
     * and the 'pass' field in the authentication process. */
    VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_AUTH_TYPE_BASIC = 1
} vtss_appl_json_rpc_notification_dest_auth_type_t;

/** \brief Enum descriptor text */
extern const vtss_enum_descriptor_t
        vtss_appl_json_rpc_notification_dest_auth_type_txt[];

/** \brief Configurations options for a JSON-RPC Notification destination host.
 * */
typedef struct {
    /** URL to where the notifications are hosted. */
    char url[VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_URL_LENGTH + 1];

    /** Type of authentication to use for this destination. */
    vtss_appl_json_rpc_notification_dest_auth_type_t auth_type;

    /** Username used in authentication */
    char user[VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_USER_LENGTH + 1];

    /** Password used in authentication */
    char pass[VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_PASS_LENGTH + 1];
} vtss_appl_json_rpc_notification_dest_conf_t;

/**
 * \brief Get the configuration of a JSON-RPC Notification destination.
 * \param dest [IN]  Name of the notification destination.
 * \param conf [OUT] Configuration of the notification host.
 * \return Error code.
 */
mesa_rc vtss_appl_json_rpc_notification_dest_get(
        const vtss_appl_json_rpc_notification_dest_name_t *const dest,
        vtss_appl_json_rpc_notification_dest_conf_t       *const conf);

/**
 * \brief Add/update a notification destination.
 * \param dest [IN]  Name of the notification destination.
 * \param conf [IN]  Configuration of the notification host.
 * \return Error code.
 */
mesa_rc vtss_appl_json_rpc_notification_dest_set(
        const vtss_appl_json_rpc_notification_dest_name_t *const dest,
        const vtss_appl_json_rpc_notification_dest_conf_t *const conf);

/**
 * \brief Delete a notification destination.
 * \param dest [IN]  Name of the notification destination.
 * \return Error code.
 */
mesa_rc vtss_appl_json_rpc_notification_dest_del(
        const vtss_appl_json_rpc_notification_dest_name_t *const dest);

/**
 * \brief An iterator function to allow iterating through the set of configured
 * destinations.
 * \param in  [IN]  A null-pointer to get-first, otherwise the notification to
 *                  the next-of.
 * \param out [OUT] First/next notification destination.
 * \return Error code.
 */
mesa_rc vtss_appl_json_rpc_notification_dest_itr(
        const vtss_appl_json_rpc_notification_dest_name_t *const in,
        vtss_appl_json_rpc_notification_dest_name_t       *const out);

/**
 * \brief Check if a given subscription exists
 * \param dest  [IN]  Name of the notification destination.
 * \param event [IN]  Name of the notification event.
 * \return Error code.
 */
mesa_rc vtss_appl_json_rpc_notification_event_subscribe_get(
        const vtss_appl_json_rpc_notification_dest_name_t  *const dest,
        const vtss_appl_json_rpc_notification_event_name_t *const event);

/**
 * \brief Add subscription of an event for a given destination.
 * \param dest  [IN]  Name of the notification destination.
 * \param event [IN]  Name of the notification event.
 * \return Error code.
 */
mesa_rc vtss_appl_json_rpc_notification_event_subscribe_add(
        const vtss_appl_json_rpc_notification_dest_name_t  *const dest,
        const vtss_appl_json_rpc_notification_event_name_t *const event);

/**
 * \brief Delete subscription of an event for a given destination.
 * \param dest  [IN]  Name of the notification destination.
 * \param event [IN]  Name of the notification event.
 * \return Error code.
 */
mesa_rc vtss_appl_json_rpc_notification_event_subscribe_del(
        const vtss_appl_json_rpc_notification_dest_name_t  *const dest,
        const vtss_appl_json_rpc_notification_event_name_t *const event);

/**
 * \brief Iterator function to allow iteration through all the configured event
 * subscriptions.
 * \param dest_in   [IN]  null-pointer to get first, otherwise the destination
 *                        name to get next of.
 * \param dest_out  [OUT] First/next destination name.
 * \param event_in  [IN]  null-pointer to get first, otherwise the event name to
 *                        get next of.
 * \param event_out [OUT] First/next event name.
 * \return Error code.
 */
mesa_rc vtss_appl_json_rpc_notification_event_subscribe_itr(
        const vtss_appl_json_rpc_notification_dest_name_t  *const dest_in,
        vtss_appl_json_rpc_notification_dest_name_t        *const dest_out,
        const vtss_appl_json_rpc_notification_event_name_t *const event_in,
        vtss_appl_json_rpc_notification_event_name_t       *const event_out);

#ifdef __cplusplus
}
#endif
#endif  // _VTSS_APPL_JSON_RPC_NOTIFICATION_H_

