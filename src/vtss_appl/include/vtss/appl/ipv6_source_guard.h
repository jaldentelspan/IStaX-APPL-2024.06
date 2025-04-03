/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \brief Public Alarm API
 * \details This header file describes IPv6 Source Guard interface
 */

#include <vtss/basics/api_types.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/module_id.h>

#ifndef _VTSS_APPL_IPV6_SOURCE_GUARD_H_
#define _VTSS_APPL_IPV6_SOURCE_GUARD_H_

/* ---------------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif
/* ---------------------------------------------------------------------------- */

/**
 * \brief Maximum number of static and dynamic entries combined.
 */
#define IPV6_SOURCE_GUARD_MAX_ENTRY_CNT           (112) 

/**
 * \brief Value when number of dynamic entries per port is set to unlimited.
 */
#define IPV6_SOURCE_GUARD_DYNAMIC_UNLIMITED       0XFFFF

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    IPV6_SOURCE_GUARD_ERROR_INV_PARAMETER = MODULE_ERROR_START(VTSS_MODULE_ID_IPV6_SOURCE_GUARD),  /**< Illegal argument in function. */
    IPV6_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL,
    IPV6_SOURCE_GUARD_ERROR_PORT_CONFIGURE,
    IPV6_SOURCE_GUARD_ERROR_DEFAULT_CONFIGURE,
    IPV6_SOURCE_GUARD_ERROR_DEFAULT_ACE_RULE,
    IPV6_SOURCE_GUARD_ERROR_DEFAULT_ACE_RULE_DEL,
    IPV6_SOURCE_GUARD_ERROR_BINDING_TABLE_FULL,
    IPV6_SOURCE_GUARD_ERROR_DYNAMIC_MAX_REACHED,
    IPV6_SOURCE_GUARD_ERROR_DISABLED,
    IPV6_SOURCE_GUARD_ERROR_ENTRY_ALREADY_IN_TABLE,
    IPV6_SOURCE_GUARD_ERROR_STATIC_LINK_LOCAL,
    IPV6_SOURCE_GUARD_ERROR_STATIC_UNSPEC,
    IPV6_SOURCE_GUARD_ERROR_STATIC_LOOPBACK,
    IPV6_SOURCE_GUARD_ERROR_STATIC_MC
};

/* IPv6 source guard structs ----------------------------------------------------- */

/**
 * \brief Global configuration of IPv6 Source Guard.
 */
typedef struct {
    /**
     * \brief Enable or disable IPv6 Source Guard function in the system.
     */
    mesa_bool_t    enabled;
} vtss_appl_ipv6_source_guard_global_config_t;

/**
 * \brief Port configuration of IPv6 Source Guard.
 *  Key : ifindex.
 */
typedef struct {
    /**
     * \brief Port configuration indicates whether source guard is enabled
     * or disabled on this specific port.
     * TRUE means IPv6 Source Guard function is enabled on this specific port
     * and FALSE means it is disabled.
     */
    mesa_bool_t    enabled;

    /**
     * \brief Max number of allowed dynamic entries on this specific port.
     */
    uint32_t     max_dynamic_entries;
} vtss_appl_ipv6_source_guard_port_config_t;

/**
 * \brief Table index of IPv6 Source Guard binding tables.
 *  Key : ifindex, vlan_id, ipv6_addr.
 */
typedef struct {
    /** \brief Interface index of binding entry.*/
    vtss_ifindex_t                  ifindex;

    /** \brief IPv6 address and prefix size of binding entry.*/
    mesa_ipv6_t                     ipv6_addr;

    /** \brief VLAN ID of binding entry.*/
    mesa_vid_t                      vlan_id;

} vtss_appl_ipv6_source_guard_entry_index_t;

/**
 * \brief Data of IPv6 Source Guard binding tables.
 *  Table index is defined in vtss_appl_ipv6_source_guard_entry_index_t.
 */
typedef struct {
    /** \brief Assigned MAC address of binding entry.*/
    mesa_mac_t  mac_addr;

} vtss_appl_ipv6_source_guard_entry_data_t;

/**
 * \brief Control action of IPv6 source guard
 *  The action is to translate the dynamic entries to the static entries.
 */
typedef struct {
    /**
     * \brief An action to translate IPv6 source guard dynamic entries into
     * static entries. When get, it always returns FALSE.
     * If it is set to be TRUE, then do the action.
     */
    mesa_bool_t    translate;
} vtss_appl_ipv6_source_guard_control_translate_t;



/* Iterators -------------------------------------------------------------- */

/**
 * \brief Iterate through all ports where IPv6 Source Guard is enabled.
 * \param prev      [IN] Provide null-pointer to get the first ifindex,
 *                       otherwise a pointer to the current ifindex.
 * \param next      [OUT] First/next ifindex.
 * \return VTSS_RC_OK if the operation succeeded, 
 *         VTSS_RC_ERROR if no "next" ifinedx exists and the end has been reached.
 */
mesa_rc vtss_appl_ipv6_source_guard_port_conf_itr(
    const vtss_ifindex_t    *const prev,
    vtss_ifindex_t          *const next
);

/**
 * \brief Iterate through all the static binding entries.
 * \param prev      [IN] Provide null-pointer to get the first entry,
 *                       otherwise a pointer to the current entry.
 * \param next      [OUT] First/next table index.
 * \return VTSS_RC_OK if the operation succeeded
 *         VTSS_RC_ERROR if no "next" entry exists and the end has been reached.
 */
mesa_rc  vtss_appl_ipv6_source_guard_static_entry_itr(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const prev,
    vtss_appl_ipv6_source_guard_entry_index_t        *const next
);

/**
 * \brief Iterate through all the dynamic binding entries.
 * \param prev      [IN] Provide null-pointer to get the first entry,
 *                       otherwise a pointer to the current entry.
 * \param next      [OUT] First/next table index.
 * \return VTSS_RC_OK if the operation succeeded
 *         VTSS_RC_ERROR if no "next" entry exists and the end has been reached.
 */
mesa_rc vtss_appl_ipv6_source_guard_dynamic_entry_itr(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const prev,
    vtss_appl_ipv6_source_guard_entry_index_t        *const next
);

/* IPv6 source guard functions ----------------------------------------------------- */

/**
 * \brief Get global configuration of IPv6 Source Guard.
 * \param conf      [OUT] Global configuration of IPv6 Source Guard.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_global_config_get(
    vtss_appl_ipv6_source_guard_global_config_t    *const conf
);

/**
 * \brief Set or modify global configuration of IPv6 Source Guard.
 * \param conf      [IN] Global configuration of IPv6 Source Guard.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_global_config_set(
    const vtss_appl_ipv6_source_guard_global_config_t  *const conf
);

/**
 * \brief Get port configuration of IPv6 Source Guard.
 * \param ifindex   [IN] Interface index - the logical interface 
 *                       index of the physical port.
 * \param port_conf [OUT] The current configuration of the port.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_port_config_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_ipv6_source_guard_port_config_t  *const port_conf
);

/**
 * \brief Set or modify port configuration of IPv6 Source Guard.
 * \param ifindex   [IN] Interface index - the logical interface index
 *                       of the physical port.
 * \param port_conf [IN] The configuration set to the port.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_port_config_set(
    vtss_ifindex_t  ifindex,
    const vtss_appl_ipv6_source_guard_port_config_t   *const port_conf
);

/**
 * \brief Get static binding entry data of IPv6 Source Guard.
 * \param static_index   [IN] Table index of static binding entry.
 * \param static_data    [OUT] Data of static binding entry.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_static_entry_data_get(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index,
    vtss_appl_ipv6_source_guard_entry_data_t         *const static_data
);

/**
 * \brief Add or modify static binding entry of IPv6 Source Guard.
 * \param static_index   [IN] Table index of static binding entry.
 * \param static_data    [IN] Data of static binding entry.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_static_entry_set(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index,
    const vtss_appl_ipv6_source_guard_entry_data_t   *const static_data
);

/**
 * \brief Get default configuration for static binding entry of IPv6 Source Guard.
 * \param static_index  [OUT] The default table index of static binding entry.
 * \param static_data   [OUT] The default configuration of static binding entry.
 * \return VTSS_RC_OK for success operation, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_static_entry_default(
    vtss_appl_ipv6_source_guard_entry_index_t   *const static_index,
    vtss_appl_ipv6_source_guard_entry_data_t    *const static_data
);

/**
 * \brief Delete static binding entry of IPv6 Source Guard.
 * \param static_index   [IN] Table index of static binding entry.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_static_entry_del(
    const vtss_appl_ipv6_source_guard_entry_index_t  *const static_index
);

/**
 * \brief Get dynamic binding entry data of IPv6 Source Guard.
 * \param dynamic_index     [IN] Table index of dynamic binding entry.
 * \param dynamic_data      [OUT] Data of dynamic binding entry.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_dynamic_entry_data_get(
    const vtss_appl_ipv6_source_guard_entry_index_t *const dynamic_index,
    vtss_appl_ipv6_source_guard_entry_data_t        *const dynamic_data
);


/**
 * \brief Get control action of IPv6 Source Guard for translating dynamic entries to static.
 * This action is active only when SET is involved.
 * It always returns FALSE when getting this action data.
 * \param action    [OUT] The IPv6 source guard action data.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_control_translate_get(
    vtss_appl_ipv6_source_guard_control_translate_t   *const action
);

/**
 * \brief Set control action of IPv6 Source Guard for translating dynamic entries to static.
 * This action is active only when SET is involved and its value is set to be TRUE.
 * When it is active, it means translating process is taking action.
 * \param action    [IN] The IPv6 source guard action data.
 * \return VTSS_RC_OK if the operation succeeded, else error code.
 */
mesa_rc vtss_appl_ipv6_source_guard_control_translate_set(
    const vtss_appl_ipv6_source_guard_control_translate_t *const action
);



//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  // _VTSS_APPL_IPV6_SOURCE_GUARD_H_

