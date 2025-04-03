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

#ifndef __VTSS_BASICS_EXPOSE_JSON_SERIALIZE_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_SERIALIZE_HXX__

#include <vtss/basics/types.hxx>
#include <vtss/basics/string.hxx>
#include <vtss/basics/api_types.h>
#include <vtss/basics/formatting_tags.hxx>
#include <vtss/basics/expose/json/pre-loader.hxx>
#include <vtss/basics/expose/json/pre-exporter.hxx>
#include <vtss/basics/expose/json/pre-handler-reflector.hxx>

#define DECL(X)                                                 \
    void serialize(vtss::expose::json::Exporter &e, const X &); \
    void serialize(vtss::expose::json::Loader &e, X &);         \
    void serialize(vtss::expose::json::HandlerReflector &e, X &);
DECL(mesa_ip_addr_t);
DECL(mesa_ip_network_t);
DECL(mesa_ipv4_network_t);
DECL(mesa_ipv6_network_t);
DECL(mesa_ipv6_t);
DECL(mesa_mac_t);
#undef DECL

namespace std {
#define DECL(X)                                                 \
    void serialize(vtss::expose::json::Exporter &e, const X &); \
    void serialize(vtss::expose::json::Loader &e, X &);         \
    void serialize(vtss::expose::json::HandlerReflector &e, X &);
DECL(std::string);
#undef DECL
}  // namespace std

namespace vtss {

#define DECL(X)                                           \
    void serialize(expose::json::Exporter &e, const X &); \
    void serialize(expose::json::Loader &e, X &);         \
    void serialize(expose::json::HandlerReflector &e, X &);
DECL(Buf);
DECL(IpAddress);
DECL(IpNetwork);
DECL(Ipv4Address);
DECL(Ipv4Network);
DECL(Ipv6Address);
DECL(Ipv6Network);
DECL(MacAddress);
DECL(AsDecimalNumber);
#undef DECL

#define DECL(X)                                   \
    void serialize(expose::json::Exporter &e, X); \
    void serialize(expose::json::Loader &e, X);   \
    void serialize(expose::json::HandlerReflector &e, X);
DECL(AsBitMask);
DECL(AsBool);
DECL(AsCounter);
DECL(AsDisplayString);
DECL(AsPasswordSetOnly);
DECL(AsInt);
DECL(AsIpv4);
DECL(AsOctetString);
DECL(AsPercent);
DECL(AsRowEditorState);
DECL(AsUnixTimeStampSeconds);
DECL(Binary);
DECL(BinaryLen);
DECL(FormatHex<uint8_t>);
DECL(FormatHex<uint16_t>);
DECL(FormatHex<uint32_t>);
DECL(AsSnmpObjectIdentifier);
#undef DECL

void serialize(expose::json::Exporter &e, const str &);

namespace expose {
namespace json {
#define DECL(X)                             \
    void serialize(Exporter &e, const X &); \
    void serialize(Loader &e, X &);         \
    void serialize(HandlerReflector &e, X &);
DECL(bool);

DECL(int8_t);
DECL(int16_t);
DECL(int32_t);
DECL(int64_t);

DECL(uint8_t);
DECL(uint16_t);
DECL(uint32_t);
DECL(uint64_t);
#undef DECL


}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_SERIALIZE_HXX__
