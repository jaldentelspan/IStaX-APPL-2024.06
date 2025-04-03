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

#include "main.h"
#include <string.h>
#include "json_rpc_notification.hxx"
#include "json_rpc_notification_api.h"
#include "vtss/appl/json_rpc_notification.h"

namespace vtss {
namespace appl {
namespace json_rpc_notification {

static size_t strnlen(const char *c, size_t max) {
    const char *s;
    for (s = c; *s && max; ++s, max--) {
    }
    return (s - c);
}

mesa_rc conv(const char *c, size_t l, std::string &s) {
    if (strnlen(c, l + 1) > l) return VTSS_RC_ERROR;
    s.assign(c);
    return VTSS_RC_OK;
}

mesa_rc conv(const std::string &s, char *c, size_t l) {
    if (s.size() > l) return VTSS_RC_ERROR;
    strncpy(c, s.c_str(), l + 1);
    return VTSS_RC_OK;
}

mesa_rc conv(const vtss_appl_json_rpc_notification_dest_conf_t *a,
             DestConf &b) {
    if (!a) {
        b.url.clear();
        return VTSS_RC_OK;
    }

    VTSS_RC(conv(a->url, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_URL_LENGTH,
                 b.url));
    b.auth_type = a->auth_type;
    VTSS_RC(conv(a->user, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_USER_LENGTH,
                 b.user));
    VTSS_RC(conv(a->pass, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_PASS_LENGTH,
                 b.pass));
    return VTSS_RC_OK;
}

mesa_rc conv(const DestConf &a,
             vtss_appl_json_rpc_notification_dest_conf_t *b) {
    VTSS_RC(conv(a.url, b->url,
                 VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_URL_LENGTH));
    b->auth_type = a.auth_type;
    VTSS_RC(conv(a.user, b->user,
                 VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_USER_LENGTH));
    VTSS_RC(conv(a.pass, b->pass,
                 VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_PASS_LENGTH));
    return VTSS_RC_OK;
}

mesa_rc conv(const vtss_appl_json_rpc_notification_dest_name_t *a,
             std::string &b) {
    if (!a) {
        b.clear();
        return VTSS_RC_OK;
    }

    VTSS_RC(conv(a->name, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH, b));
    return VTSS_RC_OK;
}

mesa_rc conv(const std::string &a,
             vtss_appl_json_rpc_notification_dest_name_t *b) {
    VTSS_RC(conv(a, b->name, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH));
    return VTSS_RC_OK;
}

mesa_rc conv(const vtss_appl_json_rpc_notification_event_name_t *a,
             std::string &b) {
    if (!a) {
        b.clear();
        return VTSS_RC_OK;
    }

    VTSS_RC(conv(a->name, VTSS_APPL_JSON_RPC_NOTIFICATION_EVENT_LENGTH, b));
    return VTSS_RC_OK;
}

mesa_rc conv(const std::string &a,
             vtss_appl_json_rpc_notification_event_name_t *b) {
    VTSS_RC(conv(a, b->name, VTSS_APPL_JSON_RPC_NOTIFICATION_EVENT_LENGTH));
    return VTSS_RC_OK;
}

}  // namespace vtss
}  // namespace appl
}  // namespace json_rpc_notification

const vtss_enum_descriptor_t
        vtss_appl_json_rpc_notification_dest_auth_type_txt[] = {
                {VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_AUTH_TYPE_NONE, "none"},
                {VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_AUTH_TYPE_BASIC, "basic"},
                {}};

mesa_rc vtss_appl_json_rpc_notification_dest_get(
        const vtss_appl_json_rpc_notification_dest_name_t *const dest,
        vtss_appl_json_rpc_notification_dest_conf_t *const conf) {
    std::string d;
    vtss::appl::json_rpc_notification::DestConf c;
    VTSS_RC(vtss::appl::json_rpc_notification::conv(dest, d));
    VTSS_RC(vtss::appl::json_rpc_notification::dest_get(d, c));
    VTSS_RC(vtss::appl::json_rpc_notification::conv(c, conf));
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_json_rpc_notification_dest_set(
        const vtss_appl_json_rpc_notification_dest_name_t *const dest,
        const vtss_appl_json_rpc_notification_dest_conf_t *const conf) {
    std::string d;
    vtss::appl::json_rpc_notification::DestConf c;
    VTSS_RC(vtss::appl::json_rpc_notification::conv(dest, d));
    VTSS_RC(vtss::appl::json_rpc_notification::conv(conf, c));
    VTSS_RC(vtss::appl::json_rpc_notification::dest_set(d, c));
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_json_rpc_notification_dest_del(
        const vtss_appl_json_rpc_notification_dest_name_t *const dest) {
    std::string d;
    VTSS_RC(vtss::appl::json_rpc_notification::conv(dest, d));
    VTSS_RC(vtss::appl::json_rpc_notification::dest_del(d));
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_json_rpc_notification_dest_itr(
        const vtss_appl_json_rpc_notification_dest_name_t *const in,
        vtss_appl_json_rpc_notification_dest_name_t *const out) {
    std::string i, o;
    VTSS_RC(vtss::appl::json_rpc_notification::conv(in, i));
    VTSS_RC(vtss::appl::json_rpc_notification::dest_itr(i, o));
    VTSS_RC(vtss::appl::json_rpc_notification::conv(o, out));
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_json_rpc_notification_event_subscribe_get(
        const vtss_appl_json_rpc_notification_dest_name_t *const dest,
        const vtss_appl_json_rpc_notification_event_name_t *const event) {
    std::string e, d;
    VTSS_RC(vtss::appl::json_rpc_notification::conv(dest, d));
    VTSS_RC(vtss::appl::json_rpc_notification::conv(event, e));
    VTSS_RC(vtss::appl::json_rpc_notification::event_subscribe_get(d, e));
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_json_rpc_notification_event_subscribe_add(
        const vtss_appl_json_rpc_notification_dest_name_t *const dest,
        const vtss_appl_json_rpc_notification_event_name_t *const event) {
    std::string e, d;
    VTSS_RC(vtss::appl::json_rpc_notification::conv(dest, d));
    VTSS_RC(vtss::appl::json_rpc_notification::conv(event, e));
    VTSS_RC(vtss::appl::json_rpc_notification::event_subscribe_add(d, e));
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_json_rpc_notification_event_subscribe_del(
        const vtss_appl_json_rpc_notification_dest_name_t *const dest,
        const vtss_appl_json_rpc_notification_event_name_t *const event) {
    std::string e, d;
    VTSS_RC(vtss::appl::json_rpc_notification::conv(dest, d));
    VTSS_RC(vtss::appl::json_rpc_notification::conv(event, e));
    VTSS_RC(vtss::appl::json_rpc_notification::event_subscribe_del(d, e));
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_json_rpc_notification_event_subscribe_itr(
        const vtss_appl_json_rpc_notification_dest_name_t *const dest_in,
        vtss_appl_json_rpc_notification_dest_name_t *const dest_out,
        const vtss_appl_json_rpc_notification_event_name_t *const event_in,
        vtss_appl_json_rpc_notification_event_name_t *const event_out) {
    std::string ie, oe, id, od;
    VTSS_RC(vtss::appl::json_rpc_notification::conv(event_in, ie));
    VTSS_RC(vtss::appl::json_rpc_notification::conv(dest_in, id));
    VTSS_RC(vtss::appl::json_rpc_notification::event_subscribe_itr(id, od, ie,
                                                                   oe));
    VTSS_RC(vtss::appl::json_rpc_notification::conv(oe, event_out));
    VTSS_RC(vtss::appl::json_rpc_notification::conv(od, dest_out));
    return VTSS_RC_OK;
}

const char *vtss_json_rpc_notification_error_txt(mesa_rc rc) {
#define CASE(X)                                       \
    case vtss::appl::json_rpc_notification::Error::X: \
        return #X

    switch (rc) {
        CASE(GENERIC);
        CASE(DESTINATION_DOES_NOT_EXISTS);
        CASE(DESTINATION_TOO_MANY);
        CASE(EVENT_NO_SUBSCRIPTION);
        CASE(EVENT_DOES_NOT_EXISTS);
        CASE(EVENT_TOO_MANY);
        CASE(URL_INVALID);
        CASE(URL_PROTOCOL_NOT_SUPPORTED);
        CASE(URL_QUERY_NOT_ALLOWED);
        CASE(URL_FRAGMENT_NOT_ALLOWED);
        CASE(URL_USERINFO_NOT_ALLOWED);
    }
    return "UNKNOWN_ERROR";
#undef CASE
}

mesa_rc vtss_json_rpc_notification_init(vtss_init_data_t *data) {
    return vtss::appl::json_rpc_notification::init(data);
}
