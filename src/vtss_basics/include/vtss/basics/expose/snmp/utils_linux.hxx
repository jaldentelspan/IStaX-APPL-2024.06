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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_UTILS_LINUX_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_UTILS_LINUX_HXX__

#include "vtss/basics/predefs.hxx"
#if defined(VTSS_OPSYS_LINUX)

#include "vtss/basics/intrusive_list.hxx"
#include "vtss/basics/expose/snmp/handlers/linux/walkcapture_linux.hxx"
#include "vtss/basics/stream.hxx"
#include "vtss/basics/expose/snmp/types.hxx"
#include "vtss/basics/expose/snmp/globals.hxx"

// TODO, move out of vtss-basics
namespace vtss {
namespace expose {
namespace snmp {

class VtssSnmpRegMibBase : public intrusive::ListNode {
  public:
    OidSequence base_oid;

    VtssSnmpRegMibBase() {}
    virtual const char *mibName() = 0;
    virtual const char *MIB_NAME() = 0;
    virtual void do_serialize(WalkCntHandler &h) = 0;

    void execute();

  protected:
    void getOidList(const NamespaceNode *n, OidSequence &oid_list);
};

typedef intrusive::List<VtssSnmpRegMibBase> VtssSnmpRegMibList;
extern VtssSnmpRegMibList reg_mib_list;

extern "C" void vtss_snmp_reg_mib_tree();

template <typename T>
class vtss_snmp_reg_mib : public VtssSnmpRegMibBase {
  public:
    vtss_snmp_reg_mib() {}
    T *mib;

    const char *mibName() { return mib->mibName; }
    const char *MIB_NAME() { return mib->MIB_NAME; }
    void do_serialize(WalkCntHandler &h) { serialize(h, *mib); }

    void regMib(T &mib_) {
        vtss::expose::snmp::vtss_snmp_globals.attach_module(mib_);
        mib = &mib_;
        getOidList(mib, base_oid);
        reg_mib_list.push_back(*this);
    }
};

class reg_ns : public VtssSnmpRegMibBase {
  public:
    reg_ns(const char *n, const char *d) : name(n), definition_name(d) {}

    NamespaceNode *ns;
    const char *name;
    const char *definition_name;

    const char *mibName() { return name; }
    const char *MIB_NAME() { return definition_name; }
    void do_serialize(WalkCntHandler &h) {
        serialize(static_cast<Reflector &>(h), *ns);
    }

    void regMib(NamespaceNode &mib_) {
        vtss::expose::snmp::vtss_snmp_globals.attach_module(mib_);
        ns = &mib_;
        getOidList(ns, base_oid);
        reg_mib_list.push_back(*this);
    }
};


}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_UTILS_LINUX_HXX__
#endif  // __VTSS_BASICS_EXPOSE_SNMP_UTILS_LINUX_HXX__
