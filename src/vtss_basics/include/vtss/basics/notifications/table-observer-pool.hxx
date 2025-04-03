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

#ifndef __VTSS_BASICS_NOTIFICATIONS_TABLE_OBSERVER_POOL_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_TABLE_OBSERVER_POOL_HXX__

#include <vtss/basics/map.hxx>
#include <vtss/basics/intrusive_list.hxx>
#include <vtss/basics/notifications/event.hxx>
#include <vtss/basics/notifications/event-type.hxx>

namespace vtss {
namespace notifications {

template <typename Key, typename Observer>
struct TableObserverPool {
    void event_add(const Key &k) {
        for (auto &e : observers_) e.second.add(k);
        signal();
    }

    void event_mod(const Key &k) {
        for (auto &e : observers_) e.second.mod(k);
        signal();
    }

    void event_del(const Key &k) {
        for (auto &e : observers_) e.second.del(k);
        signal();
    }

    mesa_rc observer_new(notifications::Event *ev) {
        // Use the pointer of ev as the key.
        auto i = observers_.get((uintptr_t)ev);
        if (i == observers_.end()) return MESA_RC_ERROR;

        // Clear - incase it was there already
        i->second.clear();

        // registere the event in the observer list.
        observer_list_.push_back(*ev);

        return MESA_RC_OK;
    }

    mesa_rc observer_del(notifications::Event *ev) {
        auto i = observers_.find((uintptr_t)ev);

        if (i == observers_.end()) return MESA_RC_ERROR;
        observers_.erase(i);
        observer_list_.unlink(*ev);

        return MESA_RC_OK;
    }

    mesa_rc observer_get(notifications::Event *ev, Observer &o) {
        o.clear();

        auto i = observers_.find((uintptr_t)ev);
        if (i == observers_.end()) return MESA_RC_ERROR;

        i->second.swap(o);

        // registere the event in the observer list.
        observer_list_.push_back(*ev);

        return MESA_RC_OK;
    }

    mesa_rc observer_mask_key(notifications::Event *ev, EventType::E et,
                              const Key &k) {
        auto i = observers_.find((uintptr_t)ev);
        if (i == observers_.end()) return MESA_RC_ERROR;
        return i->second.mask_key(et, k);
    }

  private:
    void signal() {
        while (!observer_list_.empty()) {
            notifications::Event &t = observer_list_.front();
            observer_list_.pop_front();
            t.signal();
        }
    }

    Map<uintptr_t, Observer> observers_;
    intrusive::List<notifications::Event> observer_list_;
};

}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_TABLE_OBSERVER_POOL_HXX__
