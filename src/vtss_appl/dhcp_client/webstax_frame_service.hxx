/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _WEBSTAX_FRAME_SERVICE_H_
#define _WEBSTAX_FRAME_SERVICE_H_

#include "frame_utils.hxx"
#if defined(VTSS_SW_OPTION_DHCP_HELPER)
#include "dhcp_helper_api.h"
#endif /* VTSS_SW_OPTION_DHCP_HELPER */
#include "packet_api.h"
#include "conf_api.h"

namespace vtss {

// as simple as posible... nothing fancy here
struct WebStaXRawFrameService {
    WebStaXRawFrameService(vtss_module_id_t id) : id_(id) {}

    template <template <class LL> class F>
    struct UdpStackType {
        typedef Stack4<Frame<1500>, EthernetFrame, IpFrame, UdpFrame, F> T;
    };

    template <template <class LL> class F>
    struct MacStackType {
        typedef Stack2<Frame<1500>, EthernetFrame, F> T;
    };

    Mac_t mac_address(mesa_vid_t vlan) {
        Mac_t m;
        (void)conf_mgmt_mac_addr_get(m.data, 0);
        return m;
    }

    Mac_t mac_address(vtss_ifindex_t ifidx) {
        Mac_t m;
        (void)conf_mgmt_mac_addr_get(m.data, 0);
        return m;
    }

    /* Construct a ip/udp header for a packet, send packet */
    template <typename F>
    bool send_ether(F& f, const Mac_t& dst, vtss_ifindex_t ifidx) {
        if (vtss_ifindex_is_vlan(ifidx)) {
            vtss_ifindex_elm_t elm;
            (void)vtss_ifindex_decompose(ifidx, &elm);
            mesa_vid_t vlan = elm.ordinal;
            packet_tx_props_t tx_props;
            packet_tx_props_init(&tx_props);
            tx_props.tx_info.switch_frm = TRUE;
            tx_props.packet_info.modid = id_;

            // declared on the stack to avoid need for locking
            f->update_deep();

            f.l1.src(mac_address(vlan));
            f.l1.dst(dst);
            
            u8* ptr = packet_tx_alloc(f.buf.payload_length());
            if (ptr == 0) return false;
            
            memcpy(ptr, &(*f.buf.payload_begin()), f.buf.payload_length());
            tx_props.packet_info.frm = ptr;
            tx_props.packet_info.len = f.buf.payload_length();
            tx_props.tx_info.tag.vid = vlan;

            if (packet_tx(&tx_props) != VTSS_RC_OK) {
                return false;
            }
            return true;
        } else {
            return false;
        }
    }

    /* Construct a ip/udp header for a packet, send packet */
    template <typename F>
    int send_udp(F& f, const IPv4_t sip, const int sport, const IPv4_t dip,
                 const int dport, const Mac_t& dst, vtss_ifindex_t ifidx) {
        if (vtss_ifindex_is_vlan(ifidx)) {
            vtss_ifindex_elm_t elm;
            (void)vtss_ifindex_decompose(ifidx, &elm);
            mesa_vid_t vlan = elm.ordinal;
#if !defined(VTSS_SW_OPTION_DHCP_HELPER)
            // declared on the stack to avoid need for locking
            packet_tx_props_t tx_props;
            packet_tx_props_init(&tx_props);
            tx_props.tx_info.switch_frm = TRUE;
            tx_props.packet_info.modid = id_;
#endif /* !VTSS_SW_OPTION_DHCP_HELPER */

        // declared on the stack to avoid need for locking
            f->update_deep();

            f.l1.src(mac_address(vlan));
            f.l1.dst(dst);

            f.l2.src(sip);
            f.l2.dst(dip);
            f.l2.set_defaults();

            f.l3.src(sport);
            f.l3.dst(dport);
            f.l3.set_defaults();

#if defined(VTSS_SW_OPTION_DHCP_HELPER)
            /* Receive/Transmit the DHCP packet via DHCP Helper APIs */
            void* bufref;
            u8* ptr = (u8*)dhcp_helper_alloc_xmit(f.buf.payload_length(),
                                                  VTSS_ISID_GLOBAL, &bufref);
            if (ptr == 0) {
                return -1;
            }

            memcpy(ptr, &(*f.buf.payload_begin()), f.buf.payload_length());
            if (dhcp_helper_xmit(DHCP_HELPER_USER_CLIENT, ptr,
                                 f.buf.payload_length(), vlan, VTSS_ISID_GLOBAL, 0,
                                 FALSE, VTSS_ISID_END, VTSS_PORT_NO_NONE,
                                 VTSS_GLAG_NO_NONE, bufref)) {
                return -1;
            }
#else
            u8* ptr = packet_tx_alloc(f.buf.payload_length());
            if (ptr == 0) {
                return -1;
            }

            memcpy(ptr, &(*f.buf.payload_begin()), f.buf.payload_length());
            tx_props.packet_info.frm = ptr;
            tx_props.packet_info.len = f.buf.payload_length();
            tx_props.tx_info.tag.vid = vlan;

            if (packet_tx(&tx_props) != VTSS_RC_OK) {
                return -1;
            }
#endif /* VTSS_SW_OPTION_DHCP_HELPER */

            return f.buf.payload_length();
        } else {
            return -1;
        }
    }

  private:
    const vtss_module_id_t id_;
};
} /* vtss */

#endif /* _WEBSTAX_FRAME_SERVICE_H_ */

