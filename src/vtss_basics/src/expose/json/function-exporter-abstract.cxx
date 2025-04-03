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

#include <vtss/basics/trace_grps.hxx>
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_JSON

#include <vtss/basics/json-rpc.hxx>
#include <vtss/basics/trace_basics.hxx>
#include <vtss/basics/expose/json/exporter.hxx>
#include <vtss/basics/expose/json/function-exporter-abstract.hxx>

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#endif

namespace vtss {
namespace expose {
namespace json {

#ifdef VTSS_SW_OPTION_PRIV_LVL
static priv::E convert_priv(bool write, int type) {
    if (type == VTSS_PRIV_LVL_CONFIG_TYPE) {
        if (write) {
            return priv::CONF_RW;
        } else {
            return priv::CONF_RO;
        }
    } else if (type == VTSS_PRIV_LVL_STAT_TYPE) {
        if (write) {
            return priv::STATUS_RW;
        } else {
            return priv::STATUS_RO;
        }
    } else {
        return priv::NO_ACCESS;
    }
}
#endif

FunctionExporterAbstract::FunctionExporterAbstract(NamespaceNode *parent,
                                                   const char *n, const char *d,
                                                   bool priv_write,
                                                   int priv_type,
                                                   int priv_module)
    : Node(parent, n, d)
#ifdef VTSS_SW_OPTION_PRIV_LVL
      ,
      priv_type(convert_priv(priv_write, priv_type)),
      priv_module_id(priv_module)
#endif
{
}

vtss::json::Result::Code FunctionExporterAbstract::handle(int priv,
                                                          str method_name,
                                                          const Request *req,
                                                          ostreamBuf &out) {
    vtss::json::Result::Code ec;
    VTSS_BASICS_TRACE(NOISE) << "Handling request: " << method_name;

    // Check that a leaf node was requested
    if (method_name.begin() != method_name.end()) {
        VTSS_BASICS_TRACE(NOISE) << "Method not found: " << method_name;
        ec = vtss::json::Result::METHOD_NOT_FOUND;
        goto WriteResponseMsg;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    // Check permisions
    if (priv_module_id != -1) {
        switch (priv_type) {
        case priv::NO_ACCESS_CONTROL:
            break;
        case priv::CONF_RO:
            if (!vtss_priv_is_allowed_cro(priv_module_id, priv)) goto NoAccess;
            break;
        case priv::CONF_RW:
            if (!vtss_priv_is_allowed_crw(priv_module_id, priv)) goto NoAccess;
            break;
        case priv::STATUS_RO:
            if (!vtss_priv_is_allowed_sro(priv_module_id, priv)) goto NoAccess;
            break;
        case priv::STATUS_RW:
            if (!vtss_priv_is_allowed_srw(priv_module_id, priv)) goto NoAccess;
            break;
        default:
            goto NoAccess;
        }
    } else {
        VTSS_BASICS_TRACE(NOISE) << "No access control";
    }
#endif

    // will invoke the function
    {
        error_code = MESA_RC_OK;
        VTSS_BASICS_TRACE(NOISE) << "Executing";
        ec = exec(req, &out);
        VTSS_BASICS_TRACE(NOISE) << "Execution retuls: " << ec;

        if (ec == vtss::json::Result::OK) {
            // The complete message has been compossed by exec-function! The
            // composed message may either have been an error or a result
            // message.
            return ec;
        }
    }

    if (error_code != MESA_RC_OK) {
        VTSS_BASICS_TRACE(INFO) << "Request failed. Code: " << error_code
                                << " Rc: " << AsErrorCode(error_code)
                                << " Function-pointer: " << failing_function;
    } else {
        VTSS_BASICS_TRACE(INFO) << "Request failed. Code: " << error_code;
    }

WriteResponseMsg:
    // Compose a error message
    out.clear();

    {
        Exporter e(&out);
        int64_t ptr = (int64_t)failing_function;
        const char *s_parse_error = "Parse error";
        const char *s_invalid_request = "Invalid request";
        const char *s_method_not_found = "Method not found";
        const char *s_invalid_params = "Invalid params";
        const char *s_internal_error = "Internal error";

        Exporter::Map m = e.as_map();
        m.add_leaf(req->id, vtss::tag::Name("id"));
        m.add_leaf(lit_null, vtss::tag::Name("result"));

        Exporter::Map em = m.as_map(vtss::tag::Name("error"));
        em.add_leaf(ec, vtss::tag::Name("code"));
        if (error_code != MESA_RC_OK) {
#if defined(VTSS_USE_API_HEADERS)
            em.add_leaf(str(error_txt(error_code)), vtss::tag::Name("message"));
#else
            const char *e_txt = "Internal error";
            em.add_leaf(str(e_txt), vtss::tag::Name("message"));
#endif
            Exporter::Map dm = em.as_map(vtss::tag::Name("data"));
            dm.add_leaf(error_code, vtss::tag::Name("vtss-error-code"));
            dm.add_leaf(ptr, vtss::tag::Name("vtss-failing-function-ptr"));
#if defined(VTSS_USE_API_HEADERS)
            dm.add_leaf(VTSS_RC_GET_MODULE_ID(error_code),
                        vtss::tag::Name("vtss-failing-module"));
            dm.add_leaf(VTSS_RC_GET_MODULE_CODE(error_code),
                        vtss::tag::Name("vtss-module-code"));
#endif
        } else {
            switch (ec) {
            case vtss::json::Result::OK:
                break;

            case vtss::json::Result::PARSE_ERROR:
                em.add_leaf(str(s_parse_error), vtss::tag::Name("message"));
                break;

            case vtss::json::Result::INVALID_REQUEST:
                em.add_leaf(str(s_invalid_request), vtss::tag::Name("message"));
                break;

            case vtss::json::Result::METHOD_NOT_FOUND:
                em.add_leaf(str(s_method_not_found), vtss::tag::Name("message"));
                break;

            case vtss::json::Result::INVALID_PARAMS:
                em.add_leaf(str(s_invalid_params), vtss::tag::Name("message"));
                break;

            case vtss::json::Result::INTERNAL_ERROR:
                em.add_leaf(str(s_internal_error), vtss::tag::Name("message"));
                break;
            }

            Exporter::Map dm = em.as_map(vtss::tag::Name("data"));
            if (arguments_got != arguments_expect) {
                dm.add_leaf(arguments_expect,
                            vtss::tag::Name("vtss-argument-cnt-expect"));
                dm.add_leaf(arguments_got,
                            vtss::tag::Name("vtss-argument-cnt-actual"));
            } else {
                if (error_argument_index || ec ==  vtss::json::Result::INVALID_PARAMS)
                    dm.add_leaf(error_argument_index,
                                vtss::tag::Name("vtss-failing-argument-index"));
                if (argument_not_found && ec ==  vtss::json::Result::INVALID_PARAMS)
                    dm.add_leaf(str(argument_not_found),
                                vtss::tag::Name("vtss-argument-not-found"));
            }
        }
    }
    return ec;

#ifdef VTSS_SW_OPTION_PRIV_LVL
NoAccess:
    // Compose a error message
    out.clear();

    {
        Exporter e(&out);
        const char *e_txt = "Access denied";

        Exporter::Map m = e.as_map();
        m.add_leaf(req->id, vtss::tag::Name("id"));
        m.add_leaf(lit_null, vtss::tag::Name("result"));

        Exporter::Map em = m.as_map(vtss::tag::Name("error"));
        em.add_leaf(vtss::json::Result::METHOD_NOT_FOUND,
                    vtss::tag::Name("code"));
        em.add_leaf(str(e_txt), vtss::tag::Name("message"));
    }

    return vtss::json::Result::METHOD_NOT_FOUND;
#endif
}

}  // namespace json
}  // namespace expose

}  // namespace vtss
