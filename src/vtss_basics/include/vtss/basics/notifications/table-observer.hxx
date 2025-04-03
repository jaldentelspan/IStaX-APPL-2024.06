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

#ifndef __VTSS_BASICS_NOTIFICATIONS_TABLE_OBSERVER_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_TABLE_OBSERVER_HXX__

#include <vtss/basics/trace_grps.hxx>
#include <vtss/basics/trace_basics.hxx>

#include <vtss/basics/map.hxx>
#include <vtss/basics/expose/param-tuple-key.hxx>
#include <vtss/basics/expose/param-tuple-val.hxx>
#include <vtss/basics/notifications/table-observer-common.hxx>

#define TRACE(X) VTSS_BASICS_TRACE(113, VTSS_BASICS_TRACE_GRP_NOTIFICATIONS, X)

namespace vtss {
namespace notifications {

template <typename Key>
struct TableObserver {
    void add(const Key &k) {
        auto i = events.find(k);
        if (i == events.end()) {
            events.set(k,
                       TableObserverCommon::state_machine_add(EventType::None));
        } else {
            i->second = TableObserverCommon::state_machine_add(i->second);
        }
    }

    void del(const Key &k) {
        auto i = events.find(k);
        if (i == events.end()) {
            events.set(k, TableObserverCommon::state_machine_delete(
                                  EventType::None));
        } else {
            auto e = TableObserverCommon::state_machine_delete(i->second);
            if (e == EventType::None)
                events.erase(i);
            else
                i->second = e;
        }
    }

    void mod(const Key &k) {
        auto i = events.find(k);
        if (i == events.end())
            events.set(k, TableObserverCommon::state_machine_modify(
                                  EventType::None));
        else
            i->second = TableObserverCommon::state_machine_modify(i->second);
    }

    mesa_rc mask_key(EventType::E et, const Key &k) {
        auto i = events.find(k);
        EventType::E e_new = EventType::None;

        if (i == events.end()) {
            TRACE(ERROR) << "No event found! et=" << et;
            return MESA_RC_ERROR;
        }

        switch (i->second) {
        case EventType::Modify:
            e_new = TableObserverCommon::state_machine_modify(et);
            break;

        case EventType::Add:
            e_new = TableObserverCommon::state_machine_add(et);
            break;

        case EventType::Delete:
            e_new = TableObserverCommon::state_machine_delete(et);
            break;

        default:
            TRACE(ERROR) << "Default case! " << i->second;
            return MESA_RC_ERROR;
        }

        if (e_new == EventType::None)
            events.erase(i);
        else
            i->second = e_new;

        return MESA_RC_OK;
    }

    void clear() { events.clear(); }
    void swap(TableObserver<Key> &rhs) {
        vtss::swap(events, rhs.events);
    }
    Map<Key, unsigned int> events;
};

}  // namespace notifications
}  // namespace vtss

#undef TRACE
#endif  // __VTSS_BASICS_NOTIFICATIONS_TABLE_OBSERVER_HXX__
