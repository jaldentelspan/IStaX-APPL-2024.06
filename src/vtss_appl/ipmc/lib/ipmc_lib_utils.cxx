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

#include "ipmc_lib_utils.hxx"
#include "ipmc_lib_trace.h"
#include "ip_utils.hxx"       /* For vtss_ipv6_addr_is_zero()            */
#include "misc_api.h"         /* For misc_ipv4_txt() and misc_ipv6_txt() */

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::print()
/******************************************************************************/
char *vtss_appl_ipmc_lib_ip_t::print(char *buf) const
{
    if (is_ipv4) {
        return misc_ipv4_txt(ipv4, buf);
    }

    return misc_ipv6_txt(&ipv6, buf);
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::is_zero()
/******************************************************************************/
mesa_bool_t vtss_appl_ipmc_lib_ip_t::is_zero(void) const
{
    return is_ipv4 ? ipv4 == 0 : vtss_ipv6_addr_is_zero(&ipv6);
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::is_uc()
/******************************************************************************/
mesa_bool_t vtss_appl_ipmc_lib_ip_t::is_uc(void) const
{
    return is_ipv4 ? vtss_ipv4_addr_is_unicast(&ipv4) : !vtss_ipv6_addr_is_multicast(&ipv6);
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::is_mc()
/******************************************************************************/
mesa_bool_t vtss_appl_ipmc_lib_ip_t::is_mc(void) const
{
    return is_ipv4 ? vtss_ipv4_addr_is_multicast(&ipv4) : vtss_ipv6_addr_is_multicast(&ipv6);
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::operator==()
/******************************************************************************/
bool vtss_appl_ipmc_lib_ip_t::operator==(const vtss_appl_ipmc_lib_ip_t &rhs) const
{
    if (is_ipv4 != rhs.is_ipv4) {
        T_E("Comparing IPv4 with IPv6: this = %s, rhs = %s", *this, rhs);
        return false;
    }

    return is_ipv4 ? ipv4 == rhs.ipv4 : ipv6 == rhs.ipv6;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::operator!=()
/******************************************************************************/
bool vtss_appl_ipmc_lib_ip_t::operator!=(const vtss_appl_ipmc_lib_ip_t &rhs) const
{
    return !(*this == rhs);
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::operator<()
/******************************************************************************/
bool vtss_appl_ipmc_lib_ip_t::operator<(const vtss_appl_ipmc_lib_ip_t &rhs) const
{
    if (is_ipv4 != rhs.is_ipv4) {
        T_E("Comparing IPv4 with IPv6: this = %s, rhs = %s", *this, rhs);
        return false;
    }

    return is_ipv4 ? ipv4 < rhs.ipv4 : ipv6 < rhs.ipv6;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::operator&()
/******************************************************************************/
vtss_appl_ipmc_lib_ip_t vtss_appl_ipmc_lib_ip_t::operator&(const vtss_appl_ipmc_lib_ip_t &rhs) const
{
    vtss_appl_ipmc_lib_ip_t res(*this);
    int                 i;

    if (is_ipv4 != rhs.is_ipv4) {
        T_E("Working on IPv4 and IPv6: this = %s, rhs = %s", *this, rhs);
        return res;
    }

    if (is_ipv4) {
        res.ipv4 &= rhs.ipv4;
    } else {
        for (i = 0; i < sizeof(res.ipv6.addr) / sizeof(res.ipv6.addr[0]); i++) {
            res.ipv6.addr[i] &= rhs.ipv6.addr[i];
        }
    }

    return res;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::operator~()
/******************************************************************************/
vtss_appl_ipmc_lib_ip_t vtss_appl_ipmc_lib_ip_t::operator~() const
{
    vtss_appl_ipmc_lib_ip_t res;
    int                 i;

    res.is_ipv4 = is_ipv4;
    if (is_ipv4) {
        res.ipv4 = ~ipv4;
    } else {
        for (i = 0; i < sizeof(res.ipv6.addr) / sizeof(res.ipv6.addr[0]); i++) {
            res.ipv6.addr[i] = ~ipv6.addr[i];
        }
    }

    return res;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::operator++()
// Prefix operator.
/******************************************************************************/
vtss_appl_ipmc_lib_ip_t &vtss_appl_ipmc_lib_ip_t::operator++()
{
    int i;

    if (is_ipv4) {
        ipv4++; // Wraps around from 255.255.255.255 to 0.0.0.0
    } else {
        for (i = 0; i < sizeof(ipv6.addr) / sizeof(ipv6.addr[0]); i++) {
            if (++ipv6.addr[i] != 0) {
                break;
            }
        }
    }

    return *this;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::operator--()
// Prefix operator.
/******************************************************************************/
vtss_appl_ipmc_lib_ip_t &vtss_appl_ipmc_lib_ip_t::operator--()
{
    int i;

    if (is_zero()) {
        // Don't subtract from 0.0.0.0 or ::
        return *this;
    }

    if (is_ipv4) {
        ipv4--;
    } else {
        i = sizeof(ipv6.addr) / sizeof(ipv6.addr[0]);
        do {
            i--;
            ipv6.addr[i] -= 1;
        } while (ipv6.addr[i] == 0xff && i != 0);
    }

    return *this;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::all_zeros_set()
/******************************************************************************/
void vtss_appl_ipmc_lib_ip_t::all_zeros_set(void)
{
    bool was_ipv4 = is_ipv4;
    vtss_clear(*this);
    is_ipv4 = was_ipv4;
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::all_ones_set()
/******************************************************************************/
void vtss_appl_ipmc_lib_ip_t::all_ones_set(void)
{
    if (is_ipv4) {
        ipv4 = 0xffffffff;
    } else {
        memset(ipv6.addr, 0xff, sizeof(ipv6.addr));
    }
}

/******************************************************************************/
// vtss_appl_ipmc_lib_ip_t::is_all_ones()
/******************************************************************************/
bool vtss_appl_ipmc_lib_ip_t::is_all_ones(void) const
{
    vtss_appl_ipmc_lib_ip_t temp;
    temp.is_ipv4 = is_ipv4;
    temp.all_ones_set();
    return *this == temp;
}

/******************************************************************************/
// ipmc_lib_util_tag_type_to_str()
/******************************************************************************/
const char *ipmc_lib_util_tag_type_to_str(mesa_tag_type_t tag_type)
{
    switch (tag_type) {
    case MESA_TAG_TYPE_UNTAGGED:
        return "Untagged";

    case MESA_TAG_TYPE_C_TAGGED:
        return "C-tagged";

    case MESA_TAG_TYPE_S_TAGGED:
        return "S-tagged";

    case MESA_TAG_TYPE_S_CUSTOM_TAGGED:
        return "Custom-S-tagged";

    default:
        T_E("Unknown tag type (%d)", tag_type);
        return "Unknown";
        break;
    }
}

/******************************************************************************/
// ipmc_lib_util_compatibility_to_str()
/******************************************************************************/
const char *ipmc_lib_util_compatibility_to_str(vtss_appl_ipmc_lib_compatibility_t compat, bool is_ipv4, bool full)
{
    switch (compat) {
    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_AUTO:
        return full ? "Auto" : "auto";

    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_OLD:
        if (!is_ipv4) {
            T_E("MLD cannot be set to old compatibility");
            return "auto";
        }

        return full ? "IGMPv1" : "v1";

    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_GEN:
        return is_ipv4 ? (full ? "IGMPv2" : "v2") : (full ? "MLDv1" : "v1");

    case VTSS_APPL_IPMC_LIB_COMPATIBILITY_SFM:
        return is_ipv4 ? (full ? "IGMPv3" : "v3") : (full ? "MLDv2" : "v2");

    default:
        T_E("Unknown compatibility (%d)", compat);
        return "auto";
    }
}

/******************************************************************************/
// ipmc_lib_util_filter_mode_to_str()
/******************************************************************************/
const char *ipmc_lib_util_filter_mode_to_str(vtss_appl_ipmc_lib_filter_mode_t filter_mode)
{
    switch (filter_mode) {
    case VTSS_APPL_IPMC_LIB_FILTER_MODE_EXCLUDE:
        return "Exclude";

    case VTSS_APPL_IPMC_LIB_FILTER_MODE_INCLUDE:
        return "Include";

    default:
        // Should never be possible
        T_E("Unknown filter_mode (%d)", filter_mode);
        return "None";
    }
}

/******************************************************************************/
// ipmc_lib_util_router_status_to_str()
/******************************************************************************/
const char *ipmc_lib_util_router_status_to_str(vtss_appl_ipmc_lib_router_status_t router_status)
{
    switch (router_status) {
    case VTSS_APPL_IPMC_LIB_ROUTER_STATUS_NONE:
        return "None";

    case VTSS_APPL_IPMC_LIB_ROUTER_STATUS_STATIC:
        return "Static";

    case VTSS_APPL_IPMC_LIB_ROUTER_STATUS_DYNAMIC:
        return "Dynamic";

    case VTSS_APPL_IPMC_LIB_ROUTER_STATUS_BOTH:
        return "Static and Dynamic";

    default:
        T_E("Unknown router_status (%d)", router_status);
        return "Unknown";
    }
}

/******************************************************************************/
// ipmc_lib_util_hw_location_to_str()
/******************************************************************************/
const char *ipmc_lib_util_hw_location_to_str(vtss_appl_ipmc_lib_hw_location_t hw_location)
{
    switch (hw_location) {
    case VTSS_APPL_IPMC_LIB_HW_LOCATION_NONE:
        return  "None";

    case VTSS_APPL_IPMC_LIB_HW_LOCATION_TCAM:
        return  "TCAM";

    case VTSS_APPL_IPMC_LIB_HW_LOCATION_MAC_TABLE:
        return "MAC Address Table";

    default:
        T_E("Invalid hw_location (%d)", hw_location);
        return "Unknown";
    }
}

/******************************************************************************/
// ipmc_lib_util_querier_state_to_str()
/******************************************************************************/
const char *ipmc_lib_util_querier_state_to_str(vtss_appl_ipmc_lib_querier_state_t querier_state)
{
    switch (querier_state) {
    case VTSS_APPL_IPMC_LIB_QUERIER_STATE_DISABLED:
        return "Disabled";

    case VTSS_APPL_IPMC_LIB_QUERIER_STATE_INIT:
        return "Init";

    case VTSS_APPL_IPMC_LIB_QUERIER_STATE_IDLE:
        return "Idle";

    case VTSS_APPL_IPMC_LIB_QUERIER_STATE_ACTIVE:
        return "Active";

    default:
        T_E("Unknown querier_state (%d)", querier_state);
        return "Unknown";
    }
}

/******************************************************************************/
// ipmc_lib_util_port_role_to_str()
/******************************************************************************/
const char *ipmc_lib_util_port_role_to_str(vtss_appl_ipmc_lib_port_role_t role, bool capital)
{
    switch (role) {
    case VTSS_APPL_IPMC_LIB_PORT_ROLE_NONE:
        return capital ? "None" : "none";

    case VTSS_APPL_IPMC_LIB_PORT_ROLE_SOURCE:
        return capital ? "Source" : "source";

    case VTSS_APPL_IPMC_LIB_PORT_ROLE_RECEIVER:
        return capital ? "Receiver" : "receiver";

    default:
        T_E("Unknown port role (%d)", role);
        return "Unknown";
    }
}

/******************************************************************************/
// ipmc_lib_util_compatible_mode_to_str()
/******************************************************************************/
const char *ipmc_lib_util_compatible_mode_to_str(bool compatible_mode)
{
    return compatible_mode ? "compatible" : "dynamic";
}

//*****************************************************************************/
// ipmc_lib_util_vlan_oper_warnings_to_txt()
// Buf must be > 1000 bytes long if all bits are set.
/******************************************************************************/
char *ipmc_lib_util_vlan_oper_warnings_to_txt(char *buf, size_t size, vtss_appl_ipmc_lib_vlan_oper_warnings_t oper_warnings)
{
    int  s = 0, res;
    bool first = true;

    if (!buf) {
        return buf;
    }

#define P(_str_)                                        \
    if (size - s > 0) {                                 \
        res = snprintf(buf + s, size - s, "%s", _str_); \
        if (res > 0) {                                  \
            s += res;                                   \
        }                                               \
    }

#define F(X, _name_)                                                \
    if (oper_warnings & VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_##X) { \
        oper_warnings &= ~VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_##X; \
        if (first) {                                                \
            first = false;                                          \
            P(_name_);                                              \
        } else {                                                    \
            P(", " _name_);                                         \
        }                                                           \
    }


    buf[0] = 0;
    s = 0;

    // Example of a field name (just so that we can search for this function):
    // VTSS_APPL_IPMC_LIB_VLAN_OPER_WARNING_INTERNAL_ERROR

    F(INTERNAL_ERROR,                                    "An internal error has occurred. See console or crashfile for details");
    F(IPMC_AND_MVR_BOTH_ACTIVE_IPMC,                     "MVR is active on the same VLAN. If the MVR VLAN is operationally active, it will win");
    F(PROFILE_GLOBALLY_DISABLED_MVR,                     "MVR VLAN inactive, because IPMC profiles are globally disabled");
    F(PROFILE_GLOBALLY_DISABLED_IPMC,                    "At least one port has a filter profile attached, but IPMC profiles are globally disabled");
    F(PROFILE_NOT_SET_MVR,                               "MVR VLAN inactive, because a channel profile is not configured");
    F(PROFILE_DOESNT_EXIST_MVR,                          "MVR VLAN inactive, because the configured channel profile doesn't exist");
    F(PROFILE_DOESNT_EXIST_IPMC,                         "At least one port has attached a filter profile that doesn't exist");
    F(PROFILE_HAS_NO_IPV4_RANGES_MVR,                    "MVR VLAN inactive, because the configured channel profile doesn't have any IPv4 ranges attached");
    F(PROFILE_HAS_NO_IPV4_RANGES_IPMC,                   "At least one port has attached a filter profile without any IPv4 ranges attached");
    F(PROFILE_HAS_NO_IPV6_RANGES_MVR,                    "MVR VLAN inactive, because the configured channel profile doesn't have any IPv6 ranges attached");
    F(PROFILE_HAS_NO_IPV6_RANGES_IPMC,                   "At least one port has attached a filter profile without any IPv6 ranges attached");
    F(PROFILE_HAS_NO_IPV4_PERMIT_RULES_MVR,              "MVR VLAN inactive, because the configured channel profile has no IPv4 permit rules");
    F(PROFILE_HAS_NO_IPV4_PERMIT_RULES_IPMC,             "At least one port has attached a filter profile without any IPv4 permit rules");
    F(PROFILE_HAS_NO_IPV6_PERMIT_RULES_MVR,              "MVR VLAN inactive, because the configured channel profile has no IPv6 permit rules");
    F(PROFILE_HAS_NO_IPV6_PERMIT_RULES_IPMC,             "At least one port has attached a filter profile without any IPv6 permit rules");
    F(PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV4_MVR,   "The channel profile has an IPv4 deny rule shadows a permit rule coming later in the profile's rule list");
    F(PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV4_IPMC,  "At least one port has a filter profile attached, where an IPv4 deny rule shadows a permit rule coming later in the profile's rule list");
    F(PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV6_MVR,   "The channel profile has an IPv6 deny rule shadows a permit rule coming later in the profile's rule list");
    F(PROFILE_RULE_DENY_SHADOWS_LATER_PERMIT_IPV6_IPMC,  "At least one port has a filter profile attached, where an IPv6 deny rule shadows a permit rule coming later in the profile's rule list");
    F(PROFILE_OTHER_OVERLAPS_IPV4_MVR,                   "MVR VLAN inactive, because another MVR VLAN instance uses a profile with at least one IPv4 rule that overlaps this one's");
    F(PROFILE_OTHER_OVERLAPS_IPV6_MVR,                   "MVR VLAN inactive, because another MVR VLAN instance uses a profile with at least one IPv6 rule that overlaps this one's");
    F(NO_SOURCE_PORTS_CONFIGURED,                        "No ports are configured as sources");
    F(NO_RECEIVER_PORTS_CONFIGURED,                      "No ports are configured as receivers");
    F(AT_LEAST_ONE_SOURCE_PORT_MEMBER_OF_VLAN_INTERFACE, "At least one source port is member of a VLAN interface with same MVR VLAN ID");

    buf[MIN(size - 1, s)] = 0;
#undef F
#undef P

    if (oper_warnings != 0) {
        T_E("Not all operational warnings are handled. Missing = 0x%x", oper_warnings);
    }

    return buf;
}

