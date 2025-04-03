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

#include "web_api.h"
#include "mstp_api.h"
#include "vlan_api.h"
#include "port_api.h" // For port_count_max()

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_RSTP
#define MSTP_WEB_BUF_LEN 512

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static i32 handler_config_rstp_sys(CYG_HTTPD_STATE *p)
{
    mstp_bridge_param_t *conf;
    u8                  priority;
    int                 ct, val;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP)) {
        return -1;
    }
#endif

    conf = (mstp_bridge_param_t *)VTSS_MALLOC(sizeof(*conf));
    if (conf) {
        priority = vtss_mstp_msti_priority_get(0);
        if (vtss_appl_mstp_system_config_get(conf) == VTSS_RC_OK) {
            if (p->method == CYG_HTTPD_METHOD_POST) {
                if (cyg_httpd_form_varable_int(p, "proto", &val)) {
                    conf->forceVersion = (mstp_forceversion_t)val;
                }

                if (cyg_httpd_form_varable_int(p, "prio", &val)) {
                    priority = val;
                }

                if (cyg_httpd_form_varable_int(p, "hello", &val)) {
                    conf->bridgeHelloTime = val;
                }

                if (cyg_httpd_form_varable_int(p, "delay", &val)) {
                    conf->bridgeForwardDelay = val;
                }

                if (cyg_httpd_form_varable_int(p, "maxage", &val)) {
                    conf->bridgeMaxAge = val;
                }

                if (cyg_httpd_form_varable_int(p, "maxhops", &val)) {
                    conf->MaxHops = val;
                }

                if (cyg_httpd_form_varable_int(p, "txholdcount", &val)) {
                    conf->txHoldCount = val;
                }

#ifdef VTSS_SW_OPT_MSTP_BPDU_ENH
                conf->bpduFiltering = !!cyg_httpd_form_varable_find(p, "bpdufiltering");
                conf->bpduGuard = !!cyg_httpd_form_varable_find(p, "bpduguard");

                if (cyg_httpd_form_varable_find(p, "recovery") &&
                    cyg_httpd_form_varable_int(p, "recoverytimeout", &val)) {
                    conf->errorRecoveryDelay = val;
                } else {
                    conf->errorRecoveryDelay = 0;    /* Disabled */
                }

#endif /* VTSS_SW_OPT_MSTP_BPDU_ENH */
                (void)vtss_appl_mstp_system_config_set(conf);
                (void)vtss_mstp_msti_priority_set(0, priority);
                redirect(p, "/mstp_sys_config.htm");
            } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
                cyg_httpd_start_chunked("html");
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                              conf->forceVersion,
                              priority,
                              conf->bridgeHelloTime,
                              conf->bridgeForwardDelay,
                              conf->bridgeMaxAge,
                              conf->MaxHops,
                              conf->txHoldCount,
                              conf->bpduFiltering,
                              conf->bpduGuard,
                              conf->errorRecoveryDelay);
                cyg_httpd_write_chunked(p->outbuffer, ct);
                cyg_httpd_end_chunked();
            }
        }

        VTSS_FREE(conf);
    }

    return -1; // Do not further search the file system.
}

static BOOL valid_vid(int vid)
{
    return (vid >= VTSS_APPL_VLAN_ID_MIN && vid <= MIN(VTSS_APPL_VLAN_ID_MAX, 4094));
}

static void add_map_vid_range(mstp_msti_config_t *conf, vtss_appl_mstp_mstid_t mstid, const char *str)
{
    int i, start, end;

    if (sscanf(str, "%d-%d", &start, &end) == 2 &&
        valid_vid(start) &&
        valid_vid(end) &&
        start <= end) {
        for (i = start; i <= end; i++) {
            conf->map.map[i] = mstid;
        }
    } else if (sscanf(str, "%d", &start) == 1) {
        conf->map.map[start] = mstid;
    } else {
        // T_W("MSTI map add range: '%s' is invalid", str);
    }
}

static i32 handler_config_rstp_msti_map(CYG_HTTPD_STATE *p)
{
    vtss_appl_mstp_msti_t  msti;
    vtss_appl_mstp_mstid_t mstid;
    int                    value;
    size_t                 len;
    mstp_msti_config_t     mconf, *conf = &mconf;
    const char             *configname, *delim, *err_buf_ptr;
    char                   *mapping, *ptr, *saveptr;
    uint                   ct;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP)) {
        return -1;
    }
#endif

    (void)vtss_appl_mstp_msti_config_get(conf, NULL);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* Name */
        configname = cyg_httpd_form_varable_string(p, "name", &len);
        if (configname && len > 0) {
            (void)cgi_unescape(configname, conf->configname, len, sizeof(conf->configname));
        } else {
            memset(conf->configname, 0, sizeof(conf->configname));
        }

        /* Revision */
        if (cyg_httpd_form_varable_int(p, "revision", &value) &&
            value <= 0xffff) {
            conf->revision = value;
        } else {
            conf->revision = 0;
        }

        /* Mapping */
        memset(&conf->map, 0, sizeof(conf->map)); /* Reset mapping (to CIST) */
        for (msti = 0; msti <= N_MSTI_MAX; msti++) {
            mstid = msti < N_MSTI_MAX ? msti : VTSS_MSTID_TE;
            mapping = cyg_httpd_form_varable_strdup(p, "map_%d", msti);
            if (mapping) {
                delim = " ,";
                for (ptr = strtok_r(mapping, delim, &saveptr); ptr != NULL; ptr = strtok_r(NULL, delim, &saveptr)) {
                    add_map_vid_range(conf, mstid, ptr);
                }

                VTSS_FREE(mapping);
            }
        }

        if (vtss_appl_mstp_msti_config_set(conf) != VTSS_RC_OK) {
            err_buf_ptr = "STP MSTI mapping configuration error";
            send_custom_error(p, "STP Error", err_buf_ptr, strlen(err_buf_ptr));
            return -1;
        }

        redirect(p, "/mstp_msti_map_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        char cname[sizeof(conf->configname) * 3];

        (void)cgi_escape_n(conf->configname, cname, sizeof(conf->configname));
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|%u|", cname, conf->revision);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        // N_MSTI_MAX ~ VTSS_MSTID_TE
        for (msti = 1; msti <= N_MSTI_MAX; msti++) {
            mstid = msti < N_MSTI_MAX ? msti : VTSS_MSTID_TE;
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%s/", msti, msti_name(mstid));
            cyg_httpd_write_chunked(p->outbuffer, ct);
            (void) mstp_mstimap2str(conf, mstid, p->outbuffer, sizeof(p->outbuffer));
            if ((ct = strlen(p->outbuffer)) > 0) {
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            cyg_httpd_write_chunked("|", 1);
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_rstp_msti(CYG_HTTPD_STATE *p)
{
    u8         priority;
    u8         msti, msti_max = 1;
    const char *err_buf_ptr;
    int        ct, val;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP)) {
        return -1;
    }
#endif

#if defined(VTSS_MSTP_FULL)
    msti_max = N_MSTI_MAX;
#endif  /* VTSS_MSTP_FULL */

    if (p->method == CYG_HTTPD_METHOD_POST) {
        for (msti = 0; msti < msti_max; msti++) {
            if (cyg_httpd_form_variable_int_fmt(p, &val, "mstiprio_%d", msti)) {
                priority = val;
                if (vtss_mstp_msti_priority_set(msti, priority) != VTSS_RC_OK) {
                    err_buf_ptr = "STP msti configuration error";
                    send_custom_error(p, "STP Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }
            }
        }

        redirect(p, "/mstp_msti_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        for (msti = 0; msti < msti_max; msti++) {
            priority = vtss_mstp_msti_priority_get(msti);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%s/%u|",
                          msti,
                          msti_name(msti),
                          priority);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_rstp_ports(CYG_HTTPD_STATE *p)
{
    vtss_isid_t            sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                    ct, val;
    u32                    portcount;
    mesa_port_no_t         iport;
    vtss_uport_no_t        uport;
    mstp_port_param_t      portconf;
    mstp_msti_port_param_t mportconf;
    const char             *err_buf_ptr;
    BOOL                   enable;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP)) {
        return -1;
    }
#endif

    portcount = port_count_max();

    if (p->method == CYG_HTTPD_METHOD_POST) {
        for (uport = 0 /* SIC! For aggr */; uport <= portcount; uport++) {
            iport = uport2iport(uport);

            if (vtss_mstp_port_config_get(sid, iport, &enable, &portconf) == VTSS_RC_OK &&
                vtss_mstp_msti_port_config_get(sid, 0, iport, &mportconf) == VTSS_RC_OK) {
                enable = cyg_httpd_form_variable_check_fmt(p, "enable_%d", uport);
                if (cyg_httpd_form_variable_int_fmt(p, &val, "pcauto_%d", uport) && !val) {
                    if (cyg_httpd_form_variable_int_fmt(p, &val, "pathcost_%d", uport)) {
                        mportconf.adminPathCost = val;
                    }
                } else {
                    mportconf.adminPathCost = 0;
                }

                if (cyg_httpd_form_variable_int_fmt(p, &val, "portprio_%d", uport)) {
                    mportconf.adminPortPriority = val;
                }

                if (cyg_httpd_form_variable_int_fmt(p, &val, "adminEdge_%d", uport)) {
                    portconf.adminEdgePort = !!val;
                }

                portconf.adminAutoEdgePort = cyg_httpd_form_variable_check_fmt(p, "autoEdge_%d", uport);
                if (cyg_httpd_form_variable_int_fmt(p, &val, "p2p_%d", uport)) {
                    portconf.adminPointToPointMAC = (mstp_p2p_t)val;
                }

                portconf.adminAutoEdgePort = cyg_httpd_form_variable_check_fmt(p, "autoEdge_%d", uport);
                portconf.restrictedRole    = cyg_httpd_form_variable_check_fmt(p, "rRole_%d",    uport);
                portconf.restrictedTcn     = cyg_httpd_form_variable_check_fmt(p, "rTcn_%d",     uport);

#ifdef VTSS_SW_OPT_MSTP_BPDU_ENH
                portconf.bpduGuard = cyg_httpd_form_variable_check_fmt(p, "bpduGuard_%d", uport);
#endif

                if (vtss_mstp_port_config_set(sid, iport, enable, &portconf) != VTSS_RC_OK ||
                    vtss_mstp_msti_port_config_set(sid, 0, iport, &mportconf) != VTSS_RC_OK) {
                    err_buf_ptr = "STP port configuration error";
                    send_custom_error(p, "STP Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }
            }
        }

        redirect(p, "/mstp_port_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        for (uport = 0; uport <= portcount; uport++) {
            iport = uport2iport(uport);

            if (vtss_mstp_port_config_get(sid, iport, &enable, &portconf) == VTSS_RC_OK &&
                vtss_mstp_msti_port_config_get(sid, 0, iport, &mportconf) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%d/%d/%d/%d/%d/%d/%d/%d/%d|",
                              uport,
                              enable,
                              mportconf.adminPathCost,
                              mportconf.adminPortPriority,
                              portconf.adminEdgePort,
                              portconf.adminAutoEdgePort,
                              portconf.adminPointToPointMAC,
                              portconf.restrictedRole,
                              portconf.restrictedTcn,
                              portconf.bpduGuard);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_rstp_msti_ports(CYG_HTTPD_STATE *p)
{
    vtss_isid_t            sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                    ct;
    mesa_port_no_t         iport;
    vtss_uport_no_t        uport;
    mstp_msti_port_param_t mportconf;
    int                    msti = 0, val;
    const char             *err_buf_ptr;
    u32                    portcount;

    if (cyg_httpd_form_varable_int(p, "bridge", &msti) && (msti < 0 || msti >= N_MSTI_MAX)) {
        msti = 0;
    }

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RSTP)) {
        return -1;
    }
#endif

    portcount = port_count_max();

    if (p->method == CYG_HTTPD_METHOD_POST) {
        for (uport = 0 /* SIC! For aggr */; uport <= portcount; uport++) {
            iport = uport2iport(uport);

            if (vtss_mstp_msti_port_config_get(sid, msti, iport, &mportconf) == VTSS_RC_OK) {
                if (cyg_httpd_form_variable_int_fmt(p, &val, "pcauto_%d", uport) && !val) {
                    if (cyg_httpd_form_variable_int_fmt(p, &val, "pathcost_%d", uport)) {
                        mportconf.adminPathCost = val;
                    }
                } else {
                    mportconf.adminPathCost = 0;
                }

                if (cyg_httpd_form_variable_int_fmt(p, &val, "mstiprio_%d", uport)) {
                    mportconf.adminPortPriority = val;
                }

                if (vtss_mstp_msti_port_config_set(sid, msti, iport, &mportconf) != VTSS_RC_OK) {
                    err_buf_ptr = "STP msti port configuration error";
                    send_custom_error(p, "STP Error", err_buf_ptr, strlen(err_buf_ptr));
                    return -1;
                }
            }
        }

        redirect(p, "/mstp_msti_port_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        for (uport = 0 /* SIC! For aggr */; uport <= portcount; uport++) {
            iport = uport2iport(uport);
            if (vtss_mstp_msti_port_config_get(sid, msti, iport, &mportconf) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%d/%d|",
                              uport,
                              mportconf.adminPathCost,
                              mportconf.adminPortPriority);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static void stat_rstp_statistics_port(CYG_HTTPD_STATE *p, l2_port_no_t l2port, const mstp_port_statistics_t *pp)
{
    int ct;

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                  "%s/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u|",
                  l2port2str(l2port),
                  pp->mstp_frame_xmits,
                  pp->rstp_frame_xmits,
                  pp->stp_frame_xmits,
                  pp->tcn_frame_xmits,
                  pp->mstp_frame_recvs,
                  pp->rstp_frame_recvs,
                  pp->stp_frame_recvs,
                  pp->tcn_frame_recvs,
                  pp->illegal_frame_recvs,
                  pp->unknown_frame_recvs);
    cyg_httpd_write_chunked(p->outbuffer, ct);
}

static i32 handler_stat_rstp_statistics(CYG_HTTPD_STATE *p)
{
    vtss_isid_t            isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mstp_port_statistics_t ps;
    l2port_iter_t          l2pit;
    BOOL                   clear = (cyg_httpd_form_varable_find(p, "clear") != NULL);

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_RSTP)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    (void)l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, L2PORT_ITER_TYPE_ALL);

    while (l2port_iter_getnext(&l2pit)) {
        if (mstp_get_port_statistics(l2pit.l2port, &ps, clear)) {
            stat_rstp_statistics_port(p, l2pit.l2port, &ps);
        }
    }

    cyg_httpd_write_chunked("|", 1); /* Must return something - <empty> is stack error! */
    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

static void mstp_bridge_status(CYG_HTTPD_STATE *p, u8 msti, const mstp_bridge_status_t *bp)
{
    char br_root[32], br_this[32], br_reg[32];
    int  ct;

    (void)vtss_appl_mstp_bridge2str(br_this, sizeof(br_this), bp->bridgeId);
    (void)vtss_appl_mstp_bridge2str(br_root, sizeof(br_root), bp->designatedRoot);
    (void)vtss_appl_mstp_bridge2str(br_reg, sizeof(br_reg), bp->cistRegionalRoot);

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                  "%d/%s/%s/%s/%s/%u/%s/%s/%u/%u/%s|",
                  /* 0 */ msti,
                  /* 1 */ msti_name(msti),
                  /* 2 */ br_this,
                  /* 3 */ br_root,
                  /* 4 */ bp->rootPort != L2_NULL ? l2port2str(bp->rootPort) : "-",
                  /* 5 */ bp->rootPathCost,
                  /* 6 */ bp->topologyChange ? "Changing" : "Steady",
                  /* 7 */ (bp->timeSinceTopologyChange == MSTP_TIMESINCE_NEVER ? "-" :
                           misc_time2interval(bp->timeSinceTopologyChange)),
                  /* 8 */ bp->topologyChangeCount,
                  /* 9 */ bp->cistInternalPathCost,
                  /* 10 */ br_reg);

    cyg_httpd_write_chunked(p->outbuffer, ct);
}

static i32 handler_stat_rstp_status_bridges(CYG_HTTPD_STATE *p)
{
    mstp_bridge_status_t bridge_status, *bp = &bridge_status;
    u8                   msti;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_RSTP)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    for (msti = 0; msti < N_MSTI_MAX; msti++)
        if (vtss_appl_mstp_bridge_status_get(msti, bp) == VTSS_RC_OK) {
            mstp_bridge_status(p, msti, bp);
        }

    cyg_httpd_write_chunked("|", 1); /* Must return something - <empty> is stack error! */
    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

static void rstp_web_status_bridge_port(CYG_HTTPD_STATE *p, vtss_usid_t usid, l2_port_no_t l2port, mstp_port_mgmt_status_t *pp)
{
    int  ct = 0;
    char buf[64];

    if (usid == VTSS_ISID_UNKNOWN) {
        (void)snprintf(buf, sizeof(buf), "/%s", l2port2str(l2port));
    } else {
        (void)snprintf(buf, sizeof(buf), "%d/%s", usid, l2port2str(l2port));
    }

    if (pp->parent != L2_NULL) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "%s/%s/Part of %s/%s/////|",
                      buf,
                      "-",
                      l2port2str(pp->parent),
                      pp->fwdstate);
    } else if (pp->active) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "%s/%d:%03x/%s/%s/%u/%s/%s/%s|",
                      buf,
                      pp->core.portId[0] & 0xf0,
                      pp->core.portId[1] + ((pp->core.portId[0] & 0xf) << 8),
                      pp->core.rolestr,
                      pp->core.statestr,
                      pp->core.pathCost,
                      pp->core.operEdgePort ? "Yes" : "No",
                      pp->core.operPointToPointMAC ? "Yes" : "No",
                      pp->core.uptime ? misc_time2interval(pp->core.uptime) : "-");
    }

    if (ct) {
        cyg_httpd_write_chunked(p->outbuffer, ct);
    }
}

static i32 handler_stat_rstp_status_bridge(CYG_HTTPD_STATE *p)
{
    mstp_bridge_status_t    bridge_status, *bp = &bridge_status;
    mstp_port_mgmt_status_t ps, *pp = &ps;
    l2port_iter_t           l2pit;
    int                     msti = 0;

    if (cyg_httpd_form_varable_int(p, "bridge", &msti) &&
        (msti < 0 || msti >= N_MSTI_MAX)) {
        msti = 0;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_RSTP)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    if (vtss_appl_mstp_bridge_status_get(msti, bp) == VTSS_RC_OK) {
        mstp_bridge_status(p, msti, bp);

        (void)l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, L2PORT_ITER_TYPE_ALL);
        while (l2port_iter_getnext(&l2pit)) {
            if (mstp_get_port_status(msti, l2pit.l2port, pp)) {
                rstp_web_status_bridge_port(p, l2pit.usid, l2pit.l2port, pp);
            }
        }
    }

    cyg_httpd_write_chunked("|", 1); /* Must return something - <empty> is stack error! */
    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

static void rstp_web_status_switch_port(CYG_HTTPD_STATE *p, l2_port_no_t l2port, mstp_port_mgmt_status_t *pp)
{
    size_t ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/", l2port2str(l2port));

    cyg_httpd_write_chunked(p->outbuffer, ct);
    if (pp->parent != L2_NULL) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "Port of %s/%s/%s|",
                      l2port2str(pp->parent),
                      pp->fwdstate,
                      "-");
    } else if (pp->enabled) {
        if (pp->active) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "%s/%s/%s|",
                          pp->core.rolestr,
                          pp->core.statestr,
                          pp->core.uptime ? misc_time2interval(pp->core.uptime) : "-");
        } else {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "%s/%s/%s|",
                          "Disabled",
                          pp->fwdstate,
                          "-");
        }
    } else {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "%s/%s/%s|",
                      "Non-STP",
                      pp->fwdstate,
                      "-");
    }

    if (ct) {
        cyg_httpd_write_chunked(p->outbuffer, ct);
    }
}

static i32 handler_stat_rstp_status_ports(CYG_HTTPD_STATE *p)
{
    vtss_isid_t             isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    l2port_iter_t           l2pit;
    mstp_port_mgmt_status_t ps, *pp = &ps;
    u8                      msti = 0; /* CIST status */

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_RSTP)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    (void)l2port_iter_init(&l2pit, isid, L2PORT_ITER_TYPE_ALL);
    while (l2port_iter_getnext(&l2pit)) {
        if (mstp_get_port_status(msti, l2pit.l2port, pp) &&
            (L2PIT_TYPE(&l2pit, L2PORT_ITER_TYPE_PHYS) || pp->active)) {
            rstp_web_status_switch_port(p, l2pit.l2port, pp);
        }
    }

    cyg_httpd_write_chunked("|", 1); /* Must return something - <empty> is stack error! */
    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module Config JS lib routine                                            */
/****************************************************************************/

static size_t mstp_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[MSTP_WEB_BUF_LEN];

    (void)snprintf(buff, MSTP_WEB_BUF_LEN,
#ifdef VTSS_SW_OPT_MSTP_BPDU_ENH
                   "var configHasStpEnhancements = 1;\n"
#else
                   "var configHasStpEnhancements = 0;\n"
#endif /* VTSS_SW_OPT_MSTP_BPDU_ENH */
                  );

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  Config JS lib table entry                                               */
/****************************************************************************/
web_lib_config_js_tab_entry(mstp_lib_config_js);

#if !defined(VTSS_SW_OPT_MSTP_BPDU_ENH) || !defined(VTSS_MSTP_FULL)
/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t mstp_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    /* Hide HTML and text associated */
#if !defined(VTSS_SW_OPT_MSTP_BPDU_ENH) && !defined(VTSS_MSTP_FULL)
    return webCommonBufferHandler(base_ptr, cur_ptr, length, ".hasStpEnhancements { display: none; }\r\n.hasMstpFull { display: none; }\r\n");
#elif !defined(VTSS_SW_OPT_MSTP_BPDU_ENH)
    return webCommonBufferHandler(base_ptr, cur_ptr, length, ".hasStpEnhancements { display: none; }\r\n");
#else
    return webCommonBufferHandler(base_ptr, cur_ptr, length, ".hasMstpFull { display: none; }\r\n");
#endif
}
/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(mstp_lib_filter_css);
#endif /* VTSS_SW_OPT_MSTP_BPDU_ENH || VTSS_MSTP_FULL */

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_rstp_sys,          "/config/rstp_sys",          handler_config_rstp_sys);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_rstp_msti_map,     "/config/rstp_msti_map",     handler_config_rstp_msti_map);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_rstp_msti,         "/config/rstp_msti",         handler_config_rstp_msti);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_rstp_ports,        "/config/rstp_ports",        handler_config_rstp_ports);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_rstp_msti_ports,   "/config/rstp_msti_ports",   handler_config_rstp_msti_ports);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_rstp_statistics,     "/stat/rstp_statistics",     handler_stat_rstp_statistics);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_rstp_status_bridges, "/stat/rstp_status_bridges", handler_stat_rstp_status_bridges);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_rstp_status_bridge,  "/stat/rstp_status_bridge",  handler_stat_rstp_status_bridge);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_rstp_status_ports,   "/stat/rstp_status_ports",   handler_stat_rstp_status_ports);

