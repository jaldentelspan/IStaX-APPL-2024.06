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
#ifndef __VTSS_HTTPS_SERIALIZER_HXX__
#define __VTSS_HTTPS_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/https.h"



/*****************************************************************************
    Data serializer
*****************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_https_param_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_https_param_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.mode),
        vtss::tag::Name("Mode"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Global mode of HTTPS. true is to "
                               "enable the functions of HTTPS and false is to disable it.")
    );

    m.add_leaf(
        vtss::AsBool(s.redirect_to_https),
        vtss::tag::Name("RedirectToHttps"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The flag is to enable/disable the automatic "
                               "redirection from HTTP to HTTPS. true is to enable the "
                               "redirection and false is to disable the redirection.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_https_cert_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_https_cert_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsDisplayString(s.url, sizeof(s.url)),
        vtss::tag::Name("Url"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Https Certificate Url")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.pass_phrase, sizeof(s.pass_phrase)),
        vtss::tag::Name("Pass_phrase"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Private key pass phrase")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_https_cert_del_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_https_cert_del_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.del),
        vtss::tag::Name("Delete"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Delete https certificate")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_https_cert_gen_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_https_cert_gen_t"));
    int ix = 0;

    m.add_leaf(
        vtss::AsBool(s.gen),
        vtss::tag::Name("Generate"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Generate https certificate")
    );
}

namespace vtss
{
namespace appl
{
namespace hiawatha
{
namespace interfaces
{

struct HiawathaParamsLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_https_param_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_https_param_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_https_system_config_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_https_system_config_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct HttpsCertLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_https_cert_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_https_cert_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_SET_PTR(vtss_appl_https_cert_upload);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct HttpsCertDelete {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_https_cert_del_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_https_cert_del_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_https_cert_delete_dummy);
    VTSS_EXPOSE_SET_PTR(vtss_appl_https_cert_delete);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

struct HttpsCertGenerate {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_https_cert_gen_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_https_cert_gen_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_https_cert_generate_dummy);
    VTSS_EXPOSE_SET_PTR(vtss_appl_https_cert_generate);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};

}  // namespace interfaces
}  // namespace hiawatha
}  // namespace appl
}  // namespace vtss
#endif /* __VTSS_HTTPS_SERIALIZER_HXX__ */
