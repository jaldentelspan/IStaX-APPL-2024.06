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
#include "vtss_auth_api.h"
#include "mgmt_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif
#include "vtss/appl/ssh.h"
#if defined(VTSS_SW_OPTION_FAST_CGI)
#include "fast_cgi_api.h"
#endif

#define AUTH_WEB_BUF_LEN 512

#ifdef VTSS_SW_OPTION_TACPLUS
static void AUTH_WEB_tacacs_server_get(const char *host, vtss_appl_auth_tacacs_server_conf_t &server)
{
    int i;

    for (i = 0; i < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; i++) {
        if (vtss_appl_auth_tacacs_server_get(i, &server) == VTSS_RC_OK && strcmp(server.host, host) == 0) {
            return;
        }
    }

    // Not found
    vtss_clear(server);
    strcpy(server.host, host);
}
#endif

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

#ifdef VTSS_SW_OPTION_RADIUS
static i32 handler_config_auth_radius(CYG_HTTPD_STATE *p)
{
    vtss_appl_auth_radius_global_conf_t conf;
    vtss_appl_auth_radius_server_conf_t server;
    char                                buf[INET6_ADDRSTRLEN];
    int                                 cnt, i;
    mesa_rc                             rc;
    const char                          *str;
    ulong                               val = 0;
    size_t                              len;
    vtss_auth_host_index_t              ix;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {

        if ((rc = vtss_appl_auth_radius_global_conf_get(&conf)) != VTSS_RC_OK) {
            const char *err = error_txt(rc);
            send_custom_error(p, "Authentication Error", err, strlen(err));
            return -1; // Do not further search the file system.
        }

        if (cyg_httpd_form_varable_long_int(p, "timeout", &val)) {
            conf.timeout = val;
        }
        if (cyg_httpd_form_varable_long_int(p, "retransmit", &val)) {
            conf.retransmit = val;
        }
        if (cyg_httpd_form_varable_long_int(p, "deadtime", &val)) {
            conf.deadtime = val;
        }
        if (cyg_httpd_form_varable_long_int(p, "change_secret_key", &val) && val == 1) {
            if ((str = cyg_httpd_form_varable_string(p, "key", &len))) {
                (void) cgi_unescape(str, conf.key, len, sizeof(conf.key)); // Key needs to be cgi_unescaped (e.g. %20 -> ' ').
            } else {
                strcpy(conf.key, "");
            }
            conf.encrypted = FALSE;
        }
        if ((str = cyg_httpd_form_varable_string(p, "nas-ip-address", &len))) {
            if (len) {
                (void) cgi_unescape(str, buf, len, sizeof(buf));
                if (mgmt_txt2ipv4_ext(buf, &conf.nas_ip_address, NULL, FALSE, FALSE, FALSE, FALSE) == VTSS_RC_OK) { // This has already been verified in web page
                    conf.nas_ip_address_enable = TRUE;
                }
            } else {
                conf.nas_ip_address_enable = FALSE;
            }
        }
        if ((str = cyg_httpd_form_varable_string(p, "nas-ipv6-address", &len))) {
            if (len) {
                (void) cgi_unescape(str, buf, len, sizeof(buf));
                if (mgmt_txt2ipv6(buf, &conf.nas_ipv6_address) == VTSS_RC_OK) { // This has already been verified in web page
                    conf.nas_ipv6_address_enable = TRUE;
                }
            } else {
                conf.nas_ipv6_address_enable = FALSE;
            }
        }
        if ((str = cyg_httpd_form_varable_string(p, "nas-identifier", &len))) {
            (void) cgi_unescape(str, conf.nas_identifier, len, sizeof(conf.nas_identifier));
        }

        if ((rc = vtss_appl_auth_radius_global_conf_set(&conf)) != VTSS_RC_OK) {
            const char *err = error_txt(rc);
            send_custom_error(p, "Authentication Error", err, strlen(err));
            return -1; // Do not further search the file system.
        }

        for (i = 0; i < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; i++ ) {
            memset(&server, 0, sizeof(server));   /* Nuke entry */
            if ((str = cyg_httpd_form_variable_str_fmt(p, &len, "host_%d", i))) {
                (void) cgi_unescape(str, server.host, len, sizeof(server.host)); // Host needs to be cgi_unescaped (e.g. %20 -> ' ').

                if (cyg_httpd_form_variable_long_int_fmt(p, &val, "auth_port_%d", i)) {
                    server.auth_port = val;
                }
                if (cyg_httpd_form_variable_long_int_fmt(p, &val, "acct_port_%d", i)) {
                    server.acct_port = val;
                }
                if (cyg_httpd_form_variable_long_int_fmt(p, &val, "timeout_%d", i)) {
                    server.timeout = val;
                }
                if (cyg_httpd_form_variable_long_int_fmt(p, &val, "retransmit_%d", i)) {
                    server.retransmit = val;
                }
                if ((str = cyg_httpd_form_variable_str_fmt(p, &len, "key_change_%d", i))) {
                    if ((str = cyg_httpd_form_variable_str_fmt(p, &len, "key_%d", i))) {
                        (void) cgi_unescape(str, server.key, len, sizeof(server.key)); // Key needs to be cgi_unescaped (e.g. %20 -> ' ').
                    } else {
                        strcpy(server.key, "");
                    }
                    server.encrypted = FALSE;
                }
                // host_config, delete entries
                if (cyg_httpd_form_variable_check_fmt(p, "delete_%d", i)) {
                    (void) vtss_appl_auth_radius_server_del(&server);
                } else {
                    // host_config, add entries
                    rc = vtss_appl_auth_radius_server_add(&server);
                    if (rc != VTSS_RC_OK) {
                        const char *err = error_txt(rc);
                        send_custom_error(p, "Authentication Error", err, strlen(err));
                        return -1; // Do not further search the file system.
                    }
                }
            }
        }
        redirect(p, "/auth_radius_config.htm");
    } else { /* CYG_HTTPD_METHOD_GET (+HEAD) */
        char encoded_host[3 * VTSS_APPL_AUTH_HOST_LEN];
        char encoded_ipv4[3 * INET_ADDRSTRLEN];
        char encoded_ipv6[3 * INET6_ADDRSTRLEN];
        /*
          # Configuration format:
          #
          # <timeout>#<retransmit>#<deadtime>#<nas_ip_address>#<nas_ipv6_address>#<nas_identifier>#<host_config>
          #
          # timeout          :== unsigned int # global timeout
          # retransmit       :== unsigned int # global retransmit
          # deadtime         :== unsigned int # global deadtime
          # key              :== string       # global key
          # nas_ip_address   :== string       # global nas-ip-address (Attribute 4)
          # nas_ipv6_address :== string       # global nas-ipv6-address (Attribute 95)
          # nas_identifier   :== string       # global nas-ip-address (Attribute 32)
          #
          # host_config :== <host 0>/<host 1>/...<host n>
          #   host x := <hostname>|<auth_port>|<acct_port>|<timeout>|<retransmit>
          #     hostname   :== string
          #     auth_port  :== 0..0xffff
          #     acct_port  :== 0..0xffff
          #     timeout    :== unsigned int
          #     retransmit :== unsigned int
          #     key        :== string
          #
        */
        cyg_httpd_start_chunked("html");
        if (vtss_appl_auth_radius_global_conf_get(&conf) == VTSS_RC_OK) {
            // timeout, retransmit, deadtime, key, nas_ip_address, nas_ipv6_address and nas_identifier
            (void) cgi_escape(conf.nas_ip_address_enable ? misc_ipv4_txt(conf.nas_ip_address, buf) : "", encoded_ipv4);
            (void) cgi_escape(conf.nas_ipv6_address_enable ? misc_ipv6_txt(&conf.nas_ipv6_address, buf) : "", encoded_ipv6);
            (void) cgi_escape(conf.nas_identifier, encoded_host); // <- Using encoded_host here
            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#%u#%u#%s#%s#%s",
                           conf.timeout,
                           conf.retransmit,
                           conf.deadtime,
                           encoded_ipv4,
                           encoded_ipv6,
                           encoded_host);
            cyg_httpd_write_chunked(p->outbuffer, cnt);

            // host_config
            for (i = ix = 0; ix < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; ix++) {
                if ((vtss_appl_auth_radius_server_get(ix, &server) == VTSS_RC_OK) && (strlen(server.host) > 0)) {
                    (void) cgi_escape(server.host, encoded_host);

                    cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s|%u|%u|%u|%u",
                                   (i == 0) ? "#" : "/",
                                   encoded_host,
                                   server.auth_port,
                                   server.acct_port,
                                   server.timeout,
                                   server.retransmit);
                    cyg_httpd_write_chunked(p->outbuffer, cnt);
                    i++;
                }
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
static i32 handler_config_auth_tacacs(CYG_HTTPD_STATE *p)
{
    vtss_appl_auth_tacacs_global_conf_t conf;
    vtss_appl_auth_tacacs_server_conf_t server;
    char                                host[VTSS_APPL_AUTH_HOST_LEN];
    int                                 cnt, i;
    mesa_rc                             rc;
    const char                          *str;
    ulong                               val = 0;
    size_t                              len;
    vtss_auth_host_index_t              ix;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {

        if ((rc = vtss_appl_auth_tacacs_global_conf_get(&conf)) != VTSS_RC_OK) {
            const char *err = error_txt(rc);
            send_custom_error(p, "Authentication Error", err, strlen(err));
            return -1; // Do not further search the file system.
        }

        if (cyg_httpd_form_varable_long_int(p, "timeout", &val)) {
            conf.timeout = val;
        }
        if (cyg_httpd_form_varable_long_int(p, "deadtime", &val)) {
            conf.deadtime = val;
        }
        if (cyg_httpd_form_varable_long_int(p, "change_secret_key", &val) && val == 1) {
            if ((str = cyg_httpd_form_varable_string(p, "key", &len))) {
                (void) cgi_unescape(str, conf.key, len, sizeof(conf.key)); // Key needs to be cgi_unescaped (e.g. %20 -> ' ').
            } else {
                strcpy(conf.key, "");
            }
            conf.encrypted = FALSE;
        }

        if ((rc = vtss_appl_auth_tacacs_global_conf_set(&conf)) != VTSS_RC_OK) {
            const char *err = error_txt(rc);
            send_custom_error(p, "Authentication Error", err, strlen(err));
            return -1; // Do not further search the file system.
        }

        for (i = 0; i < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; i++ ) {
            if ((str = cyg_httpd_form_variable_str_fmt(p, &len, "host_%d", i)) == nullptr) {
                continue;
            }

            (void)cgi_unescape(str, host, len, sizeof(host)); // Host needs to be cgi_unescaped (e.g. %20 -> ' ').

            // Get old configuration - if any
            AUTH_WEB_tacacs_server_get(host, server);

            if (cyg_httpd_form_variable_long_int_fmt(p, &val, "port_%d", i)) {
                server.port = val;
            }

            if (cyg_httpd_form_variable_long_int_fmt(p, &val, "timeout_%d", i)) {
                server.timeout = val;
            }

            if ((str = cyg_httpd_form_variable_str_fmt(p, &len, "key_change_%d", i))) {
                if ((str = cyg_httpd_form_variable_str_fmt(p, &len, "key_%d", i))) {
                    (void)cgi_unescape(str, server.key, len, sizeof(server.key)); // Key needs to be cgi_unescaped (e.g. %20 -> ' ').
                } else {
                    strcpy(server.key, "");
                }

                server.encrypted = FALSE;
            }

            // host_config, delete entries
            if (cyg_httpd_form_variable_check_fmt(p, "delete_%d", i)) {
                (void)vtss_appl_auth_tacacs_server_del(&server);
            } else {
                // host_config, add entries
                rc = vtss_appl_auth_tacacs_server_add(&server);
                if (rc != VTSS_RC_OK) {
                    const char *err = error_txt(rc);
                    send_custom_error(p, "Authentication Error", err, strlen(err));
                    return -1; // Do not further search the file system.
                }
            }
        }
        redirect(p, "/auth_tacacs_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        char                  encoded_host[3 * VTSS_APPL_AUTH_HOST_LEN];
        /*
          # Configuration format:
          #
          # <timeout>#<deadtime>#<global_key_configured>#<host_config>
          #
          # timeout               :== unsigned int # global timeout
          # deadtime              :== unsigned int # global deadtime
          # global_key_configured :== boolean
          #
          # host_config :== <host 0>/<host 1>/...<host n>
          #   host x := <hostname>|<port>|<timeout>|<key_configured>
          #     hostname       :== string
          #     port           :== 0..0xffff
          #     timeout        :== unsigned int
          #     key_configured := boolean
          #
        */
        cyg_httpd_start_chunked("html");
        if (vtss_appl_auth_tacacs_global_conf_get(&conf) == VTSS_RC_OK) {
            // timeout, retransmit and deadtime
            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#%u#%d",
                           conf.timeout,
                           conf.deadtime,
                           conf.key[0] != '\0');
            cyg_httpd_write_chunked(p->outbuffer, cnt);

            // host_config
            for (i = ix = 0; ix < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; ix++) {
                if ((vtss_appl_auth_tacacs_server_get(ix, &server) == VTSS_RC_OK) && (strlen(server.host) > 0)) {
                    (void) cgi_escape(server.host, encoded_host);

                    cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s|%u|%u|%d",
                                   (i == 0) ? "#" : "/",
                                   encoded_host,
                                   server.port,
                                   server.timeout,
                                   server.key[0] != '\0');
                    cyg_httpd_write_chunked(p->outbuffer, cnt);
                    i++;
                }
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* VTSS_SW_OPTION_TACPLUS */

/* Returns TRUE if agent is not part of the build */
static BOOL invalid_agent(vtss_appl_auth_agent_t agent)
{
    if (VTSS_APPL_AUTH_AGENT_LAST <= agent) {
        return TRUE;
    }
#ifdef VTSS_APPL_AUTH_ENABLE_CONSOLE
    if (VTSS_APPL_AUTH_AGENT_CONSOLE == agent) {
        return !cli_module_enabled();
    }
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
    if (VTSS_APPL_AUTH_AGENT_TELNET == agent) {
        return !telnet_module_enabled();
    }
#endif
#ifdef VTSS_SW_OPTION_SSH
    if (VTSS_APPL_AUTH_AGENT_SSH == agent) {
        return FALSE;
    }
#endif
#ifdef VTSS_SW_OPTION_WEB
    if (VTSS_APPL_AUTH_AGENT_HTTP == agent) {
        return !web_module_enabled();
    }
#endif
    return TRUE;
}

/* Returns TRUE if authentication method is not part of the build */
static BOOL invalid_method(vtss_appl_auth_authen_method_t method)
{
    if (VTSS_APPL_AUTH_AUTHEN_METHOD_LAST <= method) {
        return TRUE;
    }
#ifndef VTSS_SW_OPTION_RADIUS
    if (VTSS_APPL_AUTH_AUTHEN_METHOD_RADIUS == method) {
        return TRUE;
    }
#endif
#ifndef VTSS_SW_OPTION_TACPLUS
    if (VTSS_APPL_AUTH_AUTHEN_METHOD_TACACS == method) {
        return TRUE;
    }
#endif
    return FALSE;
}

static i32 handler_config_auth_method(CYG_HTTPD_STATE *p)
{
    int                                cnt, i, j, var_value;
    mesa_rc                            rc;
    vtss_appl_auth_authen_agent_conf_t authen_conf;
#ifdef VTSS_SW_OPTION_TACPLUS
    vtss_appl_auth_author_agent_conf_t author_conf;
    vtss_appl_auth_acct_agent_conf_t   acct_conf;
#endif /* VTSS_SW_OPTION_TACPLUS */

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {

        // --- Update Authentication Configuration ---
        for (i = 0; i < VTSS_APPL_AUTH_AGENT_LAST; i++ ) {
            if (invalid_agent((vtss_appl_auth_agent_t)i)) {
                continue;
            }

            if ((rc = vtss_appl_auth_authen_agent_conf_get((vtss_appl_auth_agent_t)i, &authen_conf)) != VTSS_RC_OK) {
                const char *err = error_txt(rc);
                send_custom_error(p, "Authentication Error", err, strlen(err));
                return -1; // Do not further search the file system.
            }

            for (j = 0; j <= VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX; j++ ) {
                // authen_method_%d_%d: First %d is client, second %d is method
                if (cyg_httpd_form_variable_int_fmt(p, &var_value, "authen_method_%d_%d", i, j)) {
                    authen_conf.method[j] = (vtss_appl_auth_authen_method_t)var_value;
                }

            }

            if ((rc = vtss_appl_auth_authen_agent_conf_set((vtss_appl_auth_agent_t)i, &authen_conf)) != VTSS_RC_OK) {
                const char *err = error_txt(rc);
                send_custom_error(p, "Authentication Error", err, strlen(err));
                return -1; // Do not further search the file system.
            }
        }

#ifdef VTSS_SW_OPTION_TACPLUS
        // --- Update Authorization Configuration ---
        for (i = 0; i < VTSS_APPL_AUTH_AGENT_LAST; i++ ) {
            if (invalid_agent((vtss_appl_auth_agent_t)i) || VTSS_APPL_AUTH_AGENT_HTTP == i) {
                continue;
            }

            if ((rc = vtss_appl_auth_author_agent_conf_get((vtss_appl_auth_agent_t)i, &author_conf)) != VTSS_RC_OK) {
                const char *err = error_txt(rc);
                send_custom_error(p, "Authorization Error", err, strlen(err));
                return -1; // Do not further search the file system.
            }

            if (cyg_httpd_form_variable_int_fmt(p, &var_value, "author_method_%d", i)) {
                author_conf.method = (vtss_appl_auth_author_method_t)var_value;
            }
            if (author_conf.method != VTSS_APPL_AUTH_AUTHOR_METHOD_NONE) {
                author_conf.cmd_enable = TRUE;
                if (cyg_httpd_form_variable_int_fmt(p, &var_value, "author_cmd_lvl_%d", i)) {
                    author_conf.cmd_priv_lvl = var_value;
                } else {
                    author_conf.cmd_priv_lvl = 0;
                }
                author_conf.cfg_cmd_enable = cyg_httpd_form_variable_check_fmt(p, "author_cfg_cmd_%d", i);
            } else {
                author_conf.cmd_enable = FALSE;
                author_conf.cmd_priv_lvl = 0;
                author_conf.cfg_cmd_enable = FALSE;
            }

            if ((rc = vtss_appl_auth_author_agent_conf_set((vtss_appl_auth_agent_t)i, &author_conf)) != VTSS_RC_OK) {
                const char *err = error_txt(rc);
                send_custom_error(p, "Authorization Error", err, strlen(err));
                return -1; // Do not further search the file system.
            }
        }

        // --- Update Accounting Configuration ---
        for (i = 0; i < VTSS_APPL_AUTH_AGENT_LAST; i++ ) {
            if (invalid_agent((vtss_appl_auth_agent_t)i) || VTSS_APPL_AUTH_AGENT_HTTP == i) {
                continue;
            }

            if ((rc = vtss_appl_auth_acct_agent_conf_get((vtss_appl_auth_agent_t)i, &acct_conf)) != VTSS_RC_OK) {
                const char *err = error_txt(rc);
                send_custom_error(p, "Accounting Error", err, strlen(err));
                return -1; // Do not further search the file system.
            }

            if (cyg_httpd_form_variable_int_fmt(p, &var_value, "acct_method_%d", i)) {
                acct_conf.method = (vtss_appl_auth_acct_method_t)var_value;
            }
            if (acct_conf.method != VTSS_APPL_AUTH_ACCT_METHOD_NONE) {
                if (cyg_httpd_form_variable_int_fmt(p, &var_value, "acct_cmd_lvl_%d", i)) {
                    acct_conf.cmd_enable = TRUE;
                    acct_conf.cmd_priv_lvl = var_value;
                } else {
                    acct_conf.cmd_enable = FALSE;
                    acct_conf.cmd_priv_lvl = 0;
                }
                acct_conf.exec_enable = cyg_httpd_form_variable_check_fmt(p, "acct_exec_%d", i);
            } else {
                acct_conf.cmd_enable = FALSE;
                acct_conf.cmd_priv_lvl = 0;
                acct_conf.exec_enable = FALSE;
            }

            if ((rc = vtss_appl_auth_acct_agent_conf_set((vtss_appl_auth_agent_t)i, &acct_conf)) != VTSS_RC_OK) {
                const char *err = error_txt(rc);
                send_custom_error(p, "Accounting Error", err, strlen(err));
                return -1; // Do not further search the file system.
            }
        }
#endif /* VTSS_SW_OPTION_TACPLUS */

        redirect(p, "/auth_method_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        /*
         * Configuration format:
         *
         * <authen_data>!<author_data>!<acct_data>
         *
         * authen_data          :== <authen_clients>#<authen_method_name>#<authen_method_num>
         *   authen_clients     :== <client 0>/<client 1>/...<client n>
         *     client x         :== <client_name>|<client_num>|<methods>
         *       client_name    :== "console" or "telnet" or "ssh" or "http"
         *       client_num     :== 0..3 # the corresponding value for client_name
         *       methods        :== <method 0>,<method 1>,...<method n> # List of configured methods. E.g {3,1,0} ~ {tacacs, local, no}
         *         method x     :== 0..3 # the method value
         *
         *   authen_method_name :== <name 0>/<name 1>/...<name n>
         *     name x           :== "no" or "local" or "radius" or "tacacs"
         *
         *   authen_method_num  :== <num 0>/<num 1>/...<num n>
         *     num x            :== 0..3 # the corresponding value for authen_method_name
         *
         * author_data          :== <author_clients>#<author_method_name>#<author_method_num>
         *   author_clients     :== <client 0>/<client 1>/...<client n>
         *     client x         :== <client_name>|<client_num>|<method>|<cmd_lvl>|<cfg_cmd>
         *       client_name    :== "console" or "telnet" or "ssh"
         *       client_num     :== 0..2   # the corresponding value for client_name
         *       method         :== 0 or 3 # "no" or "tacacs"
         *       cmd_lvl        :== 0..15  # minimum command privilege level
         *       cfg_cmd        :== 0..1   # also authorize configuration commands
         *
         *   author_method_name :== <name 0>/<name 1>/...<name n>
         *     name x           :== "no" or "tacacs"
         *
         *   author_method_num  :== <num 0>/<num 1>/...<num n>
         *     num x            :== 0 or 3 # the corresponding value for author_method_name
         *
         * acct_data            :== <acct_clients>#<acct_method_name>#<acct_method_num>
         *   acct_clients       :== <client 0>/<client 1>/...<client n>
         *     client x         :== <client_name>|<client_num>|<method>|<cmd_lvl>|<exec>
         *       client_name    :== "console" or "telnet" or "ssh"
         *       client_num     :== 0..2  * the corresponding value for client_name
         *       method         :== 0 or 3 # "no" or "tacacs"
         *       cmd_lvl        :== 0..15 or -1 # minimum command privilege level. -1 means disabled
         *       exec           :== 0..1  * Enable exec accounting
         *
         *   acct_method_name   :== <name 0>/<name 1>/...<name n>
         *     name x           :== "no" or "tacacs"
         *
         *   acct_method_num    :== <num 0>/<num 1>/...<num n>
         *     num x            :== 0 or 3 # the corresponding value for acct_method_name
         *
         */
        int method_cnt, client_cnt;

        cyg_httpd_start_chunked("html");

        // --- Create Authentication Configuration ---
        client_cnt = 0;
        for (i = 0; i < VTSS_APPL_AUTH_AGENT_LAST; i++ ) {
            if (invalid_agent((vtss_appl_auth_agent_t)i)) {
                continue;
            }
            if (vtss_appl_auth_authen_agent_conf_get((vtss_appl_auth_agent_t)i, &authen_conf) == VTSS_RC_OK) {
                cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s|%d|",
                               client_cnt++ ? "/" : "",
                               vtss_appl_auth_agent_name((vtss_appl_auth_agent_t)i),
                               i);
                cyg_httpd_write_chunked(p->outbuffer, cnt);
                method_cnt = 0;
                for (j = 0; j <= VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX; j++) {
                    cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d",
                                   method_cnt++ ? "," : "",
                                   authen_conf.method[j]);
                    cyg_httpd_write_chunked(p->outbuffer, cnt);
                }
            }
        }

        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "#");
        cyg_httpd_write_chunked(p->outbuffer, cnt);

        // method_name
        for (i = 0; i < VTSS_APPL_AUTH_AUTHEN_METHOD_LAST; i++ ) {
            if (invalid_method((vtss_appl_auth_authen_method_t)i)) {
                continue;
            }
            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s",
                           (i == 0) ? "" : "/",
                           vtss_appl_auth_authen_method_name((vtss_appl_auth_authen_method_t)i));
            cyg_httpd_write_chunked(p->outbuffer, cnt);
        }

        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "#");
        cyg_httpd_write_chunked(p->outbuffer, cnt);

        // method_num
        for (i = 0; i < VTSS_APPL_AUTH_AUTHEN_METHOD_LAST; i++ ) {
            if (invalid_method((vtss_appl_auth_authen_method_t)i)) {
                continue;
            }
            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%d",
                           (i == 0) ? "" : "/",
                           i);
            cyg_httpd_write_chunked(p->outbuffer, cnt);
        }

#ifdef VTSS_SW_OPTION_TACPLUS
        // --- Create Authorization Configuration ---
        client_cnt = 0;
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "!");
        cyg_httpd_write_chunked(p->outbuffer, cnt);

        for (i = 0; i < VTSS_APPL_AUTH_AGENT_LAST; i++ ) {
            if (invalid_agent((vtss_appl_auth_agent_t)i) || VTSS_APPL_AUTH_AGENT_HTTP == i) {
                continue;
            }
            if (vtss_appl_auth_author_agent_conf_get((vtss_appl_auth_agent_t)i, &author_conf) == VTSS_RC_OK) {
                cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s|%d|%d|%d|%d",
                               client_cnt++ ? "/" : "",
                               vtss_appl_auth_agent_name((vtss_appl_auth_agent_t)i),
                               i,
                               author_conf.method,
                               author_conf.cmd_priv_lvl,
                               (author_conf.method != VTSS_APPL_AUTH_AUTHOR_METHOD_NONE) ? author_conf.cfg_cmd_enable : FALSE);
                cyg_httpd_write_chunked(p->outbuffer, cnt);
            }
        }

        // method_name
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%s/%s",
                       vtss_appl_auth_authen_method_name(VTSS_APPL_AUTH_AUTHEN_METHOD_NONE),
                       vtss_appl_auth_authen_method_name(VTSS_APPL_AUTH_AUTHEN_METHOD_TACACS));
        cyg_httpd_write_chunked(p->outbuffer, cnt);

        // method_num
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%d/%d",
                       VTSS_APPL_AUTH_AUTHEN_METHOD_NONE,
                       VTSS_APPL_AUTH_AUTHEN_METHOD_TACACS);
        cyg_httpd_write_chunked(p->outbuffer, cnt);

        // --- Create Accounting Configuration ---
        client_cnt = 0;
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "!");
        cyg_httpd_write_chunked(p->outbuffer, cnt);

        for (i = 0; i < VTSS_APPL_AUTH_AGENT_LAST; i++ ) {
            if (invalid_agent((vtss_appl_auth_agent_t)i) || VTSS_APPL_AUTH_AGENT_HTTP == i) {
                continue;
            }
            if (vtss_appl_auth_acct_agent_conf_get((vtss_appl_auth_agent_t)i, &acct_conf) == VTSS_RC_OK) {
                cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s|%d|%d|%d|%d",
                               client_cnt++ ? "/" : "",
                               vtss_appl_auth_agent_name((vtss_appl_auth_agent_t)i),
                               i,
                               acct_conf.method,
                               ((acct_conf.method != VTSS_APPL_AUTH_ACCT_METHOD_NONE) && acct_conf.cmd_enable) ? acct_conf.cmd_priv_lvl : -1,
                               (acct_conf.method != VTSS_APPL_AUTH_ACCT_METHOD_NONE) ? acct_conf.exec_enable : FALSE);
                cyg_httpd_write_chunked(p->outbuffer, cnt);
            }
        }

        // method_name
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%s/%s",
                       vtss_appl_auth_authen_method_name(VTSS_APPL_AUTH_AUTHEN_METHOD_NONE),
                       vtss_appl_auth_authen_method_name(VTSS_APPL_AUTH_AUTHEN_METHOD_TACACS));
        cyg_httpd_write_chunked(p->outbuffer, cnt);

        // method_num
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%d/%d",
                       VTSS_APPL_AUTH_AUTHEN_METHOD_NONE,
                       VTSS_APPL_AUTH_AUTHEN_METHOD_TACACS);
        cyg_httpd_write_chunked(p->outbuffer, cnt);
#endif /* VTSS_SW_OPTION_TACPLUS */

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t auth_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[AUTH_WEB_BUF_LEN];
    (void) snprintf(buff, AUTH_WEB_BUF_LEN,
                    "var configAuthServerCnt = %d;\n"
                    "var configAuthHostLen = %d;\n"
                    "var configAuthKeyLen = %d;\n"
                    "var configAuthRadiusAuthPortDef = %d;\n"
                    "var configAuthRadiusAcctPortDef = %d;\n"
                    "var configAuthTacacsPortDef = %d;\n"
                    "var configAuthTimeoutDef = %d;\n"
                    "var configAuthTimeoutMin = %d;\n"
                    "var configAuthTimeoutMax = %d;\n"
                    "var configAuthRetransmitDef = %d;\n"
                    "var configAuthRetransmitMin = %d;\n"
                    "var configAuthRetransmitMax = %d;\n"
                    "var configAuthDeadtimeDef = %d;\n"
                    "var configAuthDeadtimeMin = %d;\n"
                    "var configAuthDeadtimeMax = %d;\n"
                    ,
                    VTSS_APPL_AUTH_NUMBER_OF_SERVERS,
                    VTSS_APPL_AUTH_HOST_LEN - 1,
                    VTSS_APPL_AUTH_UNENCRYPTED_KEY_INPUT_LEN,
                    VTSS_APPL_AUTH_RADIUS_AUTH_PORT_DEFAULT,
                    VTSS_APPL_AUTH_RADIUS_ACCT_PORT_DEFAULT,
                    VTSS_APPL_AUTH_TACACS_PORT_DEFAULT,
                    VTSS_APPL_AUTH_TIMEOUT_DEFAULT,
                    VTSS_APPL_AUTH_TIMEOUT_MIN,
                    VTSS_APPL_AUTH_TIMEOUT_MAX,
                    VTSS_APPL_AUTH_RETRANSMIT_DEFAULT,
                    VTSS_APPL_AUTH_RETRANSMIT_MIN,
                    VTSS_APPL_AUTH_RETRANSMIT_MAX,
                    VTSS_APPL_AUTH_DEADTIME_DEFAULT,
                    VTSS_APPL_AUTH_DEADTIME_MIN,
                    VTSS_APPL_AUTH_DEADTIME_MAX);
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(auth_lib_config_js);

#if !defined(VTSS_SW_OPTION_RADIUS) && !defined(VTSS_SW_OPTION_DOT1X_ACCT) && !defined(VTSS_SW_OPTION_TACPLUS)
/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t auth_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[AUTH_WEB_BUF_LEN];
    (void) snprintf(buff, AUTH_WEB_BUF_LEN, ".AUTH_SERVER { display: none; }\r\n");
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(auth_lib_filter_css);
#endif /* !defined(VTSS_SW_OPTION_RADIUS) && !defined(VTSS_SW_OPTION_DOT1X_ACCT) && !defined(VTSS_SW_OPTION_TACPLUS) */

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

#ifdef VTSS_SW_OPTION_RADIUS
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_auth_radius, "/config/auth_radius_config", handler_config_auth_radius);
#endif /* VTSS_SW_OPTION_RADIUS */

#ifdef VTSS_SW_OPTION_TACPLUS
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_auth_tacacs, "/config/auth_tacacs_config", handler_config_auth_tacacs);
#endif /* VTSS_SW_OPTION_TACPLUS */

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_auth_method, "/config/auth_method_config", handler_config_auth_method);

