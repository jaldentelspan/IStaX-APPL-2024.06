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

#ifndef __VTSS_BASICS_NOTIFICATIONS_SYNCHRONIZED_GLOBAL_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_SYNCHRONIZED_GLOBAL_HXX__

#include <vtss/basics/predefs.hxx>
#include <vtss/basics/notifications/lock-global-subject.hxx>

namespace vtss {
namespace notifications {

template <class T>
struct SynchronizedGlobal {
    struct LockedPtr {
        LockedPtr(SynchronizedGlobal* parent, unsigned, const char*) : p(parent) {
#if defined(VTSS_BASICS_OPERATING_SYSTEM_LINUX)
            lock_global_subject.lock(__FILE__, __LINE__);
#else
#error "OS Not supported"
#endif
        }

        ~LockedPtr() {
#if defined(VTSS_BASICS_OPERATING_SYSTEM_LINUX)
            lock_global_subject.unlock(__FILE__, __LINE__);
#else
#error "OS Not supported"
#endif
        }

        T* operator->() { return &p->t; }

      private:
        SynchronizedGlobal* p;
    };

    LockedPtr ref(unsigned line, const char* file) {
        return LockedPtr(this, line, file);
    }

  private:
    T t;
};

}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_SYNCHRONIZED_GLOBAL_HXX__
