/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __VTSS_BASICS_NOTIFICATIONS_TIMER_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_TIMER_HXX__

#include <vtss/basics/intrusive_list.hxx>
#include <vtss/basics/notifications/event-handler.hxx>
#include <vtss/basics/ptr-bits2.hxx>
#include <vtss/basics/time_unit.hxx>

namespace vtss {
namespace notifications {

struct Timer : public intrusive::ListNode {
    friend struct SubjectRunner;
    friend struct SubjectRunnerEvent;

    Timer(EventHandler *cb);
    ~Timer() { unlink(); }

    TimeUnit timeout() const;
    bool operator<(const Timer &rhs);

    void unlink();

    TimeUnit get_period() const { return period_; }
    void set_period(TimeUnit time_unit) { period_ = time_unit; }

    bool get_repeat() const { return cb_.bit0(); }
    void set_repeat(bool repeat) { cb_.bit0(repeat); }

  private:
    void timeout(TimeUnit to);
    void invoke_cb();

    PtrBits2<EventHandler> cb_;
    // timeout_ is relative to an absolute time which is updateded every refresh
    // period. Absolute time is updated inside function update_absolute_time
    TimeUnit timeout_;
    TimeUnit period_;
};

}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_TIMER_HXX__
