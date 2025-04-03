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
#ifndef __VTSS_PRIVILEGE_SERIALIZER_HXX__
#define __VTSS_PRIVILEGE_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/privilege.h"

/*****************************************************************************
    Enum serializer
*****************************************************************************/

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(privilege_module_name_index, vtss_appl_privilege_module_name_t, a, s) {
    a.add_leaf(
        vtss::AsDisplayString(s.inner.name, sizeof(s.inner.name)),
        vtss::tag::Name("ModuleName"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Module name.")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_privilege_config_web_t &s) {
    int ix = 0;
    typename T::Map_t m =
            a.as_map(vtss::tag::Typename("vtss_appl_privilege_config_web_t"));

    m.add_leaf(
        s.configRoPriv,
        vtss::tag::Name("ConfigRoPriv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Web privilege of read-only configuration.")
    );

    m.add_leaf(
        s.configRwPriv,
        vtss::tag::Name("ConfigRwPriv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Web privilege of read-write configuration.")
    );

    m.add_leaf(
        s.statusRoPriv,
        vtss::tag::Name("StatusRoPriv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Web privilege of read-only status.")
    );

    m.add_leaf(
        s.statusRwPriv,
        vtss::tag::Name("StatusRwPriv"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Web privilege of read-write status.")
    );
}

namespace vtss {
namespace appl {
namespace privilege {
namespace interfaces {

struct PrivilegeConfigWebEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_privilege_module_name_t>,
        vtss::expose::ParamVal<vtss_appl_privilege_config_web_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to configure web privilege";

    static constexpr const char *index_description =
        "Each module has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_privilege_module_name_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, privilege_module_name_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_privilege_config_web_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_privilege_config_web_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_privilege_config_web_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_privilege_config_web_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};
}  // namespace interfaces
}  // namespace privilege
}  // namespace appl
}  // namespace vtss


#endif /* __VTSS_PRIVILEGE_SERIALIZER_HXX__ */
