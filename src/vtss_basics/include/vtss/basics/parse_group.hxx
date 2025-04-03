/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _PARSE_GROUP_H_
#define _PARSE_GROUP_H_

namespace vtss {
namespace parser {

struct ParserBase {
    virtual bool operator()(const char *& b, const char * e)=0;
};

bool Group(const char *& b, const char * e,
           ParserBase& t1);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8,
           ParserBase& t9);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8,
           ParserBase& t9, ParserBase& t10);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8,
           ParserBase& t9, ParserBase& t10, ParserBase& t11);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8,
           ParserBase& t9, ParserBase& t10, ParserBase& t11, ParserBase& t12);

bool Group(const char *& b, const char * e,
           ParserBase& t1, ParserBase& t2, ParserBase& t3, ParserBase& t4,
           ParserBase& t5, ParserBase& t6, ParserBase& t7, ParserBase& t8,
           ParserBase& t9, ParserBase& t10, ParserBase& t11, ParserBase& t12,
           ParserBase& t13);

}  // namespace parse
}  // namespace vtss

#endif /* _PARSE_GROUP_H_ */
