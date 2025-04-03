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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_MIB_GENERATOR_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_MIB_GENERATOR_HXX__

#include <string>
#include <vtss/basics/set.hxx>
#include <vtss/basics/map.hxx>
#include <vtss/basics/vector.hxx>
#include <vtss/basics/expose/snmp/handlers/reflector.hxx>

namespace vtss {
namespace expose {
namespace snmp {

class MibGenerator : public Reflector {
  public:
    typedef MibGenerator &Map_t;
    friend void serialize(MibGenerator &, NamespaceNode &);

    static constexpr bool need_all_elements = true;
    static constexpr bool is_importer = false;
    static constexpr bool is_exporter = false;

    void go(NamespaceNode &ns);

    void argument_begin(size_t n, ArgumentType::E type);

    explicit MibGenerator(ostream &o);

    void type_def(const char *name, AsnType::E type);
    void type_ref(const char *name, const char *from, AsnType::E type);
    void type_range(std::string s);
    void type_enum(const char *name, const char *desc,
                   const vtss_enum_descriptor_t *d);
    void node_begin(const OidElement &n, const char *depends_on);
    void node_begin(const OidElement &n, MaxAccess::E a, const char *depends_on);
    void node_end(const char *depends_on);
    void table_node_begin(const OidElement &n, MaxAccess::E a,
                          const char *table_desc, const char *index_desc,
                          const char *depends_on);
    void table_node_end(const char *depends_on);
    void add_leaf_(const char *name, const char *desc, int oid,
                   const char *range, const char *depends, const char *status);

    void trap_begin(const OidElement &n, TrapType t);
    void trap_end();

    void add_capability(const char *name);

    // Meta-data
    void add_history_element(const char *data, const char *desc);
    void conformance_oid(uint32_t o) { conformance_oid_ = o; }
    void parent(const char *name, const char *import);
    void contact(const char *org, const char *name);
    void description(const char *desc);
    void name_prefix(const char *n);
    void definition_name(const char *n);

  private:
    struct Leaf;
    struct Table;
    struct Namespace;
    struct TrapNamespace;

    typedef std::shared_ptr<Leaf> LeafPtr;
    typedef std::shared_ptr<Table> TablePtr;
    typedef std::shared_ptr<Namespace> NamespacePtr;
    typedef std::shared_ptr<TrapNamespace> TrapNamespacePtr;

    typedef Vector<LeafPtr> LeafList;
    typedef Vector<NamespacePtr> NamespaceList;

    ostream &o_;
    uint32_t conformance_oid_ = 2;
    MaxAccess::E max_access = MaxAccess::NotAccessible;

    void build_mib();

    void build_mib_table_name(ostream &o, LeafPtr c);
    void build_mib_table_name(ostream &o, TablePtr c);
    void build_mib_table_entry_name(ostream &o, TablePtr c);
    void build_mib_table_entry_type(ostream &o, TablePtr c);

    void build_mib_object_name(ostream &o, LeafPtr c);
    void build_mib_object_name(LeafPtr c);
    void build_mib_object_name(ostream &o, NamespacePtr c);
    void build_mib_object_name(NamespacePtr c);

    void build_mib_object_name_raw(ostream &o, LeafPtr c);
    void build_mib_object_name_raw(LeafPtr c);
    void build_mib_object_name_raw(ostream &o, NamespacePtr c);
    void build_mib_object_name_raw(NamespacePtr c);

    void build_mib_parent_object_name(LeafPtr c);
    void build_mib_parent_object_name(NamespacePtr c);
    void build_mib_each_child(const NamespacePtr ns);
    void build_mib_ns(const NamespacePtr ns);
    void build_mib_trap(const TrapNamespacePtr ns);
    void build_mib_leaf(const LeafPtr l);
    void build_mib_table(const TablePtr ns);
    void build_conformance();
    void build_conformance_each_child(const NamespacePtr root,
                                      const NamespacePtr ns,
                                      NamespaceList &groups, uint32_t &oid);
    void build_conformance_new_group(const NamespacePtr root,
                                     const NamespacePtr ns, const LeafList &g,
                                     uint32_t &oid_);
    void build_conformance_trap(const NamespacePtr root, const NamespacePtr ns,
                                const NamespaceList &groups, uint32_t &oid_);
    void build_compliance(const NamespacePtr root, const NamespaceList &list);
    void build_imports();
    void build_types();

    struct HistoryElement {
        std::string date, desc;
    };

    enum NodeType { LEAF, NAMESPACE };

    struct Common {
        Common(NodeType t, uint32_t o, std::string n)
            : type(t), oid(o), name(n) {}
        const NodeType type;
        uint32_t oid;
        std::string name;
        std::weak_ptr<Namespace> parent;
    };

    struct Leaf : public Common {
        Leaf(uint32_t o, std::string n, std::string d, std::string st, MaxAccess::E a,
             std::string r, const char *de)
                : Common(LEAF, o, n), desc(d), status(st), access(a), range(r), depends_on(de) {}
        std::string desc;
        std::string type;
        std::string status;
        MaxAccess::E access;
        bool table_leaf = false;
        std::string range;
        const char *depends_on;
    };

    enum NamespaceType { PLAIN, TABLE, TRAP };

    struct Namespace : public Common {
        Namespace(uint32_t o, std::string n)
            : Common(NAMESPACE, o, n), ns_type(PLAIN) {}
        Vector<std::shared_ptr<Common>> childeren;
        const NamespaceType ns_type;

      protected:
        Namespace(uint32_t o, std::string n, NamespaceType t)
            : Common(NAMESPACE, o, n), ns_type(t) {}
    };

    struct TrapNamespace : public Namespace {
        TrapNamespace(uint32_t o, std::string n) : Namespace(o, n, TRAP) {}

        std::string parent_name;
        std::string desc;
    };

    struct Table : public Namespace {
        Table(uint32_t o, const std::string &n, const std::string &td,
              const std::string &ti)
            : Namespace(o, n, TABLE), table_desc(td), index_desc(ti) {}

        std::string table_desc, index_desc;
    };

    struct EnumType {
        std::string name;
        std::string desc;
        const vtss_enum_descriptor_t *data;
    };

    std::string name_prefix_;
    std::string definition_name_;
    Vector<HistoryElement> history;
    NamespaceList stack;
    Vector<const char *> dependency_stack;
    LeafPtr last_leaf;
    Map<std::string, std::string> types;
    std::string org_, contact_, desc_, parent_, parent_import_;
    Map<std::string, EnumType> enums;
    bool building_table = false;
    ArgumentType::E argument_type = ArgumentType::Val;
    Map<std::string, std::string> dependency_map;

    Reflector::TrapType trap_mode_ = Reflector::TrapType::NO_TRAP;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_MIB_GENERATOR_HXX__
