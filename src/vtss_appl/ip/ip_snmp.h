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
#ifndef _IP_SNMP_H_
#define _IP_SNMP_H_

#ifdef VTSS_SW_OPTION_SNMP
#include "ip_api.h"

#ifdef __cplusplus
extern "C" {
#endif
mesa_rc vtss_ip_snmp_init(void);
void vtss_ip_snmp_signal_global_changes(void);
mesa_rc vtss_ip_interfaces_last_change(u64 *time);
mesa_rc vtss_ip_address_created_ipv4(vtss_ifindex_t if_id, u64 *time);
mesa_rc vtss_ip_address_changed_ipv4(vtss_ifindex_t if_id, u64 *time);
mesa_rc vtss_ip_address_created_ipv6(vtss_ifindex_t if_id, u64 *time);
mesa_rc vtss_ip_address_changed_ipv6(vtss_ifindex_t if_id, u64 *time);

#ifdef __cplusplus
}
#endif
#endif /* VTSS_SW_OPTION_SNMP */
#endif /* _IP_SNMP_H_ */

