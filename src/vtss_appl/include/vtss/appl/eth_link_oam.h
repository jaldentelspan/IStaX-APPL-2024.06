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
 * \brief Public Ethernet Link OAM(802.3ah) APIs.
 * \details This header file describes public Ethernet Link OAM APIs.
 *          Operations, Administration and Maintenance (OAM) is used to 
 *          monitor the health of the network and quickly determine the location of failing links.
 */

#ifndef _VTSS_APPL_ETH_LINK_OAM_H_
#define _VTSS_APPL_ETH_LINK_OAM_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/basics/enum-descriptor.h>    // For vtss_enum_descriptor_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Link OAM admin state (Section 30.3.6.2 of IEEE Std 802.3ah).
 */
typedef enum {
    /** OAM admin state is disabled, 
        Interface does not send/receive Link OAM PDUs. 
     */
    VTSS_APPL_ETH_LINK_OAM_CONTROL_DISABLE,
    /** OAM admin state is enabled, 
        Interface sends/receives Link OAM PDUs. 
     */
    VTSS_APPL_ETH_LINK_OAM_CONTROL_ENABLE
} vtss_appl_eth_link_oam_control_t;

/**
 * \brief Link OAM mode (Section 30.3.6.1.3 of IEEE Std 802.3ah).
 */
typedef enum {
    /** Passive mode does not initiate discovery process. */
    VTSS_APPL_ETH_LINK_OAM_MODE_PASSIVE,
    /** Active mode initiates Discovery process. */
    VTSS_APPL_ETH_LINK_OAM_MODE_ACTIVE
} vtss_appl_eth_link_oam_mode_t;

/**
 *  \brief Link OAM port configurable parameters.
 */
typedef struct {
    /** Indicates whether OAM Admin state is enabled or disabled. */
    vtss_appl_eth_link_oam_control_t     admin_state;
    /** Indicates whether OAM mode is active or passive. */
    vtss_appl_eth_link_oam_mode_t        mode;
    /** Indicate whether the MIB retrieval support is enabled, 
        as defined under Section 30.3.6.1.6 of IEEE Std 802.3ah. 
     */
    mesa_bool_t                                 mib_retrieval_support;
    /** Indicate whether the remote loopback support is enabled.
        Enabling the loopback support will allow the device 
        to execute the remote loopback command that helps in the fault detection, 
        as defined under Section 30.3.6.1.6 of IEEE Std 802.3ah. 
     */
    mesa_bool_t                                 remote_loopback_support;
    /** Indicate whether the link monitoring support is enabled,
        as defined under Section 30.3.6.1.6 of IEEE Std 802.3ah. 
     */
    mesa_bool_t                                 link_monitoring_support;
} vtss_appl_eth_link_oam_port_conf_t;

/**
 *  \brief Link OAM port event configurable parameters.
 */
typedef struct {
    /** Error frame event window indicates 
        the duration of the monitoring period in terms of seconds.
     */
    uint16_t error_frame_window;                      
    /** Error frame event threshold indicates 
        the number of permissible errors frames in the period defined by error frame window. 
     */
    uint32_t error_frame_threshold;
    /** Symbol period error event window indicates 
        the duration of the monitoring period symbol errors in terms of seconds.
     */
    uint64_t symbol_period_error_window; 
    /** Symbol period error event threshold indicates 
        the number of permissible symbol errors frames in the period defined by symbol period error window.
     */
    uint64_t symbol_period_error_threshold;
    /** Error frame second summary window indicates 
        the duration of the monitoring period in terms of seconds.
     */
    uint16_t error_frame_second_summary_window;
    /** Error frame second summary threshold indicates 
        the number of permissible error frame seconds in the period 
        defined by error frame second summary window.
     */
    uint16_t error_frame_second_summary_threshold;
} vtss_appl_eth_link_oam_port_event_conf_t;

/**
 *  \brief Link OAM port PDU statistics
 *         (Section 30.3.6.1.18 to 30.3.6.1.33 of IEEE Std 802.3ah). 
 */
typedef struct {
    /** Tx unsupported codes PDUs. */
    uint32_t unsupported_codes_tx;               
    /** Rx unsupported codes PDUs. */
    uint32_t unsupported_codes_rx;               
    /** Tx information PDUs. */
    uint32_t information_tx;                     
    /** Rx information PDUs. */
    uint32_t information_rx;                     
    /** Tx unique event notification PDUs. */
    uint32_t unique_event_notification_tx;       
    /** Rx unique event notification PDUs. */
    uint32_t unique_event_notification_rx;       
    /** Tx duplicate event notification PDUs. */
    uint32_t duplicate_event_notification_tx;    
    /** Rx duplicate event notification PDUs. */
    uint32_t duplicate_event_notification_rx;    
    /** Tx loopback control PDUs. */
    uint32_t loopback_control_tx;                
    /** Rx loopback control PDUs. */
    uint32_t loopback_control_rx;                
    /** Tx variable request PDUs. */
    uint32_t variable_request_tx;                
    /** Rx variable request PDUs. */
    uint32_t variable_request_rx;
    /** Tx variable response PDUs. */
    uint32_t variable_response_tx;               
    /** Rx variable response PDUs. */
    uint32_t variable_response_rx;               
    /** Tx organization specific PDUs. */
    uint32_t org_specific_tx;                    
    /** Rx organization specific PDUs. */
    uint32_t org_specific_rx;                    
} vtss_appl_eth_link_oam_statistics_t;

/**
 *  \brief Link OAM port critical link event PDU statistics
 *         (Section 30.3.6.1.10 to 30.3.6.1.12 of IEEE Std 802.3ah).
 */
typedef struct {
    /** Tx link fault PDUs. */
    uint32_t link_fault_tx;      
    /** Rx link fault PDUs. */
    uint32_t link_fault_rx;      
    /** Tx critical event PDUs. */
    uint32_t critical_event_tx;  
    /** Rx critical event PDUs. */
    uint32_t critical_event_rx;  
    /** Tx dying gasp PDUs. */
    uint32_t dying_gasp_tx;      
    /** Rx dying gasp PDUs. */
    uint32_t dying_gasp_rx;      
} vtss_appl_eth_link_oam_crit_link_event_statistics_t;

/**
 *  \brief Link OAM remote loopback test parameters.
 *   OAM remote loopback can be used for fault localization and link performance testing.
 *   During loopback testing local device will send test PDUs to peer device,
 *   Peer device looped back test PDUs to local device without altering any field of the frames.   
 */
typedef struct {
    /** Used to Start/Stop remote loopback test on a interface. */
    mesa_bool_t loopback_test;
} vtss_appl_eth_link_oam_remote_loopback_test_t;

/** 
  * \brief Link OAM Discovery State.
  *        Indicates the current state of the discovery process, 
           as defined with 57.5 (IEEE Std 802.3ah) state machine.      
 */
typedef enum {
    /** Fault state. */
    VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_FAULT,
    /** Active send state. */
    VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_ACTIVE_SEND_LOCAL, 
    /** Passive wait state. */
    VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_PASSIVE_WAIT, 
    /** Local remote state. */
    VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_SEND_LOCAL_REMOTE, 
    /** Local remote OK state. */
    VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_SEND_LOCAL_REMOTE_OK, 
    /** Send any state. */
    VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_SEND_ANY,
    /** Unknow state. */
    VTSS_APPL_ETH_LINK_OAM_DISCOVERY_STATE_LAST
} vtss_appl_eth_link_oam_discovery_state_t;

/** 
  * \brief Local PDU control as specified in table 57.7 (IEEE Std 802.3ah).
 */
typedef enum {
    /** No OAM PDU transmission allowed, only receive OAM PDUs. 
     */
    VTSS_APPL_ETH_LINK_OAM_PDU_CONTROL_RX_INFO,
    /** Information OAM PDU's with Link Fault bit of Flags field and
        without Information TLVs can be transmitted.
     */
    VTSS_APPL_ETH_LINK_OAM_PDU_CONTROL_LF_INFO,
    /** Only Information OAM PDU's can be transmitted. */
    VTSS_APPL_ETH_LINK_OAM_PDU_CONTROL_INFO,
    /** Transmission of Information OAM PDUs with appropriate bits of Flags field set. */
    VTSS_APPL_ETH_LINK_OAM_PDU_CONTROL_ANY
} vtss_appl_eth_link_oam_pdu_control_t;

/** 
  * \brief Link OAM MUX states as specified in table 57.7 (IEEE Std 802.3ah).
 */

typedef enum {
    /** Multiplexer in forwarding state. */
    VTSS_APPL_ETH_LINK_OAM_MUX_FWD_STATE,
    /** Multiplexer in discarding state. */
    VTSS_APPL_ETH_LINK_OAM_MUX_DISCARD_STATE
} vtss_appl_eth_link_oam_mux_state_t;

/** 
  * \brief Link OAM Parser states as specified in table 57.7 (IEEE Std 802.3ah).
 */

typedef enum {
    /** Parser in forwarding state. */
    VTSS_APPL_ETH_LINK_OAM_PARSER_FWD_STATE,
    /** Parser in loopback state. */
    VTSS_APPL_ETH_LINK_OAM_PARSER_LB_STATE,
    /** Parser in discarding state. */
    VTSS_APPL_ETH_LINK_OAM_PARSER_DISCARD_STATE
} vtss_appl_eth_link_oam_parser_state_t;

/** Organization specific information length. */
#define VTSS_APPL_ETH_LINK_OAM_OUI_LEN            9

/**
 *  \brief Link OAM port status parameters.
 */
typedef struct {
    /** PDU control. */
    vtss_appl_eth_link_oam_pdu_control_t     pdu_control;
    /** OAM state machine discovery state. */
    vtss_appl_eth_link_oam_discovery_state_t discovery_state;
    /** Multiplexer state. */
    vtss_appl_eth_link_oam_mux_state_t       multiplexer_state;
    /** Parser state. */
    vtss_appl_eth_link_oam_parser_state_t    parser_state;
    /** PDU revision. */
    uint16_t                                      revision;
    /** Vendor Organization specific information. */
    char                                         oui[VTSS_APPL_ETH_LINK_OAM_OUI_LEN];
    /** MTU size. */
    uint16_t                                      mtu_size;
    /** Unidirectional support. */
    mesa_bool_t                                     uni_dir_support;
} vtss_appl_eth_link_oam_port_status_t;

/**
 *  \brief Link OAM peer device status parameters.
 */
typedef struct {
    /** Peer OAM mode. */
    vtss_appl_eth_link_oam_mode_t            peer_oam_mode;
    /** Peer MAC address. */
    mesa_mac_t                               peer_mac;
    /** Peer multiplexer state. */
    vtss_appl_eth_link_oam_mux_state_t       peer_multiplexer_state;
    /** Peer parser state. */
    vtss_appl_eth_link_oam_parser_state_t    peer_parser_state;
    /** Peer PDU revision. */
    uint16_t                                      peer_pdu_revision;
    /** Peer Organization specific information. */
    char                                         peer_oui[VTSS_APPL_ETH_LINK_OAM_OUI_LEN];
    /** Peer MTU size. */
    uint16_t                                      peer_mtu_size;
    /** Indicate whether unidirectional capability supported on peer device. */
    mesa_bool_t                                     peer_uni_dir_support;
    /** Indicate whether mib retrieval support is enabled on peer device. */
    mesa_bool_t                                     peer_mib_retrieval_support;
    /** Indicate whether loopback support is nebaled on peer device. */
    mesa_bool_t                                     peer_loopback_support;
    /** Indicate whether link monitoring support is enabled on peer device. */
    mesa_bool_t                                     peer_link_monitoring_support;
} vtss_appl_eth_link_oam_port_peer_status_t;

/**
 *  \brief Link OAM link event status parameters.
 */
typedef struct {
    /** Sequence number indicates
        the total number of events occurred. 
     */
    uint16_t sequence_number;
    /** Symbol period error event timestamp
        indicates the time reference when the event was generated, 
        in terms of 100 ms intervals. 
     */
    uint16_t symbol_period_error_event_timestamp;
    /** Symbol period error event window indicates 
        the number of symbols in the period. 
     */
    uint16_t symbol_period_error_event_window;
    /** Symbol period error event threshold 
        indicates the number of errored symbols in the period is required to be equal to 
        or greater than in order for the event to be generated. 
     */
    uint64_t symbol_period_error_event_threshold;
    /** Symbol period errors indicates 
        the number of symbol errors in the period. 
     */
    uint64_t symbol_period_error;
    /** Total symbol period errors indicates 
        the sum of symbol errors since the OAM sublayer was reset. 
     */
    uint64_t total_symbol_period_error;
    /** Total symbol period error events indicates 
        the number of Errored Symbol Period Event TLVs that have been generated 
        since the OAM sublayer was reset.
     */
    uint32_t total_symbol_period_error_events;
    /** Frame error event timestamp indicates the time reference when 
        the event was generated, in terms of 100 ms intervals.
     */
    uint16_t frame_error_event_timestamp;
    /** Frame error event window indicates 
        the duration of the period in terms of 100 ms intervals. 
     */
    uint16_t frame_error_event_window;
    /** Frame error event threshold indicates the number of detected errored frames 
        in the period is required to be equal to or greater than in order for the event to be generated.
     */
    uint32_t frame_error_event_threshold;
    /** Frame errors indicates 
        the number of detected errored frames in the period. 
     */
    uint32_t frame_error;
    /** Total frame errors indicates the sum of errored frames that 
        have been detected since the OAM sublayer was reset.
     */
    uint64_t total_frame_errors;
    /** Total frame error events indicates the number of Errored Frame Event TLVs that 
        have been generated since the OAM sublayer was reset.
     */
    uint32_t total_frame_errors_events;
    /** Frame period error event timestamp indicates the time reference when 
        the event was generated, in terms of 100 ms intervals.
     */
    uint16_t frame_period_error_event_timestamp;
    /** Frame period error event window indicates 
        the duration of period in terms of frames.
     */
    uint32_t frame_period_error_event_window;
    /** Frame period error event threshold indicates 
        the number of errored frames in the period is required to be equal to 
        or greater than in order for the event to be generated.
     */
    uint32_t frame_period_error_event_threshold;
    /** Frame period errors indicates the number of frame errors in the period. */
    uint32_t frame_period_errors;
    /** Total frame period errors indicates the sum of frame errors that 
        have been detected since the OAM sublayer was reset.
     */
    uint64_t total_frame_period_errors;
    /** Total frame period error events indicates the number of Errored Frame Period Event TLVs 
        that have been generated since the OAM sublayer was reset.
     */
    uint32_t total_frame_period_error_event;
    /** Error frame seconds summary event timestamp indicates 
        the time reference when the event was generated, in terms of 100 ms intervals.
     */
    uint16_t error_frame_seconds_summary_event_timestamp;
    /** Error frame seconds summary event window indicates 
        the duration of the period in terms of 100 ms intervals. 
     */
    uint16_t error_frame_seconds_summary_event_window;
    /** Error frame seconds summary event threshold indicates 
        the number of errored frame seconds in the period is required to be equal to 
        or greater than in order for the event to be generated.
     */
    uint16_t error_frame_seconds_summary_event_threshold;
    /** Error frame seconds summary errors indicates 
        the number of errored frame seconds in the period.
     */
    uint16_t error_frame_seconds_summary_errors;
    /** Total error frame seconds summary errors indicates 
        the sum of errored frame seconds that have been detected since the OAM sublayer was reset.
     */
    uint32_t total_error_frame_seconds_summary_errors;
    /** Total error frame seconds summary events indicates 
        the number of Errored Frame Seconds Summary Event TLVs that 
        have been generated since the OAM sublayer was reset. 
      */
    uint32_t total_error_frame_seconds_summary_events;
} vtss_appl_eth_link_oam_port_link_event_status_t;

/**
 *   \brief Ethernet Link OAM platform specific port capabilities.
 */
typedef struct {
        /** Indicate whether port is Ethernet Link OAM capable or not. */
        mesa_bool_t  eth_link_oam_capable;
} vtss_appl_eth_link_oam_port_capabilities_t;


/**
 * \brief Set Link OAM port configuration.
 * \param ifIndex  [IN]: Interface index.
 * \param conf     [IN]: Link OAM port configurable parameters.
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_conf_set(
        vtss_ifindex_t                            ifIndex,
        const vtss_appl_eth_link_oam_port_conf_t  *const conf
);

/**
 * \brief Get Link OAM port configuration.
 * \param ifIndex   [IN]: Interface index.
 * \param conf     [OUT]: Link OAM port configurable parameters.
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_conf_get(
        vtss_ifindex_t                     ifIndex,
        vtss_appl_eth_link_oam_port_conf_t *const conf
);

/**
 * \brief Set Link OAM port event configuration.
 * \param ifIndex       [IN]: Interface index.
 * \param eventConf     [IN]: Link OAM port event configurable parameters.
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_event_conf_set(
        vtss_ifindex_t                                  ifIndex,
        const vtss_appl_eth_link_oam_port_event_conf_t  *const eventConf
);

/**
 * \brief Get Link OAM port event configuration.
 * \param ifIndex        [IN]: Interface index.
 * \param eventConf     [OUT]: Link OAM port event configurable parameters.
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_event_conf_get(
        vtss_ifindex_t                           ifIndex,
        vtss_appl_eth_link_oam_port_event_conf_t *const eventConf
);

/**
 * \brief Set Link OAM remote loopback test parameter.
 * \param ifIndex       [IN]: Interface index.
 * \param loopbackTest  [IN]: Link OAM remote loopback test parameters.
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_remote_loopback_test_set(
        vtss_ifindex_t                                       ifIndex,
        const vtss_appl_eth_link_oam_remote_loopback_test_t  *const loopbackTest
);

/**
 * \brief Get Link OAM remote loopback test parameter.
 * \param ifIndex        [IN]: Interface index.
 * \param loopbackTest  [OUT]: Link OAM remote loopback test parameters.
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_remote_loopback_test_get(
        vtss_ifindex_t                                ifIndex,
        vtss_appl_eth_link_oam_remote_loopback_test_t *const loopbackTest
);


/**
 * \brief Get Link OAM port statistics. 
 * \param ifIndex     [IN]: Interface index.
 * \param stats      [OUT]: Port statistics.
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_statistics_get(
        vtss_ifindex_t                      ifIndex,
        vtss_appl_eth_link_oam_statistics_t *const stats
);

/**
 * \brief Get Link OAM port critical link event statistics. 
 * \param ifIndex               [IN]: Interface index.
 * \param statsCritLinkEvent   [OUT]: Port critical link event statistics.
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_critical_link_event_statistics_get(
        vtss_ifindex_t                                      ifIndex,
        vtss_appl_eth_link_oam_crit_link_event_statistics_t *const statsCritLinkEvent
);

/** \brief Get Link OAM port status.
  * \param ifIndex     [IN]: Interface index.
  * \param status     [OUT]: Port status.
  * \return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_eth_link_oam_port_status_get(
        vtss_ifindex_t                       ifIndex,
        vtss_appl_eth_link_oam_port_status_t *const status
);

/** \brief Get Link OAM peer port status.
  * \param ifIndex     [IN]: Interface index.
  * \param peerStatus [OUT]: Peer port status.
  * \return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_eth_link_oam_port_peer_status_get(
        vtss_ifindex_t                            ifIndex,
        vtss_appl_eth_link_oam_port_peer_status_t *const peerStatus
);
/** \brief Get Link OAM port event link status.
  * \param ifIndex              [IN]: Interface index.
  * \param eventLinkStatus     [OUT]: Port event link status.
  * \return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_eth_link_oam_port_link_event_status_get(
        vtss_ifindex_t                                  ifIndex,
        vtss_appl_eth_link_oam_port_link_event_status_t *const eventLinkStatus
);

/** \brief Get Link OAM port peer event link status.
  * \param ifIndex              [IN]: Interface index.
  * \param peerEventLinkStatus  [OUT]: Port peer event link status.
  * \return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_eth_link_oam_port_peer_link_event_status_get(
        vtss_ifindex_t                                  ifIndex,
        vtss_appl_eth_link_oam_port_link_event_status_t *const peerEventLinkStatus
);

/**
 * \brief Get Link OAM port capabilities
 *
 * \param ifIndex       [IN]: Interface index
 * \param capabilities [OUT]: Link OAM platform specific port capabilities
 *
 * \return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_eth_link_oam_port_capabilities_get(
        vtss_ifindex_t                             ifIndex,
        vtss_appl_eth_link_oam_port_capabilities_t *const capabilities
);

/**
 * \brief Function clearing Link OAM statistic counter for a specific port
 * \param ifIndex [IN]  The logical interface index/number.
 *
 * \return VTSS_RC_OK if counter was cleared else error code
 */
mesa_rc vtss_appl_eth_link_oam_counters_clear(
        vtss_ifindex_t ifIndex
);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // _VTSS_APPL_ETH_LINK_OAM_H_
