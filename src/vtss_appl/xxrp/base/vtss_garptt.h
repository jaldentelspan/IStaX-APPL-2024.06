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

#ifndef _VTSS_GARPTT_H_
#define _VTSS_GARPTT_H_

#include "vtss_garp.h"
#include <vlan_api.h>


void    vtss_garp_init(struct garp_application *a);

void    vtss_garp_leaveall(struct garp_application *a, struct garp_participant *p);

mesa_rc vtss_gid_join_request(struct garp_application *a, struct garp_participant *p,
                              garp_attribute_type_t    type, void *value);

mesa_rc vtss_gid_leave_request(struct garp_application *a, struct garp_participant *p,
                               garp_attribute_type_t type, void *value);

void    vtss_garp_leaveall(struct garp_application  *a, struct garp_participant *p);

u32     vtss_garp_timer_tick(u32 delay, struct garp_application *a);

mesa_rc vtss_garp_set_timer(struct garp_application *a, enum timer_context tc, u32 t, int full);

u32     vtss_garp_get_timer(struct garp_application *a, enum timer_context tc);

mesa_rc vtss_garp_chk_timer(enum timer_context tc, u32 new_time);

mesa_rc vtss_garp_gidtt_event_handler(struct garp_application *a, struct garp_participant *p,
                                      struct garp_gid_instance *gid, enum garp_all_event event);

mesa_rc vtss_garp_gidtt_event_handler2(struct garp_application *a, struct garp_participant *p,
                                       struct garp_gid_instance *gid, enum garp_all_event event);

mesa_rc vtss_garp_gidtt_rx_pdu_event_handler(struct garp_application *a, struct garp_participant *p,
                                             struct garp_gid_instance *gid, enum garp_attribute_event attribute_event);

void    vtss_garp_force_txPDU(struct garp_application *a);

void    vtss_garp_add_list(struct PN_participant **P, struct PN_participant *p);
void    vtss_garp_remove_list(struct PN_participant **P, struct PN_participant *p);
void    vtss_garp_reinsert_list(struct PN_participant **P, struct PN_participant *p);

int vtss_garp_move_list(struct PN_participant **P, struct PN_participant **Q, struct PN_participant *p);

// returns the truth value of p1<p2
typedef BOOL (*garp_less_t)( struct PN_participant *p1,  struct PN_participant *p2);
void vtss_garp_add_sort_list(struct PN_participant **P, struct PN_participant *p, garp_less_t less);


#define MEMBER(TYPE, ELEMENT, POINTER) ((TYPE*)( \
            ((char*)POINTER) - offsetof(TYPE,ELEMENT) \
))

#define MEMBER_(TYPE, ELEMENT, IDX, POINTER) ((TYPE*)( \
            ((char*)POINTER) - (offsetof(TYPE,ELEMENT[0]) + (sizeof(struct PN_participant)) * IDX) \
))

#define GARP_POSITIVE_U32 0x7fffffffUL

vtss_tick_count_t vtss_garp_get_clock(struct garp_application *a, enum timer_context tc);

mesa_rc vtss_garp_registrar_administrative_control(struct garp_application *a,
                                                   struct garp_participant *p,
                                                   garp_attribute_type_t type,
                                                   void *value,
                                                   vlan_registration_type_t S);

#endif
