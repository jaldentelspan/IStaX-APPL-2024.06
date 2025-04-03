/*

 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_STATUS_SPEC_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_STATUS_SPEC_HXX__

#include "vtss/basics/stream.hxx"
#include <vtss/basics/meta-data-packet.hxx>

namespace vtss {
namespace expose {
namespace snmp {

template <typename T>
struct status_spec_is_a {
    static constexpr bool val = false;
};

template <>
struct status_spec_is_a<Status::E> {
    static constexpr bool val = true;
};

template <typename... T>
void status_spec_print(ostream &o, meta::VAArgs<T...> &vaargs);

template <>
inline void status_spec_print(ostream &o, meta::VAArgs<> &vaargs) {
    o << "current"; // This is default when no status is specified
}

template <typename HEAD, typename... TAIL>
typename meta::enable_if<status_spec_is_a<HEAD>::val, void>::type
status_spec_print(ostream &o, meta::VAArgs<HEAD, TAIL...> &vaargs) {
    o << vaargs.data;
}

template <typename HEAD, typename... TAIL>
typename meta::enable_if<!status_spec_is_a<HEAD>::val, void>::type
status_spec_print(ostream &o, meta::VAArgs<HEAD, TAIL...> &vaargs) {
    return status_spec_print<TAIL...>(o, vaargs.rest);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_STATUS_SPEC_HXX__
