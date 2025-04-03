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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RO_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RO_HXX__

#include "vtss/basics/expose/snmp/types_params.hxx"
#include "vtss/basics/expose/snmp/iterator-common.hxx"
#include "vtss/basics/expose/snmp/handlers/oid_handler.hxx"

namespace vtss {
namespace expose {
namespace snmp {

template <typename... T>
struct OidIteratorRO : public IteratorCommon {
    typedef typename KeyValTypeList<T...>::type key_val_type_list;
    typedef typename ItrPtrTypeListCalc<T...>::list itr_type_list;

    typedef typename ItrPtrTypeCalc<itr_type_list>::type ItrPtr;
    typedef typename GetPtrTypeCalc<key_val_type_list>::type GetPtr;

    OidIteratorRO(GetPtr g, ItrPtr i)
        : IteratorCommon(MaxAccess::NotAccessible), get_ptr(g), itr_ptr(i) {}

    void call_serialize(GetHandler &h);
    void call_serialize_values(GetHandler &h);

    void call_serialize(SetHandler &h);
    void call_serialize_values(SetHandler &h);

    virtual mesa_rc get_next(const OidSequence &idx, OidSequence &next) {
        mesa_rc rc;
        bool underflowed = false;

        {
            // The oid string is parsed into the KEY parts of T...
            OidImporter i(idx, true, OidImporter::Mode::Normal);

            value.clear_input();
            if (!value.parse_input(i)) {
                underflowed = true;
            }
        }

        if (underflowed) {
            OidImporter i(idx, true, OidImporter::Mode::Incomplete);
            (void)value.parse_input(i);
        }


        // Continue to iterate until the get function succeeds
        while (true) {
            // Call the iterator
            rc = value.itr(itr_ptr);

            // If the iterator is failing, then there is no way to continue
            if (rc != MESA_RC_OK) {
                return rc;
            }

            // Get ready to run the iterator again
            value.copy_next_to_data();

            // Call the next-ptr
            if (value.get(get_ptr) == MESA_RC_OK) break;
        }

        // Export the resulting keys to the next oid sequence
        OidExporter e;
        value.export_output(e);

        if (e.ok_) {
            next = e.oids_;
            return MESA_RC_OK;
        } else {
            return MESA_RC_ERROR;
        }
    }


    virtual mesa_rc get(const OidSequence &oid) {
        OidImporter i(oid, false);

        if (!value.parse_input(i)) return MESA_RC_ERROR;

        if (!i.all_consumed()) return MESA_RC_ERROR;

        return value.get(get_ptr);
    }

    virtual mesa_rc set() { return MESA_RC_ERROR; }
    virtual mesa_rc undo() { return MESA_RC_ERROR; }

    // There is no user-defined set and get methods for row-editors
    virtual void *set_ptr__() const { return nullptr; }
    virtual void *get_ptr__() const { return (void *)get_ptr; }

    ParamTuple<T...> value;
    GetPtr get_ptr;
    ItrPtr itr_ptr;
};

}  // namespace snmp
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_OID_RO_HXX__
