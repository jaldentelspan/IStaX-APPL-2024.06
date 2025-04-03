/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "symreg_api.h" /* Check that public function decls. and defs. are in sync */
#include "mgmt_api.h"   /* For mgmt_txt2bf() */

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SYMREG
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SYMREG

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "symreg", "symreg"
};

#define VTSS_TRACE_GRP_DEFAULT 0

#include <vtss_trace_api.h>

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/******************************************************************************/
//
// Module Public Functions
//
/******************************************************************************/

#define SYMREG_REPL_LEN_MAX 32 /* Allocate 32 chars for the user to specify a replication list */
// TBD_IMPROVEMENT
// The following values are hardcoded, but instead
// they should come from the API since they are
// chip specific. See vtss_chipname_symregs.c in the
// CIL.
#define SYMREG_REPL_CNT_MAX 8192
#define SYMREG_NAME_LEN_MAX 68

typedef enum {
    SYMREG_COMPONENTS_TGT,
    SYMREG_COMPONENTS_GRP,
    SYMREG_COMPONENTS_REG,
    SYMREG_COMPONENTS_LAST
} symreg_components_t;

typedef struct {
    BOOL wildcards;
    BOOL all_repls;
    u8   repls[VTSS_BF_SIZE(SYMREG_REPL_CNT_MAX)];
    char pattern[SYMREG_NAME_LEN_MAX + SYMREG_REPL_LEN_MAX + 1];

    // State variables that remember where we got to.
    int  cur_idx;
    int  cur_repl;
    u32  match_cnt;
    u8   matched_repls[VTSS_BF_SIZE(SYMREG_REPL_CNT_MAX)];
    BOOL matched_at_least_one_name;
    char match_name[SYMREG_NAME_LEN_MAX + SYMREG_REPL_LEN_MAX + 1];
    u32  match_width;
    u32  match_addr;

    union {
        const mesa_symreg_target_t *t;
        const mesa_symreg_reggrp_t *g;
        const mesa_symreg_reg_t    *r;
    } u;

    // For debug
    const char *self_name;
} symreg_state_t;

typedef struct {
    symreg_state_t state[SYMREG_COMPONENTS_LAST];
} symreg_inst_t;

static mesa_symreg_data_t SYMREG_DATA;

/******************************************************************************/
//
// Private functions
//
/******************************************************************************/

#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
/******************************************************************************/
// SYMREG_component_to_name()
/******************************************************************************/
static const char *SYMREG_component_to_name(int component)
{
    switch (component) {
    case SYMREG_COMPONENTS_TGT:
        return "Tgt";

    case SYMREG_COMPONENTS_GRP:
        return "Grp";

    case SYMREG_COMPONENTS_REG:
        return "Reg";

    default:
        return "Unknown";
    }
}
#endif

/******************************************************************************/
// SYMREG_print_match_pattern()
/******************************************************************************/
static void SYMREG_print_match_pattern(int component, symreg_state_t *s)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
    char buf[SYMREG_REPL_LEN_MAX + 1]; // Output of mgmt_bf2txt() cannot exceed input to mgmt_txt2bf() in size.
#endif

    T_D("%s:", SYMREG_component_to_name(component));
    T_D("Wildcards: %s", s->wildcards ? "Yes" : "No");
    T_D("Replications: %s", s->all_repls ? "All" : mgmt_bf2txt(s->repls, 0, SYMREG_REPL_CNT_MAX - 1, buf));
    T_D("Pattern: %s\n", s->pattern);
}

/****************************************************************************/
// SYMREG_wildcmp()
// Searches for #w (which may contain wildcards ('*' and '?')) in #s
// and returns TRUE if we have a match, FALSE otherwise.
/****************************************************************************/
static BOOL SYMREG_wildcmp(const char *w, const char *s)
{
    const char *cp = NULL, *mp = NULL;
    const char *wild = w, *string = s;

    while (*string && *wild != '*') {
        if (*wild != *string && *wild != '?') {
            return FALSE;
        }

        wild++;
        string++;
    }

    while (*string) {
        if (*wild == '*') {
            if (!*++wild) {
                return TRUE;
            }

            mp = wild;
            cp = string + 1;
        } else if ((*wild == *string) || (*wild == '?')) {
            wild++;
            string++;
        } else {
            if (cp == NULL) {
                T_E("Error when matching wild = %s against string = %s", w, s);
                return FALSE;
            }

            wild = mp;
            string = cp++;
        }
    }

    if (wild == NULL) {
        T_E("Error when matching wild = %s against string = %s", w, s);
        return FALSE;
    }

    while (*wild == '*') {
        wild++;
    }

    return !*wild;
}

/****************************************************************************/
// SYMREG_match_name()
/****************************************************************************/
static BOOL SYMREG_match_name(symreg_state_t *s, const char *name)
{
    BOOL result;
    if (s->wildcards) {
        result = SYMREG_wildcmp(s->pattern, name);
    } else {
        // User has requested an exact pattern
        result = strcmp(s->pattern, name) == 0;
    }

    T_N("%s: Considering %s: %s", s->self_name, name, result ? "Yes" : "No");
    return result;
}

/****************************************************************************/
// SYMREG_match_repl()
/****************************************************************************/
static inline BOOL SYMREG_match_repl(symreg_state_t *s, u32 repl)
{
    if (s->all_repls) {
        return TRUE;
    } else {
        return VTSS_BF_GET(s->repls, repl);
    }
}

/****************************************************************************/
// SYMREG_reg_next()
/****************************************************************************/
static BOOL SYMREG_reg_next(symreg_state_t *r, mesa_symreg_reg_t const *regs)
{
    if (r->cur_idx < 0 || r->cur_repl >= (int)regs[r->cur_idx].repl_cnt - 1) {
        // Either we're not started, or we've matched the last
        // replication in the previous iteration. Find next
        // matching register by name.
        r->cur_repl = -1;
        r->cur_idx++;
    }

    while (regs[r->cur_idx].name != NULL) {
        u32 repl;

        r->u.r = &regs[r->cur_idx];

        if (SYMREG_match_name(r, r->u.r->name)) {
            // Name match. Check to see if there is a replication fit.
            r->matched_at_least_one_name = TRUE;

            for (repl = r->cur_repl + 1; repl < r->u.r->repl_cnt; repl++) {
                if (SYMREG_match_repl(r, repl)) {
                    r->cur_repl = repl;
                    r->match_cnt++;
                    r->match_addr = 4 * (repl * r->u.r->repl_width + r->u.r->addr);

                    if (r->u.r->repl_cnt == 1) {
                        strcpy(r->match_name, r->u.r->name);
                    } else {
                        sprintf(r->match_name, "%s[%u]", r->u.r->name, r->cur_repl);
                    }

                    r->match_width = strlen(r->match_name);
                    return TRUE;
                }
            }
        }

        // No replication match
        r->cur_repl = -1;
        r->cur_idx++;
    }

    return FALSE;
}

/****************************************************************************/
// SYMREG_grp_next()
/****************************************************************************/
static BOOL SYMREG_grp_next(symreg_state_t *g, mesa_symreg_reggrp_t const *reggrps)
{
    if (g->cur_idx < 0 || g->cur_repl >= (int)reggrps[g->cur_idx].repl_cnt - 1) {
        // Either we're not started, or we've matched the last
        // replication in the previous iteration. Find next
        // matching group by name.
        g->cur_repl = -1;
        g->cur_idx++;
    }

    while (reggrps[g->cur_idx].name != NULL) {
        u32 repl;

        g->u.g = &reggrps[g->cur_idx];

        if (SYMREG_match_name(g, g->u.g->name)) {
            // Name match. Check to see if there is a replication fit.
            g->matched_at_least_one_name = TRUE;

            for (repl = g->cur_repl + 1; repl < g->u.g->repl_cnt; repl++) {
                if (SYMREG_match_repl(g, repl)) {
                    g->cur_repl = repl;
                    g->match_cnt++;
                    g->match_addr = 4 * (g->u.g->base_addr + repl * g->u.g->repl_width);

                    if (g->u.g->repl_cnt == 1) {
                        strcpy(g->match_name, g->u.g->name);
                    } else {
                        sprintf(g->match_name, "%s[%u]", g->u.g->name, g->cur_repl);
                    }

                    g->match_width = strlen(g->match_name);
                    return TRUE;
                }
            }
        }

        // No replication match
        g->cur_repl = -1;
        g->cur_idx++;
    }

    return FALSE;
}

/****************************************************************************/
// SYMREG_tgt_next()
/****************************************************************************/
static BOOL SYMREG_tgt_next(symreg_state_t *t)
{
    // Each target only has one replication.
    t->cur_idx++;

    while (t->cur_idx < SYMREG_DATA.targets_cnt) {
        t->u.t = &SYMREG_DATA.targets[t->cur_idx];

        if (SYMREG_match_name(t, t->u.t->name)) {
            // Target repolications are special in that a replication number
            // of -1 indicates that it's not replicated. All replications
            // are explicit, because targets may lie on different base
            // addresses, which are not evenly distanced.
            // We allow targets with no replications to be indexed by
            // replication 0:
            u32 repl = t->u.t->repl_number < 0 ? 0 : t->u.t->repl_number;

            t->matched_at_least_one_name = TRUE;

            if (SYMREG_match_repl(t, repl)) {
                t->cur_repl = repl; // Zero-based (not -1)
                t->match_cnt++;
                t->match_addr = t->u.t->base_addr;

                if (t->u.t->repl_number < 0) {
                    // Target not replicated
                    strcpy(t->match_name, t->u.t->name);
                } else {
                    sprintf(t->match_name, "%s[%u]", t->u.t->name, t->cur_repl);
                }

                t->match_width = strlen(t->match_name);
                return TRUE;
            }
        }

        t->cur_idx++;
    }

    return FALSE;
}

/****************************************************************************/
// SYMREG_state_clear()
/****************************************************************************/
static void SYMREG_state_clear(symreg_state_t *s, BOOL all)
{
    s->cur_idx  = -1;
    s->cur_repl = -1;

    if (all) {
        s->match_cnt                 = 0;
        s->matched_at_least_one_name = FALSE;
        memset(s->matched_repls, 0, sizeof(s->matched_repls));
    }
}

/****************************************************************************/
// SYMREG_all_repls_present()
/****************************************************************************/
static BOOL SYMREG_all_repls_present(u8 *requested_repls, u8 *matched_repls)
{
    u32 repl;

    // When the user has requested a particular set of replications
    // to match, all the requested replications must be present.
    for (repl = 0; repl < SYMREG_REPL_CNT_MAX; repl++) {
        if (VTSS_BF_GET(requested_repls, repl) && !VTSS_BF_GET(matched_repls, repl)) {
            return FALSE;
        }
    }

    return TRUE;
}

/****************************************************************************/
// SYMREG_check_pattern()
/****************************************************************************/
static mesa_rc SYMREG_check_pattern(symreg_inst_t *inst, u32 *max_width, u32 *reg_cnt)
{
    symreg_state_t *t = &inst->state[SYMREG_COMPONENTS_TGT];
    symreg_state_t *g = &inst->state[SYMREG_COMPONENTS_GRP];
    symreg_state_t *r = &inst->state[SYMREG_COMPONENTS_REG];
    int            i;

    for (i = 0; i < SYMREG_COMPONENTS_LAST; i++) {
        SYMREG_state_clear(&inst->state[i], TRUE);
    }

    *max_width = 0;

    while (SYMREG_tgt_next(t)) {
        SYMREG_state_clear(g, FALSE);

        while (SYMREG_grp_next(g, t->u.t->reggrps)) {
            SYMREG_state_clear(r, FALSE);

            while (SYMREG_reg_next(r, g->u.g->regs)) {
                u32 width = t->match_width + g->match_width + r->match_width + 2 /* Colons */;

                if (width > *max_width) {
                    *max_width = width;
                }

                // Keep track of the replications that matched.
                VTSS_BF_SET(t->matched_repls, t->cur_repl, 1);
                VTSS_BF_SET(g->matched_repls, g->cur_repl, 1);
                VTSS_BF_SET(r->matched_repls, r->cur_repl, 1);

                T_D("%s:%s:%s", t->match_name, g->match_name, r->match_name);
            }
        }
    }

    *reg_cnt = r->match_cnt;

    T_D("Reg-count = %u", *reg_cnt);
    T_D("Max. width = %u", *max_width);

    // Final checks so that we can return the correct error code.
    // Discern between a name not found and a replication not found.
    if (!t->matched_at_least_one_name) {
        return SYMREG_RC_NO_SUCH_TGT;
    }

    if (!g->matched_at_least_one_name) {
        return SYMREG_RC_NO_SUCH_GRP;
    }

    if (!r->matched_at_least_one_name) {
        return SYMREG_RC_NO_SUCH_REG;
    }

    if (!t->all_repls && !SYMREG_all_repls_present(t->repls, t->matched_repls)) {
        return SYMREG_RC_NO_SUCH_REPL_TGT;
    }

    if (!g->all_repls && !SYMREG_all_repls_present(g->repls, g->matched_repls)) {
        return SYMREG_RC_NO_SUCH_REPL_GRP;
    }

    if (!r->all_repls && !SYMREG_all_repls_present(r->repls, r->matched_repls)) {
        return SYMREG_RC_NO_SUCH_REPL_REG;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
//
// Public functions
//
/******************************************************************************/

/******************************************************************************/
// symreg_query_init()
/******************************************************************************/
mesa_rc symreg_query_init(void **handle, char *pattern, u32 *max_width, u32 *reg_cnt)
{
    symreg_inst_t *inst;
    int           i;
    char          *str1 = pattern, *str2;
    size_t        cnt;
    BOOL          at_least_one_component_found = FALSE;
    mesa_rc       rc = VTSS_RC_OK;

    if (handle == NULL || pattern == NULL) {
        return SYMREG_RC_PARAM;
    }

    *handle = NULL;

    if ((inst = (symreg_inst_t *)VTSS_MALLOC(sizeof(symreg_inst_t))) == NULL) {
        return SYMREG_RC_OUT_OF_MEMORY;
    }

    memset(inst, 0, sizeof(*inst));

    // For debug:
    inst->state[SYMREG_COMPONENTS_TGT].self_name = "Tgt";
    inst->state[SYMREG_COMPONENTS_GRP].self_name = "Grp";
    inst->state[SYMREG_COMPONENTS_REG].self_name = "Reg";

    // Time to parse syntax
    for (i = 0; i < SYMREG_COMPONENTS_LAST; i++) {
        if (str1 == NULL) {
            break;
        }

        str2 = strstr(str1, ":");
        if (str2) {
            // Colon found
            if (i == SYMREG_COMPONENTS_REG) {
                rc = SYMREG_RC_PAT_TOO_MANY_COLONS;
                goto do_exit;
            }

            cnt = str2 - str1;
        } else {
            // Colon not found.
            cnt = strlen(str1);
        }

        if (cnt + 1 > sizeof(inst->state[i].pattern)) {
            rc = SYMREG_RC_PAT_COMPONENT_TOO_LONG;
            goto do_exit;
        }

        if (cnt > 0) {
            strncpy(inst->state[i].pattern, str1, cnt + 1);
            // Terminate in case a colon was found in str1.
            inst->state[i].pattern[cnt] = '\0';
            at_least_one_component_found = TRUE;
        }

        if (str2 && str2[0] == ':') {
            str1 = str2 + 1;
        } else {
            str1 = NULL;
        }
    }

    if (!at_least_one_component_found) {
        rc = SYMREG_RC_PAT_AT_LEAST_ONE_COMPONENT_MUST_BE_SPECIFIED;
        goto do_exit;
    }

    // Loop once more and find replications and whether it contains wildcards
    for (i = 0; i < SYMREG_COMPONENTS_LAST; i++) {
        char *bracket_start = NULL;
        char *bracket_end   = NULL;
        int  j;

        str1 = inst->state[i].pattern;
        cnt = strlen(str1);
        for (j = 0; j < (int)cnt; j++) {
            *str1 = toupper(*str1);

            if (*str1 == '?' || *str1 == '*') {
                inst->state[i].wildcards = TRUE;
            }

            if (*str1 == '[') {
                if (bracket_start != NULL) {
                    rc = SYMREG_RC_PAT_TWO_LEFT_BRACKETS_SEEN;
                    goto do_exit;
                }

                bracket_start = str1;
            } else if (bracket_end == NULL && *str1 == ']') {
                if (bracket_start == NULL) {
                    rc = SYMREG_RC_PAT_RIGHT_BRACKET_SEEN_BEFORE_LEFT_BRACKET;
                    goto do_exit;
                }

                bracket_end = str1;
            } else if (bracket_end != NULL) {
                rc = SYMREG_RC_PAT_EXTRA_CHARACTERS_AFTER_END_BRACKET;
                goto do_exit;
            }

            str1++;
        }

        if (bracket_start && !bracket_end) {
            rc = SYMREG_RC_PAT_MISSING_RIGHT_BRACKET;
            goto do_exit;
        } else if (bracket_start == NULL || bracket_end == NULL || bracket_end == bracket_start + 1) {
            // If no brackets are found, or an empty list is given (e.g. dev1g[<nothing_here>]:xxx:xxx), show all replications.
            inst->state[i].all_repls = TRUE;
        } else {
            inst->state[i].all_repls = FALSE;

            // Parse the list, but first terminate it in both ends.
            *bracket_start = *bracket_end = '\0';
            if (mgmt_txt2bf(bracket_start + 1, inst->state[i].repls, 0, SYMREG_REPL_CNT_MAX - 1, 0) != VTSS_RC_OK) {
                rc = SYMREG_RC_PAT_INVALID_REPLICATION_LIST;
                goto do_exit;
            }
        }

        if (inst->state[i].pattern[0] == '\0') {
            // Wildcard it if empty
            strcpy(inst->state[i].pattern, "*");
            inst->state[i].wildcards = TRUE;
        }
    }

    for (i = 0; i < SYMREG_COMPONENTS_LAST; i++) {
        SYMREG_print_match_pattern(i, &inst->state[i]);
    }

    // Check the pattern against the actual registers in the chip.
    rc = SYMREG_check_pattern(inst, max_width, reg_cnt);

do_exit:
    if (rc == VTSS_RC_OK) {
        *handle = inst;
    } else {
        VTSS_FREE(inst);
    }

    return rc;
}

/******************************************************************************/
// symreg_query_uninit()
/******************************************************************************/
mesa_rc symreg_query_uninit(void *handle)
{
    if (handle == NULL) {
        return SYMREG_RC_PARAM;
    }

    VTSS_FREE(handle);
    return VTSS_RC_OK;
}

/****************************************************************************/
// symreg_query_next()
/****************************************************************************/
/*lint --e{429} */
mesa_rc symreg_query_next(void *handle, char *const name, u32 *addr, u32 *offset, BOOL next)
{
    symreg_inst_t  *inst  = (symreg_inst_t *)handle;
    symreg_state_t *t     = &inst->state[SYMREG_COMPONENTS_TGT];
    symreg_state_t *g     = &inst->state[SYMREG_COMPONENTS_GRP];
    symreg_state_t *r     = &inst->state[SYMREG_COMPONENTS_REG];
    int            i;

    if (handle == NULL || name == NULL || addr == NULL || offset == NULL) {
        return SYMREG_RC_PARAM;
    }

    if (!next) {
        for (i = 0; i < SYMREG_COMPONENTS_LAST; i++) {
            SYMREG_state_clear(&inst->state[i], TRUE);
        }
    }

    while (next || SYMREG_tgt_next(t)) {
        if (!next) {
            SYMREG_state_clear(g, FALSE);
        }

        while (next || SYMREG_grp_next(g, t->u.t->reggrps)) {
            if (!next) {
                SYMREG_state_clear(r, FALSE);
            }

            next = FALSE;

            while (SYMREG_reg_next(r, g->u.g->regs)) {
                sprintf(name, "%s:%s:%s", t->match_name, g->match_name, r->match_name);
                *addr = t->match_addr + g->match_addr + r->match_addr;
                *offset = *addr - SYMREG_DATA.io_origin1_offset;
                T_D("name = %s, addr = %u ", name, *addr);

                return VTSS_RC_OK;
            }
        }
    }

    return SYMREG_RC_NO_MORE_REGS;
}

/******************************************************************************/
//
// Public utility functions
//
/******************************************************************************/

/******************************************************************************/
// symreg_error_txt()
/******************************************************************************/
const char *symreg_error_txt(mesa_rc rc)
{
    switch (rc) {
    case SYMREG_RC_PARAM:
        return "Invalid parameter";

    case SYMREG_RC_OUT_OF_MEMORY:
        return "Out of memory";

    case SYMREG_RC_PAT_TOO_MANY_COLONS:
        return "Syntax error: Too many colons";

    case SYMREG_RC_PAT_COMPONENT_TOO_LONG:
        return "Syntax error: Sub-component too long";

    case SYMREG_RC_PAT_AT_LEAST_ONE_COMPONENT_MUST_BE_SPECIFIED:
        return "Syntax error: At least one target, one register group, or one register must be specified. You don't want to see all registers in the chip";

    case SYMREG_RC_PAT_TWO_LEFT_BRACKETS_SEEN:
        return "Syntax error: Two left-brackets seen in one sub-component";

    case SYMREG_RC_PAT_RIGHT_BRACKET_SEEN_BEFORE_LEFT_BRACKET:
        return "Syntax error: Right-bracket seen before left-bracket in a sub-component";

    case SYMREG_RC_PAT_EXTRA_CHARACTERS_AFTER_END_BRACKET:
        return "Syntax error: Extra characters after end-bracket in a sub-component";

    case SYMREG_RC_PAT_MISSING_RIGHT_BRACKET:
        return "Syntax error: Missing right-bracket in a sub-component";

    case SYMREG_RC_PAT_INVALID_REPLICATION_LIST:
        return "Syntax error: Invalid replication list within brackets of a sub-component";

    case SYMREG_RC_NO_SUCH_REPL_TGT:
        return "No such target replication found";

    case SYMREG_RC_NO_SUCH_REPL_GRP:
        return "No such register group replication found";

    case SYMREG_RC_NO_SUCH_REPL_REG:
        return "No such register replication found";

    case SYMREG_RC_NO_SUCH_TGT:
        return "No such target. Try using wildcards";

    case SYMREG_RC_NO_SUCH_GRP:
        return "No such register group. Try using wildcards";

    case SYMREG_RC_NO_SUCH_REG:
        return "No such register. Try using wildcards";

    case SYMREG_RC_NO_MORE_REGS:
        return "No more registers (this is not an error, so please correct use of this return code)";

    default:
        return "Unknown SymReg module error";
    }
}

extern "C" int symreg_icli_cmd_register();

/******************************************************************************/
// symreg_init()
/******************************************************************************/
mesa_rc symreg_init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        symreg_icli_cmd_register();
    }

    if (data->cmd == INIT_CMD_START) {
        if (mesa_symreg_data_get(0, &SYMREG_DATA) != VTSS_RC_OK) {
            T_E("Failed to get symreg-data");
        }

        if (SYMREG_DATA.repl_cnt_max > SYMREG_REPL_CNT_MAX) {
            T_E("SYMREG_REPL_CNT_MAX buffer is too small - will cause memory overwrite! SYMREG_REPL_CNT_MAX should be " VPRIlu, SYMREG_DATA.repl_cnt_max);
            T_E("This is a bug!");
        }

        if (SYMREG_DATA.name_len_max > SYMREG_NAME_LEN_MAX) {
            T_E("SYMREG_NAME_LEN_MAX buffer is too small - will cause memory overwrite! SYMREG_NAME_LEN_MAX should be " VPRIlu, SYMREG_DATA.name_len_max);
            T_E("This is a bug!");
        }
    }

    return VTSS_RC_OK;
}

