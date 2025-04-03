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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include <vtss/basics/algorithm.hxx>
#include <vtss/basics/expose/snmp/handlers/trap.hxx>

namespace vtss {
namespace expose {
namespace snmp {

// snmpTrapOID.0 object
static oid SNMP_TRAP_OID[] = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};
static size_t SNMP_TRAP_OID_LEN = 11;

void TrapHandler::add_trap(const Node *noti, const StructBase *obj) {
    add_trap_table(noti, obj, 0);  // A bit hacky... But oid's should never be zero
}

void TrapHandler::add_trap_table(const Node *noti, const StructBase *obj,
                                 vtss_oid event) {
    // Calculate the OID of the Notification
    OidSequence noti_oid_;
    if (event != 0) noti_oid_.push_back(event);
    noti_oid_.push_back(noti->element().numeric());
    const NamespaceNode *p = noti->parent();
    while (p) {
        noti_oid_.push_back(p->element().numeric());
        p = p->parent();
    }
    reverse(noti_oid_.begin(), noti_oid_.end());

    // Calculate the base oid of the varbindings that will go into the TRAP
    oid_base_.clear();
    if (event != 0) oid_base_.push_back(1);  // Table namespace
    oid_base_.push_back(obj->element().numeric());
    p = obj->parent();
    while (p) {
        oid_base_.push_back(p->element().numeric());
        p = p->parent();
    }
    reverse(oid_base_.begin(), oid_base_.end());

    // Add the mandotory snmpTrapOID.0 to the list
    netsnmp_variable_list *n = nullptr;
    snmp_varlist_add_variable(&n, SNMP_TRAP_OID, SNMP_TRAP_OID_LEN,
                              ASN_OBJECT_ID, (u_char *)noti_oid_.data(),
                              noti_oid_.size() * sizeof(oid));
    if (!n) {
        ok_ = false;
        return;
    }
    VTSS_BASICS_TRACE(DEBUG) << "New trap " << noti_oid_ << " added to handler "
                             << (void *)this << " var-binding: " << (void *)n;

    if (!notification_vars_.push_back(n)) {
        VTSS_BASICS_TRACE(ERROR) << "Failed to store pointer";
        snmp_free_varbind(n);
        ok_ = false;
        return;
    }
}

void TrapHandler::clear() {
    for (auto e : notification_vars_) {
        VTSS_BASICS_TRACE(DEBUG) << "snmp_free_varbind: " << (void *)e;
        snmp_free_varbind(e);
    }
}

TrapHandler::~TrapHandler() { clear(); }

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
