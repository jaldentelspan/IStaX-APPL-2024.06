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
 *      dhcp_server_icfg.c
 *
 *  \brief
 *      ICFG implementation
 *
 *  \author
 *      CP Wang
 *
 *  \date
 *      05/09/2013 11:50
 */
//----------------------------------------------------------------------------
/*
******************************************************************************

    Include files

******************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include "icfg_api.h"
#include "dhcp_server_api.h"
#include "dhcp_server_platform.h"
#include "misc_api.h"
#include "icli_porting_util.h"

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/
/**
 * \brief
 *      ICFG callback function.
 */
static mesa_rc _dhcp_server_icfg(
    IN const vtss_icfg_query_request_t  *req,
    IN vtss_icfg_query_result_t         *result
)
{
    BOOL                        b_enable;
    dhcp_server_excluded_ip_t   excluded;
    char                        str[64];
    dhcp_server_pool_t          pool;
    mesa_vid_t                  vid;
    u32                         i;
    u32                         j;
    u32                         minute;
    u32                         hour;
    u32                         day;
    char                        empty_mac[DHCP_SERVER_MAC_LEN] = {0, 0, 0, 0, 0, 0};

    if ( req == NULL ) {
        return VTSS_RC_ERROR;
    }

    if ( result == NULL ) {
        return VTSS_RC_ERROR;
    }

    switch ( req->cmd_mode ) {
    case ICLI_CMD_MODE_GLOBAL_CONFIG:
        /* COMMAND = ip dhcp server */
        if ( dhcp_server_enable_get(&b_enable) != VTSS_RC_OK ) {
            T_D("dhcp_server_enable_get()\n");
            return VTSS_RC_ERROR;
        }
        if ( b_enable ) {
            VTSS_RC( vtss_icfg_printf(result, "ip dhcp server\n") );
        } else if ( req->all_defaults ) {
            VTSS_RC( vtss_icfg_printf(result, "no ip dhcp server\n") );
        }

        /* COMMAND = ip dhcp excluded-address <ipv4_addr> [<ipv4_addr>] */
        memset(&excluded, 0, sizeof(excluded));
        while ( dhcp_server_excluded_get_next(&excluded) == VTSS_RC_OK ) {
            if ( excluded.low_ip == excluded.high_ip ) {
                VTSS_RC( vtss_icfg_printf(result, "ip dhcp excluded-address") );
                VTSS_RC( vtss_icfg_printf(result, " %s\n", icli_ipv4_to_str(excluded.low_ip, str)) );
            } else {
                VTSS_RC( vtss_icfg_printf(result, "ip dhcp excluded-address") );
                VTSS_RC( vtss_icfg_printf(result, " %s", icli_ipv4_to_str(excluded.low_ip, str)) );
                VTSS_RC( vtss_icfg_printf(result, " %s\n", icli_ipv4_to_str(excluded.high_ip, str)) );
            }
        }
        break;

    case ICLI_CMD_MODE_INTERFACE_VLAN:
        vid = (mesa_vid_t)( req->instance_id.vlan );

        /* COMMAND = ip dhcp server */
        if ( dhcp_server_vlan_enable_get(vid, &b_enable) != VTSS_RC_OK ) {
            T_D("dhcp_server_vlan_enable_get( %u )\n", vid);
            return VTSS_RC_ERROR;
        }
        if ( b_enable ) {
            VTSS_RC( vtss_icfg_printf(result, " ip dhcp server\n") );
        } else if ( req->all_defaults ) {
            VTSS_RC( vtss_icfg_printf(result, " no ip dhcp server\n") );
        }
        break;

    case ICLI_CMD_MODE_DHCP_POOL:
        memset(&pool, 0, sizeof(dhcp_server_pool_t));
        (void)icli_str_cpy(pool.pool_name, req->instance_id.string);
        if ( dhcp_server_pool_get(&pool) != VTSS_RC_OK ) {
            T_D("dhcp_server_pool_get( %s )\n", pool.pool_name);
            return VTSS_RC_ERROR;
        }

        /* COMMAND = network <ipv4_addr> <ipv4_netmask> */
        /* COMMAND = host <ipv4_addr> <ipv4_netmask> */
        if ( pool.type != VTSS_APPL_DHCP_SERVER_POOL_TYPE_NONE ) {
            if ( pool.type == VTSS_APPL_DHCP_SERVER_POOL_TYPE_HOST ) {
                VTSS_RC( vtss_icfg_printf(result, " host") );
            } else {
                VTSS_RC( vtss_icfg_printf(result, " network") );
            }
            VTSS_RC( vtss_icfg_printf(result, " %s", icli_ipv4_to_str(pool.ip, str)) );
            VTSS_RC( vtss_icfg_printf(result, " %s\n", icli_ipv4_to_str(pool.subnet_mask, str)) );
        }

        /* COMMAND = broadcast <ipv4_addr> */
        if ( pool.subnet_broadcast ) {
            VTSS_RC( vtss_icfg_printf(result, " broadcast %s\n", icli_ipv4_to_str(pool.subnet_broadcast, str)) );
        }

        /* COMMAND = default-router <ipv4_ucast> [<ipv4_ucast> [<ipv4_ucast> [<ipv4_ucast>]]] */
        if ( pool.default_router[0] ) {
            VTSS_RC( vtss_icfg_printf(result, " default-router") );
            for ( i = 0; i < VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT; i++ ) {
                if ( pool.default_router[i] == 0 ) {
                    break;
                }
                VTSS_RC( vtss_icfg_printf(result, " %s", icli_ipv4_to_str(pool.default_router[i], str)) );
            }
            VTSS_RC( vtss_icfg_printf(result, "\n") );
        }

        /* COMMAND = lease { <0-365> [ <0-23> [ <uint> ] ] | infinite } */
        VTSS_RC( vtss_icfg_printf(result, " lease") );
        if ( pool.lease ) {
            day    = pool.lease / (24 * 60 * 60);
            hour   = ( pool.lease % (24 * 60 * 60) ) / (60 * 60);
            minute = ( pool.lease % (60 * 60) ) / 60;

            VTSS_RC( vtss_icfg_printf(result, " %u %u %u\n", day, hour, minute) );
        } else {
            VTSS_RC( vtss_icfg_printf(result, " infinite\n") );
        }

        /* COMMAND = domain-name <word128> */
        if ( icli_str_len(pool.domain_name) ) {
            VTSS_RC( vtss_icfg_printf(result, " domain-name %s\n", pool.domain_name) );
        }

        /* COMMAND = dns-server <ipv4_ucast> [<ipv4_ucast> [<ipv4_ucast> [<ipv4_ucast>]]] */
        if ( pool.dns_server[0] ) {
            VTSS_RC( vtss_icfg_printf(result, " dns-server") );
            for ( i = 0; i < VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT; i++ ) {
                if ( pool.dns_server[i] == 0 ) {
                    break;
                }
                VTSS_RC( vtss_icfg_printf(result, " %s", icli_ipv4_to_str(pool.dns_server[i], str)) );
            }
            VTSS_RC( vtss_icfg_printf(result, "\n") );
        }

        /* COMMAND = ntp-server <ipv4_ucast> [<ipv4_ucast> [<ipv4_ucast> [<ipv4_ucast>]]] */
        if ( pool.ntp_server[0] ) {
            VTSS_RC( vtss_icfg_printf(result, " ntp-server") );
            for ( i = 0; i < VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT; i++ ) {
                if ( pool.ntp_server[i] == 0 ) {
                    break;
                }
                VTSS_RC( vtss_icfg_printf(result, " %s", icli_ipv4_to_str(pool.ntp_server[i], str)) );
            }
            VTSS_RC( vtss_icfg_printf(result, "\n") );
        }

        /* COMMAND = netbios-name-server <ipv4_ucast> [<ipv4_ucast> [<ipv4_ucast> [<ipv4_ucast>]]] */
        if ( pool.netbios_name_server[0] ) {
            VTSS_RC( vtss_icfg_printf(result, " netbios-name-server") );
            for ( i = 0; i < VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT; i++ ) {
                if ( pool.netbios_name_server[i] == 0 ) {
                    break;
                }
                VTSS_RC( vtss_icfg_printf(result, " %s", icli_ipv4_to_str(pool.netbios_name_server[i], str)) );
            }
            VTSS_RC( vtss_icfg_printf(result, "\n") );
        }

        /* COMMAND = netbios-node-type { b-node | h-node | m-node | p-node } */
        switch ( pool.netbios_node_type ) {
        case VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_B:
            VTSS_RC( vtss_icfg_printf(result, " netbios-node-type b-node\n") );
            break;
        case VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_P:
            VTSS_RC( vtss_icfg_printf(result, " netbios-node-type p-node\n") );
            break;
        case VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_M:
            VTSS_RC( vtss_icfg_printf(result, " netbios-node-type m-node\n") );
            break;
        case VTSS_APPL_DHCP_SERVER_NETBIOS_NODE_TYPE_H:
            VTSS_RC( vtss_icfg_printf(result, " netbios-node-type h-node\n") );
            break;
        default:
            break;
        }

        /* COMMAND = netbios-scope <word128> */
        if ( icli_str_len(pool.netbios_scope) ) {
            VTSS_RC( vtss_icfg_printf(result, " netbios-scope %s\n", pool.netbios_scope) );
        }

        /* COMMAND = nis-domain-name <word128> */
        if ( icli_str_len(pool.nis_domain_name) ) {
            VTSS_RC( vtss_icfg_printf(result, " nis-domain-name %s\n", pool.nis_domain_name) );
        }

        /* COMMAND = nis-server <ipv4_ucast> [<ipv4_ucast> [<ipv4_ucast> [<ipv4_ucast>]]] */
        if ( pool.nis_server[0] ) {
            VTSS_RC( vtss_icfg_printf(result, " nis-server") );
            for ( i = 0; i < VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT; i++ ) {
                if ( pool.nis_server[i] == 0 ) {
                    break;
                }
                VTSS_RC( vtss_icfg_printf(result, " %s", icli_ipv4_to_str(pool.nis_server[i], str)) );
            }
            VTSS_RC( vtss_icfg_printf(result, "\n") );
        }

        /* COMMAND = client-identifier { { fqdn | name } <line128> | mac-address <mac_addr> } */
        switch ( pool.client_identifier.type ) {
        case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NONE:
            break;
        case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_NAME:
            VTSS_RC( vtss_icfg_printf(result, " client-identifier name %s\n", pool.client_identifier.u.name) );
            break;
        case VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_TYPE_MAC:
            VTSS_RC( vtss_icfg_printf(result, " client-identifier mac-address %s\n", icli_mac_to_str(pool.client_identifier.u.mac.addr, str)) );
            break;
        }

        /* COMMAND = hardware-address <mac_ucast> */
        if ( memcmp(pool.client_haddr.addr, empty_mac, DHCP_SERVER_MAC_LEN) ) {
            VTSS_RC( vtss_icfg_printf(result, " hardware-address %s\n", icli_mac_to_str(pool.client_haddr.addr, str)) );
        }

        /* COMMAND = client-name <word32> */
        if ( icli_str_len(pool.client_name) ) {
            VTSS_RC( vtss_icfg_printf(result, " client-name %s\n", pool.client_name) );
        }

        /* COMMAND = vendor class-identifier <string64> specific-info <hexval32> */
        for ( i = 0; i < VTSS_APPL_DHCP_SERVER_VENDOR_CLASS_INFO_CNT; i++ ) {
            if ( icli_str_len(pool.class_info[i].class_id) ) {
                VTSS_RC( vtss_icfg_printf(result, " vendor class-identifier") );
                VTSS_RC( vtss_icfg_printf(result, " \"%s\"", pool.class_info[i].class_id) );
                VTSS_RC( vtss_icfg_printf(result, " specific-info 0x") );
                for ( j = 0; j < pool.class_info[i].specific_info_len; j++ ) {
                    VTSS_RC( vtss_icfg_printf(result, "%02x", pool.class_info[i].specific_info[j]) );
                }
                VTSS_RC( vtss_icfg_printf(result, "\n") );
            }
        }

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
        if (pool.reserved_only) {
            VTSS_RC(vtss_icfg_printf(result, " reserved-only\n"));
        } else if (req->all_defaults) {
            VTSS_RC(vtss_icfg_printf(result, " no reserved-only\n"));
        }

        // address <ipv4_addr> interface <port_type_id>
        for (i = 0; i < VTSS_APPL_DHCP_SERVER_RESERVED_CNT; i++) {
            if (pool.reserved[i].ifindex != VTSS_IFINDEX_NONE) {
                vtss_ifindex_elm_t elm;
                char               if_name[64];

                (void)icli_ipv4_to_str(pool.reserved[i].address, str);
                (void)vtss_ifindex_decompose(pool.reserved[i].ifindex, &elm);
                (void)icli_port_info_txt(elm.usid, iport2uport(elm.ordinal), if_name);
                VTSS_RC(vtss_icfg_printf(result, " address %s interface %s\n", str, if_name));
            }
        }
#endif

        break;

    default:
        /* no config in other modes */
        break;
    }
    return VTSS_RC_OK;
}

/*
******************************************************************************

    Public functions

******************************************************************************
*/
/**
 * \brief Initialization function.
 *      Call once, preferably from the INIT_CMD_INIT section of
 *      the module's _init() function.
 */
mesa_rc dhcp_server_icfg_init(void)
{
    mesa_rc rc;

    /*
        Register Global config callback functions to ICFG module.
    */
    rc = vtss_icfg_query_register(VTSS_ICFG_GLOBAL_DHCP_SERVER, "dhcp_server", _dhcp_server_icfg);
    if ( rc != VTSS_RC_OK ) {
        T_D("Fail to register VTSS_ICFG_GLOBAL_DHCP_SERVER\n");
        return rc;
    }

    rc = vtss_icfg_query_register(VTSS_ICFG_INTERFACE_VLAN_DHCP_SERVER, "dhcp_server", _dhcp_server_icfg);
    if ( rc != VTSS_RC_OK ) {
        T_D("Fail to register VTSS_ICFG_INTERFACE_VLAN_DHCP_SERVER\n");
        return rc;
    }

    rc = vtss_icfg_query_register(VTSS_ICFG_DHCP_POOL_DHCP_SERVER, "dhcp_server", _dhcp_server_icfg);
    if ( rc != VTSS_RC_OK ) {
        T_D("Fail to register VTSS_ICFG_DHCP_POOL_DHCP_SERVER\n");
    }

    return rc;
}
