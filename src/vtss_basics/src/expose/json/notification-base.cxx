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

#include "vtss/basics/expose/json/notification-base.hxx"
#include "vtss/basics/expose/json/literal.hxx"

namespace vtss {
namespace expose {
namespace json {

NotificationBase::NotificationBase(ostreamBuf *os, const std::string &method)
    : n(os), n_map(n.as_map()) {
    n_map.add_leaf(method, vtss::tag::Name("method"));
    n_map.add_leaf(lit_null, vtss::tag::Name("id"));
    n_args = n_map.as_tuple(vtss::tag::Name("params"));
}

Exporter::Map NotificationBase::row() {
    return n_args.as_map();
}

void NotificationBase::close() {
    n_args.close();
    n_map.close();
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

