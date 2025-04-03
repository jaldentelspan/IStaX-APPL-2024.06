/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "alarm-trace.h"
#include "vtss/basics/trace.hxx"
#include "alarm-expose.hxx"
#include "alarm_api.hxx"

#ifdef VTSS_SW_OPTION_ICLI
#include "alarm_icfg.h"
#endif

static vtss_trace_reg_t trace_reg = {VTSS_MODULE_ID_ALARM, "alarm", "ALARM"};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

const char *alarm_error_txt(mesa_rc error) {
    switch (error) {
    case VTSS_RC_OK:
        return "ALARM: OK";
    case VTSS_RC_ERROR_ALARM_EXPR_SYNTAX_PARSE_UNKNOWN:
        return "ALARM, Expression: parse syntax";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_ALLOC_OPR_STACK_PUSH_BACK:
        return "ALARM, Expression: ShuntingYard: alloc oprator stack push back";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_ALLOC_ELEM_STACK_PUSH_BACK:
        return "ALARM, Expression: ShuntingYard: alloc element stack push back";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_POP_OPR_STACK_ZERO:
        return "ALARM, Expression: ShuntingYard: pop oprator stack when zero";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_POP_ELEM_STACK_ZERO:
        return "ALARM, Expression: ShuntingYard: pop element stack when zero";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_PARANTHES_MISMATCH:
        return "ALARM, Expression: ShuntingYard: paranthes mismatch";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_OPR_PARANTHES_MISMATCH:
        return "ALARM, Expression: ShuntingYard: operator followed by "
               "paranthes";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_END_WITH_OPR:
        return "ALARM, Expression: ShuntingYard: expression end with oprator";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_END_ELEM_STACK_SIZE_NOT_ONE:
        return "ALARM, Expression: ShuntingYard: element stack size not end "
               "with 1";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_CHILDS_PTR:
        return "ALARM, Expression: ShuntingYard: 3subtree issue with child "
               "ptrs";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_NEG_CHILDS_PTR:
        return "ALARM, Expression: ShuntingYard: NEG 3subtree issue with child "
               "ptrs";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_CHILDS_MISMATCH_IN_NUMBINOP:
        return "ALARM, Expression: ShuntingYard: 3subtree issue with child "
               "when NumericBinOp";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_CHILDS_MISMATCH_IN_BOOLBINOP:
        return "ALARM, Expression: ShuntingYard: 3subtree issue with child "
               "when BoolBinOp";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_CHILDS_MISMATCH_IN_COMPARE:
        return "ALARM, Expression: ShuntingYard: 3subtree issue with child "
               "when Compare";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_CHILD_MISMATCH_IN_NEG:
        return "ALARM, Expression: ShuntingYard: 3subtree issue with child "
               "when Neg";
    case VTSS_RC_ERROR_ALARM_EXPR_SHUNTY_SEMANTIC_3SUBTREE_PARENT:
        return "ALARM, Expression: ShuntingYard: 3subtree issue with parent";
    case VTSS_RC_ERROR_ALARM_ENTRY_NOT_FOUND:
        return "ALARM, The adressed entry was not found";
    case VTSS_RC_ERROR:
        return "ALARM: ERROR";
    }
    return "ALARM: Undefined error code";
}

extern "C" int alarm_icli_cmd_register();

/* Initialize module */
mesa_rc alarm_init(vtss_init_data_t *data)
{
    vtss_isid_t isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_I("INIT");
        vtss::appl::alarm::vtss_alarm_init();

#ifdef VTSS_SW_OPTION_PRIVATE_MIB
/* Register our private mib */
        alarm_mib_init();
#endif

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_alarm_json_init();
#endif

        alarm_icli_cmd_register();
        break;

    case INIT_CMD_START:
#ifdef VTSS_SW_OPTION_ICFG
        if (alarm_icfg_init() != VTSS_RC_OK) {
            T_D("Calling alarm_icfg_init() failed");
        }
#endif
        break;

    case INIT_CMD_CONF_DEF: {
        T_I("CONF_DEF, isid: %d", isid);
        vtss_appl_alarm_name_t tmp_name_in;
        vtss_appl_alarm_name_t tmp_name_out;
        mesa_rc res;
        res = vtss_appl_alarm_conf_itr((vtss_appl_alarm_name_t *)0, &tmp_name_out);

        while (res == VTSS_RC_OK) {
            vtss_appl_alarm_conf_del(&tmp_name_out);
            tmp_name_in = tmp_name_out;
            res = vtss_appl_alarm_conf_itr(&tmp_name_in, &tmp_name_out);
        }

        break;
    }

    default:
        break;
    }

    return VTSS_RC_OK;
}
