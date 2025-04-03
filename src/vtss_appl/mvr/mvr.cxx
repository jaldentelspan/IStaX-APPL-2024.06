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
#include "mvr_trace.h"
#include "mvr_api.h"
#include "ipmc_lib_api.h"
#include <vtss/appl/mvr.h>

static vtss_appl_ipmc_lib_global_conf_t MVR_global_conf_default[IPMC_LIB_PROTOCOL_CNT];

static vtss_trace_reg_t MVR_trace_reg = {
    VTSS_MODULE_ID_MVR, "MVR", "Multicast VLAN Registration"
};

static vtss_trace_grp_t MVR_trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MVR_TRACE_GRP_ICLI] = {
        "icli",
        "CLI printout",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
    [MVR_TRACE_GRP_WEB] = {
        "web",
        "Web",
        VTSS_TRACE_LVL_ERROR,
        VTSS_TRACE_FLAGS_USEC
    },
};

VTSS_TRACE_REGISTER(&MVR_trace_reg, MVR_trace_grps);

/******************************************************************************/
// MVR_capabilities_set()
// This is the only place where we define maximum values for various parameters,
// so if you need different max. values, change here - only!
/******************************************************************************/
static void MVR_capabilities_set(void)
{
    vtss_appl_ipmc_capabilities_t cap;
    const bool                    is_mvr = true;

    vtss_clear(cap);

    // Maximum number of MVR VLANs we can create. If both IGMP and MLD snooping
    // are supported, IPMC_LIB takes care of multiplying this by two to get the
    // correct maximum number of entries in its VLAN map.
    cap.vlan_cnt_max = 4;

    ipmc_lib_capabilities_set(is_mvr, cap);
}

/******************************************************************************/
// MVR_global_conf_default_set()
/******************************************************************************/
static void MVR_global_conf_default_set(void)
{
    vtss_appl_ipmc_lib_key_t k;
    int                      i;

    vtss_clear(k);
    k.is_mvr = true;

    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        k.is_ipv4 = i == 0;

        vtss_appl_ipmc_lib_global_conf_t &g = MVR_global_conf_default[i];

        // Parameters that MVR can change from management interfaces:
        g.admin_active = false;

        // Parameters that MVR cannot change from management interfaces:

        // By setting unregistered_flooding_enable to true, we let IPMC control
        // this, since we don't have management handles to change it.
        g.unregistered_flooding_enable = true;
        g.proxy_enable                 = false;
        g.leave_proxy_enable           = false;

        // The ssm_prefix is not used by IPMC_LIB for MVR-controlled VLANs, but
        // we must set it to a valid value anyway in order for calls to
        // vtss_appl_ipmc_lib_global_conf_set() to pass.
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
// MVR_port_conf_default_set()
/******************************************************************************/
static void MVR_port_conf_default_set(void)
{
    vtss_appl_ipmc_lib_key_t       k;
    vtss_appl_ipmc_lib_port_conf_t p;
    int                            i;

    vtss_clear(k);
    k.is_mvr = true;

    // Currently, there's no difference between IGMP and MLD defaults.
    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vtss_clear(p);
        k.is_ipv4 = i == 0;

        // Parameters that MVR can change from management interfaces:
        p.fast_leave = false;

        // Parameters that MVR cannot change from management interfaces.
        p.router              = false;
        p.grp_cnt_max         = 0;
        p.profile_key.name[0] = '\0';

        // Tell IPMC LIB about this default configuration
        ipmc_lib_port_default_conf_set(k, p);
    }
}

/******************************************************************************/
// MVR_vlan_port_conf_default_set()
/******************************************************************************/
static void MVR_vlan_port_conf_default_set(void)
{
    vtss_appl_ipmc_lib_key_t            k;
    vtss_appl_ipmc_lib_vlan_port_conf_t c;
    int                                 i;

    vtss_clear(k);
    k.is_mvr = true;

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
// MVR_vlan_conf_default_set()
/******************************************************************************/
static void MVR_vlan_conf_default_set(void)
{
    vtss_appl_ipmc_lib_key_t       k;
    vtss_appl_ipmc_lib_vlan_conf_t c;
    int                            i;

    vtss_clear(k);
    k.is_mvr = true;

    // Currently, there's no difference between IGMP and MLD defaults.
    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vtss_clear(c);
        k.is_ipv4 = i == 0;

        // Parameters that MVR can change from management interfaces:
        c.querier_address.is_ipv4 = k.is_ipv4;
        c.querier_address.all_zeros_set();
        c.name[0]                 = '\0';
        c.compatible_mode         = false;
        c.querier_enable          = false;
        c.tx_tagged               = true;
        c.pcp                     = 0;
        c.lmqi                    = 5; // Differs from IPMC's. RBNTBD: How does this go with one-second ticks?
        c.channel_profile.name[0] = '\0';

        // Parameters that MVR cannot change from management interfaces:
        c.admin_active  = true;
        c.compatibility = VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO;
        c.rv            =   2;
        c.qi            = 125;
        c.qri           = 100;
        c.uri           =   1;

        // Tell IPMC LIB about this default configuration
        ipmc_lib_vlan_default_conf_set(k, c);
    }
}

/******************************************************************************/
// MVR_vlan_add_or_delete()
/******************************************************************************/
static mesa_rc MVR_vlan_add_or_delete(mesa_vid_t vid, bool add)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key;
    int                           i;
    mesa_rc                       rc;

    vlan_key.is_mvr = true;
    vlan_key.vid    = vid;

    // Call IPMC LIB to either create or remove a VLAN interface.
    // We do it once for IGMP and once for MLD (if enabled).
    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vlan_key.is_ipv4 = i == 0;

        T_I("%s: %s VLAN", vlan_key, add ? "Adding" : "Deleting");

        if (add) {
            rc = ipmc_lib_vlan_create(vlan_key, false);
        } else {
            rc = ipmc_lib_vlan_remove(vlan_key);
        }

        if (rc != VTSS_RC_OK) {
            T_I("%s: Unable to %s VLAN ID: %s", vlan_key, add ? "add" : "delete", error_txt(rc));
        }
    }

    return rc;
}

/******************************************************************************/
// vtss_appl_mvr_global_conf_set()
// Wrapper around vtss_appl_ipmc_lib_global_conf_set() to get the same
// configuration set for both IGMP and MLD.
/******************************************************************************/
mesa_rc vtss_appl_mvr_global_conf_set(const vtss_appl_ipmc_lib_global_conf_t *conf)
{
    vtss_appl_ipmc_lib_key_t         key = {};
    vtss_appl_ipmc_lib_global_conf_t local_conf, old_conf;
    int                              i;
    mesa_rc                          rc;

    if (!conf) {
        return VTSS_APPL_IPMC_LIB_RC_INVALID_PARAMETER;
    }

    key.is_mvr  = true;
    key.is_ipv4 = true;

    // Get backup conf.
    if ((rc = vtss_appl_ipmc_lib_global_conf_get(key, &old_conf)) != VTSS_RC_OK) {
        T_I("%s: vtss_appl_ipmc_lib_global_conf_get() failed: %s", key, error_txt(rc));
        return rc;
    }

    local_conf = *conf;
    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        key.is_ipv4 = i == 0;

        // Gotta change SSM prefix to whatever is the default, because it
        // depends on whether we are setting it for IGMP or MLD, and the user
        // has typically just retrieved the current global conf for IPv4,
        // because only a single parameter (admin_active) can be changed in MVR,
        // so when setting the MLD global conf, it would otherwise attempt to
        // set it with the IPv4 SSM range. So we just default it.
        local_conf.ssm_prefix     = MVR_global_conf_default[i].ssm_prefix;
        local_conf.ssm_prefix_len = MVR_global_conf_default[i].ssm_prefix_len;

        if ((rc = vtss_appl_ipmc_lib_global_conf_set(key, &local_conf)) != VTSS_RC_OK) {
            if (!key.is_ipv4) {
                // Restore old IPv4 conf.
                key.is_ipv4 = true;
                (void)vtss_appl_ipmc_lib_global_conf_set(key, &old_conf);
            }

            break;
        }
    }

    return rc;
}

/******************************************************************************/
// vtss_appl_mvr_port_conf_set()
// Wrapper around vtss_appl_ipmc_lib_port_conf_set() to get the same
// configuration set for both IGMP and MLD.
/******************************************************************************/
mesa_rc vtss_appl_mvr_port_conf_set(mesa_port_no_t port_no, const vtss_appl_ipmc_lib_port_conf_t *conf)
{
    vtss_appl_ipmc_lib_key_t       key = {};
    vtss_appl_ipmc_lib_port_conf_t old_conf;
    int                            i;
    mesa_rc                        rc;

    key.is_mvr  = true;
    key.is_ipv4 = true;

    // Get backup conf.
    if ((rc = vtss_appl_ipmc_lib_port_conf_get(key, port_no, &old_conf)) != VTSS_RC_OK) {
        T_I("%s: vtss_appl_ipmc_lib_port_conf_get(%u) failed: %s", key, port_no, error_txt(rc));
        return rc;
    }

    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        key.is_ipv4 = i == 0;

        if ((rc = vtss_appl_ipmc_lib_port_conf_set(key, port_no, conf)) != VTSS_RC_OK) {
            if (!key.is_ipv4) {
                // Restore old IPv4 conf.
                key.is_ipv4 = true;
                (void)vtss_appl_ipmc_lib_port_conf_set(key, port_no, &old_conf);
            }

            break;
        }
    }

    return rc;
}

/******************************************************************************/
// vtss_appl_mvr_vlan_port_conf_set()
// Wrapper around vtss_appl_ipmc_lib_vlan_port_conf_set() to get the same
// configuration set for both IGMP and MLD.
/******************************************************************************/
mesa_rc vtss_appl_mvr_vlan_port_conf_set(mesa_vid_t vid, mesa_port_no_t port_no, const vtss_appl_ipmc_lib_vlan_port_conf_t *conf)
{
    vtss_appl_ipmc_lib_vlan_key_t       vlan_key = {};
    vtss_appl_ipmc_lib_vlan_port_conf_t old_conf;
    int                                 i;
    mesa_rc                             rc;

    vlan_key.is_mvr  = true;
    vlan_key.is_ipv4 = true;
    vlan_key.vid     = vid;

    // Get backup conf.
    if ((rc = vtss_appl_ipmc_lib_vlan_port_conf_get(vlan_key, port_no, &old_conf)) != VTSS_RC_OK) {
        T_I("%s: vtss_appl_ipmc_lib_vlan_port_conf_get(%u) failed: %s", vlan_key, port_no, error_txt(rc));
        return rc;
    }

    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vlan_key.is_ipv4 = i == 0;

        if ((rc = vtss_appl_ipmc_lib_vlan_port_conf_set(vlan_key, port_no, conf)) != VTSS_RC_OK) {
            if (!vlan_key.is_ipv4) {
                // Restore old IPv4 conf.
                vlan_key.is_ipv4 = true;
                (void)vtss_appl_ipmc_lib_vlan_port_conf_set(vlan_key, port_no, &old_conf);
            }

            break;
        }
    }

    return rc;
}

/******************************************************************************/
// vtss_appl_mvr_vlan_conf_set()
// Wrapper around vtss_appl_ipmc_lib_vlan_conf_set() to get the same
// configuration set for both IGMP and MLD.
/******************************************************************************/
mesa_rc vtss_appl_mvr_vlan_conf_set(mesa_vid_t vid, const vtss_appl_ipmc_lib_vlan_conf_t *conf)
{
    vtss_appl_ipmc_lib_vlan_key_t  vlan_key = {};
    vtss_appl_ipmc_lib_vlan_conf_t old_conf, local_conf;
    int                            i;
    mesa_rc                        rc;

    if (!conf) {
        return VTSS_APPL_IPMC_LIB_RC_INVALID_PARAMETER;
    }

    vlan_key.is_mvr  = true;
    vlan_key.is_ipv4 = true;
    vlan_key.vid     = vid;

    // Get backup conf.
    if ((rc = vtss_appl_ipmc_lib_vlan_conf_get(vlan_key, &old_conf)) != VTSS_RC_OK) {
        T_I("%s: vtss_appl_ipmc_lib_vlan_port_conf_get() failed: %s", vlan_key, error_txt(rc));

        if (rc == VTSS_APPL_IPMC_LIB_RC_NO_SUCH_VLAN) {
            // Attempt to create it
            VTSS_RC(MVR_vlan_add_or_delete(vid, true));
        }
    }

    local_conf = *conf;
    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vlan_key.is_ipv4 = i == 0;

        if (!vlan_key.is_ipv4) {
            // Gotta reset querier address for IPv6.
            local_conf.querier_address.is_ipv4 = false;
            local_conf.querier_address.all_zeros_set();
        }

        if ((rc = vtss_appl_ipmc_lib_vlan_conf_set(vlan_key, conf)) != VTSS_RC_OK) {
            if (!vlan_key.is_ipv4) {
                // Restore old IPv4 conf.
                vlan_key.is_ipv4 = true;
                (void)vtss_appl_ipmc_lib_vlan_conf_set(vlan_key, &old_conf);
            }

            break;
        }
    }

    return rc;
}

/******************************************************************************/
// vtss_appl_mvr_vlan_conf_del()
/******************************************************************************/
mesa_rc vtss_appl_mvr_vlan_conf_del(mesa_vid_t vid)
{
    return MVR_vlan_add_or_delete(vid, false);
}

/******************************************************************************/
// vtss_appl_mvr_vlan_statistics_clear()
/******************************************************************************/
mesa_rc vtss_appl_mvr_vlan_statistics_clear(mesa_vid_t vid)
{
    vtss_appl_ipmc_lib_vlan_key_t vlan_key = {};
    int                           i;

    vlan_key.is_mvr  = true;
    vlan_key.is_ipv4 = true;
    vlan_key.vid     = vid;

    for (i = 0; i < IPMC_LIB_PROTOCOL_CNT; i++) {
        vlan_key.is_ipv4 = i == 0;
        VTSS_RC(vtss_appl_ipmc_lib_vlan_statistics_clear(vlan_key));
    }

    return VTSS_RC_OK;
}

extern "C" void mvr_mib_init(void);
extern "C" void mvr_json_init(void);
extern "C" int  mvr_icli_cmd_register();

/******************************************************************************/
// mvr_init()
/******************************************************************************/
mesa_rc mvr_init(vtss_init_data_t *data)
{
    mesa_rc rc;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        // Create capabilities
        MVR_capabilities_set();

        // Create global configuration defaults once and for all.
        MVR_global_conf_default_set();

        // Create port configuration defaults once and for all.
        MVR_port_conf_default_set();

        // Create per-port-per-VLAN configuration defaults once and for all.
        MVR_vlan_port_conf_default_set();

        // Create VLAN configuration defaults once and for all.
        MVR_vlan_conf_default_set();

#ifdef VTSS_SW_OPTION_ICFG
        mesa_rc mvr_icfg_init(void);
        if ((rc = mvr_icfg_init()) != VTSS_RC_OK) {
            T_E("mvr_icfg_init() failed: %s", error_txt(rc));
        }
#endif

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
        mvr_mib_init();
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
        mvr_json_init();
#endif

        mvr_icli_cmd_register();
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

