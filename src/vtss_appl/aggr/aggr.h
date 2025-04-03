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

#ifndef _VTSS_AGGR_H_
#define _VTSS_AGGR_H_

/* ================================================================= *
 * Trace definitions
 * ================================================================= */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_AGGR
#define VTSS_TRACE_GRP_DEFAULT 0

#include <vtss_trace_api.h>

#include "aggr_api.h"
/* ================================================================= *
 *  Aggr local definitions
 * ================================================================= */
#define AGGR_ALL_GROUPS       999

/* Mem structs */
typedef struct {
    BOOL member[VTSS_PORT_BF_SIZE];
    mesa_port_speed_t  speed;
} aggr_conf_group_t;

/* Structure for the local switch */
typedef struct {
    CapArray<aggr_conf_group_t, VTSS_APPL_CAP_AGGR_MGMT_GROUPS> groups;
    mesa_aggr_mode_t mode;
} aggr_conf_aggr_t;

/* Structure for stack */
typedef struct {
    mesa_aggr_mode_t   mode;    /* Aggregation mmode */
    CapArray<aggr_conf_group_t, VTSS_APPL_CAP_ISID_END, VTSS_APPL_CAP_AGGR_MGMT_GROUPS> switch_id; /* All groups for all switches */
} aggr_conf_stack_aggr_t;

typedef struct {
    mesa_port_list_t member;
} aggr_glag_members_t;

/* ================================================================= *
 *  Aggr stack definitions (used with msg module)
 * ================================================================ */

/* AGGR messages IDs */
typedef enum {
    AGGR_MSG_ID_ADD_REQ,         /* Request  */
    AGGR_MSG_ID_DEL_REQ,         /* Request  */
    AGGR_MSG_ID_MODE_SET_REQ,    /* Request  */
    AGGR_MSG_ID_GLAG_UPDATE_REQ, /* Request  */
} aggr_msg_id_t;

/* AGGR_MSG_ID_ADD_REQ */
typedef struct {
    aggr_msg_id_t            msg_id;
    aggr_glag_members_t      members;
    aggr_mgmt_group_no_t     aggr_no;
    BOOL                     conf;
} aggr_msg_add_req_t;

/* AGGR_MSG_ID_AGGR_UPDATE_REQ */
typedef struct {
    aggr_msg_id_t            msg_id;
    aggr_mgmt_group_no_t     aggr_no;
    mesa_port_speed_t        grp_speed;
    aggr_mgmt_group_t        members;
} aggr_msg_glag_update_req_t;

/* AGGR_MSG_ID_DEL_REQ */
typedef struct {
    aggr_msg_id_t          msg_id;
    aggr_mgmt_group_no_t   aggr_no;
} aggr_msg_del_req_t;

/* AGGR_MSG_ID_MODE_SET_REQ */
typedef struct {
    aggr_msg_id_t          msg_id;
    mesa_aggr_mode_t       mode;
} aggr_msg_mode_set_req_t;

/* Aggr message buffer */
typedef struct {
    vtss_sem_t *sem; /* Semaphore */
    uchar      *msg; /* Message */
} aggr_msg_buf_t;

/* Request message for size calc */
typedef struct {
    union {
        aggr_msg_add_req_t         add;
        aggr_msg_glag_update_req_t glag;
        aggr_msg_del_req_t         del;
        aggr_msg_mode_set_req_t    set;
    } req;
} aggr_msg_req_t;

#ifdef VTSS_SW_OPTION_LACP
typedef struct {
    mesa_port_list_t members;
    aggr_mgmt_group_no_t aggr_no;
} aggr_lacp_t;
#endif /* VTSS_SW_OPTION_LACP */

/* ================================================================= *
 *  Aggr global struct
 * ================================================================= */
typedef struct {
    /* Request buffer and semaphore. Using the largest of the lot. */
    struct {
        vtss_sem_t sem;
        uchar      msg[sizeof(aggr_msg_req_t)];
    } request;

    u32                             aggr_callbacks;
    aggr_change_callback_t          callback_list[5];
    critd_t                         aggr_crit, aggr_cb_crit;

    CapArray<mesa_glag_no_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_glag_member;
    CapArray<aggr_mgmt_group_no_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_aggr_member;
    CapArray<aggr_mgmt_group_no_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_active_aggr_member;

    aggr_conf_aggr_t                aggr_config;
    CapArray<mesa_port_speed_t, VTSS_APPL_CAP_ISID_END, VTSS_APPL_CAP_AGGR_MGMT_GLAG_END> aggr_group_speed;
    aggr_conf_stack_aggr_t          aggr_config_stack;


    CapArray<aggr_mgmt_group_no_t, VTSS_APPL_CAP_ISID_END, MEBA_CAP_BOARD_PORT_MAP_COUNT> active_aggr_ports;
    CapArray<aggr_mgmt_group_no_t, VTSS_APPL_CAP_ISID_END, MEBA_CAP_BOARD_PORT_MAP_COUNT> config_aggr_ports;
    CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_link;

#ifdef VTSS_SW_OPTION_LACP
    CapArray<aggr_lacp_t,                 MEBA_CAP_BOARD_PORT_MAP_COUNT> aggr_lacp;
    CapArray<aggr_mgmt_group_t,           MEBA_CAP_BOARD_PORT_MAP_COUNT> lacp_group;
    CapArray<vtss_appl_aggr_group_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> group_mode;
    CapArray<aggr_mgmt_group_no_t,        VTSS_APPL_CAP_ISID_END, MEBA_CAP_BOARD_PORT_MAP_COUNT> lacp_config_ports;
    CapArray<BOOL,                        VTSS_APPL_CAP_ISID_END, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_state_fwd;
#endif /* VTSS_SW_OPTION_LACP */
    u32 aggr_group_cnt_end;
} aggr_global_t;

#define LACP_ONLY_ONE_MEMBER 999

#endif /* _VTSS_AGGR_H_ */

