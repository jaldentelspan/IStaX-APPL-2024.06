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

#include <vtss/appl/psec.h>            /* Public interface to PSEC and PSEC_LIMIT modules */
#include <vtss/appl/vlan.h>            /* For VTSS_APPL_VLAN_ID_MIN/MAX                   */
#include <vtss/basics/enum_macros.hxx> /* For VTSS_ENUM_INC()                             */
#include "web_api.h"                   /* For all web-related functions                   */
#include "msg_api.h"                   /* For msg_abstime_get()                           */
#include "misc_api.h"                  /* For iport2uport()                               */
#include "psec_trace.h"                /* For T_xxx() macros                              */
#include "psec_util.h"                 /* For psec_util_XXX()                             */
#include "port_api.h"                  /* For port_count_max()                            */
#include "port_iter.hxx"               /* For port_iter_t                                 */

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_PSEC
#define PSEC_WEB_BUF_LEN 512

// Allow operator++ on vtss_appl_psec_user_t
VTSS_ENUM_INC(vtss_appl_psec_user_t);

/*lint -sem(handler_stat_psec_status_port, thread_protected) */

/******************************************************************************/
// psec_web_switch_status()
/******************************************************************************/
static mesa_rc psec_web_switch_status(vtss_isid_t isid, CYG_HTTPD_STATE *p)
{
    mesa_rc                                        rc = VTSS_RC_OK;
    vtss_appl_psec_user_t                          user;
    int                                            cnt;
    BOOL                                           first;
    vtss_appl_psec_interface_status_t              port_status;
    vtss_appl_psec_interface_notification_status_t port_notif_status;
    port_iter_t                                    pit;
    u32                                            included_in_build_mask = psec_util_users_included_get();
#ifdef VTSS_SW_OPTION_PSEC_LIMIT
    vtss_appl_psec_interface_conf_t                port_conf;
#endif

    (void)cyg_httpd_start_chunked("html");

    // Format [Sys]#[Users]#[PortStatus1]#[PortStatus2]#...#[PortStatusN]

    // [Sys]: psec_limit_supported
    cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d#",
#ifdef VTSS_SW_OPTION_PSEC_LIMIT
                   1
#else
                   0
#endif
                  );

    // [Users]: user_name_1/user_abbr_1/user_name_2/user_abbr_2/.../user_name_N/user_abbr_N
    first = TRUE;
    for (user = (vtss_appl_psec_user_t)0; user < VTSS_APPL_PSEC_USER_LAST; user++) {
        if ((included_in_build_mask & VTSS_BIT(user)) == 0) {
            continue;
        }

        cnt += snprintf(p->outbuffer + cnt, sizeof(p->outbuffer) - cnt, "%s%s/%c", first ? "" : "/", psec_util_user_name_get(user), psec_util_user_abbr_get(user));
        first = FALSE;
    }

    (void)cyg_httpd_write_chunked(p->outbuffer, cnt);

    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        u32            violation_mode = 0, state = 0, limit = 0;
        vtss_ifindex_t ifindex;
        BOOL           psec_limit_enabled = FALSE;

        if ((rc = vtss_ifindex_from_port(isid, pit.iport, &ifindex)) != VTSS_RC_OK) {
            T_E("%u, %u => %s", isid, pit.iport, error_txt(rc));
            goto do_exit;
        }

#ifdef VTSS_SW_OPTION_PSEC_LIMIT
        // As a service, also provide the current limit, if PSEC LIMIT is enabled.
        if ((rc = vtss_appl_psec_interface_conf_get(ifindex, &port_conf)) != VTSS_RC_OK) {
            goto do_exit;
        }
#endif

        (void)cyg_httpd_write_chunked("#", 1);

        if ((rc = vtss_appl_psec_interface_status_get(ifindex, &port_status)) != VTSS_RC_OK) {
            goto do_exit;
        }

        if ((rc = vtss_appl_psec_interface_notification_status_get(ifindex, &port_notif_status)) != VTSS_RC_OK) {
            goto do_exit;
        }

#ifdef VTSS_SW_OPTION_PSEC_LIMIT
        if (port_status.users  & VTSS_BIT(VTSS_APPL_PSEC_USER_ADMIN)) {
            // The Limit Control module is enabled on this port. Get the limit.
            psec_limit_enabled = TRUE;
            limit = port_conf.limit;
            violation_mode = port_conf.violation_mode + 1;
        }

        if (port_status.users != 0) {
            if (port_notif_status.shut_down) {
                state = 3; // Shut down
            } else if (port_status.limit_reached) {
                state = 2; // Limit reached
            } else {
                state = 1; // Ready
            }
        }
#endif

        // [PortStatus]: uport/ena_users_mask/violation_mode/state/cur_mac_cnt/psec_limit_enabled/cur_viol_cnt/limit
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%u/%u/%u/%u/%d/%u/%u", pit.uport, port_status.users, violation_mode, state, port_status.mac_cnt, psec_limit_enabled, port_status.cur_violate_cnt, limit);
        (void)cyg_httpd_write_chunked(p->outbuffer, cnt);
    }

do_exit:
    cyg_httpd_end_chunked();
    return rc;
}

/******************************************************************************/
// PSEC_WEB_port_status()
/******************************************************************************/
static mesa_rc PSEC_WEB_port_status(vtss_isid_t isid, mesa_port_no_t iport, CYG_HTTPD_STATE *p)
{
    vtss_appl_psec_mac_status_map_t                 mac_status_map;
    vtss_appl_psec_mac_status_map_t::const_iterator mac_itr;
    mesa_port_no_t                                  prev_iport = MESA_PORT_NO_NONE;
    char                                            buf[100];
    int                                             cnt;

    // Get all MAC addresses learned by PSEC.
    VTSS_RC(vtss_appl_psec_interface_status_mac_get_all(mac_status_map));

    (void)cyg_httpd_start_chunked("html");

    // If iport == MESA_PORT_NO_NONE, MAC addresses for all ports are returned.
    // Format in that case is:
    //   ALL|[PortData_1]|[PortData_2]|...|[PortData_N]
    // Where
    //   [PortData_x]: port#[MACs]
    //
    // If iport != MESA_PORT_NO_NONE, only MAC addresses for the specific port
    // is returned.
    // Format in that case is:
    //   port#[MACs]
    //
    // Where
    //   [MACs]: [MAC_1]#[MAC_2]#...#[MAC_N]
    //   [MAC_x]: vid_x/mac_address_x/type_x/state_x/age_hold_time_left_x

    if (iport == MESA_PORT_NO_NONE) {
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "ALL");
        cyg_httpd_write_chunked(p->outbuffer, cnt);
    } else {
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d", iport2uport(iport));
        cyg_httpd_write_chunked(p->outbuffer, cnt);
        prev_iport = iport;
    }

    // Loop through all MAC addresses
    for (mac_itr = mac_status_map.cbegin(); mac_itr != mac_status_map.cend(); ++mac_itr) {
        const vtss_appl_psec_mac_status_t *mac_status;
        vtss_ifindex_elm_t                ife;

        VTSS_RC(vtss_ifindex_decompose(mac_itr->first.ifindex, &ife));
        if (ife.iftype != VTSS_IFINDEX_TYPE_PORT) {
            T_E("ifindex = %u is not a port type", VTSS_IFINDEX_PRINTF_ARG(mac_itr->first.ifindex));
            return MESA_RC_ERROR;
        }

        if (iport != MESA_PORT_NO_NONE && iport != ife.ordinal) {
            // Not the one we are looking for
            continue;
        }

        if (prev_iport != ife.ordinal) {
            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%d", iport2uport(ife.ordinal));
            cyg_httpd_write_chunked(p->outbuffer, cnt);
            prev_iport = ife.ordinal;
        }

        mac_status = &mac_itr->second;

        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%u/%s/%s/%s/%u",
                       mac_status->vid_mac.vid,
                       misc_mac_txt(mac_status->vid_mac.mac.addr, buf),
                       psec_util_mac_type_to_str(mac_status->mac_type),
                       mac_status->violating ? "Violating" : (mac_status->blocked || mac_status->kept_blocked) ? "Blocked" : "Forwarding",
                       mac_status->age_or_hold_time_secs);
        (void)cyg_httpd_write_chunked(p->outbuffer, cnt);
    }

    cyg_httpd_end_chunked();
    return VTSS_RC_OK;
}

/****************************************************************************/
// handler_stat_psec_status_switch()
/****************************************************************************/
static i32 handler_stat_psec_status_switch(CYG_HTTPD_STATE *p)
{
    mesa_rc     rc;
    vtss_isid_t isid = web_retrieve_request_sid(p);

    if (redirectUnmanagedOrInvalid(p, isid)) {
        /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK)) {
        return -1;
    }
#endif

    // Since the psec_web_switch_status() uses semi-public functions to get the
    // current status (and I don't want to make them fully public), I've
    // moved all of it to psec_web.c
    if ((rc = psec_web_switch_status(isid, p)) != VTSS_RC_OK) {
        T_E("%s", error_txt(rc));
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
// handler_stat_psec_status_port()
/****************************************************************************/
static i32 handler_stat_psec_status_port(CYG_HTTPD_STATE *p)
{
    vtss_isid_t    isid = web_retrieve_request_sid(p);
    mesa_port_no_t iport;
    mesa_rc        rc;
    int            val;
    const char     *str;
    size_t         str_len;

    if (redirectUnmanagedOrInvalid(p, isid)) {
        /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK)) {
        return -1;
    }
#endif

    if ((str = cyg_httpd_form_varable_string(p, "port", &str_len)) && strncmp(str, "All", str_len) == 0) {
        iport = MESA_PORT_NO_NONE;
    } else if (cyg_httpd_form_varable_int(p, "port", &val)) {
        iport = uport2iport(val);
        if (iport >= port_count_max()) {
            return -1;
        }
    } else {
        return -1;
    }

    if ((rc = PSEC_WEB_port_status(isid, iport, p)) != VTSS_RC_OK) {
        T_E("%s", error_txt(rc));
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
// handler_stat_psec_clear()
/****************************************************************************/
static i32 handler_stat_psec_clear(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                               isid = web_retrieve_request_sid(p);
    vtss_appl_psec_global_control_mac_clear_t info;
    mesa_port_no_t                            iport;
    int                                       errors = 0, i;
    const char                                *var_string;
    size_t                                    len;
    uint                                      mac_addr_uint[6];

    if (redirectUnmanagedOrInvalid(p, isid)) {
        /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK)) {
        return -1;
    }
#endif

    memset(&info, 0, sizeof(info));
    info.specific_ifindex = TRUE;

    if (p->method == CYG_HTTPD_METHOD_GET) {
        (void)cyg_httpd_start_chunked("html");
        // The Clear-button-click is implemented as a CYG_HTTPD_METHOD_GET method.

        // port=%d
        if (cyg_httpd_form_varable_int(p, "port", &i)) {
            iport = uport2iport(i);
            if (iport >= port_count_max()) {
                errors++;
            }
        } else {
            errors++;
        }

        if (!errors && vtss_ifindex_from_port(isid, iport, &info.ifindex) != VTSS_RC_OK) {
            errors++;
        }

        // There might be a VLAN ID and a MAC address too on the "command line".

        // vid=%s
        if (cyg_httpd_form_varable_int(p, "vid", &i) && i >= VTSS_APPL_VLAN_ID_MIN && i <= VTSS_APPL_VLAN_ID_MAX) {
            info.specific_vlan = TRUE;
            info.vlan = i;
        }

        // mac=%s
        if ((var_string = cyg_httpd_form_varable_string(p, "mac", &len)) != NULL && len >= 17) {
            if (sscanf(var_string, "%2x-%2x-%2x-%2x-%2x-%2x", &mac_addr_uint[0], &mac_addr_uint[1], &mac_addr_uint[2], &mac_addr_uint[3], &mac_addr_uint[4], &mac_addr_uint[5]) == 6) {
                info.specific_mac = TRUE;

                // The sscanf() cannot scan hex chars.
                for (i = 0; i < 6; i++) {
                    info.mac.addr[i] = (uchar)mac_addr_uint[i];
                }
            }
        }

        // Do the clearing
        if (!errors && vtss_appl_psec_global_control_mac_clear(&info) != VTSS_RC_OK) {
            errors++;
        }

        if (errors) {
            T_W("Error in URL");
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t psec_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[PSEC_WEB_BUF_LEN];
    (void)snprintf(buff, PSEC_WEB_BUF_LEN,
#ifdef VTSS_SW_OPTION_PSEC_LIMIT
                   ".PSEC_LIMIT_DIS { display: none; }\r\n"
#else
                   ".PSEC_LIMIT_ENA { display: none; }\r\n"
#endif /* VTSS_SW_OPTION_PSEC_LIMIT */
                  );
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(psec_lib_filter_css);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_psec_status_switch, "/stat/psec_status_switch", handler_stat_psec_status_switch);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_psec_status_port,   "/stat/psec_status_port",   handler_stat_psec_status_port);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_psec_clear,         "/stat/psec_clear",         handler_stat_psec_clear);

