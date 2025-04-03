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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_N_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_N_HXX__

#include "vtss/basics/api_types.h" // For mesa_rc and friends
#include <functional>              // For std::function

namespace vtss {
template <typename A>
struct CalcFunctionPointer {
    typedef std::function<mesa_rc(const A *, A *)> type;
};

/*
 * This template allows for increasing a multi-index table's indices by the use
 * of multiple single index iterator functions - one per index.
 *
 * The single index iterator must return MESA_RC_ERROR when an index is about to
 * overflow, and MESA_RC_OK in other cases.
 *
 * Example of an iterator function that accepts integers in range [33; 44]:
 *    mesa_rc my_itr_a(const int *prev, int *next) {
 *        if (!prev || *prev < 33) {
 *            // Get first
 *            *next = 33
 *        } else if (*prev < 44) {
 *            // Get next
 *            *next = *prev + 1;
 *        } else {
 *            // Overflow
 *            return MESA_RC_ERROR;
 *        }
 *
 *        return MESA_RC_OK;
 *    }
 *
 * In order to instantiate the multi iterator template, do the following:
 *    mesa_rc itr_abc(const int *a_prev, int *a_next,
 *                    const int *b_prev, int *b_next,
 *                    const int *c_prev, int *c_next) {
 *        IteratorComposeN<int, int, int> itr(my_itr_a, my_itr_b, my_itr_c);
 *        return itr(a_prev, a_next, b_prev, b_next, c_prev, c_next);
 *    }
 */
template <typename... T>
struct IteratorComposeN;

// base case, just a single iterator
template <typename A>
struct IteratorComposeN<A> {
    typename CalcFunctionPointer<A>::type f;
    IteratorComposeN(typename CalcFunctionPointer<A>::type a) : f(a) {}

    template <typename... Args>
    mesa_rc operator()(const A *a_prev, A *a_next) {
        return f(a_prev, a_next);
    }

  protected:
    template <typename... Args>
    mesa_rc first(const A *b_prev, A *b_next) {
        return f(nullptr, b_next);
    }
};

// combine two iterators, recursively
template <typename A, typename B, typename... T>
struct IteratorComposeN<A, B, T...> : public IteratorComposeN<B, T...> {
    // Store the iterator function pointer for this dimension
    typename CalcFunctionPointer<A>::type f;

    // Constructor, needs to be initialized with iterator function pointer for
    // every index.
    IteratorComposeN(typename CalcFunctionPointer<A>::type a,
                     typename CalcFunctionPointer<B>::type b,
                     typename CalcFunctionPointer<T>::type... n)
        : IteratorComposeN<B, T...>(b, n...), f(a) {}

    // Implements the actual multi-index iterator. It does that by combining
    // iterator (A) with (B, N...), where N represents zero or more indices.
    template <typename... Args>
    mesa_rc operator()(const A *a_prev, A *a_next, const B *b_prev, B *b_next,
                       Args... argn) {
        mesa_rc rc;

        // find first valid value for a_next
        A   a_first{0};

        rc = f(nullptr, &a_first);
        if (rc != 0) {
            return rc;
        }

        // Get first
        if ((!a_prev) || (*a_prev < a_first)) {
            *a_next = a_first;
            return IteratorComposeN<B, T...>::template first(b_prev, b_next, argn...);
        }

        // Get next on (B, N...)
        if (IteratorComposeN<B, T...>::template operator()(b_prev, b_next,
                                                           argn...) == 0) {
            // Succeeded to get first or next on (B, N...), that is, no overflow
            // on (B, N...).
            // A doesn't need to restart. Keep previous.
            *a_next = *a_prev;
        } else {
            // Overflow on (B, N...)
            // Increase A
            if ((rc = f(a_prev, a_next)) == 0) {
                // No overflow on A, so the table is not yet depleted.
                // Restart (B, N...)
                rc = IteratorComposeN<B, T...>::template first(b_prev, b_next, argn...);
            }
        }

        return rc;
    }

  protected:
    template <typename... Args>
    mesa_rc first(const A *b_prev, A *b_next, Args... argn) {
        mesa_rc rc = f(nullptr, b_next);
        if (rc != 0) return rc;
        return IteratorComposeN<B, T...>::template first(argn...);
    }
};
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_N_HXX__
