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

#ifndef VTSS_APPL_OS_CHRONO_HXX_
#define VTSS_APPL_OS_CHRONO_HXX_

//#include <chrono>

namespace vtss {
// Pull some useful aliases from std::chrono into the vtss namespace
// using system_clock  = std::chrono::system_clock;
// using steady_clock  = std::chrono::steady_clock;

//template < class T1, class T2 >
//inline std::chrono::milliseconds to_msecs(std::chrono::duration<T1,T2> d) {
//    return std::chrono::duration_cast<std::chrono::milliseconds>(d);
//}

inline time_t uptime_seconds() {
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC, &time) == 0) {
        return time.tv_sec;
    }
    return 0;
}

inline uint64_t uptime_milliseconds() {
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC, &time) == 0) {
        return ((uint64_t)time.tv_sec * 1000ULL) + (time.tv_nsec / 1000000);
    }
    return 0;
}

inline uint64_t uptime_microseconds() {
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC, &time) == 0) {
        return ((uint64_t)time.tv_sec * 1000000ULL) + (time.tv_nsec / 1000);
    }
    return 0;
}

}  // namespace vtss
#endif  // VTSS_APPL_OS_CHRONO_HXX_
