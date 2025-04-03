/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "critd_api.h"
#include "subject.hxx"
#include "vtss/basics/trace.hxx"

#include <string>
#include <vtss/basics/set.hxx>
#include <vtss/basics/map.hxx>
#include <vtss/basics/notifications.hxx>
#include <vtss/basics/expose/snmp/globals.hxx>
#include <vtss/basics/expose/snmp/struct-base-trap.hxx>

#include "vtss_snmp.h"
#include "vtss_private_trap.hxx"

#define TRACE(X) VTSS_TRACE(TRACE_GRP_TRAP, X)

namespace vtss {
namespace appl {
namespace snmp_trap {

struct Handler : public notifications::EventHandler {
    Handler()
        : notifications::EventHandler(&notifications::subject_main_thread) {}
    void execute(notifications::Event *e);
    void execute(notifications::Timer *e) {}
} handler;

struct TrapSubscription {
    TrapSubscription(const TrapSubscription&) = delete;
    TrapSubscription &operator=(const TrapSubscription&) = delete;

    TrapSubscription(std::string name) : name_(name) {}

    TrapSubscription(std::string name, expose::snmp::StructBaseTrap *subject)
        : name_(name), event_(&handler), subject_(subject) {
        TRACE(DEBUG) << "observer_new " << name_;
        mesa_rc rc = subject_->observer_new(&event_);
        if (rc == VTSS_RC_OK) {
            attached_ = true;
        } else {
            TRACE(ERROR) << "FAILED: observer_new " << name_;
        }
    }

    ~TrapSubscription() {
        if (attached_) {
            TRACE(DEBUG) << "observer_del " << name_;
            mesa_rc rc = subject_->observer_del(&event_);
            if (rc != VTSS_RC_OK) {
                TRACE(ERROR) << "FAILED: observer_del " << name_;
            }
        }
    }

    mesa_rc get(expose::snmp::TrapHandler &t) const {
        if (!attached_) {
            TRACE(ERROR) << "FAILED: not attached " << name_;
            return VTSS_RC_ERROR;
        }

        TRACE(DEBUG) << "observer_get " << name_;
        mesa_rc rc = subject_->observer_get(&event_, t);
        if (rc != VTSS_RC_OK) {
            TRACE(ERROR) << "FAILED: observer_get " << name_;
        }

        return rc;
    }

    bool is_attached() const { return attached_; }
    bool attached_ = false;
    std::string name_;
    mutable notifications::Event event_;
    expose::snmp::StructBaseTrap *subject_ = nullptr;
};

vtss::Set<TrapSubscription> trap_subscriptions;

struct SnmpStringLess {
    bool operator()(const std::string &a, const std::string &b) const {
        if (a.size() != b.size()) return a.size() < b.size();
        return a < b;
    }
};

bool operator<(const TrapSubscription &a, const TrapSubscription &b) {
    SnmpStringLess cmp;
    return cmp(a.name_, b.name_);
}


static critd_t crit_;
struct Lock {
    Lock(int line) {
        critd_enter(&crit_, __FILE__, line);
    }
    ~Lock() {
        critd_exit(&crit_, __FILE__, 0);
    }
};
#define CRIT_SCOPE() Lock __lock_guard__(__LINE__)

mesa_rc listen_add(const std::string &event_name) {
    CRIT_SCOPE();

    TRACE(INFO) << "Subscribe add: " << event_name;
    auto t = vtss::expose::snmp::vtss_snmp_globals.trap_find(event_name.c_str());

    if (!t) {
        TRACE(INFO) << "No such trap: " << event_name;
        return SNMP_TRAP_NO_SUCH_TRAP;
    }

    if (trap_subscriptions.find(event_name) != trap_subscriptions.end()) {
        TRACE(INFO) << "Already listning: " << event_name;
        return VTSS_RC_OK;
    }

    trap_subscriptions.emplace(event_name, t);

    return VTSS_RC_OK;
}

mesa_rc listen_del(const std::string &event_name) {
    CRIT_SCOPE();
    TRACE(INFO) << "Subscribe del: " << event_name;

    auto i = trap_subscriptions.find(event_name);
    if (i == trap_subscriptions.end()) return SNMP_TRAP_NO_SUCH_SUBSCRIPTION;

    trap_subscriptions.erase(i);

    return VTSS_RC_OK;
}

mesa_rc listen_get_next(std::string &event_name) {
    CRIT_SCOPE();

    TRACE(INFO) << "Subscribe get_next: " << event_name;
    auto t = vtss::expose::snmp::vtss_snmp_globals.trap_find_next(event_name.c_str());

    if (!t) {
        return SNMP_TRAP_NO_SUCH_TRAP;
    }
    TRACE(INFO) << "Found next trap source name: " << t->element().name();
    event_name = t->element().name();

    return VTSS_RC_OK;
}

void Handler::execute(notifications::Event *e) {
    CRIT_SCOPE();

    TRACE(DEBUG) << "Processing trap event";
    for (const auto &i : trap_subscriptions) {
        if (e != &i.event_) continue;

        expose::snmp::TrapHandler h;
        auto rc = i.get(h);
        if (rc != VTSS_RC_OK) {
            TRACE(ERROR) << "Failed to get trap value";
            return;
        }

        for (auto t : h.notification_vars_) {
            TRACE(DEBUG) << "Send trap";
            vtss_send_v2trap(i.name_.c_str(),h.oid_index_.valid,h.oid_index_.oids,t);
        }
    }

    TRACE(DEBUG) << "Processing trap event - DONE";
}

mesa_rc init(vtss_init_data_t *data)
{
    if (data->cmd == INIT_CMD_INIT) {
        critd_init(&crit_, "snmp.trap", VTSS_MODULE_ID_SNMP, CRITD_TYPE_MUTEX);
    }

    return VTSS_RC_OK;
}

}  // namespace snmp_trap
}  // namespace appl
}  // namespace vtss

extern "C" mesa_rc vtss_appl_snmp_private_trap_init(vtss_init_data_t *data) {
    return vtss::appl::snmp_trap::init(data);
}

extern "C" mesa_rc vtss_appl_snmp_private_trap_listen_add(char *source) {
    std::string s(source);
    return vtss::appl::snmp_trap::listen_add(s);
}

extern "C" mesa_rc vtss_appl_snmp_private_trap_listen_del(char *source) {
    std::string s(source);
    return vtss::appl::snmp_trap::listen_del(s);
}

extern "C" mesa_rc vtss_appl_snmp_private_trap_listen_get_next(char *source, size_t len) {
    mesa_rc rc;
    std::string s(source);
    if ((rc = vtss::appl::snmp_trap::listen_get_next(s)) == VTSS_RC_OK) {
        if (s.size() > len) {
            TRACE(ERROR) << "Buffer size too small";
            return VTSS_RC_ERROR;
        }
        strcpy(source, s.c_str());
    }
    return rc;
}

