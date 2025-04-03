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

#ifndef _VTSS_ACL_H_
#define _VTSS_ACL_H_

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_ACL

#define VTSS_TRACE_GRP_DEFAULT      0

#include <vtss_trace_api.h>

/* ================================================================= *
 *  ACE ID definitions
 *  we'll use a 16-bit users ID and a 16-bit ACE ID to identify an ACE.
 *  The VTSS API used a 32-bits ACE ID. These two 16-bit values could
 *  be combined when accessing the switch API. This would give each ACL
 *  user a separate ID space used for its own entries.
 * ================================================================= */

#define ACL_USER_ID_GET(ace_id)                 ((ace_id) >> 16)
#define ACL_ACE_ID_GET(ace_id)                  ((ace_id) & 0xFFff)
#define ACL_COMBINED_ID_SET(user_id, ace_id)    (((user_id) << 16) + ((ace_id) & 0xFFff))

/**
 * \brief ACL users registered mode declaration.
 * If the ACL user uses local registration, it will fully control the
 * specific ACEs. And the ACL module won't delete these ACEs when
 * INIT_CMD_ICFG_LOADING_POST state even if INIT_CMD_CONF_DEF state (restore default).
 * If the ACL user uses global registration, the ACL module will set the
 * related ACEs to each switch.
 */
enum {
    /** The ACL user use global registration */
    ACL_USER_REG_MODE_GLOBAL,

    /** The ACL user use local registration */
    ACL_USER_REG_MODE_LOCAL,

    /* count of reg modes */
    ACL_USER_REG_MODE_CNT,
};

/* ================================================================= *
 *  ACL table and list
 * ================================================================= */

/* ACL entry */
typedef struct acl_ace_t {
    struct acl_ace_t    *next; /* Next in list */
    acl_entry_conf_t    conf;  /* Configuration */
} acl_ace_t;

/* ACL lists */
typedef struct {
    acl_ace_t   *used; /* Active list */
    acl_ace_t   *free; /* Free list */
} acl_list_t;

/* ACL counters */
typedef struct {
    mesa_ace_id_t       id;      /* ACE ID */
    mesa_ace_counter_t  counter; /* ACE counter */
} ace_counter_t;

/* ACL log buffer */
#define ACL_LOG_BUF_SIZE (6*1024)

/* ================================================================= *
 *  ACL global structure
 * ================================================================= */

/* ACL global structure */
typedef struct {
    critd_t                             crit;
    CapArray<acl_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_conf;
    CapArray<vtss_appl_acl_config_rate_limiter_t, MESA_CAP_ACL_POLICER_CNT> policer_conf;
    acl_list_t                          switch_acl;
    CapArray<acl_ace_t, VTSS_APPL_CAP_ACL_ACE_CNT> switch_ace_table;
    char                                log_buf[ACL_LOG_BUF_SIZE];   /* Log buffer */
    CapArray<BOOL, MEBA_CAP_BOARD_PORT_MAP_COUNT>   def_port_ace_init;           /* Identify that default port ACE has been created */
    BOOL                                ifmux_supported;
    BOOL                                def_port_ace_supported;
    void          *filter_id; /* Packet filter ID */
    CapArray<vtss_mtimer_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_shutdown_timer;
} acl_global_t;

#endif /* _VTSS_ACL_H_ */

