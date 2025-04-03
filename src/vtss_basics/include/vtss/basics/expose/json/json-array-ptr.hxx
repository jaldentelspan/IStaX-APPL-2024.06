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

#ifndef __VTSS_BASICS_EXPOSE_JSON_JSON_ARRAY_PTR_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_JSON_ARRAY_PTR_HXX__

#include <vtss/basics/vector.hxx>
#include <vtss/basics/expose/json/pre-loader.hxx>
#include <vtss/basics/expose/json/pre-exporter.hxx>
#include <vtss/basics/expose/json/json-value-ptr.hxx>

namespace vtss {
namespace expose {
namespace json {

struct JsonArrayPtr {
  public:
    friend void serialize(Loader &l, JsonArrayPtr &s);

    size_t size() const { return data.size(); }
    JsonValuePtr &operator[](size_t idx) { return data[idx]; }
    const JsonValuePtr &operator[](size_t idx) const { return data[idx]; }

    Vector<JsonValuePtr> data;
};

void serialize(Loader &e, JsonArrayPtr &);
void serialize(Exporter &e, const JsonArrayPtr &);

#if 0
template <uint32_t MaxSize>
struct JsonArrayPtr {
  public:
    // TODO(anielsen) should be more specific
    template <uint32_t U>
    friend void serialize(Loader &l, JsonArrayPtr<U> &s);

    JsonArrayPtr() : size_(0) {}
    size_t size() const { return size_; }

    JsonValuePtr &operator[](size_t idx) { return data[idx]; }
    const JsonValuePtr &operator[](size_t idx) const { return data[idx]; }

  private:
    size_t size_;
    JsonValuePtr data[MaxSize];
};

template <size_t S>
void serialize(Exporter &e, const JsonArrayPtr<S> &);

template <size_t S>
void serialize(Loader &e, JsonArrayPtr<S> &);

void serialize_array(Loader &l, JsonValuePtr *data, size_t max, size_t *size);

template <uint32_t MaxSize>
void serialize(Loader &l, JsonArrayPtr<MaxSize> &s) {
    // Hack to avoid template bloat
    serialize_array(l, s.data, MaxSize, &s.size_);
}

#endif

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_JSON_ARRAY_PTR_HXX__
