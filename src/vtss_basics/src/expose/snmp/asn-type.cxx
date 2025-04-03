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

#include "vtss/basics/expose/snmp/asn-type.hxx"

namespace vtss {
namespace expose {
namespace snmp {

bool AsnType::set(uint8_t v) {
    switch (v) {
        case Integer:
            data = Integer;
            return true;

        case Unsigned:
            data = Unsigned;
            return true;

        case IpAddress:
            data = IpAddress;
            return true;

        case OctetString:
            data = OctetString;
            return true;

        case ObjectIdentifier:
            data = ObjectIdentifier;
            return true;

        case Counter64:
            data = Counter64;
            return true;
    }

    return false;
}

ostream& operator<<(ostream &o, const AsnType &rhs) {
#define CASE(X) case AsnType::X:\
        o << #X << "(" << static_cast<int32_t>(rhs.data) << ")"; \
        break

    switch (rhs.data) {
      CASE(Integer);
      CASE(IpAddress);
      CASE(Unsigned);
      CASE(OctetString);
      CASE(ObjectIdentifier);
      CASE(Counter64);
      CASE(SnmpPduContext_noSuchObject);
      CASE(SnmpPduContext_noSuchInstance);
      CASE(SnmpPduContext_endOfMibView);
      default:
        o << "<unknown(" << static_cast<int32_t>(rhs.data) << ")>";
        break;
    }
#undef CASE

    return o;
}

} //  namespace snmp
}  // namespace expose
} //  namespace vtss
