/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_ETH_LINK_OAM_API_H_
#define _VTSS_ETH_LINK_OAM_API_H_

#include "vtss_eth_link_oam_client.h"   /* For ETH Link OAM Client infrastructure */

/********************************************************/
/* Eth Link OAM Management configuration defaults       */
/********************************************************/
/* Default Port Link OAM control */
#define VTSS_ETH_LINK_OAM_DEFAULT_PORT_CONTROL              VTSS_APPL_ETH_LINK_OAM_CONTROL_DISABLE

/* Default Port Link OAM mode */
#define VTSS_ETH_LINK_OAM_DEFAULT_PORT_MODE                 VTSS_APPL_ETH_LINK_OAM_MODE_PASSIVE

/* Default MIB retrieval support */
#define VTSS_ETH_LINK_OAM_DEFAULT_PORT_MIB_RETRIEVAL_SUPPORT  FALSE

/* Default Remote loopback support */
#define VTSS_ETH_LINK_OAM_DEFAULT_PORT_REMOTE_LOOPBACK_SUPPORT  FALSE

/* Default Link monitoring support */
#define VTSS_ETH_LINK_OAM_DEFAULT_PORT_LINK_MONITORING_SUPPORT TRUE

/* Default Window Period */
#define VTSS_ETH_LINK_OAM_DEFAULT_EVENT_WINDOW_CONF                  1

/* Default Threshold Period */
#define VTSS_ETH_LINK_OAM_DEFAULT_EVENT_THRESHOLD_CONF               1

/* Default RxPacket Threshold*/
#define VTSS_ETH_LINK_OAM_DEFAULT_EVENT_RXTHRESHOLD_CONF             0

/* Default Minimum Event Secs Summary Window*/
#define VTSS_ETH_LINK_OAM_DEFAULT_EVENT_SECS_SUMMARY_WINDOW_MIN      60

/* Maximum Event Secs Summary Window*/
#define VTSS_ETH_LINK_OAM_DEFAULT_EVENT_SECS_SUMMARY_WINDOW_MAX      900

/* Default Minimum Event Secs Summary Threshold*/
#define VTSS_ETH_LINK_OAM_DEFAULT_EVENT_SECS_SUMMARY_THRESHOLD_MIN  1

/* Maximum Event Secs Summary Threshold */
#define VTSS_ETH_LINK_OAM_DEFAULT_EVENT_SECS_SUMMARY_THRESHOLD_MAX  0xffff
/********************************************************/
/* Eth Link OAM Management configuration data structures*/
/********************************************************/

/* This configuration parameters is same for all the link events, the usage
 * of these values changes based on the context                              */
typedef struct eth_link_oam_link_frame_error_event_conf {
    u16       window;
    u32       threshold;
} vtss_eth_link_oam_link_frame_error_event_conf_t;

typedef struct eth_link_oam_link_symbol_period_error_event_conf {
    u64       window;
    u64       threshold;
    u32       rx_packets;
} vtss_eth_link_oam_link_symbol_period_error_event_conf_t;

typedef struct eth_link_oam_link_frame_period_error_event_conf {
    u32       window;
    u32       threshold;
    u32       rx_packets;
} vtss_eth_link_oam_link_frame_period_error_event_conf_t;

typedef struct eth_link_oam_link_frame_error_secs_summary_event_conf {
    u16       window;
    u32       threshold;
} vtss_eth_link_oam_link_frame_error_secs_summary_event_conf_t;

typedef struct eth_link_oam_conf {
    vtss_eth_link_oam_control_t                   oam_control;
    vtss_eth_link_oam_mode_t                      oam_mode;
    BOOL                                          oam_mib_retrieval_support;
    BOOL                                          oam_remote_loop_back_support;
    BOOL                                          oam_link_monitoring_support;
    vtss_eth_link_oam_link_frame_error_event_conf_t  oam_error_frame_event_conf;
    vtss_eth_link_oam_link_symbol_period_error_event_conf_t oam_symbol_period_event_conf;
    vtss_eth_link_oam_link_frame_period_error_event_conf_t  oam_frame_period_event_conf;
    vtss_eth_link_oam_link_frame_error_secs_summary_event_conf_t oam_error_frame_secs_summary_event_conf;
} vtss_eth_link_oam_conf_t;

typedef struct eth_link_oam_port_conf {
    vtss_eth_link_oam_conf_t  oam_conf;
} vtss_eth_link_oam_port_conf_t;

typedef struct eth_link_oam_mgmt_conf {
    BOOL                          ready;  /* Link OAM Initited & we're acting primary switch */
    vtss_flag_t                   control_flags;   /* Link OAM thread control */
    critd_t                       oam_lock;   /* Global module/API protection */
    CapArray<vtss_eth_link_oam_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf;
} vtss_eth_link_oam_mgmt_conf_t;


/***********************************************************************************/
/* Eth Link OAM mgmt configuration Macros                                          */
/***********************************************************************************/

#define PORT_CONF(port_no)                          (\
                (&link_oam_conf.port_conf[port_no].oam_conf))
#define PORT_CONTROL_CONF(port_no)                  (\
                (&link_oam_control_conf.port_conf[port_no]))
#define PORT_CONTROL_OAM_CONF(port_no)              (\
                (&link_oam_control_conf.port_conf[port_no].oam_control_conf))
#define REMOTE_PORT_CONTROL_OAM_CONF(port_no)  (\
                (&link_oam_control_conf.port_conf[port_no].oam_remote_control_conf))

#define SET_MGMT_PORT_CONTROL(port_no, enable)  (\
                         (PORT_CONF(port_no)->oam_control = enable))

#define GET_MGMT_PORT_CONTROL(port_no)          (\
                       (PORT_CONF(port_no)->oam_control))

#define SET_MGMT_PORT_MODE(port_no, mode)       (\
                       (PORT_CONF(port_no)->oam_mode = mode))

#define GET_MGMT_PORT_MODE(port_no)             (\
                      (PORT_CONF(port_no)->oam_mode))

#define SET_MGMT_PORT_MIB_RETRIEVAL_SUPPORT(port_no, conf) (\
             (PORT_CONF(port_no)->oam_mib_retrieval_support = conf))
#define GET_MGMT_PORT_MIB_RETRIEVAL_SUPPORT(port_no)   (\
             (PORT_CONF(port_no)->oam_mib_retrieval_support))

#define SET_MGMT_PORT_REMOTE_LOOPBACK_SUPPORT(port_no, conf) (\
          (PORT_CONF(port_no)->oam_remote_loop_back_support= conf))

#define GET_MGMT_PORT_REMOTE_LOOPBACK_SUPPORT(port_no) (\
          (PORT_CONF(port_no)->oam_remote_loop_back_support))

#define SET_MGMT_PORT_LINK_MONITORING_SUPPORT(port_no, conf) (\
                (PORT_CONF(port_no)->oam_link_monitoring_support= conf))

#define GET_MGMT_PORT_LINK_MONITORING_SUPPORT(port_no)   (\
                (PORT_CONF(port_no)->oam_link_monitoring_support))


#define GET_MGMT_PORT_ERROR_FRAME_EVENT_FRAME_WINDOW(port_no) (\
                (PORT_CONF(port_no)->oam_error_frame_event_conf.window))

#define SET_MGMT_PORT_ERROR_FRAME_EVENT_FRAME_WINDOW(port_no, conf) (\
               (GET_MGMT_PORT_ERROR_FRAME_EVENT_FRAME_WINDOW(port_no) = conf))

#define SET_MGMT_PORT_ERROR_FRAME_EVENT_FRAME_THRESHOLD(port_no, conf) (\
               (PORT_CONF(port_no)->oam_error_frame_event_conf.threshold = conf))

#define GET_MGMT_PORT_ERROR_FRAME_EVENT_FRAME_THRESHOLD(port_no) (\
               (PORT_CONF(port_no)->oam_error_frame_event_conf.threshold))


#define SET_MGMT_PORT_SYMBOL_PERIOD_ERROR_EVENT_FRAME_WINDOW(port_no, conf) (\
              (PORT_CONF(port_no)->oam_symbol_period_event_conf.window = conf))

#define GET_MGMT_PORT_SYMBOL_PERIOD_ERROR_EVENT_FRAME_WINDOW(port_no) (\
              (PORT_CONF(port_no)->oam_symbol_period_event_conf.window))

#define SET_MGMT_PORT_SYMBOL_PERIOD_ERROR_EVENT_FRAME_THRESHOLD(port_no, conf) (\
             (PORT_CONF(port_no)->oam_symbol_period_event_conf.threshold = conf))

#define GET_MGMT_PORT_SYMBOL_PERIOD_ERROR_EVENT_FRAME_THRESHOLD(port_no) (\
             (PORT_CONF(port_no)->oam_symbol_period_event_conf.threshold))

#define SET_MGMT_PORT_SYMBOL_PERIOD_ERROR_EVENT_RXPACKETS_THRESHOLD(port_no, conf) (\
             (PORT_CONF(port_no)->oam_symbol_period_event_conf.rx_packets = conf))

#define GET_MGMT_PORT_SYMBOL_PERIOD_ERROR_EVENT_RXPACKETS_THRESHOLD(port_no) (\
             (PORT_CONF(port_no)->oam_symbol_period_event_conf.rx_packets))

#define SET_MGMT_PORT_FRAME_PERIOD_ERROR_EVENT_FRAME_WINDOW(port_no, conf) (\
             (PORT_CONF(port_no)->oam_frame_period_event_conf.window = conf))

#define GET_MGMT_PORT_FRAME_PERIOD_ERROR_EVENT_FRAME_WINDOW(port_no) (\
             (PORT_CONF(port_no)->oam_frame_period_event_conf.window))

#define SET_MGMT_PORT_FRAME_PERIOD_ERROR_EVENT_FRAME_THRESHOLD(port_no, conf) (\
           (PORT_CONF(port_no)->oam_frame_period_event_conf.threshold = conf))

#define GET_MGMT_PORT_FRAME_PERIOD_ERROR_EVENT_FRAME_THRESHOLD(port_no) (\
           (PORT_CONF(port_no)->oam_frame_period_event_conf.threshold))

#define SET_MGMT_PORT_FRAME_PERIOD_ERROR_EVENT_RXPACKETS_THRESHOLD(port_no, conf) (\
          (PORT_CONF(port_no)->oam_frame_period_event_conf.rx_packets = conf))

#define GET_MGMT_PORT_FRAME_PERIOD_ERROR_EVENT_RXPACKETS_THRESHOLD(port_no) (\
          (PORT_CONF(port_no)->oam_frame_period_event_conf.rx_packets))

/* Seconds Summary */
#define SET_MGMT_PORT_ERROR_FRAME_SECS_SUMMARY_EVENT_FRAME_WINDOW(port_no, conf) (\
         (PORT_CONF(port_no)->oam_error_frame_secs_summary_event_conf.window = conf))

#define GET_MGMT_PORT_ERROR_FRAME_SECS_SUMMARY_EVENT_FRAME_WINDOW(port_no) (\
         (PORT_CONF(port_no)->oam_error_frame_secs_summary_event_conf.window))

#define SET_MGMT_PORT_ERROR_FRAME_SECS_SUMMARY_EVENT_FRAME_THRESHOLD(port_no, conf) (\
         (PORT_CONF(port_no)->oam_error_frame_secs_summary_event_conf.threshold = conf))

#define GET_MGMT_PORT_ERROR_FRAME_SECS_SUMMARY_EVENT_FRAME_THRESHOLD(port_no) (\
         (PORT_CONF(port_no)->oam_error_frame_secs_summary_event_conf.threshold))

VTSS_BEGIN_HDR

/************************************************************************************/
/* Eth Link OAM Module mgmt function prototypes                                     */
/************************************************************************************/

/* Set Link OAM Port defaults                                                       */
u32 vtss_eth_link_oam_default_set(void);

/* ready:     Enable/Disable the readiness of the Module                            */
/* Set Module Ready flag                                                            */
u32 vtss_eth_link_oam_ready_conf_set(BOOL ready);

/* ready:     Get the readiness of the Module                                       */
/* Set Module Ready flag                                                            */
u32 vtss_eth_link_oam_ready_conf_get(BOOL *ready);

/* port_no:    l2 port number                                                        */
/* Initialize the port information                                                   */
u32 vtss_eth_link_oam_mgmt_port_conf_init(const u32 port_no);

/* port_no:    l2 port number                                                        */
/* conf:       Enable/Disable the OAM Control                                        */
/* Enables/Disables the OAM Control on the port                                      */
u32 vtss_eth_link_oam_mgmt_port_control_conf_set (const u32 port_no, const vtss_eth_link_oam_control_t conf);

/* port_no:    l2 port number                                                        */
/* conf:       port's OAM Control configuration                                      */
/* Retrieves the OAM Control configuration of the port                               */
u32 vtss_eth_link_oam_mgmt_port_control_conf_get (const u32 port_no, vtss_eth_link_oam_control_t *conf);

/* port_no:    l2 port number                                                        */
/* conf:       Active or Passive mode of the port                                    */
/* init_flag:  Flag to indicate initilization                                        */
/* Configures the Port OAM mode to Active or Passive                                 */
u32 vtss_eth_link_oam_mgmt_port_mode_conf_set (const u32 port_no, const vtss_eth_link_oam_mode_t conf, const BOOL init_flag);

/* port_no:    l2 port number                                                       */
/* conf:       Active or Passive mode of the port                                   */
/* Retrieves the Port OAM mode of the port                                          */
u32 vtss_eth_link_oam_mgmt_port_mode_conf_get (const u32 port_no, vtss_eth_link_oam_mode_t *conf);

/* port_no:    l2 port number                                                        */
/* conf:       Port's MIB retrieval support configuration                            */
/* Configures the Port MIB retrieval support                                         */
u32 vtss_eth_link_oam_mgmt_port_mib_retrieval_conf_set (const u32 port_no, const BOOL conf);

/* port_no:    l2 port number                                                        */
/* conf:       Port's MIB retrieval support configuration                            */
/* Retrieves the Port MIB retrieval support configuration                            */
u32 vtss_eth_link_oam_mgmt_port_mib_retrieval_conf_get (const u32 port_no, BOOL *conf);

/* port_no:    l2 port number                                                        */
/* conf:       Port's remote loopback support configuration                          */
/* init_flag:  Flag to indicate init stage or running state of protocol              */
/* Configures the Port remote loopback support                                       */
u32 vtss_eth_link_oam_mgmt_port_remote_loopback_conf_set (const u32 port_no, const BOOL conf, const BOOL init_flag);

/* port_no:    l2 port number                                                        */
/* conf:       Port's remote loopback support configuration                          */
/* Retrieves the Port remote loopback support configuration                          */
u32 vtss_eth_link_oam_mgmt_port_remote_loopback_conf_get (const u32 port_no, BOOL *conf);

/* port_no:    l2 port number                                                        */
/* conf:       Port's remote loopback support configuration                          */
/* Configures the Port remote loopback support                                       */
u32 vtss_eth_link_oam_mgmt_port_remote_loopback_oper_conf_set(const u32 port_no, const BOOL conf);

/* port_no:    l2 port number                                                        */
/* conf:       Port's link monitor support configuration                             */
/* Configures the Port link monitor support                                          */
u32 vtss_eth_link_oam_mgmt_port_link_monitoring_conf_set (const u32 port_no, const BOOL conf);

/* port_no:    l2 port number                                                        */
/* conf:       Port's MIB retrieval support configuration                            */
/* Retrieves the Port MIB retrieval support configuration                            */
u32 vtss_eth_link_oam_mgmt_port_link_monitoring_conf_get (const u32 port_no, BOOL *conf);
/* port_no:    l2 port number                                                        */
/* conf:       Port's link Error Frame Event Window                                  */
/* Retrieves the port's error frame event's window threshold                         */
u32 vtss_eth_link_oam_mgmt_port_link_error_frame_window_get (const u32  port_no, u16 *conf);

/* port_no:    l2 port number                                                        */
/* conf   :    frame event's window threshold                                        */
/* Configures the port's error frame event's window threshold                        */
u32 vtss_eth_link_oam_mgmt_port_link_error_frame_window_set (const u32 port_no, const u16  conf);

/* port_no:    l2 port number                                                        */
/* conf:       frame event's error  threshold                                        */
/* Retrieves the port's error frame event's error  threshold                         */
u32 vtss_eth_link_oam_mgmt_port_link_error_frame_threshold_get (const u32 port_no, u32 *conf);

/* port_no:    l2 port number                                                        */
/* conf:       frame event's error  threshold                                        */
/* Configures the port's error frame event's error threshold                         */
u32 vtss_eth_link_oam_mgmt_port_link_error_frame_threshold_set (const u32 port_no, const u32 conf);

/* port_no:    l2 port number                                                        */
/* conf:       symbol period's  window threshold                                     */
/* configures the symbol period's  window threshold                                  */
u32 vtss_eth_link_oam_mgmt_port_link_symbol_period_error_window_get (const u32 port_no, u64 *conf);

/* port_no:    l2 port number                                                        */
/* conf:       symbol period's  window threshold                                     */
/* retrieves the symbol period's  window threshold                                   */
u32 vtss_eth_link_oam_mgmt_port_link_symbol_period_error_window_set (const u32 port_no, const u64 conf);

/* port_no:    l2 port number                                                        */
/* conf:       symbol period event error threshold                                   */
/* retrieves the port's symbol period event error threshold                          */
u32 vtss_eth_link_oam_mgmt_port_link_symbol_period_error_threshold_get (const u32 port_no, u64 *conf);

/* port_no:    l2 port number                                                        */
/* conf:       symbol period event error threshold                                   */
/* configures the port's symbol period event error threshold                         */
u32 vtss_eth_link_oam_mgmt_port_link_symbol_period_error_threshold_set (const u32 port_no, const u64 conf);

/* port_no:    l2 port number                                                        */
/* conf:       symbol period event rx symbol count threshold                         */
/* Configures the port's symbol period event rx symbol count threshold               */
u32 vtss_eth_link_oam_mgmt_port_link_symbol_period_error_rxpackets_threshold_set (const u32 port_no, const u64 conf);
/* port_no:    l2 port number                                                        */
/* conf:       symbol period event rx symbol count threshold                         */
/* retrieves the port's symbol period event rx symbol count threshold                */
u32 vtss_eth_link_oam_mgmt_port_link_symbol_period_error_rxpackets_threshold_get (const u32 port_no, u64 *conf);

/* port_no:    l2 port number                                                        */
/* conf:       frame period window threshold                                         */
/* Retrieves the port's frame period window threshold                                */
u32 vtss_eth_link_oam_mgmt_port_link_frame_period_error_window_get (const u32  port_no, u32 *conf);

/* port_no:    l2 port number                                                        */
/* conf:       frame period window threshold                                         */
/* configures the port's frame period window threshold                               */
u32 vtss_eth_link_oam_mgmt_port_link_frame_period_error_window_set (const u32  port_no, const u32 conf);

/* port_no:    l2 port number                                                        */
/* conf:       frame period error threshold                                          */
/* Retrieves the port's frame period error threshold configuration                   */
u32 vtss_eth_link_oam_mgmt_port_link_frame_period_error_threshold_get (const u32  port_no, u32 *conf);
/* port_no:    l2 port number                                                        */
/* conf:       frame period error threshold                                          */
/* configures the port's frame period error threshold configuration                  */
u32 vtss_eth_link_oam_mgmt_port_link_frame_period_error_threshold_set (const u32  port_no, const u32  conf);

/* port_no:    l2 port number                                                        */
/* conf:       frame period error event rx  threshold                                */
/* Configures the port's frame period error event rx  threshold                      */
u32 vtss_eth_link_oam_mgmt_port_link_frame_period_error_rxpackets_threshold_set (const u32  port_no, const u64  conf);

/* port_no:    l2 port number                                                        */
/* conf:       frame period error event rx  threshold                                */
/* retrieves the port's frame period error event rx  threshold                       */
u32 vtss_eth_link_oam_mgmt_port_link_frame_period_error_rxpackets_threshold_get (const u32  port_no, u64 *conf);

/* port_no:                 l2 port number                                    */
/* port_fault_enable:       Enable/Disable port fault                         */
/* Mimics the uni-directional port fault behavior                             */
u32 vtss_eth_link_oam_mgmt_port_fault_conf_set(const u32 port_no, const BOOL port_fault_enable);

/* Error Frame Seconds Summary Event */
/* port_no:                 l2 port number                                    */
/* conf:                    seconds summary event's window configuration      */
/* Retrieves the frame seconds summary event's window configuration           */
u32 vtss_eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_get (const u32  port_no, u16 *conf);

/* port_no:                 l2 port number                                    */
/* conf:                    seconds summary event's window configuration      */
/* configures the frame seconds summary event's window configuration          */
u32 vtss_eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_set (const u32  port_no, const u16 conf);

/* port_no:                 l2 port number                                    */
/* conf:           seconds summary event's error threshold configuration      */
/* Retrieves the frame seconds summary event's error threshold configuration  */
u32 vtss_eth_link_oam_mgmt_port_link_error_frame_secs_summary_threshold_get (const u32  port_no, u16 *conf);

/* port_no:                 l2 port number                                    */
/* conf:           seconds summary event's error threshold configuration      */
/* configures the frame seconds summary event's error threshold configuration */
u32 vtss_eth_link_oam_mgmt_port_link_error_frame_secs_summary_threshold_set (const u32  port_no, const u32 conf);

/* port_no:                 l2 port number                                    */
/* status:                  Loopback status                                   */
/* Retrieve the Loopback status in accordance with RFC 4878                   */
u32 vtss_eth_link_oam_mgmt_loopback_oper_status_get(const u32 port_no, vtss_eth_link_oam_loopback_status_t *status);

/* port_no:           port number                                             */
/* mux_state:         port MUX state                                          */
/* parser_state:      port parser state                                       */
/* Configures the port's MUX/PARSER states                                    */
u32 vtss_eth_link_oam_mgmt_port_state_conf_set(const u32 port_no, const vtss_eth_link_oam_mux_state_t mux_state, const vtss_eth_link_oam_parser_state_t parser_state);

/******************************************************************************/
/*  ETH Link OAM OAM Control layer status specific management functions       */
/******************************************************************************/

/* port_no:         port number                                               */
/* flag:            OAM flag                                                  */
/* Retrieves the port's OAM Flags like link_fault,dying_gasp                  */
u32 vtss_eth_link_oam_mgmt_control_port_flags_conf_get(const u32 port_no, u8 *flag);

/* port_no:         port number                                               */
/* flag:            OAM flag                                                  */
/* enable_flag:     enable/disable the specified flag                         */
/* Sets the port's OAM Flags like link_fault,dying_gasp                       */
u32 vtss_eth_link_oam_mgmt_control_port_flags_conf_set(const u32 port_no, const u8 flag, const BOOL enable_flag);

/* port_no:     port number                                                   */
/* pdu_control: pdu control                                                   */
/* Retrieves the PDU control status of the discovery protocol                 */
u32 vtss_eth_link_oam_mgmt_control_port_pdu_control_status_get(const u32 port_no, vtss_eth_link_oam_pdu_control_t *pdu_control);

/* port_no:    port number                                                    */
/* state:      discovery state                                                */
/* Retrieves the port's discovery state                                       */
u32 vtss_eth_link_oam_mgmt_control_port_discovery_state_get(const u32 port_no, vtss_eth_link_oam_discovery_state_t *state);

/* port_no:    port number                                                    */
/* pdu_stats:  port's pdu statistics                                          */
/* Retrieves the port's standard PDU statistics                               */
u32 vtss_eth_link_oam_mgmt_control_port_pdu_stats_get(const u32 port_no, vtss_eth_link_oam_pdu_stats_t *pdu_stats);

/* port_no:    port number                                                    */
/* pdu_stats:  critical event pdu statistics                                  */
/* Retrieves the port's critical event statistics                             */
u32 vtss_eth_link_oam_mgmt_control_port_critical_event_pdu_stats_get(const u32 port_no, vtss_eth_link_oam_critical_event_pdu_stats_t *pdu_stats);

/* port_no:         port number                                               */
/* oam_client_pdu:  client pdu to be transmitted                              */
/* oam_pdu_len:     OAM PDU length to be transmitted                          */
/* oam_pdu_code:    OAM PDU code to be transmitted                            */
/* Builds and transmits the non-info OAM pdu to be transmitted                */
u32 vtss_eth_link_oam_mgmt_control_port_non_info_pdu_xmit(const u32 port_no, u8 *oam_client_pdu, const u16 oam_pdu_len, const u8 oam_pdu_code);

/* port_no:         port number                                               */
/* Resets the port counters                                                   */
u32 vtss_eth_link_oam_mgmt_control_clear_statistics(const u32 port_no);

/* port_no:         port number                                               */
/* oam_pdu:         OAM pdu                                                   */
/* pdu_len:         length of the pdu to be transmitted                       */
/* oam_code:        code of the received pdu                                  */
/* Handles the received function                                              */
u32 vtss_eth_link_oam_mgmt_control_rx_pdu_handler(const u32 port_no, const u8 *pdu, const u16 pdu_len, const u8 oam_code);

/******************************************************************************/
/*  ETH Link OAM OAM Client status specific management functions              */
/******************************************************************************/

/* port_no:    l2 port number                                                 */
/* conf:       Enable/Disable the OAM Control                                 */
/* Retrievs the port's OAM Control                                            */
u32 vtss_eth_link_oam_mgmt_client_port_control_conf_get (const u32  port_no, u8 *const conf);

/* port_no:    port number                                                    */
/* info:       local info                                                     */
/* Retrieves the Port OAM Local information                                   */
u32 vtss_eth_link_oam_mgmt_client_port_local_info_get (const u32 port_no, vtss_eth_link_oam_info_tlv_t *const local_info);

/* port_no:    port number                                                    */
/* info:       Remote info                                                    */
/* Retrieves the Port OAM Peer(remote) information                            */
u32 vtss_eth_link_oam_mgmt_client_port_remote_info_get (const u32 port_no, vtss_eth_link_oam_info_tlv_t *const remote_info);

/* port_no:    port number                                                    */
/* info:       Remote info                                                    */
/* Retrieves the Port OAM Peer(remote) MAC address                            */
u32 vtss_eth_link_oam_mgmt_client_port_remote_mac_addr_info_get (const u32 port_no, u8 *const remote_mac_addr);

/* port_no:                port number                                        */
/* local_error_info:       local error info                                   */
/* local_error_info:       Remote error info                                  */
/* Retrieves the Port's frame error  information                              */
u32 vtss_eth_link_oam_mgmt_client_port_frame_error_info_get (const u32 port_no, vtss_eth_link_oam_error_frame_event_tlv_t  *local_error_info, vtss_eth_link_oam_error_frame_event_tlv_t  *remote_error_info);

/* port_no:                port number                                        */
/* local_error_info:       local error info                                   */
/* local_error_info:       Remote error info                                  */
/* Retrieves the Port's frame period error  information                       */
u32 vtss_eth_link_oam_mgmt_client_port_frame_period_error_info_get (const u32 port_no, vtss_eth_link_oam_error_frame_period_event_tlv_t *const local_info, vtss_eth_link_oam_error_frame_period_event_tlv_t *const remote_info);

/* port_no:                port number                                        */
/* local_info:             local error info                                   */
/* local_info:             Remote error info                                  */
/* Retrieves the Port's frame period error  information                       */
u32 vtss_eth_link_oam_mgmt_client_port_symbol_period_error_info_get (const u32 port_no, vtss_eth_link_oam_error_symbol_period_event_tlv_t *const local_info, vtss_eth_link_oam_error_symbol_period_event_tlv_t *const remote_info);

/* port_no:                port number                                        */
/* local_info:             local error info                                   */
/* local_info:             Remote error info                                  */
/* Retrieves the Port's frame period error  information                       */
u32 vtss_eth_link_oam_mgmt_client_port_error_frame_secs_summary_info_get (const u32 port_no, vtss_eth_link_oam_error_frame_secs_summary_event_tlv_t *local_info, vtss_eth_link_oam_error_frame_secs_summary_event_tlv_t *remote_info);

/* port_no:    port number                                                    */
/* info:       Remote info                                                    */
/* Retrieves the Port OAM Peer(remote) information                            */
u32 vtss_eth_link_oam_mgmt_client_port_remote_seq_num_get (const u32 port_no, u16 *const remote_info);

/* port_no:           port number                                             */
/* cb:                call back function                                      */
/* buf:               buffer                                                  */
/* size:              buffer size                                             */
/* Registers a call back function to get notified on variable response        */
void vtss_eth_link_oam_mgmt_client_register_cb(const u32 port_no, void (*cb)(char *response), char *buf, u32 size);

/* port_no:           port number                                             */
/* Deregisters the call back function to get notified on variable response    */
void vtss_eth_link_oam_mgmt_client_deregister_cb(const u32 port_no);



/*************************************************************************************/
/*  ETH Link OAM platform call out interface                                         */
/*************************************************************************************/

/* This is call by ETH Link OAM upper logic to lock/unlock critical code protection */
void vtss_eth_link_oam_crit_data_lock(void);
void vtss_eth_link_oam_crit_data_unlock(void);

void vtss_eth_link_oam_crit_oper_data_lock(void);
void vtss_eth_link_oam_crit_oper_data_unlock(void);

/* System callback reboot handler to send out the dying gasp event */
void vtss_eth_link_oam_mgmt_sys_reboot_action_handler(mesa_restart_t restart);

/* To send out dying gasp per port_no */
void vtss_eth_link_oam_mgmt_sys_send_dying_gasp(mesa_port_no_t port_no);

VTSS_END_HDR

#endif /* _VTSS_ETH_LINK_OAM_API_H_ */

