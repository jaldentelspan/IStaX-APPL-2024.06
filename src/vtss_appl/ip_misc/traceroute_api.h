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

#ifndef _VTSS_TRACEROUTE_API_H
#define _VTSS_TRACEROUTE_API_H

#include "ip_utils.hxx"
#include "cli_io_api.h" /* For cli_iolayer_t */

#define TRACEROUTE_MAX_CLIENT               8

#define TRACEROUTE_DEF_EGRESS_INTF_VID      VTSS_VID_NULL
#define TRACEROUTE_DSCP_MIN                 0
#define TRACEROUTE_DSCP_MAX                 63
#define TRACEROUTE_TIMEOUT_MIN              1
#define TRACEROUTE_TIMEOUT_MAX              86400
#define TRACEROUTE_PROBES_MIN               1
#define TRACEROUTE_PROBES_MAX               60
#define TRACEROUTE_FTTL_MIN                 1
#define TRACEROUTE_FTTL_MAX                 30
#define TRACEROUTE_MTTL_MIN                 1
#define TRACEROUTE_MTTL_MAX                 255

/*
 * Initialize traceroute module
 */
mesa_rc traceroute_init(vtss_init_data_t *data);

/*
 * Start traceroute executable (IPv4)
 */
BOOL traceroute_test(vtss_ip_cli_pr *pr, const char *ip_address, const char *src_address,
                     mesa_vid_t src_vid, int dscp, int timeout, int probes, int firstttl, int maxttl,
                     BOOL icmp, BOOL numeric, BOOL is_web_client = FALSE);

#ifdef VTSS_SW_OPTION_IPV6
/*
 * Start traceroute executable (IPv6)
 */
BOOL traceroute6_test(vtss_ip_cli_pr *pr, const char *ip_address, const char *src_address,
                      mesa_vid_t src_vid, int dscp, int timeout, int probes, int maxttl,
                      BOOL numeric, BOOL is_web_client = FALSE);
#endif

#ifdef VTSS_SW_OPTION_WEB
/*
 * Traceroute test call-in from web
 */
cli_iolayer_t *traceroute_test_async(const char *ip_address, const char *src_address, mesa_vid_t src_vid,
                                     int dscp, int timeout, int probes, int firstttl, int maxttl,
                                     BOOL icmp, BOOL numeric);

#ifdef VTSS_SW_OPTION_IPV6
/*
 * Traceroute6 test call-in from web
 */
cli_iolayer_t *traceroute6_test_async(const char *ip_address, const char *src_address, mesa_vid_t src_vid,
                                      int dscp, int timeout, int probes, int maxttl, BOOL numeric);
#endif /* VTSS_SW_OPTION_IPV6 */
#endif /* VTSS_SW_OPTION_WEB */
#endif /* _VTSS_TRACEROUTE_API_H */

