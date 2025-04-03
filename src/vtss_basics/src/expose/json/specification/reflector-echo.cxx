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

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_JSON
#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/expose/json/function-exporter-abstract.hxx"
#include "vtss/basics/expose/json/specification/reflector-echo.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor-typedef.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor-encoding.hxx"
#include "vtss/basics/expose/json/specification/type-descriptor-enum.hxx"
#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss/appl/module_id.h"
#endif

namespace vtss {
namespace expose {
namespace json {
namespace specification {

ReflectorEcho::ReflectorEcho(Inventory &i, const std::string &m,
                             const std::string &p, Node *n,
                             bool target_specific)
    : inventory(i), node(n), target_specific_(target_specific) {
    if (n->is_notification()) return;
    inventory.methods.emplace_back();
    desc = &inventory.methods.back();
    desc->method_name = m;
    desc->group_name = p;
    desc->description = node->description().begin();

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (n->is_function_exporter_abstract()) {
        auto a = static_cast<FunctionExporterAbstract *>(n);
        switch (a->priv_type) {
        case priv::STATUS_RW:
            desc->priv_type = "STATUS_RW";
            break;
        case priv::STATUS_RO:
            desc->priv_type = "STATUS_RO";
            break;
        case priv::CONF_RW:
            desc->priv_type = "CONFIG_RW";
            break;
        case priv::CONF_RO:
            desc->priv_type = "CONFIG_RO";
            break;
        case priv::NO_ACCESS:
            desc->priv_type = "NO_ACCESS";
            break;
        case priv::NO_ACCESS_CONTROL:
            // do nothing
            break;
        default:
            desc->priv_type = "NO_ACCESS";
            ;
        }

        if (a->priv_module_id >= 0 && a->priv_module_id < VTSS_MODULE_ID_NONE) {
            if (vtss_module_names[a->priv_module_id]) {
                desc->priv_module = vtss_module_names[a->priv_module_id];
            }
        }
    }
#endif

    if (node->depends_on_capability().size()) {
        desc->depends_on = node->depends_on_capability().begin();
        if (node->depends_on_capability_group().size()) {
            desc->depends_on.push_back('@');
            desc->depends_on.append(
                    node->depends_on_capability_group().begin());
        }
    }
}

void ReflectorEcho::input_begin() {
    VTSS_BASICS_TRACE(NOISE) << indent << "Input:";
    input_section = true;
    indent.inc();
}

void ReflectorEcho::input_end() {
    input_section = false;
    indent.dec();
}

void ReflectorEcho::output_begin() {
    output_section = true;
    VTSS_BASICS_TRACE(NOISE) << indent << "Output:";
    indent.inc();
}

void ReflectorEcho::output_end() {
    output_section = false;
    indent.dec();
}

void ReflectorEcho::has_get_all_variant() {
    inventory.methods.back().has_get_all = true;
}

void ReflectorEcho::has_notification() {
    inventory.methods.back().has_notification = true;
}

void ReflectorEcho::argument_begin(unsigned idx) {
    VTSS_BASICS_TRACE(NOISE) << indent << "Argument " << idx << ":";
    indent.inc();

    if (input_section) {
        desc->params.emplace_back();
        p = &desc->params.back();

    } else if (output_section) {
        desc->results.emplace_back();
        p = &desc->results.back();

    } else {
        VTSS_ASSERT(0);
    }

    BufStream<SBuf16> buf;
    buf << "Argument" << idx;
    p->name = buf.cstring();
    type_level = 0;
}

void ReflectorEcho::argument_end() {
    indent.dec();
    p = nullptr;
}

void ReflectorEcho::argument_name(const char *name, const char *desc) {
    VTSS_BASICS_TRACE(NOISE) << indent << "Argument-name " << str(name);
    if (name) p->name = name;
    if (desc) p->description = desc;
}

void ReflectorEcho::variable_begin(const char *name, const char *desc,
                                   const char *depends,
                                   const char *depends_group) {
    VTSS_BASICS_TRACE(NOISE) << indent << "Variable " << name << ":";
    indent.inc();
    pending_name = name;
    pending_desc = desc;
    if (depends) {
        pending_dep = depends;
        if (depends_group) {
            pending_dep.push_back('@');
            pending_dep.append(depends_group);
        }
    } else {
        pending_dep.clear();
    }
}

void ReflectorEcho::variable_end() { indent.dec(); }

void ReflectorEcho::type_begin(const char *name, const char *desc) {
    VTSS_BASICS_TRACE(NOISE) << indent << "Type " << name << ":";
    indent.inc();

    ++type_level;
    if (type_level == 1) {
        p->encoding_type = JsonCoreType::Object;
        p->semantic_name_type = name;
    } else {
        // types which is not at level 1 msut be part of an aggreated type
        VTSS_ASSERT(typestack.size() != 0);
        typestack.back()->add(pending_name, name, pending_desc, pending_dep);
    }

    typedef TypeDescriptorStruct T;
    typestack.push_back(std::make_shared<T>(name, desc));
}

void ReflectorEcho::type_end() {
    indent.dec();
    --type_level;

    VTSS_ASSERT(typestack.size() != 0);
    inventory.types.set(typestack.back()->name, typestack.back());
    typestack.pop_back();
}

void ReflectorEcho::alias_begin() { alias_++; }

void ReflectorEcho::alias_end() { alias_--; }

void ReflectorEcho::type_terminal(JsonCoreType::E type, const char *c_type_name,
                                  const char *c_type_alias,
                                  const char *description) {
    if (std::string(c_type_name) != std::string(c_type_alias)) {
        VTSS_BASICS_TRACE(NOISE) << indent << "Type " << type << "/"
                                 << c_type_name << "/" << c_type_alias;
    } else {
        VTSS_BASICS_TRACE(NOISE) << indent << "Type " << type << "/"
                                 << c_type_name;
    }

    if (alias_ == 0) {
        VTSS_BASICS_TRACE(NOISE) << indent << "Level: " << type_level;
        if (type_level == 0) {
            p->semantic_name_type = c_type_name;
            p->encoding_type = type;
        } else {
            // type-terminal which is not at level 0 msut be part of an
            // aggreated type
            VTSS_ASSERT(typestack.size() != 0);
            typestack.back()->add(pending_name, c_type_name, pending_desc,
                                  pending_dep);
        }
    }

    if (std::string(c_type_name) != std::string(c_type_alias)) {
        typedef TypeDescriptorTypedef T;
        auto t = std::make_shared<T>(c_type_name, c_type_alias, type,
                                     description);
        // TODO, check for incompatible dublicates
        inventory.types.set(c_type_name, t);
    } else {
        typedef TypeDescriptorEncoding T;
        auto t = std::make_shared<T>(c_type_name, type, description);
        // TODO, check for incompatible dublicates
        inventory.types.set(c_type_name, t);
    }
}

void ReflectorEcho::type_terminal_enum(const char *c_type_name,
                                       const char *desc,
                                       const ::vtss_enum_descriptor_t *d) {
    VTSS_BASICS_TRACE(NOISE) << indent << "c_type_name";
    typedef TypeDescriptorEnum T;
    auto t = std::make_shared<T>(c_type_name, desc);
    while (d->valueName && (*d->valueName)) {
        t->add(d->valueName, d->intValue);
        ++d;
    }
    // TODO, check for incompatible dublicates
    inventory.types.set(c_type_name, t);

    if (type_level == 0) {
        p->semantic_name_type = c_type_name;
        p->encoding_type = JsonCoreType::String;
    } else {
        // type-terminal which is not at level 0 msut be part of an
        // aggreated type
        VTSS_ASSERT(typestack.size() != 0);
        typestack.back()->add(pending_name, c_type_name, pending_desc,
                              pending_dep);
    }
}

}  // namespace specification
}  // namespace json
}  // namespace expose
}  // namespace vtss

