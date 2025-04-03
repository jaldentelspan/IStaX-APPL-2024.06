/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "web_api.h"
#include "ip_api.h"
#include "ip_utils.hxx"
#include "ping_api.h"
#include "icli_api.h"
#include "traceroute_api.h"
#include "mgmt_api.h"
#include "vlan_api.h"

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#ifdef VTSS_SW_OPTION_DNS
#include "vtss/appl/dns.h"
#endif /* VTSS_SW_OPTION_DNS */
#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)
#include "vtss/appl/dhcp6_client.h"
#endif /* defined(VTSS_SW_OPTION_DHCP6_CLIENT) */
#include "vtss/basics/trace.hxx"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_IP

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

/******************************************************************************/
// IP_WEB_ip_by_port()
/******************************************************************************/
static bool IP_WEB_ip_by_port(mesa_port_no_t iport, mesa_ip_type_t type, mesa_ip_addr_t *addr)
{
    vtss_appl_vlan_port_conf_t vlan_conf;
    vtss_appl_ip_if_key_ipv4_t ipv4_key = {};
    vtss_appl_ip_if_key_ipv6_t ipv6_key = {};
    vtss_ifindex_t             ifindex;

    // Get PVID
    if (vlan_mgmt_port_conf_get(VTSS_ISID_LOCAL, iport, &vlan_conf, VTSS_APPL_VLAN_USER_ALL, true) != VTSS_RC_OK) {
        return false;
    }

    if (vtss_ifindex_from_vlan(vlan_conf.hybrid.pvid, &ifindex) != VTSS_RC_OK) {
        return false;
    }

    // Get the IP for the VLAN with the PVID
    if (type == MESA_IP_TYPE_IPV4) {
        ipv4_key.ifindex = ifindex;

        if (vtss_appl_ip_if_status_ipv4_itr(&ipv4_key, &ipv4_key) != VTSS_RC_OK || ipv4_key.ifindex != ifindex) {
            return false;
        }

        addr->type      = MESA_IP_TYPE_IPV4;
        addr->addr.ipv4 = ipv4_key.addr.address;
    } else if (type == MESA_IP_TYPE_IPV6) {
        if (vtss_appl_ip_if_status_ipv6_itr(&ipv6_key, &ipv6_key) != VTSS_RC_OK || ipv6_key.ifindex != ifindex) {
            return false;
        }

        addr->type       = MESA_IP_TYPE_IPV6;
        addr->addr.ipv6 = ipv6_key.addr.address;
    } else {
        return false;
    }

    return true;
}

/******************************************************************************/
// rt4_parse()
/******************************************************************************/
static bool rt4_parse(CYG_HTTPD_STATE *p, vtss_appl_ip_route_key_t *rt, vtss_appl_ip_route_conf_t *conf, int idx)
{
    int        prefix = 0, distance = 0;
    char       name[20];
    const char *buf;
    size_t     len;

    vtss_clear(*rt);
    vtss_clear(*conf);

    sprintf(name, "rt_dest_%d", idx);
    if ((buf = cyg_httpd_form_varable_string(p, name, &len)) == nullptr) {
        return false;
    }

    // Handle the destination separately, because it may contain both the
    // literal "blackhole" and an IPv4 address. If neither, then this is not an
    // IPv4 route.
    // The following strncmp() makes it possible just to say e.g. "blackh", but
    // the Web page won't allow you.
    if (strncmp(buf, "blackhole", len) == 0) {
        rt->route.ipv4_uc.destination = vtss_ipv4_blackhole_route;
    } else if (!cyg_httpd_form_varable_ipv4(p, name, &rt->route.ipv4_uc.destination)) {
        return false;
    }

    if (cyg_httpd_form_variable_ipv4_fmt(p, &rt->route.ipv4_uc.network.address, "rt_net_%d",  idx) &&
        cyg_httpd_form_variable_int_fmt( p, &prefix,                            "rt_mask_%d", idx) && prefix   >= 0 && prefix   <= 32 &&
        cyg_httpd_form_variable_int_fmt( p, &distance,                          "rt_dist_%d", idx) && distance >= 1 && distance <= 255) {
        rt->type                              = VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC;
        conf->distance                        = distance;
        rt->route.ipv4_uc.network.prefix_size = prefix;
        return true;
    }

    return false;
}

/******************************************************************************/
// rt6_parse()
/******************************************************************************/
static bool rt6_parse(CYG_HTTPD_STATE *p, vtss_appl_ip_route_key_t *rt, vtss_appl_ip_route_conf_t *conf, int idx)
{
    int        prefix = 0, next_hop_vlan = 0, distance = 0;
    char       name[20];
    const char *buf;
    size_t     len;

    vtss_clear(*rt);
    vtss_clear(*conf);

    if (!vtss_ip_hasipv6()) {
        return false;
    }

    sprintf(name, "rt_dest_%d", idx);
    if ((buf = cyg_httpd_form_varable_string(p, name, &len)) == nullptr) {
        return false;
    }

    // Handle the destination separately, because it may contain both the
    // literal "blackhole" and an IPv6 address. If neither, then this is not an
    // IPv6 route.
    // The following strncmp() makes it possible just to say e.g. "blackh", but
    // the Web page won't allow you.
    if (strncmp(buf, "blackhole", len) == 0) {
        rt->route.ipv6_uc.destination = vtss_ipv6_blackhole_route;
    } else if (!cyg_httpd_form_varable_ipv6(p, name, &rt->route.ipv6_uc.destination)) {
        return false;
    }

    if (cyg_httpd_form_variable_ipv6_fmt(p, &rt->route.ipv6_uc.network.address, "rt_net_%d",  idx) &&
        cyg_httpd_form_variable_int_fmt( p, &prefix,                            "rt_mask_%d", idx) && prefix   >= 0 && prefix   <= 128 &&
        cyg_httpd_form_variable_int_fmt( p, &distance,                          "rt_dist_%d", idx) && distance >= 1 && distance <= 255) {

        rt->type                              = VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC;
        rt->route.ipv6_uc.network.prefix_size = prefix;
        conf->distance                        = distance;

        if (vtss_ipv6_addr_is_link_local(&rt->route.ipv6_uc.destination) &&
            cyg_httpd_form_variable_int_fmt(p, &next_hop_vlan, "rt_nhvid_%d", idx) && next_hop_vlan > 0) {
            next_hop_vlan = next_hop_vlan & 0xFFFF;
            (void)vtss_ifindex_from_vlan(next_hop_vlan, &rt->vlan_ifindex);
        } else {
            rt->vlan_ifindex = VTSS_IFINDEX_NONE;
        }

        return true;
    }

    return false;
}

static bool ip2_ipv4_config_differs(const vtss_appl_ip_if_conf_ipv4_t *ipc_old,
                                    const vtss_appl_ip_if_conf_ipv4_t *ipc_new)
{
    return memcmp(ipc_old, ipc_new, sizeof(vtss_appl_ip_if_conf_ipv4_t)) != 0;
}

static bool ip2_ipv6_config_differs(const vtss_appl_ip_if_conf_ipv6_t *ipc_old,
                                    const vtss_appl_ip_if_conf_ipv6_t *ipc_new)
{
    return memcmp(ipc_old, ipc_new, sizeof(vtss_appl_ip_if_conf_ipv6_t)) != 0;
}

static BOOL net_parse(CYG_HTTPD_STATE *p,
                      mesa_ip_type_t type,
                      mesa_ip_network_t *n,
                      int ix)
{
    int prefix = 0;
    memset(n, 0, sizeof(*n));
    if (type == MESA_IP_TYPE_IPV4 &&
        cyg_httpd_form_variable_ipv4_fmt(p, &n->address.addr.ipv4, "if_addr_%d", ix) &&
        cyg_httpd_form_variable_int_fmt( p, &prefix,               "if_mask_%d", ix) && prefix >= 0 && prefix <= 32) {
        n->prefix_size = prefix;
        n->address.type = type;
        return TRUE;
    }

    if (type == MESA_IP_TYPE_IPV6 &&
        cyg_httpd_form_variable_ipv6_fmt(p, &n->address.addr.ipv6, "if_addr6_%d", ix) &&
        cyg_httpd_form_variable_int_fmt( p, &prefix,               "if_mask6_%d", ix) && prefix >= 0 && prefix <= 128) {
        n->prefix_size = prefix;
        n->address.type = type;
        return TRUE;
    }

    return FALSE;
}

typedef struct {
    vtss_ifindex_t              ifidx;
    BOOL                        change_ipv4, change_ipv6;
    vtss_appl_ip_if_conf_ipv4_t ipv4;
    vtss_appl_ip_if_conf_ipv6_t ipv6;
} ip_web_conf_t;

/******************************************************************************/
// handler_ip_config()
/******************************************************************************/
static i32 handler_ip_config(CYG_HTTPD_STATE *p)
{
    vtss_appl_ip_global_conf_t      global_conf;
    vtss_appl_ip_route_key_t        prev_rt, rt;
    vtss_appl_ip_route_conf_t       route_conf;
    bool                            first;
    mesa_vid_t                      vlan;
    mesa_rc                         rc;
    vtss_ifindex_t                  ifidx, prev_ifindex;
    int i, ct;
    char buf[128];
#ifdef VTSS_SW_OPTION_DNS
    char                            *ds, *dv;
    u32                             *ref, nxt, idx;
    vtss_ifindex_elm_t              ife;
    vtss_appl_dns_server_conf_t     dns_serv;
    vtss_appl_dns_name_conf_t       dns_name;
    vtss_appl_dns_proxy_conf_t      dns_prxy;
#endif /* VTSS_SW_OPTION_DNS */
#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)
    vtss_appl_dhcp6c_intf_conf_t    dhcp6c_cfg;
#endif /* defined(VTSS_SW_OPTION_DHCP6_CLIENT) */
    char errmsg[128];

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_IP)) {
        return -1;
    }
#endif

    T_D("handler_ip_config");

    if ((rc = vtss_appl_ip_global_conf_get(&global_conf)) != VTSS_RC_OK) {
        T_E("vtss_appl_ip_global_conf_get() failed: %s", error_txt(rc));
    }

#ifdef VTSS_SW_OPTION_DNS
    memset(&dns_prxy, 0x0, sizeof(vtss_appl_dns_proxy_conf_t));
    (void) vtss_appl_dns_proxy_config_get(&dns_prxy);
#endif /* VTSS_SW_OPTION_DNS */

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int val;
        u32 uval;
        if (cyg_httpd_form_varable_int(p, "ip_mode", &val)) {
            global_conf.routing_enable = val;
        }

        if ((rc = vtss_appl_ip_global_conf_set(&global_conf)) != VTSS_RC_OK) {
            T_E("vtss_appl_ip_global_conf_set() failed: %s", error_txt(rc));
        }
#ifdef VTSS_SW_OPTION_DNS
        if (vtss_appl_dns_domain_name_config_get(&dns_name) == VTSS_RC_OK &&
            cyg_httpd_form_varable_int(p, "ip_dns_domain_type", &val) &&
            val >= VTSS_APPL_DNS_CONFIG_TYPE_NONE &&
            val <= VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN) {
            memset(&dns_name, 0x0, sizeof(vtss_appl_dns_name_conf_t));
            dns_name.domainname_type = (vtss_appl_dns_config_type_t)val;
            if (dns_name.domainname_type == VTSS_APPL_DNS_CONFIG_TYPE_STATIC) {
                const char  *var_string;
                size_t      var_len;

                var_string = cyg_httpd_form_varable_string(p, "ip_dns_domain_value", &var_len);
                if (var_string) {
                    (void) cgi_unescape(var_string, dns_name.static_domain_name, var_len, sizeof(dns_name.static_domain_name));
                }
            }

            if (dns_name.domainname_type == VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN ||
                dns_name.domainname_type == VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN) {
                if (cyg_httpd_form_varable_int(p, "ip_dns_domain_value", &val)) {
                    (void) vtss_ifindex_from_vlan(val, &dns_name.dhcp_ifindex);
                }
            }

            if ((rc = vtss_appl_dns_domain_name_config_set(&dns_name)) != VTSS_RC_OK) {
                T_E("vtss_appl_dns_domain_name_config_set: %s", error_txt(rc));
            }
        }

        ref = NULL;
        while (vtss_appl_dns_server_config_itr(ref, &nxt) == VTSS_RC_OK) {
            idx = nxt;
            ref = &idx;
            if (vtss_appl_dns_server_config_get(&idx, &dns_serv) != VTSS_RC_OK) {
                T_D("Failed to get DNS server %u", idx);
                continue;
            }

            memset(buf, 0x0, sizeof(buf));
            ds = &buf[0];
            dv = &buf[(sizeof(buf) / 2)];
            sprintf(ds, "ip_dns_src_%u", idx);
            sprintf(dv, "ip_dns_value_%u", idx);

            if (cyg_httpd_form_varable_int(p, ds, &val) &&
                val >= VTSS_APPL_DNS_CONFIG_TYPE_NONE &&
                val <= VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN) {
                BOOL    set_cfg = TRUE;

                memset(&dns_serv, 0x0, sizeof(vtss_appl_dns_server_conf_t));
                dns_serv.server_type = (vtss_appl_dns_config_type_t)val;
                if (dns_serv.server_type == VTSS_APPL_DNS_CONFIG_TYPE_STATIC &&
                    !cyg_httpd_form_variable_ip_fmt(p, &dns_serv.static_ip_address, dv)) {
                    T_E("Invalid IP address, not changing DNS config(%u) %s", idx, dv);
                    set_cfg = FALSE;
                }

                if (dns_serv.server_type == VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN ||
                    dns_serv.server_type == VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN) {
                    if (cyg_httpd_form_varable_int(p, dv, &val) &&
                        vtss_ifindex_from_vlan(val, &dns_serv.static_ifindex) == VTSS_RC_OK) {
                    } else {
                        T_E("Invalid VLAN %d, not changing DNS config(%u)", val, idx);
                        set_cfg = FALSE;
                    }
                }

                if (set_cfg && (rc = vtss_appl_dns_server_config_set(&idx, &dns_serv)) != VTSS_RC_OK) {
                    T_E("vtss_appl_dns_server_config_set: %s", error_txt(rc));
                }
            }
        }

        dns_prxy.proxy_admin_state = cyg_httpd_form_variable_check_fmt(p, "ip_dns_proxy");
        if ((rc = vtss_appl_dns_proxy_config_set(&dns_prxy)) != VTSS_RC_OK) {
            T_E("set proxy dns: %s", error_txt(rc));
        }
#endif /* VTSS_SW_OPTION_DNS */
        memset(buf, 0x0, sizeof(buf));
        if (cyg_httpd_form_varable_int(p, "if_ct", &ct)) {
            CapArray<ip_web_conf_t, VTSS_APPL_CAP_IP_INTERFACE_CNT> cfg;
            char *ebp = &buf[0];
            int j = 0;
            for (i = 0; i < ct; i++) {
                BOOL vid_changed;
                cfg[j].change_ipv4 = cfg[j].change_ipv6 = vid_changed = FALSE;
                const char *escaped_ifidx_name;
                char ifidx_name[20];
                size_t len;
                char name[30];
                snprintf(name, sizeof(name), "if_vid_%d", i);
                if ((escaped_ifidx_name = cyg_httpd_form_varable_string(p, name, &len)) && len > 0) {
                    if (!cgi_unescape(escaped_ifidx_name, ifidx_name, len, sizeof(ifidx_name))) {
                        T_E("Invalid interface %s", escaped_ifidx_name);
                        continue;
                    }
                    /* Have a ifidx - what to do? */
                    if (vtss_str2ifindex(ifidx_name, len, &ifidx) != VTSS_RC_OK) {
                        T_E("Invalid interface %s", ifidx_name);
                        continue;
                    }

                    if (cyg_httpd_form_variable_check_fmt(p, "if_del_%d", i)) {
                        /* Delete the interface */
                        if ((rc = vtss_appl_ip_if_conf_del(ifidx)) != VTSS_RC_OK) {
                            T_E("Delete IP interface %s: %s", ifidx_name, error_txt(rc));
                        }
                    } else {
                        vtss_appl_ip_if_conf_ipv4_t ipconf4, ipconf4_old;
                        vtss_appl_ip_if_conf_ipv6_t ipconf6, ipconf6_old;

                        if (!vtss_appl_ip_if_exists(ifidx)) {
                            if ((rc = vtss_appl_ip_if_conf_set(ifidx)) != VTSS_RC_OK) {
                                T_E("Add IP interface %s: %s", ifidx_name, error_txt(rc));
                                continue;
                            }
                        }
                        /* IPv4 */
                        if ((rc = vtss_appl_ip_if_conf_ipv4_get(ifidx, &ipconf4_old)) == VTSS_RC_OK) {
                            mesa_ip_network_t network;
                            size_t            var_string_len;
                            const char        *var_string;

                            /* Track original config */
                            ipconf4 = ipconf4_old;
                            ipconf4.enable = false;

                            /* DHCP enabled? */
                            ipconf4.dhcpc_enable = cyg_httpd_form_variable_check_fmt(p, "if_dhcp_%d", i);
                            if (ipconf4.dhcpc_enable) {
                                ipconf4.enable = true;
                            }

                            /* DHCP client_id */
                            int client_id_type = 0;
                            if (ipconf4.enable && cyg_httpd_form_variable_int_fmt(p, &client_id_type, "if_client_id_type_%d", i)) {
                                memset(&ipconf4.dhcpc_params.client_id, 0, sizeof(ipconf4.dhcpc_params.client_id));
                                switch (client_id_type) {
                                case VTSS_APPL_IP_DHCP4C_ID_TYPE_IF_MAC:
                                    if (cyg_httpd_form_variable_u32_fmt(p, &uval, "if_client_id_if_mac_%d", i) &&
                                        uval != 0 /* 'none' option, no need to call vtss_ifindex_from_port() */)  {
                                        (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, uport2iport(uval), &ipconf4.dhcpc_params.client_id.if_mac);
                                    }

                                    break;

                                case VTSS_APPL_IP_DHCP4C_ID_TYPE_ASCII: {
                                    var_string = cyg_httpd_form_variable_str_fmt(p, &var_string_len, "if_client_id_ascii_%d", i);
                                    if (var_string && var_string_len >= VTSS_APPL_IP_DHCP4C_ID_MIN_LENGTH) {
                                        (void)cgi_unescape(var_string, ipconf4.dhcpc_params.client_id.ascii, var_string_len, sizeof(ipconf4.dhcpc_params.client_id.ascii));
                                    }

                                    break;
                                }

                                case VTSS_APPL_IP_DHCP4C_ID_TYPE_HEX:
                                    var_string = cyg_httpd_form_variable_str_fmt(p, &var_string_len, "if_client_id_hex_%d", i);
                                    if (var_string && (var_string_len >= (VTSS_APPL_IP_DHCP4C_ID_MIN_LENGTH * 2)) &&
                                        ((var_string_len % 2) == 0) /* Must be even since one byte hex value is presented as two octets string */) {
                                        (void)cgi_unescape(var_string, ipconf4.dhcpc_params.client_id.hex, var_string_len, sizeof(ipconf4.dhcpc_params.client_id.hex));
                                    }

                                    break;

                                case VTSS_APPL_IP_DHCP4C_ID_TYPE_AUTO:
                                default:
                                    // Do nothing
                                    break;
                                }

                                ipconf4.dhcpc_params.client_id.type = (vtss_appl_ip_dhcp4c_id_type_t) client_id_type;
                            } else {
                                memset(&ipconf4.dhcpc_params.client_id, 0, sizeof(ipconf4.dhcpc_params.client_id));
                            }

                            /* DHCP hostname */
                            var_string = cyg_httpd_form_variable_str_fmt(p, &var_string_len, "if_hostname_%d", i);
                            if (var_string && ipconf4.enable) {
                                (void) cgi_unescape(var_string, ipconf4.dhcpc_params.hostname, var_string_len, sizeof(ipconf4.dhcpc_params.hostname));
                            } else {
                                strcpy(ipconf4.dhcpc_params.hostname, "");
                            }

                            if (cyg_httpd_form_variable_u32_fmt(p, &uval, "if_tout_%d", i))  {
                                ipconf4.fallback_timeout_secs = uval;
                                ipconf4.fallback_enable = ipconf4.fallback_timeout_secs > 0;
                            }

                            if (net_parse(p, MESA_IP_TYPE_IPV4, &network, i)) {
                                ipconf4.enable = true;
                                ipconf4.network.address = network.address.addr.ipv4;
                                ipconf4.network.prefix_size = network.prefix_size;
                            }

                            T_D("Ifidx: %s, Dhcp: %u, Active: %u", ifidx_name, ipconf4.dhcpc_enable, ipconf4.enable);
                            if (ip2_ipv4_config_differs(&ipconf4_old, &ipconf4)) {
                                T_D("Ifidx: %s, differs", ifidx_name);
                                cfg[j].ifidx = ifidx;
                                cfg[j].change_ipv4 = vid_changed = TRUE;
                                cfg[j].ipv4 = ipconf4;
                            }
                        } else {
                            T_E("Get IPv4 conf, ifindex %s: %s", ifidx_name, error_txt(rc));
                        }
                        /* IPv6 */
                        if (vtss_ip_hasipv6()) {
                            if ((rc = vtss_appl_ip_if_conf_ipv6_get(ifidx, &ipconf6_old)) == VTSS_RC_OK) {
                                mesa_ip_network_t network;
                                ipconf6 = ipconf6_old;
                                if (net_parse(p, MESA_IP_TYPE_IPV6, &network, i)) {
                                    ipconf6.enable = true;
                                    ipconf6.network.address = network.address.addr.ipv6;
                                    ipconf6.network.prefix_size = network.prefix_size;
                                } else {
                                    ipconf6.enable = false;
                                }

                                if (ip2_ipv6_config_differs(&ipconf6_old, &ipconf6)) {
                                    cfg[j].ifidx = ifidx;
                                    cfg[j].change_ipv6 = vid_changed = TRUE;
                                    cfg[j].ipv6 = ipconf6;
                                }
                            } else {
                                T_E("Get IPv6 conf, ifidx %s: %s", ifidx_name, error_txt(rc));
                            }
#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)
                            if (rc == VTSS_RC_OK) {
                                BOOL    new_if = FALSE, do_cfg;

                                if (vtss_appl_dhcp6c_interface_config_get(&ifidx, &dhcp6c_cfg) != VTSS_RC_OK) {
                                    if (vtss_appl_dhcp6c_interface_config_default(&dhcp6c_cfg) == VTSS_RC_OK) {
                                        new_if = TRUE;
                                    }
                                }

                                do_cfg = cyg_httpd_form_variable_check_fmt(p, "if_dhcp6_%d", i);
                                dhcp6c_cfg.rapid_commit = cyg_httpd_form_variable_check_fmt(p, "if_rpcmt_%d", i);
                                if (!new_if) {
                                    if (!do_cfg) {
                                        if ((rc = vtss_appl_dhcp6c_interface_config_del(&ifidx)) != VTSS_RC_OK) {
                                            VTSS_TRACE(DEBUG) << "DHCP6C_DEL(" << ifidx << "): " << error_txt(rc);
                                        }
                                    } else {
                                        if ((rc = vtss_appl_dhcp6c_interface_config_set(&ifidx, &dhcp6c_cfg)) != VTSS_RC_OK) {
                                            VTSS_TRACE(DEBUG) << "DHCP6C_SET(" << ifidx << "): " << error_txt(rc);
                                        }
                                    }
                                } else {
                                    if (do_cfg &&
                                        (rc = vtss_appl_dhcp6c_interface_config_add(&ifidx, &dhcp6c_cfg)) != VTSS_RC_OK) {
                                        VTSS_TRACE(DEBUG) << "DHCP6C_ADD(" << ifidx << "): " << error_txt(rc);
                                    }
                                }
                            }
#endif /* defined(VTSS_SW_OPTION_DHCP6_CLIENT) */
                        }
                    }
                }

                if (vid_changed) {
                    j++;        /* One more vid changed */
                }
            }

            if (j) {
                T_D("Need to reconfigure %d interfaces", j);
                /* Update the changed IP interfaces */
                for (i = 0; i < j; i++) {
                    ifidx = cfg[i].ifidx;
                    char ifidx_name[20];
                    vtss_ifindex2str(ifidx_name, sizeof(ifidx_name), ifidx);
                    if (cfg[i].change_ipv4) {
                        if ((rc = vtss_appl_ip_if_conf_ipv4_set(ifidx, &cfg[i].ipv4)) != VTSS_RC_OK) {
                            sprintf(errmsg, "Failed to set IPv4 configuration on %s: %s", ifidx_name, error_txt(rc));
                            redirect_errmsg(p, "ip_config.htm", errmsg);
                            return -1;
                        }
                    }

                    if (cfg[i].change_ipv6) {
                        if ((rc = vtss_appl_ip_if_conf_ipv6_set(ifidx, &cfg[i].ipv6)) != VTSS_RC_OK) {
                            if (rc == VTSS_APPL_IP_RC_IF_ADDRESS_CONFLICT_WITH_EXISTING ||
                                rc == VTSS_APPL_IP_RC_IF_ADDRESS_CONFLICT_WITH_STATIC_ROUTE ||
                                rc == VTSS_APPL_IP_RC_ROUTE_DEST_CONFLICT_WITH_LOCAL_IF) {
                                int err_strlen = strlen(error_txt(rc));
                                err_strlen += strlen(ifidx_name) + 6;
                                /*
                                    BZ#15858
                                    SLAAC may have multiple addresses assigned.
                                    So, we alert the setting failures instead.
                                    VID: 1 ~ 4095 -> strlen(VlanXXXX:ERR\n)
                                */
                                VTSS_TRACE(DEBUG) << "Set IPv6 conf, ifidx " << ifidx
                                                  << ": " << error_txt(rc);
                                if (sizeof(buf) - strlen(buf) > err_strlen) {
                                    ebp += snprintf(ebp, sizeof(buf) - strlen(buf),
                                                    "%s:IPv6 %s\n", ifidx_name, error_txt(rc));
                                }
                            } else {
                                T_E("Set IPv6 conf, %s: %s", ifidx_name, error_txt(rc));
                            }
                        }
                    }
                }
            }
        }

        if (cyg_httpd_form_varable_int(p, "rt_ct", &ct) && ct > 0) {
            for (i = 0; i < ct; i++) {
                BOOL do_delete = cyg_httpd_form_variable_check_fmt(p, "rt_del_%d", i);
                if (rt4_parse(p, &rt, &route_conf, i) || rt6_parse(p, &rt, &route_conf, i)) {
                    T_D("Got %s %s = %s", do_delete ? "delete" : "add", rt, route_conf);
                    if (do_delete) {
                        rc = vtss_appl_ip_route_conf_del(&rt);
                    } else {
                        rc = vtss_appl_ip_route_conf_set(&rt, &route_conf);
                    }

                    if (rc != VTSS_RC_OK) {
                        sprintf(errmsg, "Operation failed: %s", error_txt(rc));
                        redirect_errmsg(p, "ip_config.htm", errmsg);
                        return -1;
                    }
                }
            }
        }

        if (strlen(buf) > 0) {
            redirect_errmsg(p, "ip_config.htm", buf);
        } else {
            redirect(p, "/ip_config.htm");
        }
    } else { // CYG_HTTPD_METHOD_GET
        cyg_httpd_start_chunked("html");
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                      "%d|%d|%d",
                      fast_cap(VTSS_APPL_CAP_IP_INTERFACE_CNT),
                      fast_cap(VTSS_APPL_CAP_IP_ROUTE_CNT),
                      global_conf.routing_enable);
        cyg_httpd_write_chunked(p->outbuffer, ct);
#ifdef VTSS_SW_OPTION_DNS
        if (vtss_appl_dns_domain_name_config_get(&dns_name) == VTSS_RC_OK) {
            char    encoded_buf[(4 * VTSS_APPL_DNS_MAX_STRING_LEN) + 1];

            memset(encoded_buf, 0x0, sizeof(encoded_buf));
            switch ( dns_name.domainname_type ) {
            case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
                (void) cgi_escape(dns_name.static_domain_name, encoded_buf);
                break;

            case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
            case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
                if (vtss_ifindex_decompose(dns_name.dhcp_ifindex, &ife) == VTSS_RC_OK &&
                    ife.iftype == VTSS_IFINDEX_TYPE_VLAN) {
                    (void) snprintf(encoded_buf, sizeof(encoded_buf), "%d", ife.ordinal);
                } else {
                    (void) snprintf(encoded_buf, sizeof(encoded_buf), "%d", 0);
                }

                break;

            default:
                break;
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          ";%d|%s", dns_name.domainname_type, encoded_buf);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), ";%d", dns_prxy.proxy_admin_state);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        ref = NULL;
        while (vtss_appl_dns_server_config_itr(ref, &nxt) == VTSS_RC_OK) {
            idx = nxt;
            ref = &idx;
            if (vtss_appl_dns_server_config_get(&idx, &dns_serv) != VTSS_RC_OK) {
                T_D("Failed to get DNS server %u", idx);
                continue;
            }

            switch (dns_serv.server_type) {
            case VTSS_APPL_DNS_CONFIG_TYPE_STATIC:
                (void)vtss_ip_ip_addr_to_txt(buf, sizeof(buf), &dns_serv.static_ip_address);
                break;

            case VTSS_APPL_DNS_CONFIG_TYPE_DHCP4_VLAN:
            case VTSS_APPL_DNS_CONFIG_TYPE_DHCP6_VLAN:
                if (vtss_ifindex_decompose(dns_serv.static_ifindex, &ife) == VTSS_RC_OK &&
                    ife.iftype == VTSS_IFINDEX_TYPE_VLAN) {
                    (void) snprintf(buf, sizeof(buf), "%d", ife.ordinal);
                } else {
                    (void) snprintf(buf, sizeof(buf), "%d", 0);
                }

                break;

            default:
                buf[0] = '\0';
                break;
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "%s%d|%s", idx ? "/" : ";", dns_serv.server_type, buf);
            cyg_httpd_write_chunked(p->outbuffer, ct);
        }
#endif /* VTSS_SW_OPTION_DNS */

        cyg_httpd_write_chunked(",", 1);
        prev_ifindex = VTSS_IFINDEX_NONE;
        while (vtss_appl_ip_if_itr(&prev_ifindex, &ifidx) == VTSS_RC_OK) {
            vtss_appl_ip_if_conf_ipv4_t ipconf;
            vtss_appl_ip_if_conf_ipv6_t ipconf6;
            vtss_appl_ip_if_status_t    ifstat;

            prev_ifindex = ifidx;

            rc = vtss_appl_ip_if_conf_ipv4_get(ifidx, &ipconf);
            if (rc != VTSS_RC_OK) {
                VTSS_TRACE(ERROR) << "interface_config(" << ifidx << "): " << error_txt(rc);
                goto NEXT;
            }

            /* Vid */
            char ifidx_txt[20];
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                          "%s#", vtss_ifindex2str(ifidx_txt, sizeof(ifidx_txt), ifidx));
            cyg_httpd_write_chunked(p->outbuffer, ct);

            /* dhcp + hostname + fallback */
            if (ipconf.enable) {
                vtss_ifindex_elm_t elm;
                char encoded_string[3 * VTSS_APPL_IP_DHCP4C_HOSTNAME_MAX_LENGTH];

                // output dhcp enable?
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d", ipconf.dhcpc_enable);
                cyg_httpd_write_chunked(p->outbuffer, ct);

                // output client_id_type/client_id_if_mac/client_id_ascii
                encoded_string[0] = '\0';
                if (ipconf.dhcpc_params.client_id.type == VTSS_APPL_IP_DHCP4C_ID_TYPE_IF_MAC) {
                    (void)vtss_ifindex_decompose(ipconf.dhcpc_params.client_id.if_mac, &elm);
                } else if (ipconf.dhcpc_params.client_id.type == VTSS_APPL_IP_DHCP4C_ID_TYPE_ASCII) {
                    (void) cgi_escape(ipconf.dhcpc_params.client_id.ascii, encoded_string);
                }

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              "#%d#%d#%s#",
                              /* client_id_type   */ ipconf.dhcpc_params.client_id.type,
                              /* client_id_if_mac */ (ipconf.dhcpc_params.client_id.type == VTSS_APPL_IP_DHCP4C_ID_TYPE_IF_MAC) ? iport2uport(elm.ordinal) : 0,
                              /* client_id_ascii  */ encoded_string);
                cyg_httpd_write_chunked(p->outbuffer, ct);

                // output client_id_hex
                if (ipconf.dhcpc_params.client_id.type == VTSS_APPL_IP_DHCP4C_ID_TYPE_HEX) {
                    size_t hex_str_len = strlen(ipconf.dhcpc_params.client_id.hex);
                    for (i = 0; i < hex_str_len; i++) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%c", toupper(ipconf.dhcpc_params.client_id.hex[i]));
                        cyg_httpd_write_chunked(p->outbuffer, ct);
                    }
                }

                // output hostname/fallback timeout
                encoded_string[0] = '\0';
                if (strlen(ipconf.dhcpc_params.hostname)) {
                    (void) cgi_escape(ipconf.dhcpc_params.hostname, encoded_string);
                }

                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              "#%s#%u#",
                              /* hostname         */ encoded_string,
                              /* fallback timeout */ ipconf.fallback_enable ? ipconf.fallback_timeout_secs : 0);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            } else {
                cyg_httpd_write_chunked("0#0#1####0#", 11);
            }

            /* static configured address */
            if (!ipconf.enable || (ipconf.dhcpc_enable && !ipconf.fallback_enable)) {
                cyg_httpd_write_chunked("##", 2);
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              VTSS_IPV4_FORMAT"#%d#",
                              VTSS_IPV4_ARGS(ipconf.network.address),
                              ipconf.network.prefix_size);
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            /* IPv4 - current */
            if (vtss_appl_ip_if_status_get(ifidx, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 1, nullptr, &ifstat) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                              VTSS_IPV4N_FORMAT,
                              VTSS_IPV4N_ARG(ifstat.u.ipv4.net));
                cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            cyg_httpd_write_chunked("#", 1);

            if (vtss_ip_hasipv6() && (rc = vtss_appl_ip_if_conf_ipv6_get(ifidx, &ipconf6)) == VTSS_RC_OK) {
                /* IPv6 - configured */
                if (ipconf6.enable) {
                    (void) misc_ipv6_txt(&ipconf6.network.address, buf);
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#%d#", buf, ipconf6.network.prefix_size);
                    cyg_httpd_write_chunked(p->outbuffer, ct);
                } else {
                    cyg_httpd_write_chunked("##", 2);
                }
            }
#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)
            if (vtss_appl_dhcp6c_interface_config_get(&ifidx, &dhcp6c_cfg) == VTSS_RC_OK) {
                vtss_appl_dhcp6c_interface_t dhcp6c_intf;

                if (vtss_appl_dhcp6c_interface_status_get(&ifidx, &dhcp6c_intf) == VTSS_RC_OK) {
                    (void)misc_ipv6_txt(&dhcp6c_intf.address, buf);
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "1#%d#%s#", dhcp6c_cfg.rapid_commit, buf);
                } else {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "1#%d##", dhcp6c_cfg.rapid_commit);
                }

                cyg_httpd_write_chunked(p->outbuffer, ct);
            } else {
                cyg_httpd_write_chunked("0#0##", 5);
            }
#endif /* defined(VTSS_SW_OPTION_DHCP6_CLIENT) */
NEXT:
            cyg_httpd_write_chunked("|", 1); /* End of interface */
        }

        cyg_httpd_write_chunked(",", 1);

        first = true;
        while (vtss_appl_ip_route_conf_itr(first ? nullptr : &prev_rt, &rt) == VTSS_RC_OK) {
            first   = false;
            prev_rt = rt;

            if (vtss_appl_ip_route_conf_get(&rt, &route_conf) != VTSS_RC_OK) {
                T_I("vtss_appl_ip_route_conf_get(%s) failed: %s", rt, error_txt(rc));
                continue;
            }

            switch (rt.type) {
            case VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC:
                if (rt.route.ipv4_uc.destination == vtss_ipv4_blackhole_route) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                  VTSS_IPV4_FORMAT "#%d#blackhole#%u#%u|",
                                  VTSS_IPV4N_ARG(rt.route.ipv4_uc.network),
                                  0, // VLAN interface is unused for IPv4
                                  route_conf.distance);
                } else {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer),
                                  VTSS_IPV4_FORMAT "#%d#" VTSS_IPV4_FORMAT "#%u#%u|",
                                  VTSS_IPV4N_ARG(rt.route.ipv4_uc.network),
                                  VTSS_IPV4_ARGS(rt.route.ipv4_uc.destination),
                                  0, // VLAN interface is unused for IPv4
                                  route_conf.distance);
                }

                cyg_httpd_write_chunked(p->outbuffer, ct);
                break;

            case VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC:
                if (!vtss_ip_hasipv6()) {
                    break;
                }

                (void)misc_ipv6_txt(&rt.route.ipv6_uc.network.address, buf);
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s#%d#", buf, rt.route.ipv6_uc.network.prefix_size);

                if (rt.route.ipv6_uc.destination == vtss_ipv6_blackhole_route) {
                    strcpy(buf, "blackhole");
                } else {
                    misc_ipv6_txt(&rt.route.ipv6_uc.destination, buf);
                }

                vlan = vtss_ifindex_as_vlan(rt.vlan_ifindex);
                ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, "%s#%u#%u|", buf, vlan, route_conf.distance);
                cyg_httpd_write_chunked(p->outbuffer, ct);
                break;

            default:
                break;
            }
        }

        cyg_httpd_end_chunked();
    }

    return -1; // Do not further search the file system.
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_ip, "/config/ip2_config", handler_ip_config);

static i32 handler_diag_ping(CYG_HTTPD_STATE *p)
{
    char            dest_ipaddr[VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN + 1];
    char            src_ipaddr[VTSS_APPL_SYSUTIL_INPUT_HOSTNAME_LEN + 1];
    const char      *dest_addr_buf;
    const char      *src_addr_buf;
    size_t          dest_len, src_len;
    cli_iolayer_t   *web_io = NULL;
    ulong           ioindex;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PING)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        u32 src_vid = 0, ping_length = 2, count = 5, interval = 0, ttlvalue = 0, pdata = 0;
        u32 src_portno = 0;
        BOOL quiet = FALSE;
        const char *pdata_str;
        size_t pdata_len = 0;
        char str_buff[64];

        dest_addr_buf = cyg_httpd_form_varable_string(p, "ip_addr", &dest_len);
        src_addr_buf = cyg_httpd_form_varable_string(p, "src_addr", &src_len);

        (void) cyg_httpd_form_varable_long_int(p, "src_vid", &src_vid);
        (void) cyg_httpd_form_varable_long_int(p, "length", &ping_length);
        (void) cyg_httpd_form_varable_long_int(p, "count", &count);
        (void) cyg_httpd_form_varable_long_int(p, "ttlvalue", &ttlvalue);
        (void) cyg_httpd_form_varable_long_int(p, "src_portno", &src_portno);

        // check for hex notation (0x<value>) for payload data field
        pdata_str = cyg_httpd_form_varable_string(p, "pdata", &pdata_len);
        if (pdata_str != nullptr && pdata_len >= 3 && pdata_str[0] == '0' && tolower(pdata_str[1]) == 'x') {
            pdata = strtoul(pdata_str, nullptr, 16);

        } else {
            (void) cyg_httpd_form_varable_long_int(p, "pdata", &pdata);
        }

        quiet = (cyg_httpd_form_varable_find(p, "quiet") != NULL);

        if (dest_addr_buf && ping_length) {
            dest_len = MIN(dest_len, sizeof(dest_ipaddr) - 1);
            strncpy(dest_ipaddr, dest_addr_buf, dest_len);
            dest_ipaddr[dest_len] = '\0';

            src_len = MIN(src_len, sizeof(src_ipaddr) - 1);
            strncpy(src_ipaddr, src_addr_buf, src_len);
            src_ipaddr[src_len] = '\0';

            if (src_portno > 0) {
                mesa_port_no_t iport = uport2iport(src_portno);
                mesa_ip_addr_t check_addr;

                if (!IP_WEB_ip_by_port(iport, MESA_IP_TYPE_IPV4, &check_addr)) {
                    const char *err = "Invalid source port number or address not configured.";
                    send_custom_error(p, "Ping Arguments Failure", err, strlen(err));
                    return -1;
                }

                (void) icli_ipv4_to_str(check_addr.addr.ipv4, src_ipaddr);
            }

            web_io = ping_test_async(dest_ipaddr, src_ipaddr, src_vid, count, interval, ping_length, pdata, ttlvalue, quiet);
            if (web_io) {
                sprintf(str_buff, "/ping4_result.htm?ioIndex=%p", web_io);
                redirect(p, str_buff);
            } else {
                const char *err = "No available internal resource, please try again later.";
                send_custom_error(p, "Ping Fail", err, strlen(err));
            }
        }
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        if ((dest_addr_buf = cyg_httpd_form_varable_find(p, "ioIndex"))) {
            (void) cyg_httpd_str_to_hex(dest_addr_buf, &ioindex);
            web_io = (cli_iolayer_t *) ioindex;
            web_send_iolayer_output(WEB_CLI_IO_TYPE_PING, web_io, "html");
        }
    }

    return -1; // Do not further search the file system.
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_diag_ping, "/config/ping4", handler_diag_ping);

#ifdef VTSS_SW_OPTION_IPV6
static i32 handler_diag_ping_ipv6(CYG_HTTPD_STATE *p)
{
    char            dest_ipaddr[VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN + 1];
    char            src_ipaddr[VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN + 1];
    const char      *dest_addr_buf;
    const char      *src_addr_buf;
    size_t          dest_len, src_len;
    cli_iolayer_t   *web_io = NULL;
    ulong           ioindex;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PING)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        u32 src_vid = 0, ping_length = 2, count = 5, interval = 0, pdata = 0;
        u32 src_portno = 0;
        BOOL quiet = FALSE;
        const char *pdata_str;
        size_t pdata_len = 0;
        char str_buff[64];

        dest_addr_buf = cyg_httpd_form_varable_string(p, "ipv6_addr", &dest_len);
        src_addr_buf = cyg_httpd_form_varable_string(p, "src_addr", &src_len);

        (void) cyg_httpd_form_varable_long_int(p, "src_vid", &src_vid);
        (void)cyg_httpd_form_varable_long_int(p, "length", &ping_length);
        (void)cyg_httpd_form_varable_long_int(p, "count", &count);
        (void) cyg_httpd_form_varable_long_int(p, "src_portno", &src_portno);

        // check for hex notation (0x<value>) for payload data field
        pdata_str = cyg_httpd_form_varable_string(p, "pdata", &pdata_len);
        if (pdata_str != nullptr && pdata_len >= 3 && pdata_str[0] == '0' && tolower(pdata_str[1]) == 'x') {
            pdata = strtoul(pdata_str, nullptr, 16);

        } else {
            (void) cyg_httpd_form_varable_long_int(p, "pdata", &pdata);
        }

        quiet = (cyg_httpd_form_varable_find(p, "quiet") != NULL);

        if (dest_addr_buf) {
            dest_len = MIN(dest_len, sizeof(dest_ipaddr) - 1);
            memset(dest_ipaddr, 0x0, sizeof(dest_ipaddr));
            (void) cgi_unescape(dest_addr_buf, dest_ipaddr, dest_len, sizeof(dest_ipaddr));

            src_len = MIN(src_len, sizeof(src_ipaddr) - 1);
            memset(src_ipaddr, 0x0, sizeof(src_ipaddr));
            (void) cgi_unescape(src_addr_buf, src_ipaddr, src_len, sizeof(src_ipaddr));

            if (src_portno > 0) {
                mesa_port_no_t iport = uport2iport(src_portno);
                mesa_ip_addr_t check_addr;

                if (!IP_WEB_ip_by_port(iport, MESA_IP_TYPE_IPV6, &check_addr)) {
                    const char *err = "Invalid source port number or address not configured.";
                    send_custom_error(p, "Ping Arguments Failure", err, strlen(err));
                    return -1;
                }

                (void) icli_ipv6_to_str(check_addr.addr.ipv6, src_ipaddr);
            }

            web_io = ping6_test_async(dest_ipaddr, src_ipaddr, src_vid, count, interval, ping_length, pdata, quiet);
            if (web_io) {
                sprintf(str_buff, "/ping6_result.htm?ioIndex=%p", web_io);
                redirect(p, str_buff);
            } else {
                const char *err = "No available internal resource, please try again later.";
                send_custom_error(p, "Ping Fail", err, strlen(err));
            }
        }
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        if ((dest_addr_buf = cyg_httpd_form_varable_find(p, "ioIndex"))) {
            (void) cyg_httpd_str_to_hex(dest_addr_buf, &ioindex);
            web_io = (cli_iolayer_t *) ioindex;
            web_send_iolayer_output(WEB_CLI_IO_TYPE_PING, web_io, "html");
        }
    }

    return -1; // Do not further search the file system.
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_diag_ping_ipv6, "/config/ping6", handler_diag_ping_ipv6);
#endif /* VTSS_SW_OPTION_IPV6 */

static i32 handler_diag_traceroute(CYG_HTTPD_STATE *p)
{
    char            dest_ipaddr[VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN + 1];
    char            src_ipaddr[VTSS_APPL_SYSUTIL_INPUT_HOSTNAME_LEN + 1];
    const char      *dest_addr_buf;
    const char      *src_addr_buf;
    size_t          dest_len, src_len;
    cli_iolayer_t   *web_io = NULL;
    ulong           ioindex;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_TRACEROUTE)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int src_vid = 0, dscp = -1, timeout = -1, probes = -1, firstttl = -1, maxttl = -1;
        BOOL icmp = FALSE, numeric = FALSE;
        char str_buff[64];

        dest_addr_buf = cyg_httpd_form_varable_string(p, "ip_addr", &dest_len);
        src_addr_buf = cyg_httpd_form_varable_string(p, "src_addr", &src_len);

        (void) cyg_httpd_form_varable_long_int(p, "src_vid", &src_vid);
        (void) cyg_httpd_form_varable_long_int(p, "dscp", &dscp);
        (void) cyg_httpd_form_varable_long_int(p, "probes", &probes);
        (void) cyg_httpd_form_varable_long_int(p, "timeout", &timeout);
        (void) cyg_httpd_form_varable_long_int(p, "firstttl", &firstttl);
        (void) cyg_httpd_form_varable_long_int(p, "maxttl", &maxttl);

        icmp = (cyg_httpd_form_varable_find(p, "icmp") != NULL);
        numeric = (cyg_httpd_form_varable_find(p, "numeric") != NULL);

        if (dest_addr_buf) {
            dest_len = MIN(dest_len, sizeof(dest_ipaddr) - 1);
            strncpy(dest_ipaddr, dest_addr_buf, dest_len);
            dest_ipaddr[dest_len] = '\0';

            src_len = MIN(src_len, sizeof(src_ipaddr) - 1);
            strncpy(src_ipaddr, src_addr_buf, src_len);
            src_ipaddr[src_len] = '\0';

            web_io = traceroute_test_async(dest_ipaddr, src_ipaddr, src_vid, dscp, timeout, probes,
                                           firstttl, maxttl, icmp, numeric);
            if (web_io) {
                sprintf(str_buff, "/traceroute4_result.htm?ioIndex=%p", web_io);
                redirect(p, str_buff);
            } else {
                const char *err = "No available internal resource, please try again later.";
                send_custom_error(p, "Traceroute Fail", err, strlen(err));
            }
        }

    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        if ((dest_addr_buf = cyg_httpd_form_varable_find(p, "ioIndex"))) {
            (void) cyg_httpd_str_to_hex(dest_addr_buf, &ioindex);
            web_io = (cli_iolayer_t *) ioindex;
            // Traceroute uses same IO type as Ping for convenience
            web_send_iolayer_output(WEB_CLI_IO_TYPE_PING, web_io, "html");
        }
    }

    return -1; // Do not further search the file system.
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_diag_traceroute, "/config/traceroute4", handler_diag_traceroute);

#ifdef VTSS_SW_OPTION_IPV6
static i32 handler_diag_traceroute_ipv6(CYG_HTTPD_STATE *p)
{
    char            dest_ipaddr[VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN + 1];
    char            src_ipaddr[VTSS_APPL_SYSUTIL_INPUT_DOMAIN_NAME_LEN + 1];
    const char      *dest_addr_buf;
    const char      *src_addr_buf;
    size_t          dest_len, src_len;
    cli_iolayer_t   *web_io = NULL;
    ulong           ioindex;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_TRACEROUTE)) {
        return -1;
    }
#endif

    if (p->method == CYG_HTTPD_METHOD_POST) {
        int src_vid = 0, dscp = -1, timeout = -1, probes = -1, maxttl = -1;
        BOOL numeric = FALSE;
        char str_buff[64];

        dest_addr_buf = cyg_httpd_form_varable_string(p, "ipv6_addr", &dest_len);
        src_addr_buf = cyg_httpd_form_varable_string(p, "src_addr", &src_len);

        (void) cyg_httpd_form_varable_long_int(p, "src_vid", &src_vid);
        (void) cyg_httpd_form_varable_long_int(p, "dscp", &dscp);
        (void) cyg_httpd_form_varable_long_int(p, "probes", &probes);
        (void) cyg_httpd_form_varable_long_int(p, "timeout", &timeout);
        (void) cyg_httpd_form_varable_long_int(p, "maxttl", &maxttl);

        numeric = (cyg_httpd_form_varable_find(p, "numeric") != NULL);

        if (dest_addr_buf) {
            dest_len = MIN(dest_len, sizeof(dest_ipaddr) - 1);
            memset(dest_ipaddr, 0x0, sizeof(dest_ipaddr));
            (void) cgi_unescape(dest_addr_buf, dest_ipaddr, dest_len, sizeof(dest_ipaddr));

            src_len = MIN(src_len, sizeof(src_ipaddr) - 1);
            memset(src_ipaddr, 0x0, sizeof(src_ipaddr));
            (void) cgi_unescape(src_addr_buf, src_ipaddr, src_len, sizeof(src_ipaddr));

            web_io = traceroute6_test_async(dest_ipaddr, src_ipaddr, src_vid, dscp, timeout, probes,
                                            maxttl, numeric);
            if (web_io) {
                sprintf(str_buff, "/traceroute6_result.htm?ioIndex=%p", web_io);
                redirect(p, str_buff);
            } else {
                const char *err = "No available internal resource, please try again later.";
                send_custom_error(p, "Traceroute Fail", err, strlen(err));
            }
        }

    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        if ((dest_addr_buf = cyg_httpd_form_varable_find(p, "ioIndex"))) {
            (void) cyg_httpd_str_to_hex(dest_addr_buf, &ioindex);
            web_io = (cli_iolayer_t *) ioindex;
            // Traceroute uses same IO type as Ping for convenience
            web_send_iolayer_output(WEB_CLI_IO_TYPE_PING, web_io, "html");
        }
    }

    return -1; // Do not further search the file system.
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_diag_traceroute6, "/config/traceroute6", handler_diag_traceroute_ipv6);
#endif

/*lint -sem(ip2_printf, thread_protected) ... function only called from web server */
static int ip2_printf(const char *fmt, ...)
{
    int ct;
    va_list ap; /*lint -e{530} ... 'ap' is initialized by va_start() */

    va_start(ap, fmt);
    ct = vsnprintf(httpstate.outbuffer, sizeof(httpstate.outbuffer), fmt, ap);
    va_end(ap);
    cyg_httpd_write_chunked(httpstate.outbuffer, ct);

    return ct;
}

static i32 handler_ip_status(CYG_HTTPD_STATE *p)
{
#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_IP)) {
        return -1;
    }
#endif

    cyg_httpd_start_chunked("html");

    (void)ip_util_if_print(ip2_printf, VTSS_APPL_IP_IF_STATUS_TYPE_ANY, false); /*[0]*/

    cyg_httpd_write_chunked("^@^@^", 5); /* Split interfaces and routes */

    (void)ip_util_route_print(VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC, ip2_printf); /*[1]*/

    cyg_httpd_write_chunked("^@^@^", 5); /* Split ipv4 and ipv6 routes */

    (void)ip_util_route_print(VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC, ip2_printf); /*[2]*/

    cyg_httpd_write_chunked("^@^@^", 5); /* Split routes and neighbors */

    (void)ip_util_nb_print(MESA_IP_TYPE_IPV4, ip2_printf); /*[3]*/

    cyg_httpd_write_chunked("^@^@^", 5); /* Split ipv4 and ipv6 neighbors */

    (void)ip_util_nb_print(MESA_IP_TYPE_IPV6, ip2_printf); /*[4]*/

    cyg_httpd_write_chunked("^@^@^", 5); /* Split ipv6 neighbors and warnings */

    (void)ip_util_route_warning_print(ip2_printf); /*[5]*/

    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_ip, "/stat/ip2_status", handler_ip_status);

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t ip2_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[2048];

    (void) snprintf(buff, sizeof(buff),
                    "var configIPMaxInterfaces = %d;\n"
                    "var configIPMaxRoutes = %d;\n"
                    "var configIPClientIdLenMin = %d;\n"
                    "var configIPClientIdLenMax = %d;\n"
                    "var configIPHostnameLenMax = %d;\n"
                    "var configIPv6Support = %d;\n"
                    "var configDHCPv6Support = %d;\n"
                    "var configIPRoutingSupport = %d;\n"
                    "var configIPDNSSupport = %d;\n"
                    "var configPingLenMin = %d;\n"
                    "var configPingLenMax = %d;\n"
                    "var configPingCntMin = %d;\n"
                    "var configPingCntMax = %d;\n"
                    "var configPingIntervalMin = %d;\n"
                    "var configPingIntervalMax = %d;\n"
                    "var configPingPdataMin = %d;\n"
                    "var configPingPdataMax = %d;\n"
                    "var configPingTtlMin = %d;\n"
                    "var configPingTtlMax = %d;\n"
                    "var configTracerouteDscpMin = %d;\n"
                    "var configTracerouteDscpMax = %d;\n"
                    "var configTracerouteTimeoutMin = %d;\n"
                    "var configTracerouteTimeoutMax = %d;\n"
                    "var configTracerouteProbesMin = %d;\n"
                    "var configTracerouteProbesMax = %d;\n"
                    "var configTracerouteFttlMin = %d;\n"
                    "var configTracerouteFttlMax = %d;\n"
                    "var configTracerouteMttlMin = %d;\n"
                    "var configTracerouteMttlMax = %d;\n"
                    "var configVidMin = %d;\n"
                    "var configVidMax = %d;\n"
                    ,
                    fast_cap(VTSS_APPL_CAP_IP_INTERFACE_CNT),
                    fast_cap(VTSS_APPL_CAP_IP_ROUTE_CNT),
                    VTSS_APPL_IP_DHCP4C_ID_MIN_LENGTH,
                    VTSS_APPL_IP_DHCP4C_ID_MAX_LENGTH - 1,
                    VTSS_APPL_IP_DHCP4C_HOSTNAME_MAX_LENGTH - 1,
                    vtss_ip_hasipv6(),
                    vtss_ip_hasdhcpv6(),
                    vtss_ip_hasrouting(),
                    vtss_ip_hasdns(),
                    PING_MIN_PACKET_LEN, PING_MAX_PACKET_LEN,
                    PING_MIN_PACKET_CNT, PING_MAX_PACKET_CNT,
                    PING_MIN_PACKET_INTERVAL, PING_MAX_PACKET_INTERVAL,
                    PING_MIN_PAYLOAD_DATA, PING_MAX_PAYLOAD_DATA,
                    PING_MIN_TTL, PING_MAX_TTL,
                    TRACEROUTE_DSCP_MIN, TRACEROUTE_DSCP_MAX,
                    TRACEROUTE_TIMEOUT_MIN, TRACEROUTE_TIMEOUT_MAX,
                    TRACEROUTE_PROBES_MIN, TRACEROUTE_PROBES_MAX,
                    TRACEROUTE_FTTL_MIN, TRACEROUTE_FTTL_MAX,
                    TRACEROUTE_MTTL_MIN, TRACEROUTE_MTTL_MAX,
                    1, (MESA_VIDS - 1)
                   );

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib_config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(ip2_lib_config_js);

