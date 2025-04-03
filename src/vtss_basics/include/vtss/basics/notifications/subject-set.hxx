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

#ifndef __VTSS_BASICS_NOTIFICATIONS_SUBJECT_SET_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_SUBJECT_SET_HXX__

#include <vtss/basics/set.hxx>
#include <vtss/basics/notifications/subject-base.hxx>
#include <vtss/basics/notifications/table-observer.hxx>
#include <vtss/basics/notifications/table-observer-pool.hxx>
#include <vtss/basics/notifications/table-observer-values.hxx>
#include <vtss/basics/notifications/lock-global-subject.hxx>

namespace vtss {
namespace notifications {

template <class K>
struct SubjectSet {
    typedef TableObserver<K> Observer;

    mesa_rc observer_new(notifications::Event* ev) {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        return observers.observer_new(ev);
    }

    mesa_rc observer_new(notifications::Event* ev, Observer& o) {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        mesa_rc rc = observers.observer_new(ev);
        if (rc != MESA_RC_OK) return rc;

        // Initially all existing objects are considered as added
        o.clear();
        for (const auto& e : data_) o.add(e);

        return MESA_RC_OK;
    }

    mesa_rc observer_del(notifications::Event* ev) {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        return observers.observer_del(ev);
    }

    mesa_rc observer_get(notifications::Event* ev, Observer& o) {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        return observers.observer_get(ev, o);
    }

    mesa_rc observer_mask_key(notifications::Event* ev, EventType::E et,
                              const K& k) {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        return observers.observer_mask_key(ev, et, k);
    }

    mesa_rc get(const K& k) const {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        auto itr = data_.find(k);
        if (itr == data_.end()) return MESA_RC_ERROR;
        return MESA_RC_OK;
    }

    mesa_rc get_first(K& k) const {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        auto itr = data_.begin();
        if (itr == data_.end()) return MESA_RC_ERROR;
        k = *itr;
        return MESA_RC_OK;
    }

    mesa_rc get_next(K& k) const {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        auto itr = data_.greater_than(k);
        if (itr == data_.end()) return MESA_RC_ERROR;
        k = *itr;
        return MESA_RC_OK;
    }

    mesa_rc set(const K& k) {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        auto itr = data_.find(k);
        if (itr == data_.end()) {  // this is an add operation
            (void)data_.insert(k);
            observers.event_add(k);
        }
        return MESA_RC_OK;
    }

    mesa_rc del(const K& k) {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        auto itr = data_.find(k);
        if (itr == data_.end()) return MESA_RC_ERROR;
        data_.erase(itr);
        observers.event_del(k);
        return MESA_RC_OK;
    }

    void clear() {
        notifications::LockGlobalSubject lock(__FILE__, __LINE__);
        data_.clear();
    }

    size_t size() const { return data_.size(); }

  protected:
    Set<K> data_;
    TableObserverPool<K, Observer> observers;
};

}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_SUBJECT_SET_HXX__
