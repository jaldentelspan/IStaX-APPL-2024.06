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

#include "vtss/basics/string-utils.hxx"

namespace vtss {

str trim(const str &s) {
    const char *begin = find_first_if(s.begin(), s.end(), &isgraph);
    const char *end = find_last_if(begin, s.end(), &isgraph);

    if (end != s.end()) {
        ++end;
    }

    return str(begin, end);
}

bool split(const str &in, char split_char, str &head, str &tail) {
    const char *split_point = find(in.begin(), in.end(), split_char);
    head = str(in.begin(), split_point);

    if (split_point == in.end()) {
        tail = str(in.end(), in.end());
        return false;
    } else {
        tail = str(split_point + 1, in.end());
        return true;
    }
}

Vector<str> split(str in, char split_char) {
    bool last;
    Vector<str> v;
    str head, tail = in;

    while ((last = split(tail, split_char, head, tail))) v.push_back(head);
    if (last || head.size()) v.push_back(head);

    return v;
}

LineIterator::I &LineIterator::I::operator++() {
    if (line_.end() != end_) {
        line_ = str(line_.end() + 1, eol(line_.end() + 1));
    }
    return *this;
}

const char *LineIterator::I::eol(const char *c) {
    str nl("\r\n");
    return find_first_of(c, end_, nl.begin(), nl.end());
}

}  // namespace vtss
