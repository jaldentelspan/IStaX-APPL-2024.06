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
 * \brief Public Port security (PSEC) API
 * \details This header file describes Port Security functions and types.
 */

#ifndef _VTSS_APPL_PSEC_H_
#define _VTSS_APPL_PSEC_H_

#include <vtss/appl/interface.h> /* For vtss_ifindex_t                 */
#include <vtss/appl/defines.h>   /* For VTSS_APPL_RFC3339_TIME_STR_LEN */
#include <vtss/appl/module_id.h> /* For MODULE_ERROR_START             */
#include <vtss/basics/map.hxx>   /* For vtss::Map<>                    */

/**
 * Read-only structure defining the Port-Security capabilities.
 */
typedef struct {
    /**
     * A bit mask indicating the software modules included in this build that
     * can control the Port Security feature.
     * For instance, if DHCP snooping is included in this build,
     * \p users & (1 << VTSS_APPL_PSEC_USER_DHCP_SNOOPING) is non-zero.
     */
    uint32_t users;

    /**
     * Total number of Port Security-controlled MAC addresses.
     */
    uint32_t pool_size;

    /**
     * The minimum limit on an interface.
     */
    uint32_t limit_min;

    /**
     * The maximum limit on an interface.
     */
    uint32_t limit_max;

    /**
     * The minimum limit on an interface.
     */
    uint32_t violate_limit_min;

    /**
     * The maximum limit on an interface.
     */
    uint32_t violate_limit_max;

    /**
     * The minimum aging period in seconds.
     */
    uint32_t age_time_min;

    /** The maximum aging period in seconds.
     */
    uint32_t age_time_max;

    /**
     * The minimum hold-time in seconds.
     */
    uint32_t hold_time_min;

    /**
     * The maximum hold-time in seconds.
     */
    uint32_t hold_time_max;
} vtss_appl_psec_capabilities_t;

/**
 * \brief Port security error codes (mesa_rc)
 */
enum {
    VTSS_APPL_PSEC_RC_INV_USER = MODULE_ERROR_START(VTSS_MODULE_ID_PSEC), /**< Invalid user parameter.                                 */
    VTSS_APPL_PSEC_RC_MUST_BE_PRIMARY_SWITCH,                             /**< Operation is only allowed on the primary switch.        */
    VTSS_APPL_PSEC_RC_INV_ISID,                                           /**< isid parameter is invalid.                              */
    VTSS_APPL_PSEC_RC_INV_PORT,                                           /**< port parameter is invalid.                              */
    VTSS_APPL_PSEC_RC_INV_IFINDEX,                                        /**< Unable to decompose ifindex                             */
    VTSS_APPL_PSEC_RC_IFINDEX_NOT_REPRESENTING_A_PORT,                    /**< Ifindex not representing a port                         */
    VTSS_APPL_PSEC_RC_INV_VLAN,                                           /**< Invalid VLAN                                            */
    VTSS_APPL_PSEC_RC_INV_AGING_PERIOD,                                   /**< The supplied aging period is invalid.                   */
    VTSS_APPL_PSEC_RC_INV_HOLD_TIME,                                      /**< The supplied hold time is invalid.                      */
    VTSS_APPL_PSEC_RC_MAC_VID_NOT_FOUND,                                  /**< The <MAC, VID> was not found on the port.               */
    VTSS_APPL_PSEC_RC_MAC_VID_ALREADY_FOUND,                              /**< The <MAC, VID> was already found on any port.           */
    VTSS_APPL_PSEC_RC_MAC_VID_ALREADY_FOUND_ON_SVL,                       /**< The <MAC, VID> was already found on any port on another VLAN mapping to the same shared VLAN */
    VTSS_APPL_PSEC_RC_INV_USER_MODE,                                      /**< The user is not allowed to call this function.          */
    VTSS_APPL_PSEC_RC_SWITCH_IS_DOWN,                                     /**< The selected switch doesn't exist.                      */
    VTSS_APPL_PSEC_RC_LINK_IS_DOWN,                                       /**< The selected port's link is down.                       */
    VTSS_APPL_PSEC_RC_MAC_POOL_DEPLETED,                                  /**< We're out of state machines.                            */
    VTSS_APPL_PSEC_RC_PORT_IS_SHUT_DOWN,                                  /**< The port has been shut down by the PSEC Limit module.   */
    VTSS_APPL_PSEC_RC_LIMIT_IS_REACHED,                                   /**< The port's limit is reached. Cannot add MAC address.    */
    VTSS_APPL_PSEC_RC_NO_USERS_ENABLED,                                   /**< No users are enabled on the port.                       */
    VTSS_APPL_PSEC_RC_STP_MSTI_DISCARDING,                                /**< The port STP MSTI state is discarding.                  */
    VTSS_APPL_PSEC_RC_INV_PARAM,                                          /**< An invalid parameter other than the above was supplied. */
    VTSS_APPL_PSEC_RC_END_OF_LIST,                                        /**< Marks the end of the list. Not a real error.            */
    VTSS_APPL_PSEC_RC_OUT_OF_MEMORY,                                      /**< Out of memory.                                          */
    VTSS_APPL_PSEC_RC_MAC_NOT_UNICAST,                                    /**< MAC address is not a unicast MAC address.               */

    /* Internal error codes */
    VTSS_APPL_PSEC_RC_INV_RATE_LIMITER_FILL_LEVEL,                        /**< Max. fill-level must be greater than the minimum.       */
    VTSS_APPL_PSEC_RC_INV_RATE_LIMITER_RATE,                              /**< The rate-limiter rate must be greater than 0.           */
    VTSS_APPL_PSEC_RC_INTERNAL_ERROR,                                     /**< An internal error occurred.                             */
};

/**
 * This enum identifies PSEC users.
 *
 * A PSEC user is a module that can modify decisions on whether to
 * allow a MAC entry to forward or not. It can also control aging and hold options.
 *
 * VTSS_APPL_PSEC_USER_ADMIN corresponds to an end-user, that is, an
 * administrator using CLI, SNMP, JSON, or Web to change functionality.
 */
typedef enum {
    VTSS_APPL_PSEC_USER_ADMIN,         /**< Administrator */
    VTSS_APPL_PSEC_USER_DOT1X,         /**< 802.1X/NAS    */
    VTSS_APPL_PSEC_USER_DHCP_SNOOPING, /**< DHCP Snooping */
    VTSS_APPL_PSEC_USER_VOICE_VLAN,    /**< Voice VLAN */

    // For the sake of persistent layout of SNMP OIDs,
    // new volatile modules must come just before this line.
    VTSS_APPL_PSEC_USER_LAST        /**< May be needed for iterations across this enum. Must come last */
} vtss_appl_psec_user_t;

/**
 * \brief Info about user in terms of a name and an abbreviation.
 */
typedef struct {
    /**
     * A string representation of a given PSEC user.
     */
    char name[30];

    /**
     * A one-letter abbreviation of a given PSEC user.
     */
    char abbr;
} vtss_appl_psec_user_info_t;

/**
 * \brief Global Port Security Configuration.
 */
typedef struct {
    /**
      * Globally enable/disable aging of secured entries.
      * This doesn't affect aging of addresses secured by
      * other modules.
      */
    mesa_bool_t enable_aging;

    /**
     * If aging is globally enabled, this is the aging period in seconds.
     * Valid range is given by vtss_appl_psec_capabilities_t::age_time_min and
     * vtss_appl_psec_capabilities_t::age_time_max.
     */
    uint32_t aging_period_secs;

    /**
     * If vtss_appl_psec_interface_conf_t::violation_mode is set to
     * VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT, violating MAC addresses will be
     * held in the MAC table in a blocked condition for this amount of time
     * (measured in seconds), and
     * vtss_appl_psec_interface_status_t::cur_violate_cnt and
     * vtss_appl_psec_interface_notification_status_t::total_violate_cnt are
     * incremented for each violating MAC address added as blocked to the MAC
     * table and decremented when its hold time expires.
     * If a violating MAC address was not held in the MAC table for a period of
     * time, the same source MAC address could cause CPU overutilization and the
     * listeners to changes in total_violate_cnt could get flooded with
     * notifications.
     *
     * Valid range is given by vtss_appl_psec_capabilities_t::hold_time_min and
     * vtss_appl_psec_capabilities_t::hold_time_max.
     */
    uint32_t hold_time_secs;
} vtss_appl_psec_global_conf_t;

/**
 * \brief List of possible actions to be taken when limit is reached.
 */
typedef enum {
    /**
     * Drop packets with unknown source MAC address until the current number of
     * learned MAC addresses on the port drops below the limit.
     */
    VTSS_APPL_PSEC_VIOLATION_MODE_PROTECT,

    /**
     * Drop packets with unknown source MAC address until the current number of
     * learned MAC addresses on the port drops below the limit. Violating MAC
     * addresses get counted in vtss_appl_psec_interface_status_t::cur_violate_cnt
     * and vtss_appl_psec_interface_notification_status_t::total_violate_cnt.
     */
    VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT,

    /**
     * Clear all learned MAC addresses on the interface and shut down the
     * interface. To get out of shut down, do a "shutdown/no shutdown" on the
     * interface or re-configure Port Security on the interface.
     * To get a notification on a shutdown, attach to changes in
     * vtss_appl_psec_interface_notification_status_t::shut_down.
     */
    VTSS_APPL_PSEC_VIOLATION_MODE_SHUTDOWN,

    /**
     * This must come last and only servers as a count for the number of
     * valid port security violation modes.
     */
    VTSS_APPL_PSEC_VIOLATION_MODE_LAST,
} vtss_appl_psec_violation_mode_t;

/**
 * \brief Per-interface Port Security Configuration.
 */
typedef struct {
    /**
      * Controls whether Port Security is enabled on this interface.
      */
    mesa_bool_t enabled;

    /**
      * Maximum number of MAC addresses allowed on this interface.
      * Valid values in range vtss_appl_psec_capabilities_t::limit_min and
      * vtss_appl_psec_capabilities_t::limit_max.
      */
    uint32_t limit;

    /**
      * Action to take if number of MAC addresses exceeds the limit.
      */
    vtss_appl_psec_violation_mode_t violation_mode;

    /**
     * Limit on number of violating MAC addresses.
     * Valid values in range vtss_appl_psec_capabilities_t::violate_limit_min
     * and vtss_appl_psec_capabilities_t::violate_limit_max.
     */
    uint32_t violate_limit;

    /**
     * Controls whether the port is a sticky port.
     * A sticky port retains forwarding MAC addresses across link-down
     * conditions. These will be part of the running-config and if copied to
     * startup-config, the port will not have to re-learn addresses from ingress
     * traffic after a boot.
     * Once enabled, all forwarding and dynamic MAC addresses get converted to
     * sticky MAC addresses. If disabled, they return from being sticky to
     * becoming dynamic again.
     */
    mesa_bool_t sticky;
} vtss_appl_psec_interface_conf_t;

/**
 * \brief Type of the MAC address.
 *
 * Each MAC address can be added dynamically, statically, or so-called sticky.
 */
typedef enum {
    /**
     * MAC addresses that are neither static nor sticky are dynamic. Such
     * entries are removed automatically on e.g. link down, rebooting, and
     * aging. Dynamic entries can be either forwarding or blocking.
     * A dynamic entry may move from one port to another.
     */
    VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC,

    /**
     * Static MAC addresses are added statically through management. They are
     * per definition forwarding and part of the running-config and and will
     * survive link changes and - if saved to startup-config - reboots. Static
     * entries cannot age out and cannot move from one port to another.
     */
    VTSS_APPL_PSEC_MAC_TYPE_STATIC,

    /**
     * Sticky MAC addresses are dynamic MAC addresses converted into static MAC
     * addresses when the sticky feature is enabled on a given port. Only
     * currently forwarding dynamic MAC addresses are converted. If a user
     * module has decided to block a MAC address, it will be kept blocked until
     * that user module puts it into forwarding, in which case, it will turn
     * into a static (sticky) MAC. Sticky MACs therefore exhibit the same
     * properties as static MACs with the exception that they will be converted
     * back into dynamic entries if the sticky feature gets disabled on the
     * port.
     */
    VTSS_APPL_PSEC_MAC_TYPE_STICKY,
} vtss_appl_psec_mac_type_t;

/**
 * \brief Structure to add/remove a <VLAN, MAC> entry to a given interface.
 */
typedef struct {
    /**
     * VLAN ID
     */
    mesa_vid_t vlan;

    /**
     * MAC address
     */
    mesa_mac_t mac;

    /**
     * Controls the entry type.
     * Under normal circumstances, this must be set to
     * VTSS_APPL_PSEC_MAC_TYPE_STATIC.
     *
     * VTSS_APPL_PSEC_MAC_TYPE_DYNAMIC is intended for debug purposes, so that
     * a dynamic entry can be added.
     *
     * VTSS_APPL_PSEC_MAC_TYPE_STICKY is used for sticky MACs. It is not
     * recommended to use this directly.
     */
    vtss_appl_psec_mac_type_t mac_type;
} vtss_appl_psec_mac_conf_t;

/**
  * \brief Global status.
  *
  * This structure holds the current global status.
  */
typedef struct {
  /**
   * Total number of MAC addresses managed by Port Security
   * in the system (fixed number).
   */
  uint32_t total_mac_cnt;

  /**
   * Number of currently unused MAC addresses in the system managed by Port
   * Security. The number of used MAC addresses can then be found by
   * subtracting cur_mac_cnt from total_mac_cnt.
   */
  uint32_t cur_mac_cnt;
} vtss_appl_psec_global_status_t;

/**
  * \brief Global notification status.
  *
  * This structure holds the current global notification status
  * which allows for SNMP Traps and JSON Notifications.
  */
typedef struct {
  /**
   * There is only a limited number of <VLAN, MAC>-entries
   * pre-reserved by Port Security. If all entries are in use,
   * this field will be TRUE. When at least one entry is free,
   * this field will be FALSE.
   */
  mesa_bool_t pool_depleted;
} vtss_appl_psec_global_notification_status_t;

/**
  * \brief Interface status.
  *
  * This structure holds the current interface status.
  * Another struct, vtss_appl_psec_interface_notification_status_t,
  * holds identifiers that one can get notifications through.
  */
typedef struct {
    /**
     * Bit mask indicating the Port Security users that are currently
     * enabled on this interface. Refer to VTSS_APPL_PSEC_USER_XXX for users.
     * If e.g. DHCP Snooping user is enabled on this interface,
     * \p users & (1 << VTSS_APPL_PSEC_USER_DHCP_SNOOPING) is non-zero.
     */
    uint32_t users;

    /**
     * Number of MAC addresses currently assigned to this interface.
     * This includes cur_violate_cnt.
     */
    uint32_t mac_cnt;

    /**
     * Current number of violating MAC addresses. Only counts when
     * violation_mode is set to VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT. Gets
     * reset when violation_mode is changed to something different. Counts up
     * when adding a MAC address to the MAC table as blocked, and down when
     * releasing the blocked MAC address from the MAC table when the hold time
     * has expired.
     */
    uint32_t cur_violate_cnt;

    /**
     * TRUE if the limit is reached on the interface, FALSE otherwise.
     */
    mesa_bool_t limit_reached;

    /**
     * TRUE if secure learning is enabled on the interface, FALSE otherwise.
     * Mainly for debugging.
     */
    mesa_bool_t sec_learning;

    /**
     * TRUE if CPU copying is enabled on the interface, FALSE otherwise.
     * Mainly for debugging.
     */
    mesa_bool_t cpu_copying;

    /**
     * TRUE if interface link is up, FALSE otherwise.
     * Mainly for debugging.
     */
    mesa_bool_t link_is_up;

    /**
     * TRUE if at least one STP MSTI instance is discarding on the interface, FALSE otherwise.
     * Mainly for debugging.
     */
    mesa_bool_t stp_discarding;

    /**
     * TRUE if H/W add of a MAC address failed on this interface, FALSE otherwise.
     * Mainly for debugging.
     */
    mesa_bool_t hw_add_failed;

    /**
     * TRUE if S/W add of a MAC address failed on this interface, FALSE otherwise.
     * Mainly for debugging.
     */
    mesa_bool_t sw_add_failed;

    /**
     * TRUE if port is configured as a sticky port, FALSE otherwise.
     */
    mesa_bool_t sticky;
} vtss_appl_psec_interface_status_t;

/**
  * \brief Interface notification status.
  *
  * This structure holds the current notification status for a given
  * interface. See also vtss_appl_psec_interface_status_t.
  */
typedef struct {
    /**
     * Total number of violating MAC addresses. Only counts when violation_mode
     * is set to VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT. Gets reset when
     * violation_mode is changed to something different. Can only count up, not
     * down.
     */
    uint32_t total_violate_cnt;

    /**
     * TRUE if the interface is shut down, FALSE otherwise.
     * This state can only be reached when violation_mode is set to
     * VTSS_APPL_PSEC_VIOLATION_MODE_SHUTDOWN.
     */
    mesa_bool_t shut_down;

    /**
     * VLAN of latest violating MAC address.
     * Used when violation_mode is set to VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT
     * or VTSS_APPL_PSEC_VIOLATION_MODE_SHUTDOWN. If latest_violating_vlan is 0,
     * neither this field nor latest_violating_mac are valid.
     */
    mesa_vid_t latest_violating_vlan;

    /**
     * MAC address of latest violating MAC address.
     * Used when violation_mode is set to VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT
     * or VTSS_APPL_PSEC_VIOLATION_MODE_SHUTDOWN.
     */
    mesa_mac_t latest_violating_mac;
} vtss_appl_psec_interface_notification_status_t;

/**
 * \brief Key to use when looking up MAC entries in one go.
 */
typedef struct {
    /**
     * The interface on which this <vid, mac> is learned.
     */
    vtss_ifindex_t ifindex;

    /**
     * The VLAN that this MAC address is learned.
     */
    mesa_vid_t vlan;

    /**
     * The MAC address learned on this interface.
     */
    mesa_mac_t mac;
} vtss_appl_psec_mac_map_key_t;

/**
  * \brief MAC Address status
  *
  * Each instance of this structure holds the state of one MAC address.
  */
typedef struct {
    /**
     * The interface (always of type port) on which this MAC address is learned.
     */
    vtss_ifindex_t ifindex;

    /**
     * The VLAN and MAC address that this is all about
     */
    mesa_vid_mac_t vid_mac;

    /**
     * And it was originally added at this time
     */
    char creation_time[VTSS_APPL_RFC3339_TIME_STR_LEN];

    /**
     * It was last changed at this time
     */
    char changed_time[VTSS_APPL_RFC3339_TIME_STR_LEN];

    /**
     * Down-counter used in blocking and aging process
     */
    uint32_t age_or_hold_time_secs;

    /**
     * When violation_mode is set to VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT,
     * violating MAC addresses are marked with a TRUE in this field. At the same
     * time such MAC addresses will be blocked or kept_blocked. Non-violating
     * MAC addresses are marked with FALSE in this field, but they may be
     * blocked or kept_blocked by other user modules.
     */
    mesa_bool_t violating;

    /**
     * TRUE if MAC address is blocked from orwarding, FALSE if forwarding.
     * See status of "keep_blocked" to see if a user module
     * has put it in a state where it's kept blocked until
     * further notice from that module.
     */
    mesa_bool_t blocked;

    /**
     * TRUE if MAC address is kept blocked in the MAC table
     * until further notice from a given user module.
     * If this flag is set, so is 'blocked'.
     * Mainly for debugging.
     */
    mesa_bool_t kept_blocked;

    /**
     * TRUE if CPU copying is enabled for this MAC address (due to aging),
     * FALSE otherwise.
     * Mainly for debugging.
     */
    mesa_bool_t cpu_copying;

    /**
     * TRUE if aging frame has been received during aging for this MAC address,
     * FALSE otherwise.
     * Mainly for debugging.
     */
    mesa_bool_t age_frame_seen;

    /**
     * Bit mask indicating the Port Security users that have indicated
     * this MAC address is eligible to forwarding.
     * Refer to VTSS_APPL_PSEC_USER_XXX for users.
     * If e.g. DHCP Snooping user says so,
     * \p users_forwarding & (1 << VTSS_APPL_PSEC_USER_DHCP_SNOOPING) is non-zero.
     */
    uint32_t users_forward;

    /**
     * Bit mask indicating the Port Security users that have indicated
     * this MAC address must be blocked.
     * Refer to VTSS_APPL_PSEC_USER_XXX for users.
     * If e.g. DHCP Snooping user says so,
     * \p users_blocking & (1 << VTSS_APPL_PSEC_USER_DHCP_SNOOPING) is non-zero.
     */
    uint32_t users_block;

    /**
     * Bit mask indicating the Port Security users that have indicated
     * this MAC address must be kept blocked (not subject to holding).
     * Refer to VTSS_APPL_PSEC_USER_XXX for users.
     * If e.g. DHCP Snooping user says so,
     * \p users_keep_blocking & (1 << VTSS_APPL_PSEC_USER_DHCP_SNOOPING) is non-zero.
     */
    uint32_t users_keep_blocked;

    /**
     * Determines the type of entry.
     * Refer to vtss_appl_psec_mac_type_t for details.
     */
    vtss_appl_psec_mac_type_t mac_type;
} vtss_appl_psec_mac_status_t;

/**
 * Make it easy to refer to a Map of this type
 */
typedef vtss::Map<vtss_appl_psec_mac_map_key_t, vtss_appl_psec_mac_status_t> vtss_appl_psec_mac_status_map_t;

/**
 * Structure for controlling which MAC address(es) to remove.
 *
 * By clearing all the boolean members of this structure, all dynamic
 * learned MAC addresses on all interfaces are removed.
 *
 * To remove sticky and static entries, use vtss_appl_psec_interface_conf_mac_del()
 */
typedef struct {
    /**
     * Set to FALSE to search through all ports when deleting MAC addresses.
     * Set to TRUE to search only the interface pointed to by \p ifindex.
     */
    mesa_bool_t specific_ifindex;

    /**
     * Interface (of type port) on which to search for MAC addresses
     * to delete.
     * Only used if \p specific_ifindex is TRUE.
     */
    vtss_ifindex_t ifindex;

    /**
     * Set to FALSE to search through all VLANs when deleting a MAC address.
     * Set to TRUE to search only the VLAN pointed to by \p vlan.
     */
    mesa_bool_t specific_vlan;

    /**
     * VLAN to remove MAC addresses for.
     * Only used if \p specific_vlan is TRUE.
     */
    mesa_vid_t vlan;

    /**
     * Set to FALSE to search through all MAC addresses when deleting a MAC address.
     * Set to TRUE to search only for the MAC address pointed to by \p mac.
     */
    mesa_bool_t specific_mac;

    /**
     * MAC address to remove.
     * Only used if \p specific_mac is TRUE.
     */
    mesa_mac_t mac;
} vtss_appl_psec_global_control_mac_clear_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get this switch's Port-Security capabilities.
 *
 * \param cap [OUT] Pointer to structure receiving the R/O capabilities.
 *
 * \return VTSS_RC_OK on success, anything else on error. Use error_txt(return code)
 *         to get a textual representation of the error code.
 */
mesa_rc vtss_appl_psec_capabilities_get(vtss_appl_psec_capabilities_t *const cap);

/**
 * \brief Get a default global configuration
 *
 * \param global_conf [IN]: Pointer to where to put the default configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_global_conf_default_get(vtss_appl_psec_global_conf_t *const global_conf);

/**
 * \brief Get the global Port Security configuration.
 *
 * \param global_conf [OUT]: Pointer to structure that receives the current configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_global_conf_get(vtss_appl_psec_global_conf_t *const global_conf);

/**
 * \brief Set the global Port Security configuration.
 *
 * \param global_conf [IN]: Pointer to structure that contains the global configuration to apply.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_global_conf_set(const vtss_appl_psec_global_conf_t *global_conf);

/**
 * \brief Get a default interface configuration
 *
 * \param interface_conf [IN]: Pointer to where to put the default configuration.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_interface_conf_default_get(vtss_appl_psec_interface_conf_t *const interface_conf);

/**
 * \brief Get an interface's configuration.
 *
 * \param ifindex        [IN]:  The interface index for which to retrieve the configuration.
 * \param interface_conf [OUT]: Pointer to structure that receives the port security configuration of the given interface.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_interface_conf_get(vtss_ifindex_t ifindex, vtss_appl_psec_interface_conf_t *const interface_conf);

/**
 * \brief Set an interface's configuration.
 *
 * \param ifindex        [IN]: The interface index for which to set the configuration.
 * \param interface_conf [IN]: Pointer to structure that contains the configuration to be applied.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_interface_conf_set(vtss_ifindex_t ifindex, const vtss_appl_psec_interface_conf_t *interface_conf);

/**
 * \brief Get a default MAC entry.
 *
 * Usually this function is used by the SNMP RowEditor to make sure to get an
 * initially valid value.
 *
 * \param ifindex  [OUT]: Provides a default interface
 * \param vlan     [OUT]: Provides a default VLAN
 * \param mac      [OUT]: Provides a default MAC
 * \param mac_conf [OUT]: Provides a default MAC configuration
 *
 * \return VTSS_RC_OK on success.
 **/
mesa_rc vtss_appl_psec_interface_conf_mac_default_get(vtss_ifindex_t *ifindex, mesa_vid_t *vlan, mesa_mac_t *mac, vtss_appl_psec_mac_conf_t *const mac_conf);

/**
 * \brief Add a <VLAN, MAC> to a given interface.
 *
 * Even though this interface is capable of adding a dynamic entry, it is not
 * recommended.
 *
 * To install it on the current port VLAN ID, use VTSS_VID_NULL (0) for
 * vtss_appl_psec_mac_conf_t::vlan or use the current port VLAN ID.
 *
 * Notice that a MAC address installed on the current port VLAN ID will move
 * along with changing the port VLAN ID (access VLAN or native VLAN).
 * If another static/sticky entry alrady exists on that other VLAN, then
 * the current entry will get lost.
 *
 * \param ifindex  [IN]: The interface index for which to add the <VLAN, MAC>.
 * \param vlan     [IN]: The VLAN on which to add the MAC
 * \param mac      [IN]: The MAC address to add
 * \param mac_conf [IN]: Pointer to structure that contains the configuration to be applied. Only mac_type need to be initialized.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_interface_conf_mac_add(vtss_ifindex_t ifindex, mesa_vid_t vlan, mesa_mac_t mac, const vtss_appl_psec_mac_conf_t *const mac_conf);

/**
 * \brief Remove a <VLAN, MAC> from a given interface.
 *
 * This interface cannot delete dynamic entries, and sticky entries may be
 * removed this way, but it's not recommended.
 *
 * To remove it on the current port VLAN ID, use VTSS_VID_NULL (0) for
 * vtss_appl_psec_mac_conf_t::vlan. It also works to insert the current port VLAN ID
 * instead.
 *
 * \param ifindex  [IN]: The interface index for which to remove the <VLAN, MAC>.
 * \param vlan     [IN]: The VLAN on which to remove the MAC
 * \param mac      [IN]: The MAC address to remove
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_interface_conf_mac_del(vtss_ifindex_t ifindex, mesa_vid_t vlan, mesa_mac_t mac);

/**
 * \brief Get MAC address statically added on an interface.
 *
 * \param ifindex  [IN]:  Interface to get entry for.
 * \param vid      [IN]:  VLAN ID to get entry for.
 * \param mac      [IN]:  MAC address to get entry for.
 * \param mac_conf [OUT]: Contains the result if return code equal VTSS_RC_OK.
 *
 * \return VTSS_RC_OK if <vid, mac> is found on ifindex, else an error-indicating return code.
 */
mesa_rc vtss_appl_psec_interface_conf_mac_get(vtss_ifindex_t ifindex, mesa_vid_t vid, mesa_mac_t mac, vtss_appl_psec_mac_conf_t *mac_conf);

/**
 * \brief Get MAC address statically added on an interface.
 *
 * This function is like vtss_appl_psec_interface_conf_mac_get(), but also
 * returns the current PVID. This is to allow e.g. CLI to avoid printing the
 * VLAN if it matches PVID.
 * \param ifindex  [IN]:  Interface to get entry for.
 * \param vid      [IN]:  VLAN ID to get entry for.
 * \param mac      [IN]:  MAC address to get entry for.
 * \param mac_conf [OUT]: Contains the result if return code equal VTSS_RC_OK.
 * \param pvid     [OUT]: Contains the current Port VLAN ID, so that the caller can default.
 *
 * \return VTSS_RC_OK if <vid, mac> is found on ifindex, else an error-indicating return code.
 */
mesa_rc vtss_appl_psec_interface_conf_mac_pvid_get(vtss_ifindex_t ifindex, mesa_vid_t vid, mesa_mac_t mac, vtss_appl_psec_mac_conf_t *mac_conf, mesa_vid_t *pvid);

/**
 * \brief Iterate through static and sticky MAC addresses.
 *
 * Use this function to iterate through all the static and sticky MAC addresses
 * configured/learned on an interface (given by \p ifindex).
 * To get started, set \p prev_ifindex, \p prev_vid, and \p prev_mac to NULL and
 * pass a valid pointer in next_mac_status.
 * In subsequent iterations, pass prev_XXX as input to next_XXX.
 *
 * \param prev_ifindex [IN]:  Previous interface index.
 * \param next_ifindex [OUT]: Next interface index.
 * \param prev_vid     [IN]:  Previous VLAN ID.
 * \param next_vid     [OUT]: Next VLAN ID.
 * \param prev_mac     [IN]:  Previous MAC address.
 * \param next_mac     [OUT]: Next MAC address.
 *
 * \return VTSS_RC_OK as long as \p next_mac_status is OK, VTSS_APPL_PSEC_RC_END_OF_LIST when at
 * the end and any other return code on a real error.
 */
mesa_rc vtss_appl_psec_interface_conf_mac_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                              const mesa_vid_t     *prev_vid,     mesa_vid_t     *next_vid,
                                              const mesa_mac_t     *prev_mac,     mesa_mac_t     *next_mac);

/**
 * \brief Get global status.
 *
 * \param global_status [OUT]: Pointer to structure receiving the current interface status.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_global_status_get(vtss_appl_psec_global_status_t *const global_status);

/**
 * \brief Get global notification status.
 *
 * \param global_notif_status [OUT]: Pointer to structure receiving the current global notification status.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_global_notification_status_get(vtss_appl_psec_global_notification_status_t *const global_notif_status);

/**
 * \brief Get interface status.
 *
 * \param ifindex          [IN]:  Interface to get status for
 * \param interface_status [OUT]: Pointer to structure receiving the current interface status.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_interface_status_get(vtss_ifindex_t ifindex, vtss_appl_psec_interface_status_t *const interface_status);

/**
 * \brief Get interface notification status.
 *
 * \param ifindex                [IN]:  Interface to get notification status for
 * \param interface_notif_status [OUT]: Pointer to structure receiving the current interface notification status.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_interface_notification_status_get(vtss_ifindex_t ifindex, vtss_appl_psec_interface_notification_status_t *const interface_notif_status);

/**
 * \brief Get MAC address status for all MAC addresses in one go.
 *
 * This makes it possible to get coherent data rather than risking race
 * conditions if using a combination of vtss_appl_psec_interface_status_mac_itr()
 * and vtss_appl_psec_interface_status_mac_get().
 *
 * \param mac_status [OUT] On call: an empty, yet constructed, map. On exit: filled with all known MAC addresses.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_interface_status_mac_get_all(vtss_appl_psec_mac_status_map_t &mac_status);

/**
 * \brief Get status of a MAC address learned on a given interface.
 *
 * See also vtss_appl_psec_interface_status_mac_get_all(), which retrieves all
 * in one go.
 *
 * \param ifindex    [IN]:  Interface to get status for.
 * \param vid        [IN]:  VLAN ID to get status for.
 * \param mac        [IN]:  MAC address to get status for.
 * \param mac_status [OUT]: Contains the result if return code equal VTSS_RC_OK.
 *
 * \return VTSS_RC_OK if <vid, mac> is found on ifindex, else an error-indicating return code.
 */
mesa_rc vtss_appl_psec_interface_status_mac_get(vtss_ifindex_t ifindex, mesa_vid_t vid, mesa_mac_t mac, vtss_appl_psec_mac_status_t *mac_status);

/**
 * \brief Iterate through securely learned MAC addresses.
 *
 * Use this function to iterate through all the MAC addresses learned on an
 * interface (given by \p ifindex).
 * To get started, set \p prev_ifindex, \p prev_vid, and \p prev_mac to NULL and
 * pass a valid pointer in next_mac.
 * In subsequent iterations, pass prev_XXX as input to next_XXX.
 *
 * Notice: The use of this function is prone to a kind of race conditions,
 * because it uses the underlying live status to get the next MAC address given
 * the previous, which in between calls may have been removed or moved to
 * another interface, in which case, the results are undefined. In some
 * protocols (e.g. SNMP), this is unavoidable, whereas in others (e.g. CLI and
 * JSON), it is indeed avoidable by using
 * vtss_appl_psec_interface_status_mac_get_all(), which retrieves all MACs in
 * one go.
 *
 * \param prev_ifindex [IN]:  Previous interface index.
 * \param next_ifindex [OUT]: Next interface index.
 * \param prev_vid     [IN]:  Previous VLAN ID.
 * \param next_vid     [OUT]: Next VLAN ID.
 * \param prev_mac     [IN]:  Previous MAC address.
 * \param next_mac     [OUT]: Next MAC address.
 *
 * \return VTSS_RC_OK as long as \p next_mac_status is OK, VTSS_APPL_PSEC_RC_END_OF_LIST when at
 * the end and any other return code on a real error.
 */
mesa_rc vtss_appl_psec_interface_status_mac_itr(const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
                                                const mesa_vid_t     *prev_vid,     mesa_vid_t     *next_vid,
                                                const mesa_mac_t     *prev_mac,     mesa_mac_t     *next_mac);

/**
 * \brief Remove MAC addresses on an interface, a VLAN, a specific MAC address, or a combi of these.
 *
 * \param info [IN]: Information on what to delete. See struct definition for details.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_global_control_mac_clear(const vtss_appl_psec_global_control_mac_clear_t *info);

/**
 * \brief Utility function that translates a PSEC user to a name and an abbreviation.
 *
 * Using one utility function all over allows for providing a uniform interface
 * on all management interfaces. To figure out which users are included in this build,
 * see vtss_appl_psec_capabilities_t::users.
 *
 * \param user [IN]:  User to obtain info for.
 * \param info [OUT]: Contains info about the user.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_psec_user_info_get(vtss_appl_psec_user_t user, vtss_appl_psec_user_info_t *const info);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_APPL_PSEC_H_ */
