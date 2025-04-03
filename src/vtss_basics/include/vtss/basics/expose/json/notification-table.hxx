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

#ifndef __VTSS_BASICS_EXPOSE_JSON_NOTIFICATION_TABLE_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_NOTIFICATION_TABLE_HXX__

#include <vtss/basics/expose/serialize-keys.hxx>
#include <vtss/basics/expose/serialize-values.hxx>
#include <vtss/basics/expose/json/notification.hxx>
#include <vtss/basics/expose/json/notification-row.hxx>
#include <vtss/basics/expose/json/notification-base.hxx>

namespace vtss {
namespace expose {
namespace json {

template <class INTERFACE, class SUBJECT>
struct NotificationTable : public Notification {
    NotificationTable(SUBJECT *s, NamespaceNode *ns, const char *n,
                      const char *d)
        : Notification(ns, n, d), s_(s) {}

    typedef typename SUBJECT::Observer Observer;

    mesa_rc observer_new(notifications::Event *ev) {
        return s_->observer_new(ev);
    }

    mesa_rc observer_get(notifications::Event *ev, std::string &s) {
        Observer o;
        mesa_rc rc = s_->observer_get(ev, o);
        if (rc != MESA_RC_OK) return rc;
        if (!o.events.size()) return MESA_RC_OK;

        StringStream ss;
        std::string method_name = abs_name();

        NotificationBase msg(&ss, method_name.c_str());
        for (auto &&e : o.events) {
            auto event = (notifications::EventType::E)e.second;

            switch (e.second) {
            case notifications::EventType::Add:
            case notifications::EventType::Modify: {
                typename SUBJECT::V val;
                if (s_->get_(e.first, val) != MESA_RC_OK) {
                    s_->observer_mask_key(ev, event, e.first);
                    break;
                }

                NotificationRow row(event, vtss::move(msg.row()));
                serialize_keys<INTERFACE>(row.key(), e.first);
                serialize_values<INTERFACE>(row.val(), val);
                break;
            }

            case notifications::EventType::Delete: {
                NotificationRow row(event, vtss::move(msg.row()));
                serialize_keys<INTERFACE>(row.key(), e.first);
                break;
            }

            default:
                ;
            }
        }

        msg.close();
        swap(ss.buf, s);
        return MESA_RC_OK;
    }

    mesa_rc observer_del(notifications::Event *ev) {
        return s_->observer_del(ev);
    }

    void handle_reflection(Reflection *r) {}

    vtss::json::Result::Code handle(int priv, str method_name,
                                    const Request *req, ostreamBuf &out) {
        return vtss::json::Result::METHOD_NOT_FOUND;
    }

    SUBJECT *s_;
};

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_NOTIFICATION_TABLE_HXX__
