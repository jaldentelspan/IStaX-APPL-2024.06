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

#include "web_api.h"        /* For web-helper functions */
#include <vtss/appl/psec.h> /* For vtss_appl_psec_XXX() */
#include "msg_api.h"        /* For msg_abstime_get()    */
#include "port_iter.hxx"    /* For port_iter_t          */

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

static i32 handler_config_psec_limit(CYG_HTTPD_STATE *p)
{
    vtss_isid_t                     isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    vtss_appl_psec_global_conf_t    global_conf;
    vtss_appl_psec_interface_conf_t port_conf;
    mesa_rc                         rc;
    int                             cnt, an_integer, errors = 0;
    const char                      *err_buf_ptr;
    port_iter_t                     pit;

    if (redirectUnmanagedOrInvalid(p, isid)) {
        /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK)) {
        return -1;
    }
#endif

    if ((rc = vtss_appl_psec_global_conf_get(&global_conf)) != VTSS_RC_OK) {
        errors++;
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        if (!errors) {
            // aging_enabled: Checkbox
            global_conf.enable_aging = cyg_httpd_form_varable_find(p, "aging_enabled") ? TRUE : FALSE; // Returns non-NULL if checked

            // aging_period: Integer. Only overwrite if enabled.
            if (global_conf.enable_aging) {
                if (cyg_httpd_form_varable_int(p, "aging_period", &an_integer)) {
                    global_conf.aging_period_secs = an_integer;
                } else {
                    errors++;
                }
            }

            // hold_time: Integer.
            if (cyg_httpd_form_varable_int(p, "hold_time", &an_integer)) {
                global_conf.hold_time_secs = an_integer;
            } else {
                errors++;
            }

            if (!errors) {
                if ((rc = vtss_appl_psec_global_conf_set(&global_conf)) != VTSS_RC_OK) {
                    T_D("vtss_appl_psec_global_conf_set() failed");
                    errors++;
                }
            }

            // Port config
            (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (!errors && port_iter_getnext(&pit)) {
                char           var_name[50];
                vtss_ifindex_t ifindex;

                if (vtss_ifindex_from_port(isid, pit.iport, &ifindex)      != VTSS_RC_OK ||
                    vtss_appl_psec_interface_conf_get(ifindex, &port_conf) != VTSS_RC_OK) {
                    errors++;
                }

                // ena_%d: Int
                sprintf(var_name, "ena_%u", pit.uport);
                if (cyg_httpd_form_varable_int(p, var_name, &an_integer)) {
                    port_conf.enabled = an_integer;
                } else {
                    errors++;
                }

                if (!errors && port_conf.enabled) {
                    // limit_%d: Int
                    sprintf(var_name, "limit_%u", pit.uport);
                    if (cyg_httpd_form_varable_int(p, var_name, &an_integer)) {
                        port_conf.limit = an_integer;
                    } else {
                        errors++;
                    }

                    // violation_mode_%d: Select
                    sprintf(var_name, "violation_mode_%u", pit.uport);
                    if (cyg_httpd_form_varable_int(p, var_name, &an_integer)) {
                        port_conf.violation_mode = (vtss_appl_psec_violation_mode_t)an_integer;
                    } else {
                        errors++;
                    }

                    if (!errors && port_conf.violation_mode == VTSS_APPL_PSEC_VIOLATION_MODE_RESTRICT) {
                        sprintf(var_name, "violation_limit_%u", pit.uport);
                        if (cyg_httpd_form_varable_int(p, var_name, &an_integer)) {
                            port_conf.violate_limit = an_integer;
                        } else {
                            errors++;
                        }
                    }

                    // Sticky mode: Checkbox
                    sprintf(var_name, "sticky_ena_%u", pit.uport);
                    port_conf.sticky = cyg_httpd_form_varable_find(p, var_name) ? TRUE : FALSE; // Returns non-NULL if checked
                }

                if (!errors) {
                    if ((rc = vtss_appl_psec_interface_conf_set(ifindex, &port_conf)) != VTSS_RC_OK) {
                        T_D("vtss_appl_psec_interface_conf_set() failed");
                        errors++;
                    }
                }
            }

            if (errors) {
                // There are two types of errors: Those where a form variable was invalid,
                // and those where a vtss_appl_psec_XXX() function failed.
                // In the first case, we redirect to the STACK_ERR_URL page, and in the
                // second, we redirect to a custom error page.
                if (rc == VTSS_RC_OK) {
                    redirect(p, STACK_ERR_URL);
                } else {
                    err_buf_ptr = error_txt(rc);
                    send_custom_error(p, "Port Security Error", err_buf_ptr, strlen(err_buf_ptr));
                }
            } else {
                // No errors. Update with the current settings.
                redirect(p, "/psec_limit.htm");
            }
        }
    } else { /* CYG_HTTPD_METHOD_GET (+HEAD) */
        (void)cyg_httpd_start_chunked("html");

        if (!errors) {
            // AgingEna/AgingPeriod/HoldTime#[PortConfig]
            // Format of [PortConfig]
            // PortNumber_1/PortEna_1/Limit_1/ViolateLimit_1/ViolationMode_1/State_1#PortNumber_2/PortEna_2/ViolateLimit_2/Limit_2/ViolationMode_2/State_2#...#PortNumber_N/PortEna_N/Limit_N/ViolateLimit_N/ViolationMode_N/State_N
            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%u/%u#",
                           global_conf.enable_aging,
                           global_conf.aging_period_secs,
                           global_conf.hold_time_secs);
            cyg_httpd_write_chunked(p->outbuffer, cnt);

            (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                int                                            state;
                vtss_ifindex_t                                 ifindex;
                vtss_appl_psec_interface_status_t              port_status;
                vtss_appl_psec_interface_notification_status_t port_notif_status;

                if ((rc = vtss_ifindex_from_port(isid, pit.iport, &ifindex)) != VTSS_RC_OK) {
                    T_E("vtss_ifindex_from_port(%u, %u) failed (%s)", isid, pit.iport, error_txt(rc));
                    continue;
                }

                if ((rc = vtss_appl_psec_interface_conf_get(ifindex, &port_conf)) != VTSS_RC_OK) {
                    T_E("vtss_appl_psec_interface_conf_get(%u, %u) failed (%s)", isid, pit.iport, error_txt(rc));
                    continue;
                }

                if ((rc = vtss_appl_psec_interface_status_get(ifindex, &port_status)) != VTSS_RC_OK) {
                    T_E("vtss_appl_psec_interface_status_get(%u, %u) failed (%s)", isid, pit.iport, error_txt(rc));
                    continue;
                }

                if ((rc = vtss_appl_psec_interface_notification_status_get(ifindex, &port_notif_status)) != VTSS_RC_OK) {
                    T_E("vtss_appl_psec_interface_notification_status_get(%u, %u) failed (%s)", isid, pit.iport, error_txt(rc));
                    continue;
                }

                // Gotta combine the values from the PSEC module with the values from the PSEC LIMIT module.
                // The final value will be encoded as follows:
                //   0 = Disabled      (PSEC LIMIT disabled)
                //   1 = Ready         (PSEC LIMIT enabled, and port is not shut down, and limit is not reached. This can be reached in all modes).
                //   2 = Limit Reached (PSEC LIMIT enabled, port is not shutdown, but the limit is reached. This can be reached in all modes).
                //   3 = Shutdown      (PSEC LIMIT enabled, port is shutdown. This can only be reached if ViolationMode = Shutdown).
                // The PSEC module just returns its two flags Limit Reached and Shutdown. If PSEC LIMIT is not enabled, it should not be possible
                // to get a TRUE value out of these, since only the PSEC LIMIT module can have the PSEC module set these flags.
                // To determine whether to return Disabled or Ready, we need the current configuration of the PSEC LIMIT module.
                if (port_conf.enabled) {
                    if (port_notif_status.shut_down) {
                        state = 3; // Shutdown
                    } else if (port_status.limit_reached) {
                        state = 2; // Limit reached
                    } else {
                        state = 1; // Ready
                    }
                } else {
                    if (port_status.limit_reached || port_notif_status.shut_down) {
                        // As said, it shouldn't be possible to have a port in its limit_reached or shutdown state
                        // if PSEC LIMIT Control is not enabled on that port.
                        T_E("Internal error.");
                    }

                    state = 0; // Disabled
                }

                if (!pit.first) {
                    cyg_httpd_write_chunked("#", 1);
                }

                // [PortConfig]: PortNumber_1/PortEna_1/Limit_1/ViolateLimit_1/ViolationMode_1/Sticky_1/State_1#PortNumber_2/PortEna_2/ViolateLimit_2/Limit_2/ViolationMode_2/Sticky_2/State_2#...#PortNumber_N/PortEna_N/Limit_N/ViolateLimit_N/ViolationMode_N/Sticky_N/State_N
                cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%d/%u/%u/%d/%d/%d",
                               pit.uport,
                               port_conf.enabled,
                               port_conf.limit,
                               port_conf.violate_limit,
                               port_conf.violation_mode,
                               port_conf.sticky,
                               state);
                cyg_httpd_write_chunked(p->outbuffer, cnt);
            }
        }
        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/
static size_t psec_limit_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    vtss_appl_psec_capabilities_t   cap;
    vtss_appl_psec_global_conf_t    global_conf_defaults;
    vtss_appl_psec_interface_conf_t interface_conf_defaults;
    char                            buf[512];
    mesa_rc                         rc;

    if ((rc = vtss_appl_psec_capabilities_get(&cap)) != VTSS_RC_OK) {
        T_E("rc = %s", error_txt(rc));
        return 0;
    }

    if ((rc = vtss_appl_psec_global_conf_default_get(&global_conf_defaults)) != VTSS_RC_OK) {
        T_E("rc = %s", error_txt(rc));
        return 0;
    }

    if ((rc = vtss_appl_psec_interface_conf_default_get(&interface_conf_defaults)) != VTSS_RC_OK) {
        T_E("rc = %s", error_txt(rc));
        return 0;
    }

    buf[sizeof(buf) - 1] = '\0';
    (void)snprintf(buf, sizeof(buf) - 1,
                   "var psec_age_time_min      = %u;\n"
                   "var psec_age_time_max      = %u;\n"
                   "var psec_age_time_def      = %u;\n"
                   "var psec_hold_time_min     = %u;\n"
                   "var psec_hold_time_max     = %u;\n"
                   "var psec_hold_time_def     = %u;\n"
                   "var psec_limit_min         = %u;\n"
                   "var psec_limit_max         = %u;\n"
                   "var psec_limit_def         = %u;\n"
                   "var psec_violate_limit_min = %u;\n"
                   "var psec_violate_limit_max = %u;\n"
                   "var psec_violate_limit_def = %u;\n",
                   cap.age_time_min,
                   cap.age_time_max,
                   global_conf_defaults.aging_period_secs,
                   cap.hold_time_min,
                   cap.hold_time_max,
                   global_conf_defaults.hold_time_secs,
                   cap.limit_min,
                   cap.limit_max,
                   interface_conf_defaults.limit,
                   cap.violate_limit_min,
                   cap.violate_limit_max,
                   interface_conf_defaults.violate_limit);

    if (strlen(buf) == sizeof(buf) - 1) {
        T_E("Increase size of buffer");
    }

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buf);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/
web_lib_config_js_tab_entry(psec_limit_lib_config_js);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_psec_limit,        "/config/psec_limit",        handler_config_psec_limit);

