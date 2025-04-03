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
#include "vtss_snmp_api.h"
#include "vtss_private_trap.hxx"
#include "ip_utils.hxx"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#include "port_iter.hxx"
#include "mgmt_api.h"
/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

#include "vtss_snmp_linux.h"

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/
static int snmp_handler_check(CYG_HTTPD_STATE *p)
{
    if (!snmp_module_enabled()) {
        return 1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    // There is no SNMP web privilege existing in privilege module, only security
    // web privilege is included since the SNMP configuration web pages is a
    // sub-tree of security menu in Web UI.
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY)) {
        return 1;
    }
#endif

    return 0;
}

#define SNMP_HANDLER_CHECK(p) { if (snmp_handler_check(p)) return -1; }

static i32 handler_config_snmp(CYG_HTTPD_STATE *p)
{
    int         ct;
    snmp_conf_t snmp_conf, newconf;
    int         var_value;
    const char  *var_string;
    size_t      len = 64 * sizeof(char);
    uchar       str_buff[16];
#ifdef VTSS_SW_OPTION_IPV6
    int         ipv6_supported = 1;
#else
    int         ipv6_supported = 0;
#endif /* VTSS_SW_OPTION_IPV6 */
    ulong       idx;
    ulong       pval;

    SNMP_HANDLER_CHECK(p);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        if (snmp_mgmt_snmp_conf_get(&snmp_conf) != VTSS_RC_OK) {
            redirect(p, "/snmp.htm");;
            return -1;
        }
        newconf = snmp_conf;

        if (cyg_httpd_form_varable_int(p, "snmp_mode", &var_value)) {
            newconf.mode = var_value;
        }

        var_string = cyg_httpd_form_varable_string(p, "snmpv3_engineid", &len);
        if (len > 0) {
            for (idx = 0; idx < len; idx = idx + 2) {
                memcpy(str_buff, var_string + idx, 2);
                str_buff[2] = '\0';
                if (cyg_httpd_str_to_hex((const char *) str_buff, &pval) == FALSE) {
                    continue;
                }
                newconf.engineid[idx / 2] = (uchar)pval;
            }
            newconf.engineid_len = len / 2;
        }

        if (memcmp(&newconf, &snmp_conf, sizeof(newconf)) != 0) {
            T_D("Calling snmp_mgmt_snmp_conf_set()");
            if (snmp_mgmt_snmp_conf_set(&newconf) < 0) {
                T_E("snmp_mgmt_snmp_conf_set(): failed");
            }
        }

        redirect(p, "/snmp.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data

            Format: [ipv6_supported]/[snmp_mode]/[snmpv3_engineid]/[aes_supported]
        */
        if (snmp_mgmt_snmp_conf_get(&snmp_conf) != VTSS_RC_OK) {
            (void)cyg_httpd_end_chunked();
            return -1;
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/", ipv6_supported);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u",
                      snmp_conf.mode);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        char buf[SNMPV3_MAX_ENGINE_ID_LEN * 2 + 1];
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s",
                      misc_engineid2str(buf, snmp_conf.engineid, snmp_conf.engineid_len));
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
#if defined(NETSNMP_USE_OPENSSL)
        (void)cyg_httpd_write_chunked("/1", 2);
#else
        (void)cyg_httpd_write_chunked("/0", 2);
#endif
        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_trap(CYG_HTTPD_STATE *p)
{
    int                 ct;
    char                ch;
    BOOL         change_flag = FALSE;
    vtss_trap_entry_t   entry;
    vtss_trap_conf_t    *conf = &entry.trap_conf;
    int                 count = 0;
    const u32           max_digest_length = (((3 * TRAP_MAX_NAME_LEN ) / 3 + (((3 * TRAP_MAX_NAME_LEN ) % 3) ? 1 : 0)) * 4);
    char                digest[max_digest_length + 1];
    char                encoded_string[3 * max_digest_length + 1], search_str[3 * max_digest_length + 7 + 1], ipStr[255];

    SNMP_HANDLER_CHECK(p);

    if (p->method == CYG_HTTPD_METHOD_POST) {

        memset(&entry, 0, sizeof(entry));
        while (trap_mgmt_conf_get_next(&entry) == VTSS_RC_OK) {
            // delete the entries if the user checked

            digest[0] = '\0';
            if (vtss_httpd_base64_encode(digest, sizeof(digest), entry.trap_conf_name, strlen(entry.trap_conf_name)) != VTSS_RC_OK) {
                T_E("Could not encode");
            };

            encoded_string[0] = '\0';
            (void) cgi_escape(digest, encoded_string);

            T_D("encode_string = %s", encoded_string);
            sprintf(search_str, "delete_%s", encoded_string);

            if (cyg_httpd_form_varable_find(p, search_str)) { /* "delete" if checked */
                // set configuration
                change_flag = TRUE;
                entry.valid = FALSE;
                (void) trap_mgmt_conf_set(&entry);
            }

            if (change_flag) {
                // save configuration
            }
        }

        redirect(p, "/trap.htm");

    } else {

        //Format: <max_entries_num>,<name>/<enable>/<version>/<dip>/<dport>|...,<remained_entries>

        (void)cyg_httpd_start_chunked("html");

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d", VTSS_TRAP_CONF_MAX);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        memset(&entry, 0, sizeof(entry));
        while (trap_mgmt_conf_get_next(&entry) == VTSS_RC_OK) {
            if (count++ == 0) {
                ch = ',';
            } else {
                ch = '|';
            }

            encoded_string[0] = '\0';
            (void) cgi_escape(entry.trap_conf_name, encoded_string);

            if (conf->dip.type == VTSS_INET_ADDRESS_TYPE_IPV4) {
                misc_ipv4_txt(conf->dip.address.ipv4, ipStr);
            } else if (conf->dip.type == VTSS_INET_ADDRESS_TYPE_IPV6) {
                misc_ipv6_txt(&conf->dip.address.ipv6, ipStr);
            } else if (conf->dip.type == VTSS_INET_ADDRESS_TYPE_DNS) {
                strncpy(ipStr, conf->dip.address.domain_name.name, 254);
                conf->dip.address.domain_name.name[254] = 0;
            } else {
                strcpy(ipStr, "0.0.0.0");
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%c%s/%d/%u/%s/%u", ch, encoded_string, conf->enable, conf->trap_version,
                          ipStr, conf->trap_port);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        }
        if ( 0 == count) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",%d", VTSS_TRAP_CONF_MAX - count);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_trap_detailed(CYG_HTTPD_STATE *p)
{
    int         ct;
    vtss_trap_entry_t trap_entry, trap_entry_tmp;
    int         var_value;
    const char  *var_string;
    size_t      len = 64 * sizeof(char);
    char        str_buff[40];
    char        host_buf[VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN + 1], ipStr[255];
    int         i;
    char        trap_conf_name[TRAP_MAX_NAME_LEN + 1] = "";
    vtss_trap_conf_t    *conf = &trap_entry.trap_conf;
#ifdef VTSS_SW_OPTION_IPV6
    int         ipv6_supported = 1;
#else
    int         ipv6_supported = 0;
#endif /* VTSS_SW_OPTION_IPV6 */
    snmpv3_users_conf_t user_conf;
    ulong       idx;
    ulong       pval;
    char        engineid_txt[SNMPV3_MAX_ENGINE_ID_LEN * 2 + 1];

    SNMP_HANDLER_CHECK(p);

    if (p->method == CYG_HTTPD_METHOD_POST) {

        /*
           trap_conf_name, trap_mode, trap_version, trap_community, trap_dip, trap_dport, trap_inform_mode, trap_inform_timeout,
                trap_inform_retries, trap_security_engineid, trap_security_name,
                linkup_radio, linkdown_radio, lldp_radio, linkup_<port>, linkdwon_<port>, lldp_<port>
        */

        var_string = cyg_httpd_form_varable_string(p, "trap_conf_name", &len);
        if (len > 0) {
            if (cgi_unescape(var_string, trap_entry_tmp.trap_conf_name, len, sizeof(trap_entry_tmp.trap_conf_name)) == FALSE) {
                redirect(p, "/trap.htm");
                return -1;
            }
        }

        if ( VTSS_RC_OK != trap_mgmt_conf_get(&trap_entry_tmp)) {
            trap_mgmt_conf_default_get(&trap_entry_tmp);
        }

        memcpy(&trap_entry, &trap_entry_tmp, sizeof(trap_entry_tmp));

        if (cyg_httpd_form_varable_int(p, "trap_mode", &var_value)) {
            conf->enable = var_value;
        }
        if (cyg_httpd_form_varable_int(p, "trap_version", &var_value)) {
            conf->trap_version = var_value;
        }
        /* Remove Port and Trap Port parameters (always use port 161/162)
        if (cyg_httpd_form_varable_int(p, "trap_port", &var_value)) {
            newconf->trap_port = var_value;
        } */
        var_string = cyg_httpd_form_varable_string(p, "trap_community", &len);
        if (len > 0) {
            (void) cgi_unescape(var_string, conf->trap_communitySecret, len, sizeof(conf->trap_communitySecret));
            (void)AUTH_secret_key_cryptography(TRUE, conf->trap_communitySecret, conf->trap_encryptedSecret);
        } else {
            strcpy(conf->trap_communitySecret, "");
            strcpy(conf->trap_encryptedSecret, "");
        }
        var_string = cyg_httpd_form_varable_string(p, "trap_dip", &len);
        if (len > 0) {
            (void) cgi_unescape(var_string, host_buf, len, sizeof(host_buf));
        } else {
            host_buf[0] = '\0';
        }

        if (mgmt_txt2ipv4(host_buf, &conf->dip.address.ipv4, NULL, FALSE) == MESA_RC_OK) {
            conf->dip.type = VTSS_INET_ADDRESS_TYPE_IPV4;
#ifdef VTSS_SW_OPTION_IPV6
        } else if ( strchr(host_buf, ':')) {
            conf->dip.type = VTSS_INET_ADDRESS_TYPE_IPV6;
            mgmt_txt2ipv6(host_buf, &conf->dip.address.ipv6);
#endif /* VTSS_SW_OPTION_IPV6 */
        } else if (misc_str_is_domainname(host_buf) == VTSS_RC_OK) {
            conf->dip.type = VTSS_INET_ADDRESS_TYPE_DNS;
            strncpy(conf->dip.address.domain_name.name, host_buf, 254);
            conf->dip.address.domain_name.name[254] = 0;
        } else {
            conf->dip.type = VTSS_INET_ADDRESS_TYPE_IPV4;
            conf->dip.address.ipv4 = 0;
        }

        if (cyg_httpd_form_varable_int(p, "trap_dport", &var_value)) {
            conf->trap_port = var_value;
        }

        if (cyg_httpd_form_varable_int(p, "trap_inform_mode", &var_value)) {
            conf->trap_inform_mode = var_value;
        }
        if (cyg_httpd_form_varable_int(p, "trap_inform_timeout", &var_value)) {
            conf->trap_inform_timeout = var_value;
        }
        if (cyg_httpd_form_varable_int(p, "trap_inform_retries", &var_value)) {
            conf->trap_inform_retries = var_value;
        }
        var_string = cyg_httpd_form_varable_string(p, "trap_security_engineid", &len);
        if (len > 0) {
            for (idx = 0; idx < len; idx = idx + 2) {
                memcpy(str_buff, var_string + idx, 2);
                str_buff[2] = '\0';
                if (cyg_httpd_str_to_hex((const char *) str_buff, &pval) == FALSE) {
                    continue;
                }
                conf->trap_engineid[idx / 2] = (uchar)pval;
            }
            conf->trap_engineid_len = len / 2;
        }
        var_string = cyg_httpd_form_varable_string(p, "trap_security_name", &len);
        if (len > 0) {
            (void) cgi_unescape(var_string, conf->trap_security_name, len, sizeof(conf->trap_security_name));
        }

        /*
                linkup_radio, linkdown_radio, lldp_radio, warm_start, cold_start, authentication_fail, stp, rmon
        */


        trap_entry.valid = TRUE;
        if (trap_mgmt_conf_set(&trap_entry) < 0) {
            T_D("snmp_mgmt_snmp_conf_set(): failed");
        }

        redirect(p, "/trap.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        char encoded_string[3 * SNMP_MGMT_MAX_COMMUNITY_LEN + 1];

        var_string = cyg_httpd_form_varable_string(p, "conf_name", &len);

        if (vtss_httpd_base64_decode(trap_conf_name, sizeof(trap_conf_name), (char *)var_string, len) != VTSS_RC_OK) {
            T_E("Could not decode");
        }

        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: [ipv6_supported],[trap_conf_name1]|[trap_conf_name2]|...,
                   [trap_mode]/[trap_version]/[trap_community]/[trap_dip]/[trap_inform_mode]/[trap_inform_timeout]/[trap_inform_retries]/[trap_security_engineid]/[trap_security_name],
                   [trap_security_name1]|
        */

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d,", ipv6_supported);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        memset(&trap_entry, 0, sizeof(trap_entry));
        i = 0;
        while (VTSS_RC_OK == trap_mgmt_conf_get_next(&trap_entry)) {
            encoded_string[0] = '\0';
            (void) cgi_escape(trap_entry.trap_conf_name, encoded_string);
            if ( i++ == 0 ) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", encoded_string);
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%s", encoded_string);
            }
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        T_I("trap_mgmt_conf_get_next Done");
        strcpy(trap_entry.trap_conf_name, trap_conf_name);
        if (trap_entry.trap_conf_name[0] == 0 ) {
            trap_mgmt_conf_default_get(&trap_entry);
            T_I("trap_mgmt_conf_default_get Done");
        } else if (trap_mgmt_conf_get(&trap_entry) != VTSS_RC_OK) {
            T_I("Could not  get trap_mgmt_conf_get");
            (void)cyg_httpd_end_chunked();
            return -1;
        }

        encoded_string[0] = '\0';
        (void) cgi_escape(trap_entry.trap_conf_name, encoded_string);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",%s/", encoded_string);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        encoded_string[0] = '\0';
        (void) cgi_escape(conf->trap_communitySecret, encoded_string);

        if (conf->dip.type == VTSS_INET_ADDRESS_TYPE_IPV4) {
            misc_ipv4_txt(conf->dip.address.ipv4, ipStr);
        } else if (conf->dip.type == VTSS_INET_ADDRESS_TYPE_IPV6) {
            misc_ipv6_txt(&conf->dip.address.ipv6, ipStr);
        } else if (conf->dip.type == VTSS_INET_ADDRESS_TYPE_DNS) {
            strncpy(ipStr, conf->dip.address.domain_name.name, 254);
            conf->dip.address.domain_name.name[254] = 0;
        } else {
            strcpy(ipStr, "");
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%u/%s/%s/%d/%u/%u/%u",
                      conf->enable,
                      conf->trap_version,
                      encoded_string,
                      ipStr,
                      conf->trap_port,
                      conf->trap_inform_mode,
                      conf->trap_inform_timeout,
                      conf->trap_inform_retries);

        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        /*remove for testing */
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s",
                      misc_engineid2str(engineid_txt, conf->trap_engineid, conf->trap_engineid_len));
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);


        if ( 0 == conf->trap_engineid_len) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s", SNMPV3_NONAME);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        } else {
            user_conf.engineid_len = conf->trap_engineid_len;
            memcpy(user_conf.engineid, conf->trap_engineid, conf->trap_engineid_len);
            strcpy(user_conf.user_name, conf->trap_security_name);
            (void) cgi_escape(conf->trap_security_name, encoded_string);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s",
                          (snmpv3_mgmt_users_conf_get(&user_conf, FALSE) == VTSS_RC_OK && user_conf.status == SNMP_MGMT_ROW_ACTIVE) ? encoded_string : SNMPV3_NONAME);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        strcpy(user_conf.user_name, SNMPV3_CONF_ACESS_GETFIRST);

        i = 0;

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s", SNMPV3_NONAME);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        while (snmpv3_mgmt_users_conf_get(&user_conf, TRUE) == VTSS_RC_OK) {
            if (user_conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            if (memcmp(conf->trap_engineid, user_conf.engineid, conf->trap_engineid_len > user_conf.engineid_len ? conf->trap_engineid_len : user_conf.engineid_len)) {
                continue;
            }
            if (cgi_escape(user_conf.user_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%s", encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        if ( 0 == i) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_stat_trap_dyna_source_name(CYG_HTTPD_STATE *p)
{
    int     ct;
    char    source_name[VTSS_APPL_TRAP_TABLE_NAME_SIZE + 1];

    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/trap_source.htm");
    } else {

        //Format: <source_name_1>/<source_name_2>...

        (void)cyg_httpd_start_chunked("html");

        strcpy(source_name, "");
        while (vtss_appl_snmp_private_trap_listen_get_next(source_name, sizeof(source_name)) == VTSS_RC_OK) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/", source_name);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_snmpv3_communities(CYG_HTTPD_STATE *p)
{
    int                       ct;
    snmpv3_communities_conf_t conf, newconf;
    u32                       idx = 0, change_flag, del_flag;
    const char                *var_string;
    size_t                    len = 64 * sizeof(char);
    char                      buf[300], ip_buf[64], ip_mask_buf[16];
    mesa_rc                   rc;
    char                      encoded_string[3 * 64], encoded_string_sec[3 * 64];
    int                       sip_prefix;
#ifdef VTSS_SW_OPTION_IPV6
    int                       ipv6_supported = 1;
#else
    int                       ipv6_supported = 0;
#endif /* VTSS_SW_OPTION_IPV6 */
    BOOL                      ipv6_is_all_zero;
    u8                        i;

    SNMP_HANDLER_CHECK(p);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        strcpy(conf.community, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_communities_conf_get(&conf, TRUE) == VTSS_RC_OK) {
            if (conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            newconf = conf;
            change_flag = del_flag = 0;

            /* delete entry */
            if (conf.sip.address.type == MESA_IP_TYPE_IPV4) {
                misc_ipv4_txt(conf.sip.address.addr.ipv4, ip_buf);
                snprintf(ip_mask_buf, sizeof(ip_mask_buf), "%u", conf.sip.prefix_size);
            } else if (conf.sip.address.type == MESA_IP_TYPE_IPV6) {
                misc_ipv6_txt(&conf.sip.address.addr.ipv6, ip_buf);
                snprintf(ip_mask_buf, sizeof(ip_mask_buf), "%u", conf.sip.prefix_size);
            } else {
                strcpy(ip_buf, "");
                strcpy(ip_mask_buf, "");
            }
            sprintf(buf, "del_%s_%s_%s_%s", conf.community, conf.communitySecret, ip_buf, ip_mask_buf);
            if (cgi_escape(buf, encoded_string) == 0) {
                continue;
            }
            (void)cyg_httpd_form_varable_string(p, encoded_string, &len);
            if (len > 0) {
                newconf.valid = 0;
                change_flag = del_flag = 1;
            } else {
                //secret
                sprintf(buf, "security_%s", conf.community);
                var_string = cyg_httpd_form_varable_string(p, buf, &len);
                if (len > 0) {
                    if (cgi_unescape(var_string, newconf.communitySecret, len, sizeof(newconf.communitySecret)) == FALSE) {
                        continue;
                    }
                    if (strcmp(newconf.communitySecret, conf.communitySecret)) {
                        change_flag = 1;
                    }
                }

                //sip
                sprintf(buf, "sip_%s", conf.community);
                (void)cyg_httpd_form_varable_ip(p, buf, &newconf.sip.address);
                if (newconf.sip.address.type != conf.sip.address.type) {
                    change_flag = 1;
                } else {
                    if (newconf.sip.address.type == MESA_IP_TYPE_IPV4) {
                        if (newconf.sip.address.addr.ipv4 != conf.sip.address.addr.ipv4) {
                            change_flag = 1;
                        }
                    }
                    if (newconf.sip.address.type == MESA_IP_TYPE_IPV6) {
                        if (newconf.sip.address.addr.ipv6 != conf.sip.address.addr.ipv6) {
                            change_flag = 1;
                        }
                    }
                }

                //sip_prefix
                sprintf(buf, "sip_prefix_%s", conf.community);
                if (cyg_httpd_form_varable_int(p, buf, &sip_prefix)) {
                    newconf.sip.prefix_size = (mesa_prefix_size_t)sip_prefix;
                }
                if (newconf.sip.address.type == MESA_IP_TYPE_IPV4) {
                    if (0 == newconf.sip.address.addr.ipv4) {
                        newconf.sip.prefix_size = 0;
                    }
                }
                if (newconf.sip.address.type == MESA_IP_TYPE_IPV6) {
                    ipv6_is_all_zero = TRUE;
                    for (i = 0; i < 16; i++) {
                        if (newconf.sip.address.addr.ipv6.addr[i] > 0) {
                            ipv6_is_all_zero = FALSE;
                            break;
                        }
                    }
                    if (ipv6_is_all_zero) {
                        newconf.sip.prefix_size = 0;
                    }
                }
                if (conf.sip.address.type != MESA_IP_TYPE_NONE &&
                    newconf.sip.prefix_size != conf.sip.prefix_size) {
                    change_flag = 1;
                }
            }

            if (change_flag) {
                T_D("Calling snmpv3_communities_conf_set(%s)", newconf.community);

                if ((rc = AUTH_secret_key_cryptography(TRUE, newconf.communitySecret, newconf.encryptedSecret)) != VTSS_RC_OK) {
                    T_E("AUTH_secret_key_cryptography(%s): failed", newconf.community);
                }

                if ((rc = snmpv3_mgmt_communities_conf_set(&newconf)) < 0) {
                    T_E("snmpv3_mgmt_communities_conf_set(%s): failed", newconf.community);
                }
                if (del_flag) {
                    strcpy(conf.community, SNMPV3_CONF_ACESS_GETFIRST);
                }
            }
        }

        /* new entry */
        idx = 1;
        sprintf(buf, "new_community_%d", idx);
        while ((var_string = cyg_httpd_form_varable_string(p, buf, &len)) && len > 0) {
            memset(&newconf, 0x0, sizeof(newconf));
            //community
            if (cgi_unescape(var_string, newconf.community, len, sizeof(newconf.community)) == FALSE) {
                continue;
            }

            //secret
            sprintf(buf, "new_security_%d", idx);
            var_string = cyg_httpd_form_varable_string(p, buf, &len);
            if (cgi_unescape(var_string, newconf.communitySecret, len, sizeof(newconf.communitySecret)) == FALSE) {
                continue;
            }

            //sip
            sprintf(buf, "new_sip_%d", idx);
            (void)cyg_httpd_form_varable_ip(p, buf, &newconf.sip.address);

            //sip_prefix
            sprintf(buf, "new_sip_prefix_%d", idx);
            if (newconf.sip.address.type != MESA_IP_TYPE_NONE) {
                if (cyg_httpd_form_varable_int(p, buf, &sip_prefix)) {
                    newconf.sip.prefix_size = (mesa_prefix_size_t)sip_prefix;
                }
                if (newconf.sip.address.type == MESA_IP_TYPE_IPV4) {
                    if (0 == newconf.sip.address.addr.ipv4) {
                        newconf.sip.prefix_size = 0;
                    }
                }
                if (newconf.sip.address.type == MESA_IP_TYPE_IPV6) {
                    ipv6_is_all_zero = TRUE;
                    for (i = 0; i < 16; i++) {
                        if (newconf.sip.address.addr.ipv6.addr[i] > 0) {
                            ipv6_is_all_zero = FALSE;
                            break;
                        }
                    }
                    if (ipv6_is_all_zero) {
                        newconf.sip.prefix_size = 0;
                    }
                }
            } else {
                newconf.sip.prefix_size = 0;
            }

            newconf.valid = 1;
            newconf.storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
            newconf.status = SNMP_MGMT_ROW_ACTIVE;
            T_D("Calling snmpv3_communities_users_conf_set(%s)", newconf.community);
            if ((rc = AUTH_secret_key_cryptography(TRUE, newconf.communitySecret, newconf.encryptedSecret)) != VTSS_RC_OK) {
                T_E("AUTH_secret_key_cryptography(%s): failed", newconf.community);
            }
            if ((rc = snmpv3_mgmt_communities_conf_set(&newconf)) < 0) {
                if (rc == SNMPV3_ERROR_COMMUNITIES_TABLE_FULL) {
                    const char *err = "SNMPv3 communities table is full";
                    send_custom_error(p, err, err, strlen(err));
                    return -1; // Do not further search the file system.
                } else if (rc == SNMPV3_ERROR_COMMUNITIES_IP_INVALID) {
                    const char *err = "SNMPv3 has invalid IP address or prefix length";
                    send_custom_error(p, err, err, strlen(err));
                    return -1; // Do not further search the file system.
                } else if (rc == SNMPV3_ERROR_COMMUNITIES_IP_OVERLAP) {
                    const char *err = "SNMPv3 community has overlapping IP";
                    send_custom_error(p, err, err, strlen(err));
                    return -1; // Do not further search the file system.
                } else if (rc != VTSS_RC_OK) {
                    T_E("snmpv3_mgmt_communities_conf_set(%s): failed", newconf.community);
                }
            }
            idx++;
            sprintf(buf, "new_community_%d", idx);
        }

        redirect(p, "/snmpv3_communities.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: <ipv6_supported>/<community_name>/<secret>/<sip>/<sip_prefix>,...
        */
        strcpy(conf.community, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_communities_conf_get(&conf, TRUE) == VTSS_RC_OK) {
            if (conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            if (cgi_escape(conf.community, encoded_string) == 0) {
                continue;
            }
            if (cgi_escape(conf.communitySecret, encoded_string_sec) == 0) {
                continue;
            }
            if (conf.sip.address.type == MESA_IP_TYPE_IPV4) {
                misc_ipv4_txt(conf.sip.address.addr.ipv4, ip_buf);
                snprintf(ip_mask_buf, sizeof(ip_mask_buf), "%u", conf.sip.prefix_size);
            } else if (conf.sip.address.type == MESA_IP_TYPE_IPV6) {
                misc_ipv6_txt(&conf.sip.address.addr.ipv6, ip_buf);
                snprintf(ip_mask_buf, sizeof(ip_mask_buf), "%u", conf.sip.prefix_size);
            } else {
                strcpy(ip_buf, "");
                strcpy(ip_mask_buf, "");
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/", ipv6_supported);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/%s/%s/%s|",
                          encoded_string,
                          encoded_string_sec,
                          ip_buf,
                          ip_mask_buf);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_snmpv3_users(CYG_HTTPD_STATE *p)
{
    int                 ct;
    int                 var_value;
    snmpv3_users_conf_t conf, newconf;
    u32                 idx = 0, idx2 = 0, change_flag, del_flag;
    const char          *var_string;
    size_t              len = 64 * sizeof(char);
    char                buf[128], buf1[4];
    mesa_rc             rc;
    char                encoded_string[3 * 64];
    ulong               pval;
    char                engineid_txt[SNMPV3_MAX_ENGINE_ID_LEN * 2 + 1];

    SNMP_HANDLER_CHECK(p);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        strcpy(conf.user_name, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_users_conf_get(&conf, TRUE) == VTSS_RC_OK) {
            if (conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            newconf = conf;
            change_flag = del_flag = 0;

            /* delete entry */
            char buf[SNMPV3_MAX_ENGINE_ID_LEN * 2 + 1];
            sprintf(buf, "del_%s%s", misc_engineid2str(engineid_txt, conf.engineid, conf.engineid_len), conf.user_name);
            if (cgi_escape(buf, encoded_string) == 0) {
                continue;
            }
            (void)cyg_httpd_form_varable_string(p, encoded_string, &len);
            if (len > 0) {
                newconf.valid = 0;
                change_flag = del_flag = 1;
            } else {
                //auth_password
                sprintf(buf, "auth_pd_%s%s", misc_engineid2str(engineid_txt, conf.engineid, conf.engineid_len), conf.user_name);
                if (cgi_escape(buf, encoded_string) == 0) {
                    continue;
                }
                var_string = cyg_httpd_form_varable_string(p, encoded_string, &len);
                if (len > 0) {
                    if (cgi_unescape(var_string, newconf.auth_password, len, sizeof(newconf.auth_password)) == FALSE) {
                        continue;
                    }
                    if (strcmp(conf.auth_password, newconf.auth_password)) {
                        change_flag = 1;
                        newconf.auth_password_encrypted = FALSE;
                    }
                }
                T_N("AuthPassword:%s %s, enc:%d", conf.auth_password, newconf.auth_password, conf.auth_password_encrypted);

                //priv_password
                sprintf(buf, "priv_pd_%s%s", misc_engineid2str(engineid_txt, conf.engineid, conf.engineid_len), conf.user_name);
                if (cgi_escape(buf, encoded_string) == 0) {
                    continue;
                }
                var_string = cyg_httpd_form_varable_string(p, encoded_string, &len);
                if (len > 0) {
                    if (cgi_unescape(var_string, newconf.priv_password, len, sizeof(newconf.priv_password)) == FALSE) {
                        continue;
                    }
                    if (strcmp(conf.priv_password, newconf.priv_password)) {
                        change_flag = 1;
                        newconf.priv_password_encrypted = FALSE;
                    }
                }
                T_N("PrivPassword:%s %s, enc:%d", conf.priv_password, newconf.priv_password, conf.priv_password_encrypted);
            }

            if (change_flag) {
                T_D("Calling snmpv3_mgmt_users_conf_set(%s)", newconf.user_name);
                if ((rc = snmpv3_mgmt_users_conf_set(&newconf)) < 0) {
                    T_E("snmpv3_mgmt_users_conf_set(%s): failed", newconf.user_name);
                }
                if (del_flag) {
                    strcpy(conf.user_name, SNMPV3_CONF_ACESS_GETFIRST);
                }
            }
        }

        /* new entry */
        idx = 1;
        sprintf(buf, "new_engineid_%d", idx);
        while ((var_string = cyg_httpd_form_varable_string(p, buf, &len)) && len > 0) {
            memset(&newconf, 0x0, sizeof(newconf));
            //engine_id
            for (idx2 = 0; idx2 < len; idx2 = idx2 + 2) {
                memcpy(buf1, var_string + idx2, 2);
                buf1[2] = '\0';
                if (cyg_httpd_str_to_hex((const char *) buf1, &pval) == FALSE) {
                    continue;
                }
                newconf.engineid[idx2 / 2] = (uchar)pval;
            }
            newconf.engineid_len = len / 2;

            //user_name
            sprintf(buf, "new_user_%d", idx);
            var_string = cyg_httpd_form_varable_string(p, buf, &len);
            if (len > 0) {
                if (cgi_unescape(var_string, newconf.user_name, len, sizeof(newconf.user_name)) == FALSE) {
                    continue;
                }
            }

            //security_level
            sprintf(buf, "new_level_%d", idx);
            if (cyg_httpd_form_varable_int(p, buf, &var_value)) {
                newconf.security_level = var_value;
            }

            if (newconf.security_level != SNMP_MGMT_SEC_LEVEL_NOAUTH) {
                //auth_proto
                sprintf(buf, "new_auth_%d", idx);
                if (cyg_httpd_form_varable_int(p, buf, &var_value)) {
                    newconf.auth_protocol = var_value;
                }

                //auth_password
                sprintf(buf, "new_auth_pd_%d", idx);
                var_string = cyg_httpd_form_varable_string(p, buf, &len);
                if (len > 0) {
                    if (cgi_unescape(var_string, newconf.auth_password, len, sizeof(newconf.auth_password)) == FALSE) {
                        continue;
                    }
                }
            }

            if (newconf.security_level == SNMP_MGMT_SEC_LEVEL_AUTHPRIV) {
                //priv_proto
                sprintf(buf, "new_priv_%d", idx);
                if (cyg_httpd_form_varable_int(p, buf, &var_value)) {
                    newconf.priv_protocol = var_value;
                }

                //priv_password
                sprintf(buf, "new_priv_pd_%d", idx);
                var_string = cyg_httpd_form_varable_string(p, buf, &len);
                if (len > 0) {
                    if (cgi_unescape(var_string, newconf.priv_password, len, sizeof(newconf.priv_password)) == FALSE) {
                        continue;
                    }
                }
            }

            newconf.valid = 1;
            newconf.storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
            newconf.status = SNMP_MGMT_ROW_ACTIVE;
            T_D("Calling snmpv3_mgmt_users_conf_set(%s)", newconf.user_name);
            if ((rc = snmpv3_mgmt_users_conf_set(&newconf)) < 0) {
                if (rc != VTSS_RC_OK) {
                    T_E("snmpv3_mgmt_users_conf_set(%s): failed", newconf.user_name);
                }
            }
            idx++;
            sprintf(buf, "new_engineid_%d", idx);
        }

        redirect(p, "/snmpv3_users.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: <engineid>/<user_name>/<group_name>/<level>/<auth_proto>/<auth_pd>/<privacy_proto>/<privacy_pd>,...
        */
        strcpy(conf.user_name, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_users_conf_get(&conf, TRUE) == VTSS_RC_OK) {
            if (conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            if (cgi_escape(conf.user_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/%s/%d/",
                          misc_engineid2str(engineid_txt, conf.engineid, conf.engineid_len),
                          encoded_string,
                          conf.security_level);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            // auth_password, auth_protocol
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d//",
                          conf.auth_protocol);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            // priv_password, priv_protocol
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/|",
                          conf.priv_protocol);

            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_snmpv3_groups(CYG_HTTPD_STATE *p)
{
    int                       ct;
    int                       var_value;
    snmp_conf_t               snmp_conf;
    snmpv3_communities_conf_t community_conf;
    snmpv3_users_conf_t       user_conf;
    snmpv3_groups_conf_t      conf, newconf;
    u32                       idx = 0, change_flag, del_flag;
    const char                *var_string;
    size_t                    len = 64 * sizeof(char);
    char                      buf[64];
    mesa_rc                   rc;
    char                      encoded_string[3 * 64];

    SNMP_HANDLER_CHECK(p);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        strcpy(conf.security_name, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_groups_conf_get(&conf, TRUE) == VTSS_RC_OK) {
            if (conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            newconf = conf;
            change_flag = del_flag = 0;

            /* delete entry */
            sprintf(buf, "del_%d%s", (u32)conf.security_model, conf.security_name);
            if (cgi_escape(buf, encoded_string) == 0) {
                continue;
            }
            (void)cyg_httpd_form_varable_string(p, encoded_string, &len);
            if (len > 0) {
                newconf.valid = 0;
                change_flag = del_flag = 1;
            } else {
                //group_name
                sprintf(buf, "group_%d%s", (u32)conf.security_model, conf.security_name);
                var_string = cyg_httpd_form_varable_string(p, buf, &len);
                if (len > 0) {
                    if (cgi_unescape(var_string, newconf.group_name, len, sizeof(newconf.group_name)) == FALSE) {
                        continue;
                    }
                    if (strcmp(newconf.group_name, conf.group_name)) {
                        change_flag = 1;
                    }
                }
            }

            if (change_flag) {
                T_D("Calling snmpv3_mgmt_groups_conf_set(%d, %s)", (u32)newconf.security_model, newconf.security_name);
                if ((rc = snmpv3_mgmt_groups_conf_set(&newconf)) < 0) {
                    T_E("snmpv3_mgmt_groups_conf_set(%d, %s): failed", (u32)newconf.security_model, newconf.security_name);
                }
                if (del_flag) {
                    strcpy(conf.security_name, SNMPV3_CONF_ACESS_GETFIRST);
                }
            }
        }

        /* new entry */
        idx = 1;
        sprintf(buf, "new_group_%d", idx);
        while ((var_string = cyg_httpd_form_varable_string(p, buf, &len)) && len > 0) {
            memset(&newconf, 0x0, sizeof(newconf));
            //group_name
            if (cgi_unescape(var_string, newconf.group_name, len, sizeof(newconf.group_name)) == FALSE) {
                continue;
            }

            //security_model
            sprintf(buf, "new_model_%d", idx);
            if (cyg_httpd_form_varable_int(p, buf, &var_value)) {
                newconf.security_model = var_value;
            }

            //security_name
            sprintf(buf, "new_security_%d", idx);
            var_string = cyg_httpd_form_varable_string(p, buf, &len);
            if (len > 0) {
                if (cgi_unescape(var_string, newconf.security_name, len, sizeof(newconf.security_name)) == FALSE) {
                    continue;
                }
            }

            newconf.valid = 1;
            newconf.storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
            newconf.status = SNMP_MGMT_ROW_ACTIVE;
            T_D("Calling snmpv3_mgmt_groups_conf_set(%d, %s)", (u32)newconf.security_model, newconf.security_name);
            if ((rc = snmpv3_mgmt_groups_conf_set(&newconf)) < 0) {
                if (rc != VTSS_RC_OK) {
                    T_E("snmpv3_mgmt_groups_conf_set(%d, %s): failed", (u32)newconf.security_model, newconf.security_name);
                }
            }
            idx++;
            sprintf(buf, "new_group_%d", idx);
        }

        redirect(p, "/snmpv3_groups.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           <community1>|<community2>|...,<user1>|<user2>|...,<model>/<security_name>/<group_name>,...
        */
        if (snmp_mgmt_snmp_conf_get(&snmp_conf) != VTSS_RC_OK) {
            (void)cyg_httpd_end_chunked();
            return -1;
        }

        strcpy(community_conf.community, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_communities_conf_get(&community_conf, TRUE) == VTSS_RC_OK) {
            if (community_conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            if (cgi_escape(community_conf.community, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        (void)cyg_httpd_write_chunked(",", 1);
        strcpy(user_conf.user_name, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_users_conf_get(&user_conf, TRUE) == VTSS_RC_OK) {
            if (user_conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            if (memcmp(snmp_conf.engineid, user_conf.engineid, snmp_conf.engineid_len > user_conf.engineid_len ? snmp_conf.engineid_len : user_conf.engineid_len)) {
                continue;
            }
            if (cgi_escape(user_conf.user_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        (void)cyg_httpd_write_chunked(",", 1);
        strcpy(conf.security_name, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_groups_conf_get(&conf, TRUE) == VTSS_RC_OK) {
            if (conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            if (cgi_escape(conf.security_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%s/",
                          (u32)conf.security_model,
                          encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            if (cgi_escape(conf.group_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|",
                          encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_snmpv3_views(CYG_HTTPD_STATE *p)
{
    int                 ct;
    int                 var_value;
    snmpv3_views_conf_t conf, newconf;
    u32                 idx = 0, change_flag, del_flag;
    const char          *var_string;
    size_t              len = 64 * sizeof(char);
    char                buf[128];
    mesa_rc             rc;
    char                encoded_string[3 * 64];

    SNMP_HANDLER_CHECK(p);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        strcpy(conf.view_name, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_views_conf_get(&conf, TRUE) == VTSS_RC_OK) {
            if (conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            newconf = conf;
            change_flag = del_flag = 0;

            /* delete entry */
            sprintf(buf, "del_%s%s", conf.view_name, conf.subtree);
            if (cgi_escape(buf, encoded_string) == 0) {
                continue;
            }
            (void)cyg_httpd_form_varable_string(p, encoded_string, &len);
            if (len > 0) {
                newconf.valid = 0;
                change_flag = del_flag = 1;
            } else {
                //view_type
                sprintf(buf, "type_%s%s", conf.view_name, conf.subtree);
                if (cyg_httpd_form_varable_int(p, buf, &var_value)) {
                    newconf.view_type = var_value;
                    if (newconf.view_type != conf.view_type) {
                        change_flag = 1;
                    }
                }
            }

            if (change_flag) {
                T_D("Calling snmpv3_mgmt_views_conf_set(%s)", newconf.view_name);
                if ((rc = snmpv3_mgmt_views_conf_set(&newconf)) < 0) {
                    T_E("snmpv3_mgmt_views_conf_set(%s): failed", newconf.view_name);
                }
                if (del_flag) {
                    strcpy(conf.view_name, SNMPV3_CONF_ACESS_GETFIRST);
                }
            }
        }

        /* new entry */
        idx = 1;
        sprintf(buf, "new_view_%d", idx);
        while ((var_string = cyg_httpd_form_varable_string(p, buf, &len)) && len > 0) {
            memset(&newconf, 0x0, sizeof(newconf));
            //view_name
            if (cgi_unescape(var_string, newconf.view_name, len, sizeof(newconf.view_name)) == FALSE) {
                continue;
            }

            //view_type
            sprintf(buf, "new_type_%d", idx);
            if (cyg_httpd_form_varable_int(p, buf, &var_value)) {
                newconf.view_type = var_value;
            }

            //subtree
            sprintf(buf, "new_subtree_%d", idx);
            if (NULL == (var_string = cyg_httpd_form_varable_string(p, buf, &len))) {
                T_D("get %s fail", buf);
                continue;
            }

            strncpy(newconf.subtree, var_string, len);
            newconf.valid = 1;
            newconf.storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
            newconf.status = SNMP_MGMT_ROW_ACTIVE;
            T_D("Calling snmpv3_mgmt_views_conf_set(%s)", newconf.view_name);
            if ((rc = snmpv3_mgmt_views_conf_set(&newconf)) < 0) {
                if (rc == SNMP_ERROR_ENGINE_FAIL) {
                    char err[512];
                    sprintf(err, "OID %s is not supported",
                            newconf.subtree);
                    send_custom_error(p, err, err, strlen(err));
                    return -1; // Do not further search the file system.
                } else if (rc != VTSS_RC_OK) {
                    T_E("snmpv3_mgmt_views_conf_set(%s): failed", newconf.view_name);
                }
            }
            idx++;
            sprintf(buf, "new_view_%d", idx);
        }

        redirect(p, "/snmpv3_views.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: <view_name>/<view_type>/<subtree>,...
        */
        strcpy(conf.view_name, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_views_conf_get(&conf, TRUE) == VTSS_RC_OK) {
            if (conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            if (cgi_escape(conf.view_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/%d/%s|",
                          encoded_string,
                          conf.view_type,
                          conf.subtree);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}


static i32 handler_config_snmpv3_accesses(CYG_HTTPD_STATE *p)
{
    int                    ct;
    int                    var_value;
    snmpv3_groups_conf_t   group_conf;
    snmpv3_views_conf_t    view_conf;
    snmpv3_accesses_conf_t conf, newconf;
    u32                    idx = 0, change_flag, del_flag;
    const char             *var_string;
    size_t                 len = 64 * sizeof(char);
    char                   buf[64];
    mesa_rc                rc;
    char                   encoded_string[3 * 64];

    SNMP_HANDLER_CHECK(p);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* store form data */
        strcpy(conf.group_name, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_accesses_conf_get(&conf, TRUE) == VTSS_RC_OK) {
            if (conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            newconf = conf;
            change_flag = del_flag = 0;

            /* delete entry */
            sprintf(buf, "del_%s%d%d", conf.group_name, (u32)newconf.security_model, newconf.security_level);
            if (cgi_escape(buf, encoded_string) == 0) {
                continue;
            }
            (void)cyg_httpd_form_varable_string(p, encoded_string, &len);
            if (len > 0) {
                newconf.valid = 0;
                change_flag = del_flag = 1;
            } else {
                //read_view_name
                sprintf(buf, "read_%s%d%d", conf.group_name, (u32)newconf.security_model, newconf.security_level);
                var_string = cyg_httpd_form_varable_string(p, buf, &len);
                if (len > 0) {
                    if (cgi_unescape(var_string, newconf.read_view_name, len, sizeof(newconf.read_view_name)) == FALSE) {
                        continue;
                    }
                    if (strcmp(newconf.read_view_name, conf.read_view_name)) {
                        change_flag = 1;
                    }
                }

                //write_view_name
                sprintf(buf, "write_%s%d%d", conf.group_name, (u32)newconf.security_model, newconf.security_level);
                var_string = cyg_httpd_form_varable_string(p, buf, &len);
                if (len > 0) {
                    if (cgi_unescape(var_string, newconf.write_view_name, len, sizeof(newconf.write_view_name)) == FALSE) {
                        continue;
                    }
                    if (strcmp(newconf.write_view_name, conf.write_view_name)) {
                        change_flag = 1;
                    }
                }
            }

            if (change_flag) {
                T_D("Calling snmpv3_mgmt_accesses_conf_set(%s, %d, %d)", newconf.group_name, (u32)newconf.security_model, newconf.security_level);
                if ((rc = snmpv3_mgmt_accesses_conf_set(&newconf)) < 0) {
                    T_E("snmpv3_mgmt_accesses_conf_set(%s, %d, %d): failed", newconf.group_name, (u32)newconf.security_model, newconf.security_level);
                }
                if (del_flag) {
                    strcpy(conf.group_name, SNMPV3_CONF_ACESS_GETFIRST);
                }
            }
        }

        /* new entry */
        idx = 1;
        sprintf(buf, "new_group_%d", idx);
        while ((var_string = cyg_httpd_form_varable_string(p, buf, &len)) && len > 0) {
            memset(&newconf, 0x0, sizeof(newconf));
            //user_name
            if (cgi_unescape(var_string, newconf.group_name, len, sizeof(newconf.group_name)) == FALSE) {
                continue;
            }

            //security_model
            sprintf(buf, "new_model_%d", idx);
            if (cyg_httpd_form_varable_int(p, buf, &var_value)) {
                newconf.security_model = var_value;
            }

            //security_level
            sprintf(buf, "new_level_%d", idx);
            if (cyg_httpd_form_varable_int(p, buf, &var_value)) {
                newconf.security_level = var_value;
            }

            //context_match
            newconf.context_match = SNMPV3_MGMT_CONTEX_MATCH_EXACT;

            //read_view_name
            sprintf(buf, "new_read_%d", idx);
            var_string = cyg_httpd_form_varable_string(p, buf, &len);
            if (len > 0) {
                if (cgi_unescape(var_string, newconf.read_view_name, len, sizeof(newconf.read_view_name)) == FALSE) {
                    continue;
                }
            }

            //write_view_name
            sprintf(buf, "new_write_%d", idx);
            var_string = cyg_httpd_form_varable_string(p, buf, &len);
            if (len > 0) {
                if (cgi_unescape(var_string, newconf.write_view_name, len, sizeof(newconf.write_view_name)) == FALSE) {
                    continue;
                }
            }

            //notify_view_name
            /* SNMP/eCos package don't support notify yet
            sprintf(buf, "new_notify_%d", idx);
            var_string = cyg_httpd_form_varable_string(p, buf, &len);
            if (len > 0) {
                if (cgi_unescape(var_string, newconf.notify_view_name, len, sizeof(newconf.notify_view_name)) == FALSE) {
                    continue;
                    }
            } */
            strcpy(newconf.notify_view_name, SNMPV3_NONAME);

            newconf.valid = 1;
            newconf.storage_type = SNMP_MGMT_STORAGE_NONVOLATILE;
            newconf.status = SNMP_MGMT_ROW_ACTIVE;
            T_D("Calling snmpv3_mgmt_accesses_conf_set(%s, %d, %d)", newconf.group_name, (u32)newconf.security_model, newconf.security_level);
            if ((rc = snmpv3_mgmt_accesses_conf_set(&newconf)) < 0) {
                if (rc != VTSS_RC_OK) {
                    T_E("snmpv3_mgmt_accesses_conf_set(%s, %d, %d): failed", newconf.group_name, (u32)newconf.security_model, newconf.security_level);
                }
            }
            idx++;
            sprintf(buf, "new_group_%d", idx);
        }

        redirect(p, "/snmpv3_accesses.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: <grouop_name1>|<grouop_name2>|...,<view_name1>|<view_name2>|...,<group_name>/<modle>/<level>/<read_view_name>/<write_view_name>/<notify_view_name>,...
        */
        strcpy(group_conf.security_name, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_groups_conf_get(&group_conf, TRUE) == VTSS_RC_OK) {
            if (group_conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            if (cgi_escape(group_conf.group_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",None|");
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        strcpy(view_conf.view_name, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_views_conf_get(&view_conf, TRUE) == VTSS_RC_OK) {
            if (view_conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            if (cgi_escape(view_conf.view_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        (void)cyg_httpd_write_chunked(",", 1);
        strcpy(conf.group_name, SNMPV3_CONF_ACESS_GETFIRST);
        while (snmpv3_mgmt_accesses_conf_get(&conf, TRUE) == VTSS_RC_OK) {
            if (conf.status != SNMP_MGMT_ROW_ACTIVE) {
                continue;
            }
            if (cgi_escape(conf.group_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/%d/%d/",
                          encoded_string,
                          (u32)conf.security_model,
                          conf.security_level);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            if (cgi_escape(conf.read_view_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/",
                          encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            if (cgi_escape(conf.write_view_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s/",
                          encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            if (cgi_escape(conf.notify_view_name, encoded_string) == 0) {
                continue;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|",
                          encoded_string);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

#define SNMP_WEB_BUF_LEN 512
static size_t snmp_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[SNMP_WEB_BUF_LEN];
    (void) snprintf(buff, SNMP_WEB_BUF_LEN,
                    "var trap_host_len = %d;\n"
                    "var snmp_community_len = %d;\n"
                    "var configSnmpv3CommunitiesMax = %d;\n"
                    "var configSnmpv3UsersMax = %d;\n"
                    "var configSnmpv3GroupsMax = %d;\n"
                    "var configSnmpv3ViewsMax = %d;\n"
                    "var configSnmpv3AccessesMax = %d;\n"
                    "var configTrapSourcesMax = %d;\n"
                    "var configTrapSourceFilterIdMax = %d;\n",
                    TRAP_MAX_NAME_LEN,
                    SNMP_MGMT_INPUT_COMMUNITY_LEN,
                    SNMPV3_MAX_COMMUNITIES,
#if defined(SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT)
                    0,
                    0,
                    0,
                    0,
#else
                    SNMPV3_MAX_USERS,
                    SNMPV3_MAX_GROUPS,
                    SNMPV3_MAX_VIEWS,
                    SNMPV3_MAX_ACCESSES,
#endif /* SNMPV3_TABLES_UNLIMITED_ENTRIES_CNT */
                    VTSS_TRAP_SOURCE_MAX,
                    VTSS_TRAP_FILTER_MAX - 1
                   );
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(snmp_lib_config_js);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_snmp, "/config/snmp", handler_config_snmp);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_trap, "/config/trap", handler_config_trap);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_trap_detailed, "/config/trap_detailed", handler_config_trap_detailed);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_trap_dyna_source_name, "/stat/trap_dyna_source_name", handler_config_stat_trap_dyna_source_name);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_snmpv3_communities, "/config/snmpv3_communities", handler_config_snmpv3_communities);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_snmpv3_users, "/config/snmpv3_users", handler_config_snmpv3_users);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_snmpv3_groups, "/config/snmpv3_groups", handler_config_snmpv3_groups);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_snmpv3_views, "/config/snmpv3_views", handler_config_snmpv3_views);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_snmpv3_accesses, "/config/snmpv3_accesses", handler_config_snmpv3_accesses);

