/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef __VTSS_SSH_SERIALIZER_HXX__
#define __VTSS_SSH_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/ssh.h"

template<typename T>
void serialize(T &a, vtss_appl_ssh_conf_t &s)
{
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_ssh_conf_t"));

    int ix = 1;  // After interface index
    m.add_leaf(vtss::AsBool(s.mode),
               vtss::tag::Name("AdminState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Enable/Disable the SSH functionality."));
}

namespace vtss
{
namespace appl
{
namespace ssh
{
namespace interfaces
{
struct SshAdminLeaf {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_ssh_conf_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_ssh_conf_t &i)
    {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_ssh_conf_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_ssh_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY);
};
}  // namespace interfaces
}  // namespace ssh
}  // namespace appl
}  // namespace vtss

#endif  /* __VTSS_SSH_SERIALIZER_HXX__ */
