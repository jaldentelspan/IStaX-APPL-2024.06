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
#ifndef __vtss_dhcp_client_API_H__
#define __vtss_dhcp_client_API_H__

#define VTSS_DHCP_MAX_OFFERS  3

#include "main_types.h"
#include "vtss/appl/ip.h"
#include "main_types.h"
#if defined(VTSS_SW_OPTION_DHCP_HELPER)
#include "dhcp_helper_api.h"
#endif

#ifdef __cplusplus
#include "vtss/basics/optional.hxx"
#include "vtss/basics/string.hxx"
#include "vtss/basics/stream.hxx"
#include "vtss/basics/types.hxx"

const char *dhcp4c_state_to_txt(vtss_appl_ip_dhcp4c_state_t s);

namespace vtss {
namespace dhcp {

struct ConfPacket {
    ConfPacket() { clear(); }

    ConfPacket(const ConfPacket& rhs);
    ConfPacket& operator=(const ConfPacket& rhs);
    bool operator==(const ConfPacket& rhs);
    bool operator!=(const ConfPacket& rhs);
    void clear();

    u32                   xid;
    Ipv4Network           ip;
    MacAddress            server_mac;
    Optional<mesa_ipv4_t> server_ip;
    Optional<mesa_ipv4_t> default_gateway;
    Optional<mesa_ipv4_t> domain_name_server;
    Buffer                domain_name;
    Buffer                vendor_specific_information;
    Buffer                boot_file_name;
};

typedef Optional<ConfPacket> AckConfPacket;

ostream &operator<<(ostream &o, const ConfPacket &e);
ostream &operator<<(ostream &o, const AckConfPacket &e);

int to_txt(char *buf, int size,
           const vtss_appl_ip_if_status_dhcp4c_t *const st);

/* CONTROL AND STATE ------------------------------------------------- */

/* Start the DHCP client on the given VLAN */
mesa_rc client_start(       vtss_ifindex_t                    ifidx,
                            const vtss_appl_ip_dhcp4c_param_t *params);

/* Stop the DHCP client on the given VLAN */
mesa_rc client_stop(        vtss_ifindex_t ifidx);

/* Set the DHCP client in fallback mode */
mesa_rc client_fallback(    vtss_ifindex_t ifidx);

/* Kill the DHCP client on the given VLAN */
mesa_rc client_kill(        vtss_ifindex_t ifidx);

/* Check if the DHCP client is bound */
BOOL client_bound_get(      vtss_ifindex_t ifidx);

/* Inspect the list of received offers */
mesa_rc client_offers_get(vtss_ifindex_t                ifidx,
                          size_t                        max_offers,
                          size_t                       *valid_ovvers,
                          ConfPacket                   *list);

/* Accept one of the received offers */
mesa_rc client_offer_accept(vtss_ifindex_t, unsigned idx);

/* Get status */
mesa_rc client_status(vtss_ifindex_t                      ifidx,
                      vtss_appl_ip_if_status_dhcp4c_t  *status);

typedef void (*client_callback_t)(vtss_ifindex_t);
mesa_rc client_callback_add(vtss_ifindex_t ifidx, client_callback_t cb);
mesa_rc client_callback_del(vtss_ifindex_t ifidx, client_callback_t cb);

mesa_rc client_fields_get(vtss_ifindex_t ifidx, ConfPacket *fields);

mesa_rc client_dns_option_ip_any_get(mesa_ipv4_t prefered, mesa_ipv4_t *ip);

mesa_rc client_dns_option_domain_any_get(vtss::Buffer *name);

#if defined(VTSS_SW_OPTION_DHCP_HELPER)
/* Receive/Transmit the DHCP packet via DHCP Helper APIs */
BOOL client_packet_handler(const u8 *const frm,
                           size_t length,
                           const dhcp_helper_frame_info_t *helper_info,
                           const dhcp_helper_rx_cb_flag_t flag);
#else
BOOL client_packet_handler(void *contxt, const u8 *const frm,
                           const mesa_packet_rx_info_t *const rx_info);
#endif /* VTSS_SW_OPTION_DHCP_HELPER */

mesa_rc client_release(vtss_ifindex_t ifidx);
mesa_rc client_decline(vtss_ifindex_t ifidx);
mesa_rc client_bind(vtss_ifindex_t ifidx);
mesa_rc client_if_down(vtss_ifindex_t ifidx);
mesa_rc client_if_up(vtss_ifindex_t ifidx);


}  // namespace dhcp
}  // namespace vtss
#endif  // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif
mesa_rc vtss_dhcp_client_init(vtss_init_data_t *data);
const char * dhcp_client_error_txt(mesa_rc rc);
#ifdef __cplusplus
}
#endif

#endif /* __vtss_dhcp_client_API_H__ */
