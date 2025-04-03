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

#ifndef __VTSS_BASICS_NOTIFICATIONS_EVENT_FD_HXX__
#define __VTSS_BASICS_NOTIFICATIONS_EVENT_FD_HXX__

#include <vtss/basics/fd.hxx>
#include <vtss/basics/predefs.hxx>
#include <vtss/basics/utility.hxx>
#include <vtss/basics/enum_macros.hxx>
#include <vtss/basics/intrusive_list.hxx>
#include <vtss/basics/notifications/event-handler-predef.hxx>
#include <vtss/basics/notifications/subject-runner-predef.hxx>

namespace vtss {
namespace notifications {

// File descriptor events. Allows the notification thread to monitor a
// file-descriptor for a specific events. File descriptors supported are limited
// to sockets, terminals, fifos and pipes (not regular files!). Under Linux this
// this functionality is implemented using the 'epoll' system call and
// configured to use 'one-shot' in level trigger mode.
struct EventFd {
    friend struct SubjectRunner;
    friend struct SubjectRunnerEvent;

    // These are the supported events:
    enum E { NONE = 0, READ = 1, WRITE = 2, EXCEPT = 4 };

    // Default construct an EventFd class without an error handler - this is
    // most likely not what you want to do.
    constexpr EventFd() = default;

    // Construct an EventFd class with error handler but without an associated
    // file descriptor.
    constexpr explicit EventFd(EventHandler *eh) : eh_(eh){};

    // Construct an EventFd class with error handler and associated file
    // descriptor. The ownership of the file descriptor is transfered to this
    // class.
    EventFd(EventHandler *eh, vtss::Fd &&fd) : fd_(vtss::move(fd)), eh_(eh){};

    // No copies
    EventFd(const EventFd &ev) = delete;
    EventFd &operator=(const EventFd &ev) = delete;

    // Destructor - will unsubscribefrom the subject runner and clsoe the file
    // descriptor.
    ~EventFd() { unsubscribe(); }

    // Assign a new file-descriptor. This will cause the EventFd to be unlinked
    // from the subject-runner and must be re-armed to remain active.
    void assign(vtss::Fd &&fd);

    // Unsubscribe the EventFd instance from the subject runner and release
    // ownership
    // of the file-descriptor.
    Fd release();

    // Unscubscribe the EventFd instance from the subject runner and close the
    // file-descriptor.
    void close();

    // Unsubscribe the EventFd instance - if the file descriptor is being
    // listing to by a subject-runner then it will be deleted.
    void unsubscribe();

#ifdef VTSS_BASICS_OPERATING_SYSTEM_LINUX
    // Check if the handler has been attached to a subject runner
    bool is_attached() const { return sr_ != nullptr; }
#endif

    // Returns a mask of events which has triggered.
    E events() const { return event_flags_; }

    int raw() const { return fd_.raw(); }

  private:
    void invoke_cb();

    Fd fd_;
    E event_flags_ = NONE;
    EventHandler *eh_ = nullptr;

    // This pointer is used to keep track on if the file-descriptor is being
    // listning to by a subject runner. It should only be updated by the
    // subject-runners.
    SubjectRunnerEvent *sr_ = nullptr;
};

// Allow to use bit-wise operations on this enumeration.
VTSS_ENUM_BITWISE(EventFd::E)

}  // namespace notifications
}  // namespace vtss

#endif  // __VTSS_BASICS_NOTIFICATIONS_EVENT_FD_HXX__
