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

#ifndef __VTSS_BASICS_STRING_UTILS_HXX__
#define __VTSS_BASICS_STRING_UTILS_HXX__

#include <vtss/basics/string.hxx>
#include <vtss/basics/vector.hxx>

namespace vtss {

inline bool isgraph(const char c) { return (c >= 0x21 && c <= 0x7E); }

str trim(const str &s);
bool split(const str &in, char split_char, str &head, str &tail);

Vector<str> split(str in, char split_char);

struct LineIterator {
    struct I : public iterator<forward_iterator_tag, str> {
        I(const char *lb, const char *e) : end_(e), line_(lb, eol(lb)) {}
        const str *operator->() const { return &line_; }
        const str &operator*() const { return line_; }

        I &operator++();

        bool operator==(const I &rhs) { return line_.end() == rhs.line_.end(); }
        bool operator!=(const I &rhs) { return line_.end() != rhs.line_.end(); }

      private:
        const char *eol(const char *c);

        const char *end_;
        str line_;
    };

    LineIterator(str s) : s_(s) {}
    I begin() { return I(s_.begin(), s_.end()); }
    I end() { return I(s_.end(), s_.end()); }

  private:
    str s_;
};


}  // namespace vtss

#endif  // __VTSS_BASICS_STRING_UTILS_HXX__
