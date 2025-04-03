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

#include "vtss_auth_serializer.hxx"
#include "vtss/appl/auth.h"
#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#include "web_api.h"
#endif

using namespace vtss;
using namespace expose::snmp;

VTSS_MIB_MODULE("authMib", "AUTH", auth_mib_init, VTSS_MODULE_ID_AUTH, root,
                h) {
    h.add_history_element("201407010000Z", "Initial version");
    h.add_history_element("201603210000Z", "Support the encrypted secret key");
    h.add_history_element("201701160000Z", "Update the valid range of authentication method priority index");
    h.description("This is a private MIB for authentication");
}


#define NS(VAR, P, ID, NAME) static NamespaceNode VAR(&P, OidElement(ID, NAME))
NS(auth_obj, root, 1, "authMibObjects");
NS(auth_config, auth_obj, 2, "authConfig");
NS(auth_globals, auth_config, 1, "authConfigGlobals");
NS(auth_agents, auth_globals, 1, "authConfigGlobalsAgents");
NS(auth_authen_console, auth_agents, 1, "authConfigGlobalsAgentsConsoleAuthen");
NS(auth_authen_telnet, auth_agents, 2, "authConfigGlobalsAgentsTelnetAuthen");
NS(auth_authen_ssh, auth_agents, 3, "authConfigGlobalsAgentsSshAuthen");
NS(auth_authen_http, auth_agents, 4, "authConfigGlobalsAgentsHttpAuthen");
NS(auth_tacacs, auth_globals, 3, "authConfigGlobalsTacacs");
NS(auth_radius, auth_globals, 2, "authConfigGlobalsRadius");

namespace vtss {
namespace appl {
namespace auth {
namespace interfaces {
static TableReadWrite2<AuthAgentAuthenConsole> auth_agent_authen_console(
        &auth_authen_console,
        vtss::expose::snmp::OidElement(1, "authConfigGlobalsAgentsConsoleAuthenMethodsTable"));
static TableReadWrite2<AuthAgentAuthenTelnet> auth_agent_authen_telnet(
        &auth_authen_telnet,
        vtss::expose::snmp::OidElement(1, "authConfigGlobalsAgentsTelnetAuthenMethodsTable"));
static TableReadWrite2<AuthAgentAuthenSsh> auth_agent_authen_ssh(
        &auth_authen_ssh,
        vtss::expose::snmp::OidElement(1, "authConfigGlobalsAgentsSshAuthenMethodsTable"));
static TableReadWrite2<AuthAgentAuthenHttp> auth_agent_authen_http(
        &auth_authen_http,
        vtss::expose::snmp::OidElement(1, "authConfigGlobalsAgentsHttpAuthenMethodsTable"));
static StructRW2<AuthAgentAuthorConsole> auth_agent_author_console(
        &auth_agents,
        vtss::expose::snmp::OidElement(11, "authConfigGlobalsAgentsConsoleAuthor"));
static StructRW2<AuthAgentAuthorTelnet> auth_agent_author_telnet(
        &auth_agents,
        vtss::expose::snmp::OidElement(12, "authConfigGlobalsAgentsTelnetAuthor"));
static StructRW2<AuthAgentAuthorSsh> auth_agent_author_ssh(
        &auth_agents,
        vtss::expose::snmp::OidElement(13, "authConfigGlobalsAgentsSshAuthor"));
static StructRW2<AuthAgentAcctConsole> auth_agent_acct_console(
        &auth_agents,
        vtss::expose::snmp::OidElement(21, "authConfigGlobalsAgentsConsoleAcct"));
static StructRW2<AuthAgentAcctTelnet> auth_agent_acct_telnet(
        &auth_agents,
        vtss::expose::snmp::OidElement(22, "authConfigGlobalsAgentsTelnetAcct"));
static StructRW2<AuthAgentAcctSsh> auth_agent_acct_ssh(
        &auth_agents,
        vtss::expose::snmp::OidElement(23, "authConfigGlobalsAgentsSshAcct"));
static StructRW2<AuthRadiusGlobal> auth_radius_global(
        &auth_radius,
        vtss::expose::snmp::OidElement(1, "authConfigGlobalsRadiusGlobal"));
static TableReadWrite2<AuthAuthRadiusHosts> auth_auth_radius_hosts(
        &auth_radius,
        vtss::expose::snmp::OidElement(3, "authConfigGlobalsRadiusHostTable"));
static StructRW2<AuthTacacsGlobal> auth_tacacs_global(
        &auth_tacacs,
        vtss::expose::snmp::OidElement(1, "authConfigGlobalsTacacsGlobal"));
static TableReadWrite2<AuthAuthTacacsHosts> auth_auth_tacacs_hosts(
        &auth_tacacs,
        vtss::expose::snmp::OidElement(2, "authConfigGlobalsTacacsHostTable"));
}  // namespace interfaces
}  // namespace aggr
}  // namespace appl
}  // namespace vtss

