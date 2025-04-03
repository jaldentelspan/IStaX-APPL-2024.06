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

#ifndef _OPTIONAL_H_
#define _OPTIONAL_H_

namespace vtss {
template<typename Type> struct Optional {
    Optional() : valid_(false) { }
    explicit Optional(const Type& t) : valid_(true), data_(t) { }
    Optional(const Optional<Type>& rhs) {
        valid_ = rhs.valid();

        if (valid_) {
            data_ = rhs.get();
        }
    }

    Optional<Type>& operator=(const Type& v) {
        valid_ = true;
        data_ = v;
        return *this;
    }

    Optional<Type>& operator=(const Optional<Type>& rhs) {
        valid_ = rhs.valid();

        if (valid_) {
            data_ = rhs.get();
        }
        return *this;
    }

    bool operator==(const Optional<Type>& rhs) const {
        if (valid_ != rhs.valid())
            return false;

        if (get() != rhs.get())
            return false;

        return true;
    }

    bool operator!=(const Optional<Type>& rhs) const {
        if (valid_ != rhs.valid())
            return true;
        return false;
    }

    Type const& get() const { return data_; }
    Type&       get() { return data_; }

    Type const* operator ->() const { return &data_; }
    Type*       operator ->() { return &data_; }

    Type const& operator *() const { return data_; }
    Type&       operator *() { return data_; }

    Type const* get_ptr() const { return &data_; }
    Type*       get_ptr() { return &data_; }

    bool        valid() const { return valid_; }
    void        clear() { valid_ = false; }

  private:
    bool valid_;
    Type data_;
};
}  // namespace vtss

#endif /* _OPTIONAL_H_ */
