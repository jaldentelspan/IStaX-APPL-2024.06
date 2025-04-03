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

#include "vcl_api.h"
#include "vcl_serializer.hxx"
#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#include "web_api.h"
#endif
#include "vcl_trace.h"

vtss_enum_descriptor_t vtss_appl_vcl_proto_encap_type_txt[] {
    {VTSS_APPL_VCL_PROTO_ENCAP_ETH2,      "Ethernet II"},
    {VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP,  "LLC SNAP"},
    {VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER, "LLC Other"},
    {0, 0},
};

using namespace vtss;
using namespace vtss::expose::snmp;
using namespace vtss::appl::vcl::interfaces;

VTSS_MIB_MODULE("vclMib", "VCL", vcl_mib_init, VTSS_MODULE_ID_VCL, root, h)
{
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201603210000Z", "Allow subnet length zero.");
    h.description("Private VCL MIB.");
}

#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))

bool consume_max(vtss::expose::snmp::OidImporter &a, uint32_t max, uint32_t &res)
{
    // pop one element, or default to zero
    if (!a.consume(res)) {
        a.flag_error();
        return false;
    }
    // Do not allow consumed elements greater than 'max' when parsing the Protocol Encapsulation OID
    if (res > max) {
        // In case of getnext request
        if (a.next_request()) {
            res = max;
            a.flag_overflow();
            return true;
        } else {
            // Normal get operations should just be terminated here with an
            // error code.
            a.flag_error();
            return false;
        }
    }
    return true;
}

void serialize(vtss::expose::snmp::OidImporter &a, AsVclProto &p)
{
    // get
    // [] -> error
    // [1] -> error
    // [1.1] -> 1.1
    // [2.1] -> error
    // [2.1.2.3.4] -> 2.1.2.3.4
    // [1.66000] -> error

    // get-next
    // [] ->  nullptr
    // [1] -> 0.0
    // [1.66000] -> 1.65535
    // [2.1] -> 2.0.255.255.65535
    // [2.1.300] -> 2.1.255.255.65535

    p.inner = {};
    T_DG(TRACE_GRP_MIB, "Entering OidImporter for AsVclProto");

    if (!a.ok_) {
        return;
    }

    uint32_t type;
    uint32_t temp;

    if (!consume_max(a, 3, type)) {
        T_NG(TRACE_GRP_MIB, "Failed to consume the 'type' octet from the OID");
        return;
    }
    T_DG(TRACE_GRP_MIB, "Protocol Encapsulation type provided is %d", type);

    switch (type) {
    case VTSS_APPL_VCL_PROTO_ENCAP_ETH2:
        T_NG(TRACE_GRP_MIB, "Ethernet II encapsulation provided");
        p.inner.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_ETH2;
        if (!consume_max(a, 65535, temp)) {
            return;
        }
        p.inner.proto.eth2_proto.eth_type = temp;
        T_NG(TRACE_GRP_MIB, "Ethernet II type provided is 0x%04x", p.inner.proto.eth2_proto.eth_type);
        T_DG(TRACE_GRP_MIB, "Exiting OidImporter after successfully setting AsVclProto for Eth2 Encapsulation");
        return;

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP:
        T_NG(TRACE_GRP_MIB, "LLC SNAP encapsulation provided");
        p.inner.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP;
        for (uint32_t count = 0; count < 3; count++) {
            if (!consume_max(a, 255, temp)) {
                return;
            }
            p.inner.proto.llc_snap_proto.oui[count] = temp;
        }
        if (!consume_max(a, 65535, temp)) {
            return;
        }
        p.inner.proto.llc_snap_proto.pid = temp;
        T_NG(TRACE_GRP_MIB, "SNAP type provided is OUI-%02x:%02x:%02x PID:0x%04x", p.inner.proto.llc_snap_proto.oui[0],
             p.inner.proto.llc_snap_proto.oui[1], p.inner.proto.llc_snap_proto.oui[2], p.inner.proto.llc_snap_proto.pid);
        T_DG(TRACE_GRP_MIB, "Exiting OidImporter after successfully setting AsVclProto for LLC SNAP Encapsulation");
        return;

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER:
        T_NG(TRACE_GRP_MIB, "LLC Other encapsulation provided");
        p.inner.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER;
        if (!consume_max(a, 255, temp)) {
            return;
        }
        p.inner.proto.llc_other_proto.dsap = temp;
        if (!consume_max(a, 255, temp)) {
            return;
        }
        p.inner.proto.llc_other_proto.ssap = temp;
        T_NG(TRACE_GRP_MIB, "LLC type provided is DSAP:0x%02x SSAP:0x%02x", p.inner.proto.llc_other_proto.dsap,
             p.inner.proto.llc_other_proto.ssap);
        T_DG(TRACE_GRP_MIB, "Exiting OidImporter after successfully setting AsVclProto for LLC Other Encapsulation");
        return;

    default:
        return;
    }
}

void serialize(vtss::expose::snmp::OidExporter &a, AsVclProto &p)
{
    if (!a.ok_) {
        return;
    }

    switch (p.inner.proto_encap_type) {
    case VTSS_APPL_VCL_PROTO_ENCAP_ETH2:
        a.oids_.push(VTSS_APPL_VCL_PROTO_ENCAP_ETH2);
        a.oids_.push((p.inner.proto.eth2_proto.eth_type));
        return;

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP:
        a.oids_.push(VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP);
        a.oids_.push(p.inner.proto.llc_snap_proto.oui[0]);
        a.oids_.push(p.inner.proto.llc_snap_proto.oui[1]);
        a.oids_.push(p.inner.proto.llc_snap_proto.oui[2]);
        a.oids_.push((p.inner.proto.llc_snap_proto.pid));
        return;

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER:
        a.oids_.push(VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER);
        a.oids_.push(p.inner.proto.llc_other_proto.dsap);
        a.oids_.push(p.inner.proto.llc_other_proto.ssap);
        return;

    default:
        return;
    }
}

static uint8_t vcl_proto_buf[6];
void serialize(vtss::expose::snmp::GetHandler &a, AsVclProto &p)
{
    a.value  = vcl_proto_buf;
    a.state(vtss::expose::snmp::HandlerState::DONE);

    T_DG(TRACE_GRP_MIB, "Entering GetHandler for AsVclProto");
    T_DG(TRACE_GRP_MIB, "Protocol Encapsulation type provided is %d", p.inner.proto_encap_type);
    switch (p.inner.proto_encap_type) {
    case VTSS_APPL_VCL_PROTO_ENCAP_ETH2:
        T_NG(TRACE_GRP_MIB, "Ethernet II encapsulation provided");
        a.length = 3;
        vcl_proto_buf[0] = VTSS_APPL_VCL_PROTO_ENCAP_ETH2;
        vcl_proto_buf[1] = (p.inner.proto.eth2_proto.eth_type & 0xff00) >> 8;
        vcl_proto_buf[2] = (p.inner.proto.eth2_proto.eth_type & 0xff);
        T_NG(TRACE_GRP_MIB, "Ethernet II type provided is ETYPE:0x%02x%02x", vcl_proto_buf[1], vcl_proto_buf[2]);
        T_DG(TRACE_GRP_MIB, "Exiting GetHandler after successfully getting AsVclProto for Eth2 Encapsulation");
        return;

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP:
        T_NG(TRACE_GRP_MIB, "LLC SNAP encapsulation provided");
        a.length = 6;
        vcl_proto_buf[0] = VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP;
        vcl_proto_buf[1] = p.inner.proto.llc_snap_proto.oui[0];
        vcl_proto_buf[2] = p.inner.proto.llc_snap_proto.oui[1];
        vcl_proto_buf[3] = p.inner.proto.llc_snap_proto.oui[2];
        vcl_proto_buf[4] = (p.inner.proto.llc_snap_proto.pid & 0xff00) >> 8;
        vcl_proto_buf[5] = (p.inner.proto.llc_snap_proto.pid & 0xff);
        T_NG(TRACE_GRP_MIB, "SNAP type provided is OUI-%02x:%02x:%02x PID:0x%02x%02x", vcl_proto_buf[1],
             vcl_proto_buf[2], vcl_proto_buf[3], vcl_proto_buf[4], vcl_proto_buf[5]);
        T_DG(TRACE_GRP_MIB, "Exiting GetHandler after successfully getting AsVclProto for LLC SNAP Encapsulation");
        return;

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER:
        T_NG(TRACE_GRP_MIB, "LLC Other encapsulation provided");
        a.length = 3;
        vcl_proto_buf[0] = VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER;
        vcl_proto_buf[1] = p.inner.proto.llc_other_proto.dsap;
        vcl_proto_buf[2] = p.inner.proto.llc_other_proto.ssap;
        T_NG(TRACE_GRP_MIB, "LLC type provided is DSAP:0x%02x SSAP:0x%02x", vcl_proto_buf[1],
             vcl_proto_buf[1]);
        T_DG(TRACE_GRP_MIB, "Exiting GetHandler after successfully getting AsVclProto for LLC Other Encapsulation");
        return;

    default:
        T_DG(TRACE_GRP_MIB, "Exiting GetHandler using default values for AsVclProto - RowEditorProtocolEncapsulation has not been set");
        a.length = 3;
        vcl_proto_buf[0] = 0;
        vcl_proto_buf[1] = 0;
        vcl_proto_buf[2] = 0;
        return;
    }

}

void serialize(vtss::expose::snmp::SetHandler &a, AsVclProto &p)
{
    if (!vtss::expose::snmp::check_type(a, vtss::expose::snmp::AsnType::OctetString)) {
        T_DG(TRACE_GRP_MIB, "Wrong type");
        return;
    }

    if ((a.val_length < 3) || (a.val_length > 6)) {
        T_DG(TRACE_GRP_MIB, "The provided value is too short/long, expected 3-6 octets");
        a.error_code(ErrorCode::wrongLength, __FILE__, __LINE__);
        return;
    }

    T_DG(TRACE_GRP_MIB, "Entering SetHandler for AsVclProto");
    p.inner = {};

    uint32_t type = (uint32_t)a.val[0];
    T_DG(TRACE_GRP_MIB, "Protocol Encapsulation type provided is %d", type);

    switch (type) {
    case VTSS_APPL_VCL_PROTO_ENCAP_ETH2:
        T_NG(TRACE_GRP_MIB, "Ethernet II encapsulation provided");
        if (a.val_length != 3) {
            T_DG(TRACE_GRP_MIB, "The provided value is too short/long, expected 3 octets");
            a.error_code(ErrorCode::wrongLength, __FILE__, __LINE__);
            return;
        }
        p.inner.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_ETH2;
        if (a.val[1] < 6) {
            T_DG(TRACE_GRP_MIB, "The provided type is invalid, expected something in the range 0x0600-0xffff");
            return;
        }
        p.inner.proto.eth2_proto.eth_type |= ((uint16_t)a.val[1] << 8);
        p.inner.proto.eth2_proto.eth_type |= ((uint16_t)a.val[2]);
        T_NG(TRACE_GRP_MIB, "Ethernet II type provided is ETYPE:0x%04x", p.inner.proto.eth2_proto.eth_type);
        a.state(HandlerState::DONE);
        T_DG(TRACE_GRP_MIB, "Exiting SetHandler after successfully setting AsVclProto for Eth2 Encapsulation");
        return;

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP: {
        BOOL snap_eth2 = TRUE;
        T_NG(TRACE_GRP_MIB, "LLC SNAP encapsulation provided");
        if (a.val_length != 6) {
            T_DG(TRACE_GRP_MIB, "The provided value is too short/long, expected 6 octets");
            a.error_code(ErrorCode::wrongLength, __FILE__, __LINE__);
            return;
        }
        p.inner.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_LLC_SNAP;
        for (uint32_t count = 0; count < 3; count++) {
            if (a.val[count + 1] != 0) {
                snap_eth2 = FALSE;
            }
            p.inner.proto.llc_snap_proto.oui[count] = (uint8_t)a.val[count + 1];
        }
        if (snap_eth2 && (a.val[4] < 6)) {
            T_DG(TRACE_GRP_MIB, "The provided PID is invalid, expected something in the range 0x0600-0xffff");
            return;
        }
        p.inner.proto.llc_snap_proto.pid |= ((uint16_t)a.val[4] << 8);
        p.inner.proto.llc_snap_proto.pid |= ((uint16_t)a.val[5]);
        T_NG(TRACE_GRP_MIB, "SNAP type provided is OUI-%02x:%02x:%02x PID:0x%04x", p.inner.proto.llc_snap_proto.oui[0],
             p.inner.proto.llc_snap_proto.oui[1], p.inner.proto.llc_snap_proto.oui[2], p.inner.proto.llc_snap_proto.pid);
        a.state(HandlerState::DONE);
        T_DG(TRACE_GRP_MIB, "Exiting SetHandler after successfully setting AsVclProto for LLC SNAP Encapsulation");
        return;
    }

    case VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER:
        T_NG(TRACE_GRP_MIB, "LLC Other encapsulation provided");
        if (a.val_length != 3) {
            T_DG(TRACE_GRP_MIB, "The provided value is too short/long, expected 3 octets");
            a.error_code(ErrorCode::wrongLength, __FILE__, __LINE__);
            return;
        }
        p.inner.proto_encap_type = VTSS_APPL_VCL_PROTO_ENCAP_LLC_OTHER;
        p.inner.proto.llc_other_proto.dsap = (uint8_t)a.val[1];
        p.inner.proto.llc_other_proto.ssap = (uint8_t)a.val[2];
        T_NG(TRACE_GRP_MIB, "LLC type provided is DSAP:0x%02x SSAP:0x%02x", p.inner.proto.llc_other_proto.dsap,
             p.inner.proto.llc_other_proto.ssap);
        a.state(HandlerState::DONE);
        T_DG(TRACE_GRP_MIB, "Exiting SetHandler after successfully setting AsVclProto for LLC Other Encapsulation");
        return;

    default:
        T_DG(TRACE_GRP_MIB, "Exiting SetHandler without setting AsVclProto - failed to provide valid encapsulation");
        a.state(HandlerState::DONE);
        return;
    }
}

void serialize(vtss::expose::snmp::Reflector &h, AsVclProto &)
{
    h.type_def("VclProtoEncap", vtss::expose::snmp::AsnType::OctetString);
}

// Parent: vtss -------------------=---------------------------------------------
NS(ns_vcl, root, 1, "vclMibObjects");

// Parent: vtss/vcl -------------------------------------------------------------
NS(ns_config, ns_vcl, 2, "vclConfig");

// Parent: vtss/vcl/config ------------------------------------------------------
NS(ns_conf_mac, ns_config, 1, "vclConfigMac");
NS(ns_conf_ip, ns_config, 2, "vclConfigIp");
NS(ns_conf_protocol, ns_config, 3, "vclConfigProtocol");

// Parent: vtss/vcl/config/protocol ---------------------------------------------
NS(ns_conf_proto, ns_conf_protocol, 1, "vclConfigProtocolProto");
NS(ns_conf_group, ns_conf_protocol, 2, "vclConfigProtocolGroup");

// Parent: vtss/vcl/config/mac --------------------------------------------------
static TableReadWriteAddDelete2<MacEntry> entry_mac(
    &ns_conf_mac, OidElement(1, "vclConfigMacTable"),
    OidElement(2, "vclConfigMacRowEditor"));

// Parent: vtss/vcl/config/ip ---------------------------------------------------
static TableReadWriteAddDelete2<IpSubnetEntry> entry_ip(
    &ns_conf_ip, OidElement(1, "vclConfigIpTable"),
    OidElement(2, "vclConfigIpRowEditor"));

// Parent: vtss/vcl/config/protocol/proto ---------------------------------------
static TableReadWriteAddDelete2<ProtocolProtoEntry> entry_proto(
    &ns_conf_proto, OidElement(1, "vclConfigProtocolProtoTable"),
    OidElement(2, "vclConfigProtocolProtoRowEditor"));

// Parent: vtss/vcl/config/protocol/group ---------------------------------------
static TableReadWriteAddDelete2<ProtocolGroupEntry> entry_group(
    &ns_conf_group, OidElement(1, "vclConfigProtocolGroupTable"),
    OidElement(2, "vclConfigProtocolGroupRowEditor"));

