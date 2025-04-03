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

#include "alarm-expression/token.hxx"

namespace vtss {
namespace appl {
namespace alarm {
namespace expression {

ostream &operator<<(ostream &o, const Token &e) {
    switch (e) {
#define CASE(X)  \
    case X:      \
        o << #X; \
        return o
        CASE(Token::plus);
        CASE(Token::minus);
        CASE(Token::mult);
        CASE(Token::div);
        CASE(Token::and_);
        CASE(Token::or_);
        CASE(Token::start_p);
        CASE(Token::end_p);
        CASE(Token::equal);
        CASE(Token::not_equal);
        CASE(Token::great);
        CASE(Token::less);
        CASE(Token::great_equal);
        CASE(Token::less_equal);
        CASE(Token::neg);
#undef CASE

    default:

        o << "Unknown(" << (int)e << ")";
        return o;
    }
}

}  // namespace expression
}  // namespace alarm
}  // namespace appl
}  // namespace vtss
