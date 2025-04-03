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

#include "vtss/basics/common.hxx"
#include "vtss/basics/notifications/table-observer-common.hxx"

namespace vtss {
namespace notifications {

EventType::E TableObserverCommon::state_machine_add(uint32_t s) {
    switch (s) {
    case EventType::None:
        return EventType::Add;

    case EventType::Modify:
        VTSS_ASSERT(0);
        return EventType::Modify;

    case EventType::Add:
        VTSS_ASSERT(0);
        return EventType::Add;

    case EventType::Delete:
        return EventType::Modify;

    default:
        VTSS_ASSERT(0);
        return (EventType::E)s;
    }
}

EventType::E TableObserverCommon::state_machine_delete(uint32_t s) {
    switch (s) {
    case EventType::None:
        return EventType::Delete;

    case EventType::Modify:
        return EventType::Delete;

    case EventType::Add:
        return EventType::None;

    case EventType::Delete:
        VTSS_ASSERT(0);
        return EventType::Delete;

    default:
        VTSS_ASSERT(0);
        return (EventType::E)s;
    }
}

EventType::E TableObserverCommon::state_machine_modify(uint32_t s) {
    switch (s) {
    case EventType::None:
        return EventType::Modify;

    case EventType::Modify:
        return EventType::Modify;

    case EventType::Add:
        return EventType::Add;

    case EventType::Delete:
        VTSS_ASSERT(0);
        return EventType::Delete;

    default:
        VTSS_ASSERT(0);
        return (EventType::E)s;
    }
}

}  // namespace notifications
}  // namespace vtss

