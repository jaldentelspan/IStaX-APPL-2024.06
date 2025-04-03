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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_ONLY_TRAP_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_ONLY_TRAP_HXX__

#include <vtss/basics/expose/types.hxx>
#include <vtss/basics/expose/param-list.hxx>
#include <vtss/basics/expose/table-status.hxx>

#include <vtss/basics/expose/snmp/types_params.hxx>
#include <vtss/basics/expose/snmp/struct-base-trap.hxx>
#include <vtss/basics/expose/snmp/list-of-handlers.hxx>
#include <vtss/basics/expose/snmp/param-list-converter.hxx>
#include <vtss/basics/expose/snmp/table-read-only-subject.hxx>

#include <vtss/basics/expose/snmp/handlers/trap.hxx>

#include <vtss/basics/notifications/event.hxx>

namespace vtss {
namespace expose {
namespace snmp {

template <class INTERFACE>
struct TableReadOnlyTrap : public StructBaseTrap {
    typedef TableStatusInterface<INTERFACE> Subject;
    typedef typename Subject::Observer Observer;

    TableReadOnlyTrap(NamespaceNode *obj_parent, const OidElement &obj_element,
                      Subject *s, NamespaceNode *trap_parent,
                      const char *trap_name, const OidElement &trap_element_add,
                      const OidElement &trap_element_mod,
                      const OidElement &trap_element_del)

        : StructBaseTrap(trap_parent,
                         OidElement(trap_element_add.numeric(), trap_name)),
          subject(obj_parent, obj_element, s),
          trap_element_add_(trap_element_add),
          trap_element_mod_(trap_element_mod),
          trap_element_del_(trap_element_del){};

    TableReadOnlyTrap(const TableReadOnlyTrap &) = delete;
    TableReadOnlyTrap &operator=(const TableReadOnlyTrap &) = delete;

    mesa_rc observer_new(notifications::Event *ev) {
        return subject.observer_new(ev);
    };

    mesa_rc observer_get(notifications::Event *ev, TrapHandler &h) {
        return subject.observer_get(ev, h, parent_, &trap_element_add_,
                                    &trap_element_mod_, &trap_element_del_);
    }

    mesa_rc observer_del(notifications::Event *ev) {
        return subject.observer_del(ev);
    }

#define X(H) \
    void do_serialize(H &h) { serialize(h, *this); }
    VTSS_SNMP_LIST_OF_HANDLERS
#undef X

    TableReadOnlySubject<INTERFACE> subject;

    const OidElement trap_element_add_;
    const OidElement trap_element_mod_;
    const OidElement trap_element_del_;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_TABLE_READ_ONLY_TRAP_HXX__
