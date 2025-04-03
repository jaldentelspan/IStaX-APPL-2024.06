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
#ifndef __VTSS_USERS_SERIALIZER_HXX__
#define __VTSS_USERS_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/users.h"

/*****************************************************************************
    Index serializer
*****************************************************************************/
VTSS_SNMP_TAG_SERIALIZE(users_username_index, vtss_appl_users_username_t, a, s) {
    a.add_leaf(
        vtss::AsDisplayString(s.inner.username, sizeof(s.inner.username)),
        vtss::tag::Name("Username"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Name of user.")
    );
}

/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_users_config_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_users_config_t"));
    int ix = 0;

    m.add_leaf(
        s.privilege,
        vtss::tag::Name("Privilege"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Privilege level of the user.")
    );

    m.add_leaf(
        vtss::AsBool(s.encrypted),
        vtss::tag::Name("Encrypted"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The flag indicates the password is encrypted or not."
            " TRUE means the password is encrypted."
            " FALSE means the password is plain text.")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.password, sizeof(s.password)),
        vtss::tag::Name("Password"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Password of the user."
            " The password length depends on the type of password."
            " If the password is encrypted, then the length is 128."
            " If it is unencrypted, then the maximum length is " TO_STR(VTSS_SYS_INPUT_PASSWD_LEN) ".")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_users_info_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_users_info_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.username, sizeof(s.username)),
        vtss::tag::Name("Username"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Myself user name.")
    );

    m.add_leaf(
        s.privilege,
        vtss::tag::Name("Privilege"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Myself privilege level.")
    );
}

namespace vtss {
namespace appl {
namespace users {
namespace interfaces {

struct UsersConfigEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_appl_users_username_t>,
        vtss::expose::ParamVal<vtss_appl_users_config_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "The table is Users onfiguration table. The index is user name.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_users_username_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, users_username_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_users_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_users_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_users_config_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_users_config_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_users_config_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_users_config_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct UsersStatusEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_users_info_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_users_info_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_users_whoami);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY);
};

}  // namespace interfaces
}  // namespace users
}  // namespace appl
}  // namespace vtss
#endif /* __VTSS_USERS_SERIALIZER_HXX__ */
