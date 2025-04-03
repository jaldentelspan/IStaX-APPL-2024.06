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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/genetlink.h>
#include <linux/if_link.h>
#include <linux/if_addr.h>
#include <linux/if_arp.h>
#include <linux/ip.h>

#include "main.h"
#include "sysutil_trace.h"
#include "vtss_netlink.hxx"
#include "vtss/basics/trace.hxx"

#define D VTSS_TRACE(VTSS_TRACE_MODULE_ID, VTSS_TRACE_SYSUTIL_GRP_NETLINK, DEBUG)
#define N VTSS_TRACE(VTSS_TRACE_MODULE_ID, VTSS_TRACE_SYSUTIL_GRP_NETLINK, NOISE)
#define I VTSS_TRACE(VTSS_TRACE_MODULE_ID, VTSS_TRACE_SYSUTIL_GRP_NETLINK, INFO)
#define W VTSS_TRACE(VTSS_TRACE_MODULE_ID, VTSS_TRACE_SYSUTIL_GRP_NETLINK, WARNING)
#define E VTSS_TRACE(VTSS_TRACE_MODULE_ID, VTSS_TRACE_SYSUTIL_GRP_NETLINK, ERROR)

namespace vtss {
namespace appl {
namespace netlink {

inline struct rtattr *nlmsg_tail(struct nlmsghdr *n) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
    return ((struct rtattr *)(((char *)(n)) + NLMSG_ALIGN((n)->nlmsg_len)));
#pragma GCC diagnostic pop
}

ostream &operator<<(ostream &o, AsRta_NLA_U32 x) {
    if (x.len < 4)
        o << "<ERROR unexpected length: " << x.len << ">";
    else
        o << *((uint32_t *)x.data);
    return o;
}

ostream &operator<<(ostream &o, AsRta_NLA_U8 x) {
    uint8_t d;
    if (x.len < 1) {
        o << "<ERROR unexpected length: " << x.len << ">";
        return o;
    }

    d = *((uint8_t *)x.data);
    o << (uint32_t)d;
    return o;
}

ostream &operator<<(ostream &o, AsRta_NLA_STRING x) {
    size_t s = strnlen((const char *)x.data, x.len);
    o << std::string((const char *)x.data, (const char *)x.data + s);
    return o;
}

ostream &operator<<(ostream &o, AsRta_NLA_ADDRESS x) {
    char buf[64];

    switch (x.len) {
    case 4:
        o << inet_ntop(AF_INET, x.data, buf, 64);
        break;

    case 6: {
        const unsigned char *m = (const unsigned char *)x.data;
        snprintf(buf, 64, "%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2],
                 m[3], m[4], m[5]);
        o << buf;
        break;
    }

    case 16:
        o << inet_ntop(AF_INET6, x.data, buf, 64);
        break;


    default:
        o << "<Unexpected length: " << x.len << ">";
    }

    return o;
}

ostream &operator<<(ostream &o, AsRtaAddr_NLA_ADDRESS x) {
    char buf[64];

    switch (x.family) {
    case AF_INET:
        if (x.len != 4) goto unexpected_length;
        o << inet_ntop(AF_INET, x.data, buf, 64);
        break;

    case AF_INET6:
        if (x.len != 16) goto unexpected_length;
        o << inet_ntop(AF_INET6, x.data, buf, 64);
        break;


    default:
        o << "<Unknown type: " << AsAddressFamilyType(x.family) << ">";
    }

    return o;

unexpected_length:
    o << "<Unexpected length: " << x.len << ", family: " << x.family << ">";
    return o;
}

ostream &operator<<(ostream &o, AsRta_IFLA_INET x) {
    int len = x.len;
    const struct rtattr *rta = (const struct rtattr *)x.data;

    while (RTA_OK(rta, len)) {
        switch (rta->rta_type) {
        case IFLA_INET_UNSPEC:
            o << "UNSPEC " << RTA_PAYLOAD(rta) << ">";
            break;

        case IFLA_INET_CONF: {
            o << x.indent << "CONF: [\n";
            auto ii = x.indent.open();
            const int *y = (const int *)RTA_DATA(rta);
            for (unsigned i = 0; i < (RTA_PAYLOAD(rta) / 4); ++i) {
                switch (i) {
#define CASE(X)                                \
    case (IPV4_DEVCONF_##X - 1):               \
        if (y[i] == 0) {                       \
            /*o << ii << "NO_"#X "\n";*/       \
        } else if (y[i] == 1) {                \
            o << ii << #X "\n";                \
        } else {                               \
            o << ii << #X ":" << y[i] << "\n"; \
        }                                      \
        break;
                    CASE(FORWARDING)
                    CASE(MC_FORWARDING)
                    CASE(PROXY_ARP)
                    CASE(ACCEPT_REDIRECTS)
                    CASE(SECURE_REDIRECTS)
                    CASE(SEND_REDIRECTS)
                    CASE(SHARED_MEDIA)
                    CASE(RP_FILTER)
                    CASE(ACCEPT_SOURCE_ROUTE)
                    CASE(BOOTP_RELAY)
                    CASE(LOG_MARTIANS)
                    CASE(TAG)
                    CASE(ARPFILTER)
                    CASE(MEDIUM_ID)
                    CASE(NOXFRM)
                    CASE(NOPOLICY)
                    CASE(FORCE_IGMP_VERSION)
                    CASE(ARP_ANNOUNCE)
                    CASE(ARP_IGNORE)
                    CASE(PROMOTE_SECONDARIES)
                    CASE(ARP_ACCEPT)
                    CASE(ARP_NOTIFY)
                    CASE(ACCEPT_LOCAL)
                    CASE(SRC_VMARK)
                    CASE(PROXY_ARP_PVLAN)
                    CASE(ROUTE_LOCALNET)
                    CASE(IGMPV2_UNSOLICITED_REPORT_INTERVAL)
                    CASE(IGMPV3_UNSOLICITED_REPORT_INTERVAL)
#undef CASE
                }
            }
            o << x.indent << "]\n";
            break;
        }

        default:
            o << x.indent << "<Unknown type: " << rta->rta_type
              << " size: " << RTA_PAYLOAD(rta) << ">\n";
            break;
        }
        rta = RTA_NEXT(rta, len);
    }

    return o;
}

ostream &operator<<(ostream &o, AsRta_IFLA_INET6 x) {
    int len = x.len;
    const struct rtattr *rta = (const struct rtattr *)x.data;

    while (RTA_OK(rta, len)) {
        switch (rta->rta_type) {
        case IFLA_INET6_FLAGS:
            o << x.indent << "FLAGS: " << AsIffFlags(*(int *)RTA_DATA(rta))
              << "(" << *(int *)RTA_DATA(rta) << ")\n";
            break;

        // IFLA_INET6_CONF,	/* sysctl parameters		*/
        // IFLA_INET6_STATS,	/* statistics			*/
        // IFLA_INET6_MCAST,	/* MC things. What of them?	*/
        // IFLA_INET6_CACHEINFO,	/* time values and max reasm size */
        // IFLA_INET6_ICMP6STATS,	/* statistics (icmpv6)		*/
        // IFLA_INET6_TOKEN,	/* device token			*/
        default:
            o << x.indent << "<Unknown type: " << rta->rta_type
              << " size: " << RTA_PAYLOAD(rta) << ">\n";
            break;
        }
        rta = RTA_NEXT(rta, len);
    }

    return o;
}

ostream &operator<<(ostream &o, AsRta_IFLA_AF_SPEC x) {
    int len = x.len;
    const struct rtattr *rta = (const struct rtattr *)x.data;

    o << "[\n";
    auto ii = x.indent.open();
    while (RTA_OK(rta, len)) {
        switch (rta->rta_type) {
        case AF_INET:
            o << ii << "AF_INET: [\n"
              << AsRta_IFLA_INET(RTA_DATA(rta), RTA_PAYLOAD(rta), AF_INET,
                                 ii.open()) << ii << "]\n";
            break;

        case AF_INET6:
            o << ii << "AF_INET6: [\n"
              << AsRta_IFLA_INET6(RTA_DATA(rta), RTA_PAYLOAD(rta), AF_INET6,
                                  ii.open()) << ii << "]\n";
            break;

        default:
            o << x.indent << "<No-handler for "
              << AsAddressFamilyType(rta->rta_type) << ">\n";
        }
        rta = RTA_NEXT(rta, len);
    }
    o << x.indent << "]";
    return o;
}

ostream &operator<<(ostream &o, AsRta_NLA_UNKNOWN x) {
    o << "<Unknown data of size: " << x.len << ">";
    return o;
}

ostream &operator<<(ostream &o, const AsRtaLink &t) {
#define CASE(X, Y)                                                 \
    case IFLA_##X:                                                 \
        o << #X ": " << Y(t.data, t.len, t.parent_type, t.indent); \
        break;

    switch (t.type) {
        CASE(ADDRESS, AsRta_NLA_ADDRESS)
        CASE(BROADCAST, AsRta_NLA_ADDRESS)
        CASE(IFNAME, AsRta_NLA_STRING)
        CASE(MTU, AsRta_NLA_U32)
        CASE(LINK, AsRta_NLA_U32)
        CASE(QDISC, AsRta_NLA_UNKNOWN)
        CASE(STATS, AsRta_NLA_UNKNOWN)
        CASE(MASTER, AsRta_NLA_U32)
        CASE(TXQLEN, AsRta_NLA_U32)
        CASE(WEIGHT, AsRta_NLA_U32)
        CASE(OPERSTATE, AsRta_NLA_U8)
        CASE(LINKMODE, AsRta_NLA_U8)
        CASE(LINKINFO, AsRta_NLA_UNKNOWN)  // AsRtaLink_NLA_NESTED
        CASE(NET_NS_PID, AsRta_NLA_U32)
        CASE(IFALIAS, AsRta_NLA_STRING)
        CASE(NUM_VF, AsRta_NLA_UNKNOWN)
        CASE(VFINFO_LIST, AsRta_NLA_UNKNOWN)  // AsRtaLink_NLA_NESTED
        CASE(STATS64, AsRta_NLA_UNKNOWN)
        CASE(AF_SPEC, AsRta_IFLA_AF_SPEC)
        CASE(GROUP, AsRta_NLA_U32)
        CASE(NET_NS_FD, AsRta_NLA_U32)
        CASE(EXT_MASK, AsRta_NLA_U32)
        CASE(PROMISCUITY, AsRta_NLA_U32)
        CASE(NUM_TX_QUEUES, AsRta_NLA_U32)
        CASE(NUM_RX_QUEUES, AsRta_NLA_U32)
        CASE(CARRIER, AsRta_NLA_U8)
    default:
        o << "IFLA_UNKNOWN(" << t.type << ")";
    }
    return o;
#undef CASE
}

ostream &operator<<(ostream &o, const AsRtaAddr &t) {
#define CASE(X, Y)                                            \
    case IFA_##X:                                             \
        o << #X ": " << Y(t.data, t.len, t.family, t.indent); \
        break;

    switch (t.type) {
        CASE(ADDRESS, AsRtaAddr_NLA_ADDRESS)
        CASE(BROADCAST, AsRtaAddr_NLA_ADDRESS)
        CASE(LOCAL, AsRtaAddr_NLA_ADDRESS)
        CASE(LABEL, AsRta_NLA_STRING)
    default:
        o << "IFA_UNKNOWN(" << t.type << ") size: " << t.len;
    }
    return o;
#undef CASE
}

ostream &operator<<(ostream &o, const AsRtaRt &t) {
#define CASE(X, Y)                                            \
    case RTA_##X:                                             \
        o << #X ": " << Y(t.data, t.len, t.family, t.indent); \
        break;

    switch (t.type) {
        CASE(DST, AsRtaAddr_NLA_ADDRESS)
        CASE(SRC, AsRtaAddr_NLA_ADDRESS)
        CASE(IIF, AsRta_NLA_U32)
        CASE(OIF, AsRta_NLA_U32)
        CASE(GATEWAY, AsRtaAddr_NLA_ADDRESS)
        CASE(PRIORITY, AsRta_NLA_U32)
        // CASE(METRICS, AsRta_NLA_U32)   // nested!
        CASE(PREFSRC, AsRta_NLA_UNKNOWN)  // TODO
        CASE(TABLE, AsRta_NLA_U32)

    default:
        o << "RTA_UNKNOWN(" << t.type << ") size: " << t.len;
    }
    return o;
#undef CASE
}

ostream &operator<<(ostream &o, const AsRtaNeighbour &t) {
#define CASE(X, Y)                                                 \
    case NDA_##X:                                                  \
        o << #X ": " << Y(t.data, t.len, t.parent_type, t.indent); \
        break;

    switch (t.type) {
        CASE(LLADDR, AsRta_NLA_ADDRESS);
        CASE(DST, AsRta_NLA_ADDRESS);
    default:
        o << "NDA_UNKNOWN(" << t.type << ")";
    }
    return o;
#undef CASE
}


ostream &operator<<(ostream &o, AsIfType t) {
#define CASE(X)                       \
    case ARPHRD_##X:                  \
        o << #X << "(" << t.i << ")"; \
        break;
    switch (t.i) {
        CASE(ETHER) CASE(LOOPBACK) CASE(PPP) CASE(SIT);
    default:
        o << "UNKNOWN(" << t.i << ")";
    }
    return o;
#undef CASE
}

ostream &operator<<(ostream &o, const AsRtmType &t) {
#define CASE(X)                              \
    case RTM_##X:                            \
        o << "RTM_" #X << "(" << t.i << ")"; \
        break;
    switch (t.i) {
        CASE(NEWLINK) CASE(DELLINK) CASE(GETLINK) CASE(SETLINK) CASE(NEWADDR) CASE(
                DELADDR) CASE(GETADDR) CASE(NEWROUTE) CASE(DELROUTE) CASE(GETROUTE)
                CASE(NEWNEIGH) CASE(DELNEIGH) CASE(GETNEIGH) CASE(NEWRULE) CASE(
                        DELRULE) CASE(GETRULE) CASE(NEWQDISC) CASE(DELQDISC) CASE(GETQDISC)
                        CASE(NEWTCLASS) CASE(DELTCLASS) CASE(GETTCLASS) CASE(
                                NEWTFILTER) CASE(DELTFILTER) CASE(GETTFILTER)
                                CASE(NEWACTION) CASE(DELACTION) CASE(GETACTION) CASE(
                                        NEWPREFIX) CASE(GETMULTICAST) CASE(GETANYCAST)
                                        CASE(NEWNEIGHTBL) CASE(GETNEIGHTBL) CASE(
                                                SETNEIGHTBL) CASE(NEWNDUSEROPT)
                                                CASE(NEWADDRLABEL) CASE(
                                                        DELADDRLABEL) CASE(GETADDRLABEL)
                                                        CASE(GETDCB) CASE(SETDCB) CASE(
                                                                NEWNETCONF) CASE(GETNETCONF)
                                                                CASE(NEWMDB) CASE(
                                                                        DELMDB)
                                                                        CASE(GETMDB);
    default:
        o << "RTM_UNKNOWN(" << t.i << ")";
    }
    return o;
#undef CASE
}

int fmt(ostream &o, const Fmt &fmt, const AsRtmType *t)
{
    o << *t;
    return 0;
}

ostream &operator<<(ostream &o, AsProtocolFamilyType t) {
#define CASE(X)                             \
    case PF_##X:                            \
        o << "PF_" #X << "(" << t.i << ")"; \
        break;
    switch (t.i) {
        CASE(UNSPEC) CASE(LOCAL) CASE(INET) CASE(AX25) CASE(IPX) CASE(
                APPLETALK) CASE(NETROM) CASE(BRIDGE) CASE(ATMPVC) CASE(X25)
                CASE(INET6) CASE(ROSE) CASE(DECnet) CASE(NETBEUI) CASE(
                        SECURITY) CASE(KEY) CASE(NETLINK) CASE(PACKET) CASE(ASH)
                        CASE(ECONET) CASE(ATMSVC) CASE(RDS) CASE(SNA) CASE(
                                IRDA) CASE(PPPOX) CASE(WANPIPE) CASE(LLC)
                                CASE(CAN) CASE(TIPC) CASE(BLUETOOTH) CASE(IUCV)
                                        CASE(RXRPC) CASE(ISDN) CASE(PHONET)
                                                CASE(IEEE802154) CASE(CAIF)
                                                        CASE(ALG) CASE(NFC);
    default:
        o << "PF_UNKNOWN(" << t.i << ")";
    }
    return o;
#undef CASE
}

ostream &operator<<(ostream &o, AsAddressFamilyType t) {
#define CASE(X)                             \
    case AF_##X:                            \
        o << "AF_" #X << "(" << t.i << ")"; \
        break;
    switch (t.i) {
        CASE(UNSPEC) CASE(LOCAL) CASE(INET) CASE(AX25) CASE(IPX) CASE(
                APPLETALK) CASE(NETROM) CASE(BRIDGE) CASE(ATMPVC) CASE(X25)
                CASE(INET6) CASE(ROSE) CASE(DECnet) CASE(NETBEUI) CASE(
                        SECURITY) CASE(KEY) CASE(NETLINK) CASE(PACKET) CASE(ASH)
                        CASE(ECONET) CASE(ATMSVC) CASE(RDS) CASE(SNA) CASE(
                                IRDA) CASE(PPPOX) CASE(WANPIPE) CASE(LLC)
                                CASE(CAN) CASE(TIPC) CASE(BLUETOOTH) CASE(IUCV)
                                        CASE(RXRPC) CASE(ISDN) CASE(PHONET)
                                                CASE(IEEE802154) CASE(CAIF)
                                                        CASE(ALG) CASE(NFC);
    default:
        o << "AF_UNKNOWN(" << t.i << ")";
    }
    return o;
#undef CASE
}

ostream &operator<<(ostream &o, AsIffFlags x) {
    bool nfirst = false;
    o << "[";

#define FLAG(F)              \
    if (x.flags & IFF_##F) { \
        if (nfirst) {        \
            o << ",";        \
        } else {             \
            nfirst = true;   \
        }                    \
        o << #F;             \
    }

    FLAG(UP) FLAG(BROADCAST) FLAG(DEBUG) FLAG(LOOPBACK) FLAG(POINTOPOINT)
            FLAG(NOTRAILERS) FLAG(RUNNING) FLAG(NOARP) FLAG(PROMISC)
                    FLAG(ALLMULTI) FLAG(MASTER) FLAG(SLAVE) FLAG(MULTICAST)
                            FLAG(PORTSEL) FLAG(AUTOMEDIA) FLAG(DYNAMIC)
                                    FLAG(LOWER_UP) FLAG(DORMANT) FLAG(ECHO);
#undef FLAG
    o << "]";
    return o;
}

ostream &operator<<(ostream &o, AsNdmState x) {
    bool nfirst = false;
    o << "[";

#define FLAG(F)              \
    if (x.state & NUD_##F) { \
        if (nfirst) {        \
            o << ",";        \
        } else {             \
            nfirst = true;   \
        }                    \
        o << #F;             \
    }

    FLAG(INCOMPLETE);
    FLAG(REACHABLE);
    FLAG(STALE);
    FLAG(DELAY);
    FLAG(PROBE);
    FLAG(FAILED);
    FLAG(NOARP);
    FLAG(PERMANENT);

#undef FLAG
    o << "]";
    return o;
}

ostream &operator<<(ostream &o, struct ifinfomsg *i) {
    o << "{family: " << AsAddressFamilyType(i->ifi_family)
      << ", type: " << AsIfType(i->ifi_type) << ", index: " << i->ifi_index
      << ", flags: " << AsIffFlags(i->ifi_flags)
      << ", change: " << i->ifi_change << "}";
    return o;
}

ostream &operator<<(ostream &o, struct ifaddrmsg *i) {
    o << "{family: " << AsAddressFamilyType(i->ifa_family)
      << ", prefix-len: " << i->ifa_prefixlen << ", iflags: " << i->ifa_flags
      << ", scope: " << i->ifa_scope << ", ifindex: " << i->ifa_index << "}";
    return o;
}

ostream &operator<<(ostream &o, struct rtmsg *i) {
    o << "{family: " << AsAddressFamilyType(i->rtm_family)
      << ", dst_len: " << i->rtm_dst_len << ", src_len: " << i->rtm_src_len
      << ", tos: " << i->rtm_tos << ", table: " << i->rtm_table
      << ", protocol: " << i->rtm_protocol << ", scope: " << i->rtm_scope
      << ", type: " << i->rtm_type << ", flags: " << i->rtm_flags << "}";
    return o;
}

ostream &operator<<(ostream &o, struct ndmsg *n) {
    o << "{family: " << AsAddressFamilyType(n->ndm_family)
      << ", ifindex: " << n->ndm_ifindex << ", state: " << AsNdmState(n->ndm_state)
      << ", flags: " << n->ndm_flags << ", type: " << n->ndm_type << "}";
    return o;
}


ostream &operator<<(ostream &o, const Indent &i) {
    for (int j = 0; j < i.level; ++j) o << "    ";
    return o;
}

// TODO, this does not work for GenericNetlink as the namespaces are collapsing
ostream &operator<<(ostream &o, const struct nlmsghdr &n) {
    o << AsRtmType(n.nlmsg_type);
    int len = n.nlmsg_len;
    Indent indent;

    switch (n.nlmsg_type) {
    case RTM_NEWLINK:
    case RTM_DELLINK:
    case RTM_GETLINK: {
        /*
         * Create, remove or get information about a specific network
         * interface. These messages contain an ifinfomsg structure followed
         * by a series of rtattr structures.
         *
         * struct ifinfomsg {
         *     unsigned char  ifi_family; // AF_UNSPEC
         *     unsigned short ifi_type;   // Device type
         *     int            ifi_index;  // Interface index
         *     unsigned int   ifi_flags;  // Device flags
         *     unsigned int   ifi_change; // change mask
         * };
         *
         * ifi_flags contains the device flags, see netdevice(7); ifi_index
         * is the unique interface index (since Linux 3.7, it is possible to
         * feed a nonzero value with the RTM_NEWLINK message, thus creating
         * a link with the given ifindex); ifi_change is reserved for future
         * use and should be always set to 0xFFFFFFFF.
         *
         * The value type for IFLA_STATS is struct rtnl_link_stats (struct
         * net_device_stats in Linux 2.4 and earlier).
         */

        struct ifinfomsg *ifi = (struct ifinfomsg *)NLMSG_DATA((&n));
        len -= NLMSG_LENGTH(sizeof(*ifi));
        if (len < 0) {
            o << " <msg too short for this type!>";
            return o;
        }
        o << " " << ifi;

        struct rtattr *rta = IFLA_RTA(ifi);
        o << "\n";
        indent.inc();
        while (RTA_OK(rta, len)) {
            o << indent << AsRtaLink(rta, ifi->ifi_type, indent) << "\n";
            rta = RTA_NEXT(rta, len);
        }
        indent.dec();

        break;
    }

    case RTM_NEWADDR:
    case RTM_DELADDR:
    case RTM_GETADDR: {
        /*
          Add,  remove  or  receive  information  about  an   IP   address
          associated  with  an  interface.   In Linux 2.2 an interface can
          carry multiple IP addresses,  this  replaces  the  alias  device
          concept  in  2.0.   In Linux 2.2 these messages support IPv4 and
          IPv6 addresses.  They contain an ifaddrmsg structure, optionally
          followed by rtaddr routing attributes.

          struct ifaddrmsg {
              unsigned char ifa_family;     // Address type
              unsigned char ifa_prefixlen;  // Prefixlength of address
              unsigned char ifa_flags;      // Address flags
              unsigned char ifa_scope;      // Address scope
              int           ifa_index;      // Interface index
          };

          ifa_family  is  the  address  family  type (currently AF_INET or
          AF_INET6), ifa_prefixlen is the length of the  address  mask  of
          the address if defined for the family (like for IPv4), ifa_scope
          is the address scope, ifa_index is the interface  index  of  the
          interface  the  address is associated with.  ifa_flags is a flag
          word  of  IFA_F_SECONDARY  for  secondary  address  (old   alias
          interface),  IFA_F_PERMANENT  for a permanent address set by the
          user and other undocumented flags.

           Attributes
          rta_type        value type             description
          -------------------------------------------------------------
          IFA_UNSPEC      -                      unspecified.
          IFA_ADDRESS     raw protocol address   interface address
          IFA_LOCAL       raw protocol address   local address
          IFA_LABEL       asciiz string          name of the interface
          IFA_BROADCAST   raw protocol address   broadcast address.
          IFA_ANYCAST     raw protocol address   anycast address
          IFA_CACHEINFO   struct ifa_cacheinfo   Address information.
        */

        struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA((&n));
        len -= NLMSG_LENGTH(sizeof(*ifa));
        if (len < 0) {
            o << " <msg too short for this type!>";
            return o;
        }
        o << " " << ifa;

        struct rtattr *rta = IFA_RTA(ifa);
        o << "\n";
        indent.inc();
        while (RTA_OK(rta, len)) {
            o << indent << AsRtaAddr(rta, ifa->ifa_family, indent) << "\n";
            rta = RTA_NEXT(rta, len);
        }
        indent.dec();
        break;
    }

    case RTM_NEWROUTE:
    case RTM_DELROUTE:
    case RTM_GETROUTE: {
        /*
           Create, remove or receive information  about  a  network  route.
           These  messages  contain  an  rtmsg  structure  with an optional
           sequence  of  rtattr  structures  following.   For  RTM_GETROUTE
           setting  rtm_dst_len  and  rtm_src_len  to  0  means you get all
           entries for the specified routing table.  For the  other  fields
           except rtm_table and rtm_protocol 0 is the wildcard.

           struct rtmsg {
               unsigned char rtm_family;    // Address family of route
               unsigned char rtm_dst_len;   // Length of source
               unsigned char rtm_src_len;   // Length of destination
               unsigned char rtm_tos;       // TOS filter

               unsigned char rtm_table;     // Routing table ID
               unsigned char rtm_protocol;  // Routing protocol; see below
               unsigned char rtm_scope;     // See below
               unsigned char rtm_type;      // See below

               unsigned int  rtm_flags;
           };

           rtm_type          Route type
           -----------------------------------------------------------
           RTN_UNSPEC        unknown route
           RTN_UNICAST       a gateway or direct route
           RTN_LOCAL         a local interface route
           RTN_BROADCAST     a  local  broadcast  route  (sent  as  a
                             broadcast)
           RTN_ANYCAST       a  local  broadcast  route  (sent  as  a
                             unicast)
           RTN_MULTICAST     a multicast route
           RTN_BLACKHOLE     a packet dropping route
           RTN_UNREACHABLE   an unreachable destination
           RTN_PROHIBIT      a packet rejection route
           RTN_THROW         continue routing lookup in another table
           RTN_NAT           a network address translation rule
           RTN_XRESOLVE      refer   to  an  external  resolver  (not
                             implemented)

           rtm_protocol      Route origin.
           ---------------------------------------------
           RTPROT_UNSPEC     unknown
           RTPROT_REDIRECT   by   an   ICMP    redirect
                             (currently unused)
           RTPROT_KERNEL     by the kernel
           RTPROT_BOOT       during boot
           RTPROT_STATIC     by the administrator

           Values  larger  than  RTPROT_STATIC  are  not interpreted by the
           kernel, they are just for user information.  They may be used to
           tag  the  source  of  a  routing  information  or to distinguish
           between multiple routing daemons.  See  <linux/rtnetlink.h>  for
           the routing daemon identifiers which are already assigned.

           rtm_scope is the distance to the destination:

           RT_SCOPE_UNIVERSE   global route
           RT_SCOPE_SITE       interior   route   in  the
                               local autonomous system
           RT_SCOPE_LINK       route on this link
           RT_SCOPE_HOST       route on the local host
           RT_SCOPE_NOWHERE    destination doesn't exist

           The  values  between  RT_SCOPE_UNIVERSE  and  RT_SCOPE_SITE  are
           available to the user.

           The rtm_flags have the following meanings:

           RTM_F_NOTIFY     if  the  route changes, notify the user via
                            rtnetlink
           RTM_F_CLONED     route is cloned from another route
           RTM_F_EQUALIZE   a multipath equalizer (not yet implemented)

           rtm_table specifies the routing table

           RT_TABLE_UNSPEC    an unspecified routing table
           RT_TABLE_DEFAULT   the default table
           RT_TABLE_MAIN      the main table
           RT_TABLE_LOCAL     the local table

           The user may assign arbitrary values between RT_TABLE_UNSPEC and
           RT_TABLE_DEFAULT.

            Attributes
           rta_type        value type         description
           --------------------------------------------------------------

           RTA_UNSPEC      -                  ignored.
           RTA_DST         protocol address   Route destination address.
           RTA_SRC         protocol address   Route source address.
           RTA_IIF         int                Input interface index.
           RTA_OIF         int                Output interface index.
           RTA_GATEWAY     protocol address   The gateway of the route
           RTA_PRIORITY    int                Priority of route.
           RTA_PREFSRC
           RTA_METRICS     int                Route metric
           RTA_MULTIPATH
           RTA_PROTOINFO
           RTA_FLOW
           RTA_CACHEINFO

           Fill these values in!
         */

        struct rtmsg *rt = (struct rtmsg *)NLMSG_DATA((&n));
        len -= NLMSG_LENGTH(sizeof(*rt));
        if (len < 0) {
            o << " <msg too short for this type!>";
            return o;
        }
        o << " " << rt;

        struct rtattr *rta = RTM_RTA(rt);
        o << "\n";
        indent.inc();
        while (RTA_OK(rta, len)) {
            o << indent << AsRtaRt(rta, rt->rtm_family, indent) << "\n";
            rta = RTA_NEXT(rta, len);
        }
        indent.dec();

        break;
    }

    case RTM_NEWNEIGH:
    case RTM_DELNEIGH:
    case RTM_GETNEIGH: {
        struct ndmsg *ndm = (struct ndmsg *)NLMSG_DATA((&n));
        o << len << " " << NLMSG_LENGTH(sizeof(*ndm));
        len -= NLMSG_LENGTH(sizeof(*ndm));
        if (len < 0) {
            o << " <msg too short for this type!>";
            return o;
        }
        o << " " << ndm;

        struct rtattr *rta = NDM_RTA(ndm);
        o << "\n";
        indent.inc();
        while (RTA_OK(rta, len)) {
            o << indent << AsRtaNeighbour(rta, ndm->ndm_type, indent) << "\n";
            rta = RTA_NEXT(rta, len);
        }
        indent.dec();

        break;
    }

    default:
        o << " <no pretty-printer for this type>";
    }

    return o;
}

void NetlinkCallbackPrint::operator()(struct sockaddr_nl *addr,
                                      struct nlmsghdr *hdr) {
    I << hdr;
}

mesa_rc attr_add_binary(struct nlmsghdr *n, int max_length, int type,
                        const void *data, int data_length) {
    int len = RTA_LENGTH(data_length);
    struct rtattr *rta;

    if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > max_length) {
        E << "Out of space in message!";
        return VTSS_RC_ERROR;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
    rta = NLMSG_END(n);
#pragma GCC diagnostic pop
    rta->rta_type = type;
    rta->rta_len = len;
    memcpy(RTA_DATA(rta), data, data_length);
    n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);

    return VTSS_RC_OK;
}

mesa_rc attr_add_ipv4(struct nlmsghdr *n, int max_length, int type,
                      mesa_ipv4_t addr) {
    unsigned char buf[4];

    buf[3] = addr & 0xff;
    addr >>= 8;
    buf[2] = addr & 0xff;
    addr >>= 8;
    buf[1] = addr & 0xff;
    addr >>= 8;
    buf[0] = addr & 0xff;

    return attr_add_binary(n, max_length, type, buf, 4);
}

mesa_rc attr_add_str(struct nlmsghdr *n, int max_length, int type,
                     const char *str) {
    return attr_add_binary(n, max_length, type, str, strlen(str) + 1);
}

struct rtattr *attr_nest(struct nlmsghdr *n, int maxlen, int type) {
    struct rtattr *nest = nlmsg_tail(n);
    if (attr_add_binary(n, maxlen, type, NULL, 0) != VTSS_RC_OK) return nullptr;
    return nest;
}

void attr_nest_end(struct nlmsghdr *n, struct rtattr *nest) {
    nest->rta_len = ((char *)nlmsg_tail(n)) - ((char *)nest);
}

int parse_nested_attr(struct rtattr *nested_attrs_table[], int max_attr_type, const struct rtattr *nested_attr) {
    int           len = RTA_PAYLOAD(nested_attr), remainder = len;
	struct rtattr *attr;

	memset(nested_attrs_table, 0, sizeof(struct rtattr *) * (max_attr_type + 1));

    for (attr = (struct rtattr *)RTA_DATA(nested_attr); RTA_OK(attr, remainder); attr = RTA_NEXT(attr, remainder)) {
		if (attr->rta_type > 0 && attr->rta_type <= max_attr_type) {
			nested_attrs_table[attr->rta_type] = attr;
		} else {
            E << "Type " << attr->rta_type << " is out of range [0; " << max_attr_type << "]";
            return -1;
        }
	}

	return 0;
}

static mesa_rc __nl_req(const void *req, size_t len, int seq, const char *func, int proto, NetlinkCallbackAbstract *cb, int sndbuf, int rcvbuf)
{
    struct sockaddr_nl sa;
    struct nlmsghdr    *nlh, *h;
    struct sockaddr_nl nladdr;
    struct iovec       iov;
    struct msghdr      msg = {};
    struct nlmsgerr    *err;
    char               buf[16384];
    int                retries, fd = -1, pid, dump_intr, status, found_done, msglen;
    const int          retry_cnt = 2;
    mesa_rc            rc;

    I << "Enter from " << func << "()";

    // At times, the netlink call fails the first time, so we retry retry_cnt
    // times.
    for (retries = 0; retries < retry_cnt + 1; retries++) {
        if (fd != -1) {
            D << "close(" << fd << ")";
            ::close(fd);
        }

        fd = socket(PF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, proto);

        if (fd < 0) {
            E << "Failed socket: " << strerror(errno);
            return VTSS_RC_ERROR;
        }

        D << "socket() = " << fd;

        if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0) {
            E << "setsockopt(SO_SNDBUF) failed: errno = " << ::strerror(errno);
            rc = VTSS_RC_ERROR;
            goto close;
        }

        if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) < 0) {
            E << "setsockopt(SO_RCVBUF) failed: errno = " << ::strerror(errno);
            rc = VTSS_RC_ERROR;
            goto close;
        }

        pid = 0;
        vtss_clear(sa);
        sa.nl_family = AF_NETLINK;
        sa.nl_pid    = pid;
        sa.nl_groups = 0;

        nlh = (struct nlmsghdr *)req;
        nlh->nlmsg_pid = pid;

        if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
            E << "bind() failed: errno = " << ::strerror(errno);
            rc = VTSS_RC_ERROR;
            goto close;
        }

        if (send(fd, req, len, 0) != len) {
            E << "send() failed: errno = " << ::strerror(errno);
            rc = VTSS_RC_ERROR;
            goto close;
        }

        msg.msg_name    = &nladdr,
        msg.msg_namelen = sizeof(nladdr),
        msg.msg_iov     = &iov,
        msg.msg_iovlen  = 1,
        dump_intr       = 0;
        iov.iov_base    = buf;

        while (1) {
            found_done  = 0;
            msglen      = 0;
            iov.iov_len = sizeof(buf);
            status      = recvmsg(fd, &msg, 0);

            if (status < 0) {
                I << "recvmsg(): " << strerror(errno);

                // Closing and re-opening the socket alleviates most problems.
                if (retries == retry_cnt) {
                    E << "Failed socket: " << strerror(errno);
                    rc = VTSS_RC_ERROR;
                    goto close;
                } else {
                    I << "Failed socket: " << strerror(errno) << ". Retrying";
                    break;
                }
            }

            if (status == 0) {
                D << "EOF on netlink";
                rc = VTSS_RC_OK;
                goto close;
            }

            h = (struct nlmsghdr *)buf;
            msglen = status;

            N << "Got message of " << msglen << " bytes";
            N << h;

            while (NLMSG_OK(h, msglen)) {
                N << h->nlmsg_pid << " " << h->nlmsg_seq << " " << seq;
                if (h->nlmsg_flags & NLM_F_DUMP_INTR) {
                    dump_intr = 1;
                }

                if (h->nlmsg_type == NLMSG_DONE) {
                    found_done = 1;
                    break; /* process next filter */
                }

                if (h->nlmsg_type == NLMSG_ERROR) {
                    err = (struct nlmsgerr *)NLMSG_DATA(h);
                    if (err->error == 0) {
                        rc = VTSS_RC_OK;
                        goto close;
                    }

                    if (h->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
                        E << "ERROR: truncated";
                    } else {
                        E << "RTNETLINK answers: " << strerror(-err->error) << " (" << err->error << ")";
                    }

                    rc = VTSS_RC_ERROR;
                    goto close;
                }

                if (cb) {
                    (*cb)(&nladdr, h);
                }

                h = NLMSG_NEXT(h, msglen);
            }

            if (found_done) {
                if (dump_intr) {
                    I << "Dump was interrupted and may be inconsistent";
                    continue;
                }

                rc = VTSS_RC_OK;
                goto close;
            }

            if (msg.msg_flags & MSG_TRUNC) {
                E << "Message truncated";
                continue;
            }

            if (msglen) {
                E << "Non-consumed " << msglen;
                rc = VTSS_RC_ERROR;
                goto close;
            }
        }
    }

    E << "Operation failed " << (retry_cnt + 1) << " times";
    rc = VTSS_RC_ERROR;

close:
    D << "close(" << fd << ")";
    I << "exit: " << error_txt(rc);
    ::close(fd);
    return rc;
}

mesa_rc nl_req(const void *req, size_t len, int seq, const char *func,
               NetlinkCallbackAbstract *cb, int sndbuf, int rcvbuf) {
    return __nl_req(req, len, seq, func, 0, cb, sndbuf, rcvbuf);
}

mesa_rc genl_req(const void *req, size_t len, int seq, const char *func,
                 NetlinkCallbackAbstract *cb, int sndbuf, int rcvbuf) {
    return __nl_req(req, len, seq, func, NETLINK_GENERIC, cb, sndbuf, rcvbuf);
}

struct NetlinkCallbackGenericNetlink : public NetlinkCallbackAbstract {
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *n) {
        int len = n->nlmsg_len;

        N << "CB type: " << (int)n->nlmsg_type;
        if (n->nlmsg_type != GENL_ID_CTRL) return;

        struct genlmsghdr *ghdr = (struct genlmsghdr *)NLMSG_DATA(n);
        len -= NLMSG_LENGTH(sizeof(*ghdr));
        if (len < 0) {
            E << " <msg too short for this type!>";
            return;
        }

        if (ghdr->cmd != CTRL_CMD_GETFAMILY &&
            ghdr->cmd != CTRL_CMD_NEWFAMILY &&
            ghdr->cmd != CTRL_CMD_NEWMCAST_GRP) {
            N << "Unexpected command: " << (int)ghdr->cmd;
            return;
        }

        struct rtattr *rta = (struct rtattr *)((char *)ghdr + sizeof(*ghdr));

        while (RTA_OK(rta, len)) {
            switch (rta->rta_type) {
            case CTRL_ATTR_FAMILY_NAME:
                name = (char *)RTA_DATA(rta);
                N << "Name: " << name;
                break;

            case CTRL_ATTR_FAMILY_ID:
                id = *((uint16_t *)RTA_DATA(rta));
                N << "ID: " << id;
                break;

            case CTRL_ATTR_VERSION:
                version = *((uint32_t *)RTA_DATA(rta));
                N << "Version: " << version;
                break;

            case CTRL_ATTR_HDRSIZE:
                header_size = *((uint32_t *)RTA_DATA(rta));
                N << "Header size: " << header_size;
                break;

            case CTRL_ATTR_MAXATTR:
                maxattr = *((uint32_t *)RTA_DATA(rta));
                N << "Max attr: " << maxattr;
                break;

            default:
                ;
            }

            rta = RTA_NEXT(rta, len);
        }
    }

    std::string name;
    uint16_t id = -1;
    uint32_t version = -1;
    uint32_t header_size = -1;
    uint32_t maxattr = -1;
};

static int nl_seq;

int netlink_seq() {
    int result;
    vtss_global_lock(__FILE__, __LINE__);
    result = ++nl_seq;
    vtss_global_unlock(__FILE__, __LINE__);
    return result;
}

int genelink_channel_by_name(const char *name, const char *func) {
    mesa_rc rc = VTSS_RC_OK;
    NetlinkCallbackGenericNetlink cb;
    int seq = netlink_seq(), len = strlen(name);

    if (len >= GENL_NAMSIZ) {
        E << "Netlink name (" << name << " with length = " << len <<
              ") too long. Max allowed = " << (GENL_NAMSIZ - 1);
        return -1;
    }

    struct {
        struct nlmsghdr n;
        struct genlmsghdr g;
        char attr[1024];
    } req;

    memset(&req, 0, sizeof(req));
    req.n.nlmsg_seq = seq;
    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct genlmsghdr));
    req.n.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    req.n.nlmsg_type = GENL_ID_CTRL;
    req.g.cmd = CTRL_CMD_GETFAMILY;

    rc = attr_add_str(&req.n, sizeof(req), CTRL_ATTR_FAMILY_NAME, name);
    if (rc != VTSS_RC_OK) {
        E << "Failed to add name: " << name;
        return -1;
    }

    rc = genl_req((const void *)&req, req.n.nlmsg_len, seq, func, &cb, 2048, 4096);
    if (rc != VTSS_RC_OK) {
        E << "Netlink request failed";
        return -1;
    }

    if (cb.name != std::string(name)) {
        E << "Names did not match";
        return -1;
    }

    return cb.id;
}

}  // namespace netlink
}  // namespace appl
}  // namespace vtss

int fmt(vtss::ostream &o, const vtss::Fmt &fmt, const nlmsghdr *n)
{
    using namespace vtss::appl::netlink;
    o << *n;
    return 0;
}

