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

#include "web_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#include "msg_api.h"
#include "qos_api.h"
#include "port_trace.h"
#include <string>
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_web_api.h"
#endif

#define PORT_WEB_BUF_LEN 1024

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t port_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buf[PORT_WEB_BUF_LEN];
    (void) snprintf(buf, PORT_WEB_BUF_LEN,
                    "var configPortFrameSizeMin = %d;\n"
                    "var configPortFrameSizeMax = %d;\n",
                    MESA_MAX_FRAME_LENGTH_STANDARD,
                    fast_cap(MESA_CAP_PORT_FRAME_LENGTH_MAX));
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buf);
}

/****************************************************************************/
/*  JS lib_config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(port_lib_config_js);

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

#if VTSS_UI_OPT_VERIPHY == 1
static i32 handler_config_veriphy(CYG_HTTPD_STATE *p)
{
    vtss_isid_t              sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                      val;
    mesa_port_no_t           iport;
    vtss_uport_no_t          selected_uport = 0;
    bool                     is_post_request_in_disguise = false;
    int                      cnt;
    mepa_cable_diag_result_t veriphy_result;
    bool                     first = true;
    u32                      port_count = port_count_max();

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PING)) {
        return -1;
    }
#endif

    // This function does not support the HTTP POST method because
    // running VeriPHY on a 10/100 connection will cause the PHY to
    // link down, which in turn may cause the browser to not get any
    // reply to a HTTP POST request, causing it to timeout. This is
    // not the case with GET requests. If it's a request, the URL
    // has a "?port=<valid_port_number>". If it's a status get, the URL
    // has a "?port=-1" or no port at all.
    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/veriphy.htm");
        return -1;
    }

    if (cyg_httpd_form_varable_int(p, "port", &val)) {
        selected_uport = val;
        if (selected_uport == 0) {
            is_post_request_in_disguise = true;
        } else {
            if (uport2iport(selected_uport) < port_count) {
                is_post_request_in_disguise = true;
            }
        }
    }

    if (is_post_request_in_disguise) {
        CapArray<port_veriphy_mode_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> veriphy_mode;

        // Fill-in array telling which ports to run VeriPHY on.
        // Even when VeriPHY is already running on a given port, it's safe to
        // call port_veriphy_start() with PORT_VERIPHY_MODE_FULL again.
        for (iport = 0; iport < port_count; iport++) {
            meba_port_cap_t port_cap = 0;
            VTSS_RC_ERR_PRINT(port_cap_get(iport, &port_cap));
            if ((selected_uport == 0 || uport2iport(selected_uport) == iport) &&
                port_cap && (port_cap & MEBA_PORT_CAP_1G_PHY)) {
                veriphy_mode[iport] = PORT_VERIPHY_MODE_FULL;
            } else {
                veriphy_mode[iport] = PORT_VERIPHY_MODE_NONE;
            }
        }

        // Don't care about return code.
        (void)port_veriphy_start(veriphy_mode.data());
    }

    // Always reply.
    cyg_httpd_start_chunked("html");

    // Only data for ports with a PHY will be transferred to the browser.
    // Format of data:
    // port_1/veriphy_status_1/status_1_1/length_1_1/status_1_2/length_1_2/status_1_3/length_1_3/status_1_4/length_1_4;
    // port_2/veriphy_status_2/status_2_1/length_2_1/status_2_2/length_2_2/status_2_3/length_2_3/status_2_4/length_2_4;
    // ...
    // port_n/veriphy_status_n/status_n_1/length_n_1/status_n_2/length_n_2/status_n_3/length_n_3/status_n_4/length_n_4;
    // If veriphy_status_n is 0, VeriPHY has not been run in this port, and the remaining data is invalid.
    // If veriphy_status_n is 1, VeriPHY is in progress, and the remaining data is invalid.
    // If veriphy_status_n is 2, VeriPHY has been run on this port, and the remaining data is ok.
    for (iport = 0; iport < port_count; iport++) {
        meba_port_cap_t port_cap = 0;
        VTSS_RC_ERR_PRINT(port_cap_get(iport, &port_cap));
        if (port_cap && (port_cap & MEBA_PORT_CAP_1G_PHY)) {
            mesa_rc rc = port_veriphy_result_get(iport, &veriphy_result, 0);
            if (rc == VTSS_RC_OK || rc == VTSS_APPL_PORT_RC_VERIPHY_RUNNING || rc == VTSS_APPL_PORT_RC_GEN) {
                // Other port errors we don't bother to handle
                cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d/%u/%u/%u/%u/%u/%u/%u/%u/%u",
                               first ? "" : ";",
                               iport2uport(iport),
                               rc == VTSS_APPL_PORT_RC_GEN ? 0 : (rc == VTSS_APPL_PORT_RC_VERIPHY_RUNNING ? 1 : 2),
                               veriphy_result.status[0],
                               veriphy_result.length[0],
                               veriphy_result.status[1],
                               veriphy_result.length[1],
                               veriphy_result.status[2],
                               veriphy_result.length[2],
                               veriphy_result.status[3],
                               veriphy_result.length[3]);
                cyg_httpd_write_chunked(p->outbuffer, cnt);
                first = false;
            }
        }
    }

    cyg_httpd_end_chunked();
    return -1; // Do not further search the file system.
}
#endif /* VTSS_UI_OPT_VERIPHY == 1 */
static i32 handler_stat_ports(CYG_HTTPD_STATE *p)
{
    vtss_isid_t          sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_port_no_t       iport;
    mesa_port_counters_t counters;
    int                  ct;
    bool                 clear = (cyg_httpd_form_varable_find(p, "clear") != NULL);
    u32                  port_count;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    if (VTSS_ISID_LEGAL(sid) &&
        msg_switch_exists(sid)) {
        port_count = port_count_max() + fast_cap(MEBA_CAP_CPU_PORTS_COUNT);
        for (iport = 0; iport < port_count; iport++) {
            vtss_ifindex_t ifindex;
            if (vtss_ifindex_from_port(sid, iport, &ifindex) != VTSS_RC_OK) {
                T_E("Could not get ifindex");
            }

            if (clear) {     /* Clear? */
                if (vtss_appl_port_statistics_clear(ifindex) != VTSS_RC_OK) {
                    break;    /* Most likely stack error - bail out */
                }

                memset(&counters, 0, sizeof(counters)); /* Cheating a little... */
            } else {
                /* Normal read */
                if (vtss_appl_port_statistics_get(ifindex, &counters) != VTSS_RC_OK) {
                    break;    /* Most likely stack error - bail out */
                }
            }
            /* Output the counters */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"%s",
                          iport2uport(iport),
                          counters.rmon.rx_etherStatsPkts,
                          counters.rmon.tx_etherStatsPkts,
                          counters.rmon.rx_etherStatsOctets,
                          counters.rmon.tx_etherStatsOctets,
                          counters.if_group.ifInErrors,
                          counters.if_group.ifOutErrors,
                          counters.rmon.rx_etherStatsDropEvents,
                          counters.rmon.tx_etherStatsDropEvents,
                          counters.bridge.dot1dTpPortInDiscards,
                          iport == (port_count - 1) ? "" : "|");
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
    }

    cyg_httpd_end_chunked();
    return -1; // Do not further search the file system.
}

/* Support for a given feature is encoded like this:
   0: No supported
   1: Supported on other ports
   2: Supported on this port */
static u8 port_feature_support(meba_port_cap_t cap, meba_port_cap_t port_cap, meba_port_cap_t mask)
{
    return ((cap & mask) ? ((port_cap & mask) ? 2 : 1) : 0);
}

/*  bool pfc[8] = {1,1,1,0,0,0,0,1} to "0-2,7" */
static char *prio_arr_to_str(mesa_bool_t *pfc, char *buf)
{
    u8 i, a = 0, first = 1, b, tot = 0, once = 1;
    for (i = 0; i < 8; i++) {
        if (!pfc[i]) {
            continue;
        }

        if (first) {
            tot = a = + sprintf(buf + a, "%d", i);
            first = 0;
        } else {
            if (pfc[i]) {
                if (pfc[i - 1]) {
                    b = sprintf(buf + a, "-%d", i);
                    if (once) {
                        tot = tot + b;
                        once = 0;
                    }
                } else {
                    a = tot = tot + sprintf(buf + tot, ",%d", i);
                    once = 1;
                }
            }
        }
    }

    if (first) {
        *buf = '\0';
    }

    return buf;
}

/* "0-2,7" to bool pfc[8] = {1,1,1,0,0,0,0,1} */
static void prio_str_to_arr(char *buf, u32 len, mesa_bool_t *pfc)
{
    u8 c, a, indx, dash = 0;
    for (indx = 0; indx < 8; indx++) {
        pfc[indx] = 0;
    }

    for (c = 0; c < len; c++) {
        if (buf[c] == '-') {
            dash = 1;
        } else if (buf[c] == ',') {
            continue;
        } else {
            if (dash) {
                for (a = indx; a < (int)buf[c] - '0'; a++) {
                    pfc[a] = 1;
                }

                dash = 0;
            }

            indx = (int)buf[c] - '0';
            pfc[indx] = 1;
        }
    }
}

static i32 handler_config_ports(CYG_HTTPD_STATE *p)
{
    vtss_isid_t           sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_uport_no_t       uport;
    mesa_port_no_t        iport;
    vtss_appl_port_conf_t conf;
    int                   ct;
    uint32_t              port_count = port_count_max() + fast_cap(MEBA_CAP_CPU_PORTS_COUNT);
    uint32_t              i;
    char                  buf[80];

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PORT)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        mesa_rc rc = MESA_RC_ERROR;
        int     errors = 0;

        for (iport = 0; iport < port_count; iport++) {
            vtss_appl_port_conf_t newconf, default_conf;
            int                   enable, speed, fdx, max, val;
            const char            *str;
            bool                  adv_fdx, adv_hdx, supports_hdx;
            bool                  auto_10, auto_100, auto_1000, auto_2500, auto_5g, auto_10g;
            char                  buf2[80] = "";
            vtss_ifindex_t        ifindex;
            meba_port_cap_t       cap = 0;
            mepa_adv_dis_t        speed_adv_dis = MEPA_ADV_DIS_SPEED;

            uport = iport2uport(iport);

            if (vtss_ifindex_from_port(sid, iport, &ifindex) != VTSS_RC_OK) {
                T_E("Could not get ifindex");
            }

            if (vtss_appl_port_conf_get(ifindex, &conf) < 0) {
                T_E("Could not get appl_port_conf_get, sid = %d, iport =%u", sid, iport);
                errors++;   /* Probably stack error */
                continue;
            }

            if (vtss_appl_port_conf_default_get(ifindex, &default_conf) < 0) {
                T_E("Could not get vtss_appl_port_conf_default_get, sid = %d, iport =%u", sid, iport);
                errors++;
                continue;
            }

            VTSS_RC_ERR_PRINT(port_cap_get(iport, &cap));
            newconf = conf;
            if ((str = cyg_httpd_form_variable_str_fmt(p, NULL, "speed_%d", uport)) &&
                sscanf(str, "%uA%uA%u", &enable, &speed, &fdx) == 3) {
                T_IG_PORT(PORT_TRACE_GRP_CONF, iport, "enable:%d speed:%d fdx:%d", enable, speed, fdx);
                if ((newconf.admin.enable = enable)) {
                    newconf.speed = (mesa_port_speed_t)speed;
                    newconf.fdx = fdx;

                    if (newconf.speed != MESA_SPEED_AUTO) {
                        // newconf.force_clause_73 is a debug feature which is
                        // not supported in the Web GUI, so if it is already
                        // enabled (through CLI), we don't change it, unless a
                        // forced speed is getting applied, in which case we
                        // must disable it. So if new speed is not auto, we
                        // disable it, otherwise we retain its value.
                        newconf.force_clause_73 = false;
                    }

                    if (newconf.speed == MESA_SPEED_AUTO) {
                        auto_10   = cyg_httpd_form_variable_check_fmt(p, "adv_speed_10_%d",   uport);
                        auto_100  = cyg_httpd_form_variable_check_fmt(p, "adv_speed_100_%d",  uport);
                        auto_1000 = cyg_httpd_form_variable_check_fmt(p, "adv_speed_1000_%d", uport);
                        auto_2500 = cyg_httpd_form_variable_check_fmt(p, "adv_speed_2500_%d", uport);
                        auto_5g   = cyg_httpd_form_variable_check_fmt(p, "adv_speed_5g_%d",   uport);
                        auto_10g  = cyg_httpd_form_variable_check_fmt(p, "adv_speed_10g_%d",  uport);

                        if (auto_10) {
                            speed_adv_dis &= ~MEPA_ADV_DIS_10M;
                        }

                        if (auto_100) {
                            speed_adv_dis &= ~MEPA_ADV_DIS_100M;
                        }

                        if (auto_1000) {
                            speed_adv_dis &= ~MEPA_ADV_DIS_1G;
                        }

                        if (auto_2500) {
                            speed_adv_dis &= ~MEPA_ADV_DIS_2500M;
                        }

                        if (auto_5g) {
                            speed_adv_dis &= ~MEPA_ADV_DIS_5G;
                        }

                        if (auto_10g) {
                            speed_adv_dis &= ~MEPA_ADV_DIS_10G;
                        }

                        // First clear all speed- and duplex- bits.
                        newconf.adv_dis &= ~(MEPA_ADV_DIS_SPEED | MEPA_ADV_DIS_DUPLEX);

                        if (speed_adv_dis == MEPA_ADV_DIS_SPEED) {
                            // User hasn't specified any speeds to advertise.
                            // Use those from the default conf.
                            newconf.adv_dis |= (default_conf.adv_dis & MEPA_ADV_DIS_SPEED);
                        } else {
                            newconf.adv_dis |= speed_adv_dis;
                        }

                        adv_fdx = cyg_httpd_form_variable_check_fmt(p, "fdx_%d", uport);
                        adv_hdx = cyg_httpd_form_variable_check_fmt(p, "hdx_%d", uport);

                        newconf.adv_dis &= ~MEPA_ADV_DIS_FDX; // Clear the bit

                        // MEPA_ADV_DIS_HDX bit is set in default conf if the
                        // interface doesn't support half duplex. Also, this is
                        // only supported on pure copper ports.
                        supports_hdx = (default_conf.adv_dis & MEPA_ADV_DIS_HDX) == 0 && (cap & MEBA_PORT_CAP_COPPER) != 0;

                        if (supports_hdx && !adv_hdx) {
                            // Half duplex is supported on the interface. The
                            // duplex bits are already cleared. Set the HDX one.
                            newconf.adv_dis |= MEPA_ADV_DIS_HDX;
                        } else if (supports_hdx && !adv_fdx) {
                            // Same story as for adv_hdx.
                            newconf.adv_dis |= MEPA_ADV_DIS_FDX;
                        } else {
                            // Either the interface doesn't support half duplex
                            // or adv_hdx and adv_fdx are both set. Use the
                            // default advertise bits from the default
                            // configuration.
                            newconf.adv_dis |= (default_conf.adv_dis & MEPA_ADV_DIS_DUPLEX);
                        }
                    } else {
                        // Speed not auto. Nothing to advertise
                        // Set the adv_dis bits to default
                        newconf.adv_dis = default_conf.adv_dis;
                    }
                }

                T_IG_PORT(PORT_TRACE_GRP_CONF, iport, "conf speed: %d, duplex: %d, adv_dis: %d", newconf.speed, newconf.fdx, newconf.adv_dis);
            }

            // dual-media configuration
            // Applicable only for dual-media ports
            if (cyg_httpd_form_variable_int_fmt(p, &val, "dual_media_%d", uport)) {
                newconf.media_type = (vtss_appl_port_media_t) val;
            }

            /* flow_%d: CHECKBOX */
            newconf.flow_control = cyg_httpd_form_variable_check_fmt(p, "flow_%d", uport);

            /* max_%d: TEXT */
            if (cyg_httpd_form_variable_int_fmt(p, &max, "max_%d", uport) &&
                max >= MESA_MAX_FRAME_LENGTH_STANDARD &&
                max <= fast_cap(MESA_CAP_PORT_FRAME_LENGTH_MAX)) {
                newconf.max_length = max;
            }

            /* exc_%d: INT */
            if (cyg_httpd_form_variable_int_fmt(p, &val, "exc_%d", uport)) {
                newconf.exc_col_cont = val;
            }

            /* frame length check CHECKBOX */
            newconf.frame_length_chk = cyg_httpd_form_variable_check_fmt(p, "frm_len_chk_%d", uport);

            /* pfc checkbox and prio_range */
            if (cyg_httpd_form_variable_check_fmt(p, "pfc_ena_%d", uport)) {
                char pfc_range[15] = "";
                size_t len;
                const char *var_string;
                sprintf(pfc_range, "pfc_range_%d", uport);
                var_string = cyg_httpd_form_varable_string(p, pfc_range, &len);
                if (len > 0) {
                    (void) cgi_unescape(var_string, buf2, len, len);
                    prio_str_to_arr(buf2, len, newconf.pfc);
                }
            } else {
                for (i = 0; i < VTSS_PRIOS; i++) {
                    newconf.pfc[i] = 0;
                }
            }

            /* fec mode */
            if (cyg_httpd_form_variable_int_fmt(p, &val, "fec_mode_%d", uport)) {
                newconf.fec_mode = (vtss_appl_port_fec_mode_t)val;
            }

            if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                T_D("appl_port_conf_set(%u,%d)", sid, iport);
                if ((rc = vtss_appl_port_conf_set(ifindex, &newconf)) !=  VTSS_RC_OK) {
                    T_IG_PORT(PORT_TRACE_GRP_CONF, iport, "appl_port_conf_set() failed, rc %d rc: %s", rc, error_txt(rc));
                    T_D("Could not set appl_port_conf_set, sid = %d, iport =%u", sid, iport);
                    std::string buf("Port ");
                    buf.append(std::to_string(iport + 1));
                    buf.append(": ");
                    buf.append(error_txt(rc));
                    std::string msg(buf.length() * 3  + 1, '\0'); // cgi_escape may triple the number of characters
                    if (cgi_escape(buf.c_str(), &msg[0]) > 0) {
                        redirect_errmsg(p, "ports.htm", msg.c_str());
                    }
                    return -1;
                }
            }
        }

        T_D("errors = %d", errors);
        redirect(p, errors ? STACK_ERR_URL : "/ports.htm");
        return -1;
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        vtss_appl_port_capabilities_t cap;
        (void)vtss_appl_port_capabilities_get(&cap);
        for (iport = 0; iport < port_count; iport++) {
            vtss_appl_port_status_t port_status;
            port_vol_status_t       vol_status;
            bool                    rx_pause, tx_pause, fdx, hdx;
            bool                    auto_10, auto_100, auto_1000, auto_2500;
            bool                    auto_5g, auto_10g;
            bool                    phy_10g;
            char                    buf2[80] = "";
            char                    port_warnings_buf[400] = "";
            meba_port_cap_t         port_cap = 0;

            vtss_ifindex_t ifindex;
            if (vtss_ifindex_from_port(sid, iport, &ifindex) != VTSS_RC_OK) {
                T_E("Could not get ifindex");
            };

            vtss_appl_port_interface_capabilities_t if_caps = {};
            VTSS_RC_ERR_PRINT(vtss_appl_port_interface_capabilities_get(ifindex, &if_caps));
            port_cap = if_caps.static_caps != 0 ? if_caps.static_caps : if_caps.sfp_caps;
            //if_caps.has_kr is required later

            if (vtss_appl_port_conf_get(ifindex, &conf) != VTSS_RC_OK ||
                vtss_appl_port_status_get(ifindex, &port_status) != VTSS_RC_OK) {
                break;    /* Probably stack error - bail out */
            }

            if (port_status.link) {
                strcpy(buf, port_speed_duplex_to_txt_from_status(port_status));
            } else if (port_vol_status_get(PORT_USER_CNT, iport, &vol_status) == VTSS_RC_OK && (vol_status.conf.disable || vol_status.conf.disable_adm_recover)) {
                sprintf(buf, "Down (%s)", vol_status.name);
            } else {
                strcpy(buf, "Down");
            }

            rx_pause  = (conf.speed == MESA_SPEED_AUTO ? (port_status.link ? port_status.aneg.obey_pause : 0) : conf.flow_control);
            tx_pause  = (conf.speed == MESA_SPEED_AUTO ? (port_status.link ? port_status.aneg.generate_pause : 0) : conf.flow_control);
            fdx       = (conf.adv_dis & MEPA_ADV_DIS_FDX)   ? false : true;
            hdx       = (conf.adv_dis & MEPA_ADV_DIS_HDX)   ? false : true;
            auto_10   = (conf.adv_dis & MEPA_ADV_DIS_10M)   ? false : true;
            auto_100  = (conf.adv_dis & MEPA_ADV_DIS_100M)  ? false : true;
            auto_1000 = (conf.adv_dis & MEPA_ADV_DIS_1G)    ? false : true;
            auto_2500 = (conf.adv_dis & MEPA_ADV_DIS_2500M) ? false : true;
            auto_5g   = (conf.adv_dis & MEPA_ADV_DIS_5G)    ? false : true;
            auto_10g  = (conf.adv_dis & MEPA_ADV_DIS_10G)   ? false : true;
            phy_10g   = (iport < port_count_max()) ? vtss_phy_10G_is_valid(NULL, iport) : false;
            port_oper_warnings_to_txt(port_warnings_buf, sizeof(port_warnings_buf), port_status.oper_warnings);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "%u/" VPRI64u "/%u/%u/%u/%u/%u/%u/%u/%s/%s/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%d/%s/%u/%u/%u/%u/%u/%s/|",
                          iport2uport(iport),                                                                                  // 0  - iport
                          port_cap,                                                                                            // 1  - capabilities (static)
                          conf.admin.enable,                                                                                   // 2  - admin_state
                          conf.speed == MESA_SPEED_AUTO,                                                                       // 3  - autoneg
                          conf.speed,                                                                                          // 4  - speed
                          conf.fdx,                                                                                            // 5  - duplex
                          conf.max_length,                                                                                     // 6  - max. frame length
                          port_feature_support(cap.aggr_caps, port_cap, MEBA_PORT_CAP_FLOW_CTRL),                              // 7  - flow control support
                          conf.flow_control,                                                                                   // 8  - flow control state
                          port_status.link ? "Up" : "Down",                                                                    // 9  - link status
                          buf,                                                                                                 // 10 - speed status
                          rx_pause,                                                                                            // 11 - rx_pause
                          tx_pause,                                                                                            // 12 - tx_pause
                          port_feature_support(cap.aggr_caps, port_cap, MEBA_PORT_CAP_HDX),                                    // 13 - HDX support
                          conf.exc_col_cont,                                                                                   // 14 - Excessive collision support
                          fdx,                                                                                                 // 15 - advertise FDX
                          hdx,                                                                                                 // 16 - advertise HDX
                          auto_10,                                                                                             // 17 - advertise 10M
                          auto_100,                                                                                            // 18 - advertise 100M
                          auto_1000,                                                                                           // 19 - advertise 1G
                          auto_2500,                                                                                           // 20 - advertise 2.5G
                          auto_5g,                                                                                             // 21 - advertise 5G
                          auto_10g,                                                                                            // 22 - advertise 10G
                          phy_10g,                                                                                             // 23 - is PHY a VTSS 10G PHY?
                          cap.has_pfc,                                                                                         // 24 - priority flow control support
                          prio_arr_to_str(conf.pfc, buf2),                                                                     // 25 - priority flow control states
                          conf.frame_length_chk,                                                                               // 26 - frame_length_check
                          port_feature_support(cap.aggr_caps, port_cap, MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER), // 27 - dual-media support
                          conf.media_type,                                                                                     // 28 - media_type
                          if_caps.has_kr,                                                                                      // 29 - KR
                          conf.fec_mode,                                                                                       // 30 - FEC mode (valid if  if_caps.has_kr == true)
                          port_warnings_buf);                                                                                  // 31 - operational warnings text.
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

// handler_stat_port()
static i32 handler_stat_port(CYG_HTTPD_STATE *p)
{
    vtss_isid_t          sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_port_no_t       iport = 0;
    mesa_port_counters_t counters;
    int ct, val;
    uint32_t last_threshold;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    if (!VTSS_ISID_LEGAL(sid) ||
        !msg_switch_exists(sid)) {
        goto out;             /* Most likely stack error - bail out */
    }

    if (cyg_httpd_form_varable_int(p, "port", &val)) {
        iport = uport2iport(val);
    }

    if (iport > port_count_max()) {
        iport = 0;
    }

    vtss_ifindex_t ifindex;
    if (vtss_ifindex_from_port(sid, iport, &ifindex) != VTSS_RC_OK) {
        T_E("Could not get ifindex");
    };

    if ((cyg_httpd_form_varable_find(p, "clear") != NULL)) {         /* Clear? */
        if (vtss_appl_port_statistics_clear(ifindex) != VTSS_RC_OK) {

            goto out;         /* Most likely stack error - bail out */
        }

        memset(&counters, 0, sizeof(counters)); /* Cheating a little... */
    } else
        /* Normal read */
        if (vtss_appl_port_statistics_get(ifindex, &counters) != VTSS_RC_OK) {
            goto out;         /* Most likely stack error - bail out */
        }

    last_threshold = fast_cap(MESA_CAP_PORT_LAST_FRAME_LEN_THRESHOLD);

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u,%u,%d/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u,
                  iport2uport(iport),
                  port_count_max(),
                  last_threshold,
                  1,            /* STD counters */
                  /* 1 */ counters.rmon.rx_etherStatsPkts,
                  /* 2 */ counters.rmon.tx_etherStatsPkts,
                  /* 3 */ counters.rmon.rx_etherStatsOctets,
                  /* 4 */ counters.rmon.tx_etherStatsOctets,
                  /* 5 */ counters.rmon.rx_etherStatsDropEvents,
                  /* 6 */ counters.rmon.tx_etherStatsDropEvents,
                  /* 7 */ counters.if_group.ifInErrors,
                  /* 8 */ counters.if_group.ifOutErrors);
    cyg_httpd_write_chunked(p->outbuffer, ct);
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%d/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u,
                  2,            /* CAST counters */
                  /* 1 */ counters.if_group.ifInUcastPkts,
                  /* 2 */ counters.if_group.ifOutUcastPkts,
                  /* 3 */ counters.rmon.rx_etherStatsMulticastPkts,
                  /* 4 */ counters.rmon.tx_etherStatsMulticastPkts,
                  /* 5 */ counters.rmon.rx_etherStatsBroadcastPkts,
                  /* 6 */ counters.rmon.tx_etherStatsBroadcastPkts);
    cyg_httpd_write_chunked(p->outbuffer, ct);
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%d/" VPRI64u"/" VPRI64u,
                  3,            /* PAUSE counters */
                  /* 1 */ counters.ethernet_like.dot3InPauseFrames,
                  /* 2 */ counters.ethernet_like.dot3OutPauseFrames);
    cyg_httpd_write_chunked(p->outbuffer, ct);
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%d/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u,
                  4,            /* RMON ADV counters */
                  /* 1 */ counters.rmon.rx_etherStatsPkts64Octets,
                  /* 2 */ counters.rmon.tx_etherStatsPkts64Octets,
                  /* 3 */ counters.rmon.rx_etherStatsPkts65to127Octets,
                  /* 4 */ counters.rmon.tx_etherStatsPkts65to127Octets,
                  /* 5 */ counters.rmon.rx_etherStatsPkts128to255Octets,
                  /* 6 */ counters.rmon.tx_etherStatsPkts128to255Octets,
                  /* 7 */ counters.rmon.rx_etherStatsPkts256to511Octets,
                  /* 8 */ counters.rmon.tx_etherStatsPkts256to511Octets,
                  /* 9 */ counters.rmon.rx_etherStatsPkts512to1023Octets,
                  /* 10 */ counters.rmon.tx_etherStatsPkts512to1023Octets,
                  /* 11 */ counters.rmon.rx_etherStatsPkts1024to1518Octets,
                  /* 12 */ counters.rmon.tx_etherStatsPkts1024to1518Octets,
                  /* 13 */ counters.rmon.rx_etherStatsCRCAlignErrors,
                  /* 14 */ counters.if_group.ifOutErrors,
                  /* 15 */ counters.rmon.rx_etherStatsUndersizePkts,
                  /* 16 */ counters.rmon.rx_etherStatsOversizePkts,
                  /* 17 */ counters.rmon.rx_etherStatsFragments,
                  /* 18 */ counters.rmon.rx_etherStatsJabbers);
    cyg_httpd_write_chunked(p->outbuffer, ct);
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%d/" VPRI64u"/" VPRI64u,
                  5,            /* JUMBO counters */
                  /* 1 */ counters.rmon.rx_etherStatsPkts1519toMaxOctets,
                  /* 2 */ counters.rmon.tx_etherStatsPkts1519toMaxOctets);
    cyg_httpd_write_chunked(p->outbuffer, ct);

    if (fast_cap(MESA_CAP_PORT_CNT_ETHER_LIKE)) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%d/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u,
                      6,            /* ETHER_LIKE counters */
                      /* 1 */ counters.rmon.rx_etherStatsCRCAlignErrors,
                      /* 2 */ counters.ethernet_like.dot3StatsLateCollisions,
                      /* 3 */ counters.ethernet_like.dot3StatsSymbolErrors,
                      /* 4 */ counters.ethernet_like.dot3StatsExcessiveCollisions,
                      /* 5 */ counters.rmon.rx_etherStatsUndersizePkts,
                      /* 6 */ counters.ethernet_like.dot3StatsCarrierSenseErrors);
        cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%d/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u,
                  7,
                  counters.prio[0].rx,
                  counters.prio[0].tx,
                  counters.prio[1].rx,
                  counters.prio[1].tx,
                  counters.prio[2].rx,
                  counters.prio[2].tx,
                  counters.prio[3].rx,
                  counters.prio[3].tx,
                  counters.prio[4].rx,
                  counters.prio[4].tx,
                  counters.prio[5].rx,
                  counters.prio[5].tx,
                  counters.prio[6].rx,
                  counters.prio[6].tx,
                  counters.prio[7].rx,
                  counters.prio[7].tx);
    cyg_httpd_write_chunked(p->outbuffer, ct);

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%d/" VPRI64u,
                  8,
                  counters.bridge.dot1dTpPortInDiscards);
    cyg_httpd_write_chunked(p->outbuffer, ct);

#if defined(VTSS_SW_OPTION_QOS_ADV)
    if (fast_cap(MESA_CAP_QOS_FRAME_PREEMPTION)) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%d/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/%s/" VPRI64u"/%s",
                      9,
                      counters.dot3br.aMACMergeFragCountRx,
                      counters.dot3br.aMACMergeFragCountTx,
                      counters.dot3br.aMACMergeFrameAssOkCount,
                      counters.dot3br.aMACMergeHoldCount,
                      counters.dot3br.aMACMergeFrameAssErrorCount,
                      "",
                      counters.dot3br.aMACMergeFrameSmdErrorCount,
                      "");
        cyg_httpd_write_chunked(p->outbuffer, ct);
    }
#endif /* VTSS_SW_OPTION_QOS_ADV */

out:
    cyg_httpd_end_chunked();
    return -1; // Do not further search the file system.
}

/*
 * Toplevel portstate handler
 */
static i32 handler_stat_portstate(CYG_HTTPD_STATE *p)
{
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    switch (fast_cap(MESA_CAP_MISC_CHIP_FAMILY)) {
    case MESA_CHIP_FAMILY_CARACAL:
        stat_portstate_switch_lu26(p, VTSS_USID_START, VTSS_ISID_START);
        break;
    case MESA_CHIP_FAMILY_OCELOT:
        stat_portstate_switch_serval(p, VTSS_USID_START, VTSS_ISID_START);
        break;
    case MESA_CHIP_FAMILY_SERVALT:
    case MESA_CHIP_FAMILY_JAGUAR2:
        stat_portstate_switch_jr2(p, VTSS_USID_START, VTSS_ISID_START);
        break;
    case MESA_CHIP_FAMILY_SPARX5:
    case MESA_CHIP_FAMILY_LAN969X:
        stat_portstate_switch_sparx5(p);
        break;
    case MESA_CHIP_FAMILY_LAN966X:
        stat_portstate_switch_lan966x(p);
        break;

    default:
        T_E("Unsupported chip family: %d", fast_cap(MESA_CAP_MISC_CHIP_FAMILY));
        return -1;
    }

    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/*
 * Get supported port types
 */
static meba_port_cap_t PORT_WEB_type_supported(bool &has_rj45_port, bool &has_sfp_port, bool &has_x2_port, bool &has_power, bool &has_kr, bool &has_pfc)
{
    vtss_appl_port_capabilities_t           cap;
    vtss_appl_port_interface_capabilities_t if_caps;
    vtss_ifindex_t                          ifindex;
    uint32_t                                port_count = port_count_max();
    mesa_port_no_t                          port_no;

    (void)vtss_appl_port_capabilities_get(&cap);

    has_rj45_port = (cap.aggr_caps  & (MEBA_PORT_CAP_COPPER  | MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER | MEBA_PORT_CAP_DUAL_SFP_DETECT)) != 0;
    has_sfp_port  = (cap.aggr_caps  & (MEBA_PORT_CAP_FIBER   | MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER | MEBA_PORT_CAP_DUAL_SFP_DETECT | MEBA_PORT_CAP_SFP_DETECT  | MEBA_PORT_CAP_SFP_ONLY)) != 0;
    has_x2_port   = (cap.aggr_caps  & (MEBA_PORT_CAP_10G_FDX | MEBA_PORT_CAP_VTSS_10G_PHY)) != 0;
    has_power     = false;
    has_pfc       = cap.has_pfc;

    for (port_no = 0; port_no < port_count; port_no++) {
        // KR may be supported on the platform, but not on a single port on the
        // switch if not a single port has a 10G or 25G SFP port (but e.g. 10G
        // copper ports). So don't use caps.has_kr, because that tells whether
        // the chip supports it.
        if (vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_no, &ifindex) != VTSS_RC_OK) {
            continue;
        }

        if (vtss_appl_port_interface_capabilities_get(ifindex, &if_caps) != VTSS_RC_OK) {
            continue;
        }


        if ((has_kr = if_caps.has_kr)) {
            break;
        }
    }

#ifdef VTSS_SW_OPTION_PHY_POWER_CONTROL
    if (cap.aggr_caps & MEBA_PORT_CAP_1G_PHY) {
        has_power = true;
    }
#endif /* VTSS_SW_OPTION_PHY_POWER_CONTROL */

    return cap.aggr_caps;
}

/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t port_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char            buf[PORT_WEB_BUF_LEN];
    bool            has_rj45_port, has_sfp_port, has_x2_port, has_power, has_kr, has_pfc;
    meba_port_cap_t cap = PORT_WEB_type_supported(has_rj45_port, has_sfp_port, has_x2_port, has_power, has_kr, has_pfc);

    (void)snprintf(buf, sizeof(buf),
                   "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
                   has_kr                                                         ? "" : ".has_kr               {display: none;}\r\n",
                   has_pfc                                                        ? "" : ".has_pfc              {display: none;}\r\n",
                   has_power                                                      ? "" : ".PHY_PWR_CTRL         {display: none;}\r\n",
                   has_rj45_port                                                  ? "" : ".switch_has_rj45_port {display: none;}\r\n",
                   has_sfp_port                                                   ? "" : ".switch_has_sfp_port  {display: none;}\r\n",
                   has_x2_port                                                    ? "" : ".switch_has_x2_port   {display: none;}\r\n",
                   cap & MEBA_PORT_CAP_FLOW_CTRL                                  ? "" : ".PORT_FLOW_CTRL       {display: none;}\r\n",
                   cap & MEBA_PORT_CAP_HDX                                        ? "" : ".PORT_EXC_CONT        {display: none;}\r\n",
                   cap & MEBA_PORT_CAP_ANY_FIBER                                  ? "" : ".PORT_FIBER_CTRL      {display: none;}\r\n",
                   cap & MEBA_PORT_CAP_2_5G_FDX                                   ? "" : ".switch_has_2_5g_port {display: none;}\r\n",
                   cap & MEBA_PORT_CAP_5G_FDX                                     ? "" : ".switch_has_5g_port   {display: none;}\r\n",
                   cap & MEBA_PORT_CAP_10G_FDX                                    ? "" : ".has_10g              {display: none;}\r\n",
                   cap & MEBA_PORT_CAP_25G_FDX                                    ? "" : ".has_25g              {display: none;}\r\n",
                   (cap & MEBA_PORT_CAP_25G_FDX) == 0                             ? "" : ".does_not_have_25g    {display: none;}\r\n",
                   (cap & MEBA_PORT_CAP_10G_FDX) || (cap & MEBA_PORT_CAP_25G_FDX) ? "" : ".has_10g_or_25g       {display: none;}\r\n",
                   cap & (MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER)   ? "" : ".switch_has_dual_media {display:none;}\r\n");

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buf);
}

/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(port_lib_filter_css);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

#if VTSS_UI_OPT_VERIPHY == 1
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_veriphy, "/config/veriphy", handler_config_veriphy);
#endif /* VTSS_UI_OPT_VERIPHY == 1 */
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_ports, "/stat/ports", handler_stat_ports);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_ports, "/config/ports", handler_config_ports);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_port, "/stat/port", handler_stat_port);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_portstate, "/stat/portstate", handler_stat_portstate);

