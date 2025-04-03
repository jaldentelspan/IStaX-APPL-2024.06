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
#include "ifIndex_api.h"
#include <vtss/appl/interface.h>

#ifdef VTSS_SW_OPTION_IP
#include "ip_utils.hxx"
#endif /* VTSS_SW_OPTION_IP */

#include <vtss_module_id.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SNMP

/* mib2/ifTable - ifIndex */
#define IFTABLE_IFINDEX_LPOAG_START                 0
#define IFTABLE_IFINDEX_LPOAG_END                   (((VTSS_ISID_CNT - 1) * IFTABLE_IFINDEX_SWITCH_INTERVAL) - 1)
#define IFTABLE_IFINDEX_GLAG_START                  VTSS_IFINDEX_GLAG_OFFSET
#define IFTABLE_IFINDEX_VLAN_START                  VTSS_IFINDEX_VLAN_OFFSET

#define IFTABLE_IFINDEX_LLAG_INTERVAL               VTSS_IFINDEX_LLAG_UNIT_OFFSET
#define IFTABLE_IFINDEX_SWITCH_INTERVAL             VTSS_IFINDEX_UNIT_MULTP

#define IfIndex2Isid(_IFINDEX)                      ((_IFINDEX) / (IFTABLE_IFINDEX_SWITCH_INTERVAL))
#define IfIndex2Interface(_IFINDEX, _START_ID)      (((_IFINDEX) > (_START_ID)) ? ((_IFINDEX) - (_START_ID)) : 0)
#define Interface2IfIndex(_INTERFACE, _START_ID)    ((_INTERFACE) + (_START_ID))

#ifdef VTSS_SW_OPTION_IP

static int cmp_ip(vtss_appl_ip_if_status_t *data, mesa_ip_addr_t *key)
{
    int cmp;
    mesa_ip_type_t data_type = data->type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV6 ? MESA_IP_TYPE_IPV6 : MESA_IP_TYPE_IPV4;
    if ( data_type > key->type ) {
        return 1;
    } else if ( data_type < key->type ) {
        return -1;
    }

    if (data_type == MESA_IP_TYPE_IPV4) {
        cmp = (data->u.ipv4.net.address > key->addr.ipv4) ? 1 : (data->u.ipv4.net.address < key->addr.ipv4) ? -1 : 0;
    } else {
        cmp = memcmp(&data->u.ipv6.net.address, &key->addr.ipv6, sizeof(mesa_ipv6_t));
    }

    return cmp;
}

static mesa_rc get_next_ipv6_status_by_if(mesa_vid_t if_id, mesa_ip_addr_t *ip_addr, vtss_appl_ip_if_status_t *v6_st)
{
#define MAX_OBJS 16
    u32                 if_st_cnt = 0;
    vtss_appl_ip_if_status_t    status[MAX_OBJS], *ptr = status;
    mesa_ip_addr_t      tmp;
    u32                 i = 0, j = 0;
    mesa_rc             found = VTSS_RC_ERROR;

    tmp.type = MESA_IP_TYPE_IPV6;
    memset(&tmp.addr.ipv6, 0xff, sizeof(tmp.addr.ipv6));

    vtss_ifindex_t ifidx = vtss_ifindex_cast_from_u32(if_id, VTSS_IFINDEX_TYPE_VLAN);
    if (vtss_appl_ip_if_status_get(ifidx, VTSS_APPL_IP_IF_STATUS_TYPE_IPV6, MAX_OBJS, &if_st_cnt, status) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    for (i = 0, ptr = status; i < if_st_cnt; i++, ptr++) {
        if ( (cmp_ip(ptr, &tmp) > 0 ) ||
             (ip_addr != NULL && (cmp_ip( ptr, ip_addr ) <= 0 )) ) {
            continue;
        }

        (void) vtss_appl_ip_if_status_to_ip( ptr, &tmp);
        j = i;
        found = VTSS_RC_OK;
    }

    if ( VTSS_RC_OK == found ) {
        *v6_st = status[j];
    }

#undef MAX_OBJS
    return found;
}


/*
 * Transfer information of vtss_ifindex_elm_t to iftable_info_t
 */
static BOOL ifIndex_elm_to_iftable_info(iftable_info_t *info)
{
    vtss_ifindex_elm_t ife;

    if (vtss_ifindex_decompose(vtss_ifindex_cast_from_u32_0(info->ifIndex), &ife) != VTSS_RC_OK) {
        return FALSE;
    }

    if ((ife.iftype == VTSS_IFINDEX_TYPE_PORT) ||
        (ife.iftype == VTSS_IFINDEX_TYPE_LLAG)) {
#ifdef IFINDEX_TEMP_SOLUTION
        vtss_handle_t netsnmp_handle;
        cyg_net_snmp_thread_handle_get(&netsnmp_handle);
        if ((vtss_thread_self() == netsnmp_handle) && !msg_switch_exists(ife.isid)) {
            return FALSE;
        }
#endif
    }

    info->isid = ife.isid;

    switch (ife.iftype) {
    case VTSS_IFINDEX_TYPE_PORT:
        info->type = IFTABLE_IFINDEX_TYPE_PORT;
        info->if_id = ife.ordinal;
        return TRUE;
    case VTSS_IFINDEX_TYPE_LLAG:
        info->type = IFTABLE_IFINDEX_TYPE_LLAG;
        info->if_id = ife.ordinal;
        return TRUE;
    case VTSS_IFINDEX_TYPE_GLAG:
        info->type = IFTABLE_IFINDEX_TYPE_GLAG;
        info->if_id = ife.ordinal;
        return TRUE;
    case VTSS_IFINDEX_TYPE_VLAN:
        info->type = IFTABLE_IFINDEX_TYPE_VLAN;
        info->if_id = ife.ordinal;
        return TRUE;
    case VTSS_IFINDEX_TYPE_ILLEGAL:
    default:
        break;
    }

    return FALSE;
}

BOOL get_next_ip(mesa_ip_addr_t *ip_addr, mesa_vid_t *if_id)
{
    mesa_rc                  rc;
    vtss_appl_ip_if_status_t st, st_v6_first;
    mesa_ip_addr_t           tmp, tmp_v6_first;
    mesa_vid_t               vid, ifid_v6_first = VTSS_VID_NULL;
    BOOL                     found = FALSE, found_v6_first = FALSE;
    vtss_ifindex_t           prev_ifindex, ifindex;

    if ( ip_addr == NULL || ip_addr->type > MESA_IP_TYPE_IPV6) {
        return FALSE;
    }

    if ( ip_addr->type < MESA_IP_TYPE_IPV6) {
        tmp.type = MESA_IP_TYPE_IPV4;
        tmp.addr.ipv4 = 0;
    } else {
        tmp.type = MESA_IP_TYPE_IPV6;
        memset(&tmp.addr.ipv6, 0, sizeof(tmp.addr.ipv6));
    }

    tmp_v6_first.type = MESA_IP_TYPE_IPV6;
    memset(&tmp_v6_first.addr.ipv6, 0xff, sizeof(tmp_v6_first.addr.ipv6));

    prev_ifindex = VTSS_IFINDEX_NONE;
    while (vtss_appl_ip_if_itr(&prev_ifindex, &ifindex, true /* only VLAN interfaces */) == VTSS_RC_OK) {
        prev_ifindex = ifindex;

        vid = vtss_ifindex_as_vlan(ifindex);

        if (ip_addr->type == MESA_IP_TYPE_IPV4) {
            if (get_next_ipv6_status_by_if(vid, NULL, &st_v6_first) == VTSS_RC_OK ) {
                if (cmp_ip(&st_v6_first, &tmp_v6_first) < 0 ) {
                    memcpy(&tmp_v6_first.addr.ipv6, &st_v6_first.u.ipv6.net.address, sizeof(st_v6_first.u.ipv6.net.address));
                    found_v6_first = TRUE;
                    ifid_v6_first = vid;
                }
            }

            rc = vtss_appl_ip_if_status_get(ifindex, VTSS_APPL_IP_IF_STATUS_TYPE_IPV4, 1, nullptr, &st);
        } else {
            rc = get_next_ipv6_status_by_if(vid, ip_addr, &st);
        }

        if (rc != VTSS_RC_OK || cmp_ip(&st, ip_addr) <= 0) {
            continue;
        }

        if (found == FALSE || cmp_ip(&st, &tmp) < 0) {
            if (st.type == VTSS_APPL_IP_IF_STATUS_TYPE_IPV4) {
                tmp.type = MESA_IP_TYPE_IPV4;
                tmp.addr.ipv4 = st.u.ipv4.net.address;
            } else {
                tmp.type = MESA_IP_TYPE_IPV6;
                memcpy(&tmp.addr.ipv6, &st.u.ipv6.net.address, sizeof(st.u.ipv6.net.address));
            }

            if (if_id) {
                *if_id = vid;
            }

            found = TRUE;
        }
    }

    if (found) {
        *ip_addr = tmp;
    } else if (found_v6_first) {
        if (if_id) {
            *if_id = ifid_v6_first;
        }

        *ip_addr = tmp_v6_first;
        found = TRUE;
    }

    return found;
}

#endif /* VTSS_SW_OPTION_IP */

typedef BOOL    (ifIndex_cb_t) ( ifIndex_id_t startIdx, iftable_info_t *info, BOOL check_exist );

typedef struct {
    ifIndex_id_t startIdx;
    ifIndex_type_t type;
    ifIndex_cb_t *get_next_cb_fn;
} ifIndex_cb_table_t;


//Add a temporary solution to deny the switch access (configurable but non-existing) via SNMP
//This solution is for v3.40 only.
#define IFINDEX_TEMP_SOLUTION

//#ifdef IFINDEX_TEMP_SOLUTION
//#include <ucd-snmp/config.h>
//#include <ucd-snmp/asn1.h>
//#include <ucd-snmp/snmp_api.h>
//#include <ucd-snmp/snmpd.h>
//#endif

#if 0
static BOOL port_get_next(ifIndex_id_t startIdx, iftable_info_t *info, BOOL check_exist)
{
    vtss_isid_t     isid = topo_usid2isid(IfIndex2Isid(info->ifIndex) + 1);
    u_long          if_id = IfIndex2Interface(info->ifIndex, startIdx);

    if (isid >= VTSS_ISID_END || !msg_switch_configurable(isid) || if_id >= port_count_max()) {
        return FALSE;
    }

#ifdef IFINDEX_TEMP_SOLUTION
    vtss_handle_t netsnmp_handle;
    cyg_net_snmp_thread_handle_get(&netsnmp_handle);
    if ((vtss_thread_self() == netsnmp_handle) && !msg_switch_exists(isid)) {
        return FALSE;
    }
#endif

//    T_D("enter startIdx = %d , isid = %d, if_id = %d, port_count = %d", (int)startIdx, (int)isid, (int)if_id, (int)port_count_max());
    info->isid = isid;
    info->if_id = uport2iport(++if_id);
    info->ifIndex = Interface2IfIndex(if_id, startIdx);
//    T_D("isid %d PORT %d ifIndex %d %s ", (int)info->isid, (int)info->if_id, (int)info->ifIndex, TRUE == check_exist ? "exist" : "valid");
    return TRUE;
}
#endif

#if 0
static BOOL llag_get_next(ifIndex_id_t startIdx, iftable_info_t *info, BOOL check_exist)
{
#if (AGGR_LLAG_CNT == 0)
    return FALSE;
#else
    vtss_isid_t                 isid = topo_usid2isid(IfIndex2Isid(info->ifIndex) + 1);
    u_long                      if_id = IfIndex2Interface(info->ifIndex, startIdx);
    aggr_mgmt_group_no_t        aggr_id = 0;
    aggr_mgmt_group_member_t    aggr_members;
//    T_D("enter isid = %d, if_id = %d, port_count = %d", (int)isid, (int)if_id, (int)port_count_max());

    if ( isid >= VTSS_ISID_END || !msg_switch_configurable(isid) ) {
        return FALSE;
    }

#ifdef IFINDEX_TEMP_SOLUTION
    vtss_handle_t netsnmp_handle;
    cyg_net_snmp_thread_handle_get(&netsnmp_handle);
    if ((vtss_thread_self() == netsnmp_handle) && !msg_switch_exists(isid)) {
        return FALSE;
    }
#endif

    if (!AGGR_MGMT_GROUP_IS_LAG((aggr_mgmt_group_no_t)(if_id + 1))) {
        return FALSE;
    }

    aggr_id = if_id;

    if (TRUE == check_exist) {
        if ((aggr_mgmt_port_members_get(isid, aggr_id >= MGMT_AGGR_ID_START ? mgmt_aggr_id2no(aggr_id) : (MGMT_AGGR_ID_START - 1), &aggr_members, TRUE) != VTSS_RC_OK)
#ifdef VTSS_SW_OPTION_LACP
            && (aggr_mgmt_lacp_members_get(isid, aggr_id >= MGMT_AGGR_ID_START ? mgmt_aggr_id2no(aggr_id) : (MGMT_AGGR_ID_START - 1), &aggr_members, TRUE) != VTSS_RC_OK)
#endif /* VTSS_SW_OPTION_LACP */
           ) {
            return FALSE;
        }
        aggr_id = mgmt_aggr_no2id(aggr_members.aggr_no);
    } else {
        aggr_id = if_id + 1;
    }

    if (!AGGR_MGMT_GROUP_IS_LAG(aggr_id)) {
        T_E("aggr_id valid");
        return FALSE;
    }

    info->if_id = mgmt_aggr_id2no(aggr_id);
    info->isid = isid;
    info->ifIndex = Interface2IfIndex(aggr_id, startIdx);
//    T_D("isid %d LLAG %d ifIndex %d %s ", (int)info->isid, (int)info->if_id, (int)info->ifIndex, TRUE == check_exist ? "exist" : "valid");

    return TRUE;
#endif
}
#endif

#if 0
static BOOL glag_get_next(ifIndex_id_t startIdx, iftable_info_t *info, BOOL check_exist)
{

#if (AGGR_GLAG_CNT == 0)
    return FALSE;
#else
    u_long                      if_id = IfIndex2Interface(info->ifIndex, startIdx);
    aggr_mgmt_group_no_t        glag_id = if_id, aggr;
    aggr_mgmt_group_member_t    aggr_members;
    vtss_isid_t                 isid = 0;
    BOOL                        found = FALSE;

    memset(&aggr_members, 0, sizeof(aggr_members));
//    T_D("enter isid %d glag %d ", (int)info->isid, (int)glag_id);
    if ( glag_id >= AGGR_GLAG_CNT ) {
        return FALSE;
    }

    if (TRUE == check_exist) {
        for (aggr = AGGR_MGMT_GLAG_START + glag_id - AGGR_MGMT_GROUP_NO_START; aggr < AGGR_MGMT_GROUP_NO_END; aggr++) {
            for (isid = VTSS_ISID_START; isid < VTSS_ISID_END; isid++) {
                if (!msg_switch_configurable(isid)) {
                    continue;
                }

#ifdef IFINDEX_TEMP_SOLUTION
                vtss_handle_t netsnmp_handle;
                cyg_net_snmp_thread_handle_get(&netsnmp_handle);
                if ((vtss_thread_self() == netsnmp_handle) && !msg_switch_exists(isid)) {
                    continue;
                }
#endif

                if ((aggr_mgmt_port_members_get(isid, aggr + 1, &aggr_members, FALSE) != VTSS_RC_OK)
#ifdef VTSS_SW_OPTION_LACP
                    && (aggr_mgmt_lacp_members_get(isid, aggr + 1, &aggr_members, FALSE) != VTSS_RC_OK)
#endif /* VTSS_SW_OPTION_LACP */
                   ) {
                    continue;
                }
                found = TRUE;
                break;
            }
            if ( TRUE == found ) {
                break;
            }
        }

        if (FALSE == found) {
            return FALSE;
        }
        glag_id = mgmt_aggr_no2id(aggr_members.aggr_no);
    } else {
        glag_id++;
    }

    info->if_id = mgmt_aggr_id2no(glag_id);
    info->ifIndex = Interface2IfIndex(glag_id, startIdx);
    info->isid = VTSS_ISID_UNKNOWN;
//    T_D("isid %d GLAG %d ifIndex %d %s ", (int)info->isid, (int)info->if_id, (int)info->ifIndex, TRUE == check_exist ? "exist" : "valid");
    return TRUE;
#endif
}
#endif

#if 0
static BOOL vlan_get_next(ifIndex_id_t startIdx, iftable_info_t *info, BOOL check_exist)
{
    u_long          if_id = IfIndex2Interface(info->ifIndex, startIdx);
//    T_D("enter isid %d ID %d ", (int)info->isid, (int)if_id);

    if (if_id >= VTSS_APPL_VLAN_ID_MAX) {
        return FALSE;
    }

    if ( FALSE == check_exist ) {
        info->isid = VTSS_ISID_UNKNOWN;
        info->if_id = ++if_id;
        info->ifIndex = Interface2IfIndex(if_id, startIdx);
    } else {
        vtss_appl_vlan_entry_t vlan_mgmt_entry;

        /* search valid interface */
        vlan_mgmt_entry.vid = if_id;
        if (vtss_appl_vlan_get(VTSS_ISID_GLOBAL, vlan_mgmt_entry.vid, &vlan_mgmt_entry, TRUE, VTSS_APPL_VLAN_USER_ALL) != VTSS_RC_OK) {
            return FALSE;
        }

        info->isid = VTSS_ISID_UNKNOWN;
        info->if_id = vlan_mgmt_entry.vid;
        info->ifIndex = Interface2IfIndex(vlan_mgmt_entry.vid, startIdx);
    }
//    T_D("isid %d VLAN %d ifIndex %d %s ", (int)info->isid, (int)info->if_id, (int)info->ifIndex, TRUE == check_exist ? "exist" : "valid");
    return TRUE;
}
#endif

/**
  * \brief Get the existent IfIndex.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [IN] ifIndex: The ifIndex
  *                       [OUT] type: The interface type
  *                       [OUT] isid: The ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get( iftable_info_t *info )
{
    vtss_ifindex_t      prev_ifindex, next_ifindex;

    prev_ifindex = vtss_ifindex_cast_from_u32_0(info->ifIndex - 1);
    if (vtss_ifindex_getnext(&prev_ifindex, &next_ifindex, TRUE) != VTSS_RC_OK) {
        return FALSE;
    }

    if (next_ifindex != info->ifIndex) {
        return FALSE;
    }

    if (ifIndex_elm_to_iftable_info(info) == FALSE) {
        return FALSE;
    }

    return TRUE;
}

/**
  * \brief Get the valid IfIndex.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [IN] ifIndex: The ifIndex
  *                       [OUT] type: The interface type
  *                       [OUT] isid: The ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_valid( iftable_info_t *info )
{
    vtss_ifindex_t      prev_ifindex, next_ifindex;

    prev_ifindex = vtss_ifindex_cast_from_u32_0(info->ifIndex - 1);
    if (vtss_ifindex_getnext(&prev_ifindex, &next_ifindex, FALSE) != VTSS_RC_OK) {
        return FALSE;
    }

    if (next_ifindex != info->ifIndex) {
        return FALSE;
    }

    if (ifIndex_elm_to_iftable_info(info) == FALSE) {
        return FALSE;
    }

    return TRUE;
}


/**
  * \brief Get the next existent IfIndex.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [INOUT] ifIndex: The next ifIndex
  *                       [OUT] type: The next interface type
  *                       [OUT] isid: The next ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: the next interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_next( iftable_info_t *info )
{
    vtss_ifindex_t          prev_ifindex;
    iftable_info_t          tmp_info;

    prev_ifindex = vtss_ifindex_cast_from_u32_0(info->ifIndex);

    while (vtss_ifindex_getnext(&prev_ifindex, (vtss_ifindex_t *)&tmp_info.ifIndex, TRUE) == VTSS_RC_OK) {
        prev_ifindex = vtss_ifindex_cast_from_u32_0(tmp_info.ifIndex);

        if (ifIndex_elm_to_iftable_info(&tmp_info) == TRUE) {
            *info = tmp_info;
            return TRUE;
        }
    } /* while */

    return FALSE;
}

/**
  * \brief Get the next valid IfIndex.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [INOUT] ifIndex: The next ifIndex
  *                       [OUT] type: The next interface type
  *                       [OUT] isid: The next ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: the next interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_next_valid( iftable_info_t *info )
{
    vtss_ifindex_t          prev_ifindex;
    iftable_info_t          tmp_info;

    prev_ifindex = vtss_ifindex_cast_from_u32_0(info->ifIndex);
    while (vtss_ifindex_getnext(&prev_ifindex, (vtss_ifindex_t *)&tmp_info.ifIndex, FALSE) == VTSS_RC_OK) {
        prev_ifindex = vtss_ifindex_cast_from_u32_0(tmp_info.ifIndex);

        if (ifIndex_elm_to_iftable_info(&tmp_info) == TRUE) {
            *info = tmp_info;
            return TRUE;
        }
    } /* while */

    return FALSE;
}

/**
  * \brief Get the first existent IfIndex in the specific type.
  *
  * \param info [INOUT]:    The info parameter has following members\n
  *                       [IN] type: The interface type
  *                       [OUT] ifIndex: The first ifIndex, if any
  *                       [OUT] isid: The first ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The first interface ID
  * \return
  *    FALSE if there is no available ifIndex.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_first_by_type( iftable_info_t *info )
{
    u32                     enumerate_types;
    vtss_ifindex_t          prev_ifindex = VTSS_IFINDEX_NONE;
    iftable_info_t          tmp_info;

    switch (info->type) {
    case IFTABLE_IFINDEX_TYPE_PORT:
        enumerate_types = VTSS_IFINDEX_GETNEXT_PORTS;
        break;
    case IFTABLE_IFINDEX_TYPE_LLAG:
        enumerate_types = VTSS_IFINDEX_GETNEXT_LLAGS;
        break;
    case IFTABLE_IFINDEX_TYPE_GLAG:
        enumerate_types = VTSS_IFINDEX_GETNEXT_GLAGS;
        break;
    case IFTABLE_IFINDEX_TYPE_VLAN:
        enumerate_types = VTSS_IFINDEX_GETNEXT_VLANS;
        break;
    default:
        return FALSE;
    }/* switch */

    while (vtss_ifindex_iterator(&prev_ifindex, (vtss_ifindex_t *)&tmp_info.ifIndex, enumerate_types, TRUE) == VTSS_RC_OK) {
        prev_ifindex = vtss_ifindex_cast_from_u32_0(tmp_info.ifIndex);
        if (ifIndex_elm_to_iftable_info(&tmp_info) == TRUE) {
            *info = tmp_info;
            return TRUE;
        }
    }
    return FALSE;
}

/**
  * \brief Get the next existent IfIndex in the specific type.
  *
  * \param info [INOUT]:    The info parameter has following members\n
  *                       [INOUT] ifIndex: The next ifIndex, if any
  *                       [OUT] type: The interface type
  *                       [OUT] isid: The first ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The first interface ID
  * \param if_types [IN]:   Using macro IFTABLE_IFINDEX_TYPE_FLAG_SET(if_types, IFTABLE_IFINDEX_TYPE_XXX) to specify interface types for filtering the get next result.
  * \return
  *    FALSE if there is no available ifIndex.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_next_by_type( iftable_info_t *info, u32 if_types )
{
    u32                     enumerate_types = 0;
    vtss_ifindex_t          prev_ifindex = vtss_ifindex_cast_from_u32_0(info->ifIndex);
    iftable_info_t          tmp_info;

    if (IFTABLE_IFINDEX_TYPE_FLAG_CHK(if_types, IFTABLE_IFINDEX_TYPE_PORT)) {
        enumerate_types |= VTSS_IFINDEX_GETNEXT_PORTS;
    }

    if (IFTABLE_IFINDEX_TYPE_FLAG_CHK(if_types, IFTABLE_IFINDEX_TYPE_LLAG)) {
        enumerate_types |= VTSS_IFINDEX_GETNEXT_LLAGS;
    }

    if (IFTABLE_IFINDEX_TYPE_FLAG_CHK(if_types, IFTABLE_IFINDEX_TYPE_GLAG)) {
        enumerate_types |= VTSS_IFINDEX_GETNEXT_GLAGS;
    }

    if (IFTABLE_IFINDEX_TYPE_FLAG_CHK(if_types, IFTABLE_IFINDEX_TYPE_VLAN)) {
        enumerate_types |= VTSS_IFINDEX_GETNEXT_VLANS;
    }

    if (enumerate_types == 0) {
        /* interface type does not specified */
        return FALSE;
    }

    while (vtss_ifindex_iterator(&prev_ifindex, (vtss_ifindex_t *)&tmp_info.ifIndex, enumerate_types, TRUE) == VTSS_RC_OK) {
        prev_ifindex = vtss_ifindex_cast_from_u32_0(tmp_info.ifIndex);
        if (ifIndex_elm_to_iftable_info(&tmp_info) == TRUE) {
            *info = tmp_info;
            return TRUE;
        }
    }
    return FALSE;
}

/**
  * \brief Get the IfIndex in the specific type.
  *
  * \param info [INOUT]:    The info parameter has following members\n
  *                       [IN] if_id: The interface ID
  *                       [IN] type: The interface type
  *                       [IN] isid: The ISID
  *                       [OUT] ifIndex: The ifIndex
  * \return
  *    FALSE if there is no available ifIndex.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_by_interface( iftable_info_t *info )
{
    BOOL                rc = FALSE;
    vtss_ifindex_t      ifindex;

    if ( (info->type == IFTABLE_IFINDEX_TYPE_PORT || info->type == IFTABLE_IFINDEX_TYPE_LLAG) && !VTSS_ISID_LEGAL(info->isid)) {
        return FALSE;
    }

    switch (info->type) {
    case IFTABLE_IFINDEX_TYPE_PORT:
        if (vtss_ifindex_from_port(info->isid, info->if_id, &ifindex) == VTSS_RC_OK) {
            rc = TRUE;
        }
        break;
    case IFTABLE_IFINDEX_TYPE_LLAG:
        if (vtss_ifindex_from_llag(info->isid, info->if_id, &ifindex) == VTSS_RC_OK) {
            rc = TRUE;
        }
        break;
    case IFTABLE_IFINDEX_TYPE_GLAG:
        if (vtss_ifindex_from_glag(info->if_id, &ifindex) == VTSS_RC_OK) {
            rc = TRUE;
        }
        break;
    case IFTABLE_IFINDEX_TYPE_VLAN:
        if (vtss_ifindex_from_vlan(info->if_id, &ifindex) == VTSS_RC_OK) {
            rc = TRUE;
        }
        break;
    default:
        return FALSE;
    }

    if (rc == TRUE) {
        info->ifIndex = vtss_ifindex_cast_to_u32(ifindex);
    }

    return rc;
}


/* ifIndex initial function
   return 0 when operation is success, - 1 otherwize. */
int ifIndex_init(void)
{
    return 0;
}
