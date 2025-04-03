/* *****************************************************************************
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
 **************************************************************************** */

#include <type_traits>  // needed for std::aligned_storage
#include <vtss/basics/assert.hxx>
#include <vtss/basics/type_traits.hxx>

namespace vtss {
namespace variant_details {

// -----------------------------------------------------------------------------
// Static calculate max value n 'SIZE...'
template <size_t... SIZE>
struct max;

template <size_t SIZE>
struct max<SIZE> {
    static constexpr size_t value = SIZE;
};

template <size_t SIZE1, size_t SIZE2, size_t... SIZEs>
struct max<SIZE1, SIZE2, SIZEs...> {
    static constexpr size_t value = SIZE1 >= SIZE2 ? max<SIZE1, SIZEs...>::value
                                                   : max<SIZE2, SIZEs...>::value;
};

// -----------------------------------------------------------------------------
// Find the index of type 'TYPE' in 'TYPEs...' (1-based index, 0 is used for no
// value)
template <unsigned IDX, typename TYPE, typename... TYPEs>
struct index_by_type_;

template <unsigned IDX, typename TYPE>
struct index_by_type_<IDX, TYPE> {
    static constexpr unsigned value = 0;
};

template <unsigned IDX, typename TYPE, typename HEAD, typename... TAIL>
struct index_by_type_<IDX, TYPE, HEAD, TAIL...> {
    static constexpr unsigned value =
            is_same<TYPE, HEAD>::value
                    ? IDX
                    : index_by_type_<IDX + 1, TYPE, TAIL...>::value;
};

template <typename TYPE, typename... TYPEs>
struct index_by_type : public index_by_type_<1, TYPE, TYPEs...> {};

// -----------------------------------------------------------------------------
// Check if type TYPE can be found in ARGS...
template <typename TYPE, typename... ARGS>
struct type_exists;

// End of recursion, type not found
template <typename TYPE>
struct type_exists<TYPE> {
    static constexpr bool value = false;
};

// Recursive step, see if TYPE == HEAD, other wise continue searching in TAIL...
template <typename TYPE, typename HEAD, typename... TAIL>
struct type_exists<TYPE, HEAD, TAIL...> {
    static constexpr bool value = is_same<TYPE, HEAD>::value
                                          ? true
                                          : type_exists<TYPE, TAIL...>::value;
};

// -----------------------------------------------------------------------------
// Check if ARGS... include duplicated types
template <typename... ARGS>
struct type_duplicates;

// End of recursion, no duplicates found
template <>
struct type_duplicates<> {
    static constexpr bool value = false;
};

// Recusrive step, see if the type HEAD can be found in TAIL...
template <typename HEAD, typename... TAIL>
struct type_duplicates<HEAD, TAIL...> {
    static constexpr bool value = type_exists<HEAD, TAIL...>::value
                                          ? true
                                          : type_duplicates<TAIL...>::value;
};

// -----------------------------------------------------------------------------
// Types in the Variant is accesed by index (IDX). The index starts from 1, as 0
// is reserved for NO-TYPE!
template <unsigned IDX, typename... TYPEs>
struct variant_helper_;

// Do nothing if type is not found
template <unsigned IDX>
struct variant_helper_<IDX> {
    inline static void destroy(unsigned idx, void *data) {}
    inline static void copy(unsigned idx, const void *from, void *to) {}
};

// Itereate until IDX is found, and destroy/copy/move/...
template <unsigned IDX, typename TYPE, typename... TYPEs>
struct variant_helper_<IDX, TYPE, TYPEs...> {
    inline static void destroy(unsigned idx, void *data) {
        if (idx == IDX)
            reinterpret_cast<TYPE *>(data)->~TYPE();
        else
            variant_helper_<IDX + 1, TYPEs...>::destroy(idx, data);
    }

    inline static void copy(unsigned idx, const void *from, void *to) {
        if (idx == IDX)
            new (to) TYPE(*reinterpret_cast<const TYPE *>(from));
        else
            variant_helper_<IDX + 1, TYPEs...>::copy(idx, from, to);
    }
};

template <typename... TYPEs>
struct variant_helper;

template <typename... TYPEs>
struct variant_helper {
    inline static void destroy(unsigned idx, void *data) {
        return variant_helper_<1, TYPEs...>::destroy(idx, data);
    }

    inline static void copy(unsigned idx, const void *from, void *to) {
        return variant_helper_<1, TYPEs...>::copy(idx, from, to);
    }
};
}

// The purpose of a Variant is to act as a union, but allow to store non-POD
// data. To do this, the Variant must ensure that the constructor/destructor are
// called when needed.
//
// This implementation of the Variant does not allow the same type to be present
// more that once, and it will assert if the user try to access a different type
// that the one stored.
//
// The Variant type will have a 'unsigned' type beside the union it-self to keep
// track on the what type is currently stored in the "union" data.
template <typename... TYPEs>
struct Variant {
    static constexpr size_t data_size =
            variant_details::max<sizeof(TYPEs)...>::value;
    static constexpr size_t data_align =
            variant_details::max<alignof(TYPEs)...>::value;

    typedef variant_details::variant_helper<TYPEs...> helper;

  private:
    // The type of "data" is identified by the index in the 'TYPEs...' list.
    // Indexing is '1' based, as '0' is used for NO-TYPE!
    unsigned type_idx_;

    // Enough storage for the biggest member of the Variant
    typename std::aligned_storage<data_size, data_align>::type data;

  public:
    constexpr Variant() : type_idx_(0) {
        static_assert(!variant_details::type_duplicates<TYPEs...>::value,
                      "Variant is not allowed to include duplicated types");
    }

    Variant(const Variant<TYPEs...> &from) : type_idx_(from.type_idx_) {
        static_assert(!variant_details::type_duplicates<TYPEs...>::value,
                      "Variant is not allowed to include duplicated types");
        helper::copy(from.type_idx_, &from.data, &data);
    }

    ~Variant() { helper::destroy(type_idx_, &data); }

    // '0' means invalid!
    unsigned type_idx() const { return type_idx_; }

    bool valid() const { return type_idx_ != 0; }

    template <typename TYPE>
    bool is() const {
        static_assert(variant_details::type_exists<TYPE, TYPEs...>::value,
                      "Type is not part of Variant");

        return type_idx_ == variant_details::index_by_type<TYPE, TYPEs...>::value;
    }

    template <typename TYPE>
    TYPE &get() {
        static_assert(variant_details::type_exists<TYPE, TYPEs...>::value,
                      "Type is not part of Variant");

        bool b = (type_idx_ ==
                  variant_details::index_by_type<TYPE, TYPEs...>::value);
        VTSS_ASSERT(b);

        return *reinterpret_cast<TYPE *>(&data);
    }

    template <typename TYPE, typename... Args>
    void emplace(Args &&... args) {
        static_assert(variant_details::type_exists<TYPE, TYPEs...>::value,
                      "Type is not part of Variant");

        helper::destroy(type_idx_, &data);
        type_idx_ = 0;

        new (&data) TYPE(vtss::forward<Args>(args)...);
        type_idx_ = variant_details::index_by_type<TYPE, TYPEs...>::value;
    }

    template <typename TYPE>
    void set(const TYPE &t) {
        static_assert(variant_details::type_exists<TYPE, TYPEs...>::value,
                      "Type is not part of Variant");

        helper::destroy(type_idx_, &data);
        type_idx_ = 0;

        new (&data) TYPE(t);

        type_idx_ = variant_details::index_by_type<TYPE, TYPEs...>::value;
    }

    template <typename TYPE>
    void set_type() {
        static_assert(variant_details::type_exists<TYPE, TYPEs...>::value,
                      "Type is not part of Variant");
        static_assert(is_pod<TYPE>::value,
                      "set_type may only be used with POD");

        helper::destroy(type_idx_, &data);
        type_idx_ = variant_details::index_by_type<TYPE, TYPEs...>::value;
    }
};
}
