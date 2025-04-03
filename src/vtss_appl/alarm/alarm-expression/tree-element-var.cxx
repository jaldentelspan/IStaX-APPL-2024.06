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


#include "alarm-expression/json-parse.hxx"
#include "alarm-expression/tree-element-var.hxx"

#include <vtss/basics/trace.hxx>
#include <vtss/basics/memory.hxx>
#include <vtss/basics/parser_impl.hxx>
#include <vtss/basics/expose/json/method-split.hxx>
#include <vtss/basics/expose/json/specification/type-descriptor-enum.hxx>
#include <vtss/basics/expose/json/specification/type-descriptor-struct.hxx>
#include "alarm-trace.h"

namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

TreeElementVar::TreeElementVar() : TreeElement(TreeElement::VAR) {}

std::unique_ptr<TreeElementVar> TreeElementVar::create(str s) {
    TreeElementVarParser p;
    const char *b = s.begin(), *e = s.end();
    if (!p(b, e) || b != e) {
        DEFAULT(INFO) << "Failed to parse variable string: '" << s << "' "
                      << str(b, e);
        return nullptr;
    }

    return vtss::move(p.value);
}

std::unique_ptr<TreeElementVar> TreeElementVar::create(const char *s) {
    return create(str(s));
}

AnySharedPtr TreeElementVar::eval(const Map<str, std::string> &bindings) {
    str name = str(json_method_name.c_str());

    DEFAULT(INFO) << "Evaluate beinding: " << name;
    auto m = bindings.find(name);
    if (m == bindings.end()) {
        DEFAULT(ERROR) << "Missing binding for: " << name;
        return std::make_shared<AnyNull>();
    }

    expression::JsonParse t(expect, index_elements_method);
    json::StreamParser s(&t);
    s.process(str(m->second));
    if (t.ok() == false) {
        DEFAULT(INFO) << "Failed to process expression: " << input_string
                      << " against binding: " << m->second;
        return std::make_shared<AnyNull>();
    }

    AnyPtr value = copy_to(name_of_type(), t.get_value());
    if (!value) {
        DEFAULT(ERROR) << "Failed to convert value";
        return std::make_shared<AnyNull>();
    }

    return std::move(value);
}

AnyPtr TreeElementVar::copy_to(str type, const AnyJsonPrimitive &v) {
    using namespace expose::json::specification;

    if (type_descriptor && type_descriptor->type_class == TypeClass::Enum) {
        DEFAULT(INFO) << "Trying to copy construct a (enum)" << type;
        if (v.type() != AnyJsonPrimitive::STR) {
            DEFAULT(WARNING) << "Only strings can be converted to enums";
            return nullptr;
        }

        auto d = std::static_pointer_cast<TypeDescriptorEnum>(type_descriptor);

        for (const auto &e : d->elements) {
            DEFAULT(DEBUG) << e.name << " " << v.as_str();
            if (str(e.name) != v.as_str()) continue;
            DEFAULT(DEBUG) << "Enum " << e.name << " to int: " << e.value;
            return make_unique<AnyInt32>(e.value);
        }

        DEFAULT(WARNING) << "Failed to convert: " << v.as_str()
                         << " to a enum of type " << type;
        return nullptr;

    } else {
        DEFAULT(INFO) << "Trying to copy construct a " << type;
        return ANY_TYPE_INVENTORY.construct(type, v);
    }
}

bool TreeElementVar::type_lookup(const expose::json::specification::Inventory &i,
                                 str var, const std::string &type) {
    using namespace expose::json::specification;

    DEFAULT(DEBUG) << "type_lookup: " << var << "@" << type;

    auto itr = i.types.find(type);
    if (itr == i.types.end()) {
        DEFAULT(WARNING) << "No such type: " << type;
        return false;
    }

    switch (itr->second->type_class) {
    case TypeClass::Enum:
    case TypeClass::Typedef:
    case TypeClass::EncodingSpecification:
        if (var.size() != 0) {
            DEFAULT(WARNING) << "Leaf type, but var part is not empty: " << var;
            return false;
        }

        if (!expect.emplace_back(JsonParseToken::set_mode_capture_value))
            return false;

        type_descriptor = itr->second;
        if (itr->second->type_class == TypeClass::Enum) {
            is_enum = true;
        }

        return true;

    case TypeClass::Struct: {
        str head, tail;
        expose::json::method_split(var, head, tail);

        if (head.size() == 0) {
            DEFAULT(WARNING) << "Struct, but var part is empty!";
            return false;
        }

        auto s = static_cast<TypeDescriptorStruct *>(itr->second.get());
        for (const auto &e : s->elements) {
            if (str(e.name) != head) continue;

            if (!expect.emplace_back(JsonParseToken::object_start))
                return false;
            if (!expect.emplace_back(JsonParseToken::object_element_start, e.name))
                return false;

            bool res = type_lookup(i, tail, e.type);

            return res;
        }

        DEFAULT(WARNING) << "Could not dereference: " << var << "@" << type;
        return false;
    }
    }

    return false;
}

bool TreeElementVar::check_json_spec(const Inventory &i, str json_var_name) {
    std::string _m = json_method_name;
    _m.append(".get");

    DEFAULT(DEBUG) << "MethodName: " << _m;
    for (const auto &m : i.methods) {
        bool res = false;

        if (m.method_name != _m) continue;
        if (!m.has_notification) {
            DEFAULT(WARNING)
                    << "Public variable does not have notification support, "
                       "method_name:"
                    << m.method_name;
            return false;
        }

        if (m.results.size() != 1) {
            DEFAULT(WARNING) << "Only supports single value structures!";
            return false;
        }

        // Currently we require all indexes to match
        if (m.params.size() != index_elements_method.size()) {
            DEFAULT(WARNING) << "Expected " << m.params.size()
                             << " index(es) and got "
                             << index_elements_method.size() << " index(es)";
            return false;
        }

        // Convert the indexes to type provided by the json spec
        for (size_t n = 0; n < index_elements_method.size(); ++n) {
            const std::string &t = m.params[n].semantic_name_type;
            auto it = i.types.find(t);
            if (it == i.types.end()) {
                DEFAULT(WARNING) << "Failed to convert index " << n << " to "
                                 << t << " type was not found in inventory";
                for (auto x : i.types) {
                    DEFAULT(WARNING) << "Found: " << x.first << "->"
                                     << "(" << x.second->type_class << ","
                                     << x.second->name << ")";
                }
                return false;
            }

            DEFAULT(DEBUG) << "Trying to convert index " << n << " to " << t
                           << "(" << it->second->type_class << ")";
            if (!index_elements_method[n].convert_to(it->second)) {
                DEFAULT(WARNING) << "Failed to convert index " << n << " to "
                                 << t << "(" << it->second->type_class << ")";
                return false;
            }
        }

        const auto &r = m.results.begin();
        DEFAULT(DEBUG) << "result type: " << r->semantic_name_type.c_str();

        if (!expect.emplace_back(JsonParseToken::object_start)) return false;
        if (!expect.emplace_back(JsonParseToken::object_element_start,
                                 std::string("result"))) {
            return false;
        }

        if (m.params.size() >= 1) {
            if (!expect.emplace_back(JsonParseToken::array_start)) return false;
            if (!expect.emplace_back(JsonParseToken::set_mode_key_search))
                return false;
        }

        res = type_lookup(i, json_var_name, r->semantic_name_type);

        return res;
    }

    DEFAULT(WARNING) << "Public variable not found in json spec";
    return false;
}

bool TreeElementVar::check(const expose::json::specification::Inventory &i,
                           Set<std::string> &json_vars) {
    if (!check_json_spec(i, json_variable_name)) {
        DEFAULT(WARNING) << "Json specification check failed for: "
                         << json_variable_name;
        return false;
    }

    name_of_type_ = type_descriptor->name;
    if (type_descriptor->type_class ==
        expose::json::specification::TypeClass::Enum) {
        name_of_type_ = AnyInt32::NAME_OF_TYPE;
    }

    DEFAULT(INFO) << "type = " << name_of_type_;
    json_vars.insert(json_method_name);

    return true;
}

////////////////////////////////////////////////////////////////////////////////
namespace {
static bool check_first_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
};

static bool check_json_name_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '.';
}

struct JsonNameChars {
    bool operator()(char c) { return check_json_name_char(c); }
};
}  // namespace

struct IndexParser : public json::StreamParserCallback {
    void reset() {
        ok = true;
        cnt = 0;
        stack_size = 0;
    }

    bool operator()(const char *&b_, const char *e) {
        reset();

        // Be sure that we do not consume anything in case of failure
        const char *b = b_;

        // The following code assume that we have at least one char
        if (b == e) return false;

        // Indexes must start with a '['
        if (*b != '[') return false;

        // Parse the index as a json message
        json::StreamParser json_stream_parser(this);
        b = json_stream_parser.process(b, e);

        // Only one element at a time
        if (cnt != 1) return false;

        // If json parser is not OK, then neither are we.
        if (!json_stream_parser.ok()) return false;

        // In case of nested arrays or objects
        if (!ok) return false;

        // Consume the characters
        b_ = b;

        // Return success
        return true;
    }

    // Implements the StreamParserCallback interface ///////////////////////////
    Action array_start() override {
        // We do not want to accept nested arrays
        if (stack_size == 0) {
            stack_size++;
            return StreamParserCallback::ACCEPT;

        } else {
            ok = false;
            return StreamParserCallback::STOP;
        }
    }

    Action object_start() override {
        // We do not want to support objects in indedexes
        ok = false;
        return StreamParserCallback::STOP;
    }

    void null() override {
        // Null index makes no sense to me.
        ok = false;
    }

    void boolean(bool b) override {
        cnt++;
        DEFAULT(DEBUG) << "Got a bool";
        elements.emplace_back(b);
    }

    void number_value(uint32_t i) override {
        cnt++;
        DEFAULT(DEBUG) << "Got a uint32_t";
        elements.emplace_back(i);
    }

    void number_value(int32_t i) override {
        cnt++;
        DEFAULT(DEBUG) << "Got a int32_t";
        elements.emplace_back(i);
    }

    void number_value(uint64_t i) override {
        cnt++;
        DEFAULT(DEBUG) << "Got a uint64_t";
        elements.emplace_back(i);
    }

    void number_value(int64_t i) override {
        cnt++;
        DEFAULT(DEBUG) << "Got a int64_t";
        elements.emplace_back(i);
    }

    void string_value(const std::string &&s) override {
        cnt++;
        DEFAULT(DEBUG) << "Got a string";
        elements.emplace_back(vtss::move(s));
    }

    bool ok;
    uint32_t cnt;
    uint32_t stack_size;
    Vector<TreeElementConst> elements;
};

struct NameWithIndexParser {
    bool operator()(const char *&b_, const char *e) {
        const char *b = b_;
        const char *b_old = nullptr;
        parser::OneOrMore<JsonNameChars> name_;

        // The following code assume that we have at least one char
        if (b == e) return false;

        // Check that the first char is OK
        if (!check_json_name_char(*b)) return false;

        // Continue as long as we can consume either a name or index
        while (b != e && b_old != b) {
            // Needed to check if we can consumed anything
            b_old = b;

            // See if we can get a name-element
            if (name_(b, e)) name.append(name_.s_.begin(), name_.s_.end());

            // See if we can get a index
            if (index_(b, e)) {
                DEFAULT(DEBUG) << "Got a index";
            }
        }

        // Consume the characters
        b_ = b;

        // Return success
        return true;
    }

    std::string name;
    IndexParser index_;
};

bool TreeElementVarParser::operator()(const char *&b_, const char *e) {
    NameWithIndexParser json_method_name_;
    NameWithIndexParser json_var_name_;
    parser::Lit method_sep("@");

    DEFAULT(DEBUG) << "Parsing: " << str(b_, e);

    // Be sure that we do not consume anything in case of failure
    const char *b = b_;

    // Try to parse the method name. This is mandatory.
    if (!json_method_name_(b, e)) return false;

    // Extra check, the method is not allowed to start with a number.
    if (!check_first_char(*json_method_name_.name.begin())) {
        return false;
    }

    // See if we can parse the optional separator ("@")
    if (!method_sep(b, e)) {
        goto END_OF_VARIABLE;
    }

    // See if we can parse the optional variable part
    if (!json_var_name_(b, e)) {
        goto END_OF_VARIABLE;
    }

END_OF_VARIABLE:
    // All elements has been parsed, start build the object (the optional
    // elements may be empty).

    // Create the object
    value = std::unique_ptr<TreeElementVar>(new TreeElementVar());
    if (!value) return false;

    // Move content out of the parsers and into the object
    value->json_method_name = vtss::move(json_method_name_.name);
    value->json_variable_name = vtss::move(json_var_name_.name);
    value->index_elements_method = vtss::move(json_method_name_.index_.elements);
    value->input_string = str(b_, b);

    // Consume the characters
    b_ = b;

    // Return success
    return true;
}

}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss
