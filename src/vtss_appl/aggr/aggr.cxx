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

#include "main.h"
#include "vtss/appl/port.h"
#include "port_api.h" // For port_count_max()
#include "port_iter.hxx"

#if defined(VTSS_SW_OPTION_DOT1X)
#include "topo_api.h"
#endif
#include "icli_api.h"

#include "msg_api.h"
#include "misc_api.h"

#include "conf_api.h"
#ifdef VTSS_SW_OPTION_PACKET
#include "packet_api.h"
#endif
#include "critd_api.h"

#ifdef VTSS_SW_OPTION_LACP
#include "l2proto_api.h"
#include "lacp_api.h"
#ifndef _VTSS_LACP_OS_H_
#include "vtss_lacp_os.h"
#endif
#endif /* VTSS_SW_OPTION_LACP */
#if defined(VTSS_SW_OPTION_DOT1X)
#include "dot1x_api.h"
#endif /* VTSS_SW_OPTION_DOT1X */
#include "aggr_api.h"
#include "aggr.h"
#include "vtss/basics/expose/table-status.hxx" // For vtss::expose::TableStatus
#include "vtss/basics/memcmp-operator.hxx"  // For VTSS_BASICS_MEMCMP_OPERATOR

#ifdef VTSS_SW_OPTION_ICLI
#include "aggr_icfg.h"
#endif
#include "vtss/appl/nas.h" // For NAS management functions

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_AGGR

/*
  Synopsis.
  The aggregation module controls static and dynamic (LACP) created aggregations.

  VSTAX_V1:  EOL!
  VSTAX_V2:  Stack configuration as supported by JR.
  STANDALONE:  Standalone configuration as supported by L26 and JR/JR-48 standalone. Only LLAGs

  Configuration.
  The static aggregations are saved to flash while only the LACP configuration is saved to ram and lost after reset.
  If a portlink goes down that port is removed from a dynamic created aggregation (at chiplevel) while the statically
  created will be left untouched (the portmask will be modified by the port-module).

  Aggregation id's.
  VSTAX_V2 (JR): No LLAGs. GLAGs numbered in the same way as AGGR_MGMT_GROUP_NO_START - AGGR_MGMT_GROUP_NO_END.
  All aggregations are identified by AGGR_MGMT_GROUP_NO_START - AGGR_MGMT_GROUP_NO_END.
  The aggregations are kept in one common pool for both static and dynamic usage, i.e. each could occupie all groups.

  Dynamic LLAGs and GLAGs.
  VSTAX_V2  (JR):  Dynamic aggregation are based on 32 GLAGs.

  Limited HW resources.
  The HW limits GLAG to hold max 8 ports while LLAG can hold 16.

  Aggregation map.
  The core of the LACP does not know anything about stacking.  All it's requests are based on aggregation id (aid) and a port number (l2_port).
  It's aid's spans the same range as number of ports, i.e. per default all ports are member of it's own aid.

  Crosshecks.
  The status of Dynamic and static aggregation are checked before they are created.
  This applies both to port member list as well as the aggr id.
  Furthermore dot1x may not coexist with an aggregation and is also checked before an aggregation
  is created.

  Callbacks.
  Some switch features are depended on aggregation status, e.g. STP.  Therefore they can be notified when an aggregation changes
  by use of callback subscription.  See aggr_api for details.

*/

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/
static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "aggr", "AGGR"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define AGGR_CRIT_ENTER()    critd_enter(&aggrglb.aggr_crit,    __FILE__, __LINE__)
#define AGGR_CRIT_EXIT()     critd_exit( &aggrglb.aggr_crit,    __FILE__, __LINE__)
#define AGGR_CB_CRIT_ENTER() critd_enter(&aggrglb.aggr_cb_crit, __FILE__, __LINE__)
#define AGGR_CB_CRIT_EXIT()  critd_exit( &aggrglb.aggr_cb_crit, __FILE__, __LINE__)

#define AGGRFLAG_ABORT_WAIT         (1 << 0)
#define AGGRFLAG_COULD_NOT_TX       (1 << 1)
#define AGGRFLAG_WAIT_DONE          (1 << 2)

/* Global structure */
static aggr_global_t aggrglb;

/* JSON notifications  */
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_aggr_group_status_t);
vtss::expose::TableStatus <
vtss::expose::ParamKey<vtss_ifindex_t>,
     vtss::expose::ParamVal<vtss_appl_aggr_group_status_t *>
     > aggr_status_update("aggr_status_update", VTSS_MODULE_ID_AGGR);

static mesa_rc vtss_aggregation_status_get_current(vtss_ifindex_t ifindex, vtss_appl_aggr_group_status_t *const status);
static mesa_rc aggr_mgmt_port_state_set(vtss_isid_t isid, mesa_port_no_t port_no, BOOL enable);
/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/
inline static void aggr_status_update_priv(vtss_isid_t isid, int aggr_no)
{
    vtss_appl_aggr_group_status_t status;
    vtss_ifindex_t           ifindex;
    if (vtss_ifindex_from_llag(isid, aggr_no, &ifindex) == VTSS_RC_OK) {
        memset(&status, 0, sizeof(vtss_appl_aggr_group_status_t));
        if (vtss_aggregation_status_get_current(ifindex, &status) != VTSS_RC_OK) {
            aggr_status_update.del(ifindex);
        } else {
            aggr_status_update.set(ifindex, &status);
        }
    } else {
        T_D("failed to form ifindex from l/g lag");
    }
}

/* Callback function if aggregation changes */
static void aggr_change_callback(vtss_isid_t isid, int aggr_no)
{
    u32 i;

    T_D("ISID:%d Aggr %d changed", isid, aggr_no);

    if (aggr_no == VTSS_AGGR_NO_NONE) {
        T_W("Invalid aggregation group number");
        return;
    }

    aggr_status_update_priv(isid, aggr_no);

    AGGR_CB_CRIT_ENTER();
    for (i = 0; i < aggrglb.aggr_callbacks; i++) {
        aggrglb.callback_list[i](isid, aggr_no);
    }
    AGGR_CB_CRIT_EXIT();
}

/****************************************************************************/
/****************************************************************************/
static vtss_isid_t get_local_isid(void)
{
    vtss_isid_t      isid;

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        if (!msg_switch_exists(isid)) {
            continue;
        }
        if (msg_switch_is_local(isid)) {
            break;
        }
    }
    return isid;
}

/****************************************************************************/
/********************* Number of aggregated ports in a group ****************/
/****************************************************************************/
static u32 total_active_ports(mesa_glag_no_t aggr_no)
{
    vtss_isid_t       isid;
    mesa_port_no_t    p;
    u32               members = 0;

    AGGR_CRIT_ENTER();
    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        for (p = VTSS_PORT_NO_START; p < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); p++) {
            if (aggrglb.active_aggr_ports[isid][p] == aggr_no) {
                members++;
            }
        }
    }
    AGGR_CRIT_EXIT();
    return members;
}

/****************************************************************************/
/********************* Returns the L2 FWD state of the port  ****************/
/****************************************************************************/
static BOOL get_lacp_glb_port_state(vtss_isid_t isid, mesa_port_no_t port_no)
{
    BOOL state;
    AGGR_CRIT_ENTER();
    state =  aggrglb.port_state_fwd[isid][port_no];
    AGGR_CRIT_EXIT();
    return state;
}

/****************************************************************************/
/********************* Sets (locally) the L2 FWD state of the port  *********/
/****************************************************************************/
static void set_lacp_glb_port_state(vtss_isid_t isid, mesa_port_no_t port_no, BOOL state)
{
    AGGR_CRIT_ENTER();
    aggrglb.port_state_fwd[isid][port_no] = state;
    AGGR_CRIT_EXIT();
}


/****************************************************************************/
/************* Returns the LACP group of the port (or VTSS_AGGR_NO_NONE) ****/
/****************************************************************************/
static mesa_aggr_no_t get_lacp_glb_config(vtss_isid_t isid, mesa_port_no_t port_no)
{
    mesa_aggr_no_t aggr;
    AGGR_CRIT_ENTER();
    aggr = aggrglb.lacp_config_ports[isid][port_no];
    AGGR_CRIT_EXIT();
    return aggr;
}

/****************************************************************************/
/************* Sets the LACP group of the port (or VTSS_AGGR_NO_NONE) *******/
/****************************************************************************/
static void set_lacp_glb_config(vtss_isid_t isid, mesa_port_no_t port_no, mesa_aggr_no_t aggr)
{
    AGGR_CRIT_ENTER();
    aggrglb.lacp_config_ports[isid][port_no] = aggr;
    AGGR_CRIT_EXIT();
}

/****************************************************************************/
/* Gets the configured aggregation group of the port (or VTSS_AGGR_NO_NONE) */
/****************************************************************************/
static mesa_aggr_no_t get_aggrglb_config(vtss_isid_t isid, mesa_port_no_t port_no)
{
    mesa_aggr_no_t aggr;
    AGGR_CRIT_ENTER();
    aggr = aggrglb.config_aggr_ports[isid][port_no];
    AGGR_CRIT_EXIT();
    return aggr;
}

/****************************************************************************/
/** Sets the configured aggregation group of the port (or VTSS_AGGR_NO_NONE)*/
/****************************************************************************/
static void set_aggrglb_config(vtss_isid_t isid, mesa_port_no_t port_no, mesa_aggr_no_t aggr)
{
    AGGR_CRIT_ENTER();
    aggrglb.config_aggr_ports[isid][port_no] = aggr;
    AGGR_CRIT_EXIT();
}

/****************************************************************************/
/* Gets the active aggregation group of the port (or VTSS_AGGR_NO_NONE) *****/
/****************************************************************************/
static mesa_aggr_no_t get_aggrglb_active(vtss_isid_t isid, mesa_port_no_t port_no)
{
    mesa_aggr_no_t aggr;
    AGGR_CRIT_ENTER();
    aggr = aggrglb.active_aggr_ports[isid][port_no];
    AGGR_CRIT_EXIT();
    return aggr;
}

/****************************************************************************/
/* Sets the active aggregation group of the port (or VTSS_AGGR_NO_NONE) *****/
/****************************************************************************/
static void set_aggrglb_active(vtss_isid_t isid, mesa_port_no_t port_no, mesa_aggr_no_t aggr)
{
    AGGR_CRIT_ENTER();
    aggrglb.active_aggr_ports[isid][port_no] = aggr;
    AGGR_CRIT_EXIT();
}

/****************************************************************************/
/****************************************************************************/
static mesa_port_speed_t get_aggrglb_spd(mesa_aggr_no_t aggr)
{
    mesa_port_speed_t spd;
    AGGR_CRIT_ENTER();
    spd = aggrglb.aggr_group_speed[VTSS_ISID_LOCAL][aggr];
    AGGR_CRIT_EXIT();
    return spd;
}

/****************************************************************************/
/****************************************************************************/
static void set_aggrglb_spd(mesa_aggr_no_t aggr, mesa_port_speed_t spd)
{
    AGGR_CRIT_ENTER();
    aggrglb.aggr_group_speed[VTSS_ISID_LOCAL][aggr] = spd;
    AGGR_CRIT_EXIT();
}

/****************************************************************************/
/****************************************************************************/
static BOOL get_port_link(mesa_port_no_t port_no)
{
    AGGR_CRIT_ENTER();
    BOOL s = aggrglb.port_link[port_no];
    AGGR_CRIT_EXIT();
    return s;
}

/****************************************************************************/
/****************************************************************************/
static void set_port_link(mesa_port_no_t port_no, BOOL state)
{
    AGGR_CRIT_ENTER();
    aggrglb.port_link[port_no] = state;
    AGGR_CRIT_EXIT();
}

/****************************************************************************/
/****************************************************************************/
static mesa_rc aggr_add(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_t  *members)
{
    mesa_port_no_t p;
    AGGR_CRIT_ENTER();
    for (p = VTSS_PORT_NO_START; p < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); p++) {
        if (aggrglb.config_aggr_ports[isid][p] == aggr_no) {
            aggrglb.active_aggr_ports[isid][p] = members->member[p] ? aggr_no : VTSS_AGGR_NO_NONE;
        }
    }
    AGGR_CRIT_EXIT();
    return mesa_aggr_port_members_set(NULL, aggr_no - AGGR_MGMT_GROUP_NO_START, &members->member);
}

/****************************************************************************/
/****************************************************************************/
static void aggr_port_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    vtss_isid_t             isid = VTSS_ISID_START;
    mesa_aggr_no_t          aggr_no = get_aggrglb_config(isid, port_no);
    mesa_port_no_t          p;
    BOOL                    active_changed = 0;
    mesa_port_speed_t       spd;
    aggr_mgmt_group_t       grp;
    vtss_isid_t             isid_tmp;
    vtss_ifindex_t          ifindex;
    vtss_appl_port_status_t ps;

    /* This function is called if a port state change occurs.
       This could mean that a aggr configured port is getting activated or kicked out of a group.
       Inactive ports (ports that does not have the group speed or HDX) must be blocked by e.g. STP */

    /* Store the link for later use */
    set_port_link(port_no, status->link);

    if (aggr_no == VTSS_AGGR_NO_NONE) {
        return;
    }

    if (get_lacp_glb_config(isid, port_no) != VTSS_AGGR_NO_NONE) {
        /* LACP -> no action needed */
        return;
    }

    T_D("Port:%u. aggr_no:%d", port_no, aggr_no);
    AGGR_CRIT_ENTER();
    for (p = VTSS_PORT_NO_START; p < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); p++) {
        grp.member[p] = aggrglb.active_aggr_ports[isid][p] == aggr_no ? 1 : 0;
    }
    AGGR_CRIT_EXIT();

    spd = get_aggrglb_spd(aggr_no);
    if (status->link) {
        if (!status->fdx) {
            /* HDX ports not allowed in an aggregation */
            T_D("Port:%u. HDX ports not allowed - removing port from aggregation", port_no);
            grp.member[port_no] = 0;
        } else if (spd == MESA_SPEED_UNDEFINED) {
            T_D("Port:%u. Speed is undefined", port_no);
            (void)set_aggrglb_spd(aggr_no, status->speed);
            for (isid_tmp = VTSS_ISID_START; isid_tmp < VTSS_ISID_END; isid_tmp++) {
                vtss_clear(grp);
                for (p = VTSS_PORT_NO_START; p < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); p++) {
                    if (get_aggrglb_config(isid_tmp, p) == aggr_no) {
                        (void)vtss_ifindex_from_port(isid_tmp, p, &ifindex);
                        if (vtss_appl_port_status_get(ifindex, &ps) == VTSS_RC_OK) {
                            if (ps.link && ps.fdx && (ps.speed == status->speed)) {
                                grp.member[p] = 1;
                                active_changed = 1;
                            }
                        }
                    }
                }
                if (active_changed) {
                    T_D("Port is added to new speed group");
                    if (aggr_add(isid_tmp, aggr_no, &grp) != VTSS_RC_OK) {
                        T_E("Could not update AGGR group:%u", aggr_no);
                    }
                    (void)aggr_change_callback(isid_tmp, aggr_no);
                    active_changed = 0;
                }
            }
            return;
        } else if (status->speed != spd) {
            grp.member[port_no] = 0;
        } else {
            grp.member[port_no] = 1;
        }
    } else {
        grp.member[port_no] = 0;
    }
    active_changed = ((get_aggrglb_active(isid, port_no) == aggr_no && !grp.member[port_no])
                      || (get_aggrglb_active(isid, port_no) != aggr_no && grp.member[port_no]));
    if (active_changed) {
        T_D("Port:%u is active status is changed", port_no);
        if (aggr_add(isid, aggr_no, &grp) != VTSS_RC_OK) {
            T_E("Could not update AGGR group:%u", aggr_no);
        }
        if (total_active_ports(aggr_no) == 0) {
            T_D("No active members left in aggr:%d, i.e. the speed is undef", aggr_no);
            /* No active members left, i.e. the speed is undef */
            (void)set_aggrglb_spd(aggr_no, MESA_SPEED_UNDEFINED);
            /* Check other ports in the group */
            for (isid_tmp = VTSS_ISID_START; isid_tmp < VTSS_ISID_END; isid_tmp++) {
                active_changed = 0;
                vtss_clear(grp);
                for (p = VTSS_PORT_NO_START; p < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); p++) {
                    if (get_aggrglb_config(isid_tmp, p) == aggr_no) {
                        (void)vtss_ifindex_from_port(isid_tmp, p, &ifindex);
                        if (vtss_appl_port_status_get(ifindex, &ps) == VTSS_RC_OK) {
                            if (ps.link && ps.fdx && (get_aggrglb_spd(aggr_no) == MESA_SPEED_UNDEFINED)) {
                                (void)set_aggrglb_spd(aggr_no, ps.speed);
                            }
                            if (ps.link && ps.fdx && (get_aggrglb_spd(aggr_no) == ps.speed)) {
                                grp.member[p] = 1;
                                active_changed = 1;
                            }
                        }
                    }
                }
                if (active_changed) {
                    if (aggr_add(isid_tmp, aggr_no, &grp) != VTSS_RC_OK) {
                        T_E("Could not update AGGR group:%u", aggr_no);
                    }
                    (void)aggr_change_callback(isid_tmp, aggr_no);
                }
            }
            return;
        }
        (void)aggr_change_callback(isid, aggr_no);
    }
}


/****************************************************************************/
// Set aggregation mode in chip API
/****************************************************************************/
static mesa_rc aggr_mode_set(mesa_aggr_mode_t *mode)
{
    T_D("Setting aggr mode:smac:%d, dmac:%d, IP:%d, port:%d.\n", mode->smac_enable,
        mode->dmac_enable, mode->sip_dip_enable, mode->sport_dport_enable);

    return mesa_aggr_mode_set(NULL, mode);
}

/****************************************************************************/
/* Sets the aggregation mode.  The mode is used by all the aggregation groups */
/****************************************************************************/
static mesa_rc aggr_local_mode_set(mesa_aggr_mode_t *mode)
{
    mesa_rc rc;

    /* Add to chip */
    if ((rc = aggr_mode_set(mode)) != VTSS_RC_OK) {
        return rc;
    }
    /* Add to local config */
    AGGR_CRIT_ENTER();
    aggrglb.aggr_config.mode = *mode;
    AGGR_CRIT_EXIT();
    return VTSS_RC_OK;
}

/****************************************************************************/
/*  Stack/switch functions                                                  */
/****************************************************************************/

static mesa_rc aggr_group_add(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_t *members)
{
    mesa_port_no_t          port_no;
    vtss_ifindex_t          ifindex;
    vtss_appl_port_status_t ps;
    u32                     port_count = port_count_max();
    mesa_port_speed_t       spd = MESA_SPEED_UNDEFINED;
    aggr_mgmt_group_t       *m, loc_members = *members;

    m = &loc_members;
    T_D("Adding new group %d ", aggr_no);
    /* In case of mixed port speeds find the highest */
    if (get_aggrglb_spd(aggr_no) == MESA_SPEED_UNDEFINED) {
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            if (m->member[port_no]) {
                VTSS_RC(vtss_ifindex_from_port(isid, port_no, &ifindex));
                if (vtss_appl_port_status_get(ifindex, &ps) == VTSS_RC_OK) {
                    if (ps.link && ps.fdx) {
                        if (ps.speed > spd) {
                            spd = ps.speed;
                        }
                    }
                }
            }
        }
        T_D("Setting speed of group %d to %d ", aggr_no, spd);
        (void)set_aggrglb_spd(aggr_no, spd);
    }
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if (get_aggrglb_config(isid, port_no) == aggr_no) {
            (void)set_aggrglb_config(isid, port_no, VTSS_AGGR_NO_NONE);
            (void)set_aggrglb_active(isid, port_no, VTSS_AGGR_NO_NONE);
        }
        if (m->member[port_no]) {
            (void)set_aggrglb_config(isid, port_no, aggr_no);
            VTSS_RC(vtss_ifindex_from_port(isid, port_no, &ifindex));
            if (vtss_appl_port_status_get(ifindex, &ps) != VTSS_RC_OK) {
                T_E("Could not get port info");
                return VTSS_UNSPECIFIED_ERROR;
            }
            /* Activate ports with the same speed and FDX, others remain inactive until the changes spd/dplx  */
            if (ps.fdx && ps.link && (get_aggrglb_spd(aggr_no) == ps.speed)) {
                m->member[port_no] = 1;
                (void)set_aggrglb_active(isid, port_no, aggr_no);
            } else {
                m->member[port_no] = 0;
            }
        }
    }
    /* JR/L26 Standalones - Apply to chip */
    return mesa_aggr_port_members_set(NULL, aggr_no - AGGR_MGMT_GROUP_NO_START, &m->member);
}

static mesa_rc aggr_del(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no)
{
    mesa_port_no_t    port_no;
    u32               port_count = port_count_max();
    aggr_mgmt_group_t m;

    vtss_clear(m);
    T_D("Deleting group %d ", aggr_no);
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if (get_aggrglb_config(isid, port_no) == aggr_no) {
            (void)set_aggrglb_config(isid, port_no, VTSS_AGGR_NO_NONE);
            (void)set_aggrglb_active(isid, port_no, VTSS_AGGR_NO_NONE);
        }
    }

    /* JR/L26 Standalones */
    return mesa_aggr_port_members_set(NULL, aggr_no - AGGR_MGMT_GROUP_NO_START, &m.member);
}

/****************************************************************************/
// Remove an aggregation group in a switch in the stack
/****************************************************************************/
static void aggr_group_del(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no)
{
    aggr_mgmt_group_no_t group_no, aggr_mgmt_group_end = aggr_mgmt_group_no_end();

    if (aggr_no == AGGR_ALL_GROUPS) {
        for (group_no = AGGR_MGMT_GROUP_NO_START; group_no < aggr_mgmt_group_end; group_no++) {
            if (aggr_del(isid, group_no) != VTSS_RC_OK) {
                T_W("Could not del GLAG group");
            }
            if (total_active_ports(group_no) == 0) {
                (void)set_aggrglb_spd(group_no, MESA_SPEED_UNDEFINED);
            }
        }
    } else {
        if (aggr_del(isid, aggr_no) != VTSS_RC_OK) {
            T_W("Could not del group:%u", aggr_no);
        }

        if (total_active_ports(aggr_no) == 0) {
            (void)set_aggrglb_spd(aggr_no, MESA_SPEED_UNDEFINED);
        }
    }
}

/****************************************************************************/
// Set the aggr mode in a switch in the stack
/****************************************************************************/
static void aggr_stack_mode_set(vtss_isid_t isid, mesa_aggr_mode_t *mode)
{
    aggr_msg_mode_set_req_t *msg;

    if (msg_switch_is_local(isid)) {
        /* As this is the local swith we can take a shortcut  */

        if (aggr_local_mode_set(mode) != VTSS_RC_OK) {
            T_E("(aggr_stack_mode_set) Could not set mode");
        }
    } else {
        msg = (aggr_msg_mode_set_req_t *)VTSS_MALLOC(sizeof(*msg));
        if (msg) {
            msg->msg_id = AGGR_MSG_ID_MODE_SET_REQ;
            msg->mode = *mode;
            msg_tx(VTSS_MODULE_ID_AGGR, isid, msg, sizeof(*msg));
        }
    }
}

/****************************************************************************/
// Add configuration to the new switch in the stack
/****************************************************************************/
static void aggr_switch_conf_set(vtss_isid_t isid)
{
    mesa_aggr_mode_t     mode;

    if (!msg_switch_exists(isid)) {
        return;
    }

    /* Set the aggregation Mode  */
    AGGR_CRIT_ENTER();
    mode = aggrglb.aggr_config_stack.mode;
    AGGR_CRIT_EXIT();
    aggr_stack_mode_set(isid, &mode);


    vtss_ifindex_t ifindex;
    vtss_appl_aggr_group_conf_t conf;
    u32 i;
    for (i = AGGR_MGMT_GROUP_NO_START; i < AGGR_MGMT_GROUP_NO_END_; i++) {
        AGGR_CRIT_ENTER();
        conf = aggrglb.group_mode[i];
        memset(&aggrglb.group_mode[i], 0, sizeof(vtss_appl_aggr_group_conf_t));
        AGGR_CRIT_EXIT();

        if (vtss_ifindex_from_llag(isid, i, &ifindex) != VTSS_RC_OK) {
            T_E("vtss_ifindex_from_llag failed");
        }
        if (conf.mode !=  VTSS_APPL_AGGR_GROUP_MODE_DISABLED) {
            if (vtss_appl_aggregation_group_set(ifindex, &conf) != VTSS_RC_OK) {
                T_E("Could not apply config.\n");
            }
        }
    }
}

#ifdef VTSS_SW_OPTION_LACP
/****************************************************************************/
/****************************************************************************/
static BOOL aggr_group_exists(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_t *members)
{
    l2_port_no_t       l2port;
    BOOL               found_group = 0;
    mesa_port_no_t     port_no = 0;
    u32                aid, port_count = port_count_max();
    AGGR_CRIT_ENTER();
    for (aid = 0; aid < (VTSS_LACP_MAX_PORTS_ + VTSS_PORT_NO_START); aid++) {
        if (aggrglb.aggr_lacp[aid].aggr_no == aggr_no) {
            for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
                l2port = L2PORT2PORT(isid, port_no);
                if ((members->member[port_no] = aggrglb.aggr_lacp[aid].members[l2port]) == 1) {
                    found_group = 1;
                }
            }
        }
    }
    AGGR_CRIT_EXIT();
    return found_group;
}
#endif
/****************************************************************************/
/****************************************************************************/
static mesa_rc aggr_isid_port_valid(vtss_isid_t isid, mesa_port_no_t port_no, aggr_mgmt_group_no_t aggr_no, BOOL set)
{
    if (!AGGR_MGMT_GROUP_IS_AGGR(aggr_no)) {
        T_W("%u is not a legal aggregation group.", aggr_no);
        return AGGR_ERROR_INVALID_ID;
    }

    if (aggr_no >= aggr_mgmt_group_no_end()) {
        return AGGR_ERROR_ENTRY_NOT_FOUND;
    }

    if (isid == VTSS_ISID_LOCAL) {
        if (set) {
            T_W("SET not allowed, isid: %d", isid);
            return AGGR_ERROR_GEN;
        }
    } else if (!msg_switch_is_primary()) {
        T_W("Not ready");
        return AGGR_ERROR_NOT_READY;
    } else if (!VTSS_ISID_LEGAL(isid)) {
        T_W("Illegal isid %d", isid);
        return AGGR_ERROR_INVALID_ISID;
    } else if (!msg_switch_configurable(isid)) {
        T_W("Switch not configurable %d", isid);
        return AGGR_ERROR_INVALID_ISID;
    }

    if (port_no >= port_count_max()) {
        T_W("Illegal port_no %u", port_no);
        return AGGR_ERROR_INVALID_PORT;
    }

    return VTSS_RC_OK;
}

/****************************************************************************/
/****************************************************************************/
static BOOL verify_max_members(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_t *members)
{
    mesa_port_no_t  p;
    u32             port_count = port_count_max(),  new_members = 0;

    for (p = VTSS_PORT_NO_START; p < port_count; p++) {
        if (members->member[p]) {
            new_members++;
        }
    }

    if (new_members > AGGR_MGMT_LAG_PORTS_MAX_) {
        return 0;
    }
    return 1;
}
/****************************************************************************/
/****************************************************************************/
#ifdef VTSS_SW_OPTION_LACP
mesa_rc check_for_lacp(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_t *members)
{
    vtss_lacp_port_config_t  conf;
    mesa_port_no_t           port_no;
    aggr_mgmt_group_member_t members_tmp;
    u32                      port_count = port_count_max();
    int                      aid;
    mesa_rc                  rc;

    /* Check if the ports are occupied by LACP */
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if (members->member[port_no]) {
            if ((rc = lacp_mgmt_port_conf_get(isid, port_no, &conf)) != VTSS_RC_OK) {
                return rc;
            }
            if (conf.enable_lacp) {
                return AGGR_ERROR_LACP_ENABLED;
            }
        }
    }

    /* Check if the group is occupied by LACP */
    if (AGGR_MGMT_GROUP_IS_LAG(aggr_no)) {
        if (aggr_mgmt_lacp_members_get(isid, aggr_no,  &members_tmp, 0) == VTSS_RC_OK) {
            return AGGR_ERROR_GROUP_IN_USE;
        }
    } else {
        AGGR_CRIT_ENTER();
        for (aid = VTSS_PORT_NO_START; aid < (VTSS_LACP_MAX_PORTS_ + VTSS_PORT_NO_START); aid++) {
            if (aggrglb.aggr_lacp[aid].aggr_no < AGGR_MGMT_GROUP_NO_START) {
                continue;
            }
            if (aggrglb.aggr_lacp[aid].aggr_no == aggr_no) {
                AGGR_CRIT_EXIT();
                return AGGR_ERROR_GROUP_IN_USE;
            }
        }
        AGGR_CRIT_EXIT();
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_SW_OPTION_LACP */

// Enum-serializing definitions for vtss_appl_aggr_mode_t
vtss_enum_descriptor_t vtss_appl_aggr_mode_txt[] {
    {VTSS_APPL_AGGR_GROUP_MODE_DISABLED,     "disabled"},
    {VTSS_APPL_AGGR_GROUP_MODE_RESERVED,     "reserved"},
    {VTSS_APPL_AGGR_GROUP_MODE_STATIC_ON,    "static"},
    {VTSS_APPL_AGGR_GROUP_MODE_LACP_ACTIVE,  "lacpActive"},
    {VTSS_APPL_AGGR_GROUP_MODE_LACP_PASSIVE, "lacpPassive"},
    {0, 0},
};

/****************************************************************************/
/****************************************************************************/
/* static mesa_rc aggr_return_error(mesa_rc rc, vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no) */
/* { */
/*     (void)set_aggrglb_spd(aggr_no, MESA_SPEED_UNDEFINED); */
/*     return rc; */
/* } */

/****************************************************************************/
// Removes all members from a aggregation group, but doesn't save to flash.
/****************************************************************************/
static mesa_rc aggr_group_del_no_flash(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no)
{
    uint i;

    T_D("Doing a aggr del group %u at switch isid:%d", aggr_no, isid);

#ifdef VTSS_SW_OPTION_LACP
    aggr_mgmt_group_member_t members_tmp;
    /* Check if the group or ports is occupied by LACP in this isid */
    if (aggr_mgmt_lacp_members_get(isid, aggr_no, &members_tmp, 0) == VTSS_RC_OK) {
        return AGGR_ERROR_GROUP_IN_USE;
    }
#endif /* VTSS_SW_OPTION_LACP */


    AGGR_CRIT_ENTER();
    /* Delete from stack config */
    for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
        VTSS_BF_SET(aggrglb.aggr_config_stack.switch_id[isid - VTSS_ISID_START][aggr_no - AGGR_MGMT_GROUP_NO_START].member, i, 0);
        if (aggr_no == aggrglb.config_aggr_ports[isid][i]) {
            aggrglb.config_aggr_ports[isid][i] = VTSS_AGGR_NO_NONE;
            aggrglb.active_aggr_ports[isid][i] = VTSS_AGGR_NO_NONE;
        }
    }
    AGGR_CRIT_EXIT();

    /* Inform subscribers of aggregation changes */
    (void)aggr_change_callback(isid, aggr_no);

    /* Remove the group */
    (void)aggr_group_del(isid, aggr_no);

    return VTSS_RC_OK;
}

/****************************************************************************/
// Read/create and activate aggregation configuration
/****************************************************************************/
static void aggr_conf_stack_default(BOOL force_defaults)
{
    u32                      i, aggr_no, group;
    aggr_mgmt_group_member_t members;
    mesa_aggr_mode_t         mode;
    switch_iter_t            sit;
    u32                      aid;
    vtss_isid_t              isid;
    mesa_port_no_t           port_no;
    aggr_mgmt_group_no_t     aggr_mgmt_group_end = aggr_mgmt_group_no_end();

    T_I("Enter, force defaults: %d", force_defaults);

#ifdef VTSS_SW_OPTION_LACP
    /* Only reset the aggrglb.aggr_lacp stucture when becoming the primary switch */
    /* When 'System Restore Default' is issued then LACP will delete */
    /* all of its members and notify subscribers.  */
    if (!force_defaults) {
        // Clear LACP runtime state
        for (aid = 0; aid < (VTSS_LACP_MAX_PORTS_ + VTSS_PORT_NO_START); aid++) {
            vtss_clear(aggrglb.aggr_lacp[aid]);
        }
    }
#endif /* VTSS_SW_OPTION_LACP */

    T_I("Defaulting aggr module");
    AGGR_CRIT_ENTER();
    // Enable default aggregation code contributions
    aggrglb.aggr_config_stack.mode.smac_enable = 1;
    aggrglb.aggr_config_stack.mode.dmac_enable = 0;
    aggrglb.aggr_config_stack.mode.sip_dip_enable = 1;
    aggrglb.aggr_config_stack.mode.sport_dport_enable = 1;
    mode = aggrglb.aggr_config_stack.mode;
    AGGR_CRIT_EXIT();

    // First run through existing aggregations and delete them gracefully
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        // Only existing switches when we get here, and only as primary switch.
        group = 0;
        while (aggr_mgmt_port_members_get(sit.isid, group, &members, TRUE) == VTSS_RC_OK) {
            group = members.aggr_no;
            // aggr_group_del_no_flash() also informs subscribers of aggreation changes
            if (aggr_group_del_no_flash(sit.isid, group) != VTSS_RC_OK) {
                T_W("Could not delete group %d at isid %d", group, sit.isid);
            }
        }

        // Better safe than sorry
        (void)aggr_group_del(sit.isid, AGGR_ALL_GROUPS);

        // Also set the default aggregation mode
        aggr_stack_mode_set(sit.isid, &mode);
    }

    // Clear configuration for *all* switches
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_ALL);
    while (switch_iter_getnext(&sit)) {
        /* Reset the local memory configuration */
        AGGR_CRIT_ENTER();
        for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < aggr_mgmt_group_end; aggr_no++) {
            for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); i++) {
                VTSS_BF_SET(aggrglb.aggr_config_stack.switch_id[sit.isid - VTSS_ISID_START][aggr_no - AGGR_MGMT_GROUP_NO_START].member, i, 0);
            }
        }
        AGGR_CRIT_EXIT();
    }

    // Enabling port forwarding state in case of port has it disabled
    for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
        if (get_lacp_glb_config(VTSS_ISID_START, port_no) != VTSS_AGGR_NO_NONE) {
            if (get_lacp_glb_port_state(VTSS_ISID_START, port_no) == FALSE) {
                if (aggr_mgmt_port_state_set(VTSS_ISID_START, port_no, TRUE) != VTSS_RC_OK) {
                    T_E("Could not set port %d state", port_no);
                }
            }
        }
    }

    // Clear all (but LACP) runtime state, whether or not this is icfg_loading_pre restore to defaults.
    for (isid = 0; isid < VTSS_ISID_END; isid++) {
        for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < aggr_mgmt_group_end; aggr_no++) {
            aggrglb.aggr_group_speed[isid][aggr_no] = MESA_SPEED_UNDEFINED;
        }
        for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            aggrglb.active_aggr_ports[isid][port_no] = VTSS_AGGR_NO_NONE;
            aggrglb.config_aggr_ports[isid][port_no] = VTSS_AGGR_NO_NONE;
            aggrglb.lacp_config_ports[isid][port_no] = VTSS_AGGR_NO_NONE;
        }
    }
    for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END_; aggr_no++) {
        memset(&aggrglb.group_mode[aggr_no], 0, sizeof(vtss_appl_aggr_group_conf_t));
    }

    T_D("exit");
}

/****************************************************************************/
/****************************************************************************/
static void aggr_start(BOOL init)
{
    aggr_mgmt_group_no_t aggr;
    vtss_isid_t          isid;
    mesa_port_no_t       port_no;

    if (init) {
        /* Initialize global area */
        vtss_clear(aggrglb);
        aggrglb.aggr_config.mode.smac_enable = 1;
        aggrglb.aggr_config.mode.dmac_enable = 0;
        aggrglb.aggr_config.mode.sip_dip_enable = 1;
        aggrglb.aggr_config.mode.sport_dport_enable = 1;

        /* Initialize message buffers */
        vtss_sem_init(&aggrglb.request.sem, 1);

        /* Reset the switch aggr conf */
        for (isid = 0; isid < VTSS_ISID_END; isid++) {
            for (aggr = AGGR_MGMT_GROUP_NO_START; aggr < AGGR_MGMT_GLAG_END_; aggr++) {
                aggrglb.aggr_group_speed[isid][aggr] = MESA_SPEED_UNDEFINED;
            }

            for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
                aggrglb.active_aggr_ports[isid][port_no] = VTSS_AGGR_NO_NONE;
                aggrglb.config_aggr_ports[isid][port_no] = VTSS_AGGR_NO_NONE;
            }
        }

        /* Open up switch API after initialization */
        critd_init(&aggrglb.aggr_crit,    "aggr",    VTSS_MODULE_ID_AGGR, CRITD_TYPE_MUTEX);
        critd_init(&aggrglb.aggr_cb_crit, "aggr_cb", VTSS_MODULE_ID_AGGR, CRITD_TYPE_MUTEX);

        aggrglb.aggr_group_cnt_end = AGGR_MGMT_GLAG_END_;
        if (port_count_max() < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
            aggrglb.aggr_group_cnt_end = port_count_max() / 2 + AGGR_MGMT_GROUP_NO_START;
        }
    } else {
        for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            aggrglb.port_glag_member[port_no] = VTSS_GLAG_NO_NONE;
            aggrglb.port_aggr_member[port_no] = VTSS_AGGR_NO_NONE;
//            aggrglb.port_active_aggr_member[port_no] = VTSS_AGGR_NO_NONE;
        }

        /* Register for Port link state changes */
        (void)port_change_register(VTSS_MODULE_ID_AGGR, aggr_port_state_change_callback);
    }
}

void aggr_mgmt_dump(aggr_dbg_printf_t dbg_printf)
{
    aggr_mgmt_group_no_t aggr_mgmt_group_end = aggr_mgmt_group_no_end();
    dbg_printf("fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT):%d \nVTSS_AGGRS:%d \nAGGR_LLAG_CNT:%d \nAGGR_GLAG_CNT:%d \nAGGR_MGMT_LAG_PORTS_MAX:%d \nAGGR_MGMT_GLAG_PORTS_MAX:%d\n",
               fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), AGGR_LLAG_CNT_, AGGR_LLAG_CNT_, AGGR_GLAG_CNT, AGGR_MGMT_LAG_PORTS_MAX_, AGGR_MGMT_GLAG_PORTS_MAX);

    dbg_printf("AGGR_MGMT_GROUP_NO_START:%d  \naggr_mgmt_group_end:%d \nAGGR_MGMT_GROUP_NO_END:%d\n",
               AGGR_MGMT_GROUP_NO_START, aggr_mgmt_group_end, AGGR_MGMT_GROUP_NO_END_);


    dbg_printf("LLAGs:\n");

    aggr_mgmt_group_no_t aggr_no;
    vtss_isid_t isid;
    mesa_port_no_t port_no;
    mesa_bool_t state;
    port_vol_conf_t vol;

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            if (get_aggrglb_config(isid, port_no) != VTSS_AGGR_NO_NONE) {
                (void)dbg_printf("ISID:%d Port:%u is configured aggr:%u  Active:%s LACP:%s\n", isid, port_no,
                                 get_aggrglb_config(isid, port_no),
                                 get_aggrglb_active(isid, port_no) != VTSS_AGGR_NO_NONE ? "Yes" : "No",
                                 get_lacp_glb_config(isid, port_no) != VTSS_AGGR_NO_NONE ? "Yes" : "No");
            }
        }
    }

    for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < aggr_mgmt_group_end; aggr_no++) {
        if (get_aggrglb_spd(aggr_no) != MESA_SPEED_UNDEFINED) {
            (void)dbg_printf("Group:%u speed:%d\n", aggr_no, get_aggrglb_spd(aggr_no));
        }
    }

    for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
        for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            if (get_lacp_glb_config(isid, port_no) != VTSS_AGGR_NO_NONE) {
                mesa_port_state_get(NULL, port_no, &state);
                (void)port_vol_conf_get(PORT_USER_AGGR, port_no, &vol);
                (void)dbg_printf("Port:%u is configured in LACP group:%u, HW active:%s, Aggr state:%s, HW state:%s Oper_down:%s\n",
                                 port_no,
                                 get_lacp_glb_config(isid, port_no),
                                 get_aggrglb_active(isid, port_no) != VTSS_AGGR_NO_NONE ? "Yes" : "No",
                                 get_lacp_glb_port_state(isid, port_no) ? "FWD" : "BLK",
                                 state ? "FWD" : "BLK",
                                 vol.oper_down ? "Yes" : "No");
            }
        }
    }
}

/****************************************************************************/
/*  Management/API functions                                                */
/****************************************************************************/

/****************************************************************************/
/****************************************************************************/
const char *aggr_error_txt(mesa_rc rc)
{
    switch (rc) {
    case AGGR_ERROR_GEN:
        return "Aggregation generic error";
    case AGGR_ERROR_PARM:
        return "Illegal parameter";
    case AGGR_ERROR_REG_TABLE_FULL:
        return "Registration table full";
    case AGGR_ERROR_REQ_TIMEOUT:
        return "Timeout on message request";
    case AGGR_ERROR_STACK_STATE:
        return "Illegal PRIMARY/SECONDARY switch state";
    case AGGR_ERROR_GROUP_IN_USE:
        return "Group already in use";
    case AGGR_ERROR_PORT_IN_GROUP:
        return "Port already in another group";
    case AGGR_ERROR_LACP_ENABLED:
        return "LACP aggregation is enabled";
    case AGGR_ERROR_DOT1X_ENABLED:
        return "DOT1X is enabled";
    case AGGR_ERROR_ENTRY_NOT_FOUND:
        return "Entry not found";
    case AGGR_ERROR_HDX_SPEED_ERROR:
        return "Illegal duplex or speed state";
    case AGGR_ERROR_MEMBER_OVERFLOW:
        return "To many port members";
    case AGGR_ERROR_INVALID_ID:
        return "Invalid group id";
    case AGGR_ERROR_INVALID_MODE:
        return "At least one hash code must be chosen";
    default:
        return "Aggregation unknown error";
    }
}

/* portlist_* are defined in vtss_appl/mac/mac.c file */
BOOL portlist_index_get(u32 i, const vtss_port_list_stackable_t *pl);
BOOL portlist_index_set(u32 i, vtss_port_list_stackable_t *pl);
BOOL portlist_index_clear(u32 i, vtss_port_list_stackable_t *pl);
u32 isid_port_to_index(vtss_isid_t i,  mesa_port_no_t p);

BOOL portlist_state_get(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_port_list_stackable_t *pl)
{
    return portlist_index_get(isid_port_to_index(isid, port_no), pl);
}

BOOL portlist_state_set(vtss_isid_t isid, mesa_port_no_t port_no, vtss_port_list_stackable_t *pl)
{
    return portlist_index_set(isid_port_to_index(isid, port_no), pl);
}

BOOL portlist_state_clr(vtss_isid_t isid, mesa_port_no_t port_no, vtss_port_list_stackable_t *pl)
{
    return portlist_index_clear(isid_port_to_index(isid, port_no), pl);
}

mesa_rc validate_aggr_index(vtss_ifindex_t ifindex, vtss_ifindex_elm_t *ifep)
{
    T_N("Input>> key: %u", VTSS_IFINDEX_PRINTF_ARG(ifindex));
    VTSS_RC(vtss_ifindex_decompose(ifindex, ifep));
    T_N("Output>> isid:%d, usid:%d, ordinal:%d, iftype:%d", ifep->isid, ifep->usid, ifep->ordinal, ifep->iftype);

    if ((ifep->iftype != VTSS_IFINDEX_TYPE_LLAG) ||
        (!AGGR_MGMT_GROUP_IS_LAG(ifep->ordinal))) {
        T_W("%u is not a legal aggregation group (or type: %d).", ifep->ordinal, ifep->iftype);
        return AGGR_ERROR_INVALID_ID;
    }

    if (!VTSS_ISID_LEGAL(ifep->isid)) {
        return AGGR_ERROR_INVALID_ISID;
    }
    return VTSS_RC_OK;
}

static mesa_rc aggr_private_port_members_add(vtss_isid_t isid,
                                             aggr_mgmt_group_no_t aggr_no,
                                             const vtss_appl_aggr_group_member_t *const members,
                                             BOOL *had_mem)
{
    mesa_rc                     rc = VTSS_RC_ERROR;
    port_iter_t                 pit;
    aggr_mgmt_group_member_t    mem;

    if (!had_mem) {
        return VTSS_RC_ERROR;
    }

    vtss_clear(mem);
    *had_mem = FALSE;

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        mem.entry.member[pit.iport] = portlist_index_get(isid_port_to_index(isid, pit.iport), &members->entry.member);
        if (mem.entry.member[pit.iport]) {
            T_D("isid:%d iport:%d", isid, pit.iport);
            *had_mem = TRUE;
        }
    }
    if (*had_mem) {
        if ((rc = aggr_mgmt_port_members_add(isid, aggr_no, &mem.entry)) != VTSS_RC_OK) {
            T_D("Failed to add port members in aggr_id:%d, on isid:%d, rc:%d", aggr_no, isid, rc);
        }
    }
    return rc;
}

static mesa_rc vtss_aggregation_port_members_add(vtss_ifindex_t ifindex, const vtss_appl_aggr_group_member_t *const members)
{
    mesa_rc                     rc = VTSS_RC_ERROR;
    switch_iter_t               sit;
    vtss_ifindex_elm_t          ife;
    BOOL                        has_mem = FALSE;

    VTSS_RC(validate_aggr_index(ifindex, &ife));

    /* if called for iftype 'LLAG sid/aggr_no' then add portmembers only for that sid */
    if (ife.iftype == VTSS_IFINDEX_TYPE_LLAG) {
        rc = aggr_private_port_members_add(ife.isid, ife.ordinal, members, &has_mem);
    } else {/* add all aggregation members for all sids */
        T_D("Enter func: %s", __FUNCTION__);
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
        while (switch_iter_getnext(&sit)) {
            rc = aggr_private_port_members_add(sit.isid, ife.ordinal, members, &has_mem);
            if (rc == VTSS_RC_ERROR && has_mem) {
                return rc;/* failed to add, report immediately */
            } else {
                rc = VTSS_RC_OK;
            }
        }
    }
    T_D("Exit func; rc:%d, ife.type:%u, aggr_no:%d", rc, ife.iftype, ife.ordinal);
    return rc;
}

mesa_rc vtss_appl_aggregation_group_set(const vtss_ifindex_t ifindex,
                                        const vtss_appl_aggr_group_conf_t *const conf)
{
    vtss_ifindex_elm_t          ife;
    vtss_appl_aggr_group_member_t    grp;
    vtss_lacp_port_config_t     lacp_conf;
    mesa_rc                     rc = VTSS_RC_OK, port_state, port_state_old;
    BOOL                        member = FALSE;
    vtss_appl_aggr_group_conf_t conf_old = {};
    u32 iport;

    VTSS_RC(validate_aggr_index(ifindex, &ife));
    T_I("Enter. ifindex:%d  mode:%d", ife.ordinal, conf->mode);

    if (!msg_switch_exists(VTSS_ISID_START)) {
        // switch not ready - store the config
        AGGR_CRIT_ENTER();
        aggrglb.group_mode[ife.ordinal] = *conf;
        AGGR_CRIT_EXIT();
        return VTSS_RC_OK;
    }

    if (ife.iftype != VTSS_IFINDEX_TYPE_LLAG) {
        T_E("Group not supported");
        return VTSS_RC_ERROR;
    }
    AGGR_CRIT_ENTER();
    conf_old = aggrglb.group_mode[ife.ordinal];
    AGGR_CRIT_EXIT();

    if (conf->mode == VTSS_APPL_AGGR_GROUP_MODE_RESERVED) {
        // do nothing, just save the mode
    } else if (conf->mode == VTSS_APPL_AGGR_GROUP_MODE_DISABLED) {
        if (conf_old.mode == VTSS_APPL_AGGR_GROUP_MODE_STATIC_ON) {
            (void)aggr_mgmt_group_del(ife.isid, ife.ordinal);
        } else if (conf_old.mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_ACTIVE ||
                   conf_old.mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_PASSIVE) {
            for (iport = VTSS_PORT_NO_START; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
                if (portlist_index_get(isid_port_to_index(ife.isid, iport), &conf_old.cfg_ports.member)) {
                    if ((rc = lacp_mgmt_port_conf_get(ife.isid, iport, &lacp_conf)) != VTSS_RC_OK) {
                        return rc;
                    }
                    lacp_conf.enable_lacp = FALSE;
                    T_D("lacp_mgmt_port_conf_set");
                    if ((rc = lacp_mgmt_port_conf_set(ife.isid, iport, &lacp_conf)) != VTSS_RC_OK) {
                        return rc;
                    }
                }
            }
        }
    } else if (conf->mode == VTSS_APPL_AGGR_GROUP_MODE_STATIC_ON) {
        for (iport = VTSS_PORT_NO_START; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
            if (portlist_index_get(isid_port_to_index(ife.isid, iport), &conf->cfg_ports.member)) {
                member = TRUE;
                break;
            }
        }
        if (member) {
            grp.aggr_no = ife.ordinal;
            grp.entry = conf->cfg_ports;
            if ((rc = vtss_aggregation_port_members_add(ifindex, &grp)) != VTSS_RC_OK) {
                return rc;
            }
        } else {
            if ((rc = aggr_mgmt_group_del(ife.isid, ife.ordinal)) != VTSS_RC_OK) {
                return rc;
            }
        }
    } else { /* LACP */
        for (iport = VTSS_PORT_NO_START; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
            port_state =     portlist_index_get(isid_port_to_index(ife.isid, iport), &conf->cfg_ports.member);
            port_state_old = portlist_index_get(isid_port_to_index(ife.isid, iport), &conf_old.cfg_ports.member);
            if (port_state_old == port_state) {
                continue;
            }
            if (port_state) {
                if ((rc = lacp_mgmt_port_conf_get(ife.isid, iport, &lacp_conf)) != VTSS_RC_OK) {
                    return rc;
                }
                lacp_conf.enable_lacp = port_state;
                lacp_conf.port_key = ife.ordinal;
                lacp_conf.active_or_passive = (conf->mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_ACTIVE) ? VTSS_LACP_ACTMODE_ACTIVE : VTSS_LACP_ACTMODE_PASSIVE;
            } else {
                (void)lacp_mgmt_port_conf_get_default(&lacp_conf);
            }
            if ((rc = lacp_mgmt_port_conf_set(ife.isid, iport, &lacp_conf)) != VTSS_RC_OK) {
                return rc;
            }
        }
    }


    AGGR_CRIT_ENTER();
    if (conf->mode == VTSS_APPL_AGGR_GROUP_MODE_DISABLED) {
        memset(&aggrglb.group_mode[ife.ordinal], 0, sizeof(vtss_appl_aggr_group_conf_t));
        aggrglb.group_mode[ife.ordinal].mode = VTSS_APPL_AGGR_GROUP_MODE_DISABLED;
    } else {
        aggrglb.group_mode[ife.ordinal] = *conf;
    }
    AGGR_CRIT_EXIT();
    return rc;
}

mesa_rc vtss_appl_aggregation_group_get(vtss_ifindex_t ifindex,
                                        vtss_appl_aggr_group_conf_t *const conf)
{
    vtss_ifindex_elm_t          ife;

    VTSS_RC(validate_aggr_index(ifindex, &ife));
    if (ife.iftype == VTSS_IFINDEX_TYPE_LLAG) {
        AGGR_CRIT_ENTER();
        *conf = aggrglb.group_mode[ife.ordinal];
        AGGR_CRIT_EXIT();
    } else {
        T_E("Group not supported");
        return VTSS_RC_ERROR;
    }
    return VTSS_RC_OK;
}

/****************************************************************************/
// Adds ports to an aggregation.
// All ports are considered valid to get added to an aggr group but
// only ports with the same speed/fdx are actually activated in the aggregation.
// In case of different speeds then the port with the highest speed controls the speed of the group.
/****************************************************************************/

mesa_rc aggr_mgmt_port_members_add(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_t *members)
{
    mesa_port_no_t           port_no;
    BOOL                     members_incl = FALSE;
    u32                      port_count = port_count_max();
    mesa_rc                  rc;
    aggr_mgmt_group_no_t     gr, aggr_mgmt_group_end = aggr_mgmt_group_no_end();

    T_D("Doing a aggr add group %u at switch isid %d", aggr_no, isid);

    VTSS_RC(aggr_isid_port_valid(isid, VTSS_PORT_NO_START, aggr_no, TRUE /* Set command */));

    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if (members->member[port_no]) {
            members_incl = 1;
        }

        /* Check if the member port is already part of another group */
        for (gr = AGGR_MGMT_GROUP_NO_START; gr < aggr_mgmt_group_end; gr++) {
            if (gr == aggr_no) {
                continue;
            }
            AGGR_CRIT_ENTER();
            if (VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[isid - VTSS_ISID_START] \
                                 [gr - AGGR_MGMT_GROUP_NO_START].member, (port_no - VTSS_PORT_NO_START)) && members->member[port_no]) {
                AGGR_CRIT_EXIT();
                return AGGR_ERROR_PORT_IN_GROUP;
            }
            AGGR_CRIT_EXIT();
        }
    }
    if (!members_incl) {
        return (aggr_mgmt_group_del(isid, aggr_no));
    }

#ifdef VTSS_SW_OPTION_LACP
    if ((rc = check_for_lacp(isid, aggr_no, members)) != VTSS_RC_OK)  {
        return rc;
    }
#endif /* VTSS_SW_OPTION_LACP */

#if defined(VTSS_SW_OPTION_DOT1X)
    vtss_nas_switch_cfg_t switch_cfg;

    /* Check if dot1x is enabled */
    vtss_usid_t usid = topo_isid2usid(isid);
    if (vtss_nas_switch_cfg_get(usid, &switch_cfg) != VTSS_RC_OK) {
        return VTSS_UNSPECIFIED_ERROR;
    }
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if (members->member[port_no] && switch_cfg.port_cfg[port_no - VTSS_PORT_NO_START].admin_state != VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED) {
            return AGGR_ERROR_DOT1X_ENABLED;
        }
    }
#endif /* VTSS_SW_OPTION_DOT1X */

    if (!verify_max_members(isid, aggr_no, members)) {
        return AGGR_ERROR_MEMBER_OVERFLOW;
    }

    /* Apply the configuration to the chip - if the switch exist */
    if (msg_switch_exists(isid) && (rc = aggr_group_add(isid, aggr_no, members)) != VTSS_RC_OK) {
        return rc;
    }

    /* Add to stack config */
    AGGR_CRIT_ENTER();
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        VTSS_BF_SET(aggrglb.aggr_config_stack.switch_id[isid - VTSS_ISID_START][aggr_no - AGGR_MGMT_GROUP_NO_START].member,
                    (port_no - VTSS_PORT_NO_START), members->member[port_no]);
    }
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if ( VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[isid - VTSS_ISID_START][aggr_no - AGGR_MGMT_GROUP_NO_START].member,
                              (port_no - VTSS_PORT_NO_START))) {
            T_D("Aggr:%u  port:%u are conf agg members", aggr_no, port_no);
        }
    }
    AGGR_CRIT_EXIT();

    /* Inform subscribers of aggregation changes */
    (void)aggr_change_callback(isid, aggr_no);

    return VTSS_RC_OK;
}

// mesa_rc vtss_aggregation_group_del(vtss_ifindex_t ifindex)
// {
//     vtss_ifindex_elm_t  ife;
//     switch_iter_t       sit;
//     mesa_rc             rc = VTSS_RC_ERROR;

//     VTSS_RC(validate_aggr_index(ifindex, &ife));

//     if (ife.iftype == VTSS_IFINDEX_TYPE_LLAG) {
//         rc = aggr_mgmt_group_del(ife.isid, ife.ordinal);
//     } else {
//         (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
//         while (switch_iter_getnext(&sit)) {
//             T_D("isid:%d aggr_no:%d", sit.isid, ife.ordinal);
//             if (aggr_mgmt_group_del(sit.isid, ife.ordinal) == VTSS_RC_OK) {
//                 rc = VTSS_RC_OK;
//             }
//         }
//     }
//     return rc;
// }

/****************************************************************************/
// Removes all members from a aggregation group.
/****************************************************************************/
mesa_rc aggr_mgmt_group_del(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no)
{
    aggr_mgmt_group_member_t members;
    VTSS_RC(aggr_isid_port_valid(isid, VTSS_PORT_NO_START, aggr_no, TRUE /* Set command */));
    if (aggr_mgmt_port_members_get(isid, aggr_no, &members, 0) == VTSS_RC_OK) {
        VTSS_RC(aggr_group_del_no_flash(isid, aggr_no));
    }
    return VTSS_RC_OK;
}

static mesa_rc aggr_private_port_members_get(vtss_isid_t isid,
                                             aggr_mgmt_group_member_t mem_list,
                                             vtss_appl_aggr_group_t *const members)
{
    mesa_rc                     rc = VTSS_RC_ERROR;
    port_iter_t                 pit;

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (mem_list.entry.member[pit.iport]) {
            (void)portlist_index_set(isid_port_to_index(isid, pit.iport), &members->member);
            T_D("isid: %u, iport: %u", isid, pit.iport);
            rc = VTSS_RC_OK;
        } else {
            (void)portlist_index_clear(isid_port_to_index(isid, pit.iport), &members->member);
        }
    }
    return rc;
}

mesa_rc vtss_appl_aggregation_port_members_get(vtss_ifindex_t ifindex, vtss_appl_aggr_group_member_t *const members)
{
    vtss_ifindex_elm_t          ife;
    switch_iter_t               sit;
    u32                         aggr_ok_cnt = 0;
    aggr_mgmt_group_member_t    mem_list;

    VTSS_RC(validate_aggr_index(ifindex, &ife));
    memset(members, 0, sizeof(vtss_appl_aggr_group_member_t));
    vtss_clear(mem_list);

    T_D("getting aggr_no:%d, type:%d", ife.ordinal, ife.iftype);
    if (ife.iftype == VTSS_IFINDEX_TYPE_LLAG) {
        (void)aggr_mgmt_port_members_get(ife.isid, ife.ordinal, &mem_list, false);
        if (aggr_private_port_members_get(ife.isid, mem_list, &members->entry) == VTSS_RC_OK) {
            aggr_ok_cnt++;
        }
    } else {
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
        while (switch_iter_getnext(&sit)) {
            vtss_clear(mem_list);
            (void)aggr_mgmt_port_members_get(sit.isid, ife.ordinal, &mem_list, false);
            if (aggr_private_port_members_get(sit.isid, mem_list, &members->entry) == VTSS_RC_OK) {
                aggr_ok_cnt++;/* if != 0, then atleast 1 stack node has port member(s) in this aggrid */
                /* if aggr_ok_cnt > 1, then it's a GLAG right? in case VTSS_GLAG_PLUS_LLAG_SUPPORT is defined */
            }
        }
    }
    return aggr_ok_cnt ? VTSS_RC_OK : (mesa_rc)AGGR_ERROR_ENTRY_NOT_FOUND;
}

static mesa_rc vtss_aggregation_status_get_priv(vtss_ifindex_t ifindex, vtss_appl_aggr_group_status_t *const status)
{
    vtss_ifindex_elm_t       ife;
    aggr_mgmt_group_member_t cfg_group = {}, aggr_group = {};
    vtss_appl_aggr_group_conf_t group_conf;
    mesa_port_no_t           port_no;
    u32                      port_count;

    VTSS_RC(validate_aggr_index(ifindex, &ife));
    memset(status, 0, sizeof(vtss_appl_aggr_group_status_t));
    (void)vtss_appl_aggregation_group_get(ifindex, &group_conf);
    status->mode = group_conf.mode;
    port_count = port_count_max();
    if (group_conf.mode == VTSS_APPL_AGGR_GROUP_MODE_DISABLED) {
        /* Disabled = not used */
        return VTSS_RC_ERROR;
    } else if (group_conf.mode == VTSS_APPL_AGGR_GROUP_MODE_RESERVED) {
        /* Reserved but no members yet */
        sprintf(status->type, "%s", "RESERVED");
    } else if (group_conf.mode == VTSS_APPL_AGGR_GROUP_MODE_STATIC_ON) {
        (void)aggr_mgmt_port_members_get(ife.isid, ife.ordinal, &cfg_group, FALSE);
        sprintf(status->type, "%s", "STATIC");
    } else if (group_conf.mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_ACTIVE || group_conf.mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_PASSIVE) {
        (void)aggr_mgmt_lacp_members_get(ife.isid, ife.ordinal, &aggr_group, FALSE);
        for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
            cfg_group.entry.member[port_no] =  (get_lacp_glb_config(ife.isid, port_no) == ife.ordinal) ? TRUE : FALSE;
        }
        if (group_conf.mode == VTSS_APPL_AGGR_GROUP_MODE_LACP_ACTIVE) {
            sprintf(status->type, "%s", "LACP_ACTIVE");
        } else {
            sprintf(status->type, "%s", "LACP_PASSIVE");
        }
    }

    /* Get the actual aggregated status */
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if (get_aggrglb_active(ife.isid, port_no) == ife.ordinal) {
            aggr_group.entry.member[port_no] = 1;
        } else {
            aggr_group.entry.member[port_no] = 0;
        }
    }
    /* /\* Get the actual aggregated status *\/ */
    /* (void)aggr_mgmt_members_get(ife.isid, ife.ordinal, &aggr_group, 0); */

    /* Also the speed */
    status->speed = aggr_mgmt_speed_get(ife.isid, ife.ordinal);

    /* Convert the type */
    (void)aggr_private_port_members_get(ife.isid, cfg_group,  &status->cfg_ports);
    (void)aggr_private_port_members_get(ife.isid, aggr_group, &status->aggr_ports);

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_aggregation_status_get(vtss_ifindex_t ifindex, vtss_appl_aggr_group_status_t *const status)
{
    return vtss_aggregation_status_get_priv(ifindex, status);
}


static mesa_rc vtss_aggregation_status_get_current(vtss_ifindex_t ifindex, vtss_appl_aggr_group_status_t *const status)
{
    vtss_ifindex_elm_t  ife;
    mesa_rc             rc = VTSS_RC_ERROR;
    vtss_ifindex_t      ifidx;
    switch_iter_t       sit;

    VTSS_RC(validate_aggr_index(ifindex, &ife));

    if (ife.iftype == VTSS_IFINDEX_TYPE_LLAG) {
        rc = vtss_aggregation_status_get_priv(ifindex, status);
    } else {
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
        while (switch_iter_getnext(&sit)) {
            (void)vtss_ifindex_from_llag(sit.isid, ife.ordinal, &ifidx);
            T_N("isid:%d aggr_no:%d", sit.isid, ife.ordinal);
            if (vtss_aggregation_status_get_priv(ifidx, status) == VTSS_RC_OK) {
                rc = VTSS_RC_OK;
            }
        }
    }
    return rc;
}

/****************************************************************************/
// Get members in a given aggr group.
// 'Next' is used to browes trough active groups.
// Configuration version. See aggr_mgmt_members_get() for runtime version.
/****************************************************************************/
mesa_rc aggr_mgmt_port_members_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_member_t *members, BOOL next)
{
    mesa_rc            rc = AGGR_ERROR_ENTRY_NOT_FOUND;
    mesa_port_no_t     port_no;
    BOOL               found_group = FALSE;
    port_iter_t        pit;
    aggr_mgmt_group_no_t aggr_mgmt_group_end = aggr_mgmt_group_no_end();

    VTSS_RC(aggr_isid_port_valid(isid, VTSS_PORT_NO_START, 1, FALSE /* Get command, VTSS_ISID_LOCAL is OK */));

    if (isid == VTSS_ISID_LOCAL) {
        isid = get_local_isid();
    }

    /* In case of next search: Get the first active aggregation group */
    if (aggr_no == 0 && next) {
        aggr_no = AGGR_MGMT_GROUP_NO_START;
    } else if (aggr_no != 0 && next) {
        aggr_no++;
    }
    AGGR_CRIT_ENTER();
    while (next && aggr_no < aggr_mgmt_group_end) {
        (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[isid - VTSS_ISID_START][aggr_no - AGGR_MGMT_GROUP_NO_START].member,
                                 (pit.iport - VTSS_PORT_NO_START))) {
                T_N("Found (next) static members in aggr:%u, isid:%d", aggr_no, isid);
                found_group = TRUE;
                break;
            }
        }
        if (found_group) {
            break;
        }
        aggr_no++;
    }
    AGGR_CRIT_EXIT();
    if (next && !found_group) {
        return AGGR_ERROR_ENTRY_NOT_FOUND;
    }

    members->aggr_no = aggr_no;
    if (AGGR_MGMT_GROUP_IS_AGGR(aggr_no)) {
        AGGR_CRIT_ENTER();
        for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            members->entry.member[port_no] = VTSS_PORT_BF_GET(aggrglb.aggr_config_stack.switch_id[isid - VTSS_ISID_START][aggr_no - AGGR_MGMT_GROUP_NO_START].member, (port_no - VTSS_PORT_NO_START));

            if (members->entry.member[port_no]) {
                rc = VTSS_RC_OK;
            }
        }
        AGGR_CRIT_EXIT();
    } else {
        return AGGR_ERROR_PARM;
    }
    return rc;
}

/****************************************************************************/
// Get the speed of the group
/****************************************************************************/
mesa_port_speed_t aggr_mgmt_speed_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no)
{
    if (!AGGR_MGMT_GROUP_IS_AGGR(aggr_no)) {
        T_E("Illegal aggr_no:%d", aggr_no);
        return MESA_SPEED_UNDEFINED;
    }
    return get_aggrglb_spd(aggr_no);
}

/****************************************************************************/
// Get the aggr group no max
/****************************************************************************/
aggr_mgmt_group_no_t aggr_mgmt_group_no_end(void)
{
    return aggrglb.aggr_group_cnt_end;
}

#ifdef VTSS_SW_OPTION_LACP
/****************************************************************************/
/****************************************************************************/
static mesa_rc aggr_mgmt_port_state_set(vtss_isid_t isid, mesa_port_no_t port_no, BOOL enable)
{
    port_vol_conf_t         conf;
    mesa_bool_t             state;
    vtss_ifindex_t          ifindex;
    vtss_appl_port_status_t port_status;
    mesa_rc                 rc;

    T_I("Change the state of port %u to %s", port_no, enable ? "FWD" : "BLK");
    if ((rc = port_vol_conf_get(PORT_USER_AGGR, port_no, &conf)) == VTSS_RC_OK && !conf.oper_up) {
        if (conf.oper_down != !enable) {
            // Inform the port module of the 'oper_down' state
            conf.oper_down = !enable;
            if (port_vol_conf_set(PORT_USER_AGGR, port_no, &conf) != VTSS_RC_OK) {
                T_E("port_vol_conf_set(%u) failed", port_no);
            }
            // Set the forwarding state of this port (if the link is up).
            // By this we avoid loops between the ports in the aggr when they are not aggregated.
            // STP thinks that this port is aggregated and will not try to block it (if STP is enabled)
            // The port module will consider 'oper_down' state before changing the fwd state.
            (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_no, &ifindex);
            if (vtss_appl_port_status_get(ifindex, &port_status) == VTSS_RC_OK && port_status.link) {
                (void)mesa_port_state_get(NULL, port_no, &state);
                if (state != enable) {
                    T_I("mesa_port_state_set:%d", enable);
                    (void)mesa_port_state_set(NULL, port_no, enable);
                    if (mesa_mac_table_port_flush(NULL, port_no) != VTSS_RC_OK) {
                        T_W("Could not flush");
                    }
                }
            }
        }
    }

    // Set the state locally
    set_lacp_glb_port_state(isid, port_no, enable);
    return VTSS_RC_OK;
}

// Return the state of the incoming port.
// The goal is to have at least one forwarding port in an aggregation (even though the aggregetion is not formed)
static BOOL get_group_fwd_state(vtss_isid_t isid, mesa_port_no_t port_no, aggr_mgmt_group_no_t aggr_no, BOOL enable)
{
    mesa_port_no_t p;
    BOOL rc = enable;

    T_I("get_group_fwd_state aggr.%d p:%d ena:%d", aggr_no, port_no, enable);

    if (enable) {
        for (p = VTSS_PORT_NO_START; p < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); p++) {
            if (p == port_no) {
                continue;
            }
            /* As 'port_no' is beeing set in FWD mode we want to disable other ports not participating in LACP, but in FWD mode */
            /* Should never be more than one though */
            if ((get_lacp_glb_config(isid, p) == aggr_no) && get_lacp_glb_port_state(isid, p) &&
                (get_aggrglb_active(isid, p) == VTSS_AGGR_NO_NONE)) {
                T_I("Disable fwd state on a not LACP particiapting port (%d)", p);
                if (aggr_mgmt_port_state_set(isid, p, FALSE) != VTSS_RC_OK) {
                    T_E("Could not set port %d state", p);
                }
                break;
            }
        }
    } else {
        rc = TRUE; // in case of this is the last port, i.e. no other port is in FWD state
        for (p = VTSS_PORT_NO_START; p < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); p++) {
            if (p == port_no) {
                continue;
            }
            if ((get_lacp_glb_config(isid, p) == aggr_no) && get_lacp_glb_port_state(isid, p)) {
                rc = FALSE;
                T_I("Another port is forwarding, i.e. set this one to blocking");
                break;
            }
        }
    }

    T_I("Return rc:%d", rc);
    return rc;
}


/****************************************************************************/
/****************************************************************************/
// Called from LACP core. Add/remove port from HW aggregation
static mesa_rc aggr_mgmt_lacp_aggr_modify(uint aid, l2_port_no_t l2port, BOOL enable)
{
    mesa_port_no_t    port_no, p;
    vtss_isid_t       isid;
    mesa_aggr_no_t    aggr_no;
    aggr_mgmt_group_t entry;
    u32               members = 0;

    if (!l2port2port(l2port, &isid, &port_no)) {
        T_E("Could not find a isid,port for l2_proto_port_t:%d\n", l2port);
        return AGGR_ERROR_PARM;
    }

    if (enable) {
        aggr_no = get_lacp_glb_config(isid, port_no);
    } else {
        aggr_no = aggrglb.aggr_lacp[aid].aggr_no;
    }

    if (enable && !AGGR_MGMT_GROUP_IS_AGGR(aggr_no)) {
        T_W("The lacp aggregation group (aggr_no:%d aid:%d isid:%d port:%d) is not created yet?!", aggr_no, aid, isid, l2port);
        return AGGR_ERROR_PARM;
    }

    AGGR_CRIT_ENTER();
    aggrglb.lacp_group[aggr_no].member[port_no] = enable;
    entry.member = aggrglb.lacp_group[aggr_no].member;
    aggrglb.aggr_lacp[aid].aggr_no = aggr_no;
    aggrglb.aggr_lacp[aid].members[l2port] = enable;
    AGGR_CRIT_EXIT();

    if (!enable) {
        for (p = VTSS_PORT_NO_START; p < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); p++) {
            if (entry.member[p]) {
                members++;
            }
        }
    }
    if (!enable && (members == 0)) {
        (void)aggr_group_del(isid, aggr_no);
    } else {
        if (aggr_group_add(isid, aggr_no, &entry) != VTSS_RC_OK) {
            return VTSS_UNSPECIFIED_ERROR;
        }
    }

    T_D("%s isid:%d, port:%u to aggr id %u", enable ? "Added" : "Removed", isid, port_no, aggr_no);
    /* Inform subscribers of aggregation changes */
    (void)aggr_change_callback(isid, aggrglb.aggr_lacp[aid].aggr_no);

    return VTSS_RC_OK;
}

/****************************************************************************/
/****************************************************************************/

mesa_rc aggr_mgmt_lacp_members_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_member_t *members, BOOL next)
{
    mesa_rc            rc = AGGR_ERROR_ENTRY_NOT_FOUND;
    mesa_port_no_t     port_no = 0;
    BOOL               found_group = false;
    u32                port_count = port_count_max();
    aggr_mgmt_group_t  local_members;
    aggr_mgmt_group_no_t aggr_mgmt_group_end = aggr_mgmt_group_no_end();

    VTSS_RC(aggr_isid_port_valid(isid, VTSS_PORT_NO_START, 1, FALSE /* Get command, VTSS_ISID_LOCAL OK */));

    if (isid == VTSS_ISID_LOCAL) {
        isid = get_local_isid();
    }

    vtss_clear(local_members);
    vtss_clear(*members);
    /* Get the first active aggregation group */
    if (aggr_no == 0 && next) {
        for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < aggr_mgmt_group_end; aggr_no++) {
            if ((found_group = aggr_group_exists(isid, aggr_no, &local_members) == 1)) {
                rc = VTSS_RC_OK;
                break;
            }
        }
        if (!found_group) {
            return AGGR_ERROR_ENTRY_NOT_FOUND;
        }
    } else if (aggr_no != 0 && next) {
        /* Get the next active aggregation group */
        for (aggr_no = (aggr_no + 1); aggr_no < aggr_mgmt_group_end; aggr_no++) {
            if ((found_group = aggr_group_exists(isid, aggr_no, &local_members) == 1)) {
                rc = VTSS_RC_OK;
                break;
            }
        }
        if (!found_group) {
            return AGGR_ERROR_ENTRY_NOT_FOUND;
        }
    }

    members->aggr_no = aggr_no;
    /* Check if the group is known */
    if (!next) {
        if (aggr_group_exists(isid, aggr_no, &local_members)) {
            /* Its there */
            rc = VTSS_RC_OK;
        }
    }
    /* Return the portlist (empty or not)  */
    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        members->entry.member[port_no] = local_members.member[port_no];
    }

    return rc;
}

/****************************************************************************/
/****************************************************************************/
/* LACP function. First step in adding a port to an aggregation - only configuration though */
/* Called from LACP module (via vtss_appl_aggregation_group_set) to add a port to a LACP group */
/* The port is NOT added to a HW aggregation */
mesa_rc aggr_mgmt_lacp_member_set(vtss_isid_t isid, mesa_port_no_t port_no, aggr_mgmt_group_no_t aggr_no)
{
    BOOL fwd_state;
    aggr_mgmt_group_no_t old_aggr;
    T_I("%s port %d %s the aggr configuration", aggr_no != VTSS_AGGR_NO_NONE ? "Add" : "Remove", port_no, aggr_no != VTSS_AGGR_NO_NONE ? "to" : "from");

    /* Verify parameters */
    VTSS_RC(aggr_isid_port_valid(isid, port_no, (aggr_no == VTSS_AGGR_NO_NONE ? 1 : aggr_no), TRUE));

    if ((old_aggr = get_lacp_glb_config(isid, port_no)) == aggr_no) {
        return VTSS_RC_OK; // Already a member or not added
    }

    if (aggr_no != VTSS_AGGR_NO_NONE) {
        // Add the port
        if (get_lacp_glb_config(isid, port_no) != VTSS_AGGR_NO_NONE) {
            return AGGR_ERROR_PORT_IN_GROUP; // Member elsewhere
        }
        /* The FWD state is based on state of other ports to.  FALSE because the HW is not activated yet */
        fwd_state = get_group_fwd_state(isid, port_no, aggr_no, FALSE);
    } else {
        // Delete the port and enable the FWD state
        fwd_state = TRUE;
    }

    /* Enable/disable port forwarding to avoid loop */
    if (aggr_mgmt_port_state_set(isid, port_no, fwd_state) != VTSS_RC_OK) {
        T_E("Could not set port %d state", port_no);
    }

    set_lacp_glb_config(isid, port_no, aggr_no);

    if (aggr_no == VTSS_AGGR_NO_NONE) {
        /* Port is leaving the aggr, flush the mac table */
        (void)mesa_mac_table_port_flush(NULL, port_no);
    }

    /* Inform subscribers of aggregation changes */
    (void)aggr_change_callback(isid, (aggr_no == VTSS_AGGR_NO_NONE) ? old_aggr : aggr_no);

    return VTSS_RC_OK;
}

/****************************************************************************/
/****************************************************************************/
/* LACP function. Add/remove member to/from the physical aggregation */
/* Called when the LACP core as found an link partner and wants to add or remove this port to an aggregation */
mesa_rc aggr_mgmt_lacp_member_add(uint aid, l2_port_no_t l2port, BOOL enable)
{
    mesa_rc rc;
    mesa_port_no_t port_no;
    vtss_isid_t    isid;
    BOOL fwd_state;
    aggr_mgmt_group_no_t aggr;

    if (!l2port2port(l2port, &isid, &port_no)) {
        return AGGR_ERROR_PARM;
    }

    aggr = get_lacp_glb_config(isid, port_no);

    T_I("%s port %d %s the physical aggregation", enable ? "Add" : "Remove", port_no, enable ? "to" : "from");

    /* Enable/disable port HW aggregation */
    if ((rc = aggr_mgmt_lacp_aggr_modify(aid, l2port, enable)) != VTSS_RC_OK) {
        T_W("Could not modify LACP group port:%d", l2port);
        return rc;
    }

    if (aggr == VTSS_AGGR_NO_NONE) {
        return VTSS_RC_OK; // Member already removed
    }

    /* The FWD state is based on state of other ports */
    fwd_state = get_group_fwd_state(isid, port_no, aggr, enable);

    /* Enable/disable port forwarding to avoid loop */
    if (aggr_mgmt_port_state_set(isid, port_no, fwd_state) != VTSS_RC_OK) {
        T_E("Could not set port %d state", port_no);
    }

    if (!enable) {
        /* Port is leaving the aggr, flush the mac table */
        (void)mesa_mac_table_port_flush(NULL, port_no);
    }

    return VTSS_RC_OK;
}
#endif /* VTSS_SW_OPTION_LACP */

#ifdef VTSS_SW_OPTION_LACP
/****************************************************************************/
/****************************************************************************/
aggr_mgmt_group_no_t lacp_to_aggr_id(int aid)
{
    aggr_mgmt_group_no_t aggr;
    AGGR_CRIT_ENTER();
    aggr = aggrglb.aggr_lacp[aid].aggr_no;
    AGGR_CRIT_EXIT();
    return aggr;
}
#endif /* VTSS_SW_OPTION_LACP */

#ifdef VTSS_SW_OPTION_LACP
/****************************************************************************/
/****************************************************************************/
mesa_rc aggr_mgmt_lacp_id_get_next(int *search_aid, int *return_aid)
{
    int aid, search;
    if (search_aid == NULL) {
        search = 0;
    } else {
        search = *search_aid;
        search++;
    }

    AGGR_CRIT_ENTER();
    for (aid = search; aid < (VTSS_LACP_MAX_PORTS_ + VTSS_PORT_NO_START); aid++) {
        if (aggrglb.aggr_lacp[aid].aggr_no != 0 && aggrglb.aggr_lacp[aid].aggr_no != LACP_ONLY_ONE_MEMBER) {
            *return_aid = aid;
            break;
        }
    }
    AGGR_CRIT_EXIT();
    if (aid >= (VTSS_LACP_MAX_PORTS_ + VTSS_PORT_NO_START)) {
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}
#endif /* VTSS_SW_OPTION_LACP */

/****************************************************************************/
// Get members in a given aggr group for both LACP and STATIC.
// 'Next' is used to browse through active groups.
// Note! Only members with portlink will be returned.
// If less than 2 members are found, 'AGGR_ERROR_ENTRY_NOT_FOUND' will be returned.
// Runtime version. See aggr_mgmt_port_member_get() for static configuration
// version.
/****************************************************************************/
mesa_rc aggr_mgmt_members_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no,  aggr_mgmt_group_member_t *members, BOOL next)
{
    mesa_port_no_t port_no;
    u32            port_count = port_count_max();
    u32            member = 0;
    mesa_rc        rc = VTSS_RC_OK;

    VTSS_RC(aggr_isid_port_valid(isid, VTSS_PORT_NO_START, 1, FALSE /* Get command, VTSS_ISID_LOCAL OK */));
    T_N("Enter aggr_mgmt_member_get (Static and LACP get) isid:%d aggr_no:%u", isid, aggr_no);

    for (port_no = VTSS_PORT_NO_START; port_no < port_count; port_no++) {
        if (get_port_link(port_no) && ((get_lacp_glb_config(isid, port_no) == aggr_no) || (get_aggrglb_active(isid, port_no) == aggr_no))) {
            if ((get_aggrglb_active(isid, port_no) == aggr_no) || (get_lacp_glb_config(isid, port_no) == aggr_no)) {
                members->entry.member[port_no] = 1;
                member++;
            } else {
                members->entry.member[port_no] = 0;
            }
        }
    }
    if (member > 0) {
        T_D("Return OK. Total active ports:%d", member);
    } else {
        T_D("Return AGGR_ERROR_ENTRY_NOT_FOUND. Total active ports:%d", member);
        vtss_clear(*members);
        rc = AGGR_ERROR_ENTRY_NOT_FOUND;
    }
    return rc;
}

/****************************************************************************/
// Returns the aggr number for a port.  Returns 0 if the port is not a member or if the link is down.
/****************************************************************************/
aggr_mgmt_group_no_t aggr_mgmt_get_aggr_id(vtss_isid_t isid, mesa_port_no_t port_no)
{
    if (aggr_isid_port_valid(isid, VTSS_PORT_NO_START, 1, FALSE) == VTSS_RC_ERROR) {
        return 0;
    }
    return get_aggrglb_active(isid, port_no);
}

/****************************************************************************/
/****************************************************************************/
aggr_mgmt_group_no_t aggr_mgmt_get_port_aggr_id(vtss_isid_t isid, mesa_port_no_t port_no)
{
    aggr_mgmt_group_member_t members;
    aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_end = aggr_mgmt_group_no_end();

    for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < aggr_mgmt_group_end; aggr_no++) {
        if (aggr_mgmt_port_members_get(isid, aggr_no,  &members, 0) == VTSS_RC_OK) {
            if (members.entry.member[port_no]) {
                return aggr_no;
            }
        }
    }

    return 0;
}

static BOOL aggr_lacp_group_port_exists(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, mesa_port_no_t port_no)
{
    l2_port_no_t       l2port;
    BOOL               found_port_group = 0;
    u32                aid;

    AGGR_CRIT_ENTER();
    for (aid = 0; aid < (VTSS_LACP_MAX_PORTS_ + VTSS_PORT_NO_START); aid++) {
        if (aggrglb.aggr_lacp[aid].aggr_no == aggr_no) {
            l2port = L2PORT2PORT(isid, port_no);
            if ((aggrglb.aggr_lacp[aid].members[l2port])) {
                found_port_group = 1;
            }
        }
    }
    AGGR_CRIT_EXIT();
    return found_port_group;
}

aggr_mgmt_group_no_t aggr_lacp_mgmt_get_port_aggr_id(vtss_isid_t isid, mesa_port_no_t port_no)
{
    aggr_mgmt_group_no_t     aggr_no;
    for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < AGGR_MGMT_GROUP_NO_END_; aggr_no++) {
        if (aggr_lacp_group_port_exists(isid, aggr_no, port_no)) {
            return aggr_no;
        }
    }
    return 0;
}

/****************************************************************************/
// Returns information if the port is participating in LACP or Static aggregation
// 0 = No participation
// 1 = Static aggregation participation
// 2 = LACP aggregation participation
/****************************************************************************/
vtss_port_participation_t aggr_mgmt_port_participation(vtss_isid_t isid, mesa_port_no_t port_no)
{
    aggr_mgmt_group_member_t members;
    aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_end = aggr_mgmt_group_no_end();

#ifdef VTSS_SW_OPTION_LACP
    vtss_lacp_port_config_t  conf;

    /* Check if the group or ports is occupied by LACP in this isid */
    if  (lacp_mgmt_port_conf_get(isid, port_no, &conf) == VTSS_RC_OK) {
        if (conf.enable_lacp) {
            return PORT_PARTICIPATION_TYPE_LACP;
        }
    }
#endif

    /* Check if the group or ports is occupied by Static aggr in this isid */
    for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < aggr_mgmt_group_end; aggr_no++) {
        if (aggr_mgmt_port_members_get(isid, aggr_no, &members, 0) == VTSS_RC_OK) {
            if (members.entry.member[port_no]) {
                return PORT_PARTICIPATION_TYPE_STATIC;
            }
        }
    }

    return PORT_PARTICIPATION_TYPE_PORT;
}

mesa_rc vtss_appl_aggregation_mode_set(const mesa_aggr_mode_t *const mode)
{
    return aggr_mgmt_aggr_mode_set((mesa_aggr_mode_t *)mode);
}

/****************************************************************************/
// Sets the aggregation mode. The mode is used by all the aggregation groups
/****************************************************************************/
mesa_rc aggr_mgmt_aggr_mode_set(mesa_aggr_mode_t *mode)
{
    switch_iter_t          sit;

    if (!msg_switch_is_primary()) {
        T_W("not ready");
        return AGGR_ERROR_STACK_STATE;
    }

    if (!mode->smac_enable && !mode->dmac_enable && !mode->sip_dip_enable && !mode->sport_dport_enable) {
        return AGGR_ERROR_INVALID_MODE;
    }

    /* Add to local and stack config */
    AGGR_CRIT_ENTER();
    aggrglb.aggr_config.mode = *mode;
    aggrglb.aggr_config_stack.mode = *mode;
    AGGR_CRIT_EXIT();

    // Loop over existing switches and send them the new configuration.
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        aggr_stack_mode_set(sit.isid, mode);
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_aggregation_mode_get(mesa_aggr_mode_t *const mode)
{
    mesa_aggr_mode_t tmp_mode;

    if (aggr_mgmt_aggr_mode_get(&tmp_mode) == VTSS_RC_OK) {
        *mode = tmp_mode;
        return VTSS_RC_OK;
    }
    return VTSS_RC_ERROR;
}

/****************************************************************************/
// Gets the aggregation mode.  The 'mode' points to the updated mode type
/****************************************************************************/
mesa_rc aggr_mgmt_aggr_mode_get(mesa_aggr_mode_t *mode)
{
    /* Get from local config */
    AGGR_CRIT_ENTER();
    *mode = aggrglb.aggr_config.mode;
    AGGR_CRIT_EXIT();
    return VTSS_RC_OK;
}

/****************************************************************************/
// Registration for callbacks if aggregation changes
/****************************************************************************/
void aggr_change_register(aggr_change_callback_t cb)
{
    VTSS_ASSERT(aggrglb.aggr_callbacks < ARRSZ(aggrglb.callback_list));
    AGGR_CB_CRIT_ENTER();
    aggrglb.callback_list[aggrglb.aggr_callbacks++] = cb;
    AGGR_CB_CRIT_EXIT();
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void aggr_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_aggr_json_init();
#endif

extern "C" int aggr_icli_cmd_register();

/****************************************************************************/
/****************************************************************************/
mesa_rc aggr_init(vtss_init_data_t *data)
{
    vtss_isid_t          isid = data->isid;
    aggr_mgmt_group_no_t aggr_no;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_I("INIT_CMD_INIT");
        /* Initilize the module */
        aggr_start(1);
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        aggr_mib_init();  /* Register our private mib */
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_aggr_json_init();
#endif
        aggr_icli_cmd_register();
        break;

    case INIT_CMD_START:
        T_I("INIT_CMD_START");
#ifdef VTSS_SW_OPTION_ICFG
        if (aggr_icfg_init() != VTSS_RC_OK) {
            T_D("Calling aggr_icfg_init() failed");
        }
#endif
        aggr_start(0);
        break;

    case INIT_CMD_CONF_DEF:
        T_I("INIT_CMD_CONF_DEF, isid: %d", isid);
        if (isid == VTSS_ISID_GLOBAL) {
            /* Reset configuration to default */
            aggr_conf_stack_default(TRUE);
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_I("ICFG_LOADING_PRE");
        aggr_conf_stack_default(FALSE);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_I("ICFG_LOADING_POST");

        /* Configure the new switch */
        aggr_switch_conf_set(isid);

        for (aggr_no = AGGR_MGMT_GROUP_NO_START; aggr_no < aggr_mgmt_group_no_end(); aggr_no++) {
            aggr_status_update_priv(isid, aggr_no);
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

