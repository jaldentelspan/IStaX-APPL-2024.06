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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_TRAP_SERIALIZE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_TRAP_SERIALIZE_HXX__


#include <vtss/basics/expose/snmp/types.hxx>
#include <vtss/basics/expose/snmp/handlers/trap.hxx>

namespace vtss {
namespace expose {
namespace snmp {

void serialize_enum(TrapHandler &h, const int32_t &i,
                    const vtss_enum_descriptor_t *d);
void serialize(TrapHandler &h, const uint64_t &s);
void serialize(TrapHandler &h, const uint32_t &s);
void serialize(TrapHandler &h, const uint16_t &s);
void serialize(TrapHandler &h, const uint8_t &s);
void serialize(TrapHandler &h, const int64_t &s);
void serialize(TrapHandler &h, const int32_t &s);
void serialize(TrapHandler &h, const int16_t &s);
void serialize(TrapHandler &h, const int8_t &s);
void serialize(TrapHandler &h, const bool &s);
void serialize(TrapHandler &h, const AsCounter &s);
void serialize(TrapHandler &h, const AsDisplayString &s);
void serialize(TrapHandler &h, const AsPasswordSetOnly &s);
void serialize(TrapHandler &h, const AsBitMask &s);
void serialize(TrapHandler &h, const AsOctetString &s);
void serialize(TrapHandler &h, const BinaryLen &s);
void serialize(TrapHandler &h, const Ipv4Address &s);
void serialize(TrapHandler &h, const AsIpv4 &s);
void serialize(TrapHandler &h, const mesa_mac_t &s);
void serialize(TrapHandler &h, const mesa_ipv6_t &s);
void serialize(TrapHandler &h, const AsBool &s);
void serialize(TrapHandler &h, const AsRowEditorState &s);
void serialize(TrapHandler &h, const AsPercent &s);
void serialize(TrapHandler &h, const AsInt &s);
void serialize(TrapHandler &h, const AsSnmpObjectIdentifier &s);
void serialize(TrapHandler &h, const AsDecimalNumber &s);
void serialize(TrapHandler &h, AsTimeStampSeconds &s);
void serialize(TrapHandler &h, AsUnixTimeStampSeconds &s);

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_TRAP_SERIALIZE_HXX__
