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

#include "vtss/basics/trace_basics.hxx"
#include "vtss/basics/notifications/subject-base.hxx"
#include <vtss/basics/notifications/lock-global-subject.hxx>

#define TRACE(X) VTSS_BASICS_TRACE(VTSS_BASICS_TRACE_GRP_NOTIFICATIONS, X)

namespace vtss {
namespace notifications {

void SubjectBase::signal() {
    LockGlobalSubject lock(__FILE__, __LINE__);

    TRACE(NOISE) << "Signal " << (void *)this;

    while (!event_list_.empty()) {
        Event &t = event_list_.front();
        event_list_.pop_front();

        TRACE(NOISE) << "Signal-event " << (void *)this << " " << (void *)(&t);
        t.signal();
    }
}

void SubjectBase::attach(Event &e) {
    LockGlobalSubject lock(__FILE__, __LINE__);
    attach_(e);
}

bool SubjectBase::detach(Event &e) {
    LockGlobalSubject lock(__FILE__, __LINE__);
    TRACE(NOISE) << "Signal-detach " << (void *)this << " " << (void *)(&e);
    event_list_.unlink(e);
    return true;
}

void SubjectBase::attach_(Event &e) {
    TRACE(NOISE) << "Signal-attach " << (void *)this << " " << (void *)(&e);

    if (e.is_linked())
        detach(e);

    event_list_.push_back(e);
}

}  // namespace notifications
}  // namespace vtss

