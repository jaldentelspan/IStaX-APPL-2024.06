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

#include "vtss/basics/trace_grps.hxx"
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_SNMP

#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/expose/snmp/iterator-oid-base.hxx"

namespace vtss {
namespace expose {
namespace snmp {

mesa_rc IteratorOidBase::get_next(const OidSequence &idx, OidSequence &next) {
    mesa_rc rc;
    bool underflowed = false;

    VTSS_BASICS_TRACE(DEBUG) << "get_next, index: " << idx << " next: " << next;

    {
        // The oid string is parsed into the KEY parts of T...
        OidImporter importer(idx, true, OidImporter::Mode::Normal);

        value_clear_input();
        if (!value_parse_input(importer)) {
            underflowed = true;
        }
    }

    if (underflowed) {
        OidImporter importer(idx, true, OidImporter::Mode::Incomplete);
        (void)value_parse_input(importer);
    }

    // Continue to iterate until the get function succeeds
    while (true) {
        // Call the iterator
        rc = value_itr();

        // If the iterator is failing, then there is no way to continue
        if (rc != MESA_RC_OK) {
            return rc;
        }

        // Get ready to run the iterator again
        value_copy_next_to_data();

        // Call the next-ptr
        if (value_get() == MESA_RC_OK) break;
    }

    // Export the resulting keys to the next oid sequence
    OidExporter exporter;
    value_export_output(exporter);

    if (exporter.ok_) {
        next = exporter.oids_;
        return MESA_RC_OK;
    } else {
        return MESA_RC_ERROR;
    }
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
