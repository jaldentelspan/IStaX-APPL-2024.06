
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

#include <vtss/basics/expose/json/json-encoded-data.hxx>
#include <vtss/basics/expose/json/handler-function-exporter-serializer.hxx>

namespace vtss {
namespace expose {
namespace json {

HandlerFunctionExporterSerializer::HandlerFunctionExporterSerializer(
        ostreamBuf *os, const JsonValue &id)
    : response(os),
      response_map(response.as_map()),
      result_buf(HandlerFunctionExporterSerializer::buf_size),
      result_stream(&result_buf),
      result(&result_stream),
      result_tuple(result.as_tuple()),
      err_buf(HandlerFunctionExporterSerializer::buf_size),
      err_stream(&err_buf),
      err(&err_stream),
      err_map(err.as_map()) {
    response_map.add_leaf(id, vtss::tag::Name("id"));
}

void HandlerFunctionExporterSerializer::error(vtss::json::Result::Code c) {
    VTSS_ASSERT(c != vtss::json::Result::OK);

    // first error thrown is the one which is caputred in the response
    if (error_ == vtss::json::Result::OK) {
        err_map.add_leaf(c, vtss::tag::Name("code"));
    }

    error_ = c;
}

HandlerFunctionExporterSerializer::~HandlerFunctionExporterSerializer() {
    result_tuple.close();
    err_map.close();

    if (ok()) {
        response_map.add_leaf(lit_null, vtss::tag::Name("error"));
        response_map.add_leaf(
                JsonEncodedData(result_stream.begin(), result_stream.end()),
                vtss::tag::Name("result"));
    } else {
        response_map.add_leaf(
                JsonEncodedData(err_stream.begin(), err_stream.end()),
                vtss::tag::Name("error"));
        response_map.add_leaf(lit_null, vtss::tag::Name("result"));
    }
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

