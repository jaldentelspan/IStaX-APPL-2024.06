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

#ifndef __VTSS_BASICS_EXPOSE_JSON_HANDLER_FUNCTION_EXPORTER_SERIALIZER_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_HANDLER_FUNCTION_EXPORTER_SERIALIZER_HXX__

#include <vtss/basics/json-rpc-function.hxx>

namespace vtss {
namespace expose {
namespace json {

struct HandlerFunctionExporterSerializer {
    typedef Exporter::Map Map_t;
    static constexpr size_t buf_size = 1024 * 256;

    explicit HandlerFunctionExporterSerializer(
            ostreamBuf *os, const JsonValue &id);

    template <typename T, typename... Args>
    void add_leaf(T&& t, Args &&... args) {
        result_tuple.add_leaf(t);
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

    bool ok() const { return error_ == vtss::json::Result::OK; }

    template<typename... Args>
    Exporter::Map as_map(const Args... args) {
        return result_tuple.as_map();
    }

    void error(vtss::json::Result::Code c);
    vtss::json::Result::Code error() const { return error_; }

    ~HandlerFunctionExporterSerializer();

    vtss::json::Result::Code error_ = vtss::json::Result::OK;
    Exporter response;
    Exporter::Map response_map;

    // TODO, this mess should be optimized! Chunks should be allocated on demand
    // instead.
    FixedBuffer result_buf;
    BufPtrStream result_stream;
    Exporter result;
    Exporter::Tuple result_tuple;

    FixedBuffer err_buf;
    BufPtrStream err_stream;
    Exporter err;
    Exporter::Map err_map;
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_HANDLER_FUNCTION_EXPORTER_SERIALIZER_HXX__
