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

#ifndef _IP_ICLI_PRIV_H_
#define _IP_ICLI_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "icli_api.h"

#ifdef VTSS_SW_OPTION_ICFG
#define IP_ICFG_DEF_ROUTING4   TRUE
#define IP_ICFG_DEF_ROUTING6   TRUE
#endif /* VTSS_SW_OPTION_ICFG */

/* IP ICLI request structure */
typedef struct {
    u32                   session_id;
    BOOL                  ipv4;
    BOOL                  system;
    icli_unsigned_range_t *vid_list;
} ip_icli_req_t;

void ip_icli_req_init(ip_icli_req_t *req, u32 session_id);
icli_rc_t ip_icli_stats_show(ip_icli_req_t *req);
icli_rc_t ip_icli_stats_clear(ip_icli_req_t *req);
icli_rc_t ip_icli_if_stats_show(ip_icli_req_t *req);
icli_rc_t ip_icli_if_stats_clear(ip_icli_req_t *req);
icli_rc_t ip_icli_acd_show(ip_icli_req_t *req);
icli_rc_t ip_icli_acd_clear(ip_icli_req_t *req);

#ifdef __cplusplus
}
#endif

#endif /* _IP_ICLI_PRIV_H_ */

