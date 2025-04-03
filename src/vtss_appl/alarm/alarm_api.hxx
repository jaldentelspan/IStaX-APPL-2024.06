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

#ifndef __ALARM_API_H__
#define __ALARM_API_H__

#include <vtss/basics/map.hxx>
#include "vtss/appl/module_id.h"  // TODO can not figure out how to use  <vtss/basics/module_id.hxx>, something with VTSS_USE_API_HEADERS
                                  /**
                                   *  Alarm module error codes (mesa_rc)
                                   */
enum {
    VTSS_RC_ERROR_ALARM_EXPR_SYNTAX_PARSE_UNKNOWN = MODULE_ERROR_START(
            VTSS_MODULE_ID_ALARM), /**< Alarm error, Expression: parse syntax */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_ALLOC_OPR_STACK_PUSH_BACK,
    /**< Alarm error, Expression: ShuntingYard: alloc oprator stack push back */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_ALLOC_ELEM_STACK_PUSH_BACK,
    /**< Alarm error, Expression: ShuntingYard: alloc element stack push back */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_POP_OPR_STACK_ZERO,
    /**< Alarm error, Expression: ShuntingYard: pop oprator stack when zero */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_POP_ELEM_STACK_ZERO,
    /**< Alarm error, Expression: ShuntingYard: pop element stack when zero */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_PARANTHES_MISMATCH,
    /**< Alarm error, Expression: ShuntingYard: paranthes mismatch */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_OPR_PARANTHES_MISMATCH,
    /**< Alarm error, Expression: ShuntingYard: operator followed by paranthes
       */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_END_WITH_OPR,
    /**< Alarm error, Expression: ShuntingYard: expression end with oprator */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_END_ELEM_STACK_SIZE_NOT_ONE,
    /**< Alarm error, Expression: ShuntingYard: element stack size not end with
       1 */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_CHILDS_PTR,
    /**< Alarm error, Expression: ShuntingYard: 3subtree issue with child ptrs
                                                                       */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_NEG_CHILDS_PTR,
    /**< Alarm error, Expression: ShuntingYard: NEG 3subtree issue with child
       ptrs */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_CHILDS_MISMATCH_IN_NUMBINOP,
    /**< Alarm error, Expression: ShuntingYard: 3subtree issue with child when
       NumericBinOp */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_CHILDS_MISMATCH_IN_BOOLBINOP,
    /**< Alarm error, Expression: ShuntingYard: 3subtree issue with child when
       BoolBinOp */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_CHILDS_MISMATCH_IN_COMPARE,
    /**< Alarm error, Expression: ShuntingYard: 3subtree issue with child when
       Compare */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_CHILD_MISMATCH_IN_NEG,
    /**< Alarm error, Expression: ShuntingYard: 3subtree issue with child when
       Neg */
    VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_PARENT,
    /**< Alarm error, Expression: ShuntingYard: 3subtree issue with parent */
    VTSS_RC_ERROR_ALARM_ENTRY_NOT_FOUND,
    /**< Alarm error, The adressed entry was not found */

};  // Leave it anonymous

/**
 * Function for converting a alarm  module error
 * (see ALARM_RC_xxx above) to a textual string.
 * Only errors in the Thread Load Monitor module's range can
 * be converted.
 *
 * \param rc [IN] Binary form of error
 *
 * \return Static string containing textual representation of #rc.
 */
const char *alarm_error_txt(mesa_rc rc);

#ifndef VTSS_BASICS_STANDALONE
/* Initialize module */
mesa_rc alarm_init(vtss_init_data_t *data);
#endif

#endif /* !defined(__ALARM_API_H__) */
