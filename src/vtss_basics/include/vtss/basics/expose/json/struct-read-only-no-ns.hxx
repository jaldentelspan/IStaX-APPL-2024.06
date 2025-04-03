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

#ifndef __VTSS_BASICS_EXPOSE_JSON_STRUCT_READ_ONLY_NO_NS_HXX__
#define __VTSS_BASICS_EXPOSE_JSON_STRUCT_READ_ONLY_NO_NS_HXX__

#include <vtss/basics/depend-on-capability.hxx>
#include <vtss/basics/expose/json/namespace-node.hxx>
#include <vtss/basics/expose/json/function-exporter-get.hxx>
#include <vtss/basics/expose/json/interface-descriptor-description.hxx>
#include <vtss/basics/expose/json/interface-descriptor-desc-get-func.hxx>

namespace vtss {
namespace expose {
namespace json {

template <class INTERFACE, class... Args>
struct StructReadOnlyNoNS_;

template <class INTERFACE, class... Args>
struct StructReadOnlyNoNS_<INTERFACE, ParamList<Args...>> {
    typedef StructReadOnlyNoNS_<INTERFACE, ParamList<Args...>> THIS;

    StructReadOnlyNoNS_(NamespaceNode *p)
        : get_(this, p, "get",
               InterfaceDescriptorDescGetFunc<INTERFACE>::value) {
        p->description(InterfaceDescriptorDescription<INTERFACE>::desc);
        get_.implemented(tag::depend_on_capability_get_ptr<INTERFACE>());
        get_.depends_on_capability(
                tag::depend_on_capability_name_t<INTERFACE>());
        get_.depends_on_capability_group(
                tag::depend_on_capability_json_ref_t<INTERFACE>());
    }

    mesa_rc get(typename Args::get_ptr_type... args) {
        return INTERFACE::get(
                std::forward<typename Args::get_ptr_type>(args)...);
    }

    virtual ~StructReadOnlyNoNS_() { get_.unlink(); }

    FunctionExporterGet<INTERFACE, THIS> get_;
};

template <class INTERFACE>
using StructReadOnlyNoNS =
        StructReadOnlyNoNS_<INTERFACE, typename INTERFACE::P>;

}  // namespace json
}  // namespace expose
}  // namespace vtss

#endif  // __VTSS_BASICS_EXPOSE_JSON_STRUCT_READ_ONLY_NO_NS_HXX__
