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
#include "mstp_api.h"           /* Our module API */
#include "mstp.h"               /* Our private definitions */
#include "vtss_mstp_api.h"      /* Core MSTP API */
#include "vtss_mstp_callout.h"  /* mstp_fwdstate_t */

/* Used APIs */
#include "critd_api.h"
#include "packet_api.h"
#include "msg_api.h"
#include "conf_api.h"
#include "vlan_api.h"
#include "misc_api.h"           /* instantiate MAC */
#include "port_api.h"
#include "port_iter.hxx"
#ifdef VTSS_SW_OPTION_DOT1X
#include "dot1x_api.h"
#endif /* VTSS_SW_OPTION_DOT1X */
#if defined(VTSS_SW_OPTION_DOT1X)
#include "topo_api.h"           /* topo_isid2mac()/topo_isid2usid() */
#endif
#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

#ifdef VTSS_SW_OPTION_ICLI
#include "mstp_icfg.h"
#endif

#include "vtss_os_wrapper.h"
#include "vtss/appl/nas.h" // For NAS management functions

#include <sys/sysinfo.h>

static const u8 ieee_bridge[6] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00};

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#define VTSS_MODULE_ID_MSTP VTSS_MODULE_ID_RSTP
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_MSTP
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_MSTP

#define VTSS_TRACE_GRP_DEFAULT   0
#define VTSS_TRACE_GRP_CONTROL   1
#define VTSS_TRACE_GRP_INTERFACE 2

#define _C VTSS_TRACE_GRP_CONTROL
#define _I VTSS_TRACE_GRP_INTERFACE

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "mstp", "Spanning Tree"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default (MSTP core)",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_CONTROL] = {
        "control",
        "MSTP control",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_INTERFACE] = {
        "interface",
        "MSTP Core interfaces",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/* Thread variables */
static vtss_handle_t mstp_thread_handle;
static vtss_thread_t mstp_thread_block;

typedef struct {
    /* Pending changes */
    CapArray<BOOL, VTSS_APPL_CAP_L2_LLAG_CNT> change;
    /* Current STP LLAG state */
    CapArray<llag_participants_t, VTSS_APPL_CAP_ISID_CNT, VTSS_APPL_CAP_L2_LLAG_CNT> llag;
    /* All ports fwd state */
    CapArray<mstp_fwdstate_t, VTSS_APPL_CAP_L2_POAG_CNT> stpstate;
    /* Physical ports -> aggregation */
    CapArray<u16, MEBA_CAP_BOARD_PORT_MAP_COUNT> parent;
} mstp_astate_t;

/* MSTP global data */
static struct {
    BOOL                    ready;                 /* MSTP Initiated & we're acting primary switch */
    vtss_flag_t             control_flags;         /* MSTP thread control */
    critd_t                 mutex;                 /* Global module/API protection */
    vtss_sem_t              defconfig_sema;        /* Signal completion of load defaults from MSTP worker thread => thread running INIT_CMD_CONF_DEF */
    u32                     switch_sync;           /* Pending switch sync-ups */
    mstp_astate_t           aggr;                  /* AGGR state */
    mstp_conf_t             conf;                  /* Current configuration */
    mstp_macaddr_t          sysmac;                /* Switch system MAC */
    u32                     traps;                 /* Aggregated trap state */
    mstp_trap_sink_t        trap_cb;               /* Trap sink */
    mstp_config_change_cb_t config_cb[3];          /* Config callbacks */
    mstp_bridge_t           *mstpi;                /* MSTP instance handle */
} mstp_global;

// Indicates whether we have H/W support for MSTP.
static bool MSTP_hw_support;
static bool MSTP_lan966x;

/*
 * Forward defs
 */

static void mstp_tether(l2_port_no_t l2aggr, l2_port_no_t l2phys);

static void mstp_liberate(l2_port_no_t l2aggr, l2_port_no_t l2phys);

/*
 * Aggregation abstraction, ( Poor Man's C++ :-) )
 */

static uint _llag_count(struct mstp_aggr_obj const *aob)
{
    llag_participants_t *llag = (llag_participants_t *)aob->data_handle;
    return llag->cmn.n_members;
}

static l2_port_no_t _llag_first_port(struct mstp_aggr_obj const *aob)
{
    llag_participants_t *llag = (llag_participants_t *)aob->data_handle;
    l2_port_no_t        l2port = L2_NULL;

    if (llag->cmn.n_members) {
        l2port = llag->cmn.port_min + aob->u.llag.port_offset;
    }

    T_NG(_C, "ret %d", l2port);
    return l2port;
}

static l2_port_no_t _llag_next_port(struct mstp_aggr_obj const *aob, l2_port_no_t l2port)
{
    llag_participants_t *llag = (llag_participants_t *)aob->data_handle;
    u16                 ix    = (u16)(l2port - aob->u.llag.port_offset);
    l2_port_no_t        l2ret = L2_NULL;

    while (++ix <= llag->cmn.port_max) {
        if (MSTP_AGGR_GET_MEMBER(ix, llag)) {
            l2ret = ix + aob->u.llag.port_offset;
            break;
        }
    }

    T_NG(_C, "ret %d", l2ret);
    return l2ret;
}

/*lint -sem(_llag_update, thread_protected) ... We are locked already */
static void _llag_update(struct mstp_aggr_obj *aob)
{
    llag_participants_t      *llag = (llag_participants_t *)aob->data_handle, oldstate, *pold = &oldstate;
    port_iter_t              pit;
    aggr_mgmt_group_member_t am;

    T_DG(_C, "port %d", aob->l2port);

    MSTP_ASSERT_LOCKED();

    *pold = *llag;
    vtss_clear(*llag);

    if (aggr_mgmt_members_get(aob->u.llag.isid, aob->u.llag.aggr_no, &am, FALSE) == VTSS_RC_OK) {
        (void)port_iter_init(&pit, NULL, aob->u.llag.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (am.entry.member[pit.iport]) {
                T_DG(_C, "isid %d, LLAG aggr %u, switch port %u",
                     aob->u.llag.isid, aob->u.llag.aggr_no, pit.iport);
                if (llag->cmn.n_members == 0) {
                    llag->cmn.port_min = llag->cmn.port_max = pit.iport;
                } else {
                    llag->cmn.port_max = pit.iport;
                }
                llag->cmn.n_members++;
                MSTP_AGGR_SET_MEMBER(pit.iport, llag, 1);
            }
        }
    }

    /* Check for new/departed physical ports */
    (void)port_iter_init(&pit, NULL, aob->u.llag.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (MSTP_AGGR_GET_MEMBER(pit.iport, llag)) {
            /* Not Member -> Member */
            if (!MSTP_AGGR_GET_MEMBER(pit.iport, pold)) {
                mstp_tether(aob->l2port, pit.iport + aob->u.llag.port_offset);
            }
        } else {
            /* Member -> Not member */
            if (MSTP_AGGR_GET_MEMBER(pit.iport, pold)) {
                mstp_liberate(aob->l2port, pit.iport + aob->u.llag.port_offset);
            }
        }
    }
}

/* NOTE: This is called from mstp_tether(), which is *always* called
 * in a critical region. Hence no locking needed.
 */
static void _llag_remove_port(struct mstp_aggr_obj const *aob, l2_port_no_t l2port)
{
    llag_participants_t *llag = (llag_participants_t *)aob->data_handle;
    u16                 ix = (u16)(l2port - aob->u.llag.port_offset);

    T_IG(_C, "%s remove port %d - ix %u", l2port2str(aob->l2port), l2port, ix);
    VTSS_ASSERT(MSTP_AGGR_GET_MEMBER(ix, llag) == 1);
    MSTP_AGGR_SET_MEMBER(ix, llag, 0);
}

static const mstp_aggr_objh_t _llag_handler = {_llag_count, _llag_first_port, _llag_next_port, _llag_update, _llag_remove_port};

mstp_aggr_obj_t *mstp_get_aggr(mstp_aggr_obj_t *paob, l2_port_no_t l2port)
{
    VTSS_ASSERT(!l2port_is_port(l2port));

    if (l2port_is_glag(l2port)) {
        VTSS_ASSERT(FALSE);
    } else {
        if (l2port2poag(l2port, &paob->u.llag.isid, &paob->u.llag.aggr_no)) {
            paob->u.llag.port_offset = L2PORT2PORT(paob->u.llag.isid, VTSS_PORT_NO_START);
            paob->data_handle = &mstp_global.aggr.llag[paob->u.llag.isid - VTSS_ISID_START][paob->u.llag.aggr_no - AGGR_MGMT_GROUP_NO_START];
            paob->handler = &_llag_handler;
        } else {
            T_E("L2port(%u) is NOT valid", l2port);
            return NULL;
        }
    }

    paob->l2port = l2port;
    return paob;
}

/* The values shown [in Table 17-3] apply to both full duplex and half
 * duplex operation. The intent of the recommended values and ranges
 * shown is to minimize the number of Bridges in which path costs need to
 * be managed to exert control over the topology of the Bridged Local
 * Area Network.
 */
static uint portspeed(mesa_port_speed_t speed)
{
    switch (speed) {
    case MESA_SPEED_10M:
        return 10;

    case MESA_SPEED_100M:
        return 100;

    case MESA_SPEED_1G:
        return 1000;

    case MESA_SPEED_2500M:
        return 2500;

    case MESA_SPEED_5G:
        return 5000;

    case MESA_SPEED_10G:
        return 10000;

    default:
        return 0;
    }
}

static uint aggrspeed(mstp_aggr_obj_t *pa)
{
    uint                    aspeed = 0, members = pa->handler->members(pa);
    l2_port_no_t            l2port;
    mesa_port_no_t          switchport;
    vtss_isid_t             isid;
    vtss_ifindex_t          ifindex;
    vtss_appl_port_status_t ps;

    VTSS_ASSERT(members > 0);
    for (l2port = pa->handler->first_port(pa); l2port != L2_NULL;
         l2port = pa->handler->next_port(pa, l2port)) {
        if (l2port2port(l2port, &isid, &switchport)                          &&
            msg_switch_exists(isid)                                          &&
            vtss_ifindex_from_port(isid, switchport, &ifindex) == VTSS_RC_OK &&
            vtss_appl_port_status_get(ifindex, &ps)            == VTSS_RC_OK &&
            ps.link) {
            aspeed += portspeed(ps.speed);
        }
    }

    T_DG(_C, "Aggregated speed: %u - avg %u", aspeed, aspeed / members);
    return aspeed;
}

const char *msti_name(vtss_appl_mstp_mstid_t msti)
{
    static const char *const mstinames[N_MSTI_MAX] = {
        "CIST", "MSTI1", "MSTI2", "MSTI3", "MSTI4", "MSTI5", "MSTI6", "MSTI7",
    };

    if (msti == VTSS_MSTID_TE) {
        return "TE";
    }

    return msti < N_MSTI_MAX ? mstinames[msti] : "?";
}

static inline char const *fwd2str(mstp_fwdstate_t state)
{
    switch (state) {
    case MSTP_FWDSTATE_BLOCKING:
        return "Discarding";    /* This is what STP calls it */

    case MSTP_FWDSTATE_LEARNING:
        return "Learning";

    case MSTP_FWDSTATE_FORWARDING:
        return "Forwarding";

    default:
        return "<unknown>";
    }
}

static void mstp_set_port_stpstate(l2_port_no_t portnum, mstp_fwdstate_t state)
{
    T_DG(_C, "Set %s state %s -> %s", l2port2str(portnum), fwd2str(mstp_global.aggr.stpstate[portnum]), fwd2str(state));

    if (mstp_global.aggr.stpstate[portnum] != state) {
        T_IG(_C, "Change %s state %s -> %s", l2port2str(portnum), fwd2str(mstp_global.aggr.stpstate[portnum]), fwd2str(state));

        switch (state) {
        case MSTP_FWDSTATE_BLOCKING:
            vtss_os_set_stpstate(portnum, MESA_STP_STATE_DISCARDING);
            break;

        case MSTP_FWDSTATE_LEARNING:
            vtss_os_set_stpstate(portnum, MESA_STP_STATE_LEARNING);
            break;

        case MSTP_FWDSTATE_FORWARDING:
            vtss_os_set_stpstate(portnum, MESA_STP_STATE_FORWARDING);
            break;

        default:
            abort();
        }

        mstp_global.aggr.stpstate[portnum] = state;
    }
}

static void mstp_aggr_sync_ports(l2_port_no_t portnum, mstp_aggr_obj_t *pa)
{
    l2_port_no_t l2port;

    VTSS_ASSERT(!l2port_is_port(portnum));

    /* Set STP state for all members */
    mstp_set_port_stpstate(portnum, MSTP_FWDSTATE_FORWARDING);
    for (l2port = pa->handler->first_port(pa); l2port != L2_NULL;
         l2port = pa->handler->next_port(pa, l2port)) {
        l2_sync_stpstates(l2port, portnum);
    }
}

static void mstp_set_all_stpstate(l2_port_no_t portnum, mstp_fwdstate_t state)
{
    mstp_set_port_stpstate(portnum, state);
    l2_set_msti_stpstate_all(portnum, (mesa_stp_state_t)state);
}

static void MSTP_port_ingress_filter(l2_port_no_t l2port, BOOL enable)
{
    mesa_port_no_t             switchport;
    vtss_isid_t                isid;
    vtss_appl_vlan_port_conf_t vlan_pconf;
    mesa_rc                    rc;

    if (MSTP_hw_support) {
        // Hardware supports MSTP table, no need to control VLAN ingress filtering
        return;
    }

    if (MSTP_lan966x) {
        // LAN966x controls ingress filtering per VLAN rather than per port,
        // because of MSTP's co-existence with FRER, where the FRER VLAN needs
        // to have ingress filtering disabled.
        return;
    }

    memset(&vlan_pconf, 0, sizeof(vlan_pconf));
    if (enable) {
        vlan_pconf.hybrid.flags = VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT;
        vlan_pconf.hybrid.ingress_filter = enable;
    }

    T_IG(_C, "Set l2port %s VLAN filtering %sabled", l2port2str(l2port), enable ? "en" : "dis");

    if (l2port2port(l2port, &isid, &switchport)) {
        /* The call might fail if we've become a secondary switch, or the
           administrator removed the switch from the stack (with
           "no switch stack <sid>") - benign */
        if ((rc = vlan_mgmt_port_conf_set(isid, switchport, &vlan_pconf, VTSS_APPL_VLAN_USER_MSTP)) != VTSS_RC_OK &&
            rc != VLAN_ERROR_MUST_BE_PRIMARY_SWITCH && rc != VLAN_ERROR_NOT_CONFIGURABLE && rc != VLAN_ERROR_ISID) {
            T_E("%u:%u: %s", isid, iport2uport(switchport), error_txt(rc));
        }
    } else {
        T_E("Set l2port %s VLAN filtering %d - not a port", l2port2str(l2port), enable);
    }
}

static void MSTP_vlan_ingress_filter(bool check_port_enabledness_only)
{
    // True whenever at least one port is STP enabled.
    static bool MSTP_at_least_one_port_enabled;

    // Array of 4K bits indicating whether we currently have ingress filtering
    // enabled for a certain VLAN ID. The trick is that - if we don't have H/W
    // MSTP support (MSTP_hw_support is false) - we need to enable ingress
    // filtering for all VLAN IDs except for those in the TE (Traffic
    // Engineering) MSTI (VTSS_MSTID_TE). The ones in the TE instance must not
    // be filtered out (unless the user has enabled ingress filtering on the
    // /port), because/ that will cause features like FRER to malfunction in the
    // FRER VLAN.
    static uint8_t MSTP_vid_ingress_filtering[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];

    uint64_t              start_ms;
    uint8_t               vid_ingress_filtering[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];
    mesa_vlan_vid_conf_t  vid_conf;
    uint32_t              cnt;
    bool                  at_least_one_port_enabled;
    mesa_port_no_t        port_no;
    mesa_vid_t            vid;
    mesa_rc               rc;

    if (MSTP_hw_support) {
        // Hardware supports MSTP table, no need to control VLAN ingress
        // filtering per VID.
        return;
    }

    if (!MSTP_lan966x) {
        // We are not running on LAN966x, and we do not need per-VLAN ingress
        // filtering, because that has potential undesired side-effects.
        // Instead, we use per-port ingress filtering (see
        // MSTP_port_ingress_filter()).
        return;
    }

    // On LAN966x, we use per-VLAN ingress filtering rather than per-port
    // ingress filtering, because we need that functionality for FRER and MSTP
    // co-existence, so that we have ingress filtering enabled on all VLANs,
    // except on the FRER VLAN(s). The end-user must then put FRER VLANs in the
    // special TE (Traffic Engineering) MSTI, which is not ingress filtered and
    // always forwarding.
    // The reason that not all platforms without MSTP H/W support use this
    // method is that it has a non-backwards compatible side-effect that goes
    // like this:
    //   If an end-user has put a port in VLAN hybrid mode (which is the only
    //   way to disable ingress filtering on the port, since both Access and
    //   Trunk modes have ingress filtering enabled with no support for changing
    //   it), it is probably because the end-user wants frames ingressing that
    //   port to be forwarded no matter what VLAN ID they get classified to.
    //   If, at the same time, at least one other port is running STP, ingress
    //   filtering would be enabled per VLAN ID, so the end-user would not
    //   obtain the desired functionality, because no matter what VLAN ID a
    //   frame will gete classified to (except for those in the TE MSTI), it
    //   will then get filtered (discarded).
    // Therefore, we are limiting this new functionality to LAN966x while
    // letting older platforms run the per-port ingress filtering.

    start_ms = vtss::uptime_milliseconds();

    at_least_one_port_enabled = false;
    for (port_no = 0; port_no < mstp_global.conf.portconfig.size(); port_no++) {
        if (port_no == MSTP_PORT_CONFIG_AGGR) {
            continue;
        }

        if (mstp_global.conf.stp_enable[port_no]) {
            at_least_one_port_enabled = true;
            break;
        }
    }

    if (check_port_enabledness_only && at_least_one_port_enabled == MSTP_at_least_one_port_enabled) {
        // There are not MSTI changes, so nothing else to do.
        T_I("No changes (check enabledness, only). %s", at_least_one_port_enabled ? "At least one port enabled" : "No ports enabled");
        return;
    }

    MSTP_at_least_one_port_enabled = at_least_one_port_enabled;

    // If at least one port is STP-enabled, we enable ingress filtering in all
    // VLANs except for those in the TE-MSTID.
    // If no ports are STP-enabled, we back out and disable ingress filtering in
    // all VLANs.
    if (at_least_one_port_enabled) {
        // If at least one port is STP-enabled, we expect to enable ingress
        // filtering in all VLANs, so set all bits in the bitmask, but don't use
        // memset(..., 0xFF, ...), because that will also set unused bits
        // in it. Instead use a function dedicated to this purpose.
        (void)vlan_mgmt_bitmask_all_ones_set(vid_ingress_filtering);

        if (mstp_global.conf.sys.forceVersion >= MSTP_PROTOCOL_VERSION_MSTP) {
            // We are running MSTP (not STP/RSTP), so filter out those VLANs
            // contained in the special TE (Traffic Engineering) MSTI, which are
            // always forwarding. If the user has asked the VLAN module to
            // enable ingress filtering, these VLANs will still be filtered out.
            for (vid = 1; vid < ARRSZ(mstp_global.conf.msti.map.map); vid++) {
                if (mstp_global.conf.msti.map.map[vid] == VTSS_MSTID_TE) {
                    T_I("Clearing ingress filtering for VID = %u", vid);
                    VTSS_BF_SET(vid_ingress_filtering, vid, 0);
                }
            }
        }
    } else {
        // Disable ingress filtering in all VLANs.
        vtss_clear(vid_ingress_filtering);
    }

    // Check if we have changes.
    if (memcmp(MSTP_vid_ingress_filtering, vid_ingress_filtering, sizeof(MSTP_vid_ingress_filtering)) == 0) {
        T_I("No changes (check MSTIs). %s", at_least_one_port_enabled ? "At least one port enabled" : "No ports enabled");
        return;
    }

    // Now send the changes to MESA.
    cnt = 0;
    for (vid = 1; vid < ARRSZ(mstp_global.conf.msti.map.map); vid++) {
        bool ingr_filter = VTSS_BF_GET(vid_ingress_filtering, vid);

        if (VTSS_BF_GET(MSTP_vid_ingress_filtering, vid) == ingr_filter) {
            // No changes
            continue;
        }

        cnt++;

        // Make sure that the calls to get() and set() are undivided
        VTSS_APPL_API_LOCK_SCOPE();
        if ((rc = mesa_vlan_vid_conf_get(NULL, vid, &vid_conf)) != VTSS_RC_OK) {
            T_E("mesa_vlan_vid_conf_get(%u) failed: %s", vid, error_txt(rc));
        }

        vid_conf.ingress_filter = ingr_filter;

        if ((rc = mesa_vlan_vid_conf_set(NULL, vid, &vid_conf)) != VTSS_RC_OK) {
            T_E("mesa_vlan_vid_conf_set(%u) failed: %s", vid, error_txt(rc));
        }
    }

    T_I("Changed ingress filtering for %u VIDs in " VPRI64u " ms. %s", cnt, vtss::uptime_milliseconds() - start_ms, at_least_one_port_enabled ? "At least one port enabled" : "No ports enabled");

    memcpy(MSTP_vid_ingress_filtering, vid_ingress_filtering, sizeof(MSTP_vid_ingress_filtering));
}

static void activate_port(l2_port_no_t l2port, u32 linkspeed, BOOL fdx, const char *reason)
{
    BOOL doadd = _vtss_mstp_port_added(mstp_global.mstpi, L2PORT2API(l2port)) != VTSS_RC_OK;

    T_I("%s", reason);

    /*
     * Enable *port* forwarding, but block all MSTI's.
     */
    mstp_set_port_stpstate(l2port, MSTP_FWDSTATE_FORWARDING);
    l2_set_msti_stpstate_all(l2port, (mesa_stp_state_t)MSTP_FWDSTATE_BLOCKING);

    /* Enable Ingress filtering. */
    MSTP_port_ingress_filter(l2port, TRUE);

    /* Add/kick the port */
    _vtss_mstp_stm_lock(mstp_global.mstpi);
    if (doadd) {
        if (_vtss_mstp_add_port(mstp_global.mstpi, L2PORT2API(l2port)) != VTSS_RC_OK) {
            T_EG(_C, "Error adding RSTP port %d - %s", l2port, l2port2str(l2port));
        }
    } else {
        if (_vtss_mstp_reinit_port(mstp_global.mstpi, L2PORT2API(l2port)) != VTSS_RC_OK) {
            T_EG(_C, "Error reinit RSTP port %d - %s", l2port, l2port2str(l2port));
        }
    }
    if (_vtss_mstp_port_enable(mstp_global.mstpi, L2PORT2API(l2port), TRUE, linkspeed, fdx) != VTSS_RC_OK) {
        T_EG(_C, "Error enabling RSTP port %u - %s at speed %u", l2port, l2port2str(l2port), linkspeed);
    }

    _vtss_mstp_stm_unlock(mstp_global.mstpi);
}

static void deactivate_port(l2_port_no_t l2port, mstp_fwdstate_t state, BOOL ingressfilter, const char *reason)
{
    if (reason) {
        T_IG(_C, "%s", reason);
    }

    if (_vtss_mstp_port_added(mstp_global.mstpi, L2PORT2API(l2port)) == VTSS_RC_OK) {  /* Must delete */
        (void)_vtss_mstp_delete_port(mstp_global.mstpi, L2PORT2API(l2port));
    }

    /* Set Ingress filtering. */
    MSTP_port_ingress_filter(l2port, ingressfilter);

    /* Set state (for all MSTI's) */
    mstp_set_all_stpstate(l2port, state);
}

/*
 * According to configuration & link state -
 * Instruct core RSTP/MSTP likewise
 */
static void port_sync(l2_port_no_t l2port, const vtss_appl_port_status_t *status)
{
    mesa_port_no_t          switchport;
    vtss_isid_t             isid;
    vtss_ifindex_t          ifindex;
    vtss_appl_port_status_t ps;
    BOOL                    enable;
    l2_port_no_t            l2aggr;

    if (l2port2port(l2port, &isid, &switchport)) {
        if (msg_switch_exists(isid)) {
            enable = mstp_global.conf.stp_enable[l2port];
            l2aggr = mstp_global.aggr.parent[l2port];

            T_IG(_C, "port %d enb %d added %d", l2port, enable, _vtss_mstp_port_added(mstp_global.mstpi, L2PORT2API(l2port)));
            if (l2aggr != L2_NULL) {
                /*
                 * Port is part of aggregation, just sync physical port to the
                 * aggregated port.
                 */
                T_IG(_C, "syncing aggregated port to %s", l2port2str(l2aggr));
                deactivate_port(l2port, mstp_global.aggr.stpstate[l2aggr], /* Use ingress filtering on aggregated port - IFF running STP */ mstp_global.conf.stp_enable[MSTP_PORT_CONFIG_AGGR], "aggr: sync up");
                return;
            }

            if (enable) {
                if (status) {
                    /* Link change */
                    if (status->link) {
                        activate_port(l2port, portspeed(status->speed), status->fdx, "portstate: link up");
                    } else {
                        deactivate_port(l2port, MSTP_FWDSTATE_BLOCKING, FALSE, "portstate: link down");
                    }
                } else {
                    /* Initial sync_up */
                    if (port_is_front_port(switchport)                                   &&
                        vtss_ifindex_from_port(isid, switchport, &ifindex) == VTSS_RC_OK &&
                        vtss_appl_port_status_get(ifindex, &ps)            == VTSS_RC_OK &&
                        ps.link) {
                        T_IG(_C, "[%d,%u] link: %d", isid, switchport, ps.link);
                        activate_port(l2port, portspeed(ps.speed), ps.fdx, "sync: link up");
                    } else {
                        deactivate_port(l2port, MSTP_FWDSTATE_BLOCKING, FALSE, "sync: no link/no port");
                    }
                }
            } else {
                deactivate_port(l2port, MSTP_FWDSTATE_FORWARDING, FALSE, "nonstp: fwd"); /* Just plain enable */
            }
        } else {
            deactivate_port(l2port, MSTP_FWDSTATE_BLOCKING, FALSE, "sync: no switch"); /* Switch gone - disable */
            mstp_global.aggr.parent[l2port] = L2_NULL;  /* Be sure to decouple from aggr */
        }
    }
}

static void sync_ports_switch(vtss_isid_t isid)
{
    port_iter_t pit;

    T_IG(_C, "sync switch %d", isid);

    MSTP_ASSERT_LOCKED();
    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        port_sync(L2PORT2PORT(isid, pit.iport), NULL);
    }
}

/*
 * Synchronize front port states (upon startup/restore defaults)
 */
static void sync_ports_all(void)
{
    switch_iter_t sit;

    _vtss_mstp_stm_lock(mstp_global.mstpi);
    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID);
    while (switch_iter_getnext(&sit)) {
        sync_ports_switch(sit.isid);
    }

    _vtss_mstp_stm_unlock(mstp_global.mstpi);
}

/*
 * Tether a Physical port into an aggregation
 */
static void mstp_tether(l2_port_no_t l2aggr, l2_port_no_t l2phys)
{
    mstp_aggr_obj_t aob, *paob;
    l2_port_no_t    oldparen;

    T_IG(_C, "%d enslaving %s", l2aggr, l2port2str(l2phys));

    if (mstp_global.aggr.parent[l2phys] != l2aggr) {
        if (mstp_global.aggr.parent[l2phys] != L2_NULL) {
            oldparen = mstp_global.aggr.parent[l2phys];

            T_IG(_C, "*** Aggregated port changing parent! %d had %s as parent", l2phys, l2port2str(oldparen));

            paob = mstp_get_aggr(&aob, oldparen);
            if (paob) {
                paob->handler->remove_port(paob, l2phys);
                if (!MSTP_AGGR_GETSET_CHANGE(oldparen, 1)) {
                    T_WG(_C, "Revisit %s for update", l2port2str(oldparen));
                    vtss_flag_setbits(&mstp_global.control_flags, CTLFLAG_MSTP_AGGRCHANGE);
                }
            }
        }

        mstp_global.aggr.parent[l2phys] = l2aggr;
    }

    /*
     * Stop the l2 physical STP port, and sync to STP state for the
     * aggregated port
     */
    deactivate_port(l2phys, mstp_global.aggr.stpstate[l2aggr], mstp_global.conf.stp_enable[MSTP_PORT_CONFIG_AGGR], "tether");

    /* Now sync the physical port MSTIs to the aggregated port MSTIs */
    l2_sync_stpstates(l2phys, l2aggr);
}

/*
 * Liberate a Physical port from an aggregation
 */
static void mstp_liberate(l2_port_no_t l2aggr, l2_port_no_t l2phys)
{
    T_IG(_C, "%d liberating %s", l2aggr, l2port2str(l2phys));
    mstp_global.aggr.parent[l2phys] = L2_NULL;
    port_sync(l2phys, NULL);
}

static void save_config(void)
{
#if defined(VTSS_MSTP_FULL)
    /* No limitations */
#else
    if (mstp_global.conf.sys.forceVersion > MSTP_PROTOCOL_VERSION_RSTP) {
        /* Only allowed to use RSTP/STP */
        mstp_global.conf.sys.forceVersion = MSTP_PROTOCOL_VERSION_RSTP;
    }
#endif  /* VTSS_MSTP_FULL */

    vtss_flag_setbits(&mstp_global.control_flags, CTLFLAG_MSTP_CONFIG_CHANGE);
}

/*
 * Update internal MSTI state, and apply MSTI configuration to base MSTP.
 */
static mesa_rc mstp_apply_msticonfig(void)
{
    mstp_msti_t msti;
    BOOL        single_mode;
    mesa_rc     rc;

    MSTP_ASSERT_LOCKED();

    /* Reset state */
    mstp_global.traps = 0;

    _vtss_mstp_stm_lock(mstp_global.mstpi); /* Don't run STMs while applying */

    if ((rc = _vtss_mstp_set_bridge_parameters(mstp_global.mstpi, &mstp_global.conf.sys) == VTSS_RC_OK) &&
        (rc = _vtss_mstp_set_config_id(mstp_global.mstpi,
                                       mstp_global.conf.msti.configname,
                                       mstp_global.conf.msti.revision)) == VTSS_RC_OK &&
        (rc = _vtss_mstp_set_mapping(mstp_global.mstpi, &mstp_global.conf.msti.map)) == VTSS_RC_OK) {
        single_mode = (mstp_global.conf.sys.forceVersion < MSTP_PROTOCOL_VERSION_MSTP);

        rc = l2_set_msti_map(single_mode,
                             ARRSZ(mstp_global.conf.msti.map.map),
                             mstp_global.conf.msti.map.map);

        // Check if this gave rise to a different set of VLANs to enable
        // ingress filtering for.
        MSTP_vlan_ingress_filter(false /* also check for TE MSTI changes */);

        // Set TE MSTI to forwarding
        if (rc == VTSS_RC_OK) {
            vtss_appl_mstp_msti_t te_msti = l2_mstid2msti(VTSS_MSTID_TE);
            port_iter_t pit;
            (void)port_iter_init_local(&pit);
            while (port_iter_getnext(&pit)) {
                if ((rc = mesa_mstp_port_msti_state_set(NULL,
                                                        pit.iport,
                                                        te_msti,
                                                        MESA_STP_STATE_FORWARDING)) != VTSS_RC_OK) {
                    T_E("Port %u: TE MSTI set to forwarding failed: %s", iport2uport(pit.iport), error_txt(rc));
                    break;
                }
            }
        }
    }

    for (msti = 0; msti < N_MSTI_MAX; msti++) {
        (void)_vtss_mstp_set_bridge_priority(mstp_global.mstpi,
                                             msti,
                                             mstp_global.conf.bridgePriority[msti]);
    }

    _vtss_mstp_stm_unlock(mstp_global.mstpi);

    T_I("Operation %s", rc == VTSS_RC_OK ? "succedded" : "failed");

    return rc;
}

/*
 * Propagate the MSTP (module) configuration to the MSTP/RSTP core
 * library.
 */
static void mstp_conf_propagate(BOOL bridge, BOOL ports)
{
    uint                    i;
    int                     msti;
    const mstp_port_param_t *pconf;
    mstp_msti_port_param_t  *mpp;

    MSTP_ASSERT_LOCKED();
    VTSS_ASSERT(mstp_global.mstpi != NULL);

    /* Make effective in MSTP core */
    if (bridge) {
        (void)mstp_apply_msticonfig();
    }

    if (ports) {
        for (i = MSTP_CONF_PORT_FIRST; i <= MSTP_CONF_PORT_LAST; i++) {
            pconf = &mstp_global.conf.portconfig[i];
            (void)_vtss_mstp_set_port_parameters(mstp_global.mstpi, L2PORT2API(i), pconf);

            for (msti = 0; msti < N_MSTI_MAX; msti++) {
                mpp = &mstp_global.conf.msticonfig[i][msti];
                (void)_vtss_mstp_set_msti_port_parameters(mstp_global.mstpi, msti, L2PORT2API(i), mpp);
            }
        }
    }
}

/*
 * Read the MSTP/RSTP configuration. @create indicates a new default
 * configuration block should be created.
 */
static void mstp_conf_default(void)
{
    uint i, j;

    /* Use default configuration */
    vtss_clear(mstp_global.conf);
    mstp_global.conf.sys.bridgeMaxAge = 20; /* 17.14 - Table 17-1: Default recommended value */
    mstp_global.conf.sys.bridgeHelloTime = 2; /* 17.14 - Table 17-1: Default recommended value */
    mstp_global.conf.sys.bridgeForwardDelay = 15; /* 17.14 - Table 17-1: Default recommended value */
    mstp_global.conf.sys.forceVersion = (mstp_forceversion_t)3; /* 17.13.4 - The normal, default value */
    mstp_global.conf.sys.txHoldCount = 6; /* 17.14 - Table 17-1: Default recommended value */
    mstp_global.conf.sys.MaxHops = 20; /* 13.37.3 MaxHops */

    /* Get System MAC address */
    (void)conf_mgmt_mac_addr_get(mstp_global.sysmac.mac, 0);
    (void)misc_mac_txt(mstp_global.sysmac.mac, mstp_global.conf.msti.configname);

    mstp_global.conf.msti.revision = 0;
    for (i = 0; i < N_MSTI_MAX; i++) {
        mstp_global.conf.bridgePriority[i] = 0x80;    /* 17.14 - Table 17-2: Default recommended value */
    }

    for (i = 0; i < mstp_global.conf.portconfig.size(); i++) {
        mstp_port_param_t *pp = &mstp_global.conf.portconfig[i];
        mstp_global.conf.stp_enable[i] = TRUE;
        pp->adminEdgePort = FALSE;
        pp->adminAutoEdgePort = TRUE;
        pp->adminPointToPointMAC = P2P_AUTO;
        for (j = 0; j < N_MSTI_MAX; j++) {
            mstp_msti_port_param_t *mpp = &mstp_global.conf.msticonfig[i][j];
            mpp->adminPathCost = MSTP_PORT_PATHCOST_AUTO; /* 0 = Auto */
            mpp->adminPortPriority = 0x80; /* 17.14 - Table 17-2: Default recommended value */
        }
    }

    // Check if this enabling of STP on ports/change of MSTIs gives rise to
    // per-VLAN ingress filtering changes
    MSTP_vlan_ingress_filter(false /* also check for TE MSTI changes */);

    /* Use different defaults for aggregated ports */
    mstp_global.conf.stp_enable[MSTP_PORT_CONFIG_AGGR] = TRUE;
    mstp_global.conf.portconfig[MSTP_PORT_CONFIG_AGGR].adminPointToPointMAC = P2P_FORCETRUE;

#if defined(VTSS_MSTP_FULL)
    /* No limitations */
#else
    if (mstp_global.conf.sys.forceVersion > MSTP_PROTOCOL_VERSION_RSTP) {
        /* Only allowed to use RSTP/STP */
        mstp_global.conf.sys.forceVersion = MSTP_PROTOCOL_VERSION_RSTP;
    }
#endif  /* VTSS_MSTP_FULL */
}

/****************************************************************************
 * Utility
 ****************************************************************************/

// Append string to str safely.
static size_t strfmt_append(char *str, size_t size /* Size of str */, const char *fmt, ...)
{
    va_list ap = {};
    size_t len = strlen(str);
    size_t cnt;

    VTSS_ASSERT(len < size);

    va_start(ap, fmt);
    cnt = vsnprintf(&str[len], size - len, fmt, ap);
    va_end(ap);

    return cnt;
}

// Convert mstimap array to string of type "1,3,4-16,25-48,49,51"
char *mstp_mstimap2str(const mstp_msti_config_t *conf, vtss_appl_mstp_mstid_t mstid, char *buf, size_t bufsize)
{
    mesa_vid_t vid;
    mesa_vid_t vid_start       = 0;
    mesa_vid_t vid_end         = 0;
    BOOL       vid_start_found = 0;
    BOOL       first_range = 1;

    buf[0] = '\0';

    for (vid = 0; vid < ARRSZ(conf->map.map); vid++) {
        if (conf->map.map[vid] == mstid) {
            // Vid present in mask
            if (!vid_start_found) {
                // New range
                vid_start = vid;
                vid_start_found = 1;
            } else if (vid != vid_end + 1) {
                // End of range

                if (!first_range) {
                    (void)strfmt_append(buf, bufsize, ",");
                }

                if (vid_start == vid_end) {
                    // Only one vid in range
                    (void)strfmt_append(buf, bufsize, "%d", vid_start);
                } else {
                    // Two or more vids in range
                    (void)strfmt_append(buf, bufsize, "%d-%d", vid_start, vid_end);
                }

                vid_start = vid;
                first_range = 0;
            }

            vid_end = vid;
        }
    }

    /* Finish off */
    if (vid_start_found) {
        if (!first_range) {
            (void)strfmt_append(buf, bufsize, ",");
        }

        if (vid_start == vid_end) {
            // Only one vid in range
            (void)strfmt_append(buf, bufsize, "%d", vid_start);
        } else {
            // Two or more vids in range
            (void)strfmt_append(buf, bufsize, "%d-%d", vid_start, vid_end);
        }
    }

    return buf;
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

/*
 * Port state change indication
 */
static void mstp_port_state_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    vtss_isid_t  isid = VTSS_ISID_START;
    l2_port_no_t l2port, l2aggr;

    MSTP_LOCK();
    if (MSTP_READY()) {
        l2port = L2PORT2PORT(isid, port_no);
        l2aggr = mstp_global.aggr.parent[l2port];
        T_IG(_C, "port_no: [%d,%u] = %u - link %s", isid, port_no, l2port, status->link ? "up" : "down");

        if (l2aggr != L2_NULL) {
            /* Update membership/speed in bulk */
            T_IG(_C, "aggr reconfig port %d => l2aggr %d (%s)", l2port, l2aggr, l2port2str(l2aggr));
            MSTP_AGGR_SET_CHANGE(l2aggr);
            vtss_flag_setbits(&mstp_global.control_flags, CTLFLAG_MSTP_AGGRCHANGE);
        } else {
            /* ANIELSEN: Why only notify non-mstp ports on link up? */
            if (mstp_global.conf.stp_enable[l2port] || status->link) {
                port_sync(l2port, status);    /* The STP ports + nonstp coming up */
            }
        }
    } else {
        T_DG(_C, "LOST portstate callback: [%d,%u] link %s", isid, port_no, status->link ? "up" : "down");
    }

    MSTP_UNLOCK();
}

/*
 * Local port packet receive indication - forward through L2 interface
 */
static BOOL RX_mstp(void *contxt, const uchar *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    // For us. Send back through L2 stack-wide interface.
    T_RG(_C, "port_no: %d len %d vid %d tagt %d", rx_info->port_no, rx_info->length, rx_info->tag.vid, rx_info->tag_type);

    // NB: Core MSTP doesn't like to receive on aggregations, so null out the GLAG (port is 1st in aggr)
    if (rx_info->tag_type == MESA_TAG_TYPE_UNTAGGED) {
        l2_receive_indication(VTSS_MODULE_ID_MSTP, frm, rx_info->length, rx_info->port_no, rx_info->tag.vid, VTSS_GLAG_NO_NONE /* Zap GLAG! */);
    }

    // Allow other subscribers to receive the packet
    return FALSE;
}

/*
 * L2 Packet receive indication
 */
static void mstp_stack_receive(const void *packet, size_t len, mesa_vid_t vid, l2_port_no_t l2port)
{
    l2_port_no_t l2paren;
    BOOL         enable;

    T_NG(_I, "RX port %d len %zd", l2port, len);

    MSTP_LOCK();
    if (MSTP_READY()) {
        /* Physical RX port is aggregated? */
        l2paren = mstp_global.aggr.parent[l2port];
        enable  = (l2paren != L2_NULL ? mstp_global.conf.stp_enable[MSTP_PORT_CONFIG_AGGR] : mstp_global.conf.stp_enable[l2port]);

        if (l2paren != L2_NULL) {
            T_NG(_I, "Map RX port %d -> %d, len %zd", l2port, l2paren, len);
            if (!enable) {
                T_IG(_I, "Receiving BPDU on aggregated port %d - %s, STP on aggrs disabled",
                     l2port, l2port2str(l2port));
            }

            l2port = l2paren;
        } else {
            if (!enable) {
                T_IG(_I, "Receiving BPDU on port %d - %s, but STP is disabled",
                     l2port, l2port2str(l2port));
            }
        }

        /* Consume through MSTP core */
        if (enable) {
            _vtss_mstp_rx(mstp_global.mstpi, L2PORT2API(l2port), packet, len);
        }
    }

    MSTP_UNLOCK();
}

/****************************************************************************
 * Aggregation Interfacing
 ****************************************************************************/

static inline l2_port_no_t aggr_to_l2port(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no)
{
    l2_port_no_t l2aggr = L2LLAG2PORT(isid, aggr_no - AGGR_MGMT_GROUP_NO_START);
    return l2aggr;
}

/*
 * Signal Aggregation as Changed
 */
void mstp_aggr_reconfigured(vtss_isid_t isid, uint aggr_no)
{
    l2_port_no_t l2aggr = aggr_to_l2port(isid, aggr_no);

    if (MSTP_READY()) {
        MSTP_LOCK();
        T_DG(_C, "aggr reconfig isid %d aggr %d => l2aggr %d (%s)", isid, aggr_no, l2aggr, l2port2str(l2aggr));
        MSTP_AGGR_SET_CHANGE(l2aggr);
        vtss_flag_setbits(&mstp_global.control_flags, CTLFLAG_MSTP_AGGRCHANGE);
        MSTP_UNLOCK();
    } else {
        T_WG(_C, "LOST aggr reconfig isid %d aggr %d => l2aggr %d (%s)", isid, aggr_no, l2aggr, l2port2str(l2aggr));
    }
}

/*
 * Activate/Stop aggregation
 */
static void mstp_aggr_sync(mstp_aggr_obj_t *pa, l2_port_no_t l2aggr)
{
    BOOL              portadded = (_vtss_mstp_port_added(mstp_global.mstpi, L2PORT2API(l2aggr)) == VTSS_RC_OK);
    mstp_port_param_t *pconf;
    uint              linkspeed;
    int               msti;
    l2_port_no_t      l2port;

    if (mstp_global.conf.stp_enable[MSTP_PORT_CONFIG_AGGR]) { /* Run RSTP on aggr's? */
        if (pa->handler->members(pa)) {
            pconf = &mstp_global.conf.portconfig[MSTP_PORT_CONFIG_AGGR];
            linkspeed = aggrspeed(pa);

            T_IG(_C, "Add l2aggr %d, %u members, speed %u", l2aggr, pa->handler->members(pa), linkspeed);
            if (!portadded) {
                if (_vtss_mstp_add_port(mstp_global.mstpi, L2PORT2API(l2aggr)) != VTSS_RC_OK) {
                    T_EG(_C, "Error adding RSTP aggregation: %d - %s", l2aggr, l2port2str(l2aggr));
                }
            }

            if (_vtss_mstp_set_port_parameters(mstp_global.mstpi, L2PORT2API(l2aggr), pconf) != VTSS_RC_OK) {
                T_EG(_C, "Error configuring RSTP aggregation: %d - %s", l2aggr, l2port2str(l2aggr));
            }

            /* Apply MSTI config */
            for (msti = 0; msti < N_MSTI_MAX; msti++) {
                mstp_msti_port_param_t *mpp = &mstp_global.conf.msticonfig[MSTP_PORT_CONFIG_AGGR][msti];
                (void)_vtss_mstp_set_msti_port_parameters(mstp_global.mstpi, msti, L2PORT2API(l2aggr), mpp);
            }

            if (_vtss_mstp_port_enable(mstp_global.mstpi, L2PORT2API(l2aggr), TRUE, linkspeed, TRUE) != VTSS_RC_OK) {
                T_EG(_C, "Error enabling RSTP aggregation %d - %s at speed %d", l2aggr, l2port2str(l2aggr), linkspeed);
            }

            /*
             * Enable *port* forwarding, but block all MSTI's.
             */
            mstp_aggr_sync_ports(l2aggr, pa);
        } else {
            T_IG(_C, "Delete l2aggr %d - %s", l2aggr, l2port2str(l2aggr));
            if (portadded) {
                (void)_vtss_mstp_delete_port(mstp_global.mstpi, L2PORT2API(l2aggr));
            }
        }
    } else {
        /* Delete the MSTP port (if we had one) */
        if (portadded) {
            (void)_vtss_mstp_delete_port(mstp_global.mstpi, L2PORT2API(l2aggr));
        }

        /* Enable forwarding statically for non-stp aggr (for later port sync-p)*/
        mstp_set_port_stpstate(l2aggr, MSTP_FWDSTATE_FORWARDING);

        /* Enable current members */
        for (l2port = pa->handler->first_port(pa); l2port != L2_NULL; l2port = pa->handler->next_port(pa, l2port)) {
            mstp_set_all_stpstate(l2port, MSTP_FWDSTATE_FORWARDING);
        }
    }
}

/*
 * Reconfigure Aggregations - we are *locked* here!
 */
static void mstp_aggr_reconfigure(BOOL all)
{
    l2_port_no_t    l2aggr;
    mstp_aggr_obj_t aob, *paob;

    T_DG(_C, "Check Aggregated Poags - Start");

    MSTP_ASSERT_LOCKED();
    for (l2aggr = L2_MAX_PORTS_; l2aggr < L2_MAX_POAGS_; l2aggr++) {
        if (all || MSTP_AGGR_GETSET_CHANGE(l2aggr, 0)) {
            paob = mstp_get_aggr(&aob, l2aggr);
            if (paob) {
                T_DG(_C, "Check Port %d - %s, initially %d members", l2aggr, l2port2str(l2aggr), paob->handler->members(paob));
                paob->handler->update(paob);
                T_DG(_C, "Check Port %d - %s, now %d members", l2aggr, l2port2str(l2aggr), paob->handler->members(paob));
                mstp_aggr_sync(paob, l2aggr);
            }
        }
    }

    T_DG(_C, "Check Aggregated Poags - Done");
}

static void mstp_call_trap_sink(void)
{
    u32              traps;
    mstp_trap_sink_t cb;

    MSTP_LOCK();
    cb = mstp_global.trap_cb;
    traps = mstp_global.traps;
    mstp_global.traps = 0;      /* Reset traps */
    MSTP_UNLOCK();  /* Unlock mstp semaphore before calling snmp */

    if (traps && cb) {
        cb(traps);
    }
}

/****************************************************************************
 * Module thread
 ****************************************************************************/

/**
 * mstp_primary_switch_initialize - initialize MSTP state when starting as
 * stack primary switch.
 *
 * Function called by main mstp thread - locked - exit locked.
 *
 */
static void mstp_primary_switch_initialize(void)
{
    uint i;

    MSTP_ASSERT_LOCKED();

    /* Get System MAC address */
    (void)conf_mgmt_mac_addr_get(mstp_global.sysmac.mac, 0);

    /* Initialize MSTP */
    mstp_global.mstpi = _vtss_mstp_create_bridge(&mstp_global.sysmac, MSTP_BRIDGE_PORTS);

    /* Propagate system config */
    mstp_conf_propagate(TRUE, TRUE);

    /* Sync port states */
    vtss_clear(mstp_global.aggr);

    for (i = 0; i < mstp_global.aggr.parent.size(); i++) {
        mstp_global.aggr.parent[i] = L2_NULL;
    }

    mstp_global.ready = TRUE; /* Ready to rock - allow portstate callbacks */
}

/**
 * mstp_primary_switch_process - process MSTP main tasks while stack primary switch.
 *
 * Function called by main mstp thread - unlocked.
 *
 * Terminates when becoming secondary switch - unlocked.
 */
static void mstp_primary_switch_process(void)
{
    vtss_tick_count_t wakeup;
    vtss_flag_value_t flags;
    int               i;

    while (msg_switch_is_primary()) {
        T_RG(_C, "tick()");

        MSTP_LOCK();    /* Lock while ticking */
        _vtss_mstp_tick(mstp_global.mstpi);
        MSTP_UNLOCK();  /* MSTP API available again */

        mstp_call_trap_sink();
        wakeup = vtss_current_time() + VTSS_OS_MSEC2TICK(1000);

        while ((flags = vtss_flag_timed_wait(&mstp_global.control_flags, 0xffff, VTSS_FLAG_WAITMODE_OR_CLR, wakeup))) {
            T_IG(_C, "MSTP thread event, flags 0x%x", flags);

            MSTP_LOCK(); /* Process flags while locked */

            if (flags & CTLFLAG_MSTP_AGGRCHANGE) {
                mstp_aggr_reconfigure(FALSE);    /* One or more AGGR's changed */
            }

            if (flags & CTLFLAG_MSTP_AGGRCONFIG) {
                mstp_aggr_reconfigure(TRUE);    /* All AGGR's changed */
            }

            if (flags & CTLFLAG_MSTP_DEFCONFIG) {
                mstp_conf_default(); /* Reset stack configuration */
                /* Make RSTP configuration effective in RSTP core */
                mstp_conf_propagate(TRUE, TRUE);
                /* Synchronize port states */
                sync_ports_all();
                T_D("Posting load defaults semaphore");
                vtss_sem_post(&mstp_global.defconfig_sema);
            }

            MSTP_UNLOCK(); /* Unlock to go back to sleep */

            /* Callbacks while *NOT* locked */
            if (flags & CTLFLAG_MSTP_CONFIG_CHANGE) {
                for (i = 0; i < ARRSZ(mstp_global.config_cb); i++) {
                    mstp_config_change_cb_t cb = mstp_global.config_cb[i];
                    if (cb) {
                        cb();
                    }
                }
            }
        }
    }
}

/*lint -sem(mstp_thread, thread_protected) */
static void mstp_thread(vtss_addrword_t data)
{
    packet_rx_filter_t rx_filter;
    void               *filter_id = NULL;
    mesa_rc            rc;

    // Wait until ICFG_LOADING_PRE event.
    msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_PRE, VTSS_MODULE_ID_MSTP);

    /* Note - locked here! */
    /*lint --e{455,456} ... Lock/unlock is suddle, but *carefully* designed */
    MSTP_ASSERT_LOCKED();

    /* MSTP frames registration */
    packet_rx_filter_init(&rx_filter);
    rx_filter.modid = VTSS_MODULE_ID_MSTP;
    rx_filter.match = PACKET_RX_FILTER_MATCH_DMAC;
    memcpy(rx_filter.dmac, ieee_bridge, sizeof(rx_filter.dmac));
    rx_filter.cb    = RX_mstp;
    rx_filter.prio  = PACKET_RX_FILTER_PRIO_NORMAL;
    rc = packet_rx_filter_register(&rx_filter, &filter_id);
    VTSS_ASSERT(rc == VTSS_RC_OK);
    l2_receive_register(VTSS_MODULE_ID_MSTP, mstp_stack_receive);

    /* Port change callback */
    (void)port_change_register(VTSS_MODULE_ID_MSTP, mstp_port_state_change_callback);

    /* AGGR config change callback */
    aggr_change_register(mstp_aggr_reconfigured);

    for (;;) {
        MSTP_ASSERT_LOCKED();   /* Locked at entry - and each time looping */

        if (msg_switch_is_primary()) {

            mstp_primary_switch_initialize();

            MSTP_UNLOCK(); /* We were locked initializing - but open here */

            mstp_primary_switch_process(); /* Process while being primary switch */

            MSTP_LOCK(); /* Lock outer airlock when becoming secondary switch */
        }

        /* Note - still locked! */
        MSTP_ASSERT_LOCKED();

        mstp_global.ready = FALSE; /* Done rocking */

        if (mstp_global.mstpi) {
            /* De-Initialize MSTP core */
            (void)_vtss_mstp_delete_bridge(mstp_global.mstpi);
            mstp_global.mstpi = NULL;
        }

        MSTP_UNLOCK();

        T_IG(_C, "Suspending MSTP thread");
        msg_wait(MSG_WAIT_UNTIL_ICFG_LOADING_PRE, VTSS_MODULE_ID_MSTP);
        T_IG(_C, "Restarting MSTP thread (became primary switch)");

        MSTP_LOCK(); /* Lock outer airlock when waking up again */
    }
}

/****************************************************************************/
/*  MSTP callout functions                                                  */
/****************************************************************************/

/**
 * BPPDU transmit.
 *
 * \param portnum The physical port on which to send the BPDU.
 *
 * \param buffer The BPDU to transmit.
 *
 * \param size The length of the BPDU buffer.
 */
void vtss_mstp_tx(uint portnum, void *buffer, size_t size)
{
    uchar           *osbuf;
    vtss_isid_t     isid;
    mesa_port_no_t  switchport;
    mstp_aggr_obj_t aob, *paob;

    /* Convert to base-zero */
    portnum = API2L2PORT(portnum);

    T_NG(_I, "Port %d - %s, tx %zd bytes", portnum, l2port2str(portnum), size);

    if (!l2port_is_port(portnum)) { /* Map aggregation to first port number */
        paob = mstp_get_aggr(&aob, portnum);
        if (paob) {
            portnum = paob->handler->first_port(paob);
            if (portnum == L2_NULL) {
                return;    /* No ports contained atm? */
            }
        }
    }

    VTSS_ASSERT(l2port2port(portnum, &isid, &switchport));

    osbuf = (uchar *)vtss_os_alloc_xmit(portnum, size);
    if (osbuf) {
        uchar *basemac = mstp_global.sysmac.mac;
        memcpy(osbuf, buffer, size);
        misc_instantiate_mac(osbuf + 6, basemac, switchport + 1 - VTSS_PORT_NO_START); /* entry 0 is the CPU port */
        (void)vtss_os_xmit(portnum, osbuf, size);
    }
}

void vtss_mstp_log(const char *message, u32 port)
{
    u32 portnum = API2L2PORT(port);
    char portbuf[L2_PORT_NAME_MAX];
    const char *port_str;

    if (port) {
        /* Convert to base-zero */
        portnum = API2L2PORT(port);
        port_str = l2port2str_icli(portnum, portbuf, sizeof(portbuf));

        T_UNSAFE(VTSS_TRACE_GRP_DEFAULT, VTSS_TRACE_LVL_WARNING, message, port_str);

#ifdef VTSS_SW_OPTION_SYSLOG
        S_W(message, port_str);
#endif
    } else {
        T_W("%s", message);
#ifdef VTSS_SW_OPTION_SYSLOG
        S_W("%s", message);
#endif
    }
}

/**
 * Switch interface access - set forwarding state.
 * \param portnum The physical port to control
 * \param state The state to set
 */
void vtss_mstp_port_setstate(uint portnum, mstp_msti_t msti, mstp_fwdstate_t state)
{
    l2_port_no_t    l2port;
    mstp_aggr_obj_t aob, *paob;

    /* Convert to base-zero */
    portnum = API2L2PORT(portnum);

    T_IG(_I, "Port %d[%d] - %s, FwdState %s", portnum, msti, l2port2str(portnum), fwd2str(state));
    if (l2port_is_port(portnum)) {
        l2_set_msti_stpstate(msti, portnum, (mesa_stp_state_t)state);
    } else {
        l2_set_msti_stpstate(msti, portnum, (mesa_stp_state_t)state); /* Keep aggr state */
        paob = mstp_get_aggr(&aob, portnum);
        if (paob) {
            for (l2port = paob->handler->first_port(paob); l2port != L2_NULL;
                 l2port = paob->handler->next_port(paob, l2port)) {
                l2_set_msti_stpstate(msti, l2port, (mesa_stp_state_t)state);
            }
        }
    }
}

/**
 * Switch interface access - \e MAC \ table..
 * \param portnum The physical port to flush
 *
 * \note In MSTI operation, the current implementation will flush more
 * than strictly necessary (all VLANS are flushed).
 */
void vtss_mstp_port_flush(uint portnum, mstp_msti_t msti)
{
    /* Convert to base-zero */
    portnum = API2L2PORT(portnum);

    T_IG(_I, "Flush Port %d[%d] - %s", portnum, msti, l2port2str(portnum));
    l2_flush_port(portnum);
}

/**
 * VLAN interface access - determine port MSTI membership
 *
 * \param portnum The physical port to query
 *
 * \param msti The MSTI instance to query for membership
 *
 * \return TRUE if the port is a member of the MSTI.
 */
BOOL vtss_mstp_port_member(uint portnum, mstp_msti_t msti)
{
    return TRUE;
}

/**
 * Time interface - get current time
 *
 * \return the current time of day in seconds (relative to an
 * arbitrary absolute time)
 */
u32 vtss_mstp_current_time(void)
{
    struct sysinfo info;
    sysinfo(&info);
    return (u32)info.uptime;
}

/**
 * Callout from MSTP - Trap event occurred. We're delivering this to
 * trap sink (if any) in a batched fashion. (After tick).
 *
 * Note: We're locked already.
 */
void vtss_mstp_trap(mstp_msti_t msti, mstp_trap_event_t event)
{
    mstp_global.traps |= (1 << (uint)event);
}

/**
 * Callout from MSTP - Allocate memory.
 */
void *vtss_mstp_malloc(size_t sz)
{
    return VTSS_MALLOC(sz);
}

/**
 * Callout from MSTP - Free memory.
 */
void vtss_mstp_free(void *ptr)
{
    VTSS_FREE(ptr);
}

/**
 * Callout from MSTP - Port role transition.
 * This implementation can be overridden.
 */

#ifdef __cplusplus
extern "C" {
#endif

static void __vtss_mstp_port_setrole(uint portnum, u8 msti, vtss_mstp_portrole_t old_role, vtss_mstp_portrole_t new_role) __asm__ ("__vtss_mstp_port_setrole");
void vtss_mstp_port_setrole(uint portnum, u8 msti, vtss_mstp_portrole_t old_role, vtss_mstp_portrole_t new_role) __attribute__ ((weak, alias("__vtss_mstp_port_setrole")));

static void __vtss_mstp_port_setrole(uint portnum, u8 msti, vtss_mstp_portrole_t old_role, vtss_mstp_portrole_t new_role)
{
    T_D("Port %s:%s - role %d -> %d", msti_name(msti), l2port2str(portnum), old_role, new_role);
}

#ifdef __cplusplus
}
#endif

/****************************************************************************/
/*  API functions                                                           */
/****************************************************************************/

mesa_rc vtss_appl_mstp_system_config_get(mstp_bridge_param_t *pconf)
{
    mesa_rc rc;

    if (msg_switch_is_primary()) {
        MSTP_LOCK();
        *pconf = mstp_global.conf.sys;
        MSTP_UNLOCK();

        rc = VTSS_RC_OK;
    } else {
        rc = VTSS_RC_INV_STATE;
    }

    return rc;
}

mesa_rc vtss_appl_mstp_system_config_set(const mstp_bridge_param_t *pconf)
{
    BOOL    single_mode;
    mesa_rc rc;

    if (!msg_switch_is_primary()) {
        return VTSS_RC_ERROR;
    }

    if ((pconf->bridgeMaxAge < 6 || pconf->bridgeMaxAge > 40) ||
        (pconf->bridgeForwardDelay < 4 || pconf->bridgeForwardDelay > 30) ||
        (pconf->bridgeMaxAge > ((pconf->bridgeForwardDelay - 1) * 2))) {
        T_I("Attempt to set illegal system timers: MaxAge %u, FwdDelay %u", pconf->bridgeMaxAge, pconf->bridgeForwardDelay);
        return VTSS_RC_ERROR;
    }
    if (mstp_global.conf.sys.bridgeHelloTime != pconf->bridgeHelloTime) {
        if (pconf->bridgeHelloTime < 1 || pconf->bridgeHelloTime > 10) {
            T_I("Illegal bridgeHelloTime: %d", pconf->bridgeHelloTime);
            return VTSS_RC_ERROR;
        }

        T_I("Changing bridgeHelloTime to %d", pconf->bridgeHelloTime);
    }
    if (pconf->MaxHops < 6 || pconf->MaxHops > 40 ||
        pconf->txHoldCount < 1 || pconf->txHoldCount > 10 ||
        (pconf->errorRecoveryDelay > 0 && pconf->errorRecoveryDelay < 30) ||
        pconf->errorRecoveryDelay > (60 * 60 * 24)) {
        T_I("Attempt to set illegal bridge params: MaxHops %u, txHoldCount %u, errorRecoveryDelay %u", pconf->MaxHops, pconf->txHoldCount, pconf->errorRecoveryDelay);
        return VTSS_RC_ERROR;
    }

    rc = VTSS_RC_OK;

    MSTP_LOCK();
    if (memcmp(&mstp_global.conf.sys, pconf, sizeof(*pconf)) != 0) {
        mstp_global.conf.sys = *pconf;
        save_config();

        /* Propagate system config */
        rc = _vtss_mstp_set_bridge_parameters(mstp_global.mstpi, pconf);
        if (rc == VTSS_RC_OK) {
            single_mode = (mstp_global.conf.sys.forceVersion < MSTP_PROTOCOL_VERSION_MSTP);
            rc = l2_set_msti_map(single_mode, ARRSZ(mstp_global.conf.msti.map.map), mstp_global.conf.msti.map.map);

            // Check if this gave rise to a different set of VLANs to enable
            // ingress filtering for.
            MSTP_vlan_ingress_filter(false /* also check for TE MSTI changes, since mode may have changed from MSTP to e.g. STP or vice versa */);
        }
    }

    MSTP_UNLOCK();

    return rc;
}

u8 vtss_mstp_msti_priority_get(mstp_msti_t msti)
{
    MSTP_LOCK();
    u8 priority = mstp_global.conf.bridgePriority[msti];
    MSTP_UNLOCK();
    return priority;
}

mesa_rc vtss_mstp_msti_priority_set(mstp_msti_t msti, u8 priority)
{
    mesa_rc rc = VTSS_RC_OK;

    if (!msg_switch_is_primary()) {
        rc = VTSS_RC_INV_STATE;
    } else if (msti < VTSS_APPL_MSTP_MAX_MSTI && ((priority & 0x0F) == 0)) {
        MSTP_LOCK();
        if (mstp_global.conf.bridgePriority[msti] != priority) {
            mstp_global.conf.bridgePriority[msti] = priority;
            save_config();
            /* The call will fail if the MSTI is not active */
            (void)_vtss_mstp_set_bridge_priority(mstp_global.mstpi, msti, priority);
        }
        MSTP_UNLOCK();
    } else {
        rc = VTSS_RC_ERROR;
    }

    return rc;
}

mesa_rc vtss_appl_mstp_msti_param_get(mstp_msti_t msti, vtss_appl_mstp_msti_param_t *param)
{
    mesa_rc rc = VTSS_RC_OK;

    if (!msg_switch_is_primary()) {
        rc = VTSS_RC_INV_STATE;
    } else if (msti < VTSS_APPL_MSTP_MAX_MSTI) {
        param->priority = vtss_mstp_msti_priority_get(msti);
    } else {
        rc = VTSS_RC_ERROR;
    }

    return rc;
}

mesa_rc vtss_appl_mstp_msti_param_set(mstp_msti_t msti, const vtss_appl_mstp_msti_param_t *param)
{
    if (!msg_switch_is_primary()) {
        return VTSS_RC_INV_STATE;
    }

    return vtss_mstp_msti_priority_set(msti, param->priority);
}

mesa_rc vtss_appl_mstp_msti_config_get(mstp_msti_config_t *conf, u8 cfg_digest[MSTP_DIGEST_LEN])
{
    mesa_rc rc;

    if (msg_switch_is_primary()) {
        MSTP_LOCK();
        if ((rc = _vtss_mstp_get_config_id(mstp_global.mstpi, NULL, NULL, cfg_digest)) == VTSS_RC_OK) {
            *conf = mstp_global.conf.msti;
        }
        MSTP_UNLOCK();
    } else {
        rc = VTSS_RC_INV_STATE;
    }

    return rc;
}

mesa_rc vtss_appl_mstp_msti_config_set(const vtss_appl_mstp_msti_config_t *conf)
{
    mesa_rc rc = VTSS_RC_OK;

    if (msg_switch_is_primary()) {
        MSTP_LOCK();
        if (memcmp(&mstp_global.conf.msti, conf, sizeof(*conf)) != 0) {
            mstp_global.conf.msti = *conf;
            save_config();

            rc = mstp_apply_msticonfig();
        }

        MSTP_UNLOCK();
    } else {
        rc = VTSS_RC_INV_STATE;
    }

    return rc;
}

mesa_rc vtss_mstp_port_config_get(vtss_isid_t isid, mesa_port_no_t port_no, BOOL *enable, mstp_port_param_t *pconf)
{
    l2_port_no_t l2port = (port_no == VTSS_PORT_NO_NONE ? MSTP_PORT_CONFIG_AGGR : L2PORT2PORT(isid, port_no)); /* Aggr or normal */

    if (!msg_switch_is_primary() || l2port > MSTP_PORT_CONFIG_AGGR) {
        return VTSS_RC_ERROR;
    }

    MSTP_LOCK();
    *pconf = mstp_global.conf.portconfig[l2port];
    *enable = mstp_global.conf.stp_enable[l2port];
    MSTP_UNLOCK();

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mstp_interface_config_get(vtss_ifindex_t ifindex, vtss_appl_mstp_port_config_t *conf)
{
    mesa_rc            rc;
    vtss_ifindex_elm_t ife;

    if (msg_switch_is_primary()) {
        if ((rc = vtss_ifindex_decompose(ifindex, &ife)) == VTSS_RC_OK && ife.iftype == VTSS_IFINDEX_TYPE_PORT) {
            rc = vtss_mstp_port_config_get(ife.isid, ife.ordinal, &conf->enable, &conf->param);
        }
    } else {
        rc = VTSS_RC_INV_STATE;
    }

    return rc;
}

mesa_rc vtss_appl_mstp_aggregation_config_get(vtss_appl_mstp_port_config_t *conf)
{
    return vtss_mstp_port_config_get(VTSS_ISID_GLOBAL, VTSS_PORT_NO_NONE, &conf->enable, &conf->param);
}

mesa_rc vtss_mstp_port_config_set(vtss_isid_t isid, mesa_port_no_t port_no, BOOL enable, const mstp_port_param_t *pconf)
{
    l2_port_no_t l2port = (port_no == VTSS_PORT_NO_NONE ? MSTP_PORT_CONFIG_AGGR : L2PORT2PORT(isid, port_no)); /* Aggr or normal */
    BOOL         enb_chg;

    if (!msg_switch_is_primary() || l2port > MSTP_PORT_CONFIG_AGGR) {
        return VTSS_RC_ERROR;
    }

#ifdef VTSS_SW_OPTION_DOT1X
    // Inter-protocol check.
    // MSTP cannot get enabled on ports that are not in 802.1X Authorized state.
    // Note that port_no == 0 is acceptable in this func, hence the extra check.
    if (VTSS_ISID_LEGAL(isid) && port_no != VTSS_PORT_NO_NONE) {
        vtss_nas_switch_cfg_t dot1x_switch_cfg;
        vtss_usid_t usid = topo_isid2usid(isid);
        if (vtss_nas_switch_cfg_get(usid, &dot1x_switch_cfg) != VTSS_RC_OK) {
            return VTSS_RC_ERROR;
        }

        if (enable && dot1x_switch_cfg.port_cfg[port_no - VTSS_PORT_NO_START].admin_state != VTSS_APPL_NAS_PORT_CONTROL_FORCE_AUTHORIZED) {
            return VTSS_RC_ERROR;
        }
    }
#endif /* VTSS_SW_OPTION_DOT1X */

    MSTP_LOCK();
    enb_chg = (mstp_global.conf.stp_enable[l2port] != enable);
    if (memcmp(&mstp_global.conf.portconfig[l2port], pconf, sizeof(*pconf)) != 0 || enb_chg) {
        mstp_global.conf.portconfig[l2port] = *pconf;
        mstp_global.conf.stp_enable[l2port] = enable;
        save_config();

        if (l2port != MSTP_PORT_CONFIG_AGGR) {           /* Plain port */
            if (enb_chg) {
                port_sync(l2port, NULL);    /* Stop/start port */
                MSTP_vlan_ingress_filter(true /* enough to check whether this is the first port getting enabled or last port getting disabled, since this doesn't change the TE MSTI */);
            }

            /* Apply in core MSTP as well */
            (void)_vtss_mstp_set_port_parameters(mstp_global.mstpi, L2PORT2API(l2port), pconf);
        } else {           /* Potential all aggrs - process in bulk */
            vtss_flag_setbits(&mstp_global.control_flags, CTLFLAG_MSTP_AGGRCONFIG);
        }
    }

    T_I("Port %d -> cport %d, enb %d", port_no, l2port, enable);
    MSTP_UNLOCK();

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mstp_interface_config_set(vtss_ifindex_t ifindex, const vtss_appl_mstp_port_config_t *conf)
{
    vtss_ifindex_elm_t ife;
    mesa_rc            rc;

    if (msg_switch_is_primary()) {
        if ((rc = vtss_ifindex_decompose(ifindex, &ife)) == VTSS_RC_OK && ife.iftype == VTSS_IFINDEX_TYPE_PORT) {
            rc = vtss_mstp_port_config_set(ife.isid, ife.ordinal, conf->enable, &conf->param);
        }
    } else {
        rc = VTSS_RC_INV_STATE;
    }

    return rc;
}

mesa_rc vtss_appl_mstp_aggregation_config_set(const vtss_appl_mstp_port_config_t *conf)
{
    if (!msg_switch_is_primary()) {
        return VTSS_RC_INV_STATE;
    }

    return vtss_mstp_port_config_set(VTSS_ISID_GLOBAL, VTSS_PORT_NO_NONE, conf->enable, &conf->param);
}

mesa_rc vtss_mstp_msti_port_config_get(vtss_isid_t isid, mstp_msti_t msti, mesa_port_no_t port_no, mstp_msti_port_param_t *pconf)
{
    l2_port_no_t l2port = (port_no == VTSS_PORT_NO_NONE ? MSTP_PORT_CONFIG_AGGR : L2PORT2PORT(isid, port_no)); /* Aggr or normal */

    if (!msg_switch_is_primary() || l2port > MSTP_PORT_CONFIG_AGGR) {
        return VTSS_RC_ERROR;
    }

    MSTP_LOCK();
    *pconf = mstp_global.conf.msticonfig[l2port][msti];
    MSTP_UNLOCK();

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mstp_interface_mstiport_config_get(vtss_ifindex_t ifindex, vtss_appl_mstp_msti_t msti, vtss_appl_mstp_msti_port_param_t *param)
{
    vtss_ifindex_elm_t ife;
    mesa_rc            rc;

    if (msg_switch_is_primary()) {
        if ((rc = vtss_ifindex_decompose(ifindex, &ife)) == VTSS_RC_OK && ife.iftype == VTSS_IFINDEX_TYPE_PORT) {
            rc = vtss_mstp_msti_port_config_get(ife.isid, msti, ife.ordinal, param);
        }
    } else {
        rc = VTSS_RC_INV_STATE;
    }

    return rc;
}

mesa_rc vtss_appl_mstp_aggregation_mstiport_config_get(vtss_appl_mstp_msti_t msti, vtss_appl_mstp_msti_port_param_t *param)
{
    if (!msg_switch_is_primary()) {
        return VTSS_RC_INV_STATE;
    }

    return vtss_mstp_msti_port_config_get(VTSS_ISID_GLOBAL, msti, VTSS_PORT_NO_NONE, param);
}

mesa_rc vtss_mstp_msti_port_config_set(vtss_isid_t isid, mstp_msti_t msti, mesa_port_no_t port_no, const mstp_msti_port_param_t *pconf)
{
    l2_port_no_t l2port = (port_no == VTSS_PORT_NO_NONE ? MSTP_PORT_CONFIG_AGGR : L2PORT2PORT(isid, port_no)); /* Aggr or normal */

    if (!msg_switch_is_primary() || l2port > MSTP_PORT_CONFIG_AGGR) {
        return VTSS_RC_ERROR;
    }

    if (msti >= VTSS_APPL_MSTP_MAX_MSTI ||
        ((pconf->adminPortPriority & 0x0F) != 0) ||
        pconf->adminPathCost > 200000000) { /* 802.1Q-2005 Sect 13.37.1 */
        return VTSS_RC_ERROR;
    }

    MSTP_LOCK();
    if (memcmp(&mstp_global.conf.msticonfig[l2port][msti], pconf, sizeof(*pconf)) != 0) {
        mstp_global.conf.msticonfig[l2port][msti] = *pconf;
        save_config();

        if (l2port != MSTP_PORT_CONFIG_AGGR) {           /* Plain port */
            /* Apply in core MSTP as well */
            (void)_vtss_mstp_set_msti_port_parameters(mstp_global.mstpi, msti, L2PORT2API(l2port), pconf);
        } else {           /* Potential all aggrs - process in bulk */
            vtss_flag_setbits(&mstp_global.control_flags, CTLFLAG_MSTP_AGGRCONFIG);
        }
    }

    T_I("MSTI %d Port %d -> cport %d, prio %d", msti, port_no, l2port, pconf->adminPortPriority);
    MSTP_UNLOCK();

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mstp_interface_mstiport_config_set(vtss_ifindex_t ifindex, vtss_appl_mstp_msti_t msti, const vtss_appl_mstp_msti_port_param_t *param)
{
    vtss_ifindex_elm_t ife;
    mesa_rc            rc;

    if (msg_switch_is_primary()) {
        if ((rc = vtss_ifindex_decompose(ifindex, &ife)) == VTSS_RC_OK && ife.iftype == VTSS_IFINDEX_TYPE_PORT) {
            rc = vtss_mstp_msti_port_config_set(ife.isid, msti, ife.ordinal, param);
        }
    } else {
        rc = VTSS_RC_INV_STATE;
    }

    return rc;
}

mesa_rc vtss_appl_mstp_aggregation_mstiport_config_set(vtss_appl_mstp_msti_t msti, const vtss_appl_mstp_msti_port_param_t *param)
{
    return vtss_mstp_msti_port_config_set(VTSS_ISID_GLOBAL, msti, VTSS_PORT_NO_NONE, param);
}

mesa_rc vtss_appl_mstp_bridge_status_get(vtss_appl_mstp_msti_t msti, vtss_appl_mstp_bridge_status_t *status)
{
    mesa_rc rc = VTSS_RC_INV_STATE;

    if (msg_switch_is_primary()) {
        MSTP_LOCK();
        if (MSTP_READY()) {
            rc = _vtss_mstp_get_bridge_status(mstp_global.mstpi, msti, status);
            if (rc == VTSS_RC_OK) {
                status->rootPort = status->rootPort ? API2L2PORT(status->rootPort) : L2_NULL;
            }
        }

        MSTP_UNLOCK();
    }

    return rc;
}

BOOL mstp_get_port_status(mstp_msti_t msti, l2_port_no_t l2port, mstp_port_mgmt_status_t *status)
{
    BOOL ok;

    MSTP_LOCK();

    ok = msg_switch_is_primary() && (MSTP_READY() && l2port_is_valid(l2port));
    if (ok) {
        memset(status, 0, sizeof(*status));
        if (l2port_is_port(l2port)) {
            status->enabled = mstp_global.conf.stp_enable[l2port];
            status->parent = mstp_global.aggr.parent[l2port];
        } else {
            status->enabled = mstp_global.conf.stp_enable[MSTP_PORT_CONFIG_AGGR]; /* Shared enabled-ness */
            status->parent = L2_NULL; /* Always top dog */
        }

        status->fwdstate = fwd2str((mstp_fwdstate_t)l2_get_msti_stpstate(msti, l2port));
        uint stpport = (status->parent != L2_NULL ? status->parent : l2port);
        status->active = (_vtss_mstp_get_port_status(mstp_global.mstpi, msti, L2PORT2API(stpport), &status->core) == VTSS_RC_OK);
    }

    MSTP_UNLOCK();

    return ok;
}

static mesa_rc mstp_ifindex_l2(vtss_ifindex_t ifindex, l2_port_no_t *l2port)
{
    vtss_ifindex_elm_t ife;
    mesa_rc            rc;

    if ((rc = vtss_ifindex_decompose(ifindex, &ife)) == VTSS_RC_OK) {
        switch (ife.iftype) {
        case VTSS_IFINDEX_TYPE_PORT:
            *l2port = L2PORT2PORT(ife.isid, ife.ordinal);
            break;

        case VTSS_IFINDEX_TYPE_LLAG:
            *l2port = L2LLAG2PORT(ife.isid, ife.ordinal);
            break;

        case VTSS_IFINDEX_TYPE_GLAG:
            *l2port = L2GLAG2PORT(ife.ordinal);
            break;

        default:
            rc = VTSS_RC_ERROR;
        }
    }

    return rc;
}

mesa_rc vtss_appl_mstp_interface_status_get(vtss_ifindex_t ifindex, vtss_appl_mstp_msti_t msti, vtss_appl_mstp_port_mgmt_status_t *status)
{
    l2_port_no_t l2port;
    mesa_rc      rc;

    if (msg_switch_is_primary()) {
        if ((rc = mstp_ifindex_l2(ifindex, &l2port)) == VTSS_RC_OK) {
            rc = mstp_get_port_status(msti, l2port, status) ? VTSS_RC_OK : VTSS_RC_ERROR;
        }
    } else {
        rc = VTSS_RC_INV_STATE;
    }

    return rc;
}

BOOL mstp_get_port_vectors(mstp_msti_t msti, l2_port_no_t l2port, mstp_port_vectors_t *vectors)
{
    BOOL ok;

    MSTP_LOCK();

    ok = (msg_switch_is_primary() && MSTP_READY() && l2port_is_valid(l2port));
    if (ok) {
        memset(vectors, 0, sizeof(*vectors));
        ok = (_vtss_mstp_get_port_vectors(mstp_global.mstpi, msti, L2PORT2API(l2port), vectors) == VTSS_RC_OK);
    }

    MSTP_UNLOCK();

    return ok;
}

BOOL mstp_get_port_statistics(l2_port_no_t l2port, mstp_port_statistics_t *stats, BOOL clear)
{
    BOOL ok;

    if (!msg_switch_is_primary()) {
        return FALSE;
    }

    MSTP_LOCK();
    if (clear) {
        (void)_vtss_mstp_clear_port_statistics(mstp_global.mstpi, L2PORT2API(l2port));
    }

    ok = (_vtss_mstp_get_port_statistics(mstp_global.mstpi, L2PORT2API(l2port), stats) == VTSS_RC_OK);
    MSTP_UNLOCK();

    return ok;
}

mesa_rc vtss_appl_mstp_interface_statistics_get(vtss_ifindex_t ifindex, vtss_appl_mstp_port_statistics_t *stats)
{
    l2_port_no_t l2port;
    mesa_rc      rc;

    if (msg_switch_is_primary()) {
        if ((rc = mstp_ifindex_l2(ifindex, &l2port)) == VTSS_RC_OK) {
            rc = mstp_get_port_statistics(l2port, stats, FALSE) ? VTSS_RC_OK : VTSS_RC_ERROR;
        }
    } else {
        rc = VTSS_RC_INV_STATE;
    }

    return rc;
}

BOOL mstp_set_port_mcheck(l2_port_no_t l2port)
{
    BOOL ok;

    T_IG(_C, "mcheck: l2port %d", l2port);

    if (msg_switch_is_primary()) {
        MSTP_LOCK();
        ok = (_vtss_mstp_port_mcheck(mstp_global.mstpi, L2PORT2API(l2port)) == VTSS_RC_OK);
        MSTP_UNLOCK();
        return ok;
    }

    return FALSE;
}

int vtss_appl_mstp_bridge2str(void *buffer, size_t size, const u8 *bridgeid)
{
    return vtss_mstp_bridge2str(buffer, size, bridgeid);
}

mesa_rc vtss_appl_mstp_msti_lookup(mesa_vid_t vid, vtss_appl_mstp_msti_t *pMsti)
{
    if (vid > VTSS_APPL_MSTP_NULL_VID && vid < VTSS_APPL_MSTP_MAX_VID && pMsti) {
        *pMsti = mstp_global.conf.msti.map.map[vid];
        return VTSS_RC_OK;
    }

    return VTSS_RC_ERROR;
}

/* Trap support */

BOOL mstp_register_trap_sink(mstp_trap_sink_t cb)
{
    BOOL rc = FALSE;

    MSTP_LOCK();
    if (cb == NULL ||
        mstp_global.trap_cb == NULL ||
        mstp_global.trap_cb == cb) {
        mstp_global.trap_cb = cb;
        rc = TRUE;
    }
    MSTP_UNLOCK();

    return rc;
}

BOOL mstp_register_config_change_cb(mstp_config_change_cb_t cb)
{
    int  i;
    BOOL rc = FALSE;

    MSTP_LOCK();

    for (i = 0; i < ARRSZ(mstp_global.config_cb); i++) {
        if (mstp_global.config_cb[i] == NULL) {
            mstp_global.config_cb[i] = cb;
            rc = TRUE;
            break;
        }
    }

    MSTP_UNLOCK();
    return rc;
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_PRIVATE_MIB
VTSS_PRE_DECLS void mstp_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_mstp_json_init();
#endif
extern "C" int mstp_icli_cmd_register();

mesa_rc mstp_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    /*lint --e{454,456} ... We leave the Mutex locked */
    switch (data->cmd) {
    case INIT_CMD_INIT:
        vtss_clear(mstp_global);
        mstp_global.ready = FALSE;
        critd_init_legacy(&mstp_global.mutex, "mstp", VTSS_MODULE_ID_MSTP, CRITD_TYPE_MUTEX);
        vtss_sem_init(&mstp_global.defconfig_sema, 0);

        MSTP_hw_support = fast_cap(MESA_CAP_L2_MSTP_HW) != 0;
        MSTP_lan966x    = fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_LAN966X;

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        mstp_mib_init();  /* Register our private mib */
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_mstp_json_init();
#endif
#ifdef VTSS_SW_OPTION_ICFG
        if (mstp_icfg_init() != VTSS_RC_OK) {
            T_D("Calling mstp_icfg_init() failed");
        }
#endif

        mstp_icli_cmd_register();

        vtss_flag_init(&mstp_global.control_flags);
        break;

    case INIT_CMD_START:
        mstp_conf_default();

        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           mstp_thread,
                           0,
                           "MSTP",
                           nullptr,
                           0,
                           &mstp_thread_handle,
                           &mstp_thread_block);
        break;

    case INIT_CMD_CONF_DEF:
        if (isid == VTSS_ISID_GLOBAL) {
            /* Load defaults. This has to happen on the thread to avoid race conditions,
             * and we cannot exit INIT_CMD_CONF_DEF until the thread is done either --
             * more race conditions. Thus, a semaphore is used for synchronization.
             */
            T_D("Signal thread to begin loading defaults");
            vtss_flag_setbits(&mstp_global.control_flags, CTLFLAG_MSTP_DEFCONFIG);
            vtss_flag_setbits(&mstp_global.control_flags, CTLFLAG_MSTP_AGGRCONFIG);
            T_D("Waiting for thread to complete loading defaults");
            vtss_sem_wait(&mstp_global.defconfig_sema);
            T_D("Load defaults done");
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        // The thread wakes up by itself and suspends itself.
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        MSTP_LOCK();
        sync_ports_switch(isid);
        MSTP_UNLOCK();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

