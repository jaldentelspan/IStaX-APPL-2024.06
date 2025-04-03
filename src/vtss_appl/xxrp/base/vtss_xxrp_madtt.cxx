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
#include "vtss_xxrp_types.h"
#include "vtss_xxrp_madtt.h"
#include "vtss_xxrp_api.h"
#include "vtss_xxrp.h"
#include "vtss_xxrp_os.h"
#include "vtss_xxrp_timers.h"
#include "../platform/xxrp_trace.h"

/* TODO: Expiration of timers should be takencare in timer thread */
/************************************************************************************************************
 *                                          LEAVEALL FSM
 ************************************************************************************************************/
static u32 start_la_timer_in_timer_thread(vtss_mrp_mad_t  *mad)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "LA FSM: start_la_timer_in_timer_thread";
    //LA timer expiry is handled in timer thread. If it is handled here, vtss_timer_tick() has to called twice.
    vtss_xxrp_stop_leaveall_timer(mad);
    vtss_xxrp_start_join_timer(mad);
    //vtss_xxrp_start_leaveall_timer(mad, FALSE);
    return VTSS_RC_OK;
}

/* Function to start LeaveAll timer */
static u32 start_la_timer(vtss_mrp_mad_t  *mad)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "LA FSM: start LA timer";
    vtss_xxrp_start_leaveall_timer(mad, TRUE);
    return VTSS_RC_OK;
}

/* Function to send LeaveAll message: This will be takencare in timer thread */
static u32 sLA(vtss_mrp_mad_t  *mad)
{
    /* TODO: Handle LA event against all the stms */
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "LA FSM: sLA";
    return VTSS_RC_OK;
}

/* Function that does nothing when STM doesn't need to do anything */
static u32 la_stm_no_change(vtss_mrp_mad_t *mad)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "LA FSM: la_stm_no_change";
    return VTSS_RC_OK;
}

#define LA_NO_STATE_CHANGE (last_la_state + 1)

/* LeaveAll State Machine */
static const xxrp_la_stm_t xxrp_la_stm[last_la_state][last_la_event]
= {
    { /* active */
        {passive_la, start_la_timer},
        {passive_la, sLA},
        {passive_la, start_la_timer},
        {active_la, start_la_timer_in_timer_thread}
    },
    { /* passive */
        {passive_la, start_la_timer},
        {LA_NO_STATE_CHANGE, la_stm_no_change},
        {passive_la, start_la_timer},
        {active_la, start_la_timer_in_timer_thread}
    }
};

/************************************************************************************************************
 *                                          Periodic FSM
 ***********************************************************************************************************/
static u32 start_periodic_timer(periodic_stm_args  *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Periodic FSM: start periodic timer";
    vtss_xxrp_start_periodic_timer(args->mad, TRUE);
    return VTSS_RC_OK;
}

/* TODO: Send periodic event to all the stms */
static u32 start_periodic_timer_and_trigger_periodic_event(periodic_stm_args  *args)
{
    u32 rc = VTSS_RC_OK;
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Periodic FSM: periodic timer and periodic event";
    rc = vtss_mrp_handle_periodic_timer(args->application, args->mad->port_no);
    vtss_xxrp_start_periodic_timer(args->mad, TRUE);
    return rc;
}

static u32 periodic_stm_no_change(periodic_stm_args  *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Periodic FSM: do nothing";
    return VTSS_RC_OK;
}
#define PERIODIC_NO_STATE_CHANGE     (last_periodic_state + 1)

/* Periodic State Machine */
static const xxrp_periodic_stm_t xxrp_periodic_stm[last_periodic_state][last_periodic_event]
= {
    { /* State: Active */
        {active_periodic, start_periodic_timer},
        {PERIODIC_NO_STATE_CHANGE, periodic_stm_no_change},
        {passive_periodic, periodic_stm_no_change},
        {active_periodic,  start_periodic_timer_and_trigger_periodic_event}
    },
    { /* State: Passive */
        {active_periodic, start_periodic_timer},
        {active_periodic, start_periodic_timer},
        {PERIODIC_NO_STATE_CHANGE, periodic_stm_no_change},
        {PERIODIC_NO_STATE_CHANGE, periodic_stm_no_change}
    }
};

/************************************************************************************************************
 *                                          Registrar FSM
 ***********************************************************************************************************/
static u32 start_leave_timer(reg_stm_args *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Reg FSM: start_leave_timer";
    vtss_xxrp_start_leave_timer(args->mad, args->indx);
    return VTSS_RC_OK;
}
static u32 New_func(reg_stm_args *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Reg FSM: New_func";
    /* Send the MAD_Join.indication to the application with the new flag set */
    return (u32)vtss_mrp_join_indication(args->application, args->mad->port_no, args->indx, TRUE);
}
static u32 reg_stm_no_change(reg_stm_args *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Reg FSM: reg_stm_no_change";
    return VTSS_RC_OK;
}

static u32 Lv_func(reg_stm_args *args)
{
    u32 rc;
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Reg FSM: Lv_func";
    /* Send the MAD_Leave.indication to the application */
    rc = (u32)vtss_mrp_leave_indication(args->application, args->mad->port_no, args->indx);
    return rc;
}

static u32 stop_leave_timer_new_func(reg_stm_args *args)
{
    u32 rc;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Reg FSM: stop_leave_timer_new_func";
    vtss_xxrp_stop_leave_timer(args->mad, args->indx);
    /* Send the MAD_Join.indication to the application with the new flag set */
    rc = (u32)vtss_mrp_join_indication(args->application, args->mad->port_no, args->indx, FALSE);
    return rc;
}

static u32 stop_leave_timer(reg_stm_args *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Reg FSM: stop_leave_timer";
    vtss_xxrp_stop_leave_timer(args->mad, args->indx);
    return VTSS_RC_OK;
}

static u32 Join_func(reg_stm_args *args)
{
    u32 rc;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Reg FSM: Join_func";
    /* Send the MAD_Join.indication to the application */
    rc = (u32)vtss_mrp_join_indication(args->application, args->mad->port_no, args->indx, FALSE);
    return rc;

}

#define REG_NO_STATE_CHANGE     (last_reg_state + 1)
/* Registrar State Machine */
static const xxrp_reg_stm_t xxrp_reg_stm[last_reg_state][last_reg_event]
= {
    { /* State: IN */
        {MT, reg_stm_no_change},
        {IN, New_func},
        {IN, reg_stm_no_change},
        {IN, reg_stm_no_change},
        {LV, start_leave_timer},
        {LV, start_leave_timer},
        {LV, start_leave_timer},
        {LV, start_leave_timer},
        {MT, reg_stm_no_change},
        {REG_NO_STATE_CHANGE, reg_stm_no_change}
    },
    { /* State: LV */
        {MT, reg_stm_no_change},
        {IN, stop_leave_timer_new_func},
        {IN, stop_leave_timer},
        {IN, stop_leave_timer},
        {REG_NO_STATE_CHANGE, reg_stm_no_change},
        {REG_NO_STATE_CHANGE, reg_stm_no_change},
        {REG_NO_STATE_CHANGE, reg_stm_no_change},
        {REG_NO_STATE_CHANGE, reg_stm_no_change},
        {MT, Lv_func},
        {MT, Lv_func}
    },
    { /* State: MT */
        {MT, reg_stm_no_change},
        {IN, New_func},
        {IN, Join_func},
        {IN, Join_func},
        {REG_NO_STATE_CHANGE, reg_stm_no_change},
        {REG_NO_STATE_CHANGE, reg_stm_no_change},
        {REG_NO_STATE_CHANGE, reg_stm_no_change},
        {REG_NO_STATE_CHANGE, reg_stm_no_change},
        {MT, reg_stm_no_change},
        {MT, reg_stm_no_change}
    }
};

/************************************************************************************************************
 *                                          Applicant FSM
 ***********************************************************************************************************/
static u32 appl_stm_no_change(appl_stm_args *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Appl FSM: do nothing";
    return VTSS_RC_OK;
}
static u32 appl_req_tx(appl_stm_args *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Appl FSM: req tx";
    vtss_xxrp_start_join_timer(args->mad);
    return VTSS_RC_OK;
}
/* Function to send join message: This will be takencare in timer thread */
static u32 appl_send_join(appl_stm_args *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Appl FSM: send join";
    vtss_xxrp_start_join_timer(args->mad);
    return VTSS_RC_OK;
}
/* Function to send join or mt message: This will be takencare in timer thread */
static u32 appl_send_join_or_mt(appl_stm_args *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Appl FSM: send join or mt";
    vtss_xxrp_start_join_timer(args->mad);
    return VTSS_RC_OK;
}
/* Function to send new message: This will be takencare in timer thread */
static u32 appl_send_new(appl_stm_args *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Appl FSM: send new";
    vtss_xxrp_start_join_timer(args->mad);
    return VTSS_RC_OK;
}
/* Function to send leave message: This will be takencare in timer thread */
static u32 appl_send_leave(appl_stm_args *args)
{
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Appl FSM: send leave";
    return VTSS_RC_OK;
}
#define APPL_NO_STATE_CHANGE     (last_appl_state + 1)

/* Applicant State Machine. See IEEE802.1Q-2011, table 10.3 */
static const xxrp_appl_stm_t xxrp_appl_stm[last_appl_state][last_appl_event]
= {
    {/* State: VO */
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {VN, appl_req_tx},
        {VP, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {AO, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {LO, appl_req_tx},
        {LO, appl_req_tx},
        {LO, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {LO, appl_req_tx},
        {LO, appl_req_tx}
    },
    {/* State: VP */
        {VO, appl_stm_no_change},
        {VN, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {VO, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {AP, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {AA, appl_send_join},
        {AA, appl_send_join_or_mt},
        {VP, appl_req_tx}
    },
    {/* State: VN */
        {VO, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {LA, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {AN, appl_send_new},
        {AN, appl_send_new},
        {VN, appl_req_tx}
    },
    {/* State: AN */
        {VO, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {LA, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {VN, appl_req_tx},
        {VN, appl_req_tx},
        {VN, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {QA, appl_send_new},
        {QA, appl_send_new},
        {VN, appl_req_tx}
    },
    {/* State: AA */
        {VO, appl_stm_no_change},
        {VN, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {LA, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {QA, appl_stm_no_change},
        {QA, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {VP, appl_req_tx},
        {VP, appl_req_tx},
        {VP, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {QA, appl_send_join},
        {QA, appl_send_join},
        {VP, appl_req_tx}
    },
    {/* State: QA */
        {VO, appl_stm_no_change},
        {VN, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {LA, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {AA, appl_req_tx},
        {AA, appl_req_tx},
        {VP, appl_req_tx},
        {VP, appl_req_tx},
        {VP, appl_req_tx},
        {AA, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_send_join},
        {VP, appl_req_tx}
    },
    {/* State: LA */
        {VO, appl_stm_no_change},
        {VN, appl_req_tx},
        {AA, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {VO, appl_send_leave},
        {LO, appl_req_tx},
        {LO, appl_req_tx}
    },
    {/* State: AO */
        {VO, appl_stm_no_change},
        {VN, appl_req_tx},
        {AP, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {QO, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {LO, appl_req_tx},
        {LO, appl_req_tx},
        {LO, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {LO, appl_req_tx},
        {LO, appl_req_tx}
    },
    {/* State: QO */
        {VO, appl_stm_no_change},
        {VN, appl_req_tx},
        {QP, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {AO, appl_stm_no_change},
        {AO, appl_stm_no_change},
        {LO, appl_req_tx},
        {LO, appl_req_tx},
        {LO, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {LO, appl_req_tx},
        {LO, appl_req_tx}
    },
    {/* State: AP */
        {VO, appl_stm_no_change},
        {VN, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {AO, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {QP, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {VP, appl_req_tx},
        {VP, appl_req_tx},
        {VP, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {QA, appl_send_join},
        {QA, appl_send_join},
        {VP, appl_req_tx}
    },
    {/* State: QP */
        {VO, appl_stm_no_change},
        {VN, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {QO, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {AP, appl_req_tx},
        {AP, appl_req_tx},
        {VP, appl_req_tx},
        {VP, appl_req_tx},
        {VP, appl_req_tx},
        {AP, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {QA, appl_send_join},
        {VP, appl_req_tx}
    },
    {/* State: LO */
        {VO, appl_stm_no_change},
        {VN, appl_req_tx},
        {VP, appl_req_tx},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {VO, appl_stm_no_change},
        {VO, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {VO, appl_send_join_or_mt},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change},
        {APPL_NO_STATE_CHANGE, appl_stm_no_change}
    }
};
/*****************************************************************************************************************************/

void vtss_mrp_madtt_init_state_machine(vtss_mrp_mad_machine_t *machine, BOOL is_attr_registered)
{
    machine->applicant = VO;
    if (is_attr_registered) {
        machine->registrar = IN;
    } else {
        machine->registrar = MT;
    }
}

void vtss_mrp_madtt_init_la_and_periodic_state_machines(vtss_mrp_mad_t *mad_port)
{
    mad_port->leaveall_stm_current_state = passive_la;
    mad_port->periodic_stm_current_state = passive_periodic;
}

mesa_rc vtss_mrp_madtt_event_handler1(vtss_mrp_appl_t application, vtss_mrp_mad_t *mad_port, vtss_mad_fsm_events *events)
{
    periodic_stm_args periodic_args;
    mesa_rc           rc = VTSS_RC_OK;
    u16               prev_state;

    /* Handle LA FSM state changes */
    if (events->la_event != last_la_event) {
        prev_state = mad_port->leaveall_stm_current_state;
        /* LA state change */
        if (xxrp_la_stm[mad_port->leaveall_stm_current_state][events->la_event].state != LA_NO_STATE_CHANGE) {
            mad_port->leaveall_stm_current_state = xxrp_la_stm[mad_port->leaveall_stm_current_state][events->la_event].state;
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "LA FSM transition from " << mad_la_or_periodic_state_to_txt(prev_state)
                                                       << " to " << mad_la_or_periodic_state_to_txt(mad_port->leaveall_stm_current_state);
        } else {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, RACKET) << "LA FSM remaining in "
                                                        << mad_la_or_periodic_state_to_txt(mad_port->leaveall_stm_current_state);
        }
        /* LA action handler */
        rc = xxrp_la_stm[prev_state][events->la_event].la_stm_callback(mad_port);
    }

    /* Handle Periodic FSM state changes */
    if (events->periodic_event != last_periodic_event) {
        prev_state = mad_port->periodic_stm_current_state;
        /* Periodic state change */
        if (xxrp_periodic_stm[mad_port->periodic_stm_current_state][events->periodic_event].state != PERIODIC_NO_STATE_CHANGE) {
            mad_port->periodic_stm_current_state = xxrp_periodic_stm[mad_port->periodic_stm_current_state][events->periodic_event].state;
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Periodic FSM transition from " << mad_la_or_periodic_state_to_txt(prev_state)
                                                       << " to " << mad_la_or_periodic_state_to_txt(mad_port->periodic_stm_current_state);
        } else {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, RACKET) << "Periodic FSM remaining in "
                                                        << mad_la_or_periodic_state_to_txt(mad_port->periodic_stm_current_state);
        }
        periodic_args.mad = mad_port;
        periodic_args.application = application;
        /* Periodic action handler */
        rc += xxrp_periodic_stm[prev_state][events->periodic_event].periodic_stm_callback(&periodic_args);
    }

    return rc;
}

mesa_rc vtss_mrp_madtt_event_handler2(vtss_mrp_appl_t application, vtss_mrp_mad_t *mad_port, u16 mad_fsm_indx, vtss_mad_fsm_events *events)
{
    reg_stm_args      args;
    appl_stm_args     appl_args;
    mesa_rc           rc = VTSS_RC_OK;
    u16               prev_state, new_state;
    BOOL              p2p = FALSE;

    /* Handle Registrar FSM state changes */
    if (events->reg_event != last_reg_event) {
        prev_state = mad_port->machines[mad_fsm_indx].registrar;
        //if ((prev_state == IN) && ()) {
        /* Registrar state change */
        if (xxrp_reg_stm[mad_port->machines[mad_fsm_indx].registrar][events->reg_event].state != REG_NO_STATE_CHANGE) {
            mad_port->machines[mad_fsm_indx].registrar =
                xxrp_reg_stm[mad_port->machines[mad_fsm_indx].registrar][events->reg_event].state;
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Reg FSM transition from " << mad_reg_state_to_txt(prev_state)
                                                       << " to " << mad_reg_state_to_txt(mad_port->machines[mad_fsm_indx].registrar);
        } else {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, RACKET) << "Reg FSM remaining in "
                                                        << mad_reg_state_to_txt(mad_port->machines[mad_fsm_indx].registrar);
        }
        args.mad = mad_port;
        args.indx = mad_fsm_indx;
        args.application = application;
        /* Registrar action handler */
        rc += xxrp_reg_stm[prev_state][events->reg_event].reg_stm_callback(&args);
    }

    /* Handle Applicant FSM state changes */
    if (events->appl_event != last_appl_event) {
        prev_state = mad_port->machines[mad_fsm_indx].applicant;
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, RACKET) << "Port " << l2port2str(mad_port->port_no)
                                                    << " index " << mad_fsm_indx
                                                    << " in state " << mad_appl_state_to_txt(prev_state);
        new_state = xxrp_appl_stm[mad_port->machines[mad_fsm_indx].applicant][events->appl_event].state;
        p2p = XXRP_is_port_point2point(mad_port->port_no);
        if (((new_state == AO) || (new_state == AP)) && (p2p == TRUE)) { /* Ignore the state transitions */
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, RACKET) << "Applicant FSM remaining in "
                                                        << mad_appl_state_to_txt(prev_state);
            return rc;
        }
        if ((p2p == FALSE) && ((prev_state == AA) && (events->appl_event == rIn_app))) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, RACKET) << "Applicant FSM remaining in "
                                                        << mad_appl_state_to_txt(prev_state);
            return rc;
        }
        /* Note 8 of Applicant state machine is handled in timer thread */
        /* Applicant state change */
        if (xxrp_appl_stm[mad_port->machines[mad_fsm_indx].applicant][events->appl_event].state != APPL_NO_STATE_CHANGE) {
            mad_port->machines[mad_fsm_indx].applicant = xxrp_appl_stm[mad_port->machines[mad_fsm_indx].applicant][events->appl_event].state;
            if (!((prev_state == VO && new_state == LO) || (prev_state == LO && new_state == VO))) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Appl FSM transition from " << mad_appl_state_to_txt(prev_state)
                                                           << " to " << mad_appl_state_to_txt(new_state);
            }
        } else {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, RACKET) << "Applicant FSM remaining in "
                                                        << mad_appl_state_to_txt(mad_port->machines[mad_fsm_indx].applicant);
        }
        appl_args.mad = mad_port;
        /* Applicant action handler */
        rc += xxrp_appl_stm[prev_state][events->appl_event].appl_stm_callback(&appl_args);
    }

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Exiting the FSM event handler";
    return rc;
}
