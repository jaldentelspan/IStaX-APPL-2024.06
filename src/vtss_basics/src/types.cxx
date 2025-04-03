/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "vtss/basics/types.hxx"
#include "vtss/basics/string.hxx"
#include "vtss/basics/parser_impl.hxx"

namespace vtss {

bool MacAddress::equal_to(const ::mesa_mac_t &rhs) const {
    return data_.addr[0] == rhs.addr[0] &&
        data_.addr[1] == rhs.addr[1] &&
        data_.addr[2] == rhs.addr[2] &&
        data_.addr[3] == rhs.addr[3] &&
        data_.addr[4] == rhs.addr[4] &&
        data_.addr[5] == rhs.addr[5];
}

bool MacAddress::less_than(const ::mesa_mac_t &rhs) const {
    if (data_.addr[0] != rhs.addr[0])
        return data_.addr[0] < rhs.addr[0];
    if (data_.addr[1] == rhs.addr[1])
        return data_.addr[1] < rhs.addr[1];
    if (data_.addr[2] == rhs.addr[2])
        return data_.addr[2] < rhs.addr[2];
    if (data_.addr[3] == rhs.addr[3])
        return data_.addr[3] < rhs.addr[3];
    if (data_.addr[4] == rhs.addr[4])
        return data_.addr[4] < rhs.addr[4];
    return data_.addr[5] < rhs.addr[5];
}

bool MacAddress::less_or_equal_than(const ::mesa_mac_t &rhs) const {
    // TODO(anielse) optimize
    if (data_.addr[0] != rhs.addr[0])
        return data_.addr[0] < rhs.addr[0];
    if (data_.addr[1] == rhs.addr[1])
        return data_.addr[1] < rhs.addr[1];
    if (data_.addr[2] == rhs.addr[2])
        return data_.addr[2] < rhs.addr[2];
    if (data_.addr[3] == rhs.addr[3])
        return data_.addr[3] < rhs.addr[3];
    if (data_.addr[4] == rhs.addr[4])
        return data_.addr[4] < rhs.addr[4];
    return data_.addr[5] <= rhs.addr[5];
}

static inline bool equal_(const ::mesa_ipv6_t &lhs, const ::mesa_ipv6_t &rhs) {
    for (int i = 0; i < 16; ++i) {
        if (lhs.addr[i] != rhs.addr[i])
            return false;
    }
    return true;
}
static inline bool less_than_(const ::mesa_ipv6_t &lhs,
                              const ::mesa_ipv6_t &rhs) {
    for (int i = 0; i < 15; ++i) {
        if (lhs.addr[i] != rhs.addr[i])
            return lhs.addr[i] < rhs.addr[i];
    }
    return lhs.addr[15] < rhs.addr[15];
}
static inline bool less_or_equal_than_(const ::mesa_ipv6_t &lhs,
                                       const ::mesa_ipv6_t &rhs) {
    for (int i = 0; i < 15; ++i) {
        if (lhs.addr[i] != rhs.addr[i])
            return lhs.addr[i] < rhs.addr[i];
    }
    return lhs.addr[15] <= rhs.addr[15];
}

bool Ipv6Address::equal_to(const ::mesa_ipv6_t &rhs) const {
    return equal_(data_, rhs);
}
bool Ipv6Address::less_than(const ::mesa_ipv6_t &rhs) const {
    return less_than_(data_, rhs);
}
bool Ipv6Address::less_or_equal_than(const ::mesa_ipv6_t &rhs) const {
    return less_or_equal_than_(data_, rhs);
}


static inline bool equal_(const ::mesa_ipv6_network_t &lhs,
                          const ::mesa_ipv6_network_t &rhs) {
    if (!equal_(lhs.address, rhs.address))
        return false;
    return lhs.prefix_size == rhs.prefix_size;
}
static inline bool less_than_(const ::mesa_ipv6_network_t &lhs,
                              const ::mesa_ipv6_network_t &rhs) {
    for (int i = 0; i < 16; ++i) {
        if (lhs.address.addr[i] != rhs.address.addr[i])
            return lhs.address.addr[i] < rhs.address.addr[i];
    }
    return lhs.prefix_size < rhs.prefix_size;
}
static inline bool less_or_equal_than_(const ::mesa_ipv6_network_t &lhs,
                                       const ::mesa_ipv6_network_t &rhs) {
    for (int i = 0; i < 16; ++i) {
        if (lhs.address.addr[i] != rhs.address.addr[i])
            return lhs.address.addr[i] < rhs.address.addr[i];
    }
    return lhs.prefix_size <= rhs.prefix_size;
}

bool Ipv6Network::equal_to(const ::mesa_ipv6_network_t &rhs) const {
    return equal_(data_, rhs);
}
bool Ipv6Network::less_than(const ::mesa_ipv6_network_t &rhs) const {
    return less_than_(data_, rhs);
}
bool Ipv6Network::less_or_equal_than(const ::mesa_ipv6_network_t &rhs) const {
    return less_or_equal_than_(data_, rhs);
}

static inline bool equal_(const ::mesa_ip_addr_t &lhs,
                          const ::mesa_ip_addr_t &rhs) {
    if (lhs.type != rhs.type)
        return false;

    switch (lhs.type) {
      case MESA_IP_TYPE_NONE:
        return true;

      case MESA_IP_TYPE_IPV4:
        return lhs.addr.ipv4 == rhs.addr.ipv4;

      case MESA_IP_TYPE_IPV6:
        return equal_(lhs.addr.ipv6, rhs.addr.ipv6);
    }

    return false;
}
static inline bool less_than_(const ::mesa_ip_addr_t &lhs,
                              const ::mesa_ip_addr_t &rhs) {
    if (lhs.type != rhs.type)
        return lhs.type < rhs.type;

    switch (lhs.type) {
      case MESA_IP_TYPE_NONE:
        return false;  // lhs == rhs

      case MESA_IP_TYPE_IPV4:
        return lhs.addr.ipv4 < rhs.addr.ipv4;

      case MESA_IP_TYPE_IPV6:
        return less_than_(lhs.addr.ipv6, rhs.addr.ipv6);
    }

    return false;
}
static inline bool less_or_equal_than_(const ::mesa_ip_addr_t &lhs,
                                       const ::mesa_ip_addr_t &rhs) {
    if (lhs.type != rhs.type)
        return lhs.type < rhs.type;

    switch (lhs.type) {
      case MESA_IP_TYPE_NONE:
        return false;  // lhs == rhs

      case MESA_IP_TYPE_IPV4:
        return lhs.addr.ipv4 <= rhs.addr.ipv4;

      case MESA_IP_TYPE_IPV6:
        return less_or_equal_than_(lhs.addr.ipv6, rhs.addr.ipv6);
    }

    return false;
}
bool IpAddress::string_parse(const char *&b, const char *e) {
    const char * _b = b;
    parser::IPv4 ipv4;
    parser::IPv6 ipv6;
    parser::Lit no_address("<no-address>");

    if (ipv4(b, e)) {
        data_.type = MESA_IP_TYPE_IPV4;
        data_.addr.ipv4 = ipv4.get().as_api_type();
        return true;
    }

    if (ipv6(b, e)) {
        data_.type = MESA_IP_TYPE_IPV6;
        data_.addr.ipv6 = ipv6.get().as_api_type();
        return true;
    }

    if (no_address(b, e)) {
        data_.type = MESA_IP_TYPE_NONE;
        return true;
    }

    b = _b;
    return false;
}
bool IpAddress::equal_to(const ::mesa_ip_addr_t &rhs) const {
    return equal_(data_, rhs);
}
bool IpAddress::less_than(const ::mesa_ip_addr_t &rhs) const {
    return less_than_(data_, rhs);
}
bool IpAddress::less_or_equal_than(const ::mesa_ip_addr_t &rhs) const {
    return less_or_equal_than_(data_, rhs);
}

bool IpNetwork::equal_to(const ::mesa_ip_network_t &rhs) const {
    if (!equal_(data_.address, rhs.address))
        return false;
    return data_.prefix_size == rhs.prefix_size;
}
bool IpNetwork::less_than(const ::mesa_ip_network_t &rhs) const {
    if (!equal_(data_.address, rhs.address))
        return less_than_(data_.address, rhs.address);
    return data_.prefix_size < rhs.prefix_size;
}
bool IpNetwork::less_or_equal_than(const ::mesa_ip_network_t &rhs) const {
    if (!equal_(data_.address, rhs.address))
        return less_or_equal_than_(data_.address, rhs.address);
    return data_.prefix_size <= rhs.prefix_size;
}

bool DomainName::equal_to(const ::vtss_domain_name_t &rhs) const {
    return strcmp(data_.name, rhs.name) == 0;
}

bool DomainName::less_than(const ::vtss_domain_name_t &rhs) const {
    return strcmp(data_.name, rhs.name) < 0;
}

bool DomainName::less_or_equal_than(const ::vtss_domain_name_t &rhs) const {
    return strcmp(data_.name, rhs.name) <= 0;
}

bool InetAddress::string_parse(const char *&b, const char *e) {
    const char * _b = b;
    parser::IPv4 ipv4;
    parser::IPv6 ipv6;
    parser::DomainName dns;
    parser::Lit no_address("<no-address>");

    if (ipv4(b, e)) {
        data_.type = VTSS_INET_ADDRESS_TYPE_IPV4;
        data_.address.ipv4 = ipv4.get().as_api_type();
        return true;
    }

    if (ipv6(b, e)) {
        data_.type = VTSS_INET_ADDRESS_TYPE_IPV6;
        data_.address.ipv6 = ipv6.get().as_api_type();
        return true;
    }

    if (dns(b, e)) {
        data_.type = VTSS_INET_ADDRESS_TYPE_DNS;
        char *i = copy(dns.b_, dns.e_,
                       data_.address.domain_name.name);
        *i = 0;
        return true;
    }

    if (no_address(b, e)) {
        data_.type = VTSS_INET_ADDRESS_TYPE_NONE;
        return true;
    }

    b = _b;
    return false;
}

bool InetAddress::equal_to(const ::vtss_inet_address_t &rhs) const {
    if (data_.type != rhs.type)
        return false;

    switch (data_.type) {
      case VTSS_INET_ADDRESS_TYPE_NONE:
        return true;

      case VTSS_INET_ADDRESS_TYPE_IPV4:
        return data_.address.ipv4 == rhs.address.ipv4;

      case VTSS_INET_ADDRESS_TYPE_IPV6:
        return equal_(data_.address.ipv6, rhs.address.ipv6);

      case VTSS_INET_ADDRESS_TYPE_DNS:
        return strcmp(data_.address.domain_name.name,
                      rhs.address.domain_name.name) == 0;

      default:
        return memcmp(this, &rhs, sizeof(vtss_inet_address_t)) == 0;
    }
}

bool InetAddress::less_than(const ::vtss_inet_address_t &rhs) const {
    if (data_.type != rhs.type)
        return data_.type < rhs.type;

    switch (data_.type) {
      case VTSS_INET_ADDRESS_TYPE_NONE:
        return false;

      case VTSS_INET_ADDRESS_TYPE_IPV4:
        return data_.address.ipv4 < rhs.address.ipv4;

      case VTSS_INET_ADDRESS_TYPE_IPV6:
        return less_than_(data_.address.ipv6, rhs.address.ipv6);

      case VTSS_INET_ADDRESS_TYPE_DNS:
        return strcmp(data_.address.domain_name.name,
                      rhs.address.domain_name.name) < 0;

      default:
        return memcmp(this, &rhs, sizeof(vtss_inet_address_t)) < 0;
    }
}

bool InetAddress::less_or_equal_than(const ::vtss_inet_address_t &rhs) const {
    if (data_.type != rhs.type)
        return data_.type < rhs.type;

    switch (data_.type) {
      case VTSS_INET_ADDRESS_TYPE_NONE:
        return true;

      case VTSS_INET_ADDRESS_TYPE_IPV4:
        return data_.address.ipv4 <= rhs.address.ipv4;

      case VTSS_INET_ADDRESS_TYPE_IPV6:
        return less_or_equal_than_(data_.address.ipv6, rhs.address.ipv6);

      case VTSS_INET_ADDRESS_TYPE_DNS:
        return strcmp(data_.address.domain_name.name,
                      rhs.address.domain_name.name) <= 0;

      default:
        return memcmp(this, &rhs, sizeof(vtss_inet_address_t)) <= 0;
    }
}

}  // namespace vtss

