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

#ifndef __VTSS_BASICS_EXPOSE_JSON_HANDLER_FUNCTION_EXPORTER_PARSER_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_HANDLER_FUNCTION_EXPORTER_PARSER_HXX__

#include <vtss/basics/json-rpc-function.hxx>
#include <vtss/basics/expose/json/loader-map-standalone.hxx>

namespace vtss {
namespace expose {
namespace json {

struct HandlerFunctionExporterParser : public ErrorReportingPath {
    typedef LoaderMapStandalone Map_t;
    HandlerFunctionExporterParser(const Request *r)
        : patch_mode_(false), req_(r) {}

    template <typename T, typename... Args>
    void add_leaf(T &&t, Args &&... args) {
        if (req_->params.size() <= argument_index_) {
            // TODO, add error number for this
            // VTSS_BASICS_TRACE(DEBUG) << req_->params.size()
            //                         << " <= " << argument_index_;
            error(vtss::json::Result::INVALID_PARAMS);
            return;
        }

        if (!req_->params[argument_index_].copy_to(t)) {
            // TODO, add error number for this
            // VTSS_BASICS_TRACE(DEBUG) << __PRETTY_FUNCTION__
            //                         << " Failsed to parse: "
            //                         <<
            // req_->params[argument_index_].as_str();
            error(vtss::json::Result::INVALID_PARAMS);
            return;
        }
    }

    template <typename T, typename... Args>
    void add_snmp_leaf(T &&value, const Args &&... args) {}

    template <typename T, typename... Args>
    void add_rpc_leaf(T &&value, const Args &&... args) {
        add_leaf(forward<T>(value), forward<const Args>(args)...);
    }

    template <typename... Args>
    void argument_properties(const Args... args) {}
    void argument_properties_clear() {}


    template <typename... Args>
    LoaderMapStandalone as_map(const Args... args) {
        if (req_->params.size() <= argument_index_) {
            error(vtss::json::Result::INVALID_PARAMS);
            return LoaderMapStandalone();
        }

        return LoaderMapStandalone(req_->params[argument_index_].begin(),
                                   req_->params[argument_index_].end(), this,
                                   patch_mode_);
    }

    HandlerFunctionExporterParser &as_tuple() {
        as_tuple_index_ = 0;
        return *this;
    }

    HandlerFunctionExporterParser &as_ref() {
        argument_index_ = as_tuple_index_;
        as_tuple_index_++;
        return *this;
    }

    void patch_mode(bool p) { patch_mode_ = p; }
    bool patch_mode() { return patch_mode_; }
    bool ok() const { return error_ == vtss::json::Result::OK; }
    void error(vtss::json::Result::Code c) { error_ = c; }
    vtss::json::Result::Code error() const { return error_; }
    const Request *req() const { return req_; }
    void increment_argument_index() { argument_index_++; }

    unsigned argument_cnt() const { return req_->params.size(); }

  private:
    bool patch_mode_;
    const Request *req_ = nullptr;
    vtss::json::Result::Code error_ = vtss::json::Result::OK;
    unsigned argument_index_ = 0;
    unsigned as_tuple_index_ = 0;
};

template <typename H, typename T>
typename meta::enable_if<is_same<H, HandlerFunctionExporterParser>::value,
                         void>::type
serialize(H &h, T &&t) {
    h.add_leaf(t);
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_HANDLER_FUNCTION_EXPORTER_PARSER_HXX__
