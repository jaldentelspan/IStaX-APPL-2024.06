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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef __VTSS_BASICS_SYNCHRONIZED_HXX__
#define __VTSS_BASICS_SYNCHRONIZED_HXX__

#include "vtss/basics/utility.hxx"

/*
For documentation and inspiration see:
https://github.com/facebook/folly/blob/master/folly/docs/Synchronized.md
https://github.com/facebook/folly/blob/master/folly/Synchronized.h
*/

#include "vtss/basics/preprocessor.h"
#include <vtss/basics/notifications/subject-runner.hxx>
#include <vtss/basics/notifications/lock-global-subject.hxx>
#if defined(VTSS_BASICS_STANDALONE)
#else
#include "critd_api.h"
#endif

#define SYNCHRONIZED(...)                                                    \
    if (bool SYNCHRONIZED_state = false) {                                   \
    } else                                                                   \
        for (auto SYNCHRONIZED_lockedPtr =                                   \
                     (VTSS_ARG_2_OR_1(__VA_ARGS__)).ref(__LINE__, __FILE__); \
             !SYNCHRONIZED_state; SYNCHRONIZED_state = true)                 \
            for (auto &VTSS_ARG_1(__VA_ARGS__) =                             \
                         *SYNCHRONIZED_lockedPtr.operator->();               \
                 !SYNCHRONIZED_state; SYNCHRONIZED_state = true)

namespace vtss {

struct SynchronizedLockGlobal {
    void lock(unsigned line, const char *name) {
        vtss::notifications::lock_global_subject.lock(name, line);
    }

    void unlock(unsigned line, const char *name) {
        vtss::notifications::lock_global_subject.unlock(name, line);
    }

    void init(unsigned line, const char *name) {}
};

#if defined(VTSS_BASICS_STANDALONE)
template <int MODULE>
struct SynchronizedLock : public SynchronizedLockGlobal {};
#else
template <int MODULE>
struct SynchronizedLock {
    void lock(unsigned line, const char *name) {
        critd_enter(&crit_, name, line);
    }

    void unlock(unsigned line, const char *name) {
        critd_exit(&crit_, name, line);
    }

    void init(unsigned line, const char *name) {
        critd_init(&crit_, name, MODULE, CRITD_TYPE_MUTEX);
    }

    critd_t crit_;
};
#endif

template <class T, int MODULE>
struct Synchronized {
    struct LockedPtr {
        LockedPtr(Synchronized *parent, unsigned l, const char *name)
            : line_(l), name_(name), p(parent) {
            p->m.lock(line_, name_);
        }

        ~LockedPtr() { p->m.unlock(line_, name_); }

        T *operator->() { return &p->t; }

      private:
        unsigned line_;
        const char *name_;
        Synchronized *p;
    };

    LockedPtr ref(unsigned line, const char *name) {
        return LockedPtr(this, line, name);
    }

    void init(unsigned line, const char *name) { m.init(line, name); }

  private:
    T t;
    mutable SynchronizedLock<MODULE> m;
};

template <class T>
struct SynchronizedSubjectRunner {
    template <typename... Args>
    SynchronizedSubjectRunner(notifications::SubjectRunner *_sr,
                              Args &&... args)
        : sr(_sr), t(vtss::forward<Args>(args)...) {}
    struct LockedPtr {
        LockedPtr(SynchronizedSubjectRunner *parent, unsigned l, const char *name)
            : line_(l), name_(name), p(parent) {
            p->sr->lock(name_, line_);
        }

        ~LockedPtr() { p->sr->unlock(name_, line_); }

        T *operator->() { return &p->t; }

      private:
        unsigned line_;
        const char *name_;
        SynchronizedSubjectRunner *p;
    };

    LockedPtr ref(unsigned line, const char *name) {
        return LockedPtr(this, line, name);
    }

    void init(unsigned line, const char *name) { }

  private:
    mutable notifications::SubjectRunner *sr;
    T t;
};

}  // namespace vtss

#endif  // __VTSS_BASICS_SYNCHRONIZED_HXX__
