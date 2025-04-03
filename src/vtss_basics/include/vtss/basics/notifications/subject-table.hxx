/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_NOTIFICATIONS_SUBJECT_TABLE_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_SUBJECT_TABLE_HXX__
#define LOCK_WRAP(F, ...)                  \
    Lock lock(&mutex, __FILE__, __LINE__); \
    return F(lock, ##__VA_ARGS__);

#include <vtss/basics/map.hxx>
#include <vtss/basics/mutex.hxx>
#include <vtss/basics/notifications/subject-base.hxx>
#include <vtss/basics/notifications/table-observer.hxx>
#include <vtss/basics/notifications/table-observer-pool.hxx>
#include <vtss/basics/notifications/table-observer-values.hxx>

namespace vtss {
namespace notifications {

template <class K, class V>
struct SubjectTable {
    typedef typename Map<K, V>::const_iterator const_iterator;
    typedef TableObserver<K> Observer;

    struct Callback {
        virtual void add(const K& k,       V& v) {};
        virtual void mod(const K& k, const V& before, V& after) {};
        virtual void del(const K& k,       V& v) {};
    };

    struct Lock {
        Lock(Critd *c, const char *file, int line) : critd(c) {
            critd->lock(file, line);
        }

        Lock(Lock &&rhs) noexcept {
            rhs.critd = critd;
            critd = nullptr;
        }

        ~Lock() {
            if (critd) {
                critd->unlock(__FILE__, __LINE__);
            }
        }

        Lock() = delete;
        Lock(const Lock &) = delete;
        Lock &operator=(const Lock &) = delete;

    private:
        Critd *critd = nullptr;
    };

    SubjectTable() = delete;
    SubjectTable(const char *mutex_name, uint32_t module_id) :
        mutex(mutex_name, module_id) {}

    Lock &&lock_get(const char *file, int line) {
//#if defined(__GNUC__) && __GNUC__ > 7
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wreturn-local-addr"
//#endif
        return vtss::move(Lock(&mutex, file, line)); // ALLANTBD
//#if defined(__GNUC__) && __GNUC__ > 7
//#pragma GCC diagnostic pop
//#endif
    }

    mesa_rc observer_new(Lock &, notifications::Event* ev) {
        return observers.observer_new(ev);
    }
    mesa_rc observer_new(notifications::Event* ev) {
        LOCK_WRAP(observer_new, ev);
    }

    mesa_rc observer_new(Lock &, notifications::Event* ev, Observer& o) {
        mesa_rc rc = observers.observer_new(ev);
        if (rc != MESA_RC_OK) return rc;

        // Initially all existing objects are considered as added
        o.clear();
        for (const auto& e : data_) o.add(e.first);

        return MESA_RC_OK;
    }

    mesa_rc observer_new(notifications::Event* ev, Observer& o) {
        LOCK_WRAP(observer_new, ev, o);
    }

    mesa_rc observer_del(Lock &, notifications::Event* ev) {
        return observers.observer_del(ev);
    }

    mesa_rc observer_del(notifications::Event* ev) {
        LOCK_WRAP(observer_del, ev);
    }

    mesa_rc observer_get(Lock &, notifications::Event* ev, Observer& o) {
        return observers.observer_get(ev, o);
    }

    mesa_rc observer_get(notifications::Event* ev, Observer& o) {
        LOCK_WRAP(observer_get, ev, o);
    }

    mesa_rc observer_mask_key(Lock &, notifications::Event* ev, EventType::E et,
                              const K& k) {
        return observers.observer_mask_key(ev, et, k);
    }
    mesa_rc observer_mask_key(notifications::Event* ev, EventType::E et,
                              const K& k) {
        LOCK_WRAP(observer_mask_key, ev, et, k);
    }

    mesa_rc get(Lock &, const K& k, V& v) const {
        auto itr = data_.find(k);
        if (itr == data_.end()) return MESA_RC_ERROR;
        v = itr->second;
        return MESA_RC_OK;
    }
    mesa_rc get(const K& k, V& v) const { LOCK_WRAP(get, k, v); }

    mesa_rc get_first(Lock &, K& k, V& v) const {
        auto itr = data_.begin();
        if (itr == data_.end()) return MESA_RC_ERROR;
        k = itr->first;
        v = itr->second;
        return MESA_RC_OK;
    }
    mesa_rc get_first(K& k, V& v) const { LOCK_WRAP(get_first, k, v); }

    mesa_rc get_next(Lock &, K& k, V& v) const {
        auto itr = data_.greater_than(k);
        if (itr == data_.end()) return MESA_RC_ERROR;
        k = itr->first;
        v = itr->second;
        return MESA_RC_OK;
    }
    mesa_rc get_next(K& k, V& v) const { LOCK_WRAP(get_next, k, v); }

    mesa_rc set(Lock &, const K& k, const V& v) {
        auto p = data_.emplace(k, v);

        if (p.second) {
            // Add operation
            observers.event_add(k);
        } else if (p.first != data_.end()) {
            if (p.first->second != v) {
                // Update existing entry
                p.first->second = v;
                observers.event_mod(k);
            }
        } else {
            // Failed to create entry!
            return MESA_RC_ERROR;
        }

        return MESA_RC_OK;
    }
    mesa_rc set(const K& k, const V& v) { LOCK_WRAP(set, k, v); }

    mesa_rc set(Lock &, const Map<K, V>& v, Callback &cb) {
        auto lhs = data_.begin();
        auto rhs = v.begin();

        // Process all delete/update events at first
        while (lhs != data_.end() && rhs != v.end()) {
            if (lhs->first < rhs->first) {
                // Entry found in data_ but not in v -> delete it from data_
                observers.event_del(lhs->first);
                cb.del(lhs->first, lhs->second);
                data_.erase(lhs++);
            } else if (rhs->first < lhs->first) {  // add to data_
                rhs++;
            } else {
                // Entry found in both -> update it in data_
                if (lhs->second != rhs->second) {
                    auto tmp = rhs->second;
                    cb.mod(lhs->first, lhs->second, tmp);
                    lhs->second = tmp;
                    observers.event_mod(lhs->first);
                }

                ++rhs, ++lhs;
            }
        }

        // No more data in v, meaning that we must delete the rest of data_
        while (lhs != data_.end()) {
            observers.event_del(lhs->first);
            cb.del(lhs->first, lhs->second);
            data_.erase(lhs++);
        }

        // Now process all add events
        lhs = data_.begin();
        rhs = v.begin();
        while (lhs != data_.end() && rhs != v.end()) {
            if (lhs->first < rhs->first) {
                // delete event - should never happen
                lhs++;
            } else if (rhs->first < lhs->first) {  // add to data_
                // Entry found in v but not in data_ -> add it to data_
                observers.event_add(rhs->first);

                // Lose const qualifier on second, so that cb may override
                auto tmp = rhs->second;
                cb.add(rhs->first, tmp);
                data_.set(rhs->first, tmp);
                rhs++;
            } else {
                // found in both - event already processed
                ++rhs, ++lhs;
            }
        }

        // No more data in data_, meaning that we must add the rest of v
        while (rhs != v.end()) {
            observers.event_add(rhs->first);

            // Lose const qualifier on second, so that cb may override
            auto tmp = rhs->second;
            cb.add(rhs->first, tmp);
            data_.set(rhs->first, tmp);
            rhs++;
        }

        return MESA_RC_OK;
    }
    mesa_rc set(const Map<K, V>& v, Callback &cb) {
        LOCK_WRAP(set, v, cb);
    }

    mesa_rc set(Lock &l, const Map<K, V>& v) {
        Callback cb;
        return set(l, v, cb);
    }
    mesa_rc set(const Map<K, V>& v) { LOCK_WRAP(set, v); }

    mesa_rc get(Lock &, Map<K, V>& v) const {
        for (auto& e : data_) v.insert(e);
        return MESA_RC_OK;
    }
    mesa_rc get(Map<K, V>& v) const { LOCK_WRAP(get, v); }

    mesa_rc del(Lock &, const K& k) {
        auto itr = data_.find(k);
        if (itr == data_.end()) return MESA_RC_ERROR;
        data_.erase(itr);
        observers.event_del(k);
        return MESA_RC_OK;
    };
    mesa_rc del(const K& k) { LOCK_WRAP(del, k); }

    void clear(Lock &) { data_.clear(); }
    void clear() { LOCK_WRAP(clear);  }

    size_t size(Lock &) const { return data_.size(); }
    size_t size() const { LOCK_WRAP(size); }

    const Map<K, V>& ref(Lock &) const { return data_; }

  protected:
    Map<K, V> data_;
    TableObserverPool<K, Observer> observers;

  private:
    mutable Critd mutex;

};

}  // namespace notifications
}  // namespace vtss


#undef LOCK_IMPL
#endif  // __VTSS_BASICS_NOTIFICATIONS_SUBJECT_TABLE_HXX__

