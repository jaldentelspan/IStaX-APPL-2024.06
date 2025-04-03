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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RO2_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RO2_HXX__

#include "vtss/basics/expose/snmp/types_params.hxx"
#include "vtss/basics/expose/snmp/iterator-oid-base.hxx"
#include "vtss/basics/expose/snmp/handlers/oid_handler.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename INTERFACE>
struct OidIteratorRO2 : public IteratorOidBase {
    typedef typename INTERFACE::P::get_t GetPtr;
    typedef typename INTERFACE::P::itr_t ItrPtr;

    OidIteratorRO2(GetPtr g, ItrPtr i)
        : IteratorOidBase(MaxAccess::NotAccessible), get_ptr(g), itr_ptr(i) {}

    void call_serialize(GetHandler &h);
    void call_serialize_values(GetHandler &h);

    void call_serialize(SetHandler &h);
    void call_serialize_values(SetHandler &h);

    mesa_rc value_itr();
    mesa_rc value_get();
    void value_clear_input();
    void value_copy_next_to_data();
    bool value_export_output(OidExporter &exporter);
    bool value_parse_input(OidImporter &importer);

    virtual mesa_rc get(const OidSequence &oid) {
        OidImporter importer(oid, false);
        INTERFACE interface;

        if (!value.parse_input2(importer, interface)) return MESA_RC_ERROR;

        if (!importer.all_consumed()) return MESA_RC_ERROR;

        return value.get(get_ptr);
    }

    virtual mesa_rc set() { return MESA_RC_ERROR; }
    virtual mesa_rc undo() { return MESA_RC_ERROR; }

    // There is no user-defined set and get methods for row-editors
    virtual void *set_ptr__() const { return nullptr; }
    virtual void *get_ptr__() const { return (void *)get_ptr; }

    typename Interface2Any<ParamTuple, INTERFACE>::type value;
    GetPtr get_ptr;
    ItrPtr itr_ptr;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RO2_HXX__
