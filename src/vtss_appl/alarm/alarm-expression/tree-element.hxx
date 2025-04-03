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

#ifndef __VTSS_APPL_ALARM_EXPRESSION_TREE_ELEMENT_HXX__
#define __VTSS_APPL_ALARM_EXPRESSION_TREE_ELEMENT_HXX__

#include "alarm-expression/any.hxx"
#include "alarm-expression/json-parse-token.hxx"
#include <vtss/basics/expose/json/specification/inventory.hxx>
#include <vtss/basics/expose/json/specification/type-descriptor-enum.hxx>

namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

// Defines a struct used as element in ongoing expr_stack as well as the build
// expr_tree .
struct TreeElement {
  public:
    enum E { OPR, VAR, CONST };

    TreeElement(E type) : type_(type) {}

    E type() const { return type_; }
    int prio() const;
    bool is_operator() const { return type_ == OPR; }

    // virtual void check(const expose::json::specification::Inventory &i);
    virtual AnySharedPtr eval(const Map<str, std::string> &bindings) = 0;
    virtual bool check(const expose::json::specification::Inventory &i,
                       Set<std::string> &json_vars) = 0;

    // Returns the name of the type that will be return by eval
    str name_of_type() const { return name_of_type_; }

    bool get_jsonvalue(const Map<str, std::string> &bindings);

    virtual ~TreeElement() {}

    const E type_;

    // This member is initialized once the "check()" method has completed.
    str name_of_type_;
};

typedef std::unique_ptr<TreeElement> ExprTreeElemPtr;

struct TreeElementOpr final : public TreeElement {
    TreeElementOpr(Token o) : TreeElement(TreeElement::OPR), opr(o) {}

    AnySharedPtr eval(const Map<str, std::string> &bindings) override;
    bool check(const expose::json::specification::Inventory &i,
               Set<std::string> &json_vars) override;

    str symbol() const;

    Token opr;
    TreeElement *child_lhs = nullptr;
    TreeElement *child_rhs = nullptr;
};

struct TreeElementConst final : public TreeElement {
    typedef expose::json::specification::TypeDescriptor TypeDescriptor;
    typedef std::shared_ptr<TypeDescriptor> TypeDescriptorPtr;

    TreeElementConst(int32_t x);
    TreeElementConst(uint32_t x);
    TreeElementConst(int64_t x);
    TreeElementConst(uint64_t x);
    TreeElementConst(const str &x);
    TreeElementConst(const std::string &x);
    TreeElementConst(std::string &&x);
    TreeElementConst(bool x);
    TreeElementConst(nullptr_t);

    AnySharedPtr eval(const Map<str, std::string> &bindings) override;
    bool check(const expose::json::specification::Inventory &i,
               Set<std::string> &json_vars) override;

    bool convert_to(str type);
    bool convert_to(const TypeDescriptorPtr td);
    bool convert_to(const TreeElement *type);
    AnyPtr copy_to(str type);
    AnyPtr copy_to(expose::json::specification::TypeDescriptorEnum *type);

    TypeDescriptorPtr type_descriptor;
    AnySharedPtr value;
};

}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#endif  // __VTSS_APPL_ALARM_EXPRESSION_TREE_ELEMENT_HXX__
