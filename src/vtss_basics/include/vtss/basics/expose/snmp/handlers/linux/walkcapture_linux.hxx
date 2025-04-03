/*

 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_LINUX_WALKCAPTURE_LINUX_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_LINUX_WALKCAPTURE_LINUX_HXX__

#include "vtss/basics/predefs.hxx"
#if defined(VTSS_OPSYS_LINUX)

// extern "C" {
//#include <ucd-snmp/config.h>
//#include <ucd-snmp/mibincl.h>
// u_char *VTSS_SNMP_default_handler(struct variable *vp,
//                                  oid             *name,
//                                  size_t          *length,
//                                  int              exact,
//                                  size_t          *var_len,
//                                  WriteMethod    **write_method);
//}

#include "memory"
#include "vtss/basics/meta.hxx"
#include "vtss/basics/vector.hxx"
#include "vtss/basics/assert.hxx"
#include "vtss/basics/expose/snmp/types.hxx"

namespace vtss {
namespace expose {
namespace snmp {

class WalkCntHandler : public Reflector {
  public:
    void argument_begin(size_t n, ArgumentType::E type);
    void type_def(const char *name, AsnType::E type);
    void type_ref(const char *name, const char *from, AsnType::E type);
    void type_range(std::string s);
    void type_enum(const char *name, const char *desc,
                   const vtss_enum_descriptor_t *d);
    void node_begin(const OidElement &n, const char *depends_on);
    void node_begin(const OidElement &n, MaxAccess::E a, const char *depends_on);
    void node_end(const char *depends_on);
    void table_node_begin(const OidElement &n, MaxAccess::E a, const char *table_desc,
                          const char *index_desc, const char *depends_on);
    void table_node_end(const char *depends_on);
    void add_leaf_(const char *name, const char *desc, int oid,
                   const char *range, const char *depends, const char *status);
    void add_capability(const char *name);

    void trap_begin(const OidElement &n, TrapType t) { trap_mode_ = true; }
    void trap_end() { trap_mode_ = false; }

    void build_linux_registry_entry();

    size_t row_size_byte() const;
    size_t row_size_words() const;
    size_t table_size() const;

    size_t leaf_cnt() const { return leaf_cnt_; }
    size_t node_cnt() const { return node_cnt_; }
    uint32_t max_depth() const { return max_depth_; }

    void *data_release() {
        void *p = data_;
        data_ = 0;
        return p;
    }

    void add_leaf_(int oid, uint8_t type);
    void leaf_trig();
    void node_trig();

    void *data_ = nullptr;
    size_t leaf_cnt_ = 0;
    size_t node_cnt_ = 0;
    uint32_t max_depth_ = 0;
    uint32_t current_depth = 0;
    OidSequence oid_seq_;

    uint16_t acl_ = 0;
    uint32_t oid_offset_ = 0;
    uint32_t recursive_call = 0;

    struct Leaf {
        OidSequence oid;
        uint8_t type;
        uint16_t acl;
        uint32_t magic;
    };

    typedef std::shared_ptr<Leaf> LeafPtr;
    typedef Vector<LeafPtr> LeafList;

    LeafPtr current_leaf;
    Vector<LeafPtr> leafs;

    bool trap_mode_ = false;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // VTSS_OPSYS_LINUX
#endif  // __VTSS_BASICS_EXPOSE_SNMP_HANDLERS_LINUX_WALKCAPTURE_LINUX_HXX__
