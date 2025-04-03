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

#ifndef __VTSS_BASICS_TIME_H__
#define __VTSS_BASICS_TIME_H__

#include <time.h>
#include <stdint.h>

#include "vtss/basics/api_types.h"
#include "vtss/basics/predefs.hxx"

#ifdef VTSS_BASICS_OPERATING_SYSTEM_LINUX
#include <chrono>
#include <thread>
#endif

namespace vtss {

template <typename T0>
struct TimeOperators {
    T0 operator+(const T0& rhs) const {
        return T0(static_cast<const T0*>(this)->raw() + rhs.raw());
    }

    T0 operator-(const T0& rhs) const {
        return T0(static_cast<const T0*>(this)->raw() - rhs.raw());
    }

    T0& operator+=(const T0& rhs) {
        static_cast<T0*>(this)->raw() += rhs.raw();
        return *(static_cast<T0*>(this));
    }

    T0& operator-=(const T0& rhs) {
        static_cast<T0*>(this)->raw() -= rhs.raw();
        return *(static_cast<T0*>(this));
    }

    bool operator<(const T0& rhs) {
        return static_cast<const T0*>(this)->raw() < rhs.raw();
    }

    bool operator>(const T0& rhs) {
        return static_cast<const T0*>(this)->raw() > rhs.raw();
    }

    bool operator<=(const T0& rhs) {
        return static_cast<const T0*>(this)->raw() >= rhs.raw();
    }

    bool operator>=(const T0& rhs) {
        return static_cast<const T0*>(this)->raw() >= rhs.raw();
    }

    bool operator!=(const T0& rhs) {
        return static_cast<const T0*>(this)->raw() != rhs.raw();
    }

    bool operator==(const T0& rhs) {
        return static_cast<const T0*>(this)->raw() == rhs.raw();
    }
};

struct seconds : public TimeOperators<seconds> {
    seconds() : d_(0) {}
    explicit seconds(uint64_t d) : d_(d) {}
    uint64_t raw() const { return d_; }
    uint32_t raw32() const {
        if (d_ > 0xffffffff) return 0xffffffff;
        return d_;
    }
    uint64_t& raw() { return d_; }

  private:
    uint64_t d_;
};

struct milliseconds : public TimeOperators<milliseconds> {
    milliseconds() : d_(0) {}
    milliseconds(seconds d) : d_(d.raw() * 1000LLU) {}
    explicit milliseconds(uint64_t d) : d_(d) {}
    uint64_t raw() const { return d_; }
    uint32_t raw32() const {
        if (d_ > 0xffffffff) return 0xffffffff;
        return d_;
    }
    uint64_t& raw() { return d_; }

  private:
    uint64_t d_;
};

struct microseconds : public TimeOperators<microseconds> {
    microseconds() : d_(0) {}
    microseconds(milliseconds d) : d_(d.raw() * 1000LLU) {}
    microseconds(seconds d) : d_(d.raw() * 1000000LLU) {}
    explicit microseconds(uint64_t d) : d_(d) {}
    uint64_t raw() const { return d_; }
    uint32_t raw32() const {
        if (d_ > 0xffffffff) return 0xffffffff;
        return d_;
    }
    uint64_t& raw() { return d_; }

  private:
    uint64_t d_;
};

struct nanoseconds : public TimeOperators<nanoseconds> {
    nanoseconds() : d_(0) {}
    nanoseconds(microseconds d) : d_(d.raw() * 1000LLU) {}
    nanoseconds(milliseconds d) : d_(d.raw() * 1000000LLU) {}
    nanoseconds(seconds d) : d_(d.raw() * 1000000000LLU) {}
    explicit nanoseconds(uint64_t d) : d_(d) {}
    uint64_t raw() const { return d_; }
    uint32_t raw32() const {
        if (d_ > 0xffffffff) return 0xffffffff;
        return d_;
    }
    uint64_t& raw() { return d_; }

  private:
    uint64_t d_;
};

#ifdef VTSS_BASICS_OPERATING_SYSTEM_LINUX
struct LinuxClock {
    typedef std::chrono::steady_clock::time_point time_point;
    typedef std::chrono::steady_clock::duration duration;

    static time_point now() {
        return std::chrono::steady_clock::now();
    }

    static time_point zero() {
        return time_point(duration::min());
    }

    static nanoseconds to_nanoseconds(duration t) {
        auto ns = std::chrono::nanoseconds(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t));
        return nanoseconds(ns.count());
    }

    static microseconds to_microseconds(duration t) {
        auto us = std::chrono::microseconds(
                std::chrono::duration_cast<std::chrono::microseconds>(t));
        return microseconds(us.count());
    }

    static milliseconds to_milliseconds(duration t) {
        auto ms = std::chrono::milliseconds(
                std::chrono::duration_cast<std::chrono::milliseconds>(t));
        return milliseconds(ms.count());
    }

    static seconds to_seconds(duration t) {
        auto s = std::chrono::seconds(
                std::chrono::duration_cast<std::chrono::seconds>(t));
        return seconds(s.count());
    }

    static duration to_time_t(nanoseconds n) {
        std::chrono::nanoseconds ns(n.raw());
        return std::chrono::duration_cast<duration>(ns);
    }

    static uint64_t time_point_to_raw(time_point t) {
        return to_microseconds(t.time_since_epoch()).raw();
    }

    static void sleep(duration d) {
        std::this_thread::sleep_for(d);
    }

    static void sleep(nanoseconds n) {
        std::this_thread::sleep_for(LinuxClock::to_time_t(n));
    }
};
#endif  // VTSS_BASICS_OPERATING_SYSTEM_LINUX

}  // namespace vtss

#endif /* __VTSS_BASICS_TIME_H__ */

