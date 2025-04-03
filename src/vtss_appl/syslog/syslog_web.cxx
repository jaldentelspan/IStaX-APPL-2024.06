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
#include "syslog_api.h"
#include "sysutil_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif
#include "misc_api.h"   // For misc_str_is_ipv4(), misc_ipv4_txt(), misc_ipv6_txt()
#include "mgmt_api.h"   // For mgmt_txt2ipv4(), mgmt_txt2ipv6()


#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SYSLOG

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

static i32 handler_stat_syslog(CYG_HTTPD_STATE *p)
{
    vtss_isid_t         sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                 ct;
    syslog_ram_stat_t   ram_stat;
    syslog_ram_entry_t  *entry;
    ulong               entry_count = 0;
    vtss_appl_syslog_lvl_t        level = VTSS_APPL_SYSLOG_LVL_ALL, clear_level = VTSS_APPL_SYSLOG_LVL_ALL;
    int                 idx, flag = 2, start_id = 0, entry_num = 20;
    const char          *var_string;
    char                *encoded_string = NULL;
#define SYSLOG_MAX_DISPLAY_LEN  96

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_GET) {
        /* syslogFlag 0 : <Clear>
           syslogFlag 1 : <GetFirst>
           syslogFlag 2 : <GetPrevious>
           syslogFlag 3 : <GetNext>
           syslogFlag 4 : <GetLast>
        */
        if ((var_string = cyg_httpd_form_varable_find(p, "syslogFlag")) != NULL) {
            flag = atoi(var_string);
            if ((var_string = cyg_httpd_form_varable_find(p, "syslogLevel")) != NULL) {
                level = (vtss_appl_syslog_lvl_t) atoi(var_string);
            }
            if ((var_string = cyg_httpd_form_varable_find(p, "syslogStartId")) != NULL) {
                start_id = atoi(var_string);
            }
            if ((var_string = cyg_httpd_form_varable_find(p, "syslogEntryNum")) != NULL) {
                entry_num = atoi(var_string);
            }
            switch (flag) {
            case 0:
                if ((var_string = cyg_httpd_form_varable_find(p, "syslogClear")) != NULL) {
                    clear_level = (vtss_appl_syslog_lvl_t) atoi(var_string);
                }
                syslog_ram_clear(sid, clear_level);
                break;
            case 1:
            case 2:
            case 3:
            case 4:
                break;
            default:
                return -1;
            }
        }

        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: [id]/[level]/[time]/[message]|[id]/[level]/[time]/[message]|...
        */
        if ((VTSS_MALLOC_CAST(entry, sizeof(*entry))) == NULL) {
            (void)cyg_httpd_end_chunked();
            return -1;
        }

        memset(&ram_stat, 0x0, sizeof(ram_stat));
        (void) syslog_ram_stat_get(sid, &ram_stat);
        if (level == VTSS_APPL_SYSLOG_LVL_ALL) {
            for (entry_count = 0, idx = VTSS_APPL_SYSLOG_LVL_START; idx <= VTSS_APPL_SYSLOG_LVL_END; idx++) {
                entry_count += ram_stat.count[idx];
            }
        } else {
            entry_count = ram_stat.count[level];
        }
        if (entry_count) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), VPRIlu"#", entry_count);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        if (flag == 2) {
            for (entry->id = 0, idx = 0;
                 syslog_ram_get(sid, TRUE, entry->id, level, VTSS_MODULE_ID_NONE, entry);
                ) {
                idx++;
                if ((int)entry->id >= start_id) {
                    entry_count = idx;
                    break;
                }
            }
        }

        if (entry_count && (flag == 2 || flag == 4)) {
            for (entry->id = 0, idx = 0;
                 syslog_ram_get(sid, TRUE, entry->id, level, VTSS_MODULE_ID_NONE, entry);
                ) {
                if ((int)entry_count <= entry_num || idx == (entry_count - entry_num)) {
                    start_id = entry->id;
                    break;
                }
                idx++;
            }
        }

        VTSS_MALLOC_CAST(encoded_string, 3 * SYSLOG_RAM_MSG_MAX);
        entry_count = 0;
        for (entry->id = (start_id - 1) > 0 ? start_id - 1 : 0;
             syslog_ram_get(sid, TRUE, entry->id, level, VTSS_MODULE_ID_NONE, entry);
            ) {
            entry_count++;
            if (entry_count > (ulong) entry_num) {
                break;
            }

            /* The summary message length is limited to SYSLOG_MAX_DISPLAY_LEN characters or one line */
            for (idx = 0; idx < SYSLOG_MAX_DISPLAY_LEN; idx++) {
                if (entry->msg[idx] == '\n') {
                    entry->msg[idx] = '\0';
                    break;
                }
            }
            if (idx == SYSLOG_MAX_DISPLAY_LEN) {
                strcpy(&entry->msg[idx - 4], " ...");
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), VPRIlu"/%s/%s/",
                          entry->id,
                          syslog_lvl_to_string(entry->lvl, FALSE),
                          misc_time2str(entry->time));
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            if (encoded_string) {
                ct = cgi_escape(entry->msg, encoded_string);
                (void)cyg_httpd_write_chunked(encoded_string, ct);
            }
            (void)cyg_httpd_write_chunked("|", 1);
        }

        if (!entry_count) {
            (void)cyg_httpd_write_chunked("null", 4);
        }

        (void)cyg_httpd_end_chunked();

        if (encoded_string) {
            VTSS_FREE(encoded_string);
        }
        VTSS_FREE(entry);
    }

    return -1; // Do not further search the file system.
}

static i32 handler_stat_syslog_detailed(CYG_HTTPD_STATE *p)
{
    vtss_isid_t         sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                 ct;
    syslog_ram_entry_t  *entry = NULL;
    ulong               idx, entry_count = 0;
    int                 flag = 0, start_id = 0, previous_id = 0;
    BOOL                next = TRUE;
    const char          *var_string;
    char                *encoded_string = NULL;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SYSTEM)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_GET) {
        /* syslogFlag 0 : <get>
           syslogFlag 1 : <GetFirst>
           syslogFlag 2 : <GetPrevious>
           syslogFlag 3 : <GetNext>
           syslogFlag 4 : <GetLast>
        */
        if ((var_string = cyg_httpd_form_varable_find(p, "syslogFlag")) != NULL) {
            flag = atoi(var_string);
            if ((var_string = cyg_httpd_form_varable_find(p, "syslogStartId")) != NULL) {
                start_id = atoi(var_string);
            }
            switch (flag) {
            case 0:
            case 1:
                if (start_id) {
                    next = FALSE;
                }
                break;
            case 2:
                if (start_id == 1) {
                    next = FALSE;
                    flag = 1;
                }
                break;
            case 3:
            case 4:
                break;
            default:
                return -1;
            }
        }

        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: [id]#[level]#[time]#[message]
        */
        if ((VTSS_MALLOC_CAST(entry, sizeof(*entry))) == NULL) {
            (void)cyg_httpd_end_chunked();
            return -1;
        }

        VTSS_MALLOC_CAST(encoded_string, 3 * SYSLOG_RAM_MSG_MAX);
        switch (flag) {
        case 0:
        case 1:
        case 3:
            entry->id = start_id;
            if (syslog_ram_get(sid, next, entry->id, VTSS_APPL_SYSLOG_LVL_ALL, VTSS_MODULE_ID_NONE, entry)) {
                for (idx = 0 ; idx < strlen(entry->msg); idx++) {
                    if (entry->msg[idx] == 0xA) { /* LF */
                        entry->msg[idx] = '|';
                    }
                }
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), VPRIlu"#%s#%s#",
                              entry->id,
                              syslog_lvl_to_string(entry->lvl, FALSE),
                              misc_time2str(entry->time));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                if (encoded_string) {
                    ct = cgi_escape(entry->msg, encoded_string);
                    (void)cyg_httpd_write_chunked(encoded_string, ct);
                }
            } else {
                (void)cyg_httpd_write_chunked("null", 4);
            }
            break;
        case 2:
            for (entry->id = 0;
                 syslog_ram_get(sid, TRUE, entry->id, VTSS_APPL_SYSLOG_LVL_ALL, VTSS_MODULE_ID_NONE, entry);
                ) {
                entry_count++;
                if (entry->id < (ulong) start_id) {
                    previous_id = entry->id;
                } else {
                    break;
                }
            }
            (void) syslog_ram_get(sid, FALSE, previous_id, VTSS_APPL_SYSLOG_LVL_ALL, VTSS_MODULE_ID_NONE, entry);
            if (entry_count) {
                for (idx = 0 ; idx < strlen(entry->msg); idx++) {
                    if (entry->msg[idx] == 0xA) { /* LF */
                        entry->msg[idx] = '|';
                    }
                }
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), VPRIlu"#%s#%s#",
                              entry->id,
                              syslog_lvl_to_string(entry->lvl, FALSE),
                              misc_time2str(entry->time));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                if (encoded_string) {
                    ct = cgi_escape(entry->msg, encoded_string);
                    (void)cyg_httpd_write_chunked(encoded_string, ct);
                }
            } else {
                (void)cyg_httpd_write_chunked("null", 4);
            }
            break;
        case 4:
            for (entry->id = 0;
                 syslog_ram_get(sid, TRUE, entry->id, VTSS_APPL_SYSLOG_LVL_ALL, VTSS_MODULE_ID_NONE, entry);
                ) {
                entry_count++;
                previous_id = entry->id;
            }
            (void) syslog_ram_get(sid, FALSE, previous_id, VTSS_APPL_SYSLOG_LVL_ALL, VTSS_MODULE_ID_NONE, entry);
            if (entry_count) {
                for (idx = 0 ; idx < strlen(entry->msg); idx++) {
                    if (entry->msg[idx] == 0xA) { /* LF */
                        entry->msg[idx] = '|';
                    }
                }
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), VPRIlu"#%s#%s#",
                              entry->id,
                              syslog_lvl_to_string(entry->lvl, FALSE),
                              misc_time2str(entry->time));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                if (encoded_string) {
                    ct = cgi_escape(entry->msg, encoded_string);
                    (void)cyg_httpd_write_chunked(encoded_string, ct);
                }
            } else {
                (void)cyg_httpd_write_chunked("null", 4);
            }
            break;
        }

        (void)cyg_httpd_end_chunked();

        if (encoded_string) {
            VTSS_FREE(encoded_string);
        }
        VTSS_FREE(entry);
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_syslog(CYG_HTTPD_STATE *p)
{
    int                             ct;
    vtss_appl_syslog_server_conf_t  conf, newconf;
    const char                      *var_string;
    size_t                          len;
    int                             var_value;
    vtss_domain_name_t              tmp;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYSTEM)) {
        return -1;
    }
#endif

    (void) vtss_appl_syslog_server_conf_get(&conf);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        newconf = conf;

        // server_mode
        if (cyg_httpd_form_varable_int(p, "server_mode", &var_value)) {
            newconf.server_mode = var_value;
        }

        //sys_location
        var_string = cyg_httpd_form_varable_string(p, "server_addr", &len);
        tmp.name[0] = '\0';
        if (len > 0) {
            memcpy(tmp.name, var_string, sizeof(tmp.name));
            tmp.name[len] = '\0';
        }
        if (strlen(tmp.name) == 0) {
            memset(&newconf.syslog_server, 0, sizeof(newconf.syslog_server));
            newconf.syslog_server.type = VTSS_INET_ADDRESS_TYPE_NONE;
        } else if (misc_str_is_ipv4(tmp.name) == VTSS_RC_OK) {
            newconf.syslog_server.type = VTSS_INET_ADDRESS_TYPE_IPV4;
            (void)mgmt_txt2ipv4(tmp.name, &newconf.syslog_server.address.ipv4, NULL, FALSE);
#if defined(VTSS_SW_OPTION_IPV6)
        } else if (misc_str_is_ipv6(tmp.name) == VTSS_RC_OK) {
            newconf.syslog_server.type = VTSS_INET_ADDRESS_TYPE_IPV6;
            (void)mgmt_txt2ipv6(tmp.name, &newconf.syslog_server.address.ipv6);
#endif /* VTSS_SW_OPTION_IPV6 */
#if defined(VTSS_SW_OPTION_DNS)
        } else {
            newconf.syslog_server.type = VTSS_INET_ADDRESS_TYPE_DNS;
            strcpy(newconf.syslog_server.address.domain_name.name, tmp.name);
#endif /* VTSS_SW_OPTION_DNS */
        }

        // syslog_level
        if (cyg_httpd_form_varable_int(p, "syslog_level", &var_value)) {
            newconf.syslog_level = (vtss_appl_syslog_lvl_t) var_value;
        }

        if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
            mesa_rc rc;
            if ((rc = vtss_appl_syslog_server_conf_set(&newconf)) < 0) {
                T_D("vtss_appl_syslog_server_conf_set: failed rc = %d", rc);
            }
        }
        redirect(p, "/syslog_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: [server_mode]/[server_addr]/[syslog_level]
        */
        tmp.name[0] = '\0';
        if (conf.syslog_server.type == VTSS_INET_ADDRESS_TYPE_IPV4) {
            (void)misc_ipv4_txt(conf.syslog_server.address.ipv4, tmp.name);
#if defined(VTSS_SW_OPTION_IPV6)
        } else if (conf.syslog_server.type == VTSS_INET_ADDRESS_TYPE_IPV6) {
            (void)misc_ipv6_txt(&conf.syslog_server.address.ipv6, tmp.name);
#endif /* VTSS_SW_OPTION_IPV6 */
#if defined(VTSS_SW_OPTION_DNS)
        } else if (conf.syslog_server.type == VTSS_INET_ADDRESS_TYPE_DNS) {
            strcpy(tmp.name, conf.syslog_server.address.domain_name.name);
#endif /* VTSS_SW_OPTION_DNS */
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%s/%d", conf.server_mode, tmp.name, conf.syslog_level);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

static i32 handler_config_sysinfo(CYG_HTTPD_STATE *p)
{
    int           ct;
    system_conf_t conf, newconf;
    const char    *var_string;
    size_t        len;
    system_conf_t sysconf;
#ifndef VTSS_SW_OPTION_DAYLIGHT_SAVING
    int           tz_off;
#endif

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SYSTEM)) {
        return -1;
    }
#endif

    (void) system_get_config(&sysconf);

    if (p->method == CYG_HTTPD_METHOD_POST) {
        (void) system_get_config(&conf);
        newconf = conf;

        //sys_contact
        var_string = cyg_httpd_form_varable_string(p, "sys_contact", &len);
        if (len > 0) {
            (void) cgi_unescape(var_string, newconf.sys_contact, len, sizeof(newconf.sys_contact));
        } else {
            strcpy(newconf.sys_contact, "");
        }

        //sys_name
        var_string = cyg_httpd_form_varable_string(p, "sys_name", &len);
        if (len > 0) {
            (void) cgi_unescape(var_string, newconf.sys_name, len, sizeof(newconf.sys_name));
        } else {
            strcpy(newconf.sys_name, "");
        }

        //sys_location
        var_string = cyg_httpd_form_varable_string(p, "sys_location", &len);
        if (len > 0) {
            (void) cgi_unescape(var_string, newconf.sys_location, len, sizeof(newconf.sys_location));
        } else {
            strcpy(newconf.sys_location, "");
        }

#ifndef VTSS_SW_OPTION_DAYLIGHT_SAVING
        //timezone
        if (cyg_httpd_form_varable_int(p, "timezone", &tz_off) && sysconf.tz_off != tz_off) {
            newconf.tz_off = tz_off;
        }
#endif

        if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
            mesa_rc rc;
            if ((rc = system_set_config(&newconf)) < 0) {
                T_D("system_set_config: failed rc = %d", rc);
            }
        }
        redirect(p, "/sysinfo_config.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        char encoded_string[3 * VTSS_SYS_STRING_LEN];
        (void)cyg_httpd_start_chunked("html");

        /* get form data
           Format: sys_contact,sys_name,sys_location,timezone
        */

        /* should get these values from management APIs */
        (void) system_get_config(&conf);

        if (strlen(conf.sys_contact)) {
            ct = cgi_escape(conf.sys_contact, encoded_string);
            (void)cyg_httpd_write_chunked(encoded_string, ct);
        }
        (void)cyg_httpd_write_chunked(",", 1);

        if (strlen(conf.sys_name)) {
            ct = cgi_escape(conf.sys_name, encoded_string);
            (void)cyg_httpd_write_chunked(encoded_string, ct);
        }
        (void)cyg_httpd_write_chunked(",", 1);

        if (strlen(conf.sys_location)) {
            ct = cgi_escape(conf.sys_location, encoded_string);
            (void)cyg_httpd_write_chunked(encoded_string, ct);
        }

#ifndef VTSS_SW_OPTION_DAYLIGHT_SAVING
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ",%d", sysconf.tz_off);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
#endif

        (void)cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_syslog, "/stat/syslog", handler_stat_syslog);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_syslog_detailed, "/stat/syslog_detailed", handler_stat_syslog_detailed);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_sysinfo, "/config/sysinfo", handler_config_sysinfo);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_syslog, "/config/syslog_config", handler_config_syslog);
