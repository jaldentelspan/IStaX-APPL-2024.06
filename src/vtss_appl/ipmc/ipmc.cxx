/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "ipmc_trace.h"
#include "ipmc_api.h"
#include "ipmc_lib_api.h"
#include "ip_api.h"       /* For vtss_ip_if_callback_add() */
#include <vtss/appl/ip.h> /* For vtss_appl_ip_if_exists()  */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_IPMC

static vtss_trace_reg_t IPMC_trace_reg = {
    VTSS_MODULE_ID_IPMC, "IPMC", "IPMC Snooping"
};

static vtss_trace_grp_t IPMC_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_TRACE_GRP_ICLI] = {
        "icli",
        "CLI printout",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_TRACE_GRP_WEB] = {
        "web",
        "Web",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [IPMC_TRACE_GRP_CALLBACK] = {
        "callback",
        "Callback",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&IPMC_trace_reg, IPMC_trace_grps);

/******************************************************************************/
// IPMC_capabilities_set()
// This is the only place where we define maximum values for various parameters,
// so if you need different max. values, change here - only!
/******************************************************************************/
static void IPMC_capabilities_set(void)
{
    vtss_appl_ipmc_capabilities_t cap;
    const bool                    is_mvr = false;

    vtss_clear(cap);

    // Maximum number of VLAN we can create corresponds to the number of VLAN
    // interfaces that can be created in the IP module.
    // If both IGMP and MLD snooping are supported, IPMC_LIB takes care of
    // multiplying this by two to get the correct maximum number of entries in
    // its VLAN map.
    cap.vlan_cnt_max = fast_cap(VTSS_APPL_CAP_IP_INTERFACE_CNT);

    ipmc_lib_capabilities_set(is_mvr, cap);
}

/******************************************************************************/
// IPMC_global_conf_default_set()
/******************************************************************************/
static void IPMC_global_conf_default_set(void)
{
    vtss_appl_ipmc_lib_key_t         k;
    vtss_appl_ipmc_lib_global_conf_t g;
    int                              i;

    vtss_clear(k);
    k.is_mvr = false;

    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vtss_clear(g);
        k.is_ipv4 = i == 0;

        // IPMC can change all these parameters from management interfaces.
        g.admin_active                 = true;
        g.unregistered_flooding_enable = true;
        g.proxy_enable                 = false;
        g.leave_proxy_enable           = false;
        g.ssm_prefix.is_ipv4           = k.is_ipv4;

        if (k.is_ipv4) {
            // 232.0.0.0/8
            g.ssm_prefix.ipv4          = 0xe8000000;
            g.ssm_prefix_len           = 8;
        } else {
            // ff3e::/96
            g.ssm_prefix.ipv6.addr[0] = 0xff;
            g.ssm_prefix.ipv6.addr[1] = 0x3e;
            g.ssm_prefix_len          = 96;
        }

        // Tell IPMC LIB about this default configuration.
        ipmc_lib_global_default_conf_set(k, g);
    }
}

/******************************************************************************/
// IPMC_port_conf_default_set()
/******************************************************************************/
static void IPMC_port_conf_default_set(void)
{
    vtss_appl_ipmc_lib_key_t       k;
    vtss_appl_ipmc_lib_port_conf_t p;
    int                            i;

    vtss_clear(k);
    k.is_mvr = false;

    // Currently, there's no difference between IGMP and MLD defaults.
    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vtss_clear(p);
        k.is_ipv4 = i == 0;

        // Parameters that IPMC can change from management interfaces:
        p.router              = false;
        p.fast_leave          = false;
        p.grp_cnt_max         = 0;
        p.profile_key.name[0] = '\0';

        // Tell IPMC LIB about this default configuration
        ipmc_lib_port_default_conf_set(k, p);
    }
}

/******************************************************************************/
// IPMC_vlan_port_conf_default_set()
/******************************************************************************/
static void IPMC_vlan_port_conf_default_set(void)
{
    vtss_appl_ipmc_lib_key_t            k;
    vtss_appl_ipmc_lib_vlan_port_conf_t c;
    int                                 i;

    vtss_clear(k);
    k.is_mvr = false;

    // Currently, there's no difference between IGMP and MLD defaults.
    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vtss_clear(c);
        k.is_ipv4 = i == 0;

        // Parameters that IPMC cannot change from management interfaces:
        c.role = VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE;

        // Tell IPMC LIB about this default configuration
        ipmc_lib_vlan_port_default_conf_set(k, c);
    }
}

/******************************************************************************/
// IPMC_vlan_conf_default_set()
/******************************************************************************/
static void IPMC_vlan_conf_default_set(void)
{
    vtss_appl_ipmc_lib_key_t       k;
    vtss_appl_ipmc_lib_vlan_conf_t c;
    int                            i;

    vtss_clear(k);
    k.is_mvr = false;

    // Currently, there's no difference between IGMP and MLD defaults.
    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vtss_clear(c);
        k.is_ipv4 = i == 0;

        // Parameters that IPMC can change from management interfaces (at least
        // if VTSS_SW_OPTION_SMB_IPMC is defined):
        c.admin_active    = false;
        c.querier_enable  = true;
        c.querier_address.is_ipv4 = k.is_ipv4;
        c.querier_address.all_zeros_set();
        c.compatibility   = VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO;
        c.pcp             =   0;
        c.rv              =   2;
        c.qi              = 125;
        c.qri             = 100;
        c.lmqi            =  10;
        c.uri             =   1;

        // Parameters that IPMC cannot change from management interfaces:
        c.compatible_mode         = false;
        c.name[0]                 = '\0';
        c.tx_tagged               = false;
        c.channel_profile.name[0] = '\0';

        // Tell IPMC LIB about this default configuration
        ipmc_lib_vlan_default_conf_set(k, c);
    }
}

/******************************************************************************/
// IPMC_ifindex_vlan_check()
/******************************************************************************/
static mesa_rc IPMC_ifindex_vlan_check(const vtss_ifindex_t ifindex, mesa_vid_t &vid)
{
    vtss_ifindex_elm_t ife;

    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK || ife.iftype != VTSS_IFINDEX_TYPE_VLAN) {
        T_D("Ifindex %s does not represent a VLAN", ifindex);
        return VTSS_APPL_IPMC_LIB_RC_IFINDEX_NOT_VLAN;
    }

    vid = ife.ordinal;

    return ipmc_lib_vlan_check(vid);
}

/******************************************************************************/
// IPMC_vlan_add_or_delete()
/******************************************************************************/
static mesa_rc IPMC_vlan_add_or_delete(mesa_vid_t vid, bool add, bool called_back)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    int                           i;
    mesa_rc                       rc;

    vlan_key.is_mvr = false;
    vlan_key.vid    = vid;

    // Call IPMC LIB to either create or remove a VLAN interface.
    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vlan_key.is_ipv4 = i == 0;

        T_I("%s: %s VLAN", vlan_key, add ? "Adding" : "Deleting");

        if (add) {
            rc = ipmc_lib_vlan_create(vlan_key, called_back);
        } else {
            rc = ipmc_lib_vlan_remove(vlan_key);
        }

        if (rc != VTSS_RC_OK) {
            T_IG(IPMC_TRACE_GRP_CALLBACK, "%s: Unable to %s VLAN ID: %s", vlan_key, add ? "add" : "delete", error_txt(rc));
        }
    }

    return rc;
}

/******************************************************************************/
// IPMC_ip_vlan_interface_callback()
/******************************************************************************/
static void IPMC_ip_vlan_interface_callback(vtss_ifindex_t ifindex)
{
    mesa_vid_t vid;
    bool       add;
    mesa_rc    rc;

    if ((rc = IPMC_ifindex_vlan_check(ifindex, vid)) != VTSS_RC_OK) {
        T_IG(IPMC_TRACE_GRP_CALLBACK, "Ifindex %s does not represent a valid VLAN: %s", ifindex, error_txt(rc));
        return;
    }

    add = vtss_appl_ip_if_exists(ifindex);
    (void)IPMC_vlan_add_or_delete(vid, add, false);
}

/******************************************************************************/
// ipmc_vlan_auto_vivify_check()
// Invoked by IPMC_LIB whenever attempting to configure a VLAN interface that it
// doesn't already know about. The reason for this is that the IP module may not
// yet have called us back when the startup-config gets parsed.
/******************************************************************************/
mesa_rc ipmc_vlan_auto_vivify_check(const vtss_appl_ipmc_lib_vlan_key_t &vlan_key)
{
    vtss_ifindex_t ifindex;
    mesa_vid_t     vid;

    if (vlan_key.is_mvr) {
        T_E("%s: Why is IPMC getting called, when it's about MVR?", vlan_key);
        return VTSS_APPL_IPMC_LIB_RC_INTERNAL_ERROR;
    }

    vid = vlan_key.vid;
    (void)vtss_ifindex_from_vlan(vid, &ifindex);
    if (!vtss_appl_ip_if_exists(ifindex)) {
        T_I("%s: VLAN interface doesn't exist", vlan_key);
        return VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN;
    }

    // Attempt to create the instance in anticipation that we get called
    // back later (see IPMC_ip_vlan_interface_callback()).
    return IPMC_vlan_add_or_delete(vid, true, true);
}

extern "C" void ipmc_mib_init(void);
extern "C" void ipmc_json_init(void);
extern "C" int  ipmc_icli_cmd_register(void);

/******************************************************************************/
// ipmc_init()
/******************************************************************************/
mesa_rc ipmc_init(vtss_init_data_t *data)
{
    mesa_rc rc;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        // Create capabilities
        IPMC_capabilities_set();

        // Create global configuration defaults once and for all.
        IPMC_global_conf_default_set();

        // Create port configuration defaults once and for all.
        IPMC_port_conf_default_set();

        // Create per-port-per-VLAN configuration defaults once and for all.
        IPMC_vlan_port_conf_default_set();

        // Create VLAN configuration defaults once and for all.
        IPMC_vlan_conf_default_set();

        // Subscribe to VLAN-inteface changes
        VTSS_RC(vtss_ip_if_callback_add(IPMC_ip_vlan_interface_callback));

#ifdef VTSS_SW_OPTION_ICFG
        mesa_rc ipmc_icfg_init(void);
        if ((rc = ipmc_icfg_init()) != VTSS_RC_OK) {
            T_E("ipmc_icfg_init() failed: %s", error_txt(rc));
        }
#endif

#if defined(VTSS_SW_OPTION_PRIVATE_MIB) && defined(VTSS_SW_OPTION_SMB_IPMC)
        ipmc_mib_init();
#endif

#if defined(VTSS_SW_OPTION_JSON_RPC)
        ipmc_json_init();
#endif

        ipmc_icli_cmd_register();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

