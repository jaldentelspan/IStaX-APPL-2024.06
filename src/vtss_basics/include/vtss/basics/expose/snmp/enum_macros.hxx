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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ENUM_MACROS_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ENUM_MACROS_HXX__

#include "vtss/basics/expose/snmp/types.hxx"

#define VTSS_SNMP_SERIALIZE_ENUM_BASE(N, ASN_NAME, TXT_ARRAY, DESCR)        \
    inline void serialize(::vtss::expose::snmp::OidImporter &h, N &k) {     \
        static_assert(sizeof(N) == sizeof(int), "Size does not match");     \
        ::vtss::expose::snmp::serialize_enum(h, reinterpret_cast<int &>(k), \
                                             TXT_ARRAY);                    \
    }                                                                       \
    inline void serialize(::vtss::expose::snmp::OidExporter &h, N &k) {     \
        static_assert(sizeof(N) == sizeof(int), "Size does not match");     \
        ::vtss::expose::snmp::serialize_enum(h, reinterpret_cast<int &>(k), \
                                             TXT_ARRAY);                    \
    }                                                                       \
    inline void serialize(::vtss::expose::snmp::SetHandler &h, N &k) {      \
        static_assert(sizeof(N) == sizeof(int), "Size does not match");     \
        ::vtss::expose::snmp::serialize_enum(h, reinterpret_cast<int &>(k), \
                                             TXT_ARRAY);                    \
    }                                                                       \
    inline void serialize(::vtss::expose::snmp::GetHandler &h, N &k) {      \
        static_assert(sizeof(N) == sizeof(int), "Size does not match");     \
        ::vtss::expose::snmp::serialize_enum(h, reinterpret_cast<int &>(k), \
                                             TXT_ARRAY);                    \
    }                                                                       \
    inline void serialize(::vtss::expose::snmp::TrapHandler &h, N &k) {     \
        static_assert(sizeof(N) == sizeof(int), "Size does not match");     \
        ::vtss::expose::snmp::serialize_enum(h, reinterpret_cast<int &>(k), \
                                             TXT_ARRAY);                    \
    }

#define VTSS_SNMP_SERIALIZE_ENUM(N, ASN_NAME, TXT_ARRAY, DESCR)              \
    VTSS_SNMP_SERIALIZE_ENUM_BASE(N, ASN_NAME, TXT_ARRAY, DESCR);            \
    inline void serialize(::vtss::expose::snmp::Reflector &h, N &k) {        \
        ::vtss::expose::snmp::serialize_enum(h, ASN_NAME, DESCR, TXT_ARRAY); \
    }


#define VTSS_SNMP_SERIALIZE_ENUM_SHARED(N, ASN_NAME, TXT_ARRAY, DESCR) \
    VTSS_SNMP_SERIALIZE_ENUM_BASE(N, ASN_NAME, TXT_ARRAY, DESCR);      \
    inline void serialize(::vtss::expose::snmp::Reflector &h, N &k) {  \
        h.type_def(ASN_NAME, ::vtss::expose::snmp::AsnType::Integer);  \
    }

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ENUM_MACROS_HXX__
