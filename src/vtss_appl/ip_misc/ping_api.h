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

#ifndef _VTSS_PING_API_H_
#define _VTSS_PING_API_H_

#include "ip_utils.hxx"
#include "cli_io_api.h" /* For cli_iolayer_t */

#ifdef __cplusplus
extern "C" {
#endif

#define PING_MAX_CLIENT             8

/* Parameters valid range */
#define PING_MIN_PACKET_LEN         2   /* MUST-NOT be ZERO */
#define PING_MAX_PACKET_LEN         1452
#define PING_MIN_PACKET_CNT         1
#define PING_MAX_PACKET_CNT         60
#define PING_MIN_PACKET_INTERVAL    0
#define PING_MAX_PACKET_INTERVAL    30  /* When PING_MIN_PACKET_LEN < 2, Max.Allow is 2 */
#define PING_MIN_PAYLOAD_DATA       0
#define PING_MAX_PAYLOAD_DATA       255
#define PING_MIN_TTL                0
#define PING_MAX_TTL                255
#define PING_TIME_STAMP_BASE        ((1 << (8 * PING_MIN_PACKET_LEN)) - 1)
#define PING_TIME_STAMP_VALUE       (vtss_current_time() % PING_TIME_STAMP_BASE)

/* Default configuration */
#define PING_DEF_PACKET_LEN         56
#define PING_DEF_PACKET_CNT         5
#define PING_DEF_PACKET_PLDATA      0
#define PING_DEF_PACKET_INTERVAL    0

#define PING_SESSION_CTRL_C_WAIT    500 /* 500 msecs */
#define PING_SESSION_ID_IGNORE      IP_IO_SESSION_ID_IGNORE

/* Initialize module */
mesa_rc ping_init(vtss_init_data_t *data);

/*
 * Ping entry point - CLI version - using Linux tools
 */
BOOL ping_test(vtss_ip_cli_pr *pr, const char *ip_address, const char *src_address,
               mesa_vid_t src_vid, size_t count, size_t interval, size_t pktlen,
               size_t data, size_t ttl_value, bool is_verbose, bool exec_internally,
               bool is_web_client = FALSE);

#ifdef VTSS_SW_OPTION_IPV6
/* Ping v6 entry point - CLI version - using Linux tools */
BOOL ping6_test(vtss_ip_cli_pr *pr, const char *ipv6_address, const char *src_address,
                mesa_vid_t src_vid, size_t count, size_t interval, size_t pktlen,
                size_t data, bool is_verbose, bool exec_internally, bool is_web_client = FALSE);
#endif /* VTSS_SW_OPTION_IPV6 */

#ifdef VTSS_SW_OPTION_WEB
/* Main ping test - Web version */
/* Returns NULL on failure */
cli_iolayer_t *ping_test_async(const char *ip_address, const char *src_address, mesa_vid_t src_vid,
                               size_t count, size_t interval, size_t pktlen, size_t data, size_t ttl_value,
                               BOOL quiet);

#ifdef VTSS_SW_OPTION_IPV6
/* Main ping6 test - Web version
   return TRUE: success, FALSE: fail */
cli_iolayer_t *ping6_test_async(const char *ipv6_address, const char *src_address, mesa_vid_t src_vid,
                                size_t count, size_t interval, size_t pktlen, size_t data, BOOL quiet);
#endif /* VTSS_SW_OPTION_IPV6 */
#endif /* VTSS_SW_OPTION_WEB */

/* Main ping test - SNMP version */
BOOL ping_test_trap_server_exist(BOOL is_ipv6, const char *addr_str, i32 cnt);

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_PING_API_H_ */

