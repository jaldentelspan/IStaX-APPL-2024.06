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

#include "vtss/basics/predefs.hxx"

#if defined(VTSS_BASICS_OPTION_MIB_GENERATOR)
#include "vtss/basics/expose/snmp/handlers/mib_generator.hxx"

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP
#define TRACE(X) \
    VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_MODULE_ID, VTSS_BASICS_TRACE_GRP_SNMP, X)

namespace vtss {
namespace expose {
namespace snmp {

namespace {
struct ElementListPrinter {
    ElementListPrinter(ostream &o, const std::string &hdr, uint32_t i,
                       uint32_t max)
        : o_(o), indent_(i), max_(max) {
        o_ << hdr << " { ";
        width = hdr.size() + 3;
    }

    ElementListPrinter(ostream &o, const std::string &hdr, uint32_t max = 70)
        : o_(o), max_(max) {
        o_ << hdr << " { ";
        width = hdr.size() + 3;
        indent_ = width;
    }

    void indent() {
        for (uint32_t i = 0; i < indent_; ++i) o_.push(' ');
    }

    void new_line() {}

    void new_element(uint32_t s) {
        if (s + 3 + width > max_) {
            if (!first) {
                o_ << ",\n";
            }
            indent();
            width = indent_ + s;
        } else {
            if (!first) {
                o_ << ", ";
                width += 2;
            }
            width += s;
        }
    }

    void add(const std::string &e) {
        new_element(e.size());
        first = false;
        o_ << e;
    }

    void close() {
        if (open) {
            o_ << " }\n";
            open = false;
        }
    }

    ~ElementListPrinter() { close(); }

    ostream &o_;
    uint32_t width;
    bool open = true;
    bool first = true;
    uint32_t indent_;
    const uint32_t max_;
};

struct Capitalize {
    Capitalize(const std::string &str) : s(str) {}
    const std::string &s;
};

vtss::ostream &operator<<(vtss::ostream &o, const Capitalize &s) {
    if (!s.s.size()) {
        return o;
    }

    char first = *s.s.begin();
    if (first >= 'a' && first <= 'z') {
        o.push((first - 'a') + 'A');
    } else {
        o.push(first);
    }

    o.write(&*(s.s.begin() + 1), s.s.c_str() + s.s.size());
    return o;
}

struct Uppercase {
    Uppercase(const std::string &str) : s(str) {}
    const std::string &s;
};

vtss::ostream &operator<<(vtss::ostream &o, const Uppercase &s) {
    for (auto c : s.s) {
        if (c >= 'a' && c <= 'z') {
            o.push((c - 'a') + 'A');
        } else {
            o.push(c);
        }
    }

    return o;
}

struct FormattedText {
    FormattedText(const std::string &str) : s(str) {}
    const std::string &s;
};

ostream &pretty_print(ostream &o, const char *txt,
                      const char *indent = "         ", int32_t max_width = 70) {
    const char *index = txt;
    const char *search = txt;
    int32_t width = 0;
    while (*index != 0) {
        while (*search <= ' ' && *search > 0) {
            if (*search == '\n') {
                o << "\n" << indent << "\n" << indent;
                width = 0;
            }
            search++;
        }
        index = search;
        while (*search > ' ' || *search < 0) search++;
        if (width + search - index > max_width) {
            o << "\n" << indent;
            width = 0;
        } else if (width > 0) {
            o << ' ';
            width++;
        }
        while (index < search) {
            if (*index > 0) {
                o << *index++;
                width++;
            } else {
                index++;
            }
        }
    }
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const FormattedText &s) {
    return pretty_print(o, s.s.c_str());
}
}

// Implement MibGenerator //////////////////////////////////////////////////////
MibGenerator::MibGenerator(ostream &o) : o_(o) {
    TRACE(DEBUG) << "Created new mib-generator: " << this;
    name_prefix_ = TO_STR(MIB_ENTERPRISE_NAME);
}

void MibGenerator::parent(const char *name, const char *import) {
    parent_ = name;
    parent_import_ = import;
}

void MibGenerator::contact(const char *org, const char *name) {
    org_ = org;
    contact_ = name;
}

void MibGenerator::description(const char *desc) { desc_ = desc; }

void MibGenerator::name_prefix(const char *n) { name_prefix_ = n; }

void MibGenerator::definition_name(const char *n) { definition_name_ = n; }

void MibGenerator::go(NamespaceNode &ns) {
    TRACE(DEBUG) << "Start walking the MIB tree";
    node_begin(ns.element(), nullptr);

    for (auto i = ns.leafs.begin(); i != ns.leafs.end(); ++i)
        serialize(*static_cast<Reflector *>(this), *i);

    o_ << "-- "
          "*****************************************************************\n";
    o_ << "-- " << definition_name_ << ":  " << VTSS_SNMP_HEADLINE_DESC << "\n";
    o_ << "-- "
          "****************************************************************\n";
    o_ << "\n";
    if (name_prefix_.size()) o_ << Uppercase(name_prefix_) << "-";
    o_ << definition_name_ << " DEFINITIONS ::= BEGIN\n\n";
    build_imports();
    build_mib();
    build_conformance();
    o_ << "END\n";
    TRACE(DEBUG) << "End walking the MIB tree. ostream status: " << o_.ok();
    node_end(nullptr);
}

void MibGenerator::add_history_element(const char *data, const char *desc) {
    history.emplace_back(HistoryElement{data, desc});
}

void MibGenerator::type_def(const char *name, AsnType::E type) {
    assert(last_leaf.get());

    StringStream n;
    n << Uppercase(name_prefix_) << name;

    StringStream f;
    f << Uppercase(name_prefix_) << "-TC";

    last_leaf->type = n.cstring();
    types.set(n.cstring(), f.cstring());
}

void MibGenerator::type_range(std::string s) {
    assert(last_leaf.get());
    last_leaf->range = s;
}

void MibGenerator::type_ref(const char *name, const char *from, AsnType::E type) {
    assert(last_leaf.get());
    last_leaf->type = name;
    if (from) types.set(name, from);
}

void MibGenerator::type_enum(const char *name, const char *desc,
                             const vtss_enum_descriptor_t *d) {
    assert(last_leaf.get());
    StringStream ss;
    ss << Uppercase(name_prefix_) << name;
    last_leaf->type = ss.cstring();
    enums.set(name, EnumType{ss.cstring(), desc, d});
}

void MibGenerator::argument_begin(size_t n, ArgumentType::E type) {
    TRACE(DEBUG) << "Argument: " << n << " " << type;
    argument_type = type;
}

void MibGenerator::node_begin(const OidElement &n, MaxAccess::E access,
                              const char *depends_on) {
    max_access = access;
    node_begin(n, depends_on);
}

void MibGenerator::node_begin(const OidElement &n, const char *depends_on) {
    if (trap_mode_ != Reflector::TrapType::NO_TRAP) {
        VTSS_ASSERT(stack.size() > 0);
        VTSS_ASSERT(stack.back()->ns_type == TRAP);

        auto p = std::static_pointer_cast<TrapNamespace>(stack.back());
        p->parent_name = n.name();

        return;
    }

    TRACE(DEBUG) << "node begin: " << n;

    if (depends_on) dependency_stack.push_back(depends_on);

    auto node = std::make_shared<Namespace>(n.numeric(), n.name());

    if (stack.size()) {
        TRACE(DEBUG) << "    parent " << stack.back()->name;
        node->parent = stack.back();
        stack.back()->childeren.push_back(node);
    }

    stack.push_back(node);
}

void MibGenerator::trap_begin(const OidElement &n, TrapType t) {
    TRACE(DEBUG) << "Trap begin: " << n;

    auto node = std::make_shared<TrapNamespace>(n.numeric(), n.name());

    switch (t) {
    case Reflector::TrapType::NO_TRAP:
        break;
    case Reflector::TrapType::SCALAR:
        node->desc =
                "This trap signals that one or more of the objects included in "
                "the trap\n         has been updated.";
        break;

    case Reflector::TrapType::ADD:
        node->desc =
                "This trap signals that a row has been added. The index(es) "
                "and value(s)\n         of the row is included in the trap.";
        break;

    case Reflector::TrapType::MODIFY:
        node->desc =
                "This trap signals that one or more of the objects included in "
                "the trap\n          has been updated.";
        break;

    case Reflector::TrapType::DELETE:
        node->desc =
                "This trap signals that a row has been deleted. The index(es) "
                "of the\n         row is included in the trap.";
        break;
    }

    if (stack.size()) {
        TRACE(DEBUG) << "    parent " << stack.back()->name;
        node->parent = stack.back();
        stack.back()->childeren.push_back(node);
    }

    stack.push_back(node);

    trap_mode_ = t;
}

void MibGenerator::trap_end() {
    TRACE(DEBUG) << "Trap end";

    stack.pop_back();
    trap_mode_ = Reflector::TrapType::NO_TRAP;
}

void MibGenerator::node_end(const char *depends_on) {
    if (trap_mode_ != Reflector::TrapType::NO_TRAP) return;

    stack.pop_back();
    if (depends_on) dependency_stack.pop_back();
}

void MibGenerator::table_node_begin(const OidElement &n, MaxAccess::E a,
                                    const char *table_desc,
                                    const char *index_desc,
                                    const char *depends_on) {
    if (trap_mode_ != Reflector::TrapType::NO_TRAP) return;
    max_access = a;
    building_table = true;
    TRACE(DEBUG) << "table begin: " << n;

    if (depends_on) dependency_stack.push_back(depends_on);

    auto node = std::make_shared<Table>(n.numeric(), n.name(), table_desc,
                                        index_desc);

    if (stack.size()) {
        TRACE(DEBUG) << "    parent " << stack.back()->name;
        node->parent = stack.back();
        stack.back()->childeren.push_back(node);
    }

    stack.push_back(node);
}

void MibGenerator::table_node_end(const char *depends_on) {
    if (trap_mode_ != Reflector::TrapType::NO_TRAP) return;
    building_table = false;
    stack.pop_back();
    if (depends_on) dependency_stack.pop_back();
}

std::string without_table(const std::string &s) {
    if (s.size() < 5) return s;

    std::string end(s.end() - 5, s.end());
    if (end == std::string("Table")) {
        return std::string(s.begin(), s.end() - 5);
    } else {
        return s;
    }
}

void MibGenerator::add_capability(const char *name) {
    if (trap_mode_ != Reflector::TrapType::NO_TRAP) return;
    StringStream ss;
    ss << name_prefix_;
    if (stack.size()) ss << Capitalize(without_table(stack.back()->name));
    ss << Capitalize(name);

    TRACE(DEBUG) << "Dependency: " << name << " -> " << ss.buf;
    dependency_map.set(name, ss.buf);
}

void MibGenerator::add_leaf_(const char *name, const char *desc, int oid,
                             const char *range, const char *depends_on, const char *status) {
    const char *depends_on_ = depends_on;
    TRACE(DEBUG) << "leaf: " << name << ":" << oid;

    if (!depends_on_) {
        if (!dependency_stack.empty()) depends_on_ = dependency_stack.back();
    }

    if (depends_on_) TRACE(DEBUG) << "    depends-on: " << depends_on_;

    MaxAccess::E access = max_access;
    if (building_table && argument_type == ArgumentType::Key)
        access = MaxAccess::AccessibleForNotify;

    auto leaf =
            std::make_shared<Leaf>(oid, name, desc, status, access, range, depends_on_);

    if (building_table) leaf->table_leaf = true;

    if (stack.size()) {
        TRACE(DEBUG) << "    parent " << stack.back()->name;
        leaf->parent = stack.back();
        stack.back()->childeren.push_back(leaf);
    }

    last_leaf = leaf;
}

void MibGenerator::build_mib_table_name(ostream &o, TablePtr c) {
    o << name_prefix_ << Capitalize(without_table(c->name)) << "Table";
}
void MibGenerator::build_mib_table_name(ostream &o, LeafPtr c) {
    auto p = c->parent.lock();
    o << name_prefix_ << Capitalize(p->name)
      << Capitalize(without_table(c->name)) << "Entry";
}
void MibGenerator::build_mib_table_entry_name(ostream &o, TablePtr c) {
    o << name_prefix_ << Capitalize(without_table(c->name)) << "Entry";
}
void MibGenerator::build_mib_table_entry_type(ostream &o, TablePtr c) {
    o << Uppercase(name_prefix_) << Capitalize(without_table(c->name))
      << "Entry";
}

void MibGenerator::build_mib_object_name(ostream &o, LeafPtr c) {
    auto p = c->parent.lock();
    o << name_prefix_ << Capitalize(without_table(p->name))
      << Capitalize(c->name);
}
void MibGenerator::build_mib_object_name(LeafPtr c) {
    build_mib_object_name(o_, c);
}

void MibGenerator::build_mib_object_name(ostream &o, NamespacePtr c) {
    o << name_prefix_ << Capitalize(without_table(c->name));
}

void MibGenerator::build_mib_object_name(NamespacePtr c) {
    build_mib_object_name(o_, c);
}

void MibGenerator::build_mib_object_name_raw(ostream &o, LeafPtr c) {
    auto p = c->parent.lock();
    o << name_prefix_ << Capitalize(p->name) << Capitalize(c->name);
}
void MibGenerator::build_mib_object_name_raw(LeafPtr c) {
    build_mib_object_name_raw(o_, c);
}

void MibGenerator::build_mib_object_name_raw(ostream &o, NamespacePtr c) {
    o << name_prefix_ << Capitalize(c->name);
}

void MibGenerator::build_mib_object_name_raw(NamespacePtr c) {
    build_mib_object_name_raw(o_, c);
}

void MibGenerator::build_mib_parent_object_name(LeafPtr c) {
    auto p = c->parent.lock();
    o_ << name_prefix_ << Capitalize(without_table(p->name));
}

void MibGenerator::build_mib_parent_object_name(NamespacePtr c) {
    auto p = c->parent.lock();
    o_ << name_prefix_ << Capitalize(without_table(p->name));
}

void MibGenerator::build_mib() {
    TRACE(DEBUG) << "Building the MIB";
    if (stack.size() != 1) {
        TRACE(ERROR) << "stack.size() != 1  Size is: " << stack.size();
        return;
    }

    build_mib_object_name(stack.back());
    o_ << " MODULE-IDENTITY\n";
    if (history.size()) {
        o_ << "    LAST-UPDATED \"" << history.back().date << "\"\n";
    }

    o_ << "    ORGANIZATION\n";
    o_ << "        \"" << org_ << "\"\n";
    o_ << "    CONTACT-INFO\n";
    o_ << "        \"" << contact_ << "\"\n";
    o_ << "    DESCRIPTION\n";
    o_ << "        \"" << FormattedText(desc_) << "\"\n";

    for (auto i = history.rbegin(); i != history.rend(); ++i) {
        o_ << "    REVISION    \"" << i->date << "\"\n";
        o_ << "    DESCRIPTION\n";
        o_ << "        \"" << FormattedText(i->desc) << "\"\n";
    }

    o_ << "    ::= { " << parent_ << " " << stack.back()->oid << " }\n\n\n";

    build_types();
    build_mib_each_child(stack.back());
}

void MibGenerator::build_mib_ns(const NamespacePtr ns) {
    build_mib_object_name(ns);
    o_ << " OBJECT IDENTIFIER\n";
    o_ << "    ::= { ";
    build_mib_parent_object_name(ns);
    o_ << " " << ns->oid << " }\n\n";

    build_mib_each_child(ns);
}

void MibGenerator::build_mib_trap(const TrapNamespacePtr ns) {
    build_mib_object_name(ns);
    o_ << " NOTIFICATION-TYPE\n";

    ElementListPrinter ep(o_, "    OBJECTS    ");
    for (std::shared_ptr<Common> c : ns->childeren) {
        if (c->type != LEAF) continue;

        auto l = std::static_pointer_cast<Leaf>(c);
        if (l->access == MaxAccess::NotAccessible) continue;

        StringStream ss;

        auto p = c->parent.lock();
        ss << name_prefix_ << Capitalize(without_table(ns->parent_name))
           << Capitalize(c->name);

        ep.add(ss.cstring());
    }
    ep.close();

    // TODO, this may need to be configurable
    o_ << "    STATUS      current\n";
    o_ << "    DESCRIPTION\n";
    o_ << "        \"" << ns->desc << "\"\n";
    o_ << "\n";

    o_ << "    ::= { ";
    build_mib_parent_object_name(ns);
    o_ << " " << ns->oid << " }\n\n";
}

void MibGenerator::build_mib_table(const TablePtr ns) {
    build_mib_table_name(o_, ns);
    o_ << " OBJECT-TYPE\n";
    o_ << "    SYNTAX      SEQUENCE OF ";
    build_mib_table_entry_type(o_, ns);
    o_ << "\n    MAX-ACCESS  not-accessible\n";
    o_ << "    STATUS      current\n";
    o_ << "    DESCRIPTION\n";
    o_ << "        \"" << FormattedText(ns->table_desc) << "\"\n";
    o_ << "    ::= { ";
    build_mib_parent_object_name(ns);
    o_ << " " << ns->oid << " }\n\n";

    build_mib_table_entry_name(o_, ns);
    o_ << " OBJECT-TYPE\n";
    o_ << "    SYNTAX      ";
    build_mib_table_entry_type(o_, ns);
    o_ << "\n    MAX-ACCESS  not-accessible\n";
    o_ << "    STATUS      current\n";
    o_ << "    DESCRIPTION\n";
    o_ << "        \"" << FormattedText(ns->index_desc) << "\"\n";

    ElementListPrinter ep(o_, "    INDEX      ");
    for (auto c : ns->childeren) {
        if (c->type != LEAF) continue;

        auto l = std::static_pointer_cast<Leaf>(c);
        if (l->access != MaxAccess::NotAccessible &&
            l->access != MaxAccess::AccessibleForNotify)
            continue;

        StringStream ss;
        build_mib_object_name(ss, l);
        ep.add(ss.cstring());
    }
    ep.close();
    o_ << "    ::= { ";
    build_mib_table_name(o_, ns);
    o_ << " 1 }\n\n";

    build_mib_table_entry_type(o_, ns);
    o_ << " ::= SEQUENCE {\n";

    size_t max_width = 0;
    for (auto c : ns->childeren) {
        if (c->type != LEAF) continue;
        auto l = std::static_pointer_cast<Leaf>(c);

        StringStream ss;
        build_mib_object_name(ss, l);
        size_t w = (ss.end() - ss.begin());
        if (w > max_width) max_width = w;
    }
    bool first = true;
    for (auto c : ns->childeren) {
        if (c->type != LEAF) continue;

        auto l = std::static_pointer_cast<Leaf>(c);
        if (first)
            first = false;
        else
            o_ << ",\n";

        StringStream ss;
        build_mib_object_name(ss, l);
        size_t w = (ss.end() - ss.begin());
        w = (max_width - w) + 2;
        o_ << "    " << ss;
        for (size_t i = 0; i < w; ++i) o_.push(' ');
        o_ << l->type;
    }

    o_ << "\n}\n\n";

    build_mib_each_child(ns);
}

void MibGenerator::build_mib_leaf(const LeafPtr l) {
    build_mib_object_name(l);
    o_ << " OBJECT-TYPE\n";
    o_ << "    SYNTAX      " << l->type;
    if (l->range.size()) o_ << l->range;
    o_ << "\n    MAX-ACCESS  " << l->access << "\n";
    o_ << "    STATUS      " << l->status <<"\n";
    o_ << "    DESCRIPTION\n";
    StringStream ss;
    ss << l->desc;
    if (l->depends_on) {
        auto itr = dependency_map.find(l->depends_on);
        if (itr != dependency_map.end()) {
            ss << "\nThis object is only available if the capability object '"
               << itr->second << "' is True.";
        } else {
            TRACE(WARNING) << "Dependency could not be found: " << l->depends_on;
        }
    }
    o_ << "        \"" << FormattedText(ss.buf) << "\"\n";
    o_ << "    ::= { ";
    build_mib_parent_object_name(l);
    if (l->table_leaf) o_ << "Entry";
    o_ << " " << l->oid << " }\n\n";
}

void MibGenerator::build_mib_each_child(const NamespacePtr ns) {
    for (std::shared_ptr<Common> c : ns->childeren) {
        if (c->type == LEAF) {
            build_mib_leaf(std::static_pointer_cast<Leaf>(c));
        } else if (c->type == NAMESPACE) {
            auto ns = std::static_pointer_cast<Namespace>(c);
            if (ns->ns_type == TABLE) {
                auto table = std::static_pointer_cast<Table>(c);
                build_mib_table(table);
            } else if (ns->ns_type == PLAIN) {
                build_mib_ns(ns);
            } else if (ns->ns_type == TRAP) {
                auto t = std::static_pointer_cast<TrapNamespace>(c);
                build_mib_trap(t);
            } else {
                TRACE(ERROR) << "Unexpected type";
            }
        } else {
            TRACE(ERROR) << "Unexpected type";
        }
    }
}

void MibGenerator::build_conformance() {
    TRACE(DEBUG) << "Building the Conformance groups";
    if (stack.size() != 1) {
        TRACE(ERROR) << "stack.size() != 0  Size is: " << stack.size();
        return;
    }

    build_mib_object_name(stack.back());
    o_ << "Conformance OBJECT IDENTIFIER\n";
    o_ << "    ::= { ";
    build_mib_object_name(stack.back());
    o_ << " " << conformance_oid_ << " }\n\n";

    build_mib_object_name(stack.back());
    o_ << "Compliances OBJECT IDENTIFIER\n";
    o_ << "    ::= { ";
    build_mib_object_name(stack.back());
    o_ << "Conformance 1 }\n\n";

    build_mib_object_name(stack.back());
    o_ << "Groups OBJECT IDENTIFIER\n";
    o_ << "    ::= { ";
    build_mib_object_name(stack.back());
    o_ << "Conformance 2 }\n\n";

    uint32_t oid = 0;
    NamespaceList groups;
    build_conformance_each_child(stack.back(), stack.back(), groups, oid);
    build_compliance(stack.back(), groups);
}

void MibGenerator::build_conformance_new_group(const NamespacePtr root,
                                               const NamespacePtr ns,
                                               const LeafList &g, uint32_t &oid_) {
    build_mib_object_name_raw(ns);
    o_ << "InfoGroup OBJECT-GROUP\n";

    ElementListPrinter ep(o_, "    OBJECTS    ");
    for (auto l : g) {
        if (l->access == MaxAccess::NotAccessible) continue;
        if (l->status == "obsolete") continue;
        StringStream ss;
        build_mib_object_name(ss, l);
        ep.add(ss.cstring());
    }
    ep.close();
    o_ << "    STATUS      current\n";
    o_ << "    DESCRIPTION\n";
    o_ << "        \"A collection of objects.\"\n";
    o_ << "    ::= { ";
    build_mib_object_name(stack.back());
    o_ << "Groups " << ++oid_ << " }\n\n";
}

void MibGenerator::build_conformance_trap(const NamespacePtr root,
                                          const NamespacePtr ns,
                                          const NamespaceList &groups,
                                          uint32_t &oid_) {
    build_mib_object_name_raw(ns);
    o_ << "InfoGroup NOTIFICATION-GROUP\n";
    o_ << "    NOTIFICATIONS { ";
    build_mib_object_name_raw(ns);
    o_ << " }\n";
    o_ << "    STATUS      current\n";
    o_ << "    DESCRIPTION\n";
    o_ << "        \"Information group containing a trap.\"\n";
    o_ << "    ::= { ";
    build_mib_object_name(stack.back());
    o_ << "Groups " << ++oid_ << " }\n\n";
}

void MibGenerator::build_conformance_each_child(const NamespacePtr root,
                                                const NamespacePtr ns,
                                                NamespaceList &groups,
                                                uint32_t &oid) {
    LeafList leafs;

    for (std::shared_ptr<Common> c : ns->childeren) {
        if (c->type == LEAF) {
            leafs.push_back(std::static_pointer_cast<Leaf>(c));
        } else if (c->type == NAMESPACE) {
            auto n = std::static_pointer_cast<Namespace>(c);
            if (n->ns_type != TRAP) {
                build_conformance_each_child(root, n, groups, oid);
            } else {
                build_conformance_trap(root, n, groups, oid);
                groups.push_back(n);
            }
        } else {
            TRACE(ERROR) << "Unexpected type";
        }
    }

    if (leafs.size() != 0) {
        build_conformance_new_group(root, ns, leafs, oid);
        groups.push_back(ns);
    }
}

void MibGenerator::build_compliance(const NamespacePtr root,
                                    const NamespaceList &list) {
    build_mib_object_name(root);
    o_ << "Compliance MODULE-COMPLIANCE\n";
    o_ << "    STATUS      current\n";
    o_ << "    DESCRIPTION\n";
    o_ << "        \"The compliance statement for the implementation.\"\n";
    o_ << "\n";
    o_ << "    MODULE      -- this module\n";
    o_ << "\n";

    ElementListPrinter ep(o_, "    MANDATORY-GROUPS");
    for (auto l : list) {
        StringStream ss;
        build_mib_object_name_raw(ss, l);
        ss << "InfoGroup";
        ep.add(ss.cstring());
    }
    ep.close();
    o_ << "\n    ::= { ";
    build_mib_object_name(root);
    o_ << "Compliances 1 }\n\n";
}

void MibGenerator::build_imports() {
    o_ << "IMPORTS\n";
    o_ << "    NOTIFICATION-GROUP, MODULE-COMPLIANCE, OBJECT-GROUP ";
    o_ << "FROM SNMPv2-CONF\n";

    o_ << "    NOTIFICATION-TYPE, MODULE-IDENTITY, OBJECT-TYPE ";
    o_ << "FROM SNMPv2-SMI\n";

    o_ << "    TEXTUAL-CONVENTION FROM SNMPv2-TC\n";
    o_ << "    " << parent_ << " FROM " << parent_import_ << "\n";

    Map<std::string, Set<std::string>> imp;

    for (auto e : types) imp.get(e.second)->second.insert(e.first);

    for (auto s : imp)
        for (auto e : s.second)
            o_ << "    " << e << " FROM " << s.first << "\n";

    o_ << "    ;\n\n";
}

void MibGenerator::build_types() {
    for (auto e : enums) {
        o_ << e.second.name << " ::= TEXTUAL-CONVENTION\n";
        o_ << "    STATUS      current\n";
        o_ << "    DESCRIPTION\n";
        o_ << "        \"" << FormattedText(e.second.desc) << "\"\n";

        ElementListPrinter ep(o_, "    SYNTAX      INTEGER");
        auto i = e.second.data;
        while (i->valueName) {
            StringStream ss;
            ss << i->valueName << "(" << i->intValue << ")";
            ep.add(ss.cstring());
            ++i;
        }
        ep.close();
        o_ << "\n";
    }
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // defined(VTSS_BASICS_OPTION_MIB_GENERATOR)
