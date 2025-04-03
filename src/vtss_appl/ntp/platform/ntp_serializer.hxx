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
#ifndef __VTSS_NTP_SERIALIZER_HXX__
#define __VTSS_NTP_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/ntp.h"

VTSS_SNMP_TAG_SERIALIZE(ntp_server_tbl_idx, u32, a, s) {
    a.add_leaf(
        vtss::AsInt(s.inner),
        vtss::tag::Name("Index"),
        vtss::expose::snmp::RangeSpec<u32>(1, 5),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("The index of NTP servers.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_ntp_global_config_t &s) {
    int ix = 0;

    a.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Global config mode of NTP. "
                "true is to enable NTP function in the system and "
                "false is to disable it.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_ntp_server_config_t &s) {
    int ix = 0;

    a.add_leaf(
        s.address,
        vtss::tag::Name("Address"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Internet address of a NTP server.")
    );
}

namespace vtss {
namespace appl {
namespace ntp {
namespace interfaces {
struct NtpConfigGlobalsLeaf {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_ntp_global_config_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ntp_global_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ntp_global_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ntp_global_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_NTP);
};

struct NtpConfigServerEntry {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_ntp_server_config_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of NTP server.";

    static constexpr const char *index_description =
        "Each server has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, ntp_server_tbl_idx(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_ntp_server_config_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ntp_server_config_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_ntp_server_config_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ntp_server_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_NTP);
};
}  // namespace interfaces
}  // namespace ntp
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_NTP_SERIALIZER_HXX__ */
