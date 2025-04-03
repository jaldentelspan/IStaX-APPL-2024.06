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

#ifndef __VTSS_DHCP6_CLIENT_HXX__
#define __VTSS_DHCP6_CLIENT_HXX__

extern "C" {
#include <string.h>
#include <stdlib.h>
}

#include "dhcp6c_porting.hxx"

#define DHCP6_PKT_BUF_SZ            (sizeof(int) * ((DHCP6_PKT_SZ_VAL + 3) / sizeof(int)))

#define DHCP6_BIP_FIFO_GROW         3
#define DHCP6_BIP_FIFO_SZ           (0x100 * DHCP6_BIP_FIFO_GROW)
#define DHCP6_BIP_BUF_SZ_B          (DHCP6_BIP_FIFO_SZ * DHCP6_PKT_SZ_VAL)

namespace vtss
{
namespace dhcp6c
{
/* ================================================================= *
 *  DHCP6C stack messages
 * ================================================================= */
/* DHCP6C request message timeout */
#define DHCP6C_STKMSG_REQ_TIMEOUT   12345   /* in msec */

/* DHCP6C stack messages IDs */
typedef enum {
    DHCP6C_STKMSG_ID_CFG_DEFAULT = 0,       /* DHCP6C configuration default request (no reply) */
    DHCP6C_STKMSG_ID_SYS_MGMT_SET_REQ,      /* DHCP6C system MAC address set request (no reply) */
    DHCP6C_STKMSG_ID_GLOBAL_PURGE_REQ,      /* Purge DHCP6C internal DB request (no reply) */

    DHCP6C_STKMSG_MAX_ID
} dhcp6c_stkmsg_id_t;

/*lint -save -e19 */
VTSS_ENUM_INC(dhcp6c_stkmsg_id_t);
/*lint -restore */

/* DHCP6C_STKMSG_ID_CFG_DEFAULT */
typedef struct {
    dhcp6c_stkmsg_id_t              msg_id;
    vtss_isid_t                     isidx;
} dhcp6c_stkmsg_cfg_default_req_t;

/* DHCP6C_STKMSG_ID_SYS_MGMT_SET_REQ */
typedef struct {
    dhcp6c_stkmsg_id_t              msg_id;
    vtss_isid_t                     isidx;
    u8                              mgmt_mac[6];
} dhcp6c_stkmsg_sysmgmt_set_req_t;

/* DHCP6C_STKMSG_ID_GLOBAL_PURGE_REQ */
typedef struct {
    dhcp6c_stkmsg_id_t              msg_id;
    vtss_isid_t                     isidx;
} dhcp6c_stkmsg_purge_req_t;

/* DHCP6C Stack Message Buffer */
typedef struct {
    vtss_sem_t                      *sem;   /* Semaphore */
    u8                              *msg;   /* Message */
} dhcp6c_stkmsg_buf_t;

#define DHCP6C_EVENT_ANY               0xFFFFFFFF  /* Any possible bit... */
#define DHCP6C_EVENT_WAKEUP            0x00000001
#define DHCP6C_EVENT_QRTV              0x00000010
#define DHCP6C_EVENT_RESUME            0x00000100
#define DHCP6C_EVENT_DEFAULT           0x00004000
#define DHCP6C_EVENT_ICFG_LOADING_POST 0x00100000
#define DHCP6C_EVENT_MSG_RA            0x01000000
#define DHCP6C_EVENT_MSG_DAD           0x02000000
#define DHCP6C_EVENT_MSG_LINK          0x04000000
#define DHCP6C_EVENT_MSG_DEL           0x08000000
#define DHCP6C_EVENT_EXIT              0x10000000

#define DHCP6C_THREAD_TICK_TIME     1000        /* 1000 msec */

struct msg_info_t {
    BOOL                            ra_msg;
    BOOL                            m_flag;
    BOOL                            o_flag;

    BOOL                            link_msg;
    i32                             new_state;
    i32                             old_state;

    mesa_ipv6_t                     address;
    BOOL                            dad_msg;

    BOOL                            del_msg;
    u8                              reserved;

    BOOL                            valid;
    u16                             ifidx;
    mesa_vid_t                      vlanx;
};

} /* dhcp6c */
} /* vtss */

#endif /* __VTSS_DHCP6_CLIENT_HXX__ */

