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

#ifndef _IP_EXPOSE_HXX_
#define _IP_EXPOSE_HXX_

#include "vtss/appl/ip.h"
#include "vtss/basics/expose.hxx"

typedef vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_ifindex_t>,                       vtss::expose::ParamVal<vtss_appl_ip_if_status_link_t *>>   StatusIfLink;
typedef vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_appl_ip_if_key_ipv4_t *>,         vtss::expose::ParamVal<vtss_appl_ip_if_info_ipv4_t *>>     StatusIfIpv4;
typedef vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_appl_ip_if_key_ipv6_t *>,         vtss::expose::ParamVal<vtss_appl_ip_if_info_ipv6_t *>>     StatusIfIpv6;
typedef vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_appl_ip_acd_status_ipv4_key_t *>, vtss::expose::ParamVal<vtss_appl_ip_acd_status_ipv4_t *>>  StatusAcdIpv4;
typedef vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_appl_ip_neighbor_key_t *>,        vtss::expose::ParamVal<vtss_appl_ip_neighbor_status_t *>>  StatusNb;
typedef vtss::expose::StructStatus<vtss::expose::ParamVal<vtss_appl_ip_global_notification_status_t *>> IpGlobalNotificationStatus;
extern IpGlobalNotificationStatus ip_global_notification_status;

extern StatusIfLink status_if_link;
extern StatusIfIpv4 status_if_ipv4;
extern vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_ifindex_t>, vtss::expose::ParamVal<vtss_appl_ip_if_status_dhcp4c_t *>> status_if_dhcp4c;
extern StatusAcdIpv4 status_acd_ipv4;
extern StatusNb status_nb_ipv4;

extern StatusIfIpv6 status_if_ipv6;

#if defined(VTSS_SW_OPTION_IPV6)
extern StatusNb status_nb_ipv6;
#endif

// Wrapper functions needed because the serializer exposes IPv4 route tables
// separately from IPv6 route tables.

typedef struct {
    vtss_appl_ip_route_key_t key;
} ip_expose_route_key_ipv4_t;

typedef struct {
    vtss_appl_ip_route_key_t key;
} ip_expose_route_key_ipv6_t;

inline mesa_rc ip_expose_route_ipv4_itr(const ip_expose_route_key_ipv4_t *in, ip_expose_route_key_ipv4_t *out)
{
    return vtss_appl_ip_route_conf_itr(in ? &in->key : nullptr, &out->key, VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC);
}

inline mesa_rc ip_expose_route_ipv6_itr(const ip_expose_route_key_ipv6_t *in, ip_expose_route_key_ipv6_t *out)
{
    return vtss_appl_ip_route_conf_itr(in ? &in->key : nullptr, &out->key, VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC);
}

inline mesa_rc ip_expose_route_ipv4_conf_get(const ip_expose_route_key_ipv4_t *key, vtss_appl_ip_route_conf_t *conf)
{
    vtss_appl_ip_route_key_t real_key = key->key;
    real_key.type = VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC;
    return vtss_appl_ip_route_conf_get(&real_key, conf);
}

inline mesa_rc ip_expose_route_ipv6_conf_get(const ip_expose_route_key_ipv6_t *key, vtss_appl_ip_route_conf_t *conf)
{
    vtss_appl_ip_route_key_t real_key = key->key;
    real_key.type = VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC;
    return vtss_appl_ip_route_conf_get(&real_key, conf);
}

inline mesa_rc ip_expose_route_ipv4_conf_set(const ip_expose_route_key_ipv4_t *key, const vtss_appl_ip_route_conf_t *conf)
{
    vtss_appl_ip_route_key_t real_key = key->key;
    real_key.type = VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC;
    return vtss_appl_ip_route_conf_set(&real_key, conf);
}

inline mesa_rc ip_expose_route_ipv6_conf_set(const ip_expose_route_key_ipv6_t *key, const vtss_appl_ip_route_conf_t *conf)
{
    vtss_appl_ip_route_key_t real_key = key->key;
    real_key.type = VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC;
    return vtss_appl_ip_route_conf_set(&real_key, conf);
}

inline mesa_rc ip_expose_route_ipv4_conf_del(const ip_expose_route_key_ipv4_t *key)
{
    vtss_appl_ip_route_key_t real_key = key->key;
    real_key.type = VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC;
    return vtss_appl_ip_route_conf_del(&real_key);
}

inline mesa_rc ip_expose_route_ipv6_conf_del(const ip_expose_route_key_ipv6_t *key)
{
    vtss_appl_ip_route_key_t real_key = key->key;
    real_key.type = VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC;
    return vtss_appl_ip_route_conf_del(&real_key);
}

inline mesa_rc ip_expose_dhcp4c_control_restart_get(vtss_ifindex_t ifidx, mesa_bool_t *action)
{
    *action = false;
    return VTSS_RC_OK;
}

inline mesa_rc ip_expose_dhcp4c_control_restart(vtss_ifindex_t ifidx, mesa_bool_t action)
{
    if (action) {
        return vtss_appl_ip_if_dhcp4c_control_restart(ifidx);
    }

    return VTSS_RC_OK;
}

inline mesa_rc ip_expose_if_itr(const vtss_ifindex_t *in, vtss_ifindex_t *out)
{
    // The following function takes an optional parameter, so vtss_basics goes
    // haywire, which is why we need to wrap it.
    return vtss_appl_ip_if_itr(in, out);
}

inline mesa_rc ip_expose_if_conf_ipv4_get(vtss_ifindex_t ifindex, vtss_appl_ip_if_conf_ipv4_t *conf)
{
    VTSS_RC(vtss_appl_ip_if_conf_ipv4_get(ifindex, conf));

    if (!conf->fallback_enable) {
        conf->fallback_timeout_secs = 0;
    }

    return VTSS_RC_OK;
}

inline mesa_rc ip_expose_if_conf_ipv4_set(vtss_ifindex_t ifindex, const vtss_appl_ip_if_conf_ipv4_t *conf)
{
    vtss_appl_ip_if_conf_ipv4_t local_conf = *conf;

    local_conf.fallback_enable = local_conf.fallback_timeout_secs > 0;
    return vtss_appl_ip_if_conf_ipv4_set(ifindex, &local_conf);
}

typedef struct {
    vtss_appl_ip_neighbor_key_t key;
} ip_expose_neighbor_key_ipv4_t;

typedef struct {
    vtss_appl_ip_neighbor_key_t key;
} ip_expose_neighbor_key_ipv6_t;

inline mesa_rc ip_expose_neighbor_status_ipv4_itr(const ip_expose_neighbor_key_ipv4_t *in, ip_expose_neighbor_key_ipv4_t *out)
{
    return vtss_appl_ip_neighbor_itr(in ? &in->key : nullptr, &out->key, MESA_IP_TYPE_IPV4);
}

inline mesa_rc ip_expose_neighbor_status_ipv6_itr(const ip_expose_neighbor_key_ipv6_t *in, ip_expose_neighbor_key_ipv6_t *out)
{
    return vtss_appl_ip_neighbor_itr(in ? &in->key : nullptr, &out->key, MESA_IP_TYPE_IPV6);
}

inline mesa_rc ip_expose_neighbor_status_ipv4_get(const ip_expose_neighbor_key_ipv4_t *key, vtss_appl_ip_neighbor_status_t *status)
{
    return vtss_appl_ip_neighbor_status_get(&key->key, status);
}

inline mesa_rc ip_expose_neighbor_status_ipv6_get(const ip_expose_neighbor_key_ipv6_t *key, vtss_appl_ip_neighbor_status_t *status)
{
    return vtss_appl_ip_neighbor_status_get(&key->key, status);
}

inline mesa_rc ip_expose_if_status_ipv4_get(const vtss_appl_ip_if_key_ipv4_t *key, vtss_appl_ip_if_info_ipv4_t *info)
{
    vtss_appl_ip_if_status_ipv4_t status;

    VTSS_RC(vtss_appl_ip_if_status_ipv4_get(key, &status));

    *info = status.info;
    return VTSS_RC_OK;
}

inline mesa_rc ip_expose_if_status_ipv6_get(const vtss_appl_ip_if_key_ipv6_t *key, vtss_appl_ip_if_info_ipv6_t *info)
{
    vtss_appl_ip_if_status_ipv6_t status;

    VTSS_RC(vtss_appl_ip_if_status_ipv6_get(key, &status));

    *info = status.info;
    return VTSS_RC_OK;
}

typedef struct {
    vtss_appl_ip_route_status_key_t key;
} ip_expose_route_status_key_ipv4_t;

typedef struct {
    vtss_appl_ip_route_status_key_t key;
} ip_expose_route_status_key_ipv6_t;

inline mesa_rc ip_expose_route_status_ipv4_itr(const ip_expose_route_status_key_ipv4_t *in, ip_expose_route_status_key_ipv4_t *out)
{
    return vtss_appl_ip_route_status_itr(in ? &in->key : nullptr, &out->key, VTSS_APPL_IP_ROUTE_TYPE_IPV4_UC);
}

inline mesa_rc ip_expose_route_status_ipv6_itr(const ip_expose_route_status_key_ipv6_t *in, ip_expose_route_status_key_ipv6_t *out)
{
    return vtss_appl_ip_route_status_itr(in ? &in->key : nullptr, &out->key, VTSS_APPL_IP_ROUTE_TYPE_IPV6_UC);
}

inline mesa_rc ip_expose_route_status_ipv4_get(const ip_expose_route_status_key_ipv4_t *key, vtss_appl_ip_route_status_t *conf)
{
    return vtss_appl_ip_route_status_get(&key->key, conf);
}

inline mesa_rc ip_expose_route_status_ipv6_get(const ip_expose_route_status_key_ipv6_t *key, vtss_appl_ip_route_status_t *conf)
{
    return vtss_appl_ip_route_status_get(&key->key, conf);
}

extern const vtss_enum_descriptor_t ip_expose_dhcp4c_state_txt[];
extern const vtss_enum_descriptor_t ip_expose_dhcp4c_id_type_txt[];
extern const vtss_enum_descriptor_t ip_expose_route_protocol_txt[];

#endif  // _IP_EXPOSE_HXX_

