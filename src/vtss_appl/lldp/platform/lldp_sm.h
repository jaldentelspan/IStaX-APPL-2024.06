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


#ifndef LLDP_SM_H
#define LLDP_SM_H

/* import of lldp types */
#include "lldp_os.h"
#include "vtss/appl/lldp.h"

typedef struct {
    /* Transmit timers */
    lldp_timer_t txShutdownWhile;
    lldp_timer_t txDelayWhile;
    lldp_timer_t txTTR;
} lldp_timers_t;

typedef enum {
    TX_INVALID_STATE,
    TX_LLDP_INITIALIZE,
    TX_IDLE,
    TX_SHUTDOWN_FRAME,
    TX_INFO_FRAME,
} lldp_tx_state_t;

typedef struct {
    lldp_tx_state_t state;
    lldp_bool_t     somethingChangedLocal;
    lldp_timer_t    txTTL;

    lldp_bool_t re_evaluate_timers;
} lldp_tx_t;

typedef enum {
    RX_INVALID_STATE,
    LLDP_WAIT_PORT_OPERATIONAL,
    DELETE_AGED_INFO,
    RX_LLDP_INITIALIZE,
    RX_WAIT_FOR_FRAME,
    RX_FRAME,
    DELETE_INFO,
    UPDATE_INFO,
} lldp_rx_state_t;

typedef struct {
    lldp_rx_state_t state;

    lldp_bool_t badFrame;
    lldp_bool_t rcvFrame;
    lldp_bool_t rxChanges;
    lldp_bool_t rxInfoAge;
    lldp_timer_t rxTTL;
    lldp_bool_t somethingChangedRemote;
} lldp_rx_t;


typedef struct {
    lldp_tx_t tx;
    lldp_rx_t rx;
    lldp_timers_t timers;
    vtss_appl_lldp_port_counters_t stats;

    vtss_appl_lldp_admin_state_t adminStatus;
    lldp_bool_t portEnabled;
    lldp_bool_t initialize;
    lldp_port_t port_no;
    lldp_port_t sid;
    mesa_glag_no_t glag_no;
} lldp_sm_t;



// See IEEE802.3at/D3 section 33.7.6.2
typedef enum {LOSS, NACK, ACK, DNULL} locAcknowledge_t;


// See IEEE802.3at/D3 section 33.7.6.2
typedef struct {
    enum {
        INITIALIZE,
        RUNNING,
        LOSS_OF_COMMUNICATIONS,
        REMOTE_REQUEST,
        REMOTE_ACK,
        REMOTE_NACK,
        WAIT_FOR_REMOTE,
        LOCAL_REQUEST,
        LOCAL_ACK,
        LOCAL_NACK,
    } state;
    locAcknowledge_t locAcknowledge;
    int remRequestedPowerValue;
    int remActualPowerValue;
    BOOL local_change;

} lldp_sm_pow_control_t;


void lldp_sm_step (lldp_sm_t *sm, BOOL rx_only);
void lldp_sm_init (lldp_sm_t *sm, lldp_port_t port);
void lldp_sm_timers_tick(lldp_sm_t  *sm);
#endif

