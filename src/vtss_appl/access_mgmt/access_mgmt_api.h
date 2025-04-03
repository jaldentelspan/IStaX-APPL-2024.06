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

#ifndef _ACCESS_MGMT_API_H_
#define _ACCESS_MGMT_API_H_

#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_t, etc. */
#include "vtss/appl/access_management.h"

/**
 * \file access_mgmt_api.h
 * \brief This file defines the APIs for the Access Management module
 */

#ifdef __cplusplus
extern "C" {
#endif

#define ACCESS_MGMT_ACCESS_ID_START         (1)
#define ACCESS_MGMT_MAX_ENTRIES             (16)

/**
 * Access management enabled/disabled
 */
#define ACCESS_MGMT_ENABLED                 (1) /**< Enable option  */
#define ACCESS_MGMT_DISABLED                (0) /**< Disable option */

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    ACCESS_MGMT_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_ACCESS_MGMT), /* Generic error code */
    ACCESS_MGMT_ERROR_PARM,                                                 /* Illegal parameter */
    ACCESS_MGMT_ERROR_STACK_STATE,                                          /* Illegal primary/secondary switch state */
    ACCESS_MGMT_ERROR_GET_CERT_INFO,                                        /* Illegal get certificate information */
    ACCESS_MGMT_ERROR_NULL_PARM,                                            /* Illegal NULL parameter */
    ACCESS_MGMT_ERROR_NOT_IPV4_INPUT_TYPE,                                  /* Not a Ipv4 entry type*/
    ACCESS_MGMT_ERROR_DUPLICATED_CONTENT,                                   /* Duplicated*/
    ACCESS_MGMT_ERROR_INVALID_SERVICE,                                      /* Invalid*/
};

typedef enum {
    ACCESS_MGMT_INTERNAL_TYPE_NONE = 0,
    ACCESS_MGMT_INTERNAL_TYPE_DNS,
    ACCESS_MGMT_INTERNAL_TYPE_DHCP,
    ACCESS_MGMT_INTERNAL_TYPE_SYSTEM,
    ACCESS_MGMT_INTERNAL_TYPE_OTHER
} access_mgmt_internal_t;

typedef enum {
    ACCESS_MGMT_PROTOCOL_DEFAULT = 0,
    ACCESS_MGMT_PROTOCOL_TCP_UDP,
    ACCESS_MGMT_PROTOCOL_OTHER
} access_mgmt_ip_protocol_t;

/* Access management services type */
#define ACCESS_MGMT_SERVICES_TYPE_TELNET    0x1
#define ACCESS_MGMT_SERVICES_TYPE_WEB       0x2
#define ACCESS_MGMT_SERVICES_TYPE_SNMP      0x4
#define ACCESS_MGMT_SERVICES_TYPE           (ACCESS_MGMT_SERVICES_TYPE_TELNET | ACCESS_MGMT_SERVICES_TYPE_WEB | ACCESS_MGMT_SERVICES_TYPE_SNMP)

/* Access management entry type */
#define ACCESS_MGMT_ENTRY_TYPE_IPV4         0x0
#define ACCESS_MGMT_ENTRY_TYPE_IPV6         0x1

/**
 * \brief Access Management entry.
 */
typedef struct {
    BOOL        valid;
    u32         service_type;
    u32         entry_type;
    mesa_ipv4_t start_ip;
    mesa_ipv4_t end_ip;
#ifdef VTSS_SW_OPTION_IPV6
    mesa_ipv6_t start_ipv6;
    mesa_ipv6_t end_ipv6;
#endif /* VTSS_SW_OPTION_IPV6 */
    mesa_vid_t  vid;
} access_mgmt_entry_t;

/**
 * \brief Inter-Module Access Management Entry.
 */
typedef struct {
    access_mgmt_internal_t      source;         /* Request source module: KEY1 */
    access_mgmt_ip_protocol_t   protocol;       /* Protocol selection */
    mesa_ip_addr_t              start_src;      /* Start source address: Address Range */
    mesa_ip_addr_t              end_src;        /* End source address: Address Range */
    mesa_ip_addr_t              start_dst;      /* Start destination address: Address Range */
    mesa_ip_addr_t              end_dst;        /* End destination address: Address Range */
    u16                         start_sport;    /* Start source protocol port: Multiport Range */
    u16                         end_sport;      /* End source protocol port: Multiport Range */
    u16                         start_dport;    /* Start destination protocol port: Multiport Range */
    u16                         end_dport;      /* End destination protocol port: Multiport Range */
    mesa_vid_t                  vidx;           /* IP VLAN interface ID */
    BOOL                        operation;      /* Permit or Drop */
    u8                          priority;       /* Priority of the entry managed by 'source': KEY2 (TODO) */
} access_mgmt_inter_module_entry_t;

/**
 * \brief Access Management configuration.
 */
typedef struct {
    u32                 mode;
    u32                 entry_num;
    access_mgmt_entry_t entry[ACCESS_MGMT_ACCESS_ID_START + ACCESS_MGMT_MAX_ENTRIES];
} access_mgmt_conf_t;

/**
 * \brief Access Management statistics.
 */
typedef struct {
    u32 http_receive_cnt;
    u32 http_discard_cnt;

    u32 https_receive_cnt;
    u32 https_discard_cnt;
    u32 snmp_receive_cnt;
    u32 snmp_discard_cnt;
    u32 telnet_receive_cnt;
    u32 telnet_discard_cnt;
    u32 ssh_receive_cnt;
    u32 ssh_discard_cnt;
} access_mgmt_stats_t;

/**
  * \brief Retrieve an error string based on a return code
  *        from one of the Access Management API functions.
  *
  * \param rc [IN]: Error code that must be in the ACCESS_MGMT_ERROR_xxx range.
  */
const char *access_mgmt_error_txt(mesa_rc rc);

/* Get access management configuration */
mesa_rc access_mgmt_conf_get(access_mgmt_conf_t *conf);

/* Set access management configuration */
mesa_rc access_mgmt_conf_set(access_mgmt_conf_t *conf);

/**
  * \brief Initialize the Access Management module
  *
  * \param data [IN]: Initial data point.
  *
  * \return
  *    VTSS_RC_OK.
  */
mesa_rc access_mgmt_init(vtss_init_data_t *data);

/* Get access management entry */
mesa_rc access_mgmt_entry_get(int access_id, access_mgmt_entry_t *entry);

/* Add access management entry
   valid access_id: ACCESS_MGMT_ACCESS_ID_START ~ ACCESS_MGMT_MAX_ENTRIES */
mesa_rc access_mgmt_entry_add(int access_id, access_mgmt_entry_t *entry);

/* Delete access management entry */
mesa_rc access_mgmt_entry_del(int access_id);

/* Clear access management entry */
mesa_rc access_mgmt_entry_clear(void);

/* Get access management statistics */
void access_mgmt_stats_get(access_mgmt_stats_t *stats);

/* Clear access management statistics */
void access_mgmt_stats_clear(void);

/* Check if entry content is the same as others
   Retrun: 0 - no duplicated, others - duplicated access_id */
int access_mgmt_entry_content_is_duplicated(int access_id, access_mgmt_entry_t *entry);

/* Internal (Inter-Module) access management entry add */
mesa_rc access_mgmt_internal_entry_add(const access_mgmt_inter_module_entry_t *const entry);

/* Internal (Inter-Module) access management entry delete */
mesa_rc access_mgmt_internal_entry_del(const access_mgmt_inter_module_entry_t *const entry);

/* Internal (Inter-Module) access management entry update */
mesa_rc access_mgmt_internal_entry_upd(const access_mgmt_inter_module_entry_t *const entry);

/* Internal (Inter-Module) access management entry get */
mesa_rc access_mgmt_internal_entry_get(access_mgmt_inter_module_entry_t *const entry);

#ifdef __cplusplus
}
#endif
#endif /* _ACCESS_MGMT_API_H_ */

