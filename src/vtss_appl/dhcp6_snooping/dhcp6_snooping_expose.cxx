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


#include "dhcp6_snooping_serializer.hxx"
#include "dhcp6_snooping_expose.h"
#include "dhcp6_snooping_priv.h"

const vtss_enum_descriptor_t dhcp6_snooping_mode_txt[]{
    { DHCP6_SNOOPING_MODE_DISABLED,         "disabled" },
    { DHCP6_SNOOPING_MODE_ENABLED,          "enabled" },
    { 0, 0 },
};

const vtss_enum_descriptor_t dhcp6_snooping_nh_unknown_mode_txt[]{
    { DHCP6_SNOOPING_NH_UNKNOWN_MODE_DROP,  "drop" },
    { DHCP6_SNOOPING_NH_UNKNOWN_MODE_ALLOW, "allow" },
    { 0, 0 },
};

const vtss_enum_descriptor_t dhcp6_snooping_port_trust_mode_txt[]{
    { DHCP6_SNOOPING_PORT_MODE_UNTRUSTED,   "untrusted" },
    { DHCP6_SNOOPING_PORT_MODE_TRUSTED,     "trusted" },
    { 0, 0 },
};

bool compare_registered_clients_info(const dhcp_duid_t& a, vtss::Pair<const dhcp_duid_t, registered_clients_info_t>& b) {
    return a < b.first;
}

bool compare_assigned_ip(const vtss_appl_dhcp6_snooping_iaid_t& a, vtss::Pair<const vtss_appl_dhcp6_snooping_iaid_t, client_interface_info_t>& b) {
    return a < b.first;
}

time_t registered_clients_info_t::get_idle_time(time_t curr_time)
{
    if (last_access_time <= curr_time) {
        return curr_time - last_access_time;
    }

    T_EG(TRACE_GRP_TIMER, "Timer wrapped around!");
    return 0;
}

/*
 * Return expiry time in seconds
 */
uint32_t registered_clients_info_t::get_curr_expiry_time(const client_interface_info_t &item) const
{
    if (item.address_state != DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED) {
        // Only items with assigned entry has a valid lease expiry time
        return 0;
    }

    return item.assigned_time + item.lease_time;
}

bool registered_clients_info_t::has_assigned_addresses()
{
    for (auto it = if_map.begin(); it != if_map.end(); it++) {
        if (it->second.address_state == DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED)
            return true;
    }
    return false;
}

bool registered_clients_info_t::has_pending_requests()
{
    for (auto it = if_map.begin(); it != if_map.end(); it++) {
        if (it->second.address_state != DHCP6_SNOOPING_ADDRESS_STATE_ASSIGNED)
            return true;
    }
    return false;
}

bool dhcp_duid_t::operator!= (const dhcp_duid_t &other_duid) const
{
    if (this->length != other_duid.length) {
        // if DUIDs are not of same size they are by definition not equal
        return true;
    }

    return (memcmp(this->data, other_duid.data, this->length) != 0);
}

bool dhcp_duid_t::operator< (const dhcp_duid_t &other_duid) const
{
    if (this->length != other_duid.length) {
        // if DUIDs are not of same size the shorter one is by definition "less" that the other
        return this->length < other_duid.length;
    }

    return (memcmp(this->data, other_duid.data, this->length) < 0);
}
