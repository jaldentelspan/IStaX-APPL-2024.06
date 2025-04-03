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

#ifndef __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_MAP_CUSTOM_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_MAP_CUSTOM_HXX__

#include <vtss/basics/trace_grps.hxx>
#include <vtss/basics/trace_basics.hxx>
#include <vtss/basics/expose/json/function-exporter-get.hxx>

#include <vtss/basics/expose/json/response-map.hxx>
#include <vtss/basics/expose/json/response-map-row-single-key-single-val.hxx>
#include <vtss/basics/expose/json/response-map-row-single-key-multi-val.hxx>
#include <vtss/basics/expose/json/response-map-row-multi-key-single-val.hxx>
#include <vtss/basics/expose/json/response-map-row-multi-key-multi-val.hxx>

#define I VTSS_BASICS_TRACE(113, VTSS_BASICS_TRACE_GRP_JSON, INFO)

namespace vtss {
namespace expose {
namespace json {

template <class INTERFACE, class SUBJECT>
struct FunctionExporterGetMapCustom
        : public FunctionExporterGet<INTERFACE, SUBJECT> {
    typedef ResponseMap<ResponseMapRowSingleKeySingleVal> SingleKeySingleVal_t;
    typedef ResponseMap<ResponseMapRowSingleKeyMultiVal> SingleKeyMultiVal_t;
    typedef ResponseMap<ResponseMapRowMultiKeySingleVal> MultiKeySingleVal_t;
    typedef ResponseMap<ResponseMapRowMultiKeyMultiVal> MultiKeyMultiVal_t;

    FunctionExporterGetMapCustom(SUBJECT *subject, NamespaceNode *ns,
                                 const char *n, const char *d)
        : FunctionExporterGet<INTERFACE, SUBJECT>(subject, ns, n, d) {}

    vtss::json::Result::Code exec_all(const Request *req, ostreamBuf *os) {
        if (INTERFACE::json_get_all(req, os) == MESA_RC_OK)
            return vtss::json::Result::OK;
        return vtss::json::Result::INTERNAL_ERROR;
    }

    vtss::json::Result::Code exec(const Request *req, ostreamBuf *os) {
        // If at least one parameter is given, then treat the request as a
        // normal get request, otherwise if no parameters is given, treat it as
        // a get-all request.
        if (req->params.size())
            return FunctionExporterGet<INTERFACE, SUBJECT>::exec(req, os);
        else
            return exec_all(req, os);

        return vtss::json::Result::OK;
    }

    virtual void handle_reflection(Reflection *r) {
        HandlerReflector handler(r);
        typename Interface2ParamTuple<INTERFACE>::type args;

        r->has_get_all_variant();
        if (this->has_notification())
            r->has_notification();

        r->input_begin();
        args.template parse_get<INTERFACE>(handler);
        r->input_end();

        r->output_begin();
        args.template serialize_get<INTERFACE>(handler);
        r->output_end();
    }
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#undef I
#endif  // __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_MAP_CUSTOM_HXX__

