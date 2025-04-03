/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_LIST_OF_HANDLERS_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_LIST_OF_HANDLERS_HXX__

#include "vtss/basics/predefs.hxx"

// Used as X-Macro (go google if you have not seen it before)
#define VTSS_SNMP_LIST_OF_COMMON_HANDLERS \
    X(::vtss::expose::snmp::GetHandler)   \
    X(::vtss::expose::snmp::SetHandler)   \
    X(::vtss::expose::snmp::Reflector)

#if defined(VTSS_OPSYS_LINUX)
#define VTSS_SNMP_LIST_OF_HANDLERS VTSS_SNMP_LIST_OF_COMMON_HANDLERS
#else
#define VTSS_SNMP_LIST_OF_HANDLERS \
    VTSS_SNMP_LIST_OF_COMMON_HANDLERS X(::vtss::expose::snmp::InitialTreeWalker)
#endif

namespace vtss {
namespace expose {
namespace snmp {
struct GetHandler;
struct SetHandler;
class Reflector;

#if defined(VTSS_BASICS_OPERATING_SYSTEM_LINUX)
struct InitialTreeWalker;
#endif
}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_LIST_OF_HANDLERS_HXX__
