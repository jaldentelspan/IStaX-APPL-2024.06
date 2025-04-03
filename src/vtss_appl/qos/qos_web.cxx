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
#include "qos_api.h"
#include "msg_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#include "topo_api.h"
#include "mgmt_api.h"

#define QOS_WEB_BUF_LEN 2000
#define CAPA vtss_appl_qos_capabilities

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static i32 handler_stat_qos_counter(CYG_HTTPD_STATE *p)
{
    vtss_isid_t          sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t          pit;
    mesa_port_counters_t counters;
    int                  ct;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT)) {
        return -1;
    }
#endif

    /*
    Format: [uport]/[low_queue_receive]/[normal_queue_transmit]/[normal_queue_receive]/[low_queue_transmit]/[medium_queue_receive]/[medium_queue_transmit]/[high_queue_receive]/[high_queue_transmit]|[uport]/[low_queue_receive]/[normal_queue_transmit]/[normal_queue_receive]/[low_queue_transmit]/[medium_queue_receive]/[medium_queue_transmit]/[high_queue_receive]/[high_queue_transmit]|...
    */
    cyg_httpd_start_chunked("html");

    if (VTSS_ISID_LEGAL(sid) && msg_switch_exists(sid)) {
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_ALL);
        while (port_iter_getnext(&pit)) {
            vtss_ifindex_t ifindex;
            if (vtss_ifindex_from_port(sid, pit.iport, &ifindex) != VTSS_RC_OK) {
                T_E("Could not get ifindex");
                break;
            };

            if (cyg_httpd_form_varable_find(p, "clear")) { /* Clear? */
                if (vtss_appl_port_statistics_clear(ifindex) != VTSS_RC_OK) {
                    T_W("Unable to clear counters for sid %u, port %u", sid, pit.uport);
                    break;
                }
                memset(&counters, 0, sizeof(counters)); /* Cheating a little... */
            } else {
                /* Normal read */
                if (vtss_appl_port_statistics_get(ifindex, &counters) != VTSS_RC_OK) {
                    T_W("Unable to get counters for sid %u, port %u", sid, pit.uport);
                    break;              /* Most likely stack error - bail out */
                }
            }
            /* Output the counters */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"/" VPRI64u"%s",
                          pit.uport,
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
                          counters.prio[7].tx,
                          pit.last ? "" : "|");
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
    }

    cyg_httpd_end_chunked();
    return -1; // Do not further search the file system.
}

static i32 handler_config_stormctrl(CYG_HTTPD_STATE *p)
{
    vtss_isid_t isid = web_retrieve_request_sid(p); /* Includes USID = ISID */

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        {
            vtss_appl_qos_conf_t conf;

            if (vtss_appl_qos_conf_get(&conf) != VTSS_RC_OK) {
                errors++;   /* Probably stack error */
            } else {
                mesa_packet_rate_t   rate;
                int                  frame_rate;
                vtss_appl_qos_conf_t newconf = conf;

                //unicast
                newconf.uc_policer.enable = cyg_httpd_form_variable_check_fmt(p, "%s", "gsp_enabled_0");
                if (cyg_httpd_form_variable_u32(p, "gsp_rate_0", &rate)) {
                    newconf.uc_policer.rate = rate;
                }
                if (cyg_httpd_form_varable_int(p, "gsp_frame_rate_0", &frame_rate)) {
                    newconf.uc_policer.frame_rate = !!frame_rate;
                }

                //multicast
                newconf.mc_policer.enable = cyg_httpd_form_variable_check_fmt(p, "%s", "gsp_enabled_1");
                if (cyg_httpd_form_variable_u32(p, "gsp_rate_1", &rate)) {
                    newconf.mc_policer.rate = rate;
                }
                if (cyg_httpd_form_varable_int(p, "gsp_frame_rate_1", &frame_rate)) {
                    newconf.mc_policer.frame_rate = !!frame_rate;
                }

                //broadcast
                newconf.bc_policer.enable = cyg_httpd_form_variable_check_fmt(p, "%s", "gsp_enabled_2");
                if (cyg_httpd_form_variable_u32(p, "gsp_rate_2", &rate)) {
                    newconf.bc_policer.rate = rate;
                }
                if (cyg_httpd_form_varable_int(p, "gsp_frame_rate_2", &frame_rate)) {
                    newconf.bc_policer.frame_rate = !!frame_rate;
                }

                if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                    mesa_rc rc;
                    if ((rc = vtss_appl_qos_conf_set(&newconf)) != VTSS_RC_OK) {
                        T_D("qos_conf_set: failed rc = %d", rc);
                        errors++; /* Probably stack error */
                    }
                }
            }
        }

        if (CAPA->has_port_storm_policers) {
            port_iter_t pit;

            (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                vtss_appl_qos_port_conf_t conf;
                mesa_rc                   rc;
                mesa_bitrate_t            rate;
                int                       frame_rate;

                if ((rc = vtss_appl_qos_port_conf_get(isid, pit.iport, &conf)) != VTSS_RC_OK) {
                    errors++; /* Probably stack error */
                    T_D("vtss_appl_qos_conf_get(%u, %d): failed rc = %d", isid, pit.uport, rc);
                    continue;
                }

                conf.uc_policer.enable = cyg_httpd_form_variable_check_fmt(p, "uc_enabled_%u", pit.iport);

                if (cyg_httpd_form_variable_u32_fmt(p, &rate, "uc_rate_%u", pit.iport)) {
                    conf.uc_policer.cir = rate;
                }

                if (cyg_httpd_form_variable_int_fmt(p, &frame_rate, "uc_frame_rate_%u", pit.iport)) {
                    conf.uc_policer.frame_rate = !!frame_rate;
                }

                conf.bc_policer.enable = cyg_httpd_form_variable_check_fmt(p, "bc_enabled_%u", pit.iport);

                if (cyg_httpd_form_variable_u32_fmt(p, &rate, "bc_rate_%u", pit.iport)) {
                    conf.bc_policer.cir = rate;
                }

                if (cyg_httpd_form_variable_int_fmt(p, &frame_rate, "bc_frame_rate_%u", pit.iport)) {
                    conf.bc_policer.frame_rate = !!frame_rate;
                }

                conf.un_policer.enable = cyg_httpd_form_variable_check_fmt(p, "un_enabled_%u", pit.iport);

                if (cyg_httpd_form_variable_u32_fmt(p, &rate, "un_rate_%u", pit.iport)) {
                    conf.un_policer.cir = rate;
                }

                if (cyg_httpd_form_variable_int_fmt(p, &frame_rate, "un_frame_rate_%u", pit.iport)) {
                    conf.un_policer.frame_rate = !!frame_rate;
                }

                if ((rc = vtss_appl_qos_port_conf_set(isid, pit.iport, &conf)) != VTSS_RC_OK) {
                    errors++; /* Probably stack error */
                    T_D("vtss_appl_qos_conf_set(%u, %d): failed rc = %d", isid, pit.uport, rc);
                }
            }
        }

        redirect(p, errors ? STACK_ERR_URL : "/stormctrl.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        /* Format:
         * <global_storm>#<port_storm>
         *
         *   global_storm   :== <unicast>/<multicast>/<broadcast>
         *     unicast      :== <enabled>|rate>|<frame_rate>
         *       enabled    :== 0..1           // 0: no, 1: yes
         *       rate       :== 0..0xffffffff  // actual bit or frame rate
         *       frame_rate :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
         *     multicast    :== <enabled>|<rate>|<frame_rate>
         *       enabled    :== 0..1           // 0: no, 1: yes
         *       rate       :== 0..0xffffffff  // actual bit or frame rate
         *       frame_rate :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
         *     broadcast    :== <enabled>|<rate>|<frame_rate>
         *       enabled    :== 0..1           // 0: no, 1: yes
         *       rate       :== 0..0xffffffff  // actual bit or frame rate
         *       frame_rate :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
         *
         * port_storm       :== <port 1>,<port 2>,<port 3>,...<port n>
         *
         *   port x         :== <port_no>/<unicast>/<broadcast>/<unknown>
         *     port_no      :== 1..max
         *     unicast      :== <enabled>|<rate>|<frame_rate>
         *       enabled    :== 0..1           // 0: no, 1: yes
         *       rate       :== 0..0xffffffff  // actual bit or frame rate
         *       frame_rate :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
         *     broadcast    :== <enabled>|<rate>|<frame_rate>
         *       enabled    :== 0..1           // 0: no, 1: yes
         *       rate       :== 0..0xffffffff  // actual bit or frame rate
         *       frame_rate :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
         *     unknown      :== <enabled>|<rate>|<frame_rate>
         *       enabled    :== 0..1           // 0: no, 1: yes
         *       rate       :== 0..0xffffffff  // actual bit or frame rate
         *       frame_rate :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
         */

        int ct;

        cyg_httpd_start_chunked("html");

        {
            vtss_appl_qos_conf_t conf;

            if (vtss_appl_qos_conf_get(&conf) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|%u|%d/%d|%u|%d/%d|%u|%d",
                              conf.uc_policer.enable,
                              conf.uc_policer.rate,
                              conf.uc_policer.frame_rate,
                              conf.mc_policer.enable,
                              conf.mc_policer.rate,
                              conf.mc_policer.frame_rate,
                              conf.bc_policer.enable,
                              conf.bc_policer.rate,
                              conf.bc_policer.frame_rate);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }

        cyg_httpd_write_chunked("#", 1);

        if (CAPA->has_port_storm_policers) {
            port_iter_t pit;

            (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                vtss_appl_qos_port_conf_t conf;

                if (vtss_appl_qos_port_conf_get(isid, pit.iport, &conf) != VTSS_RC_OK) {
                    break;          /* Probably stack error - bail out */
                }

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u/%u|%u|%u/%u|%u|%u/%u|%u|%u",
                              pit.first ? "" : ",",
                              pit.uport,
                              conf.uc_policer.enable,
                              conf.uc_policer.cir,
                              conf.uc_policer.frame_rate,
                              conf.bc_policer.enable,
                              conf.bc_policer.cir,
                              conf.bc_policer.frame_rate,
                              conf.un_policer.enable,
                              conf.un_policer.cir,
                              conf.un_policer.frame_rate);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static u8 qcl_qce_dmac_type(mesa_vcap_bit_t bc, mesa_vcap_bit_t mc)
{
    if (bc == MESA_VCAP_BIT_1) {
        return 3; // Broadcast
    } else if (mc == MESA_VCAP_BIT_1) {
        return 2; // Multicast
    } else if (mc == MESA_VCAP_BIT_0) {
        return 1; // Unicast
    } else {
        return 0; // Any
    }
}

static u8 qcl_qce_tag_type(mesa_vcap_bit_t tagged, mesa_vcap_bit_t s_tag)
{
    switch (tagged) {
    case MESA_VCAP_BIT_0:
        return 1; // Untagged
    case MESA_VCAP_BIT_1:
        switch (s_tag) {
        case MESA_VCAP_BIT_0:
            return 3; // C_Tagged
        case MESA_VCAP_BIT_1:
            return 4; // S_Tagged
        default:
            return 2; // Tagged
        }
    default:
        return 0; // Any
    }
}

static i32 handler_config_qcl_v2(CYG_HTTPD_STATE *p)
{
    mesa_qce_id_t                   qce_id;
    mesa_qce_id_t                   next_qce_id;
    int                             var_value1 = 0, var_value2 = 0, var_value3 = 0, var_value4 = 0;
    ulong                           var_ulong_value2 = 0;
    vtss_appl_qos_qce_intern_conf_t conf, conf_next;
    mesa_qce_t                      *const q = &conf.qce;
    mesa_qce_key_t                  *const k = &q->key;
    mesa_qce_action_t               *const a = &q->action;
    int                             i;
    const char                      *str;
    size_t                          len;
    vtss_isid_t                     isid;
    switch_iter_t                   sit;
    port_iter_t                     pit;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {
        BOOL add;

        // switch_id
        isid = VTSS_ISID_GLOBAL;

        // qce_id, next_qce_id
        qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
        if (cyg_httpd_form_varable_int(p, "qce_id", &var_value1)) {
            qce_id = var_value1;
        }
        add = (qce_id == VTSS_APPL_QOS_QCE_ID_NONE || vtss_appl_qos_qce_intern_get(VTSS_ISID_GLOBAL, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id, &conf, 0) != VTSS_RC_OK);
        next_qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
        if (cyg_httpd_form_varable_int(p, "next_qce_id", &var_value1)) {
            next_qce_id = var_value1;
        }
        if (!add && next_qce_id == VTSS_APPL_QOS_QCE_ID_NONE) {// get next entry when qce alreay exists & next_qce_id is none
            if (vtss_appl_qos_qce_intern_get(VTSS_ISID_GLOBAL, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id, &conf_next, 1) == VTSS_RC_OK) {
                next_qce_id = conf_next.qce.id;
            }
        }
        if (next_qce_id == VTSS_APPL_QOS_QCE_ID_END) { //if entry being edited is at last then set qce_next_id to none i.e to 0
            next_qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
        }

        // start with a default qce
        (void)vtss_appl_qos_qce_intern_get_default(&conf);
        q->id = qce_id;
        conf.isid = isid;

        // ports
        (void)port_iter_init_local_all(&pit);
        while (port_iter_getnext(&pit)) {
            k->port_list[pit.iport] = cyg_httpd_form_variable_check_fmt(p, "Port_%d", pit.uport);
        }

        // smac
        if (cyg_httpd_form_varable_int(p, "KeySMACSelect", &var_value1)) {
            uint mac[6];
            if (var_value1) { // specific
                if ((str = cyg_httpd_form_varable_string(p, "qceKeySMACValue", &len)) != NULL) {
                    if (sscanf(str, "%2x-%2x-%2x-%2x-%2x-%2x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6) {
                        for (i = 0; i < 6; i++) {
                            k->mac.smac.value[i] = mac[i];
                            k->mac.smac.mask[i] = 0xFF;
                        }
                    }
                }
            } else { // any
                memset(&k->mac.smac, 0, sizeof(k->mac.smac));
            }
        }

        // dmac
        if (cyg_httpd_form_varable_int(p, "KeyDMACSelect", &var_value1)) {
            mesa_mac_t mac;
            memset(&k->mac.dmac, 0, sizeof(k->mac.dmac));
            switch (var_value1) {
            case 1: // unicast
                k->mac.dmac_bc = MESA_VCAP_BIT_0;
                k->mac.dmac_mc = MESA_VCAP_BIT_0;
                break;
            case 2: // multicast
                k->mac.dmac_bc = MESA_VCAP_BIT_0;
                k->mac.dmac_mc = MESA_VCAP_BIT_1;
                break;
            case 3: // broadcast
                k->mac.dmac_bc = MESA_VCAP_BIT_1;
                k->mac.dmac_mc = MESA_VCAP_BIT_1;
                break;
            case 4: // specific
                if (CAPA->has_qce_dmac && cyg_httpd_form_variable_mac(p, "qceKeyDMACValue", &mac)) {
                    for (i = 0; i < 6; i++) {
                        k->mac.dmac.value[i] = mac.addr[i];
                        k->mac.dmac.mask[i] = 0xFF;
                    }
                }
            // Fall through
            default: // any
                k->mac.dmac_bc = MESA_VCAP_BIT_ANY;
                k->mac.dmac_mc = MESA_VCAP_BIT_ANY;
                break;
            }
        }

        // tag
        // tag type
        if (cyg_httpd_form_varable_int(p, "KeyTagSelect", &var_value1)) {
            switch (var_value1) {
            case 1: // untagged
                k->tag.tagged = MESA_VCAP_BIT_0;
                k->tag.s_tag = MESA_VCAP_BIT_ANY;
                break;
            case 2: // tagged
                k->tag.tagged = MESA_VCAP_BIT_1;
                k->tag.s_tag = MESA_VCAP_BIT_ANY;
                break;
            case 3: // c-tagged
                k->tag.tagged = MESA_VCAP_BIT_1;
                k->tag.s_tag = MESA_VCAP_BIT_0;
                break;
            case 4: // s-tagged
                k->tag.tagged = MESA_VCAP_BIT_1;
                k->tag.s_tag = MESA_VCAP_BIT_1;
                break;
            default: // any
                k->tag.tagged = MESA_VCAP_BIT_ANY;
                k->tag.s_tag = MESA_VCAP_BIT_ANY;
                break;
            }
        }

        // vid
        if (cyg_httpd_form_varable_int(p, "KeyVIDSelect", &var_value1)) {
            switch (var_value1) {
            case 1: // specific
                if (cyg_httpd_form_varable_int(p, "KeyVIDSpecific", &var_value2)) {
                    vtss_appl_qos_qcl_range_set(&k->tag.vid, var_value2, var_value2, 0xFFF);
                }
                break;
            case 2: // range
                if (cyg_httpd_form_varable_int(p, "KeyVidStart", &var_value2) &&
                    cyg_httpd_form_varable_int(p, "KeyVidLast", &var_value3)) {
                    vtss_appl_qos_qcl_range_set(&k->tag.vid, var_value2, var_value3, 0xFFF);
                }
                break;
            default: // any
                break;
            }
        }

        // pcp
        if (cyg_httpd_form_varable_int(p, "KeyPCPSelect", &var_value1)) {
            if (var_value1 == 0) { // any
                k->tag.pcp.mask = 0;
                k->tag.pcp.value = 0;
            } else if (var_value1 > 0 && var_value1 <= 8) {  // pcp values '0'..'7'
                k->tag.pcp.value = (var_value1 - 1);
                k->tag.pcp.mask = 7;
            } else if (var_value1 > 8 && var_value1 <= 12) { // pcp values '0-1'..'6-7'
                k->tag.pcp.value = (var_value1 - 9) * 2;
                k->tag.pcp.mask = 6;
            } else {                                         // pcp values '0-3'..'4-7'
                k->tag.pcp.value = (var_value1 == 13) ? 0 : 4;
                k->tag.pcp.mask = 4;
            }
        }

        // dei
        if (cyg_httpd_form_varable_int(p, "KeyDEISelect", &var_value1)) {
            k->tag.dei = (mesa_vcap_bit_t)var_value1;
        }

        if (CAPA->has_qce_inner_tag) {
            // inner tag
            // inner tag type
            if (cyg_httpd_form_varable_int(p, "KeyITagSelect", &var_value1)) {
                switch (var_value1) {
                case 1: // untagged
                    k->inner_tag.tagged = MESA_VCAP_BIT_0;
                    k->inner_tag.s_tag = MESA_VCAP_BIT_ANY;
                    break;
                case 2: // tagged
                    k->inner_tag.tagged = MESA_VCAP_BIT_1;
                    k->inner_tag.s_tag = MESA_VCAP_BIT_ANY;
                    break;
                case 3: // c-tagged
                    k->inner_tag.tagged = MESA_VCAP_BIT_1;
                    k->inner_tag.s_tag = MESA_VCAP_BIT_0;
                    break;
                case 4: // s-tagged
                    k->inner_tag.tagged = MESA_VCAP_BIT_1;
                    k->inner_tag.s_tag = MESA_VCAP_BIT_1;
                    break;
                default: // any
                    k->inner_tag.tagged = MESA_VCAP_BIT_ANY;
                    k->inner_tag.s_tag = MESA_VCAP_BIT_ANY;
                    break;
                }
            }

            // inner vid
            if (cyg_httpd_form_varable_int(p, "KeyIVIDSelect", &var_value1)) {
                switch (var_value1) {
                case 1: // specific
                    if (cyg_httpd_form_varable_int(p, "KeyIVIDSpecific", &var_value2)) {
                        vtss_appl_qos_qcl_range_set(&k->inner_tag.vid, var_value2, var_value2, 0xFFF);
                    }
                    break;
                case 2: // range
                    if (cyg_httpd_form_varable_int(p, "KeyIVidStart", &var_value2) &&
                        cyg_httpd_form_varable_int(p, "KeyIVidLast", &var_value3)) {
                        vtss_appl_qos_qcl_range_set(&k->inner_tag.vid, var_value2, var_value3, 0xFFF);
                    }
                    break;
                default: // any
                    break;
                }
            }

            // inner pcp
            if (cyg_httpd_form_varable_int(p, "KeyIPCPSelect", &var_value1)) {
                if (var_value1 == 0) { // any
                    k->inner_tag.pcp.mask = 0;
                    k->inner_tag.pcp.value = 0;
                } else if (var_value1 > 0 && var_value1 <= 8) {  // pcp values '0'..'7'
                    k->inner_tag.pcp.value = (var_value1 - 1);
                    k->inner_tag.pcp.mask = 7;
                } else if (var_value1 > 8 && var_value1 <= 12) { // pcp values '0-1'..'6-7'
                    k->inner_tag.pcp.value = (var_value1 - 9) * 2;
                    k->inner_tag.pcp.mask = 6;
                } else {                                         // pcp values '0-3'..'4-7'
                    k->inner_tag.pcp.value = (var_value1 == 13) ? 0 : 4;
                    k->inner_tag.pcp.mask = 4;
                }
            }

            // inner dei
            if (cyg_httpd_form_varable_int(p, "KeyIDEISelect", &var_value1)) {
                k->inner_tag.dei = (mesa_vcap_bit_t)var_value1;
            }
        }

        // frame type
        if (cyg_httpd_form_varable_int(p, "KeyFrameTypeSelect", &var_value1)) {
            switch (var_value1) {
            case 1:
                k->type = MESA_QCE_TYPE_ETYPE;
                break;
            case 2:
                k->type = MESA_QCE_TYPE_LLC;
                break;
            case 3:
                k->type = MESA_QCE_TYPE_SNAP;
                break;
            case 4:
                k->type = MESA_QCE_TYPE_IPV4;
                break;
            case 5:
                k->type = MESA_QCE_TYPE_IPV6;
                break;
            default:
                k->type = MESA_QCE_TYPE_ANY;
                break;
            }
        }

        switch (k->type) {
        case MESA_QCE_TYPE_ETYPE: {
            mesa_qce_frame_etype_t *etype = &k->frame.etype;
            if (cyg_httpd_form_varable_int(p, "EtypeSelect", &var_value1)) {
                if (var_value1) { // specific
                    if (cyg_httpd_form_varable_hex(p, "EtypeVal", &var_ulong_value2)) {
                        etype->etype.value[0] = (var_ulong_value2 >> 8) & 0xFF;
                        etype->etype.value[1] = var_ulong_value2 & 0xFF;
                        etype->etype.mask[0] = etype->etype.mask[1] = 0xFF;
                    }
                }
            }
            break;
        }
        case MESA_QCE_TYPE_LLC: {
            mesa_qce_frame_llc_t *llc = &k->frame.llc;
            if (cyg_httpd_form_varable_int(p, "LLCDSAPSelect", &var_value1)) {
                if (var_value1) { // specific
                    if (cyg_httpd_form_varable_hex(p, "LLCDSAPVal", &var_ulong_value2)) {
                        llc->data.value[0] = var_ulong_value2 & 0xFF;
                        llc->data.mask[0]  = 0xFF;
                    }
                }
            }
            if (cyg_httpd_form_varable_int(p, "LLCSSAPSelect", &var_value1)) {
                if (var_value1) { // specific
                    if (cyg_httpd_form_varable_hex(p, "LLCSSAPVal", &var_ulong_value2)) {
                        llc->data.value[1] = var_ulong_value2 & 0xFF;
                        llc->data.mask[1]  = 0xFF;
                    }
                }
            }
            if (cyg_httpd_form_varable_int(p, "LLCCntrlSelect", &var_value1)) {
                if (var_value1) { // specific
                    if (cyg_httpd_form_varable_hex(p, "LLCCntrlVal", &var_ulong_value2)) {
                        llc->data.value[2] = var_ulong_value2 & 0xFF;
                        llc->data.mask[2]  = 0xFF;
                    }
                }
            }
            break;
        }
        case MESA_QCE_TYPE_SNAP: {
            mesa_qce_frame_snap_t *snap = &k->frame.snap;
            if (cyg_httpd_form_varable_int(p, "SNAPSelect", &var_value1)) {
                if (var_value1) { // specific
                    if (cyg_httpd_form_varable_hex(p, "SNAPVal", &var_ulong_value2)) {
                        snap->data.value[3] = (var_ulong_value2 >> 8) & 0xFF;
                        snap->data.mask[3]  = 0xFF;
                        snap->data.value[4] = var_ulong_value2 & 0xFF;
                        snap->data.mask[4]  = 0xFF;
                    }
                }
            }
            break;
        }
        case MESA_QCE_TYPE_IPV4: {
            mesa_qce_frame_ipv4_t *i4 = &k->frame.ipv4;

            // protocol
            if (cyg_httpd_form_varable_int(p, "IPv4ProtoSelect", &var_value1)) {
                switch (var_value1) {
                case 1: // udp
                case 2: // tcp
                    if (var_value1 == 1) {
                        i4->proto.value = 17; // udp protocol number
                    } else {
                        i4->proto.value = 6; // tcp protocol number
                    }
                    i4->proto.mask = 0xFF; // specific

                    // source port
                    if (cyg_httpd_form_varable_int(p, "KeySportSelect", &var_value2)) {
                        switch (var_value2) {
                        case 1: // specific
                            if (cyg_httpd_form_varable_int(p, "keySportSpecValue", &var_value3)) {
                                vtss_appl_qos_qcl_range_set(&i4->sport, var_value3, var_value3, 0xFFFF);
                            }
                            break;
                        case 2: // range
                            if (cyg_httpd_form_varable_int(p, "keySportStart", &var_value3) &&
                                cyg_httpd_form_varable_int(p, "keySportLast", &var_value4)) {
                                vtss_appl_qos_qcl_range_set(&i4->sport, var_value3, var_value4, 0xFFFF);
                            }
                            break;
                        default: // any
                            break;
                        }
                    }

                    // destination port
                    if (cyg_httpd_form_varable_int(p, "KeyDportSelect", &var_value2)) {
                        switch (var_value2) {
                        case 1: // specific
                            if (cyg_httpd_form_varable_int(p, "keyDportSpecValue", &var_value3)) {
                                vtss_appl_qos_qcl_range_set(&i4->dport, var_value3, var_value3, 0xFFFF);
                            }
                            break;
                        case 2: // range
                            if (cyg_httpd_form_varable_int(p, "keyDportStart", &var_value3) &&
                                cyg_httpd_form_varable_int(p, "keyDportLast", &var_value4)) {
                                vtss_appl_qos_qcl_range_set(&i4->dport, var_value3, var_value4, 0xFFFF);
                            }
                            break;
                        default: // any
                            break;
                        }
                    }
                    break;
                case 3: // other
                    if (cyg_httpd_form_varable_int(p, "KeyProtoNbr", &var_value2)) {
                        i4->proto.value = var_value2; // other protocol number
                        i4->proto.mask = 0xFF;
                    }
                    break;
                default: // any
                    break;
                }

            }

            // ipv4 sip
            if (cyg_httpd_form_varable_int(p, "IPv4IPAddrSelect", &var_value1)) {
                if (var_value1) { // specific
                    if (cyg_httpd_form_varable_ipv4(p, "IPv4Addr", &i4->sip.value)) {
                        (void)cyg_httpd_form_varable_ipv4(p, "IPMaskValue", &i4->sip.mask);
                    }
                }
            }

            // ipv4 dip
            if (CAPA->has_qce_dip && cyg_httpd_form_varable_int(p, "IPv4DIPAddrSelect", &var_value1)) {
                if (var_value1) { // specific
                    if (cyg_httpd_form_varable_ipv4(p, "IPv4DAddr", &i4->dip.value)) {
                        (void)cyg_httpd_form_varable_ipv4(p, "DIPMaskValue", &i4->dip.mask);
                    }
                }
            }

            //set ipv4 fragment (Any:0, Yes:1, No:2) - name:value
            if (cyg_httpd_form_varable_int(p, "IPv4IPfragMenu", &var_value1)) {
                switch (var_value1) {
                case 1: // yes
                    i4->fragment = MESA_VCAP_BIT_1;
                    break;
                case 2: // no
                    i4->fragment = MESA_VCAP_BIT_0;
                    break;
                default: // any
                    break;
                }
            }

            // dscp
            if (cyg_httpd_form_varable_int(p, "IPv4DSCPSelect", &var_value1)) {
                switch (var_value1) {
                case 1: // specific
                    if (cyg_httpd_form_varable_int(p, "keyDSCPSpcValue", &var_value2)) {
                        vtss_appl_qos_qcl_range_set(&i4->dscp, var_value2, var_value2, 0x3F);
                    }
                    break;
                case 2: // range
                    if (cyg_httpd_form_varable_int(p, "keyDSCPRngStart", &var_value2) &&
                        cyg_httpd_form_varable_int(p, "keyDSCPRngLast", &var_value3)) {
                        vtss_appl_qos_qcl_range_set(&i4->dscp, var_value2, var_value3, 0x3F);
                    }
                    break;
                default: // any
                    break;
                }
            }
            break;
        }
        case MESA_QCE_TYPE_IPV6: {
            mesa_qce_frame_ipv6_t *i6 = &k->frame.ipv6;

            // protocol
            if (cyg_httpd_form_varable_int(p, "IPv6ProtoSelect", &var_value1)) {
                switch (var_value1) {
                case 1: // udp
                case 2: // tcp
                    if (var_value1 == 1) {
                        i6->proto.value = 17; // udp protocol number
                    } else {
                        i6->proto.value = 6; // tcp protocol number
                    }
                    i6->proto.mask = 0xFF; // specific

                    // source port
                    if (cyg_httpd_form_varable_int(p, "KeySportSelect", &var_value2)) {
                        switch (var_value2) {
                        case 1: // specific
                            if (cyg_httpd_form_varable_int(p, "keySportSpecValue", &var_value3)) {
                                vtss_appl_qos_qcl_range_set(&i6->sport, var_value3, var_value3, 0xFFFF);
                            }
                            break;
                        case 2: // range
                            if (cyg_httpd_form_varable_int(p, "keySportStart", &var_value3) &&
                                cyg_httpd_form_varable_int(p, "keySportLast", &var_value4)) {
                                vtss_appl_qos_qcl_range_set(&i6->sport, var_value3, var_value4, 0xFFFF);
                            }
                            break;
                        default: // any
                            break;
                        }
                    }

                    // destination port
                    if (cyg_httpd_form_varable_int(p, "KeyDportSelect", &var_value2)) {
                        switch (var_value2) {
                        case 1: // specific
                            if (cyg_httpd_form_varable_int(p, "keyDportSpecValue", &var_value3)) {
                                vtss_appl_qos_qcl_range_set(&i6->dport, var_value3, var_value3, 0xFFFF);
                            }
                            break;
                        case 2: // range
                            if (cyg_httpd_form_varable_int(p, "keyDportStart", &var_value3) &&
                                cyg_httpd_form_varable_int(p, "keyDportLast", &var_value4)) {
                                vtss_appl_qos_qcl_range_set(&i6->dport, var_value3, var_value4, 0xFFFF);
                            }
                            break;
                        default: // any
                            break;
                        }
                    }
                    break;
                case 3: // other
                    if (cyg_httpd_form_varable_int(p, "KeyProtoNbr", &var_value2)) {
                        i6->proto.value = var_value2; // other protocol number
                        i6->proto.mask = 0xFF;
                    }
                    break;
                default: // any
                    break;
                }

            }

            // ipv6 sip
            if (cyg_httpd_form_varable_int(p, "IPv6IPAddrSelect", &var_value1)) {
                if (var_value1) { // specific
                    mesa_vcap_ip_t ipv4;
                    if (cyg_httpd_form_varable_ipv4(p, "IPv6Addr", &ipv4.value)) {
                        (void)cyg_httpd_form_varable_ipv4(p, "IPMaskValue", &ipv4.mask);
                        vtss_appl_qos_qcl_ipv42ipv6(&ipv4, &i6->sip);
                    }
                }
            }

            // ipv6 dip
            if (CAPA->has_qce_dip && cyg_httpd_form_varable_int(p, "IPv6DIPAddrSelect", &var_value1)) {
                if (var_value1) { // specific
                    mesa_vcap_ip_t ipv4;
                    if (cyg_httpd_form_varable_ipv4(p, "IPv6DAddr", &ipv4.value)) {
                        (void)cyg_httpd_form_varable_ipv4(p, "DIPMaskValue", &ipv4.mask);
                        vtss_appl_qos_qcl_ipv42ipv6(&ipv4, &i6->dip);
                    }
                }
            }

            // dscp
            if (cyg_httpd_form_varable_int(p, "IPv6DSCPSelect", &var_value1)) {
                switch (var_value1) {
                case 1: // specific
                    if (cyg_httpd_form_varable_int(p, "keyDSCPSpcValue", &var_value2)) {
                        vtss_appl_qos_qcl_range_set(&i6->dscp, var_value2, var_value2, 0x3F);
                    }
                    break;
                case 2: // range
                    if (cyg_httpd_form_varable_int(p, "keyDSCPRngStart", &var_value2) &&
                        cyg_httpd_form_varable_int(p, "keyDSCPRngLast", &var_value3)) {
                        vtss_appl_qos_qcl_range_set(&i6->dscp, var_value2, var_value3, 0x3F);
                    }
                    break;
                default: // any
                    break;
                }
            }
            break;
        }
        default:
            // any
            break;
        } // k->type switch case block

        // action
        // cos
        if (cyg_httpd_form_varable_int(p, "actionCoSSel",  &var_value1)) {
            if (var_value1) {
                a->prio = var_value1 - 1;
                a->prio_enable = TRUE;
            }
        }
        // dp
        if (cyg_httpd_form_varable_int(p, "actionDPSel",  &var_value1)) {
            if (var_value1) {
                a->dp = var_value1 - 1;
                a->dp_enable = TRUE;
            }
        }
        // dscp
        if (cyg_httpd_form_varable_int(p, "actionDSCPSel", &var_value1)) {
            if (var_value1) {
                a->dscp = var_value1 - 1;
                a->dscp_enable = TRUE;
            }
        }
        // pcp
        if (cyg_httpd_form_varable_int(p, "actionPCPSel",  &var_value1)) {
            if (var_value1) {
                a->pcp = var_value1 - 1;
                a->pcp_dei_enable = TRUE ;
            }
        }
        // dei
        if (cyg_httpd_form_varable_int(p, "actionDEISel",  &var_value1)) {
            if (var_value1) {
                a->dei = var_value1 - 1;
                a->pcp_dei_enable = TRUE ;
            }
        }
        // policy
        if (cyg_httpd_form_varable_int(p, "actionPolicy",  &var_value1)) {
            a->policy_no = var_value1;
            a->policy_no_enable = TRUE;
        }
        // ingress map id
        if (CAPA->has_ingress_map && cyg_httpd_form_varable_int(p, "ingressMapId", &var_value1)) {
            a->map_id = (mesa_qos_ingress_map_id_t) var_value1;
            a->map_id_enable = TRUE;
        }

        // add the qce entry finally
        if (vtss_appl_qos_qce_intern_add(next_qce_id, &conf) == VTSS_RC_OK) {
            T_D("QCE ID %d %s ", conf.qce.id, add ? "added" : "modified");
            if (next_qce_id) {
                T_D("before QCE ID %d\n", next_qce_id);
            } else {
                T_D("last\n");
            }
            redirect(p, "/qcl_v2.htm");
        } else {
            T_E("QCL Add failed\n");
            redirect(p, "/qcl_v2.htm?error=1");
        }
    } else { /* CYG_HTTPD_METHOD_GET (+HEAD) */
        int  qce_flag = 0;
        char buf[MGMT_PORT_BUF_SIZE];
        char str_buff1[24], str_buff2[24];
        char str_buff3[24], str_buff4[24];
        BOOL first;
        int  ct;

        /*
           Format:
           <Del>         : qceConfigFlag=1 and qce_id=<qce_id to delete>
           <Move>        : qceConfigFlag=2 and qce_id=<qce_id to move>
           <Edit>        : qceConfigFlag=3 and qce_id=<qce_id to edit>
           <Insert>      : qceConfigFlag=4 and qce_id=<next_qce_id to insert before>
           <Add to Last> : qceConfigFlag=4 and qce_id=0 (inserts last if next_qce_id=0)
        */
        cyg_httpd_start_chunked("html");
        isid = VTSS_ISID_GLOBAL;
        if (cyg_httpd_form_varable_int(p, "qceConfigFlag",  &var_value1)) {
            qce_flag = var_value1;
            qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
            if (cyg_httpd_form_varable_int(p, "qce_id",  &var_value1)) {
                qce_id = var_value1;
            }

            switch (qce_flag) {
            case 1://<Del> qce entry
                (void)vtss_appl_qos_qce_intern_del(isid, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id);
                break;
            case 2://<Move> qce entry
                if (qce_id != VTSS_APPL_QOS_QCE_ID_NONE) {
                    if (vtss_appl_qos_qce_intern_get(isid, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id, &conf, 0) == VTSS_RC_OK) {
                        if (vtss_appl_qos_qce_intern_get(isid, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id, &conf_next, 1) == VTSS_RC_OK) {
                            if (vtss_appl_qos_qce_intern_add(qce_id, &conf_next) != VTSS_RC_OK) {
                                T_W("vtss_appl_qos_qce_intern_add() failed");
                            }
                        }
                    } else {
                        T_W ("vtss_appl_qos_qce_intern_get() failed");
                    }
                }
                break;
            case 3://<Edit> This format is for 'qos/html/qcl_v2_edit.htm' webpage
            case 4://<Insert> or <Add to Last> This format is for 'qos/html/qcl_v2_edit.htm' webpage
                // Create a list of present switches in usid order separated by "#"
                (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID);
                while (switch_iter_getnext(&sit)) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d", sit.first ? "" : "#", sit.usid);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
                if (qce_flag == 4) {
                    cyg_httpd_write_chunked(",-1", 3);
                    cyg_httpd_end_chunked();
                    return -1; // return from <Insert> or <Add to Last> case
                }
                /* Format:
                 * <qce_info>;<frame_info>
                 *
                 * qce_info        :== <sid_next>/<port_list>/<tag>/<vid>/<pcp>/<dei>/<itag>/<ivid>/<ipcp>/<idei>/<smac>/<dmac_type>/<dmac>/<act_cos>/<act_dpl>/<act_dscp>/<act_pcp>/<act_dei>/<act_policy>
                 *   sid_next      :== <usid_list>,<usid>,<next_qce_id>
                 *     usid_list   :== <usid l>#<usid m>#<usid n> // List of present (active) switches in usid order.
                 *     usid        :== 1..16 or -1 for all switches
                 *     next_qce_id :== 0..256
                 *   port_list     :== list of enabled uports separated by ','.
                 *   tag           :== 0: Any, 1: Untagged, 2: Tagged, 3: C-Tagged, 4: S-Tagged
                 *   vid           :== <vid_low>,<vid_high>
                 *     vid_low     :== -1 or 1..4095  // -1: Any
                 *     vid_high    :== -1 or 1..4095  // -1: Use vid_low as specific, else high range.
                 *   pcp           :== <pcp_low>,<pcp_high>
                 *     pcp_low     :== -1..7   // -1 is Any
                 *     pcp_high    :== -1..7    // Not used if pcp_low == -1. If pcp_low != pcp_high then it is a range, else use pcp_low.
                 *   dei           :== -1: Any , else dei value
                 *   itag          :== 0: Any, 1: Untagged, 2: Tagged, 3: C-Tagged, 4: S-Tagged
                 *   ivid          :== <vid_low>,<vid_high>
                 *     vid_low     :== -1 or 1..4095  // -1: Any
                 *     vid_high    :== -1 or 1..4095  // -1: Use vid_low as specific, else high range.
                 *   ipcp          :== <pcp_low>,<pcp_high>
                 *     pcp_low     :== -1..7   // -1 is Any
                 *     pcp_high    :== -1..7    // Not used if pcp_low == -1. If pcp_low != pcp_high then it is a range, else use pcp_low.
                 *   idei          :== -1: Any , else dei value
                 *   smac          :== String  // "Any" or "xx-xx-xx-xx-xx-xx"
                 *   dmac_type     :== 0..3    // One of qos_qce_dmac_type_t
                 *   dmac          :== String  // "" (use dmac_type) or "xx-xx-xx-xx-xx-xx"
                 *   act_cos       :== -1..7   // -1 is no action, else classify to selected value
                 *   act_dpl       :== -1..3   // -1 is no action, else classify to selected value
                 *   act_dscp      :== -1..63  // -1 is no action, else classify to selected value
                 *   act_pcp       :== -1..7   // -1 is no action, else classify to selected value
                 *   act_dei       :== -1..3   // -1 is no action, else classify to selected value
                 *   act_policy    :== -1..63  // -1 is no action, else classify to selected value
                 *   act_map_id    :== configQosIngressMapMin..configQosIngressMapMax
                 *
                 * frame_info      :== <frame_type>/<type_any> or <type_eth> or <type_llc> or <type_snap> or <type_ipv4> or <type_ipv6>
                 *   frame_type    :== 0..5    // One of mesa_qce_type_t
                 *   type_any      :== String "Any"
                 *   type_eth      :== <eth_spec>/<eth_val>
                 *     eth_spec    :== 0 or 1 where 0 is Any and 1 is use eth_val
                 *     eth_val     :== 0 or 0600..ffff - value in hex (without 0x prepended)
                 *   type_llc      :== <ssap_spec>/<dsap_spec>/<ctrl_spec>/<ssap_val>/<dsap_val>/<ctrl_val>
                 *     dsap_spec   :== -1 for Any, else value in decimal
                 *     ssap_spec   :== -1 for Any, else value in decimal
                 *     ctrl_spec   :== -1 for Any, else value in decimal
                 *     dsap_val    :== 0 or value in hex (without 0x prepended)
                 *     ssap_val    :== 0 or value in hex (without 0x prepended)
                 *     ctrl_val    :== 0 or value in hex (without 0x prepended)
                 *   type_snap     :== <snap_spec>/<snap_hbyte>/<snap_lbyte>
                 *     snap_spec   :== 0 or 1 where 0 is Any and 1 is use snap_hbyte and snap_lbyte
                 *     snap_hbyte  :== 00..ff - value in hex (without 0x prepended)
                 *     snap_lbyte  :== 00..ff - value in hex (without 0x prepended)
                 *   type_ipv4     :== <proto>/<sip>/<dip>/<ip_frag>/<dscp_low>/<dscp_high>[/<sport_low>/<sport_high>/<dport_low>/<dport_high>]
                 *     proto       :== -1 for Any, else value in decimal
                 *     sip         :== <ip>,<mask>
                 *       ip        :== String // "Any" or "x.y.z.w"
                 *       mask      :== String // "x.y.z.w"
                 *     dip         :== <ip>,<mask>
                 *       ip        :== String // "Any" or "x.y.z.w"
                 *       mask      :== String // "x.y.z.w"
                 *     ip_frag     :== 0: Any, 1: No, 2: Yes
                 *     dscp_low    :== -1..63 // -1 if Any
                 *     dscp_high   :== -1..63 // -1: Use dscp_low as specific, else high range.
                 *     sport_low   :== -1..65535 // -1 if Any                                         Only present if proto is TCP or UDP!
                 *     sport_high  :== -1..65535 // -1: Use sport_low as specific, else high range.   Only present if proto is TCP or UDP!
                 *     dport_low   :== -1..65535 // -1 if Any                                         Only present if proto is TCP or UDP!
                 *     dport_high  :== -1..65535 // -1: Use dport_low as specific, else high range.   Only present if proto is TCP or UDP!
                 *   type_ipv6     :== <proto>/<sip>/<dip>/<dscp_low>/<dscp_high>[/<sport_low>/<sport_high>/<dport_low>/<dport_high>]
                 *     proto       :== -1 for Any, else value in decimal
                 *     sip         :== <ip>,<mask>
                 *       ip        :== String // "Any" or "x.y.z.w"
                 *       mask      :== String // "x.y.z.w"
                 *     dip         :== <ip>,<mask>
                 *       ip        :== String // "Any" or "x.y.z.w"
                 *       mask      :== String // "x.y.z.w"
                 *     dscp_low    :== -1..63 // -1 if Any
                 *     dscp_high   :== -1..63 // -1: Use dscp_low as specific, else high range.
                 *     sport_low   :== -1..65535 // -1 if Any                                         Only present if proto is TCP or UDP!
                 *     sport_high  :== -1..65535 // -1: Use sport_low as specific, else high range.   Only present if proto is TCP or UDP!
                 *     dport_low   :== -1..65535 // -1 if Any                                         Only present if proto is TCP or UDP!
                 *     dport_high  :== -1..65535 // -1: Use dport_low as specific, else high range.   Only present if proto is TCP or UDP!
                 */
                cyg_httpd_write_chunked(",", 1);

                if (vtss_appl_qos_qce_intern_get(isid, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id, &conf, 0) == VTSS_RC_OK) {
                    int vid_low, vid_high, pcp_low, pcp_high;
                    u8  pcp_mask;

                    next_qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
                    if (vtss_appl_qos_qce_intern_get(isid, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id, &conf_next, 1) == VTSS_RC_OK) {
                        next_qce_id = conf_next.qce.id;
                    }
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d,%u/",
                                  (conf.isid == VTSS_ISID_GLOBAL) ? -1 : (int)topo_isid2usid(conf.isid),
                                  next_qce_id);
                    cyg_httpd_write_chunked(p->outbuffer, ct);

                    first = TRUE;
                    (void)port_iter_init_local_all(&pit);
                    while (port_iter_getnext(&pit)) {
                        if (k->port_list[pit.iport]) {
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), (first) ? "%u" : ",%u", pit.uport);
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                            first = FALSE;
                        }
                    }

                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", qcl_qce_tag_type(k->tag.tagged, k->tag.s_tag));
                    cyg_httpd_write_chunked(p->outbuffer, ct);

                    if (k->tag.vid.type  != MESA_VCAP_VR_TYPE_VALUE_MASK) {
                        vid_low  = k->tag.vid.vr.r.low;
                        vid_high = k->tag.vid.vr.r.high;
                    } else if (k->tag.vid.vr.v.mask) {
                        vid_low  = k->tag.vid.vr.v.value;
                        vid_high = -1;
                    } else {
                        vid_low = vid_high = -1;
                    }
                    pcp_mask = k->tag.pcp.mask & 7;
                    pcp_low  = (pcp_mask == 0) ? -1 : k->tag.pcp.value;
                    pcp_high = (pcp_mask == 0) ? -1 : (pcp_mask == 7) ? pcp_low : pcp_low + ((pcp_mask == 6) ? 1 : 3);
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d,%d/%d,%d/%d",
                                  vid_low,
                                  vid_high,
                                  pcp_low,
                                  pcp_high,
                                  k->tag.dei - 1);
                    cyg_httpd_write_chunked(p->outbuffer, ct);

                    if (CAPA->has_qce_inner_tag) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", qcl_qce_tag_type(k->inner_tag.tagged, k->inner_tag.s_tag));
                        cyg_httpd_write_chunked(p->outbuffer, ct);

                        if (k->inner_tag.vid.type  != MESA_VCAP_VR_TYPE_VALUE_MASK) {
                            vid_low  = k->inner_tag.vid.vr.r.low;
                            vid_high = k->inner_tag.vid.vr.r.high;
                        } else if (k->inner_tag.vid.vr.v.mask) {
                            vid_low  = k->inner_tag.vid.vr.v.value;
                            vid_high = -1;
                        } else {
                            vid_low = vid_high = -1;
                        }
                        pcp_mask = k->inner_tag.pcp.mask & 7;
                        pcp_low  = (pcp_mask == 0) ? -1 : k->inner_tag.pcp.value;
                        pcp_high = (pcp_mask == 0) ? -1 : (pcp_mask == 7) ? pcp_low : pcp_low + ((pcp_mask == 6) ? 1 : 3);
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d,%d/%d,%d/%d",
                                      vid_low,
                                      vid_high,
                                      pcp_low,
                                      pcp_high,
                                      k->inner_tag.dei - 1);
                    } else {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/0/-1,-1/-1,-1/-1"); // Dummy inner tag
                    }
                    cyg_httpd_write_chunked(p->outbuffer, ct);

                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s/%d",
                                  (k->mac.smac.mask[0] == 0 &&
                                   k->mac.smac.mask[1] == 0 &&
                                   k->mac.smac.mask[2] == 0 &&
                                   k->mac.smac.mask[3] == 0 &&
                                   k->mac.smac.mask[4] == 0 &&
                                   k->mac.smac.mask[5] == 0) ? "Any" : (misc_mac_txt(k->mac.smac.value, str_buff2)),
                                  qcl_qce_dmac_type(k->mac.dmac_bc, k->mac.dmac_mc));
                    cyg_httpd_write_chunked(p->outbuffer, ct);

                    if (CAPA->has_qce_dmac) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s",
                                      (k->mac.dmac.mask[0] == 0 &&
                                       k->mac.dmac.mask[1] == 0 &&
                                       k->mac.dmac.mask[2] == 0 &&
                                       k->mac.dmac.mask[3] == 0 &&
                                       k->mac.dmac.mask[4] == 0 &&
                                       k->mac.dmac.mask[5] == 0) ? "" : (misc_mac_txt(k->mac.dmac.value, str_buff2)));
                    } else {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/"); // Empty dmac
                    }
                    cyg_httpd_write_chunked(p->outbuffer, ct);

                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%d/%d",
                                  (a->prio_enable) ? a->prio : -1,
                                  (a->dp_enable)   ? a->dp :   -1,
                                  (a->dscp_enable) ? a->dscp : -1);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%d",
                                  a->pcp_dei_enable   ? a->pcp : -1,
                                  a->pcp_dei_enable   ? a->dei : -1);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d",
                                  a->policy_no_enable ? a->policy_no : -1);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                    if (CAPA->has_ingress_map) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d",
                                      a->map_id_enable ? a->map_id : -1);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                    }

                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";%u", k->type);
                    cyg_httpd_write_chunked(p->outbuffer, ct);

                    switch (k->type) {
                    case MESA_QCE_TYPE_ANY:
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s", "Any");
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        break;
                    case MESA_QCE_TYPE_ETYPE: {
                        mesa_qce_frame_etype_t *etype = &k->frame.etype;
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%04x",
                                      (etype->etype.mask[0] != 0 ||
                                       etype->etype.mask[1] != 0) ? 1 : 0,
                                      etype->etype.value[0] << 8 |
                                      etype->etype.value[1]);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        break;
                    }
                    case MESA_QCE_TYPE_LLC: {
                        mesa_qce_frame_llc_t *llc = &k->frame.llc;
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%d/%d/%02x/%02x/%02x",
                                      (llc->data.mask[0] == 0) ? -1 : llc->data.value[0],
                                      (llc->data.mask[1] == 0) ? -1 : llc->data.value[1],
                                      (llc->data.mask[2] == 0) ? -1 : llc->data.value[2],
                                      llc->data.value[0],
                                      llc->data.value[1],
                                      llc->data.value[2]);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        break;
                    }
                    case MESA_QCE_TYPE_SNAP: {
                        mesa_qce_frame_snap_t *snap = &k->frame.snap;
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%02x/%02x",
                                      ((snap->data.mask[3] == 0) && (snap->data.mask[4] == 0)) ? 0 : 1,
                                      snap->data.value[3],
                                      snap->data.value[4]);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        break;
                    }
                    case MESA_QCE_TYPE_IPV4: {
                        mesa_qce_frame_ipv4_t *i4 = &k->frame.ipv4;
                        mesa_ip_t mask = (CAPA->has_qce_dip ? i4->dip.mask : 0);
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%s,%s/%s,%s/%d/%d/%d",
                                      (i4->proto.mask == 0) ? -1 : i4->proto.value,
                                      (i4->sip.mask == 0) ? "Any" :  misc_ipv4_txt(i4->sip.value, str_buff1),
                                      misc_ipv4_txt(i4->sip.mask, str_buff2),
                                      mask == 0 ? "Any" : misc_ipv4_txt(i4->dip.value, str_buff3),
                                      misc_ipv4_txt(mask, str_buff4),
                                      i4->fragment,
                                      (i4->dscp.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i4->dscp.vr.r.low :
                                      (i4->dscp.vr.v.mask == 0) ? -1 : i4->dscp.vr.v.value,
                                      (i4->dscp.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i4->dscp.vr.r.high : -1);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        // format if TCP || UDP: /[sport low]/[sport high]/[dport low]/[dport high]
                        if (i4->proto.value == 17 || i4->proto.value == 6) { //UDP or TCP
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%d/%d/%d",
                                          (i4->sport.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i4->sport.vr.r.low :
                                          (i4->sport.vr.v.mask == 0) ? -1 : i4->sport.vr.v.value,
                                          (i4->sport.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i4->sport.vr.r.high : -1,
                                          (i4->dport.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i4->dport.vr.r.low :
                                          (i4->dport.vr.v.mask == 0) ? -1 : i4->dport.vr.v.value,
                                          (i4->dport.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i4->dport.vr.r.high : -1);
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                        }
                        break;
                    }
                    case MESA_QCE_TYPE_IPV6: {
                        mesa_qce_frame_ipv6_t *i6 = &k->frame.ipv6;
                        mesa_vcap_ip_t sip4;
                        mesa_vcap_ip_t dip4;

                        vtss_appl_qos_qcl_ipv62ipv4(&i6->sip, &sip4);
                        if (CAPA->has_qce_dip) {
                            vtss_appl_qos_qcl_ipv62ipv4(&i6->dip, &dip4);
                        } else {
                            dip4.mask = 0;
                        }

                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%s,%s/%s,%s/%d/%d",
                                      (i6->proto.mask == 0) ? -1 : i6->proto.value,
                                      (sip4.mask == 0) ? "Any" :  misc_ipv4_txt(sip4.value, str_buff1),
                                      misc_ipv4_txt(sip4.mask, str_buff2),
                                      (dip4.mask == 0) ? "Any" : misc_ipv4_txt(dip4.value, str_buff3),
                                      misc_ipv4_txt(dip4.mask, str_buff4),
                                      (i6->dscp.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i6->dscp.vr.r.low :
                                      (i6->dscp.vr.v.mask == 0) ? -1 : i6->dscp.vr.v.value,
                                      (i6->dscp.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i6->dscp.vr.r.high : -1);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        // format if TCP || UDP: /[sport low]/[sport high]/[dport low]/[dport high]
                        if (i6->proto.value == 17 || i6->proto.value == 6) { //UDP or TCP
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%d/%d/%d",
                                          (i6->sport.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i6->sport.vr.r.low :
                                          (i6->sport.vr.v.mask == 0) ? -1 : i6->sport.vr.v.value,
                                          (i6->sport.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i6->sport.vr.r.high : -1,
                                          (i6->dport.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i6->dport.vr.r.low :
                                          (i6->dport.vr.v.mask == 0) ? -1 : i6->dport.vr.v.value,
                                          (i6->dport.type != MESA_VCAP_VR_TYPE_VALUE_MASK) ? i6->dport.vr.r.high : -1);
                            cyg_httpd_write_chunked(p->outbuffer, ct);
                        }
                        break;
                    }
                    default:
                        T_W("Invalid qce_conf.type field value");
                        break;
                    }
                }
                cyg_httpd_end_chunked();
                return -1; // return from edit case
            default:
                break;
            }
        }
        /* Format (for 'qcl_v2.htm' webpage only):
         * <usid_list>#<qce_list>
         *
         * usid_list :== <usid l>;<usid m>;<usid n> // List of present (active) switches in usid order.
         *
         * qce_list  :== <qce 1>;<qce 2>;<qce 3>;...<qce n> // List of currently defined QCEs (might be empty).
         *   qce  x  :== <usid>/<qce_id>/<ports>/<key_dmac>/<key_smac>/<key_tag>/<key_vid>/<key_pcp>/<key_dei>/<frame_type>/<act_cos>/<act_dpl>/<act_dscp>/<act_pcp>/<act_dei>/<act_policy>
         *     usid       :== 1..16 or -1 for all switches
         *     qce_id     :== 1..256
         *     ports      :== String  // List of ports e.g. "1,3,5,7,9-53"
         *     key_dmac   :== String  // "Any", "Unicast", "Multicast", "Broadcast" or "xx-xx-xx-xx-xx-xx"
         *     key_smac   :== String  // "Any", "xx-xx-xx" or "xx-xx-xx-xx-xx-xx"
         *     key_tag    :== String  // "Any", "Untagged", "Tagged", "C-Tagged" or "S-Tagged"
         *     key_vid    :== String  // "Any" or vid or vid-vid
         *     key_pcp    :== String  // "Any" or pcp or pcp-pcp
         *     key_dei    :== 0..2    // 0: Any, 1: 0, 2: 1
         *     frame_type :== 0..5    // One of mesa_qce_type_t
         *     act_cos    :== -1..7   // -1 is no action, else classify to selected value
         *     act_dpl    :== -1..3   // -1 is no action, else classify to selected value
         *     act_dscp   :== -1..63  // -1 is no action, else classify to selected value
         *     act_pcp    :== -1..7   // -1 is no action, else classify to selected value
         *     act_dei    :== -1..1   // -1 is no action, else classify to selected value
         *     act_policy :== -1..63  // -1 is no action, else classify to selected value
         *     act_map_id :== configQosIngressMapMin..configQosIngressMapMax
         *
         */

        // Create a list of present switches in usid order separated by ";"
        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID);
        while (switch_iter_getnext(&sit)) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d", sit.first ? "" : ";", sit.usid);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        // Create a list of currently defined QCEs
        first = TRUE;
        qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
        while (vtss_appl_qos_qce_intern_get(VTSS_ISID_GLOBAL, VTSS_APPL_QOS_QCL_USER_STATIC, qce_id, &conf, 1) == VTSS_RC_OK) {
            const char *dmac_txt = NULL;
            char vid_txt[24];
            char pcp_txt[24];
            u8   pcp_mask;

            qce_id = q->id;
            (void)vtss_appl_qos_qcl_port_list2txt(k->port_list, buf, TRUE); // Create port list

            if (CAPA->has_qce_dip &&
                (k->mac.dmac.mask[0] || k->mac.dmac.mask[1] || k->mac.dmac.mask[2] ||
                 k->mac.dmac.mask[3] || k->mac.dmac.mask[4] || k->mac.dmac.mask[5])) {
                (void)misc_mac_txt(k->mac.dmac.value, str_buff1);
                dmac_txt = str_buff1;
            } else {
                dmac_txt = vtss_appl_qos_qcl_dmactype2txt(k->mac.dmac_bc, k->mac.dmac_mc, TRUE);
            }

            pcp_mask = k->tag.pcp.mask & 7;
            if (!pcp_mask) {
                sprintf(pcp_txt, "Any");
            } else if (pcp_mask == 7) {
                sprintf(pcp_txt, "%u", k->tag.pcp.value);
            } else {
                sprintf(pcp_txt, "%u-%u", k->tag.pcp.value, k->tag.pcp.value + ((pcp_mask == 6) ? 1 : 3));
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d/%u/%s/%s/%s/%s/%s/%s/%u/%u/%d/%d/%d",
                          (first) ? "#" : ";",
                          (conf.isid == VTSS_ISID_GLOBAL) ? -1 : topo_isid2usid(conf.isid),
                          qce_id,
                          buf,
                          dmac_txt,
                          (k->mac.smac.mask[0] == 0 &&
                           k->mac.smac.mask[1] == 0 &&
                           k->mac.smac.mask[2] == 0 &&
                           k->mac.smac.mask[3] == 0 &&
                           k->mac.smac.mask[4] == 0 &&
                           k->mac.smac.mask[5] == 0) ? "Any" : (misc_mac_txt(k->mac.smac.value, str_buff2)),
                          vtss_appl_qos_qcl_tag_type2txt(k->tag.tagged, k->tag.s_tag, TRUE),
                          vtss_appl_qos_qcl_range2txt(&k->tag.vid, vid_txt, TRUE),
                          pcp_txt,
                          k->tag.dei,
                          k->type,
                          (a->prio_enable) ? a->prio : -1,
                          (a->dp_enable)   ? a->dp :   -1,
                          (a->dscp_enable) ? a->dscp : -1);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%d",
                          a->pcp_dei_enable   ? a->pcp : -1,
                          a->pcp_dei_enable   ? a->dei : -1);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d",
                          a->policy_no_enable ? a->policy_no : -1);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            if (CAPA->has_ingress_map) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d",
                              a->map_id_enable ? a->map_id : -1);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            first = FALSE;
        } // end of while loop
        cyg_httpd_end_chunked();
    } // end of Get method response section
    return -1; // Do not further search the file system.
} // end of handler_config_qcl_v2 function

/* support for qce status*/
static void qce_overview(CYG_HTTPD_STATE *p, int user, vtss_appl_qos_qce_intern_conf_t *conf)
{
    int               ct;
    char              buf[MGMT_PORT_BUF_SIZE];
    mesa_qce_t        *const q = &conf->qce;
    mesa_qce_key_t    *const k = &q->key;
    mesa_qce_action_t *const a = &q->action;

    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/%d/%u/%s/%d/%d/%d/%d",
                  conf->conflict ? "Yes" : "No",
                  user,
                  q->id,
                  vtss_appl_qos_qcl_port_list2txt(k->port_list, buf, TRUE),
                  k->type,
                  (a->prio_enable) ? a->prio : -1,
                  (a->dp_enable)   ? a->dp :   -1,
                  (a->dscp_enable) ? a->dscp : -1);
    cyg_httpd_write_chunked(p->outbuffer, ct);
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d/%d",
                  a->pcp_dei_enable   ? a->pcp : -1,
                  a->pcp_dei_enable   ? a->dei : -1);
    cyg_httpd_write_chunked(p->outbuffer, ct);
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d",
                  a->policy_no_enable ? a->policy_no : -1);
    cyg_httpd_write_chunked(p->outbuffer, ct);
    if (CAPA->has_ingress_map) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d",
                      a->map_id_enable ? a->map_id : -1);
        cyg_httpd_write_chunked(p->outbuffer, ct);
    }
    cyg_httpd_write_chunked(";", 1);
}

static i32 handler_stat_qcl_v2(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                     isid;
    mesa_qce_id_t                   qce_id;
    vtss_appl_qos_qce_intern_conf_t conf;
    int                             ct, qcl_user = -1, conflict, user_cnt;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_GET) {
        isid = web_retrieve_request_sid(p); /* Includes USID = ISID */

        if ((cyg_httpd_form_varable_int(p, "ConflictResolve", &conflict))) {
            if (conflict == 1) {
                (void)vtss_appl_qos_qce_conflict_resolve();
            }
        }
        if ((cyg_httpd_form_varable_int(p, "qclUser", &qcl_user))) {
        }

        /* Format:
         * <qcl_info>|<qce_list>
         *
         * qcl_info       :== <sel_user>/<user 1>/<user 2>/...<user n>
         *   sel_user     :== -2: Show Conflict, -1: Show Combined, 0: Show Static, 1: Show Voice VLAN
         *   user x       :== 0..n    // List of defined users to show in user selector between "Combined" and "Conflict".
         *
         * qce_list       :== <qce 1>;<qce 2>;<qce 3>;...<qce n> // List of currently defined QCEs (might be empty).
         *   qce  x       :== <conflict>/<user>/<qce_id>/<frame_type>/<ports>/<act_class>/<act_dpl>/<act_dscp>
         *     conflict   :== String  // "Yes" or "No"
         *     user       :== 0..n    // One of the defined users
         *     qce_id     :== 1..256
         *     ports      :== String  // List of ports e.g. "1,3,5,7,9-53"
         *     frame_type :== 0..5    // One of mesa_qce_type_t
         *     act_cos    :== -1..7   // -1 is no action, else classify to selected value
         *     act_dpl    :== -1..3   // -1 is no action, else classify to selected value
         *     act_dscp   :== -1..63  // -1 is no action, else classify to selected value
         *     act_pcp    :== -1..7   // -1 is no action, else classify to selected value
         *     act_dei    :== -1..1   // -1 is no action, else classify to selected value
         *     act_policy :== -1..63  // -1 is no action, else classify to selected value
         *     act_map_id :== configQosIngressMapMin..configQosIngressMapMax
         */

        cyg_httpd_start_chunked("html");
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d", qcl_user);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        for (user_cnt = VTSS_APPL_QOS_QCL_USER_STATIC; user_cnt < VTSS_APPL_QOS_QCL_USER_CNT; user_cnt++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%d", user_cnt);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_write_chunked("|", 1);
        if (qcl_user < 0) { // Show Combined (-1) or Conflict (-2)
            for (user_cnt = VTSS_APPL_QOS_QCL_USER_STATIC; user_cnt < VTSS_APPL_QOS_QCL_USER_CNT; user_cnt++) {
                qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
                while (vtss_appl_qos_qce_intern_get(isid, (vtss_appl_qos_qcl_user_t)user_cnt, qce_id, &conf, 1) == VTSS_RC_OK) {
                    qce_id = conf.qce.id;
                    if (qcl_user == -1 || (qcl_user == -2 && conf.conflict)) {
                        qce_overview(p, user_cnt, &conf);
                    }
                }
            }
        } else { // Show selected user only (0..n)
            qce_id = VTSS_APPL_QOS_QCE_ID_NONE;
            while (vtss_appl_qos_qce_intern_get(isid, (vtss_appl_qos_qcl_user_t)qcl_user, qce_id, &conf, 1) == VTSS_RC_OK) {
                qce_id = conf.qce.id;
                qce_overview(p, qcl_user, &conf);
            }
        }
        cyg_httpd_end_chunked();
    }
    return -1;
}

#if defined(VTSS_SW_OPTION_QOS_ADV)
static i32 handler_config_dscp_port_config (CYG_HTTPD_STATE *p)
{
    vtss_isid_t              isid;
    port_iter_t               pit;
    vtss_appl_qos_port_conf_t conf;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif
    isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    if (p->method == CYG_HTTPD_METHOD_POST) {
        if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
            return -1;
        }
        int val, errors = 0;
        mesa_rc rc;

        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if ((rc = vtss_appl_qos_port_conf_get(isid, pit.iport, &conf)) != VTSS_RC_OK) {
                errors++; /* Probably stack error */
                T_D("vtss_appl_qos_conf_get(%u, %d): failed rc = %d", isid, pit.uport, rc);
                continue;
            }
            /* Translate */
            conf.port.dscp_translate = cyg_httpd_form_variable_check_fmt(p, "enable_%d", pit.uport);
            /* Classify */
            if (cyg_httpd_form_variable_int_fmt(p, &val, "classify_%u", pit.uport)) {
                conf.port.dscp_imode = (vtss_appl_qos_dscp_mode_t)val;
            }
            /* Rewrite */
            if (cyg_httpd_form_variable_int_fmt(p, &val, "rewrite_%u", pit.uport)) {
                switch (val) {
                case 1:/* Ensable */
                    conf.port.dscp_emode = VTSS_APPL_QOS_DSCP_EMODE_REMARK;
                    break;
                case 2:/* Remap DP Unaware */
                    conf.port.dscp_emode = VTSS_APPL_QOS_DSCP_EMODE_REMAP;
                    break;
                case 3:/* Remap DP aware */
                    conf.port.dscp_emode = VTSS_APPL_QOS_DSCP_EMODE_REMAP_DPA;
                    break;
                default:
                    conf.port.dscp_emode = VTSS_APPL_QOS_DSCP_EMODE_DISABLE;
                    break;
                }
            }
            (void) vtss_appl_qos_port_conf_set(isid, pit.iport, &conf);
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_port_dscp_config.htm");
    } else {//GET Method
        /*Format:
         <port number>/<ingr. translate>/<ingr. classify>/<egr. rewrite>|
        */
        int ct;
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (vtss_appl_qos_port_conf_get(isid, pit.iport, &conf) != VTSS_RC_OK) {
                break;          /* Probably stack error - bail out */
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u#%u#%u#%u",
                          pit.first ? "" : "|",
                          pit.uport,
                          conf.port.dscp_translate,
                          conf.port.dscp_imode,
                          conf.port.dscp_emode);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }
    return -1;
}
static i32 handler_config_dscp_based_qos_ingr_classi (CYG_HTTPD_STATE *p)
{
    vtss_appl_qos_conf_t conf;
    int                  cnt;
    int                  val;
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif
    if (vtss_appl_qos_conf_get(&conf) != VTSS_RC_OK) {
        T_D("qos_conf_get() failed");
        return -1;
    }
    if (p->method == CYG_HTTPD_METHOD_POST) {
        for (cnt = 0; cnt < 64; cnt++) {
            /* Trust trust_chk_ */
            conf.dscp_map[cnt].trust = cyg_httpd_form_variable_check_fmt(p, "trust_chk_%d", cnt);
            /* Class classify_sel_ */
            if (cyg_httpd_form_variable_int_fmt(p, &val, "classify_sel_%d", cnt)) {
                conf.dscp_map[cnt].cos = val;
            }
            /* DPL dpl_sel_ */
            if (cyg_httpd_form_variable_int_fmt(p, &val, "dpl_sel_%d", cnt)) {
                conf.dscp_map[cnt].dpl = val;
            }
        }
        (void)vtss_appl_qos_conf_set(&conf);
        redirect(p, "/dscp_based_qos_ingr_classifi.htm");
    } else {/* GET Method */
        /*Format:
         <dscp number>#<trust>#<QoS class>#<dpl>|
        */
        int ct;
        cyg_httpd_start_chunked("html");
        for (cnt = 0; cnt < 64; cnt++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d#%d#%u#%d",
                          (cnt != 0) ? "|" : "",
                          cnt,
                          conf.dscp_map[cnt].trust,
                          conf.dscp_map[cnt].cos,
                          conf.dscp_map[cnt].dpl);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }
    return -1;
}
static i32 handler_config_dscp_translation (CYG_HTTPD_STATE *p)
{
    vtss_appl_qos_conf_t conf;
    int                  cnt;
    int                  temp;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif
    if (vtss_appl_qos_conf_get(&conf) != VTSS_RC_OK) {
        T_D("qos_conf_get() failed");
        return -1;
    }
    if (p->method == CYG_HTTPD_METHOD_POST) {
        for (cnt = 0; cnt < 64; cnt++) {
            /*DSCP translate */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "trans_sel_%d", cnt)) {
                conf.dscp_map[cnt].dscp = temp;
            }
            /* classify */
            conf.dscp_map[cnt].remark = cyg_httpd_form_variable_check_fmt(p, "classi_chk_%d", cnt);
            /* Remap DP0 */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "rmp_dp0_%d", cnt)) {
                conf.dscp_map[cnt].dscp_remap = temp;
            }
            /* Remap DP1 */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "rmp_dp1_%d", cnt)) {
                conf.dscp_map[cnt].dscp_remap_dp1 = temp;
            }
        }
        (void)vtss_appl_qos_conf_set(&conf);
        redirect(p, "/dscp_translation.htm");
    } else {/* GET method */
        /*Format
         <dscp number>/<DSCP translate>/<classify>/<remap DP0>/<remap DP1>|...
        */
        int ct;
        cyg_httpd_start_chunked("html");
        for (cnt = 0; cnt < 64; cnt++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d/%d/%d/%d/%d",
                          (cnt != 0) ? "|" : "",
                          cnt,
                          conf.dscp_map[cnt].dscp,
                          conf.dscp_map[cnt].remark,
                          conf.dscp_map[cnt].dscp_remap,
                          conf.dscp_map[cnt].dscp_remap_dp1);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_write_chunked("|", 1);
        cyg_httpd_end_chunked();
    }
    return -1;
}
static i32 handler_config_qos_dscp_classification_map (CYG_HTTPD_STATE *p)
{
    vtss_appl_qos_conf_t conf;
    int                  cls_cnt, temp;
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif
    if (vtss_appl_qos_conf_get(&conf) != VTSS_RC_OK) {
        T_D("qos_conf_get() failed");
        return -1;
    }
    if (p->method == CYG_HTTPD_METHOD_POST) {
        for (cls_cnt = 0; cls_cnt < VTSS_APPL_QOS_PORT_PRIO_CNT; cls_cnt++) {
            /* DSCP map to dp 0 */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "dscp_0_%d", cls_cnt)) {
                conf.cos_dscp_map[cls_cnt].dscp = temp;
            }
            /* DSCP map to dp 1 */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "dscp_1_%d", cls_cnt)) {
                conf.cos_dscp_map[cls_cnt].dscp_dp1 = temp;
            }
            /* DSCP map to dp 2 */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "dscp_2_%d", cls_cnt)) {
                conf.cos_dscp_map[cls_cnt].dscp_dp2 = temp;
            }
            /* DSCP map to dp 3 */
            if (cyg_httpd_form_variable_int_fmt(p, &temp, "dscp_3_%d", cls_cnt)) {
                conf.cos_dscp_map[cls_cnt].dscp_dp3 = temp;
            }
        }
        (void)vtss_appl_qos_conf_set(&conf);
        redirect(p, "/dscp_classification.htm");
    } else {
        /*Format:
         <dscp class>/<dscp 0>/<dscp 1>/<dscp 2>/<dscp 3>|
        */
        int ct;
        cyg_httpd_start_chunked("html");
        for (cls_cnt = 0; cls_cnt < VTSS_APPL_QOS_PORT_PRIO_CNT; cls_cnt++) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d/%d/%d/%d/%d",
                          (cls_cnt != 0) ? "|" : "",
                          cls_cnt,
                          conf.cos_dscp_map[cls_cnt].dscp,
                          conf.cos_dscp_map[cls_cnt].dscp_dp1,
                          conf.cos_dscp_map[cls_cnt].dscp_dp2,
                          conf.cos_dscp_map[cls_cnt].dscp_dp3);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }
    return -1;
}
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

static i32 handler_config_qos_port_classification(CYG_HTTPD_STATE *p)
{
    vtss_isid_t               isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t               pit;
    vtss_appl_qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int val, errors = 0;
        mesa_rc rc;
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if ((rc = vtss_appl_qos_port_conf_get(isid, pit.iport, &conf)) != VTSS_RC_OK) {
                errors++; /* Probably stack error */
                T_D("vtss_appl_qos_port_conf_get(%u, %d): failed rc = %d", isid, pit.uport, rc);
                continue;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "class_%u", pit.uport)) {
                conf.port.default_cos = val;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "dpl_%u", pit.uport)) {
                conf.port.default_dpl = val;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "pcp_%u", pit.uport)) {
                conf.port.default_pcp = val;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "dei_%u", pit.uport)) {
                conf.port.default_dei = val;
            }
            conf.port.trust_dscp = cyg_httpd_form_variable_check_fmt(p, "dscp_enable_%d", pit.uport);
            if (cyg_httpd_form_variable_int_fmt(p, &val, "dmac_dip_%u", pit.uport)) {
                conf.port.dmac_dip = val;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "key_type_%u", pit.uport)) {
                conf.port.key_type = (mesa_vcap_key_type_t)val;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "wred_group_%u", pit.uport)) {
                conf.port.wred_group = (mesa_wred_group_t)val;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "ingressMapId%u", pit.uport)) {
                conf.port.ingress_map = (mesa_qos_ingress_map_id_t)val;
            } else {
                conf.port.ingress_map = MESA_QOS_MAP_ID_NONE;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "egressMapId%u", pit.uport)) {
                conf.port.egress_map = (mesa_qos_egress_map_id_t)val;
            } else {
                conf.port.egress_map = MESA_QOS_MAP_ID_NONE;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &val, "cosid_%u", pit.uport)) {
                conf.port.default_cosid = val;
            }

            if ((rc = vtss_appl_qos_port_conf_set(isid, pit.iport, &conf)) != VTSS_RC_OK) {
                errors++; /* Probably stack error */
                T_D("vtss_appl_qos_port_conf_set(%u, %d): failed rc = %d", isid, pit.uport, rc);
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_port_classification.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        int ct;
        cyg_httpd_start_chunked("html");

        /*
         * Format:
         * <options>|<ports>
         *
         * options :== <show_pcp_dei>,<show_tag_classification>,<show_dscp_classification>
         *   show_pcp_dei             :== 0..1 // 0: hide - , 1: show pcp and dei select cells
         *   show_tag_classification  :== 0..1 // 0: hide - , 1: show tag classification
         *   show_dscp_classification :== 0..1 // 0: hide - , 1: show dscp classification
         *
         * ports :== <port 1>,<port 2>,<port 3>,...<port n>
         *   port x :== <port_no>#<default_pcp>#<default_dei>#<default_class>#<volatile_class>#<default_dpl>#<tag_class>#<dscp_class>#<dmac_dip>#<key_type>#<wred_group>#<ingress_map>#<egress_map>#<default_cosid>
         *     port_no        :== 1..max
         *     default_pcp    :== 0..7
         *     default_dei    :== 0..1
         *     default_class  :== 0..7
         *     volatile_class :== 0..7 or -1 if volatile is not set
         *     default_dpl    :== 0..3
         *     tag_class      :== 0..1 // 0: Disabled, 1: Enabled
         *     dscp_class     :== 0..1 // 0: Disabled, 1: Enabled
         *     dmac_dip       :== 0..1 // 0: Disabled, 1: Enabled
         *     key_type       :== 0..3 // One of mesa_vcap_key_type_t
         *     wred_group     :== 1..3 // 1 based WRED group
         *     ingress_map    :== configQosIngressMapMin..configQosIngressMapMax
         *     egress_map     :== configQosEgressMapMin..configQosEgressMapMax
         *     default_cosid  :== 0..7
         */

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u,%u|",
                      (CAPA->has_fixed_tag_cos_map == FALSE),
                      (CAPA->has_fixed_tag_cos_map == FALSE) && CAPA->has_tag_classification,
                      CAPA->has_dscp);

        cyg_httpd_write_chunked(p->outbuffer, ct);
        (void) port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            vtss_appl_qos_port_status_t status;

            if ((vtss_appl_qos_port_conf_get(isid, pit.iport, &conf) != VTSS_RC_OK)  ||
                (vtss_appl_qos_port_status_get(isid, pit.iport, &status) != VTSS_RC_OK)) {
                break;          /* Probably stack error - bail out */
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "%s%u#%u#%u#%u#%d#%u#%u#%u#%u#%u#%u#%u#%u#%u",
                          pit.first ? "" : ",",
                          pit.uport,
                          conf.port.default_pcp,
                          conf.port.default_dei,
                          conf.port.default_cos,
                          (conf.port.default_cos == status.default_cos) ? -1 : status.default_cos,
                          conf.port.default_dpl,
                          conf.port.trust_tag,
                          conf.port.trust_dscp,
                          conf.port.dmac_dip,
                          conf.port.key_type,
                          conf.port.wred_group,
                          (CAPA->has_ingress_map == TRUE) ? conf.port.ingress_map : 0,
                          (CAPA->has_egress_map == TRUE) ? conf.port.egress_map : 0,
                          (CAPA->has_cosid_classification == TRUE) ? conf.port.default_cosid : 0
                         );
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

#if defined(VTSS_SW_OPTION_QOS_ADV)
static i32 handler_config_qos_port_classification_map(CYG_HTTPD_STATE *p)
{
    int                       ct, i, uport;
    vtss_isid_t               sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_port_no_t            iport = VTSS_PORT_NO_START;
    vtss_appl_qos_port_conf_t conf;
    char                      buf[64];

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
            if (iport >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                errors++;
            }
        } else {
            errors++;
        }

        if (errors || vtss_appl_qos_port_conf_get(sid, iport, &conf) != VTSS_RC_OK) {
            errors++;
        } else {
            int            val;
            mesa_rc        rc;

            if (cyg_httpd_form_varable_int(p, "tag_class", &val)) {
                conf.port.trust_tag = val;
            }
            for (i = 0; i < (VTSS_PCPS * 2); i++) {
                if (cyg_httpd_form_variable_int_fmt(p, &val, "default_class_%d", i)) {
                    conf.tag_cos_map[i / 2][i % 2].cos = val;
                }
                if (cyg_httpd_form_variable_int_fmt(p, &val, "default_dpl_%d", i)) {
                    conf.tag_cos_map[i / 2][i % 2].dpl = val;
                }
            }

            if ((rc = vtss_appl_qos_port_conf_set(sid, iport, &conf)) != VTSS_RC_OK) {
                T_D("vtss_appl_qos_conf_set(%u, %d): failed rc = %d", sid, iport, rc);
                errors++; /* Probably stack error */
            }
        }

        (void)snprintf(buf, sizeof(buf), "/qos_port_classification_map.htm?port=%u", iport2uport(iport));
        redirect(p, errors ? STACK_ERR_URL : buf);
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
        }
        if (iport >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
            iport = VTSS_PORT_NO_START;
        }

        if (vtss_appl_qos_port_conf_get(sid, iport, &conf) == VTSS_RC_OK) {

            /* Format:
             * <port_no>#<tag_class>#<map>
             *
             * port_no       :== 1..max
             * tag_class     :== 0..1 // 0: Disabled, 1: Enabled
             * map           :== <entry_0>/<entry_1>/...<entry_n> // n is 15.
             *   entry_x     :== <class|dpl>
             *     class     :== 0..7
             *     dpl       :== 0..3
             *
             * The map is organized as follows:
             * Entry corresponds to PCP,       DEI
             *  0                   0          0
             *  1                   0          1
             *  2                   1          0
             *  3                   1          1
             *  4                   2          0
             *  5                   2          1
             *  6                   3          0
             *  7                   3          1
             *  8                   4          0
             *  9                   4          1
             * 10                   5          0
             * 11                   5          1
             * 12                   6          0
             * 13                   6          1
             * 14                   7          0
             * 15                   7          1
             */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#%u",
                          iport2uport(iport),
                          conf.port.trust_tag);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (i = 0; i < (VTSS_PCPS * 2); i++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u|%u",
                              (i != 0) ? "/" : "#",
                              conf.tag_cos_map[i / 2][i % 2].cos,
                              conf.tag_cos_map[i / 2][i % 2].dpl);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

/* Support for a given feature is encoded like this:
   0: No supported
   1: Supported on other ports
   2: Supported on this port */
static u8 port_feature_support(meba_port_cap_t cap, meba_port_cap_t port_cap, meba_port_cap_t mask)
{
    return ((cap & mask) ? ((port_cap & mask) ? 2 : 1) : 0);
}

static i32 handler_config_qos_port_policers(CYG_HTTPD_STATE *p)
{
    int                       ct;
    vtss_isid_t               sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t               pit;
    vtss_appl_qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            mesa_rc        rc;
            ulong          rate;
            int            val;
            if ((rc = vtss_appl_qos_port_conf_get(sid, pit.iport, &conf)) != VTSS_RC_OK) {
                errors++; /* Probably stack error */
                T_D("vtss_appl_qos_conf_get(%u, %d): failed rc = %d", sid, pit.uport, rc);
                continue;
            }
            conf.port_policer.enable = cyg_httpd_form_variable_check_fmt(p, "enabled_%u", pit.iport);

            if (cyg_httpd_form_variable_long_int_fmt(p, &rate, "rate_%u", pit.iport)) {
                conf.port_policer.cir = (mesa_bitrate_t) rate;
            }

            if (cyg_httpd_form_variable_int_fmt(p, &val, "fps_%u", pit.iport)) {
                conf.port_policer.frame_rate = (val != 0);
            }

            conf.port_policer.flow_control = cyg_httpd_form_variable_check_fmt(p, "flow_control_%u", pit.iport);

            if ((rc = vtss_appl_qos_port_conf_set(sid, pit.iport, &conf)) != VTSS_RC_OK) {
                errors++; /* Probably stack error */
                T_D("vtss_appl_qos_conf_set(%u, %d): failed rc = %d", sid, pit.uport, rc);
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_port_policers.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        vtss_appl_port_status_t       port_status;
        vtss_appl_port_capabilities_t cap;

        (void)vtss_appl_port_capabilities_get(&cap);

        cyg_httpd_start_chunked("html");

        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            vtss_ifindex_t ifindex;
            if (vtss_ifindex_from_port(sid, pit.iport, &ifindex) != VTSS_RC_OK) {
                T_E("Could not get ifindex");
                break;
            };

            if ((vtss_appl_qos_port_conf_get(sid, pit.iport, &conf) != VTSS_RC_OK) ||
                (vtss_appl_port_status_get(ifindex, &port_status) != VTSS_RC_OK)) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x :== <port_no>/<enabled>/<fps>/<rate>/<flow_control>
             *   port_no      :== 1..max
             *   enabled      :== 0..1           // 0: no, 1: yes
             *   fps          :== 0..1           // 0: unit for rate is kilobits pr seconds (kbps), 1: unit for rate is frames pr second (fps)
             *   rate         :== 0..0xffffffff  // actual bit or frame rate
             *   fc_mode      :== 0..2           // 0: No ports has fc (don't show fc column), 1: This port has no fc, 2: This port has fc
             *   flow_control :== 0..1           // flow control is enabled
             */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u/%u/%u/%u/%u/%u",
                          pit.first ? "" : ",",
                          pit.uport,
                          conf.port_policer.enable,
                          conf.port_policer.frame_rate,
                          conf.port_policer.cir,
                          port_feature_support(cap.aggr_caps, port_status.static_caps, MEBA_PORT_CAP_FLOW_CTRL),
                          conf.port_policer.flow_control);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

#if defined(VTSS_SW_OPTION_QOS_ADV)
static i32 handler_config_qos_queue_policers(CYG_HTTPD_STATE *p)
{
    int                       ct, queue;
    vtss_isid_t               sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t               pit;
    vtss_appl_qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            mesa_rc        rc;
            ulong          rate;
            if ((rc = vtss_appl_qos_port_conf_get(sid, pit.iport, &conf)) != VTSS_RC_OK) {
                errors++; /* Probably stack error */
                T_D("vtss_appl_qos_port_conf_get(%u, %d): failed rc = %d", sid, pit.uport, rc);
                continue;
            }
            for (queue = 0; queue < (VTSS_QUEUE_ARRAY_SIZE); queue++) {
                conf.queue_policer[queue].enable = cyg_httpd_form_variable_check_fmt(p, "enabled_%d_%u", queue, pit.iport);

                if (cyg_httpd_form_variable_long_int_fmt(p, &rate, "rate_%d_%u", queue, pit.iport)) {
                    conf.queue_policer[queue].cir = (mesa_bitrate_t) rate;
                }
            }

            if ((rc = vtss_appl_qos_port_conf_set(sid, pit.iport, &conf)) != VTSS_RC_OK) {
                errors++; /* Probably stack error */
                T_D("vtss_appl_qos_port_conf_set(%u, %d): failed rc = %d", sid, pit.uport, rc);
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_queue_policers.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (vtss_appl_qos_port_conf_get(sid, pit.iport, &conf) != VTSS_RC_OK) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x :== <port_no>#<queues>
             *   port_no :== 1..max
             *   queues  :== <queue 0>/<queue 1>/<queue 2>/...<queue n>
             *     queue x :== <enabled>|<rate>
             *       enabled :== 0..1           // 0: no, 1: yes
             *       rate    :== 0..0xffffffff  // bit rate
             */

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u",
                          pit.first ? "" : ",",
                          pit.uport);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (queue = 0; queue < (VTSS_QUEUE_ARRAY_SIZE); queue++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u|%u",
                              (queue != 0) ? "/" : "#",
                              conf.queue_policer[queue].enable,
                              conf.queue_policer[queue].cir);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

static i32 handler_config_qos_port_schedulers(CYG_HTTPD_STATE *p)
{
    int                       ct, queue;
    vtss_isid_t               sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t               pit;
    vtss_appl_qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/qos_port_schedulers.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (vtss_appl_qos_port_conf_get(sid, pit.iport, &conf) != VTSS_RC_OK) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x :== <port_no>#<dwrr_cnt>#<queue_weights>
             *   port_no          :== 1..max
             *   dwrr_cnt         :== 0..8           // 0-1: Strict Priority, 2-8: Weighted
             *   queue_weights    :== <queue_1_weight>/<queue_2_weight>/...<queue_n_weight>
             *     queue_x_weight :== 1..100         // Configured weight
             */

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u#%u",
                          pit.first ? "" : ",",
                          pit.uport,
                          conf.port.dwrr_cnt);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (queue = 0; queue < VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u",
                              (queue != 0) ? "/" : "#",
                              conf.scheduler[queue].weight);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_qos_port_shapers(CYG_HTTPD_STATE *p)
{
    int                       ct, queue;
    vtss_isid_t               sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t               pit;
    vtss_appl_qos_port_conf_t conf;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/qos_port_shapers.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (vtss_appl_qos_port_conf_get(sid, pit.iport, &conf) != VTSS_RC_OK) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x          :== <port_no>#<queue_shapers>#<port_shaper>
             *   port_no       :== 1..max
             *   queue_shapers :== <shaper_1>/<shaper_2>/...<shaper_n>                 // n is 8.
             *     shaper_x    :== <enable|rate>
             *       enable    :== 0..1           // 0: Shaper is disabled, 1: Shaper is disabled
             *       rate      :== 0..0xffffffff  // Actual bit rate in kbps
             *   port_shaper   :== <enable|rate>
             *     enable      :== 0..1           // 0: Shaper is disabled, 1: Shaper is disabled
             *     rate        :== 0..0xffffffff  // Actual bit rate in kbps
             */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u",
                          pit.first ? "" : ",",
                          pit.uport);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (queue = 0; queue < (VTSS_QUEUE_ARRAY_SIZE); queue++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u|%u",
                              (queue != 0) ? "/" : "#",
                              conf.queue_shaper[queue].enable,
                              conf.queue_shaper[queue].cir);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%u|%u",
                          conf.port_shaper.enable,
                          conf.port_shaper.cir);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_qos_port_scheduler_edit(CYG_HTTPD_STATE *p)
{
    int                       ct, queue, uport;
    vtss_isid_t               sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_port_no_t            iport = VTSS_PORT_NO_START;
    vtss_appl_qos_port_conf_t conf;
    char                      buf[64];
    int                       queue_shaper_min = 0, queue_shaper_max = VTSS_QUEUES - 1;
    BOOL                      queue_shaper_excess = CAPA->has_queue_shapers_eb;
    BOOL                      queue_shaper_credit = CAPA->has_queue_shapers_crb;
    BOOL                      queue_cut_through   = CAPA->has_queue_cut_through;
    BOOL                      frame_preemption    = CAPA->has_queue_frame_preemption;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
            if (iport >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                errors++;
            }
        } else {
            errors++;
        }

        if (errors || vtss_appl_qos_port_conf_get(sid, iport, &conf) != VTSS_RC_OK) {
            errors++;
        } else {
            mesa_rc        rc;
            int            val;
            ulong          rate = 0;
            int            mode;

            if (cyg_httpd_form_varable_int(p, "dwrr_cnt", &val)) {
                conf.port.dwrr_cnt = val;
            }

            // Get Port Scheduler configuration
            for (queue = 0; queue < conf.port.dwrr_cnt; queue++) {
                if (cyg_httpd_form_variable_int_fmt(p, &val, "weight_%d", queue)) {
                    conf.scheduler[queue].weight = val;
                }
            }

            // Get Queue Shaper configuration
            for (queue = 0; queue < VTSS_QUEUES; queue++) {
                conf.queue_shaper[queue].enable = cyg_httpd_form_variable_check_fmt(p, "q_shaper_enable_%d", queue);

                // Only save rate and excess if shaper is enabled
                if (conf.queue_shaper[queue].enable) {
                    if (cyg_httpd_form_variable_long_int_fmt(p, &rate, "q_shaper_rate_%d", queue)) {
                        conf.queue_shaper[queue].cir = (mesa_bitrate_t) rate;
                    }
                    if (CAPA->has_shapers_rt && cyg_httpd_form_variable_int_fmt(p, &mode, "q_shaper_mode_%d", queue)) {
                        conf.queue_shaper[queue].mode = (vtss_appl_qos_shaper_mode_t)mode;
                    }
                    conf.queue_shaper[queue].excess = cyg_httpd_form_variable_check_fmt(p, "q_shaper_excess_%d", queue);
                    conf.queue_shaper[queue].credit = cyg_httpd_form_variable_check_fmt(p, "q_shaper_credit_%d", queue);
                }
                // Save queue cut-through config
                conf.scheduler[queue].cut_through = cyg_httpd_form_variable_check_fmt(p, "q_cut_through_%d", queue);
                // Save queue frame preemption config
                conf.scheduler[queue].frame_preemption = cyg_httpd_form_variable_check_fmt(p, "q_frame_preemption_%d", queue);
            }

            // Get Port Shaper configuration
            conf.port_shaper.enable = cyg_httpd_form_varable_find(p, "p_shaper_enable") ? TRUE : FALSE;

            // Only save rate if shaper is enabled
            if (conf.port_shaper.enable && cyg_httpd_form_varable_long_int(p, "p_shaper_rate", &rate)) {
                conf.port_shaper.cir = rate;
            }
            if (CAPA->has_shapers_rt && conf.port_shaper.enable && cyg_httpd_form_varable_int(p, "p_shaper_mode", &mode)) {
                conf.port_shaper.mode = (vtss_appl_qos_shaper_mode_t)mode;
            }
            if ((rc = vtss_appl_qos_port_conf_set(sid, iport, &conf)) != VTSS_RC_OK) {
                T_D("vtss_appl_qos_conf_set(%u, %d): failed rc = %d", sid, iport, rc);
                errors++; /* Probably stack error */
            }
        }

        (void)snprintf(buf, sizeof(buf), "/qos_port_scheduler_edit.htm?port=%u", iport2uport(iport));
        redirect(p, errors ? STACK_ERR_URL : buf);
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
        }
        if (iport >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
            iport = VTSS_PORT_NO_START;
        }

        if (vtss_appl_qos_port_conf_get(sid, iport, &conf) == VTSS_RC_OK) {

            /* Format:
             * <port_no>#<excess_support>#<credit_support>#<rate_type_support>#<cut_through_support>#<cut_through>#
             *  <fp_support>#<fp>#<dwrr_cnt_support#<dwrr_cnt>#<queue_weights>#<queue_shapers>#<port_shaper>
             *
             * port_no            :== 1..max
             * excess_support     :== 0..1           // 0: Excess is not supported on the queue shapers, 1: Excess is supported on the queue shapers
             * credit_support     :== 0..1           // 0: Credit-based shaper is not supported on the queue shapers, 1: Credit-based shaper is supported on the queue shapers
             * rate_type_support  :== 0..1           // 0: Shaper rate type is fixed to line rate, 1: Rate type is configurable between line and data rate
             * cut_through_support:== 0..1           // 0: Cut-through is not supported, 1: Cut-through is supported
             * cut_through        :== <queue_1_ct>/<queue_2_ct>/...<queue_n_ct>  // n is 8.
             *   queue_x_ct       :== 0..1           // 0: Cut-through is disabled on the queue, 1: Cut-through is enabled on the queue
             * fp_support         :== 0..1           // 0: Frame preemption is not supported, 1: Frame preemption is supported
             * fp                 :== <queue_1_fp>/<queue_2_fp>/...<queue_n_fp>  // n is 8.
             *   queue_x_fp       :== 0..1           // 0: Frame preemption is disabled on the queue, 1: Frame preemption is enabled on the queue
             * dwrr_cnt_support   :== 0..1           // 0: Only 6 weighted queues are supported in non-service, 1: 2-8 weighted queues are supported in non-service
             * dwrr_cnt           :== 0..8           // 0-1: Strict Priority, 2-8: Weighted
             * queue_weights      :== <queue_1_weight>/<queue_2_weight>/...<queue_n_weight>  // n is 8.
             *   queue_x_weight   :== 1..100         // Just a number. If you set all 8 weights to 100, each queue will have a weigth of 100/8 = 12.5 ~ 13%
             * queue_shapers      :== <queue_shaper_1>/<queue_shaper_2>/...<queue_shaper_n>  // n is 2 or 8.
             *   queue_shaper_x   :== <enable|rate(|excess)>
             *     enable         :== 0..1           // 0: Shaper is disabled, 1: Shaper is enabled
             *     rate           :== 0..0xffffffff  // Actual bit rate in kbps/fps
             *     excess         :== 0..1           // 0: Excess bandwidth disabled, 1: Excess bandwidth enabled
             *     rate_type      :== 0..1           // 0: Line rate shaper, 1: Data rate shaper, 2: Frame rate shaper
             * port_shaper        :== <enable|rate>  //
             *   enable           :== 0..1           // 0: Shaper is disabled, 1: Shaper is enabled
             *   rate             :== 0..0xffffffff  // Actual bit rate in kbps
             *   rate_type        :== 0..1           // 0: Line rate shaper, 1: Data rate shaper
             * queue_frame_shaper_support :== 0..1   // 0: No frame based queue shaper supported, 1: Frame based egress queue shaper supported
             */

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#%u#%u#%u",
                          iport2uport(iport),
                          queue_shaper_excess,
                          queue_shaper_credit,
                          CAPA->has_shapers_rt);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            if (queue_cut_through) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#1");
                cyg_httpd_write_chunked(p->outbuffer, ct);
                for (queue = 0; queue < VTSS_QUEUES; queue++) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u",
                                  (queue != 0) ? "/" : "#",
                                  conf.scheduler[queue].cut_through);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#0#0");
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            if (frame_preemption) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#1");
                cyg_httpd_write_chunked(p->outbuffer, ct);
                for (queue = 0; queue < VTSS_QUEUES; queue++) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u",
                                  (queue != 0) ? "/" : "#",
                                  conf.scheduler[queue].frame_preemption);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#0#0");
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%u", fast_cap(MESA_CAP_QOS_SCHEDULER_CNT_DWRR));
            cyg_httpd_write_chunked(p->outbuffer, ct);

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%u", conf.port.dwrr_cnt);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (queue = 0; queue < VTSS_QUEUES; queue++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u",
                              (queue != 0) ? "/" : "#",
                              conf.scheduler[queue].weight);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            for (queue = queue_shaper_min; queue <= queue_shaper_max; queue++) {
                BOOL first = (queue == queue_shaper_min);
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u|%u|%u",
                              first ? "#" : "/",
                              conf.queue_shaper[queue].enable,
                              conf.queue_shaper[queue].cir,
                              conf.queue_shaper[queue].mode);
                cyg_httpd_write_chunked(p->outbuffer, ct);
                if (queue_shaper_excess) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%u",
                                  conf.queue_shaper[queue].excess);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
                if (queue_shaper_credit) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%u",
                                  conf.queue_shaper[queue].credit);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%u|%u|%u",
                          conf.port_shaper.enable,
                          conf.port_shaper.cir,
                          conf.port_shaper.mode);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%u", fast_cap(MESA_CAP_QOS_EGRESS_SHAPER_FRAME));
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

#if defined(VTSS_SW_OPTION_QOS_ADV)
static i32 handler_config_qos_port_tag_remarking(CYG_HTTPD_STATE *p)
{
    vtss_isid_t     sid = web_retrieve_request_sid(p); /* Includes USID = ISID */

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/qos_port_tag_remarking.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        port_iter_t               pit;
        vtss_appl_qos_port_conf_t conf;
        int                       ct, mode;
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (vtss_appl_qos_port_conf_get(sid, pit.iport, &conf) != VTSS_RC_OK) {
                break;          /* Probably stack error - bail out */
            }

            /* Format:
             * <port 1>,<port 2>,<port 3>,...<port n>
             *
             * port x :== <port_no>#<mode>
             *   port_no       :== 1..max
             *   mode          :== 0..2              // 0: Classified, 1: Default, 2: Mapped
             */

            switch (conf.port.tag_remark_mode) {
            case VTSS_APPL_QOS_TAG_REMARK_MODE_DEFAULT:
                mode = 1;
                break;
            case VTSS_APPL_QOS_TAG_REMARK_MODE_MAPPED:
                mode = 2;
                break;
            default:
                mode = 0;
                break;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u#%u",
                          pit.first ? "" : ",",
                          pit.uport,
                          mode);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_qos_port_tag_remarking_edit(CYG_HTTPD_STATE *p)
{
    int                       ct, i, uport;
    vtss_isid_t               sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_port_no_t            iport = VTSS_PORT_NO_START;
    vtss_appl_qos_port_conf_t conf;
    char                      buf[64];

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
            if (iport >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
                errors++;
            }
        } else {
            errors++;
        }

        if (errors || vtss_appl_qos_port_conf_get(sid, iport, &conf) != VTSS_RC_OK) {
            errors++;
        } else {
            int            val;
            mesa_rc        rc;

            if (cyg_httpd_form_varable_int(p, "tr_mode", &val)) {
                switch (val) {
                case 1:
                    conf.port.tag_remark_mode = VTSS_APPL_QOS_TAG_REMARK_MODE_DEFAULT;
                    break;
                case 2:
                    conf.port.tag_remark_mode = VTSS_APPL_QOS_TAG_REMARK_MODE_MAPPED;
                    break;
                default:
                    conf.port.tag_remark_mode = VTSS_APPL_QOS_TAG_REMARK_MODE_CLASSIFIED;
                    break;
                }
            }

            // Get default PCP and DEI if mode is set to "Default"
            if (conf.port.tag_remark_mode == VTSS_APPL_QOS_TAG_REMARK_MODE_DEFAULT) {
                if (cyg_httpd_form_varable_int(p, "default_pcp", &val)) {
                    conf.port.tag_default_pcp = val;
                }
                if (cyg_httpd_form_varable_int(p, "default_dei", &val)) {
                    conf.port.tag_default_dei = val;
                }
            }

            // Get DP level and Map if mode is set to "Mapped"
            if (conf.port.tag_remark_mode == VTSS_APPL_QOS_TAG_REMARK_MODE_MAPPED) {
                for (i = 0; i < (VTSS_APPL_QOS_PORT_PRIO_CNT * 2); i++) {
                    if (cyg_httpd_form_variable_int_fmt(p, &val, "pcp_%d", i)) {
                        conf.cos_tag_map[i / 2][i % 2].pcp = val;
                    }
                    if (cyg_httpd_form_variable_int_fmt(p, &val, "dei_%d", i)) {
                        conf.cos_tag_map[i / 2][i % 2].dei = val;
                    }
                }
            }

            if ((rc = vtss_appl_qos_port_conf_set(sid, iport, &conf)) != VTSS_RC_OK) {
                T_D("vtss_appl_qos_conf_set(%u, %d): failed rc = %d", sid, iport, rc);
                errors++; /* Probably stack error */
            }
        }

        (void)snprintf(buf, sizeof(buf), "/qos_port_tag_remarking_edit.htm?port=%u", iport2uport(iport));
        redirect(p, errors ? STACK_ERR_URL : buf);
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        if (cyg_httpd_form_varable_int(p, "port", &uport)) {
            iport = uport2iport(uport);
        }
        if (iport >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)) {
            iport = VTSS_PORT_NO_START;
        }

        if (vtss_appl_qos_port_conf_get(sid, iport, &conf) == VTSS_RC_OK) {

            /* Format:
             * <port_no>#<mode>#<default_params>#<map>
             *
             * port_no          :== 1..max
             * mode             :== 0..2       // 0: Classified, 1: Default, 2: Mapped
             * default_params   :== <pcp|dei>
             *   pcp            :== 0..7
             *   dei            :== 0..1
             * map              :== <entry_0>/<entry_1>/...<entry_n> // n is 15.
             *   entry_x        :== <pcp|dei>
             *     pcp          :== 0..7
             *     dei          :== 0..1
             *
             * The map is organized as follows:
             * Entry corresponds to QoS class, DP level
             *  0                   0          0
             *  1                   0          1
             *  2                   1          0
             *  3                   1          1
             *  4                   2          0
             *  5                   2          1
             *  6                   3          0
             *  7                   3          1
             *  8                   4          0
             *  9                   4          1
             * 10                   5          0
             * 11                   5          1
             * 12                   6          0
             * 13                   6          1
             * 14                   7          0
             * 15                   7          1
             */
            int mode;
            switch (conf.port.tag_remark_mode) {
            case VTSS_APPL_QOS_TAG_REMARK_MODE_DEFAULT:
                mode = 1;
                break;
            case VTSS_APPL_QOS_TAG_REMARK_MODE_MAPPED:
                mode = 2;
                break;
            default:
                mode = 0;
                break;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#%u#%u|%u",
                          iport2uport(iport),
                          mode,
                          conf.port.tag_default_pcp,
                          conf.port.tag_default_dei);
            cyg_httpd_write_chunked(p->outbuffer, ct);

            for (i = 0; i < (VTSS_APPL_QOS_PORT_PRIO_CNT * 2); i++) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u|%u",
                              (i != 0) ? "/" : "#",
                              conf.cos_tag_map[i / 2][i % 2].pcp,
                              conf.cos_tag_map[i / 2][i % 2].dei);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

static i32 handler_config_qos_wred(CYG_HTTPD_STATE *p)
{
    int                  ct, group, queue, dpl;
    vtss_appl_qos_conf_t conf, newconf;
    vtss_appl_qos_wred_t *wred;

    if (!CAPA->has_wred2_or_wred3) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int errors = 0;

        if (vtss_appl_qos_conf_get(&conf) != VTSS_RC_OK) {
            errors++;   /* Probably stack error */
        } else {
            newconf = conf;

            for (group = 0; group < CAPA->wred_group_max; group++) {
                for (queue = 0; queue < VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
                    for (dpl = 0; dpl < CAPA->wred_dpl_max; dpl++) {
                        int val, idx;
                        idx = (group * VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_CNT * CAPA->wred_dpl_max) + (queue * CAPA->wred_dpl_max) + dpl;
                        wred = &newconf.wred[queue][dpl][group];
                        wred->enable = cyg_httpd_form_variable_check_fmt(p, "enable_%d", idx);
                        if (cyg_httpd_form_variable_int_fmt(p, &val, "min_%d", idx)) {
                            wred->min = val;
                        }
                        if (cyg_httpd_form_variable_int_fmt(p, &val, "max_%d", idx)) {
                            wred->max = val;
                        }
                        if (cyg_httpd_form_variable_int_fmt(p, &val, "max_unit_%d", idx)) {
                            wred->max_unit = (vtss_appl_qos_wred_max_t)val;
                        }
                    }
                }
            }

            if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                mesa_rc rc;
                if ((rc = vtss_appl_qos_conf_set(&newconf)) != VTSS_RC_OK) {
                    T_D("qos_conf_set: failed rc = %d", rc);
                    errors++; /* Probably stack error */
                }
            }
        }
        redirect(p, errors ? STACK_ERR_URL : "/qos_wred.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");

        if (vtss_appl_qos_conf_get(&conf) == VTSS_RC_OK) {
            BOOL first = TRUE;
            char buf[32];

            buf[0] = '\0';
            for (group = 0; group < CAPA->wred_group_max; group++) {
                for (queue = 0; queue < VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_CNT; queue++) {
                    for (dpl = 0; dpl < CAPA->wred_dpl_max; dpl++) {
                        /*
                         * Format V2/V3:
                         *
                         * wred_config :== <entry 0>,<entry 1>,<entry 2>,...<entry n>
                         *
                         *  entry x (V2):== <enable>#<min>#<max>#<max_unit>
                         *  entry x (V3):== <group>#<queue>#<dpl>#<enable>#<min>#<max>#<max_unit>
                         *
                         *   group (V3) :== 0..VTSS_APPL_QOS_WRED_GROUP_CNT - 1
                         *   queue (V3) :== 0..VTSS_APPL_QOS_PORT_WEIGHTED_QUEUE_CNT - 1
                         *   dpl   (V3) :== 0..VTSS_APPL_QOS_WRED_DPL_CNT - 1
                         *   enable     :== 0..1
                         *   min        :== 0..100
                         *   max        :== 1..100
                         *   max_unit   :== 0..1   // 0: unit for max is 'drop probability', 1: unit for max is 'fill level'
                         */
                        if (CAPA->wred_group_max > 1) {
                            sprintf(buf, "%d#%d#%d#", group, queue, dpl);
                        }
                        wred = &conf.wred[queue][dpl][group];
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s%d#%u#%u#%u",
                                      (!!first) ? "" : ",",
                                      buf,
                                      wred->enable,
                                      wred->min,
                                      wred->max,
                                      wred->max_unit);
                        first = FALSE;
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                    }
                }
            }
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

#if defined(VTSS_SW_OPTION_QOS_ADV)
static i32 handler_config_qos_ingress_map(CYG_HTTPD_STATE *p)
{
    int                              ct, count = 0;
    int                              map_flag = 0, var_value = 0;
    vtss_appl_qos_ingress_map_conf_t conf;
    mesa_qos_ingress_map_id_t        id;

    if (!CAPA->has_ingress_map) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_GET) {
        // Format       : [mapConfigFlag]/[selectMapId]
        // <Edit>       :               1/[map_id]
        // <Delete>     :               2/[map_id]
        // <Delete All> :               3/
        // <Add New>    :               4/

        if (cyg_httpd_form_varable_int(p, "mapConfigFlag", &map_flag)) {
            switch (map_flag) {
            case 1:
            case 2:
                if (cyg_httpd_form_varable_int(p, "selectMapId", &var_value)) {
                    id = (mesa_qos_ingress_map_id_t) var_value;
                    if (map_flag == 2) {
                        (void)vtss_appl_qos_ingress_map_conf_del(id);
                    }
                }
                break;
            case 3:
                while (vtss_appl_qos_ingress_map_conf_itr(count ? &id : NULL, &id) == VTSS_RC_OK) {
                    if (vtss_appl_qos_ingress_map_conf_get(id, &conf) == VTSS_RC_OK) {
                        (void)vtss_appl_qos_ingress_map_conf_del(id);
                    }
                    count++;
                }
                count = 0;
                break;
            default:
                break;
            }
        }

        (void)cyg_httpd_start_chunked("html");
        while (vtss_appl_qos_ingress_map_conf_itr(count ? &id : NULL, &id) == VTSS_RC_OK) {
            if (vtss_appl_qos_ingress_map_conf_get(id, &conf) == VTSS_RC_OK) {
                /*
                 * Format:
                 * maps       :== <map 1>;<map 2>;<map 3>;<map 4>;...<map n> // List of currentloy defined maps (might be empty)
                 *   map x    :== <id>/<key>/<action>
                 *     id     :== 0..VTSS_APPL_QOS_INGRESS_MAP_MAX
                 *     key    :== 0..3                                       // vtss_appl_qos_ingress_map_key_t
                 *     action :== <cos/dpl/pcp/dei/dscp/cosid>               // Array of BOOL
                 */
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              "%s%u/%d/%d/%d/%d/%d/%d/%d",
                              count ? ";" : "",
                              id,
                              conf.key,
                              conf.action.cos,
                              conf.action.dpl,
                              conf.action.pcp,
                              conf.action.dei,
                              conf.action.dscp,
                              conf.action.cosid);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            count++;
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_qos_ingress_map_edit(CYG_HTTPD_STATE *p)
{
    int                              ct;
    int                              map_flag = 0, var_value = 0;
    mesa_qos_ingress_map_id_t        map_id = 0;
    vtss_appl_qos_ingress_map_conf_t conf;
    u32                              error = 0;
    char                             buf[64];

    if (!CAPA->has_ingress_map) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        vtss_appl_qos_ingress_map_conf_t newconf;
        BOOL                             new_entry = FALSE;

        if (cyg_httpd_form_varable_int(p, "map_id", &var_value)) {
            map_id = (mesa_qos_ingress_map_id_t) var_value;

            memset(&conf, 0, sizeof(conf));
            new_entry = vtss_appl_qos_ingress_map_conf_get(map_id, &conf) != VTSS_RC_OK ? TRUE : FALSE;
            newconf = conf;

            // Map Key
            if (cyg_httpd_form_varable_int(p, "map_key", &var_value)) {
                newconf.key = (vtss_appl_qos_ingress_map_key_t) var_value;
            }

            // Map Action(s)
            if (cyg_httpd_form_varable_int(p, "act_cos_sel", &var_value)) {
                newconf.action.cos = (BOOL) var_value;
            }
            if (cyg_httpd_form_varable_int(p, "act_dpl_sel", &var_value)) {
                newconf.action.dpl = (BOOL) var_value;
            }
            if (cyg_httpd_form_varable_int(p, "act_pcp_sel", &var_value)) {
                newconf.action.pcp = (BOOL) var_value;
            }
            if (cyg_httpd_form_varable_int(p, "act_dei_sel", &var_value)) {
                newconf.action.dei = (BOOL) var_value;
            }
            if (cyg_httpd_form_varable_int(p, "act_dscp_sel", &var_value)) {
                newconf.action.dscp = (BOOL) var_value;
            }
            if (cyg_httpd_form_varable_int(p, "act_cosid_sel", &var_value)) {
                newconf.action.cosid = (BOOL) var_value;
            }

            // Save new configuration
            if (new_entry) {
                T_D("Calling vtss_appl_qos_ingress_map_conf_add(%u)", map_id);
                if (vtss_appl_qos_ingress_map_conf_add(map_id, &newconf) != VTSS_RC_OK) {
                    T_I("vtss_appl_qos_ingress_map_conf_add(%u): failed", map_id);
                    error = 1;
                }
            } else if (memcmp(&newconf, &conf, sizeof(newconf))) {
                T_D("Calling vtss_appl_qos_ingress_map_conf_set(%u)", map_id);
                if (vtss_appl_qos_ingress_map_conf_set(map_id, &newconf) != VTSS_RC_OK) {
                    T_I("vtss_appl_qos_ingress_map_conf_set(%u): failed", map_id);
                    error = 2;
                }
            }
        }

        (void)snprintf(buf, sizeof(buf), "/qos_ingress_map.htm?error=%u", error);
        redirect(p, buf);

    } else {
        // Format    : [mapEditFlag]/[selectMapId]
        // <Edit>    :             3/[map_id]
        // <Add New> :             4/0

        if (cyg_httpd_form_varable_int(p, "mapEditFlag", &map_flag)) {
            switch (map_flag) {
            case 3:
                if (cyg_httpd_form_varable_int(p, "selectMapId", &var_value)) {
                    map_id = (mesa_qos_ingress_map_id_t) var_value;
                }
                break;
            default:
                break;
            }
        }

        /*
         * Format:
         *   map x    :== <id>/<key>/<action>
         *     id     :== 0..VTSS_APPL_QOS_INGRESS_MAP_MAX
         *     key    :== 0..3                                       // vtss_appl_qos_ingress_map_key_t
         *     action :== <cos/dpl/pcp/dei/dscp/cosid>               // Array of BOOL
         */

        (void) cyg_httpd_start_chunked("html");
        if ((map_flag == 3 && vtss_appl_qos_ingress_map_conf_get(map_id, &conf) == VTSS_RC_OK)) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "%u/%d/%d/%d/%d/%d/%d/%d",
                          map_id,
                          conf.key,
                          conf.action.cos,
                          conf.action.dpl,
                          conf.action.pcp,
                          conf.action.dei,
                          conf.action.dscp,
                          conf.action.cosid);
            (void) cyg_httpd_write_chunked(p->outbuffer, ct);
        } else {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "0/0/0/0/0/0/0/0/0");
            (void) cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void) cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_qos_egress_map(CYG_HTTPD_STATE *p)
{
    int                              ct, count = 0;
    int                              map_flag = 0, var_value = 0;
    vtss_appl_qos_egress_map_conf_t conf;
    mesa_qos_egress_map_id_t        id;

    if (!CAPA->has_egress_map) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_GET) {
        // Format       : [mapConfigFlag]/[selectMapId]
        // <Edit>       :               1/[map_id]
        // <Delete>     :               2/[map_id]
        // <Delete All> :               3/
        // <Add New>    :               4/

        if (cyg_httpd_form_varable_int(p, "mapConfigFlag", &map_flag)) {
            switch (map_flag) {
            case 1:
            case 2:
                if (cyg_httpd_form_varable_int(p, "selectMapId", &var_value)) {
                    id = (mesa_qos_egress_map_id_t) var_value;
                    if (map_flag == 2) {
                        (void)vtss_appl_qos_egress_map_conf_del(id);
                    }
                }
                break;
            case 3:
                while (vtss_appl_qos_egress_map_conf_itr(count ? &id : NULL, &id) == VTSS_RC_OK) {
                    if (vtss_appl_qos_egress_map_conf_get(id, &conf) == VTSS_RC_OK) {
                        (void)vtss_appl_qos_egress_map_conf_del(id);
                    }
                    count++;
                }
                count = 0;
                break;
            default:
                break;
            }
        }

        (void)cyg_httpd_start_chunked("html");
        while (vtss_appl_qos_egress_map_conf_itr(count ? &id : NULL, &id) == VTSS_RC_OK) {
            if (vtss_appl_qos_egress_map_conf_get(id, &conf) == VTSS_RC_OK) {
                /*
                 * Format:
                 * maps       :== <map 1>;<map 2>;<map 3>;<map 4>;...<map n> // List of currentloy defined maps (might be empty)
                 *   map x    :== <id>/<key>/<action>
                 *     id     :== 0..VTSS_APPL_QOS_INGRESS_MAP_MAX
                 *     key    :== 0..3                                       // vtss_appl_qos_ingress_map_key_t
                 *     action :== <cos/dpl/pcp/dei/dscp/cosid>               // Array of BOOL
                 */
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              "%s%u/%d/%d/%d/%d",
                              count ? ";" : "",
                              id,
                              conf.key,
                              conf.action.pcp,
                              conf.action.dei,
                              conf.action.dscp);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            count++;
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_qos_egress_map_edit(CYG_HTTPD_STATE *p)
{
    int                              ct;
    int                              map_flag = 0, var_value = 0;
    mesa_qos_egress_map_id_t        map_id = 0;
    vtss_appl_qos_egress_map_conf_t conf;
    u32                             error = 0;
    char                            buf[64];

    if (!CAPA->has_egress_map) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        vtss_appl_qos_egress_map_conf_t newconf;
        BOOL                            new_entry = FALSE;

        if (cyg_httpd_form_varable_int(p, "map_id", &var_value)) {
            map_id = (mesa_qos_egress_map_id_t) var_value;

            memset(&conf, 0, sizeof(conf));
            new_entry = vtss_appl_qos_egress_map_conf_get(map_id, &conf) != VTSS_RC_OK ? TRUE : FALSE;
            newconf = conf;

            // Map Key
            if (cyg_httpd_form_varable_int(p, "map_key", &var_value)) {
                newconf.key = (vtss_appl_qos_egress_map_key_t) var_value;
            }

            // Map Action(s)
            if (cyg_httpd_form_varable_int(p, "act_pcp_sel", &var_value)) {
                newconf.action.pcp = (BOOL) var_value;
            }
            if (cyg_httpd_form_varable_int(p, "act_dei_sel", &var_value)) {
                newconf.action.dei = (BOOL) var_value;
            }
            if (cyg_httpd_form_varable_int(p, "act_dscp_sel", &var_value)) {
                newconf.action.dscp = (BOOL) var_value;
            }

            // Save new configuration
            if (new_entry) {
                T_D("Calling vtss_appl_qos_egress_map_conf_add(%u)", map_id);
                if (vtss_appl_qos_egress_map_conf_add(map_id, &newconf) != VTSS_RC_OK) {
                    T_I("vtss_appl_qos_egress_map_conf_add(%u): failed", map_id);
                    error = 1;
                }
            } else if (memcmp(&newconf, &conf, sizeof(newconf))) {
                T_D("Calling vtss_appl_qos_egress_map_conf_set(%u)", map_id);
                if (vtss_appl_qos_egress_map_conf_set(map_id, &newconf) != VTSS_RC_OK) {
                    T_I("vtss_appl_qos_egress_map_conf_set(%u): failed", map_id);
                    error = 2;
                }
            }
        }

        (void)snprintf(buf, sizeof(buf), "/qos_egress_map.htm?error=%u", error);
        redirect(p, buf);

    } else {
        // Format    : [mapEditFlag]/[selectMapId]
        // <Edit>    :             3/[map_id]
        // <Add New> :             4/0

        if (cyg_httpd_form_varable_int(p, "mapEditFlag", &map_flag)) {
            switch (map_flag) {
            case 3:
                if (cyg_httpd_form_varable_int(p, "selectMapId", &var_value)) {
                    map_id = (mesa_qos_egress_map_id_t) var_value;
                }
                break;
            default:
                break;
            }
        }

        /*
         * Format:
         *   map x    :== <id>/<key>/<action>
         *     id     :== 0..VTSS_APPL_QOS_INGRESS_MAP_MAX
         *     key    :== 0..3                                       // vtss_appl_qos_ingress_map_key_t
         *     action :== <cos/dpl/pcp/dei/dscp/cosid>               // Array of BOOL
         */

        (void) cyg_httpd_start_chunked("html");
        if ((map_flag == 3 && vtss_appl_qos_egress_map_conf_get(map_id, &conf) == VTSS_RC_OK)) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "%u/%d/%d/%d/%d",
                          map_id,
                          conf.key,
                          conf.action.pcp,
                          conf.action.dei,
                          conf.action.dscp);
            (void) cyg_httpd_write_chunked(p->outbuffer, ct);
        } else {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "0/0/0/0/0/0");
            (void) cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void) cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_qos_map_rule(CYG_HTTPD_STATE *p)
{
    int                                ct, count = 0, i, j;
    int                                map_type = 0, map_id = 0, var_value = 0, rule_flag = 0;
    mesa_qos_ingress_map_id_t          ing_id = 0;
    mesa_qos_egress_map_id_t           eg_id = 0;
    vtss_appl_qos_ingress_map_conf_t   ing_conf;
    vtss_appl_qos_egress_map_conf_t    eg_conf;
    vtss_appl_qos_ingress_map_values_t ingConf;
    vtss_appl_qos_egress_map_values_t  egConf;
    u32                                dpl_cnt = fast_cap(MESA_CAP_QOS_DPL_CNT);

    if (!(CAPA->has_ingress_map || CAPA->has_egress_map)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_GET) {
        // Format       : [selectMapType]/[selectMapId]/[ruleConfigFlag]/[selectRule1]/[selectRule2]
        // <Edit>       :                /     [map_id]/               1/   [rule_id1]/   [rule_id2]
        // <Delete>     :                /     [map_id]/               2/   [rule_id1]/   [rule_id2]
        // <Delete All> :                /     [map_id]/               3/   [rule_id1]/   [rule_id2]
        // <Add New>    :                /     [map_id]/               4/   [rule_id1]/   [rule_id2]
        (void) cyg_httpd_form_varable_int(p, "selectMapType", &map_type);
        (void) cyg_httpd_form_varable_int(p, "selectMapId", &map_id);

        if (cyg_httpd_form_varable_int(p, "ruleConfigFlag", &rule_flag)) {
            switch (rule_flag) {
            case 1:
            case 2:
                if (cyg_httpd_form_varable_int(p, "selectRuleId1", &var_value)) {
                    if (map_type == 0) {
                        mesa_tagprio_t pcp;
                        mesa_dei_t     dei;
                        pcp = var_value % 8;
                        dei = var_value / 8;
                        ing_id = (mesa_qos_ingress_map_id_t) map_id;
                        memset(&ingConf, 0, sizeof(ingConf));
                        if (rule_flag == 2) {
                            (void)vtss_appl_qos_ingress_map_pcp_dei_conf_set(ing_id, pcp, dei, &ingConf);
                        }
                    } else {
                        mesa_cosid_t cosid;
                        mesa_dpl_t   dpl;
                        cosid = var_value % 8 ;
                        dpl = var_value / 8;
                        eg_id = (mesa_qos_egress_map_id_t) map_id;
                        memset(&egConf, 0, sizeof(egConf));
                        if (rule_flag == 2) {
                            (void)vtss_appl_qos_egress_map_cosid_dpl_conf_set(eg_id, cosid, dpl, &egConf);
                        }
                    }
                }
                if (cyg_httpd_form_varable_int(p, "selectRuleId2", &var_value)) {
                    if (map_type == 0) {
                        mesa_dscp_t dscp;
                        dscp = var_value;
                        ing_id = (mesa_qos_ingress_map_id_t) map_id;
                        memset(&ingConf, 0, sizeof(ingConf));
                        if (rule_flag == 2) {
                            (void)vtss_appl_qos_ingress_map_dscp_conf_set(ing_id, dscp, &ingConf);
                        }
                    } else {
                        mesa_dscp_t dscp;
                        mesa_dpl_t  dpl;
                        dscp = var_value % 64;
                        dpl = var_value / 64;
                        eg_id = (mesa_qos_egress_map_id_t) map_id;
                        memset(&egConf, 0, sizeof(egConf));
                        if (rule_flag == 2) {
                            (void)vtss_appl_qos_egress_map_dscp_dpl_conf_set(eg_id, dscp, dpl, &egConf);
                        }
                    }
                }
                break;
            case 3:
                if (map_type == 0) {
                    ing_id = (mesa_qos_ingress_map_id_t) map_id;
                    memset(&ingConf, 0, sizeof(ingConf));
                    for (i = 0; i < VTSS_PCPS; i++) {
                        for (j = 0; j < VTSS_DEIS; j++) {
                            (void)vtss_appl_qos_ingress_map_pcp_dei_conf_set(ing_id, i, j, &ingConf);
                        }
                    }
                    for (i = 0; i < 64; i++) {
                        (void)vtss_appl_qos_ingress_map_dscp_conf_set(ing_id, i, &ingConf);
                    }
                } else {
                    eg_id = (mesa_qos_egress_map_id_t) map_id;
                    memset(&egConf, 0, sizeof(egConf));
                    for (i = 0; i < VTSS_COSIDS; i++) {
                        for (j = 0; j < dpl_cnt; j++) {
                            (void)vtss_appl_qos_egress_map_cosid_dpl_conf_set(eg_id, i, j, &egConf);
                        }
                    }
                    for (i = 0; i < 64; i++) {
                        for (j = 0; j < dpl_cnt; j++) {
                            (void)vtss_appl_qos_egress_map_dscp_dpl_conf_set(eg_id, i, j, &egConf);
                        }
                    }
                }
                break;
            default:
                break;
            }
        }

        (void)cyg_httpd_start_chunked("html");
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "%d#%u#",
                      map_type,
                      map_id);
        (void) cyg_httpd_write_chunked(p->outbuffer, ct);
        /* Format:
         * info           :== <map_type>#<map_id>#<map_list>#<rules>
         *   rules        :== <rule>|<rule>
         *     rule       :== <rule 1>;<rule 2>;<rule 3>;<rule 4>;...<rule n> // List of currentloy defined rules (might be empty)
         *       rule x   :== <key>/<action>
         *         key    :== <par1>/<par2> (pcp-dei, dscp, cosid-dpl, dscp-dpl)
         *         action :== <act1>/<act2>/...  (cos/dpl/pcp/dei/dscp/cosid OR pcp/dei/dscp)
         *
         * map_type           :== 0..1 // Ingress, Egress
         * map_id             :== 0..VTSS_APPL_QOS_INGRESS_MAP_MAX or 0..VTSS_APPL_QOS_EGRESS_MAP_MAX
         * map_list           :== 0/1/...n       // List of In/Egress Map IDs
         */
        if (map_type == 0) { // Ingress Map
            vtss_appl_qos_imap_entry_t c, dc;

            while (vtss_appl_qos_ingress_map_conf_itr(count ? &ing_id : NULL, &ing_id) == VTSS_RC_OK) {
                if (vtss_appl_qos_ingress_map_conf_get(ing_id, &ing_conf) == VTSS_RC_OK) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                  "%s%u",
                                  count ? "/" : "",
                                  ing_id);
                    (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                }
                count++;
            }
            count = 0;
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#");
            ing_id = (mesa_qos_ingress_map_id_t) map_id;
            (void) cyg_httpd_write_chunked(p->outbuffer, ct);
            if ((vtss_appl_qos_imap_entry_get(ing_id, &c) == VTSS_RC_OK) &&
                (vtss_appl_qos_imap_entry_get_default(ing_id, &dc) == VTSS_RC_OK)) {
                for (i = 0; i < VTSS_PCPS; i++) {
                    for (j = 0; j < VTSS_DEIS; j++) {
                        if (memcmp(&c.pcp_dei[i][j], &dc.pcp_dei[i][j], sizeof(c.pcp_dei[i][j]))) {
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                          "%s%d/%d/%d/%d/%d/%d/%d/%d",
                                          count ? ";" : "",
                                          i,
                                          j,
                                          c.pcp_dei[i][j].cos,
                                          c.pcp_dei[i][j].dpl,
                                          c.pcp_dei[i][j].pcp,
                                          c.pcp_dei[i][j].dei,
                                          c.pcp_dei[i][j].dscp,
                                          c.pcp_dei[i][j].cosid);
                            (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                            count++;
                        }
                    }
                }
                count = 0;
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|");
                (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                for (i = 0; i < 64; i++) {
                    if (memcmp(&c.dscp[i], &dc.dscp[i], sizeof(c.dscp[i]))) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                      "%s%d/%d/%d/%d/%d/%d/%d",
                                      count ? ";" : "",
                                      i,
                                      c.dscp[i].cos,
                                      c.dscp[i].dpl,
                                      c.dscp[i].pcp,
                                      c.dscp[i].dei,
                                      c.dscp[i].dscp,
                                      c.dscp[i].cosid);
                        (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                        count++;
                    }
                }
            }
        } else { // Egress Map
            vtss_appl_qos_emap_entry_t c, dc;
            while (vtss_appl_qos_egress_map_conf_itr(count ? &eg_id : NULL, &eg_id) == VTSS_RC_OK) {
                if (vtss_appl_qos_egress_map_conf_get(eg_id, &eg_conf) == VTSS_RC_OK) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                  "%s%u",
                                  count ? "/" : "",
                                  eg_id);
                    (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                }
                count++;
            }
            count = 0;
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#");
            eg_id = (mesa_qos_egress_map_id_t) map_id;
            (void) cyg_httpd_write_chunked(p->outbuffer, ct);
            if ((vtss_appl_qos_emap_entry_get(eg_id, &c) == VTSS_RC_OK) &&
                (vtss_appl_qos_emap_entry_get_default(eg_id, &dc) == VTSS_RC_OK)) {
                for (i = 0; i < VTSS_COSIDS; i++) {
                    for (j = 0; j < dpl_cnt; j++) {
                        if (memcmp(&c.cosid_dpl[i][j], &dc.cosid_dpl[i][j], sizeof(c.cosid_dpl[i][j]))) {
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                          "%s%d/%d/%d/%d/%d",
                                          count ? ";" : "",
                                          i,
                                          j,
                                          c.cosid_dpl[i][j].pcp,
                                          c.cosid_dpl[i][j].dei,
                                          c.cosid_dpl[i][j].dscp);
                            (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                            count++;
                        }
                    }
                }
                count = 0;
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|");
                (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                for (i = 0; i < 64; i++) {
                    for (j = 0; j < dpl_cnt; j++) {
                        if (memcmp(&c.dscp_dpl[i][j], &dc.dscp_dpl[i][j], sizeof(c.dscp_dpl[i][j]))) {
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                          "%s%d/%d/%d/%d/%d",
                                          count ? ";" : "",
                                          i,
                                          j,
                                          c.dscp_dpl[i][j].pcp,
                                          c.dscp_dpl[i][j].dei,
                                          c.dscp_dpl[i][j].dscp);
                            (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                            count++;
                        }
                    }
                }
            }
        }
        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static mesa_rc qos_ingress_map_rule_action_get(CYG_HTTPD_STATE *p, vtss_appl_qos_ingress_map_values_t *conf)
{
    int var_value = 0;

    // Map Action(s)
    if (cyg_httpd_form_varable_int(p, "act_cos_sel", &var_value)) {
        conf->cos = (mesa_cos_t) var_value;
    }
    if (cyg_httpd_form_varable_int(p, "act_dpl_sel", &var_value)) {
        conf->dpl = (mesa_dpl_t) var_value;
    }
    if (cyg_httpd_form_varable_int(p, "act_pcp_sel", &var_value)) {
        conf->pcp = (mesa_pcp_t) var_value;
    }
    if (cyg_httpd_form_varable_int(p, "act_dei_sel", &var_value)) {
        conf->dei = (mesa_dei_t) var_value;
    }
    if (cyg_httpd_form_varable_int(p, "act_dscp_sel", &var_value)) {
        conf->dscp = (mesa_dscp_t) var_value;
    }
    if (cyg_httpd_form_varable_int(p, "act_cosid_sel", &var_value)) {
        conf->cosid = (mesa_cosid_t) var_value;
    }

    return VTSS_RC_OK;
}

static mesa_rc qos_egress_map_rule_action_get(CYG_HTTPD_STATE *p, vtss_appl_qos_egress_map_values_t *conf)
{
    int var_value = 0;

    // Map Action(s)
    if (cyg_httpd_form_varable_int(p, "act_pcp_sel", &var_value)) {
        conf->pcp = (mesa_pcp_t) var_value;
    }
    if (cyg_httpd_form_varable_int(p, "act_dei_sel", &var_value)) {
        conf->dei = (mesa_dei_t) var_value;
    }
    if (cyg_httpd_form_varable_int(p, "act_dscp_sel", &var_value)) {
        conf->dscp = (mesa_dscp_t) var_value;
    }

    return VTSS_RC_OK;
}

static i32 handler_config_qos_map_rule_edit(CYG_HTTPD_STATE *p)
{
    int                                ct, map_id = 0, keyType = 0, map_type = 0;
    int                                rule_flag = 0, var_value = 0;
    mesa_qos_ingress_map_id_t          ing_id = 0;
    mesa_qos_ingress_map_id_t          eg_id = 0;
    vtss_appl_qos_ingress_map_values_t ingConf;
    vtss_appl_qos_egress_map_values_t  egConf;
    mesa_tagprio_t                     pcp = 0;
    mesa_dei_t                         dei = 0;
    mesa_dscp_t                        dscp = 0;
    mesa_cosid_t                       cosid = 0;
    mesa_dpl_t                         dpl = 0;
    char                               buf[64];

    if (!(CAPA->has_ingress_map || CAPA->has_egress_map)) {
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_QOS)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        (void) cyg_httpd_form_varable_int(p, "selectMapType", &map_type);
        (void) cyg_httpd_form_varable_int(p, "selectMapId", &map_id);
        if (cyg_httpd_form_varable_int(p, "selectKeyType", &var_value)) {
            keyType = var_value;

            if (keyType == 1) {
                if (map_type == 0) {
                    ing_id = (mesa_qos_ingress_map_id_t) map_id;
                    // Rule Key
                    if (cyg_httpd_form_varable_int(p, "rule_key1", &var_value)) {
                        pcp = (mesa_tagprio_t) var_value;
                    }
                    if (cyg_httpd_form_varable_int(p, "rule_key2", &var_value)) {
                        dei = (mesa_dei_t) var_value;
                    }

                    // Rule Action
                    memset(&ingConf, 0, sizeof(ingConf));
                    VTSS_RC(qos_ingress_map_rule_action_get(p, &ingConf));
                    // Save new configuration
                    T_D("Calling vtss_appl_qos_ingress_map_pcp_dei_conf_set(%u, %u)", pcp, dei);
                    if (vtss_appl_qos_ingress_map_pcp_dei_conf_set(ing_id, pcp, dei, &ingConf) != VTSS_RC_OK) {
                        T_W("Calling vtss_appl_qos_ingress_map_pcp_dei_conf_set(%u, %u): failed", pcp, dei);
                    }
                } else {
                    eg_id = (mesa_qos_egress_map_id_t) map_id;
                    // Rule Key
                    if (cyg_httpd_form_varable_int(p, "rule_key1", &var_value)) {
                        cosid = (mesa_cosid_t) var_value;
                    }
                    if (cyg_httpd_form_varable_int(p, "rule_key2", &var_value)) {
                        dpl = (mesa_dpl_t) var_value;
                    }

                    // Rule Action
                    memset(&egConf, 0, sizeof(egConf));
                    VTSS_RC(qos_egress_map_rule_action_get(p, &egConf));
                    // Save new configuration
                    T_D("Calling vtss_appl_qos_egress_map_cosid_dpl_conf_set(%u, %u)", cosid, dpl);
                    if (vtss_appl_qos_egress_map_cosid_dpl_conf_set(eg_id, cosid, dpl, &egConf) != VTSS_RC_OK) {
                        T_W("Calling vtss_appl_qos_egress_map_cosid_dpl_conf_set(%u, %u): failed", cosid, dpl);
                    }
                }
            } else if (keyType == 2) {
                if (map_type == 0) {
                    ing_id = (mesa_qos_ingress_map_id_t) map_id;
                    // Rule Key
                    if (cyg_httpd_form_varable_int(p, "rule_key1", &var_value)) {
                        dscp = (mesa_dscp_t) var_value;
                    }

                    // Rule Action
                    memset(&ingConf, 0, sizeof(ingConf));
                    VTSS_RC(qos_ingress_map_rule_action_get(p, &ingConf));
                    // Save new configuration
                    T_D("Calling vtss_appl_qos_ingress_map_dscp_conf_set(%u)", dscp);
                    if (vtss_appl_qos_ingress_map_dscp_conf_set(ing_id, dscp, &ingConf) != VTSS_RC_OK) {
                        T_W("Calling vtss_appl_qos_ingress_map_dscp_conf_set(%u): failed", dscp);
                    }
                } else {
                    eg_id = (mesa_qos_egress_map_id_t) map_id;
                    // Rule Key
                    if (cyg_httpd_form_varable_int(p, "rule_key1", &var_value)) {
                        dscp = (mesa_dscp_t) var_value;
                    }
                    if (cyg_httpd_form_varable_int(p, "rule_key2", &var_value)) {
                        dpl = (mesa_dpl_t) var_value;
                    }

                    // Rule Action
                    memset(&egConf, 0, sizeof(egConf));
                    VTSS_RC(qos_egress_map_rule_action_get(p, &egConf));
                    // Save new configuration
                    T_D("Calling vtss_appl_qos_egress_map_dscp_dpl_conf_set(%u, %u)", dscp, dpl);
                    if (vtss_appl_qos_egress_map_dscp_dpl_conf_set(eg_id, dscp, dpl, &egConf) != VTSS_RC_OK) {
                        T_W("Calling vtss_appl_qos_egress_map_dscp_dpl_conf_set(%u, %u): failed", dscp, dpl);
                    }
                }
            }
        }

        (void)snprintf(buf, sizeof(buf), "/qos_map_rule.htm?selectMapType=%u&selectMapId=%u", map_type, map_id);
        redirect(p, buf);
    } else {
        // Format       : [selectMapType]/[selectMapId]/[ruleEditFlag]/[selectKeyType]/[selectRuleId1]/[selectRuleId2]
        // <Edit>       :                /     [map_id]/             3/                     [rule_id1]/     [rule_id2]
        // <Add New>    :                /     [map_id]/             4/


        (void) cyg_httpd_form_varable_int(p, "selectMapType", &map_type);
        (void) cyg_httpd_form_varable_int(p, "selectMapId", &map_id);

        if (cyg_httpd_form_varable_int(p, "ruleEditFlag", &rule_flag)) {
            switch (rule_flag) {
            case 3:
                if (cyg_httpd_form_varable_int(p, "selectRuleId1", &var_value)) {
                    keyType = 1;
                    if (map_type == 0) {
                        pcp = var_value % 8;
                        dei = var_value / 8;
                    } else {
                        cosid = var_value % 8;
                        dpl = var_value / 8;
                    }
                }
                if (cyg_httpd_form_varable_int(p, "selectRuleId2", &var_value)) {
                    keyType = 2;
                    if (map_type == 0) {
                        dscp = var_value;
                    } else {
                        dscp = var_value % 64;
                        dpl = var_value / 64;
                    }
                }
                break;
            case 4:
                if (cyg_httpd_form_varable_int(p, "selectKeyType", &var_value)) {
                    keyType = var_value;
                }
                break;
            default:
                break;
            }
        }


        /* Format:
         * info       :== <map_type>#<map_id>#<key_type>#<rule x>
         *   rule x   :== <key>/<action>
         *     key    :== <par1>/<par2> (pcp-dei, dscp, cosid-dpl, dscp-dpl)
         *     action :== <act1>/<act2>/...  (cos/dpl/pcp/dei/dscp/cosid OR pcp/dei/dscp)
         *
         * map_type           :== 0..1 // Ingress, Egress
         * map_id             :== 0..VTSS_APPL_QOS_INGRESS_MAP_MAX or 0..VTSS_APPL_QOS_EGRESS_MAP_MAX
         * key_type           :== 0..1 // Par1 or Par2
         */

        (void) cyg_httpd_start_chunked("html");
        if (rule_flag == 3) {
            if (keyType == 1) {
                if (map_type == 0) {
                    ing_id = (mesa_qos_ingress_map_id_t) map_id;
                    memset(&ingConf, 0, sizeof(ingConf));
                    if (vtss_appl_qos_ingress_map_pcp_dei_conf_get(ing_id, pcp, dei, &ingConf) == VTSS_RC_OK) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                      "0#%u#1#%d/%d/%d/%d/%d/%d/%d/%d",
                                      map_id,
                                      pcp,
                                      dei,
                                      ingConf.cos,
                                      ingConf.dpl,
                                      ingConf.pcp,
                                      ingConf.dei,
                                      ingConf.dscp,
                                      ingConf.cosid);
                        (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                    }
                } else {
                    eg_id = (mesa_qos_egress_map_id_t) map_id;
                    memset(&egConf, 0, sizeof(egConf));
                    if (vtss_appl_qos_egress_map_cosid_dpl_conf_get(eg_id, cosid, dpl, &egConf) == VTSS_RC_OK) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                      "1#%u#1#%d/%d/%d/%d/%d",
                                      map_id,
                                      cosid,
                                      dpl,
                                      egConf.pcp,
                                      egConf.dei,
                                      egConf.dscp);
                        (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                    }
                }
            } else if (keyType == 2) {
                if (map_type == 0) {
                    ing_id = (mesa_qos_ingress_map_id_t) map_id;
                    memset(&ingConf, 0, sizeof(ingConf));
                    if (vtss_appl_qos_ingress_map_dscp_conf_get(ing_id, dscp, &ingConf) == VTSS_RC_OK) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                      "0#%u#2#%d/%d/%d/%d/%d/%d/%d",
                                      map_id,
                                      dscp,
                                      ingConf.cos,
                                      ingConf.dpl,
                                      ingConf.pcp,
                                      ingConf.dei,
                                      ingConf.dscp,
                                      ingConf.cosid);
                        (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                    }
                } else {
                    eg_id = (mesa_qos_egress_map_id_t) map_id;
                    memset(&egConf, 0, sizeof(egConf));
                    if (vtss_appl_qos_egress_map_dscp_dpl_conf_get(eg_id, dscp, dpl, &egConf) == VTSS_RC_OK) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                      "1#%u#2#%d/%d/%d/%d/%d",
                                      map_id,
                                      dscp,
                                      dpl,
                                      egConf.pcp,
                                      egConf.dei,
                                      egConf.dscp);
                        (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                    }
                }
            }
        } else if (rule_flag == 4) {
            if (keyType == 1) {
                if (map_type == 0) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "0#%u#1#0/0/0/0/0/0/0/0/0", map_id);
                    (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                } else {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "1#%u#1#0/0/0/0/0/0", map_id);
                    (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            } else if (keyType == 2) {
                if (map_type == 0) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "0#%u#2#0/0/0/0/0/0/0/0", map_id);
                    (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                } else {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "1#%u#2#0/0/0/0/0/0", map_id);
                    (void) cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            }
        }

        (void) cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t qos_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    int i;
    unsigned int cnt = 0;
    char dscp_names[64 * 10];
    char buff[QOS_WEB_BUF_LEN + sizeof(dscp_names)];

    for (i = 0; i < 64; i++) {
        cnt += snprintf(dscp_names + cnt, sizeof(dscp_names) - cnt, "%s'%s'", i == 0 ? "" : ",", vtss_appl_qos_dscp2str(i));
    }

    cnt  = snprintf(buff, sizeof(buff),
                    "var configQosClassCnt = %d;\n"
                    "var configQosClassMin = %d;\n"
                    "var configQosClassMax = %d;\n"
                    "var configQosDplCnt = %d;\n"
                    "var configQosDplMin = %d;\n"
                    "var configQosDplMax = %d;\n"
                    "var configQosQueueCount = %u;\n"
                    "var configQosQceMax = %d;\n"
                    "var configQosWredVer = %d;\n"
                    "var configQosWredDplMin = %d;\n"
                    "var configQosWredDplMax = %d;\n"
                    "var configQosWredGroupMin = %d;\n"
                    "var configQosWredGroupMax = %d;\n"
                    "var configQosPortPolicerBitRateMin = %d;\n"
                    "var configQosPortPolicerBitRateMax = %d;\n"
                    "var configQosPortPolicerFrameRateMin = %d;\n"
                    "var configQosPortPolicerFrameRateMax = %d;\n"
                    "var configQosQueuePolicerBitRateMin = %d;\n"
                    "var configQosQueuePolicerBitRateMax = %d;\n"
                    "var configQosQueuePolicerFrameRateMin = %d;\n"
                    "var configQosQueuePolicerFrameRateMax = %d;\n"
                    "var configQosPortShaperBitRateMin = %d;\n"
                    "var configQosPortShaperBitRateMax = %d;\n"
                    "var configQosPortShaperFrameRateMin = %d;\n"
                    "var configQosPortShaperFrameRateMax = %d;\n"
                    "var configQosQueueShaperBitRateMin = %d;\n"
                    "var configQosQueueShaperBitRateMax = %d;\n"
                    "var configQosQueueShaperFrameRateMin = %d;\n"
                    "var configQosQueueShaperFrameRateMax = %d;\n"
                    "var configQosGlobalStormBitRateMin = %d;\n"
                    "var configQosGlobalStormBitRateMax = %d;\n"
                    "var configQosGlobalStormFrameRateMin = %d;\n"
                    "var configQosGlobalStormFrameRateMax = %d;\n"
                    "var configQosPortStormBitRateMin = %d;\n"
                    "var configQosPortStormBitRateMax = %d;\n"
                    "var configQosPortStormFrameRateMin = %d;\n"
                    "var configQosPortStormFrameRateMax = %d;\n"
                    "var configQosHasDscpDplClassification = %d;\n"
                    "var configQosHasDscpDplRemarking = %d;\n"
                    "var configQosHasQceAddressMode = %d;\n"
                    "var configQosHasQceKeyType = %d;\n"
                    "var configQosHasQceMacOui = %d;\n"
                    "var configQosHasQceDmac = %d;\n"
                    "var configQosHasQceDip = %d;\n"
                    "var configQosHasQceInnerTag = %d;\n"
                    "var configQosHasQceSTag = %d;\n"
                    "var configQosHasQceActionPcpDei = %d;\n"
                    "var configQosHasQceActionPolicy = %d;\n"
                    "var configQoSHasIngressMap = %d;\n"
                    "var configQosIngressMapMin = %d;\n"
                    "var configQosIngressMapMax = %d;\n"
                    "var configQoSHasEgressMap = %d;\n"
                    "var configQosEgressMapMin = %d;\n"
                    "var configQosEgressMapMax = %d;\n"
                    "var configQosHasCosIdClassification = %d;\n"
                    "var configQosCosIdMin = %d;\n"
                    "var configQosCosIdMax = %d;\n"
                    "var configQosHasFramePreemption = %d;\n"
                    "var configQosDscpNames = [%s];\n"
                    ,
                    CAPA->class_max + 1,
                    CAPA->class_min,
                    CAPA->class_max,
                    CAPA->dpl_max + 1,
                    CAPA->dpl_min,
                    CAPA->dpl_max,
                    VTSS_APPL_QOS_PORT_QUEUE_CNT,
                    VTSS_APPL_QOS_QCE_MAX,
                    CAPA->has_wred_v3 ? 3 : CAPA->has_wred_v2 ? 2 : 0,
                    CAPA->wred_group_min,
                    CAPA->wred_group_max,
                    CAPA->wred_dpl_min,
                    CAPA->wred_dpl_max,
                    CAPA->port_policer_bit_rate_min,
                    CAPA->port_policer_bit_rate_max,
                    CAPA->port_policer_frame_rate_min,
                    CAPA->port_policer_frame_rate_max,
                    CAPA->queue_policer_bit_rate_min,
                    CAPA->queue_policer_bit_rate_max,
                    CAPA->queue_policer_frame_rate_min,
                    CAPA->queue_policer_frame_rate_max,
                    CAPA->port_shaper_bit_rate_min,
                    CAPA->port_shaper_bit_rate_max,
                    CAPA->port_shaper_frame_rate_min,
                    CAPA->port_shaper_frame_rate_max,
                    CAPA->queue_shaper_bit_rate_min,
                    CAPA->queue_shaper_bit_rate_max,
                    CAPA->queue_shaper_frame_rate_min,
                    CAPA->queue_shaper_frame_rate_max,
                    CAPA->global_storm_bit_rate_min,
                    CAPA->global_storm_bit_rate_max,
                    CAPA->global_storm_frame_rate_min,
                    CAPA->global_storm_frame_rate_max,
                    CAPA->port_storm_bit_rate_min,
                    CAPA->port_storm_bit_rate_max,
                    CAPA->port_storm_frame_rate_min,
                    CAPA->port_storm_frame_rate_max,
                    CAPA->has_dscp_dpl_class,
                    CAPA->has_dscp_dpl_remark,
                    CAPA->has_qce_address_mode,
                    CAPA->has_qce_key_type,
                    CAPA->has_qce_mac_oui,
                    CAPA->has_qce_dmac,
                    CAPA->has_qce_dip,
                    CAPA->has_qce_inner_tag,
                    CAPA->has_qce_stag,
                    CAPA->has_qce_action_pcp_dei,
                    CAPA->has_qce_action_policy,
                    CAPA->has_ingress_map,
                    CAPA->ingress_map_id_min,
                    CAPA->ingress_map_id_max,
                    CAPA->has_egress_map,
                    CAPA->egress_map_id_min,
                    CAPA->egress_map_id_max,
                    CAPA->has_cosid_classification,
                    CAPA->cosid_min,
                    CAPA->cosid_max,
                    CAPA->has_queue_frame_preemption,
                    dscp_names);

    if (sizeof(buff) - cnt < 2) {
        T_W("config.js might be truncated (cnt:%u, size:" VPRIz").", cnt, sizeof(buff));
    } else {
        T_I("config.js seems ok (cnt:%u, size:" VPRIz").", cnt, sizeof(buff));
    }
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib_config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(qos_lib_config_js);

/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t qos_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[QOS_WEB_BUF_LEN];
    (void) snprintf(buff, sizeof(buff), "%s%s%s%s%s%s%s%s"
                    ".has_qos_fixed_map { display: none; }\r\n",
                    CAPA->has_tag_classification ? "" : ".has_qos_tag_class { display: none; }\r\n",
                    CAPA->has_dscp ? "" : ".has_qos_dscp_class { display: none; }\r\n",
                    CAPA->has_queue_shapers_eb ? "" : ".has_qos_queue_shapers_eb { display: none; }\r\n",
                    CAPA->has_queue_shapers_crb ? "" : ".has_qos_queue_shapers_crb { display: none; }\r\n",
                    CAPA->has_queue_cut_through ? "" : ".has_qos_queue_cut_through { display: none; }\r\n",
                    //CAPA->has_queue_frame_preemption ? "" : ".has_qos_frame_preemption { display: none; }\r\n",
                    "",
                    CAPA->has_shapers_rt ? "" : ".has_qos_shapers_rt { display: none; }\r\n",
                    CAPA->has_psfp ? "" : ".has_psfp { display: none; }\r\n");
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(qos_lib_filter_css);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_qos_counter, "/stat/qos_counter",   handler_stat_qos_counter);

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_stormctrl, "/config/stormconfig", handler_config_stormctrl);

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qcl_v2, "/config/qcl_v2", handler_config_qcl_v2);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_qcl_v2,   "/stat/qcl_v2",   handler_stat_qcl_v2);

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_classification,      "/config/qos_port_classification",      handler_config_qos_port_classification);

#if defined(VTSS_SW_OPTION_QOS_ADV)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_classification_map,  "/config/qos_port_classification_map",  handler_config_qos_port_classification_map);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_dscp_classification_map,  "/config/qos_dscp_classification_map",  handler_config_qos_dscp_classification_map);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_dscp_translation, "/config/qos_dscp_translation", handler_config_dscp_translation);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_dscp_port_config, "/config/dscp_port_config", handler_config_dscp_port_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_dscp_bsd_qos_ingr_cls, "/config/dscp_based_qos_ingr_cls", handler_config_dscp_based_qos_ingr_classi);
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_policers,            "/config/qos_port_policers",            handler_config_qos_port_policers);

#if defined(VTSS_SW_OPTION_QOS_ADV)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_queue_policers,           "/config/qos_queue_policers",           handler_config_qos_queue_policers);
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_schedulers,          "/config/qos_port_schedulers",          handler_config_qos_port_schedulers);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_shapers,             "/config/qos_port_shapers",             handler_config_qos_port_shapers);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_scheduler_edit,      "/config/qos_port_scheduler_edit",      handler_config_qos_port_scheduler_edit);

#if defined(VTSS_SW_OPTION_QOS_ADV)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_tag_remarking,       "/config/qos_port_tag_remarking",       handler_config_qos_port_tag_remarking);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_port_tag_remarking_edit,  "/config/qos_port_tag_remarking_edit",  handler_config_qos_port_tag_remarking_edit);
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_wred, "/config/qos_wred", handler_config_qos_wred);

#if defined(VTSS_SW_OPTION_QOS_ADV)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_ingress_map,      "/config/qos_ingress_map",      handler_config_qos_ingress_map);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_ingress_map_edit, "/config/qos_ingress_map_edit", handler_config_qos_ingress_map_edit);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_egress_map,      "/config/qos_egress_map",      handler_config_qos_egress_map);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_egress_map_edit, "/config/qos_egress_map_edit", handler_config_qos_egress_map_edit);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_map_rule, "/config/qos_map_rule", handler_config_qos_map_rule);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_qos_map_rule_edit, "/config/qos_map_rule_edit", handler_config_qos_map_rule_edit);
#endif /* defined(VTSS_SW_OPTION_QOS_ADV) */
