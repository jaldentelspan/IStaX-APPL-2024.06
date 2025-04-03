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
#include <vtss/basics/vector.hxx>
#include <vtss/basics/notifications/process-cmd-pty.hxx>
#include <sys/stat.h>            // For mkfifo

#include "main.h"
#include "ip_api.h"
#include "ip_misc_util.h"
#include "ping.h"
#include "ping_api.h"
#include "vtss/appl/sysutil.h"

#ifdef VTSS_SW_OPTION_WEB
#include "web_api.h"
#endif /* VTSS_SW_OPTION_WEB */

#define PING4_EXECUTABLE                "ping"
#define PING6_EXECUTABLE                "ping6"

/* Thread variables */
#define PING_THREAD_NAME_MAX    16      /* Maximum thread name */
#define PING_DELAY_CONSECUTIVE  50      /* Delay between consecutive ping requests */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_PING

// Safe print using provided output handle (which may be NULL)
#define PRINTF(...)                 \
    if(pr)                          \
        (void)(*pr)(__VA_ARGS__)

/* Protected by thread */
/*lint -esym(459, ping_thread_name, ping_thread_data, ping_thread_handle) */
#ifdef VTSS_SW_OPTION_WEB
static char          ping_thread_name[PING_MAX_CLIENT][PING_THREAD_NAME_MAX];
static vtss_thread_t ping_thread_data[PING_MAX_CLIENT];
static vtss_handle_t ping_thread_handle[PING_MAX_CLIENT];
#endif /* VTSS_SW_OPTION_WEB */

typedef struct {
    char          ip[256];      /* Input arg - Dest. IP address */
    char          sip[256];     /* Input arg - Source IP address */
    size_t        len;          /* Input arg - len of ping */
    size_t        cnt;          /* Input arg - cnt of ping */
    size_t        interval;     /* Input arg - interval of ping (seconds) */
    size_t        data;         /* Input arg - data payload of ping */
    size_t        ttl;          /* Input arg - ttl payload of ping */
    mesa_vid_t    vid;          /* Input arg - source vlan interface */
    BOOL          quiet;        /* Input arg - only print results */
    cli_iolayer_t *io;          /* Input arg - Ping output layer*/
#ifdef VTSS_SW_OPTION_IPV6
    int           ipv6_act;
#endif
    int           in_use;
} t_ping_req;

/* Protected by thread */
/*lint -esym(459, ping_io) */
#ifdef VTSS_SW_OPTION_WEB
static t_ping_req ping_io[PING_MAX_CLIENT];
#endif /* VTSS_SW_OPTION_WEB */

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "ping", "Ping worker"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/



BOOL ping_test(vtss_ip_cli_pr *pr, const char *ip_address, const char *src_address, mesa_vid_t src_vid,
               size_t count, size_t interval, size_t pktlen, size_t data, size_t ttl_value,
               bool is_verbose, bool exec_internally, bool is_web_client)
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

    pCommand += sprintf(pCommand, PING4_EXECUTABLE);
    pCommand += sprintf(pCommand, " -4");
    pCommand += sprintf(pCommand, " %s", ip_address);


    if (src_address != NULL && strlen(src_address) > 0) {
        pCommand += sprintf(pCommand, " -I %s", src_address);
    } else if (src_vid > 0) {
        pCommand += sprintf(pCommand, " -I %s%d", VTSS_APPL_IP_VLAN_IF_PREFIX, src_vid);
    }
    if (count > 0) {
        pCommand += sprintf(pCommand, " -c %zd", count);
    }
    if (pktlen != 0) {
        pCommand += sprintf(pCommand, " -s %zd", pktlen);
    }
    if (data != PING_DEF_PACKET_PLDATA) {
        pCommand += sprintf(pCommand, " -p %02zx", data);
    }
    if (ttl_value > 0 && ttl_value <= 255) {
        pCommand += sprintf(pCommand, " -t %zd", ttl_value);
    }

    if (!is_verbose) {
        pCommand += sprintf(pCommand, " -q");
    }

    T_D("Ping start (is_web_client=%d, is_verbose=%d) ", is_web_client, is_verbose);
    if (is_web_client && !is_verbose) {
        /*
         * The web client needs to see that something is happening.
         */
        PRINTF("\nPing session started ...\n");
    }

    T_D("ping command: %s", command);
    mesa_rc rc = vtss::notifications::process_cmd_pty(cli_fd(), command);

    if (is_web_client) {
        /*
         * The web client needs this to know when to stop polling for updates.
         */
        PRINTF("\nPing session completed.\n");
    }

    return (rc == VTSS_RC_OK);
}

#ifdef VTSS_SW_OPTION_IPV6

BOOL ping6_test(vtss_ip_cli_pr *pr, const char *ipv6_address, const char *src_address, mesa_vid_t src_vid,
                size_t count, size_t interval, size_t pktlen, size_t data, bool is_verbose, bool exec_internally, bool is_web_client)
{
    char command[256];
    char *pCommand = command;

    if (!ipv6_address || strlen(ipv6_address) > VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN) {
        PRINTF("%% Failed to perform PING6 operation!\n");
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

    vtss::Vector<std::string> arguments;

    pCommand += sprintf(pCommand, PING6_EXECUTABLE);
    pCommand += sprintf(pCommand, " %s", ipv6_address);

    if (src_address != NULL && strlen(src_address) > 0) {
        pCommand += sprintf(pCommand, " -I %s", src_address);
        if (src_vid > 0) {
            // Add address scope
            pCommand += sprintf(pCommand, "%%%s%d", VTSS_APPL_IP_VLAN_IF_PREFIX, src_vid);
        }

    } else if (src_vid > 0) {
        pCommand += sprintf(pCommand, " -I %s%d", VTSS_APPL_IP_VLAN_IF_PREFIX, src_vid);
    }
    if (count > 0) {
        pCommand += sprintf(pCommand, " -c %zd", count);
    }
    if (pktlen != 0) {
        pCommand += sprintf(pCommand, " -s %zd", pktlen);
    }
    if (data != PING_DEF_PACKET_PLDATA) {
        pCommand += sprintf(pCommand, " -p %02zx", data);
    }

    if (!is_verbose) {
        pCommand += sprintf(pCommand, " -q");
    }

    T_D("Ping6 start");

    if (is_web_client && !is_verbose) {
        /*
         * The web client needs to see that something is happening.
         */
        PRINTF("\nPing session started ...\n");
    }

    T_D("ping command: %s", command);
    mesa_rc rc = vtss::notifications::process_cmd_pty(cli_fd(), command);

    if (is_web_client) {
        /*
         * The web client needs this to know when to stop polling for updates.
         */
        PRINTF("\nPing session completed.\n");
    }

    return (rc == VTSS_RC_OK);
}
#endif /* VTSS_SW_OPTION_IPV6 */

#ifdef VTSS_SW_OPTION_WEB
static void ping_thread(vtss_addrword_t data)
{
    t_ping_req *pReq = (t_ping_req *)data;
    cli_iolayer_t *pIO = pReq->io;

    T_D("begin");
    // make it possible to use cli_printf
    web_set_cli(pReq->io);

#ifdef VTSS_SW_OPTION_IPV6
    if (pReq->ipv6_act) {
        (void)ping6_test(cli_printf, pReq->ip, pReq->sip, pReq->vid, pReq->cnt, pReq->interval,
                         pReq->len, pReq->data, !pReq->quiet, FALSE, TRUE);
    } else
#endif /* VTSS_SW_OPTION_IPV6 */
    {
        (void)ping_test(cli_printf, pReq->ip, pReq->sip, pReq->vid, pReq->cnt, pReq->interval,
                        pReq->len, pReq->data, pReq->ttl, !pReq->quiet, FALSE, TRUE);
    }

    /* Done the job, terminate thread */
    if (pIO->cli_close) {
        pIO->cli_close(pIO);
    }

    pReq->in_use = 0;
}
#endif /* VTSS_SW_OPTION_WEB */

#ifdef VTSS_SW_OPTION_WEB
static cli_iolayer_t *ping_create_thread(const char *ip_address, const char *src_address,
                                         mesa_vid_t src_vid,
                                         size_t count, size_t interval,
                                         size_t pktlen, size_t data, size_t ttl_value,
                                         BOOL quiet, BOOL is_ipv6)
{
    int i;
    cli_iolayer_t *io;

    T_D("Create ping thread %s", ip_address);

    // Allocate available ping IO resource
    for (i = 0 ; i < PING_MAX_CLIENT; i++) {
        if (!ping_io[i].in_use) {
            break;
        }
    }

    if (i == PING_MAX_CLIENT) {
        return NULL;
    }

    io = web_get_iolayer(WEB_CLI_IO_TYPE_PING);

    if (io == NULL) {
        T_E("NO iolayer?!?!");
        return NULL;
    }

    // Fill ping IO data
    ping_io[i].io = io;
    strncpy(ping_io[i].ip, ip_address, sizeof(ping_io[i].ip) - 1);
    strncpy(ping_io[i].sip, src_address, sizeof(ping_io[i].sip) - 1);
    ping_io[i].len = pktlen;
    ping_io[i].cnt = count;
    ping_io[i].interval = interval;
    ping_io[i].data = data;
    ping_io[i].ttl = ttl_value;
    ping_io[i].vid = src_vid;
    ping_io[i].quiet = quiet;
#ifdef VTSS_SW_OPTION_IPV6
    ping_io[i].ipv6_act = is_ipv6;
#endif /* VTSS_SW_OPTION_IPV6 */
    ping_io[i].in_use = 1;

    sprintf(ping_thread_name[i], "Ping %01d", i + 1);

    // Create a thread, so we can run the scheduler and have time 'pass'
    vtss_thread_create(
        VTSS_THREAD_PRIO_DEFAULT,               // Priority
        ping_thread,                            // entry
        (vtss_addrword_t)&ping_io[i],           // entry parameter
        ping_thread_name[i],                    // Name
        nullptr,                                // Stack
        0,                                      // Size
        &ping_thread_handle[i],                 // Handle
        &ping_thread_data[i]                    // Thread data structure
    );

    return io;
}
#endif /* VTSS_SW_OPTION_WEB */

/****************************************************************************/
/*  Initialization functions                                                */
/****************************************************************************/

mesa_rc ping_init(vtss_init_data_t *data)
{
    return VTSS_RC_OK;
}

/* Main ping6 test - Web version */
#if defined(VTSS_SW_OPTION_IPV6) && defined(VTSS_SW_OPTION_WEB)
cli_iolayer_t *ping6_test_async(const char *ipv6_address, const char *src_address, mesa_vid_t src_vid,
                                size_t count, size_t interval, size_t pktlen, size_t data, BOOL quiet)
{
    return ping_create_thread(ipv6_address, src_address, src_vid, count, interval, pktlen, data, 0, quiet, TRUE);
}
#endif /* defined(VTSS_SW_OPTION_IPV6) && defined(VTSS_SW_OPTION_WEB) */

/* Main ping test - Web version */
#ifdef VTSS_SW_OPTION_WEB
cli_iolayer_t *ping_test_async(const char *ip_address, const char *src_address, mesa_vid_t src_vid,
                               size_t count, size_t interval, size_t pktlen, size_t data, size_t ttl_value,
                               BOOL quiet)
{
    return ping_create_thread(ip_address, src_address, src_vid, count, interval, pktlen, data, ttl_value, quiet, FALSE);
}
#endif /* VTSS_SW_OPTION_WEB */

BOOL ping_test_trap_server_exist(BOOL is_ipv6, const char *addr_str, i32 cnt)
{
    if (is_ipv6) {
#ifdef VTSS_SW_OPTION_IPV6
        return ping6_test(NULL, addr_str, NULL, VTSS_VID_NULL, cnt, PING_DEF_PACKET_INTERVAL, PING_DEF_PACKET_LEN, 0, TRUE, TRUE);
#else
        return FALSE;
#endif /* VTSS_SW_OPTION_IPV6 */
    } else {
        return ping_test(NULL, addr_str, NULL, VTSS_VID_NULL, cnt, PING_DEF_PACKET_INTERVAL, PING_DEF_PACKET_LEN, 0, 0, TRUE, TRUE);
    }
}

