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

#ifndef __VTSS_APPL_ALARM_EXPRESSION_TREE_ELEMENT_VAR_HXX__
#define __VTSS_APPL_ALARM_EXPRESSION_TREE_ELEMENT_VAR_HXX__

#include "alarm-expression/any.hxx"
#include "alarm-expression/tree-element.hxx"
#include "alarm-expression/json-parse-token.hxx"
#include <vtss/basics/expose/json/specification/inventory.hxx>
#include <vtss/basics/expose/json/specification/type-descriptor-enum.hxx>

namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

struct TreeElementVarParser;

struct TreeElementVar final : public TreeElement {
    friend struct TreeElementVarParser;

    typedef expose::json::specification::Inventory Inventory;
    typedef expose::json::specification::TypeDescriptor TypeDescriptor;

    typedef std::shared_ptr<TypeDescriptor> TypeDescriptorPtr;

    static std::unique_ptr<TreeElementVar> create(str s);
    static std::unique_ptr<TreeElementVar> create(const char *s);

    AnySharedPtr eval(const Map<str, std::string> &bindings) override;
    bool check(const Inventory &i, Set<std::string> &json_vars) override;
    AnyPtr copy_to(str type, const AnyJsonPrimitive &p);

    bool type_lookup(const Inventory &i, str var, const std::string &type);
    bool check_json_spec(const Inventory &i, str json_var_name);

    str input_string;

    TypeDescriptorPtr type_descriptor;

    std::string json_method_name;
    std::string json_variable_name;
    Vector<TreeElementConst> index_elements_method;

    Vector<JsonParseToken> expect;

    bool is_enum = false;

  private:
    TreeElementVar();
};

struct TreeElementVarParser {
    bool operator()(const char *&b, const char *e);

    std::unique_ptr<TreeElementVar> value;
};

}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#endif  // __VTSS_APPL_ALARM_EXPRESSION_TREE_ELEMENT_VAR_HXX__
