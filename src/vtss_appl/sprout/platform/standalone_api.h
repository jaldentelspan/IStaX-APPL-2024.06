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

#ifndef _VTSS_STANDALONE_API_H_
#define _VTSS_STANDALONE_API_H_

#ifdef __cplusplus
extern "C" {
#endif
mesa_rc topo_isid2mac(const vtss_isid_t isid, mesa_mac_addr_t mac_addr);

/* Topology module functions replacements */
static __inline__ mesa_port_no_t topo_mac2port(const mesa_mac_addr_t mac_addr) __attribute__ ((const));
static __inline__ mesa_port_no_t topo_mac2port(const mesa_mac_addr_t mac_addr)
{
    return 0;
}

static __inline__ vtss_isid_t topo_usid2isid(const vtss_usid_t usid) __attribute__ ((const));
static __inline__ vtss_isid_t topo_usid2isid(const vtss_usid_t usid)
{
    return (vtss_isid_t) usid;
}

static __inline__ vtss_usid_t topo_isid2usid(const vtss_isid_t isid) __attribute__ ((const));
static __inline__ vtss_usid_t topo_isid2usid(const vtss_isid_t isid)
{
    return (vtss_usid_t) isid;
}

/* Initialize module */
mesa_rc standalone_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_STANDALONE_API_H_ */

