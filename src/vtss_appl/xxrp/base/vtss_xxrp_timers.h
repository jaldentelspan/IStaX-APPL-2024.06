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

#ifndef _VTSS_XXRP_TIMERS_H_
#define _VTSS_XXRP_TIMERS_H_
#include "vtss_xxrp_types.h"

#define VTSS_XXRP_IS_LEAVE_TIMER_RUNNING(mad, indx)   (mad->leave_timer_running[indx / 8] & (1U << (indx % 8)))
#define VTSS_XXRP_START_LEAVE_TIMER(mad, indx)        (mad->leave_timer_running[indx / 8] |= (1U << (indx % 8)))
#define VTSS_XXRP_STOP_LEAVE_TIMER(mad, indx)         (mad->leave_timer_running[indx / 8] &= ~(1U << (indx % 8)))
#define VTSS_XXRP_IS_LEAVE_TIMER_KICK(mad, indx)      (mad->leave_timer_kick[indx / 8] & (1U << (indx % 8)))
#define VTSS_XXRP_SET_LEAVE_TIMER_KICK(mad, indx)     (mad->leave_timer_kick[indx / 8] |= (1U << (indx % 8)))
#define VTSS_XXRP_CLEAR_LEAVE_TIMER_KICK(mad, indx)   (mad->leave_timer_kick[indx / 8] &= ~(1U << (indx % 8)))

void vtss_xxrp_start_join_timer(vtss_mrp_mad_t *mad);
void vtss_xxrp_start_leave_timer(vtss_mrp_mad_t *mad, u16 index);
void vtss_xxrp_start_leaveall_timer(vtss_mrp_mad_t *mad, BOOL restart);
void vtss_xxrp_start_periodic_timer(vtss_mrp_mad_t *mad, BOOL restart);
void vtss_xxrp_stop_join_timer(vtss_mrp_mad_t *mad);
void vtss_xxrp_stop_leave_timer(vtss_mrp_mad_t *mad, u16 index);
void vtss_xxrp_stop_leaveall_timer(vtss_mrp_mad_t *mad);
void vtss_xxrp_stop_periodic_timer(vtss_mrp_mad_t *mad);
#endif /* _VTSS_XXRP_TIMERS_H_ */
