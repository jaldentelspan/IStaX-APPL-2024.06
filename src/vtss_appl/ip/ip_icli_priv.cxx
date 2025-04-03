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

#include "mgmt_api.h"
#include "misc_api.h"
#include "msg_api.h"
#include "ip_api.h"
#include "ip_utils.hxx"
#include "ip_icli_priv.h"
#include "icli_porting_util.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_IP
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_IP

void ip_icli_req_init(ip_icli_req_t *req, u32 session_id)
{
    memset(req, 0, sizeof(*req));
    req->session_id = session_id;
}

static void ip_icli_stats(u32 session_id, vtss_appl_ip_statistics_t *s)
{
    icli_cmd_stati(session_id, "Packets",    "",            s->HCInReceives,                                       s->HCOutRequests);
    icli_cmd_stati(session_id, "Octets",     "",            s->HCInOctets,                                         s->HCOutOctets);
    icli_cmd_stati(session_id, "Unicast",    "",            s->HCInReceives - s->HCInMcastPkts - s->HCInBcastPkts, s->HCOutRequests - s->HCOutMcastPkts - s->HCOutBcastPkts);
    icli_cmd_stati(session_id, "Multicast",  "",            s->HCInMcastPkts,                                      s->HCOutMcastPkts);
    icli_cmd_stati(session_id, "Broadcast",  "",            s->HCInBcastPkts,                                      s->HCOutBcastPkts);
    icli_cmd_stati(session_id, "Discards",   "",            s->InDiscards,                                         s->OutDiscards);
    icli_cmd_stati(session_id, "NoRoutes",   "",            s->InNoRoutes,                                         s->OutNoRoutes);
    icli_cmd_stati(session_id, "ReasmOKs",   "FragOKs",     s->ReasmOKs,                                           s->OutFragOKs);
    icli_cmd_stati(session_id, "ReasmReqds", "FragCreates", s->ReasmReqds,                                         s->OutFragCreates);
    icli_cmd_stati(session_id, "ReasmFails", "FragFails",   s->ReasmFails,                                         s->OutFragFails);
    icli_cmd_stati(session_id, "Delivers",   NULL,          s->InDelivers,                                         0);
    icli_cmd_stati(session_id, "HdrErrors",  NULL,          s->InHdrErrors,                                        0);
    icli_cmd_stati(session_id, "AddrErrors", NULL,          s->InAddrErrors,                                       0);
    icli_cmd_stati(session_id, "UnknProtos", NULL,          s->InUnknownProtos,                                    0);
    icli_cmd_stati(session_id, "Truncated",  NULL,          s->InTruncatedPkts,                                    0);
    ICLI_PRINTF("\n");
}

icli_rc_t ip_icli_stats_show(ip_icli_req_t *req)
{
    mesa_rc                   rc;
    vtss_appl_ip_statistics_t s;
    u32                       i, j, session_id = req->session_id;
    mesa_vid_t                vid;
    BOOL                      vid_valid[VTSS_VIDS];
    icli_unsigned_range_t     *list = req->vid_list;
    vtss_ifindex_t            ifidx;
    const char                *txt = (req->ipv4 ? "IPv4" : "IPv6");

    if (req->system || list == NULL) {
        // System counters
        if (req->ipv4) {
            rc = vtss_appl_ip_system_statistics_ipv4_get(&s);
        } else {
            rc = vtss_appl_ip_system_statistics_ipv6_get(&s);
        }

        if (rc != VTSS_RC_OK) {
            ICLI_PRINTF("Failed to get %s system statistics\n", txt);
            return ICLI_RC_ERROR;
        }

        ICLI_PRINTF("%s system statistics:\n", txt);
        ip_icli_stats(session_id, &s);
    }

    if (!req->system || list != NULL) {
        // Interface counters
        for (vid = 0; vid < VTSS_VIDS; vid++) {
            vid_valid[vid] = (list ? 0 : 1);
        }

        for (i = 0; list != NULL && i < list->cnt; i++) {
            for (j = list->range[i].min; j <= list->range[i].max; j++) {
                if (j < VTSS_VIDS) {
                    vid_valid[j] = 1;
                }
            }
        }

        for (vid = 1; vid < VTSS_VIDS; vid++) {
            if (vid_valid[vid] &&
                vtss_ifindex_from_vlan(vid, &ifidx) == VTSS_RC_OK) {
                if (req->ipv4) {
                    rc = vtss_appl_ip_if_statistics_ipv4_get(ifidx, &s);
                } else {
                    rc = vtss_appl_ip_if_statistics_ipv6_get(ifidx, &s);
                }

                if (rc == VTSS_RC_OK) {
                    ICLI_PRINTF("%s VLAN %u statistics:\n", txt, vid);
                    ip_icli_stats(session_id, &s);
                }
            }
        }
    }

    return ICLI_RC_OK;
}

icli_rc_t ip_icli_stats_clear(ip_icli_req_t *req)
{
    u32            session_id = req->session_id;
    mesa_vid_t     vid;
    vtss_ifindex_t ifidx;
    mesa_rc        rc;

    // System counters
    if (req->ipv4) {
        rc = vtss_appl_ip_system_statistics_ipv4_clear();
    } else {
        rc = vtss_appl_ip_system_statistics_ipv6_clear();
    }

    if (rc != VTSS_RC_OK) {
        ICLI_PRINTF("%% Failed to clear IP system statistics\n");
        return ICLI_RC_ERROR;
    }

    // Interface counters
    for (vid = 1; vid < VTSS_VIDS; vid++) {
        if (vtss_ifindex_from_vlan(vid, &ifidx) == VTSS_RC_OK) {
            if (req->ipv4) {
                (void)vtss_appl_ip_if_statistics_ipv4_clear(ifidx);
            } else {
                (void)vtss_appl_ip_if_statistics_ipv6_clear(ifidx);
            }
        }
    }

    return ICLI_RC_OK;
}

icli_rc_t ip_icli_if_stats_show(ip_icli_req_t *req)
{
    vtss_appl_ip_if_statistics_t s;
    u32                          i, j, session_id = req->session_id;
    mesa_vid_t                   vid;
    BOOL                         vid_valid[VTSS_VIDS];
    icli_unsigned_range_t        *list = req->vid_list;
    vtss_ifindex_t               ifidx;

    for (vid = 0; vid < VTSS_VIDS; vid++) {
        vid_valid[vid] = (list ? 0 : 1);
    }

    for (i = 0; list != NULL && i < list->cnt; i++) {
        for (j = list->range[i].min; j <= list->range[i].max; j++) {
            if (j < VTSS_VIDS) {
                vid_valid[j] = 1;
            }
        }
    }

    for (vid = 1; vid < VTSS_VIDS; vid++) {
        if (vid_valid[vid] &&
            vtss_ifindex_from_vlan(vid, &ifidx) == VTSS_RC_OK &&
            vtss_appl_ip_if_statistics_link_get(ifidx, &s) == VTSS_RC_OK) {
            ICLI_PRINTF("VLAN %u statistics:\n", vid);
            icli_cmd_stati(session_id, "Packets", "", s.in_packets, s.out_packets);
            icli_cmd_stati(session_id, "Octets", "", s.in_bytes, s.out_bytes);
            ICLI_PRINTF("\n");
        }
    }

    return ICLI_RC_OK;
}

icli_rc_t ip_icli_if_stats_clear(ip_icli_req_t *req)
{
    mesa_vid_t     vid;
    vtss_ifindex_t ifidx;

    for (vid = 1; vid < VTSS_VIDS; vid++) {
        if (vtss_ifindex_from_vlan(vid, &ifidx) == VTSS_RC_OK) {
            (void)vtss_appl_ip_if_statistics_link_clear(ifidx);
        }
    }

    return ICLI_RC_OK;
}

icli_rc_t ip_icli_acd_show(ip_icli_req_t *req)
{
    u32                                session_id = req->session_id;
    BOOL                               header = TRUE;
    vtss_appl_ip_acd_status_ipv4_key_t key, *in = NULL;
    vtss_appl_ip_acd_status_ipv4_t     status;
    vtss_ifindex_elm_t                 e_vid, e_port;
    char                               buf[32];

    while (vtss_appl_ip_acd_status_ipv4_itr(in, &key) == VTSS_RC_OK &&
           vtss_appl_ip_acd_status_ipv4_get(&key, &status) == VTSS_RC_OK &&
           vtss_ifindex_decompose(status.ifindex, &e_vid) == VTSS_RC_OK &&
           e_vid.iftype == VTSS_IFINDEX_TYPE_VLAN &&
           vtss_ifindex_decompose(status.ifindex_port, &e_port) == VTSS_RC_OK &&
           e_port.iftype == VTSS_IFINDEX_TYPE_PORT) {
        in = &key;
        if (header) {
            icli_table_header(session_id, "SIP              SMAC               VID   Interface");
            header = FALSE;
        }

        ICLI_PRINTF("%-17s", misc_ipv4_txt(key.sip, buf));
        ICLI_PRINTF("%-19s", misc_mac_txt(key.smac.addr, buf));
        ICLI_PRINTF("%-6u", e_vid.ordinal);
        icli_print_port_info_txt(session_id, VTSS_USID_START, iport2uport(e_port.ordinal));
        ICLI_PRINTF("\n");
    }

    return ICLI_RC_OK;
}

icli_rc_t ip_icli_acd_clear(ip_icli_req_t *req)
{
    u32 session_id = req->session_id;

    if (vtss_appl_ip_acd_status_ipv4_clear() != VTSS_RC_OK) {
        ICLI_PRINTF("ACD clear failed\n");
    }

    return ICLI_RC_OK;
}

