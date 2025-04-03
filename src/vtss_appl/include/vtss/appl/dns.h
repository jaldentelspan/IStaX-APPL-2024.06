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
* \brief Public DNS API.
* \details This header file describes DNS control functions and types.
* DNS stands for Domain Name System, which is a well known IP service
* that provides the mapping between IP host address(es) and name(s).
* There are two features supported: DNS client and DNS proxy.
* DNS client is set with server configuration to resolve host domain name in use.
* DNS proxy is set enabled or disabled for proxying DNS message exchange between
* DNS client and server.
*/

#ifndef _VTSS_APPL_DNS_H_
#define _VTSS_APPL_DNS_H_

#include <vtss/appl/types.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/interface.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTSS_APPL_DNS_MAX_STRING_LEN    254     /**< Maximum number of characters used for DNS configuration. */

/*! \brief DNS configuration (server or default domain name) administrative type. */
typedef enum {
    VTSS_APPL_DNS_CONFIG_TYPE_NONE      = 0,    /**< No DNS configuration is applied. */
    VTSS_APPL_DNS_CONFIG_TYPE_STATIC,           /**< DNS configuration is manually set. */
    VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY,        /**< DNS configuration is derived from DHCPv4. */
    VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN,       /**< DNS configuration is derived from DHCPv4 on specific VLAN. */
    VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY,        /**< DNS configuration is derived from DHCPv6. */
    VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN        /**< DNS configuration is derived from DHCPv6 on specific VLAN. */
} vtss_appl_dns_config_type_t;

/** \brief Collection of capability properties of the DNS module. */
typedef struct {
    /** The capability to support setting DNS server from DHCPv4. */
    mesa_bool_t                                support_dhcp4_config_server;
    /** The capability to support setting DNS server from DHCPv6. */
    mesa_bool_t                                support_dhcp6_config_server;
    /** The capability to support setting default domain name. */
    mesa_bool_t                                support_default_domain_name;
    /** The capability to support setting default domain name from DHCPv4. */
    mesa_bool_t                                support_dhcp4_domain_name;
    /** The capability to support setting default domain name from DHCPv6. */
    mesa_bool_t                                support_dhcp6_domain_name;
    /** The capability to support setting Multicast/Anycast/Linklocal DNS server. */
    mesa_bool_t                                support_mcast_anycast_ll_dns;

    /** Maximum number of nameservers */
    uint32_t ns_cnt_max;
} vtss_appl_dns_capabilities_t;

/**
 * \brief DNS proxy configuration.
 *  This configuration provides DNS proxy settings.
 */
typedef struct {
    /**
     * \brief proxy_admin_state is used to enable/disable DNS-Proxy feature. TRUE is to
     * enable DNS-Proxy function and FALSE is to disable it.
     */
    mesa_bool_t                                proxy_admin_state;
} vtss_appl_dns_proxy_conf_t;

/**
 * \brief DNS default domain name configuration.
 *  This configuration provides default domain name settings.
 */
typedef struct {
    /**
     * \brief Global administrative default domain name type.
     * A default domain name is used as the suffix of the given name in DNS lookup.
     * NONE(0) means no default domain name is used and thus no domain name suffix
     * is appended in DNS lookup.
     * DHCP4_ANY(2) means default domain name will be determined by DHCPv4 discovery.
     * DHCP6_ANY(4) means default domain name will be determined by DHCPv6 discovery.
     * These three modes can be set directly without considering the following two
     * attributes: 'static_domain_name' & 'dhcp_ifindex'.\n
     * STATIC(1) means the default domain name will be manually set.
     * When this mode is applied, 'static_domain_name' should also be assigned accordingly.\n
     * DHCP4_VLAN(3) means default domain name will be determined by DHCPv4 discovery on a
     * specific IP VLAN interface.
     * DHCP6_VLAN(5) means default domain name will be determined by DHCPv6 discovery on a
     * specific IP VLAN interface.
     * When these two modes are applied, 'dhcp_ifindex' should also be assigned accordingly.\n
     */
    vtss_appl_dns_config_type_t         domainname_type;

    /**
     * \brief dhcp_ifindex is used to denote the ifIndex of specific VLAN interface that
     * default domain name will be retrieved from DHCP. It will be a reference only when
     * 'domainname_type' is either DHCP4_VLAN(3) or DHCP6_VLAN(5).
     */
    vtss_ifindex_t                      dhcp_ifindex;

    /**
     * \brief static_domain_name is used to denote the static default domain name.
     * It will be a reference only when 'domainname_type' is STATIC(1).
     */
    char                                static_domain_name[VTSS_APPL_DNS_MAX_STRING_LEN + 1];
} vtss_appl_dns_name_conf_t;

/**
 * \brief DNS server configuration.
 *  The configuration is the DNS server setting used to determine the IP address of a
 *  domain name.
 */
typedef struct {
    /**
     * \brief Per precedence administrative DNS server type.
     * The DNS server setting will be used in DNS lookup.
     * NONE(0) means no DNS server is used and thus domain name lookup always fails.
     * DHCP4_ANY(2) means DNS server address will be determined by DHCPv4 discovery.
     * DHCP6_ANY(4) means DNS server address will be determined by DHCPv6 discovery.
     * These three modes can be set directly without considering the following two
     * attributes: 'static_ip_address' & 'static_ifindex'.\n
     * STATIC(1) means the DNS server address will be manually set, in IPv4 or IPv6 address form.
     * When this mode is applied, 'static_ip_address' should also be assigned accordingly.\n
     * DHCP4_VLAN(3) means DNS server address will be determined by DHCPv4 discovery on a specifc
     * IP VLAN interface.
     * DHCP6_VLAN(5) means DNS server address will be determined by DHCPv6 discovery on a specifc
     * IP VLAN interface.
     * When these two modes are applied, 'static_ifindex' should also be assigned accordingly.\n
     */
    vtss_appl_dns_config_type_t         server_type;

    /**
     * \brief static_ip_address is used to denote the static DNS server address.
     * It will be a reference only when 'server_type' is STATIC(1).
     */
    mesa_ip_addr_t                      static_ip_address;

    /**
     * \brief static_ifindex is used to denote the ifIndex of specific VLAN interface that
     * DNS server address will be retrieved from DHCP and where the server resides. It will
     * be a reference only when 'server_type' is either DHCP4_VLAN(3) or DHCP6_VLAN(5).
     */
    vtss_ifindex_t                      static_ifindex;
} vtss_appl_dns_server_conf_t;

/**
 * \brief System DNS active default domain name status.
 *  It provides the default domain name information.
 */
typedef struct {
    /**
     * \brief default_domain_name is the suffix of the given domain name
     * used in DNS lookup.
     */
    char                                default_domain_name[VTSS_APPL_DNS_MAX_STRING_LEN + 1];
} vtss_appl_dns_domainname_status_t;

/**
 * \brief System DNS configured server status.
 *  It provides the configred DNS server information used to determine the IP
 *  address of a domain name.
 */
typedef struct {
    /**
     * \brief Per precedence configured DNS server type.
     * NONE(0) means no DNS server is used and thus domain name lookup always fails.
     * STATIC(1) means the DNS server address will be manually set, in IPv4 or IPv6 address form.
     * DHCP4_ANY(2) means DNS server address will be determined by DHCPv4 discovery.
     * DHCP4_VLAN(3) means DNS server address will be determined by DHCPv4 discovery on a specifc
     * IP VLAN interface.
     * DHCP6_ANY(4) means DNS server address will be determined by DHCPv6 discovery.
     * DHCP6_VLAN(5) means DNS server address will be determined by DHCPv6 discovery on a specifc
     * IP VLAN interface.
     */
    vtss_appl_dns_config_type_t         configured_type;

    /**
     * \brief reference_ifindex is used to denote the ifIndex of specific VLAN interface that
     * DNS server address will be used. It will be a reference only when 'configured_type' is
     * either DHCP4_VLAN(3) or DHCP6_VLAN(5).
     */
    vtss_ifindex_t                      reference_ifindex;

    /**
     * \brief ip_address is the DNS server address that will be used for domain name lookup.
     */
    mesa_ip_addr_t                      ip_address;
} vtss_appl_dns_server_status_t;

/**
 * \brief Get the capabilities of DNS.
 *
 * \param cap       [OUT]   The capability properties of the DNS module.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_capabilities_get(
    vtss_appl_dns_capabilities_t        *const cap
);

/**
 * \brief Get DNS proxy default configuration.
 *
 * Get default configuration of the DNS proxy settings.
 *
 * \param entry     [OUT]   The default configuration of DNS proxy settings.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_proxy_config_default(
    vtss_appl_dns_proxy_conf_t          *const entry
);

/**
 * \brief Get DNS proxy configuration.
 *
 * To read current DNS proxy settings.
 *
 * \param entry     [OUT]   The current DNS proxy configuration data.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_proxy_config_get(
    vtss_appl_dns_proxy_conf_t          *const entry
);

/**
 * \brief Set DNS proxy configuration.
 *
 * To modify current DNS proxy settings.
 *
 * \param entry     [IN]    The revised DNS proxy configuration data.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_proxy_config_set(
    const vtss_appl_dns_proxy_conf_t    *const entry
);

/**
 * \brief Get DNS default domain name default configuration.
 *
 * Get default configuration of the DNS default domain name settings.
 *
 * \param entry     [OUT]   The default configuration of DNS default domain name settings.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_domain_name_config_default(
    vtss_appl_dns_name_conf_t           *const entry
);

/**
 * \brief Get DNS default domain name configuration.
 *
 * To read current DNS default domain name settings.
 *
 * \param entry     [OUT]   The current DNS default domain name configuration data.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_domain_name_config_get(
    vtss_appl_dns_name_conf_t           *const entry
);

/**
 * \brief Set DNS default domain name configuration.
 *
 * To modify current DNS default domain name settings.
 *
 * \param entry     [IN]    The revised DNS default domain name configuration data.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_domain_name_config_set(
    const vtss_appl_dns_name_conf_t     *const entry
);

/**
 * \brief Iterator for retrieving DNS server table index.
 *
 * Retrieve the 'next' configuration index of the DNS server table according to the given 'prev'.
 * Table index also represents the precedence in selecting target DNS server: less index value means
 * higher priority in round-robin selection.
 * Only one server is working at a time, that is when the chosen server is active, system marks the
 * designated server as target and stops selection.
 * When the active server becomes inactive, system starts another round of selection starting from
 * the next available server setting.
 * At maximum four server settings could be configured.
 *
 * \param prev      [IN]    Porinter of precedence index to be used for index determination.
 *
 * \param next      [OUT]   The next index should be used for the table entry.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Error code.      VTSS_RC_OK for success operation and the value in 'next' is valid,
 *                          other error code means that no "next" index and its corresponding
 *                          entry exists, and the end has been reached.
 */
mesa_rc vtss_appl_dns_server_config_itr(
    const uint32_t                           *const prev,
    uint32_t                                 *const next
);

/**
 * \brief Get DNS server default configuration.
 *
 * Get default configuration of the DNS server settings.
 *
 * \param priority  [OUT]   The default precedence in DNS server configurations.
 * \param entry     [OUT]   The default DNS server configuration settings.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_server_config_default(
    uint32_t                                 *const priority,
    vtss_appl_dns_server_conf_t         *const entry
);

/**
 * \brief Get specific index DNS server configuration.
 *
 * Get DNS server configuration of the specific index.
 *
 * \param priority  [IN]    (key) Index - the precedence in DNS server configurations.
 *
 * \param entry     [OUT]   The current DNS server configuration with the specific index.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_server_config_get(
    const uint32_t                           *const priority,
    vtss_appl_dns_server_conf_t         *const entry
);

/**
 * \brief Set/Update specific index DNS server configuration.
 *
 * Modify DNS server configuration of the specific index.
 *
 * \param priority  [IN]    (key) Index - the precedence in DNS server configurations.
 *
 * \param entry     [OUT]   The revised DNS server configuration with the specific index.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_server_config_set(
    const uint32_t                           *const priority,
    const vtss_appl_dns_server_conf_t   *const entry
);

/**
 * \brief Retrieve active default domain name.
 *
 * To read system default domain name; empty name presents default domain name is not available.
 *
 * \param entry     [OUT]   The current DNS default domain name.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_domainname_status_get(
    vtss_appl_dns_domainname_status_t   *const entry
);

/**
 * \brief Iterator for retrieving DNS server table index.
 *
 * Retrieve the 'next' configuration index of the DNS server table according to the given 'prev'.
 *
 * \param prev      [IN]    Porinter of precedence index to be used for index determination.
 *
 * \param next      [OUT]   The next index should be used for the table entry.
 *                          When IN is NULL, assign the first index.
 *                          When IN is not NULL, assign the next index according to the given IN value.
 *
 * \return Error code.      VTSS_RC_OK for success operation and the value in 'next' is valid,
 *                          other error code means that no "next" index and its corresponding
 *                          entry exists, and the end has been reached.
 */
mesa_rc vtss_appl_dns_server_status_itr(
    const uint32_t                           *const prev,
    uint32_t                                 *const next
);

/**
 * \brief Retrieve configured DNS server information.
 *
 * To read the information of configured DNS server.
 *
 * \param priority  [IN]    (key) Index - the precedence in DNS server configurations.
 *
 * \param entry     [OUT]   The current DNS server information with the specific index.
 *
 * \return Error code.      VTSS_RC_OK for success operation, otherwise for operation failure.
 */
mesa_rc vtss_appl_dns_server_status_get(
    const uint32_t                           *const priority,
    vtss_appl_dns_server_status_t       *const entry
);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_DNS_H_ */
