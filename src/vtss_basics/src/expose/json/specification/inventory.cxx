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

#include <string>

#include "vtss/basics/set.hxx"
#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_JSON
#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/expose/json/char-encode.hxx"
#include "vtss/basics/expose/json/specification/indent.hxx"
#include "vtss/basics/expose/json/specification/inventory.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor-encoding.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor-typedef.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor-struct.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor-enum.hxx"

namespace vtss {
namespace expose {
namespace json {
namespace specification {

struct StringEncode {
    StringEncode(const std::string &_s) : s(_s) {}
    const std::string &s;
};

ostream &operator<<(ostream &o, StringEncode s) {
    for (auto c : s.s) {
        char ctrl = eschape_char(c);

        if (is_non_eschape_char(c)) {
            o.push(c);

        } else if (ctrl) {
            o.push('\\');
            o.push(ctrl);
        }
    }
    return o;
}

static void print(ostream &o, Indent &i,
                  const std::shared_ptr<TypeDescriptor> t) {
    o << i.open() << "{\n";
    o << i << "\"type-name\":\"" << StringEncode(t->name) << "\",\n";
    switch (t->type_class) {
    case TypeClass::Typedef:
    case TypeClass::EncodingSpecification:
        o << i << "\"class\":\"TypeSpecification\",\n";
        break;
    default:
        o << i << "\"class\":\"" << t->type_class << "\",\n";
    }
    o << i << "\"description\":\"" << StringEncode(t->description) << "\",\n";

    switch (t->type_class) {
    case TypeClass::EncodingSpecification: {
        auto s = static_cast<TypeDescriptorEncoding *>(t.get());
        o << i << "\"json-encoding-type\":\"" << s->encoding << "\"\n";
        break;
    }

    case TypeClass::Typedef: {
        auto s = static_cast<TypeDescriptorTypedef *>(t.get());
        o << i << "\"encoding-type\":\"" << s->encoding << "\"\n";
        // o << i << "\"internal-type\":\"" << StringEncode(s->typedef_type)
        //  << "\"\n";
        break;
    }

    case TypeClass::Struct: {
        auto s = static_cast<TypeDescriptorStruct *>(t.get());
        o << i << "\"encoding-type\":\"" << JsonCoreType::Object << "\",\n";
        o << i.open() << "\"elements\":[\n";
        bool first = true;
        for (const auto &e : s->elements) {
            if (!first) o << ",\n";
            first = false;
            o << i.open() << "{\n";
            o << i << "\"name\":\"" << StringEncode(e.name);
            o << "\",\n" << i << "\"type\":\"" << StringEncode(e.type);
            o << "\",\n" << i << "\"description\":\"" << StringEncode(e.desc);
            if (e.depends.size())
                o << "\",\n" << i << "\"depends-on-capability\":\""
                  << StringEncode(e.depends);
            o << "\"\n";
            o << i.close() << "}";
        }
        if (!first) o << "\n";
        o << i.close() << "]\n";
        break;
    }

    case TypeClass::Enum: {
        auto s = static_cast<TypeDescriptorEnum *>(t.get());
        o << i << "\"encoding-type\":\"" << JsonCoreType::String << "\",\n";
        o << i.open() << "\"elements\":[\n";
        bool first = true;
        for (const auto &e : s->elements) {
            if (!first) o << ",\n";
            first = false;
            o << i << "{\"name\":\"" << StringEncode(e.name)
              << "\", \"value\":\"" << e.value << "\"}";
        }
        if (!first) o << "\n";
        o << i.close() << "]\n";
        break;
    }
    }

    o << i.close() << "}";
}

static void print(ostream &o, Indent &indent, const ParamDescriptor &i) {
    o << indent.open() << "{\n";
    o << indent << "\"name\":\"" << StringEncode(i.name) << "\",\n";
    if (i.description.size())
        o << indent << "\"description\":\"" << StringEncode(i.description)
          << "\",\n";
    o << indent << "\"type\":\"" << StringEncode(i.semantic_name_type)
      << "\"\n";
    o << indent.close() << "}";
}

static void print_get_all(ostream &o, Indent &indent,
                          const MethodDescriptor &i) {
    o << indent.open() << "{\n";
    o << indent << "\"method-name\":"
      << "\"" << StringEncode(i.method_name) << "\",\n";

    o << indent << "\"description\":"
      << "\"This is an overload of " << StringEncode(i.method_name)
      << " without any input parameters. It is used to implement a get-all "
      << "functionallity.\",\n";

    if (i.group_name.size())
        o << indent << "\"group-name\":\"" << StringEncode(i.group_name)
          << "\",\n";

    if (i.description)
        o << indent << "\"description\":\"" << str(i.description) << "\",\n";

    if (i.has_notification && i.method_name.size() >= 4) {
        // assiming that it ends with ".get"
        std::string s(i.method_name.begin(), i.method_name.end() - 4);
        s.append(".update");
        o << indent << "\"notification\":\"" << s.c_str() << "\",\n";
    }

    if (i.depends_on.size())
        o << indent << "\"depends-on-capability\":\"" << i.depends_on.c_str()
          << "\",\n";

    if (i.priv_module.size() && i.priv_type.size())
        o << indent << "\"web-privilege\":{\"id\":\"" << i.priv_module
          << "\", \"type\":\"" << i.priv_type << "\"},\n";

    // get-all has not params
    o << indent << "\"params\":[],\n";
    o << indent.open() << "\"result\":[\n";

    o << indent.open() << "{\n";
    o << indent << "\"name\":\"res\",\n";
    o << indent.open() << "\"type\":{\n";
    o << indent << "\"class\": \"Array\",\n";
    o << indent << "\"encoding-type\": \"Array\",\n";
    o << indent.open() << "\"type\":{\n";

    o << indent << "\"class\": \"Struct\",\n";
    o << indent << "\"encoding-type\": \"Object\",\n";
    o << indent.open() << "\"elements\":[\n";

    // key part
    o << indent.open() << "{\n";
    o << indent << "\"name\":\"key\",\n";
    if (i.params.size() == 1) {
        o << indent << "\"semantic-name\":\""
          << StringEncode(i.params.begin()->name) << "\",\n";
        o << indent << "\"type\":\""
          << StringEncode(i.params.begin()->semantic_name_type) << "\"\n";
    } else {
        if (i.params.size() == 0) {
            o << indent << "\"type\":null\n";
        } else {
            o << indent.open() << "\"type\":{\n";
            o << indent << "\"class\": \"Tuple\",\n";
            o << indent << "\"encoding-type\": \"Array\",\n";
            o << indent.open() << "\"elements\":[\n";
            bool first = true;
            for (const auto &e : i.params) {
                if (!first) o << ",\n";
                first = false;

                o << indent.open() << "{\n";
                o << indent << "\"name\":\"" << StringEncode(e.name) << "\",\n";
                if (e.description.size())
                    o << indent << "\"description\":\""
                      << StringEncode(e.description) << "\",\n";
                o << indent << "\"type\":\"" << StringEncode(e.semantic_name_type)
                  << "\"\n";
                o << indent.close() << "}";
            }

            if (!first) o << "\n";
            o << indent.close() << "]\n";
            o << indent.close() << "}\n";
        }
    }
    o << indent.close() << "},\n";

    // val part
    o << indent.open() << "{\n";
    o << indent << "\"name\":\"val\",\n";
    if (i.results.size() == 1) {
        o << indent << "\"semantic-name\":\""
          << StringEncode(i.results.begin()->name) << "\",\n";
        o << indent << "\"type\":\"" << i.results.begin()->semantic_name_type
          << "\"\n";
    } else {
        if (i.results.size() == 0) {
            o << indent << "\"type\":null\n";
        } else {
            o << indent.open() << "\"type\":{\n";
            o << indent << "\"class\": \"Tuple\",\n";
            o << indent << "\"encoding-type\": \"Array\",\n";
            o << indent.open() << "\"elements\":[\n";
            bool first = true;
            for (const auto &e : i.results) {
                if (!first) o << ",\n";
                first = false;

                o << indent.open() << "{\n";
                o << indent << "\"name\":\"" << StringEncode(e.name) << "\",\n";
                if (e.description.size())
                    o << indent << "\"description\":\""
                      << StringEncode(e.description) << "\",\n";
                o << indent << "\"type\":\""
                  << StringEncode(e.semantic_name_type)
                  << "\"\n";
                o << indent.close() << "}";
            }

            if (!first) o << "\n";
            o << indent.close() << "]\n";
            o << indent.close() << "}\n";
        }
    }
    o << indent.close() << "}\n";
    o << indent.close() << "]\n";
    o << indent.close() << "}\n";
    o << indent.close() << "}\n";
    o << indent.close() << "}\n";
    o << indent.close() << "]\n";
    o << indent.close() << "}";
}

static void print(ostream &o, Indent &indent, const MethodDescriptor &i) {
    o << indent.open() << "{\n";
    o << indent << "\"method-name\":"
      << "\"" << StringEncode(i.method_name) << "\",\n";

    if (i.group_name.size())
        o << indent << "\"group-name\":\"" << StringEncode(i.group_name)
          << "\",\n";

    if (i.description)
        o << indent << "\"description\":\"" << str(i.description) << "\",\n";


    if (i.has_notification && i.method_name.size() >= 4 && !i.has_get_all) {
        // assiming that it ends with ".get"
        std::string s(i.method_name.begin(), i.method_name.end() - 4);
        s.append(".update");
        o << indent << "\"notification\":\"" << s.c_str() << "\",\n";
    }

    if (i.depends_on.size())
        o << indent << "\"depends-on-capability\":\"" << i.depends_on.c_str()
          << "\",\n";

    if (i.priv_module.size() && i.priv_type.size())
        o << indent << "\"web-privilege\":{\"id\":\"" << i.priv_module
          << "\", \"type\":\"" << i.priv_type << "\"},\n";

    o << indent.open() << "\"params\":[";
    bool first = true;
    for (const auto &e : i.params) {
        if (first)
            o << "\n";
        else
            o << ",\n";
        first = false;
        print(o, indent, e);
    }
    if (!first) {
        o << "\n";
        o << indent.close() << "],\n";
    } else {
        o << "],\n";
        indent.dec();
    }

    first = true;
    o << indent.open() << "\"result\":[";
    for (const auto &e : i.results) {
        if (first)
            o << "\n";
        else
            o << ",\n";
        first = false;
        print(o, indent, e);
    }
    if (!first) {
        o << "\n";
        o << indent.close() << "]\n";
    } else {
        o << "]\n";
        indent.dec();
    }


    o << indent.close() << "}";

    if (i.has_get_all) {
        o << ",\n";
        print_get_all(o, indent, i);
    }
}

static void print(ostream &o, Indent &indent,
                  const vtss::Pair<std::string, GroupDescriptor> &i) {
    o << indent.open() << "{\n";
    o << indent << "\"group-name\":"
      << "\"" << StringEncode(i.first) << "\"";

    if (i.second.parent.size())
        o << ",\n" << indent << "\"parent-group-name\":"
          << "\"" << StringEncode(i.second.parent) << "\"";

    if (i.second.description)
        o << ",\n" << indent << "\"description\":"
          << "\"" << StringEncode(i.second.description) << "\"";

    o << "\n" << indent.close() << "}";
}

ostream &operator<<(ostream &o, const Inventory &i) {
    Indent indent;

    o << indent.open() << "{\n";
    o << indent.open() << "\"types\":[\n";

    // Topological sorting the type list
    Vector<std::shared_ptr<TypeDescriptor>> backlog;
    Vector<std::shared_ptr<TypeDescriptor>> typelist;
    for (const auto &e : i.types) backlog.push_back(e.second);

    Set<std::string> defined_types;
    size_t backlog_size_old = backlog.size();
    while (backlog.size()) {
        VTSS_BASICS_TRACE(INFO) << backlog.size();

        auto itr = backlog.begin();
        while (itr != backlog.end()) {
            bool consumed = false;

            switch ((*itr)->type_class) {
            case TypeClass::Typedef: {
                auto s = static_cast<TypeDescriptorTypedef *>(itr->get());
                if (defined_types.find(s->typedef_type) !=
                    defined_types.end()) {
                    typelist.push_back(*itr);
                    defined_types.insert((*itr)->name);
                    consumed = true;
                    VTSS_BASICS_TRACE(DEBUG) << "Consumed: " << (*itr)->name;
                } else {
                    VTSS_BASICS_TRACE(NOISE) << "Postpone: " << (*itr)->name;
                }
                break;
            }

            case TypeClass::Struct: {
                auto s = static_cast<TypeDescriptorStruct *>(itr->get());
                bool all_dependencies_met = true;
                for (const auto &e : s->elements) {
                    if (defined_types.find(e.type) == defined_types.end()) {
                        VTSS_BASICS_TRACE(NOISE)
                                << "type not defined yet: " << e.type;
                        all_dependencies_met = false;
                        break;
                    }
                }

                if (all_dependencies_met) {
                    typelist.push_back(*itr);
                    defined_types.insert((*itr)->name);
                    consumed = true;
                    VTSS_BASICS_TRACE(DEBUG) << "Consumed: " << (*itr)->name;
                } else {
                    VTSS_BASICS_TRACE(NOISE) << "Postpone: " << (*itr)->name;
                }

                break;
            }

            default:
                // these types has no dependencies and may therefor be added in
                // random order
                typelist.push_back(*itr);
                defined_types.insert((*itr)->name);
                consumed = true;
                VTSS_BASICS_TRACE(DEBUG) << "Consumed: " << (*itr)->name;
            }

            if (consumed)
                backlog.erase(itr);
            else
                ++itr;
        }

        // check for circular dependencies
        VTSS_BASICS_TRACE(INFO) << backlog.size() << " " << backlog_size_old;
        if (backlog.size() >= backlog_size_old) {
            VTSS_BASICS_TRACE(ERROR) << "circular dependencies detected!";
            for (auto &e : backlog)
                VTSS_BASICS_TRACE(ERROR) << "  Not consumed: " << e->name;

            break;
        }

        backlog_size_old = backlog.size();
    }


    bool first = true;
    for (const auto &e : typelist) {
        if (!first) o << ",\n";
        first = false;
        print(o, indent, e);
    }
    if (!first) o << "\n";

    o << indent.close() << "],\n";

    o << indent.open() << "\"groups\":[\n";
    first = true;
    for (const auto &e : i.groups) {
        if (!first) o << ",\n";
        first = false;
        print(o, indent, e);
    }
    if (!first) o << "\n";
    o << indent.close() << "],\n";


    o << indent.open() << "\"methods\":[\n";
    first = true;
    for (const auto &e : i.methods) {
        if (!first) o << ",\n";
        first = false;
        print(o, indent, e);
    }
    if (!first) o << "\n";

    o << indent.close() << "]\n";
    o << indent.close() << "}\n";
    return o;
}

}  // namespace specification
}  // namespace json
}  // namespace expose
}  // namespace vtss

