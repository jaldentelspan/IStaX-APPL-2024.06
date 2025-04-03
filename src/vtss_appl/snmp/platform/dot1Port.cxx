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

#include <main.h>
#include "misc_api.h"
#include "mgmt_api.h"
#include "msg_api.h"
#include "dot1Port_api.h"


#include <vtss_module_id.h>
//#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SNMP

#define DOT1PORT_2_L2PORT(_PORT) ((_PORT) - DOT1PORT_NO_START)
#define L2PORT_2_DOT1PORT(_PORT) ((_PORT) + DOT1PORT_NO_START)

static BOOL l2Port_exists(l2port_iter_t *l2pit)
{
#ifdef VTSS_SW_OPTION_AGGR
    vtss_isid_t                 isid = 0;
    BOOL                        found = FALSE;
    aggr_mgmt_group_member_t    aggr_members;
    aggr_mgmt_group_no_t        aggr_no;
#endif /* VTSS_SW_OPTION_AGGR */
    switch (l2pit->type) {
    case L2PORT_ITER_TYPE_PHYS:
        T_D("l2port = %d is PHYS", l2pit->l2port);
        break;
#ifdef VTSS_SW_OPTION_AGGR
    case L2PORT_ITER_TYPE_LLAG:
        (void)l2port2poag(l2pit->l2port, &isid, &aggr_no);
        if ((aggr_mgmt_port_members_get(isid, aggr_no, &aggr_members, FALSE) != VTSS_RC_OK)
#ifdef VTSS_SW_OPTION_LACP
            && (aggr_mgmt_lacp_members_get(isid, aggr_no, &aggr_members, FALSE) != VTSS_RC_OK)
#endif /* VTSS_SW_OPTION_LACP */
           ) {
            return FALSE;
        }
        T_D("l2port = %d is LLAG, aggr_no = %u", l2pit->l2port, aggr_no);
        break;
    case L2PORT_ITER_TYPE_GLAG:
        (void)l2port2glag(l2pit->l2port, &aggr_no);
        for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
            if (!msg_switch_exists(isid)) {
                continue;
            }

            if ((aggr_mgmt_port_members_get(isid, aggr_no, &aggr_members, FALSE) != VTSS_RC_OK)
#ifdef VTSS_SW_OPTION_LACP
                && (aggr_mgmt_lacp_members_get(isid, aggr_no, &aggr_members, FALSE) != VTSS_RC_OK)
#endif /* VTSS_SW_OPTION_LACP */
               ) {
                continue;
            }
            found = TRUE;
            break;
        }

        if (FALSE == found) {
            return FALSE;
        }

        T_D("l2port = %d is GLAG, aggr_no = %u", l2pit->l2port, aggr_no);
        break;
#endif /* VTSS_SW_OPTION_AGGR */
    default:
        return FALSE;

    }

    return TRUE;
}

static BOOL l2Port_get_first(l2_port_no_t *l2port)
{
    l2port_iter_t            l2pit;
    (void) l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, L2PORT_ITER_TYPE_ALL);
    (void) l2port_iter_getnext(&l2pit);

    *l2port = l2pit.l2port;
    return TRUE;
}


static BOOL l2Port_get_next(l2_port_no_t *l2port)
{
    l2port_iter_t            l2pit;
    BOOL            found = FALSE;
    (void) l2port_iter_init(&l2pit, VTSS_ISID_GLOBAL, L2PORT_ITER_TYPE_ALL);
    while (l2port_iter_getnext(&l2pit)) {
        if ( l2pit.l2port > *l2port && l2Port_exists(&l2pit)) {
            found = TRUE;
            break;
        }
    }

    if (found == FALSE) {
        return FALSE;
    }
    *l2port = l2pit.l2port;
    return TRUE;
}

static void l2port2dot1Port (l2_port_no_t l2port, dot1Port_info_t *info )
{
    vtss_poag_no_t poag;
    if (TRUE == l2port2port(l2port, &info->isid, &poag)) {
        info->if_id = poag;
        info->type = DOT1PORT_TYPE_PORT;
    } else if (TRUE == l2port2poag(l2port, &info->isid, &poag)) {
        info->if_id = poag;
        info->type = DOT1PORT_TYPE_LLAG;
    } else if (TRUE == l2port2glag(l2port, &poag)) {
        info->if_id = poag;
        info->type = DOT1PORT_TYPE_GLAG;
    }
    info->dot1port =  L2PORT_2_DOT1PORT(l2port);
}

/**
  * \brief Get the existent dot1Port.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [IN]  l2port: l2port
  *                       [OUT] type: The interface type
  *                       [OUT] isid: The ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL dot1Port_get( dot1Port_info_t *info )
{
    l2_port_no_t l2port;

    if ( info->dot1port < DOT1PORT_NO_START ) {
        return FALSE;
    } else if (info->dot1port == DOT1PORT_NO_START) {
        l2port = DOT1PORT_2_L2PORT(info->dot1port);
    } else {
        l2port = DOT1PORT_2_L2PORT(info->dot1port - 1);
        if ( FALSE == l2Port_get_next(&l2port) || l2port != DOT1PORT_2_L2PORT(info->dot1port) ) {
            return FALSE;
        }
    }

    l2port2dot1Port(l2port, info);
    return TRUE;

}

/**
  * \brief Get the next existent dot1Port.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [IN]  l2port: l2port
  *                       [OUT] type: The interface type
  *                       [OUT] isid: The ISID, if the output type is neither DOT1PORT_TYPE_PORT nor DOT1PORT_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL dot1Port_get_next( dot1Port_info_t *info )
{
    l2_port_no_t l2port;

    if ( DOT1PORT_NO_NONE == info->dot1port) {
        (void)l2Port_get_first(&l2port);
    } else {
        l2port = DOT1PORT_2_L2PORT(info->dot1port);
        if (FALSE == l2Port_get_next(&l2port)) {
            return FALSE;
        }
    }

    l2port2dot1Port(l2port, info);
    return TRUE;

}

/**
  * \brief Get the existent dot1Port in the specific type.
  *
  * \param info [INOUT]:    The info parameter has following members\n
  *                       [IN] type: The interface type
  *                       [IN] if_id: The interface ID
  *                       [IN] isid: The ISID, if the output type is neither DOT1PORT_TYPE_PORT nor DOT1PORT_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] l2port: The l2port
  * \return
  *    FALSE if there is no available ifIndex.\n
  *     Otherwise, return TRUE.
  */
BOOL dot1Port_get_by_interface( dot1Port_info_t *info )
{
    l2_port_no_t l2port;

    if ( (info->type == DOT1PORT_TYPE_PORT || info->type == DOT1PORT_TYPE_LLAG ) && ( !VTSS_ISID_LEGAL(info->isid) || !msg_switch_exists (info->isid))) {
        return FALSE;
    }

    switch (info->type) {
    case DOT1PORT_TYPE_PORT:
        l2port = L2PORT2PORT(info->isid, info->if_id);
        break;
    case DOT1PORT_TYPE_LLAG:
        l2port = L2LLAG2PORT(info->isid, info->if_id - AGGR_MGMT_GROUP_NO_START);
        break;
    case DOT1PORT_TYPE_GLAG:
        l2port = L2GLAG2PORT(info->if_id - AGGR_MGMT_GROUP_NO_START);
        break;
    default:
        return FALSE;
    }

    if (FALSE == l2port_is_valid(l2port)) {
        return FALSE;
    }
    info->dot1port = L2PORT_2_DOT1PORT(l2port);

    return TRUE;
}

