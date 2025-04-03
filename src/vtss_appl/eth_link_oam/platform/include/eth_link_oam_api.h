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

#ifndef _ETH_LINK_OAM_API_H_
#define _ETH_LINK_OAM_API_H_

#include "l2proto_api.h"              /* For port specific Macros  */
#include "vtss/appl/eth_link_oam.h"
#include "vtss_eth_link_oam_api.h"
#include "vtss_eth_link_oam_control_api.h"

/* Eth Link OAM Module error defintions */
typedef enum {
    ETH_LINK_OAM_RC_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_ETH_LINK_OAM),
    ETH_LINK_OAM_RC_INVALID_PARAMETER,          /* invalid parameter */
    ETH_LINK_OAM_RC_NOT_ENABLED,                /* Link OAM is not enable on the port */
    ETH_LINK_OAM_RC_ALREADY_CONFIGURED,         /* Management is applying same configuration */
    ETH_LINK_OAM_RC_NO_MEMORY,                  /* No Valid memory is available */
    ETH_LINK_OAM_RC_NOT_SUPPORTED,              /* Operation is not supported   */
    ETH_LINK_OAM_RC_NO_SUFFIFIENT_MEMORY,
    ETH_LINK_OAM_RC_INVALID_STATE,
    ETH_LINK_OAM_RC_INVALID_FLAGS,
    ETH_LINK_OAM_RC_INVALID_CODES,
    ETH_LINK_OAM_RC_INVALID_PDU_CNT,
    ETH_LINK_OAM_RC_TIMED_OUT,
} eth_link_oam_rc_t;

VTSS_BEGIN_HDR

/* Initialize module */
mesa_rc eth_link_oam_init(vtss_init_data_t *data);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       Enable/Disable the OAM Control                                 */
/* Enables/Disables the OAM Control on the port                               */
mesa_rc eth_link_oam_mgmt_port_control_conf_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const vtss_eth_link_oam_control_t conf);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       port's OAM Control configuration                               */
/* Retrieves the OAM Control configuration of the port                        */
mesa_rc eth_link_oam_mgmt_port_control_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_control_t  *conf);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       Active or Passive mode of the port                             */
/* Configures the Port OAM mode to Active or Passive                          */
mesa_rc eth_link_oam_mgmt_port_mode_conf_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const vtss_eth_link_oam_mode_t conf);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       Active or Passive mode of the port                             */
/* Retrieves the Port OAM mode of the port                                    */
mesa_rc eth_link_oam_mgmt_port_mode_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_mode_t *conf);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       Enable/disable the MIB retrieval support                       */
/* Configures the Port's MIB retrieval support                                */
mesa_rc eth_link_oam_mgmt_port_mib_retrieval_conf_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const BOOL conf);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* enable:     Port's MIB retrieval support configuration                     */
/* Retrieves the Port's MIB retrieval support                                 */
mesa_rc eth_link_oam_mgmt_port_mib_retrieval_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, BOOL  *conf);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* enable:     Port's MIB retrieval support configuration                     */
/* Retrieves the Port's MIB retrieval support                                 */
mesa_rc eth_link_oam_mgmt_port_mib_retrieval_oper_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const u8 conf);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* enable:     Port's remote loopback configuration                           */
/* Retrieves the Port's remote loopback support                               */
mesa_rc eth_link_oam_mgmt_port_remote_loopback_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, BOOL  *conf);
/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       Enable/disable the port remote loopback support                */
/* Configures the Port's remote loopback support                              */
mesa_rc eth_link_oam_mgmt_port_remote_loopback_conf_set (const vtss_isid_t isid, const mesa_port_no_t port_no, BOOL conf);
/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       Enable/disable the port link monitoring loopback support       */
/* Configures the Port's link monitoring supporting                           */
mesa_rc eth_link_oam_mgmt_port_link_monitoring_conf_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const BOOL conf);
/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       Enable/disable the port link monitoring loopback support       */
/* Retrieves the Port's link monitoring supporting                            */
mesa_rc eth_link_oam_mgmt_port_link_monitoring_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, BOOL *conf);
/* KPV */
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Event Window                           */
/* Configures the Error Frame Event Window Configuration                      */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_window_set (const vtss_isid_t isid, const u32  port_no, const u16 conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Event Window                           */
/* Retrieves the Error Frame Event Window Configuration                       */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_window_get (const vtss_isid_t isid, const u32  port_no, u16 *conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Event Threshold                        */
/* Configures the Error Frame Event Threshold Configuration                   */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_threshold_set(const vtss_isid_t isid, const u32  port_no, const u32 conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Event Threshold                        */
/* Retrieves the Error Frame Event Threshold Configuration                    */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_threshold_get (const vtss_isid_t isid, const u32  port_no, u32 *conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error Window                         */
/* Configures the Symbol Period Error Window Configuration                    */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_window_set (const vtss_isid_t isid, const u32  port_no, const u64  conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error Window                         */
/* Retrieves the Symbol Period Error Window Configuration                     */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_window_get (const vtss_isid_t isid, const u32  port_no, u64  *conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error Period Threshold               */
/* Configures the Symbol Period Error Period Threshold Configuration          */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_threshold_set (const vtss_isid_t isid, const u32  port_no, const u64  conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error Period Threshold               */
/* Retrieves the Symbol Period Error Period Threshold Configuration           */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_threshold_get (const vtss_isid_t isid, const u32  port_no, u64 *conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error RxPackets Threshold            */
/* Configures the Symbol Period Error RxPackets Threshold Configuration       */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_rxpackets_threshold_set (const vtss_isid_t isid, const u32 port_no, const u64  conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error RxPackets Threshold            */
/* Retreives the Symbol Period Error RxPackets Threshold Configuration        */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_rxpackets_threshold_get (const vtss_isid_t isid, const u32  port_no, u64 *conf);
/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error Window                          */
/* Configures the Frame Period Error Window Configuration                     */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_window_set (const vtss_isid_t isid, const u32  port_no, const u32  conf);
/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error Window                          */
/* Retrieves the Frame Period Error Window Configuration                      */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_window_get (const vtss_isid_t isid, const u32  port_no, u32  *conf);
/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error Period Threshold                */
/* Configures the Frame Period Error Period Threshold Configuration           */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_threshold_set (const vtss_isid_t isid, const u32 port_no, const u32 conf);
/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error Period Threshold                */
/* Retrieves the Frame Period Error Period Threshold Configuration            */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_threshold_get (const vtss_isid_t isid, const u32 port_no, u32 *conf);
/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error RxPackets Threshold             */
/* Configures the Frame Period Error RxPackets Threshold Configuration        */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_rxpackets_threshold_set (const vtss_isid_t isid, const u32  port_no, const u64  conf);
/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error RxPackets Threshold             */
/* Retrieves the Frame Period Error RxPackets Threshold Configuration         */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_rxpackets_threshold_get (const vtss_isid_t isid, const u32  port_no, u64 *conf);

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Seconds Summary Event Window           */
/* Configures the Error Frame Seconds Summary Event Window Configuration      */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_set (const vtss_isid_t isid, const u32 port_no, const u16 conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Seconds Summary Event Window           */
/* Retrieves the Error Frame Seconds Summary Event Window Configuration       */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_get (const vtss_isid_t isid, const u16  port_no, u16 *conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Seconds Summary Event Threshold        */
/* Configures the Error Frame Seconds Summary Event Threshold Configuration   */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_secs_summary_threshold_set(const vtss_isid_t isid, const u32 port_no, const u16 conf);
/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Seconds Summary Event Threshold        */
/* Retrieves the Error Frame Seconds Summary Event Threshold Configuration    */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_secs_summary_threshold_get (const vtss_isid_t isid, const u16 port_no, u16 *conf);


/* END */

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       Enable/disable the remote loop back operation                  */
/* Enables/disables the remote loop back operation                            */
mesa_rc eth_link_oam_mgmt_port_remote_loopback_oper_conf_set(const vtss_isid_t isid, const mesa_port_no_t port_no, const BOOL conf);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       Enable/disable the port link monitoring support                */
/* Configures the Port's link monitoring support */
mesa_rc eth_link_oam_mgmt_port_link_monitoring_conf_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const BOOL conf);
/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       port link monitoring configuration                             */
/* Retrieves the port's link monitoring  support                              */
mesa_rc eth_link_oam_mgmt_port_link_monitoring_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, BOOL *conf);

/******************************************************************************/
/* Link OAM Port's Client status reterival functions                          */
/******************************************************************************/

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* conf:       Control configuration                                          */
/* Retrieves the Port OAM Control information                                 */
mesa_rc eth_link_oam_client_port_control_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, u8 *conf);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* info:       local info                                                     */
/* Retrieves the Port OAM Local information                                   */
mesa_rc eth_link_oam_client_port_local_info_get (const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_info_tlv_t *local_info);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* info:       Remote info                                                    */
/* Retrieves the Port OAM Peer(remote) information                            */
mesa_rc eth_link_oam_client_port_remote_info_get (const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_info_tlv_t *remote_info);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* info:       Remote info                                                    */
/* Retreives the Port OAM Peer(remote) information                            */
mesa_rc eth_link_oam_client_port_remote_seq_num_get (const vtss_isid_t isid, const mesa_port_no_t port_no, u16 *remote_info);


/* isid:                   stack unit id                                      */
/* port_no:                port number                                        */
/* info:       Remote info                                                    */
/* Retrieves the Port OAM Peer(remote) MAC information                        */
mesa_rc eth_link_oam_client_port_remote_mac_addr_info_get (const vtss_isid_t isid, const mesa_port_no_t port_no, u8 *remote_mac_addr);


/* isid:                   stack unit id                                      */
/* port_no:                port number                                        */
/* local_error_info:       Local error info                                   */
/* remote_error_info:      Local error info                                   */
/* Retrieves the Port OAM frame error event information                       */
mesa_rc eth_link_oam_client_port_frame_error_info_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_error_frame_event_tlv_t  *local_error_info, vtss_eth_link_oam_error_frame_event_tlv_t  *remote_error_info);

/* isid:                   stack unit id                                      */
/* port_no:                port number                                        */
/* local_error_info:       Local error info                                   */
/* remote_error_info:      Local error info                                   */
/* Retrieves the Port OAM frame period error event information                */
mesa_rc eth_link_oam_client_port_frame_period_error_info_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_error_frame_period_event_tlv_t  *local_info, vtss_eth_link_oam_error_frame_period_event_tlv_t   *remote_info);

/* isid:                   stack unit id                                      */
/* port_no:                port number                                        */
/* local_error_info:       Local error info                                   */
/* remote_error_info:      Local error info                                   */
/* Retrieves the Port OAM frame symbol error event information                */
mesa_rc eth_link_oam_client_port_symbol_period_error_info_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_error_symbol_period_event_tlv_t  *local_info, vtss_eth_link_oam_error_symbol_period_event_tlv_t   *remote_info);

mesa_rc eth_link_oam_client_port_error_frame_secs_summary_info_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_error_frame_secs_summary_event_tlv_t   *local_info, vtss_eth_link_oam_error_frame_secs_summary_event_tlv_t  *remote_info);


/*tx_control:  transmit control structure                                     */
/*Converts the PDU permission code to appropriate string                      */
const char *pdu_tx_control_to_str(vtss_eth_link_oam_pdu_control_t tx_control);

/*discovery_state: Discovery state of the port                                */
/*Converts the Discovery state of the port to the appropriate string           */
const char *discovery_state_to_str(vtss_eth_link_oam_discovery_state_t discovery_state);

/*mux_state:  Multiplexer state of the port                                   */
/*Converts the Multiplexer state to a  equivalent string information          */
const char *mux_state_to_str(vtss_eth_link_oam_mux_state_t mux_state);

/*parse_state:  Parser state of the port                                      */
/*Converts the Parser state to a  equivalent string information               */
const char *parser_state_to_str(vtss_eth_link_oam_parser_state_t parse_state);

/*var_response: Response received from the peer                               */
/*Prints the response received from the peer for the particular variable request*/
void vtss_eth_link_oam_send_response_to_cli(char *var_response);
/* Call out functions to lock the events */
BOOL vtss_eth_link_oam_mib_retrieval_opr_lock(void);


/******************************************************************************/
/* Link OAM Port's status reterival functions                          */
/******************************************************************************/

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* info:       Remote info                                                    */
/* Retrieves the Port OAM information                                         */
mesa_rc eth_link_oam_mgmt_port_conf_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_conf_t *conf);

/* port_no:    port number                                                    */
mesa_rc eth_link_oam_clear_statistics(l2_port_no_t l2port);

/******************************************************************************/
/* Link OAM Port's Control status reterival functions                         */
/******************************************************************************/

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* Retrieves the PDU control status of the discovery protocol                 */
mesa_rc eth_link_oam_control_layer_port_pdu_control_status_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_pdu_control_t *pdu_control);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* Retrieves the port's discovery state                                       */
mesa_rc eth_link_oam_control_layer_port_discovery_state_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_discovery_state_t *state);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* Retrieves the port's standard PDU statistics                               */
mesa_rc eth_link_oam_control_layer_port_pdu_stats_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_pdu_stats_t *pdu_stats);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* Retrieves the port's critical event statistics                             */
mesa_rc eth_link_oam_control_layer_port_critical_event_pdu_stats_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_critical_event_pdu_stats_t *pdu_stats);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* flag:       flag to be enable or disabled                                  */
/* enable_flag: enable/disable the flag                                       */
/* Enables/disables the specified flag                                        */
mesa_rc eth_link_oam_control_port_flags_conf_set(const vtss_isid_t isid, const mesa_port_no_t port_no, const u8 flag, const BOOL enable_flag);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* flags:      port's Link OAM flags                                          */
/* Retrieves the port's OAM flags status                                      */
mesa_rc eth_link_oam_control_port_flags_conf_get(const vtss_isid_t isid, const mesa_port_no_t port_no, u8 *flag);

/* isid:       stack unit id                                                  */
/* port_no:    port number                                                    */
/* loopback_status: status of the loopback                                    */
/* Retrieves the port's Loopback status in accordance with RFC 4878           */
mesa_rc eth_link_oam_port_loopback_oper_status_get(const vtss_isid_t isid, const mesa_port_no_t port, vtss_eth_link_oam_loopback_status_t *loopback_status);

/* Debug function used to add dying-gasp PDUs to linux kernel */
void vtss_eth_link_oam_add_dying_gasp_pdu(const u32 port_no);
int vtss_eth_link_oam_add_dying_gasp_trap(const u32 vid, u8 *frame, size_t len);
/* Debug function used to add all dying-gasp PDUs to linux kernel */
void vtss_eth_link_oam_add_all_dying_gasp_pdu(void);
/* Debug function used to del dying-gasp PDUs in linux kernel */
void vtss_eth_link_oam_del_dying_gasp_pdu(const int id);
/* Debug function used to del all dying-gasp PDUs in linux kernel */
void vtss_eth_link_oam_del_all_dying_gasp_pdu(void);
void vtss_snmp_dying_gasp_trap_send_handler(void);

VTSS_END_HDR

#endif /* _ETH_LINK_OAM_API_H_ */

