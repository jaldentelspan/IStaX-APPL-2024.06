/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __VTSS_DHCP6_CLIENT_API_H__
#define __VTSS_DHCP6_CLIENT_API_H__

#include "vtss/appl/dhcp6_client.h"

#ifdef __cplusplus
//#include "vtss/basics/optional.hxx"
//#include "vtss/basics/string.hxx"
//#include "vtss/basics/types.hxx"

extern "C" {
#define DHCP6C_RUN_SLAAC_ONLY   1 << 0
#define DHCP6C_RUN_DHCP_ONLY    1 << 1
#define DHCP6C_RUN_AUTO_BOTH    (DHCP6C_RUN_SLAAC_ONLY | DHCP6C_RUN_DHCP_ONLY)

struct Dhcp6cDuid {
    u16                         duid_type;

    union {
        struct {
            mesa_mac_t          lla;
            u16                 hardware_type;
            u32                 time;
        } llt;

        struct {
            mesa_mac_t          lla;
            u16                 hardware_type;
        } ll;

        struct {
            u32                 enterprise_number;
            u32                 id;
        } en;
    } type;
};

struct Dhcp6cCounter {
    u32                         rx_advertise;
    u32                         rx_reply;
    u32                         rx_reconfigure;
    u32                         rx_error;
    u32                         rx_drop;
    u32                         rx_unknown;

    u32                         tx_solicit;
    u32                         tx_request;
    u32                         tx_confirm;
    u32                         tx_renew;
    u32                         tx_rebind;
    u32                         tx_release;
    u32                         tx_decline;
    u32                         tx_information_request;
    u32                         tx_error;
    u32                         tx_drop;
    u32                         tx_unknown;
};

struct Dhcp6cInterface {
    Dhcp6cDuid                  duid;

    mesa_vid_t                  ifidx;
    BOOL                        rapid_commit;
    u8                          srvc;

    /* Status */
    mesa_ipv6_t                 address;
    u32                         prefix_length;
    mesa_ipv6_t                 srv_addr;
    mesa_ipv6_t                 dns_srv_addr;
    char                        dns_domain_name[256];

    vtss_tick_count_t           refresh_ts;
    vtss_tick_count_t           t1;
    vtss_tick_count_t           t2;
    vtss_tick_count_t           preferred_lifetime;
    vtss_tick_count_t           valid_lifetime;

    Dhcp6cCounter               cntrs;
};

typedef enum {
    DHCP6C_MSG_IF_DEL     = -1, /**< Interface DEL operation notification */
    DHCP6C_MSG_IF_LINK    =  0, /**< Interface LINK change notification */
    DHCP6C_MSG_IF_DAD     =  1, /**< Interface DAD status notification */
    DHCP6C_MSG_IF_RA_FLAG =  2  /**< Interface RA status notification */
} Dhcp6cNotifyType;

struct Dhcp6cNotify {
    u16                         ifidx;
    mesa_vid_t                  vlanx;
    Dhcp6cNotifyType            type;

    union {
        struct {
            BOOL                m_flag;
            BOOL                o_flag;
        } ra;

        struct {
            i32                 new_state;
            i32                 old_state;
        } link;

        struct {
            mesa_ipv6_t         address;
        } dad;
    } msg;
};
}

namespace vtss
{
namespace dhcp6c
{
mesa_rc dhcp6_client_interface_itr(mesa_vid_t vidx, Dhcp6cInterface *const intf);
mesa_rc dhcp6_client_interface_get(mesa_vid_t vidx, Dhcp6cInterface *const intf);
mesa_rc dhcp6_client_interface_set(mesa_vid_t vidx, const Dhcp6cInterface *const intf);
mesa_rc dhcp6_client_interface_del(mesa_vid_t vidx);
mesa_rc dhcp6_client_interface_notify(const Dhcp6cNotify *const nmsg);
}  /* dhcp6c */
}  /* vtss */

extern "C" {

/**
 * \brief DHCPv6 client module initialization.
 *
 * Call once from the main() function.
 */
mesa_rc dhcp6_client_init(vtss_init_data_t *data);

/**
 * \brief Control action for restarting DHCPv6 client on a specific IP interface
 *
 * To restart the DHCPv6 client service on a specific IP interface.
 */
const char *dhcp6_client_error_txt(mesa_rc rc);

/**
 * \brief DHCPv6 client interface existence check.
 *
 * Return TRUE if specific DHCPv6 client interface exists; FALSE otherwise.
 */
BOOL dhcp6_client_if_exists(mesa_vid_t vidx);

}

#endif /* __cplusplus */

#endif /* __VTSS_DHCP6_CLIENT_API_H__ */
