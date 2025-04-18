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
#include "pvlan_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define PVLAN_WEB_BUF_LEN 512

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

#if defined(PVLAN_SRC_MASK_ENA)
static i32 handler_config_pvlan(CYG_HTTPD_STATE *p)
{
    pvlan_mgmt_entry_t conf;
    vtss_isid_t        sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    CapArray<mesa_pvlan_no_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> this_privatevid_is_new;
    int                ct, new_entry, i;
    BOOL               next = 0;
    port_iter_t        pit;
    int                max_pvlan_count = port_count_max();

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PVLAN)) {
        return -1;
    }
#endif

    T_D("PVLAN");
    if (p->method == CYG_HTTPD_METHOD_POST) {
        pvlan_mgmt_entry_t newconf;
        char search_str[32];
        int errors = 0;

        // Delete the PVLAN seleted by the user.
        int privatevid = 0;
        next = 0;
        while ((pvlan_mgmt_pvlan_get(sid, privatevid, &conf, next) == VTSS_RC_OK) ||
               (pvlan_mgmt_pvlan_get(sid, privatevid, &conf, 1) == VTSS_RC_OK)) {
            next = 1;
            /* delete_%d: CHECKBOX */
            sprintf(search_str, "delete_%d", iport2uport(conf.privatevid));
            T_D("Searching for  PVLAN = %u, search_str = %s ", conf.privatevid, search_str);
            if (cyg_httpd_form_varable_find(p, search_str)) { /* "delete" if checked */
                T_D("Delete PVLAN = %u, search_str = %s ", conf.privatevid, search_str);
                if (pvlan_mgmt_pvlan_del(conf.privatevid) != VTSS_RC_OK) {
                    T_E("pvlan_mgmt_pvlan_del(%d): failed", conf.privatevid);
                }

                // Do not update 'privatevid' variable, since that will cause
                // pvlan_mgmt_pvlan_get() to return NULL, since we've just deleted it.
                continue;
            }

            privatevid = conf.privatevid;
        }

        // Check for addition of new PVLANs.
        // Entries added are named "privatevid_new_X", where X always starts at 1. The value
        // of privatevid_new_X is the PVLAN ID. At most VTSS_PVLAN_NO_END IDs are supported,
        // so no need to check for more than that.
        for (new_entry = 0; new_entry < max_pvlan_count; new_entry++) {
            this_privatevid_is_new[new_entry] = VTSS_PORT_NO_NONE;
            sprintf(search_str, "privatevid_new_%d", 1 +  new_entry);
            T_D("Seaching for new PVLAN entry, search_str = %s", search_str);
            if (cyg_httpd_form_varable_int(p, search_str, (int *)&newconf.privatevid) &&
                newconf.privatevid > 0 &&
                newconf.privatevid <= (u32) max_pvlan_count) {

                newconf.privatevid = uport2iport(newconf.privatevid);
                this_privatevid_is_new[new_entry] = newconf.privatevid;


                T_D("new PVLAN entry, new_entry = %d, privatevid = %u", new_entry, newconf.privatevid);
                (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
                while (port_iter_getnext(&pit)) {
                    sprintf(search_str, "mask_new_%d_%d", new_entry + 1, pit.uport);
                    newconf.ports[pit.iport] = cyg_httpd_form_varable_find(p, search_str) != NULL;
                }

                mesa_rc pvlan_rc = pvlan_mgmt_pvlan_add(sid, &newconf);
                if (pvlan_rc != VTSS_RC_OK && pvlan_rc != PVLAN_ERROR_DEL_INSTEAD_OF_ADD) {
                    T_E("pvlan_mgmt_pvlan_add(%d): failed", newconf.privatevid);
                } else {
                    T_D("pvlan_mgmt_pvlan_add(%d): succeeded", newconf.privatevid);
                }
            }
        }

        privatevid = 0;
        next = 0;
        while ((pvlan_mgmt_pvlan_get(sid, privatevid, &conf, next) == VTSS_RC_OK) ||
               (pvlan_mgmt_pvlan_get(sid, privatevid, &conf, 1) == VTSS_RC_OK)) {
            next = 1;
            BOOL privatevid_just_added = FALSE;
            for (i = 0; i < max_pvlan_count; i++) {
                if (this_privatevid_is_new[i] == conf.privatevid) {
                    privatevid_just_added = TRUE;
                    break;
                }
            }

            if (privatevid_just_added) {
                // Move on to the next.
                privatevid = conf.privatevid;
                continue;
            }

            newconf = conf;

            /* mask_%d: CHECKBOX */
            (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                sprintf(search_str, "mask_%d_%d", iport2uport(conf.privatevid), pit.uport);
                newconf.ports[pit.iport] = cyg_httpd_form_varable_find(p, search_str) != NULL;
            }

            if (memcmp(&newconf, &conf, sizeof(newconf)) != 0) {
                mesa_rc pvlan_rc = pvlan_mgmt_pvlan_add(sid, &newconf);
                if (pvlan_rc != VTSS_RC_OK && pvlan_rc != PVLAN_ERROR_DEL_INSTEAD_OF_ADD) {
                    T_E("pvlan_mgmt_pvlan_add(%d): failed", conf.privatevid);
                    errors++; /* Probably stack error */
                }
            }

            // Move on to the next privatevid.
            privatevid = conf.privatevid;
        }

        redirect(p, errors ? STACK_ERR_URL : "/pvlan.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        conf.privatevid = 0;
        next = 0;
        while ((pvlan_mgmt_pvlan_get(sid, conf.privatevid, &conf, next) == VTSS_RC_OK) ||
               (pvlan_mgmt_pvlan_get(sid, conf.privatevid, &conf, 1) == VTSS_RC_OK)) {
            next = 1;
            T_D("Found PVLAN");
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u,", iport2uport(conf.privatevid));
            cyg_httpd_write_chunked(p->outbuffer, ct);
            (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/", conf.ports.get(pit.iport));
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            cyg_httpd_write_chunked("|", 1);
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}
#endif /* PVLAN_SRC_MASK_ENA */

static i32 handler_config_port_isolation(CYG_HTTPD_STATE *p)
{
    vtss_isid_t      sid  = web_retrieve_request_sid(p); /* Includes USID = ISID */
    mesa_port_list_t new_members, members;
    int              ct;
    port_iter_t      pit;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PVLAN)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        char var_mask[32];
        int errors = 0;

        if (pvlan_mgmt_isolate_conf_get(sid, members) == VTSS_RC_OK) {
            /* mask_%d: CHECKBOX */
            (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                sprintf(var_mask, "mask_%d", pit.uport);
                new_members[pit.iport] = FALSE;
                if (cyg_httpd_form_varable_find(p, var_mask)) { /* "on" if checked */
                    new_members[pit.iport] = TRUE;
                }
            }

            if (new_members != members) {
                if (pvlan_mgmt_isolate_conf_set(sid, new_members) < 0) {
                    T_E("pvlan_mgmt_isolate_conf_set: failed");
                    errors++; /* Probably stack error */
                }
            }
        }

        if (errors) {
            T_W("errors: %d", errors);
        }

        redirect(p, errors ? STACK_ERR_URL : "/port_isolation.htm");
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)pvlan_mgmt_isolate_conf_get(sid, members);
        cyg_httpd_start_chunked("html");
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/", members.get(pit.iport));
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

#if defined(PVLAN_SRC_MASK_ENA)
/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t pvlan_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[PVLAN_WEB_BUF_LEN];
    int  max_pvlan_count = port_count_max();
    (void) snprintf(buff, PVLAN_WEB_BUF_LEN,
                    "var configPvlanIdMin = %u;\n"
                    "var configPvlanIdMax = %u;\n",
                    iport2uport(0),
                    iport2uport(max_pvlan_count - 1) /* Last PVLAN ID (both entry and abolute ID) */
                   );

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/
web_lib_config_js_tab_entry(pvlan_lib_config_js);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_pvlan, "/config/pvlan", handler_config_pvlan);
#endif /* PVLAN_SRC_MASK_ENA */
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_port_isolation, "/config/port_isolation", handler_config_port_isolation);

