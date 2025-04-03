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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_NETSNMP_PASS_PERSIST_INITIAL_TREE_WALKER_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_NETSNMP_PASS_PERSIST_INITIAL_TREE_WALKER_HXX__

#include <vtss/basics/stream.hxx>
#include <vtss/basics/expose/snmp/oid_inventory.hxx>
#include <vtss/basics/expose/snmp/types.hxx>
#include <vtss/basics/expose/snmp/param-list-converter.hxx>

namespace vtss {
namespace expose {
namespace snmp {

struct InitialTreeWalker {
    typedef InitialTreeWalker &Map_t;
    static constexpr bool need_all_elements = false;
    static constexpr bool is_importer = false;
    static constexpr bool is_exporter = true;

    InitialTreeWalker(OidInventory &i, const OidSequence &prefix)
        : inventory(i), oid_seq_(prefix) {}

    template <typename... Args>
    InitialTreeWalker &as_map(Args &&... args) {
        return *this;
    }

    template <typename T, typename... Args>
    void add_param_value(T &&value, const Args... args) {
        add_leaf(value, args...);
    }

    template <typename T, typename... Args>
    void add_leaf(T &&value, const Args... args) {
        vtss::meta::VAArgs<Args...> argpack(args...);
        int oid = argpack.template get<OidElementValue>().i + oid_offset_;
        add_leaf_(oid);
    }

    template <typename T, typename... Args>
    void add_snmp_leaf(T &&value, const Args &&... args) {
        add_leaf(forward<T>(value), forward<const Args>(args)...);
    }

    template <typename T, typename... Args>
    void add_rpc_leaf(T &&value, const Args &&... args) {}

    void add_leaf_(int oid);

    void argument_begin(size_t n, ArgumentType::E type) {}

    template <typename... Args>
    void argument_properties(const Args... args) {
        vtss::meta::VAArgs<Args...> argpack(args...);
        OidOffset *offset = argpack.template get_optional<OidOffset>();
        if (offset) oid_offset_ += offset->i;
    }

    template <typename T, typename... Args>
    void capability(const Args... args) {
        auto val = T::get();
        add_leaf(val, vtss::tag::Name(T::name), vtss::tag::Description(T::desc),
                 forward<const Args>(args)...);
        //add_capability(T::name);
    }

    void argument_properties_clear() { oid_offset_ = 0; }

    OidInventory &inventory;
    OidSequence oid_seq_;
    uint32_t recursive_call = 0;
    uint32_t oid_offset_ = 0;
};

ostream &operator<<(ostream &o, const InitialTreeWalker &v);

void serialize(InitialTreeWalker &c, NamespaceNode &o);

template <typename... T>
void serialize(InitialTreeWalker &h, StructROBase<T...> &o) {
    typename StructROBase<T...>::ValueType value;

    h.oid_seq_.push(o.element().numeric_);
    value.do_serialize(h);
    h.oid_seq_.pop();
}

template <typename T>
void serialize(InitialTreeWalker &h, StructROBase2<T> &o) {
    T interface_descriptor;
    typename StructROBase2<T>::ValueType value;

    h.oid_seq_.push(o.element().numeric_);
    value.do_serialize2(h, interface_descriptor);
    h.oid_seq_.pop();
}

template <typename T>
void serialize(InitialTreeWalker &h, StructRoTrap<T> &o) {
    // Traps is not needed for the initial tree-walk
}

template <typename T>
void serialize(InitialTreeWalker &h, TableReadOnlyTrap<T> &o) {
    // Traps is not needed for the initial tree-walk
}

template <typename T>
void serialize(InitialTreeWalker &h, StructRoSubject<T> &o) {
    T interface_descriptor;
    typename StructRoSubject<T>::ValueType value;

    h.oid_seq_.push(o.element().numeric_);
    value.do_serialize2(h, interface_descriptor);
    h.oid_seq_.pop();
}

template <class... Args>
struct WalkParamTuple;
template <>
struct WalkParamTuple<> {
    template <typename HANDLER>
    void do_serialize(HANDLER &h) {}

    template <typename HANDLER>
    void do_serialize_(HANDLER &h) {}
};

template <class Head, class... Tail>
struct WalkParamTuple<Head, Tail...> {
    Head data;
    WalkParamTuple<Tail...> rest;

    template <typename HANDLER>
    void do_serialize_(
            typename meta::enable_if<!Head::is_key, HANDLER>::type &h) {
        auto &m = data.data.as_meta_type();
        serialize(h, m);
        rest.template do_serialize_<HANDLER>(h);
    }

    template <typename HANDLER>
    void do_serialize_(
            typename meta::enable_if<Head::is_key, HANDLER>::type &h) {
        rest.template do_serialize<HANDLER>(h);
    }

    template <typename HANDLER>
    void do_serialize(HANDLER &h) {
        do_serialize_<HANDLER>(h);
    }
};

template <typename... T>
void serialize(InitialTreeWalker &h, TableReadOnly<T...> &o) {
    WalkParamTuple<T...> v;
    h.oid_seq_.push(o.element().numeric_);
    h.oid_seq_.push(1);
    v.do_serialize(h);
    h.oid_seq_.pop();
    h.oid_seq_.pop();
}

template <typename T>
void serialize(InitialTreeWalker &h, TableReadOnly2<T> &o) {
    T interface;
    typename Interface2ParamTuple<T>::type v;

    h.oid_seq_.push(o.element().numeric_);
    h.oid_seq_.push(1);
    v.do_serialize2_values(h, interface);
    h.oid_seq_.pop();
    h.oid_seq_.pop();
}

template <typename T>
void serialize(InitialTreeWalker &h, TableReadOnlySubject<T> &o) {
    T interface;
    typename Interface2ParamTuple<T>::type v;

    h.oid_seq_.push(o.element().numeric_);
    h.oid_seq_.push(1);
    v.do_serialize2_values(h, interface);
    h.oid_seq_.pop();
    h.oid_seq_.pop();
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_NETSNMP_PASS_PERSIST_INITIAL_TREE_WALKER_HXX__
