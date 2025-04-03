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
#include <iostream>
#include "alarm-node.hxx"
#include "vtss/basics/json-rpc-server.hxx"
#include "vtss/basics/expose/json/method-split.hxx"
#include "vtss/appl/alarm.h"
#include "alarm-expose.hxx"
#include "alarm-trace.h"

#include <vtss/basics/trace.hxx>


namespace vtss {
namespace appl {
namespace alarm {

bool operator<(const std::unique_ptr<Element> &x,
               const std::unique_ptr<Element> &y) {
    return x->name_ < y->name_;
}

// Check of the class could be constructed
bool Node::ok() const {
    for (const auto &i : children) {
        if (!i->ok()) {
            return false;
        }
    }
    return true;
}

Pair<Set<std::unique_ptr<Element>>::iterator, bool> Node::add_child(
        Element *element) {
    element->event.reset(new Event(this));
    // TODO, error check
    auto r = children.insert(std::unique_ptr<Element>(element));
    if (r.second) {
        element->parent_ = this;
        execute(element->event.get());
        return r;
    }
    return r;
}

void Node::execute(Event *ev) {
    DEFAULT(DEBUG) << "execute " << name();
    bool res = false;
    for (auto &i : children) {
        res |= i->get(*i->event) && !i->suppressed;
    }
    set(res);
    vtss_appl_alarm_name_t nm;
    vtss_appl_alarm_status_t stat;
    strcpy(nm.alarm_name, name().c_str());
    stat.suppressed = suppressed;
    stat.active = res;
    stat.exposed_active = stat.active && !stat.suppressed;
    the_alarm_status.set(&nm, &stat);
    DEFAULT(DEBUG) << "Update " << nm << " to " << stat;
}


}  // namespace alarm
}  // namespace appl
}  // namespace vtss
