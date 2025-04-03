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
#include "../platform/xxrp_trace.h"
#include "vtss_xxrp_api.h"
#include "vtss_xxrp_os.h"
#include "vtss_xxrp_callout.h" /* MSTP function call */
#ifdef VTSS_SW_OPTION_MVRP
#include "vtss_xxrp_debug.h"
#endif
#include "vtss_xxrp_types.h"
#include "vtss_xxrp.h"
#include "vtss_xxrp_os.h"
#include "vtss_xxrp_madtt.h"
#include "vtss_xxrp_timers.h"
#include "vtss_xxrp_types.hxx"
#include "subject.hxx"
#ifdef VTSS_SW_OPTION_GVRP
#include "vtss_gvrp.h"
#endif
#include "misc_api.h"
#include "vtss_mrp.hxx"

static const u8 mrp_mvrp_multicast_macaddr[] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x21};
#ifdef VTSS_SW_OPTION_MVRP
static const u8 mrp_mvrp_eth_type[]          = {0x88, 0xF5};
#endif
#define MRP_MVRP_MULTICAST_MACADDR mrp_mvrp_multicast_macaddr
#define VTSS_MVRP_ETH_TYPE         mrp_mvrp_eth_type

/**
 * \brief structure to maintain MRP global data.
 **/
typedef struct {
    vtss_mrp_t *applications[VTSS_MRP_APPL_MAX];
} vtss_xxrp_glb_t;

static vtss_xxrp_glb_t glb_mrp;

#define MRP_APPL_GET(app_type)         (glb_mrp.applications[app_type])
#define MRP_MAD_GET(app_type, port_no) (glb_mrp.applications[app_type]->mad[port_no])
#define MRP_MAP_GET(app_type, port_no) (glb_mrp.applications[app_type]->map[port_no])
#define MRP_APPL_BASE_GET              (glb_mrp.applications)
#define MRP_MAD_BASE_GET(app_type)     (glb_mrp.applications[app_type]->mad)
#define MRP_MAP_BASE_GET(app_type)     (glb_mrp.applications[app_type]->map)
#define MRP_CRIT_ASSERT()              vtss_mrp_crit_assert();

struct VtssMrpCritdGuard {
    VtssMrpCritdGuard(int line)
    {
        vtss_mrp_crit_enter();
    }

    ~VtssMrpCritdGuard()
    {
        vtss_mrp_crit_exit();
    }
};

#define MRP_CRIT_SCOPE() VtssMrpCritdGuard __lock_guard__(__LINE__)

const char *vtss_mrp_event2txt(u8 event)
{
    switch (event) {
    case VTSS_XXRP_APPL_EVENT_NEW:
        return "New";
    case VTSS_XXRP_APPL_EVENT_JOININ:
        return "JoinIn";
    case VTSS_XXRP_APPL_EVENT_IN:
        return "In";
    case VTSS_XXRP_APPL_EVENT_JOINMT:
        return "JoinMt";
    case VTSS_XXRP_APPL_EVENT_MT:
        return "Mt";
    case VTSS_XXRP_APPL_EVENT_LV:
        return "Leave";
    case VTSS_XXRP_APPL_EVENT_INVALID:
        return "Invalid";
    case VTSS_XXRP_APPL_EVENT_LA:
        return "LeaveAll";
    default:
        return "Unrecognized";
    }
}

void mvrp_vid_to_mad_fsm_index(mesa_vid_t vid, u16 *mad_fsm_index)
{
    *mad_fsm_index = vid - 1;
}

/* TODO: On disable remove static mac entry, release all map and mad structures for that port, unregister with packet module
 *       On enable add static mac entry and register with packet module.
 */
mesa_rc vtss_mrp_global_control_conf_set(vtss_mrp_appl_t appl, BOOL enable)
{
    vtss_mrp_t *mrp_app = NULL;
    mesa_rc    rc = VTSS_RC_OK;

    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Set global MRP status for appl = "
            << appl << ", enable = " << enable;
    MRP_CRIT_SCOPE();
    if (enable) { /* MRP application enable */
        if (MRP_APPL_GET(appl)) { /* Application present in the db */
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << "MRP appl is already enabled";
            return VTSS_RC_OK;
        } else { /* Application is not present in the db; it means it is not globally enabled */
            rc = XXRP_ERROR_INVALID_APPL;
#ifdef VTSS_SW_OPTION_MVRP
            if (appl == VTSS_MRP_APPL_MVRP) {
                if ((rc = vtss_mvrp_global_control_conf_create(&mrp_app)) != VTSS_RC_OK) {
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, ERROR) << "unable to create application structure";
                }
            } 
#endif

#ifdef VTSS_SW_OPTION_GVRP
            if (appl == VTSS_GARP_APPL_GVRP) {
                if ((rc = vtss_gvrp_global_control_conf_create(&mrp_app)) != VTSS_RC_OK) {
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, ERROR) << "unable to create application structure";
                }
            } 
#endif

            if ((rc == VTSS_RC_OK) && (mrp_app != NULL)) {
                rc = mrp_appl_add_to_database(MRP_APPL_BASE_GET, mrp_app);
            }
        }
    } else { /* MRP application disable */
        if (MRP_APPL_GET(appl)) { /* Application present in the db */
            rc = mrp_appl_del_from_database(MRP_APPL_BASE_GET, appl);
        } else { /* Application is not present in the db; it means it is not globally enabled */
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << "MRP appl is already disabled";
            return VTSS_RC_OK;
        }
    }
    return rc;
}

mesa_rc vtss_mrp_global_control_conf_get(vtss_mrp_appl_t appl, BOOL  *const status)
{
    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (!status) {
        return XXRP_ERROR_INVALID_PARAMETER;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Get global MRP status for appl = " << appl;
    MRP_CRIT_SCOPE();
    *status = (MRP_APPL_GET(appl)) ? TRUE : FALSE;

    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_MVRP
static mesa_rc vtss_mrp_port_control_conf_set(vtss_mrp_appl_t appl, u32 l2port, BOOL enable, vtss::VlanList &vls)
{
    mesa_rc rc = VTSS_RC_OK;
    u8      msti;

    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS_) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Set MRP status for appl = " << appl
            << ", port = " << l2port2str(l2port)
            << ", enable = " << (enable ? "Enabled" : "Disabled");
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (enable) { /* MRP MAD create for the port_no */
        /* Check if already created */
        if (MRP_MAD_GET(appl, l2port)) { /* MAD already present in the db */
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << "MRP appl is already enabled on this port";
            return VTSS_RC_OK;
        } else { /* MAD not present in the db */
            rc += vtss_mrp_mad_create_port(MRP_APPL_GET(appl), MRP_MAD_BASE_GET(appl), l2port, vls);
            if (rc == VTSS_RC_OK) {
                rc += vtss_mrp_map_create_port(MRP_MAP_BASE_GET(appl), l2port);
                if (rc == VTSS_RC_OK) {
                    for (msti = 0; msti < MRP_MSTI_MAX; msti++) {
                        if (mrp_mstp_port_status_get(msti, l2port) == VTSS_RC_OK) { /* FORWARDING */
                            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_PLATFORM, DEBUG) << "Port#" << l2port2str(l2port)
                                    << ":msti#" << msti
                                    << " is in forwarding state";
                            rc += vtss_mrp_map_connect_port(MRP_MAP_BASE_GET(appl), msti, l2port);
                            rc += vtss_mrp_map_port_change_handler(appl, l2port, TRUE);
                        }
                    }
                }
            }
        }
    } else { /* MRP MAD destroy for the l2port */
        if (MRP_MAD_GET(appl, l2port)) { /* MAD already present in the db */
            rc += vtss_mrp_map_port_change_handler(appl, l2port, FALSE);
            rc += vtss_mrp_mad_destroy_port(MRP_APPL_GET(appl), MRP_MAD_BASE_GET(appl), l2port);
            if (rc == VTSS_RC_OK) {
                for (msti = 0; msti < MRP_MSTI_MAX; msti++) {
                    rc += vtss_mrp_map_disconnect_port(MRP_MAP_BASE_GET(appl), msti, l2port);
                }
                rc += vtss_mrp_map_destroy_port(MRP_MAP_BASE_GET(appl), l2port);
                if (rc != VTSS_RC_OK) {
                    return XXRP_ERROR_INVALID_PARAMETER;
                }
            }
        } else { /* MAD not present in the db */
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << "MRP appl is already disabled on this port";
            return VTSS_RC_OK;
        }
    }

    return rc;
}
#endif

mesa_rc vtss_xxrp_port_control_conf_set(vtss_mrp_appl_t appl, u32 l2port, BOOL enable, vtss::VlanList &vls)
{
    switch (appl) {
#ifdef VTSS_SW_OPTION_MVRP
    case VTSS_MRP_APPL_MVRP:
        return vtss_mrp_port_control_conf_set(appl, l2port, enable, vls);
#endif

#ifdef VTSS_SW_OPTION_GVRP
    case VTSS_GARP_APPL_GVRP:
        return vtss_gvrp_port_control_conf_set(l2port, enable);
#endif
    default:
        break;
    }
    return XXRP_ERROR_INVALID_APPL;
}

mesa_rc vtss_mrp_port_control_conf_get(vtss_mrp_appl_t appl, u32 l2port, BOOL *const status)
{
    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS_) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    if (!status) {
        return XXRP_ERROR_INVALID_PARAMETER;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, NOISE) << "Appl = " << appl
            << " is requesting status of port = " << l2port2str(l2port);
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    *status = (MRP_MAD_GET(appl, l2port)) ? TRUE : FALSE;

    return VTSS_RC_OK;
}

mesa_rc vtss_xxrp_port_control_conf_get(vtss_mrp_appl_t appl, u32 l2port, BOOL *const status)
{
    switch (appl) {
#ifdef VTSS_SW_OPTION_MVRP
    case VTSS_MRP_APPL_MVRP:
        return vtss_mrp_port_control_conf_get(appl, l2port, status);
#endif

#ifdef VTSS_SW_OPTION_GVRP
    case VTSS_GARP_APPL_GVRP:
        return vtss_gvrp_port_control_conf_get(l2port, status);
#endif
    default:
        break;
    }
    return XXRP_ERROR_INVALID_APPL;
}

mesa_rc vtss_mrp_map_get(vtss_mrp_appl_t appl, vtss_mrp_map_t ***map_ports)
{
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    *map_ports = MRP_MAP_BASE_GET(appl);

    return VTSS_RC_OK;
}

#ifdef VTSS_MRP_APPL_MVRP
mesa_rc vtss_mrp_port_ring_print(vtss_mrp_appl_t appl, u8 msti)
{
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    vtss_mrp_map_print_msti_ports(MRP_MAP_BASE_GET(appl), msti);

    return VTSS_RC_OK;
}
#endif

#ifdef VTSS_SW_OPTION_MVRP
/* This function should be called with lock */
static mesa_rc vtss_mrp_process_periodic_event(vtss_mrp_appl_t appl, u32 l2port, BOOL enable)
{
    vtss_mad_fsm_events       fsm_events;

    fsm_events.la_event = last_la_event;
    if (enable == TRUE) {
        fsm_events.periodic_event = enabled_periodic;
    } else {
        fsm_events.periodic_event = disabled_periodic;
    }
    fsm_events.reg_event = last_reg_event;
    fsm_events.appl_event = last_appl_event;
    MRP_CRIT_ASSERT();
    VTSS_RC(vtss_mrp_mad_process_events(appl, l2port, MADTT_INVALID_INDEX, &fsm_events));

    return VTSS_RC_OK;
}

static mesa_rc vtss_mrp_periodic_transmission_control_conf_set(vtss_mrp_appl_t appl, u32 l2port, BOOL enable)
{
    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS_) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Set periodic status for appl = " << appl
            << ", port = " << l2port2str(l2port)
            << ", enable = " << enable;
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }
    if (enable) {
        if ((MRP_MAD_GET(appl, l2port)->periodic_timer_control_status) == TRUE) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, NOISE) << "Periodic timer is already enabled on this port ";
            return VTSS_RC_OK;
        } else {
            MRP_MAD_GET(appl, l2port)->periodic_timer_control_status = TRUE;
            (void)vtss_mrp_process_periodic_event(appl, l2port, TRUE);
        }
    } else {
        if ((MRP_MAD_GET(appl, l2port)->periodic_timer_control_status) == TRUE) {
            MRP_MAD_GET(appl, l2port)->periodic_timer_control_status = FALSE;
            (void)vtss_mrp_process_periodic_event(appl, l2port, FALSE);
        } else {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, NOISE)  << "Periodic timer is already disabled on this port";
            return VTSS_RC_OK;
        }
    }

    return VTSS_RC_OK;
}
#endif

mesa_rc vtss_xxrp_periodic_transmission_control_conf_set(vtss_mrp_appl_t appl, u32 l2port, BOOL enable)
{
    switch (appl) {
#ifdef VTSS_SW_OPTION_MVRP
    case VTSS_MRP_APPL_MVRP:
        return vtss_mrp_periodic_transmission_control_conf_set(appl, l2port, enable);
#endif

#ifdef VTSS_SW_OPTION_GVRP
    case VTSS_GARP_APPL_GVRP:
        return VTSS_RC_OK;
#endif
    default:
        break;
    }
    return XXRP_ERROR_INVALID_APPL;
}

/* Not used at the moment - periodic feature not implemented yet - include it in vtss_xxwp_api.h when used */
#if 0
mesa_rc vtss_mrp_periodic_transmission_control_conf_get(vtss_mrp_appl_t appl, u32 l2port, BOOL *const status)
{
    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    if (!status) {
        return XXRP_ERROR_INVALID_PARAMETER;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Get periodic status for appl = " << appl
            << ", port = " << l2port2str(l2port);
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }
    *status = MRP_MAD_GET(appl, l2port)->periodic_timer_control_status;

    return VTSS_RC_OK;
}
#endif

#ifdef VTSS_SW_OPTION_MVRP
static mesa_rc vtss_mrp_timers_conf_set(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_timer_conf_t *const timers)
{
    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS_) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    if (!timers) {
        return XXRP_ERROR_NULL_PTR;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Set MRP timers for appl = " << appl
            << ", port = " << l2port2str(l2port);
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }
    MRP_MAD_GET(appl, l2port)->join_timeout = timers->join_timer;
    MRP_MAD_GET(appl, l2port)->leave_timeout = timers->leave_timer;
    MRP_MAD_GET(appl, l2port)->leaveall_timeout = timers->leave_all_timer;

    return VTSS_RC_OK;
}
#endif


mesa_rc vtss_xxrp_timers_conf_set(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_timer_conf_t *const timers)
{
    switch (appl) {
#ifdef VTSS_SW_OPTION_MVRP
    case VTSS_MRP_APPL_MVRP:
        return vtss_mrp_timers_conf_set(appl, l2port, timers);
#endif

#ifdef VTSS_SW_OPTION_GVRP
    case VTSS_GARP_APPL_GVRP:
        return 0;
#endif
    default:
        break;
    }
    return XXRP_ERROR_INVALID_APPL;
}

#if 0
#ifdef VTSS_SW_OPTION_MVRP
/* Not used at the moment */
static mesa_rc vtss_mrp_timers_conf_get(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_timer_conf_t *const timers)
{
    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    if (!timers) {
        return XXRP_ERROR_NULL_PTR;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "application = " << appl
            << ", port = " << l2port2str(l2port);
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }
    timers->join_timer = MRP_MAD_GET(appl, l2port)->join_timeout;
    timers->leave_timer = MRP_MAD_GET(appl, l2port)->leave_timeout;
    timers->leave_all_timer = MRP_MAD_GET(appl, l2port)->leaveall_timeout;

    return VTSS_RC_OK;
}
#endif

#ifdef VTSS_SW_OPTION_GVRP
/* Not used at the moment */
static mesa_rc vtss_gvrp_timers_conf_get(vtss_mrp_timer_conf_t *const timers)
{
    timers->join_timer = 0;
    timers->leave_timer = 0;
    timers->leave_all_timer = 0;

    return VTSS_RC_OK;
}
#endif

/* Not used at the moment, include in vtss_xxrp_api.h if used */
mesa_rc vtss_xxrp_timers_conf_get(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_timer_conf_t *const timers)
{
    switch (appl) {
#ifdef VTSS_SW_OPTION_MVRP
    case VTSS_MRP_APPL_MVRP:
        return vtss_mrp_timers_conf_get(appl, l2port, timers);
#endif

#ifdef VTSS_SW_OPTION_GVRP
    case VTSS_GARP_APPL_GVRP:
        return vtss_gvrp_timers_conf_get(timers);
#endif
    default:
        break;
    }
    return XXRP_ERROR_INVALID_APPL;
}
#endif

#ifdef VTSS_SW_OPTION_MVRP
static mesa_rc vtss_mrp_applicant_admin_control_conf_set(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_attribute_type_t attr_type, BOOL participant)
{
    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS_) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Set applicant status for application = "
            << appl << ", port = " << l2port2str(l2port)
            << ", participant = " << participant;
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }
#ifdef VTSS_SW_OPTION_MVRP
    if (appl == VTSS_MRP_APPL_MVRP) {
        if (attr_type.mvrp_attr_type != VTSS_MVRP_VLAN_ATTRIBUTE) {
            return XXRP_ERROR_INVALID_ATTR;
        }
    }
    MRP_MAD_GET(appl, l2port)->attr_type_admin_status[attr_type.mvrp_attr_type] = participant;
#endif

    return VTSS_RC_OK;
}
#endif

mesa_rc vtss_gvrp_applicant_admin_control_conf_set(u32 l2port, vtss_mrp_attribute_type_t attr_type, BOOL participant)
{
    return VTSS_RC_OK;
}

mesa_rc vtss_xxrp_applicant_admin_control_conf_set(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_attribute_type_t attr_type, BOOL participant)
{
    switch (appl) {
#ifdef VTSS_SW_OPTION_MVRP
    case VTSS_MRP_APPL_MVRP:
        return vtss_mrp_applicant_admin_control_conf_set(appl, l2port, attr_type, participant);
#endif

#ifdef VTSS_SW_OPTION_GVRP
    case VTSS_GARP_APPL_GVRP:
        return vtss_gvrp_applicant_admin_control_conf_set(l2port, attr_type, participant);
#endif
    default:
        break;
    }
    return XXRP_ERROR_INVALID_APPL;
}

/* Not used at the moment - include it in vtss_xxrp_api.h when used */
mesa_rc vtss_mrp_applicant_admin_control_conf_get(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_attribute_type_t attr_type, BOOL *const status)
{
    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS_) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    if (!status) {
        return XXRP_ERROR_NULL_PTR;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Get applicant status for application = "
            << appl << ", port = " << l2port2str(l2port);
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }
#ifdef VTSS_SW_OPTION_MVRP
    if (appl == VTSS_MRP_APPL_MVRP) {
        if (attr_type.mvrp_attr_type != VTSS_MVRP_VLAN_ATTRIBUTE) {
            return XXRP_ERROR_INVALID_ATTR;
        }
    }
    *status = MRP_MAD_GET(appl, l2port)->attr_type_admin_status[attr_type.mvrp_attr_type];
#endif

    return VTSS_RC_OK;
}

mesa_rc vtss_mrp_stats_get(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_stats_t *const stats)
{
    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS_) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    if (!stats) {
        return XXRP_ERROR_NULL_PTR;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Get MRP stats for application = "
            << appl << ", port = " << l2port2str(l2port);
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }
    *stats = MRP_MAD_GET(appl, l2port)->stats;

    return VTSS_RC_OK;
}

static mesa_rc vtss_mrp_rx_stats_set(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_stat_type_t type)
{
    vtss_mrp_mad_t *mad;

    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS_) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, RACKET) << "Set MRP rx stats for application = "
            << appl << ", port = " << l2port2str(l2port);
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    mad = MRP_MAD_GET(appl, l2port);
    if (!mad) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }
    switch (type) {
    case VTSS_MRP_RX_PKTS:
        mad->stats.pkts_rx++;
        break;
    case VTSS_MRP_RX_DROPPED_PKTS:
        mad->stats.pkts_dropped_rx++;
        break;
    case VTSS_MRP_RX_NEW:
        mad->stats.new_rx++;
        break;
    case VTSS_MRP_RX_JOININ:
        mad->stats.joinin_rx++;
        break;
    case VTSS_MRP_RX_IN:
        mad->stats.in_rx++;
        break;
    case VTSS_MRP_RX_JOINMT:
        mad->stats.joinmt_rx++;
        break;
    case VTSS_MRP_RX_MT:
        mad->stats.mt_rx++;
        break;
    case VTSS_MRP_RX_LV:
        mad->stats.leave_rx++;
        break;
    case VTSS_MRP_RX_LA:
        mad->stats.leaveall_rx++;
        break;
    default:
        break;
    }

    return VTSS_RC_OK;
}

static mesa_rc vtss_mrp_tx_stats_set(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_stat_type_t type)
{
    vtss_mrp_mad_t *mad;

    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS_) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, RACKET) << "Set MRP tx stats for application = "
            << appl << ", port = " << l2port2str(l2port);
    MRP_CRIT_ASSERT();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    mad = MRP_MAD_GET(appl, l2port);
    if (!mad) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }

    switch (type) {
    case VTSS_MRP_TX_PKTS:
        mad->stats.pkts_tx++;
        break;
    case VTSS_MRP_TX_NEW:
        mad->stats.new_tx++;
        break;
    case VTSS_MRP_TX_JOININ:
        mad->stats.joinin_tx++;
        break;
    case VTSS_MRP_TX_IN:
        mad->stats.in_tx++;
        break;
    case VTSS_MRP_TX_JOINMT:
        mad->stats.joinmt_tx++;
        break;
    case VTSS_MRP_TX_MT:
        mad->stats.mt_tx++;
        break;
    case VTSS_MRP_TX_LV:
        mad->stats.leave_tx++;
        break;
    case VTSS_MRP_TX_LA:
        mad->stats.leaveall_tx++;
        break;
    default:
        break;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_mrp_stats_clear(vtss_mrp_appl_t appl, u32 l2port)
{
    if ((appl >= VTSS_MRP_APPL_MAX) || (appl < 0)) {
        return XXRP_ERROR_INVALID_APPL;
    }
    if (l2port >= L2_MAX_PORTS_) {
        return XXRP_ERROR_INVALID_L2PORT;
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Clearing MRP stats for appl = "
            << appl << ", port = " << l2port2str(l2port);
    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }
    memset(&(MRP_MAD_GET(appl, l2port)->stats), 0, sizeof(vtss_mrp_stats_t));

    return VTSS_RC_OK;
}

void vtss_xxrp_update_rx_stats(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_stat_type_t type)
{
    mesa_rc rc;

    if ((rc = vtss_mrp_rx_stats_set(appl, l2port, type)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, WARNING) << error_txt(rc);
    }
}

void vtss_xxrp_update_tx_stats(vtss_mrp_appl_t appl, u32 l2port, u8 event)
{
    vtss_mrp_stat_type_t type = VTSS_MRP_STAT_MAX;
    mesa_rc              rc;

    switch (event) {
    case VTSS_XXRP_APPL_EVENT_NEW:
        type = VTSS_MRP_TX_NEW;
        break;
    case VTSS_XXRP_APPL_EVENT_JOININ:
        type = VTSS_MRP_TX_JOININ;
        break;
    case VTSS_XXRP_APPL_EVENT_IN:
        type = VTSS_MRP_TX_IN;
        break;
    case VTSS_XXRP_APPL_EVENT_JOINMT:
        type = VTSS_MRP_TX_JOINMT;
        break;
    case VTSS_XXRP_APPL_EVENT_MT:
        type = VTSS_MRP_TX_MT;
        break;
    case VTSS_XXRP_APPL_EVENT_LV:
        type = VTSS_MRP_TX_LV;
        break;
    case VTSS_XXRP_APPL_EVENT_LA:
        type = VTSS_MRP_TX_LA;
        break;
    case VTSS_XXRP_APPL_EVENT_TX_PKTS:
        type = VTSS_MRP_TX_PKTS;
        break;
    default:
        break;
    }
    if ((rc = vtss_mrp_tx_stats_set(appl, l2port, type)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, ERROR) << error_txt(rc);
    }
}

/* Not used at the moment */
BOOL vtss_mrp_is_attr_registered(vtss_mrp_appl_t appl, u32 l2port, u32 attr)
{
#ifdef VTSS_SW_OPTION_MVRP
    if (appl == VTSS_MRP_APPL_MVRP) {
        return vtss_mvrp_is_vlan_registered(l2port, attr);
    }
#endif
    return FALSE;
}

mesa_rc vtss_xxrp_mstp_port_state_change_handler(u32 l2port, u8 msti, vtss_mrp_mstp_port_state_change_type_t  port_state_type)
{
    mesa_rc rc1, rc2;

    rc1 = rc2 = VTSS_RC_OK;
#ifdef VTSS_SW_OPTION_MVRP
    rc1 = vtss::mrp::mvrp_handle_port_state_change(l2port, msti,
                                                   (vtss::mrp::port_states)port_state_type);
#endif

#ifdef VTSS_SW_OPTION_GVRP
    rc2 = vtss_gvrp_mstp_port_state_change_handler(l2port, msti, port_state_type);
#endif

    return rc1 ? rc1 : rc2;
}

#ifdef VTSS_SW_OPTION_MVRP
const char *mad_reg_state_to_txt(u8 state)
{
    switch (state) {
    case 0:
        return "IN";
    case 1:
        return "LV";
    case 2:
        return "MT";
    default:
        return "invalid";
    }
}

const char *mad_appl_state_to_txt(u8 state)
{
    switch (state) {
    case 0:
        return "VO";
    case 1:
        return "VP";
    case 2:
        return "VN";
    case 3:
        return "AN";
    case 4:
        return "AA";
    case 5:
        return "QA";
    case 6:
        return "LA";
    case 7:
        return "AO";
    case 8:
        return "QO";
    case 9:
        return "AP";
    case 10:
        return "QP";
    case 11:
        return "LO";
    default:
        return "invalid";

    }
}

const char *mad_la_or_periodic_state_to_txt(u8 state)
{
    switch (state) {
    case 0:
        return "active";
    case 1:
        return "passive";
    default:
        return "invalid";
    }
}

mesa_rc vtss_mrp_port_mad_print(u32 l2port, u32 machine_index)
{
    vtss_mrp_mad_t *mad;

    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(VTSS_MRP_APPL_MVRP)) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "MRP application is not enabled globally";
        return VTSS_RC_OK;
    }
    mad = MRP_MAD_GET(VTSS_MRP_APPL_MVRP, l2port);
    if (mad) {
        printf("%7s ", mad_appl_state_to_txt(mad->machines[machine_index].applicant));
        printf("%9s ", mad_reg_state_to_txt(mad->machines[machine_index].registrar));
        printf("%10s ", mad_la_or_periodic_state_to_txt(mad->leaveall_stm_current_state));
        printf("Periodic fsm state: %s\n", mad_la_or_periodic_state_to_txt(mad->periodic_stm_current_state));
        printf("Peer MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", mad->peer_mac_address[0], mad->peer_mac_address[1],
               mad->peer_mac_address[2], mad->peer_mac_address[3], mad->peer_mac_address[4], mad->peer_mac_address[5]);
    }
    return VTSS_RC_OK;
}
#endif

mesa_rc vtss_mrp_port_update_peer_mac_addr(vtss_mrp_appl_t appl, u32 l2port, u8 *mac_addr)
{
    vtss_mrp_mad_t *mad;

    MRP_CRIT_SCOPE();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    mad = MRP_MAD_GET(appl, l2port);
    if (mad) {
        memcpy(mad->peer_mac_address, mac_addr, sizeof(mad->peer_mac_address));
        mad->peer_mac_updated = TRUE;
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_mrp_port_mad_get(vtss_mrp_appl_t appl, u32 l2port, vtss_mrp_mad_t **mad)
{
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    *mad = MRP_MAD_GET(appl, l2port);

    return VTSS_RC_OK;
}

mesa_rc vtss_mrp_mad_process_events(vtss_mrp_appl_t appl, u32 l2port,
                                    u32 mad_fsm_indx, vtss_mad_fsm_events *fsm_events)
{
    vtss_mrp_mad_t *mad_port;
    mesa_rc        rc;

    /* Function is expected to be called with lock */
    MRP_CRIT_ASSERT();
    /* Retrieve the MAD pointer for the specific port and then call the respective event handler */
    if ((rc = vtss_mrp_port_mad_get(appl, l2port, &mad_port)) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Failed to retrieve MAD pointer of port " << l2port2str(l2port);
        return rc;
    }
#ifdef VTSS_MRP_APPL_MVRP
    rc = vtss_mrp_madtt_event_handler1(appl, mad_port, fsm_events);
    rc = vtss_mrp_madtt_event_handler2(appl, mad_port, mad_fsm_indx, fsm_events);
#endif
    return rc;
}

/* This function should be called with lock */
static mesa_rc vtss_mrp_propagate_join(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_indx)
{
    MRP_CRIT_ASSERT();
#ifdef VTSS_MRP_APPL_MVRP
    return vtss_mrp_map_propagate_join(appl, MRP_MAP_BASE_GET(appl), l2port, mad_fsm_indx);
#endif
    return VTSS_RC_OK;
}

/* This function should be called with lock */
static mesa_rc vtss_mrp_propagate_leave(vtss_mrp_appl_t appl, u32 l2port, u32 mad_indx)
{
    MRP_CRIT_ASSERT();
#ifdef VTSS_MRP_APPL_MVRP
    return vtss_mrp_map_propagate_leave(appl, MRP_MAP_BASE_GET(appl), l2port, mad_indx);
#endif
    return VTSS_RC_OK;
}

/* This function should be called with lock */
mesa_rc vtss_mrp_join_indication(vtss_mrp_appl_t appl, u32 l2port, u32 mad_indx, BOOL new_)
{
    mesa_rc rc;

    /* TODO: Check topology change and set the new accordingly */
    MRP_CRIT_ASSERT();
    MRP_APPL_GET(appl)->join_indication_fn(l2port, mad_indx, new_);
    rc = vtss_mrp_propagate_join(appl, l2port, mad_indx);
    return rc;
}

/* JGSD: Need to check the LOCK path */
mesa_rc vtss_mrp_leave_indication(vtss_mrp_appl_t appl, u32 l2port, u32 mad_indx)
{
    MRP_CRIT_ASSERT();
    MRP_APPL_GET(appl)->leave_indication_fn(l2port, mad_indx);
    VTSS_RC(vtss_mrp_propagate_leave(appl, l2port, mad_indx));

    return VTSS_RC_OK;
}

/* JGSD: Need to check the Lock path */
mesa_rc vtss_mrp_map_port_change_handler(vtss_mrp_appl_t appl, u32 l2port, BOOL add)
{
    mesa_rc                   rc = VTSS_RC_OK;
#ifdef VTSS_MRP_APPL_MVRP
    vtss_mrp_map_t            *map;
    u32                       machine_index, machine_index_cnt, tmp_port;
    u8                        msti;
    vtss_mad_fsm_events       fsm_events;
    BOOL                      another_port_has_registered = FALSE;
    u8                        tmp_state;

    MRP_CRIT_ASSERT();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }

    /* Only applicant state machines will be impacted */
    fsm_events.la_event = last_la_event;
    fsm_events.periodic_event = last_periodic_event;
    fsm_events.reg_event = last_reg_event;

    machine_index_cnt = (MRP_APPL_GET(appl)->max_mad_index - 1);
    if (add) { /* A port is added to the port set to participate in MRP protocol operation (MSTP or Port Add)*/
        fsm_events.appl_event = join_app;
        for (machine_index = 0; machine_index < (machine_index_cnt); machine_index++) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, RACKET) << "machine index = " <<  machine_index;
            /* For each registered atribure, join should be propagated on other ports in this msti */
            if (vtss_mrp_madtt_get_mad_registrar_state(MRP_MAD_GET(appl, l2port), machine_index) == IN) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, NOISE) << "Propagating join to the other ports of the msti. "
                        << "indx = " << machine_index
                        << ", port = " << l2port2str(l2port);
                rc += vtss_mrp_propagate_join(appl, l2port, machine_index);
            }
        }

        for (tmp_port = 0; tmp_port < L2_MAX_PORTS; tmp_port++) {
            for (machine_index = 0; machine_index < (machine_index_cnt); machine_index++) {
                if (MRP_MAD_GET(appl, tmp_port)) {
                    if (vtss_mrp_madtt_get_mad_registrar_state(MRP_MAD_GET(appl, tmp_port), machine_index) == IN) {
                        if (tmp_port != l2port) {
                            /* Check if port is part of this msti. If yes, propagate join on this port */
                            vtss_mrp_get_msti_from_mad_fsm_index(machine_index, &msti);
                            /* Send join request on this port */
                            if ((vtss_mrp_map_find_port(MRP_MAP_BASE_GET(appl), msti, l2port, &map)) == VTSS_RC_OK) {
                                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, NOISE) << "Propagating other ports' registered "
                                        << "attributes on this port. indx = "
                                        << machine_index << ", port = "
                                        << l2port2str(l2port);
                                rc += vtss_mrp_mad_process_events(appl, l2port, machine_index, &fsm_events);
                            }
                        }
                    }
                }
            }
        }
    } else {
        fsm_events.appl_event = lv_app;
        for (machine_index = 0; machine_index < (machine_index_cnt); machine_index++) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, RACKET) << "machine index = " << machine_index;
            /* For each registered atribure, leave should be propagated on other ports in this msti */
            tmp_state = vtss_mrp_madtt_get_mad_registrar_state(MRP_MAD_GET(appl, l2port), machine_index);
            if (tmp_state == IN) {
                another_port_has_registered = FALSE;
                for (tmp_port = 0; tmp_port < L2_MAX_PORTS; tmp_port++) {
                    if (MRP_MAD_GET(appl, tmp_port)) {
                        if (tmp_port != l2port) {
                            if (vtss_mrp_madtt_get_mad_registrar_state(MRP_MAD_GET(appl, tmp_port), machine_index) == IN) {
                                another_port_has_registered = TRUE;
                                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, NOISE) << "Propagate leave request to the other "
                                        << "registered ports. indx = " << machine_index
                                        << ", port = " << l2port2str(tmp_port);
                                /* Send leave request to tmp_port */
                                rc += vtss_mrp_mad_process_events(appl, tmp_port, machine_index, &fsm_events);
                            }
                        }
                    }
                }
                if (another_port_has_registered == FALSE) { /* Send leave request to all other ports in the msti */
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, NOISE) << "Propagating leave to other ports of the msti. "
                            << "indx = " << machine_index
                            << ", port = " << l2port2str(l2port);
                    rc += vtss_mrp_propagate_leave(appl, l2port, machine_index);
                }
            }
        }
    }
#endif
    return rc;
}

/* This function should be called with lock */
static mesa_rc vtss_mrp_handle_leaveall(vtss_mrp_appl_t appl, vtss_mrp_mad_t *mad)
{
#if 0
    vtss_mad_fsm_events       fsm_events;
    u32                       machine_index;
#endif

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "leaveall on port = " << l2port2str(mad->port_no);
    MRP_CRIT_ASSERT();
#if 0
    fsm_events.la_event = last_la_event;
    fsm_events.periodic_event = last_periodic_event;
    fsm_events.reg_event = rLA_reg;
    fsm_events.appl_event = rLA_app;
    for (machine_index = 0; machine_index < (MRP_APPL_GET(appl)->max_mad_index); machine_index++) {
        if (VTSS_XXRP_REG_ADMIN_STATUS_GET(mad, machine_index) == VLAN_REGISTRATION_TYPE_NORMAL) {
            //rc = vtss_mrp_mad_process_events(application,  mad->port_no, machine_index, &fsm_events);
        }
    }
#endif

    return VTSS_RC_OK;
}

/* JGSD: Triggered by state machine call - called with lock */
mesa_rc vtss_mrp_handle_periodic_timer(vtss_mrp_appl_t appl, u32 l2port)
{
    vtss_mad_fsm_events       fsm_events;
    u32                       machine_index;
    vtss_mrp_mad_t            *mad;
    mesa_rc                   rc = VTSS_RC_OK;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "handle periodic on port = " << l2port2str(l2port);
    MRP_CRIT_ASSERT();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    mad = MRP_MAD_GET(appl, l2port);
    if (mad) {
        fsm_events.la_event = last_la_event;
        fsm_events.periodic_event = last_periodic_event;
        fsm_events.reg_event = last_reg_event;
        fsm_events.appl_event = periodic_app;
        for (machine_index = 0; machine_index < (MRP_APPL_GET(appl)->max_mad_index); machine_index++) {
            rc += vtss_mrp_mad_process_events(appl, l2port, machine_index, &fsm_events);
        }
    }
    return rc;
}

mesa_rc vtss_mrp_process_leaveall(vtss_mrp_appl_t appl, u32 l2port)
{
    vtss_mad_fsm_events fsm_events;
    mesa_rc             rc = VTSS_RC_OK;

    fsm_events.la_event = rLA_la;
    fsm_events.periodic_event = last_periodic_event;
    fsm_events.reg_event = last_reg_event;
    fsm_events.appl_event = last_appl_event;
    MRP_CRIT_SCOPE();
    rc = vtss_mrp_mad_process_events(appl, l2port, MADTT_INVALID_INDEX, &fsm_events);
    rc += vtss_mrp_handle_leaveall(appl, MRP_MAD_GET(appl, l2port));

    return rc;
}

mesa_rc vtss_mrp_process_new_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index)
{
    vtss_mad_fsm_events fsm_events;
    mesa_rc             rc;

    fsm_events.la_event = last_la_event;
    fsm_events.periodic_event = last_periodic_event;
    fsm_events.reg_event = rNew_reg;
    fsm_events.appl_event = rNew_app;
    MRP_CRIT_SCOPE();
    rc = vtss_mrp_mad_process_events(appl, l2port, mad_fsm_index, &fsm_events);

    return rc;
}

mesa_rc vtss_mrp_process_joinin_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index)
{
    vtss_mad_fsm_events fsm_events;
    mesa_rc             rc;

    fsm_events.la_event = last_la_event;
    fsm_events.periodic_event = last_periodic_event;
    fsm_events.reg_event = rJoinIn_reg;
    fsm_events.appl_event = rJoinIn_app;
    MRP_CRIT_SCOPE();
    rc = vtss_mrp_mad_process_events(appl, l2port, mad_fsm_index, &fsm_events);

    return rc;
}

mesa_rc vtss_mrp_process_in_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index)
{
    vtss_mad_fsm_events fsm_events;
    mesa_rc             rc;

    fsm_events.la_event = last_la_event;
    fsm_events.periodic_event = last_periodic_event;
    fsm_events.reg_event = last_reg_event;
    fsm_events.appl_event = rIn_app;
    MRP_CRIT_SCOPE();
    rc = vtss_mrp_mad_process_events(appl, l2port, mad_fsm_index, &fsm_events);

    return rc;
}

mesa_rc vtss_mrp_process_joinmt_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index)
{
    vtss_mad_fsm_events fsm_events;
    mesa_rc             rc;

    fsm_events.la_event = last_la_event;
    fsm_events.periodic_event = last_periodic_event;
    fsm_events.reg_event = rJoinMt_reg;
    fsm_events.appl_event = rJoinMt_app;
    MRP_CRIT_SCOPE();
    rc = vtss_mrp_mad_process_events(appl, l2port, mad_fsm_index, &fsm_events);

    return rc;
}

mesa_rc vtss_mrp_process_mt_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index)
{
    vtss_mad_fsm_events fsm_events;
    mesa_rc             rc;

    fsm_events.la_event = last_la_event;
    fsm_events.periodic_event = last_periodic_event;
    fsm_events.reg_event = last_reg_event;
    fsm_events.appl_event = rMt_app;
    MRP_CRIT_SCOPE();
    rc = vtss_mrp_mad_process_events(appl, l2port, mad_fsm_index, &fsm_events);

    return rc;
}

mesa_rc vtss_mrp_process_lv_event(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index)
{
    vtss_mad_fsm_events fsm_events;
    mesa_rc             rc;

    fsm_events.la_event = last_la_event;
    fsm_events.periodic_event = last_periodic_event;
    fsm_events.reg_event = rLv_reg;
    fsm_events.appl_event = rLv_app;
    MRP_CRIT_SCOPE();
    rc = vtss_mrp_mad_process_events(appl, l2port, mad_fsm_index, &fsm_events);

    return rc;
}

BOOL vtss_mrp_mrpdu_rx(u32 l2port, const u8 *mrpdu, u32 length)
{
    vtss_tick_count_t start_time, end_time;
    xxrp_eth_hdr_t   *l2_hdr;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, NOISE) << "MRPDU rx, port = " << l2port2str(l2port) << ", len = " << length;
#ifdef VTSS_SW_OPTION_MVRP
    //xxrp_packet_dump(port_no, mrpdu, FALSE);
#endif
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, DEBUG) << "Start MRPDU rx processing";
    start_time = vtss_current_time();
    l2_hdr = (xxrp_eth_hdr_t *)mrpdu;

#if defined(VTSS_SW_OPTION_MVRP) || defined(VTSS_SW_OPTION_GVRP)
    /* Check whether the MRPDU is MVRP/GVRP PDU */
    if (!memcmp(l2_hdr->dst_mac, MRP_MVRP_MULTICAST_MACADDR, VTSS_XXRP_MAC_ADDR_LEN)) {
#ifdef VTSS_SW_OPTION_MVRP
        if (!memcmp(l2_hdr->eth_type, VTSS_MVRP_ETH_TYPE, VTSS_XXRP_ETH_TYPE_LEN)) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, DEBUG) << "Received MVRPDU on port " << l2port2str(l2port);
            (void)vtss::mrp::mvrp_receive_frame(l2port, mrpdu, length);
        }
#endif

#ifdef VTSS_SW_OPTION_GVRP
        // This must be a LLC packet, i.e. len/type<=1500 with DSAP=0x42, LSAP=0x42, Control=0x03
        if (xxrp_ntohs(l2_hdr->eth_type) <= 1500 && l2_hdr->dsap == 0x42 && l2_hdr->lsap == 0x42 && l2_hdr->control == 0x03) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, DEBUG) << "Received GVRPDU on port " << l2port2str(l2port);
            if (vtss_gvrp_rx_pdu(l2port, mrpdu, length)) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, WARNING) << "GVRPDU partially or not parsed";
            }
        }
#endif
    }
#endif

    end_time = vtss_current_time();
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, DEBUG) << "Time to process MRPDU rx = "
            << (u32)VTSS_OS_TICK2MSEC(end_time - start_time) << "ms";
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_RX, DEBUG) << "End of MRPDU rx processing";
    return TRUE;
}

#ifdef VTSS_SW_OPTION_MVRP
static inline void vtss_mrp_encode_sJ(vtss_mrp_mad_t *mad, u32 mad_fsm_index, u8 *all_attr_events)
{
    mesa_vid_t vid;

    mvrp_mad_fsm_index_to_vid(mad_fsm_index, &vid);
    if (vtss_mrp_madtt_get_mad_registrar_state(mad, mad_fsm_index) == IN) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, NOISE) << "Encoding JoinIn event for VID " << vid;
        VTSS_XXRP_SET_EVENT(all_attr_events, mad_fsm_index, VTSS_XXRP_APPL_EVENT_JOININ);
    } else {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, NOISE) << "Encoding joinMt event for VID " << vid;
        VTSS_XXRP_SET_EVENT(all_attr_events, mad_fsm_index, VTSS_XXRP_APPL_EVENT_JOINMT);
    }
}

static inline void vtss_mrp_encode_s(vtss_mrp_mad_t *mad, u32 mad_fsm_index, u8 *all_attr_events)
{
    mesa_vid_t vid;

    mvrp_mad_fsm_index_to_vid(mad_fsm_index, &vid);
    if (vtss_mrp_madtt_get_mad_registrar_state(mad, mad_fsm_index) == IN) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, NOISE) << "Encoding In event for VID " << vid;
        VTSS_XXRP_SET_EVENT(all_attr_events, mad_fsm_index, VTSS_XXRP_APPL_EVENT_IN);
    } else {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TX, RACKET) << "Encoding Mt event for VID " << vid;
        VTSS_XXRP_SET_EVENT(all_attr_events, mad_fsm_index, VTSS_XXRP_APPL_EVENT_MT);
    }
}

static inline void vtss_mrp_encode_sN(u32 mad_fsm_index, u8 *all_attr_events)
{
    VTSS_XXRP_SET_EVENT(all_attr_events, mad_fsm_index, VTSS_XXRP_APPL_EVENT_NEW);
}

static inline void vtss_mrp_encode_sL(u32 mad_fsm_index, u8 *all_attr_events)
{
    VTSS_XXRP_SET_EVENT(all_attr_events, mad_fsm_index, VTSS_XXRP_APPL_EVENT_LV);
}

static inline void vtss_mrp_encode_invalid(u32 mad_fsm_index, u8 *all_attr_events)
{
    VTSS_XXRP_SET_EVENT(all_attr_events, mad_fsm_index, VTSS_XXRP_APPL_EVENT_INVALID);
}
#endif

/* Called with lock */
mesa_rc vtss_mrp_reg_admin_status_set(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index, u8 t)
{
    MRP_CRIT_ASSERT();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }
    if (t) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Registrar administrative status of appl = "
                << appl << ", port = " << l2port2str(l2port)
                << " and index " << mad_fsm_index + 1
                << " is set to " << t;
    }
    VTSS_XXRP_REG_ADMIN_STATUS_CLEAR(MRP_MAD_GET(appl, l2port), mad_fsm_index);
    VTSS_XXRP_REG_ADMIN_STATUS_SET(MRP_MAD_GET(appl, l2port), mad_fsm_index, t);
    return VTSS_RC_OK;
}

/* Called with lock */
mesa_rc vtss_mrp_reg_admin_status_get(vtss_mrp_appl_t appl, u32 l2port, u32 mad_fsm_index, u8 *t)
{
    MRP_CRIT_ASSERT();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }
    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }
    *t = (u8)VTSS_XXRP_REG_ADMIN_STATUS_GET(MRP_MAD_GET(appl, l2port), mad_fsm_index);
    return VTSS_RC_OK;
}

/* called only if MVRP is enabled, but this ifdef is still needed */
#ifdef VTSS_SW_OPTION_MVRP
/* Called locked */
static mesa_rc vtss_mvrp_vlan_change_handler(u32 l2port, mesa_vid_t vid, vlan_registration_type_t t)
{
    u16     fsm_index;
    mesa_rc rc = VTSS_RC_OK;

    MRP_CRIT_ASSERT();
    mvrp_vid_to_mad_fsm_index(vid, &fsm_index);
    VTSS_XXRP_REG_ADMIN_STATUS_CLEAR(MRP_MAD_GET(VTSS_MRP_APPL_MVRP, l2port), fsm_index);
    VTSS_XXRP_REG_ADMIN_STATUS_SET(MRP_MAD_GET(VTSS_MRP_APPL_MVRP, l2port), fsm_index, t);
    /* The following check is just a precaution */
    if (VTSS_XXRP_REG_ADMIN_STATUS_GET(MRP_MAD_GET(VTSS_MRP_APPL_MVRP, l2port), fsm_index) != t) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, ERROR) << "Registrar administrative status was not set properly ";
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "Registrar administrative status changed to " << t;
    if (t == VLAN_REGISTRATION_TYPE_FIXED) {
        /* 0: Set the registrar state */
        vtss_mrp_madtt_set_mad_registrar_state(MRP_MAD_GET(VTSS_MRP_APPL_MVRP, l2port),
                                               fsm_index, IN);
        /* 1: Trigger Join! event on this port's applicant */
        /* 2: Propagate Join! event on the applicants of all other ports of the MSTI set */
        rc = vtss_mrp_propagate_join(VTSS_MRP_APPL_MVRP, l2port, fsm_index);
    } else {
        /* 0: Set the registrar state */
        vtss_mrp_madtt_set_mad_registrar_state(MRP_MAD_GET(VTSS_MRP_APPL_MVRP, l2port), fsm_index, MT);
        /* 1: Trigger Lv! event on this port's applicant */
        /* 2: Propagate Lv! event on the applicants of all other ports of the MSTI set */
        /*    only if this is last port to have the VID registered */
        if (1) {
            rc = vtss_mrp_propagate_leave(VTSS_MRP_APPL_MVRP, l2port, fsm_index);
        }
    }
    return rc;
}
#endif

/* Called locked */
mesa_rc vtss_mrp_vlan_change_handler(u32 l2port, mesa_vid_t vid, vlan_registration_type_t t)
{
#ifdef VTSS_SW_OPTION_MVRP
    vtss_mrp_appl_t appl;

    MRP_CRIT_ASSERT();
    appl = VTSS_MRP_APPL_MVRP;
    MRP_CRIT_ASSERT();
    if (!MRP_APPL_GET(appl)) {
        return XXRP_ERROR_NOT_ENABLED;
    }

    if (!MRP_MAD_GET(appl, l2port)) {
        return XXRP_ERROR_NOT_ENABLED_PORT;
    }

    return vtss_mvrp_vlan_change_handler(l2port, vid, t);
#else
    return VTSS_RC_OK;
#endif
}

#ifdef VTSS_SW_OPTION_MVRP
/* This function should be called with lock. */
static mesa_rc vtss_mrp_handle_leaveall_timer_expiry(vtss_mrp_appl_t appl, vtss_mrp_mad_t *mad)
{
    vtss_mad_fsm_events       fsm_events;
    mesa_rc                   rc = VTSS_RC_OK;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "leaveall timer expiry on port = " << l2port2str(mad->port_no);
    MRP_CRIT_ASSERT();
    fsm_events.la_event = leavealltimer_la;
    fsm_events.periodic_event = last_periodic_event;
    fsm_events.reg_event = last_reg_event;
    fsm_events.appl_event = last_appl_event;
    rc = vtss_mrp_mad_process_events(appl, mad->port_no, MADTT_INVALID_INDEX, &fsm_events);
    rc += vtss_mrp_handle_leaveall(appl, mad);

    return rc;
}

/* This function should be called with lock */
static mesa_rc vtss_mrp_handle_periodic_timer_expiry(vtss_mrp_appl_t appl, vtss_mrp_mad_t *mad)
{
    vtss_mad_fsm_events       fsm_events;
    mesa_rc                   rc = VTSS_RC_OK;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "periodic timer expiry on port = " << l2port2str(mad->port_no);
    MRP_CRIT_ASSERT();
    mad->periodic_timer_running = FALSE;
    fsm_events.la_event = last_la_event;
    fsm_events.periodic_event = periodictimer_periodic;
    fsm_events.reg_event = last_reg_event;
    fsm_events.appl_event = last_appl_event;
    rc = vtss_mrp_mad_process_events(appl, mad->port_no, MADTT_INVALID_INDEX, &fsm_events);

    return rc;
}

/* This function should be called with lock */
static mesa_rc vtss_mrp_handle_leave_timer_expiry(vtss_mrp_appl_t appl, vtss_mrp_mad_t *mad, u16 mad_fsm_index)
{
    mesa_rc rc = VTSS_RC_OK;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_DEFAULT, DEBUG) << "leave timer expiry on port = " << l2port2str(mad->port_no);
    MRP_CRIT_ASSERT();
    VTSS_XXRP_STOP_LEAVE_TIMER(mad, mad_fsm_index);
    vtss_mrp_madtt_set_mad_registrar_state(mad, mad_fsm_index, MT);
    /* leave_indication */
    rc = vtss_mrp_leave_indication(appl, mad->port_no, mad_fsm_index);

    return rc;
}

static inline void _update_min_time(u32 *min_time, u32 ttime)
{
    if (ttime != 0) {
        if (*min_time == 0) {
            *min_time = ttime;
        } else {
            if (*min_time > ttime) {
                *min_time = ttime;
            }
        }
    }
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Updated min_time to " << *min_time;
}

static mesa_rc vtss_mrp_handle_mad_timers(vtss_mrp_appl_t appl, vtss_mrp_mad_t *mad, u32 delay, u32 *port_min_time)
{
    u32                       mad_fsm_index, min_time = 0, total_events = 0;
    mesa_rc                   rc = VTSS_RC_OK;
    u8                        appl_state;
    u8                        la_state = last_la_state;
    u16                       vid;
    /* each attribute event is stored in 4 bits */
    u8                        all_attr_events[XXRP_MAX_ATTR_EVENTS_ARR];
    BOOL                      join_timer_expired = FALSE, transmit_pdu = FALSE;
    vtss_mad_fsm_events       fsm_events;

    MRP_CRIT_ASSERT();
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "MAD timers handler for appl = "
            << appl << " and port = " << l2port2str(mad->port_no);
    memset(all_attr_events, VTSS_XXRP_APPL_EVENT_INVALID_BYTE, sizeof(all_attr_events));

    if (mad->leaveall_timer_running) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "LeaveAll timer running";
        if (mad->leaveall_timer_kick == FALSE) {
            mad->leaveall_timer_count -= delay;
        } else {
            mad->leaveall_timer_kick = FALSE;
        }
        if (mad->leaveall_timer_count <= 0) { /* LA timer expiry */
            rc += vtss_mrp_handle_leaveall_timer_expiry(appl, mad);
        }
        /* mad->leaveall_timer_count will be updated to mad->leaveall_timeout after LA expiry */
        _update_min_time(&min_time, mad->leaveall_timer_count);
    }

    /* Check if join timer has expired */
    if (mad->join_timer_running) {
        /* Update join timer and check for expiry */
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Join timer running";
        if (mad->join_timer_kick == FALSE) {
            mad->join_timer_count -= delay;
        } else {
            mad->join_timer_kick = FALSE;
        }
        if (mad->join_timer_count <= 0) {
            join_timer_expired = TRUE;
            /* Join timer is stopped */
            mad->join_timer_running = FALSE;
            transmit_pdu = TRUE;
            /* TODO: For now txLAF! is not handled as MVRP doesn't require it */
            /* Check if LA timer has also expired by checking LA state (active_la) */
            la_state = vtss_mrp_madtt_get_mad_la_state(mad);
            if (la_state == active_la) {
                vtss_xxrp_start_leaveall_timer(mad, FALSE);
            }
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "la_state = " << la_state;
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Triggering tx opportunity event to LA FSM";
            /* When a join timer expires (it means tx opportunity), send tx! event to LA state machine */
            fsm_events.la_event = tx_la;
            fsm_events.periodic_event = last_periodic_event;
            fsm_events.reg_event = last_reg_event;
            fsm_events.appl_event = last_appl_event;
            rc += vtss_mrp_mad_process_events(appl, mad->port_no, MADTT_INVALID_INDEX, &fsm_events);
        } else {
            _update_min_time(&min_time, mad->join_timer_count);
        }
    }

    if (mad->periodic_timer_running) {
        /* Update periodic timer and check for expiry */
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Periodic timer running";
        if (mad->periodic_timer_kick == FALSE) {
            mad->periodic_timer_count -= delay;
        } else {
            mad->periodic_timer_kick = FALSE;
        }
        if (mad->periodic_timer_count <= 0) {
            rc += vtss_mrp_handle_periodic_timer_expiry(appl, mad);
        } else {
            _update_min_time(&min_time, mad->periodic_timer_count);
        }
    }

    for (mad_fsm_index = 0; mad_fsm_index < MRP_APPL_GET(appl)->max_mad_index; mad_fsm_index++) {
        if (VTSS_XXRP_IS_LEAVE_TIMER_RUNNING(mad, mad_fsm_index)) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Leave timer running";
            if (!VTSS_XXRP_IS_LEAVE_TIMER_KICK(mad, mad_fsm_index)) {
                mad->leave_timer_count[mad_fsm_index] -= delay;
            } else {
                VTSS_XXRP_CLEAR_LEAVE_TIMER_KICK(mad, mad_fsm_index);
            }
            if (mad->leave_timer_count[mad_fsm_index] <= 0) {
                rc += vtss_mrp_handle_leave_timer_expiry(appl, mad, mad_fsm_index);
            } else {
                _update_min_time(&min_time, mad->leave_timer_count[mad_fsm_index]);
            }
        }
        if (!join_timer_expired) {
            continue;
        }
        appl_state = vtss_mrp_madtt_get_mad_applicant_state(mad, mad_fsm_index);
        mvrp_mad_fsm_index_to_vid(mad_fsm_index, &vid);
        switch (appl_state) {
        case VN:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Applicant in VN state";
            total_events++;
            vtss_mrp_encode_sN(mad_fsm_index, all_attr_events);
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                    << " changed state from VN to AN";
            mad->machines[mad_fsm_index].applicant = AN;
            vtss_xxrp_start_join_timer(mad);
            break;
        case AN:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Applicant in AN state";
            total_events++;
            vtss_mrp_encode_sN(mad_fsm_index, all_attr_events);
            if (la_state == active_la) { /* txLA! */
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                        << " changed state from AN to QA";
                mad->machines[mad_fsm_index].applicant = QA;
            } else { /* tx! */
                if (vtss_mrp_madtt_get_mad_registrar_state(mad, mad_fsm_index) == IN) {
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                            << " changed state from AN to QA";
                    mad->machines[mad_fsm_index].applicant = QA;
                } else {
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                            << " changed state from AN to AA";
                    mad->machines[mad_fsm_index].applicant = AA;
                    vtss_xxrp_start_join_timer(mad);
                }
            }
            break;
        case LA:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Applicant in LA state";
            if (la_state == active_la) { /* txLA! */
                vtss_mrp_encode_invalid(mad_fsm_index, all_attr_events);
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                        << " changed state from LA to LO";
                mad->machines[mad_fsm_index].applicant = LO;
                vtss_xxrp_start_join_timer(mad);
            } else { /* tx! */
                total_events++;
                vtss_mrp_encode_sL(mad_fsm_index, all_attr_events);
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                        << " changed state from LA to VO";
                mad->machines[mad_fsm_index].applicant = VO;
            }
            break;
        case VP:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Applicant in VP state";
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                    << " changed state from VP to AA";
            mad->machines[mad_fsm_index].applicant = AA;
            vtss_xxrp_start_join_timer(mad);
            total_events++;
            if (la_state == active_la) { /* txLA! */
                vtss_mrp_encode_s(mad, mad_fsm_index, all_attr_events);
            } else { /* tx! */
                vtss_mrp_encode_sJ(mad, mad_fsm_index, all_attr_events);
            }
            break;
        case AP:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Applicant in AP state";
            total_events++;
            vtss_mrp_encode_sJ(mad, mad_fsm_index, all_attr_events);
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                    << " changed state from AP to QA";
            mad->machines[mad_fsm_index].applicant = QA;
            break;
        case AA:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Aplicant in AA state";
            total_events++;
            vtss_mrp_encode_sJ(mad, mad_fsm_index, all_attr_events);
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                    << " changed state from AA to QA";
            mad->machines[mad_fsm_index].applicant = QA;
            if (la_state == active_la) { /* txLA! */
            } else { /* tx! */
            }
            break;
        case LO:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Applicant in LO state";
            if (la_state == active_la) { /* txLA! */
                /* No state change */
                vtss_mrp_encode_invalid(mad_fsm_index, all_attr_events);
            } else { /* tx! */
                total_events++;
                vtss_mrp_encode_s(mad, mad_fsm_index, all_attr_events);
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, RACKET) << "Applicant of VID " << vid
                        << " changed state from LO to VO";
                mad->machines[mad_fsm_index].applicant = VO;
            }
            break;
        case VO:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Applicant in VO state";
            vtss_mrp_encode_invalid(mad_fsm_index, all_attr_events);
            if (la_state == active_la) { /* txLA! */
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, RACKET) << "Applicant of VID " << vid
                        << " changed state from VO to LO";
                mad->machines[mad_fsm_index].applicant = LO;
                vtss_xxrp_start_join_timer(mad);
            } else { /* tx! */
                /* No state change */
            }
            break;
        case QA:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Applicant in QA state";
            /* No state change */
            if (la_state == active_la) { /* txLA! */
                total_events++;
                vtss_mrp_encode_sJ(mad, mad_fsm_index, all_attr_events);
            } else { /* tx! */
                vtss_mrp_encode_invalid(mad_fsm_index, all_attr_events);
            }
            break;
        case AO:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Applicant in AO state";
            vtss_mrp_encode_invalid(mad_fsm_index, all_attr_events);
            if (la_state == active_la) { /* txLA! */
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                        << " changed state from AO to LO";
                mad->machines[mad_fsm_index].applicant = LO;
                vtss_xxrp_start_join_timer(mad);
            } else { /* tx! */
                /* No state change */
            }
            break;
        case QO:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Applicant in QO state";
            vtss_mrp_encode_invalid(mad_fsm_index, all_attr_events);
            if (la_state == active_la) { /* txLA! */
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                        << " changed state from QO to LO";
                mad->machines[mad_fsm_index].applicant = LO;
                vtss_xxrp_start_join_timer(mad);
            } else { /* tx! */
                /* No state change */
            }
            break;
        case QP:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, RACKET) << "Applicant in QP state";
            if (la_state == active_la) { /* txLA! */
                total_events++;
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_FSM, NOISE) << "Applicant of VID " << vid
                        << " changed state from QP to QA";
                mad->machines[mad_fsm_index].applicant = QA;
                vtss_mrp_encode_sJ(mad, mad_fsm_index, all_attr_events);
            } else { /* tx! */
                /* No state change */
                vtss_mrp_encode_invalid(mad_fsm_index, all_attr_events);
            }
            break;
        default:
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, ERROR) << "Invalid applicant state";
            break;
        }
    }
    if (transmit_pdu) {
        if (la_state == active_la) {
            (void)MRP_APPL_GET(appl)->transmit_fn(mad->port_no, all_attr_events, total_events, TRUE);
        } else {
            (void)MRP_APPL_GET(appl)->transmit_fn(mad->port_no, all_attr_events, total_events, FALSE);
        }
    }
    *port_min_time = min_time;

    return rc;
}

static u32 vtss_mrp_timer_tick(u32 delay)
{
    vtss_mrp_appl_t  appl;
    vtss_mrp_mad_t   *mad;
    u32              l2port, min_time = 0, port_min_time = 0;
    mesa_rc          rc = VTSS_RC_OK;
    vtss_tick_count_t start_time, end_time;
    BOOL             tmp_status;

    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Tick delay = " << delay;
    start_time = vtss_current_time();
    for (appl = (vtss_mrp_appl_t)0; appl < VTSS_MRP_APPL_MAX; appl++) {
        MRP_CRIT_SCOPE();
        tmp_status = MRP_APPL_GET(appl) ? TRUE : FALSE;
        if (tmp_status) { /* This means globally enabled */
            for (l2port = 0; l2port < L2_MAX_PORTS_; l2port++) {
                if ((mad = MRP_MAD_GET(appl, l2port))) { /* This means MRP application is enabled on this port */
                    /* TODO: Check port status such as NAS etc */
                    rc += vtss_mrp_handle_mad_timers(appl, mad, delay, &port_min_time);
                    if (rc != VTSS_RC_OK) {
                        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, DEBUG) << "Error while processing MAD timers";
                    }
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "port_min_time = " <<  port_min_time;
                    _update_min_time(&min_time, port_min_time);
                }
            }
        }
    }
    end_time = vtss_current_time();
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "Time to execute timer tick = "
            << (u32)VTSS_OS_TICK2MSEC(end_time - start_time) << "ms";
    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_TIMER, NOISE) << "min_time = " << min_time;
    return VTSS_OS_MSEC2TICK(min_time * 10);
}
#endif

u32 vtss_xxrp_timer_tick(u32 delay)
{
    BOOL app_status = FALSE;

#ifdef VTSS_SW_OPTION_MVRP
    {
        MRP_CRIT_SCOPE();
        app_status = MRP_APPL_GET(VTSS_MRP_APPL_MVRP) ? TRUE : FALSE;
    }
    if (app_status) {
        return vtss_mrp_timer_tick(delay);
    }
#endif

#ifdef VTSS_SW_OPTION_GVRP
    {
        MRP_CRIT_SCOPE();
        app_status = MRP_APPL_GET(VTSS_GARP_APPL_GVRP) ? TRUE : FALSE;
    }
    if (app_status) {
        return vtss_gvrp_timer_tick(delay);
    }
#endif
    // If no application is enabled, return 0 to stop the timer
    return 0;
}

/* MRP initialization function */
void vtss_mrp_init(void)
{
    MRP_CRIT_SCOPE();
    memset(&glb_mrp, 0, sizeof(glb_mrp));
}
