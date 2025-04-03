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

#ifndef _TYPES_HXX_
#define _TYPES_HXX_

#include <stdint.h>

#include <main_types.h>

#include "vtss/basics/string.hxx"

namespace vtss {
// Basic scalar types /////////////////////////////////////////////////////////
//ostream& operator<< (ostream&, const char);
//ostream& operator<< (ostream&, const char*);
//ostream& operator<< (ostream&, const str&);
//
//ostream& operator<< (ostream&, int);
//ostream& operator<< (ostream&, short);
//ostream& operator<< (ostream&, unsigned int);
//ostream& operator<< (ostream&, unsigned short);
//ostream& operator<< (ostream&, unsigned char);
//
//ostream& operator<< (ostream&, const FormatHex<int>&);
//ostream& operator<< (ostream&, const FormatHex<short>&);
//ostream& operator<< (ostream&, const FormatHex<char>&);
//ostream& operator<< (ostream&, const FormatHex<unsigned int>&);
//ostream& operator<< (ostream&, const FormatHex<unsigned short>&);
//ostream& operator<< (ostream&, const FormatHex<unsigned char>&);
// End of basic scalar types //////////////////////////////////////////////////

//// IPv4 ///////////////////////////////////////////////////////////////////////
//struct Ipv4
//{
//    Ipv4() : raw(0) { }
//    Ipv4(const Ipv4& rhs) : raw(rhs.raw) { }
//    explicit Ipv4(::mesa_ipv4_t c_type) : raw(c_type) { }
//
//    Ipv4& operator=(const Ipv4& rhs) {
//        raw = rhs.raw;
//        return *this;
//    }
//
//    ::mesa_ipv4_t c_type() const {
//        return raw;
//    }
//
//    ::mesa_ipv4_t raw;
//};
//
//inline bool operator==(const Ipv4& r, const Ipv4& l)
//{
//    return r.raw == l.raw;
//}
//
//inline bool operator!=(const Ipv4& r, const Ipv4& l)
//{
//    return r.raw != l.raw;
//}
//
//inline bool operator<(const Ipv4& r, const Ipv4& l)
//{
//    return r.raw < l.raw;
//}
//
//inline bool operator>(const Ipv4& r, const Ipv4& l)
//{
//    return r.raw > l.raw;
//}
//
//inline bool operator<=(const Ipv4& r, const Ipv4& l)
//{
//    return r.raw <= l.raw;
//}
//
//inline bool operator>=(const Ipv4& r, const Ipv4& l)
//{
//    return r.raw >= l.raw;
//}
//
//ostream& operator<< (ostream&, const Ipv4&);
//ostream& operator<< (ostream&, const FormatHex<Ipv4>&);
//// End of Ipv4 ////////////////////////////////////////////////////////////////
//
//// IPv6
//typedef ::mesa_ipv6_t Ipv6;
//bool operator==(const Ipv6&, const Ipv6&);
//bool operator!=(const Ipv6&, const Ipv6&);
//bool operator<(const Ipv6&, const Ipv6&);
//ostream& operator<< (ostream&, const Ipv6&);
//bool parse(const str&, Ipv6&);
//
//// IP
//
//// IPv4 network
//struct Ipv4Network {
//    Ipv4Network () : prefix(0) { }
//    Ipv4Network (const Ipv4& ip, unsigned prefix_) :
//        address(ip), prefix(prefix_) { }
//
//    Ipv4 address;
//    unsigned prefix;
//};
//
//ostream& operator<< (ostream&, const Ipv4Network&);
//bool parse(const str&, Ipv4Network&);
//
//// IPv6 network
//struct Ipv6Network {
//    Ipv6Network () : prefix(0) { }
//    Ipv6Network (const Ipv6& ip, unsigned prefix_) :
//        address(ip), prefix(prefix_) { }
//
//    Ipv6 address;
//    unsigned prefix;
//};
//
//// IP network
//
//ostream& operator<< (ostream&, const Ipv6Network&);
//bool parse(const str&, Ipv6Network&);


} /* vtss */


#endif /* _TYPES_HXX_ */

