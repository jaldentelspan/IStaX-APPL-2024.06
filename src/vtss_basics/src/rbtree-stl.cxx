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

#include "vtss/basics/common.hxx"
#include "vtss/basics/rbtree-stl.hxx"

namespace vtss {

void *RbtreeStl::recycle_nodes(const size_t target_size) {
    void **head = nullptr;
    void **last = nullptr;

    if (target_size == 0) {
        clear();
        return nullptr;
    }

    if (size() >= target_size) {
        // Re-use the already allocated memory
        size_t cnt = 0;
        RbtreeBase::NodeBase *i = RbtreeBase::begin();

        while (i) {
            // get next pointer before deleting
            auto n = i->next();

            // delete the entry from the tree
            RbtreeBase::erase(i);  // TODO could skip balancing

            // Call the destructor now as we are about to write to the
            // memory
            void *node = destruct(i);

            // Either append the node to the list, or free it
            if (!head) {
                // First node
                head = (void **)node;
                last = (void **)node;

            } else if (cnt < target_size) {
                // Append the node to the list
                *last = node;
                last = (void **)node;

            } else {
                // Free the node - we have what we need
                deallocate(node);
            }

            cnt++;
            i = n;  // Update 'i' to the next pointer
        }

    } else {
        // Allocate a new chunk of memory
        head = (void **)allocate(target_size);

        // Allocation failed!! We must exit!
        if (!head) return nullptr;

        // Can not fail after this point - clear the existing content and
        // start inserting
        clear();
    }

    return head;
}

void RbtreeStl::erase(RbtreeBase::NodeBase *n) {
    // Delete the node from the tree
    RbtreeBase::erase(n);

    // Free the node
    delete_(n);
}

size_t RbtreeStl::erase(const KeyBase &key) {
    RbtreeBase::Where where;

    // Search the tree to find a node with the requested value
    auto n = RbtreeBase::find(key, where);

    // If no found return zero
    if (where != RbtreeBase::MATCH) return 0;

    // Otherwise delete the node in the tree
    RbtreeBase::erase(n);

    // Destruct and free the memory
    delete_(n);

    return 1;
}

void RbtreeStl::erase(RbtreeBase::NodeBase *i, RbtreeBase::NodeBase *e) {
    // TODO - could we skip rebalancing??
    while (i != e) {
        auto n = i->next();
        erase(i);
        i = n;
    }
}

void RbtreeStl::clear() {
    // TODO - we could properly skip the re-balancing here.
    erase(RbtreeBase::begin(), nullptr);
}

void *RbtreeStl::allocate(size_t cnt) {
    // Base case
    if (cnt == 0) return nullptr;

    // Allocate the first chunk, and setup 'head' and 'last' pointers
    void *head = allocate();
    void *last = head;

    // Check if first allocation went well, if not return with out cleaning
    // up.
    if (head)
        --cnt;
    else
        return nullptr;

    // Allocate the remaining chunks
    for (; cnt; --cnt) {
        void *i = allocate();
        if (!i) break;

        *((void **)last) = i;
        last = i;
    }

    // If we did not manage to allocate all 'cnt' then we must free what we
    // have allocated.
    if (cnt) {
        while (true) {
            // read the next pointer
            void *n = *((void **)head);

            // Free the current head
            deallocate(head);

            // Check if we have freed the last chunk
            if (head == last) break;

            // If not, update the head pointer and continue
            head = n;
        }

        return nullptr;
    }

    return head;
}

}  // namespace vtss
