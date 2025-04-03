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

#ifndef __VTSS_APPL_JSON_RPC_NOTIFICATION_HXX__
#define __VTSS_APPL_JSON_RPC_NOTIFICATION_HXX__

#include <string>
#include "vtss/basics/vector.hxx"
#include "vtss/basics/intrusive_list.hxx"
#include "vtss/basics/notifications/event.hxx"
#include "vtss/appl/json_rpc_notification.h"

namespace vtss {
namespace appl {
namespace json_rpc_notification {

namespace Error {
enum E {
    GENERIC = MODULE_ERROR_START(VTSS_MODULE_ID_JSON_RPC_NOTIFICATION),
    DESTINATION_DOES_NOT_EXISTS,
    DESTINATION_TOO_MANY,
    EVENT_NO_SUBSCRIPTION,
    EVENT_DOES_NOT_EXISTS,
    EVENT_TOO_MANY,
    URL_INVALID,
    URL_PROTOCOL_NOT_SUPPORTED,
    URL_QUERY_NOT_ALLOWED,
    URL_FRAGMENT_NOT_ALLOWED,
    URL_USERINFO_NOT_ALLOWED,
};
}  // namespace Error

struct DestConf {
    std::string url;
    vtss_appl_json_rpc_notification_dest_auth_type_t auth_type;
    std::string user;
    std::string pass;
};

mesa_rc conv(const vtss_appl_json_rpc_notification_dest_conf_t *a, DestConf &b);
mesa_rc conv(const DestConf &a, vtss_appl_json_rpc_notification_dest_conf_t *b);

mesa_rc conv(const vtss_appl_json_rpc_notification_dest_name_t *a,
             std::string &b);
mesa_rc conv(const std::string &a,
             vtss_appl_json_rpc_notification_dest_name_t *b);

mesa_rc conv(const vtss_appl_json_rpc_notification_event_name_t *a,
             std::string &b);
mesa_rc conv(const std::string &a,
             vtss_appl_json_rpc_notification_event_name_t *b);

mesa_rc dest_get(const std::string &dest_name, DestConf &conf);

mesa_rc dest_set(const std::string &dest_name, const DestConf &conf);

mesa_rc dest_del(const std::string &dest_name);

mesa_rc dest_itr(const std::string &in, std::string &out);

mesa_rc event_subscribe_add(const std::string &dest_name,
                            const std::string &event_name);

mesa_rc event_subscribe_del(const std::string &dest_name,
                            const std::string &event_name);

mesa_rc event_subscribe_get(const std::string &dest_name,
                            const std::string &event_name);

mesa_rc event_subscribe_itr(const std::string &dest_name_in,
                            std::string &dest_name_out,
                            const std::string &event_name_in,
                            std::string &event_name_out);

const Vector<std::string> *event_complete_db();

mesa_rc init(vtss_init_data_t *data);

}  // namespace vtss
}  // namespace appl
}  // namespace json_rpc_notification

#endif  // __VTSS_APPL_JSON_RPC_NOTIFICATION_API_HXX__
