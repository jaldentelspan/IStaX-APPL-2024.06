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

#include "main.h"
#include "web_api.h"
#include "vtss_radius_api.h"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static i32 handler_stat_auth_radius_overview(CYG_HTTPD_STATE *p)
{
    vtss_radius_all_server_status_s server_status;
    int                             server, cnt;
    char                            encoded_host[3 * VTSS_RADIUS_HOST_LEN];

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    if (vtss_radius_auth_server_status_get(&server_status) == VTSS_RC_OK) {
        // Format: <server_state_1>#<server_state_2>#...#<server_state_N>
        // where <server_state_X> == ip_addr/port/state/dead_time_left_secs
        for (server = 0; server < ARRSZ(server_status.status); server++) {
            (void) cgi_escape(server_status.status[server].host, encoded_host);
            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s/%u/%d/%u",
                           (server != 0) ? "#" : "",
                           encoded_host,
                           server_status.status[server].port,
                           server_status.status[server].state,
                           server_status.status[server].dead_time_left_secs);
            cyg_httpd_write_chunked(p->outbuffer, cnt);
        }
    }

#ifdef VTSS_SW_OPTION_DOT1X_ACCT
    cyg_httpd_write_chunked("|", 1);

    if (vtss_radius_acct_server_status_get(&server_status) == VTSS_RC_OK) {
        // Format: <server_state_1>#<server_state_2>#...#<server_state_N>
        // where <server_state_X> == ip_addr/port/state/dead_time_left_secs
        for (server = 0; server < ARRSZ(server_status.status); server++) {
            (void) cgi_escape(server_status.status[server].host, encoded_host);
            cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s%s/%u/%d/%u",
                           (server != 0) ? "#" : "",
                           encoded_host,
                           server_status.status[server].port,
                           server_status.status[server].state,
                           server_status.status[server].dead_time_left_secs);
            cyg_httpd_write_chunked(p->outbuffer, cnt);
        }
    }
#endif

    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/*lint -sem(handler_stat_auth_radius_details, thread_protected) ... There is only one httpd thread */
static i32 handler_stat_auth_radius_details(CYG_HTTPD_STATE *p)
{
    vtss_radius_auth_client_server_mib_s mib_auth = {(vtss_radius_server_state_e)0};
#ifdef VTSS_SW_OPTION_DOT1X_ACCT
    vtss_radius_acct_client_server_mib_s mib_acct = {(vtss_radius_server_state_e)0};
#endif
    vtss_radius_all_server_status_s      server_status = {};
    int                                  cnt, server, errors = 0;
    char                                 encoded_host[3 * VTSS_RADIUS_HOST_LEN];

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY)) {
        return -1;
    }
#endif

    if (cyg_httpd_form_varable_int(p, "server", &server)) {
        server--;    // // convert from 1-based to 0-based
    } else {
        return -1;
    }
    if (server < 0 || server >= VTSS_RADIUS_NUMBER_OF_SERVERS) {
        return -1;
    }

    cyg_httpd_start_chunked("html");

    // This function is also used to clear the counters, when the URL contains the string "clear=1"
    if (!errors && cyg_httpd_form_varable_find(p, "clear")) {
        // If the port is MAC-based, this clears the summed up counters and all MAC-based state-machine counters.
        if (vtss_radius_auth_client_mib_clr(server) != VTSS_RC_OK) {
            errors++;
        }
#ifdef VTSS_SW_OPTION_DOT1X_ACCT
        if (vtss_radius_acct_client_mib_clr(server) != VTSS_RC_OK) {
            errors++;
        }
#endif
    }

    // Get the authentication statistics for this server
    if (!errors && vtss_radius_auth_client_mib_get(server, &mib_auth) != VTSS_RC_OK) {
        errors++;
    }

    // Get the authentication status
    if (!errors && vtss_radius_auth_server_status_get(&server_status) != VTSS_RC_OK) {
        errors++;
    }

    if (!errors) {
        (void) cgi_escape(server_status.status[server].host, encoded_host);
        // Format: server#ip_addr#udp_port#state#dead_time_left_secs#roundtriptime#cnt1#cnt2#...#cntN
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d#%s#%u#%d#%u#%u ms#%u#%u#%u#%u#%u#%u#%u#%u#%u#%u#%u#%u",
                       server + 1, // 1-based
                       encoded_host,
                       server_status.status[server].port,
                       server_status.status[server].state,
                       server_status.status[server].dead_time_left_secs,
                       mib_auth.radiusAuthClientExtRoundTripTime * 10, // It's measured in 1/100 seconds, but we report it in milliseconds.
                       mib_auth.radiusAuthClientExtAccessRequests,
                       mib_auth.radiusAuthClientExtAccessRetransmissions,
                       mib_auth.radiusAuthClientExtAccessAccepts,
                       mib_auth.radiusAuthClientExtAccessRejects,
                       mib_auth.radiusAuthClientExtAccessChallenges,
                       mib_auth.radiusAuthClientExtMalformedAccessResponses,
                       mib_auth.radiusAuthClientExtBadAuthenticators,
                       mib_auth.radiusAuthClientExtPendingRequests,
                       mib_auth.radiusAuthClientExtTimeouts,
                       mib_auth.radiusAuthClientExtUnknownTypes,
                       mib_auth.radiusAuthClientExtPacketsDropped,
                       mib_auth.radiusAuthClientCounterDiscontinuity);
        cyg_httpd_write_chunked(p->outbuffer, cnt);
    }

#ifdef VTSS_SW_OPTION_DOT1X_ACCT
    cyg_httpd_write_chunked("|", 1);

    // Get the accounting statistics for this server
    if (!errors && vtss_radius_acct_client_mib_get(server, &mib_acct) != VTSS_RC_OK) {
        errors++;
    }

    // Get the accounting status
    if (!errors && vtss_radius_acct_server_status_get(&server_status) != VTSS_RC_OK) {
        errors++;
    }

    if (!errors) {
        (void) cgi_escape(server_status.status[server].host, encoded_host);
        // Format: server#ip_addr#udp_port#state#dead_time_left_secs#roundtriptime#cnt1#cnt2#...#cntN
        cnt = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d#%s#%u#%d#%u#%u ms#%u#%u#%u#%u#%u#%u#%u#%u#%u#%u",
                       server + 1, // 1-based
                       encoded_host,
                       server_status.status[server].port,
                       server_status.status[server].state,
                       server_status.status[server].dead_time_left_secs,
                       mib_acct.radiusAccClientExtRoundTripTime * 10, // It's measured in 1/100 seconds, but we report it in milliseconds.
                       mib_acct.radiusAccClientExtRequests,
                       mib_acct.radiusAccClientExtRetransmissions,
                       mib_acct.radiusAccClientExtResponses,
                       mib_acct.radiusAccClientExtMalformedResponses,
                       mib_acct.radiusAccClientExtBadAuthenticators,
                       mib_acct.radiusAccClientExtPendingRequests,
                       mib_acct.radiusAccClientExtTimeouts,
                       mib_acct.radiusAccClientExtUnknownTypes,
                       mib_acct.radiusAccClientExtPacketsDropped,
                       mib_acct.radiusAccClientCounterDiscontinuity);
        cyg_httpd_write_chunked(p->outbuffer, cnt);
    }
#endif

    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_auth_radius_overview, "/stat/auth_status_radius_overview", handler_stat_auth_radius_overview);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_auth_radius_details, "/stat/auth_status_radius_details", handler_stat_auth_radius_details);

