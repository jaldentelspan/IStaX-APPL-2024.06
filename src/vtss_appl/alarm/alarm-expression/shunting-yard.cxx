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

#include "alarm_api.hxx"
#include "alarm-expression/shunting-yard.hxx"
#include "alarm-trace.h"

#include <vtss/basics/trace.hxx>

namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

TreeElementOpr *ShuntingYard::pop_opr() {
    if (operator_stack.size() == 0) {
        rc_ = VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_POP_OPR_STACK_ZERO;
        return nullptr;
    }
    auto *o = operator_stack.back();
    operator_stack.pop_back();

    if (o->opr != Token::start_p) {
        TreeElement *b = nullptr;
        if (element_stack.size() == 0) {
            rc_ = VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_POP_ELEM_STACK_ZERO;
            return nullptr;
        }
        TreeElement *a = element_stack.back();
        element_stack.pop_back();

        if (o->opr != Token::neg) {
            if (element_stack.size() == 0) {
                rc_ = VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_POP_ELEM_STACK_ZERO;
                return nullptr;
            }
            b = element_stack.back();
            element_stack.pop_back();
        }
        if (a && b) {
            o->child_lhs = b;
            o->child_rhs = a;
        } else if (a) {
            o->child_lhs = a;
            o->child_rhs = nullptr;
        } else {
            o->child_lhs = nullptr;
            o->child_rhs = nullptr;
        }

        if (element_stack.push_back(o) == false) {
            rc_ = VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_ALLOC_ELEM_STACK_PUSH_BACK;
            return nullptr;
        }
    } else {
        cnt_start_p++;
    }
    DEFAULT(DEBUG) << "pop " << o->opr;
    return o;
}

bool ShuntingYard::push(TreeElement *e_) {
    if (e_->is_operator()) {
        auto *o = static_cast<TreeElementOpr *>(e_);
        DEFAULT(DEBUG) << "push " << o->opr;

        while (evaluateOperatorPriority(o)) {
            auto *result = pop_opr();
            if (!result) return false;
            if (result->opr == Token::start_p) break;
        }

        if (o->opr == Token::end_p) {
            if (cnt_start_p == 0) {
                DEFAULT(DEBUG) << "end_p without matching start_p";
                rc_ = VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_PARANTHES_MISMATCH;
                return false;  // last pushed o just before end_p must not
                               // be operator
            }
            cnt_start_p--;
        }

        if (o->opr != Token::end_p) {
            if (operator_stack.push_back(o) == false) {
                rc_ = VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_ALLOC_OPR_STACK_PUSH_BACK;
                return false;
            }
        } else if (last_push_e != nullptr && last_push_e->is_operator()) {
            DEFAULT(DEBUG) << "last is operator when pop end_p ";
            rc_ = VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_OPR_PARANTHES_MISMATCH;
            return false;  // last pushed o just before end_p must not be
                           // operator
        }

        last_push_e = o;
    } else {
        DEFAULT(DEBUG) << "push leaf";
        if (element_stack.push_back(e_) == false) {
            rc_ = VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_ALLOC_ELEM_STACK_PUSH_BACK;
            return false;
        }

        last_push_e = nullptr;
    }
    return true;
}

TreeElement *ShuntingYard::finish() {
    if (last_push_e != nullptr && last_push_e->opr != Token::end_p) {
        DEFAULT(DEBUG) << "last is operator ";
        rc_ = VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_END_WITH_OPR;
        return nullptr;  // last pushed e must not be operator
    }

    while (operator_stack.size()) pop_opr();

    if (cnt_start_p != 0) {
        DEFAULT(DEBUG) << "number of start and end paratheses does not match: "
                       << cnt_start_p;
        rc_ = VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_PARANTHES_MISMATCH;
        return nullptr;  // must be last
    }
    if (element_stack.size() != 1) {
        DEFAULT(DEBUG) << "elem stack size not 1 ";
        rc_ = VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_END_ELEM_STACK_SIZE_NOT_ONE;
        return nullptr;  // must be last
    } else {
        TreeElement *b = element_stack.back();
        element_stack.pop_back();
        return b;
    }
}

bool ShuntingYard::evaluateOperatorPriority(const TreeElementOpr *element) {
    if (operator_stack.size() == 0) return false;
    if (element->prio() == -1) return false;
    if (element->prio() > operator_stack.back()->prio()) return false;

    return last_push_e == nullptr || last_push_e->opr != Token::neg ||
           element->opr != Token::neg;
}


}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss
