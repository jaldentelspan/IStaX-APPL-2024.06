/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "critd_api.h"
#include "msg_api.h"
#include "mirror_api.h"
#include "mirror_icfg.h"
#include "mirror.h"
#include "mirror_basic_api.h"
#include "vlan_api.h"
#ifdef VTSS_SW_OPTION_IP
#include "ip_api.h"
#endif
#include "port_api.h"
#include "misc_api.h"
#include "port_iter.hxx"

#include "microchip/ethernet/switch/api.h"

#if defined(VTSS_SW_OPTION_MVR)
#include "mvr_api.h"
#endif /* VTSS_SW_OPTION_MVR */
#if defined(VTSS_SW_OPTION_VOICE_VLAN)
//#include "voice_vlan_api.h"
#endif /* VTSS_SW_OPTION_VOICE_VLAN */
#if defined(VTSS_SW_OPTION_SYSLOG)
#include "syslog_api.h"            /* For definition of S_E.   */
#endif /* VTSS_SW_OPTION_SYSLOG */
#ifdef VTSS_SW_OPTION_MSTP
#include "mstp_api.h"              /* For MSTP port config     */
#endif /* VTSS_SW_OPTION_MSTP */

#if defined(VTSS_SW_OPTION_EEE)
#include "eee_api.h"
#endif

#include "standalone_api.h"

// For public header
#include "vtss_common_iterator.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-static-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"
#include "vtss/appl/types.hxx"

/*************************************************************************
 ** Global variables
 *************************************************************************/
//#ifdef __cplusplus
//extern "C" {
//#endif

static rmirror_global_t  mem_rmirror_gconf; // RMIRROR configuration located in RAM memory
static int _rmirror_global_status = 0;
static int rmirror_local_lowest_isid_find(void);

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "rmirror", "RMIRROR"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    },
    [TRACE_GRP_ICLI] = {
        "iCLI",
        "ICLI",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define RMIRROR_CRIT_ENTER() critd_enter(&mem_rmirror_gconf.crit, __FILE__, __LINE__)
#define RMIRROR_CRIT_EXIT()  critd_exit( &mem_rmirror_gconf.crit, __FILE__, __LINE__)

#define RMIRROR_DEFAULT_VID       200

/*************************************************************************
 ** Various global functions
 *************************************************************************/

/* Convert RMIRROR error code to text */
const char *mirror_error_txt(mesa_rc rc)
{
    switch (rc) {
    case RMIRROR_ERROR_MUST_BE_PRIMARY_SWITCH:
        return "RMIRROR: operation only valid on the primary switch.";

    case RMIRROR_ERROR_ISID:
        return "RMIRROR: invalid Switch ID.";

    case RMIRROR_ERROR_INV_PARAM:
        return "RMIRROR: invalid parameter supplied to function.";

    case RMIRROR_ERROR_VID_IS_CONFLICT:
        return "RMIRROR: VID is conflict with others.";

    case RMIRROR_ERROR_ALLOCATE_MEMORY:
        return "RMIRROR: Allocate memory error.";

    case RMIRROR_ERROR_CONFLICT_WITH_EEE:
        return "RMIRROR only allowed if EEE disabled.";

    case RMIRROR_ERROR_SOURCE_SESSION_EXCEED_THE_LIMIT:
        return "Mirror or RMirror source session has reached the limit.";

    case RMIRROR_ERROR_SOURCE_PORTS_ARE_USED_BY_OTHER_SESSIONS:
        return "The source ports are used as destination or reflector ports by other sessions";
    case RMIRROR_ERROR_DESTINATION_PORTS_ARE_USED_BY_OTHER_SESSIONS:
        return "The destination ports are used as source or reflector ports by other sessions";
    case RMIRROR_ERROR_REFLECTOR_PORT_IS_USED_BY_OTHER_SESSIONS:
        return "The reflector port is used as source or destination or reflector ports by other sessions";
    case RMIRROR_ERROR_REFLECTOR_PORT_IS_INCLUDED_IN_SRC_OR_DEST_PORTS:
        return "The reflector port is included in source or destination ports";

    case RMIRROR_ERROR_REFLECTOR_PORT_IS_INVALID:
        return "The port can't be used as reflector port";

    case RMIRROR_ERROR_SOURCE_VIDS_INCLUDE_RMIRROR_VID:
        return "Source VLANs can't include RMirror VLAN";
    case RMIRROR_ERROR_DESTINATION_PORTS_EXCEED_THE_LIMIT:
        return "The destination ports of Mirror session have reached the limit";

    default:
        return "RMIRROR: unknown error code.";
    }
}

/*************************************************************************
 ** Various local functions
 *************************************************************************/

/* procedure that writes the global configuration variable to the chip registers */
static void rmirror_local_regs_update(const vtss_isid_t isid, rmirror_local_switch_conf_t conf)
{
#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)

    T_D("enter");

    T_D("set loopback port on port %d with API", VTSS_SW_OPTION_MIRROR_LOOP_PORT);

    T_D("exit");

#else

    static BOOL             rmirror_init_local_variable = TRUE;
    static mesa_port_no_t   cache_reflector_port[VTSS_MIRROR_SESSION_MAX_CNT];
    vtss_phy_loopback_t     loopback_conf;
    int                     i;

    T_D("enter");
    RMIRROR_CRIT_ENTER();

    if (rmirror_init_local_variable) {
        for (i = 0; i < VTSS_MIRROR_SESSION_MAX_CNT; ++i ) {
            cache_reflector_port[i] = VTSS_PORT_NO_NONE;
        }
        rmirror_init_local_variable = FALSE;
    }

    for (i = 0; i < VTSS_MIRROR_SESSION_MAX_CNT; ++i ) {
        if (conf.reflector_port[i] != cache_reflector_port[i]) {

            T_D("i = %d, reflector port = %d, cache port = %d", i, conf.reflector_port[i], cache_reflector_port[i]);

            // disable old configuration
            if (cache_reflector_port[i] != VTSS_PORT_NO_NONE) {
                if (vtss_phy_loopback_get(NULL, cache_reflector_port[i], &loopback_conf) != VTSS_RC_OK) {
                    T_D("Failed to Get the loopback settings when calling vtss_phy_loopback_get() for reflector port %u", cache_reflector_port[i]);
                }

                loopback_conf.far_end_enable =  FALSE;
                loopback_conf.near_end_enable = FALSE;
                (void) vtss_phy_loopback_set(NULL, cache_reflector_port[i], loopback_conf);
                T_D("disable phy loopback, port = %d", cache_reflector_port[i]);
            }

            cache_reflector_port[i] = conf.reflector_port[i];

            // enable new configuration
            if (cache_reflector_port[i] != VTSS_PORT_NO_NONE) {
                if (vtss_phy_loopback_get(NULL, cache_reflector_port[i], &loopback_conf) != VTSS_RC_OK) {
                    T_D("Failed to Get the loopback settings when calling vtss_phy_loopback_get() for reflector port %u", cache_reflector_port[i]);
                }
                loopback_conf.far_end_enable =  FALSE;
                loopback_conf.near_end_enable = TRUE;
                (void) vtss_phy_loopback_set(NULL, cache_reflector_port[i], loopback_conf);
                T_D("enable phy loopback, port = %d", cache_reflector_port[i]);
            }
        }
    }



    RMIRROR_CRIT_EXIT();
    T_D("exit");

#endif

    return;
}

/* reset the configuration to default */
static mesa_rc rmirror_local_default_set(rmirror_stack_conf_t *conf, int size)
{
    BOOL                    found = FALSE;
#if !defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    vtss_isid_t             isid = rmirror_local_lowest_isid_find();
    mesa_port_no_t          reflector_port = 0;
#endif
    port_iter_t             pit;
    meba_port_cap_t         cap = 0;
    int                     i = 0;

    /* initialize RMIRROR configuration */
    for (i = 0; i < size; i++) {
        vtss_clear(conf[i]);
    }

    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        if (port_cap_get(pit.iport, &cap) == VTSS_RC_OK && (cap & MEBA_PORT_CAP_COPPER)) {
            found = TRUE;
            T_D("got %d/%d", isid, pit.iport);
            break;
        }
        T_D(" %d/%d isn't pure copper port(" VPRI64x ")", isid, pit.iport, cap);
    }

    if (found == TRUE) {
        reflector_port = pit.iport;
    }

    for (i = 0; i < VTSS_MIRROR_SESSION_MAX_CNT; ++i ) {
        conf[i].vid = RMIRROR_DEFAULT_VID;
#if !defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
        conf[i].rmirror_switch_id = isid;
        conf[i].reflector_port = reflector_port;
        conf[i].type = VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR;
#endif
    }

    return VTSS_RC_OK;
}

/* Find the switch with the lowest USID number */
static int rmirror_local_lowest_isid_find(void)
{
    int isid;

    // Default for standalone
    isid = VTSS_ISID_START;

    return isid;
}

/*************************************************************************
 ** Message module functions
 *************************************************************************/

/* RMIRROR msg text */
static const char *rmirror_msg_id_txt(rmirror_msg_id_t msg_id)
{
    const char *txt;

    switch (msg_id) {
    case RMIRROR_MSG_ID_CONF_SET_REQ:
        txt = "RMIRROR_MSG_ID_CONF_SET_REQ";
        break;
    default:
        txt = "?";
        break;
    }

    return txt;
}

/* Allocate request/reply buffer */
static void rmirror_msg_alloc(rmirror_msg_buf_t *buf, BOOL request)
{

    RMIRROR_CRIT_ENTER();
    buf->sem = (request ? &mem_rmirror_gconf.request.sem : &mem_rmirror_gconf.reply.sem);
    buf->msg = (request ?  mem_rmirror_gconf.request.msg :  mem_rmirror_gconf.reply.msg);
    RMIRROR_CRIT_EXIT();

    vtss_sem_wait(buf->sem);
}

/* Getting message from the message module. */
static BOOL rmirror_msg_rx(void *contxt, const void *const rx_msg, const size_t len, const vtss_module_id_t modid, const u32 isid)
{
    rmirror_msg_id_t msg_id = *(rmirror_msg_id_t *)rx_msg;

    T_D("enter, msg_id: %d, %s,  len: %zd, isid: %u", msg_id, rmirror_msg_id_txt(msg_id), len, isid);

    switch (msg_id) {
    case RMIRROR_MSG_ID_CONF_SET_REQ: {
        rmirror_conf_set_req_t *msg;

        msg = (rmirror_conf_set_req_t *)rx_msg;
        rmirror_local_regs_update(isid, msg->conf);
        break;
    }
    default:
        T_W("unknown message ID: %d", msg_id);
        break;
    }

    T_D("exit");
    return TRUE;
}

/* Release message buffer */
static void rmirror_msg_tx_done(void *contxt, void *msg, msg_tx_rc_t rc)
{
    rmirror_msg_id_t msg_id = *(rmirror_msg_id_t *)msg;

    T_D("msg_id: %d, %s", msg_id, rmirror_msg_id_txt(msg_id));
    vtss_sem_post((vtss_sem_t *)contxt);

    return;
}

/* Send message */
static void rmirror_msg_tx(rmirror_msg_buf_t *buf, vtss_isid_t isid, size_t len)
{
    rmirror_msg_id_t msg_id = *(rmirror_msg_id_t *)buf->msg;

    T_D("msg_id: %s, len: %zd, isid: %d", rmirror_msg_id_txt(msg_id), len, isid);
    msg_tx_adv(buf->sem, rmirror_msg_tx_done, MSG_TX_OPT_DONT_FREE,
               VTSS_MODULE_ID_RMIRROR, isid, buf->msg, len);

    return;
}

/**********************************************************************
 ** Configuration functions
 **********************************************************************/

/* Determine if global configuration has changed */
static int rmirror_conf_global_changed(rmirror_switch_conf_t *old, rmirror_switch_conf_t *new_conf)
{
    return (new_conf->vid != old->vid || new_conf->type != old->type || new_conf->rmirror_switch_id != old->rmirror_switch_id ||
            new_conf->reflector_port != old->reflector_port || new_conf->enabled != old->enabled ||
            memcmp(new_conf->source_vid.data(), old->source_vid.data(), old->source_vid.mem_size()) != 0);
}

/* Determine if switch configuration has changed */
static int rmirror_conf_local_changed(rmirror_switch_conf_t *old, rmirror_switch_conf_t *new_conf)
{
    return (memcmp(new_conf->source_port.data(), old->source_port.data(), old->source_port.mem_size()) ||
            memcmp(new_conf->intermediate_port.data(), old->intermediate_port.data(), old->intermediate_port.mem_size()) ||
            memcmp(new_conf->destination_port.data(), old->destination_port.data(), old->destination_port.mem_size()) ||
            new_conf->cpu_src_enable != old->cpu_src_enable ||
            new_conf->cpu_dst_enable != old->cpu_dst_enable);
}

/* Save RMIRROR configuration to flash */
static mesa_rc rmirror_configuration_save(void)
{
    mesa_rc             rc = VTSS_RC_OK;

    T_D("exit");
    return rc;
}

/* RMIRROR set VLAN */
static mesa_rc _rmirror_conf_vlan_set(vtss_isid_t isid)
{
    mesa_rc                    rc = VTSS_RC_OK;
    rmirror_stack_conf_t       cpy_stack_conf, *cpy_stack_conf_ptr = &cpy_stack_conf;
    vtss_appl_vlan_port_conf_t vlan_conf;
    vtss_appl_vlan_entry_t     vlan_member;
    int                        isid_idx;
    int                        session_idx;
    switch_iter_t              sit;
    port_iter_t                pit;
    BOOL                       is_set;

#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    mesa_vlan_port_conf_t       api_conf;
    mesa_port_no_t              port_no;
#endif

    T_D("enter, isid = %d", isid);

    /* check switch ID */
    if ( !VTSS_ISID_LEGAL(isid) ) {
        T_D("exit, isid: %d not legal", isid);
        return RMIRROR_ERROR_ISID;
    }
    if (!msg_switch_exists(isid)) {
        T_D("exit, isid: %d not exist", isid);
        return RMIRROR_ERROR_ISID;
    }

    /************************  set VLAN       ************************/
    for (session_idx = 0; session_idx < VTSS_MIRROR_SESSION_MAX_CNT; session_idx++) {

        RMIRROR_CRIT_ENTER();
        cpy_stack_conf = mem_rmirror_gconf.stack_conf[session_idx];
        RMIRROR_CRIT_EXIT();

        T_N("[set VLAN], isid = %u, session_idx = %d, enabled = %d, type = %d", isid, session_idx, cpy_stack_conf_ptr->enabled, cpy_stack_conf_ptr->type);
        if (cpy_stack_conf_ptr->enabled) {

            switch (cpy_stack_conf_ptr->type) {
            case VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE:
                /********* set reflector port *********/
#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
                memset(&api_conf, 0x0, sizeof(api_conf));
                port_no = VTSS_SW_OPTION_MIRROR_LOOP_PORT;

                api_conf.pvid = cpy_stack_conf_ptr->vid;
                api_conf.ingress_filter = FALSE;
                api_conf.port_type = MESA_VLAN_PORT_TYPE_UNAWARE;

                if (cpy_stack_conf_ptr->reflector_port == 1) {
                    api_conf.untagged_vid = VTSS_VIDS;    // untagged all
                } else {
                    api_conf.untagged_vid = VTSS_VID_NULL;  // tagged all
                }

                T_D("VLAN Port change port %d, pvid %d", port_no, api_conf.pvid);
                if ((rc = mesa_vlan_port_conf_set(NULL, port_no, &api_conf)) != VTSS_RC_OK) {
                    T_E("%u: %s", iport2uport(port_no), error_txt(rc));
                }
#else
                memset(&vlan_conf, 0x0, sizeof(vlan_conf));

                vlan_conf.hybrid.pvid = cpy_stack_conf_ptr->vid;
                vlan_conf.hybrid.ingress_filter = FALSE;
                vlan_conf.hybrid.port_type = VTSS_APPL_VLAN_PORT_TYPE_UNAWARE;
                vlan_conf.hybrid.flags = (VTSS_APPL_VLAN_PORT_FLAGS_PVID | VTSS_APPL_VLAN_PORT_FLAGS_INGR_FILT | VTSS_APPL_VLAN_PORT_FLAGS_AWARE);

                // set vlan
                if ((rc = vlan_mgmt_port_conf_set(cpy_stack_conf_ptr->rmirror_switch_id, cpy_stack_conf_ptr->reflector_port, &vlan_conf, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                    T_D("%u:%d: Unable to change VLAN PVID", cpy_stack_conf_ptr->rmirror_switch_id, cpy_stack_conf_ptr->reflector_port);
                }
#endif
                /********* set intermediate port *********/
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
                while (switch_iter_getnext(&sit)) {
                    if (!msg_switch_exists(sit.isid)) {
                        continue;
                    }
                    is_set = FALSE;

                    if (vtss_appl_vlan_get(sit.isid, cpy_stack_conf_ptr->vid, &vlan_member, FALSE, VTSS_APPL_VLAN_USER_RMIRROR) != VTSS_RC_OK) {
                        vtss_clear(vlan_member);
                        vlan_member.vid = cpy_stack_conf_ptr->vid;
                        T_D("%u: Unable to get VLAN (%u)", sit.isid - 1, cpy_stack_conf_ptr->vid);
                    }

                    isid_idx = sit.isid - VTSS_ISID_START;
                    (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        if (cpy_stack_conf_ptr->intermediate_port[isid_idx][pit.iport]) {
                            vlan_member.ports[pit.iport] = TRUE;
                            is_set = TRUE;

                            memset(&vlan_conf, 0x0, sizeof(vlan_conf));
                            vlan_conf.hybrid.tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL;
                            vlan_conf.hybrid.flags = VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE;

                            // set vlan
                            if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &vlan_conf, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                                T_D("%u:%d: Unable to change VLAN setting", sit.isid, pit.iport);
                            }
                        }
                    }

                    if (is_set) {
                        if ((rc = vlan_mgmt_vlan_add(sit.isid, &vlan_member, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                            T_D("%u: Unable to change VLAN (%u)", sit.isid, cpy_stack_conf_ptr->vid);
                        }
                    }
                }

                break;
            case VTSS_APPL_RMIRROR_SWITCH_TYPE_INTERMEDIATE:
                /********* set intermediate port *********/
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
                while (switch_iter_getnext(&sit)) {
                    if (!msg_switch_exists(sit.isid)) {
                        continue;
                    }
                    is_set = FALSE;

                    if (vtss_appl_vlan_get(sit.isid, cpy_stack_conf_ptr->vid, &vlan_member, FALSE, VTSS_APPL_VLAN_USER_RMIRROR) != VTSS_RC_OK) {
                        vtss_clear(vlan_member);
                        vlan_member.vid = cpy_stack_conf_ptr->vid;
                        T_D("%u: Unable to get VLAN (%u)", sit.isid, cpy_stack_conf_ptr->vid);
                    }

                    isid_idx = sit.isid - VTSS_ISID_START;
                    (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        if (cpy_stack_conf_ptr->intermediate_port[isid_idx][pit.iport]) {
                            vlan_member.ports[pit.iport] = TRUE;
                            is_set = TRUE;

                            memset(&vlan_conf, 0x0, sizeof(vlan_conf));
                            vlan_conf.hybrid.tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_TAG_ALL;
                            vlan_conf.hybrid.flags = VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE;

                            // set vlan
                            if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &vlan_conf, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                                T_D("%u:%d: Unable to change VLAN setting", sit.isid, pit.iport);
                            }
                        }
                    }

                    if (is_set) {
                        if ((rc = vlan_mgmt_vlan_add(sit.isid, &vlan_member, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                            T_D("%u: Unable to change VLAN (%u)", sit.isid, cpy_stack_conf_ptr->vid);
                        }
                    }
                }

                break;
            case VTSS_APPL_RMIRROR_SWITCH_TYPE_DESTINATION:
                /********* set destination port *********/
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
                while (switch_iter_getnext(&sit)) {
                    if (!msg_switch_exists(sit.isid)) {
                        continue;
                    }
                    is_set = FALSE;

                    if (vtss_appl_vlan_get(sit.isid, cpy_stack_conf_ptr->vid, &vlan_member, FALSE, VTSS_APPL_VLAN_USER_RMIRROR) != VTSS_RC_OK) {
                        vtss_clear(vlan_member);
                        vlan_member.vid = cpy_stack_conf_ptr->vid;
                        T_D("%u: Unable to get VLAN (%u)", sit.isid, cpy_stack_conf_ptr->vid);
                    }

                    isid_idx = sit.isid - VTSS_ISID_START;
                    (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        if (cpy_stack_conf_ptr->destination_port[isid_idx][pit.iport]) {
//#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
                            memset(&vlan_conf, 0x0, sizeof(vlan_conf));
                            vlan_conf.hybrid.tx_tag_type = VTSS_APPL_VLAN_TX_TAG_TYPE_UNTAG_ALL;
                            vlan_conf.hybrid.flags = VTSS_APPL_VLAN_PORT_FLAGS_TX_TAG_TYPE;

                            // set vlan
                            if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &vlan_conf, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                                T_D("%u:%d: Unable to change VLAN setting", sit.isid, pit.iport);
                            }
//#endif
                            vlan_member.ports[pit.iport] = TRUE;
                            is_set = TRUE;
                        }
                    }

                    if (is_set) {
                        if ((rc = vlan_mgmt_vlan_add(sit.isid, &vlan_member, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                            T_D("%u: Unable to change VLAN (%u)", sit.isid, cpy_stack_conf_ptr->vid);
                        }
                    }
                }

                /********* set intermediate port *********/
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
                while (switch_iter_getnext(&sit)) {
                    if (!msg_switch_exists(sit.isid)) {
                        continue;
                    }
                    is_set = FALSE;

                    if (vtss_appl_vlan_get(sit.isid, cpy_stack_conf_ptr->vid, &vlan_member, FALSE, VTSS_APPL_VLAN_USER_RMIRROR) != VTSS_RC_OK) {
                        vtss_clear(vlan_member);
                        vlan_member.vid = cpy_stack_conf_ptr->vid;
                        T_D("%u: Unable to get VLAN (%u)", sit.isid, cpy_stack_conf_ptr->vid);
                    }

                    isid_idx = sit.isid - VTSS_ISID_START;
                    (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        if (cpy_stack_conf_ptr->intermediate_port[isid_idx][pit.iport]) {
                            vlan_member.ports[pit.iport] = TRUE;
                            is_set = TRUE;
                        }
                    }

                    if (is_set) {
                        if ((rc = vlan_mgmt_vlan_add(sit.isid, &vlan_member, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                            T_D("%u: Unable to change VLAN (%u)", sit.isid, cpy_stack_conf_ptr->vid);
                        }
                    }
                }
                break;
            default:
                T_D("unknown type");
                break;
            }
        }
    }

    T_D("exit");
    return rc;
}

static mesa_rc MIRROR_vid_conf_set(mesa_vid_t vid, bool do_mirror)
{
    mesa_vlan_vid_conf_t vid_conf;
    mesa_rc              rc;

    // Make sure that the calls to get() and set() are undivided
    VTSS_APPL_API_LOCK_SCOPE();

    if ((rc = mesa_vlan_vid_conf_get(NULL, vid, &vid_conf)) == VTSS_RC_OK) {
        vid_conf.mirror = do_mirror;
        rc = mesa_vlan_vid_conf_set(NULL, vid, &vid_conf);
    }

    return rc;
}

/* RMIRROR disable mirror */
static mesa_rc _rmirror_conf_mirror_unset(vtss_isid_t isid)
{
    mesa_rc                 rc = VTSS_RC_OK;
    rmirror_stack_conf_t    cpy_stack_conf, *cpy_stack_conf_ptr = &cpy_stack_conf;
    mirror_conf_t           mirror_conf;
    mirror_switch_conf_t    mirror_switch_conf;
    int                     session_idx;
    switch_iter_t           sit;
    port_iter_t             pit;
    int                     vlan_idx;

    T_D("enter, isid = %d", isid);

    /* check switch ID */
    if ( !VTSS_ISID_LEGAL(isid) ) {
        T_D("exit, isid: %d not legal", isid);
        return RMIRROR_ERROR_ISID;
    }
    if (!msg_switch_exists(isid)) {
        T_D("exit, isid: %d not exist", isid);
        return RMIRROR_ERROR_ISID;
    }

    /************************  disable Mirror      ************************/
    for (session_idx = 0; session_idx < VTSS_MIRROR_SESSION_MAX_CNT; session_idx++) {

        RMIRROR_CRIT_ENTER();
        cpy_stack_conf = mem_rmirror_gconf.stack_conf[session_idx];
        RMIRROR_CRIT_EXIT();

        T_N("[disable Mirror], isid = %u, session_idx = %d, enabled = %d, type = %d", isid, session_idx, cpy_stack_conf_ptr->enabled, cpy_stack_conf_ptr->type);
        if (cpy_stack_conf_ptr->enabled && cpy_stack_conf_ptr->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE) {
        } else {
            T_N("[disable Mirror], isid = %u, session_idx = %d, enabled = %d", isid, session_idx, cpy_stack_conf_ptr->enabled);
            // set mirror configuration
            mirror_conf.mirror_switch = rmirror_local_lowest_isid_find();
            mirror_conf.dst_port = VTSS_PORT_NO_NONE;
            (void) mirror_mgmt_conf_set(&mirror_conf);

            // set mirror switch configuration
            (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
            while (switch_iter_getnext(&sit)) {
                if (!msg_switch_exists(sit.isid)) {
                    continue;
                }
                T_N("[disable Mirror], isid = %u, index = %d, enabled = %d", isid, sit.isid, cpy_stack_conf_ptr->enabled);

                vtss_clear(mirror_switch_conf);
                (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    mirror_switch_conf.src_enable[pit.iport] = FALSE;
                    mirror_switch_conf.dst_enable[pit.iport] = FALSE;
                }
                mirror_switch_conf.cpu_src_enable = FALSE;
                mirror_switch_conf.cpu_dst_enable = FALSE;
                mirror_mgmt_switch_conf_set(sit.isid, &mirror_switch_conf);
            }

            // set VLAN-based mirror configuration
            for (vlan_idx = VTSS_APPL_VLAN_ID_MIN; vlan_idx <= VTSS_APPL_VLAN_ID_MAX; vlan_idx++) {
                rc = MIRROR_vid_conf_set(vlan_idx, false);
            }
        }
    }

    T_D("exit");
    return rc;
}

/* RMIRROR set loopback */
static mesa_rc _rmirror_conf_loopback_set(vtss_isid_t isid)
{
    mesa_rc                 rc = VTSS_RC_OK;
    rmirror_msg_buf_t       buf;
    rmirror_conf_set_req_t  *msg;
    rmirror_stack_conf_t    cpy_stack_conf, *cpy_stack_conf_ptr = &cpy_stack_conf;
    int                     session_idx;
#ifdef VTSS_SW_OPTION_MSTP
    int                     check_flag = FALSE;
    mesa_port_no_t          cache_port = VTSS_PORT_NO_NONE;
    vtss_isid_t             cache_isid = VTSS_ISID_START;
#endif


    T_D("enter, isid = %d", isid);

    /* check switch ID */
    if ( !VTSS_ISID_LEGAL(isid) ) {
        T_D("exit, isid: %d not legal", isid);
        return RMIRROR_ERROR_ISID;
    }
    if (!msg_switch_exists(isid)) {
        T_D("exit, isid: %d not exist", isid);
        return RMIRROR_ERROR_ISID;
    }

    /************************  Loopback ************************/
    /* Prepare for message tranfer */
    rmirror_msg_alloc(&buf, TRUE);
    msg = (rmirror_conf_set_req_t *)buf.msg;
    msg->msg_id = RMIRROR_MSG_ID_CONF_SET_REQ;

    /* get configuration and prepare to transmit it to the secondary switch */
    for (session_idx = 0; session_idx < VTSS_MIRROR_SESSION_MAX_CNT; session_idx++) {

        RMIRROR_CRIT_ENTER();
        cpy_stack_conf = mem_rmirror_gconf.stack_conf[session_idx];
        RMIRROR_CRIT_EXIT();

        T_N("[set Loopback], isid = %u, session_idx = %d, enabled = %d", isid, session_idx, cpy_stack_conf_ptr->enabled);
        if (cpy_stack_conf_ptr->enabled && cpy_stack_conf_ptr->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE && cpy_stack_conf_ptr->rmirror_switch_id == isid) {
            msg->conf.reflector_port[session_idx] = cpy_stack_conf_ptr->reflector_port;
            T_D("enabled port = %d, isid = %u", msg->conf.reflector_port[session_idx], cpy_stack_conf_ptr->rmirror_switch_id);
#ifdef VTSS_SW_OPTION_MSTP
            check_flag = TRUE;
            cache_port = cpy_stack_conf_ptr->reflector_port;
            cache_isid = cpy_stack_conf_ptr->rmirror_switch_id;
#endif
        } else {
            msg->conf.reflector_port[session_idx] = VTSS_PORT_NO_NONE;
            T_D("disabled port = %d", msg->conf.reflector_port[session_idx] );
        }
    }

    /* transmit the message */
    rmirror_msg_tx(&buf, isid, sizeof(*msg));


    // Inter-protocol check
    // If spanning tree is enabled, then we give the warning msg.
#ifdef VTSS_SW_OPTION_MSTP
    {
        mstp_port_param_t rstp_conf;
        BOOL              stp_enabled = FALSE;

        if (check_flag && !vtss_mstp_port_config_get(cache_isid, cache_port, &stp_enabled, &rstp_conf)) {
            if (stp_enabled) {
#if defined(VTSS_SW_OPTION_SYSLOG)
                S_PORT_W(cache_isid, cache_port, "RMIRROR-CONF-CONFLICT: Please disable STP and MAC learning on Interface %s.", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
#endif /* VTSS_SW_OPTION_SYSLOG */
                T_W("Please disable STP and MAC learning on related ports, switch %u port %d.", topo_isid2usid(cache_isid), iport2uport(cache_port));
            }
        }
    }
#endif /* VTSS_SW_OPTION_MSTP */

    T_D("exit");
    return rc;
}

/* RMIRROR enable mirror */
static mesa_rc _rmirror_conf_mirror_set(vtss_isid_t isid)
{
    mesa_rc                 rc = VTSS_RC_OK;
    rmirror_stack_conf_t    cpy_stack_conf, *cpy_stack_conf_ptr = &cpy_stack_conf;
    mirror_conf_t           mirror_conf;
    mirror_switch_conf_t    mirror_switch_conf;
    int                     isid_idx;
    int                     session_idx;
    switch_iter_t           sit;
    port_iter_t             pit;
    int                     vlan_idx;

    T_D("enter, isid = %d", isid);

    /* check switch ID */
    if ( !VTSS_ISID_LEGAL(isid) ) {
        T_D("exit, isid: %d not legal", isid);
        return RMIRROR_ERROR_ISID;
    }
    if (!msg_switch_exists(isid)) {
        T_D("exit, isid: %d not exist", isid);
        return RMIRROR_ERROR_ISID;
    }

    /************************  enable Mirror      ************************/
    for (session_idx = 0; session_idx < VTSS_MIRROR_SESSION_MAX_CNT; session_idx++) {

        RMIRROR_CRIT_ENTER();
        cpy_stack_conf = mem_rmirror_gconf.stack_conf[session_idx];
        RMIRROR_CRIT_EXIT();

        T_N("[enable Mirror], isid = %u, session_idx = %d, enabled = %d, type = %d", isid, session_idx, cpy_stack_conf_ptr->enabled, cpy_stack_conf_ptr->type);
        if (cpy_stack_conf_ptr->enabled) {
            switch (cpy_stack_conf_ptr->type) {
            case VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE:

                // set mirror switch configuration
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
                while (switch_iter_getnext(&sit)) {
                    if (!msg_switch_exists(sit.isid)) {
                        continue;
                    }

                    isid_idx = sit.isid - VTSS_ISID_START;
                    vtss_clear(mirror_switch_conf);
                    (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        mirror_switch_conf.src_enable[pit.iport] = cpy_stack_conf_ptr->source_port[isid_idx][pit.iport].rx_enable;
                        mirror_switch_conf.dst_enable[pit.iport] = cpy_stack_conf_ptr->source_port[isid_idx][pit.iport].tx_enable;
                    }

                    mirror_switch_conf.cpu_src_enable = cpy_stack_conf_ptr->cpu_src_enable[isid_idx];
                    mirror_switch_conf.cpu_dst_enable = cpy_stack_conf_ptr->cpu_dst_enable[isid_idx];
                    mirror_mgmt_switch_conf_set(sit.isid, &mirror_switch_conf);
                }

                // set mirror configuration
#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
                mirror_conf.mirror_switch = cpy_stack_conf_ptr->rmirror_switch_id;
                mirror_conf.dst_port = VTSS_SW_OPTION_MIRROR_LOOP_PORT;
                (void) mirror_mgmt_conf_set(&mirror_conf);
#else
                mirror_conf.mirror_switch = cpy_stack_conf_ptr->rmirror_switch_id;
                mirror_conf.dst_port = cpy_stack_conf_ptr->reflector_port;
                (void) mirror_mgmt_conf_set(&mirror_conf);
#endif

                // set VLAN-based mirror configuration
                for (vlan_idx = VTSS_APPL_VLAN_ID_MIN; vlan_idx <= VTSS_APPL_VLAN_ID_MAX; vlan_idx++) {
                    bool do_mirror = cpy_stack_conf_ptr->source_vid[vlan_idx];
                    rc = MIRROR_vid_conf_set(vlan_idx, do_mirror);
                    if (do_mirror) {
                        T_D("vlan-based enabled: vid = %d, %d", vlan_idx, do_mirror);
                    }
                }

                break;

#if RMIRROR_USE_RMIRROR_UI_INSTEAD_OF_MIRROR_UI
            case VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR:

                // set mirror switch configuration
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
                while (switch_iter_getnext(&sit)) {
                    if (!msg_switch_exists(sit.isid)) {
                        continue;
                    }
                    T_N("[enable Mirror], isid = %u, index = %d, enabled = %d", isid, sit.isid, cpy_stack_conf_ptr->enabled);

                    isid_idx = sit.isid - VTSS_ISID_START;
                    vtss_clear(mirror_switch_conf);
                    (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        mirror_switch_conf.src_enable[pit.iport] = cpy_stack_conf_ptr->source_port[isid_idx][pit.iport].rx_enable;
                        mirror_switch_conf.dst_enable[pit.iport] = cpy_stack_conf_ptr->source_port[isid_idx][pit.iport].tx_enable;

                        // find last destination port
                        if (cpy_stack_conf_ptr->destination_port[isid_idx][pit.iport]) {
                            // set mirror configuration
                            mirror_conf.mirror_switch = sit.isid;
                            mirror_conf.dst_port = pit.iport;
                            (void) mirror_mgmt_conf_set(&mirror_conf);
                            T_D("set mirror config");
                        }
                    }

                    mirror_switch_conf.cpu_src_enable = cpy_stack_conf_ptr->cpu_src_enable[isid_idx];
                    mirror_switch_conf.cpu_dst_enable = cpy_stack_conf_ptr->cpu_dst_enable[isid_idx];
                    mirror_mgmt_switch_conf_set(sit.isid, &mirror_switch_conf);
                }

                // set VLAN-based mirror configuration
                for (vlan_idx = VTSS_APPL_VLAN_ID_MIN; vlan_idx <= VTSS_APPL_VLAN_ID_MAX; vlan_idx++) {
                    bool do_mirror = cpy_stack_conf_ptr->source_vid[vlan_idx];
                    rc = MIRROR_vid_conf_set(vlan_idx, do_mirror);
                    if (do_mirror) {
                        T_D("vlan-based enabled: vid = %d, %d", vlan_idx, do_mirror);
                    }
                }

                break;
#endif
            default:
                break;
            }
        }
    }

    T_D("exit");
    return rc;
}

/* RMIRROR remove VLAN */
static mesa_rc _rmirror_conf_vlan_unset(vtss_isid_t isid)
{
    mesa_rc                    rc = VTSS_RC_OK;
    rmirror_stack_conf_t       cpy_stack_conf, *cpy_stack_conf_ptr = &cpy_stack_conf;
    vtss_appl_vlan_port_conf_t vlan_conf;
    vtss_appl_vlan_entry_t     vlan_member;
    int                        isid_idx;
    int                        session_idx;
    switch_iter_t              sit;
    port_iter_t                pit;

#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    mesa_vlan_port_conf_t       api_conf;
    mesa_port_no_t              port_no;
#endif

    T_D("enter, isid = %d", isid);

    /* check switch ID */
    if ( !VTSS_ISID_LEGAL(isid) ) {
        T_D("exit, isid: %d not legal", isid);
        return RMIRROR_ERROR_ISID;
    }
    if (!msg_switch_exists(isid)) {
        T_D("exit, isid: %d not exist", isid);
        return RMIRROR_ERROR_ISID;
    }

    /************************  remove VLAN and restore      ************************/
    for (session_idx = 0; session_idx < VTSS_MIRROR_SESSION_MAX_CNT; session_idx++) {

        RMIRROR_CRIT_ENTER();
        cpy_stack_conf = mem_rmirror_gconf.stack_conf[session_idx];
        RMIRROR_CRIT_EXIT();

        T_N("[remove VLAN ], isid = %u, session_idx = %d, enabled = %d, type = %d", isid, session_idx, cpy_stack_conf_ptr->enabled, cpy_stack_conf_ptr->type);
        if (!cpy_stack_conf_ptr->enabled) {

            switch (cpy_stack_conf_ptr->type) {
            case VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE:
                /********* set reflector port *********/

#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
                memset(&api_conf, 0x0, sizeof(api_conf));
                port_no = VTSS_SW_OPTION_MIRROR_LOOP_PORT;

                api_conf.pvid = VTSS_VID_DEFAULT;
                api_conf.untagged_vid = VTSS_VIDS;

                T_D("VLAN Port change port %d, pvid %d", port_no, api_conf.pvid);
                if ((rc = mesa_vlan_port_conf_set(NULL, port_no, &api_conf)) != VTSS_RC_OK) {
                    T_E("%u: %s", iport2uport(port_no), error_txt(rc));
                }
#else
                memset(&vlan_conf, 0x0, sizeof(vlan_conf));
                if ((rc = vlan_mgmt_port_conf_set(cpy_stack_conf_ptr->rmirror_switch_id, cpy_stack_conf_ptr->reflector_port, &vlan_conf, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                    T_D("%u:%d: Unable to change VLAN PVID", cpy_stack_conf_ptr->rmirror_switch_id, cpy_stack_conf_ptr->reflector_port);
                }
#endif

                /********* set intermediate port *********/
                memset(&vlan_conf, 0x0, sizeof(vlan_conf));
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
                while (switch_iter_getnext(&sit)) {
                    if (!msg_switch_exists(sit.isid)) {
                        continue;
                    }

                    isid_idx = sit.isid - VTSS_ISID_START;
                    (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        if (cpy_stack_conf_ptr->intermediate_port[isid_idx][pit.iport]) {
                            if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &vlan_conf, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                                T_D("%u:%u: Unable to change VLAN awareness", sit.isid, pit.iport);
                            }
                        }
                    }

                    vtss_clear(vlan_member);
                    vlan_member.vid = cpy_stack_conf_ptr->vid;
                    if ((rc = vlan_mgmt_vlan_add(sit.isid, &vlan_member, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                        T_D("%u: Unable to change VLAN (%u)", sit.isid, cpy_stack_conf_ptr->vid);
                    }
                    if ((rc = vlan_mgmt_vlan_del(sit.isid, cpy_stack_conf_ptr->vid, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                        T_D("restore default vlan_mgmt_vlan_del() failed!!!");
                    }
                }

                break;
            case VTSS_APPL_RMIRROR_SWITCH_TYPE_INTERMEDIATE:
                /********* set intermediate port *********/
                memset(&vlan_conf, 0x0, sizeof(vlan_conf));
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
                while (switch_iter_getnext(&sit)) {
                    if (!msg_switch_exists(sit.isid)) {
                        continue;
                    }

                    isid_idx = sit.isid - VTSS_ISID_START;
                    (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        if (cpy_stack_conf_ptr->intermediate_port[isid_idx][pit.iport]) {
                            if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &vlan_conf, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                                T_D("%u:%u: Unable to change VLAN awareness", sit.isid, pit.iport);
                            }
                        }
                    }

                    vtss_clear(vlan_member);
                    vlan_member.vid = cpy_stack_conf_ptr->vid;
                    if ((rc = vlan_mgmt_vlan_add(sit.isid, &vlan_member, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                        T_D("%u: Unable to change VLAN (%u)", sit.isid, cpy_stack_conf_ptr->vid);
                    }
                    if ((rc = vlan_mgmt_vlan_del(sit.isid, cpy_stack_conf_ptr->vid, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                        T_D("restore default vlan_mgmt_vlan_del() failed!!!");
                    }
                }

                break;
            case VTSS_APPL_RMIRROR_SWITCH_TYPE_DESTINATION:
                /********* set destination port *********/
                memset(&vlan_conf, 0x0, sizeof(vlan_conf));
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
                while (switch_iter_getnext(&sit)) {
                    if (!msg_switch_exists(sit.isid)) {
                        continue;
                    }

                    isid_idx = sit.isid - VTSS_ISID_START;
                    (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        if (cpy_stack_conf_ptr->destination_port[isid_idx][pit.iport]) {
                            if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &vlan_conf, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                                T_D("%u:%u: Unable to change VLAN PVID", sit.isid, pit.iport);
                            }
                        }
                    }

                    vtss_clear(vlan_member);
                    vlan_member.vid = cpy_stack_conf_ptr->vid;
                    if ((rc = vlan_mgmt_vlan_add(sit.isid, &vlan_member, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                        T_D("%u: Unable to change VLAN (%u)", sit.isid, cpy_stack_conf_ptr->vid);
                    }
                    if ((rc = vlan_mgmt_vlan_del(sit.isid, cpy_stack_conf_ptr->vid, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                        T_D("restore default vlan_mgmt_vlan_del() failed!!!");
                    }
                }

                /********* set intermediate port *********/
                memset(&vlan_conf, 0x0, sizeof(vlan_conf));
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
                while (switch_iter_getnext(&sit)) {
                    if (!msg_switch_exists(sit.isid)) {
                        continue;
                    }

                    isid_idx = sit.isid - VTSS_ISID_START;
                    (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                    while (port_iter_getnext(&pit)) {
                        if (cpy_stack_conf_ptr->intermediate_port[isid_idx][pit.iport]) {
                            if ((rc = vlan_mgmt_port_conf_set(sit.isid, pit.iport, &vlan_conf, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                                T_D("%u:%u: Unable to change VLAN awareness", sit.isid, pit.iport);
                            }
                        }
                    }

                    vtss_clear(vlan_member);
                    vlan_member.vid = cpy_stack_conf_ptr->vid;
                    if ((rc = vlan_mgmt_vlan_add(sit.isid, &vlan_member, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                        T_D("%u: Unable to change VLAN (%u)", sit.isid, cpy_stack_conf_ptr->vid);
                    }
                    if ((rc = vlan_mgmt_vlan_del(sit.isid, cpy_stack_conf_ptr->vid, VTSS_APPL_VLAN_USER_RMIRROR)) != VTSS_RC_OK) {
                        T_D("restore default vlan_mgmt_vlan_del() failed!!!");
                    }
                }

                break;
            case VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR:
                T_D("VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR, isid = %u", isid);
                break;
            default:
                T_D("unknown type");
                break;
            }
        }
    }

    T_D("exit");
    return rc;
}

/* read configuration and transmit configuration to the switch */
static mesa_rc rmirror_conf_set(vtss_isid_t isid)
{
    mesa_rc rc = VTSS_RC_OK;

    T_D("enter, isid = %d", isid);

    /* check switch ID */
    if ( !VTSS_ISID_LEGAL(isid) ) {
        T_D("exit, isid: %d not legal", isid);
        return RMIRROR_ERROR_ISID;
    }
    /* if the real switch don't exist on the system, we don't set the chip and return VTSS_RC_OK */
    if (!msg_switch_exists(isid)) {
        T_D("exit, isid: %d not exist", isid);
        return VTSS_RC_OK;
    }

    /************************  set VLAN       ************************/
    if ((rc = _rmirror_conf_vlan_set(isid)) != VTSS_RC_OK) {
        T_D("set VLAN fail, isid = %d", isid);
    }

    /************************  disable Mirror      ************************/
    if ((rc = _rmirror_conf_mirror_unset(isid)) != VTSS_RC_OK) {
        T_D("disable Mirror fail, isid = %d", isid);
    }

    /************************  Loopback ************************/
    if ((rc = _rmirror_conf_loopback_set(isid)) != VTSS_RC_OK) {
        T_D("set Loopback fail, isid = %d", isid);
    }

    /************************  enable Mirror      ************************/
    if ((rc = _rmirror_conf_mirror_set(isid)) != VTSS_RC_OK) {
        T_D("enable Mirror fail, isid = %d", isid);
    }

    /************************  remove VLAN and restore      ************************/
    if ((rc = _rmirror_conf_vlan_unset(isid)) != VTSS_RC_OK) {
        T_D("remove VLAN fail, isid = %d", isid);
    }

    T_D("exit");
    return rc;
}

static void rmirror_conf_read(BOOL force_defaults, vtss_isid_t isid_add)
{
    mesa_rc                 rc = VTSS_RC_OK;
    rmirror_stack_conf_t    new_rmirror_conf[VTSS_MIRROR_SESSION_MAX_CNT];
    int                     i;
    BOOL                    is_changed;
    switch_iter_t           sit;

    T_D("enter, force_defaults: %d, isid = %d", force_defaults, isid_add);

    is_changed = FALSE;

    // load old configuration
    RMIRROR_CRIT_ENTER();
    for (i = 0; i < VTSS_MIRROR_SESSION_MAX_CNT; i++) {
        new_rmirror_conf[i] = mem_rmirror_gconf.stack_conf[i];
    }
    RMIRROR_CRIT_EXIT();

    /* reset to default switch id */
    if (isid_add == VTSS_ISID_GLOBAL) {

        // reset all hardware setting before reset global RMIRROR configuration
        for (i = 0; i < VTSS_MIRROR_SESSION_MAX_CNT; ++i ) {
            RMIRROR_CRIT_ENTER();
            mem_rmirror_gconf.stack_conf[i].enabled = FALSE;
            RMIRROR_CRIT_EXIT();
        }

        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
        while (switch_iter_getnext(&sit)) {
            T_D("reset default, isid = %d", sit.isid);
            rc = rmirror_conf_set(sit.isid);
            if (rc != VTSS_RC_OK) {
                T_D("Error: rc = %d", rc);
            }
        }

        /* Use default values */
        rc = rmirror_local_default_set(new_rmirror_conf, VTSS_MIRROR_SESSION_MAX_CNT);
        if (rc != VTSS_RC_OK) {
            T_D("Error: rc = %d", rc);
        }
    }

    RMIRROR_CRIT_ENTER();
    for (i = 0; i < VTSS_MIRROR_SESSION_MAX_CNT; i++) {
        if (vtss_memcmp(mem_rmirror_gconf.stack_conf[i], new_rmirror_conf[i])) {
            // reset global RMIRROR configuration
            mem_rmirror_gconf.stack_conf[i] = new_rmirror_conf[i];
            is_changed = TRUE;
        }
    }
    RMIRROR_CRIT_EXIT();

    if (is_changed && force_defaults) {
        /* Apply all configuration to switch */
        T_D("rmirror_conf_set, isid = %d", isid_add);
        rc = rmirror_conf_set(isid_add);
    }

    T_D("exit");
    if (rc != VTSS_RC_OK) {
        T_D("Error: rc = %d", rc);
    }

    return;
}

static void rmirror_port_shutdown(mesa_port_no_t port_no)
{
    mesa_rc                 rc = VTSS_RC_OK;
    rmirror_stack_conf_t    *rmirror_stack_conf;
    int                     idx;

    T_D("enter");

    RMIRROR_CRIT_ENTER();
    for (idx = 0; idx < VTSS_MIRROR_SESSION_MAX_CNT; idx++) {

        rmirror_stack_conf = &mem_rmirror_gconf.stack_conf[idx];

        if (rmirror_stack_conf->enabled && rmirror_stack_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE && msg_switch_is_local(rmirror_stack_conf->rmirror_switch_id)) {

            if (port_no == rmirror_stack_conf->reflector_port) {
                /* display warning msg on the syslog and console */
#if defined(VTSS_SW_OPTION_SYSLOG)
                S_PORT_W(rmirror_stack_conf->rmirror_switch_id, port_no, "RMIRROR-CONF-CONFLICT: Shut down Interface %s, rmirror module is out of order.", SYSLOG_PORT_INFO_REPLACE_KEYWORD);
#endif /* VTSS_SW_OPTION_SYSLOG */
                T_W("RMIRROR-CONFLICTED_CONF: shut down switch %u port %d, rmirror module is out of order.", topo_isid2usid(rmirror_stack_conf->rmirror_switch_id), iport2uport(port_no));
            }
            break;
        }
    }
    RMIRROR_CRIT_EXIT();
#if 0
    if (is_changed) {

        /* update the switch in question */
        T_N("[[[[[[[[[[[[change switch configuration]]]]]]]]]]]], isid = %u", cache_switch_id);
        rc = rmirror_conf_set(cache_switch_id);

        /* store in flash */
        rc = rmirror_configuration_save();
    }
#endif
    T_D("exit, rc = %d", rc);
}

/**********************************************************************
 ** Management functions
 **********************************************************************/

/* Reset the configuration to default */
void rmirror_mgmt_default_set(rmirror_switch_conf_t *conf)
{
#if !defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    vtss_ifindex_t          ifindex, ifindex_pre;
    vtss_ifindex_elm_t      ife;
    BOOL                    found = FALSE;
    vtss_appl_port_status_t port_status;
    mesa_rc                 rc_if = VTSS_RC_ERROR, rc_if_status = VTSS_RC_ERROR;
#endif
    /* initialize RMIRROR configuration */
    vtss_clear(*conf);

    conf->vid = RMIRROR_DEFAULT_VID;
#if !defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    memset(&ifindex, 0, sizeof(vtss_ifindex_t));
    memset(&ifindex_pre, 0, sizeof(vtss_ifindex_t));
    while (VTSS_RC_OK == (rc_if = vtss_ifindex_getnext_port(&ifindex_pre, &ifindex)) && VTSS_RC_OK == (rc_if_status = vtss_appl_port_status_get(ifindex, &port_status))) {
        // we only support reflector port on copper ports.
        if (port_status.static_caps & MEBA_PORT_CAP_COPPER) {
            found = TRUE;
            T_D("got ifindex %du", ifindex.private_ifindex_data_do_not_use_directly);
            break;
        }
        memcpy(&ifindex_pre, &ifindex, sizeof(vtss_ifindex_t));
    }

    if (found == FALSE) {
        /* In INIT_CMD_ICFG_LOADING_PRE stage, ICFG will generate startup-config if it doens't exist,
           however, the ifIndex or PORT module might not be ready, so RMirror need to get
         the local default configuration for reflect port. */
        if (rc_if != VTSS_RC_OK || rc_if_status != VTSS_RC_OK) {
            T_D("%s isn't ready yet, choose reflector port locally", rc_if != VTSS_RC_OK ? "IFINDEX" : "PORT STATUS" );
            RMIRROR_CRIT_ENTER();
            conf->rmirror_switch_id = mem_rmirror_gconf.stack_conf[0].rmirror_switch_id;
            conf->reflector_port = mem_rmirror_gconf.stack_conf[0].reflector_port;
            RMIRROR_CRIT_EXIT();
        } else {
            T_W("not found valid reflector port");
            conf->rmirror_switch_id = rmirror_local_lowest_isid_find();
            conf->reflector_port = 0;
        }
    } else {
        (void)vtss_ifindex_decompose(ifindex, &ife);
        conf->rmirror_switch_id = ife.isid;
        conf->reflector_port = ife.ordinal;
    }

    T_D("rmirror_switch_id = %d, conf->reflector_port = %d", conf->rmirror_switch_id, conf->reflector_port);
#endif
    return;
}

/* Check RMIRROR VLAN ID is conflict with other configurations */
mesa_rc rmirror_mgmt_is_valid_vid(mesa_vid_t vid)
{
    mesa_rc             rc = VTSS_RC_OK;
#if 0 /* ahu: update later... */
    ip_conf_t           ip_conf;
#if defined(VTSS_SW_OPTION_MVR)
    mesa_vid_t          mvr_vid;
#endif /* VTSS_SW_OPTION_MVR */
#if defined(VTSS_SW_OPTION_VOICE_VLAN)
    voice_vlan_conf_t   conf;
#endif

    /* check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return RMIRROR_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

#if defined(VTSS_SW_OPTION_MVR)
    /* Check conflict with MVR VID */
    if (mvr_mgmt_get_mvid(&mvr_vid) == VTSS_RC_OK) {
        if (vid == mvr_vid) {
            return RMIRROR_ERROR_VID_IS_CONFLICT_WITH_MVR_VID;
        }
    }
#endif /* VTSS_SW_OPTION_MVR */

#if defined(VTSS_SW_OPTION_VOICE_VLAN)
    /* Check conflict with Voice VLAN VID */
    if (voice_vlan_mgmt_conf_get(&conf)  == VTSS_RC_OK) {
        if (vid == conf.vid) {
            return RMIRROR_ERROR_VID_IS_CONFLICT_WITH_VOICE_VLAN_VID;
        }
    }
#endif /* VTSS_SW_OPTION_MVR */

    /* Check conflict with managed VID */
    if (ip_mgmt_conf_get(&ip_conf) == VTSS_RC_OK) {
        if (vid == ip_conf.vid) {
            return RMIRROR_ERROR_VID_IS_CONFLICT_WITH_MGMT_VID;
        }
    }
#endif

    return rc;
}

/* Get Max RMIRROR Session */
mesa_rc rmirror_mgmt_max_session_get(void)
{
    return VTSS_MIRROR_SESSION_MAX_ID;
}

/* Get Min RMIRROR Session */
mesa_rc rmirror_mgmt_min_session_get(void)
{
    return VTSS_MIRROR_SESSION_MIN_ID;
}

/* Get Total RMIRROR Session */
mesa_rc rmirror_mgmt_max_cnt_get(void)
{
    return VTSS_MIRROR_SESSION_MAX_CNT;
}

/* Get RMIRROR VLAN ID by Session */
mesa_rc rmirror_mgmt_rmirror_vid_get(ulong session)
{
    ulong                   session_idx;
    rmirror_stack_conf_t    *rmirror_stack_conf;
    mesa_vid_t              vid;

    /* check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return RMIRROR_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* check session range */
    if (session > VTSS_MIRROR_SESSION_MAX_ID || session < VTSS_MIRROR_SESSION_MIN_ID) {
        T_D("exit");
        return RMIRROR_ERROR_INV_PARAM;
    }

    session_idx = (session - VTSS_MIRROR_SESSION_MIN_ID);

    RMIRROR_CRIT_ENTER();
    rmirror_stack_conf = &mem_rmirror_gconf.stack_conf[session_idx];
    vid = rmirror_stack_conf->vid;
    RMIRROR_CRIT_EXIT();

    return vid;
}

/* Get RMIRROR Configuration */
mesa_rc rmirror_mgmt_conf_get(rmirror_switch_conf_t *conf)
{
    ulong                   session_idx;
    rmirror_stack_conf_t    *rmirror_stack_conf;

    T_D("enter");

    /* check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return RMIRROR_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* check session range */
    if (conf->session_num > VTSS_MIRROR_SESSION_MAX_ID || conf->session_num < VTSS_MIRROR_SESSION_MIN_ID) {
        T_D("exit");
        return RMIRROR_ERROR_INV_PARAM;
    }

    session_idx = (conf->session_num - VTSS_MIRROR_SESSION_MIN_ID);

    RMIRROR_CRIT_ENTER();
    rmirror_stack_conf = &mem_rmirror_gconf.stack_conf[session_idx];

    conf->vid = rmirror_stack_conf->vid;
    conf->type = rmirror_stack_conf->type;
    conf->rmirror_switch_id = rmirror_stack_conf->rmirror_switch_id;
    conf->reflector_port = rmirror_stack_conf->reflector_port;
    conf->source_vid = rmirror_stack_conf->source_vid;
    conf->enabled = rmirror_stack_conf->enabled;
    RMIRROR_CRIT_EXIT();

    T_D("type = %s", conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE ? "source" :
        conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_DESTINATION ? "destination" :
        conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR ? "mirror" :
        "unKnown");

    T_D("exit");
    return VTSS_RC_OK;
}

/* Get Next RMIRROR Configuration */
mesa_rc rmirror_mgmt_next_conf_get(rmirror_switch_conf_t *conf)
{
    T_D("enter");

    /* check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return RMIRROR_ERROR_MUST_BE_PRIMARY_SWITCH;
    }

    conf->session_num++;

    /* check session range */
    if (conf->session_num > VTSS_MIRROR_SESSION_MAX_ID || conf->session_num < VTSS_MIRROR_SESSION_MIN_ID) {
        T_D("exit");
        return RMIRROR_ERROR_INV_PARAM;
    }

    T_D("exit");
    return rmirror_mgmt_conf_get(conf);
}

/* Get RMIRROR Switch Configuration */
mesa_rc rmirror_mgmt_switch_conf_get(vtss_isid_t isid, rmirror_switch_conf_t *conf)
{
    ulong                   session_idx;
    rmirror_stack_conf_t    *rmirror_stack_conf;
    int                     isid_idx = isid - VTSS_ISID_START;
    port_iter_t             pit;

    T_D("enter");

    /* check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return RMIRROR_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* check switch ID */
    if (!VTSS_ISID_LEGAL(isid) ) {
        T_D("exit, isid: %d not legal", isid);
        return RMIRROR_ERROR_ISID;
    }
    if (!msg_switch_configurable(isid)) {
        T_D("exit, isid: %d not exist", isid);
        return RMIRROR_ERROR_ISID;
    }
    /* check session range */
    if (conf->session_num > VTSS_MIRROR_SESSION_MAX_ID || conf->session_num < VTSS_MIRROR_SESSION_MIN_ID) {
        T_D("exit");
        return RMIRROR_ERROR_INV_PARAM;
    }

    session_idx = (conf->session_num - VTSS_MIRROR_SESSION_MIN_ID);

    RMIRROR_CRIT_ENTER();
    rmirror_stack_conf = &mem_rmirror_gconf.stack_conf[session_idx];

    /* get source port and intermediate port from local switch */
    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_ALL);
    while (port_iter_getnext(&pit)) {
        conf->source_port[pit.iport].rx_enable = rmirror_stack_conf->source_port[isid_idx][pit.iport].rx_enable;
        conf->source_port[pit.iport].tx_enable = rmirror_stack_conf->source_port[isid_idx][pit.iport].tx_enable;

        conf->intermediate_port[pit.iport] = rmirror_stack_conf->intermediate_port[isid_idx][pit.iport];
        conf->destination_port[pit.iport] = rmirror_stack_conf->destination_port[isid_idx][pit.iport];
    }

    conf->cpu_src_enable = rmirror_stack_conf->cpu_src_enable[isid_idx];
    conf->cpu_dst_enable = rmirror_stack_conf->cpu_dst_enable[isid_idx];

    RMIRROR_CRIT_EXIT();

    T_D("exit");
    return VTSS_RC_OK;
}

/* Get Next RMIRROR Switch Configuration */
mesa_rc rmirror_mgmt_next_switch_conf_get(vtss_isid_t isid, rmirror_switch_conf_t *conf)
{
    T_D("enter");

    /* check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return RMIRROR_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* check switch ID */
    if (!VTSS_ISID_LEGAL(isid) ) {
        T_D("exit, isid: %d not legal", isid);
        return RMIRROR_ERROR_ISID;
    }
    if (!msg_switch_configurable(isid)) {
        T_D("exit, isid: %d not exist", isid);
        return RMIRROR_ERROR_ISID;
    }

    conf->session_num++;

    /* check session range */
    if (conf->session_num > VTSS_MIRROR_SESSION_MAX_ID || conf->session_num < VTSS_MIRROR_SESSION_MIN_ID) {
        T_D("exit");
        return RMIRROR_ERROR_INV_PARAM;
    }

    T_D("exit");
    return rmirror_mgmt_switch_conf_get(isid, conf);
}

/* Set RMIRROR Configuration */
mesa_rc rmirror_mgmt_conf_set(rmirror_switch_conf_t *new_conf)
{
    mesa_rc                 rc = VTSS_RC_OK;
    rmirror_switch_conf_t   old_conf, *old_conf_ptr = &old_conf;
    rmirror_stack_conf_t    *rmirror_stack_conf;
    ulong                   session_idx;
    int                     is_changed = FALSE;
    vtss_isid_t             cache_switch_id = VTSS_ISID_START;

    T_D("enter");

    /* check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return RMIRROR_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* check session range */
    if (new_conf->session_num > VTSS_MIRROR_SESSION_MAX_ID || new_conf->session_num < VTSS_MIRROR_SESSION_MIN_ID) {
        T_D("exit");
        return RMIRROR_ERROR_INV_PARAM;
    }

    rmirror_mgmt_default_set(old_conf_ptr);

    session_idx = (new_conf->session_num - VTSS_MIRROR_SESSION_MIN_ID);

    RMIRROR_CRIT_ENTER();
    rmirror_stack_conf = &mem_rmirror_gconf.stack_conf[session_idx];

    old_conf_ptr->vid = rmirror_stack_conf->vid;
    old_conf_ptr->type = rmirror_stack_conf->type;
    old_conf_ptr->rmirror_switch_id = rmirror_stack_conf->rmirror_switch_id;
    old_conf_ptr->reflector_port = rmirror_stack_conf->reflector_port;
    old_conf_ptr->source_vid = rmirror_stack_conf->source_vid;
    old_conf_ptr->enabled = rmirror_stack_conf->enabled;
    RMIRROR_CRIT_EXIT();

#if 0
    if (!new_conf->enabled) {
        new_conf->rmirror_switch_id = rmirror_local_lowest_isid_find();
    }
#endif

    /* update and store new configuration if the configuration has changed. */
    T_D("type = %s", new_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE ? "source" :
        new_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_DESTINATION ? "destination" :
        new_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR ? "mirror" :
        "unKnown");
    if (rmirror_conf_global_changed(new_conf, old_conf_ptr)) {
        T_D("changed");
        /* update the old switch */
        RMIRROR_CRIT_ENTER();
        rmirror_stack_conf->enabled = FALSE;
        T_N("enabled = %d", rmirror_stack_conf->enabled );
        RMIRROR_CRIT_EXIT();

        if ( VTSS_ISID_LEGAL(old_conf_ptr->rmirror_switch_id) ) {
            T_N("[[[[[[[[[[[[update old configuration]]]]]]]]]]]], isid = %u", old_conf_ptr->rmirror_switch_id);
            rc = rmirror_conf_set(old_conf_ptr->rmirror_switch_id);
        }

        /* whether the RMIRROR is disabled or not */
        RMIRROR_CRIT_ENTER();
        if (!new_conf->enabled && new_conf->enabled !=  old_conf_ptr->enabled) {
            T_D("RMIRROR load default configuration, session " VPRIlu, session_idx);
#if 0
            /* disable all configurations if you set the mode as disable */
            rc = rmirror_local_default_set(rmirror_stack_conf, 1 );
            rmirror_stack_conf->rmirror_switch_id = rmirror_local_lowest_isid_find();
            rmirror_stack_conf->vid = RMIRROR_DEFAULT_VID;
#else
            rmirror_stack_conf->vid = new_conf->vid;
            rmirror_stack_conf->type = new_conf->type;
            rmirror_stack_conf->rmirror_switch_id = new_conf->rmirror_switch_id;
            rmirror_stack_conf->reflector_port = new_conf->reflector_port;
            rmirror_stack_conf->source_vid = new_conf->source_vid;
            rmirror_stack_conf->enabled = new_conf->enabled;
#endif
        } else {
            rmirror_stack_conf->vid = new_conf->vid;
            rmirror_stack_conf->type = new_conf->type;
            rmirror_stack_conf->rmirror_switch_id = new_conf->rmirror_switch_id;
            rmirror_stack_conf->reflector_port = new_conf->reflector_port;
            rmirror_stack_conf->source_vid = new_conf->source_vid;
            rmirror_stack_conf->enabled = new_conf->enabled;
        }

        cache_switch_id = rmirror_stack_conf->rmirror_switch_id;
        RMIRROR_CRIT_EXIT();

        is_changed = TRUE;
    }

    if (is_changed) {

        T_D("save");
        /* update the switch in question */
        //rc = rmirror_conf_set(new_conf->rmirror_switch_id);
        T_N("[[[[[[[[[[[[change new configuration]]]]]]]]]]]], isid = %u", cache_switch_id);
        rc = rmirror_conf_set(cache_switch_id);

        /* store in flash */
        rc = rmirror_configuration_save();
    }

    T_D("exit");
    return rc;
}

/* Set RMIRROR Switch Configuration */
mesa_rc rmirror_mgmt_switch_conf_set(vtss_isid_t isid, rmirror_switch_conf_t *new_conf)
{
    mesa_rc                 rc = VTSS_RC_OK;
    rmirror_switch_conf_t   old_conf, *old_conf_ptr = &old_conf;
    ulong                   session_idx;
    rmirror_stack_conf_t    *rmirror_stack_conf;
    int                     isid_idx = isid - VTSS_ISID_START;
    int                     is_changed = FALSE;
    port_iter_t             pit;


    T_D("enter");

    /* check stack role */
    if (!msg_switch_is_primary()) {
        T_W("not primary switch");
        T_D("exit");
        return RMIRROR_ERROR_MUST_BE_PRIMARY_SWITCH;
    }
    /* check switch ID */
    if (!VTSS_ISID_LEGAL(isid) ) {
        T_D("exit, isid: %d not legal", isid);
        return RMIRROR_ERROR_ISID;
    }
    if (!msg_switch_configurable(isid)) {
        T_D("exit, isid: %d not exist", isid);
        return RMIRROR_ERROR_ISID;
    }
    /* check session range */
    if (new_conf->session_num > VTSS_MIRROR_SESSION_MAX_ID || new_conf->session_num < VTSS_MIRROR_SESSION_MIN_ID) {
        T_D("exit");
        return RMIRROR_ERROR_INV_PARAM;
    }

    rmirror_mgmt_default_set(old_conf_ptr);

    session_idx = (new_conf->session_num - VTSS_MIRROR_SESSION_MIN_ID);

    RMIRROR_CRIT_ENTER();
    rmirror_stack_conf = &mem_rmirror_gconf.stack_conf[session_idx];

    /* get source port and intermediate port from global memory */
    (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        old_conf_ptr->source_port[pit.iport].rx_enable = rmirror_stack_conf->source_port[isid_idx][pit.iport].rx_enable;
        old_conf_ptr->source_port[pit.iport].tx_enable = rmirror_stack_conf->source_port[isid_idx][pit.iport].tx_enable;

        old_conf_ptr->intermediate_port[pit.iport] = rmirror_stack_conf->intermediate_port[isid_idx][pit.iport];
        old_conf_ptr->destination_port[pit.iport] = rmirror_stack_conf->destination_port[isid_idx][pit.iport];
    }

    old_conf_ptr->cpu_src_enable = rmirror_stack_conf->cpu_src_enable[isid_idx];
    old_conf_ptr->cpu_dst_enable = rmirror_stack_conf->cpu_dst_enable[isid_idx];

    RMIRROR_CRIT_EXIT();

    /* update and store new configuration if the configuration has changed. */
    if (rmirror_conf_local_changed(new_conf, old_conf_ptr)) {

        /* update the old switch */
        RMIRROR_CRIT_ENTER();
        old_conf_ptr->enabled = rmirror_stack_conf->enabled;

        rmirror_stack_conf->enabled = FALSE;
        T_N("enabled = %d", rmirror_stack_conf->enabled );
        RMIRROR_CRIT_EXIT();

#if defined(VTSS_SW_OPTION_EEE)
        if (new_conf->enabled) {
            /*
             * Detects RMIRROR and EEE conflict (Jaguar1), skip config apply if it
             * happens.
             */
            eee_switch_conf_t   eee_conf;

            T_D("RMIRROR enabled in new config");

            if ((rc = eee_mgmt_switch_conf_get(isid, &eee_conf)) != VTSS_RC_OK) {
                T_W("%s", mirror_error_txt(rc));
                goto EXIT_PTR;
            }

            (void) port_iter_init(&pit, NULL, isid,
                                  PORT_ITER_SORT_ORDER_IPORT,
                                  PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (eee_conf.port[pit.iport].eee_ena) {
                    T_I("EEE conflicts with RMIRROR");
                    rc = RMIRROR_ERROR_CONFLICT_WITH_EEE;
                    goto EXIT_PTR;
                }
            }
        }
#endif

        if ( VTSS_ISID_LEGAL(isid) ) {
            T_N("[[[[[[[[[[[[update old switch configuration]]]]]]]]]]]], isid = %u", isid);
            rc = rmirror_conf_set(isid);
        }

        // restore enabled flag
        RMIRROR_CRIT_ENTER();
        rmirror_stack_conf->enabled = old_conf_ptr->enabled;

        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            rmirror_stack_conf->source_port[isid_idx][pit.iport].rx_enable = new_conf->source_port[pit.iport].rx_enable;
            rmirror_stack_conf->source_port[isid_idx][pit.iport].tx_enable = new_conf->source_port[pit.iport].tx_enable;

            rmirror_stack_conf->intermediate_port[isid_idx][pit.iport] = new_conf->intermediate_port[pit.iport];
            rmirror_stack_conf->destination_port[isid_idx][pit.iport] = new_conf->destination_port[pit.iport];
        }

        rmirror_stack_conf->cpu_src_enable[isid_idx] = new_conf->cpu_src_enable;
        rmirror_stack_conf->cpu_dst_enable[isid_idx] = new_conf->cpu_dst_enable;
        RMIRROR_CRIT_EXIT();

        is_changed = TRUE;
    }


    if (is_changed) {

        /* update the switch in question */
        T_N("[[[[[[[[[[[[change switch configuration]]]]]]]]]]]], isid = %u", isid);
        rc = rmirror_conf_set(isid);

        /* store in flash */
        rc = rmirror_configuration_save();
    }

#if defined(VTSS_SW_OPTION_EEE)
EXIT_PTR:
#endif

    T_D("exit, rc =%d", rc);
    return rc;
}

/* Get RMIRROR is enabled or not */
BOOL rmirror_mgmt_is_rmirror_enabled(void)
{
    mesa_rc                 rc = FALSE;
    rmirror_stack_conf_t    *rmirror_stack_conf;
    int                     idx;

    T_D("enter");

    RMIRROR_CRIT_ENTER();
    for (idx = 0; idx < VTSS_MIRROR_SESSION_MAX_CNT; idx++) {

        rmirror_stack_conf = &mem_rmirror_gconf.stack_conf[idx];

        if (rmirror_stack_conf->enabled && rmirror_stack_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE) {
            rc = TRUE;
            break;
        }
    }
    RMIRROR_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/* Check whether the port is a candidate reflector port or not */
BOOL rmirror_mgmt_is_valid_reflector_port(vtss_isid_t isid, mesa_port_no_t port_no)
{
    meba_port_cap_t         cap = 0;
    vtss_appl_port_conf_t   conf;
    vtss_appl_port_status_t port_status;

    T_D("enter, isid = %d, port = %d", isid, port_no);

    // check running status
    if (_rmirror_global_status == 0) {
        return TRUE;
    }

    vtss_ifindex_t ifindex;
    if (vtss_ifindex_from_port(isid, port_no, &ifindex) != VTSS_RC_OK) {
        T_E("Could not get ifindex");
        return FALSE;
    };

    // check whether the port is disabled or not
    if (port_no != VTSS_PORT_NO_NONE && vtss_appl_port_conf_get(ifindex, &conf) == VTSS_RC_OK) {
        if (!conf.admin.enable) {
            T_D("ERROR! - port is disabled, isid = %d, port = %d", isid, port_no);
            return FALSE;
        }
    }

    // get port capability
    if (vtss_appl_port_status_get(ifindex, &port_status) == VTSS_RC_OK) {
        cap = port_status.static_caps;
    }

    // if the port is combo port, return false.
    if (cap & (MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER | MEBA_PORT_CAP_DUAL_SFP_DETECT)) {
        T_D("ERROR! - port type is not correct. cap = " VPRI64x ", isid = %d, port = %d", cap, isid, port_no);
        return FALSE;
    }

    // if the port is 10G COPPER i.e. AQUANTIA 10G. This is currently not supported
    if ((cap & MEBA_PORT_CAP_COPPER) && (cap & MEBA_PORT_CAP_10G_FDX)) {
        T_D("ERROR! - port type is 10G Copper. This is currently not supported. cap = " VPRI64x " , isid = %d, port = %d", cap, isid, port_no);
        return FALSE;
    }

    // we only support reflector port on copper ports.
    if (cap & MEBA_PORT_CAP_COPPER) {
        return TRUE;
    }

    T_D("INFO - exit, return FALSE. cap = " VPRI64x ", isid = %d, port = %d", cap, isid, port_no);
    return FALSE;
}


/**********************************************************************
 ** Initialization functions
 **********************************************************************/

/* Function called when booting */
static int rmirror_init_start(void)
{
    /* Register for stack messages */
    msg_rx_filter_t filter;

    T_D("enter");

    /* Initialize message buffers */
    vtss_sem_init(&mem_rmirror_gconf.request.sem, 1);
    vtss_sem_init(&mem_rmirror_gconf.reply.sem, 1);

    memset(&filter, 0, sizeof(filter));
    filter.cb = rmirror_msg_rx;
    filter.modid = VTSS_MODULE_ID_RMIRROR;

    /* Prepare callback function for port disable */
    (void) port_shutdown_register(VTSS_MODULE_ID_RMIRROR, rmirror_port_shutdown);

    T_D("exit");
    return msg_rx_filter_register(&filter);
}

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Initialize private mib */
VTSS_PRE_DECLS void mirror_mib_init(void);
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_mirror_json_init(void);
#endif

extern "C" int mirror_icli_cmd_register();

/* Initialize module */
mesa_rc mirror_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid; // Get switch id
    mesa_rc     rc   = VTSS_RC_OK;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        critd_init(&mem_rmirror_gconf.crit, "mem_rmirror_gconf", VTSS_MODULE_ID_RMIRROR, CRITD_TYPE_MUTEX);

        _rmirror_global_status = 0;
#ifdef VTSS_SW_OPTION_ICFG
        if ((rc = vtss_rmirror_icfg_init()) != VTSS_RC_OK) {
            T_D("Calling vtss_rmirror_icfg_init() failed rc = %s", error_txt(rc));
        }
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        /* Register private mib */
        mirror_mib_init();
#endif
#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_mirror_json_init();
#endif
        mirror_icli_cmd_register();
        break;

    case INIT_CMD_START:
        rc = rmirror_init_start();
        break;

    case INIT_CMD_CONF_DEF:
        if (isid == VTSS_ISID_LOCAL) {
            /* Reset local configuration */
            T_D("enter, isid: %u", isid );
        } else if (isid == VTSS_ISID_GLOBAL || VTSS_ISID_LEGAL(isid)) {
            /* Reset configuration (specific switch or all switches) */
            T_D("enter, isid: %u", isid );
            rmirror_conf_read(TRUE, isid);
        }
        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        rmirror_conf_read(FALSE, VTSS_ISID_GLOBAL);
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        _rmirror_global_status = 1;
        rc = rmirror_conf_set(isid);
        break;

    default:
        break;
    }

    return rc;
}

/*
==============================================================================

    Public APIs in vtss_appl\include\vtss\appl\rmirror.h

==============================================================================
*/
#define _INVALID_SESSION_ID(i) \
    ((i < VTSS_MIRROR_SESSION_MIN_ID) || (i > VTSS_MIRROR_SESSION_MAX_ID))

static mesa_rc MIRROR_switch_conf_get(
    u32                     session_id,
    vtss_isid_t             isid,
    rmirror_switch_conf_t   *conf
)
{
    rmirror_mgmt_default_set(conf);

    conf->session_num = session_id;

    return rmirror_mgmt_switch_conf_get(isid, conf);
}

#define CHECK_MIRROR_PARAM(CONF_IDX, CONF) \
    do {\
        if (!(CONF)) { \
            T_W("%s is NULL", #CONF); \
            return VTSS_RC_ERROR; \
        }\
        if (((CONF_IDX) < VTSS_MIRROR_SESSION_MIN_ID) || ((CONF_IDX) > VTSS_MIRROR_SESSION_MAX_ID)) { \
            return VTSS_APPL_MIRROR_ERROR_INV_PARAM; \
        }\
    } while(0)

#define PORT_TO_PORTLIST(VAR, ISID, IPORT, PORTLIST) \
    do { \
        if ((VAR)) { \
            (PORTLIST).set((ISID), (IPORT)); \
        } else { \
            (PORTLIST).clear((ISID), (IPORT)); \
        } \
    } while(0)

#define PORTLIST_TO_PORT(PORTLIST, ISID, IPORT, VAR) \
    do { \
        if ((PORTLIST).get((ISID), (IPORT))) { \
            VAR = TRUE; \
        } else { \
            VAR = FALSE; \
        } \
    } while(0)

static BOOL MIRROR_session_entry_getnext(const u16 *const prev_session_id, u16 *const next_session_id)
{
    if (prev_session_id) {
        if (*prev_session_id < VTSS_MIRROR_SESSION_MIN_ID) {
            *next_session_id = VTSS_MIRROR_SESSION_MIN_ID;
        } else if (*prev_session_id < VTSS_MIRROR_SESSION_MAX_ID) {
            *next_session_id = *prev_session_id + 1;
        } else {
            return FALSE;
        }
    } else {
        *next_session_id = VTSS_MIRROR_SESSION_MIN_ID;
    }

    return TRUE;
}

static BOOL MIRROR_session_port_list_get(u16 session_id,
                                         vtss_port_list_stackable_t *source_port_list_rx,
                                         vtss_port_list_stackable_t *source_port_list_tx,
                                         vtss_port_list_stackable_t *destination_port_list,
                                         BOOL *cpu_rx, BOOL *cpu_tx)
{
    rmirror_switch_conf_t   conf;
    switch_iter_t           sit;
    port_iter_t             pit;
    mesa_rc                 rc = VTSS_RC_OK;
    vtss::PortListStackable         &source_pls_rx = (vtss::PortListStackable &)(*source_port_list_rx),
                                     &source_pls_tx = (vtss::PortListStackable &)(*source_port_list_tx),
                                      &destination_pls = (vtss::PortListStackable &)(*destination_port_list);

    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        if ((rc = MIRROR_switch_conf_get(session_id, sit.isid, &conf)) != VTSS_RC_OK) {
            T_W("Failed: rmirror_mgmt_switch_conf_get(%d): %s", sit.isid, error_txt(rc));
            return FALSE;
        }

        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            PORT_TO_PORTLIST(conf.source_port[pit.iport].rx_enable, sit.isid, pit.iport, source_pls_rx);
            PORT_TO_PORTLIST(conf.source_port[pit.iport].tx_enable, sit.isid, pit.iport, source_pls_tx);
            PORT_TO_PORTLIST(conf.destination_port[pit.iport], sit.isid, pit.iport, destination_pls);
        }
    }

    if (!cpu_rx || !cpu_tx ) {
        T_W("cpu_rx or cpu_tx is not allocated");
        return FALSE;
    }
    *cpu_rx = conf.cpu_src_enable;
    *cpu_tx = conf.cpu_dst_enable;

    return TRUE;
}

static mesa_rc MIRROR_session_port_list_set(u16 session_id, vtss_appl_mirror_session_type_t type,
                                            BOOL enable,
                                            vtss_isid_t rmirror_switch_id,
                                            mesa_port_no_t reflector_port,
                                            const vtss_port_list_stackable_t *source_port_list_rx,
                                            const vtss_port_list_stackable_t *source_port_list_tx,
                                            const vtss_port_list_stackable_t *destination_port_list,
                                            const BOOL *cpu_rx, const BOOL *cpu_tx)
{
    rmirror_switch_conf_t           conf;
    switch_iter_t                   sit;
    port_iter_t                     pit;
    mesa_rc                         rc = VTSS_RC_OK;
    u32                             i;
    vtss_port_list_stackable_t      reflector_port_list;
    vtss::PortListStackable         &source_pls_rx = (vtss::PortListStackable &)(*source_port_list_rx),
                                     &source_pls_tx = (vtss::PortListStackable &)(*source_port_list_tx),
                                      &destination_pls = (vtss::PortListStackable &)(*destination_port_list),
                                       &reflector_pls = (vtss::PortListStackable &)(reflector_port_list);
    vtss_appl_mirror_session_entry_t config;
    u16                             *prev_idx_ptr = NULL, next_idx;

    T_D("enter");
    reflector_pls.clear_all();

    PORT_TO_PORTLIST(TRUE, rmirror_switch_id, reflector_port, reflector_pls);
    if (enable) {
        while (VTSS_RC_OK == vtss_appl_mirror_session_entry_itr(prev_idx_ptr, &next_idx)) {
            if (next_idx == session_id) {
                goto next_loop;
            }
            if (VTSS_RC_OK != vtss_appl_mirror_session_entry_get(next_idx, &config)) {
                return VTSS_RC_ERROR;
            }

            if (!config.enable) {
                goto next_loop;
            }

            for (i = 0; i < sizeof(source_port_list_rx->data) / sizeof(source_port_list_rx->data[0]); i++) {
                if ((source_port_list_rx->data[i] | source_port_list_tx->data[i]) &
                    config.destination_port_list.data[i]) {

                    return VTSS_APPL_MIRROR_ERROR_SOURCE_PORTS_ARE_USED_BY_OTHER_SESSIONS;
                }

                if ((config.source_port_list_rx.data[i] | config.source_port_list_tx.data[i]) &
                    destination_port_list->data[i]) {
                    return VTSS_APPL_MIRROR_ERROR_DESTINATION_PORTS_ARE_USED_BY_OTHER_SESSIONS;
                }

                if (type == VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE &&
                    (config.source_port_list_rx.data[i] | config.source_port_list_tx.data[i] | config.destination_port_list.data[i]) &
                    reflector_port_list.data[i]) {
                    return VTSS_APPL_MIRROR_ERROR_REFLECTOR_PORT_IS_USED_BY_OTHER_SESSIONS;
                }
            }

next_loop:
            prev_idx_ptr = &next_idx;
        }
    }


    (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        if ((rc = MIRROR_switch_conf_get(session_id, sit.isid, &conf)) != VTSS_RC_OK) {
            T_W("Failed: rmirror_mgmt_switch_conf_get(%d): %s", sit.isid, error_txt(rc));
            return VTSS_RC_ERROR;
        }

        /* Mirror session only support one destination port, so the old destination port must be clear first */
        if (type == VTSS_APPL_MIRROR_SESSION_TYPE_MIRROR) {
            vtss_clear(conf.destination_port);
        }

        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            PORTLIST_TO_PORT(source_pls_rx, sit.isid, pit.iport, conf.source_port[pit.iport].rx_enable);
            PORTLIST_TO_PORT(source_pls_tx, sit.isid, pit.iport, conf.source_port[pit.iport].tx_enable);
            PORTLIST_TO_PORT(destination_pls, sit.isid, pit.iport, conf.destination_port[pit.iport]);
        }

        if (!cpu_rx || !cpu_tx ) {
            T_W("cpu_rx or cpu_tx is not allocated");
            return VTSS_RC_ERROR;
        }

        conf.cpu_src_enable = *cpu_rx;
        conf.cpu_dst_enable = *cpu_tx;
        if ((rc = rmirror_mgmt_switch_conf_set(sit.isid, &conf)) != VTSS_RC_OK) {
            T_W("Failed: rmirror_mgmt_switch_conf_set(%d): %s", sit.isid, error_txt(rc));
            return VTSS_RC_ERROR;
        }
    }

    T_D("exit");
    return VTSS_RC_OK;
}

static mesa_rc MIRROR_session_port_list_check(vtss_appl_mirror_session_type_t type,
                                              vtss_isid_t rmirror_switch_id,
                                              mesa_port_no_t reflector_port,
                                              const vtss_port_list_stackable_t *const source_port_list_rx,
                                              const vtss_port_list_stackable_t *const source_port_list_tx,
                                              const vtss_port_list_stackable_t *const destination_port_list
                                             )
{
    switch_iter_t               sit;
    port_iter_t                 pit;
    u32                         i;
    vtss_port_list_stackable_t  pls_mask;
    vtss_port_list_stackable_t  reflector_port_list;
    vtss::PortListStackable     &destination_pls = (vtss::PortListStackable &)(*destination_port_list),
                                 &mask = (vtss::PortListStackable &)(pls_mask),
                                  &reflector_pls = (vtss::PortListStackable &)(reflector_port_list);
    BOOL                        destination_flag = FALSE;

    T_D("enter");
    memset(&pls_mask, 0, sizeof(pls_mask));
    memset(&reflector_port_list, 0, sizeof(reflector_port_list));
    PORT_TO_PORTLIST(TRUE, rmirror_switch_id, reflector_port, reflector_pls);
    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_ISID_CFG);
    while (switch_iter_getnext(&sit)) {
        (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            mask.set(sit.isid, pit.iport);

            if (type == VTSS_APPL_MIRROR_SESSION_TYPE_MIRROR) {
                if (destination_pls.get(sit.isid, pit.iport) && destination_flag) {
                    T_D("destination %d/%d exceeding", sit.isid, pit.iport);
                    return VTSS_APPL_MIRROR_ERROR_DESTINATION_PORTS_EXCEED_THE_LIMIT;
                } else if (destination_pls.get(sit.isid, pit.iport)) {
                    T_D("destination %d/%d", sit.isid, pit.iport);
                    destination_flag = TRUE;
                }
            }
        }
    }

    /* Since the stacking topology isn't supported anymore, the loop can be ended if pls_mask.data[i] == 0 */
    for (i = 0; i < sizeof(pls_mask.data) / sizeof(pls_mask.data[0]) && pls_mask.data[i] != 0; i++) {
        T_D("i                      = %d",      i);
        T_D("mask                   = 0x%02x",  pls_mask.data[i]);
        T_D("reflector_port_list    = 0x%02x",  reflector_port_list.data[i]);
        T_D("source_port_list_rx    = 0x%02x",  source_port_list_rx->data[i]);
        T_D("source_port_list_tx    = 0x%02x",  source_port_list_tx->data[i]);
        T_D("destination_port_list  = 0x%02x",  destination_port_list->data[i]);
        if (((pls_mask.data[i] & source_port_list_rx->data[i]) != source_port_list_rx->data[i]) ||
            ((pls_mask.data[i] & source_port_list_tx->data[i]) != source_port_list_tx->data[i]) ||
            ((pls_mask.data[i] & destination_port_list->data[i]) != destination_port_list->data[i])) {
            T_D("VTSS_APPL_MIRROR_ERROR_INV_PARAM");
            return VTSS_APPL_MIRROR_ERROR_INV_PARAM;
        }
        if (type == VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE &&
            reflector_port_list.data[i] &
            (source_port_list_rx->data[i] | source_port_list_tx->data[i] | destination_port_list->data[i])
           ) {
            return VTSS_APPL_MIRROR_ERROR_REFLECTOR_PORT_IS_INCLUDED_IN_SRC_OR_DEST_PORTS;
        }

    }

    T_D("exit");
    return VTSS_RC_OK;
}

static mesa_rc MIRROR_session_check(const rmirror_switch_conf_t  *const config)
{
    u16                     idx;
    rmirror_stack_conf_t    *rmirror_stack_conf;
    u32                     source_session_cnt = 0;
    mesa_rc                 rc = VTSS_RC_OK;
    T_D("enter");
    RMIRROR_CRIT_ENTER();
    for (idx = 0, rmirror_stack_conf = &mem_rmirror_gconf.stack_conf[0]; idx < VTSS_MIRROR_SESSION_MAX_ID; idx++, rmirror_stack_conf++) {
        if ((config->session_num - VTSS_MIRROR_SESSION_MIN_ID == idx) && config->enabled) {
            T_D("skip idx = %d, %s, %d", idx, config->enabled ? "Enabled" : "Disabled", source_session_cnt);
            if (source_session_cnt > VTSS_MIRROR_SESSION_SOURCE_CNT - 1) {
                rc = VTSS_APPL_MIRROR_ERROR_SOURCE_SESSION_EXCEED_THE_LIMIT;
                goto unlock_return;
            }
            source_session_cnt++;
            continue;
        }

        T_D("idx = %d, type = %s, %s, %d", idx, rmirror_stack_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR ? "mirror" :
            rmirror_stack_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE ? "source" :
            rmirror_stack_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_DESTINATION ? "destination" : "unKnown",
            rmirror_stack_conf->enabled ? "Enabled" : "Disabled", source_session_cnt);

        /* VTSS_APPL_MIRROR_SESSION_TYPE_MIRROR or VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE:
             - No other mirror or rmirror source sessions may be enabled.
        */
        if (config->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR || config->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE) {
            if (((rmirror_stack_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR) ||
                 (rmirror_stack_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE)) && config->enabled && rmirror_stack_conf->enabled) {
                if (source_session_cnt > VTSS_MIRROR_SESSION_SOURCE_CNT - 1) {
                    rc = VTSS_APPL_MIRROR_ERROR_SOURCE_SESSION_EXCEED_THE_LIMIT;
                    goto unlock_return;
                } else {
                    source_session_cnt++;
                }
            }
        }
        if (config->session_num - VTSS_MIRROR_SESSION_MIN_ID != idx) {
            /* VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE or VTSS_APPL_RMIRROR_SESSION_TYPE_RMIRROR_DESTINATION:
                 - No other RMirror source or destination sessions may use this VLAN ID.
            */
            if ((config->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_DESTINATION || config->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE)) {
                if ((rmirror_stack_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_DESTINATION || rmirror_stack_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE) &&
                    config->vid == rmirror_stack_conf->vid) {
                    rc = VTSS_APPL_MIRROR_ERROR_VID_IS_CONFLICT;
                    goto unlock_return;
                }
            }
            /* VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE or VTSS_APPL_RMIRROR_SESSION_TYPE_RMIRROR_DESTINATION:
                 - No other RMirror source sessions may use this Reflector port.
            */
            if ((config->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE)) {
                if ((rmirror_stack_conf->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE) &&
                    config->reflector_port == rmirror_stack_conf->reflector_port) {
                    rc = VTSS_APPL_MIRROR_ERROR_REFLECTOR_PORT_IS_USED_BY_OTHER_SESSIONS;
                    goto unlock_return;
                }
            }
        }
    }

unlock_return:
    RMIRROR_CRIT_EXIT();

    T_D("exit");
    return rc;
}

/**
 * \brief Get mirror module capabilities.
 *
 * The returned structure contains info about the level of
 * mirror/rmirror support of this device.
 *
 * \param cap [OUT] Capabilities of this module.
 *
 * \return VTSS_RC_OK if the operation succeeds.
 */
mesa_rc vtss_appl_mirror_capabilities_get(vtss_appl_mirror_capabilities_t *const cap)
{
    if (cap == NULL) {
        T_W("cap == NULL\n");
        return VTSS_RC_ERROR;
    }

    cap->session_cnt_max = VTSS_MIRROR_SESSION_MAX_CNT;
    cap->session_source_cnt_max = VTSS_MIRROR_SESSION_SOURCE_CNT;
#if defined(VTSS_SW_OPTION_RMIRROR)
    cap->rmirror_support = TRUE;
#else
    cap->rmirror_support = FALSE;
#endif /* VTSS_SW_OPTION_RMIRROR */

#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    cap->internal_reflector_port_support = TRUE;
#else
    cap->internal_reflector_port_support = FALSE;
#endif /* VTSS_SW_OPTION_MIRROR_LOOP_PORT */

    cap->cpu_mirror_support = TRUE;

    return VTSS_RC_OK;
}

/**
 * \brief Iterate function of session configuration table
 *
 * \param prev_session_id [IN]  ID of previous session.
 * \param next_session_id [OUT] ID of next session.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_mirror_session_entry_itr(const u16 *const prev_session_id, u16 *const next_session_id)
{
    // check parameter
    if (next_session_id == NULL) {
        T_W("next_session_id == NULL\n");
        return VTSS_RC_ERROR;
    }

    if (FALSE == MIRROR_session_entry_getnext(prev_session_id, next_session_id)) {
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get configuration of a particular session.
 *
 * \param session_id [IN]  Session ID.
 * \param config     [OUT] Session configuration.
 *
 * \return VTSS_RC_OK if the operation succeeds.
 */
mesa_rc vtss_appl_mirror_session_entry_get(u16 session_id, vtss_appl_mirror_session_entry_t *const config)
{
    mesa_rc                     rc;
    rmirror_switch_conf_t       conf;
    mesa_vid_t                  vid;
    vtss::VlanList              &vls = (vtss::VlanList &)(config->source_vids);
    BOOL                        *cpu_tx = NULL, *cpu_rx = NULL;

    CHECK_MIRROR_PARAM(session_id, config);

    T_D("enter");
    conf.session_num = session_id;

    if ((rc = rmirror_mgmt_conf_get(&conf)) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_conf_get(): %s\n", error_txt(rc));
        return rc;
    }

    memset(config, 0, sizeof(*config));
    config->enable = conf.enabled;

    switch (conf.type) {
    case VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR:
        config->type = VTSS_APPL_MIRROR_SESSION_TYPE_MIRROR;
        break;
    case VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE:
        config->type = VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE;
        break;
    case VTSS_APPL_RMIRROR_SWITCH_TYPE_DESTINATION:
        config->type = VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_DESTINATION;
        break;
    default:
        T_W("Failed: unknown type(%d)", conf.type);
        return VTSS_RC_ERROR;
    }


    T_D("type is %s", config->type == VTSS_APPL_MIRROR_SESSION_TYPE_MIRROR ? "mirror" :
        config->type == VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE ? "source" : "destination");
    config->rmirror_vid     = conf.vid;

#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    config->strip_inner_tag = conf.reflector_port ? FALSE : TRUE;
#else
    if (vtss_ifindex_from_port(conf.rmirror_switch_id, conf.reflector_port, &config->reflector_port) != VTSS_RC_OK) {
        T_W("Failed: invalid reflector port(%d, %d)", conf.rmirror_switch_id, conf.reflector_port);
        return VTSS_RC_ERROR;
    }
#endif /*   VTSS_SW_OPTION_MIRROR_LOOP_PORT */

    vls.clear(0);
    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; ++vid) {
        if (conf.source_vid[vid]) {
            vls.set(vid);
        } else {
            vls.clear(vid);
        }
    }

    cpu_tx = &config->cpu_tx;
    cpu_rx = &config->cpu_rx;
    T_D("cpu_rx is %s, cpu_tx is %s", *cpu_rx ? "TRUE" : "FALSE",
        *cpu_tx ? "TRUE" : "FALSE");

    if (MIRROR_session_port_list_get(
            session_id,
            &config->source_port_list_rx,
            &config->source_port_list_tx,
            &config->destination_port_list, cpu_rx, cpu_tx) == FALSE) {
        return VTSS_RC_ERROR;
    }

    T_D("exit");
    return rc;
}

/**
 * \brief Get configuration of a particular session.
 *
 * \param session_id [IN]  Session ID.
 * \param config     [OUT] Session configuration.
 *
 * \return VTSS_RC_OK if the operation succeeds.
 */
mesa_rc vtss_appl_mirror_session_entry_set(u16 session_id, const vtss_appl_mirror_session_entry_t  *const config)
{
    rmirror_switch_conf_t       conf;
    vtss_ifindex_elm_t          ife;
    mesa_rc                     rc;
    mesa_vid_t                  vid;
    vtss::VlanList              &vls = (vtss::VlanList &)(config->source_vids);
    const BOOL                  *cpu_tx = NULL, *cpu_rx = NULL;

    CHECK_MIRROR_PARAM(session_id, config);

    T_D("enter session %d", session_id);
    if (config->type > VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_DESTINATION) {
        T_D("VTSS_APPL_MIRROR_ERROR_INV_PARAM");
        return VTSS_APPL_MIRROR_ERROR_INV_PARAM;
    }

    if (config->rmirror_vid < VTSS_APPL_VLAN_ID_MIN || config->rmirror_vid > VTSS_APPL_VLAN_ID_MAX) {
        T_D("VTSS_APPL_MIRROR_ERROR_INV_PARAM");
        return VTSS_APPL_MIRROR_ERROR_INV_PARAM;
    }

    vtss_clear(conf);
    conf.session_num = session_id;

    if ((rc = rmirror_mgmt_conf_get(&conf)) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_conf_get: %s\n", error_txt(rc));
        return rc;
    }

    T_D("reflector_port = %d", VTSS_IFINDEX_PRINTF_ARG(config->reflector_port));
#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    conf.reflector_port = config->strip_inner_tag ? 0 : 1;
#else
    if (vtss_ifindex_decompose(config->reflector_port, &ife) != VTSS_RC_OK) {
        T_W("Failed: invalid reflector port(%du)", config->reflector_port.private_ifindex_data_do_not_use_directly);
        return VTSS_APPL_MIRROR_ERROR_INV_PARAM;
    }

    if (config->type == VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE && rmirror_mgmt_is_valid_reflector_port(ife.isid, ife.ordinal) == FALSE) {
        return VTSS_APPL_MIRROR_ERROR_REFLECTOR_PORT_IS_INVALID;
    }
    conf.reflector_port =  ife.ordinal;
    conf.rmirror_switch_id = ife.isid;
#endif /*   VTSS_SW_OPTION_MIRROR_LOOP_PORT */

    conf.enabled = config->enable;

    T_D("session %d type is %s", session_id, config->type == VTSS_APPL_MIRROR_SESSION_TYPE_MIRROR ? "mirror" :
        config->type == VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE ? "source" : "destination");
    switch (config->type) {
    case VTSS_APPL_MIRROR_SESSION_TYPE_MIRROR:
        conf.type =  VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR;
        break;
    case VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE:
        conf.type =  VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE;
        break;
    case VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_DESTINATION:
        conf.type =  VTSS_APPL_RMIRROR_SWITCH_TYPE_DESTINATION;
        break;
    default:
        T_W("unKnown type");
        return VTSS_RC_ERROR;
    }

    T_D("rmirror_vid= %d", config->rmirror_vid);
    conf.vid = config->rmirror_vid;

    /* The port list must not include non-configurable switch */
    if (VTSS_RC_OK != (rc = MIRROR_session_port_list_check(config->type,
                                                           conf.rmirror_switch_id,
                                                           conf.reflector_port,
                                                           &config->source_port_list_rx,
                                                           &config->source_port_list_tx,
                                                           &config->destination_port_list
                                                          ))) {
        return rc;
    }

    vtss_clear(conf.source_vid);

    if (vls.get(0)) {
        T_D("VTSS_APPL_MIRROR_ERROR_INV_PARAM");
        return VTSS_APPL_MIRROR_ERROR_INV_PARAM;
    }
    for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; ++vid) {
        conf.source_vid[vid] = vls.get(vid);
        if (vls.get(vid) && vid == conf.vid &&
            config->type == VTSS_APPL_MIRROR_SESSION_TYPE_RMIRROR_SOURCE &&
            TRUE == conf.enabled) {
            return VTSS_APPL_MIRROR_ERROR_SOURCE_VIDS_INCLUDE_RMIRROR_VID;
        }
    }
    /* Do following check on configured sessions */
    if ((rc = MIRROR_session_check(&conf)) != VTSS_RC_OK) {
        return rc;
    }

    cpu_rx = &config->cpu_rx;
    cpu_tx = &config->cpu_tx;
    T_D("cpu_rx is %s, cpu_tx is %s", *cpu_rx ? "TRUE" : "FALSE",
        *cpu_tx ? "TRUE" : "FALSE");

    if (VTSS_RC_OK != (rc = MIRROR_session_port_list_set(session_id, config->type,
                                                         conf.enabled,
                                                         conf.rmirror_switch_id,
                                                         conf.reflector_port,
                                                         &config->source_port_list_rx,
                                                         &config->source_port_list_tx,
                                                         &config->destination_port_list, cpu_rx, cpu_tx))) {
        return rc;
    }

    if (rmirror_mgmt_conf_set(&conf) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_conf_set()\n");
        return VTSS_RC_ERROR;
    }

    T_D("exit");
    return VTSS_RC_OK;
}

#if 0
/**
 * \brief Iterate function of session source CPU table
 *
 * \param prev_session_id [IN]  ID of previous session.
 * \param next_session_id [OUT] ID of next session.
 * \param prev_usid       [IN]  previous switch ID.
 * \param next_usid       [OUT] next switch ID.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_cpu_itr(
    const u32           *const prev_session_id,
    u32                 *const next_session_id,
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
)
{
    vtss::IteratorComposeN<u32, vtss_usid_t> itr(
        &vtss::expose::snmp::IteratorComposeStaticRange<u32, VTSS_MIRROR_SESSION_MIN_ID, VTSS_MIRROR_SESSION_MAX_ID>,
        &vtss_appl_iterator_switch);

    return itr(prev_session_id, next_session_id, prev_usid, next_usid);
}

/**
 * \brief Get source CPU configurations per session
 *
 * \param session_id [IN]  The session ID.
 * \param usid       [IN]  Switch ID
 * \param config     [OUT] The session source CPU configurations.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_cpu_get(
    u32                                                 session_id,
    vtss_usid_t                                         usid,
    vtss_appl_rmirror_config_session_source_cpu_t       *const config
)
{
    rmirror_switch_conf_t   switch_conf;
    vtss_isid_t             isid;

    if (_INVALID_SESSION_ID(session_id)) {
        return VTSS_RC_ERROR;
    }

    if (_usid_exist(usid) == FALSE) {
        return VTSS_RC_ERROR;
    }

    if (config == NULL) {
        T_W("config == NULL\n");
        return VTSS_RC_ERROR;
    }

    isid = topo_usid2isid(usid);

    if (MIRROR_switch_conf_get(session_id, topo_usid2isid(isid), &switch_conf) != VTSS_RC_OK) {
        T_W("Failed: MIRROR_switch_conf_get(%u)\n", isid);
        return VTSS_RC_ERROR;
    }

    config->mirror_type = VTSS_APPL_RMIRROR_MIRROR_TYPE_NONE;

    if (switch_conf.cpu_src_enable && switch_conf.cpu_dst_enable) {
        config->mirror_type = VTSS_APPL_RMIRROR_MIRROR_TYPE_BOTH;
    } else if (switch_conf.cpu_src_enable) {
        config->mirror_type = VTSS_APPL_RMIRROR_MIRROR_TYPE_RX;
    } else if (switch_conf.cpu_dst_enable) {
        config->mirror_type = VTSS_APPL_RMIRROR_MIRROR_TYPE_TX;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Set source CPU configurations per session
 *
 * \param session_id [IN] The session ID.
 * \param usid       [IN]  Switch ID
 * \param config     [IN] The session source CPU configurations.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_cpu_set(
    u32                                                     session_id,
    vtss_usid_t                                             usid,
    const vtss_appl_rmirror_config_session_source_cpu_t     *const config
)
{
    rmirror_switch_conf_t   switch_conf;
    vtss_isid_t             isid;

    if (_INVALID_SESSION_ID(session_id)) {
        return VTSS_RC_ERROR;
    }

    if (_usid_exist(usid) == FALSE) {
        return VTSS_RC_ERROR;
    }

    if (config == NULL) {
        T_W("config == NULL\n");
        return VTSS_RC_ERROR;
    }

    isid = topo_usid2isid(usid);

    if (MIRROR_switch_conf_get(session_id, isid, &switch_conf) != VTSS_RC_OK) {
        T_W("Failed: MIRROR_switch_conf_get(%u)\n", isid);
        return VTSS_RC_ERROR;
    }

    switch (config->mirror_type) {
    case VTSS_APPL_RMIRROR_MIRROR_TYPE_BOTH:
        switch_conf.cpu_src_enable = TRUE;
        switch_conf.cpu_dst_enable = TRUE;
        break;

    case VTSS_APPL_RMIRROR_MIRROR_TYPE_RX:
        switch_conf.cpu_src_enable = TRUE;
        switch_conf.cpu_dst_enable = FALSE;
        break;

    case VTSS_APPL_RMIRROR_MIRROR_TYPE_TX:
        switch_conf.cpu_src_enable = FALSE;
        switch_conf.cpu_dst_enable = TRUE;
        break;

    case VTSS_APPL_RMIRROR_MIRROR_TYPE_NONE:
        switch_conf.cpu_src_enable = FALSE;
        switch_conf.cpu_dst_enable = FALSE;
        break;

    default:
        return VTSS_RC_ERROR;
    }

    if (rmirror_mgmt_switch_conf_set(isid, &switch_conf) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_switch_conf_set(%u)\n", isid);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate function of session source VLAN configuration table
 *
 * \param prev_session_id [IN]  ID of previous session.
 * \param next_session_id [OUT] ID of next session.
 * \param prev_ifindex    [IN]  ifindex of previous VLAN.
 * \param next_ifindex    [OUT] ifindex of next VLAN.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_vlan_entry_itr(
    const u32               *const prev_session_id,
    u32                     *const next_session_id,
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
)
{
    vtss_ifindex_t      ifindex;

    // check parameter
    if (next_session_id == NULL) {
        T_W("next_session_id == NULL\n");
        return VTSS_RC_ERROR;
    }

    if (next_ifindex == NULL) {
        T_W("next_ifindex == NULL\n");
        return VTSS_RC_ERROR;
    }

    // get first vlan ifindex
    if (vtss_ifindex_from_vlan(1, &ifindex) != VTSS_RC_OK) {
        T_W("get first vlan ifindex\n");
        return VTSS_RC_ERROR;
    }

    if (prev_session_id) {
        if (*prev_session_id < VTSS_MIRROR_SESSION_MIN_ID) {
            // get first
            *next_session_id = VTSS_MIRROR_SESSION_MIN_ID;
            *next_ifindex = ifindex;
        } else if (*prev_session_id > VTSS_MIRROR_SESSION_MAX_ID) {
            return VTSS_RC_ERROR;
        } else if (prev_ifindex) {
            if (*prev_ifindex < ifindex) {
                // get first
                *next_session_id = *prev_session_id;
                *next_ifindex = ifindex;
            } else if (vtss_ifindex_type(*prev_ifindex + 1) == VTSS_IFINDEX_TYPE_VLAN) {
                // get next
                *next_session_id = *prev_session_id;
                *next_ifindex = *prev_ifindex + 1;
            } else if ((*prev_session_id + 1) <= VTSS_MIRROR_SESSION_MAX_ID) {
                // get next first
                *next_session_id = *prev_session_id + 1;
                *next_ifindex = ifindex;
            } else {
                return VTSS_RC_ERROR;
            }
        } else {
            // get first
            *next_session_id = *prev_session_id;
            *next_ifindex = ifindex;
        }
    } else {
        // get first
        *next_session_id = VTSS_MIRROR_SESSION_MIN_ID;
        *next_ifindex = ifindex;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Get source VLAN configuration entry per session
 *
 * \param session_id [IN]  The session ID.
 * \param ifindex    [IN]  ifindex of VLAN
 * \param entry      [OUT] source VLAN configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_vlan_entry_get(
    u32                                                     session_id,
    vtss_ifindex_t                                          ifindex,
    vtss_appl_rmirror_config_session_source_vlan_entry_t    *const entry
)
{
    vtss_ifindex_elm_t      ife;
    rmirror_switch_conf_t   conf;

    // check parameter
    if (_INVALID_SESSION_ID(session_id)) {
        return VTSS_RC_ERROR;
    }

    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        return VTSS_RC_ERROR;
    }

    if (ife.ordinal >= VTSS_VIDS) {
        return VTSS_RC_ERROR;
    }

    if (entry == NULL) {
        T_E("entry == NULL");
        return VTSS_RC_ERROR;
    }

    memset(&conf, 0x0, sizeof(conf));
    conf.session_num = session_id;

    if (rmirror_mgmt_conf_get(&conf) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_conf_get()\n");
        return VTSS_RC_ERROR;
    }

    entry->mode = conf.source_vid[ife.ordinal] ? TRUE : FALSE;

    return VTSS_RC_OK;
}

/**
 * \brief Set source VLAN configuration entry per session
 *
 * \param session_id [IN] The session ID.
 * \param ifindex    [IN] ifindex of VLAN
 * \param entry      [IN] source VLAN configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_vlan_entry_set(
    u32                                                         session_id,
    vtss_ifindex_t                                              ifindex,
    const vtss_appl_rmirror_config_session_source_vlan_entry_t  *const entry
)
{
    vtss_ifindex_elm_t      ife;
    rmirror_switch_conf_t   conf;

    // check parameter
    if (_INVALID_SESSION_ID(session_id)) {
        return VTSS_RC_ERROR;
    }

    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        return VTSS_RC_ERROR;
    }

    if (ife.ordinal >= VTSS_VIDS) {
        return VTSS_RC_ERROR;
    }

    if (entry == NULL) {
        T_E("entry == NULL");
        return VTSS_RC_ERROR;
    }

    if (entry->mode != TRUE && entry->mode != FALSE) {
        T_W("invalid entry->mode = %u\n", entry->mode);
        return VTSS_RC_ERROR;
    }

    if (entry->mode) {

        rmirror_switch_conf_t   switch_conf;
        vtss_isid_t             isid;
        mesa_port_no_t          iport;

        // check VLAN-based MIRROR enabled or not?
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (msg_switch_configurable(isid)) {
                if (MIRROR_switch_conf_get(session_id, isid, &switch_conf) != VTSS_RC_OK) {
                    T_W("Failed: MIRROR_switch_conf_get(%u)\n", isid);
                    return VTSS_RC_ERROR;
                }

                /* if port mirror is enabled, then not able to set VLAN mirror */
                for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
                    if (switch_conf.source_port[iport].rx_enable || switch_conf.source_port[iport].tx_enable) {
                        return VTSS_RC_ERROR;
                    }
                }
            }
        }
    }

    memset(&conf, 0x0, sizeof(conf));
    conf.session_num = session_id;

    if (rmirror_mgmt_conf_get(&conf) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_conf_get()\n");
        return VTSS_RC_ERROR;
    }

    conf.source_vid[ife.ordinal] = entry->mode;

    if (rmirror_mgmt_conf_set(&conf) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_conf_set()\n");
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate function of session source port configuration table
 *
 * \param prev_session_id [IN]  ID of previous session.
 * \param next_session_id [OUT] ID of next session.
 * \param prev_ifindex    [IN]  ifindex of previous port.
 * \param next_ifindex    [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_port_entry_itr(
    const u32               *const prev_session_id,
    u32                     *const next_session_id,
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
)
{
    vtss::IteratorComposeN<u32, vtss_ifindex_t> itr(
        &vtss::expose::snmp::IteratorComposeStaticRange<u32, VTSS_MIRROR_SESSION_MIN_ID, VTSS_MIRROR_SESSION_MAX_ID>,
        &vtss_appl_iterator_ifindex_front_port);

    return itr(prev_session_id, next_session_id, prev_ifindex, next_ifindex);
}

/**
 * \brief Get source port configuration entry per session
 *
 * \param session_id [IN]  The session ID.
 * \param ifindex    [IN]  ifindex of port
 * \param entry      [OUT] source port configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_port_entry_get(
    u32                                                     session_id,
    vtss_ifindex_t                                          ifindex,
    vtss_appl_rmirror_config_session_source_port_entry_t    *const entry
)
{
    vtss_ifindex_elm_t      ife;
    rmirror_switch_conf_t   switch_conf;
    mesa_port_no_t          iport;

    // check parameter
    if (_INVALID_SESSION_ID(session_id)) {
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (entry == NULL) {
        T_E("entry == NULL");
        return VTSS_RC_ERROR;
    }

    if (MIRROR_switch_conf_get(session_id, ife.isid, &switch_conf) != VTSS_RC_OK) {
        T_W("Failed: MIRROR_switch_conf_get(%u)\n", ife.isid);
        return VTSS_RC_ERROR;
    }

    iport = (mesa_port_no_t)(ife.ordinal);

    if (switch_conf.source_port[iport].rx_enable && switch_conf.source_port[iport].tx_enable) {
        entry->mirror_type = VTSS_APPL_RMIRROR_MIRROR_TYPE_BOTH;
    } else if (switch_conf.source_port[iport].rx_enable) {
        entry->mirror_type = VTSS_APPL_RMIRROR_MIRROR_TYPE_RX;
    } else if (switch_conf.source_port[iport].tx_enable) {
        entry->mirror_type = VTSS_APPL_RMIRROR_MIRROR_TYPE_TX;
    } else {
        return VTSS_RC_ERROR;
        entry->mirror_type = VTSS_APPL_RMIRROR_MIRROR_TYPE_NONE;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Set source port configuration entry per session
 *
 * \param session_id [IN] The session ID.
 * \param ifindex    [IN] ifindex of port
 * \param entry      [IN] source port configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_source_port_entry_set(
    u32                                                         session_id,
    vtss_ifindex_t                                              ifindex,
    const vtss_appl_rmirror_config_session_source_port_entry_t  *const entry
)
{
    vtss_ifindex_elm_t      ife;
    rmirror_switch_conf_t   conf;
    rmirror_switch_conf_t   switch_conf;
    mesa_port_no_t          iport;
    mesa_vid_t              vid;

    // check parameter
    if (_INVALID_SESSION_ID(session_id)) {
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (entry == NULL) {
        T_E("entry == NULL");
        return VTSS_RC_ERROR;
    }

    memset(&conf, 0x0, sizeof(conf));
    conf.session_num = session_id;

    if (rmirror_mgmt_conf_get(&conf) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_conf_get()\n");
        return VTSS_RC_ERROR;
    }

    if (MIRROR_switch_conf_get(session_id, ife.isid, &switch_conf) != VTSS_RC_OK) {
        T_W("Failed: MIRROR_switch_conf_get(%u)\n", ife.isid);
        return VTSS_RC_ERROR;
    }

    iport = (mesa_port_no_t)(ife.ordinal);

    if (entry->mirror_type != VTSS_APPL_RMIRROR_MIRROR_TYPE_NONE) {
        // check VLAN-based MIRROR enabled or not?
        for (vid = VTSS_APPL_VLAN_ID_MIN; vid <= VTSS_APPL_VLAN_ID_MAX; ++vid) {
            if (conf.source_vid[vid]) {
                return VTSS_RC_ERROR;
            }
        }

#if ! defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
        // check reflector port
        if (conf.type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE) {
            if (conf.rmirror_switch_id == ife.isid && conf.reflector_port == iport) {
                return VTSS_RC_ERROR;
            }
        }
#endif

        // check intermediate port(s)
        if (switch_conf.intermediate_port[iport]) {
            return VTSS_RC_ERROR;
        }

        // check destination port with rx
        if (entry->mirror_type != VTSS_APPL_RMIRROR_MIRROR_TYPE_RX) {
            if (switch_conf.destination_port[iport]) {
                return VTSS_RC_ERROR;
            }
        }
    }

    switch (entry->mirror_type) {
    case VTSS_APPL_RMIRROR_MIRROR_TYPE_BOTH:
        switch_conf.source_port[iport].rx_enable = TRUE;
        switch_conf.source_port[iport].tx_enable = TRUE;
        break;

    case VTSS_APPL_RMIRROR_MIRROR_TYPE_RX:
        switch_conf.source_port[iport].rx_enable = TRUE;
        switch_conf.source_port[iport].tx_enable = FALSE;
        break;

    case VTSS_APPL_RMIRROR_MIRROR_TYPE_TX:
        switch_conf.source_port[iport].rx_enable = FALSE;
        switch_conf.source_port[iport].tx_enable = TRUE;
        break;

    case VTSS_APPL_RMIRROR_MIRROR_TYPE_NONE:
        switch_conf.source_port[iport].rx_enable = FALSE;
        switch_conf.source_port[iport].tx_enable = FALSE;
        break;

    default:
        return VTSS_RC_ERROR;
    }

    if (rmirror_mgmt_switch_conf_set(ife.isid, &switch_conf) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_switch_conf_set(%u)\n", ife.isid);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Iterate function of session port configuration table
 *
 * \param prev_session_id [IN]  ID of previous session.
 * \param next_session_id [OUT] ID of next session.
 * \param prev_ifindex    [IN]  ifindex of previous port.
 * \param next_ifindex    [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_port_entry_itr(
    const u32               *const prev_session_id,
    u32                     *const next_session_id,
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
)
{
    return vtss_appl_rmirror_config_session_source_port_entry_itr(prev_session_id,
                                                                  next_session_id,
                                                                  prev_ifindex,
                                                                  next_ifindex);
}

/**
 * \brief Get port configuration entry per session
 *
 * \param session_id [IN]  The session ID.
 * \param ifindex    [IN]  ifindex of port
 * \param entry      [OUT] Port configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_port_entry_get(
    u32                                             session_id,
    vtss_ifindex_t                                  ifindex,
    vtss_appl_rmirror_config_session_port_entry_t   *const entry
)
{
    vtss_ifindex_elm_t      ife;
    rmirror_switch_conf_t   conf;
    rmirror_switch_conf_t   switch_conf;
    mesa_port_no_t          iport;

    // check parameter
    if (_INVALID_SESSION_ID(session_id)) {
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (entry == NULL) {
        T_E("entry == NULL");
        return VTSS_RC_ERROR;
    }

    memset(&conf, 0x0, sizeof(conf));
    conf.session_num = session_id;

    if (rmirror_mgmt_conf_get(&conf) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_conf_get()\n");
        return VTSS_RC_ERROR;
    }

    if (MIRROR_switch_conf_get(session_id, ife.isid, &switch_conf) != VTSS_RC_OK) {
        T_W("Failed: MIRROR_switch_conf_get(%u)\n", ife.isid);
        return VTSS_RC_ERROR;
    }

    iport = (mesa_port_no_t)(ife.ordinal);

    if (switch_conf.destination_port[iport]) {
        entry->type = VTSS_APPL_RMIRROR_PORT_TYPE_DESTINATION;
    } else if (switch_conf.intermediate_port[iport]) {
        entry->type = VTSS_APPL_RMIRROR_PORT_TYPE_INTERMEDIATE;
    } else if (conf.rmirror_switch_id == ife.isid && conf.reflector_port == iport) {
        entry->type = VTSS_APPL_RMIRROR_PORT_TYPE_REFLECTOR;
    } else {
        entry->type = VTSS_APPL_RMIRROR_PORT_TYPE_NONE;
    }

    return VTSS_RC_OK;
}

/**
 * \brief Set port configuration entry
 *
 * To write port configuration.
 *
 * \param session_id [IN] The session ID.
 * \param ifindex    [IN] ifindex of port
 * \param entry      [IN] Port configuration
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_rmirror_config_session_port_entry_set(
    u32                                                     session_id,
    vtss_ifindex_t                                          ifindex,
    const vtss_appl_rmirror_config_session_port_entry_t     *const entry
)
{
    vtss_ifindex_elm_t      ife;
    rmirror_switch_conf_t   conf;
    rmirror_switch_conf_t   switch_conf;
    mesa_port_no_t          iport;

    // check parameter
    if (_INVALID_SESSION_ID(session_id)) {
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    if (entry == NULL) {
        T_E("entry == NULL");
        return VTSS_RC_ERROR;
    }

    memset(&conf, 0x0, sizeof(conf));
    conf.session_num = session_id;

    if (rmirror_mgmt_conf_get(&conf) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_conf_get()\n");
        return VTSS_RC_ERROR;
    }

    if (MIRROR_switch_conf_get(session_id, ife.isid, &switch_conf) != VTSS_RC_OK) {
        T_W("Failed: MIRROR_switch_conf_get(%u)\n", ife.isid);
        return VTSS_RC_ERROR;
    }

    iport = (mesa_port_no_t)(ife.ordinal);

    switch (entry->type) {
    case VTSS_APPL_RMIRROR_PORT_TYPE_DESTINATION:
        // check source port(s) tx and intermediate port(s)
        if (switch_conf.source_port[iport].tx_enable) {
            return VTSS_RC_ERROR;
        }

        if (switch_conf.intermediate_port[iport]) {
            return VTSS_RC_ERROR;
        }

#if ! defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
        // check reflector port
        if (conf.type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE) {
            if (conf.rmirror_switch_id == ife.isid && conf.reflector_port == iport) {
                return VTSS_RC_ERROR;
            }
        }
#endif

        memset(switch_conf.destination_port, 0, sizeof(switch_conf.destination_port));
        switch_conf.destination_port[iport] = TRUE;
        break;

    case VTSS_APPL_RMIRROR_PORT_TYPE_INTERMEDIATE:
        if (switch_conf.source_port[iport].rx_enable || switch_conf.source_port[iport].tx_enable) {
            return VTSS_RC_ERROR;
        }

        if (switch_conf.destination_port[iport]) {
            return VTSS_RC_ERROR;
        }

#if ! defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
        // check reflector port
        if (conf.type == VTSS_APPL_RMIRROR_SWITCH_TYPE_SOURCE) {
            if (conf.rmirror_switch_id == ife.isid && conf.reflector_port == iport) {
                return VTSS_RC_ERROR;
            }
        }
#endif
        switch_conf.intermediate_port[iport] = TRUE;
        break;

    case VTSS_APPL_RMIRROR_PORT_TYPE_REFLECTOR:
#ifdef VTSS_SW_OPTION_MIRROR_LOOP_PORT
        return VTSS_RC_ERROR;
#else
        if (rmirror_mgmt_is_valid_reflector_port(ife.isid, iport) == FALSE) {
            return VTSS_RC_ERROR;
        }

        // check source port(s)
        if (switch_conf.source_port[iport].rx_enable || switch_conf.source_port[iport].tx_enable) {
            return VTSS_RC_ERROR;
        }

        // check intermediate port(s)
        if (switch_conf.intermediate_port[iport]) {
            return VTSS_RC_ERROR;
        }

        // check destination port
        if (switch_conf.destination_port[iport]) {
            return VTSS_RC_ERROR;
        }

        conf.rmirror_switch_id = ife.isid;
        conf.reflector_port = iport;

        if (rmirror_mgmt_conf_set(&conf) != VTSS_RC_OK) {
            T_W("Failed: rmirror_mgmt_conf_set()\n");
            return VTSS_RC_ERROR;
        }

        return VTSS_RC_OK;
#endif

    case VTSS_APPL_RMIRROR_PORT_TYPE_NONE:
        if (switch_conf.destination_port[iport]) {
            switch_conf.destination_port[iport] = FALSE;
        }

        if (switch_conf.intermediate_port[iport]) {
            switch_conf.intermediate_port[iport] = FALSE;
        }

        if (conf.rmirror_switch_id == ife.isid && conf.reflector_port == iport) {
            conf.rmirror_switch_id = 0;
            conf.reflector_port = 0;

            if (rmirror_mgmt_conf_set(&conf) != VTSS_RC_OK) {
                T_W("Failed: rmirror_mgmt_conf_set()\n");
                return VTSS_RC_ERROR;
            }
        }
        break;

    default:
        return VTSS_RC_ERROR;
    }

    if (rmirror_mgmt_switch_conf_set(ife.isid, &switch_conf) != VTSS_RC_OK) {
        T_W("Failed: rmirror_mgmt_switch_conf_set(%u)\n", ife.isid);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}
#endif
//#ifdef __cplusplus
//}
//#endif

