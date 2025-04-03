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

#include <sstream>
#include <iomanip>
#include <sys/stat.h>              // For mkfifo

#include <vtss/basics/vector.hxx>
#include <vtss/basics/notifications/process-cmd-pty.hxx>

#include "main.h"
#include "vtss/appl/sysutil.h"
#include "ip_api.h"
#include "ip_misc_util.h"
#include "traceroute_api.h"

#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif /* VTSS_SW_OPTION_WEB */

#define TRACEROUTE4_EXECUTABLE                  "traceroute"
#define TRACEROUTE6_EXECUTABLE                  "traceroute6"

#define TRACEROUTE_THREAD_NAME_MAX    16    // Maximum thread name

// Safe print using provided output handle (which may be NULL)
#define PRINTF(...)                 \
    if(pr)                          \
        (void)(*pr)(__VA_ARGS__)


/*
 * Traceroute command parameters for web client thread
 */
typedef struct {
    char        ip[256];      /* Input arg - Dest. IP address */
    char        sip[256];     /* Input arg - Source IP address */
    int         dscp;
    int         timeout;
    int         probes;
    int         firstttl;
    int         maxttl;
    BOOL        icmp;
    BOOL        numeric;
    mesa_vid_t  vid;

    cli_iolayer_t *io;          /* Input arg - Traceroute output layer*/
#ifdef VTSS_SW_OPTION_IPV6
    int         ipv6_act;
#endif
    int         in_use;
} t_traceroute_req;

/* Protected by thread */
/*lint -esym(459, ping_thread_name, ping_thread_data, ping_thread_handle, traceroute_io) */
#ifdef VTSS_SW_OPTION_WEB
static char          traceroute_thread_name[TRACEROUTE_MAX_CLIENT][TRACEROUTE_THREAD_NAME_MAX];
static vtss_thread_t traceroute_thread_data[TRACEROUTE_MAX_CLIENT];
static vtss_handle_t traceroute_thread_handle[TRACEROUTE_MAX_CLIENT];

static t_traceroute_req traceroute_io[TRACEROUTE_MAX_CLIENT];
#endif /* VTSS_SW_OPTION_WEB */


/* ***************************************************************************
 * Debug trace definitions (don't confuse trace and traceroute)
 * ***************************************************************************
 */

#ifdef VTSS_TRACE_MODULE_ID
#undef VTSS_TRACE_MODULE_ID
#endif

#define VTSS_TRACE_MODULE_ID    VTSS_MODULE_ID_TRACEROUTE
#define VTSS_TRACE_GRP_DEFAULT  0

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "t_route", "Traceroute worker"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/* ***************************************************************************
 * Initialization functions
 * ***************************************************************************
 */

mesa_rc traceroute_init(vtss_init_data_t *data)
{
    return VTSS_RC_OK;
}

/* ***************************************************************************
 * Main entry functions
 * ***************************************************************************
 */

BOOL traceroute_test(vtss_ip_cli_pr *pr, const char *ip_address, const char *src_address,
                     mesa_vid_t src_vid, int dscp, int timeout, int probes, int firstttl,
                     int maxttl, BOOL icmp, BOOL numeric, BOOL is_web_client)
{
    char command[256];
    char *pCommand = command;
    if (!ip_address || strlen(ip_address) >= VTSS_APPL_SYSUTIL_DOMAIN_NAME_LEN) {
        return FALSE;
    }

    if (src_address != nullptr && strlen(src_address) > 0) {
        // Check that source address is actually used by a NID interface
        if (!vtss_ip_misc_is_src_address_used(src_address, MESA_IP_TYPE_IPV4))  {
            PRINTF("%% Error: Source IP Address is invalid or unknown!\n");
            return FALSE;
        }
    } else if (src_vid > 0) {
        // Check that the VLAN is actually configured as an IP interface
        if (!vtss_ip_misc_is_vid_ip_interface(src_vid)) {
            PRINTF("%% Error: Source VID not a valid interface!\n");
            return FALSE;
        }
    }

    // Build command for traceroute
    pCommand += sprintf(pCommand, TRACEROUTE4_EXECUTABLE);
    pCommand += sprintf(pCommand, " %s", ip_address);

    if (src_address != NULL && strlen(src_address) > 0) {
        pCommand += sprintf(pCommand, " -s %s", src_address);
    } else if (src_vid > 0) {
        pCommand += sprintf(pCommand, " -i %s%d", VTSS_APPL_IP_VLAN_IF_PREFIX, src_vid);
    }
    if (dscp >= 0) {
        // need to push the DSCP value up 2 bits since the two lower bits
        // are CU (currently unused)
        dscp = dscp << 2;
        pCommand += sprintf(pCommand, " -t %d", dscp);
    }
    if (timeout >= 0) {
        pCommand += sprintf(pCommand, " -w %d", timeout);
    }
    if (probes >= 0) {
        pCommand += sprintf(pCommand, " -q %d", probes);
    }
    if (firstttl >= 0) {
        pCommand += sprintf(pCommand, " -f %d", firstttl);
    }
    if (maxttl >= 0) {
        pCommand += sprintf(pCommand, " -m %d", maxttl);
    }
    if (icmp) {
        pCommand += sprintf(pCommand, " -I");
    }
    if (numeric) {
        pCommand += sprintf(pCommand, " -n");
    }

    T_D("Traceroute start: %s", command);

    // Call traceroute executable.
    mesa_rc rc = vtss::notifications::process_cmd_pty(cli_fd(), command);

    T_D("Traceroute finished: %s", command);

    if (is_web_client) {
        /*
         * The web client needs this to know when to stop polling for updates.
         * The traceroute executable does not emit a suitable "I'm done" message
         * as the ping executable do.
         */
        PRINTF("\nTraceroute session completed.\n");
    }

    return (rc == VTSS_RC_OK);
}

#ifdef VTSS_SW_OPTION_IPV6
/*
 * Start traceroute executable (IPv6)
 */
BOOL traceroute6_test(vtss_ip_cli_pr *pr, const char *ip_address, const char *src_address,
                      mesa_vid_t src_vid, int dscp, int timeout, int probes, int maxttl,
                      BOOL numeric, BOOL is_web_client)
{
    char command[256];
    char *pCommand = command;

    if (!ip_address || strlen(ip_address) >= VTSS_APPL_SYSUTIL_DOMAIN_NAME_LEN) {
        return FALSE;
    }

    // Check that source address is actually used by a NID interface
    if (src_address != nullptr && strlen(src_address) > 0) {
        if (!vtss_ip_misc_is_src_address_used(src_address, MESA_IP_TYPE_IPV6))  {
            PRINTF("%% Error: Source IP Address is invalid or unknown!\n");
            return FALSE;
        }
    }
    if (src_vid > 0) {
        // Check that the VLAN is actually configured as an IP interface
        if (!vtss_ip_misc_is_vid_ip_interface(src_vid)) {
            PRINTF("%% Error: Source VID not a valid interface!\n");
            return FALSE;
        }
    }

    // Build command for traceroute
    pCommand += sprintf(pCommand, TRACEROUTE6_EXECUTABLE);
    pCommand += sprintf(pCommand, " %s", ip_address);

    if (src_address != NULL && strlen(src_address) > 0) {
        pCommand += sprintf(pCommand, " -s %s", src_address);
        if (src_vid > 0) {
            pCommand += sprintf(pCommand, "%%%s%d", VTSS_APPL_IP_VLAN_IF_PREFIX, src_vid);
        }
    } else if (src_vid > 0) {
        pCommand += sprintf(pCommand, " -i %s%d",  VTSS_APPL_IP_VLAN_IF_PREFIX, src_vid);
    }
    if (dscp >= 0) {
        pCommand += sprintf(pCommand, " -t %d", dscp);
    }
    if (timeout >= 0) {
        pCommand += sprintf(pCommand, " -w %d", timeout);
    }
    if (probes >= 0) {
        pCommand += sprintf(pCommand, " -q %d", probes);
    }
    if (maxttl >= 0) {
        pCommand += sprintf(pCommand, " -m %d", maxttl);
    }
    if (numeric) {
        pCommand += sprintf(pCommand, " -n");
    }

    T_D("Traceroute6 start");

    // Call traceroute executable.
    mesa_rc rc = vtss::notifications::process_cmd_pty(cli_fd(), command);

    if (is_web_client) {
        /*
         * The web client needs this to know when to stop polling for updates.
         * The traceroute executable does not emit a suitable "I'm done" message
         * as the ping executable do.
         */
        PRINTF("\nTraceroute session completed.\n");
    }

    return (rc == VTSS_RC_OK);
}
#endif

/*
 * Traceroute test - Web version
 */
#ifdef VTSS_SW_OPTION_WEB
static void traceroute_thread(vtss_addrword_t data)
{
    t_traceroute_req *pReq = (t_traceroute_req *)data;
    cli_iolayer_t *pIO = pReq->io;

    // make it possible to use cli_printf
    web_set_cli(pReq->io);


#ifdef VTSS_SW_OPTION_IPV6
    if (pReq->ipv6_act) {
        (void)traceroute6_test(cli_printf, pReq->ip, pReq->sip, pReq->vid, pReq->dscp, pReq->timeout,
                               pReq->probes, pReq->maxttl, pReq->numeric, TRUE);
    } else
#endif /* VTSS_SW_OPTION_IPV6 */
    {
        (void)traceroute_test(cli_printf, pReq->ip, pReq->sip, pReq->vid, pReq->dscp, pReq->timeout,
                              pReq->probes, pReq->firstttl, pReq->maxttl, pReq->icmp,
                              pReq->numeric, TRUE);
    }

    /* Done the job, terminate thread */
    if (pIO->cli_close) {
        pIO->cli_close(pIO);
    }

    pReq->in_use = 0;
}

static cli_iolayer_t *traceroute_create_thread(const char *ip_address, const char *src_address,
                                               mesa_vid_t src_vid, int dscp, int timeout, int probes,
                                               int firstttl, int maxttl, BOOL icmp, BOOL numeric,
                                               BOOL is_ipv6)
{
    int i;
    cli_iolayer_t *io;

    T_D("Create traceroute thread %s", ip_address);

    // Allocate available traceroute IO resource
    for (i = 0 ; i < TRACEROUTE_MAX_CLIENT; i++) {
        if (!traceroute_io[i].in_use) {
            break;
        }
    }

    if (i == TRACEROUTE_MAX_CLIENT) {
        return NULL;
    }

    io = web_get_iolayer(WEB_CLI_IO_TYPE_PING);

    if (io == NULL) {
        return NULL;
    }

    // Fill ping IO data
    traceroute_io[i].io = io;
    strncpy(traceroute_io[i].ip, ip_address, sizeof(traceroute_io[i].ip) - 1);
    strncpy(traceroute_io[i].sip, src_address, sizeof(traceroute_io[i].sip) - 1);

    traceroute_io[i].dscp = dscp;
    traceroute_io[i].timeout = timeout;
    traceroute_io[i].probes = probes;
    traceroute_io[i].firstttl = firstttl;
    traceroute_io[i].maxttl = maxttl;
    traceroute_io[i].icmp = icmp;
    traceroute_io[i].numeric = numeric;
    traceroute_io[i].vid = src_vid;

#ifdef VTSS_SW_OPTION_IPV6
    traceroute_io[i].ipv6_act = is_ipv6;
#endif /* VTSS_SW_OPTION_IPV6 */
    traceroute_io[i].in_use = 1;

    sprintf(traceroute_thread_name[i], "TRout %01d", i + 1);

    // Create a thread, so we can run the scheduler and have time 'pass'
    vtss_thread_create(
        VTSS_THREAD_PRIO_DEFAULT,               // Priority
        traceroute_thread,                      // entry
        (vtss_addrword_t)&traceroute_io[i],     // entry parameter
        traceroute_thread_name[i],              // Name
        nullptr,                                // Stack
        0,                                      // Size
        &traceroute_thread_handle[i],           // Handle
        &traceroute_thread_data[i]              // Thread data structure
    );

    return io;
}

cli_iolayer_t *traceroute_test_async(const char *ip_address, const char *src_address, mesa_vid_t src_vid,
                                     int dscp, int timeout, int probes, int firstttl, int maxttl, BOOL icmp, BOOL numeric)
{
    return traceroute_create_thread(ip_address, src_address, src_vid, dscp, timeout, probes,
                                    firstttl, maxttl, icmp, numeric, FALSE);
}

#ifdef VTSS_SW_OPTION_IPV6
cli_iolayer_t *traceroute6_test_async(const char *ip_address, const char *src_address, mesa_vid_t src_vid,
                                      int dscp, int timeout, int probes, int maxttl, BOOL numeric)
{
    return traceroute_create_thread(ip_address, src_address, src_vid, dscp, timeout, probes,
                                    1, maxttl, FALSE, numeric, TRUE);
}

#endif /* VTSS_SW_OPTION_IPV6 */

#endif /* VTSS_SW_OPTION_WEB */
