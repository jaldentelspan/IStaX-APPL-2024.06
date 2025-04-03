/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _PSEC_API_H_
#define _PSEC_API_H_

/**
 * \file psec_api.h
 * \brief This file defines the inter-module API for the Port Security module
 *
 * It is ONLY intended to be used by the PSEC user modules, not for management.
 */

#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_mac_t, etc. */
#include "vtss/basics/enum_macros.hxx"
#include <vtss/appl/psec.h>

/**
 * \brief Defines the maximum number of MAC addresses that the port security
 *        module can manage.
 *
 * It doesn't make sense to set this to a value greater than the size of
 * the MAC table on a single switch. The pool of entries is shared amongst
 * all ports.
 *
 * The limit module will allow at most PSEC_MAC_ADDR_ENTRY_CNT -1 number of
 * MAC addresses on a specific port.
 */
#define PSEC_MAC_ADDR_ENTRY_CNT 1024

#if defined(VTSS_SW_OPTION_PSEC_LIMIT)
/**
 * Minimum limit on number of MAC addresses that can be
 * learned on a port.
 * 0 can be used to make sure that only MAC addresses
 * statically learned through the MAC module are allowed on a port,
 * and if a dynamic MAC address comes in, a notification can be sent.
 */
#define PSEC_LIMIT_MIN 0
#else
/**
 * When PSEC_LIMIT is not included, this is 0.
 */
#define PSEC_LIMIT_MIN 0
#endif

#if defined(VTSS_SW_OPTION_PSEC_LIMIT)
/**
 * Maximum limit on number of MAC addresses that can be
 * learned on a port.
 */
#define PSEC_LIMIT_MAX (PSEC_MAC_ADDR_ENTRY_CNT - 1)
#else
/**
 * When PSEC_LIMIT is not included, this is 0.
 */
#define PSEC_LIMIT_MAX 0
#endif

/**
 * Minimum limit on number of violating MAC addresses.
 * Only used when violation mode is 'restrict'.
 */
#define PSEC_VIOLATE_LIMIT_MIN 1

/**
 * Maximum limit on number of violating MAC addresses.
 */
#define PSEC_VIOLATE_LIMIT_MAX (PSEC_MAC_ADDR_ENTRY_CNT - 1)

/**
 *  Minimum allowed aging period in seconds.
 */
#define PSEC_AGE_TIME_MIN 10

/**
 *  Maximum allowed aging period in seconds.
 */
#define PSEC_AGE_TIME_MAX 10000000

/**
 * \brief Defines the minimum hold time in seconds.
 */
#define PSEC_HOLD_TIME_MIN 10

/**
 * \brief Defines the maximum hold time in seconds.
 */
#define PSEC_HOLD_TIME_MAX 10000000

/**
 * \brief Zombie Hold Time
 *
 * When a H/W or S/W add failure is detected, the
 * port in question is disabled for CPU copying
 * for this amount of time.
 */
#define PSEC_ZOMBIE_HOLD_TIME_SECS (300)

/**
 * \brief Result of calling back the On-MAC-Add callback
 *
 * The On-MAC-Add callback function is only called on ports
 * on which that module is enabled.
 *
 * Since more than one module may be enabled at the same time,
 * and since one module may e.g. allow a MAC address to forward,
 * while another wants it to be blocked, a hierarchy is implemented
 * as follows:
 * The higher the enumeration value, the higher priority.
 * For example, if one module says 'forward' and another says
 * 'block', then 'block' will win.
 */
typedef enum {
    /**
      * Allow this MAC address to forward in the MAC table right
      * from the beginning. The entry will be aged according to
      * the age rules specified in psec_mgmt_time_conf_set().
      */
    PSEC_ADD_METHOD_FORWARD,

    /**
      * Add this MAC address to the MAC table right away, but
      * don't allow it to forward. The entry will be held there
      * according to the hold-times specified in psec_mgmt_time_conf_set().
      */
    PSEC_ADD_METHOD_BLOCK,

    /**
      * Add this MAC address to the MAC table right away, but
      * don't allow it to forward. The entry will not be removed
      * from the MAC table, and will therefore not be subject
      * to the hold-time specified in psec_mgmt_time_conf_set().
      * The reason for this is as follows:
      * If 802.1X is enabled, then the 802.1X will start by
      * requesting the entry to be added as KEEP_BLOCKED,
      * while authentication is ongoing. In this period it
      * shall not be 'aged' out. If authentication succeeds
      * then the entry will be moved to FORWARD state,
      * whereas if the authentication fails, it will be moved
      * to the BLOCK state, where the hold time specified
      * with psec_mgmt_time_conf_set() takes effect.
      *
      * Modules supposed to use this:
      *  802.1X
      */
    PSEC_ADD_METHOD_KEEP_BLOCKED,

    /**
      * THIS MUST COME LAST! DON'T USE
      */
    PSEC_ADD_METHOD_CNT
} psec_add_method_t;

/**
 * \brief The reason for calling back the On-MAC-Del callback.
 */
typedef enum {
    PSEC_DEL_REASON_HW_ADD_FAILED,            /**< MAC Table add failed (number of locked entries for the hash in the MAC table was exceeded).              */
    PSEC_DEL_REASON_SW_ADD_FAILED,            /**< MAC Table add failed (MAC module S/W ran out of entries, or a reserved MAC address was attempted added). */
    PSEC_DEL_REASON_PORT_LINK_DOWN,           /**< The port link went down                                                                                  */
    PSEC_DEL_REASON_STATION_MOVED,            /**< The MAC was suddenly seen on another port                                                                */
    PSEC_DEL_REASON_AGED_OUT,                 /**< The entry aged out                                                                                       */
    PSEC_DEL_REASON_HOLD_TIME_EXPIRED,        /**< The hold time expired                                                                                    */
    PSEC_DEL_REASON_USER_DELETED,             /**< The entry was deleted by another module                                                                  */
    PSEC_DEL_REASON_PORT_SHUT_DOWN,           /**< The port was shut down by PSEC LIMIT module                                                              */
    PSEC_DEL_REASON_NO_MORE_USERS,            /**< The last user-module got disabled on this port (user modules will never see this reason).                */
    PSEC_DEL_REASON_PORT_STP_MSTI_DISCARDING, /**< The port STP MSTI state is discarding                                                                    */
    PSEC_DEL_REASON_SVL_CHANGE,               /**< The Shared VLAN Learning mapping changed                                                                 */
} psec_del_reason_t;

/**
 * \brief The action involved with adding a MAC address.
 *
 * THIS IS ONLY TO BE USED BY THE PSEC LIMIT MODULE. OTHER
 * MODULES MUST NOT ALTER ITS VALUE.
 *
 * It is used in the On-MAC-Add callback function to signal
 * to this module whether the limit is reached on a port,
 * it should be shut-down, or we can keep on going.
 */
typedef enum {
    /**
      * Keep the port open for secure learning after this
      * MAC address is added.
      */
    PSEC_ADD_ACTION_NONE,

    /**
      * This one new MAC address caused the limit to be reached. Depending on
      * the port mode (psec_port_mode_t), this will cause CPU copying to be
      * disabled or keep it enabled.
      * Once a MAC address is deleted or aged out, CPU copying may get re-
      * enabled.
      */
    PSEC_ADD_ACTION_LIMIT_REACHED,

    /**
      * This one new MAC address caused the limit to
      * be exceeded, and a port-shut-down action was
      * attached with the port.
      * This module will remove all MAC addresses attached
      * to the port, and make sure that the port doesn't
      * learn new MAC addresses until it is administratively
      * reopened with a shutdown/no shutdown or a port security
      * configuration change on the interface.
      */
    PSEC_ADD_ACTION_SHUT_DOWN,
} psec_add_action_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Set aging and hold times.
 *
 * If a MAC address is in forwarding mode (all enabled modules have
 * returned PSEC_ADD_METHOD_FORWARD), then the
 * \@aging_period_secs will be used.
 * Setting age_period_secs to 0 disables aging.
 * If more than one module have different aging requirements,
 * then the shortest aging time will be used.
 * If one module sets aging to X (X > 0) and another module
 * sets it to 0 (disable), then aging *will* be enabled.
 * Valid aging periods are in the range [10; 10,000,000] secs
 * and 0 (disable aging).
 *
 * If a MAC address is in blocking mode (at least one module
 * has set it to PSEC_ADD_METHOD_BLOCK, and none has set
 * it to PSEC_ADD_METHOD_KEEP_BLOCKED), then the
 * \@hold_time_secs will be used.
 * Valid hold-times are in the range [10; 10,000,000] secs.
 * 0 is invalid.
 *
 * The psec_mgmt_time_conf_set_special() function is identical, but is meant to
 * be used by PSEC_LIMIT only.
 *
 * \param user              [IN]: The user calling this function.
 * \param aging_period_secs [IN]: See description above.
 * \param hold_time_secs    [IN]: See description above.
 *
 * \return
 *   VTSS_RC_OK if applying the new aging and hold time settings succeeded.\n
 *   VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH if the switch is not currently the primary switch.\n
 *   VTSS_APPL_PSEC_RC_INV_USER if the \@user parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_INV_AGING_PERIOD if the supplied aging period is out of bounds.\n
 *   VTSS_APPL_PSEC_RC_INV_HOLD_TIME if the supplied hold time is out of bounds.
 */
mesa_rc psec_mgmt_time_conf_set(vtss_appl_psec_user_t user, u32 aging_period_secs, u32 hold_time_secs);
mesa_rc psec_mgmt_time_conf_set_special(vtss_appl_psec_user_t user, u32 aging_period_secs, u32 hold_time_secs);


/**
 * \brief The required state of the port.
 *
 * Some modules may require that the initial state of the port
 * is blocked, and only MAC addresses that the given module determines
 * are OK are added.
 *
 * Such a module must obtain frames (MAC-addresses) by other means
 * than through the PSEC module (e.g. through BPDUs).
 *
 * Once such a module wants to add a MAC address, it calls
 * the psec_mgmt_mac_add(), which will take care of adding
 * the MAC address to the table, and call all other enabled modules
 * to get their view of that MAC address.
 */
typedef enum {
    /**
      * This is the normal state for user modules to use.
      * The port will be enabled for CPU copying until a user
      * module tells the PSEC module to stop.
      */
    PSEC_PORT_MODE_NORMAL,

    /**
      * With this mode, a user module can keep the port blocked
      * so that not one single MAC address can reach the PSEC
      * module from the packet module. All allowed MAC addresses
      * must come through the psec_mgmt_mac_add() function.
      */
    PSEC_PORT_MODE_KEEP_BLOCKED,

    /**
      * This is a special variant of PSEC_PORT_MODE_NORMAL.
      * It should be used by PSEC_LIMIT only, and controls
      * whether CPU-copying should remain enabled after a limit
      * has been reached. It also controls whether the violate-counter
      * should increment or not (can be attached to a notification).
      */
    PSEC_PORT_MODE_RESTRICT,

    /**
     * Must come last
     */
    PSEC_PORT_MODE_LAST
} psec_port_mode_t;

/** \brief Enable or disable a user-module on a given port
 *
 * Use this function to enable or disable your module on a given
 * isid:port.
 *
 * Besides the enable parameter determining whether you're about
 * to enable or disable your module, this function takes a callback
 * function. The purpose of this callback function is to allow your
 * module to determine whether existing entries should be deleted or
 * kept.
 *
 * Your callback function will be called during the duration of
 * the call to psec_mgmt_port_conf_set(). This means that you may have taken your
 * own critical section before the call to psec_mgmt_port_conf_set().
 * psec_mgmt_port_conf_set() will then iterate over all existing entries for
 * that isid:port, and call you back. The called back function
 * should therefore not acquire your module's critical section,
 * and is not allowed to call other psec_XXX() functions, since
 * that will result in a deadlock.
 *
 * This operation must be handled atomically, i.e. without
 * letting go of the PSEC's critical section, because if we didn't
 * it might happen that your module missed an entry that was added
 * or deleted while changing the configuration.
 *
 * For each entry your module determines to delete, the On-MAC-Del
 * callback function will be called for all enabled modules but yours.
 *
 * This way of informing a new module of existing entries is useful
 * for e.g. Voice VLAN, where existing entries must be removed
 * if they aren't considered part of the Voice VLAN.
 *
 * The 802.1X module will delete all entries, and the PSEC LIMIT
 * module will delete entries until the maximum is reached.
 *
 * psec_mgmt_port_conf_set() will call back with latest added MAC address
 * first.
 *
 * If you call psec_mgmt_port_conf_set() with @port_mode = PSEC_PORT_MODE_KEEP_BLOCKED,
 * no new MAC addresses will be learned through the packet module
 * (CPU copying will be turned off with the port in secure learning).
 * This is useful if your module gets the allowed MAC addresses
 * by other means than through the PSEC module (e.g. through BPDUs).
 * Most user modules will have to use the @port_mode = PSEC_PORT_MODE_NORMAL.
 *
 * When the last user disables its usage of a port, all previously
 * learned MAC addresses will be removed and the port will return
 * to H/W-based learning.
 *
 * \param user                  [IN]: The user-module identifying you!
 * \param isid                  [IN]: The switch ID you're trying to configure.
 * \param port                  [IN]: The port on \@isid that you're trying to configure.
 *                                    You may call it with any port number between 0
 *                                    and VTSS_PORT_NO_END, even stack ports. Unavailable ports
 *                                    will not really be configured, though.
 * \param enable                [IN]: Set to TRUE to enable your module on this port,
 *                                    FALSE to disable.
 * \param port_mode             [IN]: See description above. Only used if @enable is TRUE.
 *
 * \return
 *   VTSS_RC_OK if applying the new enabledness succeeded.\n
 *   VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH if the switch is not currently the primary switch.\n
 *   VTSS_APPL_PSEC_RC_INV_USER if the \@user parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_INV_ISID if the supplied \@isid parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_INV_PORT if the supplied \@port parameter is invalid.
 */
mesa_rc psec_mgmt_port_conf_set(vtss_appl_psec_user_t user,
                                vtss_isid_t           isid,
                                mesa_port_no_t        port,
                                BOOL                  enable,
                                psec_port_mode_t      port_mode);

/** \brief Enable or disable a user-module on a given port
 *
 * ONLY TO BE USED BY PSEC_LIMIT!!
 *
 * \param user                  [IN]: The user-module identifying you!
 * \param isid                  [IN]: The switch ID you're trying to configure.
 * \param port                  [IN]: The port on \@isid that you're trying to configure.
 *                                    You may call it with any port number between 0
 *                                    and VTSS_PORT_NO_END, even stack ports. Unavailable ports
 *                                    will not really be configured, though.
 * \param enable                [IN]: Set to TRUE to enable your module on this port,
 *                                    FALSE to disable.
 * \param port_mode             [IN]: See description above. Only used if @enable is TRUE.
 * \param reopen_port           [IN]: MAY ONLY BE USED BY PSEC LIMIT. Always set to
 *                                    FALSE by any other user module. Only used when
 *                                    PSEC LIMIT disables security on the port.
 * \param limit_reached         [IN]: MAY ONLY BE USED BY PSEC LIMIT. Always set to
 *                                    FALSE by any other user module. Only used when
 *                                    PSEC LIMIT enables itself on this port with a limit of 0.
 * \param violate_limit         [IN]: Must be 0 for all modules, but PSEC LIMIT.
 * \param sticky                [IN]: Must be FALSE for all modules, but PSEC_LIMIT.
 *
 * \return
 *   VTSS_RC_OK if applying the new enabledness succeeded.\n
 *   VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH if the switch is not currently the primary switch.\n
 *   VTSS_APPL_PSEC_RC_INV_USER if the \@user parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_INV_ISID if the supplied \@isid parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_INV_PORT if the supplied \@port parameter is invalid.
 */
mesa_rc psec_mgmt_port_conf_set_special(vtss_appl_psec_user_t user,
                                        vtss_isid_t           isid,
                                        mesa_port_no_t        port,
                                        BOOL                  enable,
                                        psec_port_mode_t      port_mode,
                                        BOOL                  reopen_port,
                                        BOOL                  limit_reached,
                                        u32                   violate_limit,
                                        BOOL                  sticky);

/** \brief Change a MAC address's forwarding state
 *
 * Once the forwarding state has been determined by a module,
 * that module may change it if something should happen in the
 * module's internal state.
 *
 * The 802.1X module, for instance, may move the MAC
 * address from PSEC_ADD_METHOD_KEEP_BLOCKED to
 * PSEC_ADD_METHOD_FORWARD when authentication succeeds or
 * PSEC_ADD_METHOD_BLOCK when authentication fails.
 *
 * \param user       [IN]: The user-module identifying you!
 * \param isid       [IN]: The switch ID you're trying to change.
 * \param port       [IN]: The port on \@isid that you're trying to change.
 * \param vid_mac    [IN]: The <MAC, VID> you're trying to change.
 * \param new_method [IN]: The new forward decision made by your module.
 *
 * \return
 *   VTSS_RC_OK if applying the new forwarding state succeeded.\n
 *   VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH if the switch is not currently the primary switch.\n
 *   VTSS_APPL_PSEC_RC_INV_USER if the \@user parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_INV_ISID if the supplied \@isid parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_INV_PORT if the supplied \@port parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_MAC_VID_NOT_FOUND if the supplied \@vid_mac was not found
 *     among the attached MAC addresses on the port (both MAC and VID must match).
 */
mesa_rc psec_mgmt_mac_chg(vtss_appl_psec_user_t user, vtss_isid_t isid, mesa_port_no_t port, mesa_vid_mac_t *vid_mac, psec_add_method_t new_method);

/** \brief Add a MAC address
 *
 * Add a MAC address to the MAC table.
 * Only users that have called psec_mgmt_port_conf_set() with
 * port_mode == PSEC_PORT_MODE_KEEP_BLOCKED are allowed to call
 * this function. Others will be rejected.
 *
 * This serves as an alternative way to secure MAC addresses.
 * All modules but the calling module will get a chance to
 * allow or block the MAC address.
 *
 * \param user       [IN]: The user-module identifying you!
 * \param isid       [IN]: The switch ID you're trying to change.
 * \param port       [IN]: The port on \@isid that you're trying to change.
 * \param vid_mac    [IN]: The <MAC, VID> you're trying to change.
 * \param new_method [IN]: The new forward decision made by your module.
 *
 * \return
 *   VTSS_RC_OK if applying the new forwarding state succeeded.\n
 *   VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH if the switch is not currently the primary switch.\n
 *   VTSS_APPL_PSEC_RC_INV_USER if the \@user parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_INV_ISID if the supplied \@isid parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_INV_PORT if the supplied \@port parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_MAC_VID_ALREADY_FOUND if the supplied \@vid_mac was already found
 *     among the attached MAC addresses on any port (both MAC and VID are used in match).
 *   VTSS_APPL_PSEC_RC_INV_USER_MODE if the \@user is not enabled on the port or if he has not called the
 *     psec_mgmt_port_conf_set() function with \@port_mode == PSEC_PORT_MODE_KEEP_BLOCKED.
 *   VTSS_APPL_PSEC_RC_SWITCH_IS_DOWN if the switch pointed to by \@isid doesn't exist.
 *   VTSS_APPL_PSEC_RC_LINK_IS_DOWN if the port pointed to by \@port has link-down.
 *   VTSS_APPL_PSEC_RC_MAC_POOL_DEPLETED if the PSEC module is out of state machines.
 *   VTSS_APPL_PSEC_RC_PORT_IS_SHUT_DOWN if the PSEC Limit module has already shut the port
 *     down or shut the port down as a result of attempting to add this one.
 *   VTSS_APPL_PSEC_RC_LIMIT_IS_REACHED if the PSEC Limit module has proclamed that the limit
 *     is reached (and it doesn't result in a port-shut-down to attempt to add next MAC address).
 *   VTSS_APPL_PSEC_RC_NO_USERS_ENABLED if no users are enabled on the port (cannot be returned by this func,
 *     since it's caught by the VTSS_APPL_PSEC_RC_INV_USER_MODE).
 *   VTSS_APPL_PSEC_RC_STATE_CHG_DURING_CALLBACK if e.g. the switch was deleted or port had link-down
 *     while the enabled users were called back (which can happen because the internal
 *     mutex has to be released during callbacks to avoid deadlocks).
 *   VTSS_APPL_PSEC_RC_INTERNAL_ERROR if there's a bug in this module.
 */
mesa_rc psec_mgmt_mac_add(vtss_appl_psec_user_t user, vtss_isid_t isid, mesa_port_no_t port, mesa_vid_mac_t *vid_mac, psec_add_method_t method);

/** \brief Delete a MAC address
 *
 * Delete a MAC address from the MAC table.
 * Only users that have called psec_mgmt_port_conf_set() with
 * port_mode == PSEC_PORT_MODE_KEEP_BLOCKED are allowed to call
 * this function. Others will be rejected.
 *
 * This serves as an alternative way to unsecure MAC addresses.
 * Rather than letting the PSEC module handle timeouts, the
 * user-module may decide (e.g. as a reaction to BPDUs) to
 * remove it immediately.
 * All modules but the calling module will get notified through
 * a call to On-MAC-del-callback().
 *
 * \param user       [IN]: The user-module identifying you!
 * \param isid       [IN]: The switch ID you're trying to delete.
 * \param port       [IN]: The port on \@isid that you're trying to delete.
 * \param vid_mac    [IN]: The <MAC, VID> you're trying to delete.
 *
 * \return
 *   VTSS_RC_OK if applying the new forwarding state succeeded.\n
 *   VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH if the switch is not currently the primary switch.\n
 *   VTSS_APPL_PSEC_RC_INV_USER if the \@user parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_INV_ISID if the supplied \@isid parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_INV_PORT if the supplied \@port parameter is invalid.\n
 *   VTSS_APPL_PSEC_RC_MAC_VID_NOT_FOUND if the supplied \@vid_mac was not found
 *     among the attached MAC addresses on the port (both MAC and VID are used in match).
 *   VTSS_APPL_PSEC_RC_INV_USER_MODE if the \@user is not enabled on the port or if he has not called the
 *     psec_mgmt_port_conf_set() function with \@port_mode == PSEC_PORT_MODE_KEEP_BLOCKED.
 */
mesa_rc psec_mgmt_mac_del(vtss_appl_psec_user_t user, vtss_isid_t isid, mesa_port_no_t port, mesa_vid_mac_t *vid_mac);

/**
 * \brief Signature of On-MAC-Add callback function
 *
 * - Reentrancy: The callback may be called back while already being
 *               called back. The callback should not call other
 *               functions in this module will being called back.
 *
 * Only modules that are enabled on isid:port will be called back
 * and asked whether it's OK to add this MAC address on a port as
 * seen from that module's perspective. The module returns its
 * decision as a psec_add_method_t (see that type for details).
 *
 * The called back function must return ASAP.
 *
 * [IN] and [OUT] is seen from the called back module's perspective.
 *
 * \param isid                    [IN]: Switch ID for the MAC address being added.
 * \param port                    [IN]: Port number for the MAC address being added.
 * \param vid_mac                 [IN]: VLAN ID and MAC address to add.
 * \param mac_cnt_before_callback [IN]: The number of MAC addresses already learned on this port. Only valid for user == PSEC LIMIT
 * \param originating_user        [IN]: Who added this MAC? If VTSS_APPL_PSEC_USER_LAST, it was caused by a network frame. Only valid for user == PSEC LIMIT
 * \param action                 [OUT]: NULL for users != PSEC LIMIT. See psec_add_action_t for a detailed description.
 * \return
 *   Your callback must return one of the methods specified by psec_add_method_t.
 */
typedef psec_add_method_t (psec_on_mac_add_callback_f)(vtss_isid_t isid, mesa_port_no_t port, mesa_vid_mac_t *vid_mac, u32 mac_cnt_before_callback, vtss_appl_psec_user_t originating_user, psec_add_action_t *action);

/**
 * \brief Signature of On-MAC-Del callback function
 *
 * - Reentrancy: The callback will not be called back while already
 *               being called back. The callback may not call other
 *               functions in this module while being called back.
 *
 * Only modules that are enabled on isid:port will be notified that
 * a MAC address is being deleted from the MAC table.
 * See psec_del_reason_t for details.
 *
 * The called back function must return ASAP.
 *
 * [IN] and [OUT] is seen from the called back module's perspective.
 *
 * \param isid             [IN]: Switch ID for the MAC address being removed.
 * \param port             [IN]: Port number for the MAC address being removed.
 * \param vid_mac          [IN]: VLAN ID and MAC address being deleted.
 * \param reason           [IN]: Reason for the removal.
 * \param add_method       [IN]: The user module's own add method for this MAC address. Useful for ref-counting.
 * \param originating_user [IN]: The user who caused this delete. VTSS_APPL_PSEC_USER_LAST if it was PSEC module itself (link down, no more users, etc...)
 *
 * \return
 *   Nothing.
 */
typedef void (psec_on_mac_del_callback_f)(vtss_isid_t isid, mesa_port_no_t port, const mesa_vid_mac_t *vid_mac, psec_del_reason_t reason, psec_add_method_t add_method, vtss_appl_psec_user_t originating_user);

/**
 * \brief Register callback functions.
 *
 * Register On-MAC-Add and on-MAC-Del callback functions.
 *
 * Specifying NULL for a given callback function means that the
 * module doesn't want to be called back for the operation in
 * question.
 *
 * Modules will only be called back if they are enabled on a given
 * port.
 *
 * If the called back module calls another function in this module,
 * a deadlock will occur.
 *
 * This function should only be called once, and in the INIT_CMD_START
 * phase. The callback functions will only be used on the primary switch.
 *
 * \param user                [IN]: The user that calls this function.
 * \param on_mac_add_callback [IN]: Pointer to a callback function that gets
 *                                  called when a new MAC address is about to
 *                                  be added to the MAC table on a port that
 *                                  the \@user has enabled. May be NULL.
 * \param on_mac_del_callback [IN]: Pointer to a callback function that gets
 *                                  called when a MAC address is about to
 *                                  be deleted from the MAC table on a port that
 *                                  the \@user has enabled. May be NULL.
 *
 * \return
 *   VTSS_RC_OK: Registration succeeded.\n
 *   PSEC_ERR_INV_USER if parameter supplied in \@user is invalid.
 */
mesa_rc psec_mgmt_register_callbacks(vtss_appl_psec_user_t user, psec_on_mac_add_callback_f *on_mac_add_callback, psec_on_mac_del_callback_f *on_mac_del_callback);

/**
 * \brief Signature of On-MAC-Sticky-Change callback function
 *
 * Only supposed to be used by PSEC_LIMIT module.
 *
 * [IN] and [OUT] is seen from the called back module's perspective.
 *
 * \param ifindex  [IN]: Ifindex of MAC
 * \param mac_conf [IN]: <VLAN, MAC> in question
 *
 * \return
 *   Nothing.
 */
typedef void (psec_on_mac_sticky_change_f)(vtss_ifindex_t ifindex, vtss_appl_psec_mac_conf_t *mac_conf);

/**
 * \brief Change stickiness on a given port
 *
 * This function must ONLY be called by PSEC_LIMIT module when stickiness
 * changes for a port.
 *
 * For each MAC registered on the interface, PSEC will call back PSEC_LIMIT
 * so that it can register MAC addresses correctly in its internal structures.
 *
 * \param on_mac_add_callback [IN]: Pointer to a callback function that gets
 *                                  called when a new MAC address is about to
 *                                  be added to the MAC table on a port that
 *                                  the \@user has enabled. May be NULL.
 * \param on_mac_del_callback [IN]: Pointer to a callback function that gets
 *                                  called when a MAC address is about to
 *                                  be deleted from the MAC table on a port that
 *                                  the \@user has enabled. May be NULL.
 *
 * \return
 *   VTSS_RC_OK: Registration succeeded.\n
 *   PSEC_ERR_INV_USER if parameter supplied in \@user is invalid.
 */
mesa_rc psec_mgmt_port_sticky_set(vtss_ifindex_t ifindex, BOOL sticky, psec_on_mac_sticky_change_f *on_mac_sticky_change_callback);

/**
 * \brief Get PSEC module's mutex.
 *
 * This function must ONLY be called by PSEC_LIMIT module. It is used to avoid
 * deadlocks between PSEC and PSEC_LIMIT.
 *
 * It's an ugly hack, but there's not much to do.
 */
void psec_mgmt_mutex_enter(void);

/**
 * \brief Release PSEC module's mutex.
 *
 * This function must ONLY be called by PSEC_LIMIT module. It is used to avoid
 * deadlocks between PSEC and PSEC_LIMIT.
 *
 * It's an ugly hack, but there's not much to do.
 */
void psec_mgmt_mutex_exit(void);

/**
 * \brief Specialized way of adding a MAC.
 *
 * To be used by PSEC_LIMIT_only.
 */
mesa_rc psec_mgmt_mac_add_special(vtss_appl_psec_user_t user, vtss_ifindex_t ifindex, vtss_appl_psec_mac_conf_t *mac_conf);

/**
 * \brief Specialized way of removing a MAC.
 *
 * To be used by PSEC_LIMIT_only.
 */
mesa_rc psec_mgmt_mac_del_special(vtss_appl_psec_user_t user, vtss_ifindex_t ifindex, vtss_appl_psec_mac_conf_t *mac_conf);

/**
 * \brief Specialized way of conveying an SVL change to the PSEC module
 *
 * To be used by PSEC_LIMIT_only.
 */
void psec_mgmt_fid_change(mesa_vid_t vid, mesa_vid_t old_fid, mesa_vid_t new_fid);

//
// Other public Port Security functions.
//

/**
 * \brief Retrieve an error string based on a return code
 *        from one of the Port Security API functions.
 *
 * \param rc [IN]: Error code that must be in the VTSS_APPL_PSEC_RC_xxx range.
 *
 * \return
 *   A static string describing the error.
 */
const char *psec_error_txt(mesa_rc rc);

/**
 * \brief Initialize the Port Security module
 *
 * \param cmd [IN]: Reason why calling this function.
 * \param p1  [IN]: Parameter 1. Usage varies with cmd.
 * \param p2  [IN]: Parameter 2. Usage varies with cmd.
 *
 * \return
 *    VTSS_RC_OK.
 */
mesa_rc psec_init(vtss_init_data_t *data);

/**
 * \brief Convert delete reason to string.
 *
 * Primarily for debugging purposes.
 *
 * \param reason [IN]: Reason why calling this function.
 *
 * \return
 *    Pointer to static string with the explanation.
 */
const char *psec_del_reason_to_str(psec_del_reason_t reason);

/**
 * \brief Convert add method reason to string.
 *
 * Primarily for debugging purposes.
 *
 * \param reason [IN]: add_method to be converted.
 *
 * \return
 *    Pointer to static string with the explanation.
 */
const char *psec_add_method_to_str(psec_add_method_t add_method);

/**
 * Opaque pre-declaration of psec_mac_state_t.
 */
typedef struct tag_psec_mac_state_t psec_mac_state_t;

/**
 * \brief Internal interface state
 *
 * Each instance of this structure is used to manage the MAC addresses
 * on one interface.
 *
 * The reason to semi-publicize this structure is to make it available
 * to the vtss::expose interface, so that changes to the public sub-structure
 * (vtss_appl_psec_interface_status_t) can become JSON notifications
 * and SNMP traps.
 */
typedef struct {
    /**
     * The MAC addresses attached to this port. It also includes those entries
     * that couldn't be saved to the MAC table due to H/W limitations or
     * MAC Module S/W limitations.
     * These entries are marked with PSEC_MAC_STATE_FLAGS_HW_ADD_FAIL/
     * PSEC_MAC_STATE_FLAGS_SW_ADD_FAILED.
     * The user-modules have no knowledge about entries marked as such.
     */
    psec_mac_state_t *macs;

    /**
     * This describes individual user's selected port mode.
     */
    psec_port_mode_t port_mode[VTSS_APPL_PSEC_USER_LAST];

    /**
     * When set, new SMAC addresses received directly by this module through
     * learn frames are discarded silently.
     * This is used when #port_mode for at least one user is set to
     * PSEC_PORT_MODE_KEEP_BLOCKED, because that user module wants to keep
     * control of which MAC addresses at all are learned by PSEC (currently only
     * 802.1X does this).
     */
    BOOL block_learn_frames;

    /**
     * When set, new SMAC addresses received directly by this module through
     * learn frames are discarded silently.
     * This is used to allow PSEC_LIMIT to add static/sticky MAC addresses prior
     * to learn frames getting learned.
     */
    BOOL block_learn_frames_pending_static_add;

    /**
     * There is a race condition between calls to psec_mgmt_port_conf_set() and
     * PSEC_link_state_change_callback(). This race condition is taken care of
     * by the use of this additional state variable.
     */
    BOOL adding_static_macs;

    /**
     * When set, CPU copying of learn frames is kept enabled even when the limit
     * is reached.
     * This is used when #port_mode for at least one user is set to
     * PSEC_PORT_MODE_RESTRICT, because that user module wants to know which MAC
     * addresses are violating a limit (currently only used by PSEC LIMIT) or simply
     * needs to shut-down the port when limit is exceeded.
     * When set, vtss_appl_psec_interface_status_t::violate_cnt increments.
     */
    BOOL keep_cpu_copying_enabled;

    /**
     * This one is configured by PSEC_LIMIT during call to psec_mgmt_port_conf_set().
     * If cur_violate_cnt exceeds this limit, new <VLAN, MAC>-entries are discarded
     * unless the limit is not reached.
     */
    u32 violate_limit;

    /**
     * Interface status. This is the publicized interface status.
     */
    vtss_appl_psec_interface_status_t status;
} psec_semi_public_interface_status_t;

#ifdef __cplusplus
}
#endif

#endif /* _PSEC_API_H_ */

