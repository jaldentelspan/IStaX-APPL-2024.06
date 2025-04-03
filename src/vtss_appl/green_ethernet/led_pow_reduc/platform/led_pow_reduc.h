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

#ifndef _LED_POW_REDUC_H_
#define _LED_POW_REDUC_H_

#include "led_pow_reduc_api.h"  // for led_pow_reduc_switch_conf_t

//************************************************
// Trace definitions
//************************************************
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#include <vtss_trace_api.h>
#include "critd_api.h"

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_LED_POW_REDUC
#define VTSS_TRACE_GRP_DEFAULT 0
#define TRACE_GRP_CONF         1
#define TRACE_GRP_CLI          2

//************************************************
// Configuration
//************************************************
// Configuration for the whole stack - Overall configuration (valid on primary switch only).
typedef struct {
    // One instance of the global configuration (configuration that are shared for all switches in the stack)
    led_pow_reduc_glbl_conf_t   glbl_conf;

    // One instance per switch in the stack of the switch configuration.
    // Indices are in the range [0; VTSS_ISID_CNT[, so all derefs must
    // subtract VTSS_ISID_START from @isid to index correctly.
    // *** None at the moment *** led_pow_reduc_switch_conf_t switch_conf[VTSS_ISID_CNT];
} led_pow_reduc_stack_conf_t;

//************************************************
// messages
//************************************************

/* message buffer */
typedef struct {
    vtss_sem_t *sem; /* Semaphore */
    uchar      *msg; /* Message */
} led_pow_reduc_msg_buf_t;


typedef enum {
    LED_POW_REDUC_MSG_ID_CONF_SET_REQ,   /* Configuration set request (no reply) */
    LED_POW_REDUC_MSG_ID_STATUS_REQ,     /* Status request */
    LED_POW_REDUC_MSG_ID_STATUS_REP,     /* Status reply */
} led_pow_reduc_msg_id_t;


// Message for configuration
typedef struct {
    led_pow_reduc_msg_id_t              msg_id;        /* Message ID: */
    led_pow_reduc_local_conf_t          local_conf;   /* Configuration that are local for a switch*/
} led_pow_reduc_msg_local_switch_conf_t;

// Message for status
typedef struct {
    led_pow_reduc_msg_id_t              msg_id;        /* Message ID: */
} led_pow_reduc_msg_local_switch_status_t;


// Message for requests without
typedef struct {
    led_pow_reduc_msg_id_t             msg_id;        /* Message ID: */
} led_pow_reduc_msg_id_req_t;

/* Using the largest of the lot. */
#define msg_size  sizeof(led_pow_reduc_msg_local_switch_conf_t)

typedef struct {

    /* Thread variables */
    vtss_handle_t thread_handle;
    vtss_thread_t thread_block;


    /* Request buffer and semaphore */
    struct {
        vtss_sem_t sem;
        uchar      msg[msg_size];
    } request;

    /* Reply buffer and semaphore. */
    struct {
        vtss_sem_t sem;
        uchar      msg[msg_size];
    } reply;
} led_pow_reduc_msg_t;

#endif /* _LED_POW_REDUC_H_ */

