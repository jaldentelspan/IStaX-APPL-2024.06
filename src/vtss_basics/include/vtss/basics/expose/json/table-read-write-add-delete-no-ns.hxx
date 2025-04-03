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

#ifndef __VTSS_BASICS_EXPOSE_JSON_TABLE_READ_WRITE_ADD_DELETE_NO_NS_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_TABLE_READ_WRITE_ADD_DELETE_NO_NS_HXX__

#include <vtss/basics/expose/json/optional-default-impl.hxx>
#include <vtss/basics/expose/json/function-exporter-del.hxx>
#include <vtss/basics/expose/json/function-exporter-add.hxx>
#include <vtss/basics/expose/json/table-read-write-no-ns.hxx>
#include <vtss/basics/expose/json/interface-descriptor-desc-add-func.hxx>
#include <vtss/basics/expose/json/interface-descriptor-desc-del-func.hxx>

namespace vtss {
namespace expose {
namespace json {


template <typename INTERFACE, class... Args>
struct TableReadWriteAddDeleteNoNS_;

template <typename INTERFACE, class... Args>
struct TableReadWriteAddDeleteNoNS_<INTERFACE, ParamList<Args...>>
        : public TableReadWriteNoNS_<INTERFACE, ParamList<Args...>> {
    typedef TableReadWriteAddDeleteNoNS_<INTERFACE, ParamList<Args...>> THIS;

    TableReadWriteAddDeleteNoNS_(NamespaceNode *p)
        : TableReadWriteNoNS<INTERFACE>(p),
          add_(this, p, "add",
               InterfaceDescriptorDescAddFunc<INTERFACE>::value),
          del_(this, p, "del",
               InterfaceDescriptorDescDelFunc<INTERFACE>::value) {
        add_.implemented(tag::depend_on_capability_get_ptr<INTERFACE>());
        add_.depends_on_capability(
                tag::depend_on_capability_name_t<INTERFACE>());
        add_.depends_on_capability_group(
                tag::depend_on_capability_json_ref_t<INTERFACE>());
        del_.depends_on_capability(
                tag::depend_on_capability_name_t<INTERFACE>());
        del_.depends_on_capability_group(
                tag::depend_on_capability_json_ref_t<INTERFACE>());
    }

    template <typename... X>
    mesa_rc del(X... x) {
        return INTERFACE::del(x...);
    }

    mesa_rc add(typename Args::set_ptr_type... args) {
        return INTERFACE::add(
                std::forward<typename Args::set_ptr_type>(args)...);
    }

    mesa_rc def(typename Args::ptr_type... args) {
        // eventually we should just call INTERFACE::def directly
        return OptionalDefaultImpl<INTERFACE, Args...>::def(args...);
    }

    ~TableReadWriteAddDeleteNoNS_() {
        add_.unlink();
        del_.unlink();
    }

    FunctionExporterAdd<INTERFACE, THIS> add_;
    FunctionExporterDel<INTERFACE, THIS> del_;
};

template <class INTERFACE>
using TableReadWriteAddDeleteNoNS =
        TableReadWriteAddDeleteNoNS_<INTERFACE, typename INTERFACE::P>;

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_TABLE_READ_WRITE_ADD_DELETE_NO_NS_HXX__
