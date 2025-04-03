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

#include "syslog_serializer.hxx"
#include "vtss/appl/syslog.h"
#include "vtss_common_iterator.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-depend-N.hxx"

const vtss_enum_descriptor_t syslog_lvl_txt[] = {
    {VTSS_APPL_SYSLOG_LVL_ERROR,    "error"},
    {VTSS_APPL_SYSLOG_LVL_WARNING,  "warning"},
    {VTSS_APPL_SYSLOG_LVL_NOTICE,   "notice"},
    {VTSS_APPL_SYSLOG_LVL_INFO,     "informational"},
    {VTSS_APPL_SYSLOG_LVL_ALL,      "all"},
    {0, 0},
};


mesa_rc vtss_appl_syslog_history_itr(
    const vtss_usid_t   *const prev_swid_idx,
    vtss_usid_t         *const next_swid_idx,
    const u32           *const prev_syslog_idx,
    u32                 *const next_syslog_idx) {
    vtss::IteratorComposeDependN<vtss_usid_t, u32> itr(
        &vtss_appl_iterator_switch,
        &syslog_id_itr);

    return itr(prev_swid_idx, next_swid_idx, prev_syslog_idx, next_syslog_idx);
}

mesa_rc SL_lvl_itr(const vtss_appl_syslog_lvl_t    *const prev_lvl_idx,
                   vtss_appl_syslog_lvl_t          *const next_lvl_idx) {
    if (prev_lvl_idx) { // getnext
        if (*prev_lvl_idx <= VTSS_APPL_SYSLOG_LVL_ERROR) {
            *next_lvl_idx = VTSS_APPL_SYSLOG_LVL_WARNING;
        } else if (*prev_lvl_idx <= VTSS_APPL_SYSLOG_LVL_WARNING) {
            *next_lvl_idx = VTSS_APPL_SYSLOG_LVL_NOTICE;
        } else if (*prev_lvl_idx <= VTSS_APPL_SYSLOG_LVL_NOTICE) {
            *next_lvl_idx = VTSS_APPL_SYSLOG_LVL_INFO;
        } else if (*prev_lvl_idx <= VTSS_APPL_SYSLOG_LVL_INFO) {
            *next_lvl_idx = VTSS_APPL_SYSLOG_LVL_ALL;
        } else {
            return VTSS_RC_ERROR;
        }
    } else { // getfirst
        *next_lvl_idx = VTSS_APPL_SYSLOG_LVL_ERROR;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_syslog_history_control_itr(
    const vtss_usid_t               *const prev_swid_idx,
    vtss_usid_t                     *const next_swid_idx,
    const vtss_appl_syslog_lvl_t    *const prev_lvl_idx,
    vtss_appl_syslog_lvl_t          *const next_lvl_idx) {
    vtss::IteratorComposeN<vtss_usid_t, vtss_appl_syslog_lvl_t> itr(
            &vtss_appl_iterator_switch,
            &SL_lvl_itr);

    return itr(prev_swid_idx, next_swid_idx, prev_lvl_idx, next_lvl_idx);
}
