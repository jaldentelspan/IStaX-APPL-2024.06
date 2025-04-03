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
 * \brief Public IP Source Guard API
 * \details This header file describes IP Source Guard control functions and types.
 */

#ifndef _VTSS_APPL_IP_SOURCE_GUARD_H_
#define _VTSS_APPL_IP_SOURCE_GUARD_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

/**
 * \brief Capabilities of IP Source Guard.
 */
typedef struct {
    /**
        *\brief Indicate if IP mask is supported at static binding table.
        * If not, it means IP mask is fixed at 255.255.255.255.
        */
    mesa_bool_t    static_ip_mask_supported;

    /**
        *\brief Indicate if MAC address is supported at static binding table.
        * If not, it means that user is not allowed to configure MAC address
        * for a static binding entry.
        */
    mesa_bool_t    static_mac_address_supported;

} vtss_appl_ip_source_guard_capabilities_t;


/**
 * \brief Global configuration of IP Source Guard.
 */
typedef struct {
    /**
     * \brief Enable or disable IP Source Guard function in the system.
     */
    mesa_bool_t    mode;
} vtss_appl_ip_source_guard_global_config_t;

/**
 * \brief Control action of IP source guard
 *  The action is to translate the dynamic entries to the static entries.
 */
typedef struct {
    /**
     * \brief An action to translate IP source guard dynamic entries into
     * static entries. When get, it always returns FALSE.
     * If it is set to be TRUE, then do the action.
     */
    mesa_bool_t    translate;
} vtss_appl_ip_source_guard_control_translate_t;

/**
 * \brief Port configuration of IP Source Guard.
 *  Key : ifindex.
 */
typedef struct {
    /**
     * \brief Mode of port configuration indicates whether function is enabled
     * or disabled on this specific port.
     * TRUE means IP Source Guard function is enabled on this specific port
     * and FALSE means it is disabled.
     */
    mesa_bool_t    mode;

    /**
     * \brief Max number of dynamic entries is allowed on this specific port.
     */
    uint32_t     dynamic_entry_count;
} vtss_appl_ip_source_guard_port_config_t;


/**
 * \brief Table index of IP Source Guard static binding table.
 *  Key : ifindex, vlan_id, ip_addr, ip_mask.
 */
typedef struct {
    /** \brief Interface index of static binding entry.*/
    vtss_ifindex_t                  ifindex;

    /** \brief IP address of static binding entry.*/
    mesa_ipv4_t                     ip_addr;

    /** \brief IP mask of static binding entry.*/
    mesa_ipv4_t                     ip_mask;

    /** \brief VLAN ID of static binding entry.*/
    mesa_vid_t                      vlan_id;
} vtss_appl_ip_source_guard_static_index_t;


/**
 * \brief Data of IP Source Guard static binding table.
 *  Table index is defined in vtss_appl_ip_source_guard_static_index_t.
 */
typedef struct {
    /** \brief Assigned MAC address of static binding entry.*/
    mesa_mac_t  mac_addr;
} vtss_appl_ip_source_guard_static_config_t;


/**
 * \brief Table index of IP Source Guard dynamic binding table.
 *  Key : ifindex, vlan_id, ip_addr.
 */
typedef struct {
    /** \brief Interface index of dynamic binding entry.*/
    vtss_ifindex_t                  ifindex;

    /** \brief IP address of dynamic binding entry.*/
    mesa_ipv4_t                     ip_addr;

    /** \brief VLAN ID of dynamic binding entry.*/
    mesa_vid_t                      vlan_id;
} vtss_appl_ip_source_guard_dynamic_index_t;


/**
 * \brief Data of IP Source Guard dynamic binding table.
 *  The entries of dynamic binding table can't be configured
 *  manually and are learned automatically from DHCP snooping.
 *  Table index is defined in vtss_appl_ip_source_guard_dynamic_index_t.
 */
typedef struct {
    /** \brief Learned MAC address of dynamic binding entry.*/
    mesa_mac_t  mac_addr;
} vtss_appl_ip_source_guard_dynamic_status_t;

/**
 * \brief Get capabilities of IP Source Guard which determines function is supported or not.
 *
 * \param cap [OUT] Capabilities of IP Source Guard.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_capabilities_get(
    vtss_appl_ip_source_guard_capabilities_t    *const cap
);

/**
 * \brief Get system configuration of IP Source Guard.
 *
 * \param conf [OUT] System configuration of IP Source Guard.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_system_config_get(
    vtss_appl_ip_source_guard_global_config_t    *const conf
);

/**
 * \brief Set or modify system configuration of IP Source Guard.
 *
 * \param conf [IN] System configuration of IP Source Guard.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_system_config_set(
    const vtss_appl_ip_source_guard_global_config_t  *const conf
);

/**
 * \brief Iterator function to allow iteration through all the port configuration.
 *
 * \param prev_ifindex [IN] Null-pointer to get first,
 *                                  otherwise the ifindex to get next of.
 * \param next_ifindex [OUT] First/next ifindex.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_port_config_itr(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);

/**
 * \brief Get port configuration of IP Source Guard.
 *
 * \param ifindex     [IN] Interface index - the logical interface
 *                                index of the physical port.
 * \param port_conf  [OUT] The current configuration of the port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_port_config_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_ip_source_guard_port_config_t  *const port_conf
);

/**
 * \brief Set or modify port configuration of IP Source Guard.
 *
 * \param ifindex  [IN] Interface index - the logical interface index
 *                            of the physical port.
 * \param port_conf [IN] The configuration set to the port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_port_config_set(
    vtss_ifindex_t  ifindex,
    const vtss_appl_ip_source_guard_port_config_t   *const port_conf
);


/**
 * \brief Iterator function to allow iteration through all the static binding entries.
 *
 * \param prev      [IN] Null-pointer to get first,
 *                             otherwise the table index to get next of.
 * \param next      [OUT] First/next table index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc  vtss_appl_ip_source_guard_static_config_itr(
    const vtss_appl_ip_source_guard_static_index_t  *const prev,
    vtss_appl_ip_source_guard_static_index_t        *const next
);


/**
 * \brief Get static binding entry of IP Source Guard.
 *
 * \param static_index   [IN] Table index of static binding entry.
 * \param static_conf    [OUT] Data of static binding entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_static_config_get(
    const vtss_appl_ip_source_guard_static_index_t  *const static_index,
    vtss_appl_ip_source_guard_static_config_t       *const static_conf
);


/**
 * \brief Add or modify static binding entry of IP Source Guard.
 *
 * \param static_index   [IN] Table index of static binding entry.
 * \param static_conf    [IN] Data of static binding entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_static_config_set(
    const vtss_appl_ip_source_guard_static_index_t  *const static_index,
    const vtss_appl_ip_source_guard_static_config_t *const static_conf
);

/**
 * \brief Delete static binding entry of IP Source Guard.
 *
 * \param static_index   [IN]    Table index of static binding entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_static_config_del(
    const vtss_appl_ip_source_guard_static_index_t  *const static_index
);

/**
 * \brief Get default configuration for static binding entry of IP Source Guard.
 *
 * \param static_index  [OUT]   The default table index of static binding entry.
 * \param static_conf   [OUT]   The default configuration of static binding entry.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_ip_source_guard_static_config_default(
    vtss_appl_ip_source_guard_static_index_t * const static_index,
    vtss_appl_ip_source_guard_static_config_t * const static_conf
);


/**
 * \brief Iterator function to allow iteration through all the dynamic binding entries.
 *
 * \param prev      [IN] Null-pointer to get first,
 *                             otherwise the table index to get next of.
 * \param next      [OUT] First/next table index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_dynamic_status_itr(
    const vtss_appl_ip_source_guard_dynamic_index_t  *const prev,
    vtss_appl_ip_source_guard_dynamic_index_t        *const next
);

/**
 * \brief Get dynamic binding entry of IP Source Guard.
 *
 * \param dynamic_index [IN] Table index of dynamic binding entry.
 * \param dynamic_status    [OUT] Data of dynamic binding entry.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_dynamic_status_get(
    const vtss_appl_ip_source_guard_dynamic_index_t *const dynamic_index,
    vtss_appl_ip_source_guard_dynamic_status_t      *const dynamic_status
);

/**
 * \brief Get control action of IP Source Guard for translating dynamic entries to static.
 *
 * This action is active only when SET is involved.
 * It always returns FALSE when get this action data.
 *
 * \param action [OUT] The IP source guard action data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_control_translate_get(
    vtss_appl_ip_source_guard_control_translate_t   *const action
);

/**
 * \brief Set control action of IP Source Guard for translating dynamic entries to static.
 *
 * This action is active only when SET is involved and its value is set to be TRUE.
 * When it is active, it means translating process is taken action.
 *
 * \param action [IN] The IP source guard action data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_ip_source_guard_control_translate_set(
    const vtss_appl_ip_source_guard_control_translate_t *const action
);

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_IP_SOURCE_GUARD_H_ */
