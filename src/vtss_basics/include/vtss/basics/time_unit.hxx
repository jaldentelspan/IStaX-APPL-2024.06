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

#ifndef __VTSS_BASICS_TIME_UNIT_H__
#define __VTSS_BASICS_TIME_UNIT_H__

#include "vtss/basics/api_types.h"
#include "vtss/basics/int_wrapper.hxx"
#include "vtss/basics/time.hxx"

namespace vtss {

namespace details {
class seconds {};
class milliseconds {};
} /*details */

typedef IntWrapper<details::seconds> TimeUnitSeconds;
typedef IntWrapper<details::milliseconds> TimeUnitMilliseconds;

class TimeUnit {
  public:
    enum class Unit {
        seconds = 0x00000000,
        milliseconds = 0x40000000,
    };

    TimeUnit(nanoseconds nanosec)
        : TimeUnit(nanosec.raw() / 1000000, TimeUnit::Unit::milliseconds) {}
    TimeUnit(microseconds microsec)
        : TimeUnit(microsec.raw() / 1000, TimeUnit::Unit::milliseconds) {}
    TimeUnit(milliseconds millisec)
        : TimeUnit(millisec.raw(), TimeUnit::Unit::milliseconds) {}
    TimeUnit(seconds sec)
        : TimeUnit(sec.raw(), TimeUnit::Unit::seconds) {}

    TimeUnit(uint32_t value, TimeUnit::Unit unit) { set_data(value, unit); }

    TimeUnit(const TimeUnitSeconds seconds) {
        set_data(seconds.data_, TimeUnit::Unit::seconds);
    }

    TimeUnit(const TimeUnitMilliseconds milliseconds) {
        set_data(milliseconds.data_, TimeUnit::Unit::milliseconds);
    }

    TimeUnit::Unit get_unit() const {
        switch (data_ & unit_bits_mask) {
        case 0x00000000:
            return Unit::seconds;
        case 0x40000000:
            return Unit::milliseconds;
        }
        return Unit::seconds;
    }

    uint32_t get_value() const { return data_ & ~unit_bits_mask; }

  private:
    static constexpr int unit_bits_size = 2;
    static constexpr int value_bits_size =
            (sizeof(uint32_t) * 8 - unit_bits_size);
    static constexpr int unit_bits_mask = ((1 << unit_bits_size) - 1)
                                          << value_bits_size;

    void set_data(uint32_t value, TimeUnit::Unit unit) {
        data_ = value;
        data_ &= ~unit_bits_mask;
        data_ += static_cast<int>(unit);  // Ugly but it is doing the job
    }

    /*2 MSB represent the Unit, 30 LSB represents the value*/
    uint32_t data_;
};

vtss::seconds to_seconds(TimeUnit unit);
vtss::milliseconds to_milliseconds(TimeUnit unit);
vtss::microseconds to_microseconds(TimeUnit unit);
vtss::nanoseconds to_nanoseconds(TimeUnit unit);
}
#endif
