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

#ifndef __VTSS_NETLINK_HXX__
#define __VTSS_NETLINK_HXX__

#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "vtss/basics/stream.hxx"

#define NDM_RTA(r) \
    ((struct rtattr *)(((char *)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))

#define NLMSG_END(m) \
    ((struct rtattr *)(((char *)(m)) + NLMSG_ALIGN((m)->nlmsg_len)))

#define GENL_RTA(r) \
    ((struct rtattr *)(((char *)(r)) + NLMSG_ALIGN(sizeof(struct genlmsghdr))))


namespace vtss {
namespace appl {
namespace netlink {

struct NetlinkCallbackAbstract {
    virtual void operator()(struct sockaddr_nl *addr, struct nlmsghdr *hdr) = 0;
};

int netlink_seq();

mesa_rc nl_req(const void *req, size_t len, int seq, const char *func,
               NetlinkCallbackAbstract *cb, int sndbuf = 32768,
               int rcvbuf = 1048576);

mesa_rc genl_req(const void *req, size_t len, int seq, const char *func,
                 NetlinkCallbackAbstract *cb = nullptr, int sndbuf = 32768,
                 int rcvbuf = 1048576);

template <typename T>
inline int nl_req(const T &t, NetlinkCallbackAbstract *cb, uint16_t type,
                  uint16_t flags, int sndbuf = 32768, int rcvbuf = 1048576) {
    struct {
        struct nlmsghdr nlh;
        T msg;
    } req;
    int seq = netlink_seq();
    memset(&req, 0, sizeof(req));
    req.nlh.nlmsg_len = sizeof(req);
    req.nlh.nlmsg_type = type;
    req.nlh.nlmsg_flags = flags | NLM_F_REQUEST;
    req.nlh.nlmsg_pid = 0;
    req.nlh.nlmsg_seq = seq;

    return nl_req((const void *)&req, sizeof(req), seq, __FUNCTION__, cb, sndbuf, rcvbuf);
}

struct Indent {
    Indent() : level(0) {}
    explicit Indent(int i) : level(i) {}
    void inc() { level += 1; }
    void dec() { level -= 1; }
    Indent open() const { return Indent(level + 1); }
    Indent close() const { return Indent(level + 1); }
    int level;
};
ostream &operator<<(ostream &o, const Indent &t);

struct AsIffFlags {
    AsIffFlags(int f) : flags(f) {}
    const int flags;
};
ostream &operator<<(ostream &o, AsIffFlags x);

struct AsNdmState {
    AsNdmState(int f) : state(f) {}
    const int state;
};
ostream &operator<<(ostream &o, AsNdmState x);

struct AsIfType {
    AsIfType(int x) : i(x) {}
    int i;
};
ostream &operator<<(ostream &o, AsIfType t);

struct AsRtmType {
    AsRtmType(int x) : i(x) {}
    int i;
};
ostream &operator<<(ostream &o, const AsRtmType &t);
int fmt(ostream &o, const Fmt &fmt, const AsRtmType *t);

struct AsProtocolFamilyType {
    AsProtocolFamilyType(int x) : i(x) {}
    int i;
};
ostream &operator<<(ostream &o, AsProtocolFamilyType t);

struct AsAddressFamilyType {
    AsAddressFamilyType(int x) : i(x) {}
    int i;
};
ostream &operator<<(ostream &o, AsAddressFamilyType t);

struct AsRta_NLA_U32 {
    AsRta_NLA_U32(const void *d, int l, int ptype, Indent i)
        : data(d), len(l) {}
    const void *data;
    const int len;
};
ostream &operator<<(ostream &o, AsRta_NLA_U32 x);

struct AsRta_NLA_U8 {
    AsRta_NLA_U8(const void *d, int l, int ptype, Indent i) : data(d), len(l) {}
    const void *data;
    const int len;
};
ostream &operator<<(ostream &o, AsRta_NLA_U8 x);

struct AsRta_NLA_STRING {
    AsRta_NLA_STRING(const void *d, int l, int ptype, Indent i)
        : data(d), len(l) {}
    const void *data;
    const int len;
};
ostream &operator<<(ostream &o, AsRta_NLA_STRING x);

struct AsRta_NLA_ADDRESS {
    AsRta_NLA_ADDRESS(const void *d, int l, int ptype, Indent i)
        : data(d), len(l), parent_type(ptype) {}
    const void *data;
    const int len;
    const int parent_type;
};
ostream &operator<<(ostream &o, AsRta_NLA_ADDRESS x);

struct AsRtaAddr_NLA_ADDRESS {
    AsRtaAddr_NLA_ADDRESS(const void *d, int l, int f, Indent i)
        : data(d), len(l), family(f) {}
    const void *data;
    const int len;
    const int family;
};
ostream &operator<<(ostream &o, AsRtaAddr_NLA_ADDRESS x);

struct AsRta_NLA_UNKNOWN {
    AsRta_NLA_UNKNOWN(const void *d, int l, int ptype, Indent i)
        : data(d), len(l) {}
    const void *data;
    const int len;
};
ostream &operator<<(ostream &o, AsRta_NLA_UNKNOWN x);

struct AsRta_IFLA_AF_SPEC {
    AsRta_IFLA_AF_SPEC(const void *d, int l, int ptype, Indent i)
        : data(d), len(l), parent_type(ptype), indent(std::move(i)) {}
    const void *data;
    const int len;
    const int parent_type;
    const Indent indent;
};
ostream &operator<<(ostream &o, AsRta_IFLA_AF_SPEC x);

struct AsRta_IFLA_INET {
    AsRta_IFLA_INET(const void *d, int l, int ptype, Indent i)
        : data(d), len(l), indent(i) {}
    const void *data;
    const int len;
    const Indent indent;
};
ostream &operator<<(ostream &o, AsRta_IFLA_INET x);

struct AsRta_IFLA_INET6 {
    AsRta_IFLA_INET6(const void *d, int l, int ptype, Indent i)
        : data(d), len(l), indent(i) {}
    const void *data;
    const int len;
    const Indent indent;
};
ostream &operator<<(ostream &o, AsRta_IFLA_INET6 x);

struct AsRtaLink {
    AsRtaLink(const rtattr *r, int ptype, Indent i)
        : data(RTA_DATA(r)),
          len(RTA_PAYLOAD(r)),
          type(r->rta_type),
          parent_type(ptype),
          indent(std::move(i)) {}
    const void *data;
    const int len;
    const int type;
    const int parent_type;
    const Indent indent;
};
ostream &operator<<(ostream &o, const AsRtaLink &t);

struct AsRtaNeighbour {
    AsRtaNeighbour(const rtattr *r, int ptype, Indent i)
        : data(RTA_DATA(r)),
          len(RTA_PAYLOAD(r)),
          type(r->rta_type),
          parent_type(ptype),
          indent(std::move(i)) {}
    const void *data;
    const int len;
    const int type;
    const int parent_type;
    const Indent indent;
};
ostream &operator<<(ostream &o, const AsRtaNeighbour &t);

struct AsRtaRt {
    AsRtaRt(const rtattr *r, int fam, Indent i)
        : data(RTA_DATA(r)),
          len(RTA_PAYLOAD(r)),
          type(r->rta_type),
          family(fam),
          indent(std::move(i)) {}
    const void *data;
    const int len;
    const int type;
    const int family;
    const Indent indent;
};
ostream &operator<<(ostream &o, const AsRtaRt &t);

struct AsRtaAddr {
    AsRtaAddr(const rtattr *r, int fam, Indent i)
        : data(RTA_DATA(r)),
          len(RTA_PAYLOAD(r)),
          type(r->rta_type),
          family(fam),
          indent(std::move(i)) {}
    const void *data;
    const int len;
    const int type;
    const int family;
    const Indent indent;
};
ostream &operator<<(ostream &o, const AsRtaAddr &t);

mesa_rc attr_add_binary(struct nlmsghdr *n, int max_length, int type, const void *data, int data_length);
mesa_rc attr_add_ipv4(struct nlmsghdr *n, int max_length, int type, uint32_t addr);
mesa_rc attr_add_str(struct nlmsghdr *n, int max_length, int type, const char *str);

inline mesa_rc attr_add_u32(struct nlmsghdr *n, int max_length, int type, uint32_t i) {
    return attr_add_binary(n, max_length, type, &i, sizeof(i));
}

inline mesa_rc attr_add_u64(struct nlmsghdr *n, int max_length, int type, uint64_t i) {
    return attr_add_binary(n, max_length, type, &i, sizeof(i));
}

inline mesa_rc attr_add_ipv6(struct nlmsghdr *n, int max_length, int type, mesa_ipv6_t addr) {
    return attr_add_binary(n, max_length, type, addr.addr, 16);
}

inline mesa_rc attr_add_mac(struct nlmsghdr *n, int max_length, int type, const mesa_mac_t &m) {
    return attr_add_binary(n, max_length, type, m.addr, 6);
}

struct rtattr *attr_nest(struct nlmsghdr *n, int maxlen, int type);
void attr_nest_end(struct nlmsghdr *n, struct rtattr *nest);

int parse_nested_attr(struct rtattr *nested_attrs_table[], int max_attr_type,
                      const struct rtattr *nested_attr);

struct NetlinkCallbackPrint : public NetlinkCallbackAbstract {
    void operator()(struct sockaddr_nl *addr, struct nlmsghdr *hdr);
};

int genelink_channel_by_name(const char *name, const char *func);

ostream &operator<<(ostream &o, const struct nlmsghdr &n);

}  // namespace netlink
}  // namespace appl
}  // namespace vtss

int fmt(vtss::ostream &o, const vtss::Fmt &fmt, const struct nlmsghdr *n);

#endif  // __VTSS_NETLINK_HXX__

