/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef INCLUDE_VTSS_BASICS_API_TYPES_H_
#define INCLUDE_VTSS_BASICS_API_TYPES_H_

#include "vtss/basics/config.h"

#if defined(VTSS_USE_API_HEADERS)
#include "microchip/ethernet/switch/api/types.h"
#include "vtss/appl/types.h"

#else  // defined(VTSS_USE_API_HEADERS)
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t             i8;   /**<  8-bit signed */
typedef int16_t            i16;  /**< 16-bit signed */
typedef int32_t            i32;  /**< 32-bit signed */
typedef int64_t            i64;  /**< 64-bit signed */

typedef uint8_t            u8;   /**<  8-bit unsigned */
typedef uint16_t           u16;  /**< 16-bit unsigned */
typedef uint32_t           u32;  /**< 32-bit unsigned */
typedef uint64_t           u64;  /**< 64-bit unsigned */

typedef uint8_t            BOOL; /**< Boolean implemented as 8-bit unsigned */

typedef struct {
    /** Internal data used to store the interface index. */
    uint32_t private_ifindex_data_do_not_use_directly;
} vtss_ifindex_t;

/** Print a ifindex using printf
 * \param X [IN] Interface index
 */
#define VTSS_IFINDEX_PRINTF_ARG(X) (X).private_ifindex_data_do_not_use_directly

inline bool operator<(const vtss_ifindex_t &lhs, const vtss_ifindex_t &rhs) {
    return lhs.private_ifindex_data_do_not_use_directly <
           rhs.private_ifindex_data_do_not_use_directly;
}

inline bool operator!=(const vtss_ifindex_t &lhs, const vtss_ifindex_t &rhs) {
    return lhs.private_ifindex_data_do_not_use_directly !=
           rhs.private_ifindex_data_do_not_use_directly;
}

inline bool operator==(const vtss_ifindex_t &lhs, const vtss_ifindex_t &rhs) {
    return lhs.private_ifindex_data_do_not_use_directly ==
           rhs.private_ifindex_data_do_not_use_directly;
}

typedef int mesa_rc;

enum {
    MESA_RC_OK         =  0,  /**< Success */
    MESA_RC_ERROR      = -1,  /**< Unspecified error */
    MESA_RC_INV_STATE  = -2,  /**< Invalid state for operation */
    MESA_RC_INCOMPLETE = -3,  /**< Incomplete result */
};

typedef uint32_t mesa_port_no_t;
typedef uint16_t mesa_etype_t;
typedef uint16_t mesa_vid_t;

typedef struct {
    uint8_t addr[6];
} mesa_mac_t;

typedef uint32_t mesa_ipv4_t;

typedef struct {
    uint8_t addr[16];
} mesa_ipv6_t;

typedef enum {
    MESA_IP_TYPE_NONE = 0,
    MESA_IP_TYPE_IPV4 = 1,
    MESA_IP_TYPE_IPV6 = 2
} mesa_ip_type_t;

typedef struct {
    mesa_ip_type_t  type;
    union {
        mesa_ipv4_t ipv4;
        mesa_ipv6_t ipv6;
    } addr;
} mesa_ip_addr_t;

typedef struct {
    mesa_ipv4_t   address;
    uint32_t      prefix_size;
} mesa_ipv4_network_t;

typedef struct {
    mesa_ipv6_t  address;
    uint32_t     prefix_size;
} mesa_ipv6_network_t;

typedef struct {
    mesa_ip_addr_t     address;
    uint32_t        prefix_size;
} mesa_ip_network_t;

typedef struct {
    char name[255];
} vtss_domain_name_t;

typedef enum {
    VTSS_INET_ADDRESS_TYPE_NONE = 0,
    VTSS_INET_ADDRESS_TYPE_IPV4 = 1,
    VTSS_INET_ADDRESS_TYPE_IPV6 = 2,
    VTSS_INET_ADDRESS_TYPE_DNS = 3
} vtss_inet_address_type_t;

typedef struct {
    vtss_inet_address_type_t type;
    union {
        mesa_ipv4_t ipv4;
        mesa_ipv6_t ipv6;
        vtss_domain_name_t domain_name;
    } address;
} vtss_inet_address_t;

typedef enum
{
    MESA_ROUTING_ENTRY_TYPE_INVALID = 0,
    MESA_ROUTING_ENTRY_TYPE_IPV6_UC = 1,
    MESA_ROUTING_ENTRY_TYPE_IPV4_MC = 2,
    MESA_ROUTING_ENTRY_TYPE_IPV4_UC = 3,
} mesa_routing_entry_type_t;

/** \brief IPv4 unicast routing entry */
typedef struct
{
    mesa_ipv4_network_t network;     /**< Network to route */
    mesa_ipv4_t         destination; /**< IP address of next-hop router.
                                          Zero if local route */
} mesa_ipv4_uc_t;

/** \brief IPv6 routing entry */
typedef struct
{
    mesa_ipv6_network_t network;     /**< Network to route */
    mesa_ipv6_t         destination; /**< IP address of next-hop router.
                                          Zero if local route */
} mesa_ipv6_uc_t;

/** \brief Routing entry */
typedef struct
{
   /** Type of route */
   mesa_routing_entry_type_t type;

   union {
       /** IPv6 unicast route */
       mesa_ipv4_uc_t ipv4_uc;

       /** IPv6 unicast route */
       mesa_ipv6_uc_t ipv6_uc;
   } route; /**< Route */

   /** Link-local addresses needs to specify a egress vlan. */
   mesa_vid_t vlan;
} mesa_routing_entry_t;

#ifdef __cplusplus
}  // extern C
#endif

struct mesa_port_list_t;
struct mesa_port_list_ref {
  public:
    mesa_port_list_ref(const mesa_port_list_ref &) = delete;
    mesa_port_list_ref &operator=(const mesa_port_list_ref &rhs);
    mesa_port_list_ref(mesa_port_list_ref &&rhs)
        : parent_(rhs.parent_), bit_idx(rhs.bit_idx) {}

    mesa_port_list_ref(mesa_port_list_t *p, size_t i)
        : parent_(p), bit_idx(i) {}

    operator bool() const;
    mesa_port_list_ref &operator=(bool b);
    mesa_port_list_ref &operator&=(bool b);
    mesa_port_list_ref &operator|=(bool b);
    mesa_port_list_ref &operator^=(bool b);

  protected:
    mesa_port_list_t *parent_ = nullptr;
    size_t bit_idx = 0;
};

#define MESA_PORT_LIST_ARRAY_SIZE 16
/** \brief Port list */
struct mesa_port_list_t {
    size_t array_size() const { return MESA_PORT_LIST_ARRAY_SIZE; }
    size_t max_size() const { return MESA_PORT_LIST_ARRAY_SIZE * 8; }
    size_t size() const { return 67; /* whatever */ }

    bool get(size_t bit) const {
        size_t idx = bit / 8;
        size_t mod = bit % 8;
        return (_private[idx] >> mod) & 0x1;
    }

    void set(size_t bit, bool val = true) {
        size_t idx = bit / 8;
        size_t mod = bit % 8;
        uint8_t x = 1;
        x <<= mod;

        if (val) {
            _private[idx] |= x;
        } else {
            _private[idx] &= ~x;
        }
    }

    void clr(size_t bit) { set(bit, false); }

    void clear_all() {
        for (size_t i = 0; i < MESA_PORT_LIST_ARRAY_SIZE; ++i) _private[i] = 0;
    }

    void set_all() {
        for (size_t i = 0; i < MESA_PORT_LIST_ARRAY_SIZE; ++i)
            _private[i] = 0xff;
    }

    mesa_port_list_ref operator[](size_t bit) {
        return static_cast<mesa_port_list_ref&&>(mesa_port_list_ref(this, bit));
    }

    const bool operator[](size_t bit) const { return get(bit); }

    uint8_t _private[MESA_PORT_LIST_ARRAY_SIZE] = { 0 };
};

inline bool operator==(const mesa_port_list_t &rhs,
                       const mesa_port_list_t &lhs) {
    for (size_t i = 0; i < MESA_PORT_LIST_ARRAY_SIZE; ++i)
        if (rhs._private[i] != lhs._private[i])
            return false;
    return true;
}

inline bool operator!=(const mesa_port_list_t &rhs,
                       const mesa_port_list_t &lhs) {
    for (size_t i = 0; i < MESA_PORT_LIST_ARRAY_SIZE; ++i)
        if (rhs._private[i] != lhs._private[i])
            return true;
    return false;
}

inline mesa_port_list_ref::operator bool() const {
    return parent_->get(bit_idx);
}

inline mesa_port_list_ref &mesa_port_list_ref::operator=(
        const mesa_port_list_ref &rhs) {
    parent_->set(bit_idx, (bool)rhs);
    return *this;
}

inline mesa_port_list_ref &mesa_port_list_ref::operator=(bool b) {
    parent_->set(bit_idx, b);
    return *this;
}

inline mesa_port_list_ref &mesa_port_list_ref::operator&=(bool b) {
    bool p = parent_->get(bit_idx);
    bool n = p & b;
    parent_->set(bit_idx, n);
    return *this;
}

inline mesa_port_list_ref &mesa_port_list_ref::operator|=(bool b) {
    bool p = parent_->get(bit_idx);
    bool n = p | b;
    parent_->set(bit_idx, n);
    return *this;
}

inline mesa_port_list_ref &mesa_port_list_ref::operator^=(bool b) {
    bool p = parent_->get(bit_idx);
    bool n = p ^ b;
    parent_->set(bit_idx, n);
    return *this;
}

inline BOOL vtss_ipv4_network_equal(const mesa_ipv4_network_t *const a,
                             const mesa_ipv4_network_t *const b) {
    if (a->prefix_size != b->prefix_size) {
        return false;
    }

    return a->address == b->address;
}

inline bool operator<(const mesa_ipv4_network_t &a, const mesa_ipv4_network_t &b) {
    if (a.address != b.address) return a.address < b.address;
    return a.prefix_size < b.prefix_size;
}

inline bool operator==(const mesa_ipv4_network_t &a, const mesa_ipv4_network_t &b) {
    return vtss_ipv4_network_equal(&a, &b);
}

inline bool operator!=(const mesa_ipv4_network_t &a, const mesa_ipv4_network_t &b) {
    return !vtss_ipv4_network_equal(&a, &b);
}

#endif  // defined(VTSS_USE_API_HEADERS)
#endif  // INCLUDE_VTSS_BASICS_API_TYPES_H_
