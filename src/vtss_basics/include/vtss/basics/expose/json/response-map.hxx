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

#ifndef __VTSS_BASICS_EXPOSE_JSON_RESPONSE_MAP_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_RESPONSE_MAP_HXX__

#include <vtss/basics/expose/json/response-base.hxx>

namespace vtss {
namespace expose {
namespace json {

template <typename ROW>
struct ResponseMap : public ResponseBase {
    typedef ROW handler_type;

    ResponseMap(ostreamBuf *os, const JsonValue &id)
        : ResponseBase(os, id), result_vector(result.as_tuple()) {}

    bool ok() const { return result_vector.ok(); }

    ROW resultHandler() {
        return ROW(vtss::move(result_vector.as_map()));
    }

    Exporter::Tuple result_vector;
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_RESPONSE_MAP_HXX__
