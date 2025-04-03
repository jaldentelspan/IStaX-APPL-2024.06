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
#include "vtss_common_os.h"     /* Real "common os" API */
#include "l2proto_api.h"        /* module API */
#include "l2proto.h"            /* Private header file */
#include <vtss/appl/port.h>
#include "port_api.h"
#include "packet_api.h"
#include "msg_api.h"
#include "conf_api.h"
#include "misc_api.h"
#include "topo_api.h"
#include "icli_porting_util.h"
#include "critd_api.h"
#include "icli_api.h"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_L2PROTO

/*
 * Common Data
 */

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "l2", "L2 Protocol Helper"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [VTSS_TRACE_GRP_PACKET] = {
        "packet",
        "Packet",
        VTSS_TRACE_LVL_WARNING
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/* Stack PDU forwarding */
static uint rx_modules;
static struct {
    vtss_module_id_t modid;
    l2_stack_rx_callback_t cb;
} rx_list[5];

static CapArray<vtss_common_stpstate_t, VTSS_APPL_CAP_L2_POAG_CNT> cached_stpstates;
/* Spanning Tree state that applies to the port when the link is up. */
static CapArray<mesa_stp_state_t, VTSS_APPL_CAP_L2_POAG_CNT> port_stp_state;
static CapArray<mesa_stp_state_t, VTSS_APPL_CAP_L2_POAG_CNT, VTSS_APPL_CAP_MSTI_CNT> msti_stp_state;

#ifdef VTSS_SW_OPTION_MSTP
static l2_stp_state_change_callback_t      l2_stp_state_change_table[STP_STATE_CHANGE_REG_MAX];
static l2_stp_msti_state_change_callback_t l2_stp_msti_state_change_table[STP_STATE_CHANGE_REG_MAX];
static u8 l2_msti_map[VTSS_VIDS];
#endif

static critd_t l2_mutex;    /* Local state protection */

#define TEMP_LOCK()     vtss_global_lock(  __FILE__, __LINE__)
#define TEMP_UNLOCK()   vtss_global_unlock(__FILE__, __LINE__)

#define L2_LOCK()       critd_enter(&l2_mutex, __FILE__, __LINE__)
#define L2_UNLOCK()     critd_exit (&l2_mutex, __FILE__, __LINE__)

/****************************************************************************
 * "Common OS" functions
 ****************************************************************************/

/*
 * Common common code
 */

static BOOL common_linkstatus(vtss_common_port_t portno, vtss_appl_port_status_t *ps)
{
    vtss_ifindex_t ifindex;
    mesa_port_no_t switchport;
    vtss_isid_t    isid;

    if (l2port2port(portno, &isid, &switchport) && port_is_front_port(switchport)) {
        return vtss_ifindex_from_port(isid, switchport, &ifindex) == VTSS_RC_OK && vtss_appl_port_status_get(ifindex, ps) == VTSS_RC_OK;
    } else {
        return FALSE;
    }
}

#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
static const char *stpstatestring(const mesa_stp_state_t stpst)
{
    switch (stpst) {
    case MESA_STP_STATE_DISCARDING : /* STP state discarding */
        return "discarding";

    case MESA_STP_STATE_LEARNING :   /* STP state learning */
        return "learning";

    case MESA_STP_STATE_FORWARDING : /* STP state forwarding */
        return "forwarding";
    }

    return "<unknown>";
}
#endif

const char *vtss_common_str_stpstate(vtss_common_stpstate_t stpstate)
{
    switch (stpstate) {
    case VTSS_COMMON_STPSTATE_DISCARDING :
        return "dis";

    case VTSS_COMMON_STPSTATE_LEARNING :
        return "lrn";

    case VTSS_COMMON_STPSTATE_FORWARDING :
        return "fwd";

    default :
        return "undef";
    }
}

const char *vtss_common_str_linkstate(vtss_common_linkstate_t state)
{
    switch (state) {
    case VTSS_COMMON_LINKSTATE_DOWN :
        return "down";

    case VTSS_COMMON_LINKSTATE_UP :
        return "up";

    default :
        return "Undef";
    }
}

const char *vtss_common_str_linkduplex(vtss_common_duplex_t duplex)
{
    switch (duplex) {
    case VTSS_COMMON_LINKDUPLEX_HALF :
        return "half";

    case VTSS_COMMON_LINKDUPLEX_FULL :
        return "full";

    default :
        return "undef";
    }
}

static l2_msg_t *l2_alloc_message(size_t size, l2_msg_id_t msg_id)
{
    l2_msg_t *msg = (l2_msg_t *)VTSS_MALLOC(size);

    if (msg) {
        msg->msg_id = msg_id;
    }

    T_NG(VTSS_TRACE_GRP_PACKET, "msg len %zd, type %d => %p", size, msg_id, msg);
    return msg;
}

#ifdef VTSS_SW_OPTION_MSTP
static void do_callbacks(vtss_common_port_t portno, mesa_stp_state_t new_state)
{
    uint i;

    /* Callbacks for common port state */
    for (i = 0; i < ARRSZ(l2_stp_state_change_table); i++) {
        l2_stp_state_change_callback_t cb;

        TEMP_LOCK();
        cb = l2_stp_state_change_table[i];
        TEMP_UNLOCK();

        if (cb) {
            cb(portno, new_state);
        }
    }
}
#endif

#ifdef VTSS_SW_OPTION_MSTP
static void do_callbacks_msti(vtss_common_port_t l2port, uchar msti, mesa_stp_state_t new_state)
{
    uint i;

    /* Callbacks for MSTI state */
    for (i = 0; i < ARRSZ(l2_stp_msti_state_change_table); i++) {
        l2_stp_msti_state_change_callback_t cb;

        TEMP_LOCK();
        cb = l2_stp_msti_state_change_table[i];
        TEMP_UNLOCK();

        if (cb) {
            cb(l2port, msti, new_state);
        }
    }
}
#endif

static void l2port_stp_state_set(const l2_port_no_t l2port, const mesa_stp_state_t stp_state)
{
    mesa_port_no_t switchport;
    vtss_isid_t isid;

    VTSS_ASSERT(l2port_is_valid(l2port));

    T_N("port %d: new STP state %s", l2port, stpstatestring(stp_state) );

    port_stp_state[l2port] = stp_state;
    if (l2port2port(l2port, &isid, &switchport)) {
        T_I("Local switch port %u: new STP state %s", switchport, stpstatestring(stp_state) );
        (void)msg_stp_port_state_set(NULL, switchport, stp_state);
    } else {
        T_D("Operation on aggr port %d - state %s", l2port, stpstatestring(stp_state));
    }
}

#ifdef VTSS_SW_OPTION_MSTP
static void l2_set_msti_stpstate_local(uchar msti, mesa_port_no_t switchport, const mesa_stp_state_t stp_state)
{
    mesa_rc rc;

    VTSS_ASSERT(msti < N_L2_MSTI_MAX);
    VTSS_ASSERT(l2port_is_poag(switchport));

    T_I("MSTI%d port %d: new STP state %s", msti, switchport, stpstatestring(stp_state) );

    if (switchport >= port_count_max()) {
        T_E("Invalid port %d\n", switchport);
        return;
    }

    if ((rc = mesa_mstp_port_msti_state_set(NULL,
                                            switchport,
                                            (mesa_msti_t)(msti),
                                            stp_state)) != VTSS_RC_OK) {
        T_W("STP state set fails: port %d, state %d, rc %d\n", switchport, stp_state, rc);
    }
}
#endif

#ifdef VTSS_SW_OPTION_MSTP
static mesa_rc l2local_set_msti_map(BOOL all_to_cist /* Set if *not* MSTP mode */, size_t maplen, const vtss_appl_mstp_mstid_t *map)
{
    mesa_vid_t vid;
    mesa_rc rc = VTSS_RC_OK;

    VTSS_ASSERT(maplen <= VTSS_VIDS);
    T_I("Set map - all_to_cist %d, maplen %zd", all_to_cist, maplen);

    L2_LOCK();
    for (vid = 1; vid < maplen; vid++) {
        vtss_appl_mstp_msti_t msti = all_to_cist ? VTSS_MSTI_CIST : l2_mstid2msti(map[vid]);
        if (l2_msti_map[vid] != msti) {
            if (msti) {
                T_I("VID %d in MSTI%d", vid, msti);
            }

            if ((rc = mesa_mstp_vlan_msti_set(NULL, vid, (mesa_msti_t)(msti))) != VTSS_RC_OK) {
                T_E("vtss_mstp_vlan_set(%d, %d): Failed: %s", vid, msti, error_txt(rc));
                goto out;
            }

            l2_msti_map[vid] = msti;    // Cache current value to avoid noop apply
        }
    }

out:
    L2_UNLOCK();
    T_I("Done with msti map - rc %d", rc);
    return rc;
}
#endif

static BOOL _debug_char_put(icli_addrword_t _id, char ch)
{
    (void)fputc(ch, stdout);
    return TRUE;
}

static BOOL _debug_str_put(icli_addrword_t _id, const char *str)
{
    (void)fputs(str, stdout);
    return TRUE;
}

static void _debug_print(void)
{
    icli_session_open_data_t open_data;
    u32                      session_id;

    // Open new ICLI session
    memset(&open_data, 0, sizeof(open_data));

    open_data.name     = "DEBUG";
    open_data.way      = ICLI_SESSION_WAY_APP_EXEC;
    open_data.char_put = _debug_char_put;
    open_data.str_put  = _debug_str_put;

    if (icli_session_open(&open_data, &session_id) != ICLI_RC_OK) {
        return;
    }

    (void)icli_session_privilege_set(session_id, ICLI_PRIVILEGE_15);
    (void)icli_session_cmd_exec(session_id, "do platform debug allow", TRUE);
    (void)icli_session_cmd_exec(session_id, "do debug packet cmd 1",   TRUE);
    (void)icli_session_cmd_exec(session_id, "do debug api fdma",       TRUE);
    (void)icli_session_close(session_id);
}

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Set the MSTP MSTI mapping table
 */
mesa_rc l2_set_msti_map(BOOL all_to_cist /* Set if *not* MSTP mode */, size_t maplen, const vtss_appl_mstp_mstid_t *map)
{
    VTSS_ASSERT(maplen <= VTSS_VIDS);

    T_I("Set map - all_to_cist %d, maplen %zd", all_to_cist, maplen);

    return l2local_set_msti_map(all_to_cist, maplen, map);
}
#endif

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Get the MSTP MSTI mapping table
 */
mesa_rc l2_get_msti_map(uchar *map, size_t map_size)
{
    if (map_size != sizeof(l2_msti_map)) {
        return VTSS_UNSPECIFIED_ERROR;
    }

    L2_LOCK();
    memcpy(map, l2_msti_map, sizeof(l2_msti_map));
    L2_UNLOCK();

    return VTSS_RC_OK;
}
#endif

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Set the Spanning Tree state of all MSTIs for a specific port.
 */
void l2_set_msti_stpstate_all(vtss_common_port_t l2port, mesa_stp_state_t new_state)
{
    mesa_port_no_t switchport;
    vtss_isid_t isid;
    uchar msti;

    if (!msg_switch_is_primary()) {
        return;
    }

    T_I("MSTIx port %d: new STP state %s", l2port, stpstatestring(new_state) );

    if (!l2port_is_valid(l2port)) {
        T_E("%s: Invalid port %d", __FUNCTION__, l2port);
    }

    for (msti = 0; msti < N_L2_MSTI_MAX; msti++) {
        msti_stp_state[l2port][msti] = (mesa_stp_state_t) new_state;
    }

    if (l2port2port(l2port, &isid, &switchport)) {
        if (!port_is_front_port(switchport)) {
            T_I("Invalid port %d\n", switchport);
            return;
        }
        for (msti = 0; msti < N_L2_MSTI_MAX; msti++) {
            l2_set_msti_stpstate_local(msti, switchport, new_state);
        }
    }

    /* Looped callback on each MSTI */
    for (msti = 0; msti < N_L2_MSTI_MAX; msti++) {
        do_callbacks_msti(l2port, msti, new_state);
    }
}
#endif

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Set the Spanning Tree state of a specific MSTI port.
 */
void l2_set_msti_stpstate(uchar msti, vtss_common_port_t l2port, mesa_stp_state_t new_state)
{
    mesa_port_no_t switchport;
    vtss_isid_t    isid;

    VTSS_ASSERT(msti < N_L2_MSTI_MAX);

    if (!msg_switch_is_primary()) {
        return;
    }

    T_D("MSTI%d port %d: new STP state %s", msti, l2port, stpstatestring(new_state) );

    if (!l2port_is_valid(l2port)) {
        T_E("%s: Invalid port %d", __FUNCTION__, l2port);
        return;
    }

    if (new_state != (mesa_stp_state_t)msti_stp_state[l2port][msti]) {
        msti_stp_state[l2port][msti] = (mesa_stp_state_t) new_state;
        if (l2port2port(l2port, &isid, &switchport)) {
            if (!port_is_front_port(switchport)) {
                T_I("Invalid port %d\n", switchport);
                return;
            }

            l2_set_msti_stpstate_local(msti, switchport, new_state);
        }

        do_callbacks_msti(l2port, msti, new_state);
    }
}
#endif

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Get the Spanning Tree state of a specific MSTI port.
 */
mesa_stp_state_t l2_get_msti_stpstate(uchar msti, vtss_common_port_t l2port)
{
    uchar st;

    VTSS_ASSERT(msti < N_L2_MSTI_MAX);
    VTSS_ASSERT(l2port_is_valid(l2port));

    TEMP_LOCK();
    st = msti_stp_state[l2port][msti];
    TEMP_UNLOCK();

    return (mesa_stp_state_t)st;
}
#endif

#ifdef VTSS_SW_OPTION_MSTP
static void l2_local_set_states(mesa_port_no_t switchport, mesa_stp_state_t port_state, const mesa_stp_state_t *msti_state)
{
    uchar msti;

    T_I("Switch port %u: Set STP states", switchport);

    (void)msg_stp_port_state_set(NULL, switchport, port_state);
    for (msti = 0; msti < N_L2_MSTI_MAX; msti++) {
        l2_set_msti_stpstate_local(msti, switchport, msti_state[msti]);
    }
}
#endif

#ifdef VTSS_SW_OPTION_MSTP
void l2_sync_stpstates(vtss_common_port_t copy, vtss_common_port_t port)
{
    mesa_port_no_t switchport;
    vtss_isid_t    isid;

    if (!msg_switch_is_primary()) {
        return;
    }

    if (!l2port2port(copy, &isid, &switchport)) {
        T_E("%s: Invalid port %d", __FUNCTION__, copy);
        return;
    }

    T_I("Sync switch port %d = [%d,%d] to %d", copy, isid, switchport, port);
    l2_local_set_states(switchport, port_stp_state[port], msti_stp_state[port].data());
}
#endif

/**
 * vtss_os_get_linkspeed - Deliver the current link speed (in Kbps) of a specific port.
 */
vtss_common_linkspeed_t vtss_os_get_linkspeed(vtss_common_port_t portno)
{
    vtss_appl_port_status_t port_status;
    if (common_linkstatus(portno, &port_status)) {
        switch (port_status.speed) {
        case MESA_SPEED_UNDEFINED :
            return (vtss_common_linkspeed_t)0;

        case MESA_SPEED_10M :
            return (vtss_common_linkspeed_t)10;

        case MESA_SPEED_100M :
            return (vtss_common_linkspeed_t)100;

        case MESA_SPEED_1G :
            return (vtss_common_linkspeed_t)1000;

        case MESA_SPEED_2500M :
            return (vtss_common_linkspeed_t)2500;

        case MESA_SPEED_5G :
            return (vtss_common_linkspeed_t)5000;

        case MESA_SPEED_10G :
            return (vtss_common_linkspeed_t)10000;

        case MESA_SPEED_12G :
            return (vtss_common_linkspeed_t)12000;

        case MESA_SPEED_25G :
            return (vtss_common_linkspeed_t)25000;

        default :
            return (vtss_common_linkspeed_t)1;
        }
    }
    return (vtss_common_linkspeed_t)0;
}

/**
 * vtss_os_get_linkstate - Deliver the current link state of a specific port.
 */
vtss_common_linkstate_t vtss_os_get_linkstate(vtss_common_port_t portno)
{
    vtss_common_linkstate_t link = VTSS_COMMON_LINKSTATE_DOWN;
    vtss_appl_port_status_t port_status;

    if (common_linkstatus(portno, &port_status) &&  port_status.link) {
        link = VTSS_COMMON_LINKSTATE_UP;
    }

    T_N("port %d: link %s", portno, link == VTSS_COMMON_LINKSTATE_DOWN ? "down" : "up");
    return link;
}

/**
 * vtss_os_get_linkduplex - Deliver the current link duplex mode of a specific port.
 */
vtss_common_duplex_t vtss_os_get_linkduplex(vtss_common_port_t portno)
{
    vtss_appl_port_status_t port_status;

    return msg_switch_is_primary() && common_linkstatus(portno, &port_status) && port_status.link && port_status.fdx ? VTSS_COMMON_LINKDUPLEX_FULL : VTSS_COMMON_LINKDUPLEX_HALF;
}

/**
 * vtss_os_get_systemmac - Deliver the MAC address for the switch.
 */
void vtss_os_get_systemmac(vtss_common_macaddr_t *system_macaddr)
{
    if (msg_switch_is_primary()) {
        (void)conf_mgmt_mac_addr_get((uchar *)system_macaddr, 0);
    }
}

/**
 * vtss_os_get_portmac - Deliver the MAC address for the a specific port.
 */
void vtss_os_get_portmac(vtss_common_port_t portno, vtss_common_macaddr_t *port_macaddr)
{
    uchar *mac = (uchar *)port_macaddr;
    mesa_port_no_t switchport;
    vtss_isid_t isid;
    VTSS_ASSERT(l2port_is_port(portno));
    if (l2port2port(portno, &isid, &switchport)) {
        mesa_mac_addr_t basemac;
        (void)topo_isid2mac(isid, basemac);
        misc_instantiate_mac(mac, basemac, 1 + switchport); /* entry 0 is the CPU port */
        T_D("Port MAC address [%d,%u] = %s", isid, switchport, misc_mac2str(mac));
    }
}

/**
 * vtss_os_set_stpstate - Set the Spanning Tree state of a specific port.
 */
void vtss_os_set_stpstate(vtss_common_port_t portno, vtss_common_stpstate_t new_state)
{
    if (msg_switch_is_primary()) {
        VTSS_ASSERT(l2port_is_valid(portno));

        cached_stpstates[portno] = new_state;
        l2port_stp_state_set(portno, (mesa_stp_state_t) new_state);

#ifdef VTSS_SW_OPTION_MSTP
        /* Call registered functions outside critical region */
        do_callbacks(portno, new_state);
#endif
    }
}

/**
 * vtss_os_get_stpstate - Get the Spanning Tree state of a specific port.
 */
vtss_common_stpstate_t vtss_os_get_stpstate(vtss_common_port_t portno)
{
    VTSS_ASSERT(l2port_is_valid(portno));

    TEMP_LOCK();
    vtss_common_stpstate_t st = cached_stpstates[portno];
    TEMP_UNLOCK();

    return st;
}

/**
 * vtss_os_alloc_xmit - Allocate a buffer to be used for transmitting a frame.
 */
void *vtss_os_alloc_xmit(vtss_common_port_t l2port, vtss_common_framelen_t len)
{
    vtss_isid_t    isid;
    mesa_port_no_t port;
    void           *p = NULL;

    VTSS_ASSERT(l2port_is_valid(l2port));

    if (l2port2port(l2port, &isid, &port)) {
        p = packet_tx_alloc(len);
    }

    T_NG(VTSS_TRACE_GRP_PACKET, "%s(%d) ret %p", __FUNCTION__, len, p);
    return p;
}

int vtss_os_xmit(vtss_common_port_t l2port, void *frame, vtss_common_framelen_t len)
{
    packet_tx_props_t tx_props;
    vtss_isid_t       isid;
    mesa_port_no_t    port;

    VTSS_ASSERT(l2port_is_port(l2port));

    T_DG(VTSS_TRACE_GRP_PACKET, "%s(%d, %p, %d)", __FUNCTION__, l2port, frame, len);

    if (l2port2port(l2port, &isid, &port)) {
        packet_tx_props_init(&tx_props);
        tx_props.packet_info.modid     = VTSS_MODULE_ID_L2PROTO;
        tx_props.packet_info.frm       = (u8 *)frame;
        tx_props.packet_info.len       = len;
        tx_props.tx_info.dst_port_mask = VTSS_BIT64(port);
        tx_props.tx_info.cos           = MESA_PRIO_SUPER;

        if (packet_tx(&tx_props) == VTSS_RC_OK) {
            return VTSS_COMMON_CC_OK;
        }
    } else {
        T_EG(VTSS_TRACE_GRP_PACKET, "Transmit on non-port (%d)?", l2port);
        packet_tx_free((u8 *)frame);
    }

    T_EG(VTSS_TRACE_GRP_PACKET, "Trame transmit on port %d failed", l2port);
    _debug_print(); // Print more debug info.

    return VTSS_COMMON_CC_GENERR;
}

/*
 * l2 API
 */

/* l2port mapping functions */
BOOL l2port2port(l2_port_no_t l2port, vtss_isid_t *pisid, mesa_port_no_t *port)
{
    if (l2port_is_port(l2port)) {
        *pisid = VTSS_ISID_START; /* This is the only valid ISID for the standalone version */
        *port = l2port;
        return TRUE;
    }

    return FALSE;
}

BOOL l2port2poag(l2_port_no_t l2port, vtss_isid_t *pisid, vtss_poag_no_t *poag)
{
    if (l2port2port(l2port, pisid, poag)) {
        return TRUE;
    } else if (l2port_is_poag(l2port)) {
        *pisid = VTSS_ISID_START;
        *poag = l2port  - L2_MAX_PORTS_ + AGGR_MGMT_GROUP_NO_START;
        return TRUE;
    }

    return FALSE;
}

BOOL l2port2glag(l2_port_no_t l2port, mesa_glag_no_t *glag)
{
    return FALSE;
}

BOOL l2port_is_valid(l2_port_no_t l2port)
{
    return (l2port < L2_MAX_POAGS_);
}

BOOL l2port_is_port(l2_port_no_t l2port)
{
    return (l2port < L2_MAX_PORTS_);
}

BOOL l2port_is_poag(l2_port_no_t l2port)
{
    return (l2port < (L2_MAX_PORTS_ + L2_MAX_LLAGS_));
}

BOOL l2port_is_glag(l2_port_no_t l2port)
{
    return FALSE;
}

/*lint -sem(l2port2str, thread_protected )
 * No, this is not thread safe. But its a convenience tradeoff.
 */
const char *l2port2str(l2_port_no_t l2port)
{
    static char    _buf[16]; /* NOT THREAD SAFE */
    vtss_isid_t    isid;
    mesa_port_no_t port;

    if (l2port2port(l2port, &isid, &port)) {
        (void)snprintf(_buf, sizeof(_buf), "%u", iport2uport(port));
    } else if (l2port2poag(l2port, &isid, &port)) {
        (void)snprintf(_buf, sizeof(_buf), "LLAG%u", port);
    } else if (l2port2glag(l2port, &port)) {
        (void)snprintf(_buf, sizeof(_buf), "GLAG%u", port);
    } else {
        (void)snprintf(_buf, sizeof(_buf), "%d (L2)", l2port);
    }

    return _buf;
}

const char *l2port2str_icli(l2_port_no_t l2port, char *buf, size_t buflen)
{
    vtss_isid_t    isid;
    mesa_port_no_t port;
    char           interface_str[ICLI_PORTING_STR_BUF_SIZE];

    if (l2port2port(l2port, &isid, &port)) {
        (void)icli_port_info_txt(topo_isid2usid(isid), iport2uport(port), interface_str);
        strncpy(buf, interface_str, buflen);
    } else if (l2port2poag(l2port, &isid, &port)) {
        (void)snprintf(buf, buflen, "LLAG%u", port);
    } else if (l2port2glag(l2port, &port)) {
        (void)snprintf(buf, buflen, "GLAG%u", port);
    } else {
        (void)snprintf(buf, buflen, "%d (L2)", l2port);
    }

    return buf;
}

static void l2_do_rx_callback(vtss_module_id_t modid, const void *packet, size_t len, mesa_vid_t vid, l2_port_no_t l2port)
{
    l2_stack_rx_callback_t cb = NULL;
    uint                   i;

    TEMP_LOCK();

    for (i = 0; i < rx_modules; i++) {
        if (rx_list[i].modid == modid) {
            cb = rx_list[i].cb;
            break;
        }
    }

    TEMP_UNLOCK();

    if (cb != NULL) {
        cb(packet, len, vid, l2port);
    }
}

void l2_receive_indication(vtss_module_id_t modid, const void *packet, size_t len, mesa_port_no_t switchport, mesa_vid_t vid, mesa_glag_no_t glag_no)
{
    size_t   msg_len = sizeof(l2_msg_t) + len;
    l2_msg_t *msg    = l2_alloc_message(msg_len, L2_MSG_ID_FRAME_RX_IND);

    T_DG(VTSS_TRACE_GRP_PACKET, "len %zd port %u vid %d glag %u", len, switchport, vid, glag_no);

    if (msg) {
        msg->data.rx_ind.modid = modid;
        msg->data.rx_ind.switchport = switchport;
        msg->data.rx_ind.len = len;
        msg->data.rx_ind.glag_no = glag_no;
        memcpy(&msg[1], packet, len); /* Copy frame */
        msg_tx_adv(NULL, NULL, (msg_tx_opt_t)(MSG_TX_OPT_NO_ALLOC_ON_LOOPBACK | MSG_TX_OPT_SHAPE), VTSS_MODULE_ID_L2PROTO, 0, msg, msg_len);
    } else {
        T_WG(VTSS_TRACE_GRP_PACKET, "Unable to allocate %zd bytes, tossing frame on port %u", msg_len, switchport);
    }
}

void l2_receive_register(vtss_module_id_t modid, l2_stack_rx_callback_t cb)
{
    VTSS_ASSERT(rx_modules < ARRSZ(rx_list));

    TEMP_LOCK();
    rx_list[rx_modules].modid = modid;
    rx_list[rx_modules].cb = cb;
    rx_modules++;
    TEMP_UNLOCK();
}

static void l2local_flush_vport(l2_port_type type, int vport, vtss_common_vlanid_t vlan_id)
{
    mesa_aggr_no_t   zero_based_aggr_no;
    mesa_port_list_t members;
    mesa_port_no_t   switchport;
    u32              port_cnt = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    switch (type) {
    case L2_PORT_TYPE_POAG:
        if (vport < port_cnt) {
            /* Do a local flush vlan port / aggr */
            T_I("FLUSH_FDB: port %d, vid %d", vport, vlan_id);
            if (vlan_id == VTSS_VID_NULL) {
                (void)mesa_mac_table_port_flush(NULL, vport);
            } else {
                (void)mesa_mac_table_vlan_port_flush(NULL, vport, vlan_id);
            }
        } else {
            T_I("FLUSH_FDB: aggr %d, vid %d", vport, vlan_id);
            zero_based_aggr_no = (vport - port_cnt - AGGR_MGMT_GROUP_NO_START);

            if (mesa_aggr_port_members_get(NULL, zero_based_aggr_no, &members) == VTSS_RC_OK) {
                for (switchport = 0; switchport < port_cnt; switchport++) {
                    if (members[switchport]) {
                        T_I("FLUSH_FDB: aggr %d, port %u, vid %d", vport, switchport, vlan_id);
                        if (vlan_id == VTSS_VID_NULL) {
                            (void)mesa_mac_table_port_flush(NULL, switchport); /* Loose the vlan? */
                        } else {
                            (void)mesa_mac_table_vlan_port_flush(NULL, switchport, vlan_id); /* Loose the vlan? */
                        }
                    }
                }
            }
        }
        break;

    case L2_PORT_TYPE_GLAG:
        T_I("FLUSH_FDB: glag %d vlan %d", vport, vlan_id);
        break;

    default:
        VTSS_ASSERT(0);
    }
}

static void l2_flush_glag(mesa_glag_no_t glag)
{
    /* Do a local flush GLAG */
    l2local_flush_vport(L2_PORT_TYPE_GLAG, glag, VTSS_VID_NULL);
}

void l2_flush_port(l2_port_no_t l2port)
{
    mesa_glag_no_t glag;
    vtss_poag_no_t poag;
    vtss_isid_t    isid;

    VTSS_ASSERT(l2port_is_valid(l2port));

    if (l2port2glag(l2port, &glag)) {
        // The joys of having multiple aggr ranges...!
        l2_flush_glag(glag - AGGR_MGMT_GROUP_NO_START + VTSS_AGGR_NO_START);
    } else {
        VTSS_ASSERT(l2port2poag(l2port, &isid, &poag));
        /* Do a local flush vlan port */
        l2local_flush_vport(L2_PORT_TYPE_POAG, poag, 0);
    }
}

static void l2_flush_vlan_glag(mesa_glag_no_t glag, vtss_common_vlanid_t vlan_id)
{
    /* Do a local flush GLAG */
    l2local_flush_vport(L2_PORT_TYPE_GLAG, glag, vlan_id);
}

void l2_flush_vlan_port(l2_port_no_t l2port, vtss_common_vlanid_t vlan_id)
{
    mesa_glag_no_t glag;
    vtss_poag_no_t poag;
    vtss_isid_t    isid;

    VTSS_ASSERT(l2port_is_valid(l2port));

    if (l2port2glag(l2port, &glag)) {
        // The joys of having multiple aggr ranges...!
        l2_flush_vlan_glag(glag - AGGR_MGMT_GROUP_NO_START + VTSS_AGGR_NO_START, vlan_id);
    } else {
        VTSS_ASSERT(l2port2poag((l2port - AGGR_MGMT_GROUP_NO_START), &isid, &poag));
        /* Do a local flush vlan port */
        l2local_flush_vport(L2_PORT_TYPE_POAG, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) + poag, vlan_id);
    }
}

#ifdef VTSS_SW_OPTION_MSTP
static mesa_rc l2_stp_register(void **array, uint asize, const char *array_name, void *callback)
{
    mesa_rc rc = VTSS_RC_OK;
    uint    i;

    if (callback == NULL) {
        T_E("The callback function for %s is NULL\n", array_name);
        return VTSS_UNSPECIFIED_ERROR;
    }

    // Lookup if the callback is existing?
    // If not, found the index for inserting later
    for (i = 0; i < asize; i++) {
        if (callback == array[i]) {
            break;
        }
        if (array[i] == NULL) {
            array[i] = callback;
            break;
        }
    }

    if (i == asize) {
        T_E("The %s table full\n", array_name);
        rc = VTSS_UNSPECIFIED_ERROR;
    }

    return rc;
}
#endif

#ifdef VTSS_SW_OPTION_MSTP
/* L2 STP state change registration */
mesa_rc l2_stp_state_change_register(l2_stp_state_change_callback_t callback)
{
    mesa_rc rc;

    TEMP_LOCK();
    rc = l2_stp_register((void **)l2_stp_state_change_table, ARRSZ(l2_stp_state_change_table), "l2_stp_state_change_table", (void *)callback);
    TEMP_UNLOCK();

    return rc;
}
#endif

#ifdef VTSS_SW_OPTION_MSTP
/* STP MSTI state change callback registration */
mesa_rc l2_stp_msti_state_change_register(l2_stp_msti_state_change_callback_t callback)
{
    mesa_rc rc;

    TEMP_LOCK();
    rc = l2_stp_register((void **)l2_stp_msti_state_change_table, ARRSZ(l2_stp_msti_state_change_table), "l2_stp_msti_state_change_table", (void *)callback);
    TEMP_UNLOCK();

    return rc;
}
#endif

mesa_rc l2port_iter_init(l2port_iter_t *l2pit, vtss_isid_t isid, l2port_iter_type_t l2type)
{
    l2pit->s_order = (l2type & L2PORT_ITER_ISID_ALL) ? SWITCH_ITER_SORT_ORDER_ISID_ALL : (l2type & L2PORT_ITER_ISID_CFG) ? ((l2type & L2PORT_ITER_USID_ORDER) ? SWITCH_ITER_SORT_ORDER_USID_CFG : SWITCH_ITER_SORT_ORDER_ISID_CFG) : ((l2type & L2PORT_ITER_USID_ORDER) ? SWITCH_ITER_SORT_ORDER_USID : SWITCH_ITER_SORT_ORDER_ISID);
    l2pit->p_order = (l2type & L2PORT_ITER_PORT_ALL) ? PORT_ITER_SORT_ORDER_IPORT_ALL : PORT_ITER_SORT_ORDER_IPORT;
    l2pit->isid_req = isid;
    l2pit->itertype_req = l2pit->itertype_pend = l2type;
    l2pit->ix = -1;

    (void)switch_iter_init(&l2pit->sit, l2pit->isid_req, l2pit->s_order);
    (void)port_iter_init(&l2pit->pit, &l2pit->sit, VTSS_ISID_GLOBAL, l2pit->p_order, PORT_ITER_FLAGS_NORMAL);

    return VTSS_RC_OK;
}

BOOL l2port_iter_getnext(l2port_iter_t *l2pit)
{
    if (l2pit->itertype_pend & L2PORT_ITER_TYPE_PHYS) {
        if (port_iter_getnext(&l2pit->pit)) {
            l2pit->isid = l2pit->sit.isid;
            l2pit->usid = l2pit->sit.usid;
            l2pit->iport = l2pit->pit.iport;
            l2pit->uport = l2pit->pit.uport;
            l2pit->l2port = L2PORT2PORT(l2pit->isid, l2pit->iport);
            l2pit->type = L2PORT_ITER_TYPE_PHYS;
            return TRUE;
        }

        l2pit->itertype_pend = (l2port_iter_type_t)(l2pit->itertype_pend & (~L2PORT_ITER_TYPE_PHYS)); /* Done physports */

        /* Reinit for aggrs */
        (void)switch_iter_init(&l2pit->sit, l2pit->isid_req, l2pit->s_order);
    }

    /*lint --e{506} ... yes, L2_MAX_LLAGS_/L2_MAX_GLAGS_ are constants! */
    if ((l2pit->itertype_pend & L2PORT_ITER_TYPE_LLAG) && (L2_MAX_LLAGS_ > 0)) {
        if (l2pit->ix < 0 || l2pit->ix == (L2_MAX_LLAGS_ - 1)) {
            if (!switch_iter_getnext(&l2pit->sit)) {
                l2pit->itertype_pend = (l2port_iter_type_t) (l2pit->itertype_pend & (~L2PORT_ITER_TYPE_LLAG)); /* Done LLAGs */
                l2pit->ix = -1;                                 /* If we're doing GLAGS too */
                goto glags;
            }
            l2pit->ix = 0;      /* Start new series */
        } else {
            l2pit->ix++;
        }

        l2pit->isid = l2pit->sit.isid;
        l2pit->usid = l2pit->sit.usid;
        l2pit->iport = VTSS_PORT_NO_NONE;
        l2pit->uport = VTSS_PORT_NO_NONE;
        l2pit->l2port = L2LLAG2PORT(l2pit->sit.isid, l2pit->ix);
        l2pit->type = L2PORT_ITER_TYPE_LLAG;
        return TRUE;
    }

glags:
    if ((l2pit->itertype_pend & L2PORT_ITER_TYPE_GLAG) && (L2_MAX_GLAGS_ > 0)) {
        if (++l2pit->ix < L2_MAX_GLAGS_) {
            l2pit->isid = VTSS_ISID_UNKNOWN;
            l2pit->usid = VTSS_ISID_UNKNOWN;
            l2pit->iport = VTSS_PORT_NO_NONE;
            l2pit->uport = VTSS_PORT_NO_NONE;
            l2pit->l2port = L2GLAG2PORT(l2pit->ix);
            l2pit->type = L2PORT_ITER_TYPE_GLAG;
            return TRUE;
        }

        l2pit->itertype_pend = (l2port_iter_type_t)(l2pit->itertype_pend & (~L2PORT_ITER_TYPE_GLAG)); /* Done GLAGs*/
    }

    return FALSE;
}

#ifdef VTSS_SW_OPTION_MSTP
vtss_appl_mstp_msti_t l2_mstid2msti(vtss_appl_mstp_mstid_t mstid)
{
    if (mstid == VTSS_MSTID_TE) {
        return MESA_MSTI_END - 1;    // Maps to last MSTI
    }

    VTSS_ASSERT(mstid < VTSS_APPL_MSTP_MAX_MSTI);
    return (vtss_appl_mstp_msti_t)mstid; // Maps directly for the rest
}
#endif

/*
 * Message indication function
 */
static BOOL l2_msg_rx(void *contxt, const void *rx_msg, size_t len, vtss_module_id_t modid, u32 isid)
{
    const l2_msg_t *msg = (const l2_msg_t *)rx_msg;

    T_N("Sid %u, rx %zd bytes, msg %d", isid, len, msg->msg_id);

    switch (msg->msg_id) {
    case L2_MSG_ID_FRAME_RX_IND: {
        l2_port_no_t l2port = (msg->data.rx_ind.glag_no == VTSS_GLAG_NO_NONE) ? L2PORT2PORT(isid, msg->data.rx_ind.switchport) : L2GLAG2PORT(msg->data.rx_ind.glag_no);
        l2_do_rx_callback(msg->data.rx_ind.modid, &msg[1], msg->data.rx_ind.len, msg->data.rx_ind.vid, l2port);
        break;
    }

    default:
        T_W("Unhandled msg %d", msg->msg_id);
    }

    return TRUE;
}

/*
 * Stack Register
 */
static void l2_stack_register(void)
{
    msg_rx_filter_t filter;
    mesa_rc         rc;

    memset(&filter, 0, sizeof(filter));
    filter.cb    = l2_msg_rx;
    filter.modid = VTSS_MODULE_ID_L2PROTO;

    rc =  msg_rx_filter_register(&filter);

    VTSS_ASSERT(rc == VTSS_RC_OK);
}

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

mesa_rc l2_init(vtss_init_data_t *data)
{
    vtss_common_stpstate_t common_stp_state;
    mesa_stp_state_t       stp_state;
    int                    i;

    switch (data->cmd) {
    case INIT_CMD_INIT:
#ifdef VTSS_SW_OPTION_MSTP
        memset(l2_msti_map, 0, sizeof(l2_msti_map));
#endif
        critd_init(&l2_mutex, "l2", VTSS_MODULE_ID_L2PROTO, CRITD_TYPE_MUTEX);
        break;

    case INIT_CMD_START:
        /* Register for stack messages */
        l2_stack_register();
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        /* All ports shall be discarding at startup */
#ifdef VTSS_SW_OPTION_MSTP
        common_stp_state = VTSS_COMMON_STPSTATE_DISCARDING;
        stp_state        = MESA_STP_STATE_DISCARDING;
#else
        // When STP is not part of the product (but L2Proto is), we need
        // to initialize the STP state to FORWARDING or no-one else will.
        common_stp_state = VTSS_COMMON_STPSTATE_FORWARDING;
        stp_state        = MESA_STP_STATE_FORWARDING;
#endif

        for (i = 0; i < (int)cached_stpstates.size(); i++) {
            cached_stpstates[i] = common_stp_state;
        }

        for (i = 0; i < (int)port_stp_state.size(); i++) {
            port_stp_state[i] = stp_state;
        }

        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

