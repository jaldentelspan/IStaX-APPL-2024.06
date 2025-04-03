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
 * \brief Public Mac API
 * \details This header file describes Mac public control functions and types
 */

#ifndef _VTSS_APPL_MAC_H_
#define _VTSS_APPL_MAC_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

/**
 * Read-only structure defining the MAC capabilities.
 */
typedef struct {
    uint32_t  mac_addr_non_volatile_max;                  /**< MAX number of addresses that can be added through this module */
} vtss_appl_mac_capabilities_t;


/**
 * Read-only structure showing the total number of learned mac-address in the unit.
 */
typedef struct {
    uint32_t             dynamic_total;                   /**< Total number of learned entries in the Table */
    uint32_t             static_total;                    /**< Total number of static entrie  in the Table  */
} vtss_appl_mac_table_stats_t;

/**
 * Read-only structure showing learned entries for a port.
 */
typedef struct {
    uint32_t             dynamic;                         /**< Total number of learned entries for the port */
} vtss_appl_mac_port_stats_t;

/**
 * Read-only structure showing the mac address and its properties in a (stacked or standalone) unit.
 */
typedef struct {
    mesa_vid_mac_t               vid_mac;            /**< VLAN ID and MAC address                             */
    vtss_port_list_stackable_t   destination;        /**< Dest. ports for this mac address                    */
    mesa_bool_t                  dynamic;            /**< Dynamic (1) or Static (locked) (0)                  */
    mesa_bool_t                  volatil;            /**< Volatile (0) (saved to flash) or not (1)            */
    mesa_bool_t                  copy_to_cpu;        /**< Make a CPU copy of frames to/from this MAC (1)      */
} vtss_appl_mac_stack_addr_entry_t;

/**
 * This structure is used when insterting a mac-address in a (stacked or standalone) unit.
 */
typedef struct {
    mesa_vid_mac_t               vid_mac;            /**< VLAN ID and MAC address                             */
    vtss_port_list_stackable_t   destination;        /**< Dest. ports  for this mac address                   */
} vtss_appl_mac_stack_addr_entry_conf_t;

/**
 * The mac address age timer in the (stacked or standalone) unit.
 */
typedef struct {
    uint32_t             mac_age_time;                    /**< MAC table age time in seconds */
} vtss_appl_mac_age_conf_t;

/**
 * This write only structure flushes all dynamic addresses in the (stacked or standalone) unit(s).
 */
typedef struct {
    mesa_bool_t          flush_all;                         /**< Flush all dynamic address */
} vtss_appl_mac_flush_t;

/**
 * This enum witholds the learn mode for port.
 */
typedef enum {
    VTSS_APPL_MAC_LEARNING_AUTO,                     /**< Auto learning          */
    VTSS_APPL_MAC_LEARNING_DISABLE,                  /**< Learning disabled      */
    VTSS_APPL_MAC_LEARNING_SECURE,                   /**< Learn frames discarded */
} vtss_appl_mac_port_learn_mode_t;

/**
 * This structure holds the learn mode of the port (above) and if the mode can be changed. 
 * Note, the learn mode can be locked by e.g. a security module and the 'chg_allowed' will reflect that.
 */
typedef struct {
    vtss_appl_mac_port_learn_mode_t learn_mode;      /**< The port learn mode                   */
    mesa_bool_t                            chg_allowed;     /**< Learn mode change allowed. Read only. */
} vtss_appl_mac_learn_mode_t;

/**
 * VLAN learn mode on/off.
 */
typedef struct {
    mesa_bool_t            learning;                        /**< Auto VLAN learning enabled/disabled */
} vtss_appl_mac_vid_learn_mode_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Get mac address aging time.
 *
 * \param conf    [OUT]  structure for the age time
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_mac_age_time_get(vtss_appl_mac_age_conf_t *const conf);

/**
 * \brief Get mac address aging time.
 *
 * \param conf    [IN]  structure for the age time
 *
 * \return Return code.
 **/
mesa_rc vtss_appl_mac_age_time_set(const vtss_appl_mac_age_conf_t *const conf);

/**
 * \brief Get the learn mode of the port incl. if the learn mode can be changed.
 * If other modules have changed the learn mode then the user is not allowed to change it
 * through this mgmt interface.
 *
 * \param ifindex  [IN]   Interface index - the logical interface
 *                        index of the physical port.
 *
 * \param mode     [OUT]  The learning mode of the interface.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_learn_mode_get(vtss_ifindex_t    ifindex,
                                     vtss_appl_mac_learn_mode_t *const mode);

/**
 * \brief Set the learn mode of the port.
 *
 * \param ifindex  [IN]   Interface index - the logical interface
 *                        index of the physical port.
 *
 * \param mode     [IN]    The learning mode of the interface.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_learn_mode_set(vtss_ifindex_t    ifindex,
                                     const vtss_appl_mac_learn_mode_t *const mode);

/**
 * \brief Set the learn mode of the vlan.
 *
 * \param vid      [IN]   VLAN id, 1-4095
 *
 * \param mode     [IN]   Learning enabled or disabled.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_vlan_learn_mode_set(mesa_vid_t     vid,
                                     const vtss_appl_mac_vid_learn_mode_t *const mode);

/**
 * \brief Get the learn mode of the vlan.
 *
 * \param vid      [IN]   VLAN id, 1-4095
 *
 * \param mode     [OUT]  Learning mode of the VLAN (enabled or disabled)
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_vlan_learn_mode_get(mesa_vid_t     vid,
                                          vtss_appl_mac_vid_learn_mode_t *const mode);

/**
 * \brief Set/add an static MAC entry into the MAC table.
 *  If the MAC address exists it will get updated with the new entry.
 *
 * \param vid      [IN]   VLAN id of the MAC address
 * \param mac      [IN]   6 byte MAC address
 * \param entry    [IN]   The destination port mask
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_table_conf_set(mesa_vid_t vid, mesa_mac_t mac,
                                     const vtss_appl_mac_stack_addr_entry_conf_t *const entry);

/**
 * \brief Delete an configured MAC entry from the MAC table.
 * If the entry does not exists in the MAC module the operation will fail.
 *
 * \param vid      [IN]   VLAN id of the MAC address
 * \param mac      [IN]   6 byte MAC address
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_table_conf_del(mesa_vid_t vid, mesa_mac_t mac);

/**
 * \brief Get an MAC entry (added through vtss_appl_mac_table_conf_set()) from in the MAC table.
 * If the entry does not exists in the MAC module the operation will fail.
 *
 * \param vid      [IN]   VLAN id of the MAC address
 * \param mac      [IN]   6 byte MAC address
 *
 * \param entry    [IN]   The destination port mask
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_table_conf_get(mesa_vid_t vid, mesa_mac_t mac,
                                     vtss_appl_mac_stack_addr_entry_conf_t  *const entry);

/**
 * \brief Get an default MAC entry, usually the function is used for SNMP RowEditor to make
 * sure all paratmers are valid.
 *
 * \param vid      [IN]   VLAN id of the MAC address
 * \param mac      [IN]   6 byte MAC address
 *
 * \param entry    [OUT]  default MAC entry
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_table_conf_default(mesa_vid_t *vid, mesa_mac_t *mac,
                                vtss_appl_mac_stack_addr_entry_conf_t *const entry);

/**
 * \brief Get an Static MAC entry from the global MAC table.
 * If the entry does not exists in the MAC module the operation will fail.
 *
 * \param vid      [IN]   VLAN id of the MAC address
 * \param mac      [IN]   6 byte MAC address
 *
 * \param entry    [IN]   The destination port mask
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_table_static_get(mesa_vid_t vid, mesa_mac_t mac,
                                       vtss_appl_mac_stack_addr_entry_t  *const entry);

/**
 * \brief Get an MAC entry (any) from the global MAC table.
 * If the entry does not exists in the MAC module the operation will fail.
 *
 * \param vid      [IN]   VLAN id of the MAC address
 * \param mac      [IN]   6 byte MAC address
 *
 * \param entry    [IN]   Properties of the MAC address incl. the destination port mask
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_table_get(mesa_vid_t vid, mesa_mac_t mac,
                                vtss_appl_mac_stack_addr_entry_t  *const entry);

/**
 * \brief Iterate function of the MAC table.
 * This function will iterate through all entries in the table.
 *
 * Use NULL pointer to get the first address.
 *
 * \param prev_vid   [IN]  previous VID.
 * \param next_vid   [OUT] next VID.
 * \param prev_mac   [IN]  previous MAC.
 * \param next_mac   [OUT] next MAC.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_mac_table_all_itr(const mesa_vid_t *const prev_vid,
                                    mesa_vid_t       *const next_vid,
                                    const mesa_mac_t *const prev_mac,
                                    mesa_mac_t       *const next_mac);

/**
 * \brief Iterate function of the MAC table.
 * This function will iterate through non-volatile entries 
 * which has been added through this module.
 *
 * Use NULL pointer to get the first address.
 *
 * \param prev_vid   [IN]  previous VID.
 * \param next_vid   [OUT] next VID.
 * \param prev_mac   [IN]  previous MAC.
 * \param next_mac   [OUT] next MAC.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_mac_table_conf_itr(const mesa_vid_t *const prev_vid,
                                     mesa_vid_t       *const next_vid,
                                     const mesa_mac_t *const prev_mac,
                                     mesa_mac_t       *const next_mac);

/**
 * \brief Iterate function of the MAC table.
 * This function will iterate through all static entries 
 * found in the table.
 *
 * Use NULL pointer to get the first address.
 *
 * \param prev_vid   [IN]  previous VID.
 * \param next_vid   [OUT] next VID.
 * \param prev_mac   [IN]  previous MAC.
 * \param next_mac   [OUT] next MAC.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_mac_table_static_itr(const mesa_vid_t *const prev_vid,
                                       mesa_vid_t       *const next_vid,
                                       const mesa_mac_t *const prev_mac,
                                       mesa_mac_t       *const next_mac);

/**
 * \brief Get a MAC address table statistics.
 *
 * \param stats    [OUT]   MAC Address statistics
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_table_stats_get(vtss_appl_mac_table_stats_t *const stats);

/**
 * \brief Get a MAC address table statistics based on ports
 *
 * \param ifindex    [IN]    Interface index - the logical interface
 *                           index of the physical port.
 * \param stats      [OUT]   Port MAC Address statistics
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_port_stats_get(vtss_ifindex_t ifindex,
                                     vtss_appl_mac_port_stats_t *const stats);

/**
 * \brief Flush the MAC Address table.
 * All dynamic entries are flushed.
 *
 * \param flush      [IN]   mesa_bool_t flush operation 
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_table_flush(const vtss_appl_mac_flush_t *const flush);

/**
 * \brief Get the capabilities if the MAC module.
 *
 * \param cap      [IN]   cap structure 
 *
 * \return VTSS_RC_OK if the operation succeeded.
 **/
mesa_rc vtss_appl_mac_capabilities_get(vtss_appl_mac_capabilities_t *const cap);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_MAC_H_ */

