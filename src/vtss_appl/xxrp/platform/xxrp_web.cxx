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
#include "os_file_api.h"
#include <dirent.h>
#include <unistd.h>
#ifdef VTSS_SW_OPTION_GVRP
#include "../base/vtss_gvrp.h"
#endif
#include "xxrp_api.h"
#include "vtss_xxrp_callout.h"
#include "xxrp_trace.h"
#include "critd_api.h"
#include "vtss_mrp.hxx"
#include "vlan_api.h"

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define MRP_WEB_BUF_LEN 512

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_XXRP
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_XXRP

VTSS_CRIT_SCOPE_CLASS_EXTERN(xxrp_appl_crit, VtssXxrpApplCritdGuard);

#define XXRP_APPL_CRIT_SCOPE() VtssXxrpApplCritdGuard __lock_guard__(__LINE__)

#ifdef VTSS_SW_OPTION_GVRP
static const struct {
    const char *name;
    enum timer_context tc;
} elem_name[3] = {
    {"jointime", GARP_TC__transmitPDU},
    {"leavetime", GARP_TC__leavetimer},
    {"leavealltime", GARP_TC__leavealltimer}
};

static i32 gvrp_handler_conf_status(CYG_HTTPD_STATE *httpd_state)
{
    T_D("entry");

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(httpd_state, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_XXRP)) {
        goto early_out;
    }
#endif

    switch (httpd_state->method) {
    case CYG_HTTPD_METHOD_GET: {
        int cnt;
        char str[32]; // 2+1+2+1+3+1+5+1= 16, so plus some
        char encoded_string[32]; // 2+1+2+1+3+1+5+1= 16, so plus some

        u32 jointtime = vtss_gvrp_get_timer(GARP_TC__transmitPDU);
        u32 leavetime = vtss_gvrp_get_timer(GARP_TC__leavetimer);
        u32 leavealltime = vtss_gvrp_get_timer(GARP_TC__leavealltimer);
        u32 maxvlans = vtss_gvrp_max_vlans();
        int enabled = vtss_gvrp_is_enabled();

        sprintf(str, "OK*%d*%d*%d*%d*%d", jointtime, leavetime, leavealltime, maxvlans, enabled);

        (void)cyg_httpd_start_chunked("html");
        cnt = cgi_escape(str, encoded_string);
        (void)cyg_httpd_write_chunked(encoded_string, cnt);
        (void)cyg_httpd_end_chunked();
    }
    break;

    case CYG_HTTPD_METHOD_POST: {
        const char  *var_p;
        size_t var_len, var_len2;
        char str[10];
        int i;
        int appl_exclusive = 1;

        str[9] = 0;

        (void)cyg_httpd_form_varable_string(httpd_state, "gvrp_enable", &var_len2);

        var_p = cyg_httpd_form_varable_string(httpd_state, "maxvlans", &var_len);
        str[0] = 0;
        if (var_p && var_len) {
            strncpy(str, var_p, MIN(sizeof(str) - 1, var_len));
            str[MIN(sizeof(str) - 1, var_len)] = 0;
        }

        XXRP_APPL_CRIT_SCOPE();
        /* Check that no other MRP/GARP application is currently enabled */
        if (xxrp_mgmt_appl_exclusion(VTSS_GARP_APPL_GVRP)) {
            appl_exclusive = 0;
        }

        // (1) --- Get global GVRP settings.
        if (0 == var_len2) {
            if (appl_exclusive) {
                // --- Disable GVRP
                (void)xxrp_mgmt_global_enabled_set(VTSS_GARP_APPL_GVRP, 0);
                GVRP_CRIT_ENTER();
                vtss_gvrp_destruct(FALSE);
                GVRP_CRIT_EXIT();
            }
            (void)vtss_gvrp_max_vlans_set(atoi(str));

        } else {
            // --- Enable GVRP. Get how many VLANs shall be possible.
            if (appl_exclusive) {
                // Set number of VLANs before enabling GVRP.
                (void)vtss_gvrp_max_vlans_set(atoi(str));
                // Then enable GVRP.
                (void)vtss_gvrp_construct(-1, vtss_gvrp_max_vlans());
                (void)xxrp_mgmt_global_enabled_set(VTSS_GARP_APPL_GVRP, TRUE);
            }
        }


        // --- Get  join-time, leave-time, leaveall-time.
        for (i = 0; i < 3; ++i) {

            var_p = cyg_httpd_form_varable_string(httpd_state, elem_name[i].name, &var_len);
            str[0] = 0;
            if (var_p && var_len) {
                strncpy(str, var_p, MIN(sizeof(str) - 1, var_len));
                str[MIN(sizeof(str) - 1, var_len)] = 0;
            }

            (void)vtss_gvrp_set_timer(elem_name[i].tc, atoi(str));
        }
        redirect(httpd_state, appl_exclusive ? "/gvrp_config.htm" : "/gvrp_config.htm?GVRP_error=1");
    }
    break;

    default:
        break;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
early_out:
#endif

    return -1;
}

static i32 gvrp_handler_port_enable(CYG_HTTPD_STATE *httpd_state)
{
    vtss_isid_t    isid  = web_retrieve_request_sid(httpd_state); /* Includes USID = ISID */
    port_iter_t  pit;

    T_D("entry");

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(httpd_state, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_XXRP)) {
        goto early_out2;
    }
#endif

    switch (httpd_state->method) {
    case CYG_HTTPD_METHOD_GET: {
        static const char *const F[2] = {"%d.%d", "*%d.%d"};
        int cnt;
        char str[32];
        char encoded_string[32];
        BOOL enable;
        int first = 0;

        (void)cyg_httpd_start_chunked("html");

        (void)port_iter_init(&pit, 0, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {

            (void)xxrp_mgmt_enabled_get(isid, pit.iport, VTSS_GARP_APPL_GVRP, &enable);
            sprintf(str, F[first], pit.uport, enable ? 1 : 0);
            cnt = cgi_escape(str, encoded_string);
            (void)cyg_httpd_write_chunked(encoded_string, cnt);

            first = 1;
        }

        (void)cyg_httpd_end_chunked();
    }
    break;

    case CYG_HTTPD_METHOD_POST: {
        const char *var_p;
        size_t var_len;
        char str[10];

        str[9] = 0;

        (void)port_iter_init(&pit, 0, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {

            sprintf(str, "mode_%d", (int)pit.uport);
            var_p = cyg_httpd_form_varable_string(httpd_state, str, &var_len);

            if (var_len) {
                strncpy(str, var_p, MIN(sizeof(str) - 1, var_len));
                str[MIN(sizeof(str) - 1, var_len)] = 0;
                (void)xxrp_mgmt_enabled_set(isid, pit.iport, VTSS_GARP_APPL_GVRP, str[0] == '1');
            }
        }
        redirect(httpd_state, "/gvrp_port.htm");

    }
    break;

    default:
        break;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
early_out2:
#endif

    return -1;
}
#endif

#ifdef VTSS_SW_OPTION_MVRP
static i32 mrp_handler_conf(CYG_HTTPD_STATE *p)
{
    int                       ct;
    vtss_isid_t               sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t               pit;
    vtss_mrp_timer_conf_t     timers;
    bool                      periodic = false;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MRP)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int timer_errors = 0, periodic_errors = 0;

        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            int timeout =  0;

            if (cyg_httpd_form_variable_int_fmt(p, &timeout, "join_time_%u", pit.iport)) {
                timers.join_timer = (u32) timeout;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &timeout, "leave_time_%u", pit.iport)) {
                timers.leave_timer = (u32) timeout;
            }
            if (cyg_httpd_form_variable_int_fmt(p, &timeout, "leaveall_time_%u", pit.iport)) {
                timers.leave_all_timer = (u32) timeout;
            }
            periodic = cyg_httpd_form_variable_check_fmt(p, "periodic_%u", pit.iport);

            if (mrp_mgmt_timers_set(sid, pit.iport, &timers) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Could not set timer "
                                                           << "configuration for port "
                                                           << pit.iport;
                timer_errors++;

            }
            if (vtss::mrp::mgmt_periodic_state_set(sid, pit.iport, periodic) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Could not set periodic "
                                                           << "configuration for port "
                                                           << pit.iport;
                periodic_errors++;
            }
        }
        if (timer_errors) {
            redirect(p, "/mrp_port.htm?error_code=1");
        } else if (periodic_errors) {
            redirect(p, "/mrp_port.htm?error_code=2");
        } else {
            redirect(p, "/mrp_port.htm");
        }
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */ 
        cyg_httpd_start_chunked("html");
        /* Format: 
         * <port 1>,<port 2>,<port 3>,...<port n>
         * 
         * port x :== <port_no>/<JoinTime>/<LeaveTime>/<LeaveAllTime>/<PeriodicTransmission>
         *   port_no              :== 1..max
         *   JoinTime             :== 1..20 (cs)
         *   LeaveTime            :== 60..300 (cs)
         *   LeaveAllTime         :== 1000..5000 (cs)
         *   PeriodicTransmission :== 0..1            // 0: Disabled (default), 1: Enabled
         */
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT,
                              PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (mrp_mgmt_timers_get(sid, pit.iport, &timers) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Could not get timer "
                                                           << "configuration for port "
                                                           << pit.iport;
                break;
            }
            if (vtss::mrp::mgmt_periodic_state_get(sid, pit.iport, periodic) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Could not get periodic "
                                                           << "configuration for port "
                                                           << pit.iport;
                break;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u/%u/%u/%u/%u",
                          pit.first ? "" : ",",
                          pit.uport,
                          timers.join_timer,
                          timers.leave_timer,
                          timers.leave_all_timer,
                          periodic);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

    return -1;
}

/* mvrp_web_vlan_list_get(), modified from VLAN_WEB_vlan_list_get() */
static bool mvrp_web_vlan_list_get(CYG_HTTPD_STATE *p,
                                   const char *id,
                                   char big_storage[3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES],
                                   char small_storage[VLAN_VID_LIST_AS_STRING_LEN_BYTES],
                                   vtss::VlanList &vls)
{
    size_t found_len, cnt = 0;
    char   *found_ptr;

    if ((found_ptr = (char *)cyg_httpd_form_varable_string(p, id, &found_len)) != NULL) {

        // Unescape the string
        if (!cgi_unescape(found_ptr, big_storage, found_len,
                          3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES)) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, WARNING) << "Unable to un-escape "
                                                         << found_ptr;
            return false;
        }

        // Filter out white space
        found_ptr = big_storage;
        while (*found_ptr) {
            char c = *(found_ptr++);
            if (c != ' ') {
                small_storage[cnt++] = c;
            }
        }
    } else {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, WARNING) << "Element with id = "
                                                     << id << " not found!";
        return false;
    }

    small_storage[cnt] = '\0';

    VTSS_ASSERT(cnt < VLAN_VID_LIST_AS_STRING_LEN_BYTES);

    if (xxrp_mgmt_txt_to_vlan_list(small_storage, vls, VTSS_APPL_MVRP_VLAN_ID_MIN, VTSS_APPL_MVRP_VLAN_ID_MAX) != VTSS_RC_OK) {
        VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, WARNING) << "Failed to convert "
                                                     << small_storage
                                                     << " to VlanList form";
        return false;
    }

    return true;
}

static i32 mvrp_handler_conf(CYG_HTTPD_STATE *p)
{
    int            ct;
    vtss_isid_t    sid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    port_iter_t    pit;
    vtss::VlanList vls;
    char           *unescaped_vlan_list_as_string = NULL;
    char           *escaped_vlan_list_as_string   = NULL;
    BOOL           enabled                        = FALSE;
    int            appl_exclusive                 = 1;

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MRP)) {
        return -1;
    }
#endif

    // A couple of things comment to both POST and GET operations:
    if ((unescaped_vlan_list_as_string = (char *)VTSS_MALLOC(VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
        T_E("Alloc of %d bytes failed", VLAN_VID_LIST_AS_STRING_LEN_BYTES);
        goto do_exit;
    }

    if ((escaped_vlan_list_as_string = (char *)VTSS_MALLOC(3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES)) == NULL) {
        T_E("Alloc of %d bytes failed", 3 * VLAN_VID_LIST_AS_STRING_LEN_BYTES);
        goto do_exit;
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        enabled = FALSE;
        int var_value = 0;

        if (cyg_httpd_form_varable_int(p, "global_state_", &var_value)) {
            enabled = (BOOL) var_value;
        }
        if (!mvrp_web_vlan_list_get(p, "vlans_", escaped_vlan_list_as_string,
            unescaped_vlan_list_as_string, vls)) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Failed to fetch VLAN list";
            goto do_exit;
        }

        XXRP_APPL_CRIT_SCOPE();
        /* Check that no other MRP/GARP application is currently enabled */
        if (xxrp_mgmt_appl_exclusion(VTSS_MRP_APPL_MVRP)) {
            appl_exclusive = 0;
        }
        if (enabled) {
            if (xxrp_mgmt_global_managed_vids_set(VTSS_MRP_APPL_MVRP, vls) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Failed to update VLAN list";
                goto do_exit;
            }
            if (appl_exclusive) {
                if (xxrp_mgmt_global_enabled_set(VTSS_MRP_APPL_MVRP, enabled) != VTSS_RC_OK) {
                    VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Failed to set global state";
                    goto do_exit;
                }
            }
        } else {
            if (xxrp_mgmt_global_enabled_set(VTSS_MRP_APPL_MVRP, enabled) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Failed to set global state";
                goto do_exit;
            }
            if (xxrp_mgmt_global_managed_vids_set(VTSS_MRP_APPL_MVRP, vls) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Failed to update VLAN list";
                goto do_exit;
            }
        }

        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            enabled = FALSE;
            enabled = cyg_httpd_form_variable_check_fmt(p, "enabled_%u", pit.iport);
            if (xxrp_mgmt_enabled_set(sid, pit.iport, VTSS_MRP_APPL_MVRP, enabled) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Failed to set port state";
                goto do_exit;
            }
        }

        goto do_exit;
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */ 
        cyg_httpd_start_chunked("html");
        /* Format:
         * <state>#<vlans>#<all_port_conf>
        * state         :== 0..1                    // 0: Disabled (default), 1: Enabled
        * vlans         :== Vlan list of managed VLANs
        * all_port_conf :== <port 1>,<port 2>,<port 3>,...<port n>
        * 
        * port x :== <port_no>/<state>
        *   port_no :== 1..max
        *   state   :== 0..1   // 0: Disabled (default), 1: Enabled
        */

        enabled = FALSE;
        if (xxrp_mgmt_global_enabled_get(VTSS_MRP_APPL_MVRP, &enabled) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Failed to fetch global state";
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u#", enabled);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        for (int v = VTSS_APPL_MVRP_VLAN_ID_MIN; v <= VTSS_APPL_MVRP_VLAN_ID_MAX; ++v) {
            vls.set(v);
        }
        if (xxrp_mgmt_global_managed_vids_get(VTSS_MRP_APPL_MVRP, vls) != VTSS_RC_OK) {
            VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Failed to fetch VLAN list";
        }
        (void)cgi_escape(xxrp_mgmt_vlan_list_to_txt(vls, unescaped_vlan_list_as_string), escaped_vlan_list_as_string);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#", escaped_vlan_list_as_string);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT,
                              PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            enabled = FALSE;
            if (xxrp_mgmt_enabled_get(sid, pit.iport, VTSS_MRP_APPL_MVRP, &enabled) != VTSS_RC_OK) {
                VTSS_TRACE(VTSS_TRACE_XXRP_GRP_WEB, ERROR) << "Could not get state "
                                                           << "for port "
                                                           << pit.iport;
            }
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%u/%u",
                          pit.first ? "" : ",",
                          pit.uport,
                          enabled);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }

do_exit:
    VTSS_FREE(escaped_vlan_list_as_string);
    VTSS_FREE(unescaped_vlan_list_as_string);
    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, appl_exclusive ? "/mvrp_config.htm" : "/mvrp_config.htm?error_code=1");
    }
    return -1;
}

static i32 mvrp_handler_stat(CYG_HTTPD_STATE *p)
{
    vtss_isid_t sid = web_retrieve_request_sid(p); /* Includes USID = ISID */

    if (redirectUnmanagedOrInvalid(p, sid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MRP)) {
        return -1;
    }
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {
        redirect(p, "/mvrp_stat.htm");
    } else { /* CYG_HTTPD_METHOD_GET (+HEAD) */
        int                        ct;
        port_iter_t                pit;
        vtss::mrp::MrpApplPortStat stat;

        cyg_httpd_start_chunked("html");
        /* Format: 
        * <port 1>,<port 2>,<port 3>,...<port n>
        * 
        * port x :== <port_no>/<FailedRegistrations>/<LastPduOrigin>
        *   port_no              :== 1..max
        *   FailedRegistrations  :== 0..2^64-1
        *   LastPduOrigin        :== MAC address
        */
        (void) port_iter_init(&pit, NULL, sid, PORT_ITER_SORT_ORDER_IPORT,
                              PORT_ITER_FLAGS_NORMAL);
        while (port_iter_getnext(&pit)) {
            if (vtss::mrp::mgmt_stat_port_get(sid, pit.iport, VTSS_APPL_MRP_APPL_MVRP, stat) != VTSS_RC_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              "%s%u/%u/%02x-%02x-%02x-%02x-%02x-%02x",
                              pit.first ? "" : ",",
                              pit.uport,
                              0,
                              0x0,
                              0x0,
                              0x0,
                              0x0,
                              0x0,
                              0x0);
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              "%s%u/" VPRI64u"/%02x-%02x-%02x-%02x-%02x-%02x",
                              pit.first ? "" : ",",
                              pit.uport,
                              stat.failedRegistrations,
                              stat.lastPduOrigin[0],
                              stat.lastPduOrigin[1],
                              stat.lastPduOrigin[2],
                              stat.lastPduOrigin[3],
                              stat.lastPduOrigin[4],
                              stat.lastPduOrigin[5]);
            }
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        cyg_httpd_end_chunked();
    }
    return -1;
}
#endif

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t mrp_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[MRP_WEB_BUF_LEN];
    (void) snprintf(buff, MRP_WEB_BUF_LEN,
                    "var configMrpJoinTimeMin = %u;\n"
                    "var configMrpJoinTimeMax = %u;\n"
                    "var configMrpLeaveTimeMin = %u;\n"
                    "var configMrpLeaveTimeMax = %u;\n"
                    "var configMrpLeaveAllTimeMin = %u;\n"
                    "var configMrpLeaveAllTimeMax = %u;\n"
                    "var configMvrpVlanIdMin = %u;\n"
                    "var configMvrpVlanIdMax = %u;\n"
                    ,
                    VTSS_MRP_JOIN_TIMER_MIN,
                    VTSS_MRP_JOIN_TIMER_MAX,
                    VTSS_MRP_LEAVE_TIMER_MIN,
                    VTSS_MRP_LEAVE_TIMER_MAX,
                    VTSS_MRP_LEAVE_ALL_TIMER_MIN,
                    VTSS_MRP_LEAVE_ALL_TIMER_MAX,
                    VTSS_APPL_MVRP_VLAN_ID_MIN,
                    VTSS_APPL_MVRP_VLAN_ID_MAX
                   );
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib_config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(mrp_lib_config_js);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

#ifdef VTSS_SW_OPTION_GVRP
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_gvrp_conf,       "/config/gvrp_conf_status",      gvrp_handler_conf_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_gvrp_conf_port,  "/config/gvrp_conf_port_enable", gvrp_handler_port_enable);
#endif

#ifdef VTSS_SW_OPTION_MVRP
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_mrp_conf,
                              "/config/mrp_conf",
                              mrp_handler_conf);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_mvrp_conf,
                              "/config/mvrp_conf",
                              mvrp_handler_conf);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_mvrp_stat,
                              "/stat/mvrp_stat",
                              mvrp_handler_stat);
#endif

