/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_MAC_H_
#define _VTSS_MAC_H_

#include "microchip/ethernet/switch/api.h"
#define MAC_NON_VOLATILE 0
#define MAC_VOLATILE 1
#define MAC_MGMT_VLAN_ID_MIN 1
#define MAC_MGMT_VLAN_ID_MAX 4095


/* ================================================================= *
 * Trace definitions
 * ================================================================= */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_MAC

#include <vtss_trace_api.h>
VTSS_PRE_DECLS void mac_mib_init();
#ifdef VTSS_SW_OPTION_JSON_RPC
void vtss_appl_mac_json_init();
#endif
/* ================================================================= *
 *  Mac age local definitions
 * ================================================================= */
/* Structure for temporary age time change */
typedef struct {
    mac_age_conf_t temp_conf;
    mac_age_conf_t stored_conf;
    ulong age_period;
} age_time_temp_t;

/* ================================================================= *
 *  Static Mac Addresses local definitions
 * ================================================================= */

#define MAC_ADDR_NOT_INCL_STACK ((char)1<<0)
#define MAC_ADDR_DYNAMIC ((char)1<<1)

/* MAC address properties */
typedef struct {
    mesa_vid_mac_t        vid_mac;                           /* VLAN ID and MAC addr */
    char                  destination[VTSS_PORT_BF_SIZE];    /* Bit port mask */
    vtss_isid_t           isid;
    char                  mode;
    BOOL                  copy_to_cpu;
} mac_entry_conf_t;

/* MAC static entry pointer list - kept in memory */
typedef struct mac_static_t {
    struct mac_static_t   *next;    /* Next in list */
    mac_entry_conf_t      conf;     /* Configuration */
} mac_static_t;

/* ================================================================= *
 *  Mac learn mode definition
 * ================================================================= */
/* Bitmask structure to save space */
typedef struct {
    ulong              version;                                      /* Block version                         */
    char learn_mode[VTSS_ISID_END][VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD + 1][1]; /* Bit port mask                         */
} mac_conf_learn_mode_t;

/* Bit placements */
#define LEARN_AUTOMATIC 0
#define LEARN_CPU       1
#define LEARN_DISCARD   2
#define MAC_ALL_PORTS  99

/* ================================================================= *
 *  Mac age stack definitions (used with msg module)
 * ================================================================ */

/* MAC messages IDs */
typedef enum {
    MAC_MSG_ID_AGE_SET_REQ,           /* request */
    MAC_MSG_ID_GET_NEXT_REQ,          /* request */
    MAC_MSG_ID_GET_NEXT_REP,          /* reply   */
    MAC_MSG_ID_GET_NEXT_STACK_REQ,    /* request */
    MAC_MSG_ID_GET_STATS_REQ,         /* request */
    MAC_MSG_ID_GET_STATS_REP,         /* reply   */
    MAC_MSG_ID_ADD_REQ,               /* request */
    MAC_MSG_ID_DEL_REQ,               /* request */
    MAC_MSG_ID_LEARN_REQ,             /* request */
    MAC_MSG_ID_FLUSH_REQ,             /* request */
    MAC_MSG_ID_LOCKED_DEL_REQ,        /* request */
    MAC_MSG_ID_UPSID_FLUSH_REQ,       /* request */
    MAC_MSG_ID_UPSID_UPSPN_FLUSH_REQ, /* request */
    MAC_MSG_ID_VLAN_LEARN_REQ,        /* request */
} mac_msg_id_t;

/* MAC message request timer in secs */
#define MAC_REQ_TIMEOUT 20

/* Request message */
typedef struct {
    /* Message ID */
    mac_msg_id_t msg_id;

    union {
        /* MAC_MSG_ID_AGE_SET_REQ */
        struct {
            mac_age_conf_t conf;               /* Configuration */
        } age_set;


        /* MAC_MSG_ID_GET_NEXT_REQ */
        struct {
            mesa_vid_mac_t vid_mac;            /* Mac addr query */
            BOOL           next;               /* Get next or lookup */
        } get_next;

        /* MAC_MSG_ID_GET_STATS_REQ */
        struct {
            mesa_vid_t             vlan;
        } stats;

        /* MAC_MSG_ID_ADD_REQ */
        struct {
            mac_mgmt_addr_entry_t  entry;      /* Address to add */
        } add;

        /* MAC_MSG_ID_DEL_REQ */
        struct {
            mesa_vid_mac_t         vid_mac;    /* Mac addr to del */
            BOOL                   vol;        /* Volatile? */
        } del;

        /* MAC_MSG_ID_LEARN_REQ */
        struct {
            mesa_port_no_t         port_no;    /* Port to set */
            mesa_learn_mode_t      learn_mode; /* Learn mode  */
        } learn;

        struct {
            mesa_vid_t         vid;    /* VLAN to set */
            vtss_appl_mac_vid_learn_mode_t   learn_mode; /* Learn mode  */
        } vlan_learn;

        /* MAC_MSG_ID_FLUSH_REQ */
        /* No data */

        /* MAC_MSG_ID_LOCKED_DEL_REQ */
        /* No data */

    } req;
} mac_msg_req_t;

/* Reply message */
typedef struct {
    /* Message ID */
    mac_msg_id_t msg_id;

    union {
        /* MAC_MSG_ID_GET_NEXT_REP */
        struct {
            mesa_mac_table_entry_t entry;
            mesa_rc                rc;         /* Return code */
        } get_next;

        /* MAC_MSG_ID_GET_STATS_REP */
        struct {
            mac_table_stats_t      stats;
            mesa_rc                rc;         /* Return code */
        } stats;
    } rep;
} mac_msg_rep_t;

/* ================================================================= *
 *  Mac global struct
 * ================================================================= */
typedef struct {
    /* Thread variables */
    vtss_handle_t        thread_handle;
    vtss_thread_t        thread_block;

    /* Critical region mac_ram_table */
    critd_t              ram_crit;

    /* Critical region protection protecting the following block of variables */
    critd_t              crit;

    /* Mac age configuration */
    mac_age_conf_t       conf;

    /* Get Next reply from secondary switch */
    struct {
        mesa_mac_table_entry_t entry;
        mesa_rc                rc;
    } get_next;

    /* Get stats reply from secondary switch */
    struct {
        mac_table_stats_t      stats;
        mesa_rc                rc;
    } get_stats;

    /* Request buffer message pool */
    void *request;

    /* Reply buffer message pool */
    void *reply;

    /* mac_address flush table */
    BOOL flush_port_ignore[VTSS_ISID_END][VTSS_MAX_PORTS_LEGACY_CONSTANT_USE_CAPARRAY_INSTEAD];

} mac_global_t;

#endif /* _VTSS_MAC_H_ */

