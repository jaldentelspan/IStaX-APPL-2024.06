/* *****************************************************************************
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
 **************************************************************************** */

#ifndef __VTSS_TYPES_HXX__
#define __VTSS_TYPES_HXX__

#include <vtss/basics/string.hxx>
#include <vtss/basics/api_types.h>

namespace vtss {

namespace details {
    template<typename T, typename ST>
    struct WrapNoLessThan {
        const ST& as_api_type() const { return data_; }
        ST& as_api_type() { return data_; }
        void to_api_type(ST &t) const { t = data_; }
        void to_api_type(ST *t) const { *t = data_; }
        void from_api_type(const ST &t) { data_ = t; }
        void from_api_type(const ST *const t) { data_ = *t; }

        friend bool operator==(T const &a, T const &b) {
            return a.equal_to(b.as_api_type());
        }

        friend bool operator==(T const &a, ST const &b) {
            return a.equal_to(b);
        }

        friend bool operator==(ST const &a, T const &b) {
            return b.equal_to(a);
        }

        friend bool operator!=(T const &a, T const &b) {
            return !a.equal_to(b.as_api_type());
        }

        friend bool operator!=(T const &a, ST const &b) {
            return !a.equal_to(b);
        }

        friend bool operator!=(ST const &a, T const &b) {
            return !b.equal_to(a);
        }

      protected:
        ST data_;
    };

    template<typename T, typename ST>
    struct Wrap : public WrapNoLessThan<T, ST> {
        friend bool operator<(T const &a, T const &b) {
            return a.less_than(b.as_api_type());
        }

        friend bool operator<(T const &a, ST const &b) {
            return a.less_than(b);
        }

        friend bool operator<(ST const &a, T const &b) {
            return !b.less_than(a);
        }

        friend bool operator>(T const &a, T const &b) {
            return !a.less_than(b.as_api_type());
        }

        friend bool operator>(T const &a, ST const &b) {
            return !a.less_than(b);
        }

        friend bool operator>(ST const &a, T const &b) {
            return b.less_than(a);
        }

        friend bool operator<=(T const &a, T const &b) {
            return a.less_or_equal_than(b.as_api_type());
        }

        friend bool operator<=(T const &a, ST const &b) {
            return a.less_or_equal_than(b);
        }

        friend bool operator<=(ST const &a, T const &b) {
            return !b.less_or_equal_than(a);
        }

        friend bool operator>=(T const &a, T const &b) {
            return !a.less_or_equal_than(b.as_api_type());
        }

        friend bool operator>=(T const &a, ST const &b) {
            return !a.less_or_equal_than(b);
        }

        friend bool operator>=(ST const &a, T const &b) {
            return b.less_or_equal_than(a);
        }

    };
}  // namespace details

struct MacAddress;
struct Ipv4Address;
struct Ipv4Network;
struct Ipv6Address;
struct Ipv6Network;
struct IpAddress;
struct IpNetwork;

struct MacAddress : public details::Wrap<MacAddress, ::mesa_mac_t> {
    /*lint -save -e1008 */
    MacAddress() = default;
    /*lint -restore */
    MacAddress(const MacAddress &rhs) { data_ = rhs.as_api_type(); }
    explicit MacAddress(const ::mesa_mac_t &rhs) { data_ = rhs; }

    MacAddress& operator=(const MacAddress &rhs) {
        data_ = rhs.as_api_type();
        return *this;
    }

    MacAddress& operator=(const ::mesa_mac_t &rhs) {
        data_ = rhs;
        return *this;
    }

    void clear() {
        data_.addr[0] = 0;
        data_.addr[1] = 0;
        data_.addr[2] = 0;
        data_.addr[3] = 0;
        data_.addr[4] = 0;
        data_.addr[5] = 0;
    }

    bool equal_to(const ::mesa_mac_t &rhs) const;
    bool less_than(const ::mesa_mac_t &rhs) const;
    bool less_or_equal_than(const ::mesa_mac_t &rhs) const;
};

struct Ipv4Address : public details::Wrap<Ipv4Address, ::mesa_ipv4_t> {
    /*lint -save -e1008 */
    Ipv4Address() = default;
    /*lint -restore */
    Ipv4Address(const Ipv4Address &rhs) { data_ = rhs.as_api_type(); }
    explicit Ipv4Address(const ::mesa_ipv4_t &rhs) { data_ = rhs; }

    Ipv4Address& operator=(const Ipv4Address &rhs) {
        data_ = rhs.as_api_type();
        return *this;
    }

    Ipv4Address& operator=(const ::mesa_ipv4_t &rhs) {
        data_ = rhs;
        return *this;
    }

    bool equal_to(const ::mesa_ipv4_t &rhs) const {
        return data_ == rhs;
    }

    bool less_than(const ::mesa_ipv4_t &rhs) const {
        return data_ < rhs;
    }

    bool less_or_equal_than(const ::mesa_ipv4_t &rhs) const {
        return data_ <= rhs;
    }
};

struct Ipv4Network : public details::Wrap<Ipv4Network, ::mesa_ipv4_network_t> {
    /*lint -save -e1008 */
    Ipv4Network() = default;
    /*lint -restore */
    Ipv4Network(const Ipv4Network &rhs) { data_ = rhs.as_api_type(); }
    explicit Ipv4Network(const ::mesa_ipv4_network_t &rhs) { data_ = rhs; }

    Ipv4Network(const Ipv4Address &addr, unsigned prefix_size) {
        data_.address = addr.as_api_type();
        data_.prefix_size = prefix_size;
    }

    Ipv4Network(const mesa_ipv4_t &addr, unsigned prefix_size) {
        data_.address = addr;
        data_.prefix_size = prefix_size;
    }

    Ipv4Network& operator=(const Ipv4Network &rhs) {
        data_ = rhs.as_api_type();
        return *this;
    }

    Ipv4Network& operator=(const ::mesa_ipv4_network_t &rhs) {
        data_ = rhs;
        return *this;
    }

    unsigned prefix_size() const { return data_.prefix_size; }
    void prefix_size(unsigned p) { data_.prefix_size = p; }

    Ipv4Address address() const { return Ipv4Address(data_.address); }
    void address(Ipv4Address a) { data_.address = a.as_api_type(); }

    bool equal_to(const ::mesa_ipv4_network_t &rhs) const {
        return data_.address == rhs.address &&
               data_.prefix_size == rhs.prefix_size;
    }

    bool less_than(const ::mesa_ipv4_network_t &rhs) const {
        if (data_.address != rhs.address)
            return data_.address < rhs.address;
        return data_.prefix_size < rhs.prefix_size;
    }

    bool less_or_equal_than(const ::mesa_ipv4_network_t &rhs) const {
        if (data_.address != rhs.address)
            return data_.address < rhs.address;
        return data_.prefix_size <= rhs.prefix_size;
    }

    void clear() {
        data_.address = 0;
        data_.prefix_size = 0;
    }
};

struct Ipv6Address : public details::Wrap<Ipv6Address, ::mesa_ipv6_t> {
    /*lint -save -e1008 */
    Ipv6Address() = default;
    /*lint -restore */
    Ipv6Address(const Ipv6Address &rhs) { data_ = rhs.as_api_type(); }
    explicit Ipv6Address(const ::mesa_ipv6_t &rhs) { data_ = rhs; }

    Ipv6Address& operator=(const Ipv6Address &rhs) {
        data_ = rhs.as_api_type();
        return *this;
    }

    Ipv6Address& operator=(const ::mesa_ipv6_t &rhs) {
        data_ = rhs;
        return *this;
    }

    bool equal_to(const ::mesa_ipv6_t &rhs) const;
    bool less_than(const ::mesa_ipv6_t &rhs) const;
    bool less_or_equal_than(const ::mesa_ipv6_t &rhs) const;
};

struct Ipv6Network : public details::Wrap<Ipv6Network, ::mesa_ipv6_network_t> {
    /*lint -save -e1008 */
    Ipv6Network() = default;
    /*lint -restore */
    Ipv6Network(const Ipv6Network &rhs) { data_ = rhs.as_api_type(); }
    explicit Ipv6Network(const ::mesa_ipv6_network_t &rhs) { data_ = rhs; }

    Ipv6Network(const Ipv6Address &addr, unsigned prefix) {
        data_.address = addr.as_api_type();
        data_.prefix_size = prefix;
    }

    Ipv6Network& operator=(const Ipv6Network &rhs) {
        data_ = rhs.as_api_type();
        return *this;
    }

    Ipv6Network& operator=(const ::mesa_ipv6_network_t &rhs) {
        data_ = rhs;
        return *this;
    }

    unsigned prefix_size() const { return data_.prefix_size; }
    void prefix_size(unsigned p) { data_.prefix_size = p; }

    Ipv6Address address() const { return Ipv6Address(data_.address); }
    void address(Ipv6Address a) { data_.address = a.as_api_type(); }

    bool equal_to(const ::mesa_ipv6_network_t &rhs) const;
    bool less_than(const ::mesa_ipv6_network_t &rhs) const;
    bool less_or_equal_than(const ::mesa_ipv6_network_t &rhs) const;
};

struct DomainName : public details::Wrap<DomainName, ::vtss_domain_name_t> {
    DomainName() = default;
    DomainName(const DomainName &rhs) { data_ = rhs.as_api_type(); }
    explicit DomainName(const ::vtss_domain_name_t &rhs) { data_ = rhs; }

    DomainName& operator=(const DomainName &rhs) {
        data_ = rhs.as_api_type();
        return *this;
    }

    DomainName& operator=(const ::vtss_domain_name_t &rhs) {
        data_ = rhs;
        return *this;
    }

    bool equal_to(const ::vtss_domain_name_t &rhs) const;
    bool less_than(const ::vtss_domain_name_t &rhs) const;
    bool less_or_equal_than(const ::vtss_domain_name_t &rhs) const;
};

struct IpAddress : public details::Wrap<IpAddress, ::mesa_ip_addr_t> {
    // Constructions
    IpAddress() { data_ = {MESA_IP_TYPE_NONE}; }
    IpAddress(const IpAddress &rhs) { data_ = rhs.as_api_type(); }
    explicit IpAddress(const ::mesa_ip_addr_t &rhs) { data_ = rhs; }

    IpAddress& operator=(const IpAddress &rhs) {
        data_ = rhs.as_api_type();
        return *this;
    }

    IpAddress& operator=(const ::mesa_ip_addr_t &rhs) {
        data_ = rhs;
        return *this;
    }

    bool is_none() const { return data_.type == MESA_IP_TYPE_NONE; }
    bool is_ipv4() const { return data_.type == MESA_IP_TYPE_IPV4; }
    bool is_ipv6() const { return data_.type == MESA_IP_TYPE_IPV6; }

    Ipv4Address as_ipv4() const { return Ipv4Address(data_.addr.ipv4); }
    Ipv6Address as_ipv6() const { return Ipv6Address(data_.addr.ipv6); }

    // From ipv4 type
    explicit IpAddress(const Ipv4Address &rhs) {
        data_.type = MESA_IP_TYPE_IPV4;
        data_.addr.ipv4 = rhs.as_api_type();
    }

    explicit IpAddress(const ::mesa_ipv4_t &rhs) {
        data_.type = MESA_IP_TYPE_IPV4;
        data_.addr.ipv4 = rhs;
    }

    IpAddress& operator=(const Ipv4Address &rhs) {
        data_.type = MESA_IP_TYPE_IPV4;
        data_.addr.ipv4 = rhs.as_api_type();
        return *this;
    }

    IpAddress& operator=(const ::mesa_ipv4_t &rhs) {
        data_.type = MESA_IP_TYPE_IPV4;
        data_.addr.ipv4 = rhs;
        return *this;
    }


    // From ipv6 type
    explicit IpAddress(const Ipv6Address &rhs) {
        data_.type = MESA_IP_TYPE_IPV6;
        data_.addr.ipv6 = rhs.as_api_type();
    }

    explicit IpAddress(const ::mesa_ipv6_t &rhs) {
        data_.type = MESA_IP_TYPE_IPV6;
        data_.addr.ipv6 = rhs;
    }

    IpAddress& operator=(const Ipv6Address &rhs) {
        data_.type = MESA_IP_TYPE_IPV6;
        data_.addr.ipv6 = rhs.as_api_type();
        return *this;
    }

    IpAddress& operator=(const ::mesa_ipv6_t &rhs) {
        data_.type = MESA_IP_TYPE_IPV6;
        data_.addr.ipv6 = rhs;
        return *this;
    }

    bool string_parse(const char *&b, const char *e);
    bool equal_to(const ::mesa_ip_addr_t &rhs) const;
    bool less_than(const ::mesa_ip_addr_t &rhs) const;
    bool less_or_equal_than(const ::mesa_ip_addr_t &rhs) const;
};

struct InetAddress : public details::Wrap<InetAddress, ::vtss_inet_address_t> {
    // Constructions
    InetAddress() { data_ = {VTSS_INET_ADDRESS_TYPE_NONE}; }
    InetAddress(const InetAddress &rhs) { data_ = rhs.as_api_type(); }
    explicit InetAddress(const ::vtss_inet_address_t &rhs) { data_ = rhs; }

    InetAddress& operator=(const InetAddress &rhs) {
        data_ = rhs.as_api_type();
        return *this;
    }

    InetAddress& operator=(const ::vtss_inet_address_t &rhs) {
        data_ = rhs;
        return *this;
    }

    vtss_inet_address_type_t type() const { return data_.type; }
    bool is_none() const { return data_.type == VTSS_INET_ADDRESS_TYPE_NONE; }
    bool is_ipv4() const { return data_.type == VTSS_INET_ADDRESS_TYPE_IPV4; }
    bool is_ipv6() const { return data_.type == VTSS_INET_ADDRESS_TYPE_IPV6; }
    bool is_dns() const { return data_.type == VTSS_INET_ADDRESS_TYPE_DNS; }

    Ipv4Address as_ipv4() const { return Ipv4Address(data_.address.ipv4); }
    Ipv6Address as_ipv6() const { return Ipv6Address(data_.address.ipv6); }
    vtss_domain_name_t as_dns() const {
        return data_.address.domain_name;
    }

    void set_type_none() { data_.type = VTSS_INET_ADDRESS_TYPE_NONE; }

    // From ipv4 type
    explicit InetAddress(const Ipv4Address &rhs) {
        data_.type = VTSS_INET_ADDRESS_TYPE_IPV4;
        data_.address.ipv4 = rhs.as_api_type();
    }

    explicit InetAddress(const ::mesa_ipv4_t &rhs) {
        data_.type = VTSS_INET_ADDRESS_TYPE_IPV4;
        data_.address.ipv4 = rhs;
    }

    InetAddress& operator=(const Ipv4Address &rhs) {
        data_.type = VTSS_INET_ADDRESS_TYPE_IPV4;
        data_.address.ipv4 = rhs.as_api_type();
        return *this;
    }

    InetAddress& operator=(const ::mesa_ipv4_t &rhs) {
        data_.type = VTSS_INET_ADDRESS_TYPE_IPV4;
        data_.address.ipv4 = rhs;
        return *this;
    }

    // From ipv6 type
    explicit InetAddress(const Ipv6Address &rhs) {
        data_.type = VTSS_INET_ADDRESS_TYPE_IPV6;
        data_.address.ipv6 = rhs.as_api_type();
    }

    explicit InetAddress(const ::mesa_ipv6_t &rhs) {
        data_.type = VTSS_INET_ADDRESS_TYPE_IPV6;
        data_.address.ipv6 = rhs;
    }

    InetAddress& operator=(const Ipv6Address &rhs) {
        data_.type = VTSS_INET_ADDRESS_TYPE_IPV6;
        data_.address.ipv6 = rhs.as_api_type();
        return *this;
    }

    InetAddress& operator=(const ::mesa_ipv6_t &rhs) {
        data_.type = VTSS_INET_ADDRESS_TYPE_IPV6;
        data_.address.ipv6 = rhs;
        return *this;
    }

    // From string
    explicit InetAddress(const str &rhs) {
        operator=(rhs);
    }

    explicit InetAddress(const char *rhs) {
        operator=(rhs);
    }

    InetAddress& operator=(const str &rhs) {
        const char *b = rhs.begin();
        const char *e = rhs.end();

        if (!string_parse(b, e))
            data_.type = VTSS_INET_ADDRESS_TYPE_NONE;

        return *this;
    }

    InetAddress& operator=(const char *rhs) {
        return operator=(str(rhs));
    }

    bool string_parse(const char *&b, const char *e);
    bool equal_to(const ::vtss_inet_address_t &rhs) const;
    bool less_than(const ::vtss_inet_address_t &rhs) const;
    bool less_or_equal_than(const ::vtss_inet_address_t &rhs) const;
};

struct IpNetwork : public details::Wrap<IpNetwork, ::mesa_ip_network_t> {
    // Constructions
    IpNetwork() { data_ = {{MESA_IP_TYPE_NONE}}; }
    IpNetwork(const IpNetwork &rhs) { data_ = rhs.as_api_type(); }
    explicit IpNetwork(const ::mesa_ip_network_t &rhs) { data_ = rhs; }

    IpNetwork& operator=(const IpNetwork &rhs) {
        data_ = rhs.as_api_type();
        return *this;
    }

    IpNetwork& operator=(const ::mesa_ip_network_t &rhs) {
        data_ = rhs;
        return *this;
    }


    // From ipv4 type
    explicit IpNetwork(const Ipv4Network &rhs) {
        data_.address.type = MESA_IP_TYPE_IPV4;
        data_.address.addr.ipv4 = rhs.as_api_type().address;
        data_.prefix_size = rhs.as_api_type().prefix_size;
    }

    explicit IpNetwork(const ::mesa_ipv4_network_t &rhs) {
        data_.address.type = MESA_IP_TYPE_IPV4;
        data_.address.addr.ipv4 = rhs.address;
        data_.prefix_size = rhs.prefix_size;
    }

    IpNetwork& operator=(const Ipv4Network &rhs) {
        data_.address.type = MESA_IP_TYPE_IPV4;
        data_.address.addr.ipv4 = rhs.as_api_type().address;
        data_.prefix_size = rhs.as_api_type().prefix_size;
        return *this;
    }

    IpNetwork& operator=(const ::mesa_ipv4_network_t &rhs) {
        data_.address.type = MESA_IP_TYPE_IPV4;
        data_.address.addr.ipv4 = rhs.address;
        data_.prefix_size = rhs.prefix_size;
        return *this;
    }


    // From ipv6 type
    explicit IpNetwork(const Ipv6Network &rhs) {
        data_.address.type = MESA_IP_TYPE_IPV6;
        data_.address.addr.ipv6 = rhs.as_api_type().address;
        data_.prefix_size = rhs.as_api_type().prefix_size;
    }

    explicit IpNetwork(const ::mesa_ipv6_network_t &rhs) {
        data_.address.type = MESA_IP_TYPE_IPV6;
        data_.address.addr.ipv6 = rhs.address;
        data_.prefix_size = rhs.prefix_size;
    }

    IpNetwork& operator=(const Ipv6Network &rhs) {
        data_.address.type = MESA_IP_TYPE_IPV6;
        data_.address.addr.ipv6 = rhs.as_api_type().address;
        data_.prefix_size = rhs.as_api_type().prefix_size;
        return *this;
    }

    IpNetwork& operator=(const ::mesa_ipv6_network_t &rhs) {
        data_.address.type = MESA_IP_TYPE_IPV6;
        data_.address.addr.ipv6 = rhs.address;
        data_.prefix_size = rhs.prefix_size;
        return *this;
    }

    unsigned prefix_size() const { return data_.prefix_size; }
    void prefix_size(unsigned p) { data_.prefix_size = p; }

    IpAddress address() const { return IpAddress(data_.address); }
    void address(IpAddress a) { data_.address = a.as_api_type(); }

    bool equal_to(const ::mesa_ip_network_t &rhs) const;
    bool less_than(const ::mesa_ip_network_t &rhs) const;
    bool less_or_equal_than(const ::mesa_ip_network_t &rhs) const;
};


}  // namespace vtss

#endif
