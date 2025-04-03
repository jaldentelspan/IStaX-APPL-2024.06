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

#include "icfg_api.h"
#include "misc_api.h"
#include "ip_dns_api.h"
#include "ip_dns_icfg.h"

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/
#define VTSS_TRACE_MODULE_ID        VTSS_MODULE_ID_IP_DNS
#define IP_DNS_ICFG_REG(x, y, z, w) (((x) = vtss_icfg_query_register((y), (z), (w))) == VTSS_RC_OK)

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/
/* ICFG callback functions */
static mesa_rc _ip_dns_icfg_state_ctrl(const vtss_icfg_query_request_t *req,
                                       vtss_icfg_query_result_t *result)
{
    mesa_rc                     rc = VTSS_RC_OK;
    BOOL                        dns_proxy;
    vtss_dns_srv_conf_t         dns_srv;
    vtss_dns_domainname_conf_t  my_dn;

    if (req && result) {
        u8  idx;
        /*
            COMMAND = ip name-server [ <0-2> ] { <ipv4_ucast> | { <ipv6_ucast> [ interface vlan <vlan_id> ] } | dhcp [ ipv4 | ipv6 ] [ interface vlan <vlan_id> ] }
            COMMAND = ip domain name { <domain_name> | dhcp [ ipv4 | ipv6 ] [ interface vlan <vlan_id> ] }
            COMMAND = ip dns proxy
        */
        for (idx = 0; idx < DNS_MAX_SRV_CNT; ++idx) {
            if (vtss_dns_mgmt_get_server(idx, &dns_srv) == VTSS_RC_OK) {
                char    buf[40];
                BOOL    pr_cfg;

                pr_cfg = FALSE;
                if (VTSS_DNS_TYPE_DEFAULT(&dns_srv)) {
                    if (req->all_defaults) {
                        pr_cfg = TRUE;
                    }
                } else {
                    pr_cfg = TRUE;
                }

                switch ( VTSS_DNS_TYPE_GET(&dns_srv) ) {
                case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY:
                    if (pr_cfg) {
                        rc = vtss_icfg_printf(result, "ip name-server %u dhcp ipv4\n", idx);
                    }
                    break;
                case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY:
                    if (pr_cfg) {
                        rc = vtss_icfg_printf(result, "ip name-server %u dhcp ipv6\n", idx);
                    }
                    break;
                case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
                    if (pr_cfg) {
                        if (dns_srv.static_conf_addr.type == MESA_IP_TYPE_IPV4) {
                            rc = vtss_icfg_printf(result, "ip name-server %u %s\n", idx,
                                                  misc_ipv4_txt(VTSS_DNS_ADDR_IPA4_GET(&dns_srv), buf));
                        } else if (dns_srv.static_conf_addr.type == MESA_IP_TYPE_IPV6) {
                            mesa_ipv6_t *ipa6 = &VTSS_DNS_ADDR_IPA6_GET(&dns_srv);

                            if (vtss_dns_mgmt_support_multicast()) {
                                rc = vtss_icfg_printf(result, "ip name-server %u %s interface vlan %u\n", idx,
                                                      misc_ipv6_txt(ipa6, buf), dns_srv.egress_vlan);
                            } else {
                                rc = vtss_icfg_printf(result, "ip name-server %u %s\n", idx,
                                                      misc_ipv6_txt(ipa6, buf));
                            }
                        }
                    }
                    break;
                case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
                    if (pr_cfg) {
                        rc = vtss_icfg_printf(result, "ip name-server %u dhcp ipv4 interface vlan %u\n",
                                              idx, VTSS_DNS_VLAN_GET(&dns_srv));
                    }
                    break;
                case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
                    if (pr_cfg) {
                        rc = vtss_icfg_printf(result, "ip name-server %u dhcp ipv6 interface vlan %u\n",
                                              idx, VTSS_DNS_VLAN_GET(&dns_srv));
                    }
                    break;
                case VTSS_APPL_DNS_CONFIG_TYPE_NONE:
                default:
                    if (pr_cfg) {
                        rc = vtss_icfg_printf(result, "no ip name-server %u\n", idx);
                    }
                    break;
                }
            } else {
                if (req->all_defaults) {
                    rc = vtss_icfg_printf(result, "no ip name-server %u\n", idx);
                }
            }
        }

        if (vtss_dns_mgmt_default_domainname_get(&my_dn) == VTSS_RC_OK) {
            if (req->all_defaults || my_dn.domain_name_type != VTSS_DNS_DOMAINAME_DEF_TYPE) {
                vtss_appl_dns_config_type_t dnt = my_dn.domain_name_type;

                switch ( dnt ) {
                case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
                    rc = vtss_icfg_printf(result, "ip domain name %s\n", my_dn.domain_name_char);
                    break;
                case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY:
                case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_ANY:
                    rc = vtss_icfg_printf(result, "ip domain name dhcp %s\n",
                                          dnt == VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_ANY ? "ipv4" : "ipv6");
                    break;
                case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
                case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
                    rc = vtss_icfg_printf(result, "ip domain name dhcp %s interface vlan %u\n",
                                          dnt == VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN ? "ipv4" : "ipv6",
                                          my_dn.domain_name_vlan);
                    break;
                default:
                    rc = vtss_icfg_printf(result, "no ip domain name\n");
                    break;
                }
            }
        } else {
            if (req->all_defaults) {
                rc = vtss_icfg_printf(result, "no ip domain name\n");
            }
        }

        if (vtss_dns_mgmt_get_proxy_status(&dns_proxy) == VTSS_RC_OK) {
            if (req->all_defaults ||
                (dns_proxy != VTSS_DNS_PROXY_DEF_STATE)) {
                rc = vtss_icfg_printf(result, "%sip dns proxy\n",
                                      dns_proxy ? "" : "no ");
            }
        } else {
            if (req->all_defaults) {
                rc = vtss_icfg_printf(result, "no ip dns proxy\n");
            }
        }
    }

    return rc;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/
/* Initialization function */
mesa_rc ip_dns_icfg_init(void)
{
    mesa_rc rc;

    /* Register callback functions to ICFG module. */
    if (IP_DNS_ICFG_REG(rc, VTSS_ICFG_IP_DNS_CONF, "dns", _ip_dns_icfg_state_ctrl)) {
        T_I("ip_dns ICFG done");
    }

    return rc;
}
