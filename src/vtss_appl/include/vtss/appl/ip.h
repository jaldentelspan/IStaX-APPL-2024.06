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

/**
 * \file
 * \brief Public IP API
 * \details This header file describes IP control functions and types
 */

#ifndef _VTSS_APPL_IP_H_
#define _VTSS_APPL_IP_H_

#include <microchip/ethernet/switch/api/types.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/module_id.h>
#include <vtss/basics/enum-descriptor.h>
#include <vtss/basics/enum_macros.hxx>
#include <vtss/basics/map.hxx>

/**
 * IP error codes (mesa_rc)
 */
enum {
    VTSS_APPL_IP_RC_INVALID_ARGUMENT = MODULE_ERROR_START(VTSS_MODULE_ID_IP),   /**< Invalid argument                                                         */
    VTSS_APPL_IP_RC_ROUTE_DEST_CONFLICT_WITH_LOCAL_IF,                          /**< Next-hop address conflicts with the IP(v4/v6) address of an IP interface */
    VTSS_APPL_IP_RC_OUT_OF_MEMORY,                                              /**< Out of memory                                                            */
    VTSS_APPL_IP_RC_INTERNAL_ERROR,                                             /**< Internal error.                                                          */
    VTSS_APPL_IP_RC_IPV6_NOT_SUPPORTED,                                         /**< IPv6 not supported.                                                      */
    VTSS_APPL_IP_RC_ROUTING_NOT_SUPPORTED,                                      /**< Routing not supported                                                    */
    VTSS_APPL_IP_RC_IF_NOT_FOUND,                                               /**< No such IP interface                                                     */
    VTSS_APPL_IP_RC_OS_IF_NOT_FOUND,                                            /**< No such OS interface                                                     */
    VTSS_APPL_IP_RC_IF_MTU_INVALID,                                             /**< Invalid MTU for IP interface                                             */
    VTSS_APPL_IP_RC_IF_NOT_USING_DHCP,                                          /**< IP interface is not using DHCP                                           */
    VTSS_APPL_IP_RC_IF_DHCP_UNSUPPORTED_FLAGS,                                  /**< Invalid DHCP flags set in mask                                           */
    VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID_IF_MAC,                                   /**< Invalid DHCP client ID interface MAC value                               */
    VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID_ASCII,                                    /**< Invalid DHCP client ID ASCII value                                       */
    VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID_HEX,                                      /**< Invalid DHCP client ID Hex value                                         */
    VTSS_APPL_IP_RC_IF_DHCP_CLIENT_ID,                                          /**< Invalid DHCP client ID                                                   */
    VTSS_APPL_IP_RC_IF_DHCP_CLIENT_HOSTNAME,                                    /**< Invalid DHCP client hostname                                             */
    VTSS_APPL_IP_RC_IF_DHCP_FALLBACK_TIMEOUT,                                   /**< Invalid DHCP fallback timeout                                            */
    VTSS_APPL_IP_RC_IF_INVALID_NETWORK_ADDRESS,                                 /**< Invalid IP address/network mask                                          */
    VTSS_APPL_IP_RC_IF_ADDRESS_CONFLICT_WITH_EXISTING,                          /**< IP address conflicts with existing interface address                     */
    VTSS_APPL_IP_RC_IF_ADDRESS_CONFLICT_WITH_STATIC_ROUTE,                      /**< IP address conflicts with existing IP address or static routing table    */
    VTSS_APPL_IP_RC_IF_ADDRESS_ALREADY_EXISTS_ON_ANOTHER_INTERFACE,             /**< IP address already exists on another interface                           */
    VTSS_APPL_IP_RC_IF_IFINDEX_MUST_BE_OF_TYPE_VLAN,                            /**< Interface index must be of type VLAN                                     */
    VTSS_APPL_IP_RC_IF_IFINDEX_MUST_BE_OF_TYPE_VLAN_OR_CPU,                     /**< Interface index must be of type VLAN or CPU                              */
    VTSS_APPL_IP_RC_IF_LIMIT_REACHED,                                           /**< Number of allowed IP interfaces reached                                  */
    VTSS_APPL_IP_RC_ROUTE_DOESNT_EXIST,                                         /**< Route does not exist                                                     */
    VTSS_APPL_IP_RC_ROUTE_TYPE_INVALID,                                         /**< Invalid route type                                                       */
    VTSS_APPL_IP_RC_ROUTE_PROTOCOL_INVALID,                                     /**< Invalid route protocol                                                   */
    VTSS_APPL_IP_RC_ROUTE_SUBNET_MUST_BE_UNICAST,                               /**< Route subnet must be unicast                                             */
    VTSS_APPL_IP_RC_ROUTE_SUBNET_PREFIX_SIZE,                                   /**< Invalid subnet prefix size                                               */
    VTSS_APPL_IP_RC_ROUTE_SUBNET_BITS_SET_OUTSIDE_OF_PREFIX,                    /**< Route subnet has bits set outside of prefix                              */
    VTSS_APPL_IP_RC_ROUTE_SUBNET_CONFLICTS_WITH_ACTIVE_IP,                      /**< Route subnet conflict with active IP address                             */
    VTSS_APPL_IP_RC_ROUTE_SUBNET_NOT_ROUTABLE,                                  /**< Route subnet is not routable.                                            */
    VTSS_APPL_IP_RC_ROUTE_DEST_MUST_NOT_BE_ZERO,                                /**< Route destination must not be all-zeros                                  */
    VTSS_APPL_IP_RC_ROUTE_DEST_MUST_BE_UNICAST,                                 /**< Route destination must be a unicast address                              */
    VTSS_APPL_IP_RC_ROUTE_DEST_VLAN_INVALID,                                    /**< Invalid VLAN specified for link-local address                            */
    VTSS_APPL_IP_RC_ROUTE_DEST_MUST_NOT_BE_LOOPBACK_OR_IPV4_FORM,               /**< Route destination must not be the loopback address                       */
    VTSS_APPL_IP_RC_ROUTE_DISTANCE_INVALID,                                     /**< Invalid route distance                                                   */
    VTSS_APPL_IP_RC_ROUTE_MAX_CNT_REACHED,                                      /**< Maximum number of static routes is reached                               */
};

/** \brief A collections of capability properties of the IP module. */
typedef struct {
    /** The device has IPv4 host capabilities. */
    mesa_bool_t has_ipv4_host_capabilities;

    /** The device has IPv6 host capabilities. */
    mesa_bool_t has_ipv6_host_capabilities;

    /** The device has IPv4 unicast routing capabilities. */
    mesa_bool_t has_ipv4_unicast_routing_capabilities;

    /** The device has IPv4 unicast hardware accelerated routing capabilities.
     * */
    mesa_bool_t has_ipv4_unicast_hw_routing_capabilities;

    /** The device has IPv6 unicast routing capabilities. */
    mesa_bool_t has_ipv6_unicast_routing_capabilities;

    /** The device has IPv6 unicast hardware accelerated routing capabilities.
     * */
    mesa_bool_t has_ipv6_unicast_hw_routing_capabilities;

    /** The maximum number of IP interfaces supported by the device. */
    uint32_t interface_cnt_max;

    /** The maximum number of static configured IP routes (shared for IPv4 and
     * IPv6) */
    uint32_t static_route_cnt_max;

    /** The amount of hardware LPM entries. */
    uint32_t lpm_hw_entry_cnt_max;
} vtss_appl_ip_capabilities_t;

/**
 * Get the capabilities of the device.
 *
 * \param cap [OUT] Buffer to receive the result in.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_capabilities_get(vtss_appl_ip_capabilities_t *cap);

/**
 * Global IP configuration.
 */
typedef struct {
    /**
     * Enable routing for all interfaces.
     * Notice that this may not be possible on all devices.
     * Check vtss_appl_ip_capabilities_t::has_ipv4_unicast_routing_capabilities
     */
    mesa_bool_t routing_enable;
} vtss_appl_ip_global_conf_t;

/**
 * Get global IP configuration.
 *
 * \param conf [OUT] Global parameters
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_global_conf_get(vtss_appl_ip_global_conf_t *conf);

/**
 * Set global IP configuration.
 *
 * \param conf [IN] Global configuration
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_global_conf_set(const vtss_appl_ip_global_conf_t *conf);

/**
 * Prefix for VLAN interfaces as used in Linux.
 * After this comes the VLAN ID.
 */
#define VTSS_APPL_IP_VLAN_IF_PREFIX "vtss.vlan."

/**
 * Configuration of IP interface
 */
typedef struct {
    /**
     * Select the maximum payload size.
     * Valid values are in range [1280; 1500]
     * Default is 1500.
     */
    uint32_t mtu;
} vtss_appl_ip_if_conf_t;

/**
 * Iterate through all IP interfaces.
 *
 * \param in                   [IN]  Pointer to current interface index. Provide a null pointer to get the first interface of a pointer to an ifindex initialized to VTSS_IFINDEX_NONE.
 * \param out                  [OUT] Next interface index (relative to the value provided in 'in').
 * \param vlan_interfaces_only [IN]  If true, only VLAN ifindices are returned. Otherwise also CPU interfaces may be returned.
 *
 * \return Error code. VTSS_RC_OK means that the value in out is valid,
 *                     VTSS_RC_ERROR means that no "next" interface index exists
 *                     and the end has been reached.
 */
mesa_rc vtss_appl_ip_if_itr(const vtss_ifindex_t *in, vtss_ifindex_t *out, mesa_bool_t vlan_interfaces_only = false);

/**
 * Get whether IP interface exists.
 *
 * \param ifindex [IN] Interface index to check existence of.
 *
 * \return true if IP interface exists, false if not.
 */
mesa_bool_t vtss_appl_ip_if_exists(vtss_ifindex_t ifindex);

/**
 * Get default IP interface configuration.
 *
 * \param default_conf [OUT] Default interface configuration.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_conf_default_get(vtss_appl_ip_if_conf_t *default_conf);

/**
 * Get IP interface configuration.
 *
 * Currently no general configuration is associated with an IP interface, and
 * this function can therefore be considered a dummy that just returns whether
 * the IP interface exists.
 *
 * See also vtss_appl_ip_if_exists().
 *
 * \param ifindex [IN]  Interface index to check existence of.
 * \param conf    [OUT] IP interface configuration to obtain.
 *
 * \return Returns VTSS_RC_OK if the IP interface exists and something else if
 * it doesn't.
 */
mesa_rc vtss_appl_ip_if_conf_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_conf_t *conf);

/**
 * Add an IP VLAN or CPU interface.
 *
 * If an IP interface does not exists for the given interface index, then create
 * one, otherwise do nothing.
 *
 * To configure the created IP interface with IPv4/IPv6 settings, see
 *    vtss_appl_ip_if_conf_ipv4_set()
 *    vtss_appl_ip_if_conf_ipv6_set()
 *
 * \param ifindex [IN] Interface index of type VLAN or CPU.
 * \param conf    [IN] IP interface configuration to set. May be nullptr to assign default value.
 *
 * \return VTSS_RC_OK if the IP interface already exists or it was created
 * successfully. Otherwise an error code.
 */
mesa_rc vtss_appl_ip_if_conf_set(vtss_ifindex_t ifindex, const vtss_appl_ip_if_conf_t *conf = nullptr);

/**
 * Delete an existing IP interface.
 *
 * \param ifindex [IN] Interface index of IP interface to delete.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_conf_del(vtss_ifindex_t ifindex);

/**
 * Flags to configure the DHCPv4 client
 */
typedef enum {
    VTSS_APPL_IP_DHCP4C_FLAG_NONE      = 0x0,  /**< This value may be used to clear all flags.                                    */
    VTSS_APPL_IP_DHCP4C_FLAG_OPTION_43 = 0x1,  /**< When transmitting the discover and request frames, option 43 (vendor
                                                *   specific information) will be requested in option 55 (Parameter Request List).*/
    VTSS_APPL_IP_DHCP4C_FLAG_OPTION_60 = 0x2,  /**< When transmitting the discover and request frames option 60 (vendor
                                                *   class identifier) will be included. The content of option 60 is
                                                *   implementation depended.                                                      */
    VTSS_APPL_IP_DHCP4C_FLAG_OPTION_67 = 0x4,  /**< When transmitting the discover and request frames, option 67 (boot file name)
                                                *   will be requested in option 55 (Parameter Request List).                      */
} vtss_appl_ip_dhcp4c_flags_t;

/**
 * Operators for flags of DHCPv4 client
 */
VTSS_ENUM_BITWISE(vtss_appl_ip_dhcp4c_flags_t);

/**
 * DHCPv4 client ID field types.
 */
typedef enum {
    VTSS_APPL_IP_DHCP4C_ID_TYPE_AUTO,   /**< Default option (for backward compatibility purposes). The value is hostname-sysmac[3]-sysmac[4]-sysmac[5] */
    VTSS_APPL_IP_DHCP4C_ID_TYPE_IF_MAC, /**< The client identifier is referring to interface MAC address                                               */
    VTSS_APPL_IP_DHCP4C_ID_TYPE_ASCII,  /**< The client identifier is ASCII string                                                                     */
    VTSS_APPL_IP_DHCP4C_ID_TYPE_HEX     /**< The client identifier is HEX value                                                                        */
} vtss_appl_ip_dhcp4c_id_type_t;

#define VTSS_APPL_IP_DHCP4C_ID_MIN_LENGTH  1 /**< Minimum length of client identifier using the DHCP client. */
#if VTSS_APPL_IP_DHCP4C_ID_MIN_LENGTH < 1
#error "The DHCP Client-identifier must have one octet or more (Refer to RFC 2132 9.14. Client-identifier)"
#endif
#define VTSS_APPL_IP_DHCP4C_ID_MAX_LENGTH         32 /**< Maximum length of client identifier using the DHCP client. */
#define VTSS_APPL_IP_DHCP4C_HOSTNAME_MAX_LENGTH   64 /**< Maximum length of hostname using the DHCP client. (including the null character) */

/**
 * Parameters to configure the DHCPv4 client ID field
 */
typedef struct {
    /**
     * Configure the type of DHCPv4 client ID.
     */
    vtss_appl_ip_dhcp4c_id_type_t type;

    /**
     * The interface's MAC address is taken when type is
     * VTSS_APPL_IP_DHCP4C_ID_TYPE_IF_MAC.
     */
    vtss_ifindex_t if_mac;

    /**
     * A unique ASCII string is taken when type is
     * VTSS_APPL_IP_DHCP4C_ID_TYPE_ASCII.
     *
     * The string must be null-terminated.
     */
    char ascii[VTSS_APPL_IP_DHCP4C_ID_MAX_LENGTH];

    /**
     * A unique hexadecimal value is taken when type is
     * VTSS_APPL_IP_DHCP4C_ID_TYPE_HEX.
     * One byte hex value is presented as two octets (*2), one more octet for
     * null character (+1)
     */
    char hex[VTSS_APPL_IP_DHCP4C_ID_MAX_LENGTH * 2 + 1];
} vtss_appl_ip_dhcp4c_id_t;

/**
 * Parameters to configure a DHCPv4 client
 */
typedef struct {
    /**
     * Configure the behavior of the DHCP client.
     */
    vtss_appl_ip_dhcp4c_flags_t dhcpc_flags;

    /**
     * Configure the hostname in the DHCP option 12 field.
     * If non-empty, it must be a valid null-terminated domain name.
     */
    char hostname[VTSS_APPL_IP_DHCP4C_HOSTNAME_MAX_LENGTH];

    /**
     * Configure the client identiifier in the DHCP option 61 field.
     * The interface specify which interface's MAC address is taken.
     */
    vtss_appl_ip_dhcp4c_id_t client_id;
} vtss_appl_ip_dhcp4c_param_t;

/**
 * IPv4 interface configuration.
 */
typedef struct {
    /**
     * Activate the IPv4 protocol stack.
     * In order to use the remaining parameters in this structure, this must be
     * set to true. In other words, the remaining parameters are ignored if this
     * is false.
     */
    mesa_bool_t enable;

    /**
     * Configure the interface using a DHCP client. Only used if \p enable is
     * also set.
     */
    mesa_bool_t dhcpc_enable;

    /**
     * Configure the parameters of the DHCP client.
     */
    vtss_appl_ip_dhcp4c_param_t dhcpc_params;

    /**
     * Static configured IP address and netmask. If the IPv4 protocol stack is
     * enabled and the DHCP client is not enabled, this address is used.
     * If the DHCP client is enabled, this address may be used as fallback.
     */
    mesa_ipv4_network_t network;

    /**
     * The DHCP client can be configured with a fallback timer. If no address
     * is provided by the DHCP server before timeout, the address configured in
     * \p network will be used as fallback.
     * This member enables the fallback mechanism.
     * The timeout is configured with \p fallback_timeout_secs.
     */
    mesa_bool_t fallback_enable;

    /**
     * The DHCP client can be configured with a fallback timer. If no address
     * has been provided before timeout, the address configured in \p network
     * will be used as fallback. To disable the fallback mechanism, configure
     * this value to zero, and the DHCP client will query for an address.
     *
     * Legal values are 1 to 4294967295 seconds, default being 60 seconds.
     */
    uint32_t fallback_timeout_secs;
} vtss_appl_ip_if_conf_ipv4_t;

/**
 * Get a default IPv4 interface configuration.
 *
 * \param conf [OUT] IPv4 default configuration.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_conf_ipv4_default_get(vtss_appl_ip_if_conf_ipv4_t *conf);

/**
 * Get the IPv4 configuration for a given IP interface.
 *
 * \param ifindex [IN]  Interface index of IP interface to get configuration for.
 * \param conf    [OUT] IPv4 configuration.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_conf_ipv4_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_conf_ipv4_t *conf);

/**
 * Set/update the IPv4 configuration for a given IP interface.
 *
 * \param ifindex [IN] Interface index of IP interface to set configuration for.
 * \param conf    [IN] IPv4 configuration.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_conf_ipv4_set(vtss_ifindex_t ifindex, const vtss_appl_ip_if_conf_ipv4_t *conf);

/**
 * IPv6 interface configuration.
 */
typedef struct {
    /**
     * Activate the IPv6 protocol stack.
     */
    mesa_bool_t enable;

    /**
     * Static configured IP address and netmask.
     */
    mesa_ipv6_network_t network;
} vtss_appl_ip_if_conf_ipv6_t;

/**
 * Get a default IPv6 interface configuration.
 *
 * \param conf [OUT] IPv6 default configuration.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_ipv6_conf_default_get(vtss_appl_ip_if_conf_ipv6_t *conf);

/**
 * Get the IPv6 configuration for a given IP interface.
 *
 * \param ifindex [IN]  Interface index of IP interface to get configuration for.
 * \param conf    [OUT] IPv6 configuration.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_conf_ipv6_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_conf_ipv6_t *conf);

/**
 * Set/update the IPv6 configuration for a given IP interface.
 *
 * \param ifindex [IN] Interface index of IP interface to set configuration for.
 * \param conf    [IN] IPv6 configuration.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_conf_ipv6_set(vtss_ifindex_t ifindex, const vtss_appl_ip_if_conf_ipv6_t *conf);

/**
 * Interface lower-layer flags
 */
typedef enum {
    VTSS_APPL_IP_IF_LINK_FLAG_NONE            = 0x000, /**< No flags                                                 */
    VTSS_APPL_IP_IF_LINK_FLAG_UP              = 0x001, /**< Interface is up                                          */
    VTSS_APPL_IP_IF_LINK_FLAG_BROADCAST       = 0x002, /**< Interface is capable of transmitting broadcast traffic   */
    VTSS_APPL_IP_IF_LINK_FLAG_LOOPBACK        = 0x004, /**< Interface is a loop-back interface                       */
    VTSS_APPL_IP_IF_LINK_FLAG_RUNNING         = 0x008, /**< Interface is running (according to the operating system) */
    VTSS_APPL_IP_IF_LINK_FLAG_NOARP           = 0x010, /**< Indicates if the interface will answer to ARP requests   */
    VTSS_APPL_IP_IF_LINK_FLAG_PROMISC         = 0x020, /**< Indicates if the interface is in promiscuous mode.       */
    VTSS_APPL_IP_IF_LINK_FLAG_MULTICAST       = 0x040, /**< Indicates if the interface supports multicast traffic    */
    VTSS_APPL_IP_IF_LINK_FLAG_IPV6_RA_MANAGED = 0x080, /**< Indicates the IPv6 Router Advertisement Managed flag     */
    VTSS_APPL_IP_IF_LINK_FLAG_IPV6_RA_OTHER   = 0x100, /**< Indicates the IPv6 Router Advertisement Other flag.      */
} vtss_appl_ip_if_link_flags_t;

/**
 * Operators for IP link flags
 */
VTSS_ENUM_BITWISE(vtss_appl_ip_if_link_flags_t);

/** Interface lower-layer status. */
typedef struct {
    /**
     * Interface index used by the underlying operating system.
     */
    uint32_t os_ifindex;

    /**
     * Maximum Transmission Unit size of the interface.
     */
    uint32_t mtu;

    /**
     * MAC-address of the interface.
     */
    mesa_mac_t mac;

    /**
     * L2 broadcast address
     */
    mesa_mac_t bcast;

    /**
     * Interface flags.
     */
    vtss_appl_ip_if_link_flags_t flags;
} vtss_appl_ip_if_status_link_t;

/**
 * Query a specific interface for link-layer status information.
 *
 * \param ifindex [IN] Interface index to query.
 * \param status [OUT] Buffer to receive the status information in.
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_status_link_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_status_link_t *status);

/**
 * IPv4 IP address information.
 */
typedef struct {
    /**
     * Broadcast address used by the interface.
     */
    mesa_ipv4_t broadcast;

    /**
     * Maximum re-assembly size.
     */
    uint32_t reasm_max_size;

    /**
     * ARP retransmission time.
     */
    uint32_t arp_retransmit_time;
} vtss_appl_ip_if_info_ipv4_t;

/**
 * Interface IPv4 address key
 */
typedef struct {
    /**
     * The IP interface of the IPv4 address
     */
    vtss_ifindex_t ifindex;

    /**
     * IP address
     */
    mesa_ipv4_network_t addr;
} vtss_appl_ip_if_key_ipv4_t;

/**
 * Interface IPv4 status.
 */
typedef struct {
    /**
     * The actual IP address and prefix size used by the interface.
     */
    mesa_ipv4_network_t net;

    /**
     * IPv4 address information.
     */
    vtss_appl_ip_if_info_ipv4_t info;
} vtss_appl_ip_if_status_ipv4_t;

/**
 * Iterate through all IP interfaces' IPv4 addresses.
 *
 * This function is meant to be used in conjunction with
 * vtss_appl_ip_if_status_ipv4_get() to obtain the status pertaining to a
 * particular ifindex and IPv4 address.
 *
 * \param in  [IN]  Pointer to current key. Provide a null pointer to get the first instance of IPv4 interface status.
 * \param out [OUT] Next key.
 *
 * \return Error code. VTSS_RC_OK means that \p out is valid.
 *                     VTSS_RC_ERROR means that the end has been reached.
 */
mesa_rc vtss_appl_ip_if_status_ipv4_itr(const vtss_appl_ip_if_key_ipv4_t *in, vtss_appl_ip_if_key_ipv4_t *out);

/**
 * Query a specific interface and IPv4 network for IPv4 status information.
 *
 * Use vtss_appl_ip_if_status_ipv4_itr() to get the ifindex and addr to query.
 * Upon successful return, status->net will be identical to \p addr.
 *
 * \param key    [IN]  Interface index and interface address to query.
 * \param status [OUT] Buffer to receive the status information in.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_status_ipv4_get(const vtss_appl_ip_if_key_ipv4_t *key, vtss_appl_ip_if_status_ipv4_t *status);

/**
 * Interface IPv6 address key
 */
typedef struct {
    /**
     * The IP interface of the IPv6 address
     */
    vtss_ifindex_t ifindex;

    /**
     * IP address
     */
    mesa_ipv6_network_t addr;
} vtss_appl_ip_if_key_ipv6_t;

/**
 * IPv6 IP address flags.
 */
typedef enum {
    VTSS_APPL_IP_IF_IPV6_FLAG_NONE       = 0x00000, /**< None value - useful for initializing.                                                        */
    VTSS_APPL_IP_IF_IPV6_FLAG_ANYCAST    = 0x00001, /**< Indicates if the address is an any-cast address.                                             */
    VTSS_APPL_IP_IF_IPV6_FLAG_TENTATIVE  = 0x00002, /**< An address whose uniqueness on a link is being verified, prior to its
                                                     *   assignment to an interface.  A tentative address is not considered
                                                     *   assigned to an interface in the usual sense.                                                 */
    VTSS_APPL_IP_IF_IPV6_FLAG_DUPLICATED = 0x00004, /**< Indicates the address duplication is detected by Duplicate Address Detection (a.k.a. DAD).
                                                     *   If the address is a link-local address formed from an interface
                                                     *   identifier based on the hardware address, which is supposed to be
                                                     *   uniquely assigned (e.g., EUI-64 for an Ethernet interface), IP operation
                                                     *   on the interface SHOULD be disabled.                                                         */
    VTSS_APPL_IP_IF_IPV6_FLAG_DETACHED   = 0x00008, /**< Indicates this address is ready to be detached from the link (IPv6 network).                 */
    VTSS_APPL_IP_IF_IPV6_FLAG_DEPRECATED = 0x00010, /**< An address assigned to an interface whose use is discouraged, but not forbidden.
                                                     *   A deprecated address should no longer be used as a source address in new communications,
                                                     *   but packets sent from or to deprecated addresses are delivered as expected.
                                                     *   A deprecated address may continue to be used as a source address in communications where
                                                     *   switching to a preferred address causes hardship to a specific upper-layer activity (e.g.,
                                                     *   an existing TCP connection)                                                                  */
    VTSS_APPL_IP_IF_IPV6_FLAG_NODAD      = 0x00020, /**< Indicates this address does not perform Duplicate Address Detection (a.k.a. DAD).            */
    VTSS_APPL_IP_IF_IPV6_FLAG_AUTOCONF   = 0x00040, /**< Indicates this address is capable of being retrieved by stateless address Autoconfiguration  */
    VTSS_APPL_IP_IF_IPV6_FLAG_TEMPORARY  = 0x00080, /**< Indicates this address is a temporary address.  A temporary address is used to reduce the
                                                      *   prospect of a user identity being permanently tied to an IPv6 address portion.
                                                      *   An IPv6 node may create temporary addresses with interface identifiers based on
                                                      *   time-varying random bit strings and relatively short lifetimes (hours to days). After that,
                                                      *   they are replaced with new addresses.                                                       */
    VTSS_APPL_IP_IF_IPV6_FLAG_DHCP       = 0x00100, /**< Indicates this address is a DHCPv6 address.                                                  */
    VTSS_APPL_IP_IF_IPV6_FLAG_STATIC     = 0x00200, /**< Indicates this address is a static managed address.                                          */
    VTSS_APPL_IP_IF_IPV6_FLAG_UP         = 0x10000, /**< Indicates this address is active.                                                            */
    VTSS_APPL_IP_IF_IPV6_FLAG_RUNNING    = 0x80000  /**< Indicates this address is working.                                                           */
} vtss_appl_ip_if_ipv6_flags_t;

/**
 * Operators for IPv6 address flags
 */
VTSS_ENUM_BITWISE(vtss_appl_ip_if_ipv6_flags_t);

/**
 * IPv6 IP address information.
 */
typedef struct {
    /**
     * IPv6 address flags.
     */
    vtss_appl_ip_if_ipv6_flags_t flags;

    /**
     * Interface index used by the underlaying operating system.
     */
    uint32_t os_ifindex;
} vtss_appl_ip_if_info_ipv6_t;

/**
 * Interface IPv6 status.
 */
typedef struct {
    /**
     * IPv6 address and network.
     */
    mesa_ipv6_network_t net;

    /**
     * IPv6 address information.
     */
    vtss_appl_ip_if_info_ipv6_t info;
} vtss_appl_ip_if_status_ipv6_t;

/**
 * Iterate through all IP interfaces' IPv6 addresses.
 *
 * This function is meant to be used in conjunction with
 * vtss_appl_ip_if_status_ipv6_get() to obtain the status pertaining to a
 * particular ifindex and IPv6 address.
 *
 * \param in  [IN]  Pointer to current key. Provide a null pointer to get the first instance of IPv6 interface status.
 * \param out [OUT] Next key.
 *
 * \return Error code. VTSS_RC_OK means that \p out is valid.
 *                     VTSS_RC_ERROR means that the end has been reached.
 */
mesa_rc vtss_appl_ip_if_status_ipv6_itr(const vtss_appl_ip_if_key_ipv6_t *in, vtss_appl_ip_if_key_ipv6_t *out);

/**
 * Query a specific interface and IPv6 network for IPv6 status information.
 *
 * Use vtss_appl_ip_if_status_ipv6_itr() to get the ifindex and addr to query.
 * Upon successful return, status->net will be identical to \p addr.
 *
 * \param key    [IN]  Interface index and address to query.
 * \param status [OUT] Buffer to receive the status information in.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_status_ipv6_get(const vtss_appl_ip_if_key_ipv6_t *key, vtss_appl_ip_if_status_ipv6_t *status);

/**
 * DHCP IPv4 client state. The states INIT, SELECTING, REQUESTING,
 * REBINDING, BOUND and RENEW correspond to the states of same name shown in
 * RFC2131 Figure 5.
 */
typedef enum {
    /** The DHCP client has been stopped. */
    VTSS_APPL_IP_DHCP4C_STATE_STOPPED,

    /** The DHCP client is being initialized. */
    VTSS_APPL_IP_DHCP4C_STATE_INIT,

    /** The DHCP client has send a discover, and is now collecting offers. */
    VTSS_APPL_IP_DHCP4C_STATE_SELECTING,

    /** The DHCP client either accepts or reject the offers, and depending on
     * the action it moves on to INIT or BOUND. */
    VTSS_APPL_IP_DHCP4C_STATE_REQUESTING,

    /** The DHCP client is in the process of rebinding after a renew broadcast
     * request has been send. */
    VTSS_APPL_IP_DHCP4C_STATE_REBINDING,

    /** The DHCP client hold a valid and accepted offer from a DHCP server. */
    VTSS_APPL_IP_DHCP4C_STATE_BOUND,

    /** The offer is about to timeout, and DHCP client will send a unicast renew
     * request. */
    VTSS_APPL_IP_DHCP4C_STATE_RENEWING,

    /** The DHCP client has given up, and will not try query for DHCP servers
     * any more, instead the interface has been configured with a fallback
     * address. */
    VTSS_APPL_IP_DHCP4C_STATE_FALLBACK,

    /** The DHCP client hold a valid and accepted offer from a DHCP server, but
     * it is not applied yet as the ARP check is still not completed. */
    VTSS_APPL_IP_DHCP4C_STATE_BOUND_ARP_CHECK,
} vtss_appl_ip_dhcp4c_state_t;

/**
 * DHCP for IPv4 client state information.
 */
typedef struct {
    /**
     * State of the DHCP clients state machine.
     */
    vtss_appl_ip_dhcp4c_state_t state;

    /**
     * DHCP client server address. Only valid if state is
     * DHCP4C_STATE_REBINDING, DHCP4C_STATE_BOUND or DHCP4C_STATE_RENEWING.
     */
    mesa_ipv4_t server_ip;
} vtss_appl_ip_if_status_dhcp4c_t;

/**
 * Query a specific interface for DHCP (IPv4) client status information.
 *
 * \param ifindex [IN]  Interface index to query.
 * \param status  [OUT] Buffer to receive the status information in.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_status_dhcp4c_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_status_dhcp4c_t *status);

/**
 * The following is for support of obtaining multiple status at a time for a
 * given interface. The functions that use it return an array of status items.
 *
 * Type of IP interface status to obtain.
 */
typedef enum {
    VTSS_APPL_IP_IF_STATUS_TYPE_ANY,    /**< Get info for any of the types */
    VTSS_APPL_IP_IF_STATUS_TYPE_LINK,   /**< Get OS Link status            */
    VTSS_APPL_IP_IF_STATUS_TYPE_IPV4,   /**< Get IPv4 status               */
    VTSS_APPL_IP_IF_STATUS_TYPE_IPV6,   /**< Get IPv6 status               */
    VTSS_APPL_IP_IF_STATUS_TYPE_DHCP4C, /**< Get DHCPv4 status             */
} vtss_appl_ip_if_status_type_t;

/**
 * Structure containing IP interface status of a particular type.
 */
typedef struct {
    /**
     * The interface this status pertains to.
     */
    vtss_ifindex_t ifindex;

    /**
     * The type of status.
     */
    vtss_appl_ip_if_status_type_t type;

    /**
     * Depending on \p type, that particular member is valid in the union below.
     */
    union {
        vtss_appl_ip_if_status_link_t   link;   /**< Valid if \p type == VTSS_APPL_IP_IF_STATUS_TYPE_LINK   */
        vtss_appl_ip_if_status_ipv4_t   ipv4;   /**< Valid if \p type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV4   */
        vtss_appl_ip_if_status_ipv6_t   ipv6;   /**< Valid if \p type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV6   */
        vtss_appl_ip_if_status_dhcp4c_t dhcp4c; /**< Valid if \p type == VTSS_APPL_IP_IF_STATUS_TYPE_DHCP4C */
    } u;
} vtss_appl_ip_if_status_t;

/**
 * Get status of a particular IP interface and possibly a particular type.
 *
 * If not at least one is found, an error is returned.
 *
 * The returned array of statuses is sorted by status type.
 *
 * Example:
 *   vtss_appl_ip_if_status_get(ifindex, VTSS_APPL_IP_IF_STATUS_TYPE_LINK, 1, nullptr, &status);
 *   is equivalent to this call:
 *     vtss_appl_ip_if_status_link_get(ifindex, &status);
 *   And
 *     vtss_appl_ip_if_status_get(ifindex, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 10, &cnt, &status);
 *   is equivalent to
 *     while (vtss_appl_ip_if_status_ipv4_itr(&key, &next_key) == VTSS_RC_OK &&
 *            cnt              <= 10                                         &&
 *            next_key.ifindex == ifindex) {
 *         cnt++;
 *         vtss_appl_ip_if_status_ipv4_get(ifindex, next_addr, &status);
 *     }
 *
 * \param ifindex [IN]  The IP interface to obtain status for.
 * \param type    [IN]  The type of status to get.
 * \param max     [IN]  Max number of items to return in \p status.
 * \param cnt     [OUT] Number of items returned in \p status. May be nullptr if max is 1, because the function returns error if not at least one found
 * \param status  [OUT] Array of at least \p max status elements.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_status_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_status_type_t type, uint32_t max, uint32_t *cnt, vtss_appl_ip_if_status_t *status);

/**
 * Get status of a particular IP address.
 *
 * This function looks for the IP address indicated by \p ip in the relevant
 * table (IPv4/IPv6) and if it finds it, the return code is VTSS_RC_OK and \p
 * status will be filled with the relevant data. Only status types
 * VTSS_APPL_IP_IF_STATUS_TYPE_IPV4 and VTSS_APPL_IP_IF_STATUS_TYPE_IPV6 can be
 * returned.
 *
 * \param ip     [IN] The IPv address and type to search for.
 * \param status [OUT] Status of the element.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_status_find(const mesa_ip_addr_t *ip, vtss_appl_ip_if_status_t *status);

/**
  * \brief Global notification status.
  *
  * This structure holds the current global notification status
  * which allows for SNMP Traps and JSON Notifications.
  */
typedef struct {
    /**
     * The H/W's routing table is of limited size. When all entries are in use
     * and one more is attempted added, there will be a mismatch between the
     * kernel's routing table (FIB) and the H/W's contents, so proper routing
     * can no longer be guaranteed.
     * If this occurs, the following member will be set to true and it is up to
     * the network administrator to reorganize the network or summarize routes.
     *
     * What makes this even worse is that it is hard to predict how many entries
     * the H/W table can hold, because it may contain both IPv4 and IPv6 routes
     * and IPv6 routes typically take more resources than IPv4 routes, and on
     * some platforms, also neighbor entries take up these resources.
     */
    mesa_bool_t hw_routing_table_depleted;
} vtss_appl_ip_global_notification_status_t;

/**
 * \brief Get global notification status.
 *
 * \param global_notif_status [OUT]: Pointer to structure receiving the current global notification status.
 *
 * \return VTSS_RC_OK on success.
 */
mesa_rc vtss_appl_ip_global_notification_status_get(vtss_appl_ip_global_notification_status_t *const global_notif_status);

/**
 * Interface statistics.
 */
typedef struct {
    /**
     * Number of packets received on the interface.
     */
    uint64_t in_packets;

    /**
     * Number of packets send on the interface.
     */
    uint64_t out_packets;

    /**
     * Number of bytes received on the interface.
     */
    uint64_t in_bytes;

    /**
     * Number of packets send on the interface.
     */
    uint64_t out_bytes;

    /**
     * Number multicast packets received on the interface.
     */
    uint64_t in_multicasts;

    /**
     * Number of multicast packets transmitted on the interface.
     */
    uint64_t out_multicasts;

    /**
     * Number of broadcast packets received on the interface.
     */
    uint64_t in_broadcasts;

    /**
     * Number of packets transmitted on the interface.
     */
    uint64_t out_broadcasts;
} vtss_appl_ip_if_statistics_t;

/**
 * Query a specific interface for lower-layer statistics.
 *
 * \param ifindex    [IN]  IP interface index to query. Only VLAN interfaces are supported.
 * \param statistics [OUT] Buffer to receive the statistics in.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_statistics_link_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_statistics_t *statistics);

/**
 * Clear interface counters.
 *
 * \param ifindex [IN] Interface index of IP interface to clear statistics for. Only VLAN interfaces are supported.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_statistics_link_clear(vtss_ifindex_t ifindex);

/**
 * IP statistics as defined by RFC 4293.
 */
typedef struct {
    /**
     * The total number of input IP datagrams received, including those
     * received in error.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InReceives;

    /**
     * The total number of input IP datagrams received, including those
     * received in error.  This object counts the same datagrams as InReceives,
     * but allows for larger values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCInReceives;

    /**
     * The total number of octets received in input IP datagrams, including
     * those received in error.  Octets from datagrams counted in InReceives
     * MUST be counted here.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InOctets;

    /**
     * The total number of octets received in input IP datagrams, including
     * those received in error.  This object counts the same octets as InOctets,
     * but allows for larger value.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCInOctets;

    /**
     * The number of input IP datagrams discarded due to errors in their IP
     * headers, including version number mismatch, other format errors, hop
     * count exceeded, errors discovered in processing their IP options, etc.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InHdrErrors;

    /**
     * The number of input IP datagrams discarded because no route could be
     * found to transmit them to their destination.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InNoRoutes;

    /**
     * The number of input IP datagrams discarded because the IP address in
     * their IP header's destination field was not a valid address to be
     * received at this entity.  This count includes invalid addresses (e.g.,
     * ::0).  For entities that are not IP routers and therefore do not forward
     * datagrams, this counter includes datagrams discarded because the
     * destination address was not a local address.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InAddrErrors;

    /**
     * The number of locally-addressed IP datagrams received successfully but
     * discarded because of an unknown or unsupported protocol.
     *
     * When tracking interface statistics, the counter of the interface to which
     * these datagrams were addressed is incremented.  This interface might not
     * be the same as the input interface for some of the datagrams.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InUnknownProtos;

    /**
     * The number of input IP datagrams discarded because the datagram frame
     * didn't carry enough data.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InTruncatedPkts;

    /**
     * The number of input datagrams for which this entity was not their final
     * IP destination and for which this entity attempted to find a route to
     * forward them to that final destination.
     *
     * When tracking interface statistics, the counter of the incoming interface
     * is incremented for each datagram.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InForwDatagrams;

    /**
     * The number of input datagrams for which this entity was not their final
     * IP destination and for which this entity attempted to find a route to
     * forward them to that final destination.  This object counts the same
     * packets as InForwDatagrams, but allows for larger values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCInForwDatagrams;

    /**
     * The number of IP fragments received that needed to be reassembled at
     * this interface.
     *
     * When tracking interface statistics, the counter of the interface to which
     * these fragments were addressed is incremented.  This interface might not
     * be the same as the input interface for some of the fragments.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t ReasmReqds;

    /**
     * The number of IP datagrams successfully reassembled.
     *
     * When tracking interface statistics, the counter of the interface to which
     * these datagrams were addressed is incremented.  This interface might not
     * be the same as the input interface for some of the datagrams.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t ReasmOKs;

    /**
     * The number of failures detected by the IP re-assembly algorithm (for
     * whatever reason: timed out, errors, etc.).  Note that this is not
     * necessarily a count of discarded IP fragments since some algorithms
     * (notably the algorithm in RFC 815) can lose track of the number of
     * fragments by combining them as they are received.
     *
     * When tracking interface statistics, the counter of the interface to which
     * these fragments were addressed is incremented.  This interface might not
     * be the same as the input interface for some of the fragments.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t ReasmFails;

    /**
     * The number of input IP datagrams for which no problems were encountered
     * to prevent their continued processing, but were discarded (e.g., for lack
     * of buffer space).  Note that this counter does not include any datagrams
     * discarded while awaiting re-assembly.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InDiscards;

    /**
     * The total number of datagrams successfully delivered to IP
     * user-protocols (including ICMP).
     *
     * When tracking interface statistics, the counter of the interface to which
     * these datagrams were addressed is incremented.  This interface might not
     * be the same as the input interface for some of the datagrams.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InDelivers;

    /**
     * The total number of datagrams successfully delivered to IP
     * user-protocols (including ICMP).  This object counts the same packets as
     * ipSystemStatsInDelivers, but allows for larger values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCInDelivers;

    /**
     * The total number of IP datagrams that local IP user-protocols (including
     * ICMP) supplied to IP in requests for transmission.  Note that this
     * counter does not include any datagrams counted in OutForwDatagrams.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutRequests;

    /**
     * The total number of IP datagrams that local IP user-protocols (including
     * ICMP) supplied to IP in requests for transmission.  This object counts
     * the same packets as OutRequests, but allows for larger values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCOutRequests;

    /**
     * The number of locally generated IP datagrams discarded because no route
     * could be found to transmit them to their destination.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutNoRoutes;

    /**
     * The number of datagrams for which this entity was not their final IP
     * destination and for which it was successful in finding a path to their
     * final destination.
     *
     * When tracking interface statistics, the counter of the outgoing interface
     * is incremented for a successfully forwarded datagram.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutForwDatagrams;

    /**
     * The number of datagrams for which this entity was not their final IP
     * destination and for which it was successful in finding a path to their
     * final destination.  This object counts the same packets as
     * OutForwDatagrams, but allows for larger values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCOutForwDatagrams;

    /**
     * The number of output IP datagrams for which no problem was encountered
     * to prevent their transmission to their destination, but were discarded
     * (e.g., for lack of buffer space).  Note that this counter would include
     * datagrams counted in OutForwDatagrams if any such datagrams met this
     * (discretionary) discard criterion.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutDiscards;

    /**
     * The number of IP datagrams that would require fragmentation in order to
     * be transmitted.
     *
     * When tracking interface statistics, the counter of the outgoing interface
     * is incremented for a successfully fragmented datagram.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutFragReqds;

    /**
     * The number of IP datagrams that have been successfully fragmented.
     *
     * When tracking interface statistics, the counter of the outgoing interface
     * is incremented for a successfully fragmented datagram.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutFragOKs;

    /**
     * The number of IP datagrams that have been discarded because they needed
     * to be fragmented but could not be.  This includes IPv4 packets that have
     * the DF bit set and IPv6 packets that are being forwarded and exceed the
     * outgoing link MTU.
     *
     * When tracking interface statistics, the counter of the outgoing interface
     * is incremented for an unsuccessfully fragmented datagram.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutFragFails;

    /**
     * The number of output datagram fragments that have been generated as a
     * result of IP fragmentation.
     *
     * When tracking interface statistics, the counter of the outgoing interface
     * is incremented for a successfully fragmented datagram.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutFragCreates;

    /**
     * The total number of IP datagrams that this entity supplied to the lower
     * layers for transmission.  This includes datagrams generated locally and
     * those forwarded by this entity.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutTransmits;

    /**
     * The total number of IP datagrams that this entity supplied to the lower
     * layers for transmission.  This object counts the same datagrams as
     * OutTransmits, but allows for larger values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCOutTransmits;

    /**
     * The total number of octets in IP datagrams delivered to the lower layers
     * for transmission.  Octets from datagrams counted in OutTransmits MUST be
     * counted here.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutOctets;

    /**
     * The total number of octets in IP datagrams delivered to the lower layers
     * for transmission.  This objects counts the same octets as OutOctets, but
     * allows for larger values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCOutOctets;

    /**
     * The number of IP multicast datagrams received.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InMcastPkts;

    /**
     * The number of IP multicast datagrams received.  This object counts the
     * same datagrams as InMcastPkts but allows for larger values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCInMcastPkts;

    /**
     * The total number of octets received in IP multicast datagrams.  Octets
     * from datagrams counted in InMcastPkts MUST be counted here.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InMcastOctets;

    /**
     * The total number of octets received in IP multicast datagrams.  This
     * object counts the same octets as InMcastOctets, but allows for larger
     * values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCInMcastOctets;

    /**
     * The number of IP multicast datagrams transmitted.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutMcastPkts;

    /**
     * The number of IP multicast datagrams transmitted.  This object counts
     * the same datagrams as OutMcastPkts, but allows for larger values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCOutMcastPkts;

    /**
     * The total number of octets transmitted in IP multicast datagrams.
     * Octets from datagrams counted in OutMcastPkts MUST be counted here.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutMcastOctets;

    /**
     * The total number of octets transmitted in IP multicast datagrams.  This
     * object counts the same octets as OutMcastOctets, but allows for larger
     * values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCOutMcastOctets;

    /**
     * The number of IP broadcast datagrams received.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t InBcastPkts;

    /**
     * The number of IP broadcast datagrams received.  This object counts the
     * same datagrams as InBcastPkts but allows for larger values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCInBcastPkts;

    /**
     * The number of IP broadcast datagrams transmitted.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint32_t OutBcastPkts;

    /**
     * The number of IP broadcast datagrams transmitted.  This object counts
     * the same datagrams as OutBcastPkts, but allows for larger values.
     *
     * Discontinuities in the value of this counter can occur at
     * re-initialization of the system, and at other times as indicated by the
     * value of DiscontinuityTime.
     */
    uint64_t HCOutBcastPkts;

    /**
     * The value of sysUpTime on the most recent occasion at which any one or
     * more of this entry's counters suffered a discontinuity.
     *
     * If no such discontinuities have occurred since the last re-initialization
     * of the IP stack, then this object contains a zero value.
     */
    mesa_timestamp_t DiscontinuityTime;

    /**
     * The minimum reasonable polling interval for this entry.  This object
     * provides an indication of the minimum amount of time required to update
     * the counters in this entry.
     */
    uint32_t RefreshRate;

    /**
     * Indicates if the InReceives field is valid
     */
    uint8_t InReceivesValid;

    /**
     * Indicates if the HCInReceives field is valid
     */
    uint8_t HCInReceivesValid;

    /**
     * Indicates if the InOctets field is valid
     */
    uint8_t InOctetsValid;

    /**
     * Indicates if the HCInOctets field is valid
     */
    uint8_t HCInOctetsValid;

    /**
     * Indicates if the InHdrErrors field is valid
     */
    uint8_t InHdrErrorsValid;

    /**
     * Indicates if the InNoRoutes field is valid
     */
    uint8_t InNoRoutesValid;

    /**
     * Indicates if the InAddrErrors field is valid
     */
    uint8_t InAddrErrorsValid;

    /**
     * Indicates if the InUnknownProtos field is valid
     */
    uint8_t InUnknownProtosValid;

    /**
     * Indicates if the InTruncatedPkts field is valid
     */
    uint8_t InTruncatedPktsValid;

    /**
     * Indicates if the InForwDatagrams field is valid
     */
    uint8_t InForwDatagramsValid;

    /**
     * Indicates if the HCInForwDatagrams field is valid
     */
    uint8_t HCInForwDatagramsValid;

    /**
     * Indicates if the ReasmReqds field is valid
     */
    uint8_t ReasmReqdsValid;

    /**
     * Indicates if the ReasmOKs field is valid
     */
    uint8_t ReasmOKsValid;

    /**
     * Indicates if the ReasmFails field is valid
     */
    uint8_t ReasmFailsValid;

    /**
     * Indicates if the InDiscards field is valid
     */
    uint8_t InDiscardsValid;

    /**
     * Indicates if the InDelivers field is valid
     */
    uint8_t InDeliversValid;

    /**
     * Indicates if the HCInDelivers field is valid
     */
    uint8_t HCInDeliversValid;

    /**
     * Indicates if the OutRequests field is valid
     */
    uint8_t OutRequestsValid;

    /**
     * Indicates if the HCOutRequests field is valid
     */
    uint8_t HCOutRequestsValid;

    /**
     * Indicates if the OutNoRoutes field is valid
     */
    uint8_t OutNoRoutesValid;

    /**
     * Indicates if the OutForwDatagrams field is valid
     */
    uint8_t OutForwDatagramsValid;

    /**
     * Indicates if the HCOutForwDatagrams field is valid
     */
    uint8_t HCOutForwDatagramsValid;

    /**
     * Indicates if the OutDiscards field is valid
     */
    uint8_t OutDiscardsValid;

    /**
     * Indicates if the OutFragReqds field is valid
     */
    uint8_t OutFragReqdsValid;

    /**
     * Indicates if the OutFragOKs field is valid
     */
    uint8_t OutFragOKsValid;

    /**
     * Indicates if the OutFragFails field is valid
     */
    uint8_t OutFragFailsValid;

    /**
     * Indicates if the OutFragCreates field is valid
     */
    uint8_t OutFragCreatesValid;

    /**
     * Indicates if the OutTransmits field is valid
     */
    uint8_t OutTransmitsValid;

    /**
     * Indicates if the HCOutTransmits field is valid
     */
    uint8_t HCOutTransmitsValid;

    /**
     * Indicates if the OutOctets field is valid
     */
    uint8_t OutOctetsValid;

    /**
     * Indicates if the HCOutOctets field is valid
     */
    uint8_t HCOutOctetsValid;

    /**
     * Indicates if the InMcastPkts field is valid
     */
    uint8_t InMcastPktsValid;

    /**
     * Indicates if the HCInMcastPkts field is valid
     */
    uint8_t HCInMcastPktsValid;

    /**
     * Indicates if the InMcastOctets field is valid
     */
    uint8_t InMcastOctetsValid;

    /**
     * Indicates if the HCInMcastOctets field is valid
     */
    uint8_t HCInMcastOctetsValid;

    /**
     * Indicates if the OutMcastPkts field is valid
     */
    uint8_t OutMcastPktsValid;

    /**
     * Indicates if the HCOutMcastPkts field is valid
     */
    uint8_t HCOutMcastPktsValid;

    /**
     * Indicates if the OutMcastOctets field is valid
     */
    uint8_t OutMcastOctetsValid;

    /**
     * Indicates if the HCOutMcastOctets field is valid
     */
    uint8_t HCOutMcastOctetsValid;

    /**
     * Indicates if the InBcastPkts field is valid
     */
    uint8_t InBcastPktsValid;

    /**
     * Indicates if the HCInBcastPkts field is valid
     */
    uint8_t HCInBcastPktsValid;

    /**
     * Indicates if the OutBcastPkts field is valid
     */
    uint8_t OutBcastPktsValid;

    /**
     * Indicates if the HCOutBcastPkts field is valid
     */
    uint8_t HCOutBcastPktsValid;

    /**
     * Indicates if the DiscontinuityTime field is valid
     */
    uint8_t DiscontinuityTimeValid;

    /**
     * Indicates if the RefreshRate field is valid
     */
    uint8_t RefreshRateValid;
} vtss_appl_ip_statistics_t;

/**
 * Query a specific interface for IPv4 statistics.
 *
 * OBSOLETE. DON'T USE. ALWAYS RETURNS VTSS_RC_ERROR!
 *
 * \param ifindex    [IN]  Interface index to query.
 * \param statistics [OUT] Buffer to receive the statistics in.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_statistics_ipv4_get(vtss_ifindex_t ifindex, vtss_appl_ip_statistics_t *statistics);

/**
 * Clear IPv4 interface counters
 *
 * OBSOLETE. DON'T USE. ALWAYS RETURNS VTSS_RC_ERROR!
 * You may want to use vtss_appl_ip_system_statistics_ipv4_get() instead.
 *
 * \param ifindex [IN] Interface index.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_statistics_ipv4_clear(vtss_ifindex_t ifindex);

/**
 * Query a specific interface for IPv6 statistics.
 *
 * \param ifindex    [IN]  Interface index to query.
 * \param statistics [OUT] Buffer to receive the statistics in.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_statistics_ipv6_get(vtss_ifindex_t ifindex, vtss_appl_ip_statistics_t *statistics);

/**
 * Clear interface IPv6 counters.
 *
 * \param ifindex [IN] Interface index.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_if_statistics_ipv6_clear(vtss_ifindex_t ifindex);

/**
 * Get system IPv4 statistics.
 *
 * \param statistics [OUT] Buffer to receive the statistics in.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_system_statistics_ipv4_get(vtss_appl_ip_statistics_t *statistics);

/**
 * Clear IPv4 system counters.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_system_statistics_ipv4_clear(void);

/**
 * Get system IPv6 statistics.
 *
 * \param statistics [OUT] Buffer to receive the statistics in.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_system_statistics_ipv6_get(vtss_appl_ip_statistics_t *statistics);

/**
 * Clear IPv6 system counters.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_system_statistics_ipv6_clear(void);

/**
 * Status for IPv4 Address Conflict Detection.
 */
typedef struct {
    /**
     * IP address
     */
    mesa_ipv4_t sip;

    /**
     * Source MAC address of conflicting node.
     */
    mesa_mac_t smac;
} vtss_appl_ip_acd_status_ipv4_key_t;

/**
 * Status for IPv4 Address Conflict Detection.
 */
typedef struct {
    /**
     * IP Interface index where the conflict was detected.
     */
    vtss_ifindex_t ifindex;

    /**
     * Interface index of port where the conflict was detected.
     */
    vtss_ifindex_t ifindex_port;
} vtss_appl_ip_acd_status_ipv4_t;

/**
 * Iterator to iterate through all the IPv4 Address Conflict entries.
 *
 * \param in  [IN]  The current entry key or NULL.
 * \param out [OUT] The next entry key.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_acd_status_ipv4_itr(const vtss_appl_ip_acd_status_ipv4_key_t *in, vtss_appl_ip_acd_status_ipv4_key_t *out);

/**
 * Get IPv4 Address Conflict entry.
 *
 * \param key    [IN]  Entry key.
 * \param status [OUT] Entry status.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_acd_status_ipv4_get(const vtss_appl_ip_acd_status_ipv4_key_t *key, vtss_appl_ip_acd_status_ipv4_t *status);

/**
 * \brief Clear IPv4 Address Conflict entry.
 *
 * \param key [IN] Key to delete. Provide a nullptr to delete all entries in one go.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_acd_status_ipv4_clear(const vtss_appl_ip_acd_status_ipv4_key_t *key = nullptr);

/**
 * Neighbor (ARP) flags.
 */
typedef enum {
    VTSS_APPL_IP_NEIGHBOR_FLAG_VALID     = 0x1, /**< The neighbor entry is valid.                                   */
    VTSS_APPL_IP_NEIGHBOR_FLAG_PERMANENT = 0x2, /**< The neighbor entry is marked as permanent.                     */
    VTSS_APPL_IP_NEIGHBOR_FLAG_ROUTER    = 0x4, /**< The neighbor is a router.                                      */
    VTSS_APPL_IP_NEIGHBOR_FLAG_HARDWARE  = 0x8, /**< The neighbor is present in the LPM table of a hardware router. */
} vtss_appl_ip_neighbor_flags_t;

/**
 * Operators for neighbor flags
 */
VTSS_ENUM_BITWISE(vtss_appl_ip_neighbor_flags_t);

/** \brief IPv6 neighbor state. */
typedef enum {
    /** TODO, Charles please update this. */
    VTSS_APPL_IP_NEIGHBOR_STATE_IPV6_NOSTATE = 0,

    /** TODO, Charles please update this. */
    VTSS_APPL_IP_NEIGHBOR_STATE_IPV6_INCOMPLETE = 1,

    /** TODO, Charles please update this. */
    VTSS_APPL_IP_NEIGHBOR_STATE_IPV6_REACHABLE = 2,

    /** TODO, Charles please update this. */
    VTSS_APPL_IP_NEIGHBOR_STATE_IPV6_STALE = 3,

    /** TODO, Charles please update this. */
    VTSS_APPL_IP_NEIGHBOR_STATE_IPV6_DELAY = 4,

    /** TODO, Charles please update this. */
    VTSS_APPL_IP_NEIGHBOR_STATE_IPV6_PROBE = 5,

    /** TODO, Charles please update this. */
    VTSS_APPL_IP_NEIGHBOR_STATE_IPV6_ERROR = 100,
} vtss_appl_ip_neighbor_state_ipv6_t;

/**
 * Operators for neighbor state
 */
VTSS_ENUM_BITWISE(vtss_appl_ip_neighbor_state_ipv6_t);

/**
 * IPv4/IPv6 neighbor key
 */
typedef struct {
    /**
     * Interface index of the interface where the neighbor can be reached.
     */
    vtss_ifindex_t ifindex;

    /**
     * IP address of the neighbor
     */
    mesa_ip_addr_t dip;
} vtss_appl_ip_neighbor_key_t;

/**
 * IPv4/IPv6 neighbor status.
 */
typedef struct {
    /**
     * Indicates whether this status is for an IPv4 or an IPv6 address.
     */
    mesa_ip_type_t type;

    /**
     * OS interface index corresponding to key's ifindex.
     */
    uint32_t os_ifindex;

    /**
     * MAC address of the neighbor.
     */
    mesa_mac_t dmac;

    /**
     * Interface index of the interface where the neighbor can be reached.
     */
    vtss_ifindex_t ifindex;

    /**
     * Neighbor flags.
     */
    vtss_appl_ip_neighbor_flags_t flags;

    /**
     * IPv6 neighbor state.
     */
    vtss_appl_ip_neighbor_state_ipv6_t state;

} vtss_appl_ip_neighbor_status_t;

/**
 * Iterator to iterate through all the IPv4/IPv6 neighbors.
 *
 * \param in   [IN]  Pointer to the current neighbor. Provide a NULL pointer to get the first IP address present in the neighbor table.
 * \param out  [ONT] Next IP address present in the neighbor table.
 * \param type [IN]  Select IP type to iterate across. By default, both IPv4 and IPv6 types are returned.
 *
 * \return VTSS_RC_OK if a next IP address was found, otherwise VTSS_RC_ERROR is
 *         returned to signal end of table.
 */
mesa_rc vtss_appl_ip_neighbor_itr(const vtss_appl_ip_neighbor_key_t *in, vtss_appl_ip_neighbor_key_t *out, mesa_ip_type_t type = MESA_IP_TYPE_NONE);

/**
 * Get neighbor status for a given IPv4 address.
 *
 * \param key    [IN]  IP address to get status for.
 * \param status [OUT] Neighbor status for the given IPv4 address.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_neighbor_status_get(const vtss_appl_ip_neighbor_key_t *key, vtss_appl_ip_neighbor_status_t *status);

/**
 * Clear the IPv4/IPv6 neighbor table.
 *
 * \param type [IN] Select whether to delete IPv4 or IPv6 neighbor table. Default is to delete both.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_neighbor_clear(mesa_ip_type_t type = MESA_IP_TYPE_NONE);

/**
 * Restart the DHCPv4 process.
 *
 * \param ifindex [IN] Interface to restart the DHCPv4 process on.
 *
 * \return Return code.
 */
mesa_rc vtss_appl_ip_if_dhcp4c_control_restart(vtss_ifindex_t ifindex);

/**
 * A route can either be a unicast IPv4 or IPv6 address.
 */
typedef enum {
    VTSS_APPL_IP_ROUTE_TYPE_ANY,     /**< Any route type. Only valid in calls to vtss_appl_ip_route_*_itr() */
    VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC, /**< Route is IPv4                                                     */
    VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC  /**< Route is IPv6                                                     */
} vtss_appl_ip_route_type_t;

/**
 * This represents a route. It is used as key in iterators and can hold both
 * IPv4 and IPv6 routes.
 */
typedef struct {
    /**
     * Is this an IPv4 or IPv6 route?
     */
    vtss_appl_ip_route_type_t type;

    /**
     * The members of this union each contain network, prefix, and next-hop
     * address for this route and are for IPv4 and IPv6, respectively.
     */
    union {
        /**
         * This one is used if #type is VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC.
         * It is a blackhole route if ipv4_uc::destination is all-ones.
         */
        mesa_ipv4_uc_t ipv4_uc;

        /**
         * This one is used if #type if VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC.
         * It is a blackhole route if ipv6_uc::destination.addr is all-ones.
         */
        mesa_ipv6_uc_t ipv6_uc;
    } route;

    /**
     * For IPv6 only:
     * If the next-hop address in 'route.ipv6_uc' is a link local address, the
     * VLAN interface of the next-hop address must be specified here. Otherwise
     * ignored.
     */
    vtss_ifindex_t vlan_ifindex;
} vtss_appl_ip_route_key_t;

/**
 * This represents the configuration of a particular static IPv4 or IPv6 unicast
 * route.
 */
typedef struct {
    /**
     * The distance value for this route.
     * Valid values are in range [1; 255]. Default being 1.
     * A distance of 255 disables the route.
     */
    uint8_t distance;
} vtss_appl_ip_route_conf_t;

/**
 * Get default route configuration.
 *
 * \param conf [OUT] Default route configuration
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_route_conf_default_get(vtss_appl_ip_route_conf_t *conf);

/**
 * Get configuration for a particular static IPv4/IPv6 unicast route.
 *
 * This function may also be used to test whether a given static route has
 * already been configured.
 * It returns VTSS_APPL_IP_RC_ROUTE_DOESNT_EXIST if it doesn't.
 *
 * \param key  [IN]  Route to get configuration for
 * \param conf [OUT] Route configuration
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_route_conf_get(const vtss_appl_ip_route_key_t *key, vtss_appl_ip_route_conf_t *conf);

/**
 * Create a static IPv4 or IPv6 unicast route.
 *
 * \param key  [IN] Route
 * \param conf [IN] Route's configuration
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_route_conf_set(const vtss_appl_ip_route_key_t *key, const vtss_appl_ip_route_conf_t *conf);

/**
 * Delete an existing static IPv4 or IPV6 unicast route.
 *
 * \param key [IN] Route to delete.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_route_conf_del(const vtss_appl_ip_route_key_t *key);

/**
 * Iterate across all static routes.
 *
 * To get started, set \p in to NULL and pass a valid pointer in \p out.
 * Prior to subsequent iterations, set in = out.
 *
 * \param in   [IN]  Previous configured route.
 * \param out  [OUT] Next configured route
 * \param type [IN]  Select which type of routes to iterate through. Default is to get all types.
 *
 * \return VTSS_RC_OK as long as next_key is OK.
 */
mesa_rc vtss_appl_ip_route_conf_itr(const vtss_appl_ip_route_key_t *in, vtss_appl_ip_route_key_t *out, vtss_appl_ip_route_type_t type = VTSS_APPL_IP_ROUTE_TYPE_ANY);

/**
 * The protocol that created/creates the route.
 */
typedef enum {
    VTSS_APPL_IP_ROUTE_PROTOCOL_KERNEL,    /**< The route protocol is created by the kernel (impossible). */
    VTSS_APPL_IP_ROUTE_PROTOCOL_DHCP,      /**< The route is created by DHCP client.                      */
    VTSS_APPL_IP_ROUTE_PROTOCOL_CONNECTED, /**< The destination network is connected directly.            */
    VTSS_APPL_IP_ROUTE_PROTOCOL_STATIC,    /**< The route is created by user.                             */
    VTSS_APPL_IP_ROUTE_PROTOCOL_OSPF,      /**< The route is created by OSPF.                             */
    VTSS_APPL_IP_ROUTE_PROTOCOL_RIP,       /**< The route is created by RIP.                              */
    VTSS_APPL_IP_ROUTE_PROTOCOL_ANY,       /**< Last enumeration. Must come last.                         */
} vtss_appl_ip_route_protocol_t;

/**
 * Route status flags
 */
typedef enum {
    VTSS_APPL_IP_ROUTE_STATUS_FLAG_SELECTED           = 0x001, /**< Route is selected, that is, it has at least one active nexthop */
    VTSS_APPL_IP_ROUTE_STATUS_FLAG_ACTIVE             = 0x002, /**< The I/F for the destination for this route is up               */
    VTSS_APPL_IP_ROUTE_STATUS_FLAG_FIB                = 0x004, /**< Route is installed in system's Forwarding Information Base     */
    VTSS_APPL_IP_ROUTE_STATUS_FLAG_DIRECTLY_CONNECTED = 0x008, /**< Route is directly connected                                    */
    VTSS_APPL_IP_ROUTE_STATUS_FLAG_ONLINK             = 0x010, /**< Nexthop is an interface                                        */
    VTSS_APPL_IP_ROUTE_STATUS_FLAG_DUPLICATE          = 0x020, /**< Duplicate route                                                */
    VTSS_APPL_IP_ROUTE_STATUS_FLAG_RECURSIVE          = 0x040, /**< Recursive route                                                */
    VTSS_APPL_IP_ROUTE_STATUS_FLAG_UNREACHABLE        = 0x080, /**< Nexthop is unreachable                                         */
    VTSS_APPL_IP_ROUTE_STATUS_FLAG_REJECT             = 0x100, /**< Nexthop is ICMP unreachable                                    */
    VTSS_APPL_IP_ROUTE_STATUS_FLAG_ADMIN_PROHIBITED   = 0x200, /**< Nexthop is ICMP admin-prohibited                               */
    VTSS_APPL_IP_ROUTE_STATUS_FLAG_BLACKHOLE          = 0x400, /**< Nexthop is a blackhole                                         */
} vtss_appl_ip_route_status_flags_t;

/**
 * Operators for nexthop flags
 */
VTSS_ENUM_BITWISE(vtss_appl_ip_route_status_flags_t);

/**
 * Route status key
 */
typedef struct {
    /**
     * This field describes the route.
     */
    vtss_appl_ip_route_key_t route;

    /**
     * The protocol that installed this route.
     */
    vtss_appl_ip_route_protocol_t protocol;
} vtss_appl_ip_route_status_key_t;

/**
 * Route status
 */
typedef struct {
    /**
     * The IP interface where the destination resides.
     */
    vtss_ifindex_t nexthop_ifindex;

    /**
     * Distance of the route.
     */
    uint8_t distance;

    /**
     * Metric of the route.
     */
    uint32_t metric;

    /**
     * Time - in seconds - since the route was created.
     */
    uint64_t uptime;

    /**
     * Flags indicating the route's status.
     */
    vtss_appl_ip_route_status_flags_t flags;

    /**
     * The interface index used by the operating system (used in e.g. "ip link"
     * shell command).
     */
    uint32_t os_ifindex;
} vtss_appl_ip_route_status_t;

/**
 * Get status of a IPv4/IPv6 route.
 *
 * Notice that this function and vtss_appl_ip_route_status_itr() caches all
 * routes, so that they are up to 10 seconds old.
 *
 * To get a whole, consistent set of routes, or to get only routes for a
 * particular protocol, use vtss_appl_ip_route_status_get_all().
 *
 * \param key    [IN]  Route to get status for
 * \param status [OUT] Route status
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_route_status_get(const vtss_appl_ip_route_status_key_t *key, vtss_appl_ip_route_status_t *status);

/**
 * Iterate across all routes
 *
 * To get started, set \p in to NULL and pass a valid pointer in \p out.
 * Prior to subsequent iterations, set in = out.
 *
 * Notice that this function and vtss_appl_ip_route_status_get() caches all
 * routes, so that they are up to 10 seconds old.
 *
 * To get a whole, consistent set of routes, or to get only routes for a
 * particular protocol, use vtss_appl_ip_route_status_get_all().
 *
 * \param in   [IN]  Previous route.
 * \param out  [OUT] Next route
 * \param type [IN]  Select which type of routes to iterate through. Default is to get all types.
 *
 * \return VTSS_RC_OK as long as next_key is OK.
 */
mesa_rc vtss_appl_ip_route_status_itr(const vtss_appl_ip_route_status_key_t *in, vtss_appl_ip_route_status_key_t *out, vtss_appl_ip_route_type_t type = VTSS_APPL_IP_ROUTE_TYPE_ANY);

/**
 * A type that simplifies using vtss_appl_ip_route_status_get_all()
 */
typedef vtss::Map<vtss_appl_ip_route_status_key_t, vtss_appl_ip_route_status_t> vtss_appl_ip_route_status_map_t;

/**
 * A type that simplifies iterating across a route map.
 */
typedef vtss_appl_ip_route_status_map_t::iterator vtss_appl_ip_route_status_map_itr_t;

/**
 * Get the status of IPv4 and/or IPv6 routes.
 *
 * The \p type parameter indicates the type of routes to get. Default is to get
 * both IPv4 and IPv6 routes.
 *
 * The \p protocol parameter can be used to filter the routes, so that only
 * routes installed by a particular protocol is returned. Default is to get all
 * routes.
 *
 * \param routes   [OUT] A map that eventually will hold the requested routes.
 * \param type     [IN]  Select IPv4 and/or IPv6 routes.
 * \param protocol [IN]  Select routes installed by a particular protocol.
 *
 * \return Error code.
 */
mesa_rc vtss_appl_ip_route_status_get_all(vtss_appl_ip_route_status_map_t &routes, vtss_appl_ip_route_type_t type = VTSS_APPL_IP_ROUTE_TYPE_ANY, vtss_appl_ip_route_protocol_t protocol = VTSS_APPL_IP_ROUTE_PROTOCOL_ANY);

#endif  // _VTSS_APPL_IP_H_
