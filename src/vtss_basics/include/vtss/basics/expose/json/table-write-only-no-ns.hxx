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

#ifndef __VTSS_BASICS_EXPOSE_JSON_TABLE_WRITE_ONLY_NO_NS_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_TABLE_WRITE_ONLY_NO_NS_HXX__

#include <vtss/basics/expose/json/namespace-node.hxx>
#include <vtss/basics/expose/json/optional-default-impl.hxx>
#include <vtss/basics/expose/json/function-exporter-add.hxx>
#include <vtss/basics/expose/json/interface-descriptor-description.hxx>
#include <vtss/basics/expose/json/interface-descriptor-desc-set-func.hxx>

namespace vtss {
namespace expose {
namespace json {

template <typename INTERFACE, class... Args>
struct TableWriteOnlyNoNS_;

template <typename INTERFACE, class... Args>
struct TableWriteOnlyNoNS_<INTERFACE, ParamList<Args...>> {
    typedef TableWriteOnlyNoNS_<INTERFACE, ParamList<Args...>> THIS;

    TableWriteOnlyNoNS_(NamespaceNode *p)
        : set_(this, p, "set",
               InterfaceDescriptorDescSetFunc<INTERFACE>::value) {
        p->description(InterfaceDescriptorDescription<INTERFACE>::desc);
        set_.implemented(tag::depend_on_capability_get_ptr<INTERFACE>());
        set_.depends_on_capability(
                tag::depend_on_capability_name_t<INTERFACE>());
        set_.depends_on_capability_group(
                tag::depend_on_capability_json_ref_t<INTERFACE>());
    }

    // This is kind of strange, but write only must have a get-method, which
    // call the "def" function... To allow backwards compatibility we must allow
    // elements in a struct to be omitted in the parsing process, and when they
    // are we must have some default values to relay on. To accomplish this we
    // expose the set method using the FunctionExporterAdd which requires a
    // add method and an optional default values. Eventhough it is called "add"
    // here the JSON users will see this as a normal "set" method.
    mesa_rc add(typename Args::set_ptr_type... args) {
        return INTERFACE::set(
                std::forward<typename Args::set_ptr_type>(args)...);
    }

    mesa_rc def(typename Args::ptr_type... args) {
        return OptionalDefaultImpl<INTERFACE, Args...>::def(args...);
    }

    virtual ~TableWriteOnlyNoNS_() { set_.unlink(); }

    FunctionExporterAdd<INTERFACE, THIS> set_;
};

template <class INTERFACE>
using TableWriteOnlyNoNS =
        TableWriteOnlyNoNS_<INTERFACE, typename INTERFACE::P>;

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_TABLE_WRITE_ONLY_NO_NS_HXX__
