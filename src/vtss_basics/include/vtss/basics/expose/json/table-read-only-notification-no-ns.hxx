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

#ifndef __VTSS_BASICS_EXPOSE_JSON_TABLE_READ_ONLY_NOTIFICATION_NO_NS_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_TABLE_READ_ONLY_NOTIFICATION_NO_NS_HXX__

#include <vtss/basics/expose/serialize-keys.hxx>
#include <vtss/basics/expose/serialize-values.hxx>
#include <vtss/basics/expose/table-status.hxx>
#include <vtss/basics/expose/json/literal.hxx>
#include <vtss/basics/expose/json/namespace-node.hxx>
#include <vtss/basics/expose/json/notification-table.hxx>
#include <vtss/basics/expose/json/function-exporter-get-map-subject.hxx>
#include <vtss/basics/expose/json/interface-descriptor-description.hxx>
#include <vtss/basics/expose/json/interface-descriptor-desc-get-func.hxx>
#include <vtss/basics/expose/json/function-exporter-get-set-subject.hxx>

namespace vtss {
namespace expose {
namespace json {

template <class INTERFACE, size_t VAL_CNT, class... Args>
struct TableReadOnlyNotificationNoNS_;

template <class INTERFACE, size_t VAL_CNT, class... Args>
struct TableReadOnlyNotificationNoNS_<INTERFACE, VAL_CNT, ParamList<Args...>> {
    typedef TableStatus<Args...> Subject;

    TableReadOnlyNotificationNoNS_(NamespaceNode *p, Subject *table_status)
        : get_(table_status, p, "get",
               InterfaceDescriptorDescGetFunc<INTERFACE>::value),
          event_(table_status, p, "update", "") {
        p->description(InterfaceDescriptorDescription<INTERFACE>::desc);
        get_.implemented(tag::depend_on_capability_get_ptr<INTERFACE>());
        get_.depends_on_capability(
                tag::depend_on_capability_name_t<INTERFACE>());
        get_.depends_on_capability_group(
                tag::depend_on_capability_json_ref_t<INTERFACE>());
        get_.has_notification(true);
    }

    mesa_rc observer_new(notifications::Event *ev) {
        return event_.observer_new(ev);
    }

    mesa_rc observer_del(notifications::Event *ev) {
        return event_.observer_del(ev);
    }

    mesa_rc observer_get(notifications::Event *ev, std::string &s) {
        return event_.observer_get(ev, s);
    }

    virtual ~TableReadOnlyNotificationNoNS_() {
        get_.unlink();
        event_.unlink();
    }

    FunctionExporterGetMapSubject<INTERFACE, Subject> get_;
    NotificationTable<INTERFACE, Subject> event_;
};

// Special case if we no values
template <class INTERFACE, class... Args>
struct TableReadOnlyNotificationNoNS_<INTERFACE, 0, ParamList<Args...>> {
    typedef TableStatus<Args...> Subject;
    typedef typename Subject::Observer Observer;

    TableReadOnlyNotificationNoNS_(NamespaceNode *p, Subject *table_status)
        : s_(table_status),
          get_(table_status, p, "get",
               InterfaceDescriptorDescGetFunc<INTERFACE>::value) {
        p->description(InterfaceDescriptorDescription<INTERFACE>::desc);
        get_.implemented(tag::depend_on_capability_get_ptr<INTERFACE>());
        get_.depends_on_capability(
                tag::depend_on_capability_name_t<INTERFACE>());
        get_.depends_on_capability_group(
                tag::depend_on_capability_json_ref_t<INTERFACE>());
        get_.has_notification(true);
    }

    virtual ~TableReadOnlyNotificationNoNS_() { get_.unlink(); }

    mesa_rc observer_new(notifications::Event *ev) {
        return s_->observer_new(ev);
    }

    mesa_rc observer_get(notifications::Event *ev, std::string &s) {
        Observer o;
        mesa_rc rc = s_->observer_get(ev, o);
        if (rc != MESA_RC_OK) return rc;
        if (!o.events.size()) return MESA_RC_OK;

        StringStream ss;
        std::string method_name;
        if (get_.parent()) method_name = get_.parent()->abs_name("update");

        NotificationBase msg(&ss, method_name.c_str());
        for (auto &&e : o.events) {
            auto event = (notifications::EventType::E)e.second;

            switch (e.second) {
            case notifications::EventType::Add:
            case notifications::EventType::Delete:
            case notifications::EventType::Modify: {
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

    Subject *s_;
    FunctionExporterGetSetSubject<INTERFACE, Subject> get_;
};

template <class INTERFACE>
using TableReadOnlyNotificationNoNS = TableReadOnlyNotificationNoNS_<
        INTERFACE, INTERFACE::P::val_cnt, typename INTERFACE::P>;

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_TABLE_READ_ONLY_NOTIFICATION_NO_NS_HXX__
