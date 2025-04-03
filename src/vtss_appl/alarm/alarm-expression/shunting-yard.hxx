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

#ifndef __VTSS_APPL_ALARM_EXPRESSION_SHUNTING_YARD_HXX__
#define __VTSS_APPL_ALARM_EXPRESSION_SHUNTING_YARD_HXX__

#include "alarm-expression/tree-element.hxx"
#include "alarm-expression/tree-element-var.hxx"

namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

////////////////////////////////////////////////////////////////////////////////
// The ShuntinYard class is used to pass a Vector of Tokens (TreeElement), and
// as result generate a tree of ExprTree elements, if passing is successfull.
// else RC error reason can be seen in scc_.
// Note the ShuntingYard method use two stacks, one to stack token operators and
// one to stack elements (eg. tokens that are not opeartors such as e.g. a
// number).
// The ShuntingYard is started by continous calling the push() with each element
// of the parsed tokens (stored in a Vector outside the ShuntingYeard instance
// in the Expression instance. Then at the end finalize() is called  which end
// the ShuntingYard method and provids the root to the three as output.
struct ShuntingYard {
    // used as element stack.
    Vector<TreeElement *> element_stack;

    // used as operator stack.
    Vector<TreeElementOpr *> operator_stack;

    //  int rc_, contains result rc code of the Shunting yard.
    int rc_ = VTSS_RC_OK;  // starting as ok

    // int cnt_start_p, is used to keep track of outstanding start_paranthes
    //     counts up when push of start_p,  counts down when push of end_p
    int cnt_start_p = 0;

    // last pushed TreeElement via push method.
    //  Note this is needed since possible to stack more than one
    //  '!' back to back,  which is a special case since priority of
    //  these will be equal, but shall not 3-empty to three as e.g.
    //  '+' back to back.
    TreeElementOpr *last_push_e = nullptr;

    // Used during the ShuntingYard method to pop from operator stack.  a
    // pop_opr() can generate a series of pops internally generating
    // 3-element-subtrees that then are pushed back on operator stack.
    TreeElementOpr *pop_opr();

    // Used during the ShuntingYard method to push each TreeElement from the
    // Vector of these elements from the Expression instance. The push() will
    // figure out when to push on the operator stack and when to push on the
    // element stack.  A push() can start a series of pop_opr()
    bool push(TreeElement *e_);

    // Used during the ShuntingYard method to finish and return the root of the
    // tree (if success) else nullptr, where rc_ can be used to see the reason.
    // So finish() is called by Expression instance after Expression instance
    // have pushed all tree elementes via push().
    TreeElement *finish();

    // Used to evaluate an operator priority. If return ture the pop_opr stuff
    // will be ongoing generating 3-tree elements se pop_opr function
    bool evaluateOperatorPriority(const TreeElementOpr *element);
};

}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#endif  // __VTSS_APPL_ALARM_EXPRESSION_SHUNTING_YARD_HXX__
