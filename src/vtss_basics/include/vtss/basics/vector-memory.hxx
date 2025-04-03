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

#ifndef __VTSS_BASICS_VECTOR_MEMORY_HXX__
#define __VTSS_BASICS_VECTOR_MEMORY_HXX__

#include <vtss/basics/common.hxx>
#include <vtss/basics/utility.hxx>

namespace vtss {

struct VectorMemory {
    VectorMemory() {}
    VectorMemory(size_t cap, size_t alloc_size);
    VectorMemory(void *data, size_t cap) : data_(data), capacity_(cap) {}

    // No copies
    VectorMemory(const VectorMemory &rhs) = delete;
    VectorMemory &operator=(const VectorMemory &rhs) = delete;

    // Moving is okay
    VectorMemory(VectorMemory &&rhs);
    VectorMemory &operator=(VectorMemory &&rhs);

    // Swapping
    void swap(VectorMemory &rhs) {
        vtss::swap(data_, rhs.data_);
        vtss::swap(capacity_, rhs.capacity_);
    }

    ~VectorMemory();

    size_t capacity() const { return data_ ? capacity_ : 0; }
    void *data() { return data_; };
    const void *data() const { return data_; };

  private:
    void *data_ = nullptr;
    size_t capacity_ = 0;
};

}  // namespace vtss

#endif  // __VTSS_BASICS_VECTOR_MEMORY_HXX__
