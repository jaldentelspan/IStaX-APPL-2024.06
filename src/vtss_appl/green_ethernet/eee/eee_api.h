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

#ifndef __EEE_API_H__
#define __EEE_API_H__
#include "vtss/appl/eee.h"
#include "main.h" /* For MODULE_ERROR_START() */

#define EEE_FAST_QUEUES_MIN 1
#define EEE_FAST_QUEUES_MAX 8
#define EEE_FAST_QUEUES_CNT 8
#define EEE_OPTIMIZE_SUPPORT 1

//************************************************
// Definition of rc errors - See also eee_error_txt in eee.c
//************************************************
enum {
    EEE_ERROR_ISID = MODULE_ERROR_START(VTSS_MODULE_ID_EEE),
    EEE_ERROR_PORT,
    EEE_ERROR_NOT_PRIMARY_SWITCH,
    EEE_ERROR_VALUE,
    EEE_ERROR_NOT_CAPABLE,
};

#ifdef __cplusplus
extern "C" {
#endif
const char *eee_error_txt(mesa_rc rc);
#ifdef __cplusplus
}
#endif

//************************************************
// Configuration definition
//************************************************
typedef struct {
    /**
     * EEE enabled on this port?
     */
    BOOL eee_ena;

#if EEE_FAST_QUEUES_CNT > 0
    /**
     * Queues set in this mask will activate egress path as soon as any data is available.
     * Vector for enabling fast queues. bit 0 = queue 0, bit 1 = queue 1 and so on.
     */
    u8 eee_fast_queues;
#endif
} eee_port_conf_t;

// Switch configuration (configuration that are local for a switch in the stack)
typedef struct {
    /**
     * EEE configuration per port.
     */
    CapArray<eee_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port;

} eee_switch_conf_t;

inline int vtss_memcmp(const eee_switch_conf_t &a, const eee_switch_conf_t &b) {
    VTSS_MEMCMP_ELEMENT_CAP(a, b, port);
    return 0;
}

// Switch global configuration (configuration that are common for all switches in the stack)
typedef struct {
   /**
     * EEE can be configured to give most power savings or least traffic latency
     */
    BOOL optimized_for_power;
} eee_switch_global_conf_t;

/**
 * Per-port run-time properties.
 * All of them are R/O.
 */
typedef struct {
  /**
   * Is this port EEE capable?
   */
  BOOL eee_capable;

  /**
   * Is link up?
   */
  BOOL link;

  /**
   * Is it running full duplex?
   */
  BOOL fdx;

  /**
   * PHY rx part currently in power save state?
   */
  BOOL rx_in_power_save_state;

  /**
   * PHY tx part currently in power save state?
   */
  BOOL tx_in_power_save_state;

  /**
   * Link partner advertising EEE?
   */
  BOOL link_partner_eee_capable;

  /**
   * Is EEE running?
   */
  BOOL running;

  /**
   * What's the port speed?
   */
  mesa_port_speed_t speed;

  /**
   * Alternate Rx system value. Changed through debug commands.
   */
  u16  rx_tw;
  BOOL rx_tw_changed_by_debug_cmd;

  /**
   * Alternate Tx system value. Changed through debug commands.
   */
  u16  tx_tw;
  BOOL tx_tw_changed_by_debug_cmd;

  /**
   * Integer that indicates the value of Tw_sys_tx that the local system requests from the remote system.
   * This value is updated by the EEE Receiver L2 state diagram. This variable maps into the
   * aLldpXdot3LocRxTwSys attribute.
   */
  u16 LocRxSystemValue;

  /**
   * Integer that indicates the value of Tw_sys_tx that the local system can support. This value is updated
   * by the EEE DLL Transmitter state diagram. This variable maps into the aLldpXdot3LocTxTwSys
   * attribute.
   */
  u16 LocTxSystemValue;

  /**
   * Integer that indicates the current Tw_sys_tx supported by the remote system.
   */
  u16 LocResolvedRxSystemValue;

  /**
   * Integer that indicates the current Tw_sys_tx supported by the local system.
   */
  u16 LocResolvedTxSystemValue;

  /**
   * Integer that indicates the remote systems Receive Tw_sys_tx that was used by the local system to
   * compute the Tw_sys_tx that it can support. This value maps into the aLldpXdot3LocRxTwSysEcho
   * attribute.
   */
  u16 LocRxSystemValueEcho;

  /**
   * Integer that indicates the remote systems Transmit Tw_sys_tx that was used by the local system to
   * compute the Tw_sys_tx that it wants to request from the remote system. This value maps into the
   * aLldpXdot3LocTxTwSysEcho attribute.
   */
  u16 LocTxSystemValueEcho;

  /**
   * Integer that indicates the value of fallback Tw_sys_tx that the local system requests from the remote
   * system. This value is updated by the local system software.
   */
   u16 LocFbSystemValue;

  /**
   * Integer that indicates the value of Tw_sys_tx that the remote system requests from the local system.
   * This value maps from the aLldpXdot3RemRxTwSys attribute
   *
   * In other words, this is the link partner's Rx wake-up time.
   */
  u16 RemRxSystemValue;

  /**
   * Integer that indicates the value of Tw_sys_tx that the remote system can support. This value maps
   * from the aLldpXdot3RemTxTwSys attribute.
   *
   * In other words, this is the link partner's unresolved Tx wake-up time, which normally only
   * changes with link speed changes.
   */
  u16 RemTxSystemValue;

  /**
   * Integer that indicates the value of Receive Tw_sys_tx echoed back by the remote system. This value
   * maps from the aLldpXdot3RemRxTwSysEcho attribute.
   */
  u16 RemRxSystemValueEcho;

  /**
   * Integer that indicates the value [of] Transmit Tw_sys_tx echoed back by the remote system. This value
   * maps from the aLldpXdot3RemTxTwSysEcho attribute.
   *
   * In other words - once LLDP info has been exchanged once - this is a copy of what we sent in the previous
   * update as LocTxSystemValue.
   */
  u16 RemTxSystemValueEcho;

  /**
   * Integer used to store the value of Tw_sys_tx.
   */
  u16 TempRxVar;

  /**
   * Integer used to store the value of Tw_sys_tx.
   */
  u16 TempTxVar;
} eee_port_state_t;

typedef struct {
    CapArray<eee_port_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port;
} eee_switch_state_t;

#ifdef __cplusplus
extern "C" {
#endif

//************************************************
// Functions
//************************************************
mesa_rc eee_mgmt_switch_conf_get (vtss_isid_t isid, eee_switch_conf_t  *switch_conf);
mesa_rc eee_mgmt_switch_global_conf_get(eee_switch_global_conf_t *switch_global_conf);
mesa_rc eee_mgmt_switch_global_conf_set(eee_switch_global_conf_t *switch_global_conf);
mesa_rc eee_mgmt_switch_conf_set (vtss_isid_t isid, eee_switch_conf_t  *switch_conf);
mesa_rc eee_mgmt_switch_state_get(vtss_isid_t isid, eee_switch_state_t *switch_state);
mesa_rc eee_mgmt_port_state_get(vtss_isid_t isid, mesa_port_no_t port, eee_port_state_t *port_state);
void    eee_mgmt_remote_state_change(mesa_port_no_t port, u16 RemRxSystemValue, u16 RemTxSystemValue, u16 RemRxSystemValueEcho, u16 RemTxSystemValueEcho); // Should be called by LLDP, only
void    eee_mgmt_local_state_change(mesa_port_no_t port, BOOL clear, BOOL is_tx, u16 LocSystemValue);
mesa_rc eee_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __EEE_API_H__ */

