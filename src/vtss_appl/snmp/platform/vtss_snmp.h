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

#ifndef _VTSS_SNMP_H_
#define _VTSS_SNMP_H_

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#include "vtss_snmp_api.h"
#include "vtss_module_id.h"
#include "msg_api.h"
#include "critd_api.h"

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_SNMP

#define VTSS_TRACE_GRP_DEFAULT 0
#define TRACE_GRP_USERS        1
#define TRACE_GRP_TRAP         2

#include <vtss_trace_api.h>

#define SNMP_SMON_STAT_MAX_ROW_SIZE         128

/* ================================================================= *
 *  Dying gasp
 * ================================================================= */
struct DGMappingKey {
    char trap_name[TRAP_MAX_NAME_LEN + 1];

    bool operator== (const DGMappingKey &map_key) const
    {
        if (strcmp(trap_name, map_key.trap_name) == 0) {
            return true;
        }
        return false;
    }

    bool operator< (const DGMappingKey &map_key) const
    {
        if (strcmp(trap_name, map_key.trap_name) < 0) {
            return true;
        } else {
            return false;
        }
    }
};
struct DGValue {
    int     sysUpTimeValueOffset;
    uchar   snmp_pdu_buf[256];
    u32     snmp_pdu_length;
    BOOL    update;
    int     id;
};
typedef vtss::Map<DGMappingKey, DGValue> DGMap;

/* Databases and critical sections lock */
typedef struct {
    critd_t   crit;
    DGMap     map;
} dying_gasp_db_trap_entry_t;

/* ================================================================= *
 *  SNMP stack messages
 * ================================================================= */

/* SNMP messages IDs */
typedef enum {
    SNMP_MSG_ID_SNMP_CONF_SET_REQ,          /* SNMP configuration set request (no reply) */
} snmp_msg_id_t;

/* Request message */
typedef struct {
    /* Message ID */
    snmp_msg_id_t    msg_id;

    /* Request data, depending on message ID */
    union {
        /* SNMP_MSG_ID_SNMP_CONF_SET_REQ */
        struct {
            snmp_conf_t      conf;
        } conf_set;
    } req;
} snmp_msg_req_t;

/* SNMP message buffer */
typedef struct {
    vtss_sem_t *sem; /* Semaphore */
    void       *msg; /* Message */
} snmp_msg_buf_t;

/* ================================================================= *
 *  SNMP global structure
 * ================================================================= */

/* SNMP global structure */
typedef struct {
    critd_t                   crit;
    BOOL                      mode;
    CapArray<snmp_port_conf_t, VTSS_APPL_CAP_ISID_END, MEBA_CAP_BOARD_PORT_MAP_COUNT> snmp_port_conf;
    unsigned long             snmp_smon_stat_entry_num;
    snmp_rmon_stat_entry_t    snmp_smon_stat_entry[SNMP_SMON_STAT_MAX_ROW_SIZE];
    u32                       communities_conf_num;
    snmpv3_communities_conf_t communities_conf[SNMPV3_MAX_COMMUNITIES];

    /* Request buffer and semaphore */
    struct {
        vtss_sem_t     sem;
        snmp_msg_req_t msg;
    } request;

} snmp_global_t;


/* ================================================================= *
 *  SNMP module platform internal functions
 * ================================================================= */

extern snmp_global_t snmp_global;

void vtss_send_v2trap(const char *name, u32 index_len, oid *index, netsnmp_variable_list *t);

#endif /* _VTSS_SNMP_H_ */

