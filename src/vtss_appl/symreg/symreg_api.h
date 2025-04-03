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

#ifndef _SYMREG_API_H_
#define _SYMREG_API_H_

#include <main_types.h>
#include <vtss/appl/module_id.h>

#include <main_types.h>
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
// symreg_init()
/******************************************************************************/
mesa_rc symreg_init(vtss_init_data_t *data);

/**
 * SymReg module error codes (mesa_rc)
 */
enum {
    SYMREG_RC_PARAM = MODULE_ERROR_START(VTSS_MODULE_ID_SYMREG),
    SYMREG_RC_OUT_OF_MEMORY,
    SYMREG_RC_PAT_TOO_MANY_COLONS,
    SYMREG_RC_PAT_COMPONENT_TOO_LONG,
    SYMREG_RC_PAT_AT_LEAST_ONE_COMPONENT_MUST_BE_SPECIFIED,
    SYMREG_RC_PAT_TWO_LEFT_BRACKETS_SEEN,
    SYMREG_RC_PAT_RIGHT_BRACKET_SEEN_BEFORE_LEFT_BRACKET,
    SYMREG_RC_PAT_EXTRA_CHARACTERS_AFTER_END_BRACKET,
    SYMREG_RC_PAT_MISSING_RIGHT_BRACKET,
    SYMREG_RC_PAT_INVALID_REPLICATION_LIST,
    SYMREG_RC_NO_SUCH_REPL_TGT,
    SYMREG_RC_NO_SUCH_REPL_GRP,
    SYMREG_RC_NO_SUCH_REPL_REG,
    SYMREG_RC_NO_SUCH_TGT,
    SYMREG_RC_NO_SUCH_GRP,
    SYMREG_RC_NO_SUCH_REG,
    SYMREG_RC_NO_MORE_REGS,
}; // Leave it anonymous

/**
 * Initialize the query interface with a register pattern.
 *
 * This will allocate required memory into \p handle, which is used
 * in subsequent calls to query addresses, names, and match count.
 *
 * When done, remember to free it with a call to symreg_query_uninit().
 *
 * The pattern is on the form:
 * target[t]:reggrp[g]:reg[r]
 *   where"
 *     'target' is the name of the target (e.g. dev).
 *     'reggrp' is the name of the register group..
 *     'reg'    is the name of the register.
 *     t        is a list of target replications if applicable.
 *     g        is a list of register group replications if applicable.
 *     r        is a list of register replications if applicable.
 *   If a given replication (t, g, r) is omitted, all applicable replications will be accessed.
 *   Both 'target', 'reggrp' and 'reg' may be omitted, which corresponds to wildcarding that part
 *   of the name. Matches are exact, but wildcards ('*', '?') are allowed.
 *   Example 1. dev1g[0-7,12]::dev_ptp_tx_id[0-3]
 *   Example 2. dev1g::dev_rst_ctrl
 *   Example 3: ana_ac:aggr[10]:aggr_cfg
 *   Example 4: ana_ac::aggr_cfg
 *   Example 5: ::MAC_ENA_CFG
 *
 * \param handle    [OUT] Pointer to an area of memory which holds info about the query.
 * \param pattern   [IN]  Pointer to a pattern on the form specified above.
 * \param max_width [OUT] Pointer receiving strlen() of longest register name (to prepare for formatting).
 * \param reg_cnt   [OUT] Pointer receiving number of registers matching pattern.
 *
 * \return VTSS_RC_OK on success, otherwise an error. Use error_txt(rc) to get
 * a textual represenation of the error.
 */
mesa_rc symreg_query_init(void **handle, char *pattern, u32 *max_width, u32 *reg_cnt);

/**
 * Uninitialize the handle you got with call to symreg_query_init()
 *
 * \param handle [IN] Pointer to handle you want to free.
 *
 * \return VTSS_RC_OK on success, otherwise an error. Use error_txt(rc) to
 * get a textual representation of the error.
 */
mesa_rc symreg_query_uninit(void *handle);

/**
 * Iterate over registers matching the pattern set with symreg_query_init()
 *
 * Keep calling this function (first time with #next set to FALSE, subsequent
 * times with #next set to TRUE) to obtain all register names and addresses
 * matching the pattern you set with symreg_query_init().
 *
 * The length allocated to #addr must be at least the max_width returned
 * by symreg_query_init + 1 (for terminating NULL).
 *
 * \param handle [IN]  Pointer to handle you're investigating.
 * \param name   [OUT] Full, qualified register name (see above for minimum length).
 * \param addr   [OUT] Absolute address of register as seen from the internal CPU.
 * \param offset [OUT] Register's offset from base address (#addr - VTSS_IO_ORIGIN1_OFFSET)
 * \param next   [IN]  If very first query, set to FALSE, otherwise set to TRUE.
 *
 * \return As long as the info returned is correct, the function returns VTSS_RC_OK.
 *         Once there are no more registers to iterate across, the function
 *         return SYMREG_RC_NO_MORE_REGS. If an error occurs, another SYMREG_RC_xxx
 *         code, which can be translated into a textual representation with
 *         error_txt(rc).
 */
mesa_rc symreg_query_next(void *handle, char *const name, u32 *addr, u32 *offset, BOOL next);

/******************************************************************************/
//
// UTILITY FUNCTIONS
//
/******************************************************************************/

/**
 * Function for converting a SymReg error
 * (see SYMREG_RC_xxx above) to a textual string.
 * Only errors in the SYMREG module's range can
 * be converted.
 *
 * \param rc [IN] Binary form of error
 *
 * \return Static string containing textual representation of #rc.
 */
const char *symreg_error_txt(mesa_rc rc);

#ifdef __cplusplus
}
#endif

#endif /* _SYMREG_API_H_ */

