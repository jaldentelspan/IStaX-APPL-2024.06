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

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_JSON

#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/expose/json/literal.hxx"
#include "vtss/basics/expose/json/exporter.hxx"
#include "vtss/basics/expose/json/string-encode-no-qoutes.hxx"
#include "vtss/basics/expose/json/function-importer-abstract.hxx"

namespace vtss {
namespace expose {
namespace json {

static void print_name(Exporter &e, const Node *n, uint32_t cnt) {
    if (cnt > 128) VTSS_BASICS_TRACE(ERROR) << "Too many nested namespaces!";

    if (n->parent()) {
        print_name(e, n->parent(), cnt + 1);

        // Do not print the name of the root as the root is considered a dummy
        // node
        str s = (n->name());
        string_encode_no_qoutes(e, s.begin(), s.end());
        e.push_char('.');
    }
}

void serialize(Exporter &e, const FunctionImporterAbstract *f) {
    serialize(e, quote_start);

    if (f->parent()) print_name(e, f->parent(), 0);

    str s(f->name());
    vtss::expose::json::string_encode_no_qoutes(e, s.begin(), s.end());
    serialize(e, vtss::expose::json::quote_end);
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

