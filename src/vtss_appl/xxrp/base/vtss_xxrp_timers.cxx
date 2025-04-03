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

#include "vtss_xxrp_timers.h"
#include "vtss_xxrp_mad.h"
#include "vtss_xxrp_callout.h"
#include "vtss_xxrp_os.h"
#include "vtss/basics/trace.hxx"
#include "../platform/xxrp_trace.h"

void vtss_xxrp_start_join_timer(vtss_mrp_mad_t *mad)
{
    if (mad != NULL) {
        if (mad->join_timer_running == FALSE) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Starting join timer on port #" << mad->port_no;
            /* TODO: Randomize the join timer (0 to join_time) also Point2Point support */
            mad->join_timer_count = mad->join_timeout;
            mad->join_timer_running = TRUE;
            mad->join_timer_kick = TRUE;
            vtss_mrp_timer_kick();
        }
    }
}

void vtss_xxrp_start_leave_timer(vtss_mrp_mad_t *mad, u16 indx)
{
    if (mad != NULL) {
        if (!VTSS_XXRP_IS_LEAVE_TIMER_RUNNING(mad, indx)) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Starting leave timer on port "
                                                         << l2port2str(mad->port_no) << ", attr #" << indx;
            mad->leave_timer_count[indx] = mad->leave_timeout;
            VTSS_XXRP_START_LEAVE_TIMER(mad, indx);
            //mad->leave_timer_running[indx] = TRUE;
            VTSS_XXRP_SET_LEAVE_TIMER_KICK(mad, indx);
            vtss_mrp_timer_kick();
        }
    }
}

void vtss_xxrp_start_leaveall_timer(vtss_mrp_mad_t *mad, BOOL restart)
{
    if (mad != NULL) {
        if ((!mad->leaveall_timer_running) || (mad->leaveall_timer_running && restart)) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Starting LeaveAll timer on port " << l2port2str(mad->port_no);
            /* TODO: Randomize the LA timer 1 to 1.5 times of LA timer */
            mad->leaveall_timer_count = mad->leaveall_timeout;
            mad->leaveall_timer_running = TRUE;
            mad->leaveall_timer_kick = TRUE;
            vtss_mrp_timer_kick();
        }
    }
}

void vtss_xxrp_start_periodic_timer(vtss_mrp_mad_t *mad, BOOL restart)
{
    if (mad != NULL) {
        if ((!mad->periodic_timer_running) || (mad->periodic_timer_running && restart)) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Starting periodic timer on port " << l2port2str(mad->port_no);
            mad->periodic_timer_count = mad->periodic_tx_timeout;
            mad->periodic_timer_running = TRUE;
            mad->periodic_timer_kick = TRUE;
            vtss_mrp_timer_kick();
        }
    }
}

void vtss_xxrp_stop_join_timer(vtss_mrp_mad_t *mad)
{
    if (mad != NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Stopping join timer on port #" << mad->port_no;
        mad->join_timer_running = FALSE;
    }
}

void vtss_xxrp_stop_leave_timer(vtss_mrp_mad_t *mad, u16 indx)
{
    if (mad != NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Stopping leave timer on port #" << mad->port_no << ", attr #" << indx;
        VTSS_XXRP_STOP_LEAVE_TIMER(mad, indx);
    }
}
void vtss_xxrp_stop_leaveall_timer(vtss_mrp_mad_t *mad)
{
    if (mad != NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Stopping leaveall timer on port #" << mad->port_no;
        mad->leaveall_timer_running = FALSE;
    }
}
void vtss_xxrp_stop_periodic_timer(vtss_mrp_mad_t *mad)
{
    if (mad != NULL) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Stopping periodic timer on port #" << mad->port_no;
        mad->periodic_timer_running = FALSE;
    }
}
