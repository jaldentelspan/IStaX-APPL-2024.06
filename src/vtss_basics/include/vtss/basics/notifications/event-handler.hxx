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

#ifndef __VTSS_BASICS_NOTIFICATIONS_EVENT_HANDLER_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_EVENT_HANDLER_HXX__

#include "vtss/basics/notifications/timer-predef.hxx"
#include "vtss/basics/notifications/subject-runner-predef.hxx"
#include "vtss/basics/time_unit.hxx"

namespace vtss {
namespace notifications {

struct Event;
struct EventFd;

struct EventHandler {
    constexpr EventHandler(SubjectRunner *const s): sr(s) {}
    virtual ~EventHandler() {}  // Any class that is used as a base class should have a virtual destructor.

    void add(Event *e);
    void del(Event *e);
    void add(Timer *t, TimeUnit period);
    void inc(Timer *t, TimeUnit period);
    void del(Timer *t);

    virtual void execute(Event *e) {}
    virtual void execute(Timer *e) {}
    virtual void execute(EventFd *e) {}

  protected:
    SubjectRunner *const sr;
};

}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_EVENT_HANDLER_HXX__
