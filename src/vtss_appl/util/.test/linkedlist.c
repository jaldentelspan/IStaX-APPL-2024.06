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
#include <assert.h>
#include "../vtss_intrusive_list.h"

typedef struct item_t_ {
    VTSS_LINKED_LIST_DECL_ITEM(item_t_);
    int xx;
} item_t;

typedef VTSS_LINKED_LIST_TYPE(item_t_) list_t;

#define cnt_assert(xx) \
{ \
    int cnt = 0; \
    item_t *i; \
    VTSS_LINKED_LIST_FOREACH(list, i) \
        cnt += 1; \
    assert(cnt == xx); \
}

#define assert_0(list)                                \
    assert(list.__LIST_HEAD__ == list.__LIST_TAIL__); \
    assert(VTSS_LINKED_LIST_EMPTY(list));             \
    assert(!VTSS_LINKED_LIST_ONE(list));              \
    cnt_assert(0)

#define assert_1(list, a)                             \
    assert(!VTSS_LINKED_LIST_EMPTY(list));            \
    assert(VTSS_LINKED_LIST_ONE(list));               \
    assert(list.__LIST_HEAD__ == &a);                 \
    assert(list.__LIST_TAIL__ == &a);                 \
    assert(a.__LIST_NEXT__ == (void *)&list);         \
    assert(a.__LIST_PREV__ == (void *)&list);         \
    cnt_assert(1)

#define assert_2(list, a, b)                  \
    assert(!VTSS_LINKED_LIST_EMPTY(list));    \
    assert(!VTSS_LINKED_LIST_ONE(list));      \
    assert(list.__LIST_HEAD__ == &a);         \
    assert(list.__LIST_TAIL__ == &b);         \
    assert(a.__LIST_NEXT__ == &b);            \
    assert(a.__LIST_PREV__ == (void *)&list); \
    assert(b.__LIST_NEXT__ == (void *)&list); \
    assert(b.__LIST_PREV__ == &a);            \
    cnt_assert(2)

#define assert_3(list, a, b, c)               \
    assert(!VTSS_LINKED_LIST_EMPTY(list));    \
    assert(!VTSS_LINKED_LIST_ONE(list));      \
    assert(list.__LIST_HEAD__ == &a);         \
    assert(list.__LIST_TAIL__ == &c);         \
    assert(a.__LIST_NEXT__ == &b);            \
    assert(a.__LIST_PREV__ == (void *)&list); \
    assert(b.__LIST_NEXT__ == &c);            \
    assert(b.__LIST_PREV__ == &a);            \
    assert(c.__LIST_NEXT__ == (void *)&list); \
    assert(c.__LIST_PREV__ == &b);            \
    cnt_assert(3)

#define assert_unlinked(a) \
        assert(!VTSS_LINKED_LIST_IS_LINKED(a))

#define make_list_0(list) \
    VTSS_LINKED_LIST_INIT(list)

#define make_list_1(list, a) \
    make_list_0(list); \
    VTSS_LINKED_LIST_PUSH_BACK(list, a)

#define make_list_2(list, a, b) \
    make_list_1(list, a); \
    VTSS_LINKED_LIST_PUSH_BACK(list, b)

#define make_list_3(list, a, b, c) \
    make_list_2(list, a, b); \
    VTSS_LINKED_LIST_PUSH_BACK(list, c)


int main(int argc, const char *argv[])
{
    int cnt = 0;
    list_t list;
    item_t a, b, c;
    item_t *i;

    /////////////////////////////////////////////////////////
    VTSS_LINKED_LIST_INIT(list);
    assert_0(list);

    VTSS_LINKED_LIST_PUSH_BACK(list, a);
    assert_1(list, a);

    VTSS_LINKED_LIST_PUSH_BACK(list, b);
    assert_2(list, a, b);

    VTSS_LINKED_LIST_PUSH_BACK(list, c);
    assert_3(list, a, b, c);

    VTSS_LINKED_LIST_POP_BACK(list);
    assert_2(list, a, b);

    VTSS_LINKED_LIST_POP_BACK(list);
    assert_1(list, a);

    VTSS_LINKED_LIST_POP_BACK(list);
    assert_0(list);

    /////////////////////////////////////////////////////////
    VTSS_LINKED_LIST_INIT(list);
    assert_0(list);

    VTSS_LINKED_LIST_PUSH_FRONT(list, a);
    assert_1(list, a);

    VTSS_LINKED_LIST_PUSH_FRONT(list, b);
    assert_2(list, b, a);

    VTSS_LINKED_LIST_PUSH_FRONT(list, c);
    assert_3(list, c, b, a);

    VTSS_LINKED_LIST_POP_FRONT(list);
    assert_2(list, b, a);

    VTSS_LINKED_LIST_POP_FRONT(list);
    assert_1(list, a);

    VTSS_LINKED_LIST_POP_FRONT(list);
    assert_0(list);

    /////////////////////////////////////////////////////////
    make_list_2(list, a, b);
    VTSS_LINKED_LIST_INSERT_BEFORE(list, &a, c);
    assert_3(list, c, a, b);

    make_list_2(list, a, b);
    VTSS_LINKED_LIST_INSERT_BEFORE(list, &b, c);
    assert_3(list, a, c, b);

    make_list_2(list, a, b);
    VTSS_LINKED_LIST_INSERT_BEFORE(list, (item_t*)&list, c);
    assert_3(list, a, b, c);

    make_list_0(list);
    VTSS_LINKED_LIST_INSERT_BEFORE(list, (item_t*)&list, c);
    assert_1(list, c);

    /////////////////////////////////////////////////////////
    make_list_3(list, a, b, c);
    VTSS_LINKED_LIST_DELETE(list, &a);
    assert_2(list, b, c);

    make_list_3(list, a, b, c);
    VTSS_LINKED_LIST_DELETE(list, &b);
    assert_2(list, a, c);

    make_list_3(list, a, b, c);
    VTSS_LINKED_LIST_DELETE(list, &c);
    assert_2(list, a, b);

    make_list_2(list, a, b);
    VTSS_LINKED_LIST_DELETE(list, &a);
    assert_1(list, b);

    make_list_2(list, a, b);
    VTSS_LINKED_LIST_DELETE(list, &b);
    assert_1(list, a);

    make_list_1(list, a);
    VTSS_LINKED_LIST_DELETE(list, &a);
    assert_0(list);

    /////////////////////////////////////////////////////////
    make_list_3(list, a, b, c);
    VTSS_LINKED_LIST_UNLINK(a);
    assert_2(list, b, c);
    assert_unlinked(a);

    make_list_3(list, a, b, c);
    VTSS_LINKED_LIST_UNLINK(b);
    assert_2(list, a, c);
    assert_unlinked(b);

    make_list_3(list, a, b, c);
    VTSS_LINKED_LIST_UNLINK(c);
    assert_2(list, a, b);
    assert_unlinked(c);

    make_list_2(list, a, b);
    VTSS_LINKED_LIST_UNLINK(a);
    assert_1(list, b);
    assert_unlinked(a);

    make_list_2(list, a, b);
    VTSS_LINKED_LIST_UNLINK(b);
    assert_1(list, a);
    assert_unlinked(b);

    make_list_1(list, a);
    VTSS_LINKED_LIST_UNLINK(a);
    assert_0(list);
    assert_unlinked(a);

    return 0;
}

