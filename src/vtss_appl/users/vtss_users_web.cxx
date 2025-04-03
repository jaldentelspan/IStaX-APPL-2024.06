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
#include "sysutil_api.h"
#include "vtss_users_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define USERS_WEB_BUF_LEN 512

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

static i32 handler_stat_users(CYG_HTTPD_STATE *p)
{
    int             ct, i = 0, max_num = VTSS_USERS_NUMBER_OF_USERS;
    users_conf_t    conf;
    char            encoded_string[3 * VTSS_SYS_USERNAME_LEN];

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MISC)) {
        return -1;
    }
#endif

    (void)cyg_httpd_start_chunked("html");

    /* get form data
       Format: [max_num]|[user_idx],[username],[priv_level]|...
    */
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|", max_num);
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);

    memset(&conf, 0x0, sizeof(conf));
    while (vtss_users_mgmt_conf_get(&conf, 1) == VTSS_RC_OK) {
        encoded_string[0] = '\0';
#if defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
        (void)cgi_escape(conf.username, encoded_string);
#else
        if (cgi_escape(conf.username, encoded_string) == 0) {
            continue;
        }
#endif /* VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */

#ifdef VTSS_SW_OPTION_PRIV_LVL
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d,%s,%d|", ++i, encoded_string, conf.privilege_level);
#else
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d,%s,0|", ++i, encoded_string);
#endif
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    (void)cyg_httpd_end_chunked();
    return -1; // Do not further search the file system.
}

static i32 handler_config_user_config(CYG_HTTPD_STATE *p)
{
    int             ct;
    users_conf_t    conf;
    int             var_value, i = 0;
    const char      *var_string, *pass1, *pass2;
    char            unescaped_pass[VTSS_SYS_PASSWD_LEN];
    char            encoded_string[3 * VTSS_SYS_USERNAME_LEN];
    size_t          len, len1, len2;
    mesa_rc         rc;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    vtss_priv_conf_t priv_conf;
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        return -1;
    }
#endif

    memset(&conf, 0x0, sizeof(conf));

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        //username
        var_string = cyg_httpd_form_varable_string(p, "username", &len);
        if (len > 0) {
            if (cgi_unescape(var_string, conf.username, len, sizeof(conf.username)) == FALSE) {
                redirect(p, "/users.htm");
                return -1;
            }
        }
        if (cyg_httpd_form_varable_int(p, "delete", &var_value)) {
            //delete
            if (!strcmp(conf.username, VTSS_SYS_ADMIN_NAME)) {
                redirect(p, "/users.htm");
                return -1;
            } else {
                (void) vtss_users_mgmt_conf_del(conf.username);
            }
        } else {
            // Get user entry first if it existing
            rc = vtss_users_mgmt_conf_get(&conf, FALSE);

            //password
            if (cyg_httpd_form_varable_int(p, "password_update", &var_value) && var_value == 1) {
                conf.encrypted = FALSE;

                pass1 = cyg_httpd_form_varable_string(p, "password1", &len1);
                pass2 = cyg_httpd_form_varable_string(p, "password2", &len2);
                if (pass1 && pass2 &&   /* Safeguard - this is *also* checked by javascript */
                    len1 == len2 &&
                    strncmp(pass1, pass2, len1) == 0 &&
                    cgi_unescape(pass1, unescaped_pass, len1, sizeof(unescaped_pass))) {
                    strcpy(conf.password, unescaped_pass);
                }
            }

#ifdef VTSS_SW_OPTION_PRIV_LVL
            //priv_level
            if (cyg_httpd_form_varable_int(p, "priv_level", &var_value)) {
                conf.privilege_level = var_value;
            }
#endif

            conf.valid = 1;
            if ((rc = vtss_users_mgmt_conf_set(&conf)) != VTSS_RC_OK) {
                T_D("vtss_users_mgmt_conf_set(): failed, rc = %d", rc);
                if (rc == VTSS_USERS_ERROR_USERS_TABLE_FULL) {
                    redirect_errmsg(p, "users.htm", vtss_users_error_txt(rc));
                }
                return -1;
            }
        }

        redirect(p, "/users.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: [user_idx],[username],[priv_level],[misc_priv_level]
        */
        if ((var_string = cyg_httpd_form_varable_find(p, "user")) != NULL) {
            var_value = atoi(var_string);
            while ((rc = vtss_users_mgmt_conf_get(&conf, 1)) == VTSS_RC_OK) {
                i++;
                if (i == var_value) {
                    break;
                }
            }
            encoded_string[0] = '\0';
#if defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
            (void)cgi_escape(conf.username, encoded_string);
#endif /* VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */
            if (rc != VTSS_RC_OK
#if !defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
                || cgi_escape(conf.username, encoded_string) == 0
#endif /* !VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */
               ) {
                (void)cyg_httpd_end_chunked();
                return -1;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d,%s,", i, encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
#ifdef VTSS_SW_OPTION_PRIV_LVL
            if (vtss_priv_mgmt_conf_get(&priv_conf) != VTSS_RC_OK) {
                T_W("vtss_priv_mgmt_conf_get() failed");
                (void)cyg_httpd_end_chunked();
                return -1;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d,%d", conf.privilege_level, priv_conf.privilege_level[VTSS_MODULE_ID_MISC].configRwPriv);
#else
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "-1");
#endif
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        } else {
#ifdef VTSS_SW_OPTION_PRIV_LVL
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "-1,,0,15");
#else
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "-1,,-1,0");
#endif
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t users_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[USERS_WEB_BUF_LEN];
    (void) snprintf(buff, USERS_WEB_BUF_LEN,
                    "var configUserMaxCnt = %d;\n"
                    "var configUsernameMaxLen = %d;\n"
                    "var configPasswordMaxLen = %d;\n"
                    "var configDefaultUserNameNullStr = %d;\n"
                    , VTSS_USERS_NUMBER_OF_USERS
                    , VTSS_SYS_INPUT_USERNAME_LEN
                    , VTSS_SYS_INPUT_PASSWD_LEN,
#if defined(VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR)
                    1
#else
                    0
#endif /* VTSS_SYS_ADMIN_NAME_DEFAULT_NULL_STR */
                   );
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(users_lib_config_js);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_users, "/stat/users", handler_stat_users);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_user_config, "/config/user_config", handler_config_user_config);

