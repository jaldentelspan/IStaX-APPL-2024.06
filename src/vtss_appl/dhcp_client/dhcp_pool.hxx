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

#ifndef _DHCP_POOL_H_
#define _DHCP_POOL_H_

#include "dhcp_client.hxx"

#define VTSS_NEW(DST, TYPE, ...)                \
    DST = (TYPE *)VTSS_CALLOC(1, sizeof(TYPE)); \
    if (DST) {                                  \
        new (DST) TYPE(__VA_ARGS__);            \
    }

#define VTSS_DELETE(DST, DESTRUCTOR) \
    if (DST) {                       \
        DST->DESTRUCTOR();           \
        VTSS_FREE(DST);              \
    }


extern "C" {
#include "ip_utils.hxx"
}


#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_DHCP_CLIENT
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_DHCP_CLIENT

#define OK_OR_RETURN_ERR(IFIDX)                           \
    do {                                                  \
        if (enabled.find(IFIDX)==enabled.end() ||         \
            !enabled[IFIDX]) {                            \
            return VTSS_RC_ERROR;                         \
        }                                                 \
    } while (0)


namespace vtss {
namespace dhcp {

template <typename FrameTxService, typename TimerService, typename Lock,
          unsigned MAX>
class DhcpPool {
  public:
    typedef Client<FrameTxService, TimerService, Lock> DhcpClient_t;

    struct CStyleCB : public notifications::EventHandler {
        enum { MAX_CALLEES = 1 };

        CStyleCB(TimerService *ts, DhcpClient_t *c, vtss_ifindex_t v)
            : notifications::EventHandler(ts),
              trigger(this),
              client_(c),
              ifidx_(v) {
            for (int i = 0; i < MAX_CALLEES; ++i) callees[i] = 0;
        }

        mesa_rc callback_add(client_callback_t cb) {
            int free = -1;

            for (int i = 0; i < MAX_CALLEES; ++i) {
                if (callees[i] == 0) {
                    free = i;
                }

                if (callees[i] == cb) {
                    T_I("Callback already exists");
                    return VTSS_RC_OK;
                }
            }

            if (free != -1) {
                callees[free] = cb;
                return VTSS_RC_OK;
            } else {
                return VTSS_RC_ERROR;
            }
        }

        mesa_rc callback_del(client_callback_t cb) {
            for (int i = 0; i < MAX_CALLEES; ++i) {
                if (callees[i] != cb) {
                    continue;
                }
                callees[i] = 0;
                return VTSS_RC_OK;
            }
            return VTSS_RC_ERROR;
        }

        void execute(notifications::Event *t) {
            client_->ack_conf(*t);

            T_I("Invoke callbacks %p", t);
            for (int i = 0; i < MAX_CALLEES; ++i) {
                client_callback_t cb = callees[i];
                if (cb == 0) {
                    continue;
                }
                T_I("  Invoke callback");
                cb(ifidx_);
            }
        }

        void start() {
            client_->ack_conf(trigger);
        }

        void stop() {
            trigger.unlink();
            for (int i = 0; i < MAX_CALLEES; ++i) callees[i] = 0;
        }

        notifications::Event trigger;
        DhcpClient_t *client_;
        const vtss_ifindex_t ifidx_;
        client_callback_t callees[MAX_CALLEES];
    };


    DhcpPool(FrameTxService &tx, TimerService &ts, Lock &l)
        : tx_(tx), ts_(ts), lock_(l) {
    }

    mesa_rc start(vtss_ifindex_t ifidx, const vtss_appl_ip_dhcp4c_param_t *params, bool if_is_up) {

        if (client.find(ifidx) == client.end()) {
            T_I("Creating new client");
            VTSS_NEW(client[ifidx], DhcpClient_t, tx_, ts_, lock_, ifidx, *params);
            VTSS_NEW(c_style_cb[ifidx], CStyleCB, &ts_, client[ifidx], ifidx);
            enabled[ifidx] = false;
        } else {
            if (enabled[ifidx]) {
                T_I("Restarting existing client");
            }
            // Update the client parameteres.
            client[ifidx]->params_set(params);
        }

        if (client[ifidx] == 0 || c_style_cb[ifidx] == 0) {
            VTSS_DELETE(c_style_cb[ifidx], ~CStyleCB);
            VTSS_DELETE(client[ifidx], ~DhcpClient_t);
            c_style_cb.erase(ifidx);
            client.erase(ifidx);
            enabled.erase(ifidx);
            return VTSS_RC_ERROR;
        }

        enabled[ifidx] = true;

        // Always invoke c_style_cb[]->start(), because that one hands over a
        // trigger to the DHCP client on which it can send events, which first
        // are captured by CStyleCB::execute() and then handed over to whoever
        // listens to changes (currently only the IP module).
        c_style_cb[ifidx]->start();

        if (if_is_up) {
            return client[ifidx]->start();
        } else {
            return client[ifidx]->stop();
        }
    }

    mesa_rc kill(vtss_ifindex_t ifidx) {
        OK_OR_RETURN_ERR(ifidx);

        mesa_rc rc;
        c_style_cb[ifidx]->stop();
        rc = client[ifidx]->stop();
        enabled[ifidx] = false;

        return rc;
    }

    mesa_rc stop(vtss_ifindex_t ifidx) {
        OK_OR_RETURN_ERR(ifidx);
        return client[ifidx]->stop();
    }

    mesa_rc fallback(vtss_ifindex_t ifidx) {
        OK_OR_RETURN_ERR(ifidx);
        return client[ifidx]->fallback();
    }

    mesa_rc if_down(vtss_ifindex_t ifidx) {
        OK_OR_RETURN_ERR(ifidx);
        return client[ifidx]->if_down();
    }

    mesa_rc if_up(vtss_ifindex_t ifidx) {
        OK_OR_RETURN_ERR(ifidx);
        return client[ifidx]->if_up();
    }

    mesa_rc release(vtss_ifindex_t ifidx) {
        OK_OR_RETURN_ERR(ifidx);
        return client[ifidx]->release();
    }

    mesa_rc decline(vtss_ifindex_t ifidx) {
        OK_OR_RETURN_ERR(ifidx);
        return client[ifidx]->decline();
    }

    mesa_rc bind(vtss_ifindex_t ifidx) {
        OK_OR_RETURN_ERR(ifidx);
        return client[ifidx]->bind();
    }

    DhcpClient_t *get(vtss_ifindex_t ifidx) {
        if (client.find(ifidx) == client.end()) {
            return NULL;
        }
        return client[ifidx];
    }

    BOOL bound_get(vtss_ifindex_t ifidx) {
        OK_OR_RETURN_ERR(ifidx);
        vtss_appl_ip_dhcp4c_state_t s = client[ifidx]->state();

        return s == VTSS_APPL_IP_DHCP4C_STATE_BOUND ||
               s == VTSS_APPL_IP_DHCP4C_STATE_RENEWING ||
               s == VTSS_APPL_IP_DHCP4C_STATE_REBINDING;
    }

    mesa_rc offers_get(vtss_ifindex_t ifidx, size_t max_offers, size_t *valid_offers,
                       ConfPacket *list) {
        OK_OR_RETURN_ERR(ifidx);
        client[ifidx]->offers(max_offers, valid_offers, list);
        return VTSS_RC_OK;
    }

    mesa_rc accept(vtss_ifindex_t ifidx, unsigned idx) {
        OK_OR_RETURN_ERR(ifidx);

        if (client[ifidx]->accept(idx)) {
            return VTSS_RC_OK;
        } else {
            return VTSS_RC_ERROR;
        }
    }

    mesa_rc status(vtss_ifindex_t ifidx, vtss_appl_ip_if_status_dhcp4c_t *status) {
        OK_OR_RETURN_ERR(ifidx);
        vtss_appl_ip_if_status_dhcp4c_t s = client[ifidx]->status();
        *status = s;
        return VTSS_RC_OK;
    }

    mesa_rc callback_add(vtss_ifindex_t ifidx, client_callback_t cb) {
        OK_OR_RETURN_ERR(ifidx);
        return c_style_cb[ifidx]->callback_add(cb);
    }

    mesa_rc callback_del(vtss_ifindex_t ifidx, client_callback_t cb) {
        OK_OR_RETURN_ERR(ifidx);
        return c_style_cb[ifidx]->callback_del(cb);
    }

    mesa_rc fields_get(vtss_ifindex_t ifidx, ConfPacket *fields) {
        OK_OR_RETURN_ERR(ifidx);

        typename DhcpClient_t::AckConfPacket p(client[ifidx]->ack_conf());

        if (p.valid()) {
            *fields = p.get();
            return VTSS_RC_OK;
        } else {
            return VTSS_RC_ERROR;
        }
    }

    mesa_rc dns_option_domain_any_get(vtss::Buffer *name) {

        for (auto itr : enabled) {
            if (itr.second) {
                ConfPacket fields;
                if (fields_get(itr.first, &fields) == VTSS_RC_OK) {
                    if (fields.domain_name.size()) {
                        *name = fields.domain_name;
                        return VTSS_RC_OK;
                    }
                }
            }
        }

        return VTSS_RC_ERROR;
    }

    mesa_rc dns_option_any_get(mesa_ipv4_t prefered, mesa_ipv4_t *ip) {
        mesa_ipv4_t some_ip = 0;

        for (auto itr : enabled) {
            ConfPacket fields;

            if (fields_get(itr.first, &fields) != VTSS_RC_OK) continue;

            if (!fields.domain_name_server.valid()) continue;

            if (fields.domain_name_server.get() == prefered) {
                *ip = fields.domain_name_server.get();
                return VTSS_RC_OK;
            } else {
                some_ip = fields.domain_name_server.get();
            }
        }

        if (some_ip != 0) {
            *ip = some_ip;
            return VTSS_RC_OK;
        }

        return VTSS_RC_ERROR;
    }

    ~DhcpPool() {
        for (auto cb : c_style_cb) {
            VTSS_DELETE(cb.second, ~CStyleCB);
        }
        for (auto c : client) {
            VTSS_DELETE(c.second, ~DhcpClient_t);
        }
        c_style_cb.clear();
        client.clear();
        enabled.clear();
    }

  private:
    FrameTxService &tx_;
    TimerService &ts_;
    Lock &lock_;
    vtss::Map<vtss_ifindex_t, DhcpClient_t *> client;
    vtss::Map<vtss_ifindex_t, CStyleCB *> c_style_cb;
    vtss::Map<vtss_ifindex_t, bool> enabled;
};
};
};

#undef OK_OR_RETURN_ERR
#undef VTSS_TRACE_MODULE_ID
#endif /* _DHCP_POOL_H_ */
