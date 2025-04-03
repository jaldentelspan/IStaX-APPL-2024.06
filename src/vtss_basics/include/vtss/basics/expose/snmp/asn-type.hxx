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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ASN_TYPE_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ASN_TYPE_HXX__

#include "vtss/basics/stream.hxx"

namespace vtss {
namespace expose {
namespace snmp {
struct AsnType {
    enum E {
        Integer = 0x2,
        IpAddress = 0x40,
        Unsigned = 0x42,
        OctetString = 0x4,
        ObjectIdentifier = 0x6,
        Counter64 = 0x46,
        Integer64 = 0x7a,

        SnmpPduContext_noSuchObject = 0x80,
        SnmpPduContext_noSuchInstance = 0x81,
        SnmpPduContext_endOfMibView = 0x82,
    };

    bool operator!=(const AsnType &rhs) { return data != rhs.data; }

    AsnType() : data(SnmpPduContext_noSuchObject) { }
    AsnType(E e) : data(e) { }

    bool set(uint8_t v);

    E data;
};

ostream& operator<<(ostream &o, const AsnType &rhs);

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ASN_TYPE_HXX__
