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

#ifndef __VTSS_BASICS_NOTIFICATIONS_SUBJECT_READ_ONLY_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_SUBJECT_READ_ONLY_HXX__

#include <vtss/basics/intrusive_list.hxx>
#include <vtss/basics/notifications/subject-base.hxx>
#include <vtss/basics/notifications/lock-global-subject.hxx>

namespace vtss {
namespace notifications {

template <typename T>
struct SubjectReadOnly : public SubjectBase {
    constexpr SubjectReadOnly() {}
    constexpr explicit SubjectReadOnly(const T& v) : value_(v) {}
    ~SubjectReadOnly() {
        signal(); }

    T get() const {
        LockGlobalSubject lock(__FILE__, __LINE__);
        return value_;
    }

    T get(Event& t) {
        LockGlobalSubject lock(__FILE__, __LINE__);
        attach_(t);
        return value_;
    }

    T get(Event *e, Event& t) {
        LockGlobalSubject lock(__FILE__, __LINE__);
        if (e == &t)
            attach_(t);
        return value_;
    }

  protected:
    void set(const T& value, bool force = false) {
        LockGlobalSubject lock(__FILE__, __LINE__);
        if (force || value != SubjectReadOnly<T>::value_) {
            SubjectReadOnly<T>::value_ = value;
            SubjectReadOnly<T>::signal();
        }
    }

    T value_;
    intrusive::List<Event> event_list;
};

}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_SUBJECT_READ_ONLY_HXX__
