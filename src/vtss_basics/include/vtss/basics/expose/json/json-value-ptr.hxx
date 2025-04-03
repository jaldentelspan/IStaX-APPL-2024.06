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

#ifndef __VTSS_BASICS_EXPOSE_JSON_JSON_VALUE_PTR_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_JSON_VALUE_PTR_HXX__

#include <vtss/basics/string.hxx>
#include <vtss/basics/expose/json/value-type.hxx>
#include <vtss/basics/expose/json/pre-loader.hxx>
#include <vtss/basics/expose/json/pre-exporter.hxx>

namespace vtss {
namespace expose {
namespace json {

struct JsonValuePtr {
  public:
    friend void serialize(Loader &l, JsonValuePtr &s);
    friend bool parse(const char *&b, const char *e, JsonValuePtr &s);
    JsonValuePtr() : type_(ValueType::Null) {}
    JsonValuePtr(const JsonValuePtr &rhs) : type_(rhs.type_), data(rhs.data) {}
    JsonValuePtr &operator =(const JsonValuePtr &rhs) {
        type_ = rhs.type_;
        data = rhs.data;
        return *this;
    }

    ValueType::E type() const { return type_; }

    const char *begin() const { return data.begin(); }
    const char *end() const { return data.end(); }
    const str as_str() const { return data; }

    template <typename T>
    bool copy_to(T &t, const char **not_found = nullptr) const;

  private:
    ValueType::E type_;
    str data;
};

bool parse(const char *&b, const char *e, JsonValuePtr &s);

void serialize(Exporter &e, const JsonValuePtr &);
void serialize(Loader &e, JsonValuePtr &);

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_JSON_VALUE_PTR_HXX__
