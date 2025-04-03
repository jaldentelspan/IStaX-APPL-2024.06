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
#include <vtss/basics/expose/json/loader.hxx>
#include <vtss/basics/expose/json/enum-loader-impl.hxx>
#include <vtss/basics/expose/json/parse-and-compare.hxx>

namespace vtss {
namespace expose {
namespace json {

void enumLoader_(Loader &loader, int &value,
                 const ::vtss_enum_descriptor_t *descriptor) {
    if (!loader.ok_) return;

    while (true) {
        // break on end-of-descriptor
        if (!descriptor->valueName) break;

        // continue if nat match
        if (!parse_and_compare(loader.pos_, loader.end_,
                               str(descriptor->valueName))) {
            descriptor++;
            continue;
        }

        // found a match
        VTSS_BASICS_TRACE(NOISE) << "Match found";
        value = descriptor->intValue;
        return;
    }

    VTSS_BASICS_TRACE(DEBUG) << "Failed to load enum";
    loader.flag_error();
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

