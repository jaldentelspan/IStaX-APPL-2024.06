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

#ifndef _VTSS_APPL_TYPES_HXX_
#define _VTSS_APPL_TYPES_HXX_

#include <vtss/basics/types.hxx>  // Needed for the c++ type wrapper
#include <vtss/basics/stream.hxx>
//#include <vtss_appl_serialize.hxx>
#include "vtss/basics/snmp.hxx"
#include "vtss/basics/expose.hxx"
#include <vtss/appl/interface.h>
#include <vtss/appl/types.h>

namespace vtss {

/**
 * \brief A type for representing list of ports across a stack or standalone
 *
 * The underlying data type is a normal char array which is used as a
 * bit-field. Each port is represented by a bit, and the bit index can be used
 * to derive the given port.
 *
 * The underlying data structure should be considered as a bit sequence of 1024
 * bits, indexed 0-1023. The least significant bit of the first char is bit
 * zero, the second least significant bit of the first char is bit 2, the least
 * significant bit of the second char is bit 9 and so on.
 *
 * The switch with ISID == X owns the bit range: (X - 1) * data_array_size ...
 * X * data_array_size - 1;
 *
 * Bit zero in the this range denotes zero, bit one port one etc.
 *
 * */
class PortListStackable :
    public details::WrapNoLessThan<PortListStackable, ::vtss_port_list_stackable_t> {

  public:
    /** The size of the underlying storage array */
    static constexpr uint32_t data_array_size = 128;

    /** Amount of bits in the underlying data array */
    static constexpr uint32_t data_bit_end = 128 * 8;

    /** The number of bits reserved for each switch */
    static constexpr uint32_t port_chunk_size = 64;

    struct ConstIterator {
        friend class PortListStackable;

        ConstIterator(const ConstIterator &rhs) : parent(rhs.parent),
                idx(rhs.idx), ifindex(rhs.ifindex) { }

        const vtss_ifindex_t &operator*() const { return ifindex; }
        const vtss_ifindex_t *operator->() const { return &ifindex; }

        ConstIterator& operator++() {
            idx ++;
            update_ifindex();
            return *this;
        };

        ConstIterator operator++(int) {
            ConstIterator res(*this);
            idx ++;
            update_ifindex();
            return res;
        }

        bool operator!=(const ConstIterator &rhs) const {
            return parent != rhs.parent || idx != rhs.idx;
        }

        bool operator==(const ConstIterator &rhs) const {
            return parent == rhs.parent && idx == rhs.idx;
        }

      private:
        ConstIterator(const PortListStackable *p, uint32_t i) :
                parent(p), idx(i) {
            update_ifindex();
        }

        void update_ifindex() const;

        const PortListStackable         *parent = nullptr;
        mutable uint32_t        idx = 0;
        mutable vtss_ifindex_t  ifindex = VTSS_IFINDEX_NONE;
    };

    PortListStackable();
    PortListStackable(const PortListStackable &);
    PortListStackable(const ::vtss_port_list_stackable_t &);
    PortListStackable &operator=(const PortListStackable &rhs);
    PortListStackable &operator=(const ::vtss_port_list_stackable_t &rhs);

    void clear_all();

    bool get(const vtss_ifindex_t &i) const;
    bool set(const vtss_ifindex_t &i);
    bool clear(const vtss_ifindex_t &i);

    bool get(vtss_isid_t i, mesa_port_no_t p) const;
    bool set(vtss_isid_t i, mesa_port_no_t p);
    bool clear(vtss_isid_t i, mesa_port_no_t p);

    bool equal_to(const ::vtss_port_list_stackable_t &rhs) const;

    size_t size() const {
        auto i = begin();
        auto e = end();
        size_t c = 0;
        for (; i != e; ++i) ++c;
        return c;
    }

    ConstIterator begin() const { return ConstIterator(this, 0); }
    ConstIterator end() const { return ConstIterator(this, data_bit_end); }

    static uint32_t isid_port_to_index(vtss_isid_t i, mesa_port_no_t p);

  private:
    bool index_get(uint32_t i) const;
    bool index_set(uint32_t i);
    bool index_clear(uint32_t i);
};

ostream &operator<<(ostream &o, const PortListStackable &l);

class VlanList : public details::WrapNoLessThan<VlanList, ::vtss_vlan_list_t> {
  public:
    struct ConstIterator {
        friend class VlanList;

        ConstIterator(const ConstIterator &rhs) : parent(rhs.parent),
                idx(rhs.idx) { }

        const mesa_vid_t operator*() const { return idx; }

        ConstIterator& operator++() {
            idx ++;
            update_ifindex();
            return *this;
        };

        ConstIterator operator++(int) {
            ConstIterator res(*this);
            idx ++;
            update_ifindex();
            return res;
        }

        bool operator!=(const ConstIterator &rhs) const {
            return parent != rhs.parent || idx != rhs.idx;
        }

        bool operator==(const ConstIterator &rhs) const {
            return parent == rhs.parent && idx == rhs.idx;
        }

      private:
        ConstIterator(const VlanList *p, uint32_t i) :
                parent(p), idx(i) {
            update_ifindex();
        }

        void update_ifindex() const;

        const VlanList *parent = nullptr;
        mutable uint32_t idx = 0;
    };

    VlanList();
    VlanList(const VlanList&);
    VlanList(const ::vtss_vlan_list_t &);
    VlanList &operator=(const VlanList &rhs);
    VlanList &operator=(const ::vtss_vlan_list_t &rhs);

    void clear_all();
    bool get(const mesa_vid_t &i) const;
    bool set(const mesa_vid_t &i);
    bool clear(const mesa_vid_t &i);

    size_t size() const {
        auto i = begin();
        auto e = end();
        size_t c = 0;
        for (; i != e; ++i) ++c;
        return c;
    }

    bool equal_to(const ::vtss_vlan_list_t &rhs) const;

    ConstIterator begin() const { return ConstIterator(this, 0); }
    ConstIterator end() const { return ConstIterator(this, 4096); }
};

ostream &operator<<(ostream &o, const VlanList &l);

#ifdef VTSS_SW_OPTION_SNMP
void serialize(vtss::expose::snmp::GetHandler &h, PortListStackable &s);
void serialize(vtss::expose::snmp::SetHandler &h, PortListStackable &s);
void serialize(vtss::expose::snmp::Reflector &h, PortListStackable &);
#endif  // VTSS_SW_OPTION_SNMP
#ifdef VTSS_SW_OPTION_JSON_RPC
void serialize(expose::json::Exporter &e, PortListStackable &s);
void serialize(expose::json::Loader &e, PortListStackable &s);
void serialize(expose::json::HandlerReflector &e, PortListStackable &s);
#endif  // VTSS_SW_OPTION_JSON_RPC
};  // namespace vtss

#ifdef VTSS_SW_OPTION_SNMP
// Must be in global namespace due to ADL rules
void serialize(vtss::expose::snmp::GetHandler &h, vtss_port_list_stackable_t &s);
void serialize(vtss::expose::snmp::SetHandler &h, vtss_port_list_stackable_t &s);
void serialize(vtss::expose::snmp::Reflector &h, vtss_port_list_stackable_t &);
#endif  // VTSS_SW_OPTION_SNMP

#endif  // _VTSS_APPL_TYPES_HXX_
