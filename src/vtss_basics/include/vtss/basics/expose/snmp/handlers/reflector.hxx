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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_REFLECTOR_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_REFLECTOR_HXX__

#include <ctype.h>
#include "vtss/basics/expose/snmp/types.hxx"
#include "vtss/basics/depend-on-capability.hxx"
#include <stdio.h>
#include <string>

namespace vtss {
namespace expose {
namespace snmp {

class Reflector {
  public:
    typedef Reflector &Map_t;
    friend void serialize(Reflector &, NamespaceNode &);
    enum class TrapType { NO_TRAP, SCALAR, ADD, MODIFY, DELETE };

    virtual ~Reflector() {}
    virtual void argument_begin(size_t n, ArgumentType::E type) = 0;
    virtual void type_def(const char *name, AsnType::E type) = 0;
    virtual void type_ref(const char *name, const char *from, AsnType::E type) = 0;
    virtual void type_range(std::string s) = 0;
    virtual void type_enum(const char *name, const char *desc,
                           const vtss_enum_descriptor_t *d) = 0;
    virtual void node_begin(const OidElement &n, const char *depends_on) = 0;
    virtual void node_begin(const OidElement &n, MaxAccess::E a,
                            const char *depends_on) = 0;
    virtual void node_end(const char *depends_on) = 0;
    virtual void table_node_begin(const OidElement &n, MaxAccess::E a,
                                  const char *table_desc, const char *index_desc,
                                  const char *depends_on) = 0;
    virtual void table_node_end(const char *depends_on) = 0;
    virtual void add_leaf_(const char *name, const char *desc, int oid,
                           const char *range, const char *depends, const char *status) = 0;
    virtual void add_capability(const char *name) = 0;

    virtual void trap_begin(const OidElement &n, TrapType t) = 0;
    virtual void trap_end() = 0;

    static constexpr bool need_all_elements = true;
    static constexpr bool is_importer = false;
    static constexpr bool is_exporter = false;

    template <typename T, typename... Args>
    void capability(const Args... args) {
        auto val = T::get();
        add_leaf(val, vtss::tag::Name(T::name), vtss::tag::Description(T::desc),
                 forward<const Args>(args)...);
        add_capability(T::name);
    }

    template <typename T, typename... Args>
    void add_leaf(T &&value, const Args... args) {
        if (device_specific_ &&
            !(check_depend_on_capability(forward<const Args>(args)...)))
            return;

        vtss::meta::VAArgs<Args...> argpack(args...);

        StringStream ss;
        range_spec_print(ss, argpack);
        StringStream st;
        status_spec_print(st, argpack);
        add_leaf_(argpack.template get<tag::Name>().s,
                  argpack.template get<tag::Description>().s,
                  argpack.template get<OidElementValue>().i + oid_offset_,
                  ss.cstring(),
                  depend_on_capability_name(forward<const Args>(args)...),
                  st.cstring() );

        serialize(*this, value);
    }

    template <typename T, typename... Args>
    void add_snmp_leaf(T &&value, const Args &&... args) {
        add_leaf(forward<T>(value), forward<const Args>(args)...);
    }

    template <typename T, typename... Args>
    void add_rpc_leaf(T &&value, const Args &&... args) {}

    template <typename... Args>
    Reflector &as_map(Args &&...) {
        return *this;
    }

    template <typename... Args>
    void argument_properties(const Args... args) {
        vtss::meta::VAArgs<Args...> argpack(args...);
        OidOffset *offset = argpack.template get_optional<OidOffset>();
        if (offset) oid_offset_ += offset->i;
    }

    void argument_properties_clear() { oid_offset_ = 0; }

    uint32_t oid_offset_ = 0;
    uint32_t recursive_call = 0;
    bool device_specific_ = false;
};

void serialize(Reflector &g, NamespaceNode &o);
void serialize(Reflector &h, AsBitMask &ds);
void serialize(Reflector &h, AsBool &);
void serialize(Reflector &h, AsCounter &);
void serialize(Reflector &h, AsDisplayString &ds);
void serialize(Reflector &h, AsInt &);
void serialize(Reflector &h, AsIpv4 &val);
void serialize(Reflector &h, AsOctetString &val);
void serialize(Reflector &h, AsPasswordSetOnly &ds);
void serialize(Reflector &h, AsPercent &);
void serialize(Reflector &h, AsRowEditorState &);
void serialize(Reflector &h, AsSnmpObjectIdentifier &val);
void serialize(Reflector &h, AsTimeStampSeconds &v);
void serialize(Reflector &h, AsUnixTimeStampSeconds &v);
void serialize(Reflector &h, AsDecimalNumber &val);
void serialize(Reflector &h, BinaryLen &val);
void serialize(Reflector &h, BinaryU32Len &val);
void serialize(Reflector &h, Ipv4Address &val);
void serialize(Reflector &h, bool &);
void serialize(Reflector &h, int16_t &);
void serialize(Reflector &h, int32_t &);
void serialize(Reflector &h, int64_t &);
void serialize(Reflector &h, int8_t &);
void serialize(Reflector &h, uint16_t &);
void serialize(Reflector &h, uint32_t &v);
void serialize(Reflector &h, uint64_t &v);
void serialize(Reflector &h, uint8_t &);
void serialize(Reflector &h, mesa_ipv6_t &val);
void serialize(Reflector &h, mesa_mac_t &val);
void serialize_enum(Reflector &h, const char *name, const char *desc,
                    const vtss_enum_descriptor_t *d);

////////////////////////////////////////////////////////////////////////////////
template <typename... T>
void serialize(Reflector &a, StructROBase<T...> &o,
               MaxAccess::E default_max_access) {
    a.node_begin(o.element(), default_max_access, nullptr);

    typename StructROBase<T...>::ValueType value;
    value.do_serialize(a);

    a.node_end(nullptr);
}

template <typename... T>
void serialize(Reflector &a, StructROBase<T...> &o) {
    serialize(a, o, MaxAccess::ReadOnly);
}

template <typename... T>
void serialize(Reflector &a, StructRWBase<T...> &o) {
    serialize(a, o, MaxAccess::ReadWrite);
}

template <typename... T>
void serialize(Reflector &g, TableReadOnly<T...> &o, MaxAccess::E a) {
    g.table_node_begin(o.element(), a, o.table_description, o.index_description,
                       nullptr);

    typename StructROBase<T...>::ValueType value;
    value.do_serialize(g);

    g.table_node_end(nullptr);
}

template <typename... T>
void serialize(Reflector &g, TableReadOnly<T...> &o) {
    serialize(g, o, MaxAccess::ReadOnly);
}

template <typename... T>
void serialize(Reflector &g, TableReadWrite<T...> &o) {
    serialize(g, o, MaxAccess::ReadWrite);
}

////////////////////////////////////////////////////////////////////////////////
template <typename INTERFACE>
void serialize_struct(Reflector &a, Node &o, MaxAccess::E default_max_access) {
    if (a.device_specific_ && !(tag::check_depend_on_capability_t<INTERFACE>()))
        return;

    a.node_begin(o.element(), default_max_access,
                 tag::depend_on_capability_name_t<INTERFACE>());

    INTERFACE i;
    typename Interface2Any<ParamTuple, INTERFACE>::type values;
    values.do_serialize2(a, i);

    a.node_end(tag::depend_on_capability_name_t<INTERFACE>());
}

template <typename INTERFACE>
void serialize(Reflector &a, StructROBase2<INTERFACE> &o) {
    serialize_struct<INTERFACE>(a, o, MaxAccess::ReadOnly);
}

template <typename INTERFACE>
void serialize(Reflector &a, StructRoTrap<INTERFACE> &o) {
    a.trap_begin(o.element(), Reflector::TrapType::SCALAR);
    serialize_struct<INTERFACE>(a, o.struct_ro_subject, MaxAccess::ReadOnly);
    a.trap_end();
}

template <typename INTERFACE>
void serialize(Reflector &a, TableReadOnlyTrap<INTERFACE> &o) {
    INTERFACE i;
    typename Interface2Any<ParamTuple, INTERFACE>::type values;

    a.trap_begin(o.trap_element_add_, Reflector::TrapType::ADD);
    a.node_begin(o.subject.element(), MaxAccess::ReadOnly,
                 tag::depend_on_capability_name_t<INTERFACE>());
    values.do_serialize2(a, i);
    a.node_end(tag::depend_on_capability_name_t<INTERFACE>());
    a.trap_end();

    a.trap_begin(o.trap_element_mod_, Reflector::TrapType::MODIFY);
    a.node_begin(o.subject.element(), MaxAccess::ReadOnly,
                 tag::depend_on_capability_name_t<INTERFACE>());
    values.do_serialize2(a, i);
    a.node_end(tag::depend_on_capability_name_t<INTERFACE>());
    a.trap_end();

    a.trap_begin(o.trap_element_del_, Reflector::TrapType::DELETE);
    a.node_begin(o.subject.element(), MaxAccess::ReadOnly,
                 tag::depend_on_capability_name_t<INTERFACE>());
    values.do_serialize2_keys(a, i);
    a.node_end(tag::depend_on_capability_name_t<INTERFACE>());
    a.trap_end();
}

template <typename INTERFACE>
void serialize(Reflector &a, StructRoSubject<INTERFACE> &o) {
    serialize_struct<INTERFACE>(a, o, MaxAccess::ReadOnly);
}

template <typename INTERFACE>
void serialize(Reflector &a, StructRWBase2<INTERFACE> &o) {
    serialize_struct<INTERFACE>(a, o, MaxAccess::ReadWrite);
}

template <typename INTERFACE>
void serialize(Reflector &g, TableReadOnly2<INTERFACE> &o, MaxAccess::E a) {
    if (g.device_specific_ && !(tag::check_depend_on_capability_t<INTERFACE>()))
        return;

    g.table_node_begin(o.element(), a, INTERFACE::table_description,
                       INTERFACE::index_description,
                       tag::depend_on_capability_name_t<INTERFACE>());

    INTERFACE i;
    typename Interface2Any<ParamTuple, INTERFACE>::type values;
    values.do_serialize2(g, i);

    g.table_node_end(tag::depend_on_capability_name_t<INTERFACE>());
}

template <typename INTERFACE>
void serialize(Reflector &g, TableReadOnly2<INTERFACE> &o) {
    serialize(g, o, MaxAccess::ReadOnly);
}

template <typename INTERFACE>
void serialize(Reflector &g, TableReadOnlySubject<INTERFACE> &o) {
    if (g.device_specific_ && !(tag::check_depend_on_capability_t<INTERFACE>()))
        return;

    g.table_node_begin(o.element(), MaxAccess::ReadOnly,
                       INTERFACE::table_description, INTERFACE::index_description,
                       tag::depend_on_capability_name_t<INTERFACE>());

    INTERFACE i;
    typename Interface2Any<ParamTuple, INTERFACE>::type values;
    values.do_serialize2(g, i);

    g.table_node_end(tag::depend_on_capability_name_t<INTERFACE>());
}

template <typename INTERFACE>
void serialize(Reflector &g, TableReadWrite2<INTERFACE> &o) {
    serialize(g, o, MaxAccess::ReadWrite);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_REFLECTOR_HXX__
