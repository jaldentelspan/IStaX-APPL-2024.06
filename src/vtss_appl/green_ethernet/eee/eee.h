/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __EEE_H__
#define __EEE_H__

#include "eee_api.h"  // for eee_switch_conf_t

//************************************************
// Trace definitions
//************************************************
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>
#include "critd_api.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_EEE
#define VTSS_TRACE_GRP_DEFAULT 0
#define VTSS_TRACE_GRP_CLI     1

// Global variables shared amongst platform code
extern critd_t            EEE_crit;
extern eee_switch_conf_t  EEE_local_conf;
extern eee_switch_global_conf_t  EEE_global_conf;
extern eee_switch_state_t EEE_local_state;

#define EEE_CRIT_ENTER()         critd_enter        (&EEE_crit, __FILE__, __LINE__)
#define EEE_CRIT_EXIT()          critd_exit         (&EEE_crit, __FILE__, __LINE__)
#define EEE_CRIT_ASSERT_LOCKED() critd_assert_locked(&EEE_crit, __FILE__, __LINE__)

// Default configuration settings
#define OPTIMIZE_FOR_POWER_DEFAULT FALSE

/*
 * Functions to be implemented by chip-specific C-file.
 * These functions are not really public to others than
 * eee.c.
 */
void EEE_platform_port_link_change_event(mesa_port_no_t iport);
void EEE_platform_thread(void);
void EEE_platform_conf_reset(eee_switch_conf_t *conf);
BOOL EEE_platform_conf_valid(eee_switch_conf_t *conf);
void EEE_platform_tx_wakeup_time_changed(mesa_port_no_t port, u16 LocResolvedTxSystemValue);
void EEE_platform_local_conf_set(mesa_port_list_t &port_changes);
void EEE_platform_init(vtss_init_data_t *data);
#endif /* __EEE_H__ */

