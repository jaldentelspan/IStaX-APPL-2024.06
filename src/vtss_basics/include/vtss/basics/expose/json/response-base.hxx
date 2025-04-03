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

#ifndef __VTSS_BASICS_EXPOSE_JSON_RESPONSE_BASE_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_RESPONSE_BASE_HXX__

#include <vtss/basics/json-rpc-function.hxx>

namespace vtss {
namespace expose {
namespace json {

struct ResponseBase {
    typedef Exporter::Map Map_t;

    ResponseBase(ostreamBuf *os, const JsonValue &id);

    bool ok() const { return error_ == vtss::json::Result::OK; }

    void error(vtss::json::Result::Code c);
    vtss::json::Result::Code error() const { return error_; }

    virtual ~ResponseBase();

  protected:
    vtss::json::Result::Code error_ = vtss::json::Result::OK;
    Exporter response;
    Exporter::Map response_map;

    StringStream result_stream;
    Exporter result;

    StringStream err_stream;
    Exporter err;
    Exporter::Map err_map;
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_RESPONSE_BASE_HXX__
