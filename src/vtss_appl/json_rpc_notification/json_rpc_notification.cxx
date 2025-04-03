/*

 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#include "json_rpc_trace.h"
#include "vtss/basics/trace.hxx"
#include "vtss/basics/parser_impl.hxx"
#include "json_rpc_notification.hxx"

#include <vtss/basics/set.hxx>
#include <vtss/basics/map.hxx>
#include "vtss/basics/expose/json/root-node.hxx"
#include "vtss/basics/expose/json/method-split.hxx"
#include <vtss/basics/expose/json/notification.hxx>
#include <vtss/basics/notifications.hxx>
#include "json_rpc_notification_http_client.hxx"

#define TRACE(X) VTSS_TRACE(VTSS_TRACE_JSON_RPC_GRP_NOTI, X)
#define TRACE_ASYNC(X) VTSS_TRACE(VTSS_TRACE_JSON_RPC_GRP_NOTI_ASYNC, X)

mesa_rc vtss_appl_json_rpc_notification_icfg_init();

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
extern "C" void vtss_json_rpc_notification_mib_init();
#endif
#if defined(VTSS_SW_OPTION_JSON_RPC)
void vtss_appl_json_rpc_notification_json_init();
#endif

namespace vtss {
extern vtss::expose::json::RootNode JSON_RPC_ROOT;
namespace appl {
namespace json_rpc_notification {
static critd_t crit_;
struct Lock {
    Lock(int line) {
        critd_enter(&crit_, __FILE__, line);
    }
    ~Lock() {
        critd_exit( &crit_, __FILE__, 0);
    }
};
#define CRIT_SCOPE() Lock __lock_guard__(__LINE__)

static vtss_flag_t   libfetch_flags;
static vtss_handle_t libfetch_thread_handle;
static vtss_thread_t libfetch_thread_block;
static std::string   libfetch_thread_post_data;
static DestConf      libfetch_thread_dest;
#define LIBFETCH_THREAD_HTTP_START 1

struct Handler : public notifications::EventHandler {
    Handler()
        : notifications::EventHandler(&notifications::subject_main_thread) {}
    void execute(notifications::Event *e);
    void execute(notifications::Timer *e);
} handler;

struct Queue : public notifications::EventHandler {
    Queue()
        : notifications::EventHandler(&notifications::subject_main_thread) {}
    void execute(notifications::Event *e);
    void execute(notifications::Timer *e) {}
} queue;

notifications::Subject<bool> libfetch_event_pending;
notifications::Subject<bool> libfetch_idle;
notifications::Event libfetch_event_pending_event(&queue);
notifications::Event libfetch_idle_event(&queue);

struct EventConf {
    EventConf(std::string n) : n_(n) {}

    EventConf(DestConf *d, std::string n, expose::json::Notification *o)
        : d_(d), n_(n), e_(&handler), o_(o) {
        TRACE(DEBUG) << "observer_new " << n_.c_str();
        mesa_rc rc = o_->observer_new(&e_);
        if (rc == VTSS_RC_OK) {
            attached_ = true;
        } else {
            TRACE(ERROR) << "FAILED: observer_new " << n_.c_str();
        }
    }

    ~EventConf() {
        if (attached_) {
            TRACE(DEBUG) << "observer_del " << n_.c_str();
            mesa_rc rc = o_->observer_del(&e_);
            if (rc != VTSS_RC_OK) {
                TRACE(ERROR) << "FAILED: observer_del " << n_.c_str();
            }
        }
    }

    mesa_rc get(std::string &s) const {
        if (!attached_) {
            TRACE(ERROR) << "FAILED: not attached " << n_.c_str();
            return VTSS_RC_ERROR;
        }

        TRACE(DEBUG) << "observer_get " << n_.c_str();
        mesa_rc rc = o_->observer_get(&e_, s);
        if (rc != VTSS_RC_OK) {
            TRACE(ERROR) << "FAILED: observer_get " << n_.c_str();
        }

        return rc;
    }

    bool is_attached() const { return attached_; }
    bool attached_ = false;
    DestConf *d_ = nullptr;
    std::string n_;
    mutable notifications::Event e_;
    expose::json::Notification *o_ = nullptr;
};

struct SnmpStringLess {
    bool operator()(const std::string &a, const std::string &b) const {
        if (a.size() != b.size()) return a.size() < b.size();
        return a < b;
    }
};

bool operator<(const EventConf &a, const EventConf &b) {
    SnmpStringLess cmp;
    return cmp(a.n_, b.n_);
}

// maps a destination name to a destinaiton configuration
vtss::Map<std::string, DestConf /*, SnmpStringLess*/> destinations;

// maps a destination name to a set of EventConf's
vtss::Map<std::string, vtss::Set<EventConf> /*, SnmpStringLess*/> events;

// reverse map a EventConf::e pointer to value in destinations
vtss::Map<const void *, const EventConf *> event_to_dest;

// Event queue
intrusive::List<notifications::Event> pending_events;

void Handler::execute(notifications::Event *e) {
    CRIT_SCOPE();

    auto i = event_to_dest.find(e);
    if (i == event_to_dest.end()) {
        TRACE(ERROR) << "Unknown event: " << (void *)e;
        return;
    }

    TRACE(DEBUG) << "Pushing event to queue: " << e;
    pending_events.push_back(*e);
    libfetch_event_pending.set(true);
}

void start_new_libfetch_job(notifications::Event *e) {
    auto i = event_to_dest.find(e);
    if (i == event_to_dest.end()) {
        TRACE(ERROR) << "Unknown event: " << (void *)e;
        libfetch_idle.signal();
        return;
    }

    if (!libfetch_idle.get()) {
        TRACE(ERROR) << "Libfetch is not ready - event is lost!";
        std::string s;
        i->second->get(s);
        return;
    }

    mesa_rc rc = i->second->get(libfetch_thread_post_data);
    if (rc != VTSS_RC_OK) {
        TRACE(ERROR) << "Failed to get event from " << i->second->n_.c_str();
        return;
    }

    if (!i->second->d_->url.size()) {
        TRACE(INFO) << "Event-skipped: (no-url) " << i->second->n_.c_str()
                    << " " << libfetch_thread_post_data.c_str();
        return;
    }

    libfetch_thread_dest = *(i->second->d_);

    libfetch_idle.set(false);
    vtss_flag_setbits(&libfetch_flags, LIBFETCH_THREAD_HTTP_START);

    TRACE(INFO) << "Event: " << i->second->n_.c_str() << " "
                << libfetch_thread_post_data.c_str();
}

void Queue::execute(notifications::Event *e) {
    CRIT_SCOPE();

    if (e == &libfetch_event_pending_event) {
        if (!libfetch_event_pending.get(libfetch_event_pending_event)) {
            TRACE(DEBUG) << "No more pending events";
            return;
        } else {
            TRACE(DEBUG) << "New pending event";
        }

        // TODO, does not work for multiple destinations
        auto i = destinations.begin();
        if (i == destinations.end()) {
            TRACE(DEBUG) << "No destinations found";
            libfetch_event_pending.set(false);
            return;
        }

        notifications::Event *ee = &*pending_events.begin();
        pending_events.pop_front();
        start_new_libfetch_job(ee);

    } else if (e == &libfetch_idle_event) {
        if (libfetch_idle.get(libfetch_idle_event)) {
            TRACE(DEBUG) << "Look at pending events";

            // TODO, does not work for multiple destinations
            auto i = destinations.begin();
            if (i == destinations.end()) {
                TRACE(DEBUG) << "No destinations found";
                libfetch_event_pending.set(false);
                return;
            }

            if (pending_events.empty()) {
                TRACE(DEBUG) << "No more pending events";
                libfetch_event_pending.set(false);
                return;
            }

            notifications::Event *ee = &*pending_events.begin();
            pending_events.pop_front();
            start_new_libfetch_job(ee);

        } else {
            TRACE(NOISE) << "Event processing started";
            return;
        }

    } else {
        TRACE(ERROR) << "Unknown event";
    }
}

void Handler::execute(notifications::Timer *e) { CRIT_SCOPE(); }

expose::json::Notification *notification_find(const std::string &s) {
    typedef vtss::intrusive::List<vtss::expose::json::Node>::iterator I;
    str q(s.c_str());
    str method_head, method_tail;
    expose::json::NamespaceNode *ns = &JSON_RPC_ROOT;
    expose::json::Node *node = ns;

    TRACE(DEBUG) << "Notification search: " << q;
    while (q.size()) {
        expose::json::method_split(q, method_head, method_tail);
        TRACE(NOISE) << "Method name: " << q << " head: " << method_head
                     << " tail: " << method_tail;

        if (node->is_namespace_node()) {
            ns = static_cast<expose::json::NamespaceNode *>(node);
        } else {
            TRACE(DEBUG) << "Could not find " << s.c_str();
            return nullptr;
        }

        node = nullptr;
        for (I i = ns->leafs.begin(); i != ns->leafs.end(); ++i) {
            if (method_head == i->name()) {
                TRACE(DEBUG) << "Match: " << method_head;
                node = &*i;
                break;

            } else {
                TRACE(NOISE) << "No match: " << i->name();
            }
        }

        if (!node) {
            TRACE(DEBUG) << "Could not find " << s.c_str();
            return nullptr;
        }

        q = method_tail;
    }

    if (!node->is_notification()) {
        TRACE(DEBUG) << "Found, but is not a notification: " << s.c_str();
        return nullptr;
    }

    auto res = static_cast<expose::json::Notification *>(node);
    TRACE(DEBUG) << "Notification found: " << s.c_str() << " " << (void *)res;
    return res;
}

mesa_rc dest_get(const std::string &dest_name, DestConf &conf) {
    CRIT_SCOPE();

    auto i = destinations.find(dest_name);
    if (i == destinations.end()) return Error::DESTINATION_DOES_NOT_EXISTS;

    conf = i->second;
    return VTSS_RC_OK;
}

mesa_rc dest_set(const std::string &dest_name, const DestConf &conf) {
    CRIT_SCOPE();

    auto i = destinations.find(dest_name);
    if (i == destinations.end()) {
        if (destinations.size() > 0) return Error::DESTINATION_TOO_MANY;
    }

    if (conf.url.size()) {
        vtss::parser::Uri url;
        const char *url_b = &*conf.url.begin();
        const char *url_e = conf.url.c_str() + conf.url.size();
        if (!url(url_b, url_e)) return Error::URL_INVALID;
        TRACE(INFO) << url.host << " " << url.port << " " << url.path;
        if (url_b != url_e) return Error::URL_INVALID;
        if (url.query.size()) return Error::URL_QUERY_NOT_ALLOWED;
        if (url.fragment.size()) return Error::URL_FRAGMENT_NOT_ALLOWED;
        if (url.userinfo.size()) return Error::URL_USERINFO_NOT_ALLOWED;
        if (url.scheme != str("http")) return Error::URL_PROTOCOL_NOT_SUPPORTED;
    }

    destinations.set(dest_name, conf);
    return VTSS_RC_OK;
}

mesa_rc dest_del(const std::string &dest_name) {
    CRIT_SCOPE();

    auto i = destinations.find(dest_name);
    if (i == destinations.end()) return Error::DESTINATION_DOES_NOT_EXISTS;

    // Implement the dependent relation ship
    auto j = events.find(dest_name);
    if (j != events.end()) {
        // Delete all the cross references found in event_to_dest
        for (auto &e : j->second) {
            auto jj = event_to_dest.find(&e.e_);
            if (jj == event_to_dest.end()) {
                TRACE(ERROR) << "event_to_dest is out of sync!";
            } else {
                event_to_dest.erase(jj);
            }
        }

        // delete the entire set - the destructor will take care of deleting the
        // observers
        events.erase(j);
    }

    // Finally, delete the destination - all references should have been removed
    destinations.erase(i);
    return VTSS_RC_OK;
}

void dest_del_all() {
    uint32_t cnt = 0;
    while (cnt < 100) {
        std::string s;
        {
            CRIT_SCOPE();
            auto i = destinations.begin();
            if (i == destinations.end()) return;
            s = i->first;
        }

        if (dest_del(s) != VTSS_RC_OK) {
            TRACE(ERROR) << "Failed to delete: " << s;
            cnt++;
        }
    }
}

mesa_rc dest_itr(const std::string &in, std::string &out) {
    CRIT_SCOPE();

    auto i = destinations.greater_than(in);
    if (i == destinations.end()) return VTSS_RC_ERROR;

    out = i->first;

    return VTSS_RC_OK;
}

mesa_rc event_subscribe_get(const std::string &dest_name,
                            const std::string &event_name) {
    CRIT_SCOPE();

    auto i = destinations.find(dest_name);
    if (i == destinations.end()) return Error::DESTINATION_DOES_NOT_EXISTS;

    auto noti = notification_find(event_name);
    if (!noti) return Error::EVENT_DOES_NOT_EXISTS;

    auto ev = events.get(dest_name);
    if (ev->second.find(event_name) == ev->second.end())
        return VTSS_RC_ERROR;
    else
        return VTSS_RC_OK;
}

mesa_rc event_subscribe_add(const std::string &dest_name,
                            const std::string &event_name) {
    CRIT_SCOPE();

    auto i = destinations.find(dest_name);
    if (i == destinations.end()) return Error::DESTINATION_DOES_NOT_EXISTS;

    auto noti = notification_find(event_name);
    if (!noti) return Error::EVENT_DOES_NOT_EXISTS;

    auto j = events.get(dest_name)->second.emplace(&i->second, event_name,
                                                   noti);
    event_to_dest.set(&(j.first->e_), &(*j.first));


    return VTSS_RC_OK;
}

mesa_rc event_subscribe_del(const std::string &dest_name,
                            const std::string &event_name) {
    CRIT_SCOPE();

    auto i = destinations.find(dest_name);
    if (i == destinations.end()) return Error::DESTINATION_DOES_NOT_EXISTS;

    auto ev = events.get(dest_name);
    auto j = ev->second.find(event_name);
    if (j == ev->second.end()) return Error::EVENT_NO_SUBSCRIPTION;

    auto jj = event_to_dest.find(&(j->e_));
    if (jj == event_to_dest.end()) {
        TRACE(ERROR) << "event_to_dest is out of sync!";
    } else {
        event_to_dest.erase(jj);
    }

    ev->second.erase(j);
    return VTSS_RC_OK;
}

mesa_rc event_subscribe_itr(const std::string &dest_name_in,
                            std::string &dest_name_out,
                            const std::string &event_name_in,
                            std::string &event_name_out) {
    CRIT_SCOPE();

    TRACE(DEBUG) << "in " << dest_name_in.c_str() << " "
                 << event_name_in.c_str();
    auto i = destinations.find(dest_name_in);
    if (i == destinations.end()) i = destinations.greater_than(dest_name_in);
    if (i == destinations.end()) return VTSS_RC_ERROR;

    while (true) {
        auto ev = events.get(i->first);
        auto j = ev->second.greater_than(event_name_in);
        if (j != ev->second.end()) {
            dest_name_out = i->first;
            event_name_out = j->n_;
            TRACE(DEBUG) << "out " << dest_name_out.c_str() << " "
                         << event_name_out.c_str();
            return VTSS_RC_OK;
        }

        i = destinations.greater_than(i->first);
        if (i == destinations.end()) return VTSS_RC_ERROR;
    }

    return VTSS_RC_ERROR;
}

static bool complete_events_valid_ = false;
static Vector<std::string> complete_events_;
void walk_(expose::json::Node *node);
void walk__(expose::json::Node *node) {
    if (!node->is_notification()) return;
    complete_events_.push_back(node->abs_name());
}
void walk__(expose::json::NamespaceNode *ns) {
    for (auto i = ns->leafs.begin(); i != ns->leafs.end(); ++i) walk_(&*i);
}
void walk_(expose::json::Node *node) {
    if (node->is_namespace_node())
        walk__(static_cast<expose::json::NamespaceNode *>(node));
    else
        walk__(node);
}
const Vector<std::string> *event_complete_db() {
    CRIT_SCOPE();
    if (complete_events_valid_) return &complete_events_;
    walk_(&JSON_RPC_ROOT);
    complete_events_valid_ = true;
    return &complete_events_;
}

static void libfetch_thread(vtss_addrword_t data) {
    vtss_flag_value_t f;

    while (1) {
        f = vtss_flag_wait(&libfetch_flags, 0xFFFFFFFF,
                           VTSS_FLAG_WAITMODE_OR_CLR);

        if (f & LIBFETCH_THREAD_HTTP_START) {
            TRACE_ASYNC(INFO) << "Post: " << libfetch_thread_post_data.size()
                              << " bytes to "
                              << libfetch_thread_dest.url.c_str();
            int status =
                    http_post(libfetch_thread_dest, libfetch_thread_post_data);
            TRACE_ASYNC(INFO) << "Done " << status;
            libfetch_idle.set(true);
        }
    }
}

extern "C" int json_rpc_notification_icli_cmd_register();

mesa_rc init(vtss_init_data_t *data) {
    vtss_isid_t isid = data->isid;
    TRACE(DEBUG) << "enter, cmd: " << data->cmd << ", isid: " << data->isid
                 << ", flags: 0x" << vtss::HEX(data->flags);

    switch (data->cmd) {
    case INIT_CMD_INIT:
        TRACE(INFO) << "INIT";
        critd_init(&crit_, "json_rpc_notification",
                   VTSS_MODULE_ID_JSON_RPC_NOTIFICATION, CRITD_TYPE_MUTEX);

        vtss_flag_init(&libfetch_flags);
        libfetch_event_pending.set(false);
        libfetch_idle.set(true);
        libfetch_event_pending.attach(libfetch_event_pending_event);
        libfetch_idle.attach(libfetch_idle_event);
        vtss_appl_json_rpc_notification_icfg_init();

#if defined(VTSS_SW_OPTION_PRIVATE_MIB)
        vtss_json_rpc_notification_mib_init();
#endif
#if defined(VTSS_SW_OPTION_JSON_RPC)
        vtss_appl_json_rpc_notification_json_init();
#endif
        json_rpc_notification_icli_cmd_register();
        break;

    case INIT_CMD_START:
        TRACE(INFO) << "START";
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           libfetch_thread,
                           0,
                           "JSON RPC Notification",
                           nullptr,
                           0,
                           &libfetch_thread_handle,
                           &libfetch_thread_block);
        break;

    case INIT_CMD_CONF_DEF:
        TRACE(INFO) << "CONF_DEF, isid: " << isid;
        dest_del_all();
        break;

    default:
        break;
    }

    TRACE(DEBUG) << "exit";
    return VTSS_RC_OK;
}

}  // namespace vtss
}  // namespace appl
}  // namespace json_rpc_notification

