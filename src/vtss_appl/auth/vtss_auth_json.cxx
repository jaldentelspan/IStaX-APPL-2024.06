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
#include "vtss/basics/expose/json.hxx"

using namespace vtss;
using namespace vtss::json;
using namespace vtss::expose::json;
using namespace vtss::appl::auth::interfaces;

namespace vtss {
void json_node_add(Node *node);
}  // namespace vtss

#define NS(N, P, D) static vtss::expose::json::NamespaceNode N(&P, D);
static NamespaceNode ns_auth("authentication");
extern "C" void vtss_appl_auth_json_init() { json_node_add(&ns_auth); }

NS(ns_conf,                        ns_auth,                "config");
NS(ns_conf_agent,                  ns_conf,                "agent");
NS(ns_conf_radius,                 ns_conf,                "radius");
NS(ns_conf_tacacs,                 ns_conf,                "tacacs");

NS(ns_conf_agent_console,          ns_conf_agent,          "console");
NS(ns_conf_agent_telnet,           ns_conf_agent,          "telnet");
NS(ns_conf_agent_ssh,              ns_conf_agent,          "ssh");
NS(ns_conf_agent_http,             ns_conf_agent,          "http");

namespace vtss {
namespace appl {
namespace auth {
namespace interfaces {
// auth.conf.agent.console.authen ////////////////////////////////////////////
static TableReadWrite<AuthAgentAuthenConsole> auth_agent_authen_console(
        &ns_conf_agent_console, "authen");

// auth.conf.agent.console.author
static StructReadWrite<AuthAgentAuthorConsole> auth_agent_author_console(
        &ns_conf_agent_console, "author");

// auth.conf.agent.console.acct
static StructReadWrite<AuthAgentAcctConsole> auth_agent_acct_console(
        &ns_conf_agent_console, "acct");

// auth.conf.agent.telnet.authen /////////////////////////////////////////////
static TableReadWrite<AuthAgentAuthenTelnet> auth_agent_authen_telnet(
        &ns_conf_agent_telnet, "authen");

// auth.conf.agent.telnet.author
static StructReadWrite<AuthAgentAuthorTelnet> auth_agent_author_telnet(
        &ns_conf_agent_telnet, "author");

// auth.conf.agent.telnet.acct
static StructReadWrite<AuthAgentAcctTelnet> auth_agent_acct_telnet(
        &ns_conf_agent_telnet, "acct");

// auth.conf.agent.ssh.authen ////////////////////////////////////////////////
static TableReadWrite<AuthAgentAuthenSsh> auth_agent_authen_ssh(
        &ns_conf_agent_ssh, "authen");

// auth.conf.agent.ssh.author
static StructReadWrite<AuthAgentAuthorSsh> auth_agent_author_ssh(
        &ns_conf_agent_ssh, "author");

// auth.conf.agent.ssh.acct
static StructReadWrite<AuthAgentAcctSsh> auth_agent_acct_ssh(
        &ns_conf_agent_ssh, "acct");

// auth.conf.agent.http.authen ///////////////////////////////////////////////
static TableReadWrite<AuthAgentAuthenHttp> auth_agent_authen_http(
        &ns_conf_agent_http, "authen");

// auth.conf.radius.global ///////////////////////////////////////////////////
static StructReadWrite<AuthRadiusGlobal> auth_radius_global(
        &ns_conf_radius, "global");

// auth.conf.radius.host
static TableReadWrite<AuthAuthRadiusHosts> auth_auth_radius_hosts(
        &ns_conf_radius, "host");

#ifdef VTSS_SW_OPTION_TACPLUS
// auth.conf.tacacs.global ///////////////////////////////////////////////////
static StructReadWrite<AuthTacacsGlobal> auth_tacacs_global(
        &ns_conf_tacacs, "global");

// auth.conf.tacacs.host
static TableReadWrite<AuthAuthTacacsHosts> auth_auth_tacacs_hosts(
        &ns_conf_tacacs, "host");
#endif

}  // namespace interfaces
}  // namespace aggr
}  // namespace appl
}  // namespace vtss

