/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_MISC_H_
#define _VTSS_MISC_H_

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_MISC

#define VTSS_TRACE_GRP_DEFAULT   0
#define TRACE_GRP_USB            1

#include <vtss_trace_api.h>

/* ================================================================= *
 *  ACL stack messages
 * ================================================================= */

/* ACL messages IDs */
typedef enum {
    MISC_MSG_ID_REG_READ_REQ,  /* Register read request */
    MISC_MSG_ID_REG_WRITE_REQ, /* Register write request */
    MISC_MSG_ID_REG_READ_REP,  /* Register read reply */
    MISC_MSG_ID_PHY_READ_REQ,  /* Register read request */
    MISC_MSG_ID_PHY_WRITE_REQ, /* Register write request */
    MISC_MSG_ID_PHY_READ_REP,  /* Register read reply */
    MISC_MSG_ID_SUSPEND_RESUME /* Suspend/resume */
} misc_msg_id_t;

/* Message */
typedef struct {
    /* Message ID */
    misc_msg_id_t msg_id;

    /* Message data, depending on message ID */
    union {
        /* Register access */
        struct {
            mesa_chip_no_t chip_no;
            ulong          addr;
            uint32_t       value;
        } reg;

        /* PHY access */
        struct {
            mesa_port_no_t port_no;
            uint           addr;
            ushort         value;
            BOOL           mmd_access;
            ushort         devad;
        } phy;
        
        /* Suspend/resume */
        struct {
            BOOL resume;
        } suspend_resume;
    } data;
} misc_msg_t;

/* Message buffer */
typedef struct {
    vtss_sem_t *sem; /* Semaphore */
    misc_msg_t *msg; /* Message */
} misc_msg_buf_t;

/* ================================================================= *
 *  Global structure
 * ================================================================= */

#define MISC_FLAG_READ_IDLE (1<<0) /* Read is idle */
#define MISC_FLAG_READ_DONE (1<<1) /* Read is done */
#define MISC_FLAG_PORT_FLAP (1<<2) /* Port flap */

/* Global structure */
typedef struct {
    vtss_sem_t     sem;      /* Message semaphore */
    misc_msg_t     msg;      /* Message buffer */
    vtss_flag_t    flags;    /* Message flags */
    ulong          value;    /* Read value */
    mesa_chip_no_t chip_no;  /* Chip number context */
    vtss_inst_t    phy_inst; /* Phy instance used by debug phy commands */

    vtss_handle_t  thread_handle;
    vtss_thread_t  thread_block;
    u32            kr_thread_ms_poll;
    BOOL           kr_thread_disable;

} misc_global_t;

#endif /* _VTSS_MISC_H_ */

