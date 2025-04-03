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

#ifndef __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_DEPEND_N_HXX__
#define __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_DEPEND_N_HXX__

#include "vtss/basics/api_types.h"  // For mesa_rc and friends
#include "vtss/basics/meta.hxx"
#include <functional>  // For std::function

namespace vtss {
namespace iterator_compose_depend_N {

template <typename Tuple>
struct TuplePopHead;

template <typename A, typename... Args>
struct TuplePopHead<std::tuple<A, Args...>> {
    typedef std::tuple<Args...> type;
};

template <typename Tuple>
struct TupleHead;

template <typename A, typename... Args>
struct TupleHead<std::tuple<A, Args...>> {
    typedef A type;
};

template <size_t N, typename Vector, typename Tuple>
struct FirstArgsN_;

template <typename Vector, typename Tuple>
struct FirstArgsN_<0, Vector, Tuple> {
    typedef Vector type;
};

template <size_t N, typename Vector, typename Tuple>
struct FirstArgsN_ {
    typedef typename TuplePopHead<Tuple>::type TupleTail;
    typedef typename TupleHead<Tuple>::type TupleHead;

    typedef typename meta::push_head<
        TupleHead,
        typename FirstArgsN_<N - 1, Vector, TupleTail>::type
    >::type type;
};

template <size_t N, typename Tuple>
struct FirstArgsN {
    typedef typename FirstArgsN_<N, meta::vector<>, Tuple>::type type;
};

template <typename A, typename Vector>
struct CalcFunctionPointer_;

template <typename A, typename... Args>
struct CalcFunctionPointer_<A, meta::vector<Args...>> {
    typedef std::function<mesa_rc(const A *, A *, Args...)> type;
};

template <size_t N, typename Tuple, typename A>
struct CalcFunctionPointer {
    typedef typename FirstArgsN<N, Tuple>::type Vector;
    typedef typename CalcFunctionPointer_<A, Vector>::type type;
};

template <size_t N, size_t I>
struct Invoke_;

template <size_t N, size_t I>
struct Invoke_ {
    template<typename A, typename F, typename Tuple, typename... Args>
    mesa_rc operator()(F& f, Tuple& t, const A *p, A *n, Args... args) {
        Invoke_<N - 1, I + 1> invoke_;
        return invoke_(f, t, p, n, args..., std::get<I>(t));
    }
};

template <size_t I>
struct Invoke_<0, I> {
    template<typename A, typename F, typename Tuple, typename... Args>
    mesa_rc operator()(F& f, Tuple& t, const A *p, A *n, Args... args) {
        return f(p, n, args...);
    }
};

template<size_t N, typename A, typename F, typename Tuple>
mesa_rc invoke(F& f, Tuple& t, const A *p, A *n) {
    Invoke_<N, 0> i;
    return i(f, t, p, n);
}

template <size_t N, typename Depend, typename A, typename... T>
struct Itr;

// base case, just a single iterator
template <size_t N, typename Depend, typename A>
struct Itr<N, Depend, A> {
    // Store the iterator function pointer for this dimention
    typedef typename CalcFunctionPointer<N, Depend, A>::type F;
    F f;

    template <typename Head>
    Itr(Head a) : f(a) {}

    template <typename... Args>
    mesa_rc call(Depend &depend, const A *prev, A *next) {
        // just call the provided single dimention iterator with the dependend
        // arguments.
        return invoke<N, A>(f, depend, prev, next);
    }

    template <typename... Args>
    mesa_rc call_first(Depend &depend, const A *prev, A *next) {
        return invoke<N, A>(f, depend, nullptr, next);
    }
};

template <size_t N, typename Depend, typename A, typename... T>
struct Itr : public Itr<N + 1, Depend, T...> {
    // Store the iterator function pointer for this dimention
    typedef typename CalcFunctionPointer<N, Depend, A>::type F;
    F f;

    // Constructor, needs to be initialized with iterator function pointer for
    // every index.
    template <typename Head, typename... Tail>
    Itr(Head h, Tail... tail) : Itr<N + 1, Depend, T...>(tail...), f(h) {}

    template <typename... Args>
    mesa_rc call(Depend &depend, const A *prev, A *next, Args... argn) {
        mesa_rc rc;

        if (!prev) {
            // Call this dimention as a get-first
            rc = invoke<N, A>(f, depend, nullptr, &(std::get<N>(depend)));

            // Stop if get-first failed
            if (rc != MESA_RC_OK)
                return rc;

        } else {
            // Lets try to use the provided prev values and see if the nested
            // iterators can use it.
            std::get<N>(depend) = *prev;

            // Success if the nested iterator has succeded.
            rc = Itr<N + 1, Depend, T...>::template call(depend, argn...);
            if (rc == MESA_RC_OK) {
                *next = *prev;
                return rc;
            }

            // The provided prev values could not be used further, we must
            // therefor increment the iterator at this dimention.
            mesa_rc rc = invoke<N, A>(f, depend, prev, &std::get<N>(depend));

            // Stop if get-next failed
            if (rc != MESA_RC_OK)
                return rc;
        }

        // keep the next value up-to-date
        *next = std::get<N>(depend);

        // At this point, the value in depend.get<N> is either a get-first of a
        // *prev which has been incremented. In either case we must force nested
        // iterators to be called as get-first.

        // We must continue calling the iterator at this dimention until the
        // nested iterators succedes.

        while (true) {
            // success if the nested iterator has succeded.
            rc = Itr<N + 1, Depend, T...>::template call_first(depend, argn...);
            if (rc == MESA_RC_OK)
                return rc;

            // call get-next in this dimention
            mesa_rc rc = invoke<N, A>(f, depend, next, &std::get<N>(depend));

            // stop when the iterator of this dimention has finished
            if (rc != MESA_RC_OK)
                return rc;

            // keep the next value up-to-date
            *next = std::get<N>(depend);
        }

        assert(0);
        return MESA_RC_ERROR;
    }

    template <typename... Args>
    mesa_rc call_first(Depend &depend, const A *prev, A *next, Args... argn) {
        return call(depend, nullptr, next, argn...);
    }
};

}  // namespace iterator_compose_depend_N


template <typename... T>
struct IteratorComposeDependN {
    template <typename... Args>
    IteratorComposeDependN(Args... args) : impl(args...) {}

    template <typename... Args>
    mesa_rc operator()(Args... args) {
        std::tuple<T...> depend;
        return impl.template call(depend, args...);
    }

  private:
    iterator_compose_depend_N::Itr<0, std::tuple<T...>, T...> impl;
};

}  // namespace vtss

#undef DEPEND_TYPE_LIST

#endif  // __VTSS_BASICS_EXPOSE_SNMP_ITERATOR_COMPOSE_DEPEND_N_HXX__
