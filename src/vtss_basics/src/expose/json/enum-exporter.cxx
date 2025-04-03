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

#include <vtss/basics/trace_grps.hxx>
#define VTSS_TRACE_DEFAULT_GROUP VTSS_BASICS_TRACE_GRP_JSON

#include <vtss/basics/trace_basics.hxx>
#include <vtss/basics/expose/json/exporter.hxx>
#include <vtss/basics/expose/json/enum-exporter.hxx>

namespace vtss {
namespace expose {
namespace json {

void enumExporter(Exporter &exporter, int value,
                  const ::vtss_enum_descriptor_t *descriptor) {
    if (!exporter.ok()) return;

    while (true) {
        if (!descriptor->valueName) break;

        if (descriptor->intValue != value) {
            descriptor++;
            continue;
        }

        // found a match
        serialize(exporter, str(descriptor->valueName));
        return;
    }

    // no match found, in this case we will revert to exporting this as an
    // integer but print an error.
    VTSS_BASICS_TRACE(ERROR) << "Value: " << value
                             << " was not found in the descriptor at: "
                             << (void *)descriptor
                             << " value will be exported as an integer";
    serialize(exporter, value);
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

