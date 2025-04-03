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

#include "misc_api.h"
#include "packet_parser.hxx"
#include "dhcp6_snooping_priv.h"
#include "../packet/packet_api.h"

namespace dhcp6_snooping
{

// Local string buffer for print formatting
static char _strbuffer[80];

/*
 * Format an IPv6 address as a string for printing
 */
static char *ipv62str(mesa_ipv6_t *ipv6)
{
    return misc_ipv6_txt(ipv6, _strbuffer);
}

/*
 * List of known upper-level protocols
 */
vtss::Vector<u8> packet_parser::known_upper_protocols =
{
    VTSS_IPV6_HEADER_NXTHDR_TCP,
    VTSS_IPV6_HEADER_NXTHDR_EGP,
    VTSS_IPV6_HEADER_NXTHDR_IGP,
    VTSS_IPV6_HEADER_NXTHDR_RDP,
    VTSS_IPV6_HEADER_NXTHDR_ISOTP4,
    VTSS_IPV6_HEADER_NXTHDR_IPv6,
    VTSS_IPV6_HEADER_NXTHDR_RSVP,
    VTSS_IPV6_HEADER_NXTHDR_GRE,
    VTSS_IPV6_HEADER_NXTHDR_ICMP,
};

/*
 * Advance to the next header (IPv6 extension header or upper-level header)
 */
next_header_t packet_parser::advance_next_header(u8 *curr_frame_p, u8 *next_header, u32 *header_offset)
{
    next_header_t result = NEXT_HEADER_UNKNOWN;
    Ipv6ExtHeaderStart *ext_header;

    switch (*next_header) {
    case IPV6_EXT_HEADER_HOP_BY_HOP:
    case IPV6_EXT_HEADER_ROUTING:
    case IPV6_EXT_HEADER_DEST_OPT:
        ext_header = (Ipv6ExtHeaderStart *)curr_frame_p;
        *next_header = ext_header->next_header;
        *header_offset = 8 * (ext_header->length + 1); // RFC1883, section 4.3
        result = NEXT_HEADER_VALID;
        break;
    case IPV6_EXT_HEADER_AUTH:
        ext_header = (Ipv6ExtHeaderStart *)curr_frame_p;
        *next_header = ext_header->next_header;
        *header_offset = 4 * (ext_header->length + 2); // RFC2402, section 2.2
        result = NEXT_HEADER_VALID;
        break;
    case IPV6_EXT_HEADER_FRAGMENT: {
            ext_header = (Ipv6ExtHeaderStart *)curr_frame_p;
            *next_header = ext_header->next_header;
            *header_offset = 8; // fixed size ext. header
            
            u16 foffset = ((ext_header->pl_start << 8) | (*(&ext_header->pl_start + 1) & 0xFFF8)) >> 3;
            if (foffset == 0) {
                result = NEXT_HEADER_VALID;
            } else {
                // this is not the first fragment - just forward it
                result = NEXT_HEADER_FINAL_NOUDP;
            }
        }
        break;
    case IPV6_EXT_HEADER_NO_NEXT:
        // no next header - no need to search anymore
        result = NEXT_HEADER_FINAL_NOUDP;
        break;
    case IPV6_EXT_HEADER_ESP:
    case IPV6_EXT_HEADER_MOBILITY:
        // ESP [RFC4303] and Mobility headers terminates the header chain
        result = NEXT_HEADER_FINAL_NOUDP;
        break;
    default:
        // cannot parse
        result = NEXT_HEADER_UNKNOWN;
        break;
    }

    return result;
}

next_header_t packet_parser::locate_udp_in_ipv6_header(Ip6Header *frm_ip, size_t &remain_len, UdpHeader * &udp_header)
{
    next_header_t result = NEXT_HEADER_UNKNOWN;
    next_header_t nh_result;
    u8 next_header;
    u32 max_headers = m_max_header_depth;
    u32 header_offset = 0;
    u8 *curr_frame_p = (u8 *)frm_ip;

    udp_header = nullptr;

    next_header = IPV6_IP_HEADER_NXTHDR(frm_ip);
    header_offset = sizeof(Ip6Header);

    while (max_headers-- > 0) {
        // check if the remainder of the packet is large enough to advance to the next header
        if (remain_len < header_offset) {
            break;
        }

        T_NG(TRACE_GRP_RXPKT, "Next header check %u, ext.header level %u", next_header, max_headers);

        remain_len -= header_offset;
        curr_frame_p += header_offset;

        if (next_header == VTSS_IPV6_HEADER_NXTHDR_UDP) {
            // check if the remainder of the packet is large enough to contain a UDP header
            if (remain_len < sizeof(UdpHeader)) {
                break;
            }

            T_NG(TRACE_GRP_RXPKT, "Detected UDP header");

            udp_header = (UdpHeader *)curr_frame_p;
            remain_len -= sizeof(UdpHeader);
            result = NEXT_HEADER_FINAL;
            break;

        } else if (vtss::find(known_upper_protocols.begin(), known_upper_protocols.end(), next_header) != known_upper_protocols.end()) {
            // known non-UDP upper-level header type
            T_NG(TRACE_GRP_RXPKT, "Detected well-known upper-layer header %u", next_header);
            result = NEXT_HEADER_FINAL_NOUDP;
            break;
        }

        nh_result = advance_next_header(curr_frame_p, &next_header, &header_offset);
        if (nh_result == NEXT_HEADER_VALID) {
            T_NG(TRACE_GRP_RXPKT, "Next header %u is valid", next_header);
            continue;
        }

        T_NG(TRACE_GRP_RXPKT, "Next header final or unknown, rc = %u", nh_result);
        result = nh_result;
        break;
    }

    return result;
}

void packet_parser::extract_dhcp_options()
{
    OptionCode option_code;
    u16 option_length;
    uint8_t *option_data;
    Dhcp6Option *option = nullptr;
    uint32_t remain_length = get_dhcp_options_length();
    client_interface_info_t address_info;

    m_dhcp_options.clear();
    m_if_map.clear();
    m_rapid_commit = false;

    while ((option = get_next_dhcp_option(option, remain_length)) != nullptr) {
        option_code = (OptionCode)DHCP6_MSG_OPT_CODE(option);
        option_length = (u16)DHCP6_MSG_OPT_LENGTH(option);

        switch (option_code) {
        case dhcp6::OPT_CLIENTID:
            option_data = DHCP6_MSG_OPT_DATA_PTR_CAST(option, uint8_t);
            m_client_duid.assign(option_data, option_length);
            T_NG(TRACE_GRP_RXPKT, "CLIENTID option: <%s> (%u bytes)", m_client_duid.to_string().c_str(), m_client_duid.length);
            break;

        case dhcp6::OPT_SERVERID:
            option_data = DHCP6_MSG_OPT_DATA_PTR_CAST(option, uint8_t);
            m_server_duid.assign(option_data, option_length);
            T_NG(TRACE_GRP_RXPKT, "SERVERID option: <%s> (%u bytes)", m_server_duid.to_string().c_str(), m_server_duid.length);
            break;

        case dhcp6::OPT_STATUS_CODE:
            // check the status code for any errors
            m_status_code = (StatusCode) * DHCP6_MSG_OPT_DATA_PTR_CAST(option, uint8_t);
            T_NG(TRACE_GRP_RXPKT, "STATUS_CODE option, code %u", m_status_code);
            if (m_status_code != STATUS_SUCCESS) {
                T_DG(TRACE_GRP_RXPKT, "Server sent error status code %u", m_status_code);
            }
            break;

        case dhcp6::OPT_IA_NA:
            // get non-temporary address option
            T_NG(TRACE_GRP_RXPKT, "IA_NA option");
            vtss_clear(address_info);

            address_info.transaction_id = get_transaction_id();

            if (get_ia_na_data(option, address_info)) {
                m_if_map[address_info.iaid] = address_info;

                if (address_info.valid) {
                    T_DG(TRACE_GRP_RXPKT, "Got IA_NA address %s, valid lifetime %u secs",
                         ipv62str(&address_info.ip_address), address_info.lease_time);
                }
            }
            break;

        case dhcp6::OPT_RAPID_COMMIT:
            m_rapid_commit = true;
            break;

        default:
            T_RG(TRACE_GRP_RXPKT, "Unhandled option %u", option_code);
            break;
        }

        m_dhcp_options.push_back(option);
    }
}

parse_result_t packet_parser::parse_packet(const u8* packet, size_t length)
{
    EthHeader   *eth_h;
    Ip6Header   *ipv6_h;
    UdpHeader   *udp_h;
    u8          ip_version;
    u16         eth_type;
    size_t      remain_len;
    u16         dst_port;
    next_header_t result;

    T_NG(TRACE_GRP_RXPKT, "Parse packet: %d bytes", length);

    m_dhcp_header = nullptr;
    
    // check packet minimum size
    if (length < (sizeof(EthHeader) + sizeof(Ip6Header) + sizeof(UdpHeader) + sizeof(dhcp6::Dhcp6Message))) {
        T_DG(TRACE_GRP_RXPKT, "Packet too small: %d bytes", length);
        return PARSE_RESULT_COMPLETE_NOT_DHCP6;
    }

    eth_h = (EthHeader *)packet;
    eth_type = ETH_HEADER_TYPE(eth_h);
    if (eth_type != ETYPE_IPV6) {
        T_DG(TRACE_GRP_RXPKT, "%u Ethernet: Not IPv6", eth_type);
        return PARSE_RESULT_COMPLETE_NOT_DHCP6;
    }

    T_NG(TRACE_GRP_RXPKT, "Got IPv6 packet");

    ipv6_h = (Ip6Header *)(packet + sizeof(EthHeader));
    ip_version = IPV6_IP_HEADER_VERSION(ipv6_h);
    if (ip_version != VTSS_IPV6_HEADER_VERSION) {
        T_DG(TRACE_GRP_RXPKT, "Wrong IP version:%u", ip_version);
        return PARSE_RESULT_COMPLETE_NOT_DHCP6;
    }

    m_ipv6_src_address = ipv6_h->src;

    remain_len = length - sizeof(EthHeader);

    // Try to locate a UDP header in the frame
    result = locate_udp_in_ipv6_header(ipv6_h, remain_len, udp_h);
    switch (result) {
    case NEXT_HEADER_UNKNOWN:
        T_NG(TRACE_GRP_RXPKT, "NEXT_HEADER_UNKNOWN");
        return PARSE_RESULT_INCOMPLETE_EH;
    case NEXT_HEADER_FINAL_NOUDP:
        T_NG(TRACE_GRP_RXPKT, "NEXT_HEADER_FINAL_NOUDP");
        return PARSE_RESULT_COMPLETE_NOT_DHCP6;
    case NEXT_HEADER_VALID:
        T_DG(TRACE_GRP_RXPKT, "Error: unexpected parse result");
        return PARSE_RESULT_INCOMPLETE_EH;
    default:
        // proceed to check for DHCP
        break;
    }

    T_NG(TRACE_GRP_RXPKT, "Got UDP header, remain size: %u bytes", remain_len);

    // we now have a UDP header
    dst_port = IPV6_UDP_HEADER_DST_PORT(udp_h);
    if (dst_port == VTSS_DHCP6_CLIENT_UDP_PORT) {
        m_packet_type = PACKET_TYPE_DHCP6_SERVER_MESSAGE;
    } else if (dst_port == VTSS_DHCP6_SERVER_UDP_PORT) {
        m_packet_type = PACKET_TYPE_DHCP6_CLIENT_MESSAGE;
    } else {
        // not a DHCP packet
        return PARSE_RESULT_COMPLETE_NOT_DHCP6;
    }

    if (remain_len < sizeof(dhcp6::Dhcp6Message::msg_xid)) {
        T_DG(TRACE_GRP_RXPKT, "DHCPv6 header too small, %u bytes", remain_len);
        return PARSE_RESULT_COMPLETE_NOT_DHCP6;
    }

    m_dhcp_header = (dhcp6::Dhcp6Message*) (((u8 *)udp_h) + sizeof(UdpHeader));
    m_dhcp_header_length = remain_len;
    m_dhcp_options_length = m_dhcp_header_length - sizeof(Dhcp6Message::msg_xid);
    m_message_type = (dhcp6::MessageType)DHCP6_MSG_MSG_TYPE(m_dhcp_header);
    m_transaction_id = DHCP6_MSG_TRANSACTION_ID(m_dhcp_header);

    extract_dhcp_options();

    T_NG(TRACE_GRP_RXPKT, "DHCPv6 message: %u, TRNS ID: %X", m_message_type, m_transaction_id);

    return PARSE_RESULT_COMPLETE;
}

dhcp6::Dhcp6Option *packet_parser::get_next_dhcp_option(dhcp6::Dhcp6Option *curr, uint32_t &remain_len)
{
    static const uint32_t min_option_len = sizeof(dhcp6::Dhcp6Option::code) + sizeof(dhcp6::Dhcp6Option::length);
    uint32_t curr_option_len;

    if (m_dhcp_header == nullptr) {
        // we need an actual DHCP header to do this
        return nullptr;
    }
    if (remain_len < min_option_len) {
        // not even space for a data-less DHCP option
        return nullptr;
    }

    if (curr == nullptr) {
        // get the first option header after the DHCP main header
        curr = DHCP6_MSG_OPTIONS_PTR(m_dhcp_header);
    } else {
        // advance to the next option
        u8 *p = (u8 *)curr;
        p += DHCP6_MSG_OPT_SHIFT_LENGTH(curr);
        curr = (dhcp6::Dhcp6Option *)p;
    }

    curr_option_len = DHCP6_MSG_OPT_SHIFT_LENGTH(curr);
    if (remain_len < curr_option_len) {
        // not space enough for the DHCP option + data
        return nullptr;
    }

    remain_len -= curr_option_len;
    return curr;
}

bool packet_parser::get_ia_na_data(dhcp6::Dhcp6Option *option, client_interface_info_t &address_info)
{
    u16 option_length = DHCP6_MSG_OPT_LENGTH(option);
    u16 sub_option_length = option_length - (sizeof(OptIaNa::iaid) + 2 * sizeof(OptIaNa::t1));
    u32 t1, t2;
    u32 preferred_lifetime = 0;
    OptIaNa *iana_option_data;
    Dhcp6Option *ia_na_suboption;
    Dhcp6Option *ia_addr_suboption;
    OptIaAddr *ia_address_option_data;
    u32 chk_len = 0;
    StatusCode status_code;

    T_NG(TRACE_GRP_RXPKT, "Enter");

    vtss_clear(address_info);

    iana_option_data = (OptIaNa *)option;
    t1 = ntohl(iana_option_data->t1);
    t2 = ntohl(iana_option_data->t2);

    address_info.iaid = ntohl(iana_option_data->iaid);

    // Check t<x> values according to RFC3315, section 22.4.
    if (t1 > 0 && t2 > 0) {
        if (t1 > t2) {
            T_DG(TRACE_GRP_RXPKT, "Timer value mismatch, t1:%u, t2:%u - discarding", t1, t2);
            return false;
        }
    }

    ia_na_suboption = (Dhcp6Option *)&iana_option_data->options;

    // parse sub-options
    while (chk_len + DHCP6_MSG_OPT_SHIFT_LENGTH(ia_na_suboption) <= sub_option_length) {
        u16 sub_option_code = DHCP6_MSG_OPT_CODE(ia_na_suboption);

        switch (sub_option_code) {
        case OPT_IAADDR:
            T_NG(TRACE_GRP_RXPKT, "IA_NA contains OPT_IAADDR (%u)", sub_option_code);

            ia_address_option_data = (OptIaAddr *)ia_na_suboption;
            address_info.ip_address = ia_address_option_data->address;

            preferred_lifetime = ntohl(ia_address_option_data->preferred_lifetime);
            // we collect the valid lifetime as this is the actual lease time
            address_info.lease_time = ntohl(ia_address_option_data->valid_lifetime);

            if (address_info.lease_time == 0) {
                T_DG(TRACE_GRP_RXPKT, "Valid lifetime is zero - discarding");
                return false;
            }

            if (preferred_lifetime > address_info.lease_time) {
                T_DG(TRACE_GRP_RXPKT, "Lifetime value mismatch, p:%u, v:%u - discarding", preferred_lifetime, address_info.lease_time);
                return false;
            }

            ia_addr_suboption = (Dhcp6Option *)&ia_na_suboption->data;
            if (DHCP6_MSG_OPT_CODE(ia_addr_suboption) == OPT_STATUS_CODE) {
                status_code = (StatusCode) * DHCP6_MSG_OPT_DATA_PTR_CAST(ia_addr_suboption, uint8_t);
                T_NG(TRACE_GRP_RXPKT, "IA_addr status code option, %u", status_code);
                if (status_code != STATUS_SUCCESS) {
                    T_DG(TRACE_GRP_RXPKT, "Server sent error IA_addr status code %u - discarding", status_code);
                    // ignore this IA_addr option address
                    return false;
                }
            }

            address_info.valid = TRUE;
            break;

        default:
            break;
        }

        chk_len += DHCP6_MSG_OPT_SHIFT_LENGTH(ia_na_suboption);
        if (chk_len < sub_option_length) {
            ia_na_suboption = DHCP6_MSG_OPT_NEXT(ia_na_suboption);

        } else {
            if (chk_len > sub_option_length) {
                T_DG(TRACE_GRP_RXPKT, "Option length mismatch, %u, %u", chk_len, sub_option_length);
                return false;
            }
        }
    }

    return true;
}

}  // namespace dhcp6_snooping
