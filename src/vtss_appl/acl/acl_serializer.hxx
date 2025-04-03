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

/**
 * \file acl_serializer.h
 * \brief ACL is an acronym for Access Control List. It is the list table of
 * ACEs, containing access control entries that specify individual users or
 * groups permitted or denied to specific traffic objects, such as a process
 * or a program. Each accessible traffic object contains an identifier to its
 * ACL. The privileges determine whether there are specific traffic object
 * access rights. ACL implementations can be quite complex, for example, when
 * the ACEs are prioritized for the various situation. In networking, the ACL
 * refers to a list of service ports or network services that are available
 * on a host or server, each with a list of hosts or servers permitted or
 * denied to use the service. ACL can generally be configured to control
 * inbound traffic, and in this context, they are similar to firewalls.
 */

#ifndef __ACL_SERIALIZER_HXX__
#define __ACL_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/types.hxx"
#include "vtss/appl/acl.h"
#include "vtss_vcap_serializer.hxx"

/*****************************************************************************
 - JSON notification serializer
*****************************************************************************/
extern vtss::expose::TableStatus <
    vtss::expose::ParamKey<vtss_usid_t>,
    vtss::expose::ParamVal<vtss_appl_acl_status_ace_event_t *>
    > acl_status_ace_event_update;

/*****************************************************************************
 - MIB enum serializer
*****************************************************************************/
extern const vtss_enum_descriptor_t vtss_appl_acl_ace_frame_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_acl_ace_frame_type_t,
                         "AclAceFrameType",
                         vtss_appl_acl_ace_frame_type_txt,
                         "The frame type of the ACE.");

extern const vtss_enum_descriptor_t vtss_appl_acl_ace_ingress_mode_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_acl_ace_ingress_mode_t,
                         "AclAceIngressPortListMode",
                         vtss_appl_acl_ace_ingress_mode_txt,
                         "The ingress port list mode of the ACE.");

extern const vtss_enum_descriptor_t vtss_appl_acl_hit_action_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_acl_hit_action_t,
                         "AclHitAction",
                         vtss_appl_acl_hit_action_txt,
                         "The hit action.");

extern const vtss_enum_descriptor_t vtss_appl_acl_ace_vlan_tagged_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_acl_ace_vlan_tagged_t,
                         "AclAceVlanTagged",
                         vtss_appl_acl_ace_vlan_tagged_txt,
                         "VLAN tagged/untagged.");

extern const vtss_enum_descriptor_t vtss_appl_acl_ace_arp_op_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_acl_ace_arp_op_t,
                         "AclAceArpOp",
                         vtss_appl_acl_ace_arp_op_txt,
                         "ARP opcode.");

/*****************************************************************************
 - MIB row/table indexes serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(acl_rate_limiter_id_index, mesa_acl_policer_no_t, a, s)
{
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("RateLimiterId"),
        vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The rate limter ID.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(acl_ifindex_index, vtss_ifindex_t, a, s)
{
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The index of logical interface.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(acl_ace_id_index, vtss_appl_acl_ace_id_t, a, s)
{
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("AceId"),
        vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The ACE ID.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(acl_precedence_index, u32, a, s)
{
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("Precedence"),
        vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The ACL ACE hit precedence.")
    );
}

// Table index of AclAceStatusTable
VTSS_SNMP_TAG_SERIALIZE(acl_usid_index, u32, a, s)
{
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("SwitchId"),
        vtss::expose::snmp::RangeSpec<uint32_t>(0, 2147483647),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The switch ID.")
    );
}

/****************************************************************************
 * Capabilities
 ****************************************************************************/
struct AclCapAceIdMax {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "AceIdMax";
    static constexpr const char *desc = "Maximum ID of ACE.";
    static uint32_t get();
};

struct AclCapPolicyIdMax {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "PolicyIdMax";
    static constexpr const char *desc = "Maximum ID of policy.";
    static uint32_t get();
};

struct AclCapRateLimiterIdMax {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "RateLimiterIdMax";
    static constexpr const char *desc = "Maximum ID of rate limiter.";
    static uint32_t get();
};

struct AclCapEvcPolicerIdMax {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "EvcPolicerIdMax";
    static constexpr const char *desc = "Maximum ID of EVC policer (obsolete. Always 0).";
    static uint32_t get();
};

struct AclCapRateLimiterBitRateSupported {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "RateLimiterBitRateSupported";
    static constexpr const char *desc = "If true, the rate limiter can be configured by bit rate.";
    static constexpr bool get() {
        return true;
    }
};

struct AclCapActionEvcPolicerSupported {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "EvcPolicerSupported";
    static constexpr const char *desc = "If true, EVC policer can be configured (obsolete. Always false).";
    static bool get() {
        return false;
    }
};

struct AclCapActionMirrorSupported {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "MirrorSupported";
    static constexpr const char *desc = "If true, mirror action is supported.";
    static constexpr bool get() {
        return true;
    }
};

struct AclCapActionMultipleRedirectPortsSupported {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "MultipleRedirectPortsSupported";
    static constexpr const char *desc = "If true, redirect port list can "
        "be configured with multiple ports. If false, redirect port list "
        "can be configured with only one single port.";
    static constexpr bool get() {
        return true;
    }
};

struct AclCapSecondLookupSupported {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "SecondLookupSupported";
    static constexpr const char *desc = "If true, second lookup can be configured.";
    static bool get() {
        return (fast_cap(MESA_CAP_ACL_KEY_LOOKUP) ? true : false);
    }
};

struct AclCapMultipleIngressPortsSupported {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "MultipleIngressPortsSupported";
    static constexpr const char *desc = "If true, ingress port list can "
        "be configured with multiple ports. If false, ingress port list "
        "can be configured with only one single port.";
    static constexpr bool get() {
        return true;
    }
};

struct AclCapEgressPortSupported {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "EgressPortSupported";
    static constexpr const char *desc = "If true, egress port list can be configured.";
    static constexpr bool get() {
        return true;
    }
};

struct AclCapVlanTaggedSupported {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "VlanTaggedSupported";
    static constexpr const char *desc = "If true, VLAN tagged can be configured.";
    static constexpr bool get() {
        return true;
    }
};

struct AclCapStackableAceSupported {
    static constexpr const char *json_ref = "vtss_appl_acl_capabilities_t";
    static constexpr const char *name = "StackableAceSupported";
    static constexpr const char *desc = "If true, stackable ACE is supported. The 'switch' and 'switchport' ACE ingress type can be configured. Otherwize, only 'any' and 'specific' ACE ingress type can be configured.";
    static constexpr bool get() {
        return false;
    }
};

/*****************************************************************************
 - MIB row/table entry serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_acl_config_rate_limiter_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_acl_config_rate_limiter_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.bit_rate_enable),
        vtss::tag::Name("BitRateEnable"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapRateLimiterBitRateSupported>(),
        vtss::tag::Description("Use bit rate policing instead of packet rate. "
            "True means bit rate is used and false means packet rate is used.")
    );

    m.add_leaf(
        s.bit_rate,
        vtss::tag::Name("BitRate"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapRateLimiterBitRateSupported>(),
        vtss::tag::Description("Bit rate in kbps.")
    );

    m.add_leaf(
        s.packet_rate,
        vtss::tag::Name("PacketRate"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Packet rate in pps.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_acl_config_ace_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_acl_config_ace_t"));

    int ix = 1;
    int iy = 0;

    m.add_leaf(
        s.next_ace_id,
        vtss::tag::Name("NextAceId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ID of ACE next to this ACE.")
    );

    /*
        Action
    */
    ix = (++iy) * 100;

    m.add_leaf(
        s.action.hit_action,
        vtss::tag::Name("HitAction"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The hit action operation for this ACE.")
    );

    m.add_leaf(
        (vtss::PortListStackable &)(s.action.redirect_port),
        vtss::tag::Name("RedirectPortList"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The redirect port list for this ACE.")
    );

    m.add_leaf(
        s.action.redirect_switchport,
        vtss::tag::Name("RedirectPortListSwitchPort"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapStackableAceSupported>(),
        vtss::tag::Description("The redirect switch port for this ACE. "
        "The object is only abaliable if AclAceIngressPortListMode is switchport. "
        "And it is used when this ACE is applied on specific switch port of all switches in stackable devices."
        )
    );

    m.add_leaf(
        (vtss::PortListStackable &)(s.action.egress_port),
        vtss::tag::Name("EgressPortList"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapEgressPortSupported>(),
        vtss::tag::Description("The egress port list. The port list is to "
            "define what ports are allowed to be egress ports. "
            "If the egress port of an incoming frame is in the port list "
            "then the frame will be forwared to that port. Otherwise, if the "
            "egress port is not in the port list then "
            "the egress port is not allowed and the incoming frame will be "
            "dropped.")
    );

    m.add_leaf(
        s.action.rate_limiter_id,
        vtss::tag::Name("RateLimiterId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The rate limiter ID. 0 means to be disabled.")
    );

    m.add_leaf(
        (uint16_t)0,
        vtss::tag::Name("EvcPolicerId"),
        vtss::expose::snmp::Status::Obsoleted,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapActionEvcPolicerSupported>(),
        vtss::tag::Description("The EVC policer ID. 0 means to be disabled. Obsolete.")
    );

    m.add_leaf(
        vtss::AsBool(s.action.mirror),
        vtss::tag::Name("Mirror"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapActionMirrorSupported>(),
        vtss::tag::Description("The mirror operation. "
            "Frames matching this ACE are mirrored to the destination mirror port "
            "that is configured in the mirror module.")
    );

    m.add_leaf(
        vtss::AsBool(s.action.logging),
        vtss::tag::Name("Logging"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Specify the logging operation of the ACE. "
            "Notice that the logging message doesn't include the 4 bytes CRC "
            "information. The allowed values are: True - Frames matching the "
            "ACE are stored in the System Log. False - Frames matching the "
            "ACE are not logged. Note: The logging feature only works when "
            "the packet length is less than 1518(without VLAN tags) and the "
            "system log memory size and logging rate is limited.")
    );

    m.add_leaf(
        vtss::AsBool(s.action.shutdown),
        vtss::tag::Name("Shutdown"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Specify the port shut down operation of the ACE. "
        "The allowed values are: True: If a frame matches the ACE, the "
        "ingress port will be shuted down. False: Port shut down is disabled "
        "for the ACE. Note: The shutdown feature only works when the packet "
        "length is less than 1518(without VLAN tags).")
    );

    /*
        Key
     */
    ix = (++iy) * 100;

    m.add_leaf(
        s.key.ingress_mode,
        vtss::tag::Name("IngressPortListMode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ingress port list mode for this ACE. "
        "The possible mode are: "
        "'any': Indicates this ACE is applied on all switches and all ports. "
        "'specific': Indicates this ACE is applied on specific switch and specific ports. "
        "'switch': Indicates this ACE is applied on all ports of specific switch in stackable devices. "
        "'switchport': Indicates this ACE is applied on specific switch port of all switches in stackable devices."
        )
    );

    m.add_leaf(
        (vtss::PortListStackable &)(s.key.ingress_port),
        vtss::tag::Name("IngressPortList"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ingress port list for this ACE. "
        "The object is only abaliable if AclAceIngressPortListMode is specific. "
        "And it is used when this ACE is applied on specific switch and specific ports."
        )
    );

    m.add_leaf(
        s.key.ingress_switch,
        vtss::tag::Name("IngressPortListSwitch"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapStackableAceSupported>(),
        vtss::tag::Description("The ingress switch ID for this ACE. "
        "The object is only abaliable if AclAceIngressPortListMode is switch. "
        "And it is used when this ACE is applied on specific switch port of all switches in stackable devices."
        )
    );

    m.add_leaf(
        s.key.ingress_switchport,
        vtss::tag::Name("IngressPortListSwitchPort"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapStackableAceSupported>(),
        vtss::tag::Description("The ingress switch port for this ACE. "
        "The object is only abaliable if AclAceIngressPortListMode is switchport. "
        "And it is used when this ACE is applied on specific switch port of all switches in stackable devices."
        )
    );

    m.add_leaf(
        s.key.policy.value,
        vtss::tag::Name("PolicyValue"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The policy number for this ACE.")
    );

    m.add_leaf(
        s.key.policy.mask,
        vtss::tag::Name("PolicyMask"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The policy mask for this ACE. "
            "The allowed range is 0x0 to 0xff. If the binary bit value "
            "is '0', it means this bit is don't-care. The real matched "
            "pattern is [policy_value & policy_bitmask]. "
            "For example, if the policy value is 3 and the policy bitmask "
            "is 0x10(bit 0 is don't-care bit), then policy 2 and 3 are "
            "applied to this rule.")
    );

    m.add_leaf(
        vtss::AsBool(s.key.second_lookup),
        vtss::tag::Name("SecondLookup"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapSecondLookupSupported>(),
        vtss::tag::Description("The second lookup operation for this ACE.")
    );

    /*
        VLAN parameters
     */
    ix = (++iy) * 100;

    m.add_leaf(
        s.key.vlan.vid,
        vtss::tag::Name("VlanId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The VLAN ID for this ACE. "
            "Possible values are: 0(disabled), 1-4095.")
    );

    m.add_leaf(
        s.key.vlan.usr_prio,
        vtss::tag::Name("VlanTagPriority"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The VLAN tag priority for this ACE.")
    );

    m.add_leaf(
        s.key.vlan.tagged,
        vtss::tag::Name("VlanTagged"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapVlanTaggedSupported>(),
        vtss::tag::Description("The 802.1Q VLAN tagged for this ACE.")
    );

    /*
        Frame parameters
    */
    ix = (++iy) * 100;

    // Frame Type
    m.add_leaf(
        s.key.frame.frame_type,
        vtss::tag::Name("FrameType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The frame type for this ACE.")
    );

    // Destination MAC
    m.add_leaf(
        s.key.frame.dmac_type,
        vtss::tag::Name("DestMacOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a packet's destination MAC "
            "address to be matched.")
    );

    /*
        Ether
    */
    ix = (++iy) * 100;

    // Source MAC
    m.add_leaf(
        s.key.frame.ether.smac_type,
        vtss::tag::Name("EtherSrcMacOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a Ethernet type packet's "
            "source MAC address to be matched. This object is only "
            "available if FrameType is ether(1).")
    );

    m.add_leaf(
        s.key.frame.ether.smac,
        vtss::tag::Name("EtherSrcMac"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates the 48 bits source MAC address. "
            "This object is only available if FrameType is ether(1) and "
            "EtherSrcMacOp is specific(1).")
    );

    // Destination MAC
    m.add_leaf(
        s.key.frame.ether.dmac,
        vtss::tag::Name("EtherDestMac"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates the 48 bits destination MAC address. "
            "This object is only available if FrameType is ether(1) and "
            "EtherDestMacOp is specific(1).")
    );

    // Ether type
    m.add_leaf(
        s.key.frame.ether.etype,
        vtss::tag::Name("EtherType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates the packet's 16 bit Ethernet type field. "
            "Possible values are: 0(disabled), 0x600-0xFFFF but excluding "
            "0x800(IPv4), 0x806(ARP) and 0x86DD(IPv6). "
            "This object is only available if FrameType is ether(1) and "
            "EtherDestMacOp is specific(1).")
    );

    /*
        ARP
    */
    ix = (++iy) * 100;

    // ARP Source MAC
    m.add_leaf(
        s.key.frame.arp.smac_type,
        vtss::tag::Name("ArpSrcMacOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a ARP packet's source MAC "
            "address to be matched. "
            "This object is only available if FrameType is arp(4).")
    );

    m.add_leaf(
        s.key.frame.arp.smac,
        vtss::tag::Name("ArpSrcMac"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates the 48 bits source MAC address. "
            "This object is only available if FrameType is arp(4) and "
            "ArpSrcMacOp is specific(1).")
    );

    // ARP Source IP
    m.add_leaf(
        vtss::AsIpv4(s.key.frame.arp.sip.value),
        vtss::tag::Name("ArpSenderIp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The specified ARP sender IP address. "
            "The packet's sender address is AND-ed with the value of "
            "ArpSenderIpMask and then compared with the value of this object. "
            "If ArpSenderIp and ArpSrcIpMask are 0.0.0.0, this entry matches "
            "any sender IP address. "
            "This object is only available if FrameType is arp(4).")
    );

    m.add_leaf(
        vtss::AsIpv4(s.key.frame.arp.sip.mask),
        vtss::tag::Name("ArpSenderIpMask"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The specified ARP sender IP address mask. "
            "This object is only available if FrameType is arp(4).")
    );

    // ARP Destination IP
    m.add_leaf(
        vtss::AsIpv4(s.key.frame.arp.dip.value),
        vtss::tag::Name("ArpTargetIp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The specified ARP target IP address. "
            "The packet's target address is AND-ed with the value of "
            "ArpTragetIpMask and then compared with the value of this object. "
            "If ArpTragetIp and ArpSrcIpMask are 0.0.0.0, this entry matches "
            "any target IP address. "
            "This object is only available if FrameType is arp(4).")
    );

    m.add_leaf(
        vtss::AsIpv4(s.key.frame.arp.dip.mask),
        vtss::tag::Name("ArpTargetIpMask"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The specified ARP target IP address mask. "
            "This object is only available if FrameType is arp(4).")
    );

    // ARP Opcode
    m.add_leaf(
        s.key.frame.arp.flag.opcode,
        vtss::tag::Name("ArpOpcode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a ARP packet's opcode to be matched. "
            "This object is only available if FrameType is arp(4).")
    );

    // ARP Flag
    m.add_leaf(
        s.key.frame.arp.flag.req,
        vtss::tag::Name("ArpFlagReq"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a ARP packet's request/reply "
            "opcode is to be compared. "
            "This object is only available if FrameType is arp(4).")
    );

    m.add_leaf(
        s.key.frame.arp.flag.sha,
        vtss::tag::Name("ArpFlagSha"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a ARP packet's sender hardware "
            "address field (SHA) to be compared. "
            "This object is only available if FrameType is arp(4).")
    );

    m.add_leaf(
        s.key.frame.arp.flag.tha,
        vtss::tag::Name("ArpFlagTha"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a ARP packet's target hardware "
            "address field (THA) is to be compared. "
            "This object is only available if FrameType is arp(4).")
    );

    m.add_leaf(
        s.key.frame.arp.flag.hln,
        vtss::tag::Name("ArpFlagHln"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a ARP packet's s hardware "
            "address length field (HLN) is to be compared. "
            "The value 0 means any HLN value is allowed (don't-care field), "
            "value 1 means HLN is not equal to Ethernet (0x06) or the (PLN) "
            "is not equal to IPv4 (0x04) and value 2 means HLN is equal to "
            "Ethernet (0x06) or the (PLN) is not equal to IPv4 (0x04). "
            "This object is only available if FrameType is arp(4).")
    );

    m.add_leaf(
        s.key.frame.arp.flag.hrd,
        vtss::tag::Name("ArpFlagHrd"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a ARP packet's hardware address "
            "space field (HRD) is to be compared. "
            "The value 0 means any HRD value is allowed (don't-care field), "
            "value 1 means HRD is not equal to Ethernet (1) and "
            "value 2 means HRD is equal to Ethernet (1). "
            "This object is only available if FrameType is arp(4).")
    );

    m.add_leaf(
        s.key.frame.arp.flag.pro,
        vtss::tag::Name("ArpFlagPro"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a ARP packet's protocol address "
            "space field (PRO) is to be compared. "
            "The value 0 means any PRO value is allowed (don't-care field), "
            "value 1 means PRO is not equal to IP (0x800) and "
            "value 2 means PRO is equal to IP (0x800). "
            "This object is only available if FrameType is arp(4).")
    );

    /*
        IPv4
    */
    ix = (++iy) * 100;

    // IPv4 Protocol
    m.add_leaf(
        s.key.frame.ipv4.proto_type,
        vtss::tag::Name("Ipv4ProtocolOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a IPv4 packet's protocol field "
            "is to be compared. "
            "This object is only available if FrameType is ipv4(5).")
    );

    m.add_leaf(
        s.key.frame.ipv4.proto,
        vtss::tag::Name("Ipv4Protocol"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The protocol number field in the IPv4 header "
            "used to indicate a higher layer protocol. "
            "Possible values are 0-255. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4ProtocolOp is specific(1).")
    );

    // IPv4 Source IP
    m.add_leaf(
        vtss::AsIpv4(s.key.frame.ipv4.sip.value),
        vtss::tag::Name("Ipv4SrcIp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The specified source IPv4 address. "
            "The packet's sender address is AND-ed with the value of "
            "Ipv4SrcIpMask and then compared with the value of this object. "
            "If Ipv4SrcIp and Ipv4SrcIpMask are 0.0.0.0, this entry matches "
            "any source IP address. "
            "This object is only available if FrameType is ipv4(5).")
    );

    m.add_leaf(
        vtss::AsIpv4(s.key.frame.ipv4.sip.mask),
        vtss::tag::Name("Ipv4SrcIpMask"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The specified source IPv4 address mask. "
            "This object is only available if FrameType is ipv4(5).")
    );

    // IPv4 Destination IP
    m.add_leaf(
        vtss::AsIpv4(s.key.frame.ipv4.dip.value),
        vtss::tag::Name("Ipv4DestIp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The specified destination IPv4 address. "
            "The packet's sender address is AND-ed with the value of "
            "Ipv4DestIpMask and then compared with the value of this object. "
            "If Ipv4DestIp and Ipv4DestIpMask are 0.0.0.0, this entry matches "
            "any destination IP address. "
            "This object is only available if FrameType is ipv4(5).")
    );

    m.add_leaf(
        vtss::AsIpv4(s.key.frame.ipv4.dip.mask),
        vtss::tag::Name("Ipv4DestIpMask"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The specified destination IPv4 address mask. "
            "This object is only available if FrameType is ipv4(5).")
    );

    // IPv4 ICMP
    m.add_leaf(
        s.key.frame.ipv4.icmp.type_match,
        vtss::tag::Name("Ipv4IcmpTypeOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a IPv4 packet's ICMP type field "
            "is to be compared. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4Protocol is icmp(1).")
    );

    m.add_leaf(
        s.key.frame.ipv4.icmp.type,
        vtss::tag::Name("Ipv4IcmpType"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ICMP type field in the IPv4 header. "
            "Possible values are 0-255. "
            "This object is only available if FrameType is ipv4(5), "
            "Ipv4Protocol is icmp(1) and Ipv4IcmpTypeOp is specific(1).")
    );

    m.add_leaf(
        s.key.frame.ipv4.icmp.code_match,
        vtss::tag::Name("Ipv4IcmpCodeOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a IPv4 packet's ICMP code "
            "field is to be compared. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4Protocol is icmp(1).")
    );

    m.add_leaf(
        s.key.frame.ipv4.icmp.code,
        vtss::tag::Name("Ipv4IcmpCode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ICMP code field in the IPv4 header. "
            "Possible values are 0-255. "
            "This object is only available if FrameType is ipv4(5), "
            "Ipv4Protocol is icmp(1) and Ipv4IcmpCodeOp is specific(1).")
    );

    // IPv4 Source Port
    m.add_leaf(
        s.key.frame.ipv4.sport.match,
        vtss::tag::Name("Ipv4SrcPortOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a packet's source TCP/UDP port number is to be compared. "
            "This object is only available if FrameType is ipv4(5).")
    );

    m.add_leaf(
        s.key.frame.ipv4.sport.low,
        vtss::tag::Name("Ipv4SrcPort"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The source port number of the TCP or UDP protocol. "
            "If the Ipv4SrcPortOp object is range(2), "
            "this object will be the starting port number of the port range. "
            "Valid range is 0-65535. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4SrcPortOp is not any(0).")
    );

    m.add_leaf(
        s.key.frame.ipv4.sport.high,
        vtss::tag::Name("Ipv4SrcPortRange"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The source port number of the TCP or UDP protocol. "
            "If the Ipv4SrcPortOp object is range(2), "
            "this object will be the ending port number of the port range. "
            "Valid range is 0-65535. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4SrcPortOp is range(2).")
    );

    // IPv4 Destination Port
    m.add_leaf(
        s.key.frame.ipv4.dport.match,
        vtss::tag::Name("Ipv4DestPortOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a packet's destination TCP/UDP port number is to be compared. "
            "This object is only available if FrameType is ipv4(5).")
    );

    m.add_leaf(
        s.key.frame.ipv4.dport.low,
        vtss::tag::Name("Ipv4DestPort"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The destination port number of the TCP or UDP protocol. "
            "If the Ipv4DestPortOp object in the same row is range(2), "
            "this object will be the starting port number of the port range. "
            "Valid range is 0-65535. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4DestPortOp is not any(0).")
    );

    m.add_leaf(
        s.key.frame.ipv4.dport.high,
        vtss::tag::Name("Ipv4DestPortRange"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The destination port number of the TCP or UDP protocol. "
            "If the Ipv4DestPortOp object in the same row is range(2), "
            "this object will be the ending port number of the port range. "
            "Valid range is 0-65535. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4DestPortOp is range(2).")
    );

    // IPv4 flag
    m.add_leaf(
        s.key.frame.ipv4.ipv4_flag.ttl,
        vtss::tag::Name("Ipv4FlagTtl"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a IPv4 packet's Time-to-Live field (TTL) is to be compared. "
            "The value 0 means any TTL value is allowed (don't-care field), "
            "value 1 means TTL is not equal zero and "
            "value 2 means TTL is equal to zero. "
            "This object is only available if FrameType is ipv4(5).")
    );

    m.add_leaf(
        s.key.frame.ipv4.ipv4_flag.fragment,
        vtss::tag::Name("Ipv4FlagFragment"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a IPv4 packet's More-Fragments field (MF) is to be compared. "
            "The value 0 means any MF value is allowed (don't-care field), "
            "value 1 means MF is not equal one (MF field isn't set) and "
            "value 2 means MF is equal to one (MF field is set). "
            "This object is only available if FrameType is ipv4(5).")
    );

    m.add_leaf(
        s.key.frame.ipv4.ipv4_flag.option,
        vtss::tag::Name("Ipv4FlagIpOption"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a IPv4 packet's options field is to be compared. "
            "This object is only available if FrameType is ipv4(5).")
    );

    // IPv4 TCP flag
    m.add_leaf(
        s.key.frame.ipv4.tcp_flag.fin,
        vtss::tag::Name("Ipv4TcpFlagFin"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how the IPv4 TCP FIN flag is matched. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4Protocol is tcp(6).")
    );

    m.add_leaf(
        s.key.frame.ipv4.tcp_flag.syn,
        vtss::tag::Name("Ipv4TcpFlagSyn"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how the IPv4 TCP SYN flag is matched. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4Protocol is tcp(6).")
    );

    m.add_leaf(
        s.key.frame.ipv4.tcp_flag.rst,
        vtss::tag::Name("Ipv4TcpFlagRst"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how the IPv4 TCP RST flag is matched. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4Protocol is tcp(6).")
    );

    m.add_leaf(
        s.key.frame.ipv4.tcp_flag.psh,
        vtss::tag::Name("Ipv4TcpFlagPsh"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how the IPv4 TCP PSH flag is matched. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4Protocol is tcp(6).")
    );

    m.add_leaf(
        s.key.frame.ipv4.tcp_flag.ack,
        vtss::tag::Name("Ipv4TcpFlagAck"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how the IPv4 TCP ACK flag is matched. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4Protocol is tcp(6).")
    );

    m.add_leaf(
        s.key.frame.ipv4.tcp_flag.urg,
        vtss::tag::Name("Ipv4TcpFlagUrg"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how the IPv4 TCP URG flag is matched. "
            "This object is only available if FrameType is ipv4(5) and "
            "Ipv4Protocol is tcp(6).")
    );

    /*
        IPv6
    */
    ix = (++iy) * 100;

    // IPv6 Next Header
    m.add_leaf(
        s.key.frame.ipv6.next_header_type,
        vtss::tag::Name("Ipv6NextHeaderOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Specify the IPv6 next header filter for this ACE. "
            "This object is only available if FrameType is ipv6(6).")
    );

    m.add_leaf(
        s.key.frame.ipv6.next_header,
        vtss::tag::Name("Ipv6NextHeader"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("When 'Specific' is selected for the IPv6 next header value, "
            "you can enter a specific value. The allowed range is 0 to 255. "
            "A frame that hits this ACE matches this IPv6 protocol value. "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6NextHeaderOp is specific(1).")
    );

    // IPv6 ICMP
    m.add_leaf(
        s.key.frame.ipv6.icmp.type_match,
        vtss::tag::Name("Ipv6Icmpv6TypeOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a IPv6 packet's ICMPv6 type field "
            "is to be compared. "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6NextHeader is icmpv6(58).")
    );

    m.add_leaf(
        s.key.frame.ipv6.icmp.type,
        vtss::tag::Name("Ipv6Icmpv6Type"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ICMPv6 type field in the IPv6 header. "
            "Possible values are 0-255. "
            "This object is only available if FrameType is ipv6(6), "
            "Ipv6NextHeader is  icmpv6(58) and Ipv6Icmpv6TypeOp is specific(1).")
    );

    m.add_leaf(
        s.key.frame.ipv6.icmp.code_match,
        vtss::tag::Name("Ipv6Icmpv6CodeOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates how a IPv6 packet's ICMPv6 code "
            "field is to be compared. "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6NextHeader is icmpv6(58).")
    );

    m.add_leaf(
        s.key.frame.ipv6.icmp.code,
        vtss::tag::Name("Ipv6Icmpv6Code"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ICMP code field in the IPv6 header. "
            "Possible values are 0-255. "
            "This object is only available if FrameType is ipv6(6), "
            "Ipv6NextHeader is icmpv6(58) and Ipv6Icmpv6CodeOp is specific(1).")
    );
    
    // IPv6 Source IP
    m.add_leaf(
        s.key.frame.ipv6.sip.value,
        vtss::tag::Name("Ipv6SrcIp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The specified source IPv6 address. "
            "The packet's sender address is AND-ed with the value of "
            "Ipv6SrcIpMask and then compared with the value of this object. "
            "If Ipv6SrcIp and Ipv4SrcIpMask are 0::, this entry matches any source IP address. "
            "This object is only available if FrameType is ipv6(6).")
    );

    m.add_leaf(
        s.key.frame.ipv6.sip.mask,
        vtss::tag::Name("Ipv6SrcIpMask"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The specified source IPv6 address mask. "
            "This object is only available if FrameType is ipv6(6).")
    );

    // IPv6 Source Port
    m.add_leaf(
        s.key.frame.ipv6.sport.match,
        vtss::tag::Name("Ipv6SrcPortOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a packet's source TCP/UDP port number is to be compared. "
            "This object is only available if FrameType is ipv6(6).")
    );

    m.add_leaf(
        s.key.frame.ipv6.sport.low,
        vtss::tag::Name("Ipv6SrcPort"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The source port number of the TCP or UDP protocol. "
            "If the Ipv6SrcPortOp object in the same row is range(2), "
            "this object will be the starting port number of the port range. "
            "Valid range is 0-65535. "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6SrcPortOp is not any(0).")
    );

    m.add_leaf(
        s.key.frame.ipv6.sport.high,
        vtss::tag::Name("Ipv6SrcPortRange"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The source port number of the TCP or UDP protocol. "
            "If the Ipv6SrcPortOp object in the same row is range(2), "
            "this object will be the ending port number of the port range. "
            "Valid range is 0-65535. "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6SrcPortOp is range(2).")
    );

    // IPv6 Destination Port
    m.add_leaf(
        s.key.frame.ipv6.dport.match,
        vtss::tag::Name("Ipv6DestPortOp"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a packet's destination TCP/UDP port number is to be compared. "
            "This object is only available if FrameType is ipv6(6).")
    );

    m.add_leaf(
        s.key.frame.ipv6.dport.low,
        vtss::tag::Name("Ipv6DestPort"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The destination port number of the TCP or UDP protocol. "
            "If the Ipv6DestPortOp object in the same row is range(2), "
            "this object will be the starting port number of the port range. "
            "Valid range is 0-65535. "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6DestPortOp is not any(0).")
    );

    m.add_leaf(
        s.key.frame.ipv6.dport.high,
        vtss::tag::Name("Ipv6DestPortRange"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "The destination port number of the TCP or UDP protocol. "
            "If the Ipv6DestPortOp object in the same row is range(2), "
            "this object will be the ending port number of the port range. "
            "Valid range is 0-65535. "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6DestPortOp is range(2).")
    );

    // IPv6 flag
    m.add_leaf(
        s.key.frame.ipv6.ipv6_flag.ttl,
        vtss::tag::Name("Ipv6FlagTtl"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a IPv6 packet's Time-to-Live field (TTL) is to be compared. "
            "The value 0 means any TTL value is allowed (don't-care field), "
            "value 1 means TTL is not equal zero and "
            "value 2 means TTL is equal to zero. "
            "This object is only available if FrameType is ipv6(6).")
    );

    // IPv6 TCP flag
    m.add_leaf(
        s.key.frame.ipv6.tcp_flag.fin,
        vtss::tag::Name("Ipv6TcpFlagFin"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a IPv6 TCP packet's 'No more data from sender' "
            "field (FIN) is to be compared. "
            "The value 0 means any FIN value is allowed (don't-care field), "
            "value 1 means FIN is not equal one (FIN field isn't set) and "
            "value 2 means FIN is equal to one (FIN field is set). "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6NextHeader is tcp(6).")
    );

    m.add_leaf(
        s.key.frame.ipv6.tcp_flag.syn,
        vtss::tag::Name("Ipv6TcpFlagSyn"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a IPv6 TCP packet's 'Synchronize sequence numbers' "
            "field (SYN) is to be compared. "
            "The value 0 means any SYN value is allowed (don't-care field), "
            "value 1 means SYN is not equal one (FIN field isn't set) and "
            "value 2 means SYN is equal to one (FIN field is set). "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6NextHeader is tcp(6).")
    );

    m.add_leaf(
        s.key.frame.ipv6.tcp_flag.rst,
        vtss::tag::Name("Ipv6TcpFlagRst"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a IPv6 TCP packet's 'Reset the connection' field "
            "(RST) is to be compared. "
            "The value 0 means any RST value is allowed (don't-care field), "
            "value 1 means RST is not equal one (FIN field isn't set) and "
            "value 2 means RST is equal to one (FIN field is set). "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6NextHeader is tcp(6).")
    );

    m.add_leaf(
        s.key.frame.ipv6.tcp_flag.psh,
        vtss::tag::Name("Ipv6TcpFlagPsh"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a IPv6 TCP packet's 'Push Function' field (PSH) is "
            "to be compared. "
            "The value 0 means any PSH value is allowed (don't-care field), "
            "value 1 means PSH is not equal one (FIN field isn't set) and "
            "value 2 means PSH is equal to one (FIN field is set). "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6NextHeader is tcp(6).")
    );

    m.add_leaf(
        s.key.frame.ipv6.tcp_flag.ack,
        vtss::tag::Name("Ipv6TcpFlagAck"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a IPv6 TCP packet's 'Acknowledgment field significant' "
            "field (ACK) is to be compared. "
            "The value 0 means any FIN value is allowed (don't-care field), "
            "value 1 means ACK is not equal one (FIN field isn't set) and "
            "value 2 means ACK is equal to one (FIN field is set). "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6NextHeader is tcp(6).")
    );

    m.add_leaf(
        s.key.frame.ipv6.tcp_flag.urg,
        vtss::tag::Name("Ipv6TcpFlagUrg"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description(
            "Indicates how a IPv6 TCP packet's 'Urgent Pointer field significant' "
            "field (URG) is to be compared. "
            "The value 0 means any URG value is allowed (don't-care field), "
            "value 1 means URG is not equal one (FIN field isn't set) and "
            "value 2 means URG is equal to one (FIN field is set). "
            "This object is only available if FrameType is ipv6(6) and "
            "Ipv6NextHeader is tcp(6).")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_acl_config_ace_precedence_t &s) {
    int ix = 0;

    a.add_leaf(
        s.ace_id,
        vtss::tag::Name("AceId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Identifier of ACE")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_acl_config_interface_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_acl_config_interface_t"));

    int ix = 1;
    int iy = 0;

    m.add_leaf(
        s.policy_id,
        vtss::tag::Name("PolicyId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The policy ID.")
    );

    /*
        Action
    */
    ix = (++iy) * 100;

    m.add_leaf(
        s.action.hit_action,
        vtss::tag::Name("HitAction"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The hit action operation for this interface. "
            "egress(2) is not supported.")
    );

    m.add_leaf(
        (vtss::PortListStackable &)(s.action.redirect_port),
        vtss::tag::Name("RedirectPortList"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The redirect port list for this interface.")
    );

    m.add_leaf(
        s.action.rate_limiter_id,
        vtss::tag::Name("RateLimiterId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The rate limiter ID. 0 means to be disabled.")
    );

    m.add_leaf(
        (uint16_t)0,
        vtss::tag::Name("EvcPolicerId"),
        vtss::expose::snmp::Status::Obsoleted,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapActionEvcPolicerSupported>(),
        vtss::tag::Description("The EVC policer ID. 0 means to be disabled. Obsolete.")
    );

    m.add_leaf(
        vtss::AsBool(s.action.mirror),
        vtss::tag::Name("Mirror"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::DependOnCapability<AclCapActionMirrorSupported>(),
        vtss::tag::Description("The mirror operation. "
            "Frames matching this interface rule are mirrored to the "
            "destination mirror port that is configured in the mirror modules.")
    );

    m.add_leaf(
        vtss::AsBool(s.action.logging),
        vtss::tag::Name("Logging"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Specify the logging operation of the interface. "
            "Notice that the logging message doesn't include the 4 bytes CRC "
            "information. The allowed values are: True - Frames matching the "
            "ACE are stored in the System Log. False - Frames matching the "
            "ACE are not logged. Note: The logging feature only works when "
            "the packet length is less than 1518(without VLAN tags) and the "
            "System Log memory size and logging rate is limited.")
    );

    m.add_leaf(
        vtss::AsBool(s.action.shutdown),
        vtss::tag::Name("Shutdown"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Specify the port shut down operation of the interface. "
        "The allowed values are: True - If a frame matches the interface rule, the "
        "ingress port will be disabled. False - Port shut down is disabled "
        "for the interface. Note: The shutdown feature only works when the packet "
        "length is less than 1518(without VLAN tags).")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_acl_status_ace_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_acl_status_ace_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.acl_user, sizeof(s.acl_user)),
        vtss::tag::Name("AclUser"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("User of ACE")
    );

    m.add_leaf(
        s.ace_id,
        vtss::tag::Name("AceId"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Identifier of ACE")
    );

    m.add_leaf(
        vtss::AsBool(s.conflict),
        vtss::tag::Name("Conflict"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The conflict status of this ACE. When the "
            "status value is true, it means the specific ACE is not "
            "applied to the hardware due to hardware limitations.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_acl_status_ace_hit_count_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_acl_status_ace_hit_count_t"));
    int ix = 0;

    m.add_leaf(
        s.counter,
        vtss::tag::Name("Counter"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The hit count of this ACE.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_acl_status_interface_hit_count_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_acl_status_interface_hit_count_t"));
    int ix = 0;

    m.add_leaf(
        s.counter,
        vtss::tag::Name("Counter"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The hit count of this interface.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_acl_control_globals_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_acl_control_globals_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.clear_all_hit_count),
        vtss::tag::Name("ClearAllHitCount"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Set true to clear hit counts of all ACEs and interfaces.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_acl_control_interface_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_acl_control_interface_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.state),
        vtss::tag::Name("State"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Specify the port status. True is to enable "
            "the port and false is to shutdown the port")
    );
}

/* JSON notification */
template<typename T>
void serialize(T &a, vtss_appl_acl_status_ace_event_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_acl_status_ace_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.crossed_threshold),
        vtss::tag::Name("CrossedThreshold"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The ACE event status. When the status value is true, "
            "it means the ACE status is crossed the hardware threshold.")
    );
}

namespace vtss {
namespace appl {
namespace acl {
namespace interfaces {

struct AclCapabilitiesLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_acl_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_acl_capabilities_t &s) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));

        typename HANDLER::Map_t m = h.as_map(vtss::tag::Typename("vtss_appl_acl_capabilities_t"));

        int ix = 0;

        m.template capability<AclCapAceIdMax>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapPolicyIdMax>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapRateLimiterIdMax>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapEvcPolicerIdMax>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapRateLimiterBitRateSupported>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapActionEvcPolicerSupported>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapActionMirrorSupported>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapActionMultipleRedirectPortsSupported>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapSecondLookupSupported>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapMultipleIngressPortsSupported>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapEgressPortSupported>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapVlanTaggedSupported>(vtss::expose::snmp::OidElementValue(ix++));
        m.template capability<AclCapStackableAceSupported>(vtss::expose::snmp::OidElementValue(ix++));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_acl_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct AclConfigRateLimiterEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<mesa_acl_policer_no_t>,
        vtss::expose::ParamVal<vtss_appl_acl_config_rate_limiter_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of rate limiter configuration";

    static constexpr const char *index_description =
        "Each rate limiter has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(mesa_acl_policer_no_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, acl_rate_limiter_id_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_acl_config_rate_limiter_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_acl_config_rate_limiter_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_acl_config_rate_limiter_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_acl_config_rate_limiter_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct AclConfigAceEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_acl_ace_id_t>,
        vtss::expose::ParamVal<vtss_appl_acl_config_ace_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 10000;
    static constexpr const char *table_description =
        "The ACL ACE configuration table.";

    static constexpr const char *index_description =
        "Each row contains a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_acl_ace_id_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, acl_ace_id_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_acl_config_ace_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_acl_config_ace_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_acl_config_ace_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_acl_config_ace_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_acl_config_ace_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_acl_config_ace_del);
    VTSS_EXPOSE_DEF_PTR(vtss_appl_acl_config_ace_default);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct AclConfigAcePrecedenceEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_acl_config_ace_precedence_t *>
    > P;

    static constexpr const char *table_description =
        "It displays the sequence of ACEs to be hit.";

    static constexpr const char *index_description =
        "Each row contains the corresponding ACE ID.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, acl_precedence_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_acl_config_ace_precedence_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_acl_config_ace_precedence_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_acl_config_ace_precedence_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct AclConfigInterfaceEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_acl_config_interface_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of interface configuration";

    static constexpr const char *index_description =
        "Each interface has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, acl_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_acl_config_interface_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_acl_config_interface_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_acl_config_interface_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_acl_config_interface_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct AclStatusAceEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_usid_t>,
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_acl_status_ace_t *>
    > P;

    static constexpr const char *table_description =
        "Table of ACE status.";

    static constexpr const char *index_description =
        "Each row contains the status per switch per precedence.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, acl_usid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, acl_precedence_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_acl_status_ace_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_acl_status_ace_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_acl_status_ace_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct AclStatusAceHitCountEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_acl_ace_id_t>,
        vtss::expose::ParamVal<vtss_appl_acl_status_ace_hit_count_t *>
    > P;

    static constexpr const char *table_description =
        "Table of ACE hit count status.";

    static constexpr const char *index_description =
        "Each row contains the hit count status per ACE.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_acl_ace_id_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, acl_ace_id_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_acl_status_ace_hit_count_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_acl_status_ace_hit_count_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_acl_status_ace_hit_count_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct AclStatusInterfaceHitCountEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_acl_status_interface_hit_count_t *>
    > P;

    static constexpr const char *table_description =
        "Table of interface hit count status.";

    static constexpr const char *index_description =
        "Each row contains the hit count status per interface.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, acl_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_acl_status_interface_hit_count_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_acl_status_interface_hit_count_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_acl_status_interface_hit_count_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct AclControlGlobalsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_acl_control_globals_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_acl_control_globals_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_acl_control_globals_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_acl_control_globals_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct AclControlInterfaceEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamVal<vtss_appl_acl_control_interface_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of interface control";

    static constexpr const char *index_description =
        "Each interface has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, acl_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_acl_control_interface_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_acl_control_interface_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_acl_control_interface_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_acl_control_interface_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

/* JSON notification */
struct AclStatusAceEventEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_usid_t>,
        vtss::expose::ParamVal<vtss_appl_acl_status_ace_event_t *>
    > P;

    static constexpr const char *table_description =
        "Table of ACE event status.";

    static constexpr const char *index_description =
        "Each row contains the event status per switch.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_usid_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, acl_usid_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_acl_status_ace_event_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_acl_status_ace_event_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_acl_status_ace_event_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

}  // namespace interfaces
}  // namespace acl
}  // namespace appl
}  // namespace vtss

#endif /* __ACL_SERIALIZER_HXX__ */
