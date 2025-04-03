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

#include "vtss/basics/time_unit.hxx"


namespace vtss {

vtss::seconds to_seconds(TimeUnit unit) {
    switch (unit.get_unit()) {
    case TimeUnit::Unit::seconds:
        return vtss::seconds{unit.get_value()};
    case TimeUnit::Unit::milliseconds:
        return vtss::seconds{unit.get_value() / 1000};
    }
    return vtss::seconds{0};
}

vtss::milliseconds to_milliseconds(TimeUnit unit) {
    switch (unit.get_unit()) {
    case TimeUnit::Unit::seconds:
        return vtss::milliseconds{unit.get_value() * 1000};
    case TimeUnit::Unit::milliseconds:
        return vtss::milliseconds{unit.get_value()};
    }
    return vtss::milliseconds{0};
}

vtss::microseconds to_microseconds(TimeUnit unit) {
    switch (unit.get_unit()) {
    case TimeUnit::Unit::seconds:
        return vtss::microseconds{unit.get_value() * 1000000};
    case TimeUnit::Unit::milliseconds:
        return vtss::microseconds{unit.get_value() * 1000};
    }
    return vtss::microseconds{0};
}

vtss::nanoseconds to_nanoseconds(TimeUnit unit) {
    switch (unit.get_unit()) {
    case TimeUnit::Unit::seconds:
        return vtss::nanoseconds{unit.get_value() * 1000000000};
    case TimeUnit::Unit::milliseconds:
        return vtss::nanoseconds{unit.get_value() * 1000000};
    }
    return vtss::nanoseconds{0};
}
}
