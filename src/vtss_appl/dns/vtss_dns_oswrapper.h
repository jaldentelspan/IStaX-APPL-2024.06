/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
/*---------------------------------------------------------------------------*/
/* DNS                                                                       */

#ifndef _VTSS_DNS_OSWRAPPER_H_
#define _VTSS_DNS_OSWRAPPER_H_

#include "vtss_hostaddr.h"

void vtss_dns_lib_initialize(void);

mesa_rc vtss_dns_lib_tick_cache(unsigned long tick);

void vtss_dns_lib_query_name_get(unsigned char *dns_header_ptr, unsigned char *current_ptr, unsigned char *name);

int vtss_dns_lib_build_qname(unsigned char *ptr, const char *hostname);

mesa_rc vtss_dns_lib_current_server_get(mesa_ip_addr_t *const srv, u8 *const idx, i32 *const ecnt);

mesa_rc vtss_dns_lib_current_server_set(mesa_vid_t vidx, const mesa_ip_addr_t *const srv, u8 idx);

mesa_rc vtss_dns_lib_current_server_rst(u8 idx);

mesa_rc vtss_dns_lib_default_domainname_get(char *const ns);

mesa_rc vtss_dns_lib_default_domainname_set(const char *const ns);

#endif /* _VTSS_DNS_OSWRAPPER_H_ */
