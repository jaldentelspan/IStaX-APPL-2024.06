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

#ifndef __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_PATCH_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_PATCH_HXX__

#include <vtss/basics/json-rpc-function.hxx>
#include <vtss/basics/expose/json/handler-function-exporter-parser.hxx>
#include <vtss/basics/expose/json/response-scalar-single-argument.hxx>
#include <vtss/basics/expose/json/interface-2-param-tuple.hxx>
#include <vtss/basics/expose/json/interface-descriptor-priv-type.hxx>
#include <vtss/basics/expose/json/interface-descriptor-priv-module.hxx>

namespace vtss {
namespace expose {
namespace json {

template <class INTERFACE_DESCRIPTOR, class SUBJECT>
struct FunctionExporterPatch : public FunctionExporterAbstract {
    FunctionExporterPatch(SUBJECT *subject, NamespaceNode *ns, const char *n,
                        const char *d)
        : FunctionExporterAbstract(
                  ns, n, d, true,
                  InterfaceDescriptorPrivType<INTERFACE_DESCRIPTOR>::value,
                  InterfaceDescriptorPrivModule<INTERFACE_DESCRIPTOR>::value),
          subject_(subject) {}

    vtss::json::Result::Code exec(const Request *req, ostreamBuf *os) {
        HandlerFunctionExporterParser handler_in(req);
        handler_in.patch_mode(true);
        typedef typename Interface2ParamTuple<INTERFACE_DESCRIPTOR>::type Args;
        typedef ResponseScalarSingleArgument::handler_type handler_type;

        Args args;
        if (handler_in.argument_cnt() != Args::set_context_input_count()) {
            arguments_got = handler_in.argument_cnt();
            arguments_expect = Args::set_context_input_count();
            return vtss::json::Result::INVALID_PARAMS;
        }

        // start by parsing all the keys - preparing for calling get function
        unsigned i = args.template parse_keys<INTERFACE_DESCRIPTOR>(handler_in);
        if (!handler_in.ok()) {
            error_argument_index = i;
            return vtss::json::Result::INVALID_PARAMS;
        }

        // Invoke the actual get-function
        mesa_rc rc = args.call_as_get(subject_);
        if (rc != MESA_RC_OK) {
            error_code = rc;
            failing_function = (void *)subject_;
            return vtss::json::Result::INTERNAL_ERROR;
        }

        // Now, the values in "args" has been initialized with default values
        // from the get-function. These values are now being overwritten with
        // the values provided in the json-request. Values not provided in the
        // json request will have the default value assigned in from the "get"
        // step above.
        i = args.template parse_values<INTERFACE_DESCRIPTOR>(handler_in);
        if (!handler_in.ok()) {
            error_argument_index = i;
            return vtss::json::Result::INVALID_PARAMS;
        }

        // Invoke the actual set function
        rc = args.call_as_set(subject_);
        if (rc != MESA_RC_OK) {
            error_code = rc;
            failing_function = (void *)subject_;
            return vtss::json::Result::INTERNAL_ERROR;
        }

        // Create an exporter which the output parameters is written to
        ResponseScalarSingleArgument response(os, req->id);
        handler_type handler_out = response.resultHandler();

        // set function are not allowed to produce any output, and handler_out
        // will therefor not be used for further serializtion.
        serialize(handler_out, lit_null);

        return vtss::json::Result::OK;
    }

    virtual void handle_reflection(Reflection *r) {
        HandlerReflector handler(r);
        typename Interface2ParamTuple<INTERFACE_DESCRIPTOR>::type args;

        r->input_begin();
        args.template parse_set<INTERFACE_DESCRIPTOR>(handler);
        r->input_end();

        r->output_begin();
        r->output_end();
    }

    SUBJECT *subject_;
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_FUNCTION_EXPORTER_PATCH_HXX__
