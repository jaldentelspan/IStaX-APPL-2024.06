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
#include "alarm-trace.h"
#include "alarm-element.hxx"
#include "alarm-node.hxx"
#include "alarm-leaf.hxx"
#include "vtss/appl/alarm.h"
#include "alarm-expose.hxx"
#include "vtss/basics/expose/json/method-split.hxx"

#include <vtss/basics/trace.hxx>


namespace vtss {
namespace appl {
namespace alarm {

mesa_rc Element::make_alarm(SubjectRunner *const s, const std::string &prefix,
                            const std::string &nm, const std::string &expr,
                            const expose::json::specification::Inventory &i,
                            expose::json::RootNode &r) {
    DEFAULT(DEBUG) << "Create alarm " << nm;
    str name(nm);
    str head, tail;
    expose::json::method_split(name, head, tail);
    if (head != str("alarm")) {
        DEFAULT(DEBUG) << "Invalid alarm name: " << nm << " : " << expr;
        return VTSS_RC_ERROR;
    }
    return make_alarm(s, prefix, tail, expr, i, r);
}

mesa_rc Element::make_alarm(SubjectRunner *const s, const std::string &prefix,
                            const str &nm, const std::string &expr,
                            const expose::json::specification::Inventory &i,
                            expose::json::RootNode &r) {
    str head, tail;
    StringStream name_;
    if (type() != Type::NODE) {
        DEFAULT(ERROR) << "Alarms can only be created under a NODE";
        return VTSS_RC_ERROR;
    }
    Node *node = static_cast<Node *>(this);
    // consume until first delimitor, and forward the remaining
    expose::json::method_split(nm, head, tail);
    StringStream prefix_and_head;
    prefix_and_head << prefix << "." << head;
    if (tail.begin() == tail.end()) {
        for (auto &elem : node->children) {
            if (str(elem->name_) == head) {
                DEFAULT(DEBUG) << "Leaf already exists";
                return VTSS_RC_ERROR;
            }
        }
        name_.clear();
        name_ << nm;
        // TODO, Check that the leaf is created successfully before inserting
        // it. This is where we can catch syntax/semantic error in the
        // expression!
        auto l = new Leaf(s, prefix, name_.buf, expr, i, r);
        if (!l->ok()) {
            DEFAULT(DEBUG) << "Failed to create leaf " << nm << " to node "
                           << prefix;
            delete l;
            return VTSS_RC_ERROR;
        }
        auto p_newNode = node->add_child(l);
        if (!p_newNode.second) {
            DEFAULT(DEBUG) << "Failed to add child " << nm << " to node "
                           << prefix;
            return VTSS_RC_ERROR;
        }
        DEFAULT(DEBUG) << "Added leaf " << nm << " to node " << prefix;
        return VTSS_RC_OK;
    }

    for (auto &elem : node->children) {
        if (str(elem->name_) == head) {
            DEFAULT(DEBUG) << "node " << head << " already created under "
                           << prefix;
            return elem->make_alarm(s, prefix_and_head.buf, tail, expr, i, r);
        }
    }
    DEFAULT(DEBUG) << "Create node " << prefix_and_head;

    // TODO, please try to avoid "raw" pointers. This will go-away if you change
    // the interface of Node::add_child to accept a unique_ptr instead.
    // auto newNode = std::unique_ptr<Element>(new Node(s, name_.buf));
    // <error handling>
    // node->add_child(move(newNode));
    name_.clear();
    name_ << head;
    Node *newNode = new Node(s, name_.buf);
    if (!newNode) {
        DEFAULT(ERROR) << "Could not alloc node " << name_;
        return VTSS_RC_ERROR;
    }
    auto p_newNode = node->add_child(newNode);
    if (!p_newNode.second) {
        DEFAULT(DEBUG) << "Could not add child " << name_ << " to "
                       << prefix_and_head;
        return VTSS_RC_ERROR;
    }
    auto rc = newNode->make_alarm(s, prefix_and_head.buf, tail, expr, i, r);
    if (VTSS_RC_OK != rc) {
        DEFAULT(DEBUG) << "Could not make_alarms " << prefix_and_head;
        vtss_appl_alarm_name_t nm_stat;
        strcpy(nm_stat.alarm_name, (*p_newNode.first)->name().c_str());
        the_alarm_status.del(&nm_stat);
        node->children.erase(p_newNode.first);
    }
    return rc;
}

mesa_rc Element::delete_alarm(const std::string &nm) {
    DEFAULT(DEBUG) << "Delete alarm " << nm;
    str name(nm);
    str head, tail;
    expose::json::method_split(name, head, tail);
    if (head != str("alarm")) {
        return VTSS_RC_ERROR;
    }
    return delete_alarm(tail);
}

mesa_rc Element::delete_alarm(const str &nm) {
    str head, tail;
    if (type() != Type::NODE) {
        DEFAULT(ERROR) << "Alarms can only be contained under a NODE";
        return VTSS_RC_ERROR;
    }
    Node *node = static_cast<Node *>(this);
    // consume until first delimitor, and forward the remaining
    expose::json::method_split(nm, head, tail);
    if (tail.begin() == tail.end()) {
        for (auto elem = node->children.begin(); elem != node->children.end();
             elem++) {
            if (str((*elem)->name_) == head) {
                DEFAULT(DEBUG) << "Found entry " << head << ", delete it";
                vtss_appl_alarm_name_t nm_stat;
                strcpy(nm_stat.alarm_name, (*elem)->name().c_str());
                the_alarm_status.del(&nm_stat);
                node->children.erase(elem);
                node->execute(nullptr);
                return VTSS_RC_OK;
            }
        }
        DEFAULT(DEBUG) << "Did not find leaf " << head << " to node " << name();
        return VTSS_RC_ERROR;
    }

    for (auto elem = node->children.begin(); elem != node->children.end();
         elem++) {
        if (str((*elem)->name_) == head) {
            if ((*elem)->type() != Type::NODE) return VTSS_RC_ERROR;
            Node *elem_node = static_cast<Node *>(&(*(*elem)));
            DEFAULT(DEBUG) << "Found node " << head;
            auto rc = elem_node->delete_alarm(tail);
            if (rc != VTSS_RC_OK) return rc;
            if (elem_node->children.empty()) {
                DEFAULT(DEBUG) << "All children of " << name() << "." << head
                               << " deleted";
                vtss_appl_alarm_name_t nm_stat;
                strcpy(nm_stat.alarm_name, (*elem)->name().c_str());
                the_alarm_status.del(&nm_stat);
                node->execute(nullptr);
                node->children.erase(elem);
            }
            return VTSS_RC_OK;
        }
    }
    DEFAULT(DEBUG) << "Did not find " << head << " to node " << name();
    return VTSS_RC_ERROR;
}

const std::string Element::name() const {
    if (parent_) {
        std::string tmp_name = parent_->name() + "." + name_;
        return tmp_name;
    } else {
        return name_;
    }
}

Element *Element::lookup(const std::string &nm) { return lookup(str(nm)); }

Element *Element::lookup(const str &nm) {
    DEFAULT(DEBUG) << "Search for " << nm;
    str head, tail;
    // consume until first delimitor, and forward the remaining
    expose::json::method_split(nm, head, tail);
    if (head != str(name_)) {
        return nullptr;
    }
    if (tail.begin() == tail.end()) {
        // Done, found it
        return this;
    }
    if (type() != Type::NODE) {
        return nullptr;  // Nothing is contained under a leaf
    }
    // It is a NODE
    Node *node = static_cast<Node *>(this);
    for (auto &elem : node->children) {
        auto p = elem->lookup(tail);
        if (p) {
            return p;
        }
    }

    return nullptr;
}

Element *Element::next(const str &nm) {
    DEFAULT(DEBUG) << "Search for next: " << nm;
    if (nm.begin() == nm.end()) return this;

    str head, tail;
    // consume until first delimitor, and forward the remaining
    expose::json::method_split(nm, head, tail);
    if (head != str(name_)) {
        DEFAULT(DEBUG) << "Abort next, Invalid end point: " << nm;
        return nullptr;
    }
    if (type() != Type::NODE) {
        DEFAULT(DEBUG) << "Abort next, nothing containd under leaf: " << nm;
        return nullptr;  // Nothing is contained under a leaf
    }
    // It is a NODE
    Node *node = static_cast<Node *>(this);
    for (auto &elem : node->children) {
        DEFAULT(NOISE) << "Testing: " << elem->name_;
        if (tail < str(elem->name_)) {
            return elem->next(str(""));
        } else {
            DEFAULT(DEBUG) << "Continue into: " << tail;
            auto p = elem->next(tail);
            if (p) {
                return p;
            }
        }
    }

    return nullptr;
}

mesa_rc Element::suppress(const std::string &nm, bool s) {
    Element *e = lookup(nm);
    if (!e) {
        DEFAULT(ERROR) << "Did not find: " << nm;
        return VTSS_RC_ERROR;
    }
    DEFAULT(DEBUG) << "Suppress " << nm;
    e->suppressed = s && e->get();
    e->signal();

    vtss_appl_alarm_name_t nm_stat;
    vtss_appl_alarm_status_t stat;
    strcpy(nm_stat.alarm_name, nm.c_str());
    stat.suppressed = e->suppressed;
    stat.active = e->get();
    stat.exposed_active = stat.active && (!stat.suppressed);
    the_alarm_status.set(&nm_stat, &stat);

    return VTSS_RC_OK;
}

void Element::set(const bool &value, bool force) {
    if (!value && suppressed) {
        // When alarm is cleared, remove suppress
        DEFAULT(DEBUG) << "Clear suppression for " << name();
        suppressed = false;
    }
    Subject<bool>::set(value, force);
}

}  // namespace alarm
}  // namespace appl
}  // namespace vtss
