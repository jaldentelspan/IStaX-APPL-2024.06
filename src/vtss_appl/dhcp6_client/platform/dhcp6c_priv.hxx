/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef _DHCP6C_PRIV_HXX_
#define _DHCP6C_PRIV_HXX_

extern "C" {
#include "main_types.h"
#include <assert.h>
}

#define DHCP6_PKT_SZ_VAL            1516

#define DHCP6C_PRIV_RC_TW(x)        do { mesa_rc __dhcp6rc = (x); if (__dhcp6rc != VTSS_RC_OK) T_W("%s", error_txt(__dhcp6rc)); } while (0)
#define DHCP6C_PRIV_RC_TI(x)        do { mesa_rc __dhcp6rc = (x); if (__dhcp6rc != VTSS_RC_OK) T_I("%s", error_txt(__dhcp6rc)); } while (0)
#define DHCP6C_PRIV_RC_TD(x)        do { mesa_rc __dhcp6rc = (x); if (__dhcp6rc != VTSS_RC_OK) T_D("%s", error_txt(__dhcp6rc)); } while (0)
#define DHCP6C_PRIV_RC_TN(x)        do { mesa_rc __dhcp6rc = (x); if (__dhcp6rc != VTSS_RC_OK) T_N("%s", error_txt(__dhcp6rc)); } while (0)
#define DHCP6C_PRIV_TW_RETURN(x)    do { mesa_rc __dhcp6rc = (x); if (__dhcp6rc != VTSS_RC_OK) T_W("%s", error_txt(__dhcp6rc)); return __dhcp6rc; } while (0)
#define DHCP6C_PRIV_TI_RETURN(x)    do { mesa_rc __dhcp6rc = (x); if (__dhcp6rc != VTSS_RC_OK) T_I("%s", error_txt(__dhcp6rc)); return __dhcp6rc; } while (0)
#define DHCP6C_PRIV_TD_RETURN(x)    do { mesa_rc __dhcp6rc = (x); if (__dhcp6rc != VTSS_RC_OK) T_D("%s", error_txt(__dhcp6rc)); return __dhcp6rc; } while (0)
#define DHCP6C_PRIV_TN_RETURN(x)    do { mesa_rc __dhcp6rc = (x); if (__dhcp6rc != VTSS_RC_OK) T_N("%s", error_txt(__dhcp6rc)); return __dhcp6rc; } while (0)

namespace vtss
{
namespace dhcp6c
{
typedef mesa_vid_t                  dhcp6c_ifidx_t;

struct pkt_info_t {
    dhcp6c_ifidx_t                  ifidx;
    u32                             len;
    vtss_tick_count_t               tstamp;
};

mesa_rc frame_snd(dhcp6c_ifidx_t ifx, u32 len, u8 *const frm);

BOOL queue_snd(const u8 *const frm, const pkt_info_t *const info);
void queue_rtv(void);

namespace utils
{
BOOL device_mac_get(mesa_mac_t *const mac);
void device_mac_set(const mesa_mac_t *const mac);
void device_mac_clr(void);

BOOL system_mac_get(mesa_mac_t *const mac);
void system_mac_set(const mesa_mac_t *const mac);
void system_mac_clr(void);

mesa_rc convert_ip_to_mac(const mesa_ipv6_t *const ipa, mesa_mac_t *const mac);
mesa_rc eui64_linklocal_addr_get(mesa_ipv6_t *const ipv6_addr);
} /* utils */
} /* dhcp6c */
} /* vtss */
#endif /* _DHCP6C_PRIV_HXX_ */
