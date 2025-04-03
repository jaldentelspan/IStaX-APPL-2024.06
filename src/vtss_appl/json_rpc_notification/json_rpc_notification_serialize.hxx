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

#ifndef __JSON_RPC_NOTIFICATION_SERIALIZE_HXX__
#define __JSON_RPC_NOTIFICATION_SERIALIZE_HXX__

#include "vtss/appl/json_rpc_notification.h"
#include "vtss_appl_serialize.hxx"
#include "vtss/basics/expose.hxx"
#include "vtss/appl/module_id.h"

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_json_rpc_notification_dest_auth_type_t,
                         "JsonRpcNotificationDestAuthType",
                         vtss_appl_json_rpc_notification_dest_auth_type_txt,
                         "Type of authentication (if any).");

namespace vtss {
namespace appl {
namespace json_rpc_notification {
namespace interfaces {

struct Dest {
    typedef expose::ParamList<
            expose::ParamKey<vtss_appl_json_rpc_notification_dest_name_t *>,
            expose::ParamVal<vtss_appl_json_rpc_notification_dest_conf_t *>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
            "Table of JSON-RPC Notification destinations.";
    static constexpr const char *index_description =
            "Entries in this table represent a JSON-RPC Notification "
            "destination which can be referred to in the notification "
            "subscription table.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(
            vtss_appl_json_rpc_notification_dest_name_t &i) {
        h.add_leaf(AsDisplayString(i.name, sizeof(i.name)), tag::Name("name"),
                   expose::snmp::OidElementValue(1),
                   tag::Description("Name of destination"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(
            vtss_appl_json_rpc_notification_dest_conf_t &i) {
        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename(
                "vtss_appl_json_rpc_notification_dest_conf_t "));

        m.add_leaf(AsDisplayString(i.url, sizeof(i.url)), tag::Name("url"),
                   expose::snmp::OidElementValue(2),
                   tag::Description(
                           "URL of the destination where the events are "
                           "delivered to."));

        m.add_leaf(i.auth_type, tag::Name("authType"), expose::snmp::OidElementValue(3),
                   tag::Description("Type of authentication to use (if any)"));

        m.add_leaf(
                AsDisplayString(i.user, sizeof(i.user)), tag::Name("username"),
                expose::snmp::OidElementValue(4),
                tag::Description("User name for the authentication process"));

        m.add_leaf(AsDisplayString(i.pass, sizeof(i.pass)),
                   expose::snmp::OidElementValue(5), tag::Name("password"),
                   tag::Description("Password for the authentication process"));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_json_rpc_notification_dest_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_json_rpc_notification_dest_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_json_rpc_notification_dest_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_json_rpc_notification_dest_del);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_json_rpc_notification_dest_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_JSON_RPC);
};

struct EventSubscribe {
    typedef expose::ParamList<
            expose::ParamKey<vtss_appl_json_rpc_notification_dest_name_t *>,
            expose::ParamKey<vtss_appl_json_rpc_notification_event_name_t *>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
            "Table of JSON-RPC Notifications subscriptions.";
    static constexpr const char *index_description =
            "Each entry represents a subscription of a given notification to a "
            "given destination. If the corresponding destination is deleted, "
            "then subscription is deleted as well.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(
            vtss_appl_json_rpc_notification_dest_name_t &i) {
        h.add_leaf(AsDisplayString(i.name, sizeof(i.name)),
                   expose::snmp::OidElementValue(1), tag::Name("destination"),
                   tag::Description("Name of destination"));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(
            vtss_appl_json_rpc_notification_event_name_t &i) {
        h.add_leaf(AsDisplayString(i.name, sizeof(i.name)),
                   expose::snmp::OidElementValue(2), tag::Name("notification"),
                   tag::Description("Name of notification"));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_json_rpc_notification_event_subscribe_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_json_rpc_notification_event_subscribe_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_json_rpc_notification_event_subscribe_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_json_rpc_notification_event_subscribe_del);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_json_rpc_notification_event_subscribe_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_JSON_RPC);
};

}  // namespace interfaces
}  // namespace json_rpc_notification
}  // namespace appl
}  // namespace vtss

#endif  // __JSON_RPC_NOTIFICATION_SERIALIZE_HXX__
