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
#include "control_api.h"
#ifdef VTSS_SW_OPTION_FIRMWARE
#include "firmware_api.h"
#endif
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_web_api.h"
#endif
#ifdef VTSS_SW_OPTION_ETH_LINK_OAM
#include "vtss_eth_link_oam_api.h"
#endif

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_MAIN
#include <vtss_trace_api.h>
/* ============== */

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static const char *wreset_status = "idle";
static i32 handler_wreset_status(CYG_HTTPD_STATE* p)
{
    cyg_httpd_start_chunked("html");
    cyg_httpd_write_chunked(wreset_status, strlen(wreset_status));
    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}


static i32 handler_misc(CYG_HTTPD_STATE* p)
{
    int rc = 0;



    mesa_restart_t restart_type = MESA_RESTART_COOL;


#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC))
        return -1;
#endif

    if(p->method == CYG_HTTPD_METHOD_POST) {
        if(cyg_httpd_form_varable_find(p, "warm")) {
            // "Yes" button on restart page clicked



            redirect(p, "/wreset_booting.htm");

            if(vtss_switch_standalone()) {
                wreset_status = "Restarting...";
                rc = control_system_reset(TRUE, VTSS_USID_ALL, restart_type);
            } else {
                wreset_status = "Restarting stack...";
                rc = control_system_reset(FALSE, VTSS_USID_ALL, restart_type);
            }
            if (rc) {
                wreset_status = "Restart failed! System flash is being updated.";
            }
        } else if(cyg_httpd_form_varable_find(p, "altimage")) {
            redirect(p, "/wreset_booting.htm");
            wreset_status = "Swapping images ... ";
#ifdef VTSS_SW_OPTION_FIRMWARE
            if(firmware_swap_images() == VTSS_RC_OK) {
                wreset_status = "Swapping images done, rebooting!";
                /* Now restart */
                if (vtss_switch_standalone()) {
                    (void) control_system_reset(TRUE, VTSS_USID_ALL, restart_type);
                } else {
                    (void) control_system_reset(FALSE, VTSS_USID_ALL, restart_type);
                }
            }else
                wreset_status = "Swapping images failed!";

#endif  /* VTSS_SW_OPTION_FIRMWARE */
        } else if(cyg_httpd_form_varable_find(p, "factory")) {
            redirect(p, "/factory_done.htm");
            control_config_reset(VTSS_USID_ALL, INIT_CMD_PARM2_FLAGS_IP);
        }
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        cyg_httpd_start_chunked("html");
        cyg_httpd_end_chunked();
    }
    return -1;
}

static i32 handler_wreset(CYG_HTTPD_STATE* p)
{
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MISC))
        return -1;
#endif
    if(p->method == CYG_HTTPD_METHOD_GET) {
        cyg_httpd_start_chunked("html");
        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(post_cb_misc, "/config/misc", handler_misc);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_wreset_status, "/config/wreset_status", handler_wreset_status);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_wreset, "/config/wreset", handler_wreset);

