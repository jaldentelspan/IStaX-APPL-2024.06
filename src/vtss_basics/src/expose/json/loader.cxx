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

#include "vtss/basics/expose/json/loader.hxx"
#include "vtss/basics/expose/json/skip-value.hxx"
#include "vtss/basics/expose/json/skip-string.hxx"
#include "vtss/basics/expose/json/parse-and-compare.hxx"

namespace vtss {
namespace expose {
namespace json {

Loader::Map::Map(Loader *p, bool patch)
    : parent(p), allow_uninitialized_values(patch) {
    if (!ok()) return;

    serialize(*parent, map_start);

    // check for empty map
    if (parse(p->pos_, p->end_, map_end)) {
        // record the end-of map as this is where we should leave the cursor
        // when leaving the destructor
        end_of_map = p->pos_;
        return;
    }

    // Run through the entire map and record position of all element names.
    while (1) {
        SkipString string;
        SkipValue value;

        // record the possistion of element names, as we need those when doing
        // look-ups in the map
        index.push_back(p->pos_);

        // element name
        if (!parse(p->pos_, p->end_, string)) {
            parent->flag_error();
            return;
        }

        // delimitor
        if (!parse(p->pos_, p->end_, map_assign)) {
            parent->flag_error();
            return;
        }

        // value
        if (!parse(p->pos_, p->end_, value)) {
            parent->flag_error();
            return;
        }

        // check for end-of-map
        if (parse(p->pos_, p->end_, map_end)) {
            // record the end-of map as this is where we should leave the cursor
            // when leaving the destructor
            end_of_map = p->pos_;
            return;
        }

        // check of comma (more values)
        if (!parse(p->pos_, p->end_, delimetor)) {
            // not end-of-map, and not comma (more values)
            parent->flag_error();
            return;
        }
    }
}

bool Loader::Map::search(const char *name) {
    // Use the index to find the correct entry point for the element
    // with the given name
    bool match = false;


    for (auto pos : index) {
        parent->pos_ = pos;
        if (parse_and_compare(parent->pos_, parent->end_, str(name))) {
            match = true;
            break;
        }
    }

    // Parse the map-assign symbol, such that the next value parser is ready to
    // begin.
    if (match) {
        serialize(*parent, map_assign);
        return true;
    }

    return false;
}

}  // namespace json
}  // namespace expose
}  // namespace vtss

