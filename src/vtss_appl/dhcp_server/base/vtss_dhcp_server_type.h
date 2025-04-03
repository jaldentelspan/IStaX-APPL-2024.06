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
//----------------------------------------------------------------------------
/**
 *  \file
 *      vtss_dhcp_server_type.h
 *
 *  \brief
 *      type's definitions
 *
 *  \author
 *      CP Wang
 *
 *  \date
 *      05/08/2013 16:38
 */
//----------------------------------------------------------------------------
#ifndef __VTSS_DHCP_SERVER_TYPE_H__
#define __VTSS_DHCP_SERVER_TYPE_H__
//----------------------------------------------------------------------------

/*
==============================================================================

    Include File

==============================================================================
*/
#include <main.h>
#include <main_types.h>
#include <vtss/appl/dhcp_server.h>

/*
==============================================================================

    Constant

==============================================================================
*/
#define DHCP_SERVER_EXCLUDED_MAX_CNT            16              /**< Number of excluded IP address range */
#define DHCP_SERVER_BINDING_MAX_CNT             1024            /**< Number of binding */
#define DHCP_SERVER_POOL_MAX_CNT                64              /**< Number of DHCP pool. */
#define DHCP_SERVER_LEASE_DEFAULT_DAYS          1               /**< default lease time, days */
#define DHCP_SERVER_LEASE_DEFAULT_HOURS         0               /**< default lease time, hours */
#define DHCP_SERVER_LEASE_DEFAULT_MINUTES       0               /**< default lease time, minutes */
#define DHCP_SERVER_MESSAGE_MAX_LEN             2048            /**< maximum sent message length */
#define DHCP_SERVER_DEFAULT_MAX_MESSAGE_SIZE    576             /**< default max size of DHCP message from client */
#define DHCP_SERVER_ALLOCATION_EXPIRE_TIME      30              /**< expire time in second of allocation binding */
#define DHCP_SERVER_MAC_LEN                     6               /**< Length of MAC address */

/*
==============================================================================

    Macro

==============================================================================
*/
#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef INOUT
#define INOUT
#endif

/*
==============================================================================

    Type Definition

==============================================================================
*/
typedef int mesa_rc;

/**
 *  \brief
 *      Excluded IP database.
 *      index : low_ip, high_ip
 *      both of low_ip and high_ip can not be 0 at the same time.
 *      high_ip >= low_ip
 */
typedef struct {
    mesa_ipv4_t         low_ip;     /**< Begin excluded IP address */
    mesa_ipv4_t         high_ip;    /**< End excluded IP address */
} dhcp_server_excluded_ip_t;

/**
 *  \brief
 *      Vendor class information for option 43 and 60.
 *      If string length of class_id is 0, then this is not used.
 */
typedef struct {
    char        class_id[VTSS_APPL_DHCP_SERVER_VENDOR_CLASS_ID_LEN + 1];        /*!< Vendor class identifier               */
    u8          specific_info[VTSS_APPL_DHCP_SERVER_VENDOR_SPECIFIC_INFO_LEN];  /*!< Vendor specific information           */
    u32         specific_info_len;                                              /*!< Length of Vendor specific information */
} dhcp_server_vendor_class_info_t;

/**
 *  \brief
 *      Client identifier for option 61.
 *      Type 0: NAME is other than hardware type,
 *           1: MAC address.
 */
typedef struct {
    vtss_appl_dhcp_server_client_identifier_type_t      type;   /*!< type of identifier */
    union {
        char        name[VTSS_APPL_DHCP_SERVER_CLIENT_IDENTIFIER_NAME_LEN + 1];   /*!< NAME           */
        mesa_mac_t  mac;                                        /*!< MAC address    */
    } u;                                                        /*!< union of NAME and MAC address    */
} dhcp_server_client_identifier_t;

/**
 *  \brief
 *      DHCP IP allocation pool.
 *      index of name avlt              : name
 *      index of ip avlt                : ip & netmask
 *      index of client identifier avlt : identifier
 *      index of hardware address avlt  : chaddr
 */
typedef struct {
    /* configuration */
    char                                pool_name[VTSS_APPL_DHCP_SERVER_POOL_NAME_LEN + 4];     /**< Pool name */
    mesa_ipv4_t                         ip;                                                     /**< IP address */
    vtss_appl_dhcp_server_pool_type_t   type;                                                   /**< vtss_appl_dhcp_server_pool_type_t */

    // the followings are for options
    mesa_ipv4_t                                 subnet_mask;                                                /**< Subnet mask */
    mesa_ipv4_t                                 subnet_broadcast;                                           /**< Subnet broadcast address */
    mesa_ipv4_t                                 default_router[VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT];       /**< default router */
    u32                                         lease;                                                      /**< Lease time in second */
    char                                        domain_name[VTSS_APPL_DHCP_SERVER_DOMAIN_NAME_LEN + 4];     /**< Domain name */
    mesa_ipv4_t                                 dns_server[VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT];           /**< DNS server */
    mesa_ipv4_t                                 ntp_server[VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT];           /**< NTP server */
    mesa_ipv4_t                                 netbios_name_server[VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT];  /**< Netbios name server */
    vtss_appl_dhcp_server_netbios_node_type_t   netbios_node_type;                                          /**< vtss_appl_dhcp_server_netbios_node_type_t */
    char                                        netbios_scope[VTSS_APPL_DHCP_SERVER_DOMAIN_NAME_LEN + 4];   /**< Netbios scope */
    char                                        nis_domain_name[VTSS_APPL_DHCP_SERVER_DOMAIN_NAME_LEN + 4]; /**< NIS domain name */
    mesa_ipv4_t                                 nis_server[VTSS_APPL_DHCP_SERVER_SERVER_MAX_CNT];           /**< NIS server */

    dhcp_server_vendor_class_info_t             class_info[VTSS_APPL_DHCP_SERVER_VENDOR_CLASS_INFO_CNT];    /**< Vendor class information */

#ifdef VTSS_SW_OPTION_DHCP_SERVER_RESERVED_ADDRESSES
    // for reserved entries
    BOOL                                        reserved_only;                                              /**< Whether to only hand out reserved addresses (TRUE) or not (FALSE) */
    vtss_appl_dhcp_server_reserved_entry_t      reserved[VTSS_APPL_DHCP_SERVER_RESERVED_CNT];               /**< Reserved entries */
#endif

    // for host, manual binding
    dhcp_server_client_identifier_t             client_identifier;                                          /**< Client identifier */
    mesa_mac_t                                  client_haddr;                                               /**< Client hardware address */
    char                                        client_name[VTSS_APPL_DHCP_SERVER_HOST_NAME_LEN + 4];       /**< Client Host name */

    /* runtime data */
    u32             total_cnt;  /**< total number of IP in the pool, = ~netmask */
    u32             alloc_cnt;  /**< number of IP's allocated */
} dhcp_server_pool_t;

/**
 *  \brief
 *      DHCP server statistics.
 */
typedef struct {
    /* database counts */
    u32             pool_cnt;               /**< Number of IP pools       */
    u32             excluded_cnt;           /**< Number of excluded range */
    u32             declined_cnt;           /**< Number of declined IP    */

    /* binding counts */
    u32             automatic_binding_cnt;  /**< Number of automatic bindings */
    u32             manual_binding_cnt;     /**< Number of manual bindings    */
    u32             expired_binding_cnt;    /**< Number of expired bindings   */

    u32             allocated_state_cnt;    /**< Number of allocated state bindings */
    u32             committed_state_cnt;    /**< Number of committed state bindings */
    u32             expired_state_cnt;      /**< Number of expired state bindings   */

    /* the following fields can be cleared by UI */

    /* message counts */
    u32             discover_cnt; /**< Number of Discover packets   */
    u32             offer_cnt;    /**< Number of Offer packets      */
    u32             request_cnt;  /**< Number of Request packets    */
    u32             ack_cnt;      /**< Number of ACK packets        */
    u32             nak_cnt;      /**< Number of NACK packets       */
    u32             decline_cnt;  /**< Number of Decline packets    */
    u32             release_cnt;  /**< Number of Release packets    */
    u32             inform_cnt;   /**< Number of Inform packets     */
} dhcp_server_statistics_t;

/**
 *  \brief
 *      DHCP binding.
 *          index of binding list : ip
 *          index of binding list : client_id
 *          index of binding list : pool_name, ip
 *          index of binding list : expire_time
 */
typedef struct {
    mesa_ipv4_t                                 ip;             /**< IP address allocated */
    mesa_ipv4_t                                 subnet_mask;    /**< Subnet mask */
    vtss_appl_dhcp_server_binding_state_t       state;          /**< vtss_appl_dhcp_server_binding_state_t */
    vtss_appl_dhcp_server_binding_type_t        type;           /**< vtss_appl_dhcp_server_binding_type_t */
    dhcp_server_pool_t                          *pool;          /**< IP pool that binding belongs to */
    mesa_ipv4_t                                 server_id;      /**< Server identifier to check if this is mine */
    mesa_vid_t                                  vid;            /**< VLAN */
    dhcp_server_client_identifier_t             identifier;     /**< Client identifier */
    mesa_mac_t                                  chaddr;         /**< Client hardware address */
    u32                                         lease;          /**< Lease time of the binding in sec */
    u32                                         time_to_start;  /**< system time to start allocation or lease in sec */
    vtss_ifindex_t                              ifindex;        /**< Port interface this binding is offered on */

    //
    // if STATE_ALLOCATED, expire_time = time_to_start + DHCP_SERVER_ALLOCATION_EXPIRE_TIME
    // if STATE_COMMITTED, expire_time = time_to_start + lease
    //
    u32                                         expire_time;    /**< when the binding expires in sec */
} dhcp_server_binding_t;

//----------------------------------------------------------------------------
#endif //__VTSS_DHCP_SERVER_TYPE_H__
