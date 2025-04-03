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

#include "critd_api.h"         /* For critd_t                   */
#include "ip_trace.h"          /* For IP_TRACE_GRP_API          */
#include "ip_chip.hxx"         /* For ourselves                 */
#include "ip_utils.hxx"        /* For fmt(mesa_routing_entry_t) */
#include "mac_utils.hxx"       /* For operator==(mesa_mac_t)    */
#include "packet_api.h"        /* For PACKET_XTR_QU_xxx         */
#include "port_iter.hxx"       /* For port_iter_t               */
#include <vtss/basics/set.hxx> /* For vtss::Set<>               */

// Mutex protecting calls to mesa_l3_common_get()/mesa_l3_common_set() and
// IP_CHIP_vlan_list.
static critd_t IP_CHIP_crit;

// This box' own base MAC address
static mesa_mac_t IP_CHIP_base_mac;

// List of VLANs added as router legs.
static vtss::Set<mesa_vid_t> IP_CHIP_vlan_list;

// Indicates whether this chip has L3 capabilities.
static bool IP_CHIP_has_l3;

/******************************************************************************/
// IpChipLock
// Protects IP_CHIP_vlan_list and calls to mesa_l3_common_get() and
// mesa_l3_common_set(). Nothing else needs to be protected (we don't care about
// IP_CHIP_base_mac).
/******************************************************************************/
struct IpChipLock {
    IpChipLock(int line)
    {
        critd_enter(&IP_CHIP_crit, __FILE__, line);
    }

    ~IpChipLock()
    {
        critd_exit(&IP_CHIP_crit, __FILE__, 0);
    }
};

#define IP_CHIP_LOCK_SCOPE() IpChipLock __lock_guard__(__LINE__)

/******************************************************************************/
// IP_CHIP_is_base_mac()
/******************************************************************************/
static bool IP_CHIP_is_base_mac(const mesa_mac_t *mac)
{
    return *mac == IP_CHIP_base_mac;
}

/******************************************************************************/
// IP_CHIP_is_multicast_mac()
/******************************************************************************/
static bool IP_CHIP_is_multicast_mac(const mesa_mac_t *mac)
{
    return (mac->addr[0] & 0x1) != 0;
}

#if defined(VTSS_SW_OPTION_L3RT)
/******************************************************************************/
// mesa_l3_neighbour_t::operator<<()
/******************************************************************************/
static vtss::ostream &operator<<(vtss::ostream &o, const mesa_l3_neighbour_t &nb)
{
    o << "DIP = " << nb.dip << " => DMAC = " << nb.dmac << "@" << nb.vlan;
    return o;
}
#endif // VTSS_SW_OPTION_L3RT

#if defined(VTSS_SW_OPTION_L3RT)
/******************************************************************************/
// mesa_l3_neighbour_t::fmt()
/******************************************************************************/
static size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_l3_neighbour_t *nb)
{
    o << *nb;
    return 0;
}
#endif // VTSS_SW_OPTION_L3RT

#if defined(VTSS_SW_OPTION_L3RT)
/******************************************************************************/
// IP_CHIP_rleg_counters_get()
/******************************************************************************/
static mesa_rc IP_CHIP_rleg_counters_get(mesa_vid_t vlan, mesa_l3_counters_t *counters)
{
    mesa_rc rc;

    T_IG(IP_TRACE_GRP_API, "Getting RLEG counters for VLAN %u", vlan);

    if ((rc = mesa_l3_counters_rleg_get(nullptr, vlan, counters)) != VTSS_RC_OK) {
        T_E("mesa_l3_counters_rleg_get(%u) failed: %s", vlan, error_txt(rc));
    }

    return rc;
}
#endif /* VTSS_SW_OPTION_L3RT */

/******************************************************************************/
// vtss_ip_chip_get_ready()
/******************************************************************************/
mesa_rc vtss_ip_chip_get_ready(const mesa_mac_t *const mac)
{
    mesa_l3_common_conf_t common;
    mesa_rc               rc;

    T_DG(IP_TRACE_GRP_API, "Base-MAC = %s", *mac);

    IP_CHIP_LOCK_SCOPE();

    IP_CHIP_base_mac = *mac;

    if (!IP_CHIP_has_l3) {
        return VTSS_RC_OK;
    }

    if ((rc = mesa_l3_common_get(NULL, &common)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_API, "mesa_l3_common_get() failed: %s", error_txt(rc));
        return rc;
    }

    common.rleg_mode    = MESA_ROUTING_RLEG_MAC_MODE_SINGLE;
    common.base_address = *mac;

    if ((rc = mesa_l3_common_set(NULL, &common)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_API, "mesa_l3_common_set() failed: %s", error_txt(rc));
    }

    return rc;
}

/******************************************************************************/
// vtss_ip_chip_mac_subscribe()
/******************************************************************************/
mesa_rc vtss_ip_chip_mac_subscribe(mesa_vid_t vlan, const mesa_mac_t *mac)
{
    mesa_mac_table_entry_t entry;
    port_iter_t            pit;
    mesa_rc                rc;

    if (!IP_CHIP_is_base_mac(mac) && !IP_CHIP_is_multicast_mac(mac)) {
        // Not supported!
        T_EG(IP_TRACE_GRP_API, "%u::%s: Not supported", vlan, *mac);
        return VTSS_RC_ERROR;
    }

    vtss_clear(entry);
    entry.vid_mac.vid = vlan;
    entry.vid_mac.mac = *mac;
    entry.locked      = TRUE;
    entry.cpu_queue   = (mac->addr[0] == 0xff ? PACKET_XTR_QU_BC : PACKET_XTR_QU_MGMT_MAC);
    entry.copy_to_cpu = TRUE;

    if (IP_CHIP_is_multicast_mac(mac)) {
        (void)port_iter_init(&pit, NULL, VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_ALL);

        while (port_iter_getnext(&pit)) {
            entry.destination[pit.iport] = TRUE;
        }
    }

    T_NG(IP_TRACE_GRP_API, "MAC-ADD: %u::%s", vlan, *mac);

    if ((rc = mesa_mac_table_add(NULL, &entry)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_API, "MAC-ADD: %u::%s failed: %s", vlan, *mac, error_txt(rc));
    }

    return rc;
}

/******************************************************************************/
// vtss_ip_chip_mac_unsubscribe()
/******************************************************************************/
mesa_rc vtss_ip_chip_mac_unsubscribe(mesa_vid_t vlan, const mesa_mac_t *mac)
{
    mesa_vid_mac_t vid_mac;
    mesa_rc        rc;

    if (!IP_CHIP_is_base_mac(mac) && !IP_CHIP_is_multicast_mac(mac)) {
        // Not supported!
        T_EG(IP_TRACE_GRP_API, "%u::%s: Not supported", vlan, *mac);
        return VTSS_RC_ERROR;
    }

    vid_mac.vid = vlan;
    vid_mac.mac = *mac;

    T_NG(IP_TRACE_GRP_API, "MAC-DEL: %u::%s", vlan, *mac);

    if ((rc = mesa_mac_table_del(NULL, &vid_mac)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_API, "mesa_mac_table_del(%u::%s) failed: %s", vlan, *mac, error_txt(rc));
    }

    return rc;
}

/******************************************************************************/
// vtss_ip_chip_routing_enable()
/******************************************************************************/
mesa_rc vtss_ip_chip_routing_enable(bool enable)
{
    mesa_l3_common_conf_t common;
    mesa_rc               rc;

    if (!IP_CHIP_has_l3) {
        return VTSS_RC_OK;
    }

    T_DG(IP_TRACE_GRP_API, "Enable = %d", enable);

    IP_CHIP_LOCK_SCOPE();

    if ((rc = mesa_l3_common_get(NULL, &common)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_API, "mesa_l3_common_get() failed: %s", error_txt(rc));
        return rc;
    }

    common.routing_enable = enable;

    if ((rc = mesa_l3_common_set(NULL, &common)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_API, "mesa_l3_common_set(%d) failed: %s", enable, error_txt(rc));
    }

    return rc;
}

/******************************************************************************/
// vtss_ip_chip_rleg_add()
/******************************************************************************/
mesa_rc vtss_ip_chip_rleg_add(mesa_vid_t vlan)
{
    mesa_l3_rleg_conf_t rl;
    mesa_rc             rc;

    if (!IP_CHIP_has_l3) {
        return VTSS_RC_OK;
    }

    vtss_clear(rl);
    rl.ipv4_unicast_enable       = FALSE; // IPv4 disabled until we get an IPv4 address
    rl.ipv6_unicast_enable       = TRUE;
    rl.ipv4_icmp_redirect_enable = TRUE;
    rl.ipv6_icmp_redirect_enable = TRUE;
    rl.vlan                      = vlan;

    IP_CHIP_LOCK_SCOPE();

    T_DG(IP_TRACE_GRP_API, "RLEG-ADD %u", vlan);

    if (IP_CHIP_vlan_list.find(vlan) != IP_CHIP_vlan_list.end()) {
        T_EG(IP_TRACE_GRP_API, "Router leg for VLAN %u already added", vlan);
        return VTSS_RC_ERROR;
    }

    if (IP_CHIP_vlan_list.size() > fast_cap(MESA_CAP_L3_RLEG_CNT)) {
        T_EG(IP_TRACE_GRP_API, "Cannot add %u. Limit of %u entries is reached", vlan, fast_cap(MESA_CAP_L3_RLEG_CNT));
        return VTSS_RC_ERROR;
    }

    // Add it to the chip
    if ((rc = mesa_l3_rleg_add(NULL, &rl)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_API, "mesa_l3_rleg_add(%u) failed: %s", vlan, error_txt(rc));
        return rc;
    }

    if (!IP_CHIP_vlan_list.set(vlan)) {
        T_EG(IP_TRACE_GRP_API, "Failed to add %u to rleg list", vlan);
        return VTSS_RC_ERROR;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_ip_chip_rleg_del()
/******************************************************************************/
mesa_rc vtss_ip_chip_rleg_del(mesa_vid_t vlan)
{
    mesa_rc rc;

    if (!IP_CHIP_has_l3) {
        return VTSS_RC_OK;
    }

    IP_CHIP_LOCK_SCOPE();

    T_DG(IP_TRACE_GRP_API, "RLEG-DEL %u", vlan);

    if (IP_CHIP_vlan_list.find(vlan) == IP_CHIP_vlan_list.end()) {
        T_EG(IP_TRACE_GRP_API, "Router leg for VLAN %u not found", vlan);
        return VTSS_RC_ERROR;
    }

    // Remove it from our list of VLANs
    IP_CHIP_vlan_list.erase(vlan);

    // Delete it from the chip
    if ((rc = mesa_l3_rleg_del(NULL, vlan)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_API, "mesa_l3_rleg_del(%u) failed: %s", vlan, error_txt(rc));
    }

    return rc;
}

/******************************************************************************/
// vtss_ip_chip_route_add()
/******************************************************************************/
mesa_rc vtss_ip_chip_route_add(const mesa_routing_entry_t *rt)
{
#if defined(VTSS_SW_OPTION_L3RT)
    mesa_rc rc;

    if (!IP_CHIP_has_l3) {
        return VTSS_RC_OK;
    }

    T_DG(IP_TRACE_GRP_API, "Add %s", *rt);

    if ((rc = mesa_l3_route_add(NULL, rt)) != VTSS_RC_OK) {
        // Cannot use T_EG(), because this may occur in a live scenario if e.g.
        // running a dynamic routing protocol, which provides more routes than
        // the chip can handle. The caller must, however, take proper action.
        T_IG(IP_TRACE_GRP_API, "mesa_l3_route_add(%s) failed: %s", *rt, error_txt(rc));
    }

    return rc;
#else
    return VTSS_RC_OK;
#endif /* VTSS_SW_OPTION_L3RT */
}

/******************************************************************************/
// vtss_ip_chip_route_del()
/******************************************************************************/
mesa_rc vtss_ip_chip_route_del(const mesa_routing_entry_t *rt)
{
#if defined(VTSS_SW_OPTION_L3RT)
    mesa_rc rc;

    if (!IP_CHIP_has_l3) {
        return VTSS_RC_OK;
    }

    T_DG(IP_TRACE_GRP_API, "Del %s", *rt);

    if ((rc = mesa_l3_route_del(NULL, rt)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_API, "mesa_l3_route_del(%s) failed: %s", *rt, error_txt(rc));
    }

    return rc;
#else
    return VTSS_RC_OK;
#endif /* VTSS_SW_OPTION_L3RT */
}

/******************************************************************************/
// vtss_ip_chip_route_bulk_add()
/******************************************************************************/
mesa_rc vtss_ip_chip_route_bulk_add(uint32_t cnt, const mesa_routing_entry_t *rt, uint32_t *cnt_out)
{
    uint32_t i;
    mesa_rc  rc;

    if (!IP_CHIP_has_l3) {
        *cnt_out = cnt;
        return VTSS_RC_OK;
    }

    for (i = 0; i < cnt; i++) {
        T_DG(IP_TRACE_GRP_API, "Add-Bulk %u/%u: %s", i + 1, cnt, rt[i]);
    }

    if ((rc = mesa_l3_route_bulk_add(NULL, cnt, rt, cnt_out)) != VTSS_RC_OK) {
        // Cannot use T_EG(), because this may occur in a live scenario if e.g.
        // running a dynamic routing protocol, which provides more routes than
        // the chip can handle. The caller must, however, take proper action.
        T_IG(IP_TRACE_GRP_API, "mesa_l3_route_bulk_add(%u) failed (installed %u): %s", cnt, *cnt_out, error_txt(rc));
    } else {
        T_DG(IP_TRACE_GRP_API, "Add-Bulk %u of %u", *cnt_out, cnt);
    }

    return rc;
}

/******************************************************************************/
// vtss_ip_chip_route_bulk_del()
/******************************************************************************/
mesa_rc vtss_ip_chip_route_bulk_del(uint32_t cnt, const mesa_routing_entry_t *rt, uint32_t *cnt_out)
{
    uint32_t i;
    mesa_rc  rc;

    if (!IP_CHIP_has_l3) {
        *cnt_out = cnt;
        return VTSS_RC_OK;
    }

    for (i = 0; i < cnt; i++) {
        T_DG(IP_TRACE_GRP_API, "Del-Bulk %u/%u: %s", i, cnt, rt[i]);
    }

    if ((rc = mesa_l3_route_bulk_del(NULL, cnt, rt, cnt_out)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_API, "mesa_l3_route_bulk_del(%u) failed: %s", cnt, error_txt(rc));
    } else {
        T_DG(IP_TRACE_GRP_API, "Del-Bulk %u of %u", *cnt_out, cnt);
    }

    return rc;
}

/******************************************************************************/
// vtss_ip_chip_neighbor_add()
/******************************************************************************/
mesa_rc vtss_ip_chip_neighbor_add(const vtss_appl_ip_neighbor_key_t &k, const vtss_appl_ip_neighbor_status_t &v)
{
#if defined(VTSS_SW_OPTION_L3RT)
    mesa_l3_neighbour_t nb;
    mesa_rc             rc;

    if (!IP_CHIP_has_l3) {
        return VTSS_RC_OK;
    }

    T_DG(IP_TRACE_GRP_API, "Add neighbor: %s:%s", k, v);

    if ((nb.vlan = vtss_ifindex_as_vlan(k.ifindex)) == 0) {
        // Not a VLAN interface
        return VTSS_RC_OK;
    }

    nb.dip  = k.dip;
    nb.dmac = v.dmac;

    if ((rc = mesa_l3_neighbour_add(NULL, &nb)) != VTSS_RC_OK) {
        // Cannot use T_EG(), because this may occur in a live scenario.
        T_IG(IP_TRACE_GRP_API, "mesa_l3_neighbour_add(%s) failed: %s", nb, error_txt(rc));
    }

    return rc;
#else
    return VTSS_RC_OK;
#endif /* VTSS_SW_OPTION_L3RT */
}

/******************************************************************************/
// vtss_ip_chip_neighbor_del()
/******************************************************************************/
mesa_rc vtss_ip_chip_neighbor_del(const vtss_appl_ip_neighbor_key_t &k, const vtss_appl_ip_neighbor_status_t &v)
{
#if defined(VTSS_SW_OPTION_L3RT)
    mesa_l3_neighbour_t nb;
    mesa_rc             rc;

    if (!IP_CHIP_has_l3) {
        return VTSS_RC_OK;
    }

    T_DG(IP_TRACE_GRP_API, "Del neighbor: %s:%s", k, v);

    if ((nb.vlan = vtss_ifindex_as_vlan(k.ifindex)) == 0) {
        // Not a VLAN interface
        return VTSS_RC_OK;
    }

    nb.dip  = k.dip;
    nb.dmac = v.dmac;

    if ((rc = mesa_l3_neighbour_del(NULL, &nb)) != VTSS_RC_OK) {
        // This may happen occassionally, so only using T_IG(), not T_EG()
        T_IG(IP_TRACE_GRP_API, "mesa_l3_neighbour_del(%s) failed: %s", nb, error_txt(rc));
    }

    return rc;
#else
    return VTSS_RC_OK;
#endif /* VTSS_SW_OPTION_L3RT */
}

/******************************************************************************/
// vtss_ip_chip_counters_vlan_get()
/******************************************************************************/
mesa_rc vtss_ip_chip_counters_vlan_get(mesa_vid_t vlan, mesa_l3_counters_t *counters)
{
#if defined(VTSS_SW_OPTION_L3RT)

    if (!IP_CHIP_has_l3) {
        vtss_clear(*counters);
        return VTSS_RC_OK;
    }

    IP_CHIP_LOCK_SCOPE();

    if (IP_CHIP_vlan_list.find(vlan) == IP_CHIP_vlan_list.end()) {
        T_EG(IP_TRACE_GRP_API, "Router leg for VLAN %u not found", vlan);
        return VTSS_RC_ERROR;
    }

    return IP_CHIP_rleg_counters_get(vlan, counters);
#else
    vtss_clear(*counters);
    return VTSS_RC_OK;
#endif
}

/******************************************************************************/
// vtss_ip_chip_counters_vlan_clear()
/******************************************************************************/
mesa_rc vtss_ip_chip_counters_vlan_clear(mesa_vid_t vlan)
{
#if defined(VTSS_SW_OPTION_L3RT)
    mesa_rc rc;

    if (!IP_CHIP_has_l3) {
        return VTSS_RC_OK;
    }

    if ((rc = mesa_l3_counters_rleg_clear(NULL, vlan)) != VTSS_RC_OK) {
        T_EG(IP_TRACE_GRP_API, "mesa_l3_counters_rleg_clear(%u) failed: %s", vlan, error_txt(rc));
    }

    return rc;
#else
    return VTSS_RC_OK;
#endif /* VTSS_SW_OPTION_L3RT */
}

/******************************************************************************/
// vtss_ip_chip_counters_system_get()
/******************************************************************************/
mesa_rc vtss_ip_chip_counters_system_get(mesa_l3_counters_t *counters)
{
    vtss_clear(*counters);

#if defined(VTSS_SW_OPTION_L3RT)
    mesa_l3_counters_t tmp;

    if (!IP_CHIP_has_l3) {
        return VTSS_RC_OK;
    }

    IP_CHIP_LOCK_SCOPE();

    // Sum up all rleg counters.
    for (auto i = IP_CHIP_vlan_list.begin(); i != IP_CHIP_vlan_list.end(); ++i) {
        VTSS_RC(IP_CHIP_rleg_counters_get(*i, &tmp));

        counters->ipv4uc_received_octets    += tmp.ipv4uc_received_octets;
        counters->ipv4uc_received_frames    += tmp.ipv4uc_received_frames;
        counters->ipv6uc_received_octets    += tmp.ipv6uc_received_octets;
        counters->ipv6uc_received_frames    += tmp.ipv6uc_received_frames;
        counters->ipv4uc_transmitted_octets += tmp.ipv4uc_transmitted_octets;
        counters->ipv4uc_transmitted_frames += tmp.ipv4uc_transmitted_frames;
        counters->ipv6uc_transmitted_octets += tmp.ipv6uc_transmitted_octets;
        counters->ipv6uc_transmitted_frames += tmp.ipv6uc_transmitted_frames;
    }
#endif

    return VTSS_RC_OK;
}

/******************************************************************************/
// vtss_ip_chip_init()
/******************************************************************************/
mesa_rc vtss_ip_chip_init(void)
{
    mesa_routing_entry_t rt;

    if (fast_cap(MESA_CAP_L3)) {
        IP_CHIP_has_l3 = true;
    }

    T_DG(IP_TRACE_GRP_API, "Enter. Has L3 = %d", IP_CHIP_has_l3);

    critd_init(&IP_CHIP_crit, "ip.chip", VTSS_MODULE_ID_IP, CRITD_TYPE_MUTEX);

    // Install non-routable networks, so that a possible default route won't
    // route these networks. See also
    //  - vtss_ipv4_addr_is_routable()
    //  - vtss_ipv6_addr_is_routable()

    // First 169.254.0.0/16 (see RFC3927, section 7):
    vtss_clear(rt);
    rt.type = MESA_ROUTING_ENTRY_TYPE_IPV4_UC;
    rt.route.ipv4_uc.network.prefix_size = 16;
    rt.route.ipv4_uc.network.address     = (169 << 24) | (254 << 16);
    (void)vtss_ip_chip_route_add(&rt);

    // Then fe80::/10:
    vtss_clear(rt);
    rt.type = MESA_ROUTING_ENTRY_TYPE_IPV6_UC;
    rt.route.ipv6_uc.network.prefix_size = 10;
    rt.route.ipv6_uc.network.address.addr[0] = 0xfe;
    rt.route.ipv6_uc.network.address.addr[1] = 0x80;
    (void)vtss_ip_chip_route_add(&rt);

    return VTSS_RC_OK;
}

