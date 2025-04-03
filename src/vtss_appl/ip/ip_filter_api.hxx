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

#ifndef _IP_FILTER_API_HXX_
#define _IP_FILTER_API_HXX_

/**
 * /file
 * /brief IP filter API.
 *
 * Overview
 * ========
 *
 * This API is implemented for Linux, only.
 *
 * The purpose of this API is to provide a filtering mechanism that can prevent
 * frames from being injected into L3 interfaces.
 *
 * In previous WebStaX design all frames were passed through all modules that have
 * registered a callback handler. Each of these modules could consume the packet
 * and thereby prevent the packet from being seen by subsequent handlers. To
 * allow for firewall-like ARP inspection and similar, the L3 interfaces are
 * placed at the end of this call chain.
 *
 * This design has been changed on Linux targets, where frames are being
 * dispatched to the switch user-space application in parallel with being
 * injected into the IP stack. This means that the user-space application cannot
 * perform the filtering the same way it used to.
 *
 * This API provides a filtering mechanism that may be used to filter out frames
 * that would otherwise get injected into the IP stack. Such frames are still
 * exposed to the user-space application through the vtss.ifh interface.
 *
 * The filtering engine must set a bit in the VTSS-Header to signal whether the
 * frame was prevented from being injected to the IP stack or not. This will
 * allow the switch application to implement various counting and logging on
 * frames that have been filtered out. TODO, not done yet.
 *
 * NOTE: All frames will be avialable on the vtss.ifh interface whether or not
 * they are dropped by a filter.
 *
 * Filtering
 * =========
 *
 * The filtering mechanism is designed around two lists - a deny list and an
 * allow list. Each list may contain a number of rules.
 *
 * A rule is composed by a number of 'Element' instances, and if a given frame
 * matches all elements, the rule is considered matched. Each element can match
 * various part of a frame, and a rule with multiple elements can thereby
 * correlate various part of the frame when matching.
 *
 * When a frame is being analyzed by the filter it will be matched against the
 * rules (one-by-one) in the deny list. Each rule in the deny list is
 * associated with an action: 'drop' or 'check_allow_list'. If a given frame
 * hits a rule in the deny list and the associated action is 'drop' then the
 * processing stops here and the frame will be prevented from entering the IP
 * stack.
 *
 * If the frame hits a rule with the 'check_allow_list' action, then the owner
 * of the matching rule is recorded in the 'gray_owners' list[1], and the processing
 * of the deny list continues. If the processing of the deny list continues
 * to the end, without hitting a 'drop' rule, then the frame will go through
 * further processing.
 *
 * When the deny list has completed the processing, the frame is allowed to
 * enter the IP stack if the 'gray_owners' list is empty, otherwise it must
 * continue to the allow-list processing.
 *
 * Processing of the allow list is similar to the deny list. The rules are
 * being processed one by one, and if a frame matches a rule in the allow list,
 * the owner of the allow list rule is deleted from the 'gray_owners' list if
 * present in that list (if not, nothing will happen). After the allow list is
 * processed, the 'gray_owners' list is checked. If it is empty (all owners have
 * been deleted by a matching allow list rule), the frame is allowed to enter
 * the IP stack, otherwise it is not.
 *
 * The process is illustrated in the following pseudo code:
 *
 *     filter_out_frame(frame f) {
 *             gray_owners = []
 *             for each rule r in deny_list do
 *                     if rule_match(r, f)
 *                             if r.action == DROP
 *                                     return TRUE  // Drop the frame
 *                             else if r.action == check_allow_list
 *                                     list_add(gray_owners, r.owner)
 *
 *             for each rule r in allow_list do
 *                     if rule_match(r, f)
 *                             list_del_by_value(gray_owners, r.owner)
 *
 *             if list_empty(gray_owners)
 *                     return FALSE  // Accept the frame
 *             else
 *                     return TRUE   // Drop the frame
 *     }
 *
 *
 * [1] The 'gray_owners' list is instantiated for each frame, while the deny
 * and allow lists are global instances.
 */

#include <main_types.h>
#include <vtss/basics/array.hxx>
#include <vtss/basics/vector.hxx>

#include <bitset>

#define PORT_MASK_BITS 128
#define PORT_MASK_LEN (PORT_MASK_BITS/8)

namespace vtss
{
namespace appl
{
namespace ip
{
namespace filter
{

/**
 * Enumerating the set of supported actions that may be associated with a rule
 * in the deny list. */
enum class Action {
    /**
     * Drop the packet means that the packet will not be injected into the IP
     * stack. The rule checking engine will stop evaluating rules after a 'drop'
     * action has been hit. */
    drop,

    /**
     * Check allow list means that the packet is being dropped (not injected
     * into the IP stack) unless there is a rule in the allow list that can
     * clear the packet. The rule checking engine will continue evaluating
     * deny list rules after a 'check_allow_list' rule has been hit. If later
     * on the frame matches a rule with a 'drop' action, the packet is dropped
     * without considering the allow list. */
    check_allow_list,
};

/** Enumerating the set of supported rule element types. */
enum class Type {
    /** Invalid element. */
    none = 0,

    /**
     * Check if the reception port is included in the port mask
     * (Element::data::mask). */
    port_mask = 1,

    /**
     * Check if the source MAC address matches the provided MAC address
     * (Element::data::mac). */
    mac_src = 2,

    /**
     * Check if the destination MAC address matches the provided MAC address
     * (Element::data::mac). */
    mac_dst = 3,

    /**
     * Check if the destination or source MAC address matches the provided MAC
     * address (Element::data::mac). */
    mac_src_or_dst = 4,

    /**
     * Check if the classified VLAN matches the provided VLAN
     * (Element::data::vlan). */
    vlan = 5,

    /**
     * Check if the ether type matches the provided protocol number
     * (Element::data::protocol). */
    ether_type = 6,

    /**
     * Check if the source IPv4 address is included in the provided IPv4
     * network (Element::data::ipv4). */
    ipv4_src = 7,

    /**
     * Check if the destination IPv4 address is included in the provided IPv4
     * network (Element::data::ipv4). */
    ipv4_dst = 8,

    /**
     * Check if the source or destination IPv4 address is included in the
     * provided IPv4 network (Element::data::ipv4). */
    ipv4_src_or_dst = 9,

    /**
     * Check if the source IPv6 address is included in the provided IPv6
     * network (Element::data::ipv6). */
    ipv6_src = 10,

    /**
     * Check if the destination IPv6 address is included in the provided IPv6
     * network (Element::data::ipv6). */
    ipv6_dst = 11,

    /**
     * Check if the source or destination IPv6 address is included in the
     * provided IPv6 network (Element::data::ipv6). */
    ipv6_src_or_dst = 12,

    /**
     * Check if the ARP operation matches the value provided in
    //(Element::data::operation). */
    arp_operation = 22,

    /**
     * Check if the sender hardware address (SHA) matches the provided MAC
     * address (Element::data::mac). */
    arp_hw_sender = 23,

    /**
     * Check if the target hardware address (THA) matches the provided MAC
     * address (Element::data::mac). */
    arp_hw_target = 24,

    /**
     * Check if the sender protocol address (SPA) is included in the provided
     * IPv4 network. (Element::data::ipv4). */
    arp_proto_sender = 25,

    /**
     * Check if the target protocol address (TPA) is included in the provided
     * IPv4 network. (Element::data::ipv4). */
    arp_proto_target = 26,

    /**
     * Check if the frame has hit a specific ACL rule included in the bitmask.
     * (matched against the ACL ID field in the private frame header.
     * (Element::data::acl_id).
     *
     * This facility is only available on Luton26 targets. */
    acl_id = 27,

    /**
     * Check if the frame is a gratuitous ARP frame (sender protocol address ==
     * target protocol address) */
    arp_gratuitous = 28,
};

typedef Array<u8, PORT_MASK_LEN> PortMaskArray;

class PortMask : public std::bitset<PORT_MASK_BITS>
{
private:
    void to_arr(u8 *byte, int idx)
    {
        u8 res = 0;
        int end = idx + 8;

        while (idx < end) {
            if (test(idx)) {
                res |= 1 << (idx & 0x7);
            }

            ++idx;
        }

        *byte = res;
    }

    void from_arr(u8 *byte, int idx)
    {
        u8 res = *byte;
        int end = idx + 8;

        while (idx < end) {
            if (res & 1) {
                set(idx);
            }

            res >>= 1;
            ++idx;
        }
    }

public:
    void to_array(u8 *bytes)
    {
        for (int idx = 0; idx < size(); idx += 8, ++bytes) {
            to_arr(bytes, idx);
        }
    }

    PortMask()
    {
    }

    PortMask(int idx)
    {
        set(idx);
    }

    PortMask(PortMaskArray arr)
    {
        u8 *bytes = arr.data;
        for (int idx = 0; idx < size(); idx += 8, ++bytes) {
            from_arr(bytes, idx);
        }
    }
};

/** Bitmask, used to match against a range of values. */
struct Bitmask {
    /** Offset.
     * When set to 0, the range covered is   0-127
     * When set to 1, the range covered is 128-255
     * When set to 2, the range covered is 256-383
     * When set to 3, the range covered is 384-512
     * etc.
     * */
    u32 offset;

    /** Bitmask area, relative to offset. */
    Array<u8, 16> buf;
};

/** The Element class is representing a single match criterion. */
struct Element {
    /** Specifies the type of the rule. */
    Type type = Type::none;

    union {
        /** Port mask to match on. Used along with Type::port_mask. */
        PortMaskArray mask;

        /**
         * MAC address to match on. Used along with Type::mac_src,
         * Type::mac_dst, Type::mac_src_or_dst, Type::arp_hw_sender
         * and Type::arp_hw_target */
        mesa_mac_t mac;

        /** VLAN to match on. Used along with Type::vlan. */
        mesa_vid_t vlan;

        /**
         * IPv4 network to match on. Used along with Type::ipv4_src,
         * Type::ipv4_dst, Type::ipv4_src_or_dst, Type::arp_proto_sender
         * and Type::arp_proto_target. */
        mesa_ipv4_network_t ipv4;

        /**
         * IPv6 network to match on. Used along with Type::ipv6_src,
         * Type::ipv6_dst, Type::ipv6_src_or_dst. */
        mesa_ipv6_network_t ipv6;

        /**
         * Protocol number to match on.
         * - If used with Type::ether_type it will represent an ether type. */
        u32 protocol;

        /**
         * Protocol operation. Used with Type::arp_operation. */
        u32 operation;

        /** ACL ID. Used with Type::acl_id.
         * It will match if the frame matches at least one of the acl_ids
         * included in the bitmask. */
        Bitmask acl_id;

    } data;
};

inline Element element_port_mask(PortMask mask)
{
    Element e;
    e.type = Type::port_mask;
    mask.to_array(e.data.mask.data);
    return e;
}

inline Element element_mac_src(mesa_mac_t m)
{
    Element e;
    e.type = Type::mac_src;
    e.data.mac = m;
    return e;
}

inline Element element_mac_dst(mesa_mac_t m)
{
    Element e;
    e.type = Type::mac_dst;
    e.data.mac = m;
    return e;
}

inline Element element_mac_src_or_dst(mesa_mac_t m)
{
    Element e;
    e.type = Type::mac_src_or_dst;
    e.data.mac = m;
    return e;
}

inline Element element_vlan(mesa_vid_t v)
{
    Element e;
    e.type = Type::vlan;
    e.data.vlan = v;
    return e;
}

inline Element element_ether_type(u16 et)
{
    Element e;
    e.type = Type::ether_type;
    e.data.protocol = et;
    return e;
}

inline Element element_ipv4_src(mesa_ipv4_network_t n)
{
    Element e;
    e.type = Type::ipv4_src;
    e.data.ipv4 = n;
    return e;
}

inline Element element_ipv4_dst(mesa_ipv4_network_t n)
{
    Element e;
    e.type = Type::ipv4_dst;
    e.data.ipv4 = n;
    return e;
}

inline Element element_ipv4_src_or_dst(mesa_ipv4_network_t n)
{
    Element e;
    e.type = Type::ipv4_src_or_dst;
    e.data.ipv4 = n;
    return e;
}

inline Element element_ipv6_src(mesa_ipv6_network_t n)
{
    Element e;
    e.type = Type::ipv6_src;
    e.data.ipv6 = n;
    return e;
}

inline Element element_ipv6_dst(mesa_ipv6_network_t n)
{
    Element e;
    e.type = Type::ipv6_dst;
    e.data.ipv6 = n;
    return e;
}

inline Element element_ipv6_src_or_dst(mesa_ipv6_network_t n)
{
    Element e;
    e.type = Type::ipv6_src_or_dst;
    e.data.ipv6 = n;
    return e;
}

inline Element element_arp_operation(int o)
{
    Element e;
    e.type = Type::arp_operation;
    e.data.operation = o;
    return e;
}

inline Element element_arp_hw_sender(mesa_mac_t m)
{
    Element e;
    e.type = Type::arp_hw_sender;
    e.data.mac = m;
    return e;
}

inline Element element_arp_hw_target(mesa_mac_t m)
{
    Element e;
    e.type = Type::arp_hw_target;
    e.data.mac = m;
    return e;
}

inline Element element_arp_proto_sender(mesa_ipv4_network_t n)
{
    Element e;
    e.type = Type::arp_proto_sender;
    e.data.ipv4 = n;
    return e;
}

inline Element element_arp_proto_target(mesa_ipv4_network_t n)
{
    Element e;
    e.type = Type::arp_proto_target;
    e.data.ipv4 = n;
    return e;
}

inline Element element_acl_id(Bitmask m)
{
    Element e;
    e.type = Type::acl_id;
    e.data.acl_id = m;
    return e;
}

inline Element element_arg_gratuitous()
{
    Element e = {};
    e.type = Type::arp_gratuitous;
    return e;
}

/**
 * A rule is simply a sequence of entries. A rule is considered a match if all
 * entries in the rule match. */
typedef Vector<Element> Rule;

/**
 * Representing the owner of a given rule. The idea is that modules should
 * create a const static instance of this and pass it along when
 * adding/deleting rules. The library will only be using the pointer, the
 * values inside are only for debugging. */
struct Owner {
    /** Module ID */
    int module_id;

    /** Rule name. */
    const char *name;
};

/**
 * The #id in the following function declarations is a value that can never
 * become VTSS_IP_FILTER_ID_NONE, so you may use this #define in your code to
 * figure out whether you already have a rule installed or not.
 */
#define VTSS_IP_FILTER_ID_NONE (0)

/**
 * Add a rule to the deny list.
 *
 * @param id [OUT] If the rule can be installed successfully then the 'id'
 *                 field will be updated with a unique number that must be used
 *                 to manipulate or delete the rule at a later point in time.
 * @param o  [IN]  The owner is used to correlate entries in the deny list and
 *                 the allow list, and it allows for deleting a group of rules,
 *                 and for debugging.
 * @param r  [IN]  The rule to install in the deny list.
 * @param a  [IN]  The action to perform if the rule matches.
 *
 * @return    Return code. */
mesa_rc deny_list_rule_add(int *id, const Owner *o, const Rule &r, Action a);

/**
 * Add a rule to the allow list.
 *
 * @param id [OUT] If the rule can be installed successfully then the 'id'
 *                 field will be updated with a unique number that must be used
 *                 to manipulate or delete the rule at a later point in time.
 * @param o  [IN]  The owner is used to correlate entries in the deny list and
 *                 the allow list, and it allows for deleting a group of rules,
 *                 and for debugging.
 * @param r  [IN]  The rule to install in the allow list.
 *
 * @return    Return code. */
mesa_rc allow_list_rule_add(int *id, const Owner *o, const Rule &r);

/**
 * Update an already installed rule.
 *
 * @param id  ID of the rule to be updated (replaced)
 * @param r   Rule to replace the old rule with.
 *
 * @return    Return code. This method will fail if the rule has not already
 *            been created. */
mesa_rc rule_update(int id, const Rule &r);

/**
 * Delete a group of rules. All rules installed with the provided owner 'o'
 * (only the pointer is used when comparing) will be deleted.
 *
 * @param o   [IN]  Owner to look for when deciding which rules must be deleted.
 * @param cnt [OUT] Returns the number of deleted rules.
 *
 * @return    Return code. This method will only fail in case of netlink error
 *            or similar. Zero rules deleted is not considered an error. */
mesa_rc rule_del(Owner *o, u32 *cnt);

/**
 * Delete a rule from the list.
 *
 * @param id [IN]  ID of the rule to be deleted.
 *
 * @return    Return code. This method will fail if the rule doesn't exist. */
mesa_rc rule_del(int id);

mesa_rc port_conf_update(void);

}  // namespace filter
}  // namespace ip
}  // namespace appl
}  // namespace vtss

#endif  // _IP_FILTER_API_HXX_

