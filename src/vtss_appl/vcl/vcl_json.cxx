/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vcl_trace.h"
#include "vtss/basics/trace.hxx"
#include "vcl_serializer.hxx"
#include "vtss/basics/expose/json.hxx"
#include "vtss_appl_formatting_tags.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::vcl::interfaces;

namespace vtss
{
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_vcl("vcl");
extern "C" void vcl_json_init()
{
    json_node_add(&ns_vcl);
}

static void serialize(expose::json::Exporter &e, AsVclProto &p)
{
    auto o = e.encoded_stream();

    switch (p.inner.proto_encap_type) {
    case VTSS_APPL_VCL_PROTO_ENCAP_ETH2:
        o << "ETH2 "
          << HEX_fixed<4>(p.inner.proto.eth2_proto.eth_type);
        return;

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP:
        o << "SNAP OUI:"
          << HEX_fixed<2>(p.inner.proto.llc_snap_proto.oui[0]) << "-"
          << HEX_fixed<2>(p.inner.proto.llc_snap_proto.oui[1]) << "-"
          << HEX_fixed<2>(p.inner.proto.llc_snap_proto.oui[2]) << " PID:"
          << HEX_fixed<4>(p.inner.proto.llc_snap_proto.pid);
        return;

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER:
        o << "LLC DSAP:"
          << HEX_fixed<2>(p.inner.proto.llc_other_proto.dsap) << " SSAP:"
          << HEX_fixed<2>(p.inner.proto.llc_other_proto.ssap);
        return;

    default:
        return;
    }
}

static bool parse(const char *&b, const char *e, vtss_appl_vcl_proto_t &val)
{
    const char *i = b;
    int type = 0;
    parser::Lit oui_lit("OUI:");
    parser::Lit pid_lit("PID:");
    parser::Lit dsap_lit("DSAP:");
    parser::Lit ssap_lit("SSAP:");
    parser::Lit dash_lit("-");
    parser::OneOrMoreSpaces space;
    parser::Int<uint16_t, 16, 4, 4> eth2_number, pid;
    parser::Int<uint8_t, 16, 2, 2> hex1, hex2, hex3;

    const vtss_enum_descriptor_t interface_names[] = {{1, "ETH2"},
        {2, "SNAP"},
        {3, "LLC"},
        {0, 0}
    };

    if (!parse(i, e, vtss::expose::json::quote_start)) {
        return false;
    }

    // Parse the type
    if (!enum_element_parse(i, e, interface_names, type)) {
        VTSS_TRACE(TRACE_GRP_MIB, DEBUG) << "Failed to parse interface type. Input string: "
                                         << str(b, e);
        return false;
    }

    // Parse one of more white-spaces followed by a number
    if (!space(i, e)) {
        VTSS_TRACE(TRACE_GRP_MIB, DEBUG) << "Parse error. Input string: " << str(b, e);
        return false;
    }

    switch (type) {
    case 1:  // eth2
        if (!eth2_number(i, e)) {
            VTSS_TRACE(TRACE_GRP_MIB, DEBUG) << "Parse error of ETH2 encapsulation string: " << str(b, e);
            return false;
        }
        val.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_ETH2;
        val.proto.eth2_proto.eth_type = eth2_number.get();
        break;

    case 2:  // snap
        if (!parser::Group(i, e, oui_lit, hex1, dash_lit, hex2, dash_lit, hex3, space, pid_lit, pid)) {
            VTSS_TRACE(TRACE_GRP_MIB, DEBUG) << "Parse error of SNAP encapsulation string: " << str(b, e);
            return false;
        }
        val.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP;
        val.proto.llc_snap_proto.oui[0] = hex1.get();
        val.proto.llc_snap_proto.oui[1] = hex2.get();
        val.proto.llc_snap_proto.oui[2] = hex3.get();
        val.proto.llc_snap_proto.pid = pid.get();
        break;

    case 3:  // llc
        if (!parser::Group(i, e, dsap_lit, hex1, space, ssap_lit, hex2)) {
            VTSS_TRACE(TRACE_GRP_MIB, DEBUG) << "Parse error of LLC encapsulation string: " << str(b, e);
            return false;
        }
        val.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER;
        val.proto.llc_other_proto.dsap = hex1.get();
        val.proto.llc_other_proto.ssap = hex2.get();
        break;

    default:
        return false;
    }

    if (!parse(i, e, vtss::expose::json::quote_end)) {
        return false;
    }

    b = i;

    return true;
}

static void serialize(expose::json::Loader &l, AsVclProto &val)
{
    if (!parse(l.pos_, l.end_, val.inner)) {
        VTSS_TRACE(TRACE_GRP_MIB, DEBUG) << "flag_error";
        l.flag_error();
        return;
    }
}

static void serialize(expose::json::HandlerReflector &e, AsVclProto &)
{
    e.type_terminal(vtss::expose::json::JsonCoreType::String, "vtss_appl_vcl_proto_t",
                    "String describing the Protocol encapsulation. The string has two parts separated by a space. "
                    "The first part specifies the encapsulation type and the output can be either ETH2, LLC or SNAP. "
                    "The second part specifies the encapsulation parameters and its format depends on the selected "
                    "encapsulation type: 1) ETH2: a hex ranging from 0600 to ffff indicating the protocol type, "
                    "e.g. 0800 for IP 2) SNAP: OUI:xx-xx-xx PID:xxxx The OID can range from 00-00-00 to ff-ff-ff and "
                    "the PID from 0000 to ffff. Special case is when OID is 00-00-00; then PID can only range "
                    "from 0600 to ffff) 3) LLC: DSAP:xxx SSAP:xxx Each of these two fields can range from 0 to 255.");
}

// Parent: vtss/vcl -------------------------------------------------------------
NS(ns_config,    ns_vcl,    "config");
NS(ns_interface, ns_config, "interface");
NS(ns_status,    ns_vcl,    "status");

// Parent: vtss/vcl/config ------------------------------------------------------
NS(ns_conf_protocol, ns_config, "protocol");

// Parent: vtss/vcl/config ------------------------------------------------------
static TableReadWriteAddDelete<MacEntry> entry_mac(&ns_config, "mac");

// Parent: vtss/vcl/config ------------------------------------------------------
static TableReadWriteAddDelete<IpSubnetEntry> entry_ip(&ns_config, "ip");

// Parent: vtss/vcl/config/protocol ---------------------------------------------
static TableReadWriteAddDelete<ProtocolProtoEntry> entry_proto(&ns_conf_protocol, "proto");

// Parent: vtss/vcl/config/protocol ---------------------------------------------
static TableReadWriteAddDelete<ProtocolGroupEntry> entry_group(&ns_conf_protocol, "group");

