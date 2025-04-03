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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RO_TRAP_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RO_TRAP_HXX__


#include <vtss/basics/expose/types.hxx>
#include <vtss/basics/expose/param-list.hxx>
#include <vtss/basics/expose/struct-status.hxx>

#include <vtss/basics/expose/snmp/types_params.hxx>
#include <vtss/basics/expose/snmp/struct-base-trap.hxx>
//#include <vtss/basics/expose/snmp/list-of-handlers.hxx>
#include <vtss/basics/expose/snmp/struct-ro-subject.hxx>
#include <vtss/basics/expose/snmp/param-list-converter.hxx>

#include <vtss/basics/expose/snmp/handlers/trap.hxx>

#include <vtss/basics/notifications/event.hxx>

namespace vtss {
namespace expose {
namespace snmp {

template <class INTERFACE>
struct StructRoTrap : public StructBaseTrap {
    typedef typename Interface2ParamTuple<INTERFACE>::type ValueType;
    typedef StructStatusInterface<INTERFACE> Subject;

    StructRoTrap(NamespaceNode *obj_parent, const OidElement &obj_element,
                 NamespaceNode *trap_parent, const OidElement &trap_element,
                 Subject *s)
        : StructBaseTrap(trap_parent, trap_element),
          struct_ro_subject(obj_parent, obj_element, s){};

    virtual ~StructRoTrap() {}

    mesa_rc observer_new(notifications::Event *ev) {
        return struct_ro_subject.observer_new(ev);
    };

    mesa_rc observer_get(notifications::Event *ev, TrapHandler &h) {
        return struct_ro_subject.observer_get(ev, h, this);
    }

    mesa_rc observer_del(notifications::Event *ev) {
        return struct_ro_subject.observer_del(ev);
    }

    virtual unique_ptr<IteratorCommon> new_iterator() {
        return unique_ptr<IteratorCommon>(nullptr);
    }

#define X(H) \
    void do_serialize(H &h) { serialize(h, *this); }
    VTSS_SNMP_LIST_OF_HANDLERS
#undef X

    StructRoSubject<INTERFACE> struct_ro_subject;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_STRUCT_RO_TRAP_HXX__
