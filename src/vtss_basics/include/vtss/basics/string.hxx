/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _STRING_H_
#define _STRING_H_

#include <string>
#include "vtss/basics/common.hxx"
#include "vtss/basics/algorithm.hxx"

namespace vtss {

struct CBuf {
    virtual const char *begin() const = 0;
    virtual const char *end() const = 0;
    size_t size() const { return end() - begin(); }
    virtual ~CBuf() {}
};

struct Buf : public CBuf {
    // Just to avoid warnings on older compilers
    virtual const char *begin() const = 0;
    virtual const char *end() const = 0;

    virtual char *begin() = 0;
    virtual char *end() = 0;
    virtual ~Buf() {}
};

bool operator==(const CBuf &lhs, const CBuf &rhs);
bool operator!=(const CBuf &lhs, const CBuf &rhs);
bool operator<(const CBuf &lhs, const CBuf &rhs);

struct str : public CBuf {
    str() : b_(0), e_(0) {}
    explicit str(const char *b) : b_(b) { e_ = find_end(b); }
    str(const std::string &s) : b_(&*s.begin()), e_(s.c_str() + s.size()) {}
    str(const char *b, size_t l) : b_(b), e_(b + l) {
        if (!b) e_ = 0;
    }
    str(const char *b, const char *e) : b_(b), e_(e) {
        if (!b) e_ = 0;
    }
    explicit str(const CBuf &rhs) : b_(rhs.begin()), e_(rhs.end()) {}
    str &operator=(const CBuf &rhs) {
        b_ = rhs.begin();
        e_ = rhs.end();
        return *this;
    }

    const char *begin() const { return b_; }
    const char *end() const { return e_; }

    virtual ~str() {}

  private:
    const char *b_;
    const char *e_;
};

void copy_c_str(const char *str, Buf &b);
bool copy_str_to_c(str in, size_t size, char *buf);

str null_terminated(const CBuf &b);

struct BufPtr : public Buf {
    BufPtr() : b_(0), e_(0) {}
    explicit BufPtr(Buf &rhs) : b_(rhs.begin()), e_(rhs.end()) {}
    BufPtr(char *b, char *e) : b_(b), e_(e) {
        if (!b) e_ = 0;
    }
    BufPtr(char *b, size_t l) : b_(b), e_(b + l) {
        if (!b) e_ = 0;
    }

    BufPtr &operator=(Buf &rhs) {
        b_ = rhs.begin();
        e_ = rhs.end();
        return *this;
    }

    virtual const char *begin() const { return b_; }
    virtual const char *end() const { return e_; }
    virtual char *begin() { return b_; }
    virtual char *end() { return e_; }

    virtual ~BufPtr() {}

  private:
    char *b_, *e_;
};


template <size_t SIZE>
struct StaticBuffer : public Buf {
    StaticBuffer() {}
    StaticBuffer(char s) { memset(begin(), s, size()); }
    explicit StaticBuffer(const char *str) { copy_c_str(str, *this); }

    StaticBuffer<SIZE> &operator=(const char *str) {
        copy_c_str(str, *this);
        return *this;
    }

    StaticBuffer<SIZE> &operator=(const CBuf &buf) {
        size_t s = min(buf.size(), SIZE);
        copy(buf.begin(), buf.begin() + s, StaticBuffer<SIZE>::buf);
        return *this;
    }

    virtual ~StaticBuffer() {}

    const char *begin() const { return buf; }
    const char *end() const { return buf + SIZE; }
    char *begin() { return buf; }
    char *end() { return buf + SIZE; }
    unsigned size() const { return SIZE; }

  private:
    char buf[SIZE];
};
typedef StaticBuffer<4> SBuf4;
typedef StaticBuffer<8> SBuf8;
typedef StaticBuffer<16> SBuf16;
typedef StaticBuffer<32> SBuf32;
typedef StaticBuffer<64> SBuf64;
typedef StaticBuffer<128> SBuf128;
typedef StaticBuffer<256> SBuf256;
typedef StaticBuffer<512> SBuf512;

struct FixedBuffer : public Buf {
    FixedBuffer() : size_(0), buf_(0) {}
    FixedBuffer(size_t s, char *b) : size_(s), buf_(b) {}

    explicit FixedBuffer(size_t size);

    FixedBuffer(FixedBuffer &&rhs) : size_(rhs.size_), buf_(rhs.buf_) {
        rhs.size_ = 0;
        rhs.buf_ = 0;
    }

    FixedBuffer &operator=(FixedBuffer &&rhs) {
        size_ = rhs.size_;
        buf_ = rhs.buf_;
        rhs.size_ = 0;
        rhs.buf_ = 0;
        return *this;
    }

    FixedBuffer &operator=(const char *str) {
        copy_c_str(str, *this);
        return *this;
    }

    FixedBuffer &operator=(const CBuf &buf) {
        size_t s = min(buf.size(), size_);
        copy(buf.begin(), buf.begin() + s, buf_);
        return *this;
    }

    virtual ~FixedBuffer();

    const char *begin() const { return buf_; }
    const char *end() const { return buf_ + size_; }
    char *begin() { return buf_; }
    char *end() { return buf_ + size_; }
    unsigned size() const { return size_; }

    char *release() {
        char *c = buf_;
        size_ = 0;
        buf_ = 0;
        return c;
    }

  private:
    size_t size_;
    char *buf_;
};

// A dynamic allocated buffer which is always null-terminated. The null
// termination is not included in size/end.
struct Buffer : public Buf {
    Buffer() : size_(0), buf_(0) {}
    Buffer(size_t s);
    Buffer(const char *b, const char *e) : size_(0), buf_(0) {
        copy_construct(e - b, b, e);
    }
    Buffer(const Buffer &b) : size_(0), buf_(0) {
        copy_construct(b.size(), b.begin(), b.end());
    }

    explicit Buffer(const CBuf &b) : size_(0), buf_(0) {
        copy_construct(b.size(), b.begin(), b.end());
    }

    virtual ~Buffer() { clear(); }

    Buffer &operator=(const CBuf &b);
    Buffer &operator=(const Buffer &b);

    void clear();

    const char *begin() const {
        if (size_) return buf_;

        empty_buffer = 0;
        return &empty_buffer;
    }

    const char *end() const {
        if (size_) return buf_ + size_;

        empty_buffer = 0;
        return &empty_buffer;
    }

    char *begin() {
        if (size_) return buf_;

        empty_buffer = 0;
        return &empty_buffer;
    }

    char *end() {
        if (size_) return buf_ + size_;

        empty_buffer = 0;
        return &empty_buffer;
    }

    size_t size() const { return size_; }

  private:
    void copy_construct(size_t s, const char *b, const char *e);
    size_t size_;
    char *buf_;
    static char empty_buffer;
};

struct WhiteSpace {
    explicit WhiteSpace(unsigned int n) : width(n) {}
    unsigned int width;
};

}  // namespace vtss

#endif /* _STRING_H_ */
