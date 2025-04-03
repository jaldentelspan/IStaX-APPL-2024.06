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
 * \brief sFlow API
 * \details This header file describes public functions applicable to sFlow management.
 */

#ifndef _VTSS_APPL_SFLOW_H_
#define _VTSS_APPL_SFLOW_H_

#include <vtss/appl/interface.h>
#include <microchip/ethernet/switch/api.h>
#include <vtss/appl/types.h>

/**
 * Number of Flow Sampler and Counter Poller instances per port (one
 * unified value).
 * For Flow Samplers:
 * Setting this to 1 will cause us to support any flow sample rate
 * from [1; N] that the H/W supports.
 * Setting this to a value > 1 will cause us to support only powers of
 * two of N, i.e. 1, 2, 4, ..., up to the number that the physical
 * hardware supports. In this case, the S/W will configure the H/W
 * to the fastest rate, and perform S/W-based sub-sampling.
 * See also www.sflow.org/sflow_version_5.txt, chapter 4.2.2.
 *
 * For Counters Pollers:
 * Counter polling interval can be set from 1..X regardless of
 * the value of VTSS_APPL_SFLOW_INSTANCE_CNT.
 */
#define VTSS_APPL_SFLOW_INSTANCE_CNT 1

/**
 * Setting the number of receivers to the number of flow samplers/counter
 * pollers (VTSS_APPL_SFLOW_INSTANCE_CNT) is advisable, since that will provide
 * any receiver with the option of sampling a given port even when
 * other receivers are sampling the same port.
 */
#define VTSS_APPL_SFLOW_RECEIVER_CNT VTSS_APPL_SFLOW_INSTANCE_CNT

/**
 * Default value of IP type
 */
#define VTSS_APPL_SFLOW_AGENT_IP_TYPE_DEFAULT          MESA_IP_TYPE_IPV4

/**
 * Default value of SFLOW AGENT's IP address
 */
#define VTSS_APPL_SFLOW_AGENT_IP_ADDR_DEFAULT          0x7f000001 /* loopback */

/**
 * Maximum valid value of SFLOW receiver timeout
 */
#define VTSS_APPL_SFLOW_RECEIVER_TIMEOUT_MAX           0x7FFFFFFF

/**
 * Maximum value of SFLOW receiver's UDP port number
 */
#define VTSS_APPL_SFLOW_RECEIVER_UDP_PORT_MAX          65535

/**
 * Default value of SFLOW receiver's UDP port number
 */
#define VTSS_APPL_SFLOW_RECEIVER_UDP_PORT_DEFAULT      6343

/**
 * Minimum size of UDP datagram message to be sent to SFLOW receiver
 */
#define VTSS_APPL_SFLOW_RECEIVER_DATAGRAM_SIZE_MIN     200

/**
 * Maximum size of UDP datagrame message to be sent to SFLOW receiver
 * Note: this value must be multiple of 4, and 50 bytes less than an MTU to allow
 *       for Ethernet+IP+UDP header
 */
#define VTSS_APPL_SFLOW_RECEIVER_DATAGRAM_SIZE_MAX     1468

/**
 * Default size of UDP datagram message to be sent to SFLOW receiver
 */
#define VTSS_APPL_SFLOW_RECEIVER_DATAGRAM_SIZE_DEFAULT 1400

/**
 * Minimum size of SFLOW header
 */
#define VTSS_APPL_SFLOW_FLOW_HEADER_SIZE_MIN           14

/**
 * Maximum size of SFLOW header
 */
#define VTSS_APPL_SFLOW_FLOW_HEADER_SIZE_MAX           200

/**
 * Default size of SFLOW header
 */
#define VTSS_APPL_SFLOW_FLOW_HEADER_SIZE_DEFAULT       128

/**
 * Default sampling rate of flow sampling (0 means flow sampling is disabled)
 */
#define VTSS_APPL_SFLOW_FLOW_SAMPLING_RATE_DEFAULT     0

/**
 * Minimum value of counter polling interval (0 means counter polling is disabled)
 */
#define VTSS_APPL_SFLOW_POLLING_INTERVAL_MIN           0

/**
 * Maximum value of counter polling interval
 */
#define VTSS_APPL_SFLOW_POLLING_INTERVAL_MAX           3600

/**
 * Default value of counter polling interval
 */
#define VTSS_APPL_SFLOW_POLLING_INTERVAL_DEFAULT       VTSS_APPL_SFLOW_POLLING_INTERVAL_MIN

/**
 * SFLOW datagram version
 */
#define VTSS_APPL_SFLOW_DATAGRAM_VERSION               5

/**
 * SFLOW Owner string's length
 */
#define VTSS_APPL_SFLOW_OWNER_LEN                      128

/**
 * SFLOW Owner's string value for locally managed SFLOW agent
 */
#define VTSS_APPL_SFLOW_OWNER_LOCAL_MANAGEMENT_STRING  "<Configured through local management>"

/**
 * vtss_appl_sflow_agent_t
 *
 * Holds part of the agent configuration, particularly
 * for use by SNMP.
 */
typedef struct {

    /**
     * This switch's IP address, or rather, by default the IPv4 loopback address,
     * but it can be changed from CLI and Web. Will not get persisted.
     * The aggregate type allows for holding either an IPv4 and IPv6 address.
     */
    mesa_ip_addr_t agent_ip_addr;
} vtss_appl_sflow_agent_t;

/**
 * vtss_appl_sflow_rcvr_t
 * Uniquely identifies one sFlow Receiver (a.k.a. collector).
 *
 * Up to VTSS_APPL_SFLOW_RECEIVER_CNT receivers can be configured.
 */
typedef struct {
    /**
     * The entity making use of this receiver table entry.
     * Set to empty string when not owned.
     */
    char owner[VTSS_APPL_SFLOW_OWNER_LEN];

    /**
     * The time (in seconds) remaining before the sampler is
     * released and stops sampling.
     * Valid values: [0; VTSS_APPL_SFLOW_RECEIVER_TIMEOUT_MAX].
     *
     * Note that this is the original value configured when
     * read back. The current timeout left is held in
     * sflow_rcvr_info_t::timeout_left.
     *
     * Implementation note:
     *   This attribute is the reason that it doesn't make sense
     *   to save the receiver configuration to flash. How would
     *   you maintain a timeout across boots? One could reserve
     *   0xFFFFFFFF for a receiver that never times out, of course,
     *   but that's beyond the sFlow Spec.
     */
    uint32_t timeout;

    /**
     * The maximum number of data bytes that can be sent in
     * a single datagram. The manager should set this value
     * to avoid fragmentation of the sFlow datagrams.
     *
     * Valid values: [SFLOW_RECEIVER_DATAGRAM_SIZE_MIN; SFLOW_RECEIVER_DATAGRAM_SIZE_MAX].
     * Default: SFLOW_RECEIVER_DATAGRAM_SIZE_DEFAULT.
     */
    uint32_t max_datagram_size;

    /**
     * IPv4/IPv6 address or hostname of receiver to which the datagrams are sent.
     */
    vtss_inet_address_t hostname;

    /**
     * UDP port number on the receiver to send the datagrams to.
     *
     * Valid values: [1; 65535].
     * Default: 6343.
     */
    uint16_t udp_port;

    /**
     * The version of sFlow datagrams that we should send.
     * Valid values [5; 5] (we only support v. 5).
     * Default: 5.
     */
    int32_t datagram_version;
} vtss_appl_sflow_rcvr_t;

/**
 * vtss_appl_sflow_rcvr_info_t
 */
typedef struct {
    /**
     * The IP address of the current receiver, as binary and including type.
     * If there is no current receiver, the type is IPv4 and address 0.0.0.0
     */
    mesa_ip_addr_t ip_addr;

    /**
     * The number of seconds left for this receiver.
     */
    uint32_t  timeout_left;
} vtss_appl_sflow_rcvr_info_t;

/**
 * vtss_appl_sflow_fs_t
 * Flow sampler entry. There are VTSS_APPL_SFLOW_INSTANCE_CNT such samplers per port.
 *
 * Indexed by <sFlowFsDataSource, sFlowFsInstance>
 */
typedef struct {
    /**
     * Enable or disable this entry.
     * There are several ways to make this entry active, but having a
     * single enable/disable parameter allows for keeping old config
     * and simply disable it.
     */
    mesa_bool_t enabled;

    /**
     * One-based index into the sflow_rcvr table. 0 means that this entry is free.
     * Valid values: [0; VTSS_APPL_SFLOW_RECEIVER_CNT].
     * Default: 0.
     */
    uint32_t receiver;

    /**
     * The statistical sampling rate for packet sampling from this source.
     *
     * See discussion under VTSS_APPL_SFLOW_INSTANCE_CNT for a thorough description
     * of possible adjustments made by the S/W.
     */
    uint32_t sampling_rate;

    /**
     * The maximum number of bytes that should be copied from a sampled packet.
     * Valid values: [VTSS_APPL_SFLOW_FLOW_HEADER_SIZE_MIN; VTSS_APPL_SFLOW_FLOW_HEADER_SIZE_MAX].
     * Default: VTSS_APPL_SFLOW_FLOW_HEADER_SIZE_DEFAULT.
     */
    uint32_t max_header_size;

    /**
     * The flow sampler type.
     *
     * The sFlow spec says:
     *   Ideally the sampling entity will perform sampling on all
     *   flows originating from or destined to the specified interface.
     *   However, if the switch architecture only allows input or output
     *   sampling then the sampling agent is permitted to only sample input
     *   flows input or output flows.
     *
     * In the first take, this was set to MESA_SFLOW_TYPE_ALL, but it appears
     * that sFlow monitors (like InMon's sFlowTrend) report too high ingress
     * rate, because when sending an Rx sample, we're only able to report
     * input interface and not output interface. For Tx samples, we can
     * report both interfaces. Therefore, it has been decided to change
     * the default to MESA_SFLOW_TYPE_TX. This in turn means that if only
     * one port is sFlow-enabled, and that port never transmits any frames,
     * then no sFlow samples will be sent.
     * But as https://groups.google.com/forum/#!topic/sflow/dhnKOHyGl9I and
     * a later private conversation with Stuart Johnston from InMon state,
     * sFlow is meant to be enabled on all interfaces, which means that this
     * will normally not be a problem.
     *
     */
    mesa_sflow_type_t type;
} vtss_appl_sflow_fs_t;

/**
 * vtss_appl_sflow_cp_t
 * Counter Poller entry. There are VTSS_APPL_SFLOW_INSTANCE_CNT such samplers per port.
 *
 * Indexed by <sFlowCpDataSource, sFlowCpInstance>
 */
typedef struct {
    /**
     * Enable or disable this entry.
     * There are several ways to make this entry active, but having a
     * single enable/disable parameter allows for keeping old config
     * and simply disable it.
     */
    mesa_bool_t enabled;

    /**
     * One-based index into the sflow_rcvr table. 0 means that this entry is free.
     * Valid values: [0; VTSS_APPL_SFLOW_RECEIVER_CNT] (where 0 frees it).
     * Default: 0
     */
    uint32_t receiver;

    /**
     * The maximum number of seconds between sampling of counters for this port. 0 disables sampling.
     * Valid values: [VTSS_APPL_SFLOW_POLLING_INTERVAL_MIN; VTSS_APPL_SFLOW_POLLING_INTERVAL_MAX].
     * Default: 0
     */
    uint32_t interval;
} vtss_appl_sflow_cp_t;

/**
 * vtss_appl_sflow_rcvr_statistics_t
 * Per-receiver statistics.
 * The statistics get cleared automatically when
 * a new receiver is configured for this receiver
 * index or a receiver unregisters itself or
 * times out.
 */
typedef struct {
    /**
     * Counting number of times datagrams were sent successfully.
     */
    uint64_t dgrams_ok;

    /**
     * Counting number of times datagram transmission failed
     * (for instance because the receiver could not be reached).
     */
    uint64_t dgrams_err;

    /**
     * Counting number of attempted transmitted flow samples.
     * If #dgrams_err is 0, this corresponds to the actual number
     * of transmitted flow samples.
     */
    uint64_t fs;

    /**
     * Counting number of attempted transmitted counter samples.
     * If #dgrams_err is 0, this corresponds to the actual number
     * of transmitted counter samples.
     */
    uint64_t cp;
} vtss_appl_sflow_rcvr_statistics_t;

/**
 * vtss_appl_sflow_instance_statistics_t
 * Per-instance statistics.
 */
typedef struct {
    /**
     * Number of flow samples received on the primary switch that were
     * Rx sampled.
     */
    uint64_t fs_rx;

    /**
     * Number of flow samples received on the primary switch that were Tx
     * sampled.
     */
    uint64_t fs_tx;

    /**
     * Number of counter samples received on the primary switch.
     */
    uint64_t cp;
} vtss_appl_sflow_instance_statistics_t;

/*--------------------------- Public API functions -----------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Get general agent information.
 *
 * \param agent_cfg [OUT] Current configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_agent_cfg_get(vtss_appl_sflow_agent_t *const agent_cfg);

/**
 * \brief Set general agent information.
 *
 * \param agent_cfg [IN] Current configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_agent_cfg_set(const vtss_appl_sflow_agent_t *const agent_cfg);

/**
 * \brief Get receiver configuration.
 *
 * \param rcvr_idx [IN]  Receiver index ([1; VTSS_APPL_SFLOW_RECEIVER_CNT]).
 * \param rcvr_cfg [OUT] Current configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_rcvr_cfg_get(uint32_t rcvr_idx, vtss_appl_sflow_rcvr_t *const rcvr_cfg);

/**
 * \brief Get receiver status.
 *
 * \param rcvr_idx [IN]  Receiver index ([1; VTSS_APPL_SFLOW_RECEIVER_CNT]).
 * \param rcvr_info [OUT] Current receiver status information. May be NULL.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_rcvr_status_get(uint32_t rcvr_idx, vtss_appl_sflow_rcvr_info_t *const rcvr_info);

/**
 * \brief Set receiver configuration.
 *
 * \param rcvr_idx [IN] Receiver index ([1; VTSS_APPL_SFLOW_RECEIVER_CNT]).
 * \param rcvr_cfg [IN] New configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_rcvr_cfg_set(uint32_t rcvr_idx, const vtss_appl_sflow_rcvr_t *const rcvr_cfg);

/**
 * \brief Delete receiver configuration.
 *
 * \param rcvr_idx [IN] Receiver index ([1; VTSS_APPL_SFLOW_RECEIVER_CNT]).
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_rcvr_cfg_del(uint32_t rcvr_idx);

/**
 * \brief Receiver index iterator.
 *
 * \param rcvr_idx_in  [IN]  previous/NULL receiver index (if NULL then rcvr_cfg_out will be index of first receiver).
 * \param rcvr_idx_out [OUT] next receiver index.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_rcvr_itr(const uint32_t *const rcvr_idx_in, uint32_t *const rcvr_idx_out);

/**
 * \brief Get flow sampler configuration.
 *
 * \param ifindex  [IN]  Interface index (which can be decomposed into stack identifier and physical port number)
 * \param instance [IN]  Instance (1..VTSS_APPL_SFLOW_INSTANCE_CNT)
 * \param cfg      [OUT] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_flow_sampler_cfg_get(vtss_ifindex_t ifindex, uint16_t instance, vtss_appl_sflow_fs_t *const cfg);

/**
 * \brief Set flow sampler configuration.
 *
 * \param ifindex  [IN] Interface index (which can be decomposed into stack identifier and physical port number)
 * \param instance [IN] Instance (1..VTSS_APPL_SFLOW_INSTANCE_CNT)
 * \param cfg      [IN] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_flow_sampler_cfg_set(vtss_ifindex_t ifindex, uint16_t instance, const vtss_appl_sflow_fs_t *const cfg);

/**
 * \brief Remove non-default flow sampler configuration.
 *
 * \param ifindex  [IN] Interface index (which can be decomposed into stack identifier and physical port number)
 * \param instance [IN] Instance (1..VTSS_APPL_SFLOW_INSTANCE_CNT)
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_flow_sampler_cfg_del(vtss_ifindex_t ifindex, uint16_t instance);

/**
 * \brief Get flow sampler configuration.
 *
 * \param ifindex  [IN]  Interface index (which can be decomposed into stack identifier and physical port number)
 * \param instance [IN]  Instance (1..VTSS_APPL_SFLOW_INSTANCE_CNT)
 * \param cfg      [OUT] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_counter_poller_cfg_get(vtss_ifindex_t ifindex, uint16_t instance, vtss_appl_sflow_cp_t *const cfg);

/**
 * \brief Set counter polling configuration.
 *
 * \param ifindex  [IN] Interface index (which can be decomposed into stack identifier and physical port number)
 * \param instance [IN] Instance (1..VTSS_APPL_SFLOW_INSTANCE_CNT)
 * \param cfg      [IN] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_counter_poller_cfg_set(vtss_ifindex_t ifindex, uint16_t instance, const vtss_appl_sflow_cp_t *const cfg);

/**
 * \brief Restore to default counter polling configuration.
 *
 * \param ifindex  [IN] Interface index (which can be decomposed into stack identifier and physical port number)
 * \param instance [IN] Instance (1..VTSS_APPL_SFLOW_INSTANCE_CNT)
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_counter_poller_cfg_del(vtss_ifindex_t ifindex, uint16_t instance);

/**
 * \brief SFLOW Instance iterator.
 *
 * \param prev_ifindex   [IN]  Previous Interface index (which can be decomposed into stack identifier and physical port number)
 * \param next_ifindex   [OUT] Next Interface index.
 * \param prev_instance  [IN]  Previous Instance (1..VTSS_APPL_SFLOW_INSTANCE_CNT)
 * \param next_instance  [OUT] Next Instance (1..VTSS_APPL_SFLOW_INSTANCE_CNT)
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_instance_itr(const vtss_ifindex_t *const prev_ifindex,
                                     vtss_ifindex_t *const next_ifindex,
                                     const uint16_t *const prev_instance,
                                     uint16_t *const next_instance);

/**
 * \brief Get receiver statistics
 *
 * \param rcvr_idx   [IN]  Receiver Index
 * \param statistics [OUT] Statistics.
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_rcvr_statistics_get(uint32_t rcvr_idx, vtss_appl_sflow_rcvr_statistics_t *const statistics);

/**
 * \brief Clear receiver statistics
 *
 * \param rcvr_idx   [IN]  Receiver Index
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_rcvr_statistics_clear(uint32_t rcvr_idx);

/**
 * \brief Get switch counter polling and flow sampling statistics
 *
 * Notice that the instance indices in vtss_appl_sflow_port_statistics_t are 0-based.
 *
 * \param ifindex    [IN]  Switch identifier and physical port index
 * \param instance   [IN]  valid sflow instance number
 * \param statistics [OUT] Statistics
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_instance_statistics_get(
    vtss_ifindex_t ifindex,
    uint16_t instance,
    vtss_appl_sflow_instance_statistics_t *const statistics
);

/**
 * \brief Clear per-instance per-port counter polling and flow sampling statistics
 *
 * \param ifindex    [IN] Interface index (which can be decomposed into stack identifier and physical port number)
 * \param instance   [IN] Instance (1..N)
 *
 * \return VTSS_RC_OK on success. Anything else on error.
 */
mesa_rc vtss_appl_sflow_instance_statistics_clear(vtss_ifindex_t ifindex, uint16_t instance);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_APPL_SFLOW_H_ */
