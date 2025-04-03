/*

 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __ALARM_LEAF_HXX__
#define __ALARM_LEAF_HXX__

#include "alarm-expression.hxx"
#include "alarm-element.hxx"
#include "vtss/basics/expose/json/root-node.hxx"
#include "vtss/basics/expose/json/notification.hxx"

namespace vtss {
namespace appl {
namespace alarm {

// The alarm leaf is a subject which automatic updates it self with the value
// of the alarm.
struct Leaf : public Element, public EventHandler {
    // Constructor. Check for success by calling the 'ok()' method after
    // construction.
    //   s:    Subject runner to evaluate the expression
    //   name: Identification of the alarm.
    //   expr: This is the alarm expression. This is used to evaluate if the
    //         alarm is active or not.
    //   i:    This is the json-specification of the current json tree. This is
    //         required to validate if the alarm expression is OK, and to derive
    //         the list of public variables.
    //   r:    This is the root-node of the json-tree. This is used to locate
    //         the public variables, needed to evaluate the alarm expression.
    Leaf(SubjectRunner *const s, const std::string &prefix,
         const std::string &name, const std::string &expr,
         const expose::json::specification::Inventory &i,
         expose::json::RootNode &r);

    // Check of the class could be constructed
    bool ok() const;
    virtual void execute(Event *e);

    // TODO, should this be a member?? I'm not sure I see the point in storing
    // it. We are fetching all the variables everytime anyway, so it could just
    // as well be stored on the stack.
    Map<str, std::string> bindings;

  private:
    bool ok_ = false;
    Expression expr_;
    expose::json::RootNode &r_;
    void get_values();

    // This is the inventory of the public variables needed by the alarm
    // expression.

    struct NotifEventPair {
        NotifEventPair(expose::json::Notification *n,
                       notifications::EventHandler *eh)
            : notif(n), event(eh) {
            if (!notif || !eh) {
                // T(ERROR) << "Invalid parameter, n=" << n << " eh=" << eh;
                return;
            }
            notif->observer_new(&event);
        }
        ~NotifEventPair() {
            if (notif) {
                notif->observer_del(&event);
            }
        }

        expose::json::Notification *notif;
        notifications::Event event;
    };
    Map<str, std::unique_ptr<NotifEventPair>> public_variables;
};

}  // namespace alarm
}  // namespace appl
}  // namespace vtss

#endif  // __ALARM_ELEMENT_HXX__
