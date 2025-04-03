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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMMON_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMMON_HXX__

#include "vtss/basics/expose/snmp/max-access.hxx"
#include "vtss/basics/expose/snmp/oid_sequence.hxx"
#include "vtss/basics/expose/snmp/param-list-converter.hxx"
#include "vtss/basics/expose/snmp/list-of-handlers.hxx"
//#include "vtss/basics/expose/snmp/handlers/get.hxx"

namespace vtss {
namespace expose {
namespace snmp {

class IteratorCommon {
  public:
    IteratorCommon(MaxAccess::E a) : max_access(a) {}
    virtual ~IteratorCommon() {}

    virtual mesa_rc get_next(const OidSequence &idx, OidSequence &next) = 0;
    virtual mesa_rc get(const OidSequence &oid) = 0;
    virtual mesa_rc set() = 0;
    virtual mesa_rc undo() = 0;

    virtual void call_serialize(GetHandler &h) = 0;
    virtual void call_serialize_values(GetHandler &h) = 0;

    virtual void call_serialize(SetHandler &h) = 0;
    virtual void call_serialize_values(SetHandler &h) = 0;

    MaxAccess::E max_access;

    // for debugging only
    virtual void *set_ptr__() const = 0;
    virtual void *get_ptr__() const = 0;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMMON_HXX__
