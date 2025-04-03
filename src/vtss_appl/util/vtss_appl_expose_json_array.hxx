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

#ifndef __UTIL_VTSS_APPL_EXPOSE_JSON_ARRAY_HXX__
#define __UTIL_VTSS_APPL_EXPOSE_JSON_ARRAY_HXX__

#include "vtss/basics/expose/json.hxx"

namespace vtss {

template <typename T>
struct ArrayExporter {
    ArrayExporter(expose::json::Exporter &e) : e_(e) {
        serialize(e_, expose::json::array_start);
    }

    ~ArrayExporter() { serialize(e_, expose::json::array_end); }

    void add(const T &value) {
        if (cnt++) serialize(e_, expose::json::delimetor);
        expose::json::serialize_class(e_, value);
    }

    void add(T &value) {
        if (cnt++) serialize(e_, expose::json::delimetor);
        expose::json::serialize_class(e_, value);
    }

    expose::json::Exporter &e_;
    uint32_t cnt = 0;
};

template <typename T>
struct ArrayLoad {
    ArrayLoad(expose::json::Loader &l) : l_(l) {}

    bool get(T &value) {
        // do not consume beyond the array limit
        if (last) return false;

        // consume array start
        if (first) {
            first = false;
            serialize(l_, expose::json::array_start);
        }

        // do not continue on a bad state
        if (!l_.ok_) return false;

        // check for end of array
        if (parse(l_.pos_, l_.end_, expose::json::array_end)) {
            last = true;
            return false;
        }

        // consume a delimitor unless this is the first entry
        if (cnt++) serialize(l_, expose::json::delimetor);

        // do not continue on a bad state
        if (!l_.ok_) return false;

        // consume value (if this is the last, then next call to get will
        // consume the trailing array_end char)
        expose::json::serialize_class(l_, value);

        // if serialization of the value above are okay, then we are okay
        return l_.ok_;
    }

    uint32_t cnt = 0;
    bool first = true;
    bool last = false;
    expose::json::Loader &l_;
};

template <typename T>
void serialize(expose::json::Exporter &e, const AsArray<T> x) {
    ArrayExporter<T> a(e);
    for (int i = 0; i < x.size; ++i) a.add(x.array[i]);
}

template <typename T>
void serialize(expose::json::Loader &l, AsArray<T> x) {
    T val;
    ArrayLoad<T> a(l);
    size_t cnt = 0;

    while (a.get(val)) {
        if (cnt >= x.size) {
            l.flag_error();
            return;
        }

        x.array[cnt++] = val;
    }

    if (cnt != x.size) l.flag_error();
}


template <typename T>
struct AsArrayDouble {
    AsArrayDouble(T *a, uint32_t s0, uint32_t s1) : array(a), size0(s0), size1(s1) {}
    T *array;
    uint32_t size0, size1;
};

template <typename T>
void serialize(expose::json::Exporter &e, const AsArrayDouble<T> x) {
    ArrayExporter<T> a(e);
    for (int i = 0; i < x.size0; ++i) {
        if (i) serialize(e, expose::json::delimetor);
        ArrayExporter<T> b(e);
        for (int j = 0; j < x.size1; ++j) {
            b.add(x.array[(i * x.size1) + j]);
        }
    }
}

template <typename T>
void serialize(expose::json::Loader &l, AsArrayDouble<T> x) {
    T val;
    size_t cnt0 = 0, cnt1;

    serialize(l, expose::json::array_start);
    while (l.ok_ && cnt0 < x.size0) {
        if (!l.ok_) break;
        ArrayLoad<T> a(l);
        cnt1 = 0;
        while (l.ok_ && a.get(val)) {
            if (cnt1 >= x.size1) {
                l.flag_error();
                return;
            }

            x.array[(cnt0 * x.size1) + cnt1] = val;
            cnt1++;
        }
        if (cnt1 != x.size1) {
            l.flag_error();
            return;
        }

        cnt0++;
        if ((cnt0 < x.size0 && !parse(l.pos_, l.end_, expose::json::delimetor)) ||
            (cnt0 == x.size0 && !parse(l.pos_, l.end_, expose::json::array_end))) {
            l.flag_error();
            return;
        }
    }
    if (cnt0 != x.size0) l.flag_error();
}

}  // namespace vtss

#endif  // __UTIL_VTSS_APPL_EXPOSE_JSON_ARRAY_HXX__
