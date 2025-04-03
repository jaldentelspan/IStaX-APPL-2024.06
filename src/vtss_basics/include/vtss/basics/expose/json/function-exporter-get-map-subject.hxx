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

#ifndef __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_MAP_SUBJECT_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_MAP_SUBJECT_HXX__

#include <vtss/basics/expose/serialize-keys.hxx>
#include <vtss/basics/expose/serialize-values.hxx>
#include <vtss/basics/expose/json/response-map.hxx>
#include <vtss/basics/expose/json/response-map-row-single-key-single-val.hxx>
#include <vtss/basics/expose/json/function-exporter-get.hxx>
#include <vtss/basics/expose/json/function-exporter-get-subject.hxx>

namespace vtss {
namespace expose {
namespace json {

template <class INTERFACE, class SUBJECT>
struct FunctionExporterGetMapSubject
        : public FunctionExporterGetSubject<INTERFACE, SUBJECT> {
    typedef FunctionExporterGetSubject<INTERFACE, SUBJECT> BASE;
    typedef ResponseMap<ResponseMapRowSingleKeySingleVal> SingleKeySingleVal_t;

    FunctionExporterGetMapSubject(SUBJECT *subject, NamespaceNode *ns,
                                  const char *n, const char *d)
        : BASE(subject, ns, n, d) {}

    vtss::json::Result::Code exec_all(const Request *req, ostreamBuf *os) {
        // Parse the request
        HandlerFunctionExporterParser handler_in(req);

        // Create an exporter which the output parameters is written to
        SingleKeySingleVal_t r(os, req->id);

        // Key-value pair
        typename SUBJECT::K k;
        typename SUBJECT::V v;

        // Get-first
        if (BASE::subject_->get_first_(k, v) != MESA_RC_OK) {
            // Empty map - not an error but we have no value to add
            return vtss::json::Result::OK;
        } else {
            // Serialize the values
            typename SingleKeySingleVal_t::handler_type h = r.resultHandler();
            serialize_keys<INTERFACE>(h.keyHandler(), k);
            serialize_values<INTERFACE>(h.valHandler(), v);
        }

        // Keep asking for the next value until end-of-table has been reached
        while (BASE::subject_->get_next_(k, v) == MESA_RC_OK) {
            // Serialize the next value
            typename SingleKeySingleVal_t::handler_type h = r.resultHandler();
            serialize_keys<INTERFACE>(h.keyHandler(), k);
            serialize_values<INTERFACE>(h.valHandler(), v);
        }

        return vtss::json::Result::OK;
    }

    vtss::json::Result::Code exec(const Request *req, ostreamBuf *os) {
        // If at least one parameter is given, then treat the request as a
        // normal get request, otherwise if no parameters is given, treat it as
        // a get-all request.
        if (req->params.size())
            return BASE::exec(req, os);
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

#endif  // __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_MAP_SUBJECT_HXX__
