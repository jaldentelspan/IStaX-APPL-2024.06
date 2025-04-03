/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
 * \brief vlan icli functions
 * \details This header file describes vlan icli functions
 */

#ifndef _VLAN_ICLI_FUNCTIONS_H_
#define _VLAN_ICLI_FUNCTIONS_H_

#include "icli_api.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL VLAN_ICLI_runtime_erps(      u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL VLAN_ICLI_runtime_mrp(       u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL VLAN_ICLI_runtime_mvr(       u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL VLAN_ICLI_runtime_gvrp(      u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL VLAN_ICLI_runtime_mstp(      u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL VLAN_ICLI_runtime_nas(       u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL VLAN_ICLI_runtime_rmirror(   u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL VLAN_ICLI_runtime_vcl(       u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);
BOOL VLAN_ICLI_runtime_voice_vlan(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

mesa_rc VLAN_ICLI_show_status(i32 session_id, icli_stack_port_range_t *plist, BOOL has_admin, BOOL has_combined, BOOL has_conflicts, BOOL has_erps, BOOL has_gvrp, BOOL has_mrp, BOOL has_mstp, BOOL has_mvr, BOOL has_nas, BOOL has_rmirror, BOOL has_vcl, BOOL has_voice_vlan);

/**
 * \brief Function for converting VLAN static tag to printable string
 * \param tx_tag_type [IN] Static tag type
 * \return string */
const char *VLAN_ICLI_tx_tag_type_to_txt(vtss_appl_vlan_tx_tag_type_t tx_tag_type);
mesa_rc VLAN_ICLI_hybrid_port_conf(u32 session_id, icli_stack_port_range_t *plist, vtss_appl_vlan_port_conf_t *new_conf, BOOL frametype, BOOL ingressfilter, BOOL porttype, BOOL tx_tag);
mesa_rc VLAN_ICLI_pvid_set(u32 session_id, icli_stack_port_range_t *plist, vtss_appl_vlan_port_mode_t port_mode, mesa_vid_t new_pvid);
mesa_rc VLAN_ICLI_mode_set(u32 session_id, icli_stack_port_range_t *plist, vtss_appl_vlan_port_mode_t new_mode);
mesa_rc VLAN_ICLI_trunk_tag_pvid_set(u32 session_id, icli_stack_port_range_t *plist, BOOL trunk_tag_pvid);
const char *VLAN_ICLI_port_mode_txt(vtss_appl_vlan_port_mode_t mode);
mesa_rc VLAN_ICLI_show_vlan(u32 session_id, icli_unsigned_range_t *vlan_list, char *name, BOOL has_vid, BOOL has_name, BOOL has_all, BOOL has_forbidden);
mesa_rc VLAN_ICLI_add_remove_forbidden(icli_stack_port_range_t *plist, icli_unsigned_range_t *vlan_list, BOOL has_add);
mesa_rc VLAN_ICLI_allowed_vids_set(u32 session_id, icli_stack_port_range_t *plist, vtss_appl_vlan_port_mode_t port_mode, icli_unsigned_range_t *vlan_list, BOOL has_default, BOOL has_all, BOOL has_none, BOOL has_add, BOOL has_remove, BOOL has_except);
void VLAN_ICLI_vlan_mode_enter(u32 session_id, icli_unsigned_range_t *vlan_list);
BOOL VLAN_ICLI_runtime_vlan_name(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif /* _VLAN_ICLI_FUNCTIONS_H_ */

