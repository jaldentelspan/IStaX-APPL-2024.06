/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss_common_iterator.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-static-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-filter2.hxx"
#include "port_api.h"    // For port_is_front_port()
#include "port_trace.h"
#include "msg.h"
#include <vtss/appl/interface.h>
#ifdef VTSS_SW_OPTION_POE
#include "poe.h"
#endif

#include "standalone_api.h"

#define VTSS_APPL_CPU_MAX 100

using namespace vtss;
using namespace expose::snmp;

BOOL is_usid_exist(
    vtss_usid_t     usid
)
{
    vtss_isid_t     isid;

    if (usid >= VTSS_USID_END) {
        return FALSE;
    }

    isid = topo_usid2isid(usid);

    if (msg_switch_is_local(isid) || msg_switch_exists(isid)) {
        return TRUE;
    }

    return FALSE;
}

BOOL is_usid_configurable(
    vtss_usid_t     usid
)
{
    vtss_isid_t     isid;

    if (usid >= VTSS_USID_END) {
        return FALSE;
    }

    isid = topo_usid2isid(usid);

    if (msg_switch_is_local(isid) || msg_switch_configurable(isid)) {
        return TRUE;
    }

    return FALSE;
}

mesa_rc vtss_appl_iterator_switch(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
)
{
    // Define a range iterator
    IteratorComposeRange<vtss_usid_t>   usid_range(VTSS_USID_START, VTSS_USID_END);

    // filter out the valid part of the range
    return usid_range.filtered(prev_usid, next_usid, &is_usid_exist);
}

mesa_rc vtss_appl_iterator_switch_all(
    const vtss_usid_t   *const prev_usid,
    vtss_usid_t         *const next_usid
)
{
    // Define a range iterator
    IteratorComposeRange<vtss_usid_t>   usid_range(VTSS_USID_START, VTSS_USID_END);

    // filter out the valid part of the range
    return usid_range.filtered(prev_usid, next_usid, &is_usid_configurable);
    
    return VTSS_RC_OK;
}


// See vtss_common_iterator.hxx
mesa_rc vtss_appl_iterator_ifindex_front_port_exist(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex)
{
    vtss_ifindex_t      prev_tmp, next_tmp;
    vtss_ifindex_elm_t  ife;

    if (!next_ifindex) {
        return VTSS_RC_ERROR;
    }

    if (prev_ifindex) {
        prev_tmp = *prev_ifindex;
    } else {
        prev_tmp = VTSS_IFINDEX_NONE;
    }

    while (vtss_ifindex_getnext_port_exist(&prev_tmp, &next_tmp) == VTSS_RC_OK) {
        prev_tmp = next_tmp;
        if (vtss_ifindex_decompose(next_tmp, &ife) != VTSS_RC_OK) {
            continue;
        }

        if (!port_is_front_port(ife.ordinal)) {
            continue;
        }

        *next_ifindex = next_tmp;
        return VTSS_RC_OK;
    }/* while */

    return VTSS_RC_ERROR;
}



#ifdef VTSS_SW_OPTION_POE
// See vtss_common_iterator.hxx
mesa_rc vtss_appl_iterator_ifindex_poe_front_port_exist(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex)
{
    vtss_ifindex_t      prev_tmp, next_tmp;
    vtss_ifindex_elm_t  ife;

    if (!next_ifindex) {
        return VTSS_RC_ERROR;
    }

    if (prev_ifindex) {
        prev_tmp = *prev_ifindex;
    } else {
        prev_tmp = VTSS_IFINDEX_NONE;
    }

    while (vtss_ifindex_getnext_port_exist(&prev_tmp, &next_tmp) == VTSS_RC_OK) {
        prev_tmp = next_tmp;
        if (vtss_ifindex_decompose(next_tmp, &ife) != VTSS_RC_OK) {
            continue;
        }

        if (!port_is_front_port(ife.ordinal)) {
            continue;
        }

        *next_ifindex = next_tmp;
        
        T_D("next_tmp= %d  ife.ordinal=%d, poe_support=%d", next_tmp.private_ifindex_data_do_not_use_directly, ife.ordinal, port_custom_table[ife.ordinal].poe_support);
        if(!port_custom_table[ife.ordinal].poe_support) {
            continue;
        }

        return VTSS_RC_OK;
    }/* while */

    return VTSS_RC_ERROR;
}
#endif




/*
    Check if ifindex of physical port is valid and existed.
    If elm is NULL, then do checking only.
    If valid and elm is not NULL, then get the corresponding isid and iport.

*/
mesa_rc vtss_appl_ifindex_port_exist(
    vtss_ifindex_t          ifindex,
    vtss_ifindex_elm_t      *elm
)
{
    vtss_ifindex_elm_t  ife;

    /* get isid/iport from ifindex and validate them */
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT                ||
        msg_switch_exists(ife.isid) == FALSE                ||
        ife.ordinal >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)               ) {
        return VTSS_RC_ERROR;
    }

    if (elm) {
        memcpy(elm, &ife, sizeof(vtss_ifindex_elm_t));
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_iterator_ifindex_front_port(
    const vtss_ifindex_t    *const prev_ifindex,
    vtss_ifindex_t          *const next_ifindex
)
{
    vtss_ifindex_t      prev_tmp = VTSS_IFINDEX_NONE, next_tmp;
    vtss_ifindex_elm_t  ife;

    if (prev_ifindex) {
        prev_tmp = *prev_ifindex;
    }

    while (vtss_ifindex_getnext_port_configurable(&prev_tmp, &next_tmp) == VTSS_RC_OK) {
        prev_tmp = next_tmp;
        if (vtss_ifindex_decompose(next_tmp, &ife) != VTSS_RC_OK) {
            continue;
        }

        if (!port_is_front_port(ife.ordinal)) {
            continue;
        }

        *next_ifindex = next_tmp;
        return VTSS_RC_OK;
    }/* while */

    return VTSS_RC_ERROR;
}

/*
    Check if ifindex of physical port is valid and configurable.
    If elm is NULL, then do checking only.
    If valid and elm is not NULL, then get the corresponding isid and iport.

*/
mesa_rc vtss_appl_ifindex_port_configurable(
    vtss_ifindex_t          ifindex,
    vtss_ifindex_elm_t      *elm
)
{
    vtss_ifindex_elm_t  ife;

    /* get isid/iport from ifindex and validate them */
    if (vtss_ifindex_decompose(ifindex, &ife) != VTSS_RC_OK ||
        ife.iftype != VTSS_IFINDEX_TYPE_PORT                ||
        msg_switch_configurable(ife.isid) == FALSE          ||
        ife.ordinal >= fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT)               ) {
        return VTSS_RC_ERROR;
    }

    if (elm) {
        memcpy(elm, &ife, sizeof(vtss_ifindex_elm_t));
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_ifindex_getnext_port_queue(
    const vtss_ifindex_t *prev_ifindex, vtss_ifindex_t *next_ifindex,
    const mesa_prio_t    *prev_queue,   mesa_prio_t    *next_queue)
{
    if (!prev_queue) {
        *next_queue = 0;
    } else {
        if (*prev_queue >= (VTSS_PRIOS - 1)) {
            *next_queue = 0;
        } else {
            *next_ifindex = *prev_ifindex;
            *next_queue   = *prev_queue + 1;
            return VTSS_RC_OK;
        }
    }

    return vtss_appl_iterator_ifindex_front_port(prev_ifindex, next_ifindex);
}
