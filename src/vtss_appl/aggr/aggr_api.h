/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_AGGR_API_H_
#define _VTSS_AGGR_API_H_

#include "vtss/appl/aggr.h"

/* In this module LAGS start from 1 and GLAGS start where the LAGS ends.
   (In the vtss api the LAGS and GLAGS start from 0)
   To browse all aggregations use AGGR_MGMT_GROUP_NO_START to AGGR_MGMT_GROUP_NO_END.
*/

#undef VTSS_GLAG_PLUS_LLAG_SUPPORT /* if enabled, only supported on stackable switch(s) */


/* Aggregation numbers starting from 1 */
#define AGGR_MGMT_GROUP_NO_START   1
#define AGGR_MGMT_GLAG_START       (AGGR_MGMT_GROUP_NO_START + AGGR_LLAG_CNT)
#define AGGR_MGMT_GLAG_END         (AGGR_MGMT_GLAG_START + AGGR_GLAG_CNT)
#define AGGR_MGMT_GROUP_NO_END     (AGGR_MGMT_GLAG_END)

/* Determine the number of LLAGs, GLAGs and LAGs */
#define AGGR_UPLINK_NONE 0xFFFF
#define AGGR_LLAG_CNT VTSS_AGGRS       /* Standalone: LLAGs available */
#define AGGR_UPLINK   AGGR_UPLINK_NONE /* Standalone: No reserved LLAG */
#define AGGR_GLAG_CNT 0                /* Standalone: No GLAGs available */
#define AGGR_LAG_CNT (AGGR_LLAG_CNT + AGGR_GLAG_CNT)


#define AGGR_LLAG_CNT_              (((int)fast_cap(MEBA_CAP_BOARD_PORT_COUNT))/2)
#define AGGR_MGMT_GLAG_START_       (AGGR_MGMT_GROUP_NO_START + AGGR_LLAG_CNT_)
#define AGGR_MGMT_GLAG_END_         (AGGR_MGMT_GLAG_START_ + AGGR_GLAG_CNT)
#define AGGR_MGMT_GROUP_NO_END_     (AGGR_MGMT_GLAG_END_)
#define AGGR_MGMT_GROUP_NO_LAST_    (AGGR_MGMT_GLAG_END_ - AGGR_MGMT_GROUP_NO_START) // The last legal group id


/* Determine whether x is LAG/GLAG/AGGR */
#define AGGR_MGMT_GROUP_IS_LAG(x)  ((x) >= AGGR_MGMT_GROUP_NO_START && (x) < AGGR_MGMT_GLAG_START_ && (x) != AGGR_UPLINK)
#define AGGR_MGMT_GROUP_IS_GLAG(x) ((x) >= AGGR_MGMT_GLAG_START_ && (x) < aggr_mgmt_group_no_end())
#define AGGR_MGMT_GROUP_IS_AGGR(x) (AGGR_MGMT_GROUP_IS_LAG(x) || AGGR_MGMT_GROUP_IS_GLAG(x))
#define AGGR_MGMT_GROUP_IS_SUPPORTED(x) ((x >= AGGR_MGMT_GROUP_NO_START) && (x <aggr_mgmt_group_no_end()))

/* Use for management where LAGs and GLAG starts from 1 */
#define AGGR_MGMT_NO_TO_ID(x) ((x<AGGR_MGMT_GROUP_NO_START||x>AGGR_MGMT_GLAG_END_)?0:(x>=AGGR_MGMT_GLAG_START_?(x-AGGR_MGMT_GLAG_START_+1):x))
#define AGGR_MGMT_GLAG_PORTS_MAX   8 /* JR always supports max 8 ports in a GLAG */
/* Number of ports in LLAG/GLAG */
#define AGGR_MGMT_LAG_PORTS_MAX_   (/*lint -e(506)*/((int)fast_cap(MEBA_CAP_BOARD_PORT_COUNT) < 16) ?  (int)fast_cap(MEBA_CAP_BOARD_PORT_COUNT) : 16)

/* Type which holds ports and aggregation port groups id's */
typedef mesa_aggr_no_t aggr_mgmt_group_no_t;
typedef unsigned int l2_port_no_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Aggr API error codes (mesa_rc) */
enum {
    AGGR_ERROR_GEN = MODULE_ERROR_START(VTSS_MODULE_ID_AGGR),  /* Generic error code */
    AGGR_ERROR_PARM,           /* Illegal parameter                      */
    AGGR_ERROR_REG_TABLE_FULL, /* Registration table full                */
    AGGR_ERROR_REQ_TIMEOUT,    /* Timeout on message request             */
    AGGR_ERROR_STACK_STATE,    /* Illegal primary/secondary switch state */
    AGGR_ERROR_NOT_READY,      /* Not ready                              */
    AGGR_ERROR_GROUP_IN_USE,   /* Group already in use                   */
    AGGR_ERROR_PORT_IN_GROUP,  /* Port is already a member               */
    AGGR_ERROR_LACP_ENABLED,   /* Static aggregation is enabled          */
    AGGR_ERROR_DOT1X_ENABLED,  /* DOT1X is enabled                       */
    AGGR_ERROR_ENTRY_NOT_FOUND,/* Entry not found                        */
    AGGR_ERROR_HDX_SPEED_ERROR,/* Illegal duplex or speed state          */
    AGGR_ERROR_MEMBER_OVERFLOW,/* To many port members                   */
    AGGR_ERROR_INVALID_ID,     /* Invalid group id                       */
    AGGR_ERROR_INVALID_PORT,   /* Invalid port                           */
    AGGR_ERROR_INVALID_ISID,   /* Invalid ISID                           */
    AGGR_ERROR_INVALID_MODE    /* Invalid or no mode                     */
};

/* port participation type */
typedef enum {
    PORT_PARTICIPATION_TYPE_PORT,
    PORT_PARTICIPATION_TYPE_STATIC,
    PORT_PARTICIPATION_TYPE_LACP
} vtss_port_participation_t;

/* Initialize module */
mesa_rc aggr_init(vtss_init_data_t *data);

/* Mgmt API structs */
typedef struct {
    mesa_port_list_t member; /**< The member list for an aggregation group     */
} aggr_mgmt_group_t;

typedef struct {
    aggr_mgmt_group_t    entry;        /**< Aggregation entry associated with the aggr_no */
    mesa_aggr_no_t       aggr_no;      /**< The number of this aggregation group          */
} aggr_mgmt_group_member_t;

/* aggr_mgmt_members_get:
 * Get members of a aggr group (both static and dynamic created).
 * 'aggr_no' : Aggregation group id. 'aggr_no=0' and 'next=1' gets the group and members of the first active group.
 *           :'aggr_no='x' and 'next=1' gets the group and members of the group after 'x'
 * 'members' : Points to the updated group and memberlist.
 *  'next'   : Next = 0 returns the memberlist of the group.   next = 1 see above.
 *
 *  Note: Ports which are members of a statically created group - but without a portlink
 *  will not be included in the returned portlist. */
mesa_rc aggr_mgmt_members_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_member_t *members, BOOL next);

/*  Same functionality as for <aggr_mgmt_members_get> - only for STATIC created groups
 *  Note: Ports which are members of aggregation group will always be included in the returned portlist,
 *  even though the portlink is down.*/
mesa_rc aggr_mgmt_port_members_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_member_t *members, BOOL next);

/* Adds member(s) or modifies a group. Per switch */
/* Members must be in the same speed and in full duplex */
mesa_rc aggr_mgmt_port_members_add(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no, aggr_mgmt_group_t *members);

/* Deletes all members from a group. Per switch  */
mesa_rc aggr_mgmt_group_del(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no);

/* Set the aggregation mode for all groups. See the 'mesa_aggr_mode_t' description in vtss.h */
/* All switches in the stack will be configured with the same aggr mode */
mesa_rc aggr_mgmt_aggr_mode_set(mesa_aggr_mode_t *mode);

/* Returns the aggregation mode (same for all switches in a stack)  */
mesa_rc aggr_mgmt_aggr_mode_get(mesa_aggr_mode_t *mode);

/* Register for aggregation changes */
typedef void (*aggr_change_callback_t)(vtss_isid_t isid, uint aggr_no);
void aggr_change_register(aggr_change_callback_t cb);

/* Returns the LLAG/GLAG id for the given port. Returns VTSS_AGGR_NO_NONE if the port is down or not aggregated */
aggr_mgmt_group_no_t aggr_mgmt_get_aggr_id(vtss_isid_t isid, mesa_port_no_t port_no);

/* Returns the LLAG/GLAG id for the given port. Returns 0 if the port is not aggregated */
aggr_mgmt_group_no_t aggr_mgmt_get_port_aggr_id(vtss_isid_t isid, mesa_port_no_t port_no);

/* Returns the (LACP)LLAG id for the given port. Returns 0 if the port is not LACP aggregated */
aggr_mgmt_group_no_t aggr_lacp_mgmt_get_port_aggr_id(vtss_isid_t isid, mesa_port_no_t port_no);

/* Returns information if the port is participating in LACP or Static aggregation, */
/* regardless of link status.                                                      */
/* 0 = No participation                                                            */
/* 1 = Static aggregation participation                                            */
/* 2 = LACP aggregation participation                                              */
vtss_port_participation_t aggr_mgmt_port_participation(vtss_isid_t isid, mesa_port_no_t port_no);

/* Returns the speed of the group  */
mesa_port_speed_t aggr_mgmt_speed_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no);

/* 'AGGR_MGMT_GROUP_NO_END' represents the max group number based on the 'VTSS_PORTS' macro */
/* aggr_mgmt_group_no_end() return the actual max groups based on how many fronports (frontports/2) has been defined in the portmap */
aggr_mgmt_group_no_t aggr_mgmt_group_no_end(void);

void aggr_mgmt_test(void);

/* aggr error text */
const char *aggr_error_txt(mesa_rc rc);

/* Get the state of a port_list_stackable index */
BOOL portlist_state_get(vtss_isid_t isid, mesa_port_no_t port_no, const vtss_port_list_stackable_t *pl);

/* Set the state of a port_list_stackable index */
BOOL portlist_state_set(vtss_isid_t isid, mesa_port_no_t port_no, vtss_port_list_stackable_t *pl);

/* Clear the state of a port_list_stackable index */
BOOL portlist_state_clr(vtss_isid_t isid, mesa_port_no_t port_no, vtss_port_list_stackable_t *pl);

/* Debug dump */
typedef int (*aggr_dbg_printf_t)(const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));
void aggr_mgmt_dump(aggr_dbg_printf_t dbg_printf);


#ifdef VTSS_SW_OPTION_LACP
/****************************/
/* LACP specific functions: */
/****************************/

/* LACP function. Add/Remove port to LACP configuration (not HW)  Use 'aggr-no = VTSS_AGGR_NO_NONE' when deleting,*/
mesa_rc aggr_mgmt_lacp_member_set(vtss_isid_t isid, mesa_port_no_t port_no, aggr_mgmt_group_no_t aggr_no);

/* LACP function. Add/remove member to/from HW aggregation */
mesa_rc aggr_mgmt_lacp_member_add(uint aid, l2_port_no_t l2port, BOOL enable);

/* Same functionality as for <aggr_mgmt_port_members_get> - only for LACP created groups */
mesa_rc aggr_mgmt_lacp_members_get(vtss_isid_t isid, aggr_mgmt_group_no_t aggr_no,  aggr_mgmt_group_member_t *members, BOOL next);

/* Convert the lacp 'core' aggregation id into Aggr API id */
aggr_mgmt_group_no_t lacp_to_aggr_id(int aid);

/* Loop through LACP id's in use */
mesa_rc aggr_mgmt_lacp_id_get_next(int *search_aid, int *return_aid);

/* filters the aggregation key type, for a given platform */
mesa_rc validate_aggr_index(vtss_ifindex_t ifindex, vtss_ifindex_elm_t *ifep);
#endif /* VTSS_SW_OPTION_LACP */

#ifdef __cplusplus
}
#endif

#endif /* _VTSS_AGGR_API_H_ */

