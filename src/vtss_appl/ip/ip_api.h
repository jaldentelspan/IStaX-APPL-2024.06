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

#ifndef _IP_API_H_
#define _IP_API_H_

#include <vtss/appl/ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Feature config ---------------------------------------------------------- */

static __inline__ bool vtss_ip_hasipv6(void) __attribute__ ((const));
static __inline__ bool vtss_ip_hasipv6(void)
{
#if defined(VTSS_SW_OPTION_IPV6)
    return true;
#else
    return false;
#endif
}

static __inline__ bool vtss_ip_hasrouting(void) __attribute__ ((const));
static __inline__ bool vtss_ip_hasrouting(void)
{
#if defined(VTSS_SW_OPTION_L3RT)
    return true;
#else
    return false;
#endif
}

static __inline__ bool vtss_ip_hasdns(void) __attribute__ ((const));
static __inline__ bool vtss_ip_hasdns(void)
{
#if defined(VTSS_SW_OPTION_DNS)
    return true;
#else
    return false;
#endif
}

static __inline__ bool vtss_ip_hasdhcpv6(void) __attribute__ ((const));
static __inline__ bool vtss_ip_hasdhcpv6(void)
{
#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)
    return true;
#else
    return false;
#endif
}

/* Interface functions ----------------------------------------------------- */
/*
    This function is used to validate the input network (n) of an existing IP interface (if_id).
    It mainly checks network overlapping w.r.t the given interface's network in our system.
*/
bool vtss_ip_if_address_valid(mesa_vid_t vlan, const mesa_ip_network_t *n);

/* Gets the exactly matched IPv6 status entry for a given index */
//mesa_rc vtss_appl_ip_if_status_get_ipv6(const vtss_appl_ip_if_ipv6_status_key_t *const index,
//                                        vtss_appl_ip_if_status_t                *const status);

/* Provide post-notifications for other modules when an interface has
 * changed. No delete function. */
typedef void (*vtss_ip_if_callback_t)(vtss_ifindex_t         if_id);
mesa_rc vtss_ip_if_callback_add(const vtss_ip_if_callback_t cb);

/* IP address functions ---------------------------------------------------- */

mesa_rc ip_dhcp6c_add(mesa_vid_t vid, const mesa_ipv6_network_t *network, uint64_t lt_valid);
mesa_rc ip_dhcp6c_del(mesa_vid_t vid, const mesa_ipv6_network_t *network);

/* Module initialization --------------------------------------------------- */
mesa_rc vtss_ip_init(vtss_init_data_t *data);

void vtss_ip_if_mo_flag_update(mesa_vid_t vid);
void vtss_ip_vlan_state_update(bool up[MESA_VIDS]);

const char *ip_error_txt(mesa_rc rc);

#ifdef __cplusplus
}
#endif

#endif /* _IP_API_H_ */

