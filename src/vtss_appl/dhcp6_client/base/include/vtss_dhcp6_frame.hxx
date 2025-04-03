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

#ifndef _VTSS_DHCP6_FRAME_HXX_
#define _VTSS_DHCP6_FRAME_HXX_

namespace vtss
{
#define IPV6_IP_HEADER_VERSION(x)           (ntohl((x)->ver_tc_fl) >> 28)
#define IPV6_IP_HEADER_TRAFFIC_CLASS(x)     ((ntohl((x)->ver_tc_fl) << 4) >> 24)
#define IPV6_IP_HEADER_FLOW_LABEL(x)        (ntohl((x)->ver_tc_fl) & 0xFFFFF)
#define IPV6_IP_HEADER_PAYLOAD_LEN(x)       (ntohs((x)->payload_len))
#define IPV6_IP_HEADER_NXTHDR(x)            ((x)->next_header)
#define IPV6_IP_HEADER_HOP_LIMIT(x)         ((x)->hop_limit)
#define IPV6_IP_HEADER_SRC_ADDR_PTR(x)      (&((x)->src))
#define IPV6_IP_HEADER_DST_ADDR_PTR(x)      (&((x)->dst))

struct Ip6Header {
    u32                     ver_tc_fl;      /* Version, Traffic Class, Flow Label */
    u16                     payload_len;    /* Payload Length */
    u8                      next_header;    /* Next Header */
    u8                      hop_limit;      /* Hop Limit */
    mesa_ipv6_t             src;            /* Source Address */
    mesa_ipv6_t             dst;            /* Destination Address */
} VTSS_IPV6_PACK_STRUCT;

#define IPV6_UDP_HEADER_SRC_PORT(x)         (ntohs((x)->src_port))
#define IPV6_UDP_HEADER_DST_PORT(x)         (ntohs((x)->dst_port))
#define IPV6_UDP_HEADER_CHECKSUM(x)         (ntohs((x)->check_sum))
#define IPV6_UDP_HEADER_LENGTH(x)           (ntohs((x)->len))

struct UdpHeader {
    u16                     src_port;       /* UDP Source Port */
    u16                     dst_port;       /* UDP Destination Port */
    u16                     len;            /* UDP Length */
    u16                     check_sum;      /* UDP Checksum */
} VTSS_IPV6_PACK_STRUCT;

namespace dhcp6
{
/*
    RFC-3315

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  msg-type   |                  transaction-id                 |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    .                          options                              .
    .                         (variable)                            .
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    msg-type:       Identifies the DHCP message type
    transaction-id: The transaction ID for this message exchange.
    options:        Options carried in this message.
*/

#define DHCP6_MSG_MSG_TYPE(x)               (ntohl((x)->msg_xid) >> 24)
#define DHCP6_MSG_TRANSACTION_ID(x)         ((ntohl((x)->msg_xid) << 8) >> 8)
#define DHCP6_MSG_OPTIONS_PTR(x)            (Dhcp6Option *)(&((x)->options))

struct Dhcp6Message {
    u32                     msg_xid;        /* DHCPv6 Message Type & Transaction ID */
    u8                      *options;       /* DHCPv6 Options */
} VTSS_DHCP6_PACK_STRUCT;

mesa_rc rx_solicit(
    ServiceRole             r,
    const mesa_ipv6_t       *const sip,
    const Dhcp6Message      *const msg,
    vtss_tick_count_t       ts,
    u32                     len,
    mesa_vid_t              ifx
);

mesa_rc rx_advertise(
    ServiceRole             r,
    const mesa_ipv6_t       *const sip,
    const Dhcp6Message      *const msg,
    vtss_tick_count_t       ts,
    u32                     len,
    mesa_vid_t              ifx
);

mesa_rc rx_request(
    ServiceRole             r,
    const mesa_ipv6_t       *const sip,
    const Dhcp6Message      *const msg,
    vtss_tick_count_t       ts,
    u32                     len,
    mesa_vid_t              ifx
);

mesa_rc rx_confirm(
    ServiceRole             r,
    const mesa_ipv6_t       *const sip,
    const Dhcp6Message      *const msg,
    vtss_tick_count_t       ts,
    u32                     len,
    mesa_vid_t              ifx
);

mesa_rc rx_renew(
    ServiceRole             r,
    const mesa_ipv6_t       *const sip,
    const Dhcp6Message      *const msg,
    vtss_tick_count_t       ts,
    u32                     len,
    mesa_vid_t              ifx
);

mesa_rc rx_rebind(
    ServiceRole             r,
    const mesa_ipv6_t       *const sip,
    const Dhcp6Message      *const msg,
    vtss_tick_count_t       ts,
    u32                     len,
    mesa_vid_t              ifx
);

mesa_rc rx_reply(
    ServiceRole             r,
    const mesa_ipv6_t       *const sip,
    const Dhcp6Message      *const msg,
    vtss_tick_count_t       ts,
    u32                     len,
    mesa_vid_t              ifx
);

mesa_rc rx_release(
    ServiceRole             r,
    const mesa_ipv6_t       *const sip,
    const Dhcp6Message      *const msg,
    vtss_tick_count_t       ts,
    u32                     len,
    mesa_vid_t              ifx
);

mesa_rc rx_decline(
    ServiceRole             r,
    const mesa_ipv6_t       *const sip,
    const Dhcp6Message      *const msg,
    vtss_tick_count_t       ts,
    u32                     len,
    mesa_vid_t              ifx
);

mesa_rc rx_reconfigure(
    ServiceRole             r,
    const mesa_ipv6_t       *const sip,
    const Dhcp6Message      *const msg,
    vtss_tick_count_t       ts,
    u32                     len,
    mesa_vid_t              ifx
);

mesa_rc rx_information_request(
    ServiceRole             r,
    const mesa_ipv6_t       *const sip,
    const Dhcp6Message      *const msg,
    vtss_tick_count_t       ts,
    u32                     len,
    mesa_vid_t              ifx
);

template<unsigned SIZE>
struct Buf_t {
    Buf_t() {}

    template<typename I>
    Buf_t(I d)
    {
        for (unsigned i = 0; i < SIZE; ++i) {
            data_[i] = *d++;
        }
    }

    unsigned size() const
    {
        return SIZE;
    }

    void copy_to(u8 *buf) const
    {
        for (unsigned i = 0; i < SIZE; ++i) {
            *buf++ = data_[i];
        }
    }

    template<typename I>
    void copy_from(I buf)
    {
        for (unsigned i = 0; i < SIZE; ++i) {
            data_[i] = *buf++;
        }
    }

    u8 data_[SIZE];
};

/*
    RFC-3315

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | option-code |                    option-len                   |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                        option-data                            |
    |                    (option-len octets)                        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    option-code:    An unsigned integer identifying the specific option
                    type carried in this option.
    option-len:     An unsigned integer giving the length of the option-data
                    field in this option in octets.
    option-data:    The data for the option; the format of this data depends
                    on the definition of the option.
*/

#define DHCP6_MSG_OPT_CODE(x)               (ntohs((x)->code))
#define DHCP6_MSG_OPT_LENGTH(x)             (ntohs((x)->length))
#define DHCP6_MSG_OPT_DATA_PTR_CAST(x, y)   (DHCP6_MSG_OPT_LENGTH(x) ? (y *)(&((x)->data)) : NULL)
#define DHCP6_MSG_OPT_SHIFT_LENGTH(x)       (sizeof((x)->code) + sizeof((x)->length) + DHCP6_MSG_OPT_LENGTH((x)))
#define DHCP6_MSG_OPT_NEXT(x)               (Dhcp6Option *)((u8 *)(x) + DHCP6_MSG_OPT_SHIFT_LENGTH((x)))

struct Dhcp6Option {
    u16                     code;           /* DHCPv6 Option Code */
    u16                     length;         /* DHCPv6 Option Length */
    u8                      *data;          /* DHCPv6 Option Data */
} VTSS_DHCP6_PACK_STRUCT;

struct OptCommon {
    u16                         code;
    u16                         length;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: DUID-LLT  (9.2)
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |              1                |    hardware type (16 bits)    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                       time (32 bits)                          |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    .                                                               .
    .            link-layer address (variable length)               .
    .                                                               .
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    RFC-3315: DUID-EN   (9.3)
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |              2                |       enterprise-number       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |   enterprise-number (contd)   |                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
    .                           identifier                          .
    .                       (variable length)                       .
    .                                                               .
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    RFC-3315: DUID-LL   (9.4)
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |              3              |     hardware type (16 bits)     |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    .                                                               .
    .            link-layer address (variable length)               .
    .                                                               .
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
struct DeviceDuid {
    u16                         duid_type;

    union {
        struct {
            u16                 hardware_type;
            u32                 time;
            mesa_mac_t          lla;
        } VTSS_DHCP6_PACK_STRUCT llt;

        struct {
            u16                 hardware_type;
            mesa_mac_t          lla;
        } VTSS_DHCP6_PACK_STRUCT ll;

        struct {
            u32                 enterprise_number;
            u32                 id;
        } VTSS_DHCP6_PACK_STRUCT en;
    } type;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Client Identifier Option (22.2)

    The Client Identifier option is used to carry a DUID (see section 9)
    identifying a client between a client and a server.
*/
struct OptClientId {
    OptCommon                   common;
    DeviceDuid                  duid;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Server Identifier Option (22.3)

    The Server Identifier option is used to carry a DUID (see section 9)
    identifying a server between a client and a server.
*/
struct OptServerId {
    OptCommon                   common;
    DeviceDuid                  duid;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Identity Association for Non-temporary Addresses Option (22.4)

    The Identity Association for Non-temporary Addresses option (IA_NA
    option) is used to carry an IA_NA, the parameters associated with the
    IA_NA, and the non-temporary addresses associated with the IA_NA.
*/
struct OptIaNa {
    OptCommon                   common;
    u32                         iaid;
    u32                         t1;
    u32                         t2;
    u8                          *options;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Identity Association for Temporary Addresses Option (22.5)

    The Identity Association for the Temporary Addresses (IA_TA) option
    is used to carry an IA_TA, the parameters associated with the IA_TA
    and the addresses associated with the IA_TA.
*/
struct OptIaTa {
    OptCommon                   common;
    u32                         iaid;
    u8                          *options;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: IA Address Option (22.6)

    The IA Address option is used to specify IPv6 addresses associated
    with an IA_NA or an IA_TA.
*/
struct OptIaAddr {
    OptCommon                   common;
    mesa_ipv6_t                 address;
    u32                         preferred_lifetime;
    u32                         valid_lifetime;
    u8                          *options;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Option Request Option (22.7)

    A client MAY include an Option Request option in a Solicit, Request,
    Renew, Rebind, Confirm or Information-request message to inform the
    server about options the client wants the server to send to the client.
*/
struct OptOro {
    OptCommon                   common;
    u32                         *options;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Preference Option (22.8)

    The Preference option is sent by a server to a client to affect the
    selection of a server by the client.
*/
struct OptPreference {
    OptCommon                   common;
    u8                          pref_value;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Elapsed Time Option (22.9)

    A client MUST include an Elapsed Time option in messages to indicate
    how long the client has been trying to complete a DHCP message exchange.
*/
struct OptElapsedTime {
    OptCommon                   common;
    u16                         elapsed_time;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Authentication Option (22.11)*NotSupport

    The Authentication option carries authentication information to
    authenticate the identity and contents of DHCP messages.
*/
struct OptAuth {
    OptCommon                   common;
    u8                          protocol;
    u8                          algorithm;
    u8                          replay_detection_method;
    u64                         replay_detection;
    u8                          *authentication_information;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Server Unicast Option (22.12)

    The server sends this option to a client to indicate to the client
    that it is allowed to unicast messages to the server.
*/
struct OptUnicast {
    OptCommon                   common;
    mesa_ipv6_t                 server_address;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Status Code Option (22.13)

    This option returns a status indication related to the DHCP message
    or option in which it appears.
*/
struct OptStatusCode {
    OptCommon                   common;
    u16                         status_code;
    char                        *status_message;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Rapid Commit Option (22.14)

    The Rapid Commit option is used to signal the use of the two message
    exchange for address assignment.
*/
struct OptRapidCommit {
    OptCommon                   common;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: User Class Option (22.15)*NotSupport

    The User Class option is used by a client to identify the type or
    category of user or applications it represents.
*/
struct OptUserClass {
    OptCommon                   common;
    u16                         user_class_len;
    u8                          *opaque_data;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Vendor Class Option (22.16)*NotSupport

    This option is used by a client to identify the vendor that
    manufactured the hardware on which the client is running.
*/
struct OptVendorClass {
    OptCommon                   common;
    u32                         enterprise_number;
    u16                         vendor_class_len;
    u8                          *opaque_data;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Vendor-specific Information Option (22.17)*NotSupport

    This option is used by clients and servers to exchange
    vendor-specific information.
*/
struct OptVendorOpt {
    OptCommon                   common;
    u32                         enterprise_number;
    u8                          *option_data;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Reconfigure Message Option (22.19)

    A server includes a Reconfigure Message option in a Reconfigure
    message to indicate to the client whether the client responds with a
    Renew message or an Information-request message.
*/
struct OptReconfMsg {
    OptCommon                   common;
    u8                          msg_type;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3315: Reconfigure Accept Option (22.20)

    A client uses the Reconfigure Accept option to announce to the server
    whether the client is willing to accept Reconfigure messages, and a
    server uses this option to tell the client whether or not to accept
    Reconfigure messages.
*/
struct OptReconfAccept {
    OptCommon                   common;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3633: IA_PD Prefix option (10)

    The IA_PD Prefix option is used to specify IPv6 address prefixes
    associated with an IA_PD. The IA_PD Prefix option must be
    encapsulated in the IA_PD-options field of an IA_PD option.
*/
struct OptIaPrefix {
    OptCommon                   common;
    u32                         preferred_lifetime;
    u32                         valid_lifetime;
    u8                          prefix_length;
    mesa_ipv6_t                 ipv6_prefix;
    u8                          *ia_prefix_options;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3633: IA_PD option (9)

    The IA_PD option is used to carry a prefix delegation identity
    association, the parameters associated with the IA_PD and the
    prefixes associated with it.
*/
struct OptIaPd {
    OptCommon                   common;
    u32                         iaid;
    u32                         t1;
    u32                         t2;
    OptIaPrefix                 *ia_pd_options;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3646: DNS Recursive Name Server option (3)

    The DNS Recursive Name Server option provides a list of one or more
    IPv6 addresses of DNS recursive name servers to which a client's DNS
    resolver MAY send DNS queries [STD 13, RFC 1035].
*/
struct OptDnsServers {
    OptCommon                   common;
    mesa_ipv6_t                 *servers;
} VTSS_DHCP6_PACK_STRUCT;

/*
    RFC-3646: Domain Search List option (4)

    The Domain Search List option specifies the domain search list the
    client is to use when resolving hostnames with DNS.
*/
struct OptDomainList {
    OptCommon                   common;
    char                        *srchlst;
} VTSS_DHCP6_PACK_STRUCT;

} /* dhcp6 */
} /* vtss */

#endif /* _VTSS_DHCP6_FRAME_HXX_ */
