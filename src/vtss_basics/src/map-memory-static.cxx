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

#include "vtss/basics/map-memory-static.hxx"

namespace vtss {

MapMemoryStatic::MapMemoryStatic(void *data, size_t cnt, size_t size) {
    data_ = (void **)data;

    // Handle error
    if (!data_) return;

    // Build the linked list by freeing all the chunks one at a time
    unsigned char *p = (unsigned char *)data_;
    for (size_t i = 0; i < cnt; ++i) {
        deallocate(p);
        p += size;
    }
}

void *MapMemoryStatic::allocate() {
    if (!head_) return nullptr;

    // Store the current head
    void *res = head_;

    // Pop the head
    head_ = (void **)(*head_);

    // Return the old-head that we have popped off the list
    return res;
}

void MapMemoryStatic::deallocate(void *ptr) {
    if (!head_) {
        // The list is empty meaning that this will be the new head
        head_ = (void **)ptr;

        // Mark that this is the end of the list (we have no next pointer)
        *head_ = nullptr;

    } else {
        // Push this entry into the front of the current head
        void **old_head = head_;
        head_ = (void **)ptr;

        // Update the next pointer to point to the old head
        *head_ = old_head;
    }
}

}  // namespace vtss
