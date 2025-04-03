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
#include "sysutil_api.h"
#include "conf_api.h"
#include "msg_api.h"
#include "control_api.h"
#include "misc_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SYSTEM

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
//#include <vtss_trace_api.h>
/* ============== */

#define SYS_WEB_BUF_LEN     512

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static i32 handler_status_cpuload(CYG_HTTPD_STATE *p)
{
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM)) {
        return -1;
    }
#endif

    u32 average_point1s, average_1s, average_10s;
    average_point1s = average_1s = average_10s = 100;
    (void) control_sys_get_cpuload(&average_point1s, &average_1s, &average_10s);

    (void)cyg_httpd_start_chunked("html");
    int ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u|%u|%u",
                      average_point1s, average_1s, average_10s);
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    (void)cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_status_cpuload, "/stat/cpuload", handler_status_cpuload);

#ifndef VTSS_SW_OPTION_USERS
static i32 handler_config_passwd(CYG_HTTPD_STATE *p)
{
    const char  *pass1, *pass2;
    char        unescaped_pass[VTSS_SYS_PASSWD_LEN];
    char        encrypted_passwd[VTSS_SYS_PASSWD_ENCRYPTED_LEN];
    size_t      len1, len2;
    mesa_rc     rc;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        const char *oldpass_entered = cyg_httpd_form_varable_string(p, "oldpass", &len1);
        rc = cgi_unescape(oldpass_entered, unescaped_pass, len1, sizeof(unescaped_pass));
        system_password_encode(unescaped_pass, encrypted_passwd);
        oldpass_entered = encrypted_passwd;
        len1 = strlen(oldpass_entered);
        // Special case when len1 is 0 (in that case strncmp() always returns 0).
        if (rc != TRUE || system_clear_password_verify(unescaped_pass) != TRUE) {
            static const char *err_str = "The old password is incorrect. New password is not set.";
            send_custom_error(p, "Password Error", err_str, strlen(err_str));
            return -1;
        }
        pass1 = cyg_httpd_form_varable_string(p, "pass1", &len1);
        pass2 = cyg_httpd_form_varable_string(p, "pass2", &len2);
        if (pass1 && pass2 &&   /* Safeguard - this is *also* checked by javascript */
                len1 == len2 &&
                strncmp(pass1, pass2, len1) == 0 &&
                cgi_unescape(pass1, unescaped_pass, len1, sizeof(unescaped_pass))) {
            T_D("Password changed");
            (void) system_set_passwd(FALSE, unescaped_pass);
        } else {
            T_W("Password check failed, password left unchanged");
        }
        redirect(p, "/passwd_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");
        (void)cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}
#endif /* VTSS_SW_OPTION_USERS */

static i32 handler_stat_sys(CYG_HTTPD_STATE *p)
{
    int           ct = 0;
    conf_board_t  conf;
    system_conf_t sys_conf;
    char          encoded_string[3 * VTSS_SYS_STRING_LEN];
    char          chipid_str[20];
    char          sep[] = "/";

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM)) {
        return -1;
    }
#endif

    i32 cursecs = vtss_current_time() / 1000;
    time_t t = time(NULL);

    /* get form data */
    (void)cyg_httpd_start_chunked("html");

    // sysInfo[0-2]
    (void)conf_mgmt_board_get(&conf);
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%02x-%02x-%02x-%02x-%02x-%02x%c%s%c%s%c",
                  conf.mac_address.addr[0],
                  conf.mac_address.addr[1],
                  conf.mac_address.addr[2],
                  conf.mac_address.addr[3],
                  conf.mac_address.addr[4],
                  conf.mac_address.addr[5],
                  sep[0],
                  misc_time2interval(cursecs),
                  sep[0],
                  misc_time2str(t),
                  sep[0]);
    (void)cyg_httpd_write_chunked(p->outbuffer, ct);

    // sysInfo[3]
    ct = cgi_escape(misc_software_version_txt(), encoded_string);
    if (ct) {
        (void)cyg_httpd_write_chunked(encoded_string, ct);
    }
    (void)cyg_httpd_write_chunked(sep, 1);

    // sysInfo[4]
    ct = cgi_escape(misc_software_date_txt(), encoded_string);
    if (ct) {
        (void)cyg_httpd_write_chunked(encoded_string, ct);
    }
    (void)cyg_httpd_write_chunked(sep, 1);

    // sysInfo[5]
    sprintf(chipid_str, "VSC%x", misc_chiptype());
    ct = cgi_escape(chipid_str, encoded_string);
    if (ct) {
        (void)cyg_httpd_write_chunked(encoded_string, ct);
    }
    (void)cyg_httpd_write_chunked(sep, 1);

    (void)system_get_config(&sys_conf);

    // sysInfo[6]
    ct = cgi_escape(sys_conf.sys_contact, encoded_string);
    if (ct) {
        (void)cyg_httpd_write_chunked(encoded_string, ct);
    }
    (void)cyg_httpd_write_chunked(sep, 1);

    // sysInfo[7]
    ct = cgi_escape(sys_conf.sys_name, encoded_string);
    if (ct) {
        (void)cyg_httpd_write_chunked(encoded_string, ct);
    }
    (void)cyg_httpd_write_chunked(sep, 1);

    // sysInfo[8]
    ct = cgi_escape(sys_conf.sys_location, encoded_string);
    if (ct) {
        (void)cyg_httpd_write_chunked(encoded_string, ct);
    }
    (void)cyg_httpd_write_chunked(sep, 1);

    // sysInfo[9]
    ct = cgi_escape(misc_software_code_revision_txt(), encoded_string);
    if (ct) {
        (void)cyg_httpd_write_chunked(encoded_string, ct);
    }
    (void)cyg_httpd_write_chunked(sep, 1);


    (void)cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

#if defined(VTSS_SW_OPTION_PSU)
static i32 handler_status_psu(CYG_HTTPD_STATE *p)
{
    vtss_usid_t                     usid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_usid_t                     prev_usid = usid;
    vtss_usid_t                     next_usid;
    u32                             prev_psuid;
    u32                             next_psuid;
    vtss_appl_sysutil_psu_status_t  status;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM)) {
        return -1;
    }
#endif

    (void)cyg_httpd_start_chunked("html");
    prev_psuid = 0;
    while (vtss_appl_sysutil_psu_status_itr(&prev_usid, &next_usid, &prev_psuid, &next_psuid) == VTSS_RC_OK && prev_usid == next_usid ) {
        if ( vtss_appl_sysutil_psu_status_get(next_usid, next_psuid, &status) == VTSS_RC_OK) {
            int ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%s/%u|",
                              next_psuid, status.descr, status.state);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        prev_usid  = next_usid;
        prev_psuid = next_psuid;
    }

    (void)cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}
#endif /* VTSS_SW_OPTION_PSU */

static i32 handler_status_led(CYG_HTTPD_STATE *p)
{
    vtss_usid_t                             usid = VTSS_USID_START;
    int                                     ct, var_int;
    const char                              *var_string;
    vtss_appl_sysutil_system_led_status_t   status;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM)) {
        return -1;
    }
#endif


    if ((var_string = cyg_httpd_form_varable_find(p, "clear")) != NULL) {
        var_int = atoi(var_string);
        if (var_int >= 0) {
            vtss_appl_sysutil_control_system_led_t clear;
            clear.type = (vtss_appl_sysutil_system_led_clear_type_t) var_int;
            (void)vtss_appl_sysutil_control_system_led_set(usid, &clear);
        }
    }

    (void)cyg_httpd_start_chunked("html");

    if (vtss_appl_sysutil_system_led_status_get(usid, &status) == VTSS_RC_OK) {
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", status.descr);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
    }

    (void)cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

static i32 handler_status_licenses(CYG_HTTPD_STATE *p)
{
    void  *handle;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM)) {
        return -1;
    }
#endif

    (void)cyg_httpd_start_chunked("html");

    if (system_license_open(&handle) != VTSS_RC_OK) {
        goto do_exit;
    }

    while (system_license_gets(handle, p->outbuffer, sizeof(p->outbuffer), true) == VTSS_RC_OK && p->outbuffer[0] != '\0') {
        (void)cyg_httpd_write_chunked(p->outbuffer, strlen(p->outbuffer));
    }

    (void)system_license_close(handle);

do_exit:
    (void)cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t SYS_lib_config_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[SYS_WEB_BUF_LEN];

    (void) snprintf(buff, SYS_WEB_BUF_LEN, 
#if defined(VTSS_SW_OPTION_ZTP)
                    "function configHasZtp() {\n"
                    "  return true;\n"
                    "}\n"
#endif /* VTSS_SW_OPTION_ZTP */

#if defined(VTSS_SW_OPTION_SPROUT) && defined(VTSS_SPROUT_FW_VER_CHK)
                    "function configHasStackFwChk() {\n"
                    "  return true;\n"
                    "}\n"
#endif /* VTSS_SW_OPTION_SPROUT && VTSS_SPROUT_FW_VER_CHK */
                    " ");

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t SYS_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[SYS_WEB_BUF_LEN];

    // Do not display help text of Code Revision line in sys.htm if the length
    // of the Code Revision string is 0.
    (void) snprintf(buff, SYS_WEB_BUF_LEN,
                    ".post_only { display: none; }\r\n"

#if !defined(VTSS_SW_OPTION_ZTP)
                    ".ztp_only { display: none; }\r\n"
#endif /* !VTSS_SW_OPTION_ZTP */

#if defined(VTSS_SW_OPTION_SPROUT) || !defined(VTSS_SPROUT_FW_VER_CHK)
                    ".stack_fw_chk_only { display: none; }\r\n"
#endif /* !VTSS_SW_OPTION_SPROUT || !VTSS_SPROUT_FW_VER_CHK */
                    "%s"
                    , strlen(misc_software_code_revision_txt()) == 0 ? ".SYS_CODE_REV {display:none;}\r\n" : ""
                   );
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/
web_lib_config_js_tab_entry(SYS_lib_config_css);

/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(SYS_lib_filter_css);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
#ifndef VTSS_SW_OPTION_USERS
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_passwd, "/config/passwd", handler_config_passwd);
#endif /* VTSS_SW_OPTION_USERS */
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_sys, "/stat/sys", handler_stat_sys);
#if defined(VTSS_SW_OPTION_PSU)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_sys_psu_status, "/stat/sys_psu_status", handler_status_psu);
#endif /* VTSS_SW_OPTION_PSU */
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_sys_led_status, "/stat/sys_led_status", handler_status_led);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_sys_licenses,   "/stat/sys_licenses",   handler_status_licenses);

