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

#ifndef _VTSS_MIRROR_BASIC_H_
#define _VTSS_MIRROR_BASIC_H_

/* ================================================================= *
 * Trace definitions
 * ================================================================= */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_MIRROR
#define VTSS_TRACE_GRP_DEFAULT 0
#define TRACE_GRP_CLI          1

#include <vtss_trace_api.h>
/* ============== */
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_MIRROR

#include "critd_api.h"
/* messages IDs */
typedef enum {
    MIRROR_MSG_ID_CONF_SET_REQ,     /* Configuration set request (no reply) */
} mirror_msg_id_t;


/* Mac message buffer */
typedef struct {
    vtss_sem_t *sem; /* Semaphore */
    uchar      *msg; /* Message */
} mirror_msg_buf_t;

typedef struct {
    mesa_port_no_t   mirror_port;    /* Mirroring port. Port 0 is disable mirroring */
    mesa_port_list_t src_enable;     // Enable for source mirroring
    mesa_port_list_t dst_enable;     // Enable for detination mirroring
    BOOL             cpu_dst_enable; // Enable for CPU source mirroring
    BOOL             cpu_src_enable; // Enable for CPU destination mirroring
}  mirror_local_switch_conf_t;

// Message for configuration
typedef struct {
    mirror_msg_id_t             msg_id;        /* Message ID: MAC_AGE_MSG_ID_CONFIG_SET_REQ */
    mirror_local_switch_conf_t  conf;          /* Configuration */
} mirror_conf_set_req_t;

/* Mirror configuration */
typedef struct {
    ulong                version; /* Block version */
    vtss_isid_t    mirror_switch;              // Switch for the switch with the active mirror port.
    mesa_port_no_t mirror_port;                 /* Mirroring port. Setting port to  VTSS_PORT_NO_NONE disables mirroring */
    CapArray<BOOL, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> src_enable; // Enable for source mirroring
    CapArray<BOOL, VTSS_APPL_CAP_ISID_CNT, MEBA_CAP_BOARD_PORT_MAP_COUNT> dst_enable; // Enable for detination mirroring
    BOOL           cpu_src_enable[VTSS_ISID_CNT];    // Enable for CPU source mirroring
    BOOL           cpu_dst_enable[VTSS_ISID_CNT];    // Enable for CPU destination mirroring
} mirror_stack_conf_t;

#define MIRROR_CONF_VERSION 1

typedef struct {
    mirror_stack_conf_t stack_conf; // configuration for all switches in the stack

    /* Request buffer and semaphore. Using the largest of the lot. */
    struct {
        vtss_sem_t sem;
        uchar      msg[sizeof(mirror_conf_set_req_t)];
    } request;

    /* Reply buffer and semaphore. Using the largest of the lot. */
    struct {
        vtss_sem_t sem;
        uchar      msg[sizeof(mirror_conf_set_req_t)];
    } reply;

} mirror_global_t;
#endif /* _VTSS_MIRROR_BASIC_H_ */

