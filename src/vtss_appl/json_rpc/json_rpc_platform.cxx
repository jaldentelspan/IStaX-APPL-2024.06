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

#include "json_rpc_trace.h"

#include "main.h"
#include "vtss_trace_api.h"

#include "fast_cgi_api.hxx"

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#ifdef VTSS_SW_OPTION_WEB
#include "vtss_privilege_web_api.h"
#endif
#endif

#include "json_rpc_api.hxx"
#include "vtss_json_rpc_api.h"

#include <string>
#include "vtss/basics/trace.hxx"
#include "vtss/basics/vector.hxx"
#include "vtss/basics/memory.hxx"
#include "vtss/basics/json-rpc-function.hxx"
#include "vtss/basics/expose/json/root-node.hxx"
#include "vtss/basics/expose/json/method-split.hxx"
#include "vtss/basics/expose/json/namespace-node.hxx"
#include "vtss/basics/expose/json/json-value-ptr-impl.hxx"
#include "vtss/basics/expose/json/specification/walk.hxx"
#include "vtss/basics/expose/json/specification/reflector-echo.hxx"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_JSON_RPC


namespace vtss {

static vtss_trace_reg_t trace_reg =
{
    VTSS_MODULE_ID_JSON_RPC, "json_rpc", "json-rpc"
};

static vtss_trace_grp_t trace_grps[] =
{
    [VTSS_TRACE_JSON_RPC_GRP_DEFAULT] = { 
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
    [VTSS_TRACE_JSON_RPC_GRP_MSG_DUMP] = { 
        "msg",
        "msg",
        VTSS_TRACE_LVL_ERROR
    },
    [VTSS_TRACE_JSON_RPC_GRP_NOTI] = { 
        "event",
        "notifications",
        VTSS_TRACE_LVL_ERROR
    },
    [VTSS_TRACE_JSON_RPC_GRP_NOTI_ASYNC] = { 
        "async_event",
        "async notifications",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

class json_http_req {
  public:
    json_http_req(CYG_HTTPD_STATE *init);
    ~json_http_req();
    bool is_post();
    str post_data();
    BufPtr output_buf();
    void write(std::string &s);

  private:
    CYG_HTTPD_STATE *req;
};

json_http_req::json_http_req(CYG_HTTPD_STATE *init) { req = init; }

json_http_req::~json_http_req() {}

bool json_http_req::is_post() { return (req->method == CYG_HTTPD_METHOD_POST); }

str json_http_req::post_data() {
    return str(req->post_data, req->post_data + req->content_len);
}

BufPtr json_http_req::output_buf() {
    return BufPtr(req->outbuffer, req->outbuffer + CYG_HTTPD_MAXOUTBUFFER);
}

void json_http_req::write(std::string &s) {
    size_t cnt = 0;
    ssize_t res = 0;

    while (cnt < s.size()) {
        res = cyg_httpd_write((&*s.begin()) + cnt, s.size() - cnt);

        if (res >= 0) {
            cnt += res;
        } else {
            T_I("Failed to write msg res=" VPRIz ", cnt=" VPRIz ", size=" VPRIz,
                res, cnt, s.size());
            return;
        }
    }
}

vtss::expose::json::RootNode JSON_RPC_ROOT;

std::shared_ptr<expose::json::specification::Inventory>
    JSON_RPC_ROOT_INVENTORY_SPECIFIC;
std::shared_ptr<expose::json::specification::Inventory> JSON_RPC_ROOT_INVENTORY;

static JsonInventoryPtr json_rpc_inventory_get_(
        bool specific,
        std::shared_ptr<expose::json::specification::Inventory> &cache) {
    if (cache) {
        VTSS_TRACE(DEBUG) << "Returning cached inventory";
        return cache;
    }

    VTSS_TRACE(DEBUG) << "No cache, building new inventory";
    auto inv = make_unique<expose::json::specification::Inventory>();
    for (auto &i : JSON_RPC_ROOT.leafs) {
        walk(*inv.get(), &i, specific);
    }

    VTSS_TRACE(DEBUG) << "New inventory ready";
    cache = vtss::move(inv);

    return cache;
}

JsonInventoryPtr json_rpc_inventory_get(bool specific) {
    if (specific)
        return json_rpc_inventory_get_(specific,
                                       JSON_RPC_ROOT_INVENTORY_SPECIFIC);
    else
        return json_rpc_inventory_get_(specific,
                                       JSON_RPC_ROOT_INVENTORY);
}

void json_node_add(vtss::expose::json::Node *node) {
    // TODO, mutex
    JSON_RPC_ROOT_INVENTORY.reset();
    JSON_RPC_ROOT_INVENTORY_SPECIFIC.reset();
    node->attach_to(&JSON_RPC_ROOT);
}


namespace appl {
namespace json_rpc {

mesa_rc inventory_get(const std::string *s,
                      expose::json::specification::Inventory *const inv,
                      bool specific) {
    typedef vtss::intrusive::List<vtss::expose::json::Node>::iterator I;
    str q(s->c_str());
    str method_head, method_tail;
    expose::json::NamespaceNode *ns = &JSON_RPC_ROOT;
    expose::json::Node *node = ns;

    VTSS_TRACE(DEBUG) << "inventory_get: " << q;

    Vector<str> stack;
    Vector<std::string> stack_groups;

    while (q.size()) {
        expose::json::method_split(q, method_head, method_tail);
        VTSS_TRACE(NOISE) << "Method name: " << q << " head: " << method_head
                          << " tail: " << method_tail;

        if (node->is_namespace_node()) {
            ns = static_cast<expose::json::NamespaceNode *>(node);
        } else {
            VTSS_TRACE(DEBUG) << "Could not find " << s->c_str();
            return VTSS_RC_ERROR;
        }

        if (ns != &JSON_RPC_ROOT) stack.push_back(ns->name());

        node = nullptr;
        for (I i = ns->leafs.begin(); i != ns->leafs.end(); ++i) {
            if (method_head == i->name()) {
                VTSS_TRACE(DEBUG) << "Match: " << method_head;
                node = &*i;
                break;

            } else {
                VTSS_TRACE(NOISE) << "No match: " << i->name();
            }
        }

        if (!node) {
            VTSS_TRACE(DEBUG) << "Could not find " << s->c_str();
            return VTSS_RC_ERROR;
        }

        q = method_tail;
    }

    if (node->is_namespace_node()) {
        ns = static_cast<expose::json::NamespaceNode *>(node);

        if (ns != &JSON_RPC_ROOT) stack.push_back(ns->name());

        for (I i = ns->leafs.begin(); i != ns->leafs.end(); ++i)
            expose::json::specification::walk_(*inv, &*i, stack, stack_groups,
                                               specific);

    } else {
        std::string s, p;

        stack.push_back(node->name());
        for (auto n : stack) {
            if (s.size() != 0) s.append(".");
            s.append(n.begin(), n.size());
        }

        if (stack_groups.size()) p = stack_groups.back();
        expose::json::specification::ReflectorEcho r(*inv, s, p, node, specific);
        node->handle_reflection(&r);
    }

    return VTSS_RC_OK;
}

mesa_rc inventory_generic_get(const std::string *s,
                              expose::json::specification::Inventory *const i) {
    return inventory_get(s, i, false);
}

mesa_rc inventory_specific_get(const std::string *s,
                               expose::json::specification::Inventory *const i) {
    return inventory_get(s, i, true);
}


mesa_rc nodes_get(const std::string *s, Vector<std::string> *n, bool specific) {
    typedef vtss::intrusive::List<vtss::expose::json::Node>::iterator I;
    str q(s->c_str());
    str method_head, method_tail;
    expose::json::NamespaceNode *ns = &JSON_RPC_ROOT;
    expose::json::Node *node = ns;

    VTSS_TRACE(DEBUG) << "nodes_get: " << q;

    Vector<str> stack;

    while (q.size()) {
        expose::json::method_split(q, method_head, method_tail);
        VTSS_TRACE(NOISE) << "Method name: " << q << " head: " << method_head
                          << " tail: " << method_tail;

        if (node->is_namespace_node()) {
            ns = static_cast<expose::json::NamespaceNode *>(node);
        } else {
            VTSS_TRACE(DEBUG) << "Could not find " << s->c_str();
            return VTSS_RC_ERROR;
        }

        if (ns != &JSON_RPC_ROOT) stack.push_back(ns->name());

        node = nullptr;
        for (I i = ns->leafs.begin(); i != ns->leafs.end(); ++i) {
            if (method_head == i->name()) {
                VTSS_TRACE(DEBUG) << "Match: " << method_head;
                node = &*i;
                break;

            } else {
                VTSS_TRACE(NOISE) << "No match: " << i->name();
            }
        }

        if (!node) {
            VTSS_TRACE(DEBUG) << "Could not find " << s->c_str();
            return VTSS_RC_ERROR;
        }

        q = method_tail;
    }


    if (node->is_namespace_node()) {
        ns = static_cast<expose::json::NamespaceNode *>(node);
        if (ns != &JSON_RPC_ROOT) stack.push_back(ns->name());

        std::string base;
        for (auto n : stack) {
            if (base.size() != 0) base.append(".");
            base.append(n.begin(), n.size());
        }

        for (I i = ns->leafs.begin(); i != ns->leafs.end(); ++i) {
            std::string s = base;
            if (s.size() != 0) s.append(".");
            s.append(i->name().begin(), i->name().size());
            n->push_back(s);
        }
    }

    return VTSS_RC_OK;
}

mesa_rc nodes_generic_get(const std::string *q, Vector<std::string> *nodes) {
    return nodes_get(q, nodes, false);
}

}  // namespace json_rpc
}  // namespace appl


bool write_hdr(ostream &o, json::Result::Code res, size_t length) {
#define HTTP_RC_FORMAT(_c, _t) "Status: " _c " " _t "\r\n"
    o << "HTTP/1.1 ";
    switch (res) {
    case json::Result::OK:
        o << HTTP_RC_FORMAT("200", "OK");
        break;

    case json::Result::INVALID_REQUEST:
        o << HTTP_RC_FORMAT("400", "Bad Request");
        break;

    case json::Result::METHOD_NOT_FOUND:
        o << HTTP_RC_FORMAT("404", "Not Found");
        break;

    case json::Result::PARSE_ERROR:
    case json::Result::INVALID_PARAMS:
    case json::Result::INTERNAL_ERROR:
    default:
        o << HTTP_RC_FORMAT("500", "Internal Server Error");
        break;
    }

    o << "Content-Type: application/json\r\n";
    o << "Cache-Control: no-cache\r\n";
    o << "Content-Length: " << length << "\r\n";
    o << "\r\n";

    return o.ok();
}

static i32 json_http_handler(CYG_HTTPD_STATE *p) {
    json_http_req req(p);

// TODO, mutex
#if defined(VTSS_SW_OPTION_PRIV_LVL) && \
        (defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_FAST_CGI))
    int priv_lvl = cyg_httpd_current_privilege_level();
#else
    int priv_lvl = 0x7fffffff;
#endif

    T_I("Handling json-rpc request");

    json::Result::Code res;

    str input(req.post_data());
    StringStream output;
    StringStream header;

    if (!req.is_post()) {
        // TODO, set http error code
        T_I("Only post method is supported for json rpc.");
        res = json::Result::INVALID_REQUEST;
        goto END;
    }

    T_I("Got " VPRIz " bytes of input", input.size());

    // Porcess the input request and write the result to the output
    res = JSON_RPC_ROOT.process(priv_lvl, input, output);

    if (res != json::Result::OK)
        VTSS_TRACE(NOISE) << "Failed to process JSON "
                          << "To be fixed: p->post_data";

    // Check for overflows
    if (!output.ok()) {
        T_I("Output overflow");
        output.clear();
        res = json::Result::INTERNAL_ERROR;
        goto END;
    }

    T_I("Result code: %d, output size: " VPRIz, res, output.buf.size());

END:
    if (!write_hdr(header, res, output.buf.size())) {
        T_I("Output overflow");
        header.clear();
        output.clear();

        if (!write_hdr(header, json::Result::INTERNAL_ERROR, output.buf.size())) {
            T_I("Output overflow - AGAIN!!!");
            return -1;
        }
    }

    req.write(header.buf);
    if (output.buf.size()) {
        req.write(output.buf);
    }
    T_I("Done");
    return -1;
}
CYG_HTTPD_HANDLER_TABLE_ENTRY_JSON(json_cb, "/json_rpc", json_http_handler);

static i32 json_http_handler_spec(CYG_HTTPD_STATE *p, bool specific) {
    json_http_req req(p);

    T_I("Handling json-spec request");
    json::Result::Code res = json::Result::Code::OK;
    str input(req.post_data());

    vtss::StringStream output;
    vtss::StringStream header;

    auto inv = json_rpc_inventory_get(specific);
    output << *inv;

    T_I("Result code: %d, output size: " VPRIz, res, output.buf.size());

    T_I("Got %u bytes of input", p->content_len);

    if (!write_hdr(header, res, output.buf.size())) {
        T_I("Output overflow");
        output.clear();
        header.clear();

        if (!write_hdr(output, json::Result::INTERNAL_ERROR, output.buf.size())) {
            T_I("Output overflow - AGAIN!!!");
            return -1;
        }
    }
    req.write(header.buf);
    if (output.buf.size()) {
        req.write(output.buf);
    }

    return -1;
}

static i32 json_http_handler_spec_generic(CYG_HTTPD_STATE *p) {
    return json_http_handler_spec(p, false);
}

static i32 json_http_handler_spec_specific(CYG_HTTPD_STATE *p) {
    return json_http_handler_spec(p, true);
}

CYG_HTTPD_HANDLER_TABLE_ENTRY_JSON(json_cb_spec_generic, "/json_spec",
                                   json_http_handler_spec_generic);
CYG_HTTPD_HANDLER_TABLE_ENTRY_JSON(json_cb_spec_specific,
                                   "/json_spec_target_specific",
                                   json_http_handler_spec_specific);
}  // namespace vtss

////////////////////////////////////////////////////////////////////////////////
const char *vtss_json_rpc_error_txt(mesa_rc rc) { return 0; }
void vtss_appl_json_rpc_json_init();

mesa_rc vtss_json_rpc_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_ICFG_LOADING_PRE) {
        VTSS_TRACE(INFO) << "ICFG_LOADING_PRE";
        vtss_appl_json_rpc_json_init();
    }

    return VTSS_RC_OK;
}
