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

#ifndef __VTSS_BASICS_NOTIFICATIONS_TABLE_OBSERVER_VALUES_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_TABLE_OBSERVER_VALUES_HXX__

#include <vtss/basics/map.hxx>
#include <vtss/basics/utility.hxx>
#include <vtss/basics/notifications/table-observer-common.hxx>

namespace vtss {
namespace notifications {

template <typename Key, typename Val>
struct TableObserverValues {
  private:
    typedef Pair<unsigned int, Val> VAL;
    typedef Key KEY;
    typedef Pair<KEY, VAL> KEY_VAL;

  public:
    void add(const Key &k, const Val &v) {
        EventType::E e = EventType::None;
        auto i = events.find(k);

        if (i == events.end()) {
            events.insert(KEY_VAL(
                    k, VAL(TableObserverCommon::state_machine_add(e), v)));
        } else {
            e = (EventType::E)i->second.first;
            i->second = VAL(TableObserverCommon::state_machine_add(e), v);
        }
    }

    void del(const Key &k) {
        EventType::E e = EventType::None;
        Val v;  // dummy
        auto i = events.find(k);
        if (i == events.end()) {
            events.insert(KEY_VAL(
                    k, VAL(TableObserverCommon::state_machine_delete(e), v)));
        } else {
            e = (EventType::E)i->second.first;
            if (e == EventType::None) {
                events.erase(i);
            } else {
                auto new_e = TableObserverCommon::state_machine_modify(e);
                i->second = VAL(new_e, v);
            }
        }
    }

    void mod(const Key &k, const Val &v) {
        EventType::E e = EventType::None;
        auto i = events.find(k);
        if (i == events.end()) {
            auto new_e = TableObserverCommon::state_machine_modify(e);
            events.insert(KEY_VAL(k, VAL(new_e, v)));
        } else {
            e = (EventType::E)i->second.first;
            i->second = VAL(TableObserverCommon::state_machine_modify(e), v);
        }
    }

    void clear() { events.clear(); }
    Map<Key, Pair<unsigned int, Val>> events;
};


}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_TABLE_OBSERVER_VALUES_HXX__
