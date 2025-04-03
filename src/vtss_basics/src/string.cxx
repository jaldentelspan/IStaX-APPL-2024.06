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
#include "vtss/basics/string.hxx"
#include "vtss/basics/trace_basics.hxx"

namespace vtss {

void copy_c_str(const char *str, Buf &b) {
    // can't do anything with a null-buf
    if (b.begin() == b.end()) return;

    // output iterator
    char *i = b.begin();

    if (!str) {  // null ptr -> empty string
        *i = 0;
        return;
    }

    // Copy input ptr, with range protection
    while (*str && i != b.end()) *i++ = *str++;

    // Make sure we have room for a null-terminator
    if (i == b.end()) --i;

    // write null terminator
    *i = 0;
}

bool copy_str_to_c(str in, size_t s, char *b) {
    if (in.size() + 1 > s) return false;

    const char *i = in.begin();
    while (i != in.end()) *b++ = *i++;

    *b++ = '\0';
    return true;
}

str null_terminated(const CBuf &buf) {
    const char *b = buf.begin(), *i = buf.begin(), *e = buf.end();

    while (i != e) {
        if (!*i) break;

        ++i;
    }

    return str(b, i);
}

FixedBuffer::FixedBuffer(size_t s) : size_(0), buf_(0) {
    buf_ = static_cast<char *>(VTSS_BASICS_MALLOC(s + 1));
    if (buf_) size_ = s;
}

FixedBuffer::~FixedBuffer() {
    if (buf_) VTSS_BASICS_FREE(buf_);
}

char Buffer::empty_buffer = 0;
void Buffer::copy_construct(size_t s, const char *b, const char *e) {
    if (s == 0 || b == e) return;

    buf_ = static_cast<char *>(VTSS_BASICS_MALLOC(s + 1));
    if (buf_) {
        size_ = s;
        copy(b, e, buf_);
        *(buf_ + s) = 0;
    }
}

Buffer::Buffer(size_t s) {
    buf_ = static_cast<char *>(VTSS_BASICS_MALLOC(s + 1));
    if (buf_) {
        size_ = s;
        *(buf_ + s) = 0;
    } else {
        size_ = 0;
    }
}

Buffer &Buffer::operator=(const CBuf &b) {
    clear();
    copy_construct(b.size(), b.begin(), b.end());
    return *this;
}

Buffer &Buffer::operator=(const Buffer &b) {
    clear();
    copy_construct(b.size(), b.begin(), b.end());
    return *this;
}

void Buffer::clear() {
    if (buf_) {
        VTSS_BASICS_FREE(buf_);
    }
    buf_ = 0;
    size_ = 0;
}


bool operator==(const CBuf &lhs, const CBuf &rhs) {
    return equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator!=(const CBuf &lhs, const CBuf &rhs) {
    return !equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator<(const CBuf &lhs, const CBuf &rhs) {
    return lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
                                   rhs.end());
}

}  // namespace vtss
