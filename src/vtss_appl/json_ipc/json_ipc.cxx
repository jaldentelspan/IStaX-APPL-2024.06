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

#include "main.h"
#include "critd_api.h"
#include <sys/un.h>

#include <string>
#include "subject.hxx"
#include <vtss/basics/set.hxx>
#include <vtss/basics/map.hxx>
#include "vtss/basics/trace.hxx"
#include "vtss/basics/vector.hxx"
#include "vtss/basics/memory.hxx"
#include "vtss/basics/json-rpc-function.hxx"
#include "vtss/basics/expose/json/loader.hxx"
#include "vtss/basics/expose/json/root-node.hxx"
#include "vtss/basics/expose/json/method-split.hxx"
#include <vtss/basics/expose/json/notification.hxx>
#include <vtss/basics/notifications.hxx>

using namespace vtss::notifications;
using namespace vtss::expose::json;

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_JSON_IPC
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_JSON_IPC

#define VTSS_TRACE_GRP_DEFAULT 0

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "json_ipc", "JSON IPC Server"
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_ERROR
    },
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

/* Maximum number of connections */
#define JSON_IPC_CON_MAX 32

/* Message format: 4 bytes length field followed by data */
#define JSON_IPC_HDR_LEN 4

namespace vtss {

extern RootNode JSON_RPC_ROOT;

namespace appl {

namespace json_rpc_notification {
extern Notification *notification_find(const std::string &s);
}  // namespace json_rpc_notification

namespace json_ipc {

struct EventConf {
    EventConf(std::string n) : n_(n) {}

    EventConf(std::string n, EventHandler *h, Notification *o)
    : n_(n), e_(h), o_(o) {
        VTSS_TRACE(DEBUG) << "observer_new " << n_;
        if (o_->observer_new(&e_) == VTSS_RC_OK) {
            attached_ = true;
        } else {
            VTSS_TRACE(ERROR) << "FAILED: observer_new " << n_;
        }
    }

    ~EventConf() {
        if (attached_) {
            VTSS_TRACE(DEBUG) << "observer_del " << n_;
            if (o_->observer_del(&e_) != VTSS_RC_OK) {
                VTSS_TRACE(ERROR) << "FAILED: observer_del " << n_;
            }
        }
    }

    mesa_rc get(std::string &s) const {
        mesa_rc rc;

        if (attached_) {
            VTSS_TRACE(DEBUG) << "observer_get " << n_;
            if ((rc = o_->observer_get(&e_, s)) != VTSS_RC_OK) {
                VTSS_TRACE(ERROR) << "FAILED: observer_get " << n_;
            }
        } else {
            VTSS_TRACE(ERROR) << "FAILED: not attached " << n_;
            rc = VTSS_RC_ERROR;
        }
        return rc;
    }

    bool is_attached() const { return attached_; }
    bool attached_ = false;
    std::string n_;
    mutable Event e_;
    Notification *o_ = nullptr;
};

bool operator<(const EventConf &a, const EventConf &b)
{
    return (a.n_ < b.n_);
}

struct Connection : public EventHandler {
    Connection() :
        EventHandler(&subject_main_thread),
        state(State::FREE),
        fd(this) {};

    // Associate connection with a file-descriptor
    bool assign(Fd &&fd_new, int i) {
        if (state != State::FREE){
            T_I("connection %u used", i);
            return false;
        }
        sprintf(con_txt, "con[%u]:", i);
        T_I("%s free", con_txt);
        fd.assign(vtss::move(fd_new));
        new_state(State::IDLE);
        return true;
    }

    // Start event transmission
    bool tx_event(Event *e) {
        for (auto i = events.begin(); i != events.end(); i++) {
            if (e == &i->e_) {
                i->get(output.buf);
                tx_cnt = 0;
                new_state(State::TX_MSG);
                return true;
            }
        }
        return false;
    }

    // Callback when something happens on one of the events
    void execute(Event *e) override {
        if (state == State::IDLE) {
            T_I("%s tx event", con_txt);
            (void)tx_event(e);
        } else {
            T_I("%s queue event", con_txt);
            pending_events.push_back(*e);
        }
    }

    // Callback when something happens on the file descriptor
    void execute(EventFd *e) override {
        T_I("%s %s", con_txt, state_txt(state));
        switch (state) {
        case State::FREE:
            break;

        case State::IDLE:
            if (e->events() & EventFd::READ) {
                char hdr[JSON_IPC_HDR_LEN], *p = hdr;
                u32  len;

                if (read(e->raw(), hdr, JSON_IPC_HDR_LEN) < JSON_IPC_HDR_LEN) {
                    T_I("header small");
                } else if ((len = *(u32 *)p) == 0 || len > (4*1024*1024)) {
                    T_I("illegal length: %u", len);
                } else if ((rx_buf = (char *)VTSS_MALLOC(len + 1)) == NULL) {
                    T_I("malloc failed");
                } else {
                    T_I("len: %u", len);
                    rx_buf[len] = '\0';
                    rx_len = len;
                    rx_cnt = 0;
                    new_state(State::RX_REQ);
                    break;
                }
                // Close in case of error
                close();
            }
            break;

        case State::RX_REQ:
            if (e->events() & EventFd::READ) {
                int n;

                if ((n = read(e->raw(), rx_buf + rx_cnt, rx_len - rx_cnt)) <= 0) {
                    T_I("no data");
                    close();
                    break;
                }

                rx_cnt += n;
                if (rx_cnt < rx_len) {
                    T_I("awaiting more data, got %u bytes", n);
                    new_state(State::RX_REQ);
                    break;
                }

                T_I("Request[%u]: %s", rx_len, rx_len > 256 ? "<long request>" : rx_buf);

                Request req;
                Loader  l(rx_buf, rx_buf + rx_cnt);

                if (!l.load(req)) {
                    T_I("load failed");
                    close();
                } else if (notif(&req)) {
                    T_I("notification");
                    rx_free();
                    new_state(State::IDLE);
                } else {
                    T_I("request");
                    (void)JSON_RPC_ROOT.handle(0x7fffffff, str(req.method.begin()), &req, output);
                    rx_free();
                    tx_cnt = 0;
                    new_state(State::TX_MSG);
                }
            }
            break;

        case State::TX_MSG:
            if (e->events() & EventFd::WRITE) {
                char hdr[JSON_IPC_HDR_LEN], *p = hdr;
                int  n = output.buf.size();

                if (tx_cnt >= n) {
                    // Transmit done
                    output.clear();
                    new_state(State::IDLE);
                    break;
                }

                // Write header
                if (tx_cnt == 0) {
                    *(u32 *)p = n;
                    if (write(e->raw(), hdr, JSON_IPC_HDR_LEN) < JSON_IPC_HDR_LEN) {
                        T_I("header write failed");
                        close();
                        break;
                    }
                    T_I("wrote header");
                }

                // Write data
                if ((n = write(e->raw(), output.cstring() + tx_cnt, n - tx_cnt)) < 0) {
                    T_I("write failed");
                    close();
                    break;
                }
                T_I("wrote %u data bytes", n);
                tx_cnt += n;
                new_state(State::TX_MSG);
            }
            break;
        }
    }

    bool notif(const Request *req) {
        const char *name = "jsonIpc.config.notification.";
        const char *method = req->method.begin();

        /* Check method base name match */
        if (strstr(method, name) != method || req->params.size() != 1) {
            return false;
        }

        /* Remote quotes from event string */
        str str = req->params[0].as_str();
        VTSS_TRACE(INFO) << "method: " << method << ", params[0]: " << str;
        if (str.size() < 3) {
            return false;
        }
        std::string event_name(str.begin() + 1, str.end() - 1);

        method += strlen(name);
        if (strcmp("add", method) == 0) {
            auto noti = json_rpc_notification::notification_find(event_name);
            if (noti) {
                events.emplace(event_name, this, noti);
                return true;
            }
        } else if (strcmp("del", method) == 0) {
            auto itr = events.find(event_name);
            if (itr != events.end()) {
                events.erase(itr);
                return true;
            }
        }
        return false;
    }

    void rx_free() {
        if (rx_buf != NULL) {
            VTSS_FREE(rx_buf);
            rx_buf = NULL;
        }
    }

    void close() {
        fd.unsubscribe();
        events.clear();
        output.clear();
        state = State::FREE;
        rx_free();
    }

    enum class State {
        FREE,    // No file-descriptor associated
        IDLE,    // File descriptor is associated, but the connection is idle
        RX_REQ,  // Reading a request
        TX_MSG   // Transmitting a response
    } state;

    const char *state_txt(State s) {
        return (s == State::IDLE ? "IDLE" : s == State::RX_REQ ? "RX_REQ" : s == State::TX_MSG ? "TX_MSG" : "FREE");
    }

    // Change state and signal thread
    void new_state(State s) {
        if (s == State::IDLE) {
            // Check for pending events
            while (!pending_events.empty()) {
                Event *e = &*pending_events.begin();
                pending_events.pop_front();
                if (tx_event(e)) {
                    return;
                }
            }
        }
        if (state != s) {
            T_I("%s %s -> %s", con_txt, state_txt(state), state_txt(s));
            state = s;
        }
        subject_main_thread.event_fd_add(fd, s == State::TX_MSG ? EventFd::WRITE : EventFd::READ);
    }

    EventFd fd;
    char con_txt[16];
    u32  rx_len;
    u32  rx_cnt;
    char *rx_buf;
    StringStream output;
    u32 tx_cnt;

    vtss::Set<EventConf> events;
    intrusive::List<Event> pending_events;
};

static Connection con_table[JSON_IPC_CON_MAX];

struct Server : public EventHandler {
    Server() : EventHandler(&subject_main_thread), fd(this) {};

    // Start server
    void start() {
        int                s;
        struct sockaddr_un local;

        local.sun_family = AF_UNIX;
        strcpy(local.sun_path, "/var/run/json_ipc.socket");
        unlink(local.sun_path);

        if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            T_E("socket failed (%s)", strerror(errno));
        } else if (bind(s, (struct sockaddr *)&local, sizeof(local.sun_family) + strlen(local.sun_path)) < 0) {
            T_E("bind failed");
            close(s);
        } else if (listen(s, JSON_IPC_CON_MAX) < 0) {
            T_E("listen failed");
            close(s);
        } else {
            Fd fd_new(s);
            fd.assign(vtss::move(fd_new));
            subject_main_thread.event_fd_add(fd, EventFd::READ);
        }
    }

    // Handle events
    void execute(EventFd *e) override {
        if (e->events() & EventFd::READ) {
            int                n, i;
            struct sockaddr_un remote;
            socklen_t          rl = sizeof(remote);
            struct ucred       cr;
            socklen_t          cl = sizeof(cr);

            T_I("new connection");
            if ((n = accept(e->raw(), (struct sockaddr *)&remote, &rl)) < 0) {
                T_I("accept failed (%s)", strerror(errno));
            } else if (getsockopt(n, SOL_SOCKET, SO_PEERCRED, &cr, &cl) < 0) {
                T_I("getsockopt failed (%s)", strerror(errno));
                close(n);
            } else if (cr.uid != 0) {
                /* Peer is not root */
                T_I("not root, closing");
                close(n);
            } else {
                Fd fd_new(n);
                for (i = 0; i < JSON_IPC_CON_MAX; i++) {
                    if (con_table[i].assign(vtss::move(fd_new), i)) {
                        break;
                    }
                }
                if (i == JSON_IPC_CON_MAX) {
                    T_I("no free connections");
                }
            }
            subject_main_thread.event_fd_add(fd, EventFd::READ);
        }
    }

    EventFd fd;
};

static Server server;

static mesa_rc init(vtss_init_data_t *data)
{
    switch (data->cmd) {
    case INIT_CMD_START:
        T_I("START");
        server.start();
        break;

    default:
        break;
    }
    return VTSS_RC_OK;
}

} /* namespace json_ipc */
} /* namespace appl */
} /* namespace vtss */

mesa_rc json_ipc_init(vtss_init_data_t *data)
{
    return vtss::appl::json_ipc::init(data);
}
