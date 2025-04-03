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

#ifndef _MSTP_H_
#define _MSTP_H_

#include "aggr_api.h"

#define CTLFLAG_MSTP_OBSOLETED      (1 << 0)
#define CTLFLAG_MSTP_AGGRCHANGE     (1 << 1)
#define CTLFLAG_MSTP_AGGRCONFIG     (1 << 2)
#define CTLFLAG_MSTP_DEFCONFIG      (1 << 3)
#define CTLFLAG_MSTP_CONFIG_CHANGE  (1 << 5)

#define MSTP_PHYS_PORTS (L2_MAX_PORTS_) /* Number of physical ports */
#define MSTP_AGGR_PORTS (L2_MAX_LLAGS_+L2_MAX_GLAGS_) /* Number of aggregations */

/* physical ports + aggragations + entry 'zero' - ]0 ; MAX]*/
#define MSTP_BRIDGE_PORTS   (MSTP_PHYS_PORTS+MSTP_AGGR_PORTS)

#define MSTP_CONF_PORT_FIRST    0
#define MSTP_CONF_PORT_LAST     (MSTP_PHYS_PORTS-1)

#define TEMP_LOCK()     vtss_global_lock(  __FILE__, __LINE__)
#define TEMP_UNLOCK()   vtss_global_unlock(__FILE__, __LINE__)

#define MSTP_AGGR_SET_CHANGE(l2) mstp_global.aggr.change[(uint)(l2-MSTP_PHYS_PORTS)] = 1

#define MSTP_AGGR_GETSET_CHANGE(l2, v)                                  \
    ({                                                                  \
        BOOL rc; uint ix = l2-MSTP_PHYS_PORTS-VTSS_PORT_NO_START;       \
        TEMP_LOCK();                                                    \
        rc = mstp_global.aggr.change[ix];                               \
        if(v != rc)                                                     \
            mstp_global.aggr.change[ix] = v;                            \
        TEMP_UNLOCK();                                                  \
        rc;                                                             \
    })

#define MSTP_AGGR_SET_MEMBER(_pno, p, v) p->members[_pno] = v
#define MSTP_AGGR_GET_MEMBER(_pno, p)    p->members[_pno]

#define MSTP_READY()    (mstp_global.ready && mstp_global.mstpi != NULL)

#define MSTP_LOCK()          critd_enter(        &mstp_global.mutex, __FILE__, __LINE__)
#define MSTP_UNLOCK()        critd_exit (        &mstp_global.mutex, __FILE__, __LINE__)
#define MSTP_ASSERT_LOCKED() critd_assert_locked(&mstp_global.mutex, __FILE__, __LINE__)

typedef struct {
    u16 n_members;                               /* Count of ports */
    u16 port_min;                                /* Start */
    u16 port_max;                                /* Stop */
} aggr_participants_t;

/* LLAG state */
typedef struct {
    aggr_participants_t cmn;     /* Common parts */
    mesa_port_list_t    members; /* Member ports - switch local */
} llag_participants_t;

struct mstp_aggr_obj;

typedef struct mstp_aggr_objh {
    uint         (*members)(struct mstp_aggr_obj const *);
    l2_port_no_t (*first_port)(struct mstp_aggr_obj const *);
    l2_port_no_t (*next_port)(struct mstp_aggr_obj const *, l2_port_no_t);
    void         (*update)(struct mstp_aggr_obj *);
    void         (*remove_port)(struct mstp_aggr_obj const *, l2_port_no_t);
} mstp_aggr_objh_t;

typedef struct mstp_aggr_obj {
    void *data_handle;
    mstp_aggr_objh_t const *handler;
    l2_port_no_t l2port;
    union {
        struct {
            mesa_glag_no_t glag;
        } glag;
        struct {
            vtss_isid_t isid;
            vtss_poag_no_t aggr_no;
            u16 port_offset;    /* L2 port offset */
        } llag;
    } u;
} mstp_aggr_obj_t;

#define MSTP_PORT_CONFIG_COUNT (1+MSTP_PHYS_PORTS)        /* [n-1] for aggrs */
#define MSTP_PORT_CONFIG_AGGR  (MSTP_PORT_CONFIG_COUNT-1) /* aggrs index */

/* Configuration */
typedef struct {
    mstp_bridge_param_t    sys;
    u8                     bridgePriority[N_MSTI_MAX];
    mstp_msti_config_t     msti;
    CapArray<BOOL, VTSS_APPL_CAP_MSTP_PORT_CONF_CNT> stp_enable;
    CapArray<mstp_port_param_t, VTSS_APPL_CAP_MSTP_PORT_CONF_CNT> portconfig;
    CapArray<mstp_msti_port_param_t, VTSS_APPL_CAP_MSTP_PORT_CONF_CNT, VTSS_APPL_CAP_MSTP_MSTI_CNT> msticonfig;
} mstp_conf_t;

#define API2L2PORT(p) (p - 1)
#define L2PORT2API(p) (p + 1)

#endif /* _MSTP_H_ */

