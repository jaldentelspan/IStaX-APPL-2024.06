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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

/**
 * \file
 * \brief Public TOPO API
 * \details 
 *
 * The TOPO module implements the topology protocol for Vitesse stacking.
 * 
 * Introduction to TOPO API
 * ==========================
 * TOPO provides the following functionality:
 * + Topology control
 *   Discover topology (and topology changes) within a stack.
 *   Configure the switch according to the discovered topology.
 * + Master election 
 *   Election of a master switch within the stack.
 *
 * Master Election 
 * ===============
 * Within the stack one switch must be elected "master".
 * The master switch provides a common interface to all switches in the stack.


 * The following parameters control the master election algorithm:
 * + Master election priority (mst_elect_prio)
 *   A number 1-4 controlling the probability of a given switch becoming master.
 *   Smaller priority => Higher probability for becoming master.
 * + Switch MAC address
 *   Switch MAC address (which must be unique among the switches in the stack) is
 *   used as the final tie-breaker in the master election algorithm.
 * + Master time
 *   The number of seconds a switch has been master. This parameter is
 *   used to avoid unnecessary master reelection.
 *
 * The master election algorithm works as follows:
 * 1) If any switch(es) has been master for more than
 *    30 seconds, then choose the one, which
 *    has been master for the longest period of time.
 * 2) Pick the switch with the smallest mst_elect_prio.
 * 3) Pick the switch with the smallest switch_addr.
 *
 */

#ifndef _VTSS_APPL_TOPO_H_
#define _VTSS_APPL_TOPO_H_

#include <vtss/appl/types.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/interface.h>
#include <vtss/basics/enum-descriptor.h>
#include <vtss/basics/array.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * TOPO types and defines
 */

// Master election priority. *
#define TOPO_MST_ELECT_PRIO_START    1 /**< Highest possible priority.*/ 
#define TOPO_MST_ELECT_PRIOS         4 /**< Lowest possible priority.*/ 

/** \brief Stacking (port) configuration */
typedef struct {
    mesa_bool_t             stacking; /**< Set to TRUE to enable stacking.*/
    vtss_ifindex_t ifindex_0;  /**< Stack interface number. For dual-chip switches, ifindex_0 is stack port on primary chip.*/
    vtss_ifindex_t ifindex_1;  /**< Stack interface number. For dual-chip switches, ifindex_1 is stack port on secondary chip.*/
} vtss_appl_topo_stack_config_t;


/** \brief Stacking master election priority configuration */
typedef struct {
    uint8_t prio; /**< master election priority.*/
} vtss_appl_topo_mst_elect_prio_t;


/** \brief TOPO uses a protocol for communication between the switches in the stack. This protocol is named Sprout.
 *     
 *   The frames transmitted by the Sprout protocol are counted for statistics use.
 */
typedef struct {
    /* Topo stack port state */
    mesa_bool_t proto_up;                   /**< TOPO protocol link up state.*/

    /* Sprout Update Counters */
    uint update_rx_cnt;              /**< Rx'ed Updates, including bad ones.*/
    uint update_periodic_tx_cnt;     /**< Tx'ed Updates, periodic (tx OK).  */
    uint update_triggered_tx_cnt;    /**< Tx'ed Updates, triggered (tx OK). */
    uint update_tx_policer_drop_cnt; /**< Tx-drops by Tx-policer.           */
    uint update_rx_err_cnt;          /**< Rx'ed errornuous Updates.         */
    uint update_tx_err_cnt;          /**< Tx of Update failed.              */

    /* Sprout Alert Counters */
    uint alert_rx_cnt;               /**< Rx'ed Alerts, including bad ones. */
    uint alert_tx_cnt;               /**< Tx'ed Alerts (tx OK).             */
    uint alert_tx_policer_drop_cnt;  /**< Tx-drops by Tx-policer.           */
    uint alert_rx_err_cnt;           /**< Rx'ed errornuous Alerts.          */
    uint alert_tx_err_cnt;           /**< Tx of Alert failed.               */
} vtss_appl_topo_stack_port_stat_t;

/** \brief TOPO error return codes (mesa_rc) */
typedef enum {
    VTSS_APPL_TOPO_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_TOPO),  /**< Generic error code.                            */
    VTSS_APPL_TOPO_ERROR_PARM,                                           /**< Illegal parameter.                             */
    VTSS_APPL_TOPO_ERROR_ASSERT_FAILURE,                                 /**< Assertion failed.                              */
    VTSS_APPL_TOPO_ERROR_ALLOC_FAILED,                                   /**< Resources allocation failed.                   */
    VTSS_APPL_TOPO_ERROR_TX_FAILED,                                      /**< Update tx failed.                              */
    VTSS_APPL_TOPO_ERROR_SWITCH_NOT_PRESENT,                             /**< Switch not present in stack.                   */
    VTSS_APPL_TOPO_ERROR_SID_NOT_ASSIGNED,                               /**< SID not assigned to any switch.                */
    VTSS_APPL_TOPO_ERROR_REMOTE_PARM_ONLY_FROM_MST,                      /**< Remote parameter can only be set from master.  */
    VTSS_APPL_TOPO_ERROR_REMOTE_PARM_NOT_SUPPORTED,                      /**< Remote assignment not supported.               */
    VTSS_APPL_TOPO_ERROR_NO_SWITCH_SELECTED,                             /**< No switch selected.                            */
    VTSS_APPL_TOPO_ERROR_NOT_MASTER,                                     /**< Operation only supported on master.            */
    VTSS_APPL_TOPO_ERROR_MASTER_SID,                                     /**< Cannot delete master SID.                      */
    VTSS_APPL_TOPO_ERROR_SID_IN_USE,                                     /**< SID is in use by another switch.               */
    VTSS_APPL_TOPO_ERROR_SWITCH_HAS_SID,                                 /**< Switch has already been assigned to a SID.     */
    VTSS_APPL_TOPO_ERROR_ISID_DELETE_PENDING,                            /**< Operation could be done, due to pending delete.*/
    VTSS_APPL_TOPO_ERROR_CONFIG_ILLEGAL_STACK_PORT,                      /**< Illegal stack port given.                      */
    VTSS_APPL_TOPO_ERROR_SID_NOT_FOUND,                                  /**< The switch was not found.                      */
    VTSS_APPL_TOPO_ERROR_ILLEGAL_SID,                                    /**< The switch ID is not valid                     */
    VTSS_APPL_TOPO_ERROR_STANDALONE,                                     /**< The switch is a standalone switch              */
} vtss_appl_topo_error_t;

/** \brief Current Topology */
typedef enum {
    VtssTopoBack2Back, /**< Two units connected with two stacking cables */
    VtssTopoClosedLoop,/**< Ring topology with more than two units.      */
    VtssTopoOpenLoop   /**< Chain topology.                              */
} vtss_appl_topo_topology_type_t;

/** \brief TOPO master status*/
typedef struct _vtss_appl_topo_mst_stat_t {
    mesa_mac_addr_t                 mst_switch_mac_addr;  /**< The MAC address of the switch which is currently the master.*/
    uint32_t                        mst_change_time;      /**< Time of last master change in seconds.*/
} vtss_appl_topo_mst_stat_t;


/** \brief A stacking interface consist of 2 interfaces. This is a pair of interfaces which can form a stacking interface*/
typedef struct {
    vtss_ifindex_t interfaceA; /**< First interface in the interface pair.*/
    vtss_ifindex_t interfaceB; /**< 2nd interface in the interface pair.  */
} vtss_appl_interface_pair_t;

/** \brief List of valid interface pairs for forming a stacking interface.*/
typedef struct {
    VTSS_ARRAY(vtss_appl_interface_pair_t, 6, pair); /**< List of valid interface pair combinations*/
    uint8_t  count;                                       /**< The actually number of valid combinations of interface pairs.*/
    uint8_t  selected_pair;                               /**< The current selected pair in the list used to form a stacking interface*/
} vtss_appl_topo_valid_interfaces_pair_t;

/** \brief TOPO status and statistics */
typedef struct _vtss_appl_topo_switch_stat_t {
    uint                             switch_cnt;           /**< Current number of switches in the stack.*/
    vtss_appl_topo_topology_type_t   topology_type;        /**< Current Topology type.                  */
    uint32_t                              topology_change_time; /**< Time of last topology change in seconds.*/
    vtss_appl_topo_stack_port_stat_t stack_port[2];        /**< Topo statistics.                        */

    /** The two stack interfaces are named A and B, and the statistics for each of them is stored in the stack_port array 
     * above, but which interface that matches which index in the array depends upon how the stack is 
     * connected.*/
    uint                            if_a_index;            /**< if_a_index gives the index (0 or 1) for the interface A statistics. */
    uint                            if_b_index;            /**< if_b_index gives the index (0 or 1) for the interface B statistics. */ 
} vtss_appl_topo_switch_stat_t;



/*
 * TOPO management functions 
 */

/**
 * Get list of valid interfaces combinations that can be used to form a stack interface.
 *
 * \param usid  [IN]  Switch ID for switch for which to get configuration.
 *
 * \param list  [OUT] Pointer to the where to put the list of valid pairs.
 *
 * \return VTSS_RC_OK if list is valid, else error code.
 */
mesa_rc vtss_appl_valid_interfaces_pairs_get(vtss_usid_t usid, vtss_appl_topo_valid_interfaces_pair_t *list);

/**
 * Get Stack (port) configuration.
 *
 * \param usid  [IN]  Switch ID for switch for which to get configuration.
 *
 * \param conf  [OUT] Pointer to the where to put the configuration. 
 *
 * \return VTSS_RC_OK if conf contains valid configuration, else error code.
 */
mesa_rc vtss_appl_topo_stack_config_get(vtss_usid_t usid, vtss_appl_topo_stack_config_t *conf);

/**
 * Get if a reboot is required.
 *
 * \param usid            [IN]  Switch ID for switch for getting if a reboot is required for getting the configuration applied.
 *
 * \param reboot_required [OUT] If TRUE then a reboot is required in order to get the configuration applied.
 *
 * \return VTSS_RC_OK if reboot_required is valid , else error code.
 */
mesa_rc vtss_appl_topo_stack_reboot_required_get(vtss_usid_t usid, mesa_bool_t *reboot_required);
                        
/**
 * Set Stack (port) configuration.
 *
 * \param usid [IN]  Switch ID for switch to configure.
 *
 * \param conf [OUT] Pointer to the new configuration. 
 *
 * \return VTSS_RC_OK if configuration was successful, else error code.
 */
mesa_rc vtss_appl_topo_stack_config_set(vtss_usid_t usid, const vtss_appl_topo_stack_config_t *conf);

/**
 * Start master reelection.
 *
 * \return VTSS_RC_OK if reelection was done successful, else error code.
 */
mesa_rc vtss_appl_topo_mst_reelection_start(void);

/**
 * Set master election priority.
 *
 * \param usid [IN] Switch ID for switch to configure.
 *
 * \param prio [IN] Master election priority (range 1-4).
 *
 * \return VTSS_RC_OK if configuration was successful, else error code.
 */
mesa_rc vtss_appl_topo_mst_prio_set(vtss_usid_t usid, const vtss_appl_topo_mst_elect_prio_t *prio);

/**
 * Get master election priority
 *
 * \param usid [IN] Switch ID for switch to get configuration for.
 *
 * \param prio [OUT] Master election priority (range 1-4)
 *
 * \return VTSS_RC_OK if prio is valid, else error code.
 */
mesa_rc vtss_appl_topo_mst_prio_get(vtss_usid_t usid, vtss_appl_topo_mst_elect_prio_t *prio);

/**
 * Swap switch ids. Swap switch ID values, such that usida points to the switch that
 * usidb was previously referring to, and vice versa.
 *
 * \param usida [IN] Switch id.
 *
 * \param usidb [IN] Switch id.
 *
 *  \return VTSS_RC_OK if swap was successful, else error code.
 */
mesa_rc vtss_appl_topo_usid_swap(vtss_usid_t usida, const vtss_usid_t usidb);

/** Delete a switch ID assignment.
 *  Only allowed if corresponding switch is currently not present in stack.
 *
 * \param usid [IN] Switch ID for switch to delete.
 *
 * \return VTSS_RC_OK if deletion was successful, else error code.
 */
mesa_rc vtss_appl_topo_usid_assignment_del(const vtss_usid_t usid);

/** Get the Switch ID assigned to a given switch MAC Address.
 *
 * \param mac_addr [IN] MAC address.
 *
 * \param usid     [OUT] Pointer to where to put the switch id.
 *
 *  \return VTSS_RC_OK if usid is valid, else error code.
 */
mesa_rc vtss_appl_topo_usid_assignment_get(vtss_usid_t usid, mesa_mac_addr_t *mac_addr);

/** Assign a switch (identified by switch MAC address) to a Switch ID.
 *
 * When changing switch id for the switches in the stack, the vtss_appl_topo_usid_swap() function is normally used. 
 * In the case where a switch has been added to a stack, but could not be assigned a switch ID at detection time (e.g. due to 
 * switches which are no longer in the stack still have switch ID assigned), it might be necessary to assign the switch ID 
 * based on the MAC address.
 *
 * The following must be true in order to be able to assign the switch id.
 * 
 * 1) The switch ID must not already be assigned to another switch.
 *
 * 2) The switch must have been detected (the MAC address must be shown in the status).
 *
 * \param usid      [IN] New switch id.
 *
 * \param mac_addr  [IN] MAC address for the switch ID to which to assign the switch id.
 *
 *  \return VTSS_RC_OK if usid is valid, else error code.
 */
mesa_rc vtss_appl_topo_usid_assignment_set(const vtss_usid_t usid, const mesa_mac_addr_t mac_addr);

/*!
 * \brief Iterator for getting assigned switch ids. 
 *
 * \param prev [IN]  Pointer to previous switch MAC address. 
 *                   If NULL, the first MAC address is returned.
 *
 * \param next [OUT] The next valid MAC address larger than the previous MAC address.
 *
 * \return VTSS_RC_OK if next MAC address found else error return code.
 */
mesa_rc vtss_appl_topo_usid_assignment_itr(const mesa_mac_addr_t *prev, mesa_mac_addr_t *next);

/**
 * Get switch status/statistics.
 *
 * \param usid   [IN]  Switch ID for switch to get status/statistics for.
 *
 * \param stat_p [OUT] Pointer to where to put the status/statistics.
 *
 *  \return VTSS_RC_OK if stat_p is valid, else error code.
 */
mesa_rc vtss_appl_topo_switch_stat_get(const vtss_usid_t usid, vtss_appl_topo_switch_stat_t *const stat_p);

/**
 * Get master switch's status.
 *
 * \param stat_p [OUT] Pointer to where to put the status.
 *
 *  \return VTSS_RC_OK if stat_p is valid, else error code.
 */
mesa_rc vtss_appl_topo_mst_stat_get(vtss_appl_topo_mst_stat_t *const stat_p);
#ifdef __cplusplus
}
#endif

#endif  // _VTSS_APPL_TOPO_H_

