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
 * \brief Public DHCP Snooping API
 * \details This header file describes DHCP Snooping control functions and types.
 */

#ifndef _VTSS_APPL_DHCP_SNOOPING_H_
#define _VTSS_APPL_DHCP_SNOOPING_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

/**
 * \brief DHCP Snooping global configuration
 * The configuration defines the behaviour of the DHCP Snooping system function.
 */
typedef struct {
    /** 
     * \brief Indicates the DHCP snooping mode operation. Possible modes are -
     * TRUE: Enable DHCP snooping mode operation. When DHCP snooping mode
     * operation is enabled, the DHCP request messages will be forwarded to
     * trusted ports and only allow reply packets from trusted ports.
     * FALSE: Disable DHCP snooping mode operation.
     */
    mesa_bool_t            mode;

} vtss_appl_dhcp_snooping_param_t;

/**
 * \brief DHCP Snooping port configuration
 * The configuration defines the behaviour of the DHCP Snooping per physical port.
 */
typedef struct {
    /** 
     * \brief Indicates the DHCP snooping port mode. Possible port modes are -
     * TRUE: Configures the port as trusted source of the DHCP messages.
     * FALSE: Configures the port as untrusted source of the DHCP messages.
     */
    mesa_bool_t            trustMode;

} vtss_appl_dhcp_snooping_port_config_t;

/**
 * \brief DHCP Snooping assigned IP table
 * The table gets down all IP's assigned to DHCP client by DHCP server per
 * physical port.
 */
typedef struct {
    vtss_ifindex_t      ifIndex;        /*!< Logical interface number of the
                                          * physical port of the DHCP client.
                                          */
    mesa_ipv4_t         ipAddr;         /*!< IP address assigned to DHCP client
                                          * by DHCP server.
                                          */
    mesa_ipv4_t         netmask;        /*!< Netmask assigned to DHCP client
                                          * by DHCP server.
                                          */
    mesa_ipv4_t         dhcpServerIp;   /*!< IP address of the DHCP server
                                          * that assigns the IP address and
                                          * netmask.
                                          */
} vtss_appl_dhcp_snooping_assigned_ip_t;

/**
 * \brief DHCP Snooping port statistics table
 */
typedef struct {
    uint32_t rxDiscover;             /*!< The number of discover (option 53 with value 1) packets received */
    uint32_t rxOffer;                /*!< The number of offer (option 53 with value 2) packets received */
    uint32_t rxRequest;              /*!< The number of request (option 53 with value 3) packets received */
    uint32_t rxDecline;              /*!< The number of decline (option 53 with value 4) packets received */
    uint32_t rxAck;                  /*!< The number of ACK (option 53 with value 5) packets received */
    uint32_t rxNak;                  /*!< The number of NAK (option 53 with value 6) packets received */
    uint32_t rxRelease;              /*!< The number of release (option 53 with value 7) packets received */
    uint32_t rxInform;               /*!< The number of inform (option 53 with value 8) packets received */
    uint32_t rxLeaseQuery;           /*!< The number of lease query (option 53 with value 10) packets received */
    uint32_t rxLeaseUnassigned;      /*!< The number of lease unassigned (option 53 with value 11) packets received */
    uint32_t rxLeaseUnknown;         /*!< The number of lease unknown (option 53 with value 12) packets received */
    uint32_t rxLeaseActive;          /*!< The number of lease active (option 53 with value 13) packets received */
    uint32_t rxDiscardChksumErr;     /*!< The number of discard packet that IP/UDP checksum is error */
    uint32_t rxDiscardUntrust;       /*!< The number of discard packet that are coming from untrusted port */

    uint32_t txDiscover;             /*!< The number of discover (option 53 with value 1) packets transmited */
    uint32_t txOffer;                /*!< The number of offer (option 53 with value 2) packets transmited */
    uint32_t txRequest;              /*!< The number of request (option 53 with value 3) packets transmited */
    uint32_t txDecline;              /*!< The number of decline (option 53 with value 4) packets transmited */
    uint32_t txAck;                  /*!< The number of ACK (option 53 with value 5) packets transmited */
    uint32_t txNak;                  /*!< The number of NAK (option 53 with value 6) packets transmited */
    uint32_t txRelease;              /*!< The number of release (option 53 with value 7) packets transmited */
    uint32_t txInform;               /*!< The number of inform (option 53 with value 8) packets transmited */
    uint32_t txLeaseQuery;           /*!< The number of lease query (option 53 with value 10) packets transmited */
    uint32_t txLeaseUnassigned;      /*!< The number of lease unassigned (option 53 with value 11) packets transmited */
    uint32_t txLeaseUnknown;         /*!< The number of lease unknown (option 53 with value 12) packets transmited */
    uint32_t txLeaseActive;          /*!< The number of lease active (option 53 with value 13) packets transmited */
} vtss_appl_dhcp_snooping_port_statistics_t;

/**
 * \brief DHCP Snooping clear statistics table
 */
typedef struct {
    mesa_bool_t clearPortStatistics;   /*!< clear statistics per physical port */
} vtss_appl_dhcp_snooping_clear_port_statistics_t;

/*
==============================================================================

    Public APIs

==============================================================================
*/
/**
 * \brief Get DHCP Snooping System Parameters
 *
 * To read current system parameters in DHCP Snooping.
 *
 * \param param [OUT] The DHCP Snooping system configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_system_config_get(
    vtss_appl_dhcp_snooping_param_t      *const param
);

/**
 * \brief Set DHCP Snooping System Parameters
 *
 * To modify current system parameters in DHCP Snooping.
 *
 * \param param [IN] The DHCP Snooping system configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_system_config_set(
    const vtss_appl_dhcp_snooping_param_t    *const param
);

/**
 * \brief Iterate function of DHCP Snooping Port Configuration
 *
 * To get first and get next indexes.
 *
 * \param prev_ifindex [IN]  previous ifindex.
 * \param next_ifindex [OUT] next ifindex.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_port_config_itr(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);

/**
 * \brief Get DHCP Snooping Port Configuration
 *
 * To read configuration of the port in DHCP Snooping.
 *
 * \param ifindex   [IN]  (key 1) Interface index - the logical interface
 *                                index of the physical port.
 * \param port_conf [OUT] The current configuration of the port
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_port_config_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_dhcp_snooping_port_config_t    *const port_conf
);

/**
 * \brief Set DHCP Snooping Port Configuration
 *
 * To modify configuration of the port in DHCP Snooping.
 *
 * \param ifindex   [IN] (key 1) Interface index - the logical interface index
 *                               of the physical port.
 * \param port_conf [IN] The configuration set to the port
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_port_config_set(
    vtss_ifindex_t  ifindex,
    const vtss_appl_dhcp_snooping_port_config_t      *const port_conf
);

/**
 * \brief Iterate function of DHCP Snooping Assigned IP Table
 *
 * To get first and get next indexes.
 *
 * \param prev_mac [IN]  previous MAC address.
 * \param next_mac [OUT] next MAC address.
 * \param prev_vid [IN]  previous VLAN ID.
 * \param next_vid [OUT] next VLAN ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_assigned_ip_itr(
    const mesa_mac_t        *const prev_mac,
    mesa_mac_t              *const next_mac,
    const mesa_vid_t        *const prev_vid,
    mesa_vid_t              *const next_vid
);

/**
 * \brief Get DHCP Snooping Assigned IP entry
 *
 * To read data of assigned IP in DHCP Snooping.
 *
 * \param mac_addr    [IN]  (key 1) MAC address
 * \param vid         [IN]  (key 2) VLAN ID
 * \param assigned_ip [OUT] The information of the IP assigned to DHCP client by DHCP server
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_assigned_ip_get(
    mesa_mac_t  mac_addr,
    mesa_vid_t  vid,
    vtss_appl_dhcp_snooping_assigned_ip_t    *const assigned_ip
);

/**
 * \brief Iterate function of DHCP Snooping Port Statistics table
 *
 * To get first and get next indexes.
 *
 * \param prev_ifindex [IN]  previous ifindex.
 * \param next_ifindex [OUT] next ifindex.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_port_statistics_itr(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
);

/**
 * \brief Get DHCP Snooping Port Statistics entry
 *
 * To read statistics of the port in DHCP Snooping.
 *
 * \param ifindex         [IN]  (key 1) Interface index - the logical interface
 *                                      index of the physical port.
 * \param port_statistics [OUT] The statistics of the port
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_port_statistics_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_dhcp_snooping_port_statistics_t    *const port_statistics
);

/**
 * \brief Get DHCP Snooping Clear Port Statistics Action entry
 *
 * To get status of clear statistics action of the port in DHCP Snooping.
 *
 * \param ifindex               [IN]  (key 1) Interface index - the logical interface
 *                                            index of the physical port.
 * \param clear_port_statistics [OUT] The status of clear statistics action of the port
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_clear_port_statistics_get(
    vtss_ifindex_t  ifindex,
    vtss_appl_dhcp_snooping_clear_port_statistics_t  *const clear_port_statistics
);

/**
 * \brief Set DHCP Snooping Clear Port Statistics Action entry
 *
 * To set to clear statistics on the port in DHCP Snooping.
 *
 * \param ifindex               [IN] (key 1) Interface index - the logical interface
 *                                           index of the physical port.
 * \param clear_port_statistics [IN] Action to clear port statistics
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_snooping_clear_port_statistics_set(
    vtss_ifindex_t  ifindex,
    const vtss_appl_dhcp_snooping_clear_port_statistics_t    *const clear_port_statistics
);

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_DHCP_SNOOPING_H_ */
