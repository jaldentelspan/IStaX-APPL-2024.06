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
#include <vtss/basics/trace.hxx>
#include "vtss/appl/alarm.h"
#include "alarm-leaf.hxx"
#include "alarm-expose.hxx"
#include "vtss/basics/json-rpc-server.hxx"
#include "alarm-trace.h"

vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_alarm_status_t &v) {
    o << "{\"suppressed\":" << v.suppressed
      << ", \"active\":" << v.active
      << ", \"exposed_active\":" << v.exposed_active << "}";
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_alarm_name_t &v) {
    o << v.alarm_name;
    return o;
}

vtss::ostream &operator<<(vtss::ostream &o,
                          const vtss_appl_alarm_expression_t &v) {
    o << v.alarm_expression;
    return o;
}

namespace vtss {
namespace appl {
namespace alarm {

// // TODO: It look kind of "private" could they be static???
// static ostream &operator<<(ostream &o, const vtss::Set<std::string>
// json_vars) {
//     for (auto i : json_vars) {
//         o << i << "\n";
//     }
//     return o;
// }

// TODO: It look kind of "private" could they be static???
static ostream &operator<<(ostream &o, const Map<str, std::string> bindings)
{
    for (auto i : bindings) {
        o << i.first << "->" << i.second << "\n";
    }
    return o;
}


Leaf::Leaf(SubjectRunner *const s, const std::string &prefix,
           const std::string &n, const std::string &expr,
           const expose::json::specification::Inventory &i,
           expose::json::RootNode &r)
    : Element(Element::Type::LEAF, n), EventHandler(s), expr_(expr, i), r_(r) {
    DEFAULT(INFO) << "Building leaf (" << prefix << "/" << n << ") for " << expr;
    ok_ = expr_.ok();
    if (!ok_) {
        DEFAULT(DEBUG) << "Expression '" << expr << "' not ok";
        return;
    }

    const auto &json_vars = expr_.bindings();
    for (const auto &nm : json_vars) {
        expose::json::Node *node = r_.lookup(nm);

        if (node == nullptr) {
            DEFAULT(ERROR) << "Null pointer met: node=" << node;
            ok_ = false;
            return;
        }

        expose::json::Node *update = node->lookup(str("update"));
        if (update == nullptr || !update->is_notification()) {
            DEFAULT(ERROR) << "Invalid result: update=" << update;
            ok_ = false;
            return;
        }
        auto *notif = static_cast<expose::json::Notification *>(update);
        auto ne =
                std::unique_ptr<NotifEventPair>(new NotifEventPair(notif, this));
        if (!ne) {
            DEFAULT(ERROR) << "Could not create ne=" << ne.get();
            ok_ = false;
            return;
        }
        auto res = public_variables.set(nm, vtss::move(ne));
        if (!res) {
            DEFAULT(ERROR) << "Could not insert: res=" << res;
            ok_ = false;
            return;
        }
    }

    get_values();
    if (!ok_) {
        return;
    }
    DEFAULT(INFO) << "Bindings: " << bindings;
    set(!expr_.evaluate(bindings));
    StringStream prefix_and_head;
    prefix_and_head << prefix << "." << n;
    vtss_appl_alarm_name_t nm;
    vtss_appl_alarm_status_t stat;
    strcpy(nm.alarm_name, prefix_and_head.cstring());
    stat.suppressed = suppressed;
    stat.active = get();
    stat.exposed_active = stat.active && !stat.suppressed;
    the_alarm_status.set(&nm, &stat);
    DEFAULT(DEBUG) << "Initialize " << nm << " to " << stat;

    ok_ = expr_.ok();
    if (!ok_) {
        DEFAULT(ERROR) << "Expression failed";
    } else {
        DEFAULT(DEBUG) << "Evaluated expression to: " << get();
    }
}

void Leaf::get_values() {
    if (ok_) {
        for (const auto &i : public_variables) {
            StringStream msg;
            msg << "{\"method\":\"" << i.first
                << ".get\",\"params\":[],\"id\":1}";

            // Check for error
            if (!msg.ok()) {
                DEFAULT(ERROR) << "Failed to generate method";
                ok_ = false;
                return;
            }

            StringStream out;

            ok_ = vtss::json::Result::OK ==
                  vtss::json::process_request(str(msg.cstring()), out, &r_);
            if (!ok_) {
                DEFAULT(ERROR) << "Failed to call method " << msg;
                return;
            }
            DEFAULT(DEBUG) << "Got: " << out;

            if (!bindings.set(i.first, vtss::move(out.buf))) {
                ok_ = false;
                DEFAULT(ERROR) << "Failed to update binding";
                return;
            }
        }
    }
}

void Leaf::execute(Event *e) {
    DEFAULT(DEBUG) << "Execute: " << name();
    if (!ok_) {
        return;
    }
    for (auto &i : public_variables) {
        if (&i.second->event == e) {
            std::string s;
            ok_ = (VTSS_RC_OK == i.second->notif->observer_get(e, s));
            if (!ok_) {
                DEFAULT(ERROR) << "Failed to do observer_get";
            }
            break;
        }
    }
    get_values();
    if (!ok_) {
        return;
    }
    DEFAULT(INFO) << "Bindings: " << bindings;
    set(!expr_.evaluate(bindings));
    vtss_appl_alarm_name_t nm;
    vtss_appl_alarm_status_t stat;
    strcpy(nm.alarm_name, name().c_str());
    stat.suppressed = suppressed;
    stat.active = get();
    stat.exposed_active = stat.active && !stat.suppressed;
    the_alarm_status.set(&nm, &stat);
    DEFAULT(DEBUG) << "Update " << nm << " to " << stat;

    ok_ = expr_.ok();
    if (!ok_) {
        DEFAULT(ERROR) << "Expression failed";
    } else {
        DEFAULT(DEBUG) << "Evaluated expression to: " << get();
    }
}

bool Leaf::ok() const { return ok_; }


}  // namespace alarm
}  // namespace appl
}  // namespace vtss
