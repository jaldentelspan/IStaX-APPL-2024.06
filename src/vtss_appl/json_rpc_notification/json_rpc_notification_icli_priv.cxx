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

#include <string>
#include "json_rpc_trace.h"
#include "vtss/basics/trace.hxx"
#include "json_rpc_notification.hxx"
#include "json_rpc_notification_icli_priv.h"
#include "vtss/appl/json_rpc_notification.h"
#include "icfg_api.h"

#define COPY_STR(FROM, TO, MAX)                       \
    if (FROM) {                                       \
        if (strlen(FROM) > MAX) return VTSS_RC_ERROR; \
        strncpy(TO, FROM, MAX + 1);                   \
    } else {                                          \
        TO[0] = 0;                                    \
    }

#define CLEAR_STR(S, MAX) memset(S, 0, MAX + 1);

#define PRINTF(...) (void) vtss_icfg_printf(result, __VA_ARGS__);

#define TRACE(X) VTSS_TRACE(VTSS_TRACE_JSON_RPC_GRP_NOTI, X)

mesa_rc JSON_RPC_icli_event_host_add(const char *host) {
    vtss_appl_json_rpc_notification_dest_name_t n = {};
    vtss_appl_json_rpc_notification_dest_conf_t c = {};

    COPY_STR(host, n.name, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH);

    if (vtss_appl_json_rpc_notification_dest_get(&n, &c) == VTSS_RC_OK)
        return VTSS_RC_OK;

    return vtss_appl_json_rpc_notification_dest_set(&n, &c);
}

mesa_rc JSON_RPC_icli_event_host_del(const char *host) {
    vtss_appl_json_rpc_notification_dest_name_t n = {};

    COPY_STR(host, n.name, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH);

    return vtss_appl_json_rpc_notification_dest_del(&n);
}

mesa_rc JSON_RPC_icli_event_host_next(char *host) {
    vtss_appl_json_rpc_notification_dest_name_t a, b;
    strcpy(a.name, host);

    if (vtss_appl_json_rpc_notification_dest_itr(&a, &b) == VTSS_RC_OK) {
        strcpy(host, b.name);
        return VTSS_RC_OK;
    } else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc JSON_RPC_icli_event_host_url_set(const char *host, const char *url) {
    vtss_appl_json_rpc_notification_dest_name_t n = {};
    vtss_appl_json_rpc_notification_dest_conf_t c = {};

    // Prepare query ///////////////////////////////////////////////////////////
    COPY_STR(host, n.name, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH);

    // Get /////////////////////////////////////////////////////////////////////
    VTSS_RC(vtss_appl_json_rpc_notification_dest_get(&n, &c));

    // Update //////////////////////////////////////////////////////////////////
    COPY_STR(url, c.url, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_URL_LENGTH);

    // Set /////////////////////////////////////////////////////////////////////
    return vtss_appl_json_rpc_notification_dest_set(&n, &c);
}

mesa_rc JSON_RPC_icli_event_host_url_del(const char *host) {
    vtss_appl_json_rpc_notification_dest_name_t n = {};
    vtss_appl_json_rpc_notification_dest_conf_t c = {};

    // Prepare query ///////////////////////////////////////////////////////////
    COPY_STR(host, n.name, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH);

    // Get /////////////////////////////////////////////////////////////////////
    VTSS_RC(vtss_appl_json_rpc_notification_dest_get(&n, &c));

    // Update //////////////////////////////////////////////////////////////////
    CLEAR_STR(c.url, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_URL_LENGTH);

    // Set /////////////////////////////////////////////////////////////////////
    return vtss_appl_json_rpc_notification_dest_set(&n, &c);
}

mesa_rc JSON_RPC_icli_event_host_auth_basic_set(const char *host, const char *u,
                                                const char *p) {
    vtss_appl_json_rpc_notification_dest_name_t n = {};
    vtss_appl_json_rpc_notification_dest_conf_t c = {};

    // Prepare query ///////////////////////////////////////////////////////////
    COPY_STR(host, n.name, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH);

    // Get /////////////////////////////////////////////////////////////////////
    VTSS_RC(vtss_appl_json_rpc_notification_dest_get(&n, &c));

    // Update //////////////////////////////////////////////////////////////////
    c.auth_type = VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_AUTH_TYPE_BASIC;
    COPY_STR(u, c.user, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_USER_LENGTH);
    COPY_STR(p, c.pass, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_PASS_LENGTH);

    // Set /////////////////////////////////////////////////////////////////////
    return vtss_appl_json_rpc_notification_dest_set(&n, &c);
}

mesa_rc JSON_RPC_icli_event_host_auth_basic_del(const char *host) {
    vtss_appl_json_rpc_notification_dest_name_t n = {};
    vtss_appl_json_rpc_notification_dest_conf_t c = {};

    // Prepare query ///////////////////////////////////////////////////////////
    COPY_STR(host, n.name, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH);

    // Get /////////////////////////////////////////////////////////////////////
    VTSS_RC(vtss_appl_json_rpc_notification_dest_get(&n, &c));

    // Update //////////////////////////////////////////////////////////////////
    c.auth_type = VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_AUTH_TYPE_NONE;
    CLEAR_STR(c.user, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_USER_LENGTH);
    CLEAR_STR(c.pass, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_PASS_LENGTH);

    // Set /////////////////////////////////////////////////////////////////////
    return vtss_appl_json_rpc_notification_dest_set(&n, &c);
}

mesa_rc JSON_RPC_icli_event_listen_add(const char *name, const char *host) {
    vtss_appl_json_rpc_notification_dest_name_t n = {};
    vtss_appl_json_rpc_notification_event_name_t e = {};

    COPY_STR(host, n.name, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH);
    COPY_STR(name, e.name, VTSS_APPL_JSON_RPC_NOTIFICATION_EVENT_LENGTH);

    return vtss_appl_json_rpc_notification_event_subscribe_add(&n, &e);
}

mesa_rc JSON_RPC_icli_event_listen_del(const char *name, const char *host) {
    vtss_appl_json_rpc_notification_dest_name_t n = {};
    vtss_appl_json_rpc_notification_event_name_t e = {};

    COPY_STR(host, n.name, VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_NAME_LENGTH);
    COPY_STR(name, e.name, VTSS_APPL_JSON_RPC_NOTIFICATION_EVENT_LENGTH);

    return vtss_appl_json_rpc_notification_event_subscribe_del(&n, &e);
}

BOOL runtime_cword_json_rpc_notifications_events(u32 session_id,
                                                 icli_runtime_ask_t ask,
                                                 icli_runtime_t *runtime) {
    if (runtime == NULL) return FALSE;
    if (ask != ICLI_ASK_CWORD) return FALSE;

    auto db = vtss::appl::json_rpc_notification::event_complete_db();
    if (db->size() > ICLI_CWORD_MAX_CNT) {
        TRACE(ERROR) << "auto complete DB is too big: " << db->size() << "/"
                     << ICLI_CWORD_MAX_CNT;
        return FALSE;
    }

    for (size_t i = 0; i < db->size(); i++) {
        TRACE(INFO) << "Add callback: " << i << " " << db->at(i)->c_str();
        runtime->cword[i] = (char *)db->at(i)->c_str();
    }

    return TRUE;
}

mesa_rc JSON_RPC_NOTIFICATION_icfg_(const vtss_icfg_query_request_t *req,
                                    vtss_icfg_query_result_t *result) {
    switch (req->cmd_mode) {
    case ICLI_CMD_MODE_JSON_NOTI_HOST: {
        vtss_appl_json_rpc_notification_dest_name_t a;
        vtss_appl_json_rpc_notification_dest_conf_t c;
        strcpy(a.name, req->instance_id.string);

        if (vtss_appl_json_rpc_notification_dest_get(&a, &c) == VTSS_RC_OK) {
            if (strlen(c.url)) PRINTF(" url %s\n", c.url);
            switch (c.auth_type) {
            case VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_AUTH_TYPE_BASIC:
                PRINTF(" authentication basic username %s password %s\n",
                       c.user, c.pass);
                break;

            case VTSS_APPL_JSON_RPC_NOTIFICATION_DEST_AUTH_TYPE_NONE:
                break;
            }
        }

        break;
    }

    case ICLI_CMD_MODE_GLOBAL_CONFIG: {
        vtss_appl_json_rpc_notification_dest_name_t a1, b1;
        vtss_appl_json_rpc_notification_dest_name_t *pa1 = nullptr, *pb1 = &b1;
        vtss_appl_json_rpc_notification_event_name_t a2, b2;
        vtss_appl_json_rpc_notification_event_name_t *pa2 = nullptr, *pb2 = &b2;

        while (vtss_appl_json_rpc_notification_event_subscribe_itr(
                       pa1, pb1, pa2, pb2) == VTSS_RC_OK) {
            PRINTF("json notification listen %s %s\n", pb2->name, pb1->name);
            pa1 = (pa1 == &b1 ? &a1 : &b1);
            pb1 = (pb1 == &b1 ? &a1 : &b1);
            pa2 = (pa2 == &b2 ? &a2 : &b2);
            pb2 = (pb2 == &b2 ? &a2 : &b2);
        }

        break;
    }

    default:
        // Not needed
        break;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_json_rpc_notification_icfg_init() {
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_JSON_NOTI_HOST,
                                     "json_rpc_notification",
                                     JSON_RPC_NOTIFICATION_icfg_));

    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_JSON_NOTI_LISTEN,
                                     "json_rpc_notification",
                                     JSON_RPC_NOTIFICATION_icfg_));

    return VTSS_RC_OK;
}

