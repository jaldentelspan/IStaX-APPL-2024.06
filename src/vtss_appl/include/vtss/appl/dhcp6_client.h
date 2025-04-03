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
* \brief Public DHCPv6 Client API
* \details This header file describes DHCPv6 Client control functions and types.
* The configuration management of DHCPv6 client is associated with IP interface management.
* The public API for the IP module must be used to create/delete IP interface.
* A DHCPv6 client depends on an IP interface.
* This means that a DHCPv6 client can only be created in a given ifindex if the IP
* interface is created in advance.
* Once a DHCPv6 client interface is created, system starts DHCPv6 service on the
* specific IP interface.
* If IP interface with a corresponding DHCPv6 client is deleted, then both DHCPv6
* client and interface is deleted.
*/

#ifndef _VTSS_APPL_DHCP6_CLIENT_H_
#define _VTSS_APPL_DHCP6_CLIENT_H_

#include <vtss/appl/types.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief DHCPv6 Client Error Return Codes (mesa_rc)
 */
enum {
    VTSS_APPL_DHCP6C_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_DHCP6C),     /**< Generic DHCPv6 client error code */
    VTSS_APPL_DHCP6C_ERROR_PARM,                                                /**< Illegal parameter */
    VTSS_APPL_DHCP6C_ERROR_IPIF_NOT_EXISTING,                                   /**< The corresponding IP interface is invalid (e.g. not existing) */
    VTSS_APPL_DHCP6C_ERROR_TABLE_FULL,                                          /**< Table is full */
    VTSS_APPL_DHCP6C_ERROR_MEMORY_NG,                                           /**< Something wrong in memory allocation or free */

    VTSS_APPL_DHCP6C_ERROR_ENTRY_NOT_FOUND,                                     /**< Entry is not found */
    VTSS_APPL_DHCP6C_ERROR_ENTRY_INVALID,                                       /**< Invalid entry */
    VTSS_APPL_DHCP6C_ERROR_ENTRY_EXISTING,                                      /**< Existing entry */

    VTSS_APPL_DHCP6C_ERROR_PKT_GEN,                                             /**< Something wrong in processing DHCPv6 control frame */
    VTSS_APPL_DHCP6C_ERROR_PKT_CONTENT,                                         /**< Invalid packet content in DHCPv6 control frame */
    VTSS_APPL_DHCP6C_ERROR_PKT_FORMAT,                                          /**< Invalid packet format in DHCPv6 control frame */
    VTSS_APPL_DHCP6C_ERROR_PKT_ADDRESS,                                         /**< Invalid address used in DHCPv6 control frame */

    VTSS_APPL_DHCP6C_ERROR_UNKNOWN                                              /**< Unknown system errors in running DHCPv6 client */
};

/** \brief Collection of capability properties of the DHCPv6 client module. */
typedef struct {
    /** The maximum number of DHCPv6 client interfaces supported by the device. */
    uint32_t  max_number_of_interfaces;
} vtss_appl_dhcp6c_capabilities_t;

/**
 * \brief DHCPv6 Client interface configuration
 * The configuration is per IP interface configuration that can manage
 * the DHCPv6 client functions.
 */
typedef struct {
    /**
     * \brief Rapid commit option for DHCPv6 client, TRUE is to enable
     * rapid commit in DHCPv6 message exchange and FALSE is to disable it.
     */
    mesa_bool_t                                rapid_commit;
} vtss_appl_dhcp6c_intf_conf_t;

/**
 * \brief DHCPv6 Client interface counter
 * The DHCPv6 client interface counters from the running DHCPv6 client functions.
 */
typedef struct {
    /**
     * \brief Received DHCPv6 ADVERTISE message count.
     */
    uint32_t                                 rx_advertise;
    /**
     * \brief Received DHCPv6 REPLY message count.
     */
    uint32_t                                 rx_reply;
    /**
     * \brief Received DHCPv6 RECONFIGURE message count.
     */
    uint32_t                                 rx_reconfigure;
    /**
     * \brief Received DHCPv6 message error count.
     */
    uint32_t                                 rx_error;
    /**
     * \brief Received DHCPv6 message drop count.
     */
    uint32_t                                 rx_drop;
    /**
     * \brief Received DHCPv6 unknown message type count.
     */
    uint32_t                                 rx_unknown;
    /**
     * \brief Transmitted DHCPv6 SOLICIT message count.
     */
    uint32_t                                 tx_solicit;
    /**
     * \brief Transmitted DHCPv6 REQUEST message count.
     */
    uint32_t                                 tx_request;
    /**
     * \brief Transmitted DHCPv6 CONFIRM message count.
     */
    uint32_t                                 tx_confirm;
    /**
     * \brief Transmitted DHCPv6 RENEW message count.
     */
    uint32_t                                 tx_renew;
    /**
     * \brief Transmitted DHCPv6 REBIND message count.
     */
    uint32_t                                 tx_rebind;
    /**
     * \brief Transmitted DHCPv6 RELEASE message count.
     */
    uint32_t                                 tx_release;
    /**
     * \brief Transmitted DHCPv6 DECLINE message count.
     */
    uint32_t                                 tx_decline;
    /**
     * \brief Transmitted DHCPv6 INFORMATION-REQUEST message count.
     */
    uint32_t                                 tx_information_request;
    /**
     * \brief Transmitted DHCPv6 message error count.
     */
    uint32_t                                 tx_error;
    /**
     * \brief Transmitted DHCPv6 message drop count.
     */
    uint32_t                                 tx_drop;
    /**
     * \brief Transmitted DHCPv6 unknown message type count.
     */
    uint32_t                                 tx_unknown;
} vtss_appl_dhcp6c_intf_cntr_t;

/**
 * \brief DHCPv6 Client interface timer
 * The DHCPv6 client interface timers for the running DHCPv6 client functions.
 */
typedef struct {
    /**
     * \brief The recorded Preferred-Lifetime for the DHCPv6 client interface.
     * From RFC-4862 and RFC-3315:
     * It is the preferred lifetime for the IPv6 address, expressed in units of seconds.
     * When the preferred lifetime expires, the address becomes deprecated.
     */
    mesa_timestamp_t                    preferred_lifetime;
    /**
     * \brief The recorded Valid-Lifetime for the DHCPv6 client interface.
     * From RFC-4862 and RFC-3315:
     * It is the valid lifetime for the IPv6 address, expressed in units of seconds.
     * The valid lifetime must be greater than or equal to the preferred lifetime.
     * When the valid lifetime expires, the address becomes invalid.
     */
    mesa_timestamp_t                    valid_lifetime;
    /**
     * \brief The recorded T1 for the DHCPv6 client interface.
     * From RFC-3315:
     * It is the time at which the client contacts the server from which
     * the address is obtained to extend the lifetimes of the non-temporary
     * address assigned; T1 is a time duration relative to the current time
     * expressed in units of seconds.
     */
    mesa_timestamp_t                    t1;
    /**
     * \brief The recorded T2 for the DHCPv6 client interface.
     * From RFC-3315:
     * It is the time at which the client contacts any available server to
     * extend the lifetimes of the non-temporary address assigned; T2 is a
     * time duration relative to the current time expressed in units of seconds.
     */
    mesa_timestamp_t                    t2;
} vtss_appl_dhcp6c_intf_timer_t;

/**
 * \brief DHCPv6 Client interface status
 * The DHCPv6 client interface parameters, timers and counters from the running
 * DHCPv6 client functions.
 */
typedef struct {
    /**
     * \brief The IPv6 address determined from DHCPv6 for this interface.
     */
    mesa_ipv6_t                         address;
    /**
     * \brief The IPv6 address of the bounded DHCPv6 server for this interface.
     */
    mesa_ipv6_t                         srv_addr;
    /**
     * \brief The DNS server address retrieved from DHCPv6.
     */
    mesa_ipv6_t                         dns_srv_addr;
    /**
     * \brief Per DHCPv6 client interface's protocol timers.
     */
    vtss_appl_dhcp6c_intf_timer_t       timers;
    /**
     * \brief Per DHCPv6 client interface's statistics regarding message TX and RX.
     */
    vtss_appl_dhcp6c_intf_cntr_t        counters;
} vtss_appl_dhcp6c_interface_t;

/**
 * \brief Get the capabilities of DHCPv6 client.
 *
 * \param cap       [OUT]   The capability properties of the DHCPv6 client module.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_capabilities_get(vtss_appl_dhcp6c_capabilities_t *const cap);

/**
 * \brief Iterator for retrieving DHCPv6 client interface table key/index
 *
 * To walk information (configuration and status) index of the DHCPv6 client interface.
 *
 * \param prev      [IN]    Interface index to be used for indexing determination.
 *
 * \param next      [OUT]   The key/index should be used for the GET operation.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_itr(
    const vtss_ifindex_t                *const prev,
    vtss_ifindex_t                      *const next
);

/**
 * \brief Get DHCPv6 client IP interface default configuration
 *
 * To get default configuration of the DHCPv6 client interface.
 *
 * \param entry     [OUT]   The default configuration of the DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_config_default(
    vtss_appl_dhcp6c_intf_conf_t        *const entry
);

/**
 * \brief Get DHCPv6 client specific IP interface configuration
 *
 * To get configuration of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 *
 * \param entry     [OUT]   The current configuration of the specific DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_config_get(
    const vtss_ifindex_t                *const ifindex,
    vtss_appl_dhcp6c_intf_conf_t        *const entry
);

/**
 * \brief Add DHCPv6 client specific IP interface configuration
 *
 * To create configuration of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 * \param entry     [IN]    The new configuration of the specific DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.  VTSS_APPL_DHCP6C_ERROR_IPIF_NOT_EXISTING tells the
 * specific IP interface is not created in advance.
 */
mesa_rc vtss_appl_dhcp6c_interface_config_add(
    const vtss_ifindex_t                *const ifindex,
    const vtss_appl_dhcp6c_intf_conf_t  *const entry
);

/**
 * \brief Set/Update DHCPv6 client specific IP interface configuration
 *
 * To modify configuration of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 * \param entry     [IN]    The revised configuration of the specific DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_config_set(
    const vtss_ifindex_t                *const ifindex,
    const vtss_appl_dhcp6c_intf_conf_t  *const entry
);

/**
 * \brief Delete DHCPv6 client specific IP interface configuration
 *
 * To remove configuration of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_config_del(
    const vtss_ifindex_t                *const ifindex
);

/**
 * \brief Get DHCPv6 client specific IP interface status
 *
 * To get running status of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 *
 * \param entry     [OUT]   The running status of the specific DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_status_get(
    const vtss_ifindex_t                *const ifindex,
    vtss_appl_dhcp6c_interface_t        *const entry
);

/**
 * \brief Get DHCPv6 client specific IP interface statistics
 *
 * To get DHCPv6 control message statistics of the specific DHCPv6 client interface.
 *
 * \param ifindex   [IN]    (key) Interface index - the logical interface index of IP interface.
 *
 * \param entry     [OUT]   The packet counter of the specific DHCPv6 client interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_statistics_get(
    const vtss_ifindex_t                *const ifindex,
    vtss_appl_dhcp6c_intf_cntr_t        *const entry
);

/**
 * \brief Control action for restarting DHCPv6 client on a specific IP interface
 *
 * To restart the DHCPv6 client service on a specific IP interface.
 *
 * \param ifindex   [IN]    Interface index - the logical interface index of the IP interface.
 *
 * \return VTSS_RC_OK for success operation.
 */
mesa_rc vtss_appl_dhcp6c_interface_restart_act(
    const vtss_ifindex_t                *const ifindex
);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_DHCP6_CLIENT_H_ */
