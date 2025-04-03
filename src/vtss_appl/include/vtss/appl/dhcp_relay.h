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
 * \brief Public DHCP Relay API
 * \details This header file describes DHCP Relay control functions and types.
 */

#ifndef _VTSS_APPL_DHCP_RELAY_H_
#define _VTSS_APPL_DHCP_RELAY_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

//----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
//----------------------------------------------------------------------------

/**
 * \brief DHCP relay information policy
 */
typedef enum {
    VTSS_APPL_DHCP_RELAY_INFO_POLICY_REPLACE, /*!< Replace the original relay
                                                * information when a DHCP message
                                                * that already contains it is
                                                * received. */
    VTSS_APPL_DHCP_RELAY_INFO_POLICY_KEEP,    /*!< Keep the original relay
                                                * information when a DHCP message
                                                * that already contains it is
                                                * received. */
    VTSS_APPL_DHCP_RELAY_INFO_POLICY_DROP,    /*!< Drop the package when a DHCP
                                                * message that already contains
                                                * relay information is received.
                                                */
} vtss_appl_dhcp_relay_information_policy_t;

/**
 * \brief DHCP relay global configuration
 * The configuration defines the behaviour of the DHCP relay function.
 */
typedef struct {
    /** 
     * \brief Global config mode. TRUE is to enable DHCP relay function
     * in the system and FALSE is to disable it.
     */
    mesa_bool_t            mode;

    /** 
     * \brief Server IP address. This IP address is for DHCP server where the DHCP
     * relay will relay DHCP packets to.
     */
    mesa_ipv4_t     serverIpAddr;

    /** 
     * \brief Indicates the DHCP relay information mode option operation.
     * Possible modes are - Enabled: Enable DHCP relay information mode
     * operation. When DHCP relay information mode operation is enabled,
     * the agent inserts specific information (option 82) into a DHCP message
     * when forwarding to DHCP server and removes it from a DHCP message
     * when transferring to DHCP client. It only works when DHCP relay
     * operation mode is enabled. Disabled: Disable DHCP relay information
     * mode operation.
     */
    mesa_bool_t            informationMode;

    /** 
     * \brief Indicates the DHCP relay information option policy. When DHCP relay
     * information mode operation is enabled, if the agent receives a DHCP
     * message that already contains relay agent information it will enforce
     * the policy. The 'Replace' policy is invalid when relay information
     * mode is disabled.
     */
    vtss_appl_dhcp_relay_information_policy_t    informationPolicy;

} vtss_appl_dhcp_relay_param_t;

/**
 * \brief DHCP Relay statistics.
 */
typedef struct {
    uint32_t server_packets_relayed;       /*!< Packets relayed from server to client. */
    uint32_t server_packet_errors;         /*!< Errors sending packets to servers. */
    uint32_t client_packets_relayed;       /*!< Packets relayed from client to server. */
    uint32_t client_packet_errors;         /*!< Errors sending packets to clients. */
    uint32_t agent_option_errors;          /*!< Number of packets forwarded without
                                        * agent options because there was no room.
                                        */
    uint32_t missing_agent_option;         /*!< Number of packets dropped because no
                                        * RAI option matching our ID was found.
                                        */
    uint32_t bad_circuit_id;               /*!< Circuit ID option in matching RAI option
                                        * did not match any known circuit ID.
                                        */
    uint32_t missing_circuit_id;           /*!< Circuit ID option in matching RAI option
                                        * was missing.
                                        */
    uint32_t bad_remote_id;                /*!< Remote ID option in matching RAI option
                                        * did not match any known remote ID.
                                        */
    uint32_t missing_remote_id;            /*!< Remote ID option in matching RAI option
                                        * was missing.
                                        */
    uint32_t receive_server_packets;       /*!< Receive DHCP message from server */
    uint32_t receive_client_packets;       /*!< Receive DHCP message from client */
    uint32_t receive_client_agent_option;  /*!< Receive relay agent information option from client */
    uint32_t replace_agent_option;         /*!< Replace relay agent information option */
    uint32_t keep_agent_option;            /*!< Keep relay agent information option */
    uint32_t drop_agent_option;            /*!< Drop relay agent information option */
} vtss_appl_dhcp_relay_statistics_t;

/**
 * \brief DHCP relay control that includes all the actions
 */
typedef struct {
    mesa_bool_t    clearStatistics;       /*!< clear statistics */
} vtss_appl_dhcp_relay_control_t;

/*
==============================================================================

    Public APIs

==============================================================================
*/
/**
 * \brief Get DHCP Relay Parameters
 *
 * To read current system parameters in DHCP relay.
 *
 * \param param [OUT] The DHCP relay system configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_relay_system_config_get(
    vtss_appl_dhcp_relay_param_t     *const param
);

/**
 * \brief Set DHCP Relay Parameters
 *
 * To modify current system parameters in DHCP relay.
 *
 * \param param [IN] The DHCP relay system configuration data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_relay_system_config_set(
    const vtss_appl_dhcp_relay_param_t   *const param
);

/**
 * \brief Get DHCP Relay Statistics
 *
 * To read current statistics in DHCP relay.
 *
 * \param statistics [OUT] The DHCP relay statistics
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_relay_statistics_get(
    vtss_appl_dhcp_relay_statistics_t   *const statistics
);

/**
 * \brief Get DHCP Relay Control
 *
 * To read current action parameters in DHCP relay.
 *
 * \param control [OUT] The DHCP relay action data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_relay_control_get(
    vtss_appl_dhcp_relay_control_t   *const control
);

/**
 * \brief Set IP Source Guard Control
 *
 * To do the action in DHCP relay.
 *
 * \param control [IN] What to do
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_dhcp_relay_control_set(
    const vtss_appl_dhcp_relay_control_t     *const control
);

//----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//----------------------------------------------------------------------------

#endif  /* _VTSS_APPL_DHCP_RELAY_H_ */
