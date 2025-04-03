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

#ifndef _VTSS_INTRUSIVE_LIST_H_
#define _VTSS_INTRUSIVE_LIST_H_

// Example: see ./vtss_appl/util/.test/linkedlist.c

/*lint --e{10} */
/*lint --e{40} */
/*lint --e{63} */
/*lint --e{19} */

#define VTSS_CAST_ASIGN(X, Y) X = (__typeof__(X))(Y)

#define VTSS_LINKED_LIST_TYPE(T) \
    struct T##_list_t_ {         \
        struct T* __LIST_HEAD__; \
        struct T* __LIST_TAIL__; \
    }

#define VTSS_LINKED_LIST_DECL_ITEM(T) \
    struct T* __LIST_PREV__;          \
    struct T* __LIST_NEXT__;          \
    struct T##_list_t_* __LIST__;

#define VTSS_LINKED_LIST_INIT(list)                     \
    do {                                                \
        VTSS_CAST_ASIGN((list).__LIST_HEAD__, &(list)); \
        VTSS_CAST_ASIGN((list).__LIST_TAIL__, &(list)); \
    } while (0)

#define VTSS_LINKED_LIST_ITEM_INIT(item) \
    do {                                 \
        (item).__LIST_PREV__ = 0;        \
        (item).__LIST_NEXT__ = 0;        \
        (item).__LIST__ = 0;             \
    } while (0)

#define VTSS_LINKED_LIST_EMPTY(list)                                      \
    ((list).__LIST_HEAD__ == (__typeof__((list).__LIST_HEAD__))(&list) || \
     (list).__LIST_HEAD__ == 0 ||                                         \
     (list).__LIST_TAIL__ == (__typeof__((list).__LIST_TAIL__))(&list) || \
     (list).__LIST_TAIL__ == 0)

#define VTSS_LINKED_LIST_ONE(list)          \
    ((!VTSS_LINKED_LIST_EMPTY(list)) &&     \
     (list).__LIST_HEAD__->__LIST_NEXT__ == \
             (__typeof__((list).__LIST_HEAD__->__LIST_NEXT__))(&list))

#define VTSS_LINKED_LIST_BACK(list)                                            \
    (((list).__LIST_TAIL__ == 0) ? (__typeof__((list).__LIST_TAIL__))(&(list)) \
                                 : (list).__LIST_TAIL__)

#define VTSS_LINKED_LIST_FRONT(list)                                           \
    (((list).__LIST_HEAD__ == 0) ? (__typeof__((list).__LIST_HEAD__))(&(list)) \
                                 : (list).__LIST_HEAD__)

#define VTSS_LINKED_LIST_INSERT_IN_EMPTY(list, item) \
    (list).__LIST_HEAD__ = &item;                    \
    (list).__LIST_TAIL__ = &item;                    \
    VTSS_CAST_ASIGN((item).__LIST_NEXT__, (&list));  \
    VTSS_CAST_ASIGN((item).__LIST_PREV__, (&list))

#define VTSS_LINKED_LIST_PUSH_BACK(list, item)               \
    do {                                                     \
        (item).__LIST__ = &(list);                           \
        if (VTSS_LINKED_LIST_EMPTY(list)) {                  \
            VTSS_LINKED_LIST_INSERT_IN_EMPTY(list, item);    \
        } else {                                             \
            __typeof__(&(item)) last = (list).__LIST_TAIL__; \
            last->__LIST_NEXT__ = &(item);                   \
            (list).__LIST_TAIL__ = &(item);                  \
            (item).__LIST_PREV__ = last;                     \
            VTSS_CAST_ASIGN((item).__LIST_NEXT__, (&list));  \
        }                                                    \
    } while (0)

#define VTSS_LINKED_LIST_POP_BACK(list)                                      \
    do {                                                                     \
        if (VTSS_LINKED_LIST_EMPTY(list)) {                                  \
            ;                                                                \
        } else if (VTSS_LINKED_LIST_ONE(list)) {                             \
            __typeof__((list).__LIST_TAIL__) last = (list).__LIST_TAIL__;    \
            VTSS_LINKED_LIST_INIT(list);                                     \
            VTSS_LINKED_LIST_ITEM_INIT(*last);                               \
        } else {                                                             \
            __typeof__((list).__LIST_TAIL__) last = (list).__LIST_TAIL__;    \
            __typeof__((list).__LIST_TAIL__) new_last = last->__LIST_PREV__; \
            VTSS_CAST_ASIGN(new_last->__LIST_NEXT__, (&list));               \
            VTSS_CAST_ASIGN((list).__LIST_TAIL__, new_last);                 \
            VTSS_LINKED_LIST_ITEM_INIT(*last);                               \
        }                                                                    \
    } while (0)

#define VTSS_LINKED_LIST_PUSH_FRONT(list, item)                               \
    do {                                                                      \
        (item).__LIST__ = &(list);                                            \
        if (VTSS_LINKED_LIST_EMPTY(list)) {                                   \
            VTSS_LINKED_LIST_INSERT_IN_EMPTY(list, item);                     \
        } else {                                                              \
            __typeof__((list).__LIST_HEAD__) old_head = (list).__LIST_HEAD__; \
            (item).__LIST_NEXT__ = old_head;                                  \
            VTSS_CAST_ASIGN((item).__LIST_PREV__, &list);                     \
            VTSS_CAST_ASIGN((list).__LIST_HEAD__, &item);                     \
            old_head->__LIST_PREV__ = &item;                                  \
        }                                                                     \
    } while (0)

#define VTSS_LINKED_LIST_POP_FRONT(list)                                     \
    do {                                                                     \
        if (VTSS_LINKED_LIST_EMPTY(list)) {                                  \
            ;                                                                \
        } else if (VTSS_LINKED_LIST_ONE(list)) {                             \
            __typeof__((list).__LIST_TAIL__) last = (list).__LIST_TAIL__;    \
            VTSS_LINKED_LIST_INIT(list);                                     \
            VTSS_LINKED_LIST_ITEM_INIT(*last);                               \
        } else {                                                             \
            __typeof__((list).__LIST_TAIL__) head = (list).__LIST_HEAD__;    \
            __typeof__((list).__LIST_TAIL__) new_head = head->__LIST_NEXT__; \
            VTSS_CAST_ASIGN(new_head->__LIST_PREV__, (&list));               \
            VTSS_CAST_ASIGN((list).__LIST_HEAD__, new_head);                 \
            VTSS_LINKED_LIST_ITEM_INIT(*head);                               \
        }                                                                    \
    } while (0)

#define VTSS_LINKED_LIST_INSERT_BEFORE(list, ptr, item)                       \
    do {                                                                      \
        (item).__LIST__ = &(list);                                            \
        if ((__typeof__(&list))(ptr) == &list) {                              \
            VTSS_LINKED_LIST_PUSH_BACK(list, item);                           \
        } else if (VTSS_LINKED_LIST_FRONT(list) == ptr) {                     \
            VTSS_LINKED_LIST_PUSH_FRONT(list, item);                          \
        } else {                                                              \
            __typeof__((list).__LIST_HEAD__) old_prev = (ptr)->__LIST_PREV__; \
            old_prev->__LIST_NEXT__ = (&(item));                              \
            (ptr)->__LIST_PREV__ = (&(item));                                 \
            (item).__LIST_NEXT__ = (ptr);                                     \
            (item).__LIST_PREV__ = old_prev;                                  \
        }                                                                     \
    } while (0)

#define VTSS_LINKED_LIST_DELETE(list, ptr)                                \
    do {                                                                  \
        if (VTSS_LINKED_LIST_FRONT(list) == ptr) {                        \
            VTSS_LINKED_LIST_POP_FRONT(list);                             \
        } else if (VTSS_LINKED_LIST_BACK(list) == ptr) {                  \
            VTSS_LINKED_LIST_POP_BACK(list);                              \
        } else {                                                          \
            __typeof__((list).__LIST_HEAD__) prev = (ptr)->__LIST_PREV__; \
            __typeof__((list).__LIST_HEAD__) next = (ptr)->__LIST_NEXT__; \
            prev->__LIST_NEXT__ = next;                                   \
            next->__LIST_PREV__ = prev;                                   \
        }                                                                 \
        VTSS_LINKED_LIST_ITEM_INIT(*ptr);                                 \
    } while (0)

#define VTSS_LINKED_LIST_IS_LINKED(item) ((item).__LIST__)

#define VTSS_LINKED_LIST_UNLINK(item)                               \
    do {                                                            \
        if (VTSS_LINKED_LIST_IS_LINKED(item)) {                     \
            VTSS_LINKED_LIST_DELETE((*(item).__LIST__), (&(item))); \
        }                                                           \
    } while (0)

#define VTSS_LINKED_LIST_FOREACH(list, iter)              \
    for (iter = VTSS_LINKED_LIST_FRONT(list);             \
         iter != (__typeof__(list.__LIST_HEAD__))(&list); \
         iter = iter->__LIST_NEXT__)


#endif /* _VTSS_INTRUSIVE_LIST_H_ */
