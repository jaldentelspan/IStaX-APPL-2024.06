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

#include "syslog_serializer.hxx"
#include "vtss/basics/expose/json.hxx"
#include "syslog_api.h"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose;
using namespace vtss::expose::json;
using namespace vtss::appl::syslog::interfaces;

mesa_rc vtss_appl_syslog_history_get_all(const vtss::expose::json::Request *req,
                                       vtss::ostreamBuf *os)
{
    HandlerFunctionExporterParser handler_in(req);
    /*
        Single   key use ResponseMapRowSingleKeySingleVal
        Multiple key use ResponseMapRowMultiKeySingleVal
    */
    typedef ResponseMap<ResponseMapRowMultiKeySingleVal> response_type;

    // The handler type is derived from the response type.
    typedef typename response_type::handler_type handler_type;

    // Create an exporter which the output parameters is written to
    response_type response(os, req->id);

    vtss_appl_syslog_history_t history;
    vtss_isid_t isid  = VTSS_ISID_START;
    uint32_t    SL_id = 0;
    void        *ptr  = NULL;

    do {
        if (vtss_appl_syslog_history_get_all_by_step(&isid, &SL_id, &ptr, &history) == VTSS_RC_OK) {
            handler_type handler_out = response.resultHandler();

            // add the switch_id and syslog_entry_id as key
            {
                auto &&key_handler = handler_out.keyHandler();

                serialize(key_handler, syslog_history_tbl_idx_usid(isid));
                serialize(key_handler, syslog_history_tbl_idx_msg_id(SL_id));

                if (!key_handler.ok()) {
                    return VTSS_RC_ERROR;
                }
            }

            // serialize the value (as normal)
            {
                auto &&val_handler = handler_out.valHandler();
                serialize(val_handler, history);
                if (!val_handler.ok()) {
                    return VTSS_RC_ERROR;
                }
            }
        }
    } while(ptr != NULL);

    return VTSS_RC_OK;
}

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_syslog("syslog");
extern "C" void vtss_appl_syslog_json_init() { json_node_add(&ns_syslog); }

NS(ns_conf,       ns_syslog, "config");
NS(ns_status,     ns_syslog, "status");
NS(ns_statistics, ns_syslog, "statistics");
NS(ns_control,    ns_syslog, "control");

static StructReadWrite<SyslogConfigGlobals> syslog_config_globals(
        &ns_conf, "server");

#ifdef VTSS_SW_OPTION_JSON_RPC_NOTIFICATION
static TableReadWriteAddDelete<SyslogConfigNotifications> syslog_config_notifications(
        &ns_conf, "notifications");
#endif //VTSS_SW_OPTION_JSON_RPC_NOTIFICATION

static TableReadOnly<SyslogStatusHistoryTbl> syslog_status_history_tbl(
        &ns_status, "history");

static TableReadWrite<SyslogControlHistoryTbl> syslog_control_history_tbl(
        &ns_control, "history");

