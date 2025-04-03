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

#ifndef _VTSS_MIRROR_H_
#define _VTSS_MIRROR_H_

/* ================================================================= *
 * Trace definitions
 * ================================================================= */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID    VTSS_MODULE_ID_RMIRROR
#define VTSS_ALLOC_MODULE_ID    VTSS_MODULE_ID_RMIRROR

#define VTSS_TRACE_GRP_DEFAULT 0
#define TRACE_GRP_ICLI         1

#include <vtss_trace_api.h>
/* ================================================================= *
 * Trace End
 * ================================================================= */

/** Maximum number of sessions */
#if defined(VTSS_SW_OPTION_RMIRROR)
#define VTSS_MIRROR_SESSION_MAX_CNT         5
#else
#define VTSS_MIRROR_SESSION_MAX_CNT         1
#endif /*  VTSS_SW_OPTION_RMIRROR */

/** Minimum ID of sessions */
#define VTSS_MIRROR_SESSION_MIN_ID          1

/** Maximum ID of sessions */
#define VTSS_MIRROR_SESSION_MAX_ID          (VTSS_MIRROR_SESSION_MIN_ID + VTSS_MIRROR_SESSION_MAX_CNT - 1)

/** Maximum number of mirror and RMirror source sessions */
#define VTSS_MIRROR_SESSION_SOURCE_CNT      1

/** The value is used when there are no active mirror and RMirror source sessions */
#define VTSS_MIRROR_SESSION_NULL_ID         0

#include "critd_api.h"
/* messages IDs */
typedef enum {
    RMIRROR_MSG_ID_CONF_SET_REQ,     /* Configuration set request (no reply) */
} rmirror_msg_id_t;

/* RMIRROR message buffer */
typedef struct {
    vtss_sem_t *sem;     /* Semaphore */
    uchar      *msg;     /* Message */
} rmirror_msg_buf_t;

/* RMIRROR remote configuration for stacking */
typedef struct {
    mesa_port_no_t reflector_port[VTSS_MIRROR_SESSION_MAX_CNT];            /* reflector port for source switch */
}  rmirror_local_switch_conf_t;

/* Message for configuration */
typedef struct {
    rmirror_msg_id_t                msg_id;        /* Message ID: RMIRROR_MSG_ID_CONF_SET_REQ */
    rmirror_local_switch_conf_t     conf;          /* Configuration */
} rmirror_conf_set_req_t;

/* RMIRROR stack configurations for single session */
typedef struct {
    mesa_vid_t              vid;                                            /* VLAN id*/
    vtss_appl_rmirror_switch_type_t   type;                                           /* switch type: source, intermediate, destination */
    vtss_isid_t             rmirror_switch_id;                              /* internal switch id for RMIRROR */
    mesa_port_no_t          reflector_port;                                 /* reflector port for source switch */
    CapArray<rmirror_source_port_t, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> source_port; /* source port for source switch */
    CapArray<BOOL, VTSS_APPL_CAP_VLAN_VID_CNT> source_vid;                       /* source VLANs for source switch */
    CapArray<BOOL, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> intermediate_port; /* Enable for intermediate link */
    CapArray<BOOL, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> destination_port;  /* monitor port for destination switch*/
    BOOL                    enabled;                                        /* enabled or disabled */
    BOOL                    cpu_src_enable[VTSS_ISID_CNT];                  /* Enable for CPU source mirroring */
    BOOL                    cpu_dst_enable[VTSS_ISID_CNT];                  /* Enable for CPU destination mirroring */
} rmirror_stack_conf_t;

inline int vtss_memcmp(const rmirror_stack_conf_t &a, const rmirror_stack_conf_t &b)
{
    VTSS_MEMCMP_ELEMENT(a, b, vid);
    VTSS_MEMCMP_ELEMENT(a, b, type);
    VTSS_MEMCMP_ELEMENT(a, b, rmirror_switch_id);
    VTSS_MEMCMP_ELEMENT(a, b, reflector_port);
    VTSS_MEMCMP_ELEMENT_CAP(a, b, source_port);
    VTSS_MEMCMP_ELEMENT_CAP(a, b, source_vid);
    VTSS_MEMCMP_ELEMENT_CAP(a, b, intermediate_port);
    VTSS_MEMCMP_ELEMENT_CAP(a, b, destination_port);
    VTSS_MEMCMP_ELEMENT(a, b, enabled);
    VTSS_MEMCMP_ELEMENT(a, b, cpu_src_enable);
    VTSS_MEMCMP_ELEMENT(a, b, cpu_dst_enable);
    return 0;
}

/* RMIRROR global configuration */
typedef struct {
    /* Critical region protection protecting the following block of variables */
    critd_t              crit;

    /* configuration for all switches in the stack */
    rmirror_stack_conf_t stack_conf[VTSS_MIRROR_SESSION_MAX_CNT];

    /* Request buffer and semaphore. Using the largest of the lot. */
    struct {
        vtss_sem_t       sem;
        uchar            msg[sizeof(rmirror_conf_set_req_t)];
    } request;

    /* Reply buffer and semaphore. Using the largest of the lot. */
    struct {
        vtss_sem_t       sem;
        uchar            msg[sizeof(rmirror_conf_set_req_t)];
    } reply;

} rmirror_global_t;

#endif /* _VTSS_MIRROR_H_ */

