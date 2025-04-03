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

#include "json_rpc_notification_serialize.hxx"

using namespace vtss;
using namespace vtss::expose::snmp;
using namespace vtss::appl::json_rpc_notification::interfaces;

VTSS_MIB_MODULE("jsonRpcNotificationMib", "JSON-RPC-NOTIFICATION",
                vtss_json_rpc_notification_mib_init,
                VTSS_MODULE_ID_JSON_RPC_NOTIFICATION, root, h) {
    h.add_history_element("201410030000Z", "Initial version.");
    h.description("Private JSON-RPC Notification MIB.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(ns, root, 1, "jsonRpcNotificationMibObjects");

namespace {
NS(ns_config, ns, 2, "jsonRpcNotificationConfig");
TableReadWriteAddDelete2<Dest> dest_(
        &ns_config, OidElement(1, "jsonRpcNotificationConfigDestinationTable"),
        OidElement(2, "jsonRpcNotificationConfigDestinationRowEditor"));

TableReadWriteAddDelete2<EventSubscribe> event_(
        &ns_config, OidElement(3, "jsonRpcNotificationConfigNotificationTable"),
        OidElement(4, "jsonRpcNotificationConfigNotificationRowEditor"));
}  // namespace

