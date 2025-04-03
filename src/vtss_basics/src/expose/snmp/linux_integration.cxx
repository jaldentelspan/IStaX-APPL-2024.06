/* *****************************************************************************
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
 **************************************************************************** */

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include <stdlib.h>
#include "vtss/basics/snmp.hxx"
#include "vtss/basics/expose/snmp/utils.hxx"
#include "vtss/basics/expose/snmp/handlers/linux/walkcapture_linux.hxx"
#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/formatting_tags.hxx"

#if defined(VTSS_SW_OPTION_SNMP)
#include "mibContextTable.h"
#endif

#include "vtss_os_wrapper_snmp.h"
extern "C" {
u_char *VTSS_SNMP_default_handler(struct variable *vp, vtss_oid *name,
                                  size_t *length, int exact, size_t *var_len,
                                  WriteMethod **write_method);
}

namespace vtss {
namespace expose {
namespace snmp {

// HACK HACK HACK
struct snmp_variable_base {
    unsigned char magic;
    char type;
    unsigned short acl;
    void *findVar;
    unsigned char namelen;
    // var length array
};

struct snmp_variable_var_length {
    unsigned char magic;
    char type;
    unsigned short acl;
    void *findVar;
    unsigned char namelen;
    unsigned long name[256];
};

void WalkCntHandler::argument_begin(size_t n, ArgumentType::E type) {}

void WalkCntHandler::type_def(const char *name, AsnType::E type) {
    if (trap_mode_) return;
    if (current_leaf.get()) current_leaf->type = type;
}

void WalkCntHandler::type_ref(const char *name, const char *from,
                              AsnType::E type) {
    if (trap_mode_) return;
    if (current_leaf.get()) current_leaf->type = type;
}

void WalkCntHandler::type_range(std::string s) {
    if (trap_mode_) return;
}

void WalkCntHandler::type_enum(const char *name, const char *desc,
                               const vtss_enum_descriptor_t *d) {
    if (trap_mode_) return;
    if (current_leaf.get()) current_leaf->type = AsnType::Integer;
}

void WalkCntHandler::node_begin(const OidElement &n, const char *depends_on) {
    if (trap_mode_) return;
    if (current_depth != 0)  // skip first oid element
        oid_seq_.push(n.numeric_);

    current_depth++;
}

void WalkCntHandler::node_begin(const OidElement &n, MaxAccess::E a,
                                const char *depends_on) {
    if (trap_mode_) return;
    if (a == MaxAccess::ReadOnly)
        acl_ = 1;
    else if (a == MaxAccess::ReadWrite)
        acl_ = 2;
    node_begin(n, depends_on);
}

void WalkCntHandler::node_end(const char *depends_on) {
    if (trap_mode_) return;
    current_depth--;
    acl_ = 0;

    if (current_depth != 0)  // skip first oid element
        oid_seq_.pop();
}

void WalkCntHandler::table_node_begin(const OidElement &n, MaxAccess::E a,
                                      const char *table_desc,
                                      const char *index_desc,
                                      const char *depends_on) {
    if (trap_mode_) return;
    oid_seq_.push(n.numeric_);
    oid_seq_.push(1);

    if (a == MaxAccess::ReadOnly)
        acl_ = 1;
    else if (a == MaxAccess::ReadWrite)
        acl_ = 2;

    current_depth += 2;
}

void WalkCntHandler::table_node_end(const char *depends_on) {
    if (trap_mode_) return;
    current_depth -= 2;

    oid_seq_.pop();
    oid_seq_.pop();
    acl_ = 0;
}

size_t WalkCntHandler::WalkCntHandler::row_size_byte() const {
    size_t s =
            sizeof(snmp_variable_base) + (sizeof(unsigned long) * max_depth_);
    if (s % sizeof(void *)) s = ((s / sizeof(void *)) + 1) * sizeof(void *);
    return s;
}

size_t WalkCntHandler::WalkCntHandler::row_size_words() const {
    return row_size_byte() / sizeof(void *);
}

size_t WalkCntHandler::WalkCntHandler::table_size() const {
    return row_size_byte() * leaf_cnt_;
}

void WalkCntHandler::add_leaf_(const char *name, const char *desc, int oid,
                               const char *range, const char *depends, const char *status) {
    if (trap_mode_) return;
    oid_seq_.push(oid);
    leaf_cnt_++;
    max_depth_ = max(oid_seq_.length(), max_depth_);
    //    auto l = std::make_shared<Leaf>(Leaf{oid_seq_, 0, acl_, leaf_cnt_});
    //    leaf_cnt_ does not appear to be used for anything
    auto l = std::make_shared<Leaf>(Leaf{oid_seq_, 0, acl_, 0});
    leafs.push_back(l);
    current_leaf = l;
    oid_seq_.pop();
}

void WalkCntHandler::build_linux_registry_entry() {
    if (data_) VTSS_BASICS_FREE(data_);
    VTSS_BASICS_TRACE(DEBUG) << "Alloc " << leaf_cnt_ << " x "
                             << row_size_byte() << " ("
                             << (leaf_cnt_ * row_size_byte()) << ")";
    data_ = static_cast<uint8_t *>(
            VTSS_BASICS_CALLOC(leaf_cnt_, row_size_byte()));
    VTSS_ASSERT(data_);

    uint32_t cnt = 0;
    for (auto l : leafs) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
        struct variable *p = reinterpret_cast<struct variable *>(
                static_cast<char *>(data_) + (row_size_byte() * (cnt++)));
#pragma GCC diagnostic pop
        p->magic = l->magic;
        p->type = l->type;
        p->acl = l->acl;
        p->findVar = VTSS_SNMP_default_handler;
        p->namelen = l->oid.copy_to(p->name);
        VTSS_BASICS_TRACE(DEBUG) << "Adding leaf: " << l->oid << " type: 0x"
                                 << hex(l->type) << " acl: 0x" << hex(l->acl);
    }
}

void WalkCntHandler::add_capability(const char *name) {}

namespace Err {
enum E {
    noerror = SNMP_ERR_NOERROR,
    toobig = SNMP_ERR_TOOBIG,
    nosuchname = SNMP_ERR_NOSUCHNAME,
    badvalue = SNMP_ERR_BADVALUE,
    readonly = SNMP_ERR_READONLY,
    generr = SNMP_ERR_GENERR,
    noaccess = SNMP_ERR_NOACCESS,
    wrongtype = SNMP_ERR_WRONGTYPE,
    wronglength = SNMP_ERR_WRONGLENGTH,
    wrongencoding = SNMP_ERR_WRONGENCODING,
    wrongvalue = SNMP_ERR_WRONGVALUE,
    nocreation = SNMP_ERR_NOCREATION,
    inconsistentvalue = SNMP_ERR_INCONSISTENTVALUE,
    resourceunavailable = SNMP_ERR_RESOURCEUNAVAILABLE,
    commitfailed = SNMP_ERR_COMMITFAILED,
    undofailed = SNMP_ERR_UNDOFAILED,
    authorizationerror = SNMP_ERR_AUTHORIZATIONERROR,
    notwritable = SNMP_ERR_NOTWRITABLE,
};

ostream &operator<<(ostream &o, const E e) {
#define CASE(X)  \
    case X:      \
        o << #X; \
        break
    switch (e) {
        CASE(noerror);
        CASE(toobig);
        CASE(nosuchname);
        CASE(badvalue);
        CASE(readonly);
        CASE(generr);
        CASE(noaccess);
        CASE(wrongtype);
        CASE(wronglength);
        CASE(wrongencoding);
        CASE(wrongvalue);
        CASE(nocreation);
        CASE(inconsistentvalue);
        CASE(resourceunavailable);
        CASE(commitfailed);
        CASE(undofailed);
        CASE(authorizationerror);
        CASE(notwritable);
    default:
        o << "<unknown(" << static_cast<int32_t>(e) << ")>";
        break;
    }
#undef CASE
    return o;
}
}  // namespace Err

VtssSnmpRegMibList reg_mib_list;

void vtss_snmp_reg_mib_tree() {
    for (auto mib = vtss::expose::snmp::reg_mib_list.begin();
         mib != vtss::expose::snmp::reg_mib_list.end(); mib++) {
        mib->execute();
    }
}

void VtssSnmpRegMibBase::execute() {
    WalkCntHandler w;
    struct variable *var;

    // We do net want this visitor to include objects that are not avialable due
    // to the lack of capabilities.
    w.device_specific_ = true;

    VTSS_BASICS_TRACE(NOISE) << "Before serialize";

    // w.prepare_pass_1();
    do_serialize(w);

    w.build_linux_registry_entry();
    VTSS_BASICS_TRACE(INFO) << mibName() << "(" << base_oid << ")"
                            << " Row size: " << w.row_size_byte()
                            << " leaf cnt: " << w.leaf_cnt();

    var = static_cast<struct variable *>(w.data_release());
    if (register_mib(mibName(), var, w.row_size_byte(), w.leaf_cnt(),
                     (oid *)(base_oid.oids),
                     base_oid.valid) != MIB_REGISTERED_OK) {
        VTSS_BASICS_TRACE(INFO) << "Failed to registerer mib: " << mibName();
        // When snmp is disabled entries are not removed from fromt the SNMP
        // MIB context table. When re-enabling SNMP a new attempt will be made
        // to
        // add the MIBs to the MIB context table which will then fail.
        // Therefore,
        // only debug message. See Bz#14020
    } else {
        VTSS_BASICS_TRACE(INFO) << "Registerer mib: " << mibName();
    }

#if defined(VTSS_SW_OPTION_SNMP)
    char mib_name[MIBCONTEXTTABLE_STR_LEN_MAX + 1];
    char *p_mib_name = mib_name;
    p_mib_name += sprintf(mib_name, "%s-%s : ", TO_STR(MIBPREFIX), MIB_NAME());
    sprintf(p_mib_name, "%s", TO_STR(MIBPREFIX));
    for (; *p_mib_name != 0; p_mib_name++) {
        *p_mib_name = tolower(*p_mib_name);
    }
    sprintf(p_mib_name, "%s", mibName());
    *p_mib_name = toupper(*p_mib_name);

    mibContextTable_register((oid *)(base_oid.oids), base_oid.valid, mib_name);
    VTSS_BASICS_TRACE(INFO) << "Mib added to mibContextTable_register";
#endif
}

void VtssSnmpRegMibBase::getOidList(const NamespaceNode *n,
                                    OidSequence &oid_list) {
    if (n == nullptr) return;

    getOidList(n->parent(), oid_list);
    oid_list.push(n->element().numeric_);
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

// hack hack hack... luckily the linux snmp agent is single threaded...
// static int SNMP_DEFAULT_RETURN_VALUE = 0;
static vtss::expose::snmp::OidSequence SNMP_oid_index;
static vtss::expose::snmp::OidSequence SNMP_oid_name;
static vtss::expose::snmp::OidSequence SNMP_oid_name_and_index;
static vtss::expose::snmp::OidSequence SNMP_seq_in;
static vtss::expose::snmp::Action::E last_action = vtss::expose::snmp::Action::reserve1;

static int VTSS_SNMP_write_handler_reserve1(u_char *var_val,
                                            u_char var_val_type,
                                            size_t var_val_len) {
    using namespace vtss;
    using namespace vtss::expose::snmp;
    AsnType t;

    if (!t.set(var_val_type)) {
        VTSS_BASICS_TRACE(DEBUG) << "Unsupported type: " << HEX(var_val_type)
                                 << " value: " << Binary(var_val, var_val_len)
                                 << " o: " << SNMP_oid_name;
        return Err::wrongtype;
    }

    SetHandler set(SNMP_oid_name, SNMP_oid_index, var_val, var_val_len, t);
    serialize(set, vtss::expose::snmp::vtss_snmp_globals.iso);

    VTSS_BASICS_TRACE(INFO) << "Snmp write handler reserve1: " << SNMP_oid_name
                            << " value: " << Binary(var_val, var_val_len)
                            << " type: " << t << " Result: " << set.error_code_;

    return set.error_code_;
}

static int VTSS_SNMP_write_handler_free() {
    using namespace vtss;
    using namespace vtss::expose::snmp;

    for (int i = 0; i < VTSS_SNMP_SET_CACHE_SIZE; ++i) {
        if (!set_cache[i].iterator.get()) continue;

        VTSS_BASICS_TRACE(INFO) << "Reset: " << set_cache[i].oid;
        set_cache[i].iterator.reset();
    }

    return Err::noerror;
}

static int VTSS_SNMP_write_handler_action() {
    using namespace vtss;
    using namespace vtss::expose::snmp;

    for (int i = 0; i < VTSS_SNMP_SET_CACHE_SIZE; ++i) {
        // skip null pointers
        if (!set_cache[i].iterator.get()) continue;

        // call the set method on the iterator
        VTSS_BASICS_TRACE(DEBUG) << "Set: " << set_cache[i].oid;
        mesa_rc rc = set_cache[i].iterator->set();
        if (rc != MESA_RC_OK) {
            VTSS_BASICS_TRACE(WARNING)
                    << "Set failed: " << set_cache[i].oid << " RC=" << rc
                    << " PTR=" << set_cache[i].iterator->set_ptr__();
            return Err::commitfailed;

        } else {
            VTSS_BASICS_TRACE(INFO)
                    << "Set success: " << set_cache[i].oid << " RC=" << rc
                    << " PTR=" << set_cache[i].iterator->set_ptr__();
        }
    }

    return Err::noerror;
}

static int VTSS_SNMP_write_handler_undo() {
    using namespace vtss;
    using namespace vtss::expose::snmp;

    Err::E e = Err::noerror;

    for (int i = VTSS_SNMP_SET_CACHE_SIZE - 1; i >= 0; --i) {
        if (!set_cache[i].iterator.get()) continue;

        VTSS_BASICS_TRACE(DEBUG) << "Undo: " << set_cache[i].oid;
        if (set_cache[i].iterator->undo() != MESA_RC_OK) {
            VTSS_BASICS_TRACE(WARNING) << "Undo failed: " << set_cache[i].oid;
            e = Err::undofailed;
        }
        set_cache[i].iterator.reset();
    }

    return e;
}
int VTSS_SNMP_default_write_handler(int action, u_char *var_val,
                                    u_char var_val_type, size_t var_val_len,
                                    u_char *statP, oid *name, size_t name_len) {
    using namespace vtss;
    using namespace vtss::expose::snmp;

    /*
     * Following is the state-machine implemented in the Linux SNMP agent. The
     * Linux agent will in each step call the VTSS_SNMP_default_write_handler
     * for every var-bindings present in the SNMP PDU. If processing of a single
     * var-binding is failing it will take the error path in the state machine.
     *
     *  +--------------+
     *  |     GET      |
     *  +--------------+
     *         |
     *         | OK and RW
     *         v
     *  +--------------+
     *  |  RESERVED_1  |----------+
     *  +--------------+          |
     *         |                  |
     *         | OK               |
     *         v                  v
     *  +--------------+  ERR   +--------------+
     *  |  RESERVED_2  |------->|     FREE     |
     *  +--------------+        +--------------+
     *         |
     *         | OK
     *         v
     *  +--------------+  ERR   +--------------+
     *  |    ACTION    |------->|     UNDO     |
     *  +--------------+        +--------------+
     *         |
     *         | OK
     *         v
     *  +--------------+
     *  |    COMMIT    |
     *  +--------------+
     *
     * */


    Action::E a = static_cast<Action::E>(action);
    static u_char dummy_data;

    if (!var_val) {
        VTSS_BASICS_TRACE(DEBUG) << "Null-ptr: " << a
                                 << " o: " << SNMP_oid_name;
        var_val = &dummy_data;
        var_val_len = 0;
    }

    VTSS_BASICS_TRACE(NOISE) << "Action: " << a
                             << " last action: " << last_action;

    switch (a) {
    case Action::reserve1:
        // Allow repeated calls when handling bulk requests
        last_action = a;
        return VTSS_SNMP_write_handler_reserve1(var_val, var_val_type,
                                                var_val_len);

    case Action::action:
        // No repeation
        if (last_action != a) {
            last_action = a;
            return VTSS_SNMP_write_handler_action();
        }
        break;

    case Action::commit:  // Yes free and commit must do the same thing
    case Action::free:
        // No repeation
        if (last_action != Action::free) {
            last_action = Action::free;
            return VTSS_SNMP_write_handler_free();
        }
        break;

    case Action::undo:
        // No repeation
        if (last_action != a) {
            last_action = a;
            return VTSS_SNMP_write_handler_undo();
        }
        break;

    default:  // Dont care
        last_action = a;
        break;
    }

    return Err::noerror;
}

u_char *VTSS_SNMP_default_handler(struct variable *vp, oid *name,
                                  size_t *length, int exact, size_t *var_len,
                                  WriteMethod **write_method) {
    using namespace vtss;
    using namespace vtss::expose::snmp;
    SNMP_oid_index.clear();
    SNMP_oid_name_and_index.clear();
    SNMP_oid_name.copy_from(reinterpret_cast<oid *>(vp->name),
                            static_cast<uint32_t>(vp->namelen));
    {
        OidSequence seq_in(name, *length);

        if (seq_in.isSubtreeOf(SNMP_oid_name))
            SNMP_oid_index = seq_in % SNMP_oid_name;

        VTSS_BASICS_TRACE(INFO) << "Snmp handler: in: " << seq_in << " "
                                << (exact ? "(exact)" : "(next)")
                                << " var: " << SNMP_oid_name << "@"
                                << SNMP_oid_index;
    }

    // Setup defaults in-case nothing is found
    u_char *res = 0;
    *write_method = 0;
    *var_len = 0;
    *length = 0;

    if (exact) {
        GetHandler get(SNMP_oid_name, SNMP_oid_index, false);
        serialize(get, vtss::expose::snmp::vtss_snmp_globals.iso);

        if (get.state() != vtss::expose::snmp::HandlerState::DONE) {
            VTSS_BASICS_TRACE(INFO) << "Could not complete: " << SNMP_oid_name
                                    << "@" << SNMP_oid_index << " -> "
                                    << get.state();
            return 0;
        }

        SNMP_oid_name_and_index = SNMP_oid_name;
        SNMP_oid_name_and_index.push(SNMP_oid_index);
        *length = SNMP_oid_name_and_index.copy_to(name);
        *var_len = get.length;
        res = get.value;

        if (get.rw) *write_method = VTSS_SNMP_default_write_handler;

    } else {
        GetHandler getnext(SNMP_oid_name, SNMP_oid_index, true);
        serialize(getnext, vtss::expose::snmp::vtss_snmp_globals.iso);

        if (getnext.state() != vtss::expose::snmp::HandlerState::DONE) {
            VTSS_BASICS_TRACE(INFO) << "Could not complete: " << SNMP_oid_name
                                    << "@" << SNMP_oid_index << " -> "
                                    << getnext.state();
            return 0;
        }

        SNMP_oid_name_and_index = getnext.oid_seq_out();
        *length = SNMP_oid_name_and_index.copy_to(name);
        *var_len = getnext.length;
        res = getnext.value;

        if (getnext.rw) *write_method = VTSS_SNMP_default_write_handler;
    }

    const char *acl_s;
    OidSequence seq_o(name,(*length));
    AsnType t((AsnType::E)vp->type);
    if (*write_method)
        acl_s = "(rw)";
    else
        acl_s = "(ro)";

    VTSS_BASICS_TRACE(INFO) << "Returning entry: " << seq_o << " " << acl_s
                            << " value: " << Binary(res, *var_len)
                            << " type: " << t;

    return res;
}
