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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_OID_INVENTORY_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_OID_INVENTORY_HXX__

#include <vtss/basics/vector.hxx>
#include <vtss/basics/expose/snmp/types.hxx>

namespace vtss {
namespace expose {
namespace snmp {

struct OidInventory {
    friend ostream& operator<<(ostream &o, const OidInventory &i);
    struct Path {
        enum E {EXACT, NEXT, DONE, END_OF_MIB};
    };

    struct Walkable {
        virtual void leaf(const OidSequence &o) = 0;
    };

    // Useful when performing debug prints in recursive calls
    struct Indent {
        Indent() : i(0) { }
        Indent(const Indent &_i) : i(_i.i + 1) { }
        uint32_t i = 0;
    };

    struct Pair {
        Pair (uint32_t c, uint32_t v) : cnt(c), val(v) { }
        uint32_t cnt;
        uint32_t val;
    };

    // Clear all content
    void clear();

    // Walk the complete Oidinventory
    void walk(Walkable &o);

    // Insert an OID sequence into the inventory
    bool insert(const OidSequence &v);

    // Search the inventory for an existing OID sequence. The "in" parameter may
    // contain both the oid sequence and the key. If a matching leaf is found,
    // the input sequence will be split into oid sequence and key parts.
    // Example, say out inventory contain to leaf: 1.3.6.1.4.1.6603.1.42.1.2,
    // and we will now search "1.3.6.1.4.1.6603.1.42.1.2.10.99.5.1". This will
    // result in oid: 1.3.6.1.4.1.6603.1.42.1.2 key: 10.99.5.1
    bool find_split(const OidSequence &in,
                    OidSequence &out_oid,
                    OidSequence &out_key);

    // Returns the first leaf which is greater than the input sequence
    bool find_next(const OidSequence &in, OidSequence &out_oid);

  private:
    // The internal data structure consist of (count, value) tuples. The count
    // is the number of following pairs which the value is valid for.
    //
    // An example:
    // Assume the oid sequence 1.3.6.1.4.1.6603.1.42.1.1 is stored in this
    // inventory. Then the data vector will contain the following tuple
    // sequence:
    // (10, 1), (9, 3), (8, 6), (7, 1), (6, 4), (5, 1), (4, 6603), (3, 1),
    // (2, 42), (1, 1), (0, 1)
    // The last oid element will always have count==0 (An oid sequence may not
    // be both a node and a leaf).
    //
    // If we assume that the inventory must contain 4 oid sequences:
    //   1.3.6.1.4.1.6603.1.42.1.1
    //   1.3.6.1.4.1.6603.1.42.1.2
    //   1.3.6.1.4.1.6603.1.62.1.1
    //   1.3.6.1.4.1.6603.1.62.1.2
    //
    // The data structure will not store the complete data sequence for all four
    // oid sequences, but instead reuse their common prefixes.
    //
    // What will be stored can be illustrated as follows:
    //   1.3.6.1.4.1.6603.1.42.1.1
    //                          .2
    //                      62.1.1
    //                          .2
    //
    // When spaces are used, the number from an earlier line apply. In this
    // example we have 16 numbers (just count the numbers above), and the first
    // "1" is valid for all the following numbers, which means that it is stored
    // as (15, 1) -- (we have 16 numbers in total, which means that 15 numbers
    // follows the first "1". "42" only applies for the following 3 values which
    // means that "42" is stored as (3, 42).
    //
    // Here is how the complete example is encoded:
    //   (15, 1), (14, 3), (13, 6), (12, 1), (11, 4), (10, 1), (9, 6603),
    //   (8, 1), (3, 42), (1, 1), (0, 1) (0, 2) (3, 62), (1, 1), (0, 1) (0, 2)
    //
    // Or with some white-spaces to make it easier to read:
    //   ..., (9, 6603), (8, 1), (3, 42), (1, 1), (0, 1)
    //                                            (0, 2)
    //                           (3, 62), (1, 1), (0, 1)
    //                                            (0, 2)
    //
    // This data structure is compact and does not waste a lot of memory when
    // many oid sequences are stored, it is fast to search for an exact match or
    // a next value. But it is not very fast to update, but fortunately updates
    // in this inventory are rare.
    Vector<Pair> data;

    bool insert_raw(Vector<Pair>::iterator i,
                    const OidSequence &v,
                    uint32_t offset);

    uint32_t insert_(uint32_t data_idx, const OidSequence &v, uint32_t v_i);
    uint32_t walk_(Walkable &o, uint32_t idx,
                   OidSequence &v, Indent in);

    uint32_t find_next_(const OidSequence &in, OidSequence &out,
                        uint32_t data_i, uint32_t in_i, Path::E &path,
                        Indent indent);
};

ostream& operator<<(ostream &o, OidInventory::Indent i);
ostream& operator<<(ostream &o, OidInventory::Path::E i);
ostream& operator<<(ostream &o, const OidInventory::Pair &v);
ostream& operator<<(ostream &o, const OidInventory &i);

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_OID_INVENTORY_HXX__
