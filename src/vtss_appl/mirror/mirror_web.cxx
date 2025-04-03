/*
 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "mirror_api.h"
#include "port_iter.hxx"
#include "msg_api.h"
#include "vlan_api.h"
#include "mgmt_api.h"
#include "standalone_api.h"

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_RMIRROR

#define RMIRROR_WEB_DEFAULT_SESSION     1
#define RMIRROR_WEB_DEFAULT_MODE        0           // disabled
#define RMIRROR_WEB_DEFAULT_VID         200
#define RMIRROR_WEB_DEFAULT_TYPE        VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR
#define RMIRROR_WEB_DEFAULT_REFLECTOR   0           // disabled
#define RMIRROR_WEB_DEFAULT_SWITCH_ID   1

#define RMIRROR_WEB_DEFAULT_TAGGED      0           // tagged all
#define RMIRROR_WEB_UNTAGGED_ALL        1           // untagged all

#define RMIRROR_WEB_DEFAULT_MIRROR_ID     999

static char *rmirror_list2txt(BOOL *list, int min, int max, char *buf)
{
    int  i, first = 1, count = 0;
    BOOL member;
    char *p;

    p = buf;
    *p = '\0';
    for (i = min; i <= max; i++) {
        member = list[i];
        if ((member && (count == 0 || i == max)) || (!member && count > 1)) {
            p += sprintf(p, "%s%d",
                         first ? "" : count > (member ? 1 : 2) ? "-" : ",",
                         member ? (i) : i - 1);
            first = 0;
        }
        if (member) {
            count++;
        } else {
            count = 0;
        }
    }
    return buf;
}

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/
//
// RMIRROR handler
//
static i32 handler_config_rmirror(CYG_HTTPD_STATE *p)
{
    mesa_rc                 rc = VTSS_RC_OK;
    vtss_isid_t             sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    rmirror_switch_conf_t   conf, *conf_ptr = &conf;
    int                     ct;
    int                     form_value;
    int                     idx;
    int                     min_session = rmirror_mgmt_min_session_get();
    int                     max_session = rmirror_mgmt_max_session_get();
    char                    form_name[32];
    const char              *var_string;
    size_t                  nlen;
    port_iter_t             pit;

    char                    unescaped_str[VLAN_VID_LIST_AS_STRING_LEN_BYTES];
    char                    escaped_str[3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES];
    size_t                  cnt = 0;
    char                    *found_ptr;

    T_D ("RMIRROR web access - SID =  %d", sid );

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_RMIRROR)) {
        return -1;
    }
#endif

    rmirror_mgmt_default_set(conf_ptr);

    if (p->method == CYG_HTTPD_METHOD_POST) {

        // Get Session
        if (cyg_httpd_form_varable_int(p, "sessionid", &form_value) &&
            form_value > 0) {
            T_D ("session id set to %d via web", form_value);
            conf_ptr->session_num = form_value;
        } else {
            T_D ("session id set failed from web");
            conf_ptr->session_num = RMIRROR_WEB_DEFAULT_SESSION;
        }

        // Get the current configuration
        rc = rmirror_mgmt_conf_get(conf_ptr);
        if (rc != VTSS_RC_OK) {
            goto EXIT_POINT;
        }

        rc = rmirror_mgmt_switch_conf_get(sid, conf_ptr);
        if (rc != VTSS_RC_OK) {
            goto EXIT_POINT;
        }

        T_D ("Updating from web");

        // Get Mode
        if (cyg_httpd_form_varable_int(p, "mode", &form_value) &&
            form_value >= 0) {
            T_D ("mode set to %d via web", form_value);
            conf_ptr->enabled = form_value;
        } else {
            T_D ("mode set failed from web");
            conf_ptr->enabled = RMIRROR_WEB_DEFAULT_MODE;
        }

        // Get Vid
        if (cyg_httpd_form_varable_int(p, "vid", &form_value) &&
            form_value > 0) {
            T_D ("vid set to %d via web", form_value);
            conf_ptr->vid = form_value;
        } else {
            T_D ("vid set failed from web");
            conf_ptr->vid = RMIRROR_WEB_DEFAULT_VID;
        }

        // check vid is valid or not
        if ((rc = rmirror_mgmt_is_valid_vid(conf_ptr->vid)) !=  VTSS_RC_OK) {
            send_custom_error(p, error_txt(rc), " ", 1 );
            rc = VTSS_INCOMPLETE;
            goto EXIT_POINT;
        }

        // Get Type
        if (cyg_httpd_form_varable_int(p, "type", &form_value) &&
            form_value >= 0) {
            T_D ("type set to %d via web", form_value);
            conf_ptr->type = (vtss_appl_rmirror_switch_type_t)form_value;
        } else {
            T_D ("type set failed from web");
            conf_ptr->type = RMIRROR_WEB_DEFAULT_TYPE;
        }

        // Get switch id from WEB ( Form name = "switchselect" )

#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
        // Get taggedselect from WEB ( Form name = "taggedselect" )
        if (cyg_httpd_form_varable_int(p, "taggedselect", &form_value) && form_value > 0) {
            T_D ("taggedselect set to %d via web", form_value);
            conf_ptr->reflector_port = form_value;
        } else {
            T_D ("taggedselect disabled from web");
            conf_ptr->reflector_port = RMIRROR_WEB_DEFAULT_TAGGED;
        }
#else
        // Get reflector port from WEB ( Form name = "portselect" )
        if (cyg_httpd_form_varable_int(p, "portselect", &form_value) &&
            form_value > 0 && rmirror_mgmt_is_valid_reflector_port(conf_ptr->rmirror_switch_id, uport2iport(form_value))) {
            T_D ("reflector port set to %d via web", form_value);
            conf_ptr->reflector_port = uport2iport(form_value);
        } else {
            T_D ("reflector port disabled from web");
            conf_ptr->reflector_port = RMIRROR_WEB_DEFAULT_REFLECTOR;
        }
#endif

        // Switch configuration table
        rc = rmirror_mgmt_switch_conf_get(sid, conf_ptr);
        if (rc != VTSS_RC_OK) {
            goto EXIT_POINT;
        }

        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);
        while (port_iter_getnext(&pit)) {

            // Get source
            sprintf(form_name, "mode_%d", iport2uport(pit.iport)); // Set to the htm checkbox form name
            if (cyg_httpd_form_varable_int(p, form_name, &form_value)   && (form_value >= 0 && form_value < 4)) {
                // form_value ok
            } else {
                form_value = 0;
            }

            if (form_value == 0) { // disable
                conf_ptr->source_port[pit.iport - VTSS_PORT_NO_START].rx_enable = 0;
                conf_ptr->source_port[pit.iport - VTSS_PORT_NO_START].tx_enable = 0;
            } else if (form_value == 1) { // rx
                conf_ptr->source_port[pit.iport - VTSS_PORT_NO_START].rx_enable = 1;
                conf_ptr->source_port[pit.iport - VTSS_PORT_NO_START].tx_enable = 0;
            } else if (form_value == 2) { // tx
                conf_ptr->source_port[pit.iport - VTSS_PORT_NO_START].rx_enable = 0;
                conf_ptr->source_port[pit.iport - VTSS_PORT_NO_START].tx_enable = 1;
            } else { // both
                // form_value is 3
                conf_ptr->source_port[pit.iport - VTSS_PORT_NO_START].rx_enable = 1;
                conf_ptr->source_port[pit.iport - VTSS_PORT_NO_START].tx_enable = 1;
            }

            // Get intermediate
            sprintf(form_name, "intermediate_%d", iport2uport(pit.iport)); // Set to the htm checkbox form name
            if (cyg_httpd_form_varable_string(p, form_name, &nlen)) {
                T_D("intermediate port [%d] set to enable via web", iport2uport(pit.iport));
                conf_ptr->intermediate_port[pit.iport - VTSS_PORT_NO_START] = TRUE;
            } else {
                T_D("intermediate port [%d] re-set to default via web", iport2uport(pit.iport));
                conf_ptr->intermediate_port[pit.iport - VTSS_PORT_NO_START] = FALSE;
            }

            // Get destination
            sprintf(form_name, "destination_%d", iport2uport(pit.iport)); // Set to the htm checkbox form name
            if (cyg_httpd_form_varable_string(p, form_name, &nlen)) {
                T_D("destination port [%d] set to enable via web", iport2uport(pit.iport));
                conf_ptr->destination_port[pit.iport - VTSS_PORT_NO_START] = TRUE;
            } else {
                T_D("destination port [%d] re-set to default via web", iport2uport(pit.iport));
                conf_ptr->destination_port[pit.iport - VTSS_PORT_NO_START] = FALSE;
            }
        }

        // Getting CPU configuration
        if (cyg_httpd_form_varable_int(p, "mode_CPU", &form_value)   && (form_value >= 0 && form_value < 4)) {
            // form_value ok
        } else {
            form_value = 0;
        }

        if (form_value == 0) {
            conf_ptr->cpu_src_enable = 0;
            conf_ptr->cpu_dst_enable = 0;
        } else if (form_value == 1) {
            conf_ptr->cpu_src_enable = 1;
            conf_ptr->cpu_dst_enable = 0;
        } else if (form_value == 2) {
            conf_ptr->cpu_src_enable = 0;
            conf_ptr->cpu_dst_enable = 1;
        } else {
            // form_value is 3
            conf_ptr->cpu_src_enable = 1;
            conf_ptr->cpu_dst_enable = 1;
        }

        // Get Vlans
        if ((found_ptr = (char *)cyg_httpd_form_varable_string(p, "vlans", &nlen)) != NULL) {

            // Unescape the string
            if (!cgi_unescape(found_ptr, escaped_str, nlen, 3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES)) {
                T_E("Unable to unescape %s", found_ptr);
                rc = VTSS_INCOMPLETE;
                goto EXIT_POINT;
            }

            // Filter out white space
            found_ptr = escaped_str;
            while (*found_ptr) {
                char c = *(found_ptr++);
                if (c != ' ') {
                    unescaped_str[cnt++] = c;
                }
            }
            unescaped_str[cnt] = '\0';

            if (mgmt_txt2list(unescaped_str, conf_ptr->source_vid.data(), VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX, 0) != VTSS_RC_OK) {
                T_E("Failed to convert %s to list form", unescaped_str);
            }
        } else {
            vtss_clear(conf_ptr->source_vid);
        }

        // Set configuration
        if ((rc = rmirror_mgmt_switch_conf_set(sid, conf_ptr)) !=  VTSS_RC_OK) { // Update switch with new configuration
            send_custom_error(p, error_txt(rc), " ", 1 );
            rc = VTSS_INCOMPLETE;
            goto EXIT_POINT;
        }

        if ((rc = rmirror_mgmt_conf_set(conf_ptr)) !=  VTSS_RC_OK) { // Write new configuration
            send_custom_error(p, error_txt(rc), " ", 1 );
            rc = VTSS_INCOMPLETE;
            goto EXIT_POINT;
        }

#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
        redirect(p, "/rmirror_no_reflector_port.htm");
#else
        redirect(p, "/rmirror.htm");
#endif
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */

        // Format:
        //  <session_id#session_id#..>,<sessionid>,<mode>,<vid>,<type>,<switch_id>,<reflector_port>,<sid#sid#..>,<hidden_switch_id>
        //  |<vlans>
        //  |<source enable>/<destination enable>/<intermediate>/<destination>,<source enable>/<destination enable>/<intermediate>/<destination>,.....
        //  |<sid1>/<reflector_port1>#<reflector_port2>#..,<sid2>/<reflector_port1>#<reflector_port2>#..

        switch_iter_t           sit;
        BOOL                    send_flag = FALSE;

        cyg_httpd_start_chunked("html");

        // get session id
        if ((var_string = cyg_httpd_form_varable_find(p, "sessionid")) != NULL) {
            conf_ptr->session_num = atol(var_string);
            T_D("session_num=" VPRIlu, conf_ptr->session_num);
        } else {
            conf_ptr->session_num = RMIRROR_WEB_DEFAULT_SESSION;
            T_D("session_num=" VPRIlu, conf_ptr->session_num);
        }
        rc = rmirror_mgmt_conf_get(conf_ptr); // Get the current configuration
        if (rc != VTSS_RC_OK) {
            goto EXIT_POINT;
        }

        // send all session id
        for (idx = min_session; idx <= max_session; ++idx) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d#", idx);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            T_N("cyg_httpd_write_chunked -> %s", p->outbuffer);
        }

        // send configuration
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "," VPRIlu",%d,%u,%d,",
                      conf_ptr->session_num,
                      conf_ptr->enabled,
                      conf_ptr->vid,
                      conf_ptr->type);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        T_N("cyg_httpd_write_chunked -> %s", p->outbuffer);

#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u,",
                      topo_isid2usid(conf_ptr->rmirror_switch_id),
                      conf_ptr->reflector_port);

        cyg_httpd_write_chunked(p->outbuffer, ct);
        T_N("cyg_httpd_write_chunked -> %s", p->outbuffer);
#else
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,%u,",
                      topo_isid2usid(conf_ptr->rmirror_switch_id),
                      iport2uport(conf_ptr->reflector_port));

        cyg_httpd_write_chunked(p->outbuffer, ct);
        T_N("cyg_httpd_write_chunked -> %s", p->outbuffer);
#endif


        // send hidden switch id
        if (conf_ptr->type == VTSS_APPL_RMIRROR_SWITCH_TYPE_MIRROR) {
            // find destination port, only one destination port
            (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID);
            while (switch_iter_getnext(&sit)) {

                rc = rmirror_mgmt_switch_conf_get(sit.isid, conf_ptr);
                if (rc != VTSS_RC_OK) {
                    goto EXIT_POINT;
                }

                (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_FRONT);
                while (port_iter_getnext(&pit)) {

                    if (conf_ptr->destination_port[pit.iport - VTSS_PORT_NO_START]) {

                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",%u", sit.usid);
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                        T_N("cyg_httpd_write_chunked -> %s", p->outbuffer);
                        send_flag = TRUE;
                        break;
                    }
                }
            }
        }
        if (!send_flag) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",%d", RMIRROR_WEB_DEFAULT_MIRROR_ID);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            T_N("cyg_httpd_write_chunked -> %s", p->outbuffer);
        }

        // Currently setting VLANs
        (void)cgi_escape(rmirror_list2txt(conf_ptr->source_vid.data(), VTSS_APPL_VLAN_ID_MIN, VTSS_APPL_VLAN_ID_MAX, unescaped_str), escaped_str);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%s", escaped_str);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        // Get the SID config
        cyg_httpd_write_chunked("|", 1);

        rc = rmirror_mgmt_switch_conf_get(sid, conf_ptr);
        if (rc != VTSS_RC_OK) {
            goto EXIT_POINT;
        }

        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_FRONT);
        while (port_iter_getnext(&pit)) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%u/%u/%d/%d,",
                          iport2uport(pit.iport),
                          conf_ptr->source_port[pit.iport - VTSS_PORT_NO_START].rx_enable,
                          conf_ptr->source_port[pit.iport - VTSS_PORT_NO_START].tx_enable,
                          conf_ptr->intermediate_port[pit.iport - VTSS_PORT_NO_START],
                          conf_ptr->destination_port[pit.iport - VTSS_PORT_NO_START]);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            T_N("cyg_httpd_write_chunked -> %s", p->outbuffer);
        }

        // CPU port
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/%u/%u/0/0,",
                      "CPU",
                      conf_ptr->cpu_src_enable,
                      conf_ptr->cpu_dst_enable);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        T_N("cyg_httpd_write_chunked -> %s", p->outbuffer);

        cyg_httpd_write_chunked("|", 1);

        // send all reflector ports
        (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID);
        while (switch_iter_getnext(&sit)) {
            if (!msg_switch_exists(sit.isid)) {
                continue;
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/", sit.usid);
            cyg_httpd_write_chunked(p->outbuffer, ct);
            T_N("cyg_httpd_write_chunked -> %s", p->outbuffer);

            // check all reflector ports by isid
            (void) port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_FRONT);
            while (port_iter_getnext(&pit)) {
                if (rmirror_mgmt_is_valid_reflector_port(sit.isid, pit.iport)) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#", iport2uport(pit.iport));
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            }

            cyg_httpd_write_chunked(",", 1);
        }

        cyg_httpd_end_chunked();
    }

EXIT_POINT:

    if (rc != VTSS_RC_OK) {
        T_D("Error: rc = %d", rc);
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/
#define RMIRROR_WEB_BUF_LEN 512

static size_t RMIRROR_WEB_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[RMIRROR_WEB_BUF_LEN];

    (void)snprintf(buff, RMIRROR_WEB_BUF_LEN,
                   "var configVlanIdMin = %d;\n"
                   "var configVlanIdMax = %d;\n"
                   "var configSupportCPUMirror = %d;\n"
                   "var configSupportInternalPort = %d;\n",
                   VTSS_APPL_VLAN_ID_MIN,   /* First VLAN ID */
                   VTSS_APPL_VLAN_ID_MAX,   /* Last VLAN ID */
                   1,
#ifdef VTSS_SW_OPTION_MIRROR_LOOP_PORT
                   1
#else
                   0
#endif
                  );
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(RMIRROR_WEB_config_js);


/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t RMIRROR_WEB_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[RMIRROR_WEB_BUF_LEN];

    (void) snprintf(buff, RMIRROR_WEB_BUF_LEN, ".rmirror_cpu_display { }\r\n");

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/

web_lib_filter_css_tab_entry(RMIRROR_WEB_lib_filter_css);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_rmirror, "/config/rmirror", handler_config_rmirror);

