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

#ifndef __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_MAP_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_MAP_HXX__


#include <vtss/basics/expose/json/function-exporter-get.hxx>
#include <vtss/basics/expose/json/response-map.hxx>
#include <vtss/basics/expose/json/response-map-row-single-key-single-val.hxx>
#include <vtss/basics/expose/json/response-map-row-single-key-multi-val.hxx>
#include <vtss/basics/expose/json/response-map-row-multi-key-single-val.hxx>
#include <vtss/basics/expose/json/response-map-row-multi-key-multi-val.hxx>

namespace vtss {
namespace expose {
namespace json {

template <class INTERFACE_DESCRIPTOR, class SUBJECT>
struct FunctionExporterGetMap
        : public FunctionExporterGet<INTERFACE_DESCRIPTOR, SUBJECT> {
    typedef FunctionExporterGet<INTERFACE_DESCRIPTOR, SUBJECT> BASE;
    typedef ResponseMap<ResponseMapRowSingleKeySingleVal> SingleKeySingleVal_t;
    typedef ResponseMap<ResponseMapRowSingleKeyMultiVal> SingleKeyMultiVal_t;
    typedef ResponseMap<ResponseMapRowMultiKeySingleVal> MultiKeySingleVal_t;
    typedef ResponseMap<ResponseMapRowMultiKeyMultiVal> MultiKeyMultiVal_t;

    FunctionExporterGetMap(SUBJECT *subject, NamespaceNode *ns, const char *n,
                           const char *d)
        : BASE(subject, ns, n, d) {}

    vtss::json::Result::Code exec_all(const Request *req, ostreamBuf *os) {
        HandlerFunctionExporterParser handler_in(req);
        typedef typename Interface2ParamTuple<INTERFACE_DESCRIPTOR>::type Tuple;

        // Single key, single value -> SingleKeySingleVal_t
        // Multiple keys, single value -> MultiKeySingleVal_t
        // Single key, Multiple values -> SingleKeyMultiVal_t
        // Multiple keys, Multiple values -> MultiKeyMultiVal_t
        typedef typename meta::__if<
                (Tuple::get_context_output_count() > 1),

                // multi-val
                typename meta::__if<(Tuple::get_context_input_count() > 1),
                                    // multi-val-multi-key
                                    MultiKeyMultiVal_t,
                                    // multi-val-single-key
                                    SingleKeyMultiVal_t>::type,

                // single-val
                typename meta::__if<(Tuple::get_context_input_count() > 1),
                                    // single-val-multi-key
                                    MultiKeySingleVal_t,
                                    // single-val-single-key
                                    SingleKeySingleVal_t>::type>::type
                response_type;

        // The handler type is derived from the response type.
        typedef typename response_type::handler_type handler_type;

        // A tuple representing a argument list matching the prototype of the
        // function pointers in INTERFACE_DESCRIPTOR
        Tuple args;

        // Create an exporter which the output parameters is written to
        response_type response(os, req->id);

        // get first input parameter-set (pass null-pointers to the itr
        // function).
        if (args.call_as_itr_first(BASE::subject_) != MESA_RC_OK) {
            // If the itr-first iterator fails then the array is empty, which is
            // not an error! The empty array is added by the response object
            // returned by 'response(...)'.
            return vtss::json::Result::OK;
        }

        // Use the get-next and the get functions to collect the complete array.
        // The array is serialized row-by-row using the result-handler.
        while (true) {
            // get the value related to the keys found in 'args'
            if (args.call_as_get(BASE::subject_) == MESA_RC_OK) {
                // Error returned by the get function is expected and it just
                // mean "the input values returned by the iterator was not
                // found". The iterator are allowed to suggest non-existing
                // values, and we can therefor just skip over such error and
                // call the iterator again.
                //
                // But this time the get function succeeded, and the result is
                // ready to be serialized.
                handler_type handler_out = response.resultHandler();

                // serialize values, explicit scoping is needed as value handler
                // must be destructed before the "handler_out" is usable again.
                {
                    auto &&key_handler = handler_out.keyHandler();
                    args.template serialize_keys<INTERFACE_DESCRIPTOR>(
                            key_handler);
                    if (!key_handler.ok()) {
                        return vtss::json::Result::INTERNAL_ERROR;
                    }
                }

                // serialize values
                auto &&val_handler = handler_out.valHandler();
                if (Tuple::get_context_output_count() == 0) {
                    val_handler.add_leaf(lit_null);
                } else {
                    args.template serialize_values<INTERFACE_DESCRIPTOR>(
                            val_handler);
                    if (!val_handler.ok()) {
                        return vtss::json::Result::INTERNAL_ERROR;
                    }
                }
            }

            // Increment the input values
            if (args.call_as_itr(BASE::subject_) != MESA_RC_OK) {
                // When the iterator fails it means end-of-table
                break;
            }
        }

        return vtss::json::Result::OK;
    }

    vtss::json::Result::Code exec(const Request *req, ostreamBuf *os) {
        // If at least one parameter is given, then treat the request as a
        // normal get request, otherwise if no parameters is given, treat it as
        // a get-all request.
        if (req->params.size())
            return FunctionExporterGet<INTERFACE_DESCRIPTOR, SUBJECT>::exec(req,
                                                                            os);
        else
            return exec_all(req, os);

        return vtss::json::Result::OK;
    }

    virtual void handle_reflection(Reflection *r) {
        HandlerReflector handler(r);
        typename Interface2ParamTuple<INTERFACE_DESCRIPTOR>::type args;

        r->has_get_all_variant();
        if (this->has_notification())
            r->has_notification();

        r->input_begin();
        args.template parse_get<INTERFACE_DESCRIPTOR>(handler);
        r->input_end();

        r->output_begin();
        args.template serialize_get<INTERFACE_DESCRIPTOR>(handler);
        r->output_end();
    }
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#undef I
#endif  // __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_MAP_HXX__
