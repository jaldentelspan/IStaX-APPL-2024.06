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

#ifndef _VTSS_GVRP_H_
#define _VTSS_GVRP_H_


#ifdef VTSS_SW_OPTION_GVRP

#include "vtss_garp.h"
#include <vtss_xxrp_api.h>
#include <vlan_api.h>

#ifdef __cplusplus
extern "C" {
#endif
#define GVRP_CRIT_ENTER() vtss_mrp_crit_enter()
#define GVRP_CRIT_EXIT()  vtss_mrp_crit_exit()

extern mesa_rc  vtss_gvrp_construct(int max_participants, int max_vlans);
extern void vtss_gvrp_destruct(BOOL set_default);

extern int vtss_gvrp_rx_pdu(int port_no, const u8 *pdu, int len);

extern mesa_rc vtss_gvrp_gid(int port_no, mesa_vid_t vid, struct garp_gid_instance **gid);
extern void    vtss_gvrp_gid_done(struct garp_gid_instance **gid);

extern mesa_rc vtss_gvrp_participant(int port_no, struct garp_participant **p);

//extern u32 vtss_GVRP_txPDU_timeout(void);
extern u32 vtss_gvrp_txPDU_timeout(struct garp_gid_instance *gid);
extern int vtss_gvrp_leave_timeout(struct garp_gid_instance *gid);
extern u32 vtss_gvrp_leaveall_timeout(int port_no);

// Management function for Join, Leave, setting timer values and administrative control for registrar.
extern mesa_rc vtss_gvrp_join_request(int port_no, mesa_vid_t vid);
extern mesa_rc vtss_gvrp_leave_request(int port_no, mesa_vid_t vid);
extern mesa_rc vtss_gvrp_set_timer(enum timer_context tc, u32 T);
extern u32     vtss_gvrp_get_timer(enum timer_context tc);
extern mesa_rc vtss_gvrp_registrar_administrative_control(int port_no, mesa_vid_t vid, vlan_registration_type_t S);
extern mesa_rc vtss_gvrp_chk_timer(enum timer_context tc, u32 T);

// Enabling of ports
extern mesa_rc vtss_gvrp_port_control_conf_get(u32 port_no, BOOL *const status);
extern mesa_rc vtss_gvrp_port_control_conf_set(u32 port_no, BOOL enable);

extern void vtss_gvrp_update_vlan_to_msti_mapping(void);
extern mesa_rc vtss_gvrp_mstp_port_state_change_handler(int port_no, u8 msti, vtss_mrp_mstp_port_state_change_type_t  port_state_type);

extern u32 vtss_gvrp_global_control_conf_create(vtss_mrp_t **mrp_app);

extern uint vtss_gvrp_timer_tick(uint);

extern int vtss_gvrp_is_enabled(void);
extern int vtss_gvrp_max_vlans(void);
extern mesa_rc vtss_gvrp_max_vlans_set(int m);
extern int vtss_gvrp_gip_context(struct garp_gid_instance *gid);

extern void gvrp_dump_msti_state(void);
extern void vtss_gvrp_internal_statistic(void);
extern void vtss_gvrp_dump_gip(void);
extern void vtss_gvrp_dump_gip2(void);
extern void vtss_gvrp_state_tracing(mesa_vid_t vid);

extern int vtss_gvrp_vlan_membership_getnext_add(u32 *port, mesa_vid_t *vid);
extern int vtss_gvrp_vlan_membership_getnext_del(u32 *port, mesa_vid_t *vid);

// For Zyxel branch only
extern BOOL vtss_gvrp_max_check(u8 *vids);

#ifdef __cplusplus
}
#endif

#endif  /* VTSS_SW_OPTION_GVRP */

#endif  /* _VTSS_GVRP_H_ */
