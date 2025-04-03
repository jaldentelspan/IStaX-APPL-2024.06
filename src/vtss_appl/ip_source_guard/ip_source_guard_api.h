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

#ifndef _VTSS_IP_SOURCE_GUARD_API_H_
#define _VTSS_IP_SOURCE_GUARD_API_H_

#include "microchip/ethernet/switch/api.h" /* For mesa_rc, mesa_vid_t, etc. */

/* for public APIs */
#include "vtss/appl/ip_source_guard.h"

/* IP_SOURCE_GUARD management enabled/disabled */
#define IP_SOURCE_GUARD_MGMT_ENABLED            1
#define IP_SOURCE_GUARD_MGMT_DISABLED           0

//#define IP_SOURCE_GUARD_MAX_STATIC_CNT          28
//#define IP_SOURCE_GUARD_MAX_DYNAMIC_CNT         84
#define IP_SOURCE_GUARD_MAX_ENTRY_CNT           (112) // reserved 16 entry for system trap

#define IP_SOURCE_GUARD_DYNAMIC_UNLIMITED       0XFFFF
#define IP_SOURCE_GUARD_MAX_DYNAMIC_LIMITED_CNT     2

/**
 * \brief API Error Return Codes (mesa_rc)
 */
enum {
    IP_SOURCE_GUARD_MUST_BE_PRIMARY_SWITCH = MODULE_ERROR_START(VTSS_MODULE_ID_IP_SOURCE_GUARD), /**< Operation is only allowed on the primary switch. */
    IP_SOURCE_GUARD_ERROR_ISID,                                                                  /**< isid parameter is invalid.                       */
    IP_SOURCE_GUARD_ERROR_ISID_NON_EXISTING,                                                     /**< isid parameter is non-existing.                  */
    IP_SOURCE_GUARD_ERROR_INV_PARAM,                                                             /**< Invalid parameter.                               */
    IP_SOURCE_GUARD_ERROR_STATIC_TABLE_FULL,                                                     /**< IP source guard static table full.               */
    IP_SOURCE_GUARD_ERROR_DYNAMIC_TABLE_FULL,                                                    /**< IP source guard dynamic table full.              */
    IP_SOURCE_GUARD_ERROR_ACE_AUTO_ASSIGNED_FAIL,                                                /**< IP source guard ACE auto-assigned fail.          */
    IP_SOURCE_GUARD_ERROR_DATABASE_ACCESS,                                                       /**< Database access error.                           */
    IP_SOURCE_GUARD_ERROR_DATABASE_CREATE,                                                       /**< Database create error.                           */
    IP_SOURCE_GUARD_ERROR_ENTRY_EXIST_ON_DB,                                                     /**< The entry exist on DB.                           */
    IP_SOURCE_GUARD_ERROR_DATABASE_ADD,                                                          /**< The entry insert error on DB.                    */
    IP_SOURCE_GUARD_ERROR_DATABASE_DEL,                                                          /**< The entry delete error on DB.                    */
    IP_SOURCE_GUARD_ERROR_LOAD_CONF,                                                             /**< Open configuration error.                        */
    IP_SOURCE_GUARD_ERROR_OP_UNSUCCESSFUL                                                        /**< Operation unsuccessful.                          */
};

/**
 * Default configuration
 */
#define IP_SOURCE_GUARD_DEFAULT_MODE                IP_SOURCE_GUARD_MGMT_DISABLED       /**< Default global mode         */
#define IP_SOURCE_GUARD_DEFAULT_PORT_MODE           IP_SOURCE_GUARD_MGMT_DISABLED       /**< Default port mode           */
#define IP_SOURCE_GUARD_DEFAULT_DYNAMIC_ENTRY_CNT   IP_SOURCE_GUARD_DYNAMIC_UNLIMITED   /**< Default dynamic entry count */

/* IP source guard entry type */
typedef enum {
    IP_SOURCE_GUARD_STATIC_TYPE,
    IP_SOURCE_GUARD_DYNAMIC_TYPE
} ip_source_guard_entry_type_t;

typedef struct {
    mesa_vid_t                      vid;
    uchar                           assigned_mac[6];
    mesa_ipv4_t                     assigned_ip;
    mesa_ipv4_t                     ip_mask;
    vtss_isid_t                     isid;
    mesa_port_no_t                  port_no;
    ip_source_guard_entry_type_t    type;
    mesa_ace_id_t                   ace_id;
    ulong                           valid;
} ip_source_guard_entry_t;

typedef struct {
    CapArray<ulong, MEBA_CAP_BOARD_PORT_MAP_COUNT> mode;
} ip_source_guard_port_mode_conf_t;

inline int vtss_memcmp(const ip_source_guard_port_mode_conf_t &a, const ip_source_guard_port_mode_conf_t &b)
{
    VTSS_MEMCMP_ELEMENT_CAP(a, b, mode);
    return 0;
}

typedef struct {
    CapArray<ulong, MEBA_CAP_BOARD_PORT_MAP_COUNT> entry_cnt;
} ip_source_guard_port_dynamic_entry_conf_t;

inline int vtss_memcmp(const ip_source_guard_port_dynamic_entry_conf_t &a, const ip_source_guard_port_dynamic_entry_conf_t &b)
{
    VTSS_MEMCMP_ELEMENT_CAP(a, b, entry_cnt);
    return 0;
}

/* IP_SOURCE_GUARD configuration */
typedef struct {
    ulong                                       mode; /* IP Source Guard Global Mode */
    ip_source_guard_port_mode_conf_t            port_mode_conf[VTSS_ISID_CNT];
    ip_source_guard_port_dynamic_entry_conf_t   port_dynamic_entry_conf[VTSS_ISID_CNT];
    ip_source_guard_entry_t                     ip_source_guard_static_entry[IP_SOURCE_GUARD_MAX_ENTRY_CNT];
} ip_source_guard_conf_t;

inline int vtss_memcmp(const ip_source_guard_conf_t &a, const ip_source_guard_conf_t &b)
{
    VTSS_MEMCMP_ELEMENT_INT(a, b, mode);
    VTSS_MEMCMP_ELEMENT_ARRAY_RECURSIVE(a, b, port_mode_conf);
    VTSS_MEMCMP_ELEMENT_ARRAY_RECURSIVE(a, b, port_dynamic_entry_conf);
    VTSS_MEMCMP_ELEMENT_ARRAY(a, b, ip_source_guard_static_entry);
    return 0;
}

/* Set IP_SOURCE_GUARD defaults */
void ip_source_guard_default_set(ip_source_guard_conf_t *conf);

/* IP_SOURCE_GUARD error text */
const char *ip_source_guard_error_txt(mesa_rc rc);

/* Get IP_SOURCE_GUARD configuration */
mesa_rc ip_source_guard_mgmt_conf_get(ip_source_guard_conf_t *conf);

/* Set IP_SOURCE_GUARD configuration */
mesa_rc ip_source_guard_mgmt_conf_set(ip_source_guard_conf_t *conf);

/* Get IP_SOURCE_GUARD mode */
mesa_rc ip_source_guard_mgmt_conf_get_mode(ulong *mode);

/* Set IP_SOURCE_GUARD mode */
mesa_rc ip_source_guard_mgmt_conf_set_mode(ulong mode);

/* Set IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_set_static_entry(ip_source_guard_entry_t *entry);

/* Get first IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_get_first_static_entry(ip_source_guard_entry_t *entry);

/* Get IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_get_static_entry(ip_source_guard_entry_t *entry);

/* Get Next IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_get_next_static_entry(ip_source_guard_entry_t *entry);

/* Delete IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_del_static_entry(ip_source_guard_entry_t *entry);

/* Set IP_SOURCE_GUARD dynamic entry */
mesa_rc ip_source_guard_mgmt_conf_set_dynamic_entry(ip_source_guard_entry_t *entry);

/* Delete IP_SOURCE_GUARD dynamic entry */
mesa_rc ip_source_guard_mgmt_conf_del_dynamic_entry(ip_source_guard_entry_t *entry);

/* Get first IP_SOURCE_GUARD dynamic entry */
mesa_rc ip_source_guard_mgmt_conf_get_first_dynamic_entry(ip_source_guard_entry_t *entry);

/* Get IP_SOURCE_GUARD dynamic entry */
mesa_rc ip_source_guard_mgmt_conf_get_dynamic_entry(ip_source_guard_entry_t *entry);

/* Get Next IP_SOURCE_GUARD dynamic entry */
mesa_rc ip_source_guard_mgmt_conf_get_next_dynamic_entry(ip_source_guard_entry_t *entry);

/* del all IP_SOURCE_GUARD static entry */
mesa_rc ip_source_guard_mgmt_conf_del_all_static_entry(void);

/* set IP_SOURCE_GUARD port mode */
mesa_rc ip_source_guard_mgmt_conf_set_port_mode(vtss_isid_t isid, ip_source_guard_port_mode_conf_t *port_mode_conf);

/* get IP_SOURCE_GUARD port mode */
mesa_rc ip_source_guard_mgmt_conf_get_port_mode(vtss_isid_t isid, ip_source_guard_port_mode_conf_t *port_mode_conf);

/* set IP_SOURCE_GUARD port dynamic entry count */
mesa_rc ip_source_guard_mgmt_conf_set_port_dynamic_entry_cnt(vtss_isid_t isid, ip_source_guard_port_dynamic_entry_conf_t *port_dynamic_entry_conf);

/* get IP_SOURCE_GUARD port dynamic entry count */
mesa_rc ip_source_guard_mgmt_conf_get_port_dynamic_entry_cnt(vtss_isid_t isid, ip_source_guard_port_dynamic_entry_conf_t *port_dynamic_entry_conf);

/* Translate IP_SOURCE_GUARD dynamic entries into static entries */
mesa_rc ip_source_guard_mgmt_conf_translate_dynamic_into_static(void);

/* Initialize module */
mesa_rc ip_source_guard_init(vtss_init_data_t *data);

#endif /* _VTSS_IP_SOURCE_GUARD_API_H_ */

