/* *****************************************************************************
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
 **************************************************************************** */

#ifndef __VTSS_BASICS_STREAM_HXX__
#define __VTSS_BASICS_STREAM_HXX__

#include <string>
#include "vtss/basics/common.hxx"
#include "vtss/basics/string.hxx"
#include "vtss/basics/api_types.h"
#include "vtss/basics/types.hxx"
#include "vtss/basics/formatting_tags.hxx"

#ifdef VTSS_BASICS_OPERATING_SYSTEM_LINUX
#include <chrono>
#endif

namespace vtss {
struct ostream {
    virtual bool ok() const = 0;
    virtual bool push(char val) = 0;
    virtual size_t write(const char *b, const char *e) = 0;
    virtual size_t fill(size_t s, char c) {
        size_t i = 0;
        for (; i < s; i++)
            if (!push(c))
                break;
        return i;
    }
    virtual ~ostream() { }
};

struct ostreamBuf : public ostream {
    virtual void clear() = 0;
    virtual const char * begin() const = 0;
    virtual const char * end() const = 0;
};

struct nullstream : public ostreamBuf {
    bool ok() const { return true; }
    bool push(char val) { return true; }
    void clear() { }
    const char * begin() const { return 0; }
    const char * end() const { return 0; }
    size_t write(const char *b, const char *e) { return 0; }
    ~nullstream() { }
};

struct fdstream : public ostream {
    fdstream() : fd_(-1) { }
    explicit fdstream(int fd) : fd_(fd) { }

    void fd(int f) { fd_ = f; }

    bool ok() const { return error_ == 0; }
    bool push(char val);
    size_t write(const char *b, const char *e);

  private:
    int fd_, error_;
};

struct httpstream : public ostream {
    httpstream(void *httpd_state, const char *mime = "text");
    bool ok() const;
    bool push(char val);
    size_t write(const char *b, const char *e);
    ~httpstream();

  private:
    bool ok_;
};

template <typename T, typename S = ostream>
class obuffer_iterator {
  public:
    typedef T value_type;
    typedef value_type* pointer;
    typedef value_type& reference;

    explicit  obuffer_iterator(S& os) : os_(os) {}
    obuffer_iterator(const obuffer_iterator& iter) : os_(iter.os_) {}
    obuffer_iterator& operator=(const T& v) { os_ << v; return (*this); }
    obuffer_iterator& operator*()  { return (*this); }
    obuffer_iterator& operator++() { return (*this); }
    obuffer_iterator  operator++(int /*dummy*/)  { return (*this); }
    obuffer_iterator& operator--() { return (*this); }
    obuffer_iterator  operator--(int /*dummy*/)  { return (*this); }

    bool operator== (const obuffer_iterator& i) const {
        return (os_.pos() == i.os_.pos());
    }

    bool operator< (const obuffer_iterator& i) const {
        return (os_.pos() < i.os_.pos());
    }

  private:
    S& os_;
};

struct BufPtrStream : public ostreamBuf {
    explicit BufPtrStream(Buf *b)
            : ostreamBuf(), buf_(b), ok_(true) { pos_ = begin(); }
    explicit BufPtrStream() : ostreamBuf(), buf_(0), ok_(false), pos_(0) { }

    ~BufPtrStream() {}

    bool ok() const { return ok_; }

    bool push(char c) {
        if (!buf_) {
            ok_ = false;
            return false;
        }

        if (pos_ != buf_->end()) {
            *pos_++ = c;
            return true;

        } else {
            ok_ = false;
            return false;
        }
    }

    size_t write(const char *b, const char *e) {
        size_t buf_free_length = (buf_->end() - pos_);
        size_t input_length = e - b;

        if (input_length > buf_free_length) {
            ok_ = false;
            e = b + buf_free_length;
        }

        copy(b, e, pos_);
        pos_ += (e - b);

        return e - b;
    }


    char * begin() {
        if (buf_) return buf_->begin();
        else      return NULL;
    }

    const char * begin() const {
        if (buf_) return buf_->begin();
        else      return NULL;
    }

    char * end() { return pos_; }
    const char * end() const { return pos_; }

    char * buf_end() {
        if (buf_) return buf_->end();
        else      return NULL;
    }

    const char * buf_end() const {
        if (buf_) return buf_->end();
        else      return NULL;
    }

    size_t size() const { return end() - begin(); }

    // Add null-termination but do not update the possistion pointer.
    const char *cstring();
    const char *cstringnl();
    const str as_str() const { return str(begin(), end()); }

    void clear() {
        pos_ = begin();
        ok_ = true;
    }

    Buf *detach() {
        Buf *tmp = buf_;
        ok_ = false;
        pos_ = 0;
        buf_ = 0;
        return tmp;
    }

    void attach(Buf *tmp) {
        buf_ = tmp;
        pos_ = buf_->begin();
        ok_ = true;
    }

  protected:
    Buf *buf_;
    bool ok_;
    char *pos_;
};

struct ReverseBufPtrStream : public ostreamBuf {
    explicit ReverseBufPtrStream(Buf *b) : ostreamBuf(), buf_(b), ok_(true) {
        pos_ = buf_->end();
    }

    explicit ReverseBufPtrStream() : ostreamBuf(), buf_(0), ok_(false),
                pos_(0) { }

    ~ReverseBufPtrStream() { }

    bool ok() const { return ok_; }

    bool push(char c) {
        if (!buf_) {
            ok_ = false;
            return false;
        }

        if (pos_ != buf_->begin()) {
            --pos_;
        } else {
            ok_ = false;
            return false;
        }

        *pos_ = c;
        return true;
    }

    size_t write(const char *b, const char *e) {
        const char *i = b;

        while (i != e) {
            if (push(*i)) {
                ++i;
            } else {
                break;
            }
        }

        return i - b;
    }

    const char * begin() const {
        if (buf_) return pos_;
        else      return NULL;
    }

    const char * end() const { return buf_->end(); }

    void clear() {
        pos_ = buf_->end();
        ok_ = true;
    }

    Buf *detach() {
        Buf *tmp = buf_;
        ok_ = false;
        pos_ = 0;
        buf_ = 0;
        return tmp;
    }

    void attach(Buf *tmp) {
        buf_ = tmp;
        pos_ = buf_->end();
        ok_ = true;
    }

  protected:
    Buf *buf_;
    bool ok_;
    char *pos_;
};

template<typename Buf>
struct BufStream : public ostreamBuf {
    BufStream() : impl(&buf_) { }
    virtual ~BufStream() { }

    bool ok() const { return impl.ok(); }
    bool push(char c) { return impl.push(c); }
    char * end() { return impl.end(); }
    const char * end() const { return impl.end(); }
    char * begin() { return impl.begin(); }
    const char * begin() const { return impl.begin(); }
    void clear() { impl.clear(); }
    const char *cstring() { return impl.cstring(); }
    const char *cstringnl() { return impl.cstringnl(); }
    size_t write(const char *b, const char *e) { return impl.write(b, e); }

  protected:
    Buf buf_;
    BufPtrStream impl;
};

struct StringStream : public ostreamBuf {
    StringStream() { }
    virtual ~StringStream() { }

    bool ok() const { return true; }
    bool push(char c) { buf.push_back(c); return true; }
    char *end() { return &*buf.end(); }
    const char * end() const { return buf.c_str() + buf.size(); }
    char *begin() { return  &*buf.begin(); }
    const char *begin() const { return &*buf.begin(); }
    void clear() { buf.clear(); }
    const char *cstring() { return buf.c_str(); }
    const char *cstringnl() {
        buf.reserve(buf.size() + 2);
        char *i = &*buf.end();
        *i++ = '\n';
        *i++ = 0;
        return &*buf.begin();
    }

    size_t write(const char *b, const char *e) {
        buf.append(b, e - b);
        return e - b;
    }

    std::string buf;
};

template<typename Buf>
struct ReverseBufStream : public ostreamBuf {
    ReverseBufStream() : impl(&buf_) { }
    virtual ~ReverseBufStream() { }

    bool ok() const { return impl.ok(); }
    bool push(char c) { return impl.push(c); }
    const char * end() const { return impl.end(); }
    const char * begin() const { return impl.begin(); }
    void clear() { impl.clear(); }
    size_t write(const char *b, const char *e) { return impl.write(b, e); }

  protected:
    Buf buf_;
    ReverseBufPtrStream impl;
};

template<unsigned S>
struct AlignStream : public StaticBuffer<S>, public ostream {
    explicit AlignStream(ostream& o) :
        o_(o), ok_(true),
        pos_(StaticBuffer<S>::begin()),
        flush_(StaticBuffer<S>::begin()) { }

    bool ok() const { return o_.ok(); }
    virtual void flush() = 0;

    bool push(char c) {
        if (pos_ != StaticBuffer<S>::end()) {
            *pos_++ = c;
            return true;
        } else {
            flush();
            return o_.push(c);
        }
    }

    size_t write(const char *b, const char *e) {
        const char *i = b;
        for (; i != e; ++i) {
            if (!push(*i))
                break;
        }

        return i - b;
    }

    //void fill(unsigned i, char c) { while (i--) push(c); }
    char * pos() { return pos_; }
    const char * pos() const { return pos_; }

  protected:
    ostream& o_;
    bool ok_;
    char * pos_;
    char * flush_;
};

template<unsigned S>
struct RightAlignStream : public AlignStream<S> {
    RightAlignStream(ostream& o, char f) :
        AlignStream<S>(o), fill_char(f) { }
    ~RightAlignStream() { flush(); }

    void flush() {
        typedef AlignStream<S> B;
        B::o_.fill(B::end() - B::pos_, fill_char);
        copy(B::flush_, B::pos_, obuffer_iterator<char>(B::o_));
        B::flush_ = B::pos_;
    }

    const char fill_char;
};

template<unsigned S>
struct LeftAlignStream : public AlignStream<S> {
    LeftAlignStream(ostream& o, char f) :
        AlignStream<S>(o), fill_char(f) { }
    ~LeftAlignStream() { flush(); }
    void flush() {
        typedef AlignStream<S> B;
        copy(B::flush_, B::pos_, obuffer_iterator<char>(B::o_));
        B::o_.fill(B::end() - B::pos_, fill_char);
        B::flush_ = B::pos_;
    }

    const char fill_char;
};

template<unsigned S, typename T0>
ostream& operator<<(ostream& o, const FormatLeft<S, T0>& l) {
    LeftAlignStream<S> los(o, l.fill);
    los << l.t;
    return o;
}

template<unsigned S, typename T0>
ostream& operator<<(ostream& o, const FormatRight<S, T0>& r) {
    RightAlignStream<S> ros(o, r.fill);
    ros << r.t0;
    return o;
}

inline ostream& operator<<(ostream& o, const char &s) { o.push(s); return o; }

ostream& operator<<(ostream& o, const std::string &s);
ostream& operator<<(ostream& o, const ostreamBuf &s);
ostream& operator<<(ostream& o, const str &s);
ostream& operator<<(ostream& o, const Buffer &s);
ostream& operator<<(ostream& o, const char *s);
ostream& operator<<(ostream& o, const ssize_t s);
ostream& operator<<(ostream& o, const void *s);

ostream& operator<<(ostream& o, const int8_t s);
ostream& operator<<(ostream& o, const int16_t s);
ostream& operator<<(ostream& o, const int32_t s);
ostream& operator<<(ostream& o, const int64_t s);

ostream& operator<<(ostream& o, const uint8_t s);
ostream& operator<<(ostream& o, const uint16_t s);
ostream& operator<<(ostream& o, const uint32_t s);
ostream& operator<<(ostream& o, const uint64_t s);
#if VTSS_SIZEOF_VOID_P == 4
ostream& operator<<(ostream& o, const long unsigned int s);
#endif

ostream& operator<<(ostream& o, FormatHex<const uint8_t> s);
ostream& operator<<(ostream& o, FormatHex<const uint16_t> s);
ostream& operator<<(ostream& o, FormatHex<const uint32_t> s);
ostream& operator<<(ostream& o, FormatHex<const uint64_t> s);

ostream& operator<<(ostream& o, const Binary b);

ostream& operator<<(ostream& o, AsLocaltime_HrMinSec<int> s);
ostream& operator<<(ostream& o, AsTime_HrMinSec<uint64_t> s);
ostream& operator<<(ostream& o, AsTimeUs<uint64_t> s);
ostream& operator<<(ostream& o, AsCounter s);

ostream& operator<<(ostream& o, const mesa_mac_t &m);
ostream& operator<<(ostream& o, const MacAddress &m);

ostream& operator<<(ostream& o, const Ipv4Address &i);
ostream& operator<<(ostream& o, const AsIpv4 &i);
ostream& operator<<(ostream& o, const AsIpv4_const &i);

ostream& operator<<(ostream& o, const ::mesa_ipv4_network_t&i);
ostream& operator<<(ostream& o, const Ipv4Network &i);

ostream& operator<<(ostream& o, const ::mesa_ipv6_t &i);
ostream& operator<<(ostream& o, const Ipv6Address &i);

ostream& operator<<(ostream& o, const ::mesa_ipv6_network_t &i);
ostream& operator<<(ostream& o, const Ipv6Network &i);

ostream& operator<<(ostream& o, const ::mesa_ip_addr_t &i);
ostream& operator<<(ostream& o, const IpAddress &i);

ostream& operator<<(ostream& o, const ::mesa_ip_network_t &i);
ostream& operator<<(ostream& o, const IpNetwork &i);

ostream& operator<<(ostream& o, const ::mesa_port_list_t &p);

ostream& operator<<(ostream& o, const WhiteSpace &ws);

ostream& operator<<(ostream &o, const AsBool &rhs);

ostream& operator<<(ostream &o, const AsBitMask &rhs);

ostream& operator<<(ostream &o, const AsDisplayString &rhs);

ostream& operator<<(ostream &o, const AsOctetString &rhs);

ostream& operator<<(ostream &o, const AsRowEditorState &rhs);

ostream& operator<<(ostream &o, const AsPercent &rhs);

ostream& operator<<(ostream &o, const AsInt &rhs);

ostream& operator<<(ostream &o, AsErrorCode rhs);

ostream &operator<<(ostream &o, const ::vtss_inet_address_t &a);

ostream& operator<<(ostream &o, const BinaryLen &rhs);

ostream& operator<<(ostream &o, const AsSnmpObjectIdentifier &rhs);

ostream& operator<<(ostream &o, const AsDecimalNumber &rhs);

#ifdef VTSS_BASICS_OPERATING_SYSTEM_LINUX
ostream& operator<<(ostream &o, const std::chrono::nanoseconds& ns);
#endif

}  // namespace vtss

#endif  // __VTSS_BASICS_STREAM_HXX__

