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

#ifndef __ALARM_EXPRESSION_HXX__
#define __ALARM_EXPRESSION_HXX__


#include <string>
#include <vtss/basics/map.hxx>
#include <vtss/basics/vector.hxx>
#include <vtss/basics/stream.hxx>
#include <vtss/basics/expose/json/specification/inventory.hxx>

#include "alarm_api.hxx"

#include "alarm-expression/tree-element.hxx"

namespace vtss {
namespace appl {
namespace alarm {

// This class encapsulates an alarm expression.
//
// Intended use:
//  - The class is constructed with an alarm expression string, and a reference
//    to the json-specification inventory. The alarm string is parsed and
//    validated against the json specification.
//    - If everything is okay, then the alarm expression is stored (in some
//      representation) and the 'ok_' variable is set to true.
//    - The parsing process must extract the list of public variables which the
//      alarm expression is referencing. This list is stored in the json_vars_
//      variable.
//  - Once the class is constructed, the expression can not be updated.
//  - The class can now be used to evaluate if the expression is true or false.
//    This is done by the 'evaluate' method. This method is accepting a map
//    where the 'key' is the names of the public variable, and the value is the
//    json-string representation of the value.
//    The map must include all the keys listed in the 'json_vars_' variable.
struct Expression {
  public:
    friend struct expression::TreeElement;
    friend ostream &operator<<(ostream &o, const Expression &e);

    ~Expression(){};

    // The constructor is accepting an alarm expression 'e', and a reference to
    // the json-specification. The json-specification is needed to check if the
    // alarm expression is valid or not, and to derive the list of public
    // variables.
    Expression(const std::string &e,
               const expose::json::specification::Inventory &i);

    // This is the method to evaluate the expression. The caller of this class
    // must provide all the bindings and their JSON encoded values in the map.
    bool evaluate(const Map<str, std::string> &bindings);

    // Check if the class was constructed without any errors.
    bool ok() const { return ok_; }
    int rc() const { return rc_; }

    // outputs a string of the generated tree, starting from root, used to makes
    // i easy to check expected result in e.g. unit_tests or similar
    std::string parsed_expression() const;

    // Get the list of public variables needed by the alarm expression.
    const Set<std::string> &bindings() const { return json_vars; }

  private:
    // This is the list of public variables referenced by the alarm
    // expression.
    Set<std::string> json_vars;

    // ok_ track on if the class was constructed with a valid expression
    bool ok_ = false;

    //  rc_ is the associatd RC error code (or ok) to ok_
    int rc_ = VTSS_RC_OK;

    // expr_ is the alarm expression provided in the constructor. It may be
    // represented differently (parse tree...), but it must be possible to
    // get a
    // string representation of the expression.
    const std::string expr_;

    //  root_ is used to contian a pointer to the root of the tree generated
    //  during parsing suing the
    //  CheckSyntaxOfExpressionAndBuildExpressionTree()
    expression::TreeElement *root_ = nullptr;

    //    this function is called by constructor and used to syntax parse
    //    Expression and build ExpresssionTree, top will be stored in root_.
    void CheckSyntaxOfExpressionAndBuildExpressionTree(
            const expose::json::specification::Inventory &i);

    // tree_elements is a vector of all the parsed text elements generated
    // during parsing of CheckSynxOfExpresssionAndBuildExpressionTree().
    // There after this is used inside that function to do the three
    // generation
    // using ShuntingYeard class method.The class Token defines the
    // different
    // possible types in the Vector.
    Vector<expression::ExprTreeElemPtr> tree_elements;
};

}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#endif  // __ALARM_EXPRESSION_HXX__
