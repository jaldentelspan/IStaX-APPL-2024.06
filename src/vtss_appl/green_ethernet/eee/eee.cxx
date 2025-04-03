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

#include "microchip/ethernet/switch/api.h" /* For mesa_xxx() functions                  */
#include "msg_api.h"                       /* For msg_xxx() functions                   */
#include "eee_api.h"                       /* For our own definitions                   */
#include "eee.h"                           /* For trace definitions and other internals */
#include "port_api.h"                      /* For port_count_max()                      */
#include "vtss_common_iterator.hxx"
#include "port_iter.hxx"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_EEE

// The PHYs must conform to IEEE802.3az table 78-4
#define EEE_RX_TW_VALUE_1G    2
#define EEE_RX_TW_VALUE_100M 10
#define EEE_TX_TW_VALUE_1G   17
#define EEE_TX_TW_VALUE_100M 30
#define EEE_TW_VALUE_INIT    30

#define EEE_PHY_RX_WAKE_VALUE(_speed_) ((_speed_) == MESA_SPEED_1G ? EEE_RX_TW_VALUE_1G : EEE_RX_TW_VALUE_100M)
#define EEE_PHY_TX_WAKE_VALUE(_speed_) ((_speed_) == MESA_SPEED_1G ? EEE_TX_TW_VALUE_1G : EEE_TX_TW_VALUE_100M)
#define VTSS_INTERFACE_INDEX_GET(func) if (func != VTSS_RC_OK) {\
                                           T_I("Invalid interface index, ifIndex: %u", VTSS_IFINDEX_PRINTF_ARG(ifIndex));\
                                           return VTSS_RC_ERROR;\
                                       }

//****************************************
// TRACE
//****************************************
static vtss_trace_reg_t EEE_trace_reg = {
  VTSS_TRACE_MODULE_ID, "eee", "Energy Eficient Ethernet"
};

static vtss_trace_grp_t EEE_trace_grps[] = {
  [VTSS_TRACE_GRP_DEFAULT] = {
    "default",
    "Default",
    VTSS_TRACE_LVL_WARNING
  },
  [VTSS_TRACE_GRP_CLI] = {
    "cli",
    "CLI",
    VTSS_TRACE_LVL_WARNING
  }
};

VTSS_TRACE_REGISTER(&EEE_trace_reg, EEE_trace_grps);

// Configuration for the whole stack - Overall configuration (valid on the primary switch, only).
typedef struct {
  // One instance per switch in the stack of the switch configuration.
  // Indices are in the range [0; VTSS_ISID_CNT[, so all dereferences
  // must subtract VTSS_ISID_START from @isid to index correctly.
  eee_switch_conf_t switch_conf[VTSS_ISID_CNT];
  eee_switch_global_conf_t     global; // Configuration that is common for all switches in a stack
} eee_stack_conf_t;

//************************************************
// Global Variables
//************************************************
critd_t                   EEE_crit;        // Cannot be static.
eee_switch_conf_t         EEE_local_conf;  // Cannot be static.
eee_switch_global_conf_t  EEE_global_conf;  // Cannot be static.
eee_switch_state_t        EEE_local_state; // Cannot be static.
static eee_stack_conf_t   EEE_stack_conf;  // Configuration for whole stack (used when we're primary switch, only). Indexed [0; VTSS_ISID_CNT - 1[, so must subtract VTSS_ISID_START from a vtss_isid_t.
static vtss_handle_t      EEE_thread_handle;
static vtss_thread_t      EEE_thread_block;
static mesa_chip_id_t     EEE_chip_id;

static mesa_rc eee_if2ife(vtss_ifindex_t ifindex, vtss_ifindex_elm_t *ife)
{
  if (vtss_ifindex_is_port(ifindex)) {
    return vtss_ifindex_decompose(ifindex, ife);
  }
  return VTSS_RC_ERROR;
}
/****************************************************************************/
// EEE_local_state_power_save_state_update()
// Updates the local state with the rx and tx power save state
/****************************************************************************/
static void EEE_local_state_power_save_state_update(void)
{
  port_iter_t              pit;
  (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);

  while (port_iter_getnext(&pit)) {
    EEE_local_state.port[pit.iport].rx_in_power_save_state = FALSE;
    EEE_local_state.port[pit.iport].tx_in_power_save_state = FALSE;
    if (EEE_local_state.port[pit.iport].eee_capable) {

      // Get PHYs current power save state.
      if (vtss_phy_eee_power_save_state_get(NULL, pit.iport, &EEE_local_state.port[pit.iport].rx_in_power_save_state, &EEE_local_state.port[pit.iport].tx_in_power_save_state) != VTSS_RC_OK) {
        T_E("Could not get PHY power save state");
      }
    }
  }
}

/******************************************************************************/
// EEE_check_isid_port()
/******************************************************************************/
static mesa_rc EEE_check_isid_port(vtss_isid_t isid, mesa_port_no_t port, BOOL allow_local, BOOL check_port)
{
  if ((isid != VTSS_ISID_LOCAL && !VTSS_ISID_LEGAL(isid)) || (isid == VTSS_ISID_LOCAL && !allow_local)) {
    return EEE_ERROR_ISID;
  }

  if (isid != VTSS_ISID_LOCAL && !msg_switch_is_primary()) {
    return EEE_ERROR_NOT_PRIMARY_SWITCH;
  }

  if (check_port && port >= port_count_max()) {
    return EEE_ERROR_PORT;
  }

  return VTSS_RC_OK;
}

/******************************************************************************/
// EEE_state_change()
/******************************************************************************/
static void EEE_state_change(mesa_port_no_t port,
                             BOOL           local,
                             u16            RemRxSystemValue,
                             u16            RemTxSystemValue,
                             u16            RemRxSystemValueEcho,
                             u16            RemTxSystemValueEcho,
                             BOOL           callback)
{
  eee_port_state_t *p = &EEE_local_state.port[port];
  u16 NEW_RX_VALUE, NEW_TX_VALUE = 0;
  BOOL tx_update = FALSE;
  BOOL changed = local; // State is always changed when this function call is caused by local changes.

  if (p->running) {
    if ((RemTxSystemValueEcho != p->RemTxSystemValueEcho) ||
        (RemRxSystemValue     != p->RemRxSystemValue)     ||
        (RemRxSystemValueEcho != p->RemRxSystemValueEcho) ||
        (RemTxSystemValue     != p->RemTxSystemValue)) {
      changed = TRUE;
    }

    // Tx SM
    // Snippet from 802.3az-2010, 78.4.3.1
    // A transmitting link partner is said to be in sync with the receiving link partner if the presently advertised
    // value of Transmit Tw_sys_tx (aLldpXdot3LocTxTwSys/LocTxSystemValue) and the corresponding echoed value
    // (aLldpXdot3RemTxTwSysEcho/RemTxSystemValueEcho) are equal.
    // During normal operation, the transmitting link partner is in the RUNNING state. If the transmitting link
    // partner wants to initiate a change to the presently resolved value of Tw_sys_tx, the local_system_change is
    // asserted and the transmitting link partner enters the LOCAL CHANGE state where NEW_TX_VALUE is
    // computed. If the new value is smaller than the presently advertised value of Tw_sys_tx or if the transmitting
    // link partner is in sync with the receiving link partner, then it enters TX UPDATE state. Otherwise, it returns
    // to the RUNNING state.
    // If the transmitting link partner sees a change in the Tw_sys_tx requested by the receiving link partner, it
    // recognizes the request only if it is in sync with the transmitting link partner. The transmitting link partner
    // examines the request by entering the REMOTE CHANGE state where a NEW TX VALUE is computed and
    // it then enters the TX UPDATE state.
    // Upon entering the TX UPDATE state, the transmitter updates the advertised value of Transmit Tw_sys_tx with
    // NEW_TX_VALUE. If the NEW_TX_VALUE is equal to or greater than either the resolved Tw_sys_tx value
    // or the value requested by the receiving link partner then it enters the SYSTEM REALLOCATION state
    // where it updates the value of resolved Tw_sys_tx with NEW_TX_VALUE. The transmitting link partner
    // enters the MIRROR UPDATE state either from the SYSTEM REALLOCATION state or directly from the
    // TX UPDATE state. The UPDATE MIRROR state then updates the echo for the Receive Tw_sys_tx and
    // returns to the RUNNING state.
    if (local) {
      // State: LOCAL CHANGE
      p->TempRxVar = p->RemRxSystemValue;
      NEW_TX_VALUE = MAX(p->tx_tw, p->RemRxSystemValue);
      if (p->LocTxSystemValue == p->RemTxSystemValueEcho || NEW_TX_VALUE < p->LocTxSystemValue) {
        tx_update = TRUE;
      }
    } else {
      p->RemTxSystemValueEcho = RemTxSystemValueEcho;
      p->RemRxSystemValue     = RemRxSystemValue;

      if (p->RemRxSystemValue != p->TempRxVar && p->LocTxSystemValue == p->RemTxSystemValueEcho) {
        // State: REMOTE CHANGE
        // We only get here when a change in the remote end's Rx value is detected, and
        // when we're in sync (see above).
        p->TempRxVar = p->RemRxSystemValue;
        NEW_TX_VALUE = MAX(p->tx_tw, p->RemRxSystemValue);
        tx_update = TRUE;
      }
    }

    if (tx_update) {
      // State: TX UPDATE
      p->LocTxSystemValue = NEW_TX_VALUE;

      if (NEW_TX_VALUE >= p->LocResolvedTxSystemValue || NEW_TX_VALUE >= p->TempRxVar) {
        // State: SYSTEM REALLOCATION
        p->LocResolvedTxSystemValue = NEW_TX_VALUE;
      }

      // State: MIRROR UPDATE
      p->LocRxSystemValueEcho = p->TempRxVar;
    }

    // Rx SM
    // Snippet from 802.3az-2010, 78.4.3.2
    // A receiving link partner is said to be in sync with the transmitting link partner if the presently requested
    // value of Receive Tw_sys_tx and the corresponding echoed value are equal.
    // During normal operation, the receiving link partner is in the RUNNING state. If the receiving link partner
    // wants to request a change to the presently resolved value of Tw_sys_tx, the local_system_change is asserted.
    // When local_system_change is asserted or when the receiving link partner sees a change in the Tw_sys_tx
    // advertised by the transmitting link partner, it enters the CHANGE state where NEW_RX_VALUE is
    // computed. If NEW_RX_VALUE is less than either the presently resolved value of Tw_sys_tx or the presently
    // advertised value by the transmitting link partner, it enters the SYSTEM REALLOCATION state where it
    // updates the resolved value of Tw_sys_tx to NEW_RX_VALUE. The receiving link partner ultimately enters
    // the RX UPDATE state, either from the SYSTEM REALLOCATION state or directly from the CHANGE
    // state.
    // In the RX UPDATE state, it updates the presently requested value to NEW_RX_VALUE, then it updates the
    // echo for the Transmit Tw_sys_tx in the UPDATE MIRROR state and finally goes back to the RUNNING
    // state.
    if (!local) {
      p->RemRxSystemValueEcho = RemRxSystemValueEcho;
      p->RemTxSystemValue     = RemTxSystemValue;
    }
    if (local || p->RemTxSystemValue != p->TempTxVar) {
      // State: CHANGE
      p->TempTxVar = p->RemTxSystemValue;
      NEW_RX_VALUE = MAX(p->rx_tw, p->RemTxSystemValue);
      if (NEW_RX_VALUE <= p->LocResolvedRxSystemValue || NEW_RX_VALUE <= p->TempTxVar) {
        // State: SYSTEM REALLOCATION
        p->LocResolvedRxSystemValue = NEW_RX_VALUE;
      }

      // State: RX UPDATE
      p->LocRxSystemValue = NEW_RX_VALUE;
      p->LocFbSystemValue = NEW_RX_VALUE;

      // State: UPDATE MIRROR
      p->LocTxSystemValueEcho = p->TempTxVar;
    }
  }

  // Update switch-specific part and the primary switch with any changes.
  if (changed && callback) {
    EEE_CRIT_ASSERT_LOCKED();
    EEE_platform_tx_wakeup_time_changed(port, p->LocResolvedTxSystemValue);
  }
}

/******************************************************************************/
// EEE_state_init()
/******************************************************************************/
static void EEE_local_state_init(mesa_port_no_t port, const vtss_appl_port_status_t *status, BOOL callback)
{
  eee_port_state_t *p = &EEE_local_state.port[port];
  u8               lp_advertisement;

  p->link  = status->link;
  p->speed = status->speed;
  p->fdx   = status->fdx;

  if (status->link) {
    BOOL capable;
    if (vtss_phy_port_eee_capable(NULL, port, &capable) == VTSS_RC_OK && capable) {
      if (vtss_phy_eee_link_partner_advertisements_get(NULL, port, &lp_advertisement) != VTSS_RC_OK) {
        T_E("Could not get link partner EEE auto negotiation information");
        lp_advertisement = 0;
      }
      p->link_partner_eee_capable = lp_advertisement == 0 ? FALSE : TRUE;
    } else {
      p->link_partner_eee_capable = FALSE;
    }
  } else {
    p->link_partner_eee_capable = FALSE;
  }

  if (!p->rx_tw_changed_by_debug_cmd) {
    p->rx_tw = EEE_PHY_RX_WAKE_VALUE(p->speed);
  }

  if (!p->tx_tw_changed_by_debug_cmd) {
    p->tx_tw = EEE_PHY_TX_WAKE_VALUE(p->speed);
  }

  p->running = EEE_local_conf.port[port].eee_ena && p->eee_capable && p->link_partner_eee_capable && status->link && (status->speed == MESA_SPEED_100M || status->speed == MESA_SPEED_1G) && status->fdx == TRUE;

  // And then a range of flow-control exceptions.
  if (p->running) {
    if (status->aneg.obey_pause || status->aneg.generate_pause) {
      // Running flow control. Check chip family.
      uint32_t chip_family = fast_cap(MESA_CAP_MISC_CHIP_FAMILY);
      if (chip_family == MESA_CHIP_FAMILY_CARACAL) {
        // EEE and flow control not supported on Rev. A and B.
        p->running = EEE_chip_id.revision != 0 && EEE_chip_id.revision != 1;
      } else if (chip_family ==  MESA_CHIP_FAMILY_OCELOT) {
        p->running = TRUE;
      } else {
        T_E("Unknown chip family: %d. Please revise eee.c", chip_family);
      }
    }
  }

  if (!p->running) {
    // Rx SM
    p->LocRxSystemValue         = EEE_TW_VALUE_INIT;
    p->RemRxSystemValueEcho     = 0;
    p->RemTxSystemValue         = 0;
    p->LocTxSystemValueEcho     = 0;
    p->LocResolvedRxSystemValue = EEE_TW_VALUE_INIT;
    p->LocFbSystemValue         = 0;
    p->TempTxVar                = 0;

    // Tx SM
    p->LocTxSystemValue         = EEE_TW_VALUE_INIT;
    p->RemTxSystemValueEcho     = 0;
    p->RemRxSystemValue         = 0;
    p->LocRxSystemValueEcho     = 0;
    p->LocResolvedTxSystemValue = EEE_TW_VALUE_INIT;
    p->TempRxVar                = 0;
  } else {
    // Rx SM
    p->LocRxSystemValue         = p->rx_tw;
    p->RemRxSystemValueEcho     = p->tx_tw;
    p->RemTxSystemValue         = p->tx_tw;
    p->LocTxSystemValueEcho     = p->tx_tw;
    p->LocResolvedRxSystemValue = p->tx_tw;
    p->LocFbSystemValue         = p->tx_tw;
    p->TempTxVar                = p->tx_tw;

    // Tx SM
    p->LocTxSystemValue         = p->tx_tw;
    p->RemTxSystemValueEcho     = p->tx_tw;
    p->RemRxSystemValue         = p->tx_tw;
    p->LocRxSystemValueEcho     = p->tx_tw;
    p->LocResolvedTxSystemValue = p->tx_tw;
    p->TempRxVar                = p->tx_tw;
  }

  // Run the state machine once.
  EEE_state_change(port, TRUE, 0, 0, 0, 0, callback);
}

/****************************************************************************/
// EEE_msg_tx_switch_conf()
/****************************************************************************/
static void EEE_msg_tx_switch_conf(vtss_isid_t isid)
{
  eee_switch_conf_t        *new_switch_conf;
  eee_switch_global_conf_t *new_global_conf;
  mesa_port_list_t         port_changes;
  port_iter_t              pit;

  EEE_CRIT_ASSERT_LOCKED();

  if (!msg_switch_exists(isid)) {
    return;
  }

  new_switch_conf = &EEE_stack_conf.switch_conf[isid - VTSS_ISID_START];
  new_global_conf = &EEE_stack_conf.global;
  (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);

  // When we get here, EEE_local_state.eee_capable[] is definitely updated because the thread
  // doesn't let go of the EEE crit before it's initialized.

  // Enable or disable EEE-advertisement in PHY. This may be done whether or not the PHY has link.
  while (port_iter_getnext(&pit)) {
    if (EEE_local_state.port[pit.iport].eee_capable) {
      if (vtss_phy_eee_ena(NULL, pit.iport, new_switch_conf->port[pit.iport].eee_ena) != VTSS_RC_OK) {
        T_E("Unable to set PHY EEE-advertisement for port %u", pit.uport);
      }
      port_changes[pit.iport] = memcmp(&EEE_local_conf.port[pit.iport], &new_switch_conf->port[pit.iport], sizeof(eee_port_conf_t)) != 0 || EEE_global_conf.optimized_for_power != new_global_conf->optimized_for_power;
      if (port_changes[pit.iport]) {
        vtss_appl_port_status_t port_status = {};
        vtss_ifindex_t          ifindex;

        (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, pit.iport, &ifindex);
        (void)vtss_appl_port_status_get(ifindex, &port_status);

        EEE_local_state_init(pit.iport, &port_status, TRUE);
      }
    }
  }

  EEE_local_conf = *new_switch_conf;
  EEE_global_conf  = *new_global_conf;

  // Call switch-specific function.
  EEE_CRIT_EXIT();
  EEE_platform_local_conf_set(port_changes);
  EEE_CRIT_ENTER();
}

/****************************************************************************/
// EEE_conf_default_set()
// Setting configuration to default
/****************************************************************************/
static void  EEE_conf_defaut_set(void)
{
  vtss_isid_t isid_iter;

  EEE_CRIT_ENTER();

  // Set default configuration
  for (isid_iter = VTSS_ISID_START; isid_iter < VTSS_ISID_END; isid_iter++) {
    // Re-initialize switch(es).
    eee_switch_conf_t *conf = &EEE_stack_conf.switch_conf[isid_iter - VTSS_ISID_START];
    vtss_clear(*conf);
    EEE_platform_conf_reset(conf);
  }

  EEE_stack_conf.global.optimized_for_power = OPTIMIZE_FOR_POWER_DEFAULT;


  // Send new configuration to all switches
  for (isid_iter = VTSS_ISID_START; isid_iter < VTSS_ISID_END; isid_iter++) {
    EEE_msg_tx_switch_conf(isid_iter);
  }
  EEE_CRIT_EXIT();
}

/******************************************************************************/
// EEE_port_link_change_event()
/******************************************************************************/
static void EEE_port_link_change_event(mesa_port_no_t iport, const vtss_appl_port_status_t *status)
{
  if (status->static_caps & MEBA_PORT_CAP_1G_PHY) {
    EEE_CRIT_ENTER();
    // Update state
    EEE_local_state_init(iport, status, TRUE);

    // Call platform-specific code
    EEE_platform_port_link_change_event(iport);
    EEE_CRIT_EXIT();
  }
}

/******************************************************************************/
// EEE_thread()
/******************************************************************************/
/* Lint cannot see that EEE_thread() is locked on entry, so tell it that it's
   safe to update EEE_local_state.port[].eee_capable even though other threads
   are reading it. */
/*lint -sem(EEE_thread, thread_protected) */
static void EEE_thread(vtss_addrword_t data)
{
  BOOL        capable;
  port_iter_t pit;

  (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);

  // Register for port link change events on local switch.
  if (port_change_register(VTSS_MODULE_ID_EEE, EEE_port_link_change_event) != VTSS_RC_OK) {
    T_E("Couldn't register for port link change events");
    // Gotta exit crit so that management functions can be called.
    /*lint -e(455) */
    EEE_CRIT_EXIT();

    // The EEE-capability array shows that no PHYs are EEE capable at this point in time-
    // No need for this thread at all.
    return;
  }

  // This will block this thread from running further until the PHYs are initialized.
  port_phy_wait_until_ready();

  // Now the PHYs are ready, we can continue querying for EEE-capableness.
  while (port_iter_getnext(&pit)) {
    if (vtss_phy_port_eee_capable(NULL, pit.iport, &capable) == VTSS_RC_OK) {
      EEE_local_state.port[pit.iport].eee_capable = capable;
      T_D("EEE_local_state.port[%d].eee_capable:%d", pit.iport, EEE_local_state.port[pit.iport].eee_capable);
    }
  }

  // Platform-specific thread handler that never returns.
  // It's up to the platform specific thread handler to
  // call EEE_CRIT_EXIT() when it's ready.
  EEE_platform_thread();
}

/******************************************************************************/
//
// PUBLIC FUNCTIONS
//
/******************************************************************************/

/******************************************************************************/
// eee_error_txt()
// Converts EEE error to printable text
/******************************************************************************/
const char *eee_error_txt(mesa_rc rc)
{
  switch (rc) {
  case EEE_ERROR_ISID:
    return "Invalid Switch ID";

  case EEE_ERROR_PORT:
    return "Invalid port number";

  case EEE_ERROR_NOT_PRIMARY_SWITCH:
    return "Switch must be the primary";

  case EEE_ERROR_VALUE:
    return "Invalid value";

  case EEE_ERROR_NOT_CAPABLE:
    return "Interface not EEE capable";

  default:
    return "";
  }
}

/******************************************************************************/
// eee_mgmt_switch_global_conf_get()
// Function that returns the current global configuration for a stack.
// In/out : switch_global_conf - Pointer to configuration struct where the current
// global configuration is copied to.
/******************************************************************************/
mesa_rc eee_mgmt_switch_global_conf_get(eee_switch_global_conf_t *switch_global_conf)
{

  if (switch_global_conf == NULL) {
    return EEE_ERROR_VALUE;
  }

  EEE_CRIT_ENTER();
  *switch_global_conf = EEE_stack_conf.global;
  EEE_CRIT_EXIT();

  return VTSS_RC_OK;
}

/******************************************************************************/
// eee_mgmt_switch_global_conf_set()
// Function for setting the current global configuration for a stack.
//      switch_global_conf - Pointer to configuration struct with the new configuration.
// Return : VTSS error code
/******************************************************************************/
mesa_rc eee_mgmt_switch_global_conf_set(eee_switch_global_conf_t *switch_global_conf)
{
  vtss_isid_t isid;
  mesa_rc result = VTSS_RC_OK;
  if (switch_global_conf == NULL) {
    return EEE_ERROR_VALUE;
  }

  // Transfer new configuration to the all switches.
  EEE_CRIT_ENTER();

  // Update the configuration for the switch
  EEE_stack_conf.global = *switch_global_conf;

  for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
    EEE_msg_tx_switch_conf(isid);
  }
  EEE_CRIT_EXIT();

  return result;
}

/******************************************************************************/
// eee_mgmt_switch_conf_get()
// Function that returns the current configuration for a switch.
// In : isid - isid for the switch the shall return its configuration
// In/out : switch_conf - Pointer to configuration struct where the current
// configuration is copied to.
/******************************************************************************/
mesa_rc eee_mgmt_switch_conf_get(vtss_isid_t isid, eee_switch_conf_t *switch_conf)
{
  mesa_rc result = VTSS_RC_OK;
  if ((result = EEE_check_isid_port(isid, VTSS_PORT_NO_START, TRUE, FALSE)) != VTSS_RC_OK) {
    return result;
  }

  if (switch_conf == NULL) {
    return EEE_ERROR_VALUE;
  }

  EEE_CRIT_ENTER();
  if (isid == VTSS_ISID_LOCAL) {
    *switch_conf = EEE_local_conf;
  } else {
    *switch_conf = EEE_stack_conf.switch_conf[isid - VTSS_ISID_START];
  }
  EEE_CRIT_EXIT();

  return VTSS_RC_OK;
}

/******************************************************************************/
// eee_mgmt_switch_conf_set()
// Function for setting the current configuration for a switch.
// In : isid - isid for the switch the shall return its configuration
//      switch_conf - Pointer to configuration struct with the new configuration.
// Return : VTSS error code
/******************************************************************************/
mesa_rc eee_mgmt_switch_conf_set(vtss_isid_t isid, eee_switch_conf_t *switch_conf)
{
  mesa_rc               result = VTSS_RC_OK;
  port_iter_t           pit;

  T_N("isid:%d", isid);

  // Configuration changes only allowed by the primary switch
  VTSS_RC(EEE_check_isid_port(isid, VTSS_PORT_NO_START, TRUE, FALSE));

  if (switch_conf == NULL) {
    return EEE_ERROR_VALUE;
  }

  VTSS_RC(port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL));

  EEE_CRIT_ENTER();

  while (port_iter_getnext(&pit)) {
    T_N("pit.iport:%d", pit.iport);
    if (msg_switch_exists(isid)) { //If the switch doesn't exists, we don't know if the port supports EEE, so we acts as it do, and only test for eee capable for switches which do exist.
      T_N("eee_capable:%d, ena:%d", EEE_local_state.port[pit.iport].eee_capable, switch_conf->port[pit.iport].eee_ena);
      if (switch_conf->port[pit.iport].eee_ena && !EEE_local_state.port[pit.iport].eee_capable) {
        T_I("Attempting to enable EEE on non-EEE-capable port (%u)", pit.uport);
        result = EEE_ERROR_NOT_CAPABLE;
        goto do_exit;
      }
    }
  }

  // Update the configuration for the switch
  EEE_stack_conf.switch_conf[isid - VTSS_ISID_START] = *switch_conf;

  // Transfer new configuration to the switch in question.
  EEE_msg_tx_switch_conf(isid);

do_exit:
  EEE_CRIT_EXIT();
  return result;
}

/******************************************************************************/
// eee_mgmt_switch_state_get()
/******************************************************************************/
#define MSG_TIMEOUT (vtss_current_time() + VTSS_OS_MSEC2TICK(5000)) /* Wait for timeout (5 seconds) or synch. flag to be set */
mesa_rc eee_mgmt_switch_state_get(vtss_isid_t isid, eee_switch_state_t *switch_state)
{
  mesa_rc result;

  if ((result = EEE_check_isid_port(isid, VTSS_PORT_NO_START, TRUE, FALSE)) != VTSS_RC_OK) {
    return result;
  }

  if (switch_state == NULL) {
    return EEE_ERROR_VALUE;
  }

  EEE_CRIT_ENTER();
  EEE_local_state_power_save_state_update(); // Update the power save state
  *switch_state = EEE_local_state;
  EEE_CRIT_EXIT();

  return VTSS_RC_OK;
}

/******************************************************************************/
// eee_mgmt_port_state_get()
/******************************************************************************/
mesa_rc eee_mgmt_port_state_get(vtss_isid_t isid, mesa_port_no_t port, eee_port_state_t *port_state)
{
  mesa_rc result;
  if ((result = EEE_check_isid_port(isid, port, TRUE, TRUE)) != VTSS_RC_OK) {
    return result;
  }

  if (port_state == NULL) {
    return EEE_ERROR_VALUE;
  }

  EEE_CRIT_ENTER();
  EEE_local_state_power_save_state_update(); // Update the power save state
  *port_state = EEE_local_state.port[port];
  EEE_CRIT_EXIT();

  return VTSS_RC_OK;
}

/******************************************************************************/
// eee_mgmt_remote_state_change()
// Called by LLDP on local switch, so no isid.
/******************************************************************************/
void eee_mgmt_remote_state_change(mesa_port_no_t port,
                                  u16            RemRxSystemValue,     /* tx state machine */
                                  u16            RemTxSystemValue,     /* rx state machine */
                                  u16            RemRxSystemValueEcho, /* rx state machine */
                                  u16            RemTxSystemValueEcho) /* tx state machine */

{
  if (port >= port_count_max()) {
    return;
  }

  // Possible remote system change request/update.
  EEE_CRIT_ENTER();
  EEE_state_change(port, FALSE, RemRxSystemValue, RemTxSystemValue, RemRxSystemValueEcho, RemTxSystemValueEcho, TRUE);
  EEE_CRIT_EXIT();
}

/******************************************************************************/
// eee_mgmt_local_state_change()
// Debug function called by CLI to request local changes.
// Only works on local switch.
/******************************************************************************/
void eee_mgmt_local_state_change(mesa_port_no_t port, BOOL clear, BOOL is_tx, u16 LocSystemValue)
{
  if (port >= port_count_max()) {
    return;
  }

  EEE_CRIT_ENTER();
  if (is_tx) {
    EEE_local_state.port[port].tx_tw                      =  clear ? EEE_PHY_TX_WAKE_VALUE(EEE_local_state.port[port].speed) : LocSystemValue;
    EEE_local_state.port[port].tx_tw_changed_by_debug_cmd = !clear;
  } else {
    EEE_local_state.port[port].rx_tw                      =  clear ? EEE_PHY_RX_WAKE_VALUE(EEE_local_state.port[port].speed) : LocSystemValue;
    EEE_local_state.port[port].rx_tw_changed_by_debug_cmd = !clear;
  }

  // Local system change request
  EEE_state_change(port, TRUE, 0, 0, 0, 0, TRUE);
  EEE_CRIT_EXIT();
}
/**
 * Set Energy Efficient Ethernet global parameters.
 * conf : EEE global configurable parameters
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eee_global_conf_set(const vtss_appl_eee_global_conf_t  *const conf)
{
#ifdef EEE_OPTIMIZE_SUPPORT
  eee_switch_global_conf_t switch_global_conf;
  if (conf == NULL) {
    T_I("conf == NULL");
    return VTSS_RC_ERROR;
  }
  VTSS_RC(eee_mgmt_switch_global_conf_get(&switch_global_conf));

  if (conf->preference == VTSS_APPL_EEE_OPTIMIZATION_PREFERENCE_POWER) {
    switch_global_conf.optimized_for_power = TRUE;
  }
  if (conf->preference == VTSS_APPL_EEE_OPTIMIZATION_PREFERENCE_LATENCY) {
    switch_global_conf.optimized_for_power = FALSE;
  }
  VTSS_RC(eee_mgmt_switch_global_conf_set(&switch_global_conf));
  return VTSS_RC_OK;
#else
  T_D("Optimization preference is not supported");
  return VTSS_RC_ERROR;
#endif
}
/**
 * Get Energy Efficient Ethernet global parameters.
 * conf  [OUT]: EEE global configurable parameters
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eee_global_conf_get(vtss_appl_eee_global_conf_t  *const conf)
{
#ifdef EEE_OPTIMIZE_SUPPORT
  eee_switch_global_conf_t switch_global_conf;
  if (conf == NULL) {
    T_I("conf == NULL");
    return VTSS_RC_ERROR;
  }
  VTSS_RC(eee_mgmt_switch_global_conf_get(&switch_global_conf));

  if (switch_global_conf.optimized_for_power) {
    conf->preference = VTSS_APPL_EEE_OPTIMIZATION_PREFERENCE_POWER;
  } else {
    conf->preference = VTSS_APPL_EEE_OPTIMIZATION_PREFERENCE_LATENCY;
  }
  return VTSS_RC_OK;
#else
  T_D("Optimization preference is not supported");
  return VTSS_RC_ERROR;
#endif
}
/**
 * Get global capabilities
 * capabilities [OUT]: EEE platform specific global capabilities
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eee_global_capabilities_get(vtss_appl_eee_global_capabilities_t *const capabilities)
{
#ifdef EEE_OPTIMIZE_SUPPORT
  if (capabilities != NULL) {
    capabilities->optimization_preference_capable = TRUE;
  } else {
    T_I("capabilities == NULL");
    return VTSS_RC_ERROR;
  }
#else
  capabilities->optimization_preference_capable = FALSE;
#endif
  return VTSS_RC_OK;
}
/**
 * Get port capabilities
 * param ifIndex       [IN]: Interface index
 * param capabilities [OUT]: EEE platform specific port capabilities
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eee_port_capabilities_get(vtss_ifindex_t                    ifIndex,
                                            vtss_appl_eee_port_capabilities_t *const capabilities)
{
  vtss_ifindex_elm_t  ife;
  eee_switch_state_t  eee_switch_state;

  if (capabilities == NULL) {
    T_I("capabilities == NULL");
    return VTSS_RC_ERROR;
  }
  VTSS_INTERFACE_INDEX_GET(eee_if2ife(ifIndex, &ife));
  VTSS_RC(eee_mgmt_switch_state_get(ife.isid, &eee_switch_state));
  capabilities->queue_count = EEE_FAST_QUEUES_CNT;
  capabilities->eee_capable = eee_switch_state.port[ife.ordinal].eee_capable;
  return VTSS_RC_OK;
}
/**
 * Set Energy Efficient Ethernet port configuration.
 * param ifIndex  [IN]: Interface index
 * param conf     [IN]: EEE port configurable parameters
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eee_port_conf_set(vtss_ifindex_t ifIndex,
                                    const vtss_appl_eee_port_conf_t  *const conf)
{
  vtss_ifindex_elm_t  ife;
  eee_switch_conf_t   eee_switch_conf;

  if (conf == NULL) {
    T_I("conf == NULL");
    return VTSS_RC_ERROR;
  }
  VTSS_INTERFACE_INDEX_GET(eee_if2ife(ifIndex, &ife));
  VTSS_RC(eee_mgmt_switch_conf_get(ife.isid, &eee_switch_conf));
  eee_switch_conf.port[ife.ordinal].eee_ena = conf->eee_enable;
  return eee_mgmt_switch_conf_set(ife.isid, &eee_switch_conf);
}
/**
 * Get Energy Efficient Ethernet port configuration.
 * param ifIndex  [IN] : Interface index
 * param conf     [OUT]: EEE port configurable parameters
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eee_port_conf_get(vtss_ifindex_t ifIndex,
                                    vtss_appl_eee_port_conf_t *const conf)
{
  eee_switch_conf_t   eee_switch_conf;
  vtss_ifindex_elm_t  ife;

  if (conf == NULL) {
    T_I("conf == NULL");
    return VTSS_RC_ERROR;
  }
  VTSS_INTERFACE_INDEX_GET(eee_if2ife(ifIndex, &ife));
  VTSS_RC(eee_mgmt_switch_conf_get(ife.isid, &eee_switch_conf));
  conf->eee_enable = eee_switch_conf.port[ife.ordinal].eee_ena;
  T_N("isid:%d, port:%d", ife.isid, ife.ordinal);
  return VTSS_RC_OK;
}
/**
 * Set whether given egress port queue is urgent queue or not.
 * param ifIndex     [IN]: Interface index
 * param queueIndex  [IN]: Queue index value.
 * param type        [IN]: Port queue is whether urgent queue or not.
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eee_port_queue_type_set(vtss_ifindex_t ifIndex,
                                          vtss_appl_eee_port_queue_index_t queueIndex,
                                          const vtss_appl_eee_port_queue_t *const type)
{
#if EEE_FAST_QUEUES_CNT > 0
  eee_switch_conf_t   eee_switch_conf;
  vtss_ifindex_elm_t  ife;

  VTSS_INTERFACE_INDEX_GET(eee_if2ife(ifIndex, &ife));
  if (queueIndex >= EEE_FAST_QUEUES_MAX) {
    T_I("Invalid queue index");
    return VTSS_RC_ERROR;
  }
  if (type == NULL) {
    T_I("conf == NULL");
    return VTSS_RC_ERROR;
  }
  VTSS_RC(eee_mgmt_switch_conf_get(ife.isid, &eee_switch_conf));
  if (type->queue_type == VTSS_APPL_EEE_QUEUE_TYPE_URGENT) {
    eee_switch_conf.port[ife.ordinal].eee_fast_queues |= (1 << queueIndex);
  } else {
    eee_switch_conf.port[ife.ordinal].eee_fast_queues &= ~(u8)(1 << queueIndex);
  }
  return eee_mgmt_switch_conf_set(ife.isid, &eee_switch_conf);
#else
  T_D("Egress port queues conf not supported");
  return VTSS_RC_ERROR;
#endif
}
/**
 * Get whether given egress port queue is urgent queue or not.
 * param ifIndex      [IN]: Interface index
 * param queueIndex   [IN]: Queue index value
 * param type        [OUT]: The port queue is whether urgent queue or not
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eee_port_queue_type_get(vtss_ifindex_t                    ifIndex,
                                          vtss_appl_eee_port_queue_index_t queueIndex,
                                          vtss_appl_eee_port_queue_t       *const type)
{
#if EEE_FAST_QUEUES_CNT > 0
  eee_switch_conf_t   eee_switch_conf;
  vtss_ifindex_elm_t  ife;
  u8                  urgent_queues;

  VTSS_INTERFACE_INDEX_GET(eee_if2ife(ifIndex, &ife));
  if (queueIndex >= EEE_FAST_QUEUES_MAX) {
    T_I("Invalid queue index");
    return VTSS_RC_ERROR;
  }
  if (type == NULL) {
    T_I("conf == NULL");
    return VTSS_RC_ERROR;
  }
  VTSS_RC(eee_mgmt_switch_conf_get(ife.isid, &eee_switch_conf));
  urgent_queues = eee_switch_conf.port[ife.ordinal].eee_fast_queues;

  if (urgent_queues & (1 << queueIndex)) {
    type->queue_type = VTSS_APPL_EEE_QUEUE_TYPE_URGENT;
  } else {
    type->queue_type = VTSS_APPL_EEE_QUEUE_TYPE_NORMAL;
  }
  return VTSS_RC_OK;
#else
  T_D("Egress port urgent queues conf not supported");
  return VTSS_RC_ERROR;
#endif
}
/**
 * Egress port queue iterate function, it is used to get first and get next egress port queue indexes.
 * param prevIfindex     [IN]: Ifindex of previous port
 * param nextIfindex    [OUT]: Ifindex of next port
 * param prevQueueIndex  [IN]: Previous port queue index
 * param nextQueueIndex [OUT]: Next port queue index
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eee_port_queue_iterator(const vtss_ifindex_t                   *const prevIfindex,
                                          vtss_ifindex_t                         *const nextIfindex,
                                          const vtss_appl_eee_port_queue_index_t *const prevQueueIndex,
                                          vtss_appl_eee_port_queue_index_t       *const nextQueueIndex)
{
  if (!nextIfindex || !nextQueueIndex) {
    T_D("Invalid Input!");
    return VTSS_RC_ERROR;
  }

#if EEE_FAST_QUEUES_CNT > 0
  if (!prevIfindex) {
    *nextQueueIndex = 0;
  } else {
    vtss_ifindex_elm_t  ife;

    if (vtss_ifindex_decompose(*prevIfindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT ||
        ife.ordinal >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
      T_D("No next IfIndex");
      return VTSS_RC_ERROR;
    }

    if (!prevQueueIndex) {
      *nextQueueIndex = 0;
      *nextIfindex = *prevIfindex;
      return VTSS_RC_OK;
    } else {
      if (*prevQueueIndex >= (EEE_FAST_QUEUES_MAX - 1)) {
        *nextQueueIndex = 0;
      } else {
        *nextQueueIndex = *prevQueueIndex + 1;
        *nextIfindex = *prevIfindex;
        return VTSS_RC_OK;
      }
    }
  }
  return vtss_appl_iterator_ifindex_front_port_exist(prevIfindex, nextIfindex);
#else
  T_D("No egress port queues");
  return VTSS_RC_ERROR;
#endif
}

/**
 * Get the port current status
 * param ifIndex     [IN]: Interface index
 * param status     [OUT]: Port status data
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eee_port_status_get(vtss_ifindex_t ifIndex,
                                      vtss_appl_eee_port_status_t *const status)
{
  eee_switch_state_t  eee_switch_state;
  vtss_ifindex_elm_t  ife;

  VTSS_INTERFACE_INDEX_GET(eee_if2ife(ifIndex, &ife));
  if (status == NULL) {
    T_I("status == NULL");
    return VTSS_RC_ERROR;
  }
  VTSS_RC(eee_mgmt_switch_state_get(ife.isid, &eee_switch_state));

  if (eee_switch_state.port[ife.ordinal].eee_capable) {
    if (eee_switch_state.port[ife.ordinal].link_partner_eee_capable) {
      status->link_partner_eee = VTSS_APPL_EEE_STATUS_ENABLE;
    } else {
      status->link_partner_eee = VTSS_APPL_EEE_STATUS_DISABLE;
    }
    if (eee_switch_state.port[ife.ordinal].tx_in_power_save_state ||
        eee_switch_state.port[ife.ordinal].rx_in_power_save_state) {
      status->rx_in_power_save_state = VTSS_APPL_EEE_STATUS_ENABLE;
    } else {
      status->rx_in_power_save_state = VTSS_APPL_EEE_STATUS_DISABLE;
    }
  } else {
    status->link_partner_eee = VTSS_APPL_EEE_STATUS_NOT_SUPPORTED;
    status->rx_in_power_save_state = VTSS_APPL_EEE_STATUS_NOT_SUPPORTED;
  }
  return VTSS_RC_OK;
}



/******************************************************************************/
// eee_init()
/******************************************************************************/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void eee_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_eee_json_init(void);
#endif
mesa_rc eee_init(vtss_init_data_t *data)
{
  vtss_isid_t             isid = data->isid;
  mesa_port_no_t          iport;
  vtss_appl_port_status_t status;

  /*lint --e{454,456} */

  switch (data->cmd) {
  case INIT_CMD_INIT:
    vtss_clear(status);
    for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
      EEE_local_state_init(iport, &status, FALSE);
    }
    // Create our mutex. Don't release it until we're ready (i.e. in the EEE_thread()).
    critd_init_legacy(&EEE_crit, "eee", VTSS_MODULE_ID_EEE, CRITD_TYPE_MUTEX);
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
    /* Register private mib */
    eee_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
    vtss_appl_eee_json_init();
#endif
    // We need a thread because we need to figure out whether the PHYs are EEE-capable, and
    // this querying must be postponed until the PHYs are ready.
    vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                       EEE_thread,
                       0,
                       "EEE",
                       nullptr,
                       0,
                       &EEE_thread_handle,
                       &EEE_thread_block);
    break;

  case INIT_CMD_START:
    (void)mesa_chip_id_get(NULL, &EEE_chip_id);
    break;

  case INIT_CMD_CONF_DEF:
    if (isid == VTSS_ISID_LOCAL || isid == VTSS_ISID_GLOBAL) {
      // Reset local configuration or global configuration.
      // No such configuration for this module.
    } else if (VTSS_ISID_LEGAL(isid)) {
      // Set to default
      EEE_conf_defaut_set();
    }
    break;

  case INIT_CMD_ICFG_LOADING_PRE:
    // Set to default.
    EEE_conf_defaut_set();
    break;

  case INIT_CMD_ICFG_LOADING_POST:
    EEE_CRIT_ENTER();
    EEE_msg_tx_switch_conf(isid);
    EEE_CRIT_EXIT();
    break;

  default:
    break;
  }

  // Call chip-specific initialization function.
  EEE_platform_init(data);

  return VTSS_RC_OK;
}

