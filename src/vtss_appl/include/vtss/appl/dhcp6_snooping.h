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
 * \brief Public DHCPv6 Snooping API
 * \details This header file describes DHCPv6 Snooping control functions and types.
 */

#ifndef _VTSS_APPL_DHCP6_SNOOPING_H_
#define _VTSS_APPL_DHCP6_SNOOPING_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/appl/module_id.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

/**
 * DHCPv6 snooping system mode
 */
typedef enum {
    DHCP6_SNOOPING_MODE_DISABLED,               /**< DHCPv6 snooping is disabled. */
    DHCP6_SNOOPING_MODE_ENABLED,                /**< DHCPv6 snooping is enabled. */
} dhcp6_system_mode_t;

/**
 * DHCPv6 snooping unknown next-header behavior
 */
typedef enum {
    DHCP6_SNOOPING_NH_UNKNOWN_MODE_DROP,      /**< Drop packets with unknown IPv6 ext. headers. */
    DHCP6_SNOOPING_NH_UNKNOWN_MODE_ALLOW,     /**< Allow packets with unknown IPv6 ext. headers. */
} dhcp6_nh_unknown_mode_t;

/**
 * DHCPv6 snooping port mode
 */
typedef enum {
    DHCP6_SNOOPING_PORT_MODE_UNTRUSTED,         /**< This port is untrusted wrt DHCPv6 snooping. */
    DHCP6_SNOOPING_PORT_MODE_TRUSTED            /**< This port is trusted wrt DHCPv6 snooping. */
} dhcp6_port_trust_mode_t;

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    DHCP6_SNOOPING_ERROR_INV_PARAM = MODULE_ERROR_START(VTSS_MODULE_ID_DHCP6_SNOOPING),     /**< Invalid parameter. */
    DHCP6_SNOOPING_ERROR_CB_COUNT,                                                          /**< Too many callback registrations. */
    DHCP6_SNOOPING_ERROR_CB_NOTFOUND,                                                       /**< Registered callback not found. */
    DHCP6_SNOOPING_ERROR_RESOURCES,                                                         /**< Lack of resources. */
};

/**
 * \brief DHCPv6 Snooping global status
 */
typedef struct {
    uint32_t    last_change_ts;         /**< Timestamp for last change to snooping table status. */
} vtss_appl_dhcp6_snooping_global_status_t;


/**
 * \brief DHCPv6 Snooping global configuration
 * The configuration defines the behaviour of the DHCPv6 Snooping system function.
 */
typedef struct {
    /**
     * \brief Indicates the DHCPv6 snooping mode operation. Possible modes are:
     * 
     * DHCP6_SNOOPING_MGMT_ENABLED: Enable DHCPv6 snooping mode operation.
     * When DHCPv6 snooping mode operation is enabled, the DHCPv6 request messages
     * will only be forwarded to trusted ports and only allow reply packets from
     * trusted ports.
     *
     * DHCP6_SNOOPING_MGMT_DISABLED: Disable DHCPv6 snooping mode operation.
     */
    dhcp6_system_mode_t snooping_mode = DHCP6_SNOOPING_MODE_DISABLED;

    /**
     * \brief Determines how to treat IPv6 packets with unknown extension
     * headers (aka. next header IDs). See RFC7610, section 5, part 3 for details:
     * "DHCPv6-Shield MUST provide a configuration knob that controls whether
     * or not packets with unrecognized Next Header values are dropped".
     *
     * Possible modes are:
     * - DHCP6_SNOOPING_NH_UNKNOWN_MODE_DROP:   Drop packets with unknown IPv6 ext. headers.
     * - DHCP6_SNOOPING_NH_UNKNOWN_MODE_ALLOW:  Allow packets with unknown IPv6 ext. headers.
     */
    dhcp6_nh_unknown_mode_t nh_unknown_mode = DHCP6_SNOOPING_NH_UNKNOWN_MODE_DROP;

} vtss_appl_dhcp6_snooping_conf_t;


/**
 * \brief DHCPv6 Snooping port configuration
 * The configuration defines the behaviour of the DHCPv6 Snooping per physical port.
 */
typedef struct {
    /**
     * \brief Indicates the DHCPv6 snooping port mode. Possible port modes are:
     *
     * DHCP6_SNOOPING_PORT_MODE_TRUSTED: Configures the port as trusted source of the DHCPv6 messages.
     * DHCP6_SNOOPING_PORT_MODE_UNTRUSTED: Configures the port as untrusted source of the DHCPv6 messages.
     */
    dhcp6_port_trust_mode_t    trust_mode;

} vtss_appl_dhcp6_snooping_port_conf_t;

/**
 * \brief Max. size of DHCP DUID
 */
#define DHCP6_DUID_MAX_SIZE         128U

/**
 * \brief DHCP Unique Identifier (DUID) data structure. A DUID is a globally
 * unique identifier for a DHCP client.
 */
struct vtss_appl_dhcp6_snooping_duid_t {
    uint8_t data[DHCP6_DUID_MAX_SIZE];          /**< DUID data as unsigned octets */
    uint32_t length;                            /**< DUID length in octets */
};

/**
 * \brief DHCP Identity Association Identifier (IAID) type
 */
typedef uint32_t vtss_appl_dhcp6_snooping_iaid_t;

/**
 * \brief DHCPv6 Snooping client entry
 * The entry contains information about a DHCPv6 client.
 */
typedef struct {
    vtss_appl_dhcp6_snooping_duid_t duid;   /*!< DHCPv6 client DHCP Unique Identifier (DUID) */
    mesa_mac_t          mac;                /*!< MAC address of DHCPv6 client. */
    vtss_ifindex_t      if_index;           /*!< Logical interface number of the local ingress port for the DHCPv6 client. */
} vtss_appl_dhcp6_snooping_client_info_t;

/**
 * \brief DHCPv6 Snooping assigned IP table entry
 * The entry contains an IP address assigned to DHCPv6 client by DHCPv6 server.
 *
 * Note that a client may have multiple interfaces, and can thus also have multiple
 * addresses assigned; one for each interface. Each interface is identified by
 * a Identity Association Identifier (IAID) value. This value is only unique
 * in the context of the client.
 */
typedef struct {
    vtss_appl_dhcp6_snooping_iaid_t iaid;   /*!< Interface IAID */
    mesa_bool_t         valid;              /*!< Set to true if this item contains an valid address value */
    mesa_ipv6_t         ip_address;         /*!< IP address assigned to DHCPv6 client by DHCP server. */
    mesa_vid_t          vid;                /*!< VLAN ID (VID) of DHCPv6 client interface. */
    uint32_t            lease_time;         /*!< Lease time in seconds */
    time_t              assigned_time;      /*!< Timestamp when address was assigned */
    mesa_ipv6_t         dhcp_server_ip;     /*!< IP address of the DHCPv6 server that assigns the IP address and prefix. */
} vtss_appl_dhcp6_snooping_assigned_ip_t;

/**
 * Callback function for IP assigned information
 */
typedef enum {
    DHCP6_SNOOPING_INFO_REASON_ASSIGNED,        /*!< Address has been assigned */
    DHCP6_SNOOPING_INFO_REASON_RELEASE,         /*!< Address has been released */
    DHCP6_SNOOPING_INFO_REASON_LEASE_TIMEOUT,   /*!< Address lease timeout */
    DHCP6_SNOOPING_INFO_REASON_MODE_DISABLED,   /*!< Snooping function has been disabled */
    DHCP6_SNOOPING_INFO_REASON_PORT_LINK_DOWN,  /*!< Port link went down - TODO - why is this interesting?*/
    DHCP6_SNOOPING_INFO_REASON_SWITCH_DOWN,     /*!< TODO - que? */
    DHCP6_SNOOPING_INFO_REASON_ENTRY_DUPLEXED   /*!< TODO */
} vtss_appl_dhcp6_snooping_info_reason_t;

/**
 * \brief DHCPv6 snooping IP assigned information callback type
 */
typedef void (*vtss_appl_dhcp6_snooping_ip_assigned_info_callback_t)(
    const vtss_appl_dhcp6_snooping_client_info_t *client_info,
    const vtss_appl_dhcp6_snooping_assigned_ip_t *address_info,
    vtss_appl_dhcp6_snooping_info_reason_t reason);


/**
 * \brief DHCPv6 Snooping port statistics table for a direction (RX or TX)
 */
typedef struct  {
    uint32_t solicit;             /*!< The number of SOLICIT packets */
    uint32_t request;             /*!< The number of REQUEST packets */
    uint32_t infoRequest;         /*!< The number of INFOREQUEST packets */
    uint32_t confirm;             /*!< The number of CONFIRM packets */
    uint32_t renew;               /*!< The number of RENEW packets */
    uint32_t rebind;              /*!< The number of REBIND packets */
    uint32_t decline;             /*!< The number of DECLINE packets */
    uint32_t advertise;           /*!< The number of ADVERTISE packets */
    uint32_t reply;               /*!< The number of REPLY packets */
    uint32_t reconfigure;         /*!< The number of RECONFIGURE packets */
    uint32_t release;             /*!< The number of RELEASE packets */
} vtss_appl_dhcp6_snooping_port_statistics_dir_t;

/**
 * \brief DHCPv6 Snooping port statistics table
 */
typedef struct {

    uint32_t rxDiscardUntrust;      /*!< Number of discarded server packets due to port being untrusted */

    vtss_appl_dhcp6_snooping_port_statistics_dir_t rx;  /*!< RX packet counters */
    vtss_appl_dhcp6_snooping_port_statistics_dir_t tx;  /*!< TX packet counters */

    mesa_bool_t clear_stats;        /*!< Clear statistics. Only valid when written. */

} vtss_appl_dhcp6_snooping_port_statistics_t;


/*
==============================================================================

    Public APIs

==============================================================================
*/
/**
 * \brief Get DHCPv6 Snooping global status
 *
 * @param status    [OUT] The DHCPv6 Snooping global status
 * @return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_global_status_get(
    vtss_appl_dhcp6_snooping_global_status_t   *const status);

/**
 * \brief Get DHCPv6 Snooping System Parameters
 *
 * To read current system parameters in DHCPv6 Snooping.
 *
 * \param conf [OUT] The DHCPv6 Snooping system configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_conf_get(
    vtss_appl_dhcp6_snooping_conf_t      *const conf
);

/**
  * \brief Get the global DHCPv6 snooping default configuration.
  *
  * \param conf [IN_OUT]:   Pointer to structure that contains the
  *                         configuration to get the default setting.
  */
void vtss_appl_dhcp6_snooping_conf_get_default(vtss_appl_dhcp6_snooping_conf_t *conf);

/**
  * \brief Determine if DHCPv6 snooping configuration has changed.
  *
  * \param old_conf [IN]: Pointer to structure that contains the
  *                  old configuration.
  * \param new_conf [IN]: Pointer to structure that contains the
  *                       new configuration.
  *
  * \return
  *   false: No change.\n
  *   true: Configuration changed.\n
  */
bool vtss_appl_dhcp6_snooping_conf_changed(const vtss_appl_dhcp6_snooping_conf_t *const old_conf,
                                           const vtss_appl_dhcp6_snooping_conf_t *const new_conf);

/**
 * \brief Set DHCPv6 Snooping System Parameters
 *
 * To modify current system parameters in DHCPv6 Snooping.
 *
 * \param conf [IN] The DHCPv6 Snooping system configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_conf_set(
    const vtss_appl_dhcp6_snooping_conf_t    *const conf
);

/**
 * \brief Get DHCP6 Snooping Port Configuration
 *
 * To read configuration of the port in DHCP6 Snooping.
 *
 * \param ifindex   [IN]  (key 1) Interface index - the logical interface
 *                                index of the physical port.
 * \param port_conf [OUT] The current configuration of the port
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_port_conf_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_dhcp6_snooping_port_conf_t    *const port_conf
);

/**
  * \brief Get the DHCPv6 snooping default configuration for a single port.
  *
  * \param port_conf [IN_OUT]:   Pointer to structure that contains the
  *                         configuration to get the default setting.
  */
void vtss_appl_dhcp6_snooping_port_conf_get_default(vtss_appl_dhcp6_snooping_port_conf_t *port_conf);

/**
  * \brief Determine if DHCPv6 snooping port configuration has changed.
  *
  * \param old_conf [IN]: Pointer to structure that contains the
  *                  old configuration.
  * \param new_conf [IN]: Pointer to structure that contains the
  *                       new configuration.
  *
  * \return
  *   false: No change.\n
  *   true: Configuration changed.\n
  */
bool vtss_appl_dhcp6_snooping_port_conf_changed(const vtss_appl_dhcp6_snooping_port_conf_t *const old_conf,
                                                const vtss_appl_dhcp6_snooping_port_conf_t *const new_conf);

/**
 * \brief Iterate function of DHCPv6 Snooping Port Configuration
 *
 * To get first and get next indexes.
 *
 * \param prev_ifindex [IN]  previous ifindex.
 * \param next_ifindex [OUT] next ifindex.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_port_conf_itr(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);

/**
 * \brief Set DHCP6 Snooping Port Configuration
 *
 * To modify configuration of the port in DHCP6 Snooping.
 *
 * \param ifindex   [IN] (key 1) Interface index - the logical interface index
 *                               of the physical port.
 * \param port_conf [IN] The configuration set to the port
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_port_conf_set(
    vtss_ifindex_t  ifindex,
    const vtss_appl_dhcp6_snooping_port_conf_t      *const port_conf
);

/**
 * \brief Iterate function of DHCP6 Snooping Port Statistics table
 *
 * To get first and get next indexes.
 *
 * \param prev_ifindex [IN]  previous ifindex.
 * \param next_ifindex [OUT] next ifindex.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_port_statistics_itr(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);

/**
 * \brief Get DHCP6 Snooping Port Statistics entry
 *
 * To read statistics of the port in DHCP6 Snooping.
 *
 * \param ifindex         [IN]  (key 1) Interface index - the logical interface
 *                                      index of the physical port.
 * \param port_statistics [OUT] The statistics of the port
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_port_statistics_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_dhcp6_snooping_port_statistics_t    *const port_statistics
);

/**
 * \brief Set DHCP6 Snooping Clear Port Statistics Action entry
 *
 * To clear statistics on the port in DHCP6 Snooping set the clear_stats
 * value to TRUE.
 *
 * \param ifindex               [IN] (key 1) Interface index - the logical interface
 *                                           index of the physical port.
 * \param port_statistics [IN] Action to clear port statistics
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_port_statistics_set(
    vtss_ifindex_t  ifindex,
    const vtss_appl_dhcp6_snooping_port_statistics_t    *const port_statistics
);

/**
 * \brief Register for IP assigned information notifications
 *
 * \param cb [IN]  Callback function for notifications.
 *
 * \return
 *   VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_assigned_ip_info_register(vtss_appl_dhcp6_snooping_ip_assigned_info_callback_t cb);

/**
 * \brief Unregister for IP assigned information notifications
 *
 * \param cb [IN]  Callback function for notifications.
 *
 * \return
 *   VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_assigned_ip_info_unregister(vtss_appl_dhcp6_snooping_ip_assigned_info_callback_t cb);

/**
 * \brief Iterate function for DHCP6 Snooping client table
 *
 * \param prev_duid [IN]  previous client DUID. If NULL the first entry in the table is returned.
 * \param next_duid [OUT] next client DUID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_client_info_itr(
    const vtss_appl_dhcp6_snooping_duid_t   *const prev_duid,
    vtss_appl_dhcp6_snooping_duid_t         *const next_duid
);

/**
 * \brief Get DHCP6 Snooping client table entry.
 *
 * \param duid    [IN] client DUID
 * \param client_info [OUT] The information for the DHCP client
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_client_info_get(
    const vtss_appl_dhcp6_snooping_duid_t   &duid,
    vtss_appl_dhcp6_snooping_client_info_t  *const client_info
);

/**
 * \brief Iterate function for DHCP6 Snooping assigned addresses table
 *
 * \param prev_duid [IN]  Previous client DUID. If NULL the first entry in the client table is returned.
 * \param next_duid [OUT] Next client DUID.
 * \param prev_iaid [IN]  Previous IAID. If NULL the first entry in the IAID table is returned.
 * \param next_iaid [OUT] Next interface IAID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_assigned_ip_itr(
    const vtss_appl_dhcp6_snooping_duid_t   *const prev_duid,
    vtss_appl_dhcp6_snooping_duid_t         *const next_duid,
    const vtss_appl_dhcp6_snooping_iaid_t   *const prev_iaid,
    vtss_appl_dhcp6_snooping_iaid_t         *const next_iaid
);

/**
 * \brief Get DHCP6 Snooping assigned addresses table entry.
 *
 * \param duid    [IN] Client DUID
 * \param iaid    [IN] IAID of interface on client
 * \param address_info [OUT] The assigned address information for the IAID
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp6_snooping_assigned_ip_get(
    const vtss_appl_dhcp6_snooping_duid_t   &duid,
    const vtss_appl_dhcp6_snooping_iaid_t   iaid,
    vtss_appl_dhcp6_snooping_assigned_ip_t  *const address_info
);


//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_DHCP6_SNOOPING_H_ */
