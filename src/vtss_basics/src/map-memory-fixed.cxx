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

#include "vtss/basics/new.hxx"
#include "vtss/basics/map-memory-fixed.hxx"

namespace vtss {

MapMemoryFixed::MapMemoryFixed(size_t cnt, size_t size)
    : MapMemoryStatic(VTSS_BASICS_MALLOC(cnt * size), cnt, size) {}

void MapMemoryFixed::clear() {
    if (data_) VTSS_BASICS_FREE(data_);
    data_ = nullptr;
    head_ = nullptr;
}

MapMemoryFixed::MapMemoryFixed(MapMemoryFixed &&rhs)
    : MapMemoryStatic(nullptr, 0, 0) {
    data_ = rhs.data_;
    head_ = rhs.head_;

    rhs.data_ = nullptr;
    rhs.head_ = nullptr;
}

MapMemoryFixed &MapMemoryFixed::operator=(MapMemoryFixed &&rhs) {
    if (data_) VTSS_BASICS_FREE(data_);

    data_ = rhs.data_;
    head_ = rhs.head_;

    rhs.data_ = nullptr;
    rhs.head_ = nullptr;

    return *this;
}

MapMemoryFixed::~MapMemoryFixed() { clear(); }

}  // namespace vtss
