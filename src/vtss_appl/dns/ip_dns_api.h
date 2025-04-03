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

/* debug msg can be enabled by cmd "debug trace module level dns default debug" */

#ifndef _VTSS_IP_DNS_API_H_
#define _VTSS_IP_DNS_API_H_

#include "vtss_dns.h"

/* IP DNS error codes (mesa_rc) */
typedef enum {
    IP_DNS_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_IP_DNS),  /* Generic error code */
} ip_dns_error_t;

#define VTSS_DNS_SERVER_DEF_TYPE    VTSS_APPL_DNS_CONFIG_TYPE_NONE
#define VTSS_DNS_TYPE_DEFAULT(x)    ((x)->dns_type == VTSS_DNS_SERVER_DEF_TYPE)
#define VTSS_DNS_DOMAINAME_DEF_TYPE VTSS_APPL_DNS_CONFIG_TYPE_NONE
#define VTSS_DNS_PROXY_DISABLE      0
#define VTSS_DNS_PROXY_ENABLE       1
#define VTSS_DNS_PROXY_DEF_STATE    VTSS_DNS_PROXY_DISABLE
#define VTSS_DNS_SUPPORT_MULTICAST  FALSE

#ifdef __cplusplus
extern "C" {
#endif

mesa_rc ip_dns_init(vtss_init_data_t *data);
const char *ip_dns_error_txt(ip_dns_error_t rc);

void vtss_dns_signal(void);
BOOL vtss_dns_mgmt_support_multicast(void);

mesa_rc vtss_dns_mgmt_get_proxy_status(BOOL *status);
mesa_rc vtss_dns_mgmt_set_proxy_status(BOOL *status);

#define VTSS_DNS_TYPE_PTR(x)        (&((x)->dns_type))
#define VTSS_DNS_ADDR_TYPE_PTR(x)   (&((x)->static_conf_addr.type))
#define VTSS_DNS_ADDR_IPA4_PTR(x)   (&((x)->static_conf_addr.addr.ipv4))
#define VTSS_DNS_ADDR_IPA6_PTR(x)   (&((x)->static_conf_addr.addr.ipv6))
#define VTSS_DNS_ADDR_PTR(x)        (&((x)->static_conf_addr))
#define VTSS_DNS_VLAN_PTR(x)        (&((x)->dynamic_conf_vlan))
#define VTSS_DNS_TYPE_GET(x)        ((x)->dns_type)
#define VTSS_DNS_ADDR_TYPE_GET(x)   ((x)->static_conf_addr.type)
#define VTSS_DNS_ADDR_IPA4_GET(x)   ((x)->static_conf_addr.addr.ipv4)
#define VTSS_DNS_ADDR_IPA6_GET(x)   ((x)->static_conf_addr.addr.ipv6)
#define VTSS_DNS_ADDR_GET(x)        ((x)->static_conf_addr)
#define VTSS_DNS_VLAN_GET(x)        ((x)->dynamic_conf_vlan)

#define VTSS_DNS_TYPE_SET(x, y)     do {VTSS_DNS_TYPE_GET((x)) = (y);} while (0)
#define VTSS_DNS_ADDR4_SET(x, y)    do {VTSS_DNS_ADDR_TYPE_GET((x)) = MESA_IP_TYPE_IPV4; VTSS_DNS_ADDR_IPA4_GET((x)) = (y);} while (0)
#define VTSS_DNS_ADDR6_SET(x, y)    do {VTSS_DNS_ADDR_TYPE_GET((x)) = MESA_IP_TYPE_IPV6; memcpy(VTSS_DNS_ADDR_IPA6_PTR((x)), (y), sizeof(mesa_ipv6_t));} while (0)
#define VTSS_DNS_ADDR_SET(x, y)     do {memcpy(VTSS_DNS_ADDR_PTR((x)), (y), sizeof(mesa_ip_addr_t));} while (0)
#define VTSS_DNS_VLAN_SET(x, y)     do {VTSS_DNS_VLAN_GET((x)) = (y);} while (0)

mesa_rc vtss_dns_mgmt_get_server(u8 idx, vtss_dns_srv_conf_t *dns_srv);
mesa_rc vtss_dns_mgmt_set_server(u8 idx, vtss_dns_srv_conf_t *dns_srv);
mesa_rc vtss_dns_mgmt_active_server_get(u8 *const idx, mesa_ip_addr_t *const srv);
mesa_rc vtss_dns_mgmt_default_domainname_get(vtss_dns_domainname_conf_t *const domain_name);
mesa_rc vtss_dns_mgmt_default_domainname_set(const vtss_dns_domainname_conf_t *const domain_name);

mesa_rc vtss_dns_mgmt_get_server4(mesa_ipv4_t *dns_srv);
#ifdef VTSS_SW_OPTION_IPV6
mesa_rc vtss_dns_mgmt_get_server6(mesa_ipv6_t *dns_srv);
#endif /* VTSS_SW_OPTION_IPV6 */

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_IP_DNS_API_H_ */

