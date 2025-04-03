/*

 Vitesse sFlow software.

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

#ifndef _SFLOW_API_H_
#define _SFLOW_API_H_

#if defined(VTSS_SW_OPTION_IP)
#include "ip_api.h"
#include "vtss_network.h" /* INET6_ADDRSTRLEN */
#endif
#include "vtss/appl/sflow.h" /* SFLOW Public header file */

/** \file sFlow API.
 *
 * Defines the sFlow Agent API.
 *
 * Note: None of the configuration is persisted. The reason
 * can be read under #sflow_rcvr_t::timeout.
 *
 * This means that the configuration will be lost if you boot or
 * change primary switch.
 */

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Definition of return code errors.
 * See also sflow_error_txt in sflow.c
 */
enum {
    SFLOW_ERROR_ISID = MODULE_ERROR_START(VTSS_MODULE_ID_SFLOW),
    SFLOW_ERROR_NOT_PRIMARY_SWITCH,
    SFLOW_ERROR_PORT,
    SFLOW_ERROR_INSTANCE,
    SFLOW_ERROR_RECEIVER,
    SFLOW_ERROR_RECEIVER_IDX,
    SFLOW_ERROR_RECEIVER_VS_SAMPLING_DIRECTECTION,
    SFLOW_ERROR_RECEIVER_OWNER,
    SFLOW_ERROR_RECEIVER_TIMEOUT,
    SFLOW_ERROR_RECEIVER_DATAGRAM_SIZE,
    SFLOW_ERROR_RECEIVER_HOSTNAME,
    SFLOW_ERROR_RECEIVER_UDP_PORT,
    SFLOW_ERROR_RECEIVER_SOCKET_CREATE,
    SFLOW_ERROR_RECEIVER_CP_INSTANCE,
    SFLOW_ERROR_RECEIVER_FS_INSTANCE,
    SFLOW_ERROR_DATAGRAM_VERSION,
    SFLOW_ERROR_RECEIVER_ACTIVE,
    SFLOW_ERROR_AGENT_IP,
    SFLOW_ERROR_ARGUMENT,
};
const char *sflow_error_txt(mesa_rc rc);

/**
 * Setting the number of receivers to a value smaller than the number of
 * instances per port doesn't make sense, since none will be able to
 * take advantage of the additional per-port instances, because the code
 * doesn't allow the same receiver to take control of more than one per-port
 * instance at a time.
 */
#if VTSS_APPL_SFLOW_INSTANCE_CNT > VTSS_APPL_SFLOW_RECEIVER_CNT
#error "Setting SFLOW_INSTANCE_CNT > SFLOW_RECEIVER_CNT doesn't make sense"
#endif

#if VTSS_APPL_SFLOW_INSTANCE_CNT == 1
/**
 * This is the index to use if there's only one instance per port.
 */
#define SFLOW_INSTANCE_IDX 1
#endif

#if VTSS_APPL_SFLOW_RECEIVER_CNT == 1
/**
 * This is the index to use if there's only one receiver.
 */
#define SFLOW_RECEIVER_IDX 1
#endif

/* To support both IPv4, IPv6, and a valid hostname (for CLI and Web, but not SNMP) */
#define SFLOW_HOSTNAME_LEN                   255

/**
 * sflow_rcvr_t
 * Uniquely identifies one sFlow Receiver (a.k.a. collector).
 *
 * Up to VTSS_APPL_SFLOW_RECEIVER_CNT receivers can be configured.
 */
typedef struct {
    /**
     * The entity making use of this receiver table entry.
     * Set to none when empty.
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
     *   to persist the receiver configuration. How would
     *   you maintain a timeout across boots? One could reserve
     *   0xFFFFFFFF for a receiver that never times out, of course,
     *   but that's beyond the sFlow Spec.
     */
    u32 timeout;

    /**
     * The maximum number of data bytes that can be sent in
     * a single datagram. The manager should set this value
     * to avoid fragmentation of the sFlow datagrams.
     *
     * Valid values: [VTSS_APPL_SFLOW_RECEIVER_DATAGRAM_SIZE_MIN; VTSS_APPL_SFLOW_RECEIVER_DATAGRAM_SIZE_MAX].
     * Default: VTSS_APPL_SFLOW_RECEIVER_DATAGRAM_SIZE_DEFAULT.
     */
    u32 max_datagram_size;

    /**
     * IPv4/IPv6 address or hostname of receiver to which the datagrams are sent.
     */
    char hostname[SFLOW_HOSTNAME_LEN];

    /**
     * UDP port number on the receiver to send the datagrams to.
     *
     * Valid values: [1; 65535].
     * Default: 6343.
     */
    u16 udp_port;

    /**
     * The version of sFlow datagrams that we should send.
     * Valid values [5; 5] (we only support v. 5).
     * Default: 5.
     */
    i32 datagram_version;
} sflow_rcvr_t;

/**
 * sflow_rcvr_info_t
 */
typedef struct {
    /**
     * The IP address of the current receiver, as binary and including type.
     * If there is no current receiver, the type is IPv4 and address 0.0.0.0
     */
    mesa_ip_addr_t ip_addr;

    /**
     * This is an IPv4 or IPv6 representation of #ip_addr.
     */
    char ip_addr_str[INET6_ADDRSTRLEN];

    /**
     * The number of seconds left for this receiver.
     */
    u32  timeout_left;
} sflow_rcvr_info_t;

typedef vtss_appl_sflow_fs_t sflow_fs_t;

typedef vtss_appl_sflow_cp_t sflow_cp_t;

typedef vtss_appl_sflow_rcvr_statistics_t sflow_rcvr_statistics_t;

/**
 * vtss_appl_sflow_port_statistics_t
 * Per-port statistics.
 * The statistics get cleared automatically for
 * a given instance on the port, when that instance
 * changes configuration (also when just the interval
 * is changed). fs_rx/tx and cp get cleared separately.
 */
typedef struct {
    /**
     * Number of flow samples received on the primary switch that
     * were Rx sampled.
     */
    u64 fs_rx[VTSS_APPL_SFLOW_INSTANCE_CNT];

    /**
     * Number of flow samples received on the primary switch that
     * were Tx sampled.
     */
    u64 fs_tx[VTSS_APPL_SFLOW_INSTANCE_CNT];

    /**
     * Number of counter samples received on the primary switch.
     */
    u64 cp[VTSS_APPL_SFLOW_INSTANCE_CNT];
} vtss_appl_sflow_port_statistics_t;

/**
 * sflow_switch_statistics_t
 * Per-switch statistics.
 * Bundles together all port statistics.
 */
typedef struct {
    /**
     * One port statistics set of counters per port.
     */
    CapArray<vtss_appl_sflow_port_statistics_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port;
} vtss_appl_sflow_switch_statistics_t;

typedef vtss_appl_sflow_port_statistics_t sflow_port_statistics_t;

typedef vtss_appl_sflow_switch_statistics_t sflow_switch_statistics_t;

/**
 * sflow_agent_t
 *
 * Holds part of the agent configuration, particularly
 * for use by SNMP.
 */
typedef struct {
    /**
     * Uniquely identifies the version and implementation of the sFlow MIB.
     * Format: <MIB Version>;<Organization>;<Software Revision>
     */
    const char *version;

    /**
     * This switch's IP address, or rather, by default the IPv4 loopback address,
     * but it can be changed from CLI and Web. Will not get persisted.
     * The aggregate type allows for holding either an IPv4 and IPv6 address.
     */
    mesa_ip_addr_t agent_ip_addr;
} sflow_agent_t;

/**
 * \brief Get general agent information.
 *
 * \param agent_cfg [OUT] Current configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to a string.
 */
mesa_rc sflow_mgmt_agent_cfg_get(sflow_agent_t *agent_cfg);

/**
 * \brief Set general agent information.
 *
 * \param agent_cfg [IN] Current configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to a string.
 */
mesa_rc sflow_mgmt_agent_cfg_set(sflow_agent_t *agent_cfg);

/**
 * \brief Get receiver configuration.
 *
 * Along with the configuration, we also pass the resolved IP address
 * of the receiver - in case the user has inserted a hostname in a
 * previous call to sflow_mgmt_rcvr_cfg_set().
 *
 * \param rcvr_idx [IN]  Receiver index ([1; SFLOW_RECEIVER_CNT]).
 * \param rcvr_cfg [OUT] Current configuration.
 * \param rcvr_info [OUT] Current receiver info. May be NULL.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to a string.
 */
mesa_rc sflow_mgmt_rcvr_cfg_get(u32 rcvr_idx, sflow_rcvr_t *rcvr_cfg, sflow_rcvr_info_t *rcvr_info);

/**
 * \brief Set receiver configuration.
 *
 * \param rcvr_idx [IN] Receiver index ([1; SFLOW_RECEIVER_CNT]).
 * \param rcvr_cfg [IN] New configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to get error string.
 */
mesa_rc sflow_mgmt_rcvr_cfg_set(u32 rcvr_idx, sflow_rcvr_t *rcvr_cfg);

/**
 * \brief Get flow sampler configuration.
 *
 * \param isid     [IN]  Switch ID
 * \param port     [IN]  Port number
 * \param instance [IN]  Instance (1..SFLOW_INSTANCE_CNT)
 * \param cfg      [OUT] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc sflow_mgmt_flow_sampler_cfg_get(vtss_isid_t isid, mesa_port_no_t port, u16 instance, sflow_fs_t *cfg);

/**
 * \brief Set flow sampler configuration.
 *
 * \param isid     [IN] Switch ID
 * \param port     [IN] Port number
 * \param instance [IN] Instance (1..SFLOW_INSTANCE_CNT)
 * \param cfg      [IN] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc sflow_mgmt_flow_sampler_cfg_set(vtss_isid_t isid, mesa_port_no_t port, u16 instance, sflow_fs_t *cfg);

/**
 * \brief Get counter polling configuration.
 *
 * \param isid     [IN]  Switch ID
 * \param port     [IN]  Port number
 * \param instance [IN]  Instance (1..SFLOW_INSTANCE_CNT)
 * \param cfg      [OUT] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc sflow_mgmt_counter_poller_cfg_get(vtss_isid_t isid, mesa_port_no_t port, u16 instance, sflow_cp_t *cfg);

/**
 * \brief Set counter polling configuration.
 *
 * \param isid     [IN] Switch ID
 * \param port     [IN] Port number
 * \param instance [IN] Instance (1..SFLOW_INSTANCE_CNT)
 * \param cfg      [IN] Configuration.
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc sflow_mgmt_counter_poller_cfg_set(vtss_isid_t isid, mesa_port_no_t port, u16 instance, sflow_cp_t *cfg);

/**
 * \brief Get receiver statistics
 *
 * \param isid       [IN]  Switch ID
 * \param statistics [OUT] Statistics. If NULL, #clear must be TRUE.
 * \param clear      [IN]  TRUE => Clears statistics before returning them (if #statistics is non-NULL)
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc sflow_mgmt_rcvr_statistics_get(u32 rcvr_idx, sflow_rcvr_statistics_t *statistics, BOOL clear);

/**
 * \brief Get switch counter polling and flow sampling statistics
 *
 * Notice that the instance indices in sflow_port_statistics_t are 0-based.
 *
 * \param isid       [IN]  Switch ID
 * \param statistics [OUT] Statistics
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc sflow_mgmt_switch_statistics_get(vtss_isid_t isid, sflow_switch_statistics_t *statistics);

/**
 * \brief Clear per-instance per-port counter polling and flow sampling statistics
 *
 * \param isid       [IN] Switch ID
 * \param port       [IN] Port number
 * \param instance   [IN] Instance (1..N)
 *
 * \return VTSS_RC_OK on success. Anything else on error. Use error_txt() to convert to string.
 */
mesa_rc sflow_mgmt_instance_statistics_clear(vtss_isid_t isid, mesa_port_no_t port, u16 instance);

/**
 * \brief sFlow module initialization function.
 *
 * \param data [IN]  Initialization state.
 *
 * \return VTSS_RC_OK always.
 */
mesa_rc sflow_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif
#endif /* _SFLOW_API_H_ */

