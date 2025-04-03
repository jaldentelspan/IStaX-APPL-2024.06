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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_GLOBALS_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_GLOBALS_HXX__

#include "vtss/basics/expose/snmp/types.hxx"
#include "vtss/basics/expose/snmp/handlers.hxx"

#ifdef VTSS_BASICS_OPERATING_SYSTEM_LINUX
#include <vtss/basics/vector.hxx>
#endif

namespace vtss {
namespace expose {
namespace snmp {

// TODO, rename to Agent
struct Globals
#if defined(VTSS_BASICS_STANDALONE)
    : public OidInventory
#endif
{
    NamespaceNode iso;

    Globals();
    mesa_rc attach_module(Node &n);

#if defined(VTSS_BASICS_STANDALONE)
    ErrorCode::E get(const OidSequence &in, ostream *result);
    ErrorCode::E get_next(OidSequence in, OidSequence &next, ostream *result);
    ErrorCode::E set(const OidSequence &in, str v, ostream *result);

    ErrorCode::E get(Vector<int> v, ostream *result);
    ErrorCode::E get_next(Vector<int> v, OidSequence &next, ostream *result);
#endif

    StructBaseTrap *trap_find(const char *n);
    StructBaseTrap *trap_find_next(const char *n);

  private:
    NamespaceNode org;
    NamespaceNode dod;
    NamespaceNode internet;
    NamespaceNode priv;
    NamespaceNode enterprises;
    NamespaceNode vtss_root;
    NamespaceNode modules_root;
};

extern Globals vtss_snmp_globals;

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_GLOBALS_HXX__
