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

#ifndef __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_SUBJECT_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_SUBJECT_HXX__

#include <vtss/basics/meta.hxx>
#include <vtss/basics/json-rpc-function.hxx>
#include <vtss/basics/expose/serialize-keys.hxx>
#include <vtss/basics/expose/serialize-values.hxx>
#include <vtss/basics/expose/json/handler-reflector.hxx>
#include <vtss/basics/expose/json/interface-2-param-tuple.hxx>
#include <vtss/basics/expose/json/function-exporter-abstract.hxx>
#include <vtss/basics/expose/json/response-scalar-single-argument.hxx>
#include <vtss/basics/expose/json/handler-function-exporter-parser.hxx>
#include <vtss/basics/expose/json/interface-descriptor-priv-type.hxx>
#include <vtss/basics/expose/json/interface-descriptor-priv-module.hxx>

namespace vtss {
namespace expose {
namespace json {

template <class INTERFACE, class SUBJECT>
struct FunctionExporterGetSubject : public FunctionExporterAbstract {
    FunctionExporterGetSubject(SUBJECT *subject, NamespaceNode *ns,
                               const char *n, const char *d)
        : FunctionExporterAbstract(
                  ns, n, d, false,
                  InterfaceDescriptorPrivType<INTERFACE>::value,
                  InterfaceDescriptorPrivModule<INTERFACE>::value),
          subject_(subject) {}

    virtual vtss::json::Result::Code exec(const Request *req, ostreamBuf *os) {
        // Parse the request
        HandlerFunctionExporterParser handler_in(req);

        typedef ResponseScalarSingleArgument response_type;

        // The handler type is derived from the response type.
        typedef typename response_type::handler_type handler_type;

        if (handler_in.argument_cnt() != INTERFACE::P::key_cnt) {
            arguments_got = handler_in.argument_cnt();
            arguments_expect = INTERFACE::P::key_cnt;
            return vtss::json::Result::INVALID_PARAMS;
        }

        typename SUBJECT::K k;
        typename SUBJECT::V v;

        // Parse the input parameters from 'req' and store them in 'k'
        serialize_keys<INTERFACE>(handler_in, k);

        if (!handler_in.ok()) {
            error_argument_index = 0;  // TODO
            return vtss::json::Result::INVALID_PARAMS;
        }

        // Invoke the actual function
        mesa_rc rc = subject_->get_(k, v);
        if (rc != MESA_RC_OK) {
            error_code = rc;
            failing_function = (void *)subject_;
            return vtss::json::Result::INTERNAL_ERROR;
        }

        // Create an exporter which the output parameters is written to
        response_type response(os, req->id);
        handler_type handler_out = response.resultHandler();
        serialize_values<INTERFACE>(handler_out, v);

        return vtss::json::Result::OK;
    }

    virtual void handle_reflection(Reflection *r) {
        HandlerReflector handler(r);
        typename Interface2ParamTuple<INTERFACE>::type args;

        r->input_begin();
        args.template parse_get<INTERFACE>(handler);
        r->input_end();

        r->output_begin();
        args.template serialize_get<INTERFACE>(handler);
        r->output_end();
    }

    SUBJECT *subject_;
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_GET_SUBJECT_HXX__
