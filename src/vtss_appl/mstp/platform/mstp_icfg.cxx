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

#include "mstp_api.h"
#include "icfg_api.h"
#include "misc_api.h"
#include "icli_api.h"
#include "topo_api.h"

#define VTSS_TRACE_MODULE_ID        VTSS_MODULE_ID_RSTP
#define VTSS_ALLOC_MODULE_ID        VTSS_MODULE_ID_RSTP

//******************************************************************************
// ICFG callback functions
//******************************************************************************

static mesa_rc mstp_icfg_global_conf(const vtss_icfg_query_request_t *req,
                                     vtss_icfg_query_result_t *result)
{
    mesa_rc               rc = VTSS_RC_OK;
    mstp_bridge_param_t   sc;
    u32 def_ver = MSTP_PROTOCOL_VERSION_MSTP;
    vtss_appl_mstp_msti_t msti, mstimax = N_MSTI_MAX;
    u32 prio;
#if !defined(VTSS_MSTP_FULL)
    def_ver = MSTP_PROTOCOL_VERSION_RSTP;
#endif /* VTSS_MSTP_FULL */

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_appl_mstp_system_config_get(&sc));

    /* Global level */
    if (req->all_defaults || sc.forceVersion != def_ver) {
        VTSS_RC(vtss_icfg_printf(result, "spanning-tree mode %s\n",
                                 sc.forceVersion == MSTP_PROTOCOL_VERSION_MSTP ? "mstp" :
                                 sc.forceVersion == MSTP_PROTOCOL_VERSION_RSTP ? "rstp" : "stp"));
    }
    if (req->all_defaults || sc.bridgeHelloTime != 2) {
        VTSS_RC(vtss_icfg_printf(result, "spanning-tree mst hello-time %d\n", sc.bridgeHelloTime));
    }
    if (req->all_defaults || sc.bridgeMaxAge != 20 || sc.bridgeForwardDelay != 15)  {
        VTSS_RC(vtss_icfg_printf(result, "spanning-tree mst max-age %d forward-time %d\n", sc.bridgeMaxAge, sc.bridgeForwardDelay));
    }
    if (req->all_defaults || sc.txHoldCount != 6) {
        VTSS_RC(vtss_icfg_printf(result, "spanning-tree transmit hold-count %d\n", sc.txHoldCount));
    }
    if (req->all_defaults || sc.MaxHops != 20) {
        VTSS_RC(vtss_icfg_printf(result, "spanning-tree mst max-hops %d\n", sc.MaxHops));
    }
#ifdef VTSS_SW_OPT_MSTP_BPDU_ENH
    if (req->all_defaults || sc.bpduFiltering) {
        VTSS_RC(vtss_icfg_printf(result, "%sspanning-tree edge bpdu-filter\n", sc.bpduFiltering ? "" : "no "));
    }
    if (req->all_defaults || sc.bpduGuard) {
        VTSS_RC(vtss_icfg_printf(result, "%sspanning-tree edge bpdu-guard\n", sc.bpduGuard ? "" : "no "));
    }
    if (req->all_defaults || sc.errorRecoveryDelay != 0) {
        if (sc.errorRecoveryDelay == 0) {
            VTSS_RC(vtss_icfg_printf(result, "no spanning-tree recovery interval\n"));
        } else {
            VTSS_RC(vtss_icfg_printf(result, "spanning-tree recovery interval %d\n", sc.errorRecoveryDelay));
        }
    }
#endif /* VTSS_SW_OPT_MSTP_BPDU_ENH */

    if (vtss_appl_mstp_system_config_get(&sc) == VTSS_RC_OK && sc.forceVersion < MSTP_PROTOCOL_VERSION_MSTP) {
        mstimax = 1;
    }
    for (msti = 0; msti < mstimax; msti++) {
        prio = vtss_mstp_msti_priority_get(msti) << 8;
        if (req->all_defaults || prio != 32768) {
            VTSS_RC(vtss_icfg_printf(result, "spanning-tree mst %d priority %d\n", msti, prio));
        }
    }

#if defined(VTSS_MSTP_FULL)
    const int vlanrange_size = 10 / 2 * 2 + (100 - 10) / 2 * 3 + (1000 - 100) / 2 * 4 + (4096 - 1000) / 2 * 5; // Worst-case: Every odd-numbered VLAN, comma-separated
    mstp_msti_config_t *conf = (mstp_msti_config_t *)VTSS_MALLOC(sizeof(*conf));
    char *vlanrange = (char *)VTSS_MALLOC(vlanrange_size);

    if (!conf || !vlanrange) {
        T_E("Out of heap memory");
        VTSS_FREE(conf);
        VTSS_FREE(vlanrange);
        return VTSS_RC_ERROR;
    }

    if (vtss_appl_mstp_msti_config_get(conf, NULL) == VTSS_RC_OK) {
        if (req->all_defaults || strlen(conf->configname) != 0) {
            if (strlen(conf->configname) == 0) {
                rc = vtss_icfg_printf(result, "no spanning-tree mst name\n");
            } else {
                rc = vtss_icfg_printf(result, "spanning-tree mst name %.*s revision %d\n",
                                      (int)sizeof(conf->configname), conf->configname, conf->revision);
            }
        }

        for (msti = 1; rc == VTSS_RC_OK && msti < N_MSTI_MAX; msti++) {
            vlanrange[0] = vlanrange[vlanrange_size - 1] = '\0';
            (void)mstp_mstimap2str(conf, msti, vlanrange, vlanrange_size);
            if (req->all_defaults || strlen(vlanrange) != 0) {
                if (strlen(vlanrange) == 0) {
                    rc = vtss_icfg_printf(result, "no spanning-tree mst %d vlan\n", msti);
                } else {
                    rc = vtss_icfg_printf(result, "spanning-tree mst %d vlan %s\n", msti, vlanrange);
                }
            }
        }
        (void) mstp_mstimap2str(conf, VTSS_MSTID_TE, vlanrange, vlanrange_size);
        if (req->all_defaults || strlen(vlanrange) != 0) {
            if (strlen(vlanrange) == 0) {
                rc = vtss_icfg_printf(result, "no spanning-tree mst %s vlan\n", "te");
            } else {
                rc = vtss_icfg_printf(result, "spanning-tree mst %s vlan %s\n", "te", vlanrange);
            }
        }
    }

    VTSS_FREE(conf);
    VTSS_FREE(vlanrange);
#endif /* VTSS_MSTP_FULL */

    return rc;
}

static mesa_rc mstp_icfg_intf_conf(const vtss_icfg_query_request_t *req,
                                   vtss_icfg_query_result_t *result)
{
    mesa_rc                  rc = VTSS_RC_OK;
    vtss_isid_t              isid;
    mesa_port_no_t           iport;
    mstp_port_param_t        conf;
    BOOL                     enable;

    if (!req || !result) {
        return VTSS_RC_ERROR;
    }

    /* Interface level */
    if (req->cmd_mode == ICLI_CMD_MODE_STP_AGGR) {
        isid = VTSS_ISID_LOCAL;
        iport = VTSS_PORT_NO_NONE;
    } else {
        isid = topo_usid2isid(req->instance_id.port.usid);
        iport = uport2iport(req->instance_id.port.begin_uport);
    }

    VTSS_RC(vtss_mstp_port_config_get(isid, iport, &enable, &conf));

    if (req->all_defaults || !enable) {
        VTSS_RC(vtss_icfg_printf(result, " %sspanning-tree\n", enable ? "" : "no "));
    }
    if (req->all_defaults || conf.adminEdgePort) {
        VTSS_RC(vtss_icfg_printf(result, " %sspanning-tree edge\n", conf.adminEdgePort ? "" : "no "));
    }
    if (req->all_defaults || !conf.adminAutoEdgePort) {
        VTSS_RC(vtss_icfg_printf(result, " %sspanning-tree auto-edge\n", conf.adminAutoEdgePort ? "" : "no "));
    }
    if (req->all_defaults || (conf.adminPointToPointMAC != P2P_AUTO)) {
        VTSS_RC(vtss_icfg_printf(result, " spanning-tree link-type %s\n",
                                 conf.adminPointToPointMAC == P2P_AUTO ? "auto" :
                                 conf.adminPointToPointMAC == P2P_FORCETRUE ? "point-to-point" : "shared"));
    }
    if (req->all_defaults || conf.restrictedRole) {
        VTSS_RC(vtss_icfg_printf(result, " %sspanning-tree restricted-role\n", conf.restrictedRole ? "" : "no "));
    }
    if (req->all_defaults || conf.restrictedTcn) {
        VTSS_RC(vtss_icfg_printf(result, " %sspanning-tree restricted-tcn\n", conf.restrictedTcn ? "" : "no "));
    }
#ifdef VTSS_SW_OPT_MSTP_BPDU_ENH
    if (req->all_defaults || conf.bpduGuard) {
        VTSS_RC(vtss_icfg_printf(result, " %sspanning-tree bpdu-guard\n", conf.bpduGuard ? "" : "no "));
    }
#endif /* VTSS_SW_OPT_MSTP_BPDU_ENH */

    mstp_msti_port_param_t    msti_conf;
    u8 msti;
    for (msti = 0; msti < N_MSTI_MAX; msti++) {
        if (vtss_mstp_msti_port_config_get(isid, msti, iport, &msti_conf) == VTSS_RC_OK) {
            if (req->all_defaults || msti_conf.adminPathCost != 0) {
                if (msti_conf.adminPathCost != 0) {
                    VTSS_RC(vtss_icfg_printf(result, " spanning-tree mst %d cost %d\n", msti, msti_conf.adminPathCost));
                } else {
                    VTSS_RC(vtss_icfg_printf(result, " spanning-tree mst %d cost %s\n", msti, "auto"));
                }
            }
            if (req->all_defaults || msti_conf.adminPortPriority != 128) {
                VTSS_RC(vtss_icfg_printf(result, " spanning-tree mst %d port-priority %d\n", msti, msti_conf.adminPortPriority));
            }
        }
    }

    return rc;
}

//******************************************************************************
//   Public functions
//******************************************************************************

mesa_rc mstp_icfg_init(void)
{
    mesa_rc rc;

    /* Interface - Register callback functions to ICFG module */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_MSTP_INTERFACE_CONF, "mstp", mstp_icfg_intf_conf)) != VTSS_RC_OK) {
        return rc;
    }

    /* Aggregation - Register callback functions to ICFG module */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_MSTP_AGGR_CONF, "mstp", mstp_icfg_intf_conf)) != VTSS_RC_OK) {
        return rc;
    }

    /* Global - Register callback functions to ICFG module */
    if ((rc = vtss_icfg_query_register(VTSS_ICFG_MSTP_GLOBAL_CONF, "mstp", mstp_icfg_global_conf)) != VTSS_RC_OK) {
        return rc;
    }

    return VTSS_RC_OK;
}

