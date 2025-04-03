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

#include "vtss/basics/expose/snmp/handlers/linux/set.hxx"

namespace vtss {
namespace expose {
namespace snmp {

SetCacheEntry set_cache[VTSS_SNMP_SET_CACHE_SIZE];

IteratorCommon *SetHandler::cache_loopup(const OidSequence &oid) {
    for (int i = 0; i < VTSS_SNMP_SET_CACHE_SIZE; ++i)
        if (set_cache[i].iterator.get() && set_cache[i].oid == oid)
            return set_cache[i].iterator.get();

    return nullptr;
}

bool SetHandler::cache_add(const OidSequence &oid, IteratorCommon *iter) {
    for (int i = 0; i < VTSS_SNMP_SET_CACHE_SIZE; ++i) {
        if (set_cache[i].iterator.get())
            continue;

        set_cache[i].iterator = unique_ptr<IteratorCommon>(iter);
        set_cache[i].oid = oid;
        return true;
    }

    return false;
}

}  // namespace snmp
}  // namespace expose
}  // namespace vtss
