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

#include <vtss/basics/trace_grps.hxx>
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_JSON

#include <vtss/basics/trace_basics.hxx>
#include <vtss/basics/json-rpc.hxx>
#include "vtss/basics/expose/json/exporter.hxx"
#include "vtss/basics/expose/json/loader.hxx"
#include "vtss/basics/expose/json/root-node.hxx"

namespace vtss {
namespace expose {
namespace json {

static vtss::json::Result::Code make_invalid_req(Exporter::Map &m) {
    vtss::json::Result::Code rc = vtss::json::Result::INVALID_REQUEST;
    const char *s_invalid_request = "Invalid request";
    m.add_leaf(lit_null, vtss::tag::Name("id"));
    m.add_leaf(lit_null, vtss::tag::Name("result"));
    Exporter::Map em = m.as_map(vtss::tag::Name("error"));
    em.add_leaf(rc, vtss::tag::Name("code"));
    em.add_leaf(str(s_invalid_request), vtss::tag::Name("message"));

    return rc;
}

vtss::json::Result::Code RootNode::single_req(int priv, str input,
                                              ostreamBuf &out) {
    Request single_req;
    Loader l(input.begin(), input.end());

    if (!l.load(single_req)) return vtss::json::Result::INVALID_REQUEST;

    VTSS_BASICS_TRACE(NOISE) << "Handling single request";

    return handle(priv, str(single_req.method.begin()), &single_req, out);
}

vtss::json::Result::Code RootNode::batch_req(int priv, str input,
                                             ostreamBuf &out) {
    JsonArrayPtr batch_req;
    Loader l(input.begin(), input.end());
    vtss::json::Result::Code rc = vtss::json::Result::INVALID_REQUEST;

    if (!l.load(batch_req)) return rc;

    VTSS_BASICS_TRACE(NOISE) << "Handling batch request";

    Exporter e(&out);
    if (batch_req.data.size() == 0) {
        Exporter::Map m = e.as_map();
        return make_invalid_req(m);
    }

    VTSS_BASICS_TRACE(NOISE) << "Handling batch request - start";
    Exporter::Tuple t = e.as_tuple();
    for (const auto &e : batch_req.data) {
        Exporter::Ref ref = t.as_ref();
        Request single_req;
        Loader ll(e.begin(), e.end());
        StringStream oo;

        VTSS_BASICS_TRACE(NOISE) << "Sub msg: " << e.as_str();

        if (!ll.load(single_req)) {
            Exporter::Map m = ref.as_map();
            (void)make_invalid_req(m);
            continue;
        }

        rc = handle(priv, str(single_req.method.begin()), &single_req, oo);
        if (oo.buf.size() == 0) {
            Exporter::Map m = ref.as_map();
            (void)make_invalid_req(m);
        } else {
            ref.raw_write(oo.begin(), oo.end());
        }
    }

    VTSS_BASICS_TRACE(NOISE) << "Handling batch request - done";
    return vtss::json::Result::OK;
}

vtss::json::Result::Code RootNode::process(int priv, str input, ostreamBuf &out) {
    vtss::json::Result::Code rc;

    VTSS_BASICS_TRACE(NOISE) << "Root node";

    // Try load as a single request
    rc = single_req(priv, input, out);
    if (rc != vtss::json::Result::INVALID_REQUEST) return rc;

    // Try load as a batch request
    rc = batch_req(priv, input, out);
    if (rc != vtss::json::Result::INVALID_REQUEST) return rc;

    // Failed to load
    VTSS_BASICS_TRACE(NOISE) << "Failed to load request";
    return vtss::json::Result::INVALID_REQUEST;
}

}  // namespace json
}  // namespace expose
}  // namespace vtss
