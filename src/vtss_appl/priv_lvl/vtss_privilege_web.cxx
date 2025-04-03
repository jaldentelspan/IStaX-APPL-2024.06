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

#include "vtss_privilege_web_api.h"

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

int web_process_priv_lvl(CYG_HTTPD_STATE *p, vtss_priv_lvl_type_t type, vtss_module_id_t id)
{
    mesa_rc rc1, rc2;

    int current_priv_lvl = cyg_httpd_current_privilege_level();

#if defined(VTSS_SW_OPTION_JSON_RPC)
    if (id == VTSS_MODULE_ID_JSON_RPC) {
        /* For JSON based web pages, the privilege level implementation does
           not use the http header to report access level, but expose the
           required access level through the json-rpc introspection interface
           instead.
           The current implementation requires VTSS_PRIV_LVL_CONFIG_TYPE (RW)
           for the VTSS_MODULE_ID_JSON_RPC group before it starts evaluating
           any commands. If this requirement is met, it will then start
           searching for the method requested, and evaluate the current access
           level against the required access level for the given method.
         */

        if (!vtss_priv_is_allowed_crw(id, current_priv_lvl)) {
            redirect(p, "/insuf_priv_lvl.htm");
            return -1;
        }
        return 0;
    }
#endif /* VTSS_SW_OPTION_JSON_RPC */

    /* Note:
     * The following implemetation only suit for the original pages (none JSON based).
     *
     * 1. The original web pages always use POST method for writing operation and GET
     *    method for reading operation. And each page owns its unique web handler
     *    function.
     * 2. The web handler function will call web_process_priv_lvl() to check the
     *    access privilege level.
     * 3. In web_process_priv_lvl() function.
     *    3.1 WRITE operation:
     *        a) If the web page doesn't allow writing operation, it will be redirect
     *           to a specific page "insuf_priv_lvl.htm page".
     *    3.2 READ operation:
     *        a) If the web page only allows read-only, a proprietary tag
     *           "X-ReadOnly: true" will be added in HTML response header.
     *        b) If the web page doesn't allow any action, a proprietary tag
     *           "X-ReadOnly: null" will be added in HTML response header.
     *        c) Do nothing when the web page allows read-write.
     */
    if (type == VTSS_PRIV_LVL_CONFIG_TYPE) {
        rc1 = vtss_priv_is_allowed_crw(id, current_priv_lvl);
    } else {
        rc1 = vtss_priv_is_allowed_srw(id, current_priv_lvl);
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* If the web page doesn't allow writing operation, it will be redirect
         * to a specific page "insuf_priv_lvl.htm page".
        */
        if (!rc1) {
            redirect(p, "/insuf_priv_lvl.htm");
            return -1;
        }
    } else {
        /* If the web page only allows read-only, a proprietary tag "X-ReadOnly: true" will be added in HTML response header.
         * If the web page doesn't allow any action, a proprietary tag "X-ReadOnly: null" will be added in HTML response header.
         * Do nothing when the web page allows read-write.
         */
        if (type == VTSS_PRIV_LVL_CONFIG_TYPE) {
            rc2 = vtss_priv_is_allowed_cro(id, current_priv_lvl);
        } else {
            rc2 = vtss_priv_is_allowed_sro(id, current_priv_lvl);
        }

        if (!rc1 && rc2) {
            cyg_httpd_set_xreadonly_tag(1);    //Readonly privilege level
        } else if (!rc1 && !rc2) {
            int ct;
            cyg_httpd_set_xreadonly_tag(0);    //Insufficient privilege level
            (void)cyg_httpd_start_chunked("html");
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "Insufficient privilege level");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            (void)cyg_httpd_end_chunked();
            return -1;
        }
    }
    return 0;
}

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static i32 handler_config_priv_lvl(CYG_HTTPD_STATE *p)
{
    int                 ct;
    vtss_priv_conf_t    conf, newconf;
    int                 var_value;
    char                buf[VTSS_PRIV_LVL_NAME_LEN_MAX];
    char                encoded_string[3 * VTSS_PRIV_LVL_NAME_LEN_MAX];
    char                priv_group_name[VTSS_PRIV_LVL_NAME_LEN_MAX] = "";
    vtss_module_id_t    module_id;
    users_conf_t        users_conf;

    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        return -1;
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        if (vtss_priv_mgmt_conf_get(&conf) == VTSS_RC_OK) {
            /* store form data */
            newconf = conf;

            while (vtss_privilege_group_name_get(priv_group_name, &module_id, TRUE)) {
                //configRoPriv
                sprintf(buf, "cro_%s", vtss_module_names[module_id]);
                (void) cgi_escape(buf, encoded_string);
                if (cyg_httpd_form_varable_int(p, encoded_string, &var_value)) {
                    newconf.privilege_level[module_id].configRoPriv = var_value;
                }

                //configRwPriv
                sprintf(buf, "crw_%s", vtss_module_names[module_id]);
                (void) cgi_escape(buf, encoded_string);
                if (cyg_httpd_form_varable_int(p, encoded_string, &var_value)) {
                    newconf.privilege_level[module_id].configRwPriv = var_value;
                }

                //statusRoPriv
                sprintf(buf, "sro_%s", vtss_module_names[module_id]);
                (void) cgi_escape(buf, encoded_string);
                if (cyg_httpd_form_varable_int(p, encoded_string, &var_value)) {
                    newconf.privilege_level[module_id].statusRoPriv = var_value;
                }

                //statusRwPriv
                sprintf(buf, "srw_%s", vtss_module_names[module_id]);
                (void) cgi_escape(buf, encoded_string);
                if (cyg_httpd_form_varable_int(p, encoded_string, &var_value)) {
                    newconf.privilege_level[module_id].statusRwPriv = var_value;
                }
            }

            if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                T_D("Calling vtss_priv_mgmt_conf_set()");
                if (vtss_priv_mgmt_conf_set(&newconf) < 0) {
                    T_E("vtss_priv_mgmt_conf_set(): failed");
                }
            }
        }
        redirect(p, "/priv_lvl.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: [admin_priv_level],[group_name]/[configRoPriv]/[configRwPriv]/[statusRoPriv]/[statusRwPriv]|...
        */
        strcpy(users_conf.username, VTSS_SYS_ADMIN_NAME);
        if (vtss_priv_mgmt_conf_get(&conf) == VTSS_RC_OK &&
            vtss_users_mgmt_conf_get(&users_conf, FALSE) == VTSS_RC_OK) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d,|", users_conf.privilege_level);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            while (vtss_privilege_group_name_get(priv_group_name, &module_id, TRUE)) {
                if (cgi_escape((char *)vtss_module_names[module_id], encoded_string) == 0) {
                    continue;
                }
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/%d/%d/%d/%d/|",
                              encoded_string,
                              conf.privilege_level[module_id].configRoPriv,
                              conf.privilege_level[module_id].configRwPriv,
                              conf.privilege_level[module_id].statusRoPriv,
                              conf.privilege_level[module_id].statusRwPriv);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            };
        }
        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_priv_lvl, "/config/priv_lvl", handler_config_priv_lvl);

