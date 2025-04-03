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

#ifndef PACKET_PARSER_HXX
#define PACKET_PARSER_HXX

#include <vtss/basics/vector.hxx>
#include <vtss/basics/algorithm.hxx>
#include "vtss/appl/dhcp6_snooping.h"
#include "dhcp6_snooping_frame.h"
#include "dhcp6_snooping_expose.h"

using namespace vtss;
using namespace dhcp6;

namespace dhcp6_snooping
{

/*
 * Next header type result from parser
 */
typedef enum {
    NEXT_HEADER_VALID,              // the next header is valid but not final
    NEXT_HEADER_FINAL,              // the next header is valid and final and UDP
    NEXT_HEADER_FINAL_NOUDP,        // The next header is valid and final but not UDP
    NEXT_HEADER_UNKNOWN,            // Unknown extension header - cannot parse
} next_header_t;

/*
 * Result code for a packet parse attempt
 */
typedef enum {
    PARSE_RESULT_COMPLETE,              // packet was parsed successfully as a DHCPv6 message, results can now be read
    PARSE_RESULT_COMPLETE_NOT_DHCP6,    // packet was parsed but is not DHCPv6
    PARSE_RESULT_INCOMPLETE_EH          // packet could not be parsed due to unknown ext. header
} parse_result_t;

/*
 * DHCPv6 message type group
 */
typedef enum {
    PACKET_TYPE_DHCP6_CLIENT_MESSAGE,   // Message sent from client
    PACKET_TYPE_DHCP6_SERVER_MESSAGE,   // Message sent from server
} dhcp6_packet_type_t;


/*
 * This class parses received packets to determine if they are DHCPv6 client or
 * server messages.
 */
class packet_parser {
public:
    /**
     *
     * @param mhd   The max. number of IPv6 extension headers to parse.
     */
    packet_parser(u32 mhd) 
        : m_max_header_depth(mhd) {}
    
    virtual ~packet_parser() {}

    /*
     * Parse a received packet
     */
    parse_result_t parse_packet(const u8* packet, size_t length);

    /*
     * Result accessors. These are only valid if the parse_packet returned PARSE_RESULT_COMPLETE.
     */

    mesa_ipv6_t get_ipv6_src_address() { return m_ipv6_src_address; }

    dhcp6_packet_type_t get_dhcp_packet_type() { return m_packet_type; }

    dhcp6::MessageType get_dhcp_message_type() { return m_message_type; }

    uint32_t get_transaction_id() { return m_transaction_id; }

    const dhcp6::Dhcp6Message *get_dhcp_header() { return m_dhcp_header; }

    const vtss::Vector<dhcp6::Dhcp6Option *> &get_dhcp_options() const { return m_dhcp_options; }

    uint32_t get_dhcp_header_length() { return m_dhcp_header_length; }

    uint32_t get_dhcp_options_length() { return m_dhcp_options_length; }

    StatusCode get_dhcp_status_code() { return m_status_code; }

    const interface_address_map_t & get_if_map() const { return m_if_map; }

    dhcp_duid_t &get_client_duid() { return m_client_duid; }
    dhcp_duid_t &get_server_duid() { return m_server_duid; }
    bool get_rapid_commit() { return m_rapid_commit; }

private:
    next_header_t advance_next_header(u8 *curr_frame_p, u8 *next_header, u32 *header_offset);
    next_header_t locate_udp_in_ipv6_header(Ip6Header *frm_ip, size_t &remain_len, UdpHeader * &udp_header);
    void extract_dhcp_options();
    bool get_ia_na_data(dhcp6::Dhcp6Option *option, client_interface_info_t &address_info);
    Dhcp6Option *get_next_dhcp_option(Dhcp6Option *curr, uint32_t &remain_len);

    // List of known upper-level protocols
    static vtss::Vector<u8> known_upper_protocols;

    // List of DHCP options in packet
    vtss::Vector<dhcp6::Dhcp6Option *> m_dhcp_options;

    uint32_t                m_max_header_depth;
    dhcp6_packet_type_t     m_packet_type;
    mesa_ipv6_t             m_ipv6_src_address;
    dhcp6::MessageType      m_message_type;
    uint32_t                m_transaction_id;
    dhcp6::Dhcp6Message     *m_dhcp_header = nullptr;
    uint32_t                m_dhcp_header_length = 0;
    uint32_t                m_dhcp_options_length = 0;
    dhcp_duid_t             m_client_duid;
    dhcp_duid_t             m_server_duid;
    StatusCode              m_status_code = dhcp6::STATUS_SUCCESS;
    interface_address_map_t m_if_map;
    bool                    m_rapid_commit = false;;
};

} // namespace dhcp6_snooping

#endif /* PACKET_PARSER_HXX */

