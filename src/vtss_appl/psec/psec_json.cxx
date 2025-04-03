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

#include "psec_serializer.hxx"
#include "vtss/basics/expose/json.hxx"
#include "json_rpc_api.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::psec;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode PSEC_JSON_node_psec("portSecurity");
extern "C" void vtss_appl_psec_json_init(void) { json_node_add(&PSEC_JSON_node_psec); }

NS(PSEC_JSON_node_config,            PSEC_JSON_node_psec,    "config");
NS(PSEC_JSON_node_config_interface,  PSEC_JSON_node_config,  "interface");
NS(PSEC_JSON_node_status,            PSEC_JSON_node_psec,    "status");
NS(PSEC_JSON_node_status_global,     PSEC_JSON_node_status,  "global");
NS(PSEC_JSON_node_status_interface,  PSEC_JSON_node_status,  "interface");
NS(PSEC_JSON_node_control,           PSEC_JSON_node_psec,    "control");
NS(PSEC_JSON_node_control_global,    PSEC_JSON_node_control, "global");
NS(PSEC_JSON_node_control_interface, PSEC_JSON_node_control, "interface");

namespace vtss {
namespace appl {
namespace psec {

static StructReadOnly            <Capabilities>                capabilities                 (&PSEC_JSON_node_psec,              "capabilities");
static StructReadWrite           <ConfigGlobal>                config_global                (&PSEC_JSON_node_config,            "global");
static TableReadWriteNoNS        <ConfigInterface>             config_interface             (&PSEC_JSON_node_config_interface);
static TableReadWriteAddDelete   <ConfigInterfaceMac>          config_interface_mac         (&PSEC_JSON_node_config_interface,  "mac");
static StructReadOnly            <StatusGlobal>                status_global                (&PSEC_JSON_node_status_global,     "main");
static StructReadOnlyNotification<StatusGlobalNotification>    status_global_notification   (&PSEC_JSON_node_status_global,     "notification", &psec_global_notification_status);
static TableReadOnlyNoNS         <StatusInterface>             status_interface             (&PSEC_JSON_node_status_interface);
static TableReadOnlyNotification <StatusInterfaceNotification> status_interface_notification(&PSEC_JSON_node_status_interface,  "notification", &psec_interface_notification_status);
static TableReadOnly             <StatusInterfaceMac>          status_interface_mac         (&PSEC_JSON_node_status_interface,  "mac");
static StructWriteOnly           <ControlGlobalMacClear>       control_global_mac_clear     (&PSEC_JSON_node_control_global,    "mac_clear");

/******************************************************************************/
// psec_interface_status_mac_get_all_json()
/******************************************************************************/
mesa_rc psec_interface_status_mac_get_all_json(const vtss::expose::json::Request *req, vtss::ostreamBuf *os)
{
    typedef ResponseMap<ResponseMapRowMultiKeySingleVal> response_t;
    typedef typename response_t::handler_type            handler_t;

    vtss_appl_psec_mac_status_map_t                 result;
    vtss_appl_psec_mac_status_map_t::const_iterator mac_itr;
    HandlerFunctionExporterParser                   handler_in(req);
    response_t                                      response(os, req->id); // Exporter that output parameters are written to

    VTSS_RC(vtss_appl_psec_interface_status_mac_get_all(result));

    for (mac_itr = result.cbegin(); mac_itr != result.cend(); ++mac_itr) {
        handler_t handler_out = response.resultHandler();

        // Serialize the key
        {
            auto                         &&key_handler = handler_out.keyHandler();
            vtss_appl_psec_mac_map_key_t key           = mac_itr->first;

            serialize(key_handler, PSEC_SERIALIZER_ifindex_t(key.ifindex));
            serialize(key_handler, PSEC_SERIALIZER_vid_t(key.vlan));
            serialize(key_handler, PSEC_SERIALIZER_mac_t(key.mac));
            if (!key_handler.ok()) {
                return VTSS_RC_ERROR;
            }
        }

        // Serialize the value
        {
            auto                        &&val_handler = handler_out.valHandler();
            vtss_appl_psec_mac_status_t value         = mac_itr->second;
            serialize(val_handler, value);

            if (!val_handler.ok()) {
                return VTSS_RC_ERROR;
            }
        }
    }

    return VTSS_RC_OK;
}

}  // namespace psec
}  // namespace appl
}  // namespace vtss

