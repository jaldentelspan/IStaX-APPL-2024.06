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
#include <unistd.h>
#include <iostream>
#include "vtss/basics/list.hxx"

namespace vtss {
namespace listExample {

typedef Pair<char *, size_t> BufRef;
static constexpr size_t BUF_SIZE = 16;
struct SingleBuf { char data[BUF_SIZE]; };

struct DynamicBuf {
    size_t produced() const { return produced_; }

    BufRef produce_prepare(); // Producer interface
    void produce_commit(size_t s);

    BufRef consume_prepare(); // Consume interface
    void consume_commit(size_t s);

  private:
    bool grow();
    size_t produce_offset() const;
    size_t produce_free() const;
    size_t consume_offset() const;
    size_t consume_valid() const;
    size_t capacity_ = 0, produced_ = 0, consumed_ = 0;

    List<SingleBuf> list_;
};

bool DynamicBuf::grow() {
    if (list_.emplace_back()) {
        capacity_ += BUF_SIZE;
        return true;
    }

    return false;
}

size_t DynamicBuf::produce_offset() const { return produced_ % BUF_SIZE; }
size_t DynamicBuf::produce_free() const { return BUF_SIZE - produce_offset(); }
size_t DynamicBuf::consume_offset() const { return consumed_ % BUF_SIZE; }
size_t DynamicBuf::consume_valid() const {
    switch (list_.size()) {
    case 0:
        return 0;
    case 1:
        return produce_offset() - consume_offset();
    default:
        return BUF_SIZE - consume_offset();
    }
}

void DynamicBuf::produce_commit(size_t s) {
    VTSS_ASSERT(s <= produce_free());
    produced_ += s;
}

BufRef DynamicBuf::produce_prepare() {
    if (capacity_ == produced_) {
        if (!grow()) return BufRef(nullptr, 0);
    }

    size_t offset = produce_offset();
    size_t free_ = produce_free();
    return BufRef(list_.back().data + offset, free_);
}

BufRef DynamicBuf::consume_prepare() {
    size_t offset = consume_offset();
    size_t valid = consume_valid();

    if (valid == 0) return BufRef(nullptr, 0);
    return BufRef(list_.front().data + offset, valid);
}

void DynamicBuf::consume_commit(size_t s) {
    VTSS_ASSERT(s <= consume_valid());
    consumed_ += s;
    if (s != 0 && consumed_ % BUF_SIZE == 0) list_.pop_front();
}

}  // namespace listExample
}  // namespace vtss


int main(int argc, char *argv[]) {
    vtss::listExample::DynamicBuf buf;

    // Read input
    while (true) {
        auto b = buf.produce_prepare();
        VTSS_ASSERT(b.first);  // Handle error, please...

        ssize_t s = read(STDIN_FILENO, b.first, b.second);
        if (s <= 0) break;
        buf.produce_commit(s);
    }

    // Print the header
    printf("\n\nsize: %zu\n", buf.produced());

    // Write output
    while (true) {
        auto b = buf.consume_prepare();
        if (!b.first) break;

        ssize_t s = write(STDOUT_FILENO, b.first, b.second);
        if (s <= 0) break;  // Handle error, please...
        buf.consume_commit(s);
    }

    return 0;
}
