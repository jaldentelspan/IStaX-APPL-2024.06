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

#if defined(VTSS_SW_OPTION_DHCP6_CLIENT)

#include "dhcp6_client_api.h"
#include "ip_dhcp6c.hxx"
#include "ip_expose.hxx" /* For status_if_ipv6 */
#include "ip_os.hxx"
#include "ip_trace.h"
#include "subject.hxx"
#include <vtss/basics/notifications/event.hxx>
#include <vtss/basics/notifications/event-handler.hxx>
#include <vtss/basics/types.hxx>
#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif
#if defined(VTSS_SW_OPTION_ICLI)
#include "icli_api.h"   // For icli_session_printf_to_all()
#endif

/******************************************************************************/
// IP_DHCP6C_DadEventHandler()
// Event handler to monitor the Duplicate Address Detection (DAD) state on the
// IPv6 interfaces for the sake of the DHCPv6 client.
/******************************************************************************/
static struct IP_DHCP6C_DadEventHandler : public vtss::notifications::EventHandler {
    IP_DHCP6C_DadEventHandler() :
        EventHandler(&vtss::notifications::subject_main_thread),
        e_if_ipv6_status(this) {}

    void init()
    {
        status_if_ipv6.observer_new(&e_if_ipv6_status);
    }

    void execute(vtss::notifications::Event *e)
    {
        if (e == &e_if_ipv6_status) {
            status_if_ipv6.observer_get(&e_if_ipv6_status, o_if_ipv6_status);
        }

        this->operator()();
    }

    void operator()();

    vtss::notifications::Event   e_if_ipv6_status;
    StatusIfIpv6::Observer o_if_ipv6_status;
} IP_DHCP6_dad_handler;

/******************************************************************************/
// IP_DHCP6C_msg_init()
/******************************************************************************/
static bool IP_DHCP6C_msg_init(vtss_ifindex_t ifindex, Dhcp6cNotify &msg)
{
    int32_t os_ifindex;

    vtss_clear(msg);

    if ((msg.vlanx = vtss_ifindex_as_vlan(ifindex)) == 0) {
        return false;
    }

    if ((os_ifindex = ip_os_ifindex_from_ifindex(ifindex)) < 0) {
        return false;
    }

    msg.ifidx = os_ifindex;

    return true;
}

/******************************************************************************/
// IP_DHCP6C_DadEventHandler::operator()()
/******************************************************************************/
void IP_DHCP6C_DadEventHandler::operator()()
{
    auto       lock = status_if_ipv6.lock_get(__FILE__, __LINE__);
    const auto &st  = status_if_ipv6.ref(lock);

    // Loop through all interfaces and check if they have duplicate addresses
    for (const auto &i : st) {
        if (i.second.flags & VTSS_APPL_IP_IF_IPV6_FLAG_DUPLICATED) {
            Dhcp6cNotify msg;

            T_DG(IP_TRACE_GRP_DHCP6C, "%s has duplicate address %s", i.first.ifindex, i.first.addr);

            if (!IP_DHCP6C_msg_init(i.first.ifindex, msg)) {
                continue;
            }

            msg.type = DHCP6C_MSG_IF_DAD;
            memcpy(&msg.msg.dad.address, &i.first.addr.address, sizeof(msg.msg.dad.address));
            (void)vtss::dhcp6c::dhcp6_client_interface_notify(&msg);

            // We log the message on syslog and all active ICLI sessions,
            // since the address conflict detection is an asynchronous
            // process and it came from the user manual address assignment.
            vtss::StringStream log_str;
            log_str << " Duplicate address " << i.first.addr
                    << ". The address is inactive on interface " << i.first.ifindex
                    << ". Re-configure the setting again after the conflict is solved.";
            T_IG(IP_TRACE_GRP_DHCP6C, "%s", log_str.buf.c_str());
#ifdef VTSS_SW_OPTION_SYSLOG
            S_I("%%IP-6-DUPADDR: %s", log_str.cstring());
#endif /* VTSS_SW_OPTION_SYSLOG */

#if defined(VTSS_SW_OPTION_ICLI)
            // Alert message on all ICLI sessions
            (void)icli_session_printf_to_all("%%IP-6-DUPADDR: %s\n", log_str.cstring());
#endif /* VTSS_SW_OPTION_ICLI */
        }
    }
}

/******************************************************************************/
// ip_dhcp6c_fwd_change()
/******************************************************************************/
void ip_dhcp6c_fwd_change(vtss_ifindex_t ifindex, bool old_fwd, bool new_fwd)
{
    Dhcp6cNotify msg;

    T_DG(IP_TRACE_GRP_DHCP6C, "I/F %s, old_fwd = %d, new_fwd = %d", ifindex, old_fwd, new_fwd);

    if (!IP_DHCP6C_msg_init(ifindex, msg)) {
        return;
    }

    msg.type = DHCP6C_MSG_IF_LINK;
    msg.msg.link.old_state = old_fwd ? 1 : -1;
    msg.msg.link.new_state = new_fwd ? 1 : -1;

    (void)vtss::dhcp6c::dhcp6_client_interface_notify(&msg);
}

/******************************************************************************/
// ip_dhcp6c_if_del()
/******************************************************************************/
void ip_dhcp6c_if_del(vtss_ifindex_t ifindex)
{
    Dhcp6cNotify msg;

    T_DG(IP_TRACE_GRP_DHCP6C, "I/F %s", ifindex);

    if (!IP_DHCP6C_msg_init(ifindex, msg)) {
        return;
    }

    msg.type = DHCP6C_MSG_IF_DEL;

    (void)vtss::dhcp6c::dhcp6_client_interface_notify(&msg);
}

/******************************************************************************/
// ip_dhcp6c_ra_flags_change()
/******************************************************************************/
void ip_dhcp6c_ra_flags_change(vtss_ifindex_t ifindex, vtss_appl_ip_if_link_flags_t new_flags)
{
    Dhcp6cNotify msg;

    T_DG(IP_TRACE_GRP_DHCP6C, "I/F %s, new_flags = %s", ifindex, new_flags);

    if (!IP_DHCP6C_msg_init(ifindex, msg)) {
        return;
    }

    msg.type          = DHCP6C_MSG_IF_RA_FLAG;
    msg.msg.ra.m_flag = (new_flags & VTSS_APPL_IP_IF_LINK_FLAG_IPV6_RA_MANAGED) != 0;
    msg.msg.ra.o_flag = (new_flags & VTSS_APPL_IP_IF_LINK_FLAG_IPV6_RA_OTHER)   != 0;

    (void)vtss::dhcp6c::dhcp6_client_interface_notify(&msg);
}

/******************************************************************************/
// ip_dhcp6c_init()
/******************************************************************************/
void ip_dhcp6c_init(void)
{
    IP_DHCP6_dad_handler.init();
}

#else // !defined(VTSS_SW_OPTION_DHCP6_CLIENT)

// DHCP6 client is not included. Make stubs for the public API.
#include "ip_dhcp6c.hxx"

/******************************************************************************/
// ip_dhcp6c_fwd_change()
/******************************************************************************/
void ip_dhcp6c_fwd_change(vtss_ifindex_t ifindex, bool old_fwd, bool new_fwd)
{
}

/******************************************************************************/
// ip_dhcp6c_if_del()
/******************************************************************************/
void ip_dhcp6c_if_del(vtss_ifindex_t ifindex)
{
}

/******************************************************************************/
// ip_dhcp6c_ra_flags_change()
/******************************************************************************/
void ip_dhcp6c_ra_flags_change(vtss_ifindex_t ifindex, vtss_appl_ip_if_link_flags_t new_flags)
{
}

/******************************************************************************/
// ip_dhcp6c_init()
/******************************************************************************/
void ip_dhcp6c_init(void)
{
}

#endif // defined(VTSS_SW_OPTION_DHCP6_CLIENT)
