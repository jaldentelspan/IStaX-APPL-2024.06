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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_TRAP_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_TRAP_HXX__

#include <vtss/basics/vector.hxx>
#include <vtss/basics/expose/snmp/vtss_oid.hxx>
#include <vtss/basics/expose/snmp/oid-offset.hxx>
#include <vtss/basics/expose/snmp/struct-base.hxx>
#include <vtss/basics/expose/snmp/pre-get-condition.hxx>

// Forward declaration of net-snmp types to avoid including those headers files
// here.
struct variable_list;
typedef variable_list netsnmp_variable_list;

namespace vtss {
namespace expose {
namespace snmp {

struct TrapHandler {
    typedef TrapHandler &Map_t;

    void add_trap(const Node *noti, const StructBase *obj);
    void add_trap_table(const Node *noti, const StructBase *obj, vtss_oid event);
    void clear();

    template <typename... Args>
    void argument_properties(const Args... args) {
        vtss::meta::VAArgs<Args...> argpack(args...);
        OidOffset *offset = argpack.template get_optional<OidOffset>();
        if (offset) oid_offset_ += offset->i;
    }

    void argument_begin(size_t n, ArgumentType::E type) {}
    void argument_properties_clear() { oid_offset_ = 0; }


    template <typename T, typename... Args>
    void add_leaf(T &&value, const Args... args) {
        typedef SnmpType<typename meta::BaseType<T>::type> SNMP_TYPE;

        // Build the argument pack, which allows the position independent
        // interface
        vtss::meta::VAArgs<Args...> argpack(args...);

        // Store the OID of this index
        oid_element_ = argpack.template get<OidElementValue>().i + oid_offset_;

        // A wrapper class to call the serialize function through.
        SerializeClass<TrapHandler, T, typename SNMP_TYPE::TypeClass> s;

        // If the pre-get-conditions are met, then serialize the varbinding.
        if (check_pre_get_condition(argpack)) {
            s(*this, value);
        }
    }

    template <typename T, typename... Args>
    void add_snmp_leaf(T &&value, const Args &&... args) {
        add_leaf(forward<T>(value), forward<const Args>(args)...);
    }

    template <typename T, typename... Args>
    void add_rpc_leaf(T &&value, const Args &&... args) {}

    TrapHandler &as_tuple() { return *this; }
    TrapHandler &as_ref() { return *this; }

    template <typename... Args>
    TrapHandler &as_map(Args &&... args) { return *this; }

    virtual ~TrapHandler();

    uint32_t oid_element_;
    OidSequence oid_index_;
    OidSequence oid_base_;
    Vector<netsnmp_variable_list *> notification_vars_;
    uint32_t oid_offset_ = 0;
    bool ok_ = true;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_TRAP_HXX__
