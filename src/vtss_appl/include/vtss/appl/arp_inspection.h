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
* \brief Public ARP Inspection API
* \details This header file describes ARP Inspection control functions and types.
*/

#ifndef _VTSS_APPL_ARP_INSPECTION_H_
#define _VTSS_APPL_ARP_INSPECTION_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief ARP inspection log type */
typedef enum {
    VTSS_APPL_ARP_INSPECTION_LOG_NONE,      /**< Nothing to be logged */
    VTSS_APPL_ARP_INSPECTION_LOG_DENY,      /**< Log denied entry only */
    VTSS_APPL_ARP_INSPECTION_LOG_PERMIT,    /**< Log permitted entry only */
    VTSS_APPL_ARP_INSPECTION_LOG_ALL        /**< Log all kinds of entry */
} vtss_appl_arp_inspection_log_t;

/*! \brief ARP inspection entry type */
typedef enum {
    VTSS_APPL_ARP_INSPECTION_STATIC_TYPE,   /**< The entry is set staically */
    VTSS_APPL_ARP_INSPECTION_DYNAMIC_TYPE   /**< The entry is learnt dynamically */
} vtss_appl_arp_inspection_status_t;

/**
 * \brief ARP Inspection global configuration
 * The configuration is the system configuration that can enable/disable
 * the ARP Inspection function.
 */
typedef struct {
    /**
     * \brief Global administrative mode, TRUE is to enable ARP Inspection function
     * in the system and FALSE is to disable it.
     */
    mesa_bool_t                                    mode;
} vtss_appl_arp_inspection_conf_t;

/**
 * \brief ARP Inspection port configuration table
 * The configuration for each port, and this is a
 * static table for managing ARP inspection.
 * Key : IfIndex
 */
typedef struct {
    /**
     * \brief mode is to configure the mode of each port. TRUE is to
     * enable ARP Inspection function on the port and FALSE is to
     * disable it.
     */
    mesa_bool_t                                    mode;

    /**
     * \brief check_vlan is to enable/disable the VLAN checking. TRUE is to
     * include VLAN checking for the port and FALSE is to bypass VLAN
     * checking for the port.
     */
    mesa_bool_t                                    check_vlan;

    /**
     * \brief Per port basis, log_type is to used to decide how
     * the inspected ARP entry should be logged.
     */
    vtss_appl_arp_inspection_log_t          log_type;
} vtss_appl_arp_inspection_port_config_t;

/**
 * \brief ARP Inspection VLAN configuration table
 * The configuration for each VLAN, and this is a
 * dynamic table for managing ARP inspection.
 * Key : VlanId
 */
typedef struct {
    /**
     * \brief Per VLAN basis, log_type is used to decide how
     * the inspected ARP entry should be logged.
     */
    vtss_appl_arp_inspection_log_t          log_type;
} vtss_appl_arp_inspection_vlan_config_t;

/**
 * \brief ARP Inspection entry status
 * The static or dynamic entry managed by ARP inspection.
 * Key : IfIndex, VlanId, MacAddress, IpAddress
 */
typedef struct {
    /*! \brief To denote the specific ARP entry is learnt from dynamic or manually set */
    vtss_appl_arp_inspection_status_t       reg_status;
} vtss_appl_arp_inspection_entry_t;

/**
 * \brief Get ARP Inspection Parameters
 *
 * To read current system parameters in ARP Inspection.
 *
 * \param conf [OUT]    The ARP Inspection system configuration data.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc
vtss_appl_arp_inspection_system_config_get(
    vtss_appl_arp_inspection_conf_t         *const conf
);

/**
 * \brief Set ARP Inspection Parameters
 *
 * To modify current system parameters in ARP Inspection.
 *
 * \param conf [IN]     The ARP Inspection system configuration data.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc
vtss_appl_arp_inspection_system_config_set(
    const vtss_appl_arp_inspection_conf_t   *const conf
);

/**
 * \brief ARP Inspection Control ACTION
 *
 * Action flag to denote translating dynamic ARP entries to be static ones.
 * This flag is active only when SET is involved and its value is set to be TRUE.
 * When it is active, it means we should take action for converting all dynamic
 * entries to be static entries.
 *
 * \param act_flag  [IN]    The ARP Inspection action to be taken.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc
vtss_appl_arp_inspection_control_translate_dynamic_to_static_act(
    const mesa_bool_t                              *act_flag
);

/**
 * \brief Iterator for retrieving ARP Inspection port configuration key/index
 *
 * To walk configuration index of the port in ARP Inspection.
 *
 * \param prev      [IN]    Interface index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_arp_inspection_port_config_itr(
    const vtss_ifindex_t                    *const prev,
    vtss_ifindex_t                          *const next
);

/**
 * \brief Get ARP Inspection specific port configuration
 *
 * To read configuration of the specific port in ARP Inspection.
 *
 * \param ifindex   [IN]    (key)   Interface index - the logical interface
 *                                  index of the physical port.
 * \param port_conf [OUT]   The current configuration of the port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_port_config_get(
    vtss_ifindex_t                          ifindex,
    vtss_appl_arp_inspection_port_config_t  *const port_conf
);

/**
 * \brief Set ARP Inspection specific port configuration
 *
 * To modify configuration of the specific port in ARP Inspection.
 *
 * \param ifindex   [IN]    (key)   Interface index - the logical interface index
 *                                  of the physical port.
 * \param port_conf [IN]    The configuration set to the port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_port_config_set(
    vtss_ifindex_t                                  ifindex,
    const vtss_appl_arp_inspection_port_config_t    *const port_conf
);

/**
 * \brief Iterator for retrieving ARP Inspection VLAN configuration key/index
 *
 * To walk configuration index of the VLAN in ARP Inspection.
 *
 * \param prev      [IN]    VLAN ID to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_itr(
    const mesa_vid_t                        *const prev,
    mesa_vid_t                              *const next
);

/**
 * \brief Get ARP Inspection specific VLAN configuration
 *
 * To read configuration of the specific VLAN in ARP Inspection.
 *
 * \param vlan_id   [IN]    (key)   VLAN ID - VID of the VLAN.
 * \param vlan_conf [OUT]   The current configuration of the VLAN.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_get(
    mesa_vid_t                              vlan_id,
    vtss_appl_arp_inspection_vlan_config_t  *const vlan_conf
);

/**
 * \brief Set ARP Inspection specific VLAN configuration
 *
 * To modify configuration of the specific VLAN in ARP Inspection.
 *
 * \param vlan_id   [IN]    (key)   VLAN ID - VID of the VLAN.
 * \param vlan_conf [IN]    The configuration set to the VLAN.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_set(
    mesa_vid_t                                      vlan_id,
    const vtss_appl_arp_inspection_vlan_config_t    *const vlan_conf
);

/**
 * \brief Delete ARP Inspection specific VLAN configuration
 *
 * To delete configuration of the specific VLAN in ARP Inspection.
 *
 * \param vlan_id   [IN]    (key)   VLAN ID - VID of the VLAN.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_del(
    mesa_vid_t                              vlan_id
);

/**
 * \brief Add ARP Inspection specific VLAN configuration
 *
 * To add configuration of the specific VLAN in ARP Inspection.
 *
 * \param vlan_id   [IN]    (key)   VLAN ID - VID of the VLAN.
 * \param vlan_conf [IN]    The configuration set to the new created instance.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_add(
    mesa_vid_t                                      vlan_id,
    const vtss_appl_arp_inspection_vlan_config_t    *const vlan_conf
);

/**
 * \brief Get ARP Inspection default VLAN
 *
 * This is way to fill the RowEditor with values that are within
 * the expected range (e.g. VlanId between 1 - 4095). Without this,
 * fields are set to zero which is outside the expected range.
 *
 * \param vlan_id   [OUT]   (key)   VLAN ID - VID of the VLAN.
 * \param vlan_conf [OUT]   The current configuration of the VLAN.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_vlan_config_default (
    mesa_vid_t                              *const vlan_id,
    vtss_appl_arp_inspection_vlan_config_t  *const vlan_conf
);

/**
 * \brief Iterator for retrieving ARP Inspection static entry table key/index
 *
 * To walk configuration index of static entry table in ARP Inspection.
 *
 * \param ifx_prev  [IN]    Interface index to be used for indexing determination.
 * \param vid_prev  [IN]    VLAN ID to be used for indexing determination.
 * \param mac_prev  [IN]    MAC address to be used for indexing determination.
 * \param ipa_prev  [IN]    IPv4 address to be used for indexing determination.
 *
 * \param ifx_next  [OUT]   The key/index of Interface index should be used for the GET operation.
 * \param vid_next  [OUT]   The key/index of VLAN ID should be used for the GET operation.
 * \param mac_next  [OUT]   The key/index of MAC address should be used for the GET operation.
 * \param ipa_next  [OUT]   The key/index of IPv4 address should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *                          The precedence of IN key/index is in given sequential order.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_itr(
    const vtss_ifindex_t                    *const ifx_prev,
    vtss_ifindex_t                          *const ifx_next,
    const mesa_vid_t                        *const vid_prev,
    mesa_vid_t                              *const vid_next,
    const mesa_mac_t                        *const mac_prev,
    mesa_mac_t                              *const mac_next,
    const mesa_ipv4_t                       *const ipa_prev,
    mesa_ipv4_t                             *const ipa_next
);

/**
 * \brief Get ARP Inspection specific static entry configuration
 *
 * To read configuration of the specific static entry in ARP Inspection.
 *
 * \param ifindex   [IN]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [IN]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [IN]    (key 3) Assigned MAC address.
 * \param ip_addr   [IN]    (key 4) Assigned IPv4 address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_get(
    vtss_ifindex_t                          ifindex,
    mesa_vid_t                              vlan_id,
    mesa_mac_t                              mac_addr,
    mesa_ipv4_t                             ip_addr
);

/**
 * \brief Set ARP Inspection specific static entry configuration
 *
 * To modify configuration of the specific static entry in ARP Inspection.
 *
 * \param ifindex   [IN]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [IN]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [IN]    (key 3) Assigned MAC address.
 * \param ip_addr   [IN]    (key 4) Assigned IPv4 address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_set(
    vtss_ifindex_t                          ifindex,
    mesa_vid_t                              vlan_id,
    mesa_mac_t                              mac_addr,
    mesa_ipv4_t                             ip_addr
);

/**
 * \brief Delete ARP Inspection specific static entry configuration
 *
 * To delete configuration of the specific static entry in ARP Inspection.
 *
 * \param ifindex   [IN]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [IN]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [IN]    (key 3) Assigned MAC address.
 * \param ip_addr   [IN]    (key 4) Assigned IPv4 address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_del(
    vtss_ifindex_t                          ifindex,
    mesa_vid_t                              vlan_id,
    mesa_mac_t                              mac_addr,
    mesa_ipv4_t                             ip_addr
);

/**
 * \brief Add ARP Inspection specific static entry configuration
 *
 * To add configuration of the specific static entry in ARP Inspection.
 *
 * \param ifindex   [IN]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [IN]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [IN]    (key 3) Assigned MAC address.
 * \param ip_addr   [IN]    (key 4) Assigned IPv4 address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_add(
    vtss_ifindex_t                          ifindex,
    mesa_vid_t                              vlan_id,
    mesa_mac_t                              mac_addr,
    mesa_ipv4_t                             ip_addr
);

/**
 * \brief Get ARP Inspection static entry default VLAN
 *
 * This is way to fill the RowEditor with values that are within
 * the expected range (e.g. VlanId between 1 - 4095). Without this,
 * fields are set to zero which is outside the expected range.
 *
 * \param ifindex   [OUT]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [OUT]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [OUT]    (key 3) Assigned MAC address.
 * \param ip_addr   [OUT]    (key 4) Assigned IPv4 address.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_static_entry_default(
    vtss_ifindex_t                          *const ifindex,
    mesa_vid_t                              *const vlan_id,
    mesa_mac_t                              *const mac_addr,
    mesa_ipv4_t                             *const ip_addr
);

/**
 * \brief Iterator for retrieving ARP Inspection dynamic entry table key/index
 *
 * To walk configuration index of the dynamic entry table in ARP Inspection.
 *
 * \param ifx_prev  [IN]    Interface index to be used for indexing determination.
 * \param vid_prev  [IN]    VLAN ID to be used for indexing determination.
 * \param mac_prev  [IN]    MAC address to be used for indexing determination.
 * \param ipa_prev  [IN]    IPv4 address to be used for indexing determination.
 *
 * \param ifx_next  [OUT]   The key/index of Interface index should be used for the GET operation.
 * \param vid_next  [OUT]   The key/index of VLAN ID should be used for the GET operation.
 * \param mac_next  [OUT]   The key/index of MAC address should be used for the GET operation.
 * \param ipa_next  [OUT]   The key/index of IPv4 address should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *                          The precedence of IN key/index is in given sequential order.
 *
 * \return VTSS_RC_OK if the operation is successful.
 */
mesa_rc vtss_appl_arp_inspection_dynamic_entry_itr(
    const vtss_ifindex_t                    *const ifx_prev,
    vtss_ifindex_t                          *const ifx_next,
    const mesa_vid_t                        *const vid_prev,
    mesa_vid_t                              *const vid_next,
    const mesa_mac_t                        *const mac_prev,
    mesa_mac_t                              *const mac_next,
    const mesa_ipv4_t                       *const ipa_prev,
    mesa_ipv4_t                             *const ipa_next
);

/**
 * \brief Get ARP Inspection specific dynamic entry status
 *
 * To read the status of specific dynamic entry in ARP Inspection.
 *
 * \param ifindex   [IN]    (key 1) Interface index to be used of the physical port.
 * \param vlan_id   [IN]    (key 2) VLAN ID - VID of the VLAN.
 * \param mac_addr  [IN]    (key 3) Assigned MAC address.
 * \param ip_addr   [IN]    (key 4) Assigned IPv4 address.
 * \param entry     [OUT]   The current status of the dynamic entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_dynamic_entry_get(
    vtss_ifindex_t                          ifindex,
    mesa_vid_t                              vlan_id,
    mesa_mac_t                              mac_addr,
    mesa_ipv4_t                             ip_addr,
    vtss_appl_arp_inspection_entry_t        *const entry
);

/**
 * \brief ARP inspection event status
 */
typedef struct {
    /** Indicates the ARP inspection status is reached the
      * maximum entries or not. */
    mesa_bool_t    crossed_maximum_entries;
} vtss_appl_arp_inspection_status_event_t;

/**
 * \brief Get ARP Inspection event status.
 *
 * \param status     [OUT] ARP inspection event status.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_arp_inspection_status_event_get(
    vtss_appl_arp_inspection_status_event_t *const status
);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_ARP_INSPECTION_H_ */
