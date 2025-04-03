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
//----------------------------------------------------------------------------
/**
 *  \file
 *      vtss_dhcp_server.c
 *
 *  \brief
 *      DHCP server engine
 *
 *  \author
 *      CP Wang
 *
 *  \date
 *      05/09/2013 11:45
 */
//----------------------------------------------------------------------------
/*
==============================================================================

    Include File

==============================================================================
*/
#include "vtss_dhcp_server.h"
#include "vtss_dhcp_server_message.h"
#include "vtss/appl/dhcp_relay.h"
#include "dhcp_server_platform.h"
#include "vtss_avl_tree_api.h"
#include "vtss_free_list_api.h"
#include "ip_api.h"

/*
==============================================================================

    Constant

==============================================================================
*/
#define _PARAMETER_LIST_MAX_LEN     128 /**< max length of option 55 */
#define _RELAY_AGENT_INFO_MAX_LEN   128 /**< max length of option 55 */
#define _HTYPE_ETHERNET             1

/*
==============================================================================

    Macro

==============================================================================
*/
#define STATISTICS_INC(_f_)         ++( g_statistics._f_ )
#define STATISTICS_DEC(_f_)         --( g_statistics._f_ );

/* bit array macro's for VLAN mode */
#define _VLAN_BF_SIZE               VTSS_BF_SIZE( VTSS_VIDS )
#define _VLAN_BF_GET(vid)           VTSS_BF_GET( g_vlan_bit, vid )
#define _VLAN_BF_SET(vid, val)      VTSS_BF_SET( g_vlan_bit, vid, val )
#define _VLAN_BF_CLR_ALL()          VTSS_BF_CLR( g_vlan_bit, VTSS_VIDS )

/*
==============================================================================

    Type Definition

==============================================================================
*/

/*
==============================================================================

    Static Variable

==============================================================================
*/
static BOOL                         g_enable;   /**< global mode */
static dhcp_server_statistics_t     g_statistics; /**< statistics database */
static u8                           g_vlan_bit[ _VLAN_BF_SIZE ]; /**< VLAN mode database */
static int                          s_sock;

/* use u32 to make sure the 4-byte alignment */
static u32                          _send_message_buf[ DHCP_SERVER_MESSAGE_MAX_LEN / 4 ]; /**< message buffer for sending */
static u8                           *g_message_buf = (u8 *)_send_message_buf;

/*
==============================================================================

    Compare Function

==============================================================================
*/
/**
 *  \brief
 *      compare IP address.
 */
static i32 _u32_cmp(
    IN u32      a,
    IN u32      b
)
{
    if ( a > b ) {
        return VTSS_AVL_TREE_CMP_RESULT_A_LARGER;
    } else if ( a < b ) {
        return VTSS_AVL_TREE_CMP_RESULT_A_SMALLER;
    }

    /* all equal */
    return VTSS_AVL_TREE_CMP_RESULT_A_B_SAME;
}

/**
 *  \brief
 *      compare name string.
 */
static i32 _str_cmp(
    IN char     *a,
    IN char     *b
)
{
    int     r;

    if ( a == NULL && b == NULL ) {
        return VTSS_AVL_TREE_CMP_RESULT_A_B_SAME;
    }

    if ( a == NULL ) {
        return VTSS_AVL_TREE_CMP_RESULT_A_SMALLER;
    }

    if ( b == NULL ) {
        return VTSS_AVL_TREE_CMP_RESULT_A_LARGER;
    }

    r = strcmp(a, b);
    if ( r < 0 ) {
        return VTSS_AVL_TREE_CMP_RESULT_A_SMALLER;
    } else if ( r == 0 ) {
        return VTSS_AVL_TREE_CMP_RESULT_A_B_SAME;
    } else { // r > 0
        return VTSS_AVL_TREE_CMP_RESULT_A_LARGER;
    }
}

/**
 *  \brief
 *      compare MAC address.
 */
static i32 _mac_cmp(
    IN mesa_mac_t   *a,
    IN mesa_mac_t   *b
)
{
    int     r;

    r = memcmp(a->addr, b->addr, DHCP_SERVER_MAC_LEN);
    if ( r > 0 ) {
        return VTSS_AVL_TREE_CMP_RESULT_A_LARGER;
    } else if ( r < 0 ) {
        return VTSS_AVL_TREE_CMP_RESULT_A_SMALLER;
    }

    /* all equal */
    return VTSS_AVL_TREE_CMP_RESULT_A_B_SAME;
}

/**
 *  \brief
 *      compare client identifier.
 */
static i32 _client_identifier_cmp(
    IN  dhcp_server_client_identifier_t     *a,
    IN  dhcp_server_client_identifier_t     *b
)
{
    int     r;

    /* compare type */
    r = _u32_cmp(a->type, b->type);
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    switch ( a->type ) {
    case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE:
        break;

    case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NAME:
        /* compare NAME */
        r = _str_cmp(a->u.name, b->u.name);
        break;

    case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_MAC:
        /* compare MAC */
        r = _mac_cmp( &(a->u.mac), &(b->u.mac) );
        break;
    }

    return r;
}

/**
 *  \brief
 *      index: low_ip, high_ip.
 */
static i32 _excluded_cmp_func(
    IN  void    *data_a,
    IN  void    *data_b
)
{
    dhcp_server_excluded_ip_t   *a;
    dhcp_server_excluded_ip_t   *b;
    i32                         r;

    a = (dhcp_server_excluded_ip_t *)data_a;
    b = (dhcp_server_excluded_ip_t *)data_b;

    r = _u32_cmp(a->low_ip, b->low_ip);
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    r = _u32_cmp(a->high_ip, b->high_ip);
    return r;
}

/**
 *  \brief
 *      index: pool_name.
 */
static i32 _pool_name_cmp_func(
    IN  void    *data_a,
    IN  void    *data_b
)
{
    dhcp_server_pool_t      *a;
    dhcp_server_pool_t      *b;
    i32                     r;

    a = (dhcp_server_pool_t *)data_a;
    b = (dhcp_server_pool_t *)data_b;

    i32 a_len = strlen(a->pool_name);
    i32 b_len = strlen(b->pool_name);

    if (a_len != b_len) {
        return a_len < b_len ? -1 : 1;
    }

    r = _str_cmp(a->pool_name, b->pool_name);
    return r;
}

/**
 *  \brief
 *      index: subnet_mask, ip & subnet_mask.
 */
static i32 _pool_ip_cmp_func(
    IN  void    *data_a,
    IN  void    *data_b
)
{
    dhcp_server_pool_t      *a;
    dhcp_server_pool_t      *b;
    i32                     r;

    a = (dhcp_server_pool_t *)data_a;
    b = (dhcp_server_pool_t *)data_b;

    r = _u32_cmp(a->subnet_mask, b->subnet_mask);
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    r = _u32_cmp(a->ip & a->subnet_mask, b->ip & b->subnet_mask);
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    r = _u32_cmp(a->type, b->type);
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    /* if both pools are type of network, then they are the same */
    if ( a->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK ) {
        return r;
    }

    /* if both pools are type of host, then compare ip */
    r = _u32_cmp(a->ip, b->ip);

    return r;
}

/**
 *  \brief
 *      index: client identifier, subnet_mask, ip & subnet_mask
 */
static i32 _pool_id_cmp_func(
    IN  void    *data_a,
    IN  void    *data_b
)
{
    dhcp_server_pool_t      *a;
    dhcp_server_pool_t      *b;
    i32                     r;

    a = (dhcp_server_pool_t *)data_a;
    b = (dhcp_server_pool_t *)data_b;

    /* compare client identifier */
    r = _client_identifier_cmp( &(a->client_identifier), &(b->client_identifier) );
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    /* compare subnet mask, longest match first */
    r = _u32_cmp(a->subnet_mask, b->subnet_mask);
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    /* compare subnet */
    r = _u32_cmp(a->ip & a->subnet_mask, b->ip & b->subnet_mask);
    return r;
}

/**
 *  \brief
 *      index: chaddr, subnet_mask, ip & subnet_mask
 */
static i32 _pool_chaddr_cmp_func(
    IN  void    *data_a,
    IN  void    *data_b
)
{
    dhcp_server_pool_t      *a;
    dhcp_server_pool_t      *b;
    i32                     r;

    a = (dhcp_server_pool_t *)data_a;
    b = (dhcp_server_pool_t *)data_b;

    /* compare chaddr */
    r = _mac_cmp( &(a->client_haddr), &(b->client_haddr) );
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    /* compare subnet mask, longest match first */
    r = _u32_cmp(a->subnet_mask, b->subnet_mask);
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    /* compare subnet */
    r = _u32_cmp(a->ip & a->subnet_mask, b->ip & b->subnet_mask);
    return r;
}

/**
 *  \brief
 *      index: ip.
 */
static i32 _binding_ip_cmp_func(
    IN  void    *data_a,
    IN  void    *data_b
)
{
    dhcp_server_binding_t   *a;
    dhcp_server_binding_t   *b;
    i32                     r;

    a = (dhcp_server_binding_t *)data_a;
    b = (dhcp_server_binding_t *)data_b;

    r = _u32_cmp(a->ip, b->ip);
    return r;
}

/**
 *  \brief
 *      index: identifier, ip & subnet_mask
 */
static i32 _binding_id_cmp_func(
    IN  void    *data_a,
    IN  void    *data_b
)
{
    dhcp_server_binding_t   *a;
    dhcp_server_binding_t   *b;
    i32                     r;

    a = (dhcp_server_binding_t *)data_a;
    b = (dhcp_server_binding_t *)data_b;

    /* compare client identifier */
    r = _client_identifier_cmp( &(a->identifier), &(b->identifier) );
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    /* compare IP & subnet mask */
    r = _u32_cmp(a->ip & a->subnet_mask, b->ip & b->subnet_mask);
    return r;
}

/**
 *  \brief
 *      index: chaddr, ip & subnet_mask
 */
static i32 _binding_chaddr_cmp_func(
    IN  void    *data_a,
    IN  void    *data_b
)
{
    dhcp_server_binding_t   *a;
    dhcp_server_binding_t   *b;
    i32                     r;

    a = (dhcp_server_binding_t *)data_a;
    b = (dhcp_server_binding_t *)data_b;

    /* compare chaddr */
    r = _mac_cmp( &(a->chaddr), &(b->chaddr) );
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    /* compare IP & subnet mask */
    r = _u32_cmp(a->ip & a->subnet_mask, b->ip & b->subnet_mask);
    return r;
}

/**
 *  \brief
 *      index: pool_name, ip.
 */
static i32 _binding_name_ip_cmp_func(
    IN  void    *data_a,
    IN  void    *data_b
)
{
    dhcp_server_binding_t   *a;
    dhcp_server_binding_t   *b;
    i32                     r;

    a = (dhcp_server_binding_t *)data_a;
    b = (dhcp_server_binding_t *)data_b;

    /* compare pool_name */
    r = _str_cmp(a->pool->pool_name, b->pool->pool_name);
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    /* compare IP */
    r = _u32_cmp(a->ip, b->ip);
    return r;
}

/**
 *  \brief
 *      index: expire_time, ip
 */
static i32 _binding_time_ip_cmp_func(
    IN  void    *data_a,
    IN  void    *data_b
)
{
    dhcp_server_binding_t   *a;
    dhcp_server_binding_t   *b;
    i32                     r;

    a = (dhcp_server_binding_t *)data_a;
    b = (dhcp_server_binding_t *)data_b;

    /* compare expire_time */
    r = _u32_cmp(a->expire_time, b->expire_time);
    if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
        return r;
    }

    /* compare IP */
    r = _u32_cmp(a->ip, b->ip);
    return r;
}

/**
 *  \brief
 *      index: ip.
 */
static i32 _decline_ip_cmp_func(
    IN  void    *data_a,
    IN  void    *data_b
)
{
    mesa_ipv4_t     *a;
    mesa_ipv4_t     *b;
    i32             r;

    a = (mesa_ipv4_t *)data_a;
    b = (mesa_ipv4_t *)data_b;

    r = _u32_cmp(*a, *b);
    return r;
}

/*
==============================================================================

    AVL tree and Free list

==============================================================================
*/
/*
    Excluded IP list
*/
// Free list
VTSS_FREE_LIST(g_excluded_ip_flist, dhcp_server_excluded_ip_t, DHCP_SERVER_EXCLUDED_MAX_CNT)
// AVL tree, index : low_ip, high_ip
VTSS_AVL_TREE(g_excluded_ip_avlt, "EXCLUDED_IP", 0, _excluded_cmp_func, DHCP_SERVER_EXCLUDED_MAX_CNT)

/*
    Pool list
*/
// Free list
VTSS_FREE_LIST(g_pool_flist, dhcp_server_pool_t, DHCP_SERVER_POOL_MAX_CNT)
// AVL tree for both of network and host
// index: pool_name
VTSS_AVL_TREE(g_pool_name_avlt, "POOL_NAME", 0, _pool_name_cmp_func, DHCP_SERVER_POOL_MAX_CNT)
// AVL tree for network
// index: subnet_mask, ip & subnet_mask
VTSS_AVL_TREE(g_pool_ip_avlt, "POOL_IP", 0, _pool_ip_cmp_func, DHCP_SERVER_POOL_MAX_CNT)
// AVL tree for host
// index: client identifier, subnet_mask, ip & subnet_mask
VTSS_AVL_TREE(g_pool_id_avlt, "POOL_ID", 0, _pool_id_cmp_func, DHCP_SERVER_POOL_MAX_CNT)
// AVL tree for host
// index: chaddr, subnet_mask, ip & subnet_mask
VTSS_AVL_TREE(g_pool_chaddr_avlt, "POOL_CHADDR", 0, _pool_chaddr_cmp_func, DHCP_SERVER_POOL_MAX_CNT)

/*
    Binding list

    when the binding is retrieved from free list, it will be added into ip, id,
    chaddr and name avlts. If it is in use, then it is in lease avlt. Otherwise,
    if it is expired, then it is in expired avlt.
*/
// Free List
VTSS_FREE_LIST(g_binding_flist, dhcp_server_binding_t, DHCP_SERVER_BINDING_MAX_CNT)
// AVL tree, index: ip
VTSS_AVL_TREE(g_binding_ip_avlt, "BINDING_IP", 0, _binding_ip_cmp_func, DHCP_SERVER_BINDING_MAX_CNT)
// AVL tree, index: identifier, ip & subnet_mask
VTSS_AVL_TREE(g_binding_id_avlt, "BINDING_ID", 0, _binding_id_cmp_func, DHCP_SERVER_BINDING_MAX_CNT)
// AVL tree, index: chaddr, ip & subnet_mask
VTSS_AVL_TREE(g_binding_chaddr_avlt, "BINDING_CHADDR", 0, _binding_chaddr_cmp_func, DHCP_SERVER_BINDING_MAX_CNT)
// AVL tree, index: pool_name, ip
// pool_name is not unique, so the second key, ip, is needed.
VTSS_AVL_TREE(g_binding_name_avlt, "BINDING_NAME", 0, _binding_name_ip_cmp_func, DHCP_SERVER_BINDING_MAX_CNT)

// AVL tree, index: expire_time, ip
// expire_time is not unique, so the second key, ip, is needed.
VTSS_AVL_TREE(g_binding_lease_avlt, "BINDING_LEASE", 0, _binding_time_ip_cmp_func, DHCP_SERVER_BINDING_MAX_CNT)
// AVL tree, index: ip
VTSS_AVL_TREE(g_binding_expired_avlt, "BINDING_EXPIRED", 0, _binding_ip_cmp_func, DHCP_SERVER_BINDING_MAX_CNT)

/*
    Decline IP list
*/
// Free List
VTSS_FREE_LIST(g_decline_flist, mesa_ipv4_t, DHCP_SERVER_BINDING_MAX_CNT)
// AVL tree, index: ip
VTSS_AVL_TREE(g_decline_ip_avlt, "DECLINE_IP", 0, _decline_ip_cmp_func, DHCP_SERVER_BINDING_MAX_CNT)

/*
==============================================================================

    Static Function

==============================================================================
*/
/**
 *  \brief
 *      check if the MAC address is not empty.
 */
static BOOL _not_empty_mac(
    IN  mesa_mac_t  *mac
)
{
    u8 empty_mac[DHCP_SERVER_MAC_LEN] = {0, 0, 0, 0, 0, 0};

    if ( memcmp(mac, empty_mac, DHCP_SERVER_MAC_LEN) ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 *  \brief
 *      check if ip is in the range of excluded.
 */
static BOOL _ip_in_excluded(
    IN dhcp_server_excluded_ip_t    *excluded,
    IN mesa_ipv4_t                  ip
)
{
    if ( ip >= excluded->low_ip && ip <= excluded->high_ip ) {
        return TRUE;
    }
    return FALSE;
}

/**
 *  \brief
 *      check if ip is excluded.
 */
static BOOL _ip_in_all_excluded(
    IN  mesa_ipv4_t                 ip,
    OUT dhcp_server_excluded_ip_t   **excluded_ip
)
{
    dhcp_server_excluded_ip_t     excluded;
    dhcp_server_excluded_ip_t     *ep;

    memset(&excluded, 0, sizeof(excluded));
    ep = &excluded;
    while ( vtss_avl_tree_get(&g_excluded_ip_avlt, (void **)&ep, VTSS_AVL_TREE_GET_NEXT) ) {
        if ( _ip_in_excluded(ep, ip) ) {
            if (excluded_ip) {
                memcpy(*excluded_ip, ep, sizeof(dhcp_server_excluded_ip_t));
            }
            return TRUE;
        }
    }
    return FALSE;
}

/**
 *  \brief
 *      check if ip is declined.
 */
static BOOL _ip_is_declined(
    IN mesa_ipv4_t      ip
)
{
    mesa_ipv4_t     *i;

    i = &ip;
    if ( vtss_avl_tree_get(&g_decline_ip_avlt, (void **)&i, VTSS_AVL_TREE_GET) ) {
        return TRUE;
    }
    return FALSE;
}

/**
 *  \brief
 *      check if ip is in the address pool.
 */
static BOOL _ip_in_pool(
    IN dhcp_server_pool_t   *pool,
    IN mesa_ipv4_t          ip
)
{
    if ( pool->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_NONE ) {
        /* IP and subnet_mask not configured yet */
        return FALSE;
    }

    if ( (pool->ip & pool->subnet_mask) != (ip & pool->subnet_mask) ) {
        return FALSE;
    }
    return TRUE;
}

/**
 *  \brief
 *      check if pool is in the subnet of (vlan_ip, vlan_netmask).
 */
static BOOL _pool_in_subnet(
    IN dhcp_server_pool_t   *pool,
    IN mesa_ipv4_t          vlan_ip,
    IN mesa_ipv4_t          vlan_netmask
)
{
    mesa_ipv4_t     netmask;

    if ( pool->subnet_mask > vlan_netmask ) {
        netmask = vlan_netmask;
    } else {
        netmask = pool->subnet_mask;
    }

    if ( (pool->ip & netmask) != (vlan_ip & netmask) ) {
        return FALSE;
    }
    return TRUE;
}

/**
 *  \brief
 *      check if ip is in the subnet of (vlan_ip, vlan_netmask).
 */
static BOOL _ip_in_subnet(
    IN mesa_ipv4_t      ip,
    IN mesa_ipv4_t      vlan_ip,
    IN mesa_ipv4_t      vlan_netmask
)
{
    if ( ip == vlan_ip ) {
        return FALSE;
    }

    /*
        check VLAN interface subnet
    */
    // avoid the first 0 address
    if ( (ip & (~vlan_netmask)) == 0 ) {
        return FALSE;
    }

    // avoid broadcast address
    if ( (ip | vlan_netmask) == 0xFFffFFff ) {
        return FALSE;
    }

    // in subnet
    if ( (vlan_ip & vlan_netmask) != (ip & vlan_netmask) ) {
        return FALSE;
    }
    return TRUE;
}

BOOL _find_pool_from_ip (
    IN mesa_ipv4_t      ip,
    OUT mesa_ipv4_t     *networl_ip,
    OUT mesa_ipv4_t     *network_mask
)
{
    dhcp_server_pool_t      pool;
    dhcp_server_pool_t      *p;
    memset(&pool, 0, sizeof(pool));
    pool.ip = 0xffFFffFF;
    pool.subnet_mask = 0xffFFffFF;
    p = &pool;
    while (vtss_avl_tree_get(&g_pool_ip_avlt, (void **)&p, VTSS_AVL_TREE_GET_PREV)) {
        if (p->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST) {
            T_D("VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST");
            continue;
        }
        T_D("Checking pool @ %s: IP/net = 0x%08x / 0x%08x", p->pool_name, p->ip, p->subnet_mask);
        if (_ip_in_subnet(ip, p->ip, p->subnet_mask)) {
            T_D("Found pool @ %s: IP/net = 0x%08x / 0x%08x", p->pool_name, p->ip, p->subnet_mask);
            *networl_ip = p->ip;
            *network_mask = p->subnet_mask;
            return TRUE;
        }
    }
    return FALSE;
}


/**
 *  \brief
 *      get old binding according to the received DHCP message
 *
 *  \param
 *      message [IN] : DHCP message.
 *      binding [OUT]: encap identifier and chaddr
 *
 *  \return
 *      n/a
 */
static BOOL _binding_id_encap(
    IN  dhcp_server_message_t     *message,
    OUT dhcp_server_binding_t     *binding
)
{
    u8      identifier[VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_NAME_LEN + 4];
    u32     length;

    memset(&(binding->identifier), 0, sizeof(binding->identifier));
    memset(binding->chaddr.addr, 0, sizeof(binding->chaddr.addr));

    memset(identifier, 0, sizeof(identifier));

    length = VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_NAME_LEN + 4;
    if ( vtss_dhcp_server_message_option_get(message, DHCP_SERVER_MESSAGE_OPTION_CODE_CLIENT_ID, identifier, &length) ) {
        switch ( identifier[0] ) {
        case DHCP_SERVER_MESSAGE_CLIENT_IDENTIFIER_TYPE_NAME:
            binding->identifier.type = VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NAME;
            strcpy(binding->identifier.u.name, (char *)(identifier + 1));
            break;

        case DHCP_SERVER_MESSAGE_CLIENT_IDENTIFIER_TYPE_MAC:
            binding->identifier.type = VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_MAC;
            memcpy(&(binding->identifier.u.mac), &(identifier[1]), sizeof(binding->identifier.u.mac));
            break;

        default:
            T_D("Client ID type %u not supported\n", identifier[0]);
            break;
        }
    }

    // then, check chaddr
    if ( message->htype != _HTYPE_ETHERNET || message->hlen != DHCP_SERVER_MAC_LEN ) {
        T_D("hardware type %02x not supported\n", message->htype);
        return FALSE;
    }

    // get chaddr
    memcpy(binding->chaddr.addr, message->chaddr, DHCP_SERVER_MAC_LEN);

    // pack client id
    if ( binding->identifier.type == VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
        binding->identifier.type = VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_MAC;
        memcpy(&(binding->identifier.u.mac), message->chaddr, sizeof(binding->identifier.u.mac));
    }

    return TRUE;
}

/**
 *  \brief
 *      get binding type according to pool
 *
 *  \param
 *      pool    [IN]: pool
 *
 *  \return
 *      vtss_appl_dhcp_server_binding_type_t
 */
static vtss_appl_dhcp_server_binding_type_t _binding_type_get(
    IN  dhcp_server_pool_t  *pool
)
{
    if ( pool->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST ) {
        return VTSS_APPL_DHCP_SERVER_BINDING_TYPE_MANUAL;
    }
    return VTSS_APPL_DHCP_SERVER_BINDING_TYPE_AUTOMATIC;
}

/**
 *  \brief
 *      increase statistics for binding type
 *
 *  \param
 *      type [IN]: binding type
 *
 *  \return
 *      n/a
 */
static void _binding_statistic_inc(
    IN  dhcp_server_binding_t   *binding
)
{
    switch ( binding->type ) {
    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_NONE:
    default:
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_AUTOMATIC:
        STATISTICS_INC( automatic_binding_cnt );
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_MANUAL:
        STATISTICS_INC( manual_binding_cnt );
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_EXPIRED:
        STATISTICS_INC( expired_binding_cnt );
        break;
    }

    switch ( binding->state ) {
    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_NONE:
    default:
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_ALLOCATED:
        STATISTICS_INC( allocated_state_cnt );
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_COMMITTED:
        STATISTICS_INC( committed_state_cnt );
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_EXPIRED:
        STATISTICS_INC( expired_state_cnt );
        break;
    }
}

/**
 *  \brief
 *      decrease statistics for binding type
 *
 *  \param
 *      type [IN]: binding type
 *
 *  \return
 *      n/a
 */
static void _binding_statistic_dec(
    IN  dhcp_server_binding_t   *binding
)
{
    switch ( binding->type ) {
    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_NONE:
    default:
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_AUTOMATIC:
        STATISTICS_DEC( automatic_binding_cnt );
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_MANUAL:
        STATISTICS_DEC( manual_binding_cnt );
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_EXPIRED:
        STATISTICS_DEC( expired_binding_cnt );
        break;
    }

    switch ( binding->state ) {
    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_NONE:
    default:
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_ALLOCATED:
        STATISTICS_DEC( allocated_state_cnt );
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_COMMITTED:
        STATISTICS_DEC( committed_state_cnt );
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_EXPIRED:
        STATISTICS_DEC( expired_state_cnt );
        break;
    }
}

/**
 *  \brief
 *      get a new binding from free list
 *
 *  \param
 *      message [IN]: DHCP message.
 *      vlan_ip [IN]: IP address of the interface from that DHCP message comes
 *
 *  \return
 *      *    : successful.
 *      NULL : failed
 */
static dhcp_server_binding_t *_binding_new(
    IN  dhcp_server_pool_t      *pool,
    IN  dhcp_server_message_t   *message,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask,
    IN  mesa_ipv4_t             yiaddr
)
{
    dhcp_server_binding_t   *binding;
    BOOL                    b_id_add;
    BOOL                    b_chaddr_add;

    binding = (dhcp_server_binding_t *)vtss_free_list_malloc( &g_binding_flist );
    if ( binding == NULL ) {
        T_D("no free memory in g_binding_flist\n");
        return NULL;
    }

    binding->ip            = yiaddr;
    binding->subnet_mask   = vlan_netmask;
    binding->state         = VTSS_APPL_DHCP_SERVER_BINDING_STATE_ALLOCATED;
    binding->type          = _binding_type_get(pool);
    binding->server_id     = vlan_ip;
    binding->pool          = pool;
    binding->lease         = DHCP_SERVER_ALLOCATION_EXPIRE_TIME;
    binding->time_to_start = dhcp_server_platform_current_time_get();
    binding->expire_time   = binding->time_to_start + binding->lease;

    if ( _binding_id_encap(message, binding) == FALSE ) {
        return NULL;
    }

    if ( vtss_avl_tree_add(&g_binding_ip_avlt, (void *)binding) == FALSE ) {
        vtss_free_list_free( &g_binding_flist, binding );
        T_D("Full in g_binding_ip_avlt\n");
        return NULL;
    }

    b_id_add     = FALSE;
    b_chaddr_add = FALSE;

    if ( binding->identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
        if ( vtss_avl_tree_add(&g_binding_id_avlt, (void *)binding) == FALSE ) {
            (void)vtss_avl_tree_delete(&g_binding_ip_avlt, (void **)&binding);
            vtss_free_list_free( &g_binding_flist, binding );
            T_D("Full in g_binding_id_avlt\n");
            return NULL;
        }
        b_id_add = TRUE;
    } else if ( _not_empty_mac(&(binding->chaddr)) ) {
        if ( vtss_avl_tree_add(&g_binding_chaddr_avlt, (void *)binding) == FALSE ) {
            (void)vtss_avl_tree_delete(&g_binding_ip_avlt, (void **)&binding);
            vtss_free_list_free( &g_binding_flist, binding );
            T_D("Full in g_binding_chaddr_avlt\n");
            return NULL;
        }
        b_chaddr_add = TRUE;
    }

    if ( vtss_avl_tree_add(&g_binding_name_avlt, (void *)binding) == FALSE ) {
        (void)vtss_avl_tree_delete(&g_binding_ip_avlt, (void **)&binding);
        if ( b_id_add ) {
            (void)vtss_avl_tree_delete(&g_binding_id_avlt, (void **)&binding);
        }
        if ( b_chaddr_add ) {
            (void)vtss_avl_tree_delete(&g_binding_chaddr_avlt, (void **)&binding);
        }
        vtss_free_list_free( &g_binding_flist, binding );
        T_D("Full in g_binding_name_avlt\n");
        return NULL;
    }

    // if with lease, add into lease tree
    // if not lease then it is automatic allocation with permanent IP address
    if ( vtss_avl_tree_add(&g_binding_lease_avlt, (void *)binding) == FALSE ) {
        (void)vtss_avl_tree_delete(&g_binding_ip_avlt, (void **)&binding);
        if ( b_id_add ) {
            (void)vtss_avl_tree_delete(&g_binding_id_avlt, (void **)&binding);
        }
        if ( b_chaddr_add ) {
            (void)vtss_avl_tree_delete(&g_binding_chaddr_avlt, (void **)&binding);
        }
        (void)vtss_avl_tree_delete(&g_binding_name_avlt, (void **)&binding);
        vtss_free_list_free( &g_binding_flist, binding );
        T_D("Full in g_binding_lease_avlt\n");
        return NULL;
    }

    _binding_statistic_inc( binding );

    ++( pool->alloc_cnt );
    return binding;
}

/**
 *  \brief
 *      remove binding from all avl trees and put it back to free list.
 *
 *  \param
 *      binding  [IN]: binding to be removed.
 *
 *  \return
 *      n/a.
 */
static void _binding_remove(
    IN  dhcp_server_binding_t     *binding
)
{
    switch ( binding->state ) {
    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_COMMITTED:
    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_ALLOCATED:
        --( binding->pool->alloc_cnt );
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_EXPIRED:
    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_NONE:
    default:
        break;
    }

    _binding_statistic_dec( binding );

    (void)vtss_avl_tree_delete(&g_binding_ip_avlt,      (void **)&binding);
    (void)vtss_avl_tree_delete(&g_binding_id_avlt,      (void **)&binding);
    (void)vtss_avl_tree_delete(&g_binding_chaddr_avlt,  (void **)&binding);
    (void)vtss_avl_tree_delete(&g_binding_name_avlt,    (void **)&binding);
    (void)vtss_avl_tree_delete(&g_binding_expired_avlt, (void **)&binding);

    if (vtss_avl_tree_delete(&g_binding_lease_avlt, (void **)&binding)) {
        T_D("delete %08x from lease tree\n", binding->ip);
    }

    vtss_free_list_free(&g_binding_flist, binding);
}

/**
 *  \brief
 *      move binding to expired state.
 *
 *  \param
 *      binding [INOUT]: binding.
 *
 *  \return
 *      n/a.
 */
static void _binding_expired_move(
    INOUT  dhcp_server_binding_t     *binding
)
{
    switch ( binding->state ) {
    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_COMMITTED:
    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_ALLOCATED:
        --( binding->pool->alloc_cnt );
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_EXPIRED:
    case VTSS_APPL_DHCP_SERVER_BINDING_STATE_NONE:
    default:
        break;
    }

    _binding_statistic_dec( binding );

    // remove from lease
    if (vtss_avl_tree_delete(&g_binding_lease_avlt, (void **)&binding)) {
        T_D("delete %08x from lease tree\n", binding->ip);
    }
    (void)vtss_avl_tree_delete(&g_binding_expired_avlt, (void **)&binding);

    // update state
    binding->type        = VTSS_APPL_DHCP_SERVER_BINDING_TYPE_EXPIRED;
    binding->state       = VTSS_APPL_DHCP_SERVER_BINDING_STATE_EXPIRED;
    binding->expire_time = dhcp_server_platform_current_time_get();

    // add into expired
    if ( vtss_avl_tree_add(&g_binding_expired_avlt, (void *)binding) == FALSE ) {
        T_D("Full in g_binding_expired_avlt\n");
    }

    _binding_statistic_inc( binding );
}

/**
 *  \brief
 *      Move all lease bindings to expired.
 *
 *  \param
 *      n/a.
 *
 *  \return
 *      n/a.
 */
static void _binding_expired_move_all(
    void
)
{
    dhcp_server_binding_t     binding;
    dhcp_server_binding_t     *bp;

    memset(&binding, 0, sizeof(binding));
    bp = &binding;
    while ( vtss_avl_tree_get(&g_binding_lease_avlt, (void **)&bp, VTSS_AVL_TREE_GET_NEXT) ) {
        _binding_expired_move( bp );
    }
}

/**
 *  \brief
 *      move binding to allocated state.
 *
 *  \param
 *      binding [INOUT]: binding.
 *
 *  \return
 *      n/a.
 */
static void _binding_allocated_move(
    INOUT   dhcp_server_binding_t     *binding
)
{
    u32                     old_state;

    // remove from lease and expired
    if (vtss_avl_tree_delete(&g_binding_lease_avlt, (void **)&binding)) {
        T_D("delete %08x from lease tree\n", binding->ip);
    }
    (void)vtss_avl_tree_delete(&g_binding_expired_avlt, (void **)&binding);

    _binding_statistic_dec( binding );

    // get old state
    old_state = binding->state;

    // update state
    binding->state         = VTSS_APPL_DHCP_SERVER_BINDING_STATE_ALLOCATED;
    binding->type          = _binding_type_get(binding->pool);
    binding->lease         = DHCP_SERVER_ALLOCATION_EXPIRE_TIME;
    binding->time_to_start = dhcp_server_platform_current_time_get();
    binding->expire_time   = binding->time_to_start + binding->lease;

    _binding_statistic_inc( binding );

    // add into lease tree
    if ( vtss_avl_tree_add(&g_binding_lease_avlt, (void *)binding) == FALSE ) {
        T_D("Full in g_binding_lease_avlt\n");
    }

    if ( old_state == VTSS_APPL_DHCP_SERVER_BINDING_STATE_EXPIRED ) {
        ++( binding->pool->alloc_cnt );
    }
}

/**
 *  \brief
 *      move binding to committed state.
 *
 *  \param
 *      binding [INOUT]: binding.
 *
 *  \return
 *      n/a.
 */
static void _binding_committed_move(
    INOUT   dhcp_server_binding_t     *binding
)
{
    u32                     old_state;

    // remove from lease and expired
    if (vtss_avl_tree_delete(&g_binding_lease_avlt, (void **)&binding)) {
        T_D("delete %08x from lease tree\n", binding->ip);
    }
    (void)vtss_avl_tree_delete(&g_binding_expired_avlt, (void **)&binding);

    _binding_statistic_dec( binding );

    // get old state
    old_state = binding->state;

    // update state
    binding->state         = VTSS_APPL_DHCP_SERVER_BINDING_STATE_COMMITTED;
    binding->type          = _binding_type_get(binding->pool);
    binding->lease         = binding->pool->lease;
    binding->time_to_start = dhcp_server_platform_current_time_get();
    binding->expire_time   = binding->time_to_start + binding->lease;

    _binding_statistic_inc( binding );

    // add into lease
    if ( binding->lease ) {
        if ( vtss_avl_tree_add(&g_binding_lease_avlt, (void *)binding) == FALSE ) {
            T_D("Full in g_binding_lease_avlt\n");
        }
    }

    if ( old_state == VTSS_APPL_DHCP_SERVER_BINDING_STATE_EXPIRED ) {
        ++( binding->pool->alloc_cnt );
    }
}

/**
 *  \brief
 *      move old binding to allocated state.
 */
static BOOL _old_binding_allocated(
    IN    dhcp_server_pool_t      *pool,
    IN    dhcp_server_message_t   *message,
    IN    mesa_ipv4_t             vlan_ip,
    IN    mesa_ipv4_t             vlan_netmask,
    INOUT dhcp_server_binding_t   *binding
)
{
    u32     old_state;
    BOOL    b_id_add;
    BOOL    b_chaddr_add;

    /* remove from lease and expired */
    if (vtss_avl_tree_delete(&g_binding_lease_avlt, (void **)&binding)) {
        T_D("delete %08x from lease tree\n", binding->ip);
    }
    (void)vtss_avl_tree_delete(&g_binding_expired_avlt, (void **)&binding);

    /* remove from id or chaddr tree */
    if ( binding->identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
        (void)vtss_avl_tree_delete(&g_binding_id_avlt, (void **)&binding);
    } else if ( _not_empty_mac(&(binding->chaddr)) ) {
        (void)vtss_avl_tree_delete(&g_binding_chaddr_avlt, (void **)&binding);
    }

    /* decrease binding statistics */
    _binding_statistic_dec( binding );

    /* get new submask */
    binding->subnet_mask = vlan_netmask;

    /* get new ID */
    if ( _binding_id_encap(message, binding) == FALSE ) {
        return FALSE;
    }

    b_id_add     = FALSE;
    b_chaddr_add = FALSE;

    if ( binding->identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
        if ( vtss_avl_tree_add(&g_binding_id_avlt, (void *)binding) == FALSE ) {
            vtss_free_list_free( &g_binding_flist, binding );
            T_D("Full in g_binding_id_avlt\n");
            return FALSE;
        }
        b_id_add = TRUE;
    } else if ( _not_empty_mac(&(binding->chaddr)) ) {
        if ( vtss_avl_tree_add(&g_binding_chaddr_avlt, (void *)binding) == FALSE ) {
            vtss_free_list_free( &g_binding_flist, binding );
            T_D("Full in g_binding_chaddr_avlt\n");
            return FALSE;
        }
        b_chaddr_add = TRUE;
    }

    /* update state */
    old_state = binding->state;

    // ip is reused, so not updated
    binding->state         = VTSS_APPL_DHCP_SERVER_BINDING_STATE_ALLOCATED;
    binding->type          = _binding_type_get(pool);
    binding->server_id     = vlan_ip;
    binding->pool          = pool;
    binding->lease         = DHCP_SERVER_ALLOCATION_EXPIRE_TIME;
    binding->time_to_start = dhcp_server_platform_current_time_get();
    binding->expire_time   = binding->time_to_start + binding->lease;

    /* add into lease tree */
    if ( vtss_avl_tree_add(&g_binding_lease_avlt, (void *)binding) == FALSE ) {
        if ( b_id_add ) {
            (void)vtss_avl_tree_delete(&g_binding_id_avlt, (void **)&binding);
        }
        if ( b_chaddr_add ) {
            (void)vtss_avl_tree_delete(&g_binding_chaddr_avlt, (void **)&binding);
        }
        vtss_free_list_free(&g_binding_flist, binding);
        T_D("Full in g_binding_lease_avlt\n");
        return FALSE;
    }

    /* increase binding statistics */
    _binding_statistic_inc( binding );

    if ( old_state == VTSS_APPL_DHCP_SERVER_BINDING_STATE_EXPIRED ) {
        ++( pool->alloc_cnt );
    }

    return TRUE;
}

static BOOL _ip_is_manual(
    IN  mesa_ipv4_t     ip
)
{
    dhcp_server_pool_t    pool;
    dhcp_server_pool_t    *p;

    memset(&pool, 0, sizeof(pool));
    p = &pool;
    while ( vtss_avl_tree_get(&g_pool_name_avlt, (void **)&p, VTSS_AVL_TREE_GET_NEXT) ) {
        if ( p->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST && p->ip == ip ) {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 *  \brief
 *      check if ip is not used.
 */
static BOOL _ip_is_free(
    IN  mesa_ipv4_t             ip,
    IN  mesa_ipv4_t             netmask,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask,
    OUT dhcp_server_binding_t   **expired
)
{
    dhcp_server_binding_t     binding;
    dhcp_server_binding_t     *bp;

    /*
        check pool subnet
    */
    // avoid the first 0 address
    if ( (ip & (~netmask)) == 0 ) {
        return FALSE;
    }

    // avoid broadcast address
    if ( (ip | netmask) == 0xFFffFFff ) {
        return FALSE;
    }

    /* ip in the VLAN subnet? */
    if ( _ip_in_subnet(ip, vlan_ip, vlan_netmask) == FALSE ) {
        return FALSE;
    }

    /* ip is manual IP? */
    if ( _ip_is_manual(ip) ) {
        return FALSE;
    }

    /* ip is excluded? */
    if ( _ip_in_all_excluded(ip, NULL) ) {
        return FALSE;
    }

    /* ip is declined? */
    if ( _ip_is_declined(ip) ) {
        return FALSE;
    }

    // check binding
    memset(&binding, 0, sizeof(binding));
    binding.ip = ip;
    bp = &binding;
    if ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&bp, VTSS_AVL_TREE_GET) ) {
        switch ( bp->state ) {
        case VTSS_APPL_DHCP_SERVER_BINDING_STATE_COMMITTED:
        case VTSS_APPL_DHCP_SERVER_BINDING_STATE_ALLOCATED:
            /* in use */
            return FALSE;

        case VTSS_APPL_DHCP_SERVER_BINDING_STATE_EXPIRED:
        case VTSS_APPL_DHCP_SERVER_BINDING_STATE_NONE:
            /* free */
            *expired = bp;
            break;
        }
    }

    return TRUE;
}

/**
 *  \brief
 *      delete declined IP from avl tree and put it back to free list
 *
 *  \param
 *      declined_ip [IN]: declined IP to be deleted
 *
 *  \return
 *      n/a.
 */
static void _declined_ip_delete(
    IN mesa_ipv4_t      *declined_ip
)
{
    mesa_ipv4_t     *ipp;

    ipp = declined_ip;
    if ( vtss_avl_tree_delete(&g_decline_ip_avlt, (void **)&ipp) ) {
        vtss_free_list_free(&g_decline_flist, ipp);
        STATISTICS_DEC( declined_cnt );
    }
}

/**
 *  \brief
 *      get a new free IP address in IP pool
 *
 *  \param
 *      pool         [IN]: IP pool.
 *      vlan_ip      [IN]: ip address of VLAN interface
 *      vlan_netmask [IN]: subnet mask of VLAN interface
 *      binding     [OUT]: expired binding of the free IP
 *
 *  \return
 *      * : free IP.
 *      0 : no free IP.
 */
static mesa_ipv4_t _free_ip_get(
    IN  dhcp_server_pool_t      *pool,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask,
    OUT dhcp_server_binding_t   **expired
)
{
    u32                         i;
    mesa_ipv4_t                 ip;
    u32                         count;
    mesa_ipv4_t                 *ipp;
    dhcp_server_binding_t       *ep;
    dhcp_server_binding_t       *longest_expired;
    dhcp_server_excluded_ip_t   *excluded_ptr;
    mesa_ipv4_t                 vlan_high_ip;
    dhcp_server_excluded_ip_t   excluded;

    longest_expired = NULL;

    memset(&excluded, 0, sizeof(dhcp_server_excluded_ip_t));
    excluded_ptr = &excluded;

    /* 1. get free IP first */
    if ( pool->subnet_mask > vlan_netmask ) {
        ip = pool->ip & pool->subnet_mask;
        count = ( ~(pool->subnet_mask) ) + 1;
    } else {
        ip = vlan_ip & vlan_netmask;
        count = ( ~vlan_netmask ) + 1;
    }

    /*
        It will take a long long time of check IP one by one when all A-class
        subnet is excluded. Therefore, we need to calculate excluded IP range
        and just skip that range, but not check IP one by one.
    */
    vlan_high_ip = vlan_ip | (~vlan_netmask);
    for ( i = 0; i < count; ++i ) {
        /* check excluded IP range */
        if ( _ip_in_all_excluded(ip + i, &excluded_ptr) ) {
            T_D("check excluded: high 0X%x, low 0X%x, vlan_high 0X%x\n", excluded_ptr->high_ip, excluded_ptr->low_ip, vlan_high_ip);
            // check high IP is in or over VLAN subnet
            if (excluded_ptr->high_ip >= vlan_high_ip) {
                // excluded high IP is over VLAN subnet
                // it means no free IP
                break;
            }

            // excluded high IP is in VLAN subnet
            // then check next IP not in the range of excluded IP
            i = excluded_ptr->high_ip - ip;
            continue;
        }

        /* check others for free */
        ep = NULL;
        if ( _ip_is_free(ip + i, pool->subnet_mask, vlan_ip, vlan_netmask, &ep) ) {
            if ( ep ) {
                if ( longest_expired ) {
                    if ( ep->expire_time < longest_expired->expire_time ) {
                        longest_expired = ep;
                    }
                } else {
                    longest_expired = ep;
                }
            } else {
                return (ip + i);
            }
        }
    }

    /* 2. no free IP, then use longest expired IP */
    if ( longest_expired ) {
        *expired = longest_expired;
        return longest_expired->ip;
    }

    /* 3. no expired IP, then use declined IP */
    ip  = 0;
    ipp = &ip;

    while ( vtss_avl_tree_get(&g_decline_ip_avlt, (void **)&ipp, VTSS_AVL_TREE_GET_NEXT) ) {
        if ( _ip_in_pool(pool, *ipp) && !_ip_in_all_excluded(*ipp, NULL) ) {
            ip = *ipp;
            _declined_ip_delete( ipp );
            return ip;
        }
    }

    /* 4. all are in used, no any IP is available */
    return 0;
}

/**
 *  \brief
 *      translate the value to RFC
 *
 */
static u8 _netbios_node_type_get(
    IN  u32     netbios_node_type
)
{
    switch ( netbios_node_type ) {
    case VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_NONE:
    default:
        return 0;

    case VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_B:
        return 1;

    case VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_P:
        return 2;

    case VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_M:
        return 4;

    case VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_H:
        return 8;
    }
}

#define _server_option_pack(_op_, _f_) \
    if ( pool->_f_[0] == 0 ) { \
        break; \
    } \
    option[option_len++] = _op_; \
    len_index = option_len++; \
    for ( j = 0; j < VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT; j++ ) { \
        if ( pool->_f_[j] ) { \
            n = htonl( pool->_f_[j] ); \
            memcpy(&(option[option_len]), &n, 4); \
            option_len += 4; \
        } else { \
            break; \
        } \
    } \
    option[len_index] = j * 4;

#define _name_option_pack(_op_, _f_) \
    n = strlen( pool->_f_ ) + 1; \
    if ( n == 1 ) { \
        break; \
    } \
    option[option_len++] = _op_; \
    option[option_len++] = (u8)n; \
    s = (char *)&( option[option_len] ); \
    strcpy(s, pool->_f_); \
    option_len += n;

#define _u32_option_pack(_op_, _v_) \
    option[option_len++] = _op_; \
    option[option_len++] = 4; \
    n = htonl( _v_ ); \
    memcpy(&(option[option_len]), &n, 4); \
    option_len += 4;

/**
 *  \brief
 *      pack options according to the incoming message and
 *      the corresponding pool
 */
static u32  _option_pack(
    IN  dhcp_server_message_t   *message,
    IN  u8                      message_type,
    IN  dhcp_server_pool_t      *pool,
    IN  mesa_ipv4_t             vlan_ip,
    IN  BOOL                    b_lease,
    OUT u8                      *option
)
{
    u32     length;
    u8      paramter[_PARAMETER_LIST_MAX_LEN];
    u32     option_len;
    u32     i;
    u8      j;
    u32     n;
    u32     len_index;
    char    *s;
    char    class_id[VTSS_APPL_DHCP_SERVER_VENDOR_CLASS_ID_LEN + 1];
    char    relay_agent_information[_RELAY_AGENT_INFO_MAX_LEN + 1];


    option_len = 0;

    /* magic cookie */
    option[option_len++] = 0x63;
    option[option_len++] = 0x82;
    option[option_len++] = 0x53;
    option[option_len++] = 0x63;

    /* message type */
    option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_MESSAGE_TYPE;
    option[option_len++] = 1;
    option[option_len++] = message_type;

    /* option 60 */
    memset(class_id, 0, sizeof(class_id));
    length = VTSS_APPL_DHCP_SERVER_VENDOR_CLASS_ID_LEN;
    if ( vtss_dhcp_server_message_option_get(message, DHCP_SERVER_MESSAGE_OPTION_CODE_VENDOR_CLASS_ID, class_id, &length) ) {
        for ( i = 0; i < VTSS_APPL_DHCP_SERVER_VENDOR_CLASS_INFO_CNT; i++ ) {
            if ( strlen(pool->class_info[i].class_id) && strcmp(pool->class_info[i].class_id, class_id) == 0 ) {
                if ( pool->class_info[i].specific_info_len ) {
                    option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_VENDOR_SPECIFIC_INFO;
                    option[option_len++] = (u8)( pool->class_info[i].specific_info_len );
                    for ( n = 0; n < pool->class_info[i].specific_info_len; n++ ) {
                        option[option_len++] = pool->class_info[i].specific_info[n];
                    }
                }
                break;
            }
        }
    }

    /* pack option */
    // DHCP_SERVER_MESSAGE_OPTION_CODE_SERVER_ID:
    _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_SERVER_ID, vlan_ip);

    // DHCP_SERVER_MESSAGE_OPTION_CODE_SUBNET_MASK:
    _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_SUBNET_MASK, pool->subnet_mask);

    /* pack lease */
    if ( b_lease ) {
        if ( pool->lease ) {
            // DHCP_SERVER_MESSAGE_OPTION_CODE_LEASE_TIME
            _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_LEASE_TIME, pool->lease);
            // DHCP_SERVER_MESSAGE_OPTION_CODE_RENEWAL_TIME
            _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_RENEWAL_TIME, pool->lease / 2);
            // DHCP_SERVER_MESSAGE_OPTION_CODE_REBINDING_TIME
            _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_REBINDING_TIME, pool->lease * 3 / 4);
        } else {
            // DHCP_SERVER_MESSAGE_OPTION_CODE_LEASE_TIME
            _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_LEASE_TIME, 0xFFffFFff);
        }
    }

    /* parameter list */
    length = _PARAMETER_LIST_MAX_LEN;
    if ( vtss_dhcp_server_message_option_get(message, DHCP_SERVER_MESSAGE_OPTION_CODE_PARAMETER_LIST, paramter, &length) == FALSE ) {
        /* END */
        option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_END;
        return option_len;
    }

    for ( i = 0; i < length; i++ ) {
        switch ( paramter[i] ) {
#if 0
        case DHCP_SERVER_MESSAGE_OPTION_CODE_SUBNET_MASK:
            _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_SUBNET_MASK, pool->subnet_mask);
            break;
#endif
        case DHCP_SERVER_MESSAGE_OPTION_CODE_ROUTER:
            _server_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_ROUTER, default_router);
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_DNS_SERVER:
            _server_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_DNS_SERVER, dns_server);
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_HOST_NAME:
            _name_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_HOST_NAME, client_name);
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_DOMAIN_NAME:
            _name_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_DOMAIN_NAME, domain_name);
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_BROADCAST:
            if ( pool->subnet_broadcast == 0 ) {
                break;
            }
            _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_BROADCAST, pool->subnet_broadcast);
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_NIS_DOMAIN_NAME:
            _name_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_NIS_DOMAIN_NAME, nis_domain_name);
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_NIS_SERVER:
            _server_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_NIS_SERVER, nis_server);
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_NTP_SERVER:
            _server_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_NTP_SERVER, ntp_server);
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_NETBIOS_NAME_SERVER:
            _server_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_NETBIOS_NAME_SERVER, netbios_name_server);
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_NETBIOS_NODE_TYPE:
            if ( pool->netbios_node_type == VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_NONE ) {
                break;
            }
            option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_NETBIOS_NODE_TYPE;
            option[option_len++] = 1;
            option[option_len++] = _netbios_node_type_get( pool->netbios_node_type );
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_NETBIOS_SCOPE:
            _name_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_NETBIOS_SCOPE, netbios_scope);
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_MAX_MESSAGE_SIZE:
            _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_MAX_MESSAGE_SIZE, DHCP_SERVER_DEFAULT_MAX_MESSAGE_SIZE);
            break;

        case DHCP_SERVER_MESSAGE_OPTION_CODE_VENDOR_CLASS_ID:
        case DHCP_SERVER_MESSAGE_OPTION_CODE_CLIENT_ID:
        case DHCP_SERVER_MESSAGE_OPTION_CODE_REQUESTED_IP:
        case DHCP_SERVER_MESSAGE_OPTION_CODE_OPTION_OVERLOAD:
        case DHCP_SERVER_MESSAGE_OPTION_CODE_MESSAGE_TYPE:
        case DHCP_SERVER_MESSAGE_OPTION_CODE_PARAMETER_LIST:
        case DHCP_SERVER_MESSAGE_OPTION_CODE_RELAY_AGENT:
            break;

        default:
            //T_D("option %u not supported\n", paramter[i]);
            break;
        }
    }

    // option 82  For relay Relay agent.
    // RFC 3046 2.2 Server Operation. This must be the last element
    memset(relay_agent_information, 0, sizeof(relay_agent_information));
    length = _RELAY_AGENT_INFO_MAX_LEN;
    if (vtss_dhcp_server_message_option_get(message, DHCP_SERVER_MESSAGE_OPTION_CODE_RELAY_AGENT, relay_agent_information, &length) == FALSE) {
        T_D("vtss_dhcp_server_message_option_get for DHCP_SERVER_MESSAGE_OPTION_CODE_RELAY_AGENT failed");
        option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_END;
        return option_len;
    } else {
        option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_RELAY_AGENT;
        option[option_len++] = (u8)length;
        for (int n = 0; n < length; n++) {
            option[option_len++] = relay_agent_information[n];
        }
    }

    /* END */
    option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_END;

    return option_len;
}

/**
 *  \brief
 *      pack options to the default value
 */
static u32  _default_option_pack(
    IN  u8                  message_type,
    IN  dhcp_server_pool_t  *pool,
    IN  mesa_ipv4_t         vlan_ip,
    OUT u8                  *option
)
{
    u32     option_len;
    u32     n;

    option_len = 0;

    /* magic cookie */
    option[option_len++] = 0x63;
    option[option_len++] = 0x82;
    option[option_len++] = 0x53;
    option[option_len++] = 0x63;

    /* message type */
    option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_MESSAGE_TYPE;
    option[option_len++] = 1;
    option[option_len++] = message_type;

    /* subnet mask */
    _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_SUBNET_MASK, pool->subnet_mask);

    /* server identifier */
    option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_SERVER_ID;
    option[option_len++] = 4;
    n = htonl( vlan_ip );
    memcpy(&(option[option_len]), &n, 4);
    option_len += 4;

    /* lease */
    _u32_option_pack( DHCP_SERVER_MESSAGE_OPTION_CODE_LEASE_TIME, pool->lease );

    /* renew */
    _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_LEASE_TIME, pool->lease / 2);

    /* rebind */
    _u32_option_pack(DHCP_SERVER_MESSAGE_OPTION_CODE_LEASE_TIME, pool->lease * 3 / 4);

    /* END */
    option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_END;

    return option_len;
}

/**
 *  \brief
 *      move the binding of ip to expired state
 */
static void _ip_binding_expired(
    IN  mesa_ipv4_t         ip
)
{
    dhcp_server_binding_t     binding;
    dhcp_server_binding_t     *bp;

    memset(&binding, 0, sizeof(binding));
    binding.ip = ip;
    bp = &binding;
    if ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&bp, VTSS_AVL_TREE_GET) ) {
        _binding_expired_move( bp );
    }
}

/**
 *  \brief
 *      find binding according to the incoming message.
 *      we can find one iff:
 *          binding's network address matches VLAN's network address
 *          AND
 *            message contains the Client Identifier DHCP option containing a NAME or MAC && we have that ID in an AVL tree
 *            OR we have the message chaddr in an AVL tree
 */
static dhcp_server_binding_t *_binding_get_by_id(
    IN  dhcp_server_message_t   *message,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask
)
{
    dhcp_server_binding_t     binding;
    dhcp_server_binding_t     *bp;

    memset(&binding, 0, sizeof(binding));

    binding.ip          = vlan_ip;
    binding.subnet_mask = vlan_netmask;

    if ( _binding_id_encap(message, &binding) == FALSE ) {
        return NULL;
    }

    // find binding avlt
    bp = &binding;
    if ( bp->identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
        if ( vtss_avl_tree_get(&g_binding_id_avlt, (void **)&bp, VTSS_AVL_TREE_GET) ) {
            return bp;
        }
    } else if ( _not_empty_mac(&(bp->chaddr)) ) {
        if ( vtss_avl_tree_get(&g_binding_chaddr_avlt, (void **)&bp, VTSS_AVL_TREE_GET) ) {
            return bp;
        }
    }
    return NULL;
}

/**
 *  \brief
 *      get binding by DHCP message
 */
static dhcp_server_binding_t *_binding_get_by_message(
    IN  dhcp_server_message_t   *message,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask
)
{
    dhcp_server_binding_t   binding;
    dhcp_server_binding_t   *bp;
    mesa_ipv4_t             ip;
    u32                     length;

    memset( &binding, 0, sizeof(binding) );

    /* get by client IP */
    if ( message->ciaddr ) {
        binding.ip = ntohl( message->ciaddr );
        bp = &binding;
        if ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&bp, VTSS_AVL_TREE_GET) ) {
            return bp;
        }
    }

    /* get by requested IP */
    length = 4;
    if ( vtss_dhcp_server_message_option_get(message, DHCP_SERVER_MESSAGE_OPTION_CODE_REQUESTED_IP, &ip, &length) ) {
        binding.ip = ip;
        bp = &binding;
        if ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&bp, VTSS_AVL_TREE_GET) ) {
            return bp;
        }
    }

    /* get by ID */
    binding.ip          = vlan_ip;
    binding.subnet_mask = vlan_netmask;

    if ( _binding_id_encap(message, &binding) == FALSE ) {
        return NULL;
    }

    /* get binding by chaddr */
    bp = &binding;
    if ( bp->identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
        if ( vtss_avl_tree_get(&g_binding_id_avlt, (void **)&bp, VTSS_AVL_TREE_GET) ) {
            return bp;
        }
    } else if ( _not_empty_mac(&(bp->chaddr)) ) {
        if ( vtss_avl_tree_get(&g_binding_chaddr_avlt, (void **)&bp, VTSS_AVL_TREE_GET) ) {
            return bp;
        }
    }

    return NULL;
}

/**
 *  \brief
 *      get old binding according to the received DHCP message
 *
 *  \param
 *      message      [IN]: DHCP message.
 *      vlan_ip      [IN]: IP address of the interface from that DHCP message comes
 *      vlan_netmask [IN]: netmask of the interface from that DHCP message comes
 *
 *  \return
 *      *    : successful.
 *      NULL : failed
 */
static dhcp_server_binding_t *_binding_get_old(
    IN  dhcp_server_message_t   *message,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask
)
{
    dhcp_server_binding_t     *bp;
    dhcp_server_pool_t        *p;

    // Find binding based on Client ID/chaddr AND binding is in our VLAN subnet
    bp = _binding_get_by_id( message, vlan_ip, vlan_netmask );
    if ( bp == NULL ) {
        return NULL;
    }

    p = bp->pool;
    if ( _pool_in_subnet(p, vlan_ip, vlan_netmask) && _ip_in_subnet(bp->ip, vlan_ip, vlan_netmask) ) {
        // The binding's pool's network fits inside our VLAN interface, and the binding's IP also fits the VLAN interface.
        // (This should always be the case; changing the network of a pool clears its bindings)
        T_D("Found binding with matching client ID/chaddr in VLAN subnet");
        return bp;
    }

    T_D("Found binding with IP 0x%08x, but in wrong pool %s. Clearing binding.", bp->ip, p->pool_name);
    /* pool is not correct, return NULL to get new */
    _binding_remove( bp );
    return NULL;
}

/**
 *  \brief
 *      create a new binding according to the received DHCP_DISCOVER message
 *
 *  \param
 *      message [IN]: DHCP DISCOVER message.
 *      vlan_ip [IN]: IP address of the interface from that DHCP message comes
 *      ifindex [IN]: Interface that message arrived on.
 *
 *  \return
 *      *    : successful.
 *      NULL : failed
 */
static dhcp_server_binding_t *_binding_get_new(
    IN  dhcp_server_message_t   *message,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask,
    IN  vtss_ifindex_t          ifindex
)
{
    u8                      identifier[VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_NAME_LEN + 4];
    u32                     length;
    dhcp_server_pool_t      pool;
    dhcp_server_pool_t      *p;
    dhcp_server_pool_t      *pool_expired;
    dhcp_server_binding_t   *binding;
    dhcp_server_binding_t   *binding_expired;
    BOOL                    b_get;
    mesa_ipv4_t             requested_ip;
    mesa_ipv4_t             ip;
    i32                     r;

    if (message->op != DHCP_SERVER_MESSAGE_TYPE_DISCOVER) {
        T_D("_binding_get_new but type not DISCOVER?! It's %u", message->op);
    }

    /*
        find host
    */
    memset(&pool, 0, sizeof(pool));
    memset(identifier, 0, sizeof(identifier));

    ip = 0;
    length = VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_NAME_LEN + 4;
    if ( vtss_dhcp_server_message_option_get(message, DHCP_SERVER_MESSAGE_OPTION_CODE_CLIENT_ID, identifier, &length) ) {
        p = NULL;
        switch ( identifier[0] ) {
        case DHCP_SERVER_MESSAGE_CLIENT_IDENTIFIER_TYPE_NAME:
            pool.client_identifier.type = VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NAME;
            strcpy(pool.client_identifier.u.name, (char *)(identifier + 1));
            p = &pool;
            T_D("Found match on name");
            break;

        case DHCP_SERVER_MESSAGE_CLIENT_IDENTIFIER_TYPE_MAC:
            pool.client_identifier.type = VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_MAC;
            memcpy(&(pool.client_identifier.u.mac), &(identifier[1]), sizeof(pool.client_identifier.u.mac));
            p = &pool;
            T_D("Found match on mac address");
            break;

        default:
            T_D("Client ID type %u not supported\n", identifier[0]);
            break;
        }

        if ( p ) {
            p->subnet_mask = 0xFFffFFff;
            while ( vtss_avl_tree_get(&g_pool_id_avlt, (void **)&p, VTSS_AVL_TREE_GET_PREV) ) {
                /* compare client identifier */
                r = _client_identifier_cmp( &(p->client_identifier), &(pool.client_identifier) );
                if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
                    break;
                }
                if ( _pool_in_subnet(p, vlan_ip, vlan_netmask) && _ip_in_subnet(p->ip, vlan_ip, vlan_netmask) &&
                     ( ! _ip_in_all_excluded(p->ip, NULL) ) && ( ! _ip_is_declined(p->ip) ) ) {
                    binding = _binding_new( p, message, message->giaddr ? ntohl(message->giaddr) : vlan_ip, vlan_netmask, p->ip );
                    if ( binding == NULL ) {
                        T_D("fail to get a new binding\n");
                    } else {
                        T_D("Found binding in pool while looking for client_identifier vlan_ip 0x%08x vlan_netmask %u  pool_ip 0x%08x",
                            message->giaddr ? ntohl(message->giaddr) : vlan_ip, vlan_netmask, p->ip);
                    }
                    return binding;
                } else {
                    /* the host is not valid, find free IP in pool */
                    T_D("The host is not valid, find free IP in pool after client_identifier lookup");
                }
            }
        }
    } else {
        // no client identifier in message, check chaddr
        if ( message->htype == _HTYPE_ETHERNET && message->hlen == DHCP_SERVER_MAC_LEN ) {
            memcpy(pool.client_haddr.addr, message->chaddr, DHCP_SERVER_MAC_LEN);
            p = &pool;
            p->subnet_mask = 0xFFffFFff;
            while ( vtss_avl_tree_get(&g_pool_chaddr_avlt, (void **)&p, VTSS_AVL_TREE_GET_PREV) ) {
                /* compare chaddr */
                r = _mac_cmp( &(p->client_haddr), &(pool.client_haddr) );
                if ( r != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
                    break;
                }
                if ( _pool_in_subnet(p, vlan_ip, vlan_netmask) && _ip_in_subnet(p->ip, vlan_ip, vlan_netmask) &&
                     ( ! _ip_in_all_excluded(p->ip, NULL) ) && ( ! _ip_is_declined(p->ip) ) ) {
                    binding = _binding_new( p, message, message->giaddr ? ntohl(message->giaddr) : vlan_ip, vlan_netmask, p->ip );
                    if ( binding == NULL ) {
                        T_D("fail to new a binding\n");
                    } else {
                        T_D("Found binding in pool while looking for chaddr vlan_ip 0x%08x vlan_netmask %u pool_ip 0x%08x",
                            message->giaddr ? ntohl(message->giaddr) : vlan_ip, vlan_netmask, p->ip);
                    }
                    return binding;
                } else {
                    /* the host is not valid, find free IP in pool */
                    T_D("The host is not valid, find free IP in pool after chaddr lookup");
                }
            }
        } else {
            T_D("invalid chaddr, htype %u, hlen %u\n", message->htype, message->hlen);
        }
    }

    /*
        find network
    */

    /* Reserved address processing only takes place for "automatic" pools, not "manual".
     * This restriction is enforced via configuration-time checks.
     *
     * The overall strategy is to ensure that an OFFER received on an interface for which
     * there is a reserved entry is checked for validity, and, if it passes, allocate the
     * reserved address from the free pool (or expired entry pool, depending); and from then
     * on all processing is unchanged. (We do check for consistency in REQUEST processing,
     * too.)
     *
     * We have two main cases:
     *
     * 1. DISCOVER with a request for a specific IP address
     * 2. DISCOVER without a such request
     *
     * For 1:
     * =====
     * Let {Resv.IP} and {Resv.IfIndex} denote the sets of reserved IPs and interfaces,
     * respectively, and Req.IP/Req.IfIndex denote the IP and InIndex of the request message.
     * Also, let Resv[IP].IfIndex denote the IfIndex of the reserved entry with the given IP,
     * and Resv[IfIndex].IP the reverse mapping.
     *
     *   1. Req.IP IN {Resv.IP} && Req.IfIndex == Resv[IP].IfIndex => Allocate reserved IP
     *      from pool.
     *
     * That was easy: Request for matching (reserved) IP on matching interface leads to
     * allocation. Otherwise:
     *
     *   2. Req.IfIndex IN {Resv.IfIndex} => ignore requested IP, allocate the reserved IP
     *      for the interface, Resv[IfIndex].IP. In other words, mismatched IP is overruled
     *      by reserved IP for the interface.
     *
     *   3. Pool.reserved_only? Yes, ignore message since we don't have a reserved address for
     *      that interface.
     *
     * Otherwise, we have a pool that allows non-reserved addresses and the ingress interface
     * isn't one occupied by a reserved address.
     *
     *   4. Req.IP in {Resv.IP} => Ignore request IP. We don't hand out reserved IPs on mismatching
     *      interfaces.
     *
     *   5. Allocate non-reserved IP from pool.
     *
     * For 2:
     * =====
     * Reusing the above notation, we get:
     *
     *   1. Req.IfIndex in {Resv.IfIndex} => Allocate reserved IP from pool, Resv[IfIndex].IP.
     *
     *   2. Else, Pool.reserved_only? Yes => ignore message since we don't have a reserved address
     *      for that interface.
     *
     *   3. Allocate non-reserved IP from pool.
     *
     * Regarding allocation of reserved IPs: Once we know which IP we want, we check if it's
     * available in the free pool or in the expired pool. If it is, we use it; otherwise we fail
     * the allocation attempt -- the reserved IP is then (obviously) in use.
     */


    // get client request IP from message
    length = 4;
    requested_ip = 0;
    (void)vtss_dhcp_server_message_option_get( message, DHCP_SERVER_MESSAGE_OPTION_CODE_REQUESTED_IP, &requested_ip, &length );

    // go to avlt
    pool_expired    = NULL;
    binding_expired = NULL;
    b_get = FALSE;

    memset(&pool, 0, sizeof(pool));
    pool.ip          = 0xffFFffFF;
    pool.subnet_mask = 0xffFFffFF;
    p                = &pool;

    /*
        hint: go ip avlt from last by PREV, so the netmask will be longest match
    */
    while ( vtss_avl_tree_get(&g_pool_ip_avlt, (void **)&p, VTSS_AVL_TREE_GET_PREV) ) {
        if ( p->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST ) {
            continue;
        }

        T_D("Checking network pool @ %p: IP/net = 0x%08x / 0x%08x", p, p->ip, p->subnet_mask);
        if (! _pool_in_subnet(p, vlan_ip, vlan_netmask)) {
            continue;
        }

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
        // Simple linear search for reserved entry. Yes, optimization potential here.
        u32 resv_ifindex_idx = VTSS_APPL_DHCP_SERVER_RESERVED_CNT;
        u32 i;

        for (i = 0; i < VTSS_APPL_DHCP_SERVER_RESERVED_CNT; i++) {
            if (p->reserved[i].ifindex == ifindex) {
                resv_ifindex_idx = i;
                T_D("Found reserved entry by ifindex: idx %d, ifindex %d. Using it for requested IP", i, vtss_ifindex_cast_to_u32(ifindex));
                requested_ip = p->reserved[i].address;
                break;
            }
        }
        if (i == VTSS_APPL_DHCP_SERVER_RESERVED_CNT && p->reserved_only) {
            T_D("No reserved entry found and pool is reserved-only: Skipping");
            continue;
        }
#endif

        // if client requests specific IP, check this IP first
        if ( requested_ip ) {
            T_D("Processing requested IP 0x%08x", requested_ip);
            if (! _ip_in_pool(p, requested_ip) ) {
                T_D("Requested IP does not belong to pool");
                // (For a reserved entry, this would be a bug: inconsistent configuration state)
            } else {
                T_D("Requested IP belongs to pool");
                binding = NULL;
#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
                if ( resv_ifindex_idx != VTSS_APPL_DHCP_SERVER_RESERVED_CNT && _ip_is_declined(requested_ip)) {
                    T_D("DISCOVER came in on interface with reserved entry, but the reserved address is declined. Moving Requested IP to Free");
                    _declined_ip_delete(&requested_ip);
                }
#endif
                if (! _ip_is_free(requested_ip, p->subnet_mask, vlan_ip, vlan_netmask, &binding)) {
                    T_D("Requested IP is NOT free");
                } else {
                    T_D("Requested IP is free (available)");
                    if ( binding ) {
                        T_D("Found an expired binding");
                        pool_expired    = p;
                        binding_expired = binding;
                    } else {
                        T_D("No expired binding, must alloc from free pool");
                        b_get = TRUE;
                        ip    = requested_ip;
                    }
                    break; // out of while: We can continue processing with the requested IP
                }
            }
        }

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
        if (resv_ifindex_idx != VTSS_APPL_DHCP_SERVER_RESERVED_CNT) {
            T_D("DISCOVER came in on interface with reserved entry, but the reserved address is in use");
            return NULL;
        }
#endif

        T_D("Not a reserved entry, and requested IP (if any) isn't available: Find a new, free IP");
        binding = NULL;
        ip = _free_ip_get( p, vlan_ip, vlan_netmask, &binding );
        if ( ip ) {
            if ( binding ) {
                if ( pool_expired == NULL ) {
                    pool_expired    = p;
                    binding_expired = binding;
                    // FIXME: Shouldn't there be a 'break' here?!
                }
            } else {
                b_get = TRUE;
                break;
            }
        }

    } // while

    binding = NULL;
    if ( b_get ) {
        T_D("About to alloc new binding");
        binding = _binding_new( p, message, message->giaddr ? ntohl(message->giaddr) : vlan_ip, vlan_netmask, ip );
        if ( binding == NULL ) {
            T_D("fail to new a binding\n");
            return NULL;
        }
    } else if ( pool_expired ) {
        T_D("About to re-use old binding from expired");
        binding = binding_expired;
        if ( _old_binding_allocated( pool_expired, message, message->giaddr ? ntohl(message->giaddr) : vlan_ip, vlan_netmask, binding_expired ) == FALSE ) {
            T_D("fail to move expired binding to allocated\n");
            return NULL;
        }
    }

    T_D("OK, got a binding!");
    return binding;
}

/**
 *  \brief
 *      send DHCP NAK message
 */
static void _nak_send(
    IN  dhcp_server_message_t   *message,
    IN  mesa_vid_t              vid,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             sip
)
{
    dhcp_server_message_t   *send_message;
    u8                      *option;
    u32                     option_len;
    u8                      dmac[DHCP_SERVER_MAC_LEN];
    u32                     n;

    memset(g_message_buf, 0, DHCP_SERVER_MESSAGE_MAX_LEN);
    send_message = (dhcp_server_message_t *)g_message_buf;

    /* encap message */
    send_message->op     = 2;
    send_message->htype  = _HTYPE_ETHERNET;
    send_message->hlen   = DHCP_SERVER_MAC_LEN;
    send_message->xid    = message->xid;
    send_message->flags  = DHCP_SERVER_MESSAGE_FLAG_BROADCAST;
    memcpy(send_message->chaddr, message->chaddr, DHCP_SERVER_MESSAGE_CHADDR_LEN);

    /* option */
    option = g_message_buf + DHCP_SERVER_OPTION_OFFSET;
    option_len = 0;

    /* magic cookie */
    option[option_len++] = 0x63;
    option[option_len++] = 0x82;
    option[option_len++] = 0x53;
    option[option_len++] = 0x63;

    /* message type */
    option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_MESSAGE_TYPE;
    option[option_len++] = 1;
    option[option_len++] = DHCP_SERVER_MESSAGE_TYPE_NAK;

    /* Server ID */
    option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_SERVER_ID;
    option[option_len++] = 4;
    n = htonl( vlan_ip );
    memcpy(&(option[option_len]), &n, 4);
    option_len += 4;

    /* END */
    option[option_len++] = DHCP_SERVER_MESSAGE_OPTION_CODE_END;

#if 0 /* CP, 05/28/2013 14:44, CC-11017, already in 2-byte alignment */
    /* PAD */
    if ( option_len % 2 ) {
        option_len++;
    }
#endif

    /* client IP and MAC */
    /*
        RFC-2131 p23, always broadcast DHCPNAK when giaddr = 0
    */
    memset( dmac, 0xFF, DHCP_SERVER_MAC_LEN );

    /* send message */
    if ( dhcp_server_platform_packet_tx(send_message, option_len, vid, sip, dmac, INADDR_BROADCAST) ) {
        STATISTICS_INC( nak_cnt );
    } else {
        T_D("fail to send NAK message\n");
    }
}

/**
 *  \brief
 *      check if the binding is for the incoming message
 */
static BOOL _binding_id_check(
    IN  dhcp_server_message_t     *message,
    IN  dhcp_server_binding_t     *binding
)
{
    u8      identifier[VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_NAME_LEN + 4];
    u32     length;

    // check client identifier first
    memset(identifier, 0, sizeof(identifier));

    length = VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_NAME_LEN + 4;
    if ( vtss_dhcp_server_message_option_get(message, DHCP_SERVER_MESSAGE_OPTION_CODE_CLIENT_ID, identifier, &length) ) {
        // get cllient identifer
        switch ( identifier[0] ) {
        case DHCP_SERVER_MESSAGE_CLIENT_IDENTIFIER_TYPE_NAME:
            if ( binding->identifier.type == VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NAME ) {
                if ( strcmp(binding->identifier.u.name, (char *)(identifier + 1)) == 0 ) {
                    return TRUE;
                }
            }
            break;

        case DHCP_SERVER_MESSAGE_CLIENT_IDENTIFIER_TYPE_MAC:
            if ( binding->identifier.type == VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_MAC ) {
                if ( memcmp(binding->identifier.u.mac.addr, &(identifier[1]), sizeof(binding->identifier.u.mac.addr)) == 0 ) {
                    return TRUE;
                }
            }
            break;

        default:
            T_D("Client ID type %u not supported\n", identifier[0]);
            break;
        }
        return FALSE;
    }

    // then, check chaddr
    if ( message->htype != _HTYPE_ETHERNET || message->hlen != DHCP_SERVER_MAC_LEN ) {
        T_D("hardware type %02x not supported\n", message->htype);
        return FALSE;
    }

    if ( memcmp(binding->chaddr.addr, message->chaddr, DHCP_SERVER_MAC_LEN) ) {
        return FALSE;
    }
    return TRUE;
}

/**
 *  \brief
 *      get destination MAC and IP to send DHCP message
 *      This follows RFC-2131 p23.
 *      The returned dmac parameter is either a copy of the mac address held in
 *      dhcp message field chaddr or the broadcast value FFFFFFFF.
 */
static void _destination_get(
    IN  dhcp_server_message_t   *message,
    IN  dhcp_server_binding_t   *binding,
    OUT mesa_ipv4_t             *dip,
    OUT u8                      *dmac
)
{
    if ( message->giaddr ) {
        *dip = ntohl(message->giaddr);

        T_D("message->giaddr");
    } else if ( message->ciaddr ) {
        /*
            if ciaddr != 0 then unicast ciaddr
        */
        T_D("message->ciaddr");
        *dip = ntohl( message->ciaddr );
    } else {
        /*
            if ciaddr == 0
            then if broadcast bit is set
                    then broadcast
                    else unicast yiaddr
        */
        if ( ntohs(message->flags) & DHCP_SERVER_MESSAGE_FLAG_BROADCAST ) {
            *dip = INADDR_BROADCAST;
            memset( dmac, 0xFF, DHCP_SERVER_MAC_LEN );
        } else {
            *dip = binding->ip;
        }
    }
}

/**
 *  \brief
 *      send DHCP ACK message
 */
static void _ack_send(
    IN  dhcp_server_message_t   *message,
    IN  mesa_vid_t              vid,
    IN  mesa_ipv4_t             vlan_ip,
    IN  dhcp_server_binding_t   *binding,
    IN  BOOL                    b_lease,
    IN  u8                      *received_smac,
    IN  mesa_ipv4_t             sip
)
{
    dhcp_server_message_t   *send_message;
    u32                     option_len;
    mesa_ipv4_t             dip;
    u8                      dmac[DHCP_SERVER_MAC_LEN];

    /* pack message to send */
    memset(g_message_buf, 0, DHCP_SERVER_MESSAGE_MAX_LEN);
    send_message = (dhcp_server_message_t *)g_message_buf;
    T_D("_ack_send, received_smac=%02x:%02x:%02x:%02x:%02x:%02x",
        received_smac[0], received_smac[1], received_smac[2],
        received_smac[3], received_smac[4], received_smac[5]);

    /* options */
    option_len = _option_pack( message, DHCP_SERVER_MESSAGE_TYPE_ACK, binding->pool, sip, b_lease, (u8 *) & (send_message->options) );
    if ( option_len == 0 ) {
        if ( b_lease ) {
            option_len = _default_option_pack(DHCP_SERVER_MESSAGE_TYPE_ACK, binding->pool, sip, (u8 *) & (send_message->options) );
        } else {
            T_D("fail to pack options\n");
            return;
        }
    }

    /* encap message */
    send_message->op     = 2;
    send_message->htype  = _HTYPE_ETHERNET;
    send_message->hlen   = DHCP_SERVER_MAC_LEN;
    send_message->hops   = 0;
    send_message->xid    = message->xid;
    send_message->secs   = 0;
    send_message->ciaddr = 0;
    send_message->yiaddr = b_lease ? htonl( binding->ip ) : 0;
    send_message->siaddr = 0;
    send_message->flags  = 0;
    send_message->giaddr = message->giaddr;  // If message from relay agent, copy gateway address
    memcpy(send_message->chaddr, message->chaddr, DHCP_SERVER_MESSAGE_CHADDR_LEN);
    // sname, not supported
    // file,  not supported

    /* get destination IP and MAC */
    memcpy(dmac, received_smac, DHCP_SERVER_MAC_LEN);
    _destination_get( message, binding, &dip, dmac );

    /* send message */
    if ( dhcp_server_platform_packet_tx(send_message, option_len, vid, sip, dmac, dip) ) {
        STATISTICS_INC( ack_cnt );
    } else {
        if ( b_lease ) {
            _ip_binding_expired( dip );
        }
        T_D("fail to send ACK message\n");
    }
}

/**
 *  \brief
 *      process DHCPREQUEST message for OFFER
 *
 *  \return
 *      n/a.
 */
static void _request_by_old_binding(
    IN  dhcp_server_message_t   *message,
    IN  mesa_vid_t              vid,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask,
    IN  mesa_ipv4_t             client_ip,
    IN  vtss_ifindex_t          ifindex,
    IN  u8                      *received_smac,
    IN  mesa_ipv4_t             sip
)
{
    dhcp_server_binding_t     binding;
    dhcp_server_binding_t     *bp;


    // check client identifier first
    memset(&binding, 0, sizeof(binding));
    binding.ip = client_ip;
    bp = &binding;
    if ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&bp, VTSS_AVL_TREE_GET) == FALSE ) {
        _nak_send(message, vid, vlan_ip, sip);
        return;
    }

    if (vlan_ip == 0) {
        // This is a renew message. According to RFC2131 section
        // 4.3.2, subsection ' DHCPREQUEST generated during RENEWING
        // state', server identifier must not be filled in. Server
        // shall trust address in ci_addr
        vlan_ip = bp->server_id;
        vlan_netmask = bp->subnet_mask;
    }

    // if the client is valid
    if ( _binding_id_check(message, bp) == FALSE ) {
        /* this binding is not for the client, it means the requested IP is used by other client */
        _nak_send(message, vid, vlan_ip, sip);
        return;
    }

    // update vid
    bp->vid = vid;

    // move to committed state
    _binding_committed_move( bp );

    // send DHCP ACK
    _ack_send( message, vid, vlan_ip, bp, TRUE, received_smac, sip);
}

/**
 *  \brief
 *      Get DHCP message type.
 *
 *  \param
 *      message [IN] : DHCP message.
 *      type    [OUT]: message type, DHCP_SERVER_MESSAGE_TYPE_XXX
 *
 *  \return
 *      TRUE  : successful.\n
 *      FALSE : failed.
 */
static BOOL _pkt_type_get(
    IN      dhcp_server_message_t   *message,
    OUT     u8                      *type
)
{
    u32     length = 1;

    if ( vtss_dhcp_server_message_option_get(message, DHCP_SERVER_MESSAGE_OPTION_CODE_MESSAGE_TYPE, type, &length) == FALSE ) {
        return FALSE;
    }
    return TRUE;
}

/**
 *  \brief
 *      send DHCP OFFER message
 */
static void _offer_send(
    IN  dhcp_server_message_t   *message,
    IN  mesa_vid_t              vid,
    IN  mesa_ipv4_t             vlan_ip,
    IN  dhcp_server_binding_t   *binding,
    IN  u8                      *received_smac,
    IN  mesa_ipv4_t             sip
)
{
    dhcp_server_message_t   *send_message;
    u32                     option_len;
    mesa_ipv4_t             yiaddr;
    mesa_ipv4_t             dip;
    u8                      dmac[DHCP_SERVER_MAC_LEN];

    // client IP
    yiaddr = binding->ip;

    /* pack message to send */
    memset(g_message_buf, 0, DHCP_SERVER_MESSAGE_MAX_LEN);
    send_message = (dhcp_server_message_t *)g_message_buf;

    T_D("_offer_send");

    /* options */
    option_len = _option_pack( message, DHCP_SERVER_MESSAGE_TYPE_OFFER, binding->pool, sip, TRUE, (u8 *) & (send_message->options) );
    if ( option_len == 0 ) {
        _ip_binding_expired( yiaddr );
        T_D("fail to pack options\n");
        return;
    }

    /* encap message */
    send_message->op     = 2;
    send_message->htype  = _HTYPE_ETHERNET;
    send_message->hlen   = DHCP_SERVER_MAC_LEN;
    send_message->hops   = 0;
    send_message->xid    = message->xid;
    send_message->secs   = 0;
    send_message->ciaddr = 0;
    send_message->yiaddr = htonl( yiaddr );
    send_message->siaddr = 0;
    send_message->flags  = 0;
    send_message->giaddr = message->giaddr;  // Because we do support Relay agents.
    memcpy(send_message->chaddr, message->chaddr, DHCP_SERVER_MESSAGE_CHADDR_LEN);
    // sname, not supported
    // file,  not supported

    /* get destination IP and MAC */
    memcpy(dmac, received_smac, DHCP_SERVER_MAC_LEN);
    _destination_get( message, binding, &dip, dmac );

    /* send message */
    if ( dhcp_server_platform_packet_tx(send_message, option_len, vid, sip, dmac, dip) == FALSE ) {
        _ip_binding_expired( yiaddr );
        T_D("fail to send OFFER message\n");
        return;
    }

    // binding VID
    binding->vid = vid;

    STATISTICS_INC( offer_cnt );
}

/**
 *  \brief
 *      process DHCP DISCOVER message
 *
 *  \param
 *      message      [IN]: DHCP DISCOVER message.
 *      vid          [IN]: VLAN ID of the message.
 *      vlan_ip      [IN]: IP address of the VLAN
 *      vlan_netmask [IN]: subnet mask of the VLAN
 *      ifindex [IN]: Interface that message arrived on.
 *
 *  \return
 *      n/a.
 */
static void _dhcp_discover(
    IN  dhcp_server_message_t   *message,
    IN  mesa_vid_t              vid,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask,
    IN  vtss_ifindex_t          ifindex,
    IN  u8                      *received_smac,
    IN  mesa_ipv4_t             sip
)
{
    dhcp_server_binding_t       *binding;

    T_D("DISCOVER: Looking up old binding");
    /* yiaddr */
    binding = _binding_get_old( message, vlan_ip, vlan_netmask );
    if ( binding ) {
        /* move binding to allocated state */
        T_D("DISCOVER - moving binding to allocated state");
        _binding_allocated_move( binding );
    } else {
        T_D("DISCOVER - get new binding");
        /* get a new binding */
        binding = _binding_get_new( message, vlan_ip, vlan_netmask, ifindex );
        if ( binding == NULL ) {
            T_D("DISCOVER - could NOT get new binding");
            return;
        }
    }

    // send OFFER
    _offer_send( message, vid, vlan_ip, binding, received_smac, sip);
}

/**
 *  \brief
 *      process DHCP REQUEST message
 *
 *  \param
 *      message      [IN]: DHCP REQUEST message.
 *      vid          [IN]: VLAN ID of the message.
 *      vlan_ip      [IN]: IP address of the VLAN
 *      vlan_netmask [IN]: subnet mask of the VLAN
 *      ifindex      [IN]: Interface that message arrived on.
 *
 *  \return
 *      n/a.
 */
static void _dhcp_request(
    IN  dhcp_server_message_t   *message,
    IN  mesa_vid_t              vid,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask,
    IN vtss_ifindex_t           ifindex,
    IN  u8                      *received_smac,
    IN  mesa_ipv4_t             sip
)
{
    u32             length;
    mesa_ipv4_t     server_id;
    mesa_ipv4_t     requested_ip;

    // get Server ID
    length    = 4;
    server_id = 0;
    if ( vtss_dhcp_server_message_option_get(message, DHCP_SERVER_MESSAGE_OPTION_CODE_SERVER_ID, &server_id, &length) ) {
        // the request is for offer, check if it is for us
        if ( server_id && sip != server_id ) {
            // clear binding by allocation timer, so do nothing here
            return;
        }
    }

    // get requested IP
    length       = 4;
    requested_ip = 0;
    if ( vtss_dhcp_server_message_option_get(message, DHCP_SERVER_MESSAGE_OPTION_CODE_REQUESTED_IP, &requested_ip, &length) == FALSE ) {
        requested_ip = ntohl( message->ciaddr );
    }

    if ( requested_ip == 0 ) {
        T_D("REQUEST: Neither Requested-IP option nor ciaddr provided");
        return;
    }

    _request_by_old_binding(message, vid, vlan_ip, vlan_netmask, requested_ip, ifindex, received_smac, sip);
}

/**
 *  \brief
 *      process DHCP DECLINE message
 *
 *  \param
 *      message [IN]: DHCP DECLINE message.
 *
 *  \return
 *      n/a.
 */
static void _dhcp_decline(
    IN  dhcp_server_message_t   *message,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask
)
{
    dhcp_server_binding_t   *bp;
    mesa_ipv4_t             ip;
    mesa_ipv4_t             *ipp;

    bp = _binding_get_by_message(message, vlan_ip, vlan_netmask);

    if ( bp == NULL ) {
        T_D("not get binding for decline\n");
        return;
    }

    ip = bp->ip;

    dhcp_server_platform_syslog("DHCP_SERVER-IP_ADDR_DECLINED: IP %u.%u.%u.%u is declined.\n",
                                (ip & 0xff000000) >> 24,
                                (ip & 0x00ff0000) >> 16,
                                (ip & 0x0000ff00) >>  8,
                                (ip & 0x000000ff) >>  0);

    /* remove binding */
    _binding_remove( bp );

    /* add decline IP */
    ipp = (mesa_ipv4_t *)vtss_free_list_malloc( &g_decline_flist );
    if ( ipp == NULL ) {
        T_D("no free memory in g_decline_flist\n");
        return;
    }

    *ipp = ip;

    if ( vtss_avl_tree_add(&g_decline_ip_avlt, (void *)ipp) == FALSE ) {
        vtss_free_list_free( &g_decline_flist, ipp );
        // T_D("g_decline_ip_avlt is full\n");
        return;
    }

    STATISTICS_INC( declined_cnt );
}

/**
 *  \brief
 *      process DHCP RELEASE message
 *
 *  \param
 *      message [IN]: DHCP RELEASE message.
 *
 *  \return
 *      n/a.
 */
static void _dhcp_release(
    IN dhcp_server_message_t    *message,
    IN  mesa_ipv4_t             vlan_ip,
    IN  mesa_ipv4_t             vlan_netmask
)
{
    dhcp_server_binding_t   *bp;

    bp = _binding_get_by_message(message, vlan_ip, vlan_netmask);

    if ( bp == NULL ) {
        T_D("not get binding for release\n");
        return;
    }

    /* move binding to expired avlt */
    _binding_expired_move( bp );
}

/**
 *  \brief
 *      process DHCP INFORM message
 *
 *  \param
 *      message      [IN]: DHCP INFORM message.
 *      vid          [IN]: VLAN ID of the message.
 *      vlan_ip      [IN]: IP address of the VLAN
 *
 *  \return
 *      n/a.
 */
static void _dhcp_inform(
    IN  dhcp_server_message_t   *message,
    IN  mesa_vid_t              vid,
    IN  mesa_ipv4_t             vlan_ip,
    IN  u8                      *received_smac,
    IN  mesa_ipv4_t             sip
)
{
    u32                     ciaddr;
    dhcp_server_binding_t   binding;
    dhcp_server_binding_t   *bp;

    /* get ciaddr */
    ciaddr = ntohl( message->ciaddr );

    /* find pool */
    // check client identifier first
    memset(&binding, 0, sizeof(binding));
    binding.ip = ciaddr;
    bp = &binding;
    if ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&bp, VTSS_AVL_TREE_GET) == FALSE ) {
        return;
    }

    // if the client is valid
    if ( _binding_id_check(message, bp) == FALSE ) {
        /* this binding is not for the client, it means the requested IP is used by other client */
        return;
    }

    // send DHCP ACK
    _ack_send( message, vid, vlan_ip, bp, FALSE, received_smac, sip);
}

/**
 *  \brief
 *      process DHCP message from DHCP client.
 *
 *  \param
 *      message [IN]: DHCP message
 *      vid     [IN]: VLAN ID of the message.
 *      ifindex [IN]: Interface that message arrived on.
 *
 *  \return
 *      TRUE  : successful.\n
 *      FALSE : failed.
 */
static BOOL _message_process(
    IN  dhcp_server_message_t   *message,
    IN  mesa_vid_t              vid,
    IN vtss_ifindex_t           ifindex,
    IN u8                       *received_smac
)
{
    mesa_ipv4_t     vlan_ip;
    mesa_ipv4_t     vlan_netmask;
    mesa_ipv4_t     network_ip;
    mesa_ipv4_t     network_mask;
    mesa_ipv4_t     src_ip;
    mesa_ipv4_t     tmp_vlan_ip;
    mesa_ipv4_t     tmp_vlan_netmask;
    u8              message_type;

    /* DHCP client message process */
    if (_pkt_type_get(message, &message_type) == FALSE) {
        T_D("fail to get message type\n");
        return FALSE;
    }

    /* check if the VLAN has IP interface. if yes, then get IP and netmask */
    if (dhcp_server_platform_vid_info_get(vid, &tmp_vlan_ip, &tmp_vlan_netmask) == FALSE) {
        T_D("dhcp_server_platform_vid_info_get failed with vid %u", vid);
        return FALSE;
    }

    if (message->giaddr) { // If it is message from a Relay Agent
        // We search the list of pools to find appropriate values for
        // netowrk_ip and network mask. This is required for managing address alloction to bindings.
        if (_find_pool_from_ip(ntohl(message->giaddr), &network_ip, &network_mask) == FALSE) {
            T_D("Could not find pool matching Relay Agent address IP = 0x%08x", ntohl(message->giaddr));
            return FALSE;
        }
    }

    if (message->giaddr) {
        T_D("Preparing IP parameters:  giaddr = 0x%08x  network_ip = 0x%08x  network_mask = 0x%08x", ntohl(message->giaddr), network_ip, network_mask);
        vlan_ip         = ntohl(message->giaddr);
        vlan_netmask    = network_mask;
    } else {
        vlan_ip         = tmp_vlan_ip;
        vlan_netmask    = tmp_vlan_netmask;
    }
    src_ip = tmp_vlan_ip;

    T_D("Process message type %u", message_type);


    switch ( message_type ) {
    case DHCP_SERVER_MESSAGE_TYPE_DISCOVER:
        STATISTICS_INC( discover_cnt );
        _dhcp_discover( message, vid, vlan_ip, vlan_netmask, ifindex, received_smac, src_ip );
        break;

    case DHCP_SERVER_MESSAGE_TYPE_REQUEST:
        STATISTICS_INC( request_cnt );
        _dhcp_request( message, vid, vlan_ip, vlan_netmask, ifindex, received_smac, src_ip );
        break;

    case DHCP_SERVER_MESSAGE_TYPE_DECLINE:
        STATISTICS_INC( decline_cnt );
        _dhcp_decline( message, vlan_ip, vlan_netmask );
        break;

    case DHCP_SERVER_MESSAGE_TYPE_RELEASE:
        STATISTICS_INC( release_cnt );
        _dhcp_release( message, vlan_ip, vlan_netmask );
        break;

    case DHCP_SERVER_MESSAGE_TYPE_INFORM:
        STATISTICS_INC( inform_cnt );
        _dhcp_inform( message, vid, vlan_ip, received_smac, src_ip );
        break;

    case DHCP_SERVER_MESSAGE_TYPE_OFFER:
    case DHCP_SERVER_MESSAGE_TYPE_ACK:
    case DHCP_SERVER_MESSAGE_TYPE_NAK:
        return FALSE;

    default:
        T_D("invalid message type %u\n", message_type);
        return FALSE;
    }

    return TRUE;
}

/* Opening socket bound to UDP port 67. */
static int dhcp_server_open_socket()
{
    struct sockaddr_in addr_in;
    int sock = 0;

    T_D("Enter");

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock <= 0) {
        T_D("socket() failed");
        return sock;
    }

    memset(&addr_in, 0, sizeof(addr_in));

    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(67);
    addr_in.sin_addr.s_addr = INADDR_ANY;

    int flag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, sizeof(flag)) < 0) {
        T_D("Could not set SO_REUSEADDR on socket");
        close(sock);
        return -1;
    }

    if (bind(sock, (struct sockaddr *)&addr_in, sizeof(addr_in)) < 0) {
        T_D("bind() failed. Closing socket.");
        close(sock);
        return -1;
    }

    T_D("Exit after opening socket.");
    return sock;

}

/**
 *  \brief
 *      Enable/Disable DHCP server.
 *
 *  \param
 *      b_enable [IN]: TRUE - enable, FALSE - disable.
 *
 *  \return
 *      TRUE  : successful.\n
 *      FALSE : failed.
 */
static BOOL _enable(
    IN  BOOL    b_enable
)
{
    if ( g_enable != b_enable ) {
        if ( b_enable ) {
#ifdef VTSS_SW_OPTION_DHCP_RELAY
            vtss_appl_dhcp_relay_param_t relay_conf;
            /* If dhcp relay is enabled on DUT, dhcp server should not be started. Both
               open a socket on UDP port 67 and should
               not run at the same time to ensure that packets go to the correct module. */
            if (vtss_appl_dhcp_relay_system_config_get(&relay_conf) != VTSS_RC_OK) {
                T_W("Could not get dhcp relay configuration.");
                return FALSE;
            }

            if (relay_conf.mode && (relay_conf.serverIpAddr != 0)) {
                T_W("Can not start DHCP server while DHCP Relay is active on device.");
                return FALSE;
            }
#endif
            g_enable = b_enable;
            dhcp_server_platform_packet_rx_register();
            s_sock = dhcp_server_open_socket();

        } else {
            g_enable = b_enable;
            dhcp_server_platform_packet_rx_deregister();
            if (s_sock > 0) {
                T_D("Closing socket");
                close(s_sock);
                s_sock = 0;
            }
        }
    }

    /* move all binding to expired */
    _binding_expired_move_all();

    return TRUE;
}

/**
 *  \brief
 *      remove binding by pool name
 *
 *  \param
 *      pool [IN]: pool->pool_name
 *
 *  \return
 *      n/a
 */
static void _binding_remove_by_pool_name(
    IN  dhcp_server_pool_t      *pool
)
{
    dhcp_server_binding_t       binding;
    dhcp_server_binding_t       *bp;

    T_D("Removing bindings by pool name for pool %s", pool->pool_name);
    memset( &binding, 0, sizeof(binding) );
    binding.pool = pool;
    bp = &binding;
    while ( vtss_avl_tree_get(&g_binding_name_avlt, (void **)&bp, VTSS_AVL_TREE_GET_NEXT) ) {
        if ( bp->pool != binding.pool ) {
            break;
        }
        _binding_remove( bp );
    }
}

/**
 *  \brief
 *      remove binding by IP address
 *
 *  \param
 *      ip [IN]: binding IP address
 *
 *  \return
 *      n/a
 */
static void _binding_remove_by_ip(
    IN  mesa_ipv4_t     ip
)
{
    dhcp_server_binding_t       binding;
    dhcp_server_binding_t       *bp;

    T_D("Removing bindings by IP 0x%08x", ip);
    memset( &binding, 0, sizeof(binding) );
    binding.ip = ip;
    bp = &binding;
    if ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&bp, VTSS_AVL_TREE_GET) ) {
        _binding_remove( bp );
    }
}

/**
 *  \brief
 *      remove binding those are the same client identifier with pool
 *      and binding IP address in the pool
 *
 *  \param
 *      pool [IN]: client identifier and subnet address
 *
 *  \return
 *      n/a
 */
static void _binding_remove_by_pool_client_id(
    IN  dhcp_server_pool_t      *pool
)
{
    dhcp_server_binding_t       binding;
    dhcp_server_binding_t       *bp;

    if ( pool->type != VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST ) {
        return;
    }

    if ( pool->client_identifier.type == VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
        return;
    }

    memset( &binding, 0, sizeof(binding) );
    memcpy( &(binding.identifier), &(pool->client_identifier), sizeof(binding.identifier) );
    bp = &binding;
    while ( vtss_avl_tree_get(&g_binding_id_avlt, (void **)&bp, VTSS_AVL_TREE_GET_NEXT) ) {
        if ( _client_identifier_cmp(&(binding.identifier), &(bp->identifier)) != VTSS_AVL_TREE_CMP_RESULT_A_B_SAME ) {
            // not same client ID, so break the loop
            break;
        }
        if ( (pool->ip & bp->subnet_mask) == (bp->ip & bp->subnet_mask) ) {
            _binding_remove( bp );
        }
    }
}

/**
 *  \brief
 *      remove binding those are the same client hardware address with pool
 *      and binding IP address in the pool
 *
 *  \param
 *      pool [IN]: hardware address and subnet address
 *
 *  \return
 *      n/a
 */
static void _binding_remove_by_pool_chaddr(
    IN  dhcp_server_pool_t      *pool
)
{
    dhcp_server_binding_t       binding;
    dhcp_server_binding_t       *bp;

    if ( pool->type != VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST ) {
        return;
    }

    if ( pool->client_identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
        // using client ID, not mac
        return;
    }

    if ( _not_empty_mac( &(pool->client_haddr) ) == FALSE ) {
        // mac is empty
        return;
    }

    memset( &binding, 0, sizeof(binding) );
    memcpy( binding.chaddr.addr, pool->client_haddr.addr, DHCP_SERVER_MAC_LEN );
    bp = &binding;
    while ( vtss_avl_tree_get(&g_binding_chaddr_avlt, (void **)&bp, VTSS_AVL_TREE_GET_NEXT) ) {
        if ( memcmp(binding.chaddr.addr, bp->chaddr.addr, DHCP_SERVER_MAC_LEN) ) {
            // not same MAC, so break the loop
            break;
        }
        if ( (pool->ip & bp->subnet_mask) == (bp->ip & bp->subnet_mask) ) {
            _binding_remove( bp );
        }
    }
}

/**
 *  \brief
 *      update bindings by old pool to new pool.
 *
 *  \param
 *      new_pool  [IN]: new DHCP pool.
 *      old_pool  [IN]: old DHCP pool.
 *
 *  \return
 *      n/a.
 */
static void _binding_update_by_pool(
    IN    dhcp_server_pool_t    *new_pool,
    INOUT dhcp_server_pool_t    *old_pool
)
{
    int                         diff;
    BOOL                        b_remove_old = FALSE;

    // Processing of reserved entries:
    //  * Reserved-only => not reserved-only: Keep bindings as-is, the pool of available addresses simply grows
    //  * Not reserved-only => reserved-only: Clear all bindings
    //  * Add or Change reserved address: Clear all bindings; it could be an already-in-use IP, or an interface
    //    with other, active, bindings
    //  * Delete reserved address: Clear binding for the reserved address only

    /*
        1. remove bindings for old pool
    */

    // new pool type is None, remove all old bindings
    if ( new_pool->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_NONE ) {
        _binding_remove_by_pool_name( old_pool );
        return;
    }

    /*
        2. remove corresponding bindings
    */

    if ( old_pool->type == new_pool->type ) {

        /* 1. remove bindings for old pool */

#ifndef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
        if ( (old_pool->subnet_mask != new_pool->subnet_mask) ||
             ((old_pool->ip & old_pool->subnet_mask) != (new_pool->ip & new_pool->subnet_mask)) ||
             ((old_pool->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST) && (old_pool->ip != new_pool->ip)) ) {
            _binding_remove_by_pool_name( old_pool );
            b_remove_old = TRUE;
        }
#else
        if ( (old_pool->subnet_mask != new_pool->subnet_mask) ||
             ((old_pool->ip & old_pool->subnet_mask) != (new_pool->ip & new_pool->subnet_mask)) ||
             ((old_pool->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST) && (old_pool->ip != new_pool->ip)) ||
             ((old_pool->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK) && new_pool->reserved_only)
           ) {
            b_remove_old = TRUE;
        } else if (old_pool->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK) {
            // Processing of reserved entries (reserved-only mode is handled above).
            // Code here depends on entries being sorted by IP, and unused entries being at the end of the array
            int idx_old = 0, idx_new = 0;
            while ( (idx_old < VTSS_APPL_DHCP_SERVER_RESERVED_CNT) &&
                    (idx_new < VTSS_APPL_DHCP_SERVER_RESERVED_CNT) &&
                    (old_pool->reserved[idx_old].ifindex != VTSS_IFINDEX_NONE) &&
                    (new_pool->reserved[idx_new].ifindex != VTSS_IFINDEX_NONE) ) {
                if (old_pool->reserved[idx_old].address == new_pool->reserved[idx_new].address) {
                    if (old_pool->reserved[idx_old].ifindex != new_pool->reserved[idx_new].ifindex) {
                        // Same address, different interface: Remove all bindings
                        b_remove_old = TRUE;
                        break;
                    }
                    // Same address, same interface: Move on
                    idx_old++;
                    idx_new++;
                } else if (old_pool->reserved[idx_old].address < new_pool->reserved[idx_new].address) {
                    // Entry has been removed: Clear binding for entry's IP
                    _binding_remove_by_ip(old_pool->reserved[idx_old].address);
                    idx_old++;
                } else {
                    // Entry has been added: Remove all bindings
                    b_remove_old = TRUE;
                    break;
                }
            }

            // If there are new entries added: Remove all bindings
            if ( (idx_new < VTSS_APPL_DHCP_SERVER_RESERVED_CNT) &&
                 (new_pool->reserved[idx_new].ifindex != VTSS_IFINDEX_NONE) ) {
                b_remove_old = TRUE;
            }

            // Remove bindings for deleted items we haven't looked at yet (unless we'll be clearing everything anyway)
            if (! b_remove_old) {
                while ( (idx_old < VTSS_APPL_DHCP_SERVER_RESERVED_CNT) &&
                        (old_pool->reserved[idx_old].ifindex != VTSS_IFINDEX_NONE) ) {
                    _binding_remove_by_ip(old_pool->reserved[idx_old].address);
                    idx_old++;
                }
            }
        }
        if (b_remove_old) {
            _binding_remove_by_pool_name( old_pool );
        }
#endif // VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES

        /* 2. remove bindings for new pool */
        if ( new_pool->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST ) {
            // ip
            if ( old_pool->ip != new_pool->ip ) {
                // remove ip to prepare for new
                _binding_remove_by_ip( new_pool->ip );

                if ( new_pool->client_identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
                    _binding_remove_by_pool_client_id( new_pool );
                } else {
                    _binding_remove_by_pool_chaddr( new_pool );
                }
            }

            // client identifier
            diff = memcmp( &(old_pool->client_identifier), &(new_pool->client_identifier), sizeof(old_pool->client_identifier) );
            if ( diff ) {
                // delete old binding
                if ( ! b_remove_old ) {
                    // remove old
                    _binding_remove_by_ip( old_pool->ip );
                }

                // delete binding to prepare for new
                if ( new_pool->client_identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
                    _binding_remove_by_pool_client_id( new_pool );
                }
            }

            // chaddr
            diff = memcmp( &(old_pool->client_haddr), &(new_pool->client_haddr), sizeof(old_pool->client_haddr) );
            if ( diff ) {
                // delete old binding
                if ( ! b_remove_old ) {
                    // remove old
                    _binding_remove_by_ip( old_pool->ip );
                }

                // delete binding of other pool
                _binding_remove_by_pool_chaddr( new_pool );
            }
        }
    } else {   // Change of pool type
        /* 1. remove bindings for old pool */
        _binding_remove_by_pool_name( old_pool );

        /* 2. remove bindings for new pool */
        if ( new_pool->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST ) {
            // remove ip to prepare for new
            _binding_remove_by_ip( new_pool->ip );

            if ( new_pool->client_identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
                _binding_remove_by_pool_client_id( new_pool );
            } else {
                _binding_remove_by_pool_chaddr( new_pool );
            }
        }
    }
}

/**
 *  \brief
 *      count the number of IP addresses in the pool
 */
static void _pool_total_cnt_get(
    OUT dhcp_server_pool_t    *pool
)
{
    switch ( pool->type ) {
    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NONE:
    default:
        pool->total_cnt = 0;
        break;

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK:
        pool->total_cnt = ( ~(pool->subnet_mask) ) + 1;
        break;

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST:
        pool->total_cnt = 1;
        break;
    }
}

/**
 *  \brief
 *      update old pool to new pool.
 *
 *  \param
 *      new_pool  [IN]: new DHCP pool.
 *      old_pool  [IN]: old DHCP pool.
 *      old_pool [OUT]: update to new DHCP pool.
 *
 *  \return
 *      TRUE  : successful
 *      FALSE : failed
 */
static BOOL _pool_update(
    IN    dhcp_server_pool_t    *new_pool,
    INOUT dhcp_server_pool_t    *old_pool
)
{
    dhcp_server_binding_t       binding;
    dhcp_server_binding_t       *bp;
    dhcp_server_pool_t          *p;

    // check if new is duplicate
    switch ( new_pool->type ) {
    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NONE:
        break;

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST:
        p = new_pool;
        if ( p->client_identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
            if ( vtss_avl_tree_get(&g_pool_id_avlt, (void **)&p, VTSS_AVL_TREE_GET) ) {
                if ( p != old_pool ) {
                    // duplicate
                    return FALSE;
                }
            }
        } else if ( _not_empty_mac(&(new_pool->client_haddr)) ) {
            if ( vtss_avl_tree_get(&g_pool_chaddr_avlt, (void **)&p, VTSS_AVL_TREE_GET) ) {
                if ( p != old_pool ) {
                    // duplicate
                    return FALSE;
                }
            }
        }

    // fall through

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK:
        p = new_pool;
        if ( vtss_avl_tree_get(&g_pool_ip_avlt, (void **)&p, VTSS_AVL_TREE_GET) ) {
            if ( p != old_pool ) {
                // duplicate
                return FALSE;
            }
        }
        break;

    default:
        return FALSE;
    }

    // remove old from avlt
    switch ( old_pool->type ) {
    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NONE:
    default:
        break;

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST:
        if ( old_pool->client_identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
            (void)vtss_avl_tree_delete(&g_pool_id_avlt, (void **)&old_pool);
        } else if ( _not_empty_mac(&(old_pool->client_haddr)) ) {
            (void)vtss_avl_tree_delete(&g_pool_chaddr_avlt, (void **)&old_pool);
        }

    // fall through

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK:
        (void)vtss_avl_tree_delete(&g_pool_ip_avlt, (void **)&old_pool);
        break;
    }

    // update binding
    _binding_update_by_pool( new_pool, old_pool );

    // update pool
    *old_pool = *new_pool;

    // get new total count
    _pool_total_cnt_get( old_pool );

    // update alloc count
    old_pool->alloc_cnt = 0;
    memset( &binding, 0, sizeof(binding) );
    binding.pool = old_pool;
    bp = &binding;
    while ( vtss_avl_tree_get(&g_binding_name_avlt, (void **)&bp, VTSS_AVL_TREE_GET_NEXT) ) {
        if ( bp->pool != binding.pool ) {
            break;
        }
        if ( bp->state != VTSS_APPL_DHCP_SERVER_BINDING_STATE_EXPIRED ) {
            ++( old_pool->alloc_cnt );
        }
    }

    // add new into avlt
    switch ( old_pool->type ) {
    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NONE:
    default:
        break;

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST:
        if ( old_pool->client_identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
            if ( old_pool->ip && old_pool->subnet_mask ) {
                if ( vtss_avl_tree_add(&g_pool_id_avlt, (void *)old_pool) == FALSE ) {
                    T_D("host id duplicate\n");
                    return FALSE;
                }
            }
        } else if ( _not_empty_mac(&(old_pool->client_haddr)) ) {
            if ( old_pool->ip && old_pool->subnet_mask ) {
                if ( vtss_avl_tree_add(&g_pool_chaddr_avlt, (void *)old_pool) == FALSE ) {
                    T_D("host mac duplicate\n");
                    return FALSE;
                }
            }
        }

    // fall through

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK:
        if ( old_pool->ip && old_pool->subnet_mask ) {
            if ( vtss_avl_tree_add(&g_pool_ip_avlt, (void *)old_pool) == FALSE ) {
                T_D("network duplicate\n");
                return FALSE;
            }
        }
        break;
    }

    return TRUE;
}

/**
 *  \brief
 *      update old pool to new pool.
 *
 *  \param
 *      new_pool  [IN]: new DHCP pool.
 *      new_pool [OUT]: update to new DHCP pool.
 *
 *  \return
 *      n/a.
 */
static void _pool_new(
    INOUT dhcp_server_pool_t    *new_pool
)
{
    _pool_total_cnt_get( new_pool );

    new_pool->alloc_cnt = 0;

    STATISTICS_INC( pool_cnt );
}

/**
 *  \brief
 *      Delete DHCP pool
 *      the memory is the real memory for delete
 *
 *  \param
 *      pool [IN]: DHCP pool to be deleted.
 *
 *  \return
 *      n/a.
 */
static void _pool_delete(
    IN  dhcp_server_pool_t     *pool
)
{
    dhcp_server_pool_t        *p;

    /* remove from avlt */
    p = pool;
    (void)vtss_avl_tree_delete( &g_pool_name_avlt, (void **)&p );

    // check if new is duplicate
    switch ( pool->type ) {
    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NONE:
        break;

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST:
        p = pool;
        if ( p->client_identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
            if ( vtss_avl_tree_get(&g_pool_id_avlt, (void **)&p, VTSS_AVL_TREE_GET) ) {
                if ( p == pool ) {
                    (void)vtss_avl_tree_delete(&g_pool_id_avlt, (void **)&p);
                }
            }
        } else if ( _not_empty_mac(&(pool->client_haddr)) ) {
            if ( vtss_avl_tree_get(&g_pool_chaddr_avlt, (void **)&p, VTSS_AVL_TREE_GET) ) {
                if ( p == pool ) {
                    (void)vtss_avl_tree_delete(&g_pool_chaddr_avlt, (void **)&p);
                }
            }
        }

    // fall through

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK:
        p = pool;
        if ( vtss_avl_tree_get(&g_pool_ip_avlt, (void **)&p, VTSS_AVL_TREE_GET) ) {
            if ( p == pool ) {
                (void)vtss_avl_tree_delete(&g_pool_ip_avlt, (void **)&p);
            }
        }
        break;

    default:
        break;
    }

    /* delete all related bindings */
    _binding_remove_by_pool_name( p );

    // free pool
    vtss_free_list_free(&g_pool_flist, pool);

    STATISTICS_DEC( pool_cnt );
}

static void _declined_ip_remove(
    void
)
{
    mesa_ipv4_t     *ipp;
    mesa_ipv4_t     ip;

    ip  = 0;
    ipp = &ip;

    while ( vtss_avl_tree_get(&g_decline_ip_avlt, (void **)&ipp, VTSS_AVL_TREE_GET_NEXT) ) {
        _declined_ip_delete( ipp );
    }
}

static BOOL _is_netmask(
    IN mesa_ipv4_t  netmask
)
{
    int             i;
    BOOL            b_one;
    mesa_ipv4_t     r;

    b_one = FALSE;
    for ( i = 0; i < 32; ++i ) {
        r = ( netmask >> i ) & 0x01;
        if ( r ) {
            if ( b_one == FALSE ) {
                b_one = TRUE;
            }
        } else {
            if ( b_one ) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

/* Callback to let the IP module notify us when vlan interfaces are added/deleted. */
void dhcp_server_ip_vlan_interface_callback(vtss_ifindex_t ifidx)
{
    if (vtss_ifindex_is_vlan(ifidx)) {
        vtss_ifindex_elm_t ife;
        (void)vtss_ifindex_decompose(ifidx, &ife);
        mesa_vid_t if_id = ife.ordinal;
        // We just need to reset the per-interface enable bit to default.
        if (!vtss_appl_ip_if_exists(ifidx)) {
            _VLAN_BF_SET(if_id, FALSE);
        }
    }
}

/*
==============================================================================

    Public Function

==============================================================================
*/
/**
 *  \brief
 *      Initialize DHCP server engine.
 *
 *  \param
 *      void
 *
 *  \return
 *      TRUE  : successful.\n
 *      FALSE : failed.
 */
BOOL vtss_dhcp_server_init(
    void
)
{
    /* default configuration */
    g_enable = FALSE;
    memset(&g_statistics, 0, sizeof(g_statistics));

    /* clear all VLANs */
    _VLAN_BF_CLR_ALL();

    /* Subscribe to VLAN-inteface changes. */
    VTSS_RC(vtss_ip_if_callback_add(dhcp_server_ip_vlan_interface_callback));

    /*
        Excluded IP list
    */
    if ( vtss_free_list_init(&g_excluded_ip_flist) == FALSE ) {
        T_D("Fail to create Free list for g_excluded_ip_flist\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_excluded_ip_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_excluded_ip_avlt\n");
        return FALSE;
    }

    /*
        Pool list
    */
    if ( vtss_free_list_init(&g_pool_flist) == FALSE ) {
        T_D("Fail to create Free list for g_pool_flist\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_pool_name_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_pool_name_avlt\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_pool_ip_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_pool_ip_avlt\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_pool_id_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_pool_id_avlt\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_pool_chaddr_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_pool_chaddr_avlt\n");
        return FALSE;
    }

    /*
        Binding list
    */
    if ( vtss_free_list_init(&g_binding_flist) == FALSE ) {
        T_D("Fail to create Free list for g_binding_flist\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_binding_ip_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_binding_ip_avlt\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_binding_id_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_binding_id_avlt\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_binding_chaddr_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_binding_chaddr_avlt\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_binding_name_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_binding_name_avlt\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_binding_lease_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_binding_lease_avlt\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_binding_expired_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_binding_expired_avlt\n");
        return FALSE;
    }

    /*
        Decline list
    */
    if ( vtss_free_list_init(&g_decline_flist) == FALSE ) {
        T_D("Fail to create Free list for g_decline_flist\n");
        return FALSE;
    }

    if ( vtss_avl_tree_init(&g_decline_ip_avlt) == FALSE ) {
        T_D("Fail to create AVL tree for g_decline_ip_avlt\n");
        return FALSE;
    }

    return TRUE;
}

/**
 *  \brief
 *      reset DHCP server engine to default configuration.
 *
 *  \param
 *      n/a.
 *
 *  \return
 *      n/a.
 */
void vtss_dhcp_server_reset_to_default(
    void
)
{
    dhcp_server_pool_t          *p;
    dhcp_server_pool_t          pool;
    dhcp_server_excluded_ip_t   excluded;
    dhcp_server_excluded_ip_t   *excluded_p;

    /* disable DHCP server */
    if ( _enable(FALSE) == FALSE ) {
        T_D("fail to disable DHCP server\n");
    }

    /* clear all VLANs */
    _VLAN_BF_CLR_ALL();

    /* free all IP pools and related bindings */
    memset(&pool, 0, sizeof(pool));
    p = &pool;
    while ( vtss_avl_tree_get(&g_pool_name_avlt, (void **)&p, VTSS_AVL_TREE_GET_NEXT) ) {
        _pool_delete( p );
    }

    /* declined IP */
    _declined_ip_remove();

    /* Excluded IP */
    memset(&excluded, 0, sizeof(excluded));
    excluded_p = &excluded;
    while ( vtss_avl_tree_get(&g_excluded_ip_avlt, (void **)&excluded_p, VTSS_AVL_TREE_GET_NEXT) ) {
        if ( vtss_avl_tree_delete(&g_excluded_ip_avlt, (void **)&excluded_p) ) {
            vtss_free_list_free(&g_excluded_ip_flist, excluded_p);
        } else {
            T_D("Fail to delete excluded IP %08x %08x\n", excluded_p->low_ip, excluded_p->high_ip);
        }
    }

    /* clear statistics */
    memset(&g_statistics, 0, sizeof(g_statistics));
}

/**
 *  \brief
 *      Enable/Disable DHCP server.
 *
 *  \param
 *      b_enable [IN]: TRUE - enable, FALSE - disable.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_enable_set(
    IN  BOOL    b_enable
)
{
    mesa_rc  rc;

    if ( _enable(b_enable) ) {
        rc = VTSS_RC_OK;
    } else {
        rc = VTSS_APPL_DHCP_SERVER_RC_ERROR;
    }

    return rc;
}

/**
 *  \brief
 *      Get if DHCP server is enabled or not.
 *
 *  \param
 *      b_enable [OUT]: TRUE - enable, FALSE - disable.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_enable_get(
    OUT BOOL    *b_enable
)
{
    if ( b_enable == NULL ) {
        T_D("b_enable == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    *b_enable = g_enable;

    return VTSS_RC_OK;
}

/**
 *  \brief
 *      Add excluded IP address range.
 *      index : low_ip, high_ip
 *
 *  \param
 *      excluded [IN]: IP address range to be excluded.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_excluded_add(
    IN  dhcp_server_excluded_ip_t     *excluded
)
{
    dhcp_server_excluded_ip_t     *ep;
    dhcp_server_binding_t         binding;
    dhcp_server_binding_t         *bp;

    if ( excluded == NULL ) {
        T_D("excluded == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( excluded->low_ip > excluded->high_ip ) {
        T_D("excluded->low_ip(%08x) > excluded->high_ip(%08x)\n", excluded->low_ip, excluded->high_ip);
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( excluded->low_ip == 0 && excluded->high_ip == 0 ) {
        T_D("excluded->low_ip == 0 && excluded->high_ip == 0\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check if exist */
    if ( vtss_avl_tree_get(&g_excluded_ip_avlt, (void **)&excluded, VTSS_AVL_TREE_GET) ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_DUPLICATE;
    }

    /* get free memory */
    ep = (dhcp_server_excluded_ip_t *)vtss_free_list_malloc( &g_excluded_ip_flist );
    if ( ep == NULL ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_MEMORY;
    }

    /* add */
    *ep = *excluded;
    if ( vtss_avl_tree_add(&g_excluded_ip_avlt, (void *)ep) == FALSE ) {
        vtss_free_list_free(&g_excluded_ip_flist, ep);
        return VTSS_APPL_DHCP_SERVER_RC_ERR_FULL;
    }

    /* remove binding in the new range */
    memset(&binding, 0, sizeof(binding));
    bp = &binding;
    while ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&bp, VTSS_AVL_TREE_GET_NEXT) ) {
        if ( _ip_in_excluded(ep, bp->ip) ) {
            _binding_remove( bp );
        }
    }

    STATISTICS_INC( excluded_cnt );

    return VTSS_RC_OK;
}

/**
 *  \brief
 *      Delete excluded IP address range.
 *      index : low_ip, high_ip
 *
 *  \param
 *      excluded [IN]: Excluded IP address range to be deleted.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_excluded_delete(
    IN  dhcp_server_excluded_ip_t     *excluded
)
{
    mesa_rc  rc;

    if ( excluded == NULL ) {
        T_D("excluded == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* delete */
    if ( vtss_avl_tree_delete(&g_excluded_ip_avlt, (void **)&excluded) ) {
        vtss_free_list_free(&g_excluded_ip_flist, excluded);
        STATISTICS_DEC( excluded_cnt );
        rc = VTSS_RC_OK;
    } else {
        rc = VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_EXIST;
    }

    return rc;
}

/**
 *  \brief
 *      Get excluded IP address range.
 *      index : low_ip, high_ip
 *
 *  \param
 *      excluded [IN] : index.
 *      excluded [OUT]: Excluded IP address range data.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_excluded_get(
    INOUT  dhcp_server_excluded_ip_t     *excluded
)
{
    dhcp_server_excluded_ip_t     *p;

    if ( excluded == NULL ) {
        T_D("excluded == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check if exist */
    p = excluded;
    if ( vtss_avl_tree_get(&g_excluded_ip_avlt, (void **)&p, VTSS_AVL_TREE_GET) ) {
        *excluded = *p;
        return VTSS_RC_OK;
    } else {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_EXIST;
    }
}

/**
 *  \brief
 *      Get next of current excluded IP address range.
 *      index : low_ip, high_ip
 *
 *  \param
 *      excluded [IN] : Currnet excluded IP address range index.
 *      excluded [OUT]: Next excluded IP address range data to get.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_excluded_get_next(
    INOUT  dhcp_server_excluded_ip_t     *excluded
)
{
    dhcp_server_excluded_ip_t     *p;

    if ( excluded == NULL ) {
        T_D("excluded == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check if exist */
    p = excluded;
    if ( vtss_avl_tree_get(&g_excluded_ip_avlt, (void **)&p, VTSS_AVL_TREE_GET_NEXT) ) {
        *excluded = *p;
        return VTSS_RC_OK;
    } else {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_EXIST;
    }
}

/**
 *  \brief
 *      Helper: Is IP a valid unicast host given a netmask?
 *
 *  \param
 *      ip   [IN]: IP
 *      mask [IN]: Netmask
 *
 *  \return
 *      TRUE  = valid host address
 *      FALSE = not a host address
 */
BOOL vtss_dhcp_server_is_host_addr(mesa_ipv4_t ip, mesa_ipv4_t mask)
{
    // Host part can't be zero          (network address)
    // Host part can't be all-1's       (directed broadcast)
    // Address can't be local broadcast (255.255.255.255)
    // Address can't be multicast       (224.0.0.0 - 239.255.255.255)
    // Address can't be class E         (240.x.y.z and up)
    // Address can't be localhost       (127.x.y.z. By convention.)
    return ((ip & ~mask) != 0) &&
           ((ip | mask) != 0xFFffFFff) &&
           ((ip >> 24) < 224) &&
           ((ip >> 24) != 127);
}

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
// Helper for qsort(): Compare based on IP address
static int reserved_entry_qsort_compare(const void *avoid, const void *bvoid)
{
    const vtss_appl_dhcp_server_reserved_entry_t *a = static_cast<const vtss_appl_dhcp_server_reserved_entry_t *>(avoid);
    const vtss_appl_dhcp_server_reserved_entry_t *b = static_cast<const vtss_appl_dhcp_server_reserved_entry_t *>(bvoid);

    // Entries without ifindex are "higher"; that way they go to the end of the sorted array
    const mesa_ipv4_t aip = (a->ifindex != VTSS_IFINDEX_NONE) ? a->address : 0xffffFFFF;
    const mesa_ipv4_t bip = (b->ifindex != VTSS_IFINDEX_NONE) ? b->address : 0xffffFFFF;

    if (aip < bip) {
        return -1;
    } else if (aip == bip) {
        return 0;
    }
    return 1;
}
#endif

/**
 *  \brief
 *      Set DHCP pool.
 *      index: name
 *
 *  \param
 *      pool [IN]: new or modified DHCP pool.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_pool_set(
    IN  dhcp_server_pool_t     *pool_in
)
{
    dhcp_server_pool_t      pool;       // Temporary, used for sorting of reserved entries. To avoid molesting pool_in.
    dhcp_server_pool_t      *p;
    dhcp_server_pool_t      *new_p;
    u8                      u;
    BOOL                    b_id_avlt;
    BOOL                    b_chaddr_avlt;

    if ( pool_in == NULL ) {
        T_D("pool_in == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( strlen(pool_in->pool_name) == 0 ) {
        T_D("strlen(pool_in->pool_name) == 0\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( pool_in->ip ) {
        u = (u8)( (pool_in->ip & 0xff000000) >> 24 );
        if ( u == 0 || u == 127 || u >= 224 ) {
            return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
        }
    }

    if ( _is_netmask(pool_in->subnet_mask) == FALSE ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    pool = *pool_in;

    switch ( pool.type ) {
    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NONE:
        break;

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST:
        if ( pool.ip ) {
            // check if Host IP is duplicate or not
            p = NULL;
            while ( vtss_avl_tree_get(&g_pool_name_avlt, (void **)&p, VTSS_AVL_TREE_GET_NEXT) ) {
                if ( strcmp(p->pool_name, pool.pool_name) && p->type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST && p->ip == pool.ip ) {
                    return VTSS_APPL_DHCP_SERVER_RC_ERR_IP;
                }
            }

            if (! vtss_dhcp_server_is_host_addr(pool.ip, pool.subnet_mask)) {
                return VTSS_APPL_DHCP_SERVER_RC_ERR_IP;
            }
        }

    // fall through

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK:
        if ( pool.ip && pool.subnet_mask ) {
            /* check if IP exists in ip avlt */
            p = &pool;
            if ( vtss_avl_tree_get(&g_pool_ip_avlt, (void **)&p, VTSS_AVL_TREE_GET) ) {
                /* check if the same pool */
                if ( strcmp(p->pool_name, pool.pool_name) ) {
                    /* different pool, not allowed */
                    return VTSS_APPL_DHCP_SERVER_RC_ERR_DUPLICATE;
                }
            }
        }
#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
        // Sort array of reserved entries by IP; unused entries to to the end.
        // This is necessary when removing bindings during _pool_update()
        qsort(pool.reserved, VTSS_APPL_DHCP_SERVER_RESERVED_CNT, sizeof(pool.reserved[0]), reserved_entry_qsort_compare);
#endif
        break;

    default:
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* Update pool, if it exists in name avlt */
    p = &pool;
    if ( vtss_avl_tree_get(&g_pool_name_avlt, (void **)&p, VTSS_AVL_TREE_GET) ) {
        T_D("Updating pool %s", p->pool_name);
        if (_pool_update( &pool, p )) {
            return VTSS_RC_OK;
        } else {
            return VTSS_APPL_DHCP_SERVER_RC_ERR_DUPLICATE;
        }
    }

    /* totally new one */
    T_D("Building new pool instance: %s", pool.pool_name);

    /* get free memory */
    new_p = (dhcp_server_pool_t *)vtss_free_list_malloc( &g_pool_flist );
    if ( new_p == NULL ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_MEMORY;
    }

    /* add */
    p = new_p;
    *p = pool;

    // add name avlt
    if ( vtss_avl_tree_add(&g_pool_name_avlt, (void *)p) == FALSE ) {
        vtss_free_list_free( &g_pool_flist, new_p );
        return VTSS_APPL_DHCP_SERVER_RC_ERR_FULL;
    }

    /*
        the followings do not return FULL and just show error message only
        it is because if full, then it should happen when adding name avlt
        so if the followings are still full, then it is a bug
    */
    b_id_avlt = FALSE;
    b_chaddr_avlt = FALSE;

    switch ( p->type ) {
    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NONE:
    default:
        break;

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST:
        // remove ip to prepare for new
        _binding_remove_by_ip( p->ip );

        if ( p->client_identifier.type != VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE ) {
            // add to id AVL tree
            if ( p->ip && p->subnet_mask ) {
                if ( vtss_avl_tree_add(&g_pool_id_avlt, (void *)p) ) {
                    b_id_avlt = TRUE;
                } else {
                    (void)vtss_avl_tree_delete(&g_pool_name_avlt, (void **)&p);
                    vtss_free_list_free( &g_pool_flist, new_p );
                    return VTSS_APPL_DHCP_SERVER_RC_ERR_DUPLICATE;
                }

                // delete binding for the new pool
                _binding_remove_by_pool_client_id( p );
            }
        } else if ( _not_empty_mac(&(p->client_haddr)) ) {
            // add to chaddr AVL tree
            if ( p->ip && p->subnet_mask ) {
                if ( vtss_avl_tree_add(&g_pool_chaddr_avlt, (void *)p) ) {
                    b_chaddr_avlt = TRUE;
                } else {
                    (void)vtss_avl_tree_delete(&g_pool_name_avlt, (void **)&p);
                    vtss_free_list_free( &g_pool_flist, new_p );
                    return VTSS_APPL_DHCP_SERVER_RC_ERR_DUPLICATE;
                }

                // delete binding for the new pool
                _binding_remove_by_pool_chaddr( p );
            }
        }

    // fall through

    case VTSS_APPL_DHCP_SERVER_POOL_TYPE_NETWORK:
        // add ip avlt
        if ( p->ip && p->subnet_mask ) {
            if ( vtss_avl_tree_add(&g_pool_ip_avlt, (void *)p) == FALSE ) {
                (void)vtss_avl_tree_delete(&g_pool_name_avlt, (void **)&p);
                if ( b_id_avlt ) {
                    (void)vtss_avl_tree_delete(&g_pool_id_avlt, (void **)&p);
                }
                if ( b_chaddr_avlt ) {
                    (void)vtss_avl_tree_delete(&g_pool_chaddr_avlt, (void **)&p);
                }
                vtss_free_list_free( &g_pool_flist, new_p );
                return VTSS_APPL_DHCP_SERVER_RC_ERR_DUPLICATE;
            }
        }
        break;
    }

    // new pool
    _pool_new( p );

    return VTSS_RC_OK;
}

/**
 *  \brief
 *      Delete DHCP pool.
 *      index: name
 *
 *  \param
 *      pool [IN]: DHCP pool to be deleted.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_pool_delete(
    IN  dhcp_server_pool_t     *pool
)
{
    dhcp_server_pool_t  *p;
    mesa_rc    rc;
    BOOL                b;

    if ( pool == NULL ) {
        T_D("pool == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check if exist */
    p = pool;
    b = vtss_avl_tree_get(&g_pool_name_avlt, (void **)&p, VTSS_AVL_TREE_GET);
    if ( b ) {
        _pool_delete( p );
        rc = VTSS_RC_OK;
    } else {
        rc = VTSS_APPL_DHCP_SERVER_RC_ERR_POOL_NOT_EXIST;
    }

    return rc;
}

/**
 *  \brief
 *      Get DHCP pool.
 *      index: name
 *
 *  \param
 *      pool [IN] : index.
 *      pool [OUT]: DHCP pool data.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_pool_get(
    INOUT  dhcp_server_pool_t     *pool
)
{
    dhcp_server_pool_t      *p;

    if ( pool == NULL ) {
        T_D("pool == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check if exist */
    p = pool;
    if ( vtss_avl_tree_get(&g_pool_name_avlt, (void **)&p, VTSS_AVL_TREE_GET) ) {
        *pool = *p;
        return VTSS_RC_OK;
    }

    return VTSS_APPL_DHCP_SERVER_RC_ERR_POOL_NOT_EXIST;
}

/**
 *  \brief
 *      Get next of current DHCP pool.
 *      index: name
 *
 *  \param
 *      pool [IN] : Currnet DHCP pool index.
 *      pool [OUT]: Next DHCP pool data to get.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_pool_get_next(
    INOUT  dhcp_server_pool_t     *pool
)
{
    dhcp_server_pool_t *p;

    if ( pool == NULL ) {
        T_D("pool == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check if exist */
    p = pool;
    if ( vtss_avl_tree_get(&g_pool_name_avlt, (void **)&p, VTSS_AVL_TREE_GET_NEXT) ) {
        *pool = *p;
        return VTSS_RC_OK;
    }

    return VTSS_APPL_DHCP_SERVER_RC_ERR_POOL_NOT_EXIST;
}

/**
 *  \brief
 *      Set DHCP pool to be default value.
 *
 *  \param
 *      pool [OUT]: default DHCP pool.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_pool_default(
    IN  dhcp_server_pool_t     *pool
)
{
    if ( pool == NULL ) {
        T_D("pool == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    memset(pool, 0, sizeof(dhcp_server_pool_t));
    pool->lease =
        DHCP_SERVER_LEASE_DEFAULT_DAYS    * 24 * 60 * 60 +
        DHCP_SERVER_LEASE_DEFAULT_HOURS   * 60 * 60    +
        DHCP_SERVER_LEASE_DEFAULT_MINUTES * 60;

    return VTSS_RC_OK;
}

/**
 *  \brief
 *      Clear DHCP message statistics.
 *
 *  \param
 *      n/a.
 *
 *  \return
 *      n/a.
 */
void vtss_dhcp_server_statistics_clear(
    void
)
{
    g_statistics.discover_cnt = 0;
    g_statistics.offer_cnt    = 0;
    g_statistics.request_cnt  = 0;
    g_statistics.ack_cnt      = 0;
    g_statistics.nak_cnt      = 0;
    g_statistics.decline_cnt  = 0;
    g_statistics.release_cnt  = 0;
    g_statistics.inform_cnt   = 0;
}

/**
 *  \brief
 *      Get DHCP message statistics.
 *
 *  \param
 *      statistics [OUT]: DHCP message statistics data to get.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_statistics_get(
    OUT dhcp_server_statistics_t  *statistics
)
{
    if ( statistics == NULL ) {
        T_D("statistics == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    *statistics = g_statistics;

    return VTSS_RC_OK;
}

/**
 *  \brief
 *      Delete DHCP binding.
 *      index: ip
 *
 *  \param
 *      binding [IN]: DHCP binding to be deleted.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_binding_delete(
    IN  dhcp_server_binding_t     *binding
)
{
    if ( binding == NULL ) {
        T_D("binding == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check if exist */
    if ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&binding, VTSS_AVL_TREE_GET) == FALSE ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_EXIST;
    }

    /* delete */
    switch ( binding->type ) {
    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_AUTOMATIC:
    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_MANUAL:
        _binding_expired_move( binding );
        break;

    case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_EXPIRED:
        _binding_remove( binding );
        break;

    default:
        T_D("invalid binding type : %u of ip %08x\n", binding->type, binding->ip);
        break;
    }

    return VTSS_RC_OK;
}

/**
 *  \brief
 *      Get DHCP binding.
 *      index: ip
 *
 *  \param
 *      binding [IN] : index.
 *      binding [OUT]: DHCP binding data.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_binding_get(
    INOUT  dhcp_server_binding_t     *binding
)
{
    dhcp_server_binding_t     *bp;

    if ( binding == NULL ) {
        T_D("binding == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check if exist */
    bp = binding;
    if ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&bp, VTSS_AVL_TREE_GET) ) {
        *binding = *bp;
        return VTSS_RC_OK;
    } else {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_EXIST;
    }
}

/**
 *  \brief
 *      Get next of current DHCP binding.
 *      index: ip
 *
 *  \param
 *      binding [IN] : Currnet DHCP binding index.
 *      binding [OUT]: Next DHCP binding data to get.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_binding_get_next(
    INOUT  dhcp_server_binding_t     *binding
)
{
    dhcp_server_binding_t     *bp;

    if ( binding == NULL ) {
        T_D("binding == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check if exist */
    bp = binding;
    if ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&bp, VTSS_AVL_TREE_GET_NEXT) ) {
        *binding = *bp;
        return VTSS_RC_OK;
    } else {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_EXIST;
    }
}

/**
 *  \brief
 *      Clear DHCP bindings by binding type.
 *
 *  \param
 *      type - binding type
 *
 *  \return
 *      n/a.
 */
void vtss_dhcp_server_binding_clear_by_type(
    IN vtss_appl_dhcp_server_binding_type_t     type
)
{
    dhcp_server_binding_t     binding;
    dhcp_server_binding_t     *bp;

    memset(&binding, 0, sizeof(binding));
    bp = &binding;

    while ( vtss_avl_tree_get(&g_binding_ip_avlt, (void **)&bp, VTSS_AVL_TREE_GET_NEXT) ) {
        switch ( type ) {
        case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_AUTOMATIC:
        case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_MANUAL:
            if ( bp->type != type ) {
                break;
            }
            _binding_expired_move( bp );
            break;

        case VTSS_APPL_DHCP_SERVER_BINDING_TYPE_EXPIRED:
            if ( bp->type != type ) {
                break;
            }
            _binding_remove( bp );
            break;

        default:
            T_D("invalid binding type : %u of ip %08x\n", bp->type, bp->ip);
            break;
        }
    }
}

/**
 *  \brief
 *      Enable/Disable DHCP server per VLAN.
 *
 *  \param
 *      vid      [IN]: VLAN ID
 *      b_enable [IN]: TRUE - enable, FALSE - disable.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_vlan_enable_set(
    IN  mesa_vid_t      vid,
    IN  BOOL            b_enable
)
{
    dhcp_server_binding_t     binding;
    dhcp_server_binding_t     *bp;

    if ( vid < 1 || vid > (VTSS_VIDS - 1) ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    _VLAN_BF_SET( vid, b_enable );

    if ( b_enable == FALSE ) {
        /* move all binding of the VLAN to expired */
        memset(&binding, 0, sizeof(binding));
        bp = &binding;
        while ( vtss_avl_tree_get(&g_binding_lease_avlt, (void **)&bp, VTSS_AVL_TREE_GET_NEXT) ) {
            if ( bp->vid == vid ) {
                _binding_expired_move( bp );
            }
        }
    }

    return VTSS_RC_OK;
}

/**
 *  \brief
 *      Get Enable/Disable DHCP server per VLAN.
 *
 *  \param
 *      vid      [IN] : VLAN ID
 *      b_enable [OUT]: TRUE - enable, FALSE - disable.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_vlan_enable_get(
    IN  mesa_vid_t      vid,
    OUT BOOL            *b_enable
)
{
    if (!vtss_appl_ip_if_exists(vtss_ifindex_cast_from_u32(vid, VTSS_IFINDEX_TYPE_VLAN))) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    if ( b_enable == NULL ) {
        T_D("b_enable == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    *b_enable = _VLAN_BF_GET( vid );

    return VTSS_RC_OK;
}

/**
 *  \brief
 *      process timer related function.
 *
 *  \param
 *      n/a.
 *
 *  \return
 *      n/a.
 */
void vtss_dhcp_server_timer_process(
    void
)
{
    dhcp_server_binding_t   binding;
    dhcp_server_binding_t   *bp;
    u32                     current_time;

    /* get current time in second */
    current_time = dhcp_server_platform_current_time_get();

    /* check lease */
    memset(&binding, 0, sizeof(binding));
    bp = &binding;
    while ( vtss_avl_tree_get(&g_binding_lease_avlt, (void **)&bp, VTSS_AVL_TREE_GET_NEXT) ) {
        if ( current_time >= bp->expire_time ) {
            if ( bp->state == VTSS_APPL_DHCP_SERVER_BINDING_STATE_ALLOCATED ) {
                /* the allocation is failed, so just free it */
                _binding_remove( bp );
            } else {
                /* this binding is expired */
                _binding_expired_move( bp );
            }
        } else {
            break;
        }
    }
}

/**
 *  \brief
 *      receive and process DHCP message from client.
 *
 *  \param
 *      packet  [IN]: Ethernet packet with DHCP message inside.
 *      vid     [IN]: VLAN ID.
 *      ifindex [IN]: Interface that packet arrived on.
 *
 *  \return
 *      TRUE  : DHCP server process this packet.
 *      FALSE : the packet is not processed, so pass it to other application.
 */
BOOL vtss_dhcp_server_packet_rx(
    IN const void       *packet,
    IN mesa_vid_t       vid,
    IN vtss_ifindex_t   ifindex
)
{
    u32                         ip_len;
    dhcp_server_message_t       *message;
    dhcp_server_eth_header_t    *eth_hdr;
    dhcp_server_ip_header_t     *ip_hdr;
    BOOL                        b;

    /* check global mode */
    if ( g_enable == FALSE ) {
        /*
            global is disabled.
            DHCP server does not process this packet
            and deregister to helper.
        */
        dhcp_server_platform_packet_rx_deregister();
        T_D("DHCP server not enabled");
        return FALSE;
    }

    /* check VLAN mode */
    if ( _VLAN_BF_GET(vid) == FALSE ) {
        /*
            global enable, but this VLAN is disabled.
            DHCP server does not process this packet.
        */
        T_D("Vlan mode not enabled for vlan %u", vid);
        return FALSE;
    }

    /* get ether header */
    eth_hdr = (dhcp_server_eth_header_t *)packet;
    if (eth_hdr->etype[0] != 0x08 || eth_hdr->etype[1] != 0x00) {
        T_D("Wrong etype 0x%02x%02x\n", eth_hdr->etype[0], eth_hdr->etype[1]);
    }

    /* get ip length */
    ip_hdr = (dhcp_server_ip_header_t *)( (u8 *)packet + sizeof(dhcp_server_eth_header_t) );
    ip_len = ( ip_hdr->vhl & 0x0F ) * 4;
    if (ip_len != 20) {
        T_D("Wrong ip_len %u\n", ip_len);
    }

    /* get DHCP message */
    message = (dhcp_server_message_t *)( (u8 *)packet + sizeof(dhcp_server_eth_header_t) + ip_len + sizeof(dhcp_server_udp_header_t) );

    /* not support relay agent */
    if ( message->giaddr ) {
        T_N("This message is from a Relay Agent");
    }

    /* support MAC address only */
    if ( message->htype != _HTYPE_ETHERNET || message->hlen != DHCP_SERVER_MAC_LEN ) {
        T_D("hardware type %02x not supported\n", message->htype);
        return FALSE;
    }

    /* process message */
    b = _message_process( message, vid, ifindex, eth_hdr->smac );
    return b;
}

/**
 *  \brief
 *      add a declined IP. This API should be used for debug only.
 *      index: declined_ip
 *
 *  \param
 *      declined_ip [IN] : index.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_declined_add(
    IN  mesa_ipv4_t     declined_ip
)
{
    mesa_ipv4_t     *ip;

    if ( declined_ip == 0 ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* add decline IP */
    ip = (mesa_ipv4_t *)vtss_free_list_malloc( &g_decline_flist );
    if ( ip == NULL ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_FULL;
    }

    *ip = declined_ip;

    if ( vtss_avl_tree_add(&g_decline_ip_avlt, (void *)ip) == FALSE ) {
        vtss_free_list_free( &g_decline_flist, ip );
        return VTSS_APPL_DHCP_SERVER_RC_ERR_FULL;
    }

    STATISTICS_INC( declined_cnt );

    return VTSS_RC_OK;
}

/**
 *  \brief
 *      delete a declined IP. This API should be used for debug only.
 *      index: declined_ip
 *
 *  \param
 *      declined_ip [IN] : index.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_declined_delete(
    IN  mesa_ipv4_t     declined_ip
)
{
    if ( declined_ip == 0 ) {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    _declined_ip_delete( &declined_ip );
    return VTSS_RC_OK;
}

/**
 *  \brief
 *      Get declined IP.
 *      index: declined_ip
 *
 *  \param
 *      declined_ip [IN] : index.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_declined_get(
    IN  mesa_ipv4_t     *declined_ip
)
{
    mesa_ipv4_t     *ip;

    if ( declined_ip == NULL ) {
        T_D("declined_ip == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check if exist */
    ip = declined_ip;
    if ( vtss_avl_tree_get(&g_decline_ip_avlt, (void **)&ip, VTSS_AVL_TREE_GET) ) {
        *declined_ip = *ip;
        return VTSS_RC_OK;
    } else {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_EXIST;
    }
}

/**
 *  \brief
 *      Get next declined IP.
 *      index: declined_ip
 *
 *  \param
 *      declined_ip [IN] : Currnet declined IP.
 *      declined_ip [OUT]: Next declined IP to get.
 *
 *  \return
 *      VTSS_RC_OK : successful.\n
 *      VTSS_APPL_DHCP_SERVER_RC_XX : failed.
 */
mesa_rc vtss_dhcp_server_declined_get_next(
    INOUT   mesa_ipv4_t     *declined_ip
)
{
    mesa_ipv4_t     *ip;

    if ( declined_ip == NULL ) {
        T_D("declined_ip == NULL\n");
        return VTSS_APPL_DHCP_SERVER_RC_ERR_PARAMETER;
    }

    /* check if exist */
    ip = declined_ip;
    if ( vtss_avl_tree_get(&g_decline_ip_avlt, (void **)&ip, VTSS_AVL_TREE_GET_NEXT) ) {
        *declined_ip = *ip;
        return VTSS_RC_OK;
    } else {
        return VTSS_APPL_DHCP_SERVER_RC_ERR_NOT_EXIST;
    }
}

// Debug
BOOL vtss_dhcp_server_binding_x_get_next(
    IN      u32                     i,
    INOUT   dhcp_server_binding_t   *binding
)
{
    vtss_avl_tree_t         *ptree;
    dhcp_server_binding_t   *bp;

    if ( binding == NULL ) {
        T_D("binding == NULL\n");
        return FALSE;
    }

    switch (i) {
    case 0:
        ptree = &g_binding_ip_avlt;
        break;

    case 1:
        ptree = &g_binding_id_avlt;
        break;

    case 2:
        ptree = &g_binding_chaddr_avlt;
        break;

    case 3:
        ptree = &g_binding_name_avlt;
        break;

    case 4:
        ptree = &g_binding_lease_avlt;
        break;

    case 5:
        ptree = &g_binding_expired_avlt;
        break;

    default:
        return FALSE;
    }

    /* check if exist */
    bp = binding;
    if ( vtss_avl_tree_get(ptree, (void **)&bp, VTSS_AVL_TREE_GET_NEXT) ) {
        *binding = *bp;
        return TRUE;
    } else {
        return FALSE;
    }
}
