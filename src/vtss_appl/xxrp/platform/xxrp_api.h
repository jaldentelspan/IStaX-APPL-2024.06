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

#ifndef _XXRP_API_H_
#define _XXRP_API_H_

#include "main.h"
#include "vtss/appl/types.hxx"
#include "vtss/appl/mvrp.h"
#include "vtss_xxrp_api.h"
#include <vlan_api.h>
#include "../base/vtss_xxrp_callout.h"
#include "icli_porting_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MVRP_CRIT_ENTER() vtss_mrp_crit_enter()
#define MVRP_CRIT_EXIT() vtss_mrp_crit_exit()

typedef struct {
    u64 alloc_cnt;
    u64 alloc_size;
    u64 free_cnt;
    u64 free_size;
} xxrp_mgmt_mem_stat_t;

/***************************************************************************************************
 * Definition of rc errors - See also xxrp_error_txt in xxrp.c
 **************************************************************************************************/
enum {
    XXRP_ERROR_ISID = XXRP_RC_LAST,
    XXRP_ERROR_PORT,
    XXRP_ERROR_FLASH,
    XXRP_ERROR_NOT_PRIMARY_SWITCH,
    XXRP_ERROR_VALUE,
    XXRP_ERROR_APPL_OVERLAP,
    XXRP_ERROR_APPL_ENABLED_ALREADY,
    XXRP_ERROR_INVALID_IF_TYPE,
    XXRP_ERROR_INVALID_TIMER,
    XXRP_ERROR_INVALID_VID
};

const char *xxrp_error_txt(mesa_rc rc);
/***************************************************************************************************
 * Configuration definition
 **************************************************************************************************/

/***************************************************************************************************
 * Functions
 **************************************************************************************************/
const char *const registrar_state2txt(u8 S);
const char *const registrar_admin_state2txt(u8 S);
const char *const applicant_state2txt(u8 S);
const char *const bool_state2txt(u8 S);

mesa_rc xxrp_mgmt_appl_exclusion(vtss_mrp_appl_t appl);
mesa_rc xxrp_mgmt_global_enabled_get(vtss_mrp_appl_t appl, BOOL *enable);
mesa_rc xxrp_mgmt_global_enabled_set(vtss_mrp_appl_t appl, BOOL enable);

mesa_rc xxrp_mgmt_global_timers_get(vtss_mrp_appl_t appl, vtss_mrp_timer_conf_t *timers);
mesa_rc xxrp_mgmt_global_timers_set(vtss_mrp_appl_t appl, const vtss_mrp_timer_conf_t *const timers);
mesa_rc xxrp_mgmt_timers_check(vtss_mrp_appl_t appl, const vtss_mrp_timer_conf_t *const timers);

char *xxrp_mgmt_vlan_list_to_txt(vtss::VlanList &vls, char *txt);
mesa_rc xxrp_mgmt_txt_to_vlan_list(char *txt, vtss::VlanList &vls,
                                   ulong min, ulong max);
mesa_rc xxrp_mgmt_global_managed_vids_get(vtss_mrp_appl_t appl, vtss::VlanList &vls);
mesa_rc xxrp_mgmt_global_managed_vids_set(vtss_mrp_appl_t appl, vtss::VlanList &vls);

mesa_rc xxrp_mgmt_enabled_get(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, BOOL *enable);
mesa_rc xxrp_mgmt_enabled_set(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, BOOL enable);

mesa_rc mrp_mgmt_timers_get(vtss_isid_t isid, mesa_port_no_t iport,
                            vtss_mrp_timer_conf_t *timers);
mesa_rc mrp_mgmt_timers_set(vtss_isid_t isid, mesa_port_no_t iport,
                            const vtss_mrp_timer_conf_t *timers);

mesa_rc xxrp_mgmt_reg_admin_status_get(vtss_mrp_appl_t appl, vtss_isid_t isid,
                                       mesa_port_no_t iport, u32 mad_fsm_index, u8 *t);

mesa_rc xxrp_mgmt_port_stats_get(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl, vtss_mrp_stats_t *stats);
mesa_rc xxrp_mgmt_port_stats_clear(vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_appl_t appl);

mesa_rc xxrp_init(vtss_init_data_t *data);

mesa_rc xxrp_mgmt_mad_get(vtss_mrp_appl_t appl, vtss_isid_t isid, mesa_port_no_t iport, vtss_mrp_mad_t **mad);
mesa_rc xxrp_mgmt_map_get(vtss_mrp_appl_t appl, vtss_mrp_map_t ***map_ports);

#ifdef __cplusplus
}
#endif
#endif /* _XXRP_API_H_ */
