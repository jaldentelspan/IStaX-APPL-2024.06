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

#ifndef __VTSS_BASICS_EXPOSE_JSON_STRUCT_READ_ONLY_NOTIFICATION_NO_NS_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_STRUCT_READ_ONLY_NOTIFICATION_NO_NS_HXX__

#include <vtss/basics/depend-on-capability.hxx>
#include <vtss/basics/expose/param-list.hxx>
#include <vtss/basics/expose/struct-status.hxx>
#include <vtss/basics/expose/json/namespace-node.hxx>
#include <vtss/basics/expose/json/notification-struct.hxx>
#include <vtss/basics/expose/json/function-exporter-get.hxx>
#include <vtss/basics/expose/json/interface-descriptor-description.hxx>
#include <vtss/basics/expose/json/interface-descriptor-desc-get-func.hxx>

namespace vtss {
namespace expose {
namespace json {

template <class INTERFACE, class... Args>
struct StructReadOnlyNotificationNoNS_;

template <class INTERFACE, class... Args>
struct StructReadOnlyNotificationNoNS_<INTERFACE, ParamList<Args...>> {
    typedef StructStatus<Args...> Subject;
    StructReadOnlyNotificationNoNS_(NamespaceNode *p, Subject *struct_status)
        : get_(struct_status, p, "get",
               InterfaceDescriptorDescGetFunc<INTERFACE>::value),
          event_(struct_status, p, "update", "") {
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

    virtual ~StructReadOnlyNotificationNoNS_() {
        get_.unlink();
        event_.unlink();
    }

    FunctionExporterGet<INTERFACE, Subject> get_;
    NotificationStruct<INTERFACE, Subject> event_;
};

template <class INTERFACE>
using StructReadOnlyNotificationNoNS =
        StructReadOnlyNotificationNoNS_<INTERFACE, typename INTERFACE::P>;


}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_STRUCT_READ_ONLY_NOTIFICATION_NO_NS_HXX__
