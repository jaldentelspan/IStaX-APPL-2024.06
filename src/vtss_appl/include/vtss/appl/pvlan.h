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
 * \brief Public Private VLAN (PVLAN) API
 *
 * \details This header file describes Private VLAN and port isolation
 * facilities.
 *
 * Private VLANs are based on the source port masks, and have no connections to
 * 802.1Q VLANs. The private VLAN mask is applied after the normal L2
 * forwarding decisions, which means that a port must be configured according to
 * the normal L2 forwarding configuration (802.1Q and other) and a Private VLAN
 * to be able to forward packets.  The private VLAN ID is just an identifier,
 * and is not used by the switch. This is called Private VLAN Membership
 * Configuration.
 *
 * Example:
 *  - Do: Delete all private VLANs
 *
 *  - Observe: No ports can communicate!
 *
 *  - Do: Add private VLAN ID 2, with port mask 1, 2, 3
 *
 *  - Observe: Port 1-3 can communicate
 *  - Observe: No other ports can communicate
 *
 *  - Do: Add private VLAN ID 19, with port mask 1, 5, 6
 *
 *  - Observe: Port 1 can forward frames to port 2, 3, 5, 6
 *  - Observe: Port 2 can forward frames to port 1, 3
 *  - Observe: Port 3 can forward frames to port 1, 2
 *  - Observe: Port 5 can forward frames to port 1, 6
 *  - Observe: Port 6 can forward frames to port 1, 5
 *  - Observe: No other port can communicate
 *
 * Port isolation: Port isolation is a property which can be applied to a port.
 * Frames received on an isolated port may only be forwarded to non-isolated
 * ports. Frames received on a non-isolated port may be forwarded to any port.
 *
**/

#ifndef _VTSS_APPL_PVLAN_H_
#define _VTSS_APPL_PVLAN_H_

#include <vtss/appl/types.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Collection of capability properties of the PVLAN module. */
typedef struct {
    /** The capability to support PVLAN membership configuration by the device. */
    mesa_bool_t support_pvlan_membership_mgmt;
    /** The maximum VLAN ID of PVLAN membership configuration supported by the device. */
    uint32_t  max_pvlan_membership_vlan_id;
    /** The minimum VLAN ID of PVLAN membership configuration supported by the device. */
    uint32_t  min_pvlan_membership_vlan_id;
} vtss_appl_pvlan_capabilities_t;

/**
 * \brief PVLAN Membership configuration
 * The configuration is per PVLAN ID configuration that can manage the membership
 * of the specific PVLAN.
 */
typedef struct {
    /**
     * \brief member_ports is used to denote the memberships of the specific PVLAN.
     */
    vtss_port_list_stackable_t member_ports;
} vtss_appl_pvlan_membership_conf_t;

/**
 * \brief PVLAN Isolation configuration
 * The configuration is per port configuration that can manage the isolation among
 * the ports on the device.
 */
typedef struct {
    /**
     * \brief isolated is used to denote the isolation of the specific port in PVLAN.
     */
    mesa_bool_t isolated;
} vtss_appl_pvlan_isolation_conf_t;

/**
 * \brief Get the capabilities of PVLAN.
 *
 * \param cap       [OUT]   The capability properties of the PVLAN module.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_capabilities_get(vtss_appl_pvlan_capabilities_t *const cap);

/**
 * \brief Iterator for retrieving PVLAN membership table key/index
 *
 * To walk information (configuration) index of PVLAN membership table.
 *
 * \param prev      [IN]    PVLAN index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_membership_itr(const uint32_t *const prev, uint32_t *const next);

/**
 * \brief Iterator for retrieving PVLAN isolation table key/index
 *
 * To walk information (configuration) port index of PVLAN isolation table.
 *
 * \param prev      [IN]    Interface index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_isolation_itr(const vtss_ifindex_t *const prev, vtss_ifindex_t *const next);

/**
 * \brief Get PVLAN specific membership configuration
 *
 * To get configuration of the specific PVLAN membership entry.
 *
 * \param idx       [IN]    (key) PVLAN index - the index of PVLAN membership.
 *
 * \param entry     [OUT]   The current configuration of the specific PVLAN membership.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_membership_config_get(const uint32_t *const idx, vtss_appl_pvlan_membership_conf_t *const entry);

/**
 * \brief Add PVLAN specific membership configuration
 *
 * To create configuration of the specific PVLAN membership entry.
 *
 * \param idx       [IN]    (key) PVLAN index - the index of PVLAN membership.
 * \param entry     [IN]    The new configuration of the specific PVLAN membership entry.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_membership_config_add(const uint32_t *const idx, const vtss_appl_pvlan_membership_conf_t *const entry);

/**
 * \brief Set/Update PVLAN specific membership configuration
 *
 * To modify configuration of the specific PVLAN membership entry.
 *
 * \param idx       [IN]    (key) PVLAN index - the index of PVLAN membership.
 * \param entry     [IN]    The revised configuration of the specific PVLAN membership entry.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_membership_config_set(const uint32_t *const idx, const vtss_appl_pvlan_membership_conf_t *const entry);

/**
 * \brief Delete PVLAN specific membership configuration
 *
 * To remove configuration of the specific PVLAN membership entry.
 *
 * \param idx       [IN]    (key) PVLAN index - the index of PVLAN membership.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_membership_config_del(const uint32_t *const idx);

/**
 * \brief Get PVLAN port isolation configuration
 *
 * To get configuration of the PVLAN port isolation.
 *
 * \param ifindex   [IN]    (key) Port index - the port index of PVLAN isolation.
 *
 * \param entry     [OUT]   The current isolation configuration of the specific PVLAN port.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_isolation_config_get(const vtss_ifindex_t *const ifindex, vtss_appl_pvlan_isolation_conf_t *const entry);

/**
 * \brief Set/Update PVLAN port isolation configuration
 *
 * To modify configuration of the PVLAN port isolation.
 *
 * \param ifindex   [IN]    (key) Port index - the port index of PVLAN isolation.
 *
 * \param entry     [OUT]   The current isolation configuration of the specific PVLAN port.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_pvlan_isolation_config_set(const vtss_ifindex_t *const ifindex, const vtss_appl_pvlan_isolation_conf_t *const entry);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_PVLAN_H_ */
