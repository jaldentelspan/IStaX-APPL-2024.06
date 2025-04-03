/*

 Copyright (c) 2006-2020 Microsemi Corporation "Microsemi". All Rights Reserved.

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

/*
******************************************************************************

    Include files

******************************************************************************
*/
#include "icfg_api.h"
#include "vtss_auth_api.h"
#include "vtss_auth_icfg.h"
#include "misc_api.h"
#include "vtss_os_wrapper_network.h" /* INET6_ADDRSTRLEN */

/*
******************************************************************************

    Constant and Macro definition

******************************************************************************
*/
#undef IC_RC
#define IC_RC ICLI_VTSS_RC

/*
******************************************************************************

    Data structure type definition

******************************************************************************
*/

/*
******************************************************************************

    Static Function

******************************************************************************
*/

#if defined(VTSS_APPL_AUTH_ENABLE_CONSOLE) || defined(VTSS_SW_OPTION_CLI_TELNET) || defined(VTSS_SW_OPTION_SSH) || defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_FAST_CGI)
static mesa_rc VTSS_AUTH_ICFG_authen_agent_print(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result, vtss_appl_auth_agent_t agent)
{
    vtss_appl_auth_authen_agent_conf_t c;
    vtss_appl_auth_authen_method_t     m;
    vtss_appl_auth_authen_agent_conf_t dc;
    int                                i;

    IC_RC(vtss_appl_auth_authen_agent_conf_get(agent, &c));
    if (req->all_defaults || memcmp(&c, vtss_appl_auth_authen_agent_conf_default(&dc), sizeof(c))) {
        if (c.method[0] == VTSS_APPL_AUTH_AUTHEN_METHOD_NONE) {
            IC_RC(vtss_icfg_printf(result, "no aaa authentication login %s\n", vtss_appl_auth_agent_name(agent)));
        } else {
            IC_RC(vtss_icfg_printf(result, "aaa authentication login %s", vtss_appl_auth_agent_name(agent)));
            for (i = 0; i <= VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX; i++) {
                m = c.method[i];
                if (m == VTSS_APPL_AUTH_AUTHEN_METHOD_NONE) {
                    break;
                }
                IC_RC(vtss_icfg_printf(result, " %s", vtss_appl_auth_authen_method_name(m)));
            }
            IC_RC(vtss_icfg_printf(result, "\n"));
        }
    }
    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_TACPLUS
static mesa_rc VTSS_AUTH_ICFG_author_agent_print(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result, vtss_appl_auth_agent_t agent)
{
    vtss_appl_auth_author_agent_conf_t c;

    IC_RC(vtss_appl_auth_author_agent_conf_get(agent, &c));
    if (req->all_defaults || c.cmd_enable) {
        if ((c.method == VTSS_APPL_AUTH_AUTHOR_METHOD_NONE) || (!c.cmd_enable)) {
            IC_RC(vtss_icfg_printf(result, "no aaa authorization %s\n", vtss_appl_auth_agent_name(agent)));
        } else {
            IC_RC(vtss_icfg_printf(result, "aaa authorization %s tacacs commands %u%s",
                                   vtss_appl_auth_agent_name(agent),
                                   c.cmd_priv_lvl,
                                   c.cfg_cmd_enable ? " config-commands\n" : "\n"));
        }
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_SW_OPTION_TACPLUS */

#ifdef VTSS_SW_OPTION_TACPLUS
static mesa_rc VTSS_AUTH_ICFG_acct_agent_print(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result, vtss_appl_auth_agent_t agent)
{
    vtss_appl_auth_acct_agent_conf_t c;

    IC_RC(vtss_appl_auth_acct_agent_conf_get(agent, &c));
    if (req->all_defaults || c.cmd_enable || c.exec_enable) {
        if ((c.method == VTSS_APPL_AUTH_ACCT_METHOD_NONE) || (!c.cmd_enable && !c.exec_enable)) {
            IC_RC(vtss_icfg_printf(result, "no aaa accounting %s\n", vtss_appl_auth_agent_name(agent)));
        } else {
            IC_RC(vtss_icfg_printf(result, "aaa accounting %s tacacs", vtss_appl_auth_agent_name(agent)));
            if (c.cmd_enable) {
                IC_RC(vtss_icfg_printf(result, " commands %u", c.cmd_priv_lvl));
            }
            if (c.exec_enable) {
                IC_RC(vtss_icfg_printf(result, " exec"));
            }
            IC_RC(vtss_icfg_printf(result, "\n"));
        }
    }
    return VTSS_RC_OK;
}
#endif /* VTSS_SW_OPTION_TACPLUS */

static mesa_rc VTSS_AUTH_ICFG_agent_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
#ifdef VTSS_APPL_AUTH_ENABLE_CONSOLE
    IC_RC(VTSS_AUTH_ICFG_authen_agent_print(req, result, VTSS_APPL_AUTH_AGENT_CONSOLE));
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
    IC_RC(VTSS_AUTH_ICFG_authen_agent_print(req, result, VTSS_APPL_AUTH_AGENT_TELNET));
#endif
#ifdef VTSS_SW_OPTION_SSH
    IC_RC(VTSS_AUTH_ICFG_authen_agent_print(req, result, VTSS_APPL_AUTH_AGENT_SSH));
#endif
#if defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_FAST_CGI)
    IC_RC(VTSS_AUTH_ICFG_authen_agent_print(req, result, VTSS_APPL_AUTH_AGENT_HTTP));
#endif

#ifdef VTSS_SW_OPTION_TACPLUS
#ifdef VTSS_APPL_AUTH_ENABLE_CONSOLE
    IC_RC(VTSS_AUTH_ICFG_author_agent_print(req, result, VTSS_APPL_AUTH_AGENT_CONSOLE));
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
    IC_RC(VTSS_AUTH_ICFG_author_agent_print(req, result, VTSS_APPL_AUTH_AGENT_TELNET));
#endif
#ifdef VTSS_SW_OPTION_SSH
    IC_RC(VTSS_AUTH_ICFG_author_agent_print(req, result, VTSS_APPL_AUTH_AGENT_SSH));
#endif
#endif /* VTSS_SW_OPTION_TACPLUS */

#ifdef VTSS_SW_OPTION_TACPLUS
#ifdef VTSS_APPL_AUTH_ENABLE_CONSOLE
    IC_RC(VTSS_AUTH_ICFG_acct_agent_print(req, result, VTSS_APPL_AUTH_AGENT_CONSOLE));
#endif
#ifdef VTSS_SW_OPTION_CLI_TELNET
    IC_RC(VTSS_AUTH_ICFG_acct_agent_print(req, result, VTSS_APPL_AUTH_AGENT_TELNET));
#endif
#ifdef VTSS_SW_OPTION_SSH
    IC_RC(VTSS_AUTH_ICFG_acct_agent_print(req, result, VTSS_APPL_AUTH_AGENT_SSH));
#endif
#endif /* VTSS_SW_OPTION_TACPLUS */
    return VTSS_RC_OK;
}
#endif /* defined(VTSS_APPL_AUTH_ENABLE_CONSOLE) || defined(VTSS_SW_OPTION_CLI_TELNET) || defined(VTSS_SW_OPTION_SSH) || defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_FAST_CGI) */

#if defined(VTSS_SW_OPTION_RADIUS)
static mesa_rc VTSS_AUTH_ICFG_radius_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    vtss_appl_auth_radius_global_conf_t conf;
    vtss_appl_auth_radius_server_conf_t hc;
    char                                buf[INET6_ADDRSTRLEN];
    vtss_auth_host_index_t              ix;

    IC_RC(vtss_appl_auth_radius_global_conf_get(&conf));

    if (req->all_defaults || (conf.timeout != VTSS_APPL_AUTH_TIMEOUT_DEFAULT)) {
        if (conf.timeout == VTSS_APPL_AUTH_TIMEOUT_DEFAULT) {
            IC_RC(vtss_icfg_printf(result, "no radius-server timeout\n"));
        } else {
            IC_RC(vtss_icfg_printf(result, "radius-server timeout %u\n", conf.timeout));
        }
    }

    if (req->all_defaults || (conf.retransmit != VTSS_APPL_AUTH_RETRANSMIT_DEFAULT)) {
        if (conf.retransmit == VTSS_APPL_AUTH_RETRANSMIT_DEFAULT) {
            IC_RC(vtss_icfg_printf(result, "no radius-server retransmit\n"));
        } else {
            IC_RC(vtss_icfg_printf(result, "radius-server retransmit %u\n", conf.retransmit));
        }
    }

    if (req->all_defaults || (conf.deadtime != VTSS_APPL_AUTH_DEADTIME_DEFAULT)) {
        if (conf.deadtime == VTSS_APPL_AUTH_DEADTIME_DEFAULT) {
            IC_RC(vtss_icfg_printf(result, "no radius-server deadtime\n"));
        } else {
            IC_RC(vtss_icfg_printf(result, "radius-server deadtime %u\n", conf.deadtime));
        }
    }

    if (conf.key[0]) {
        // Any non-empty key we have MUST be encrypted
        IC_RC(vtss_icfg_printf(result, "radius-server key encrypted %s\n", conf.key));
    } else if (req->all_defaults) {
        IC_RC(vtss_icfg_printf(result, "no radius-server key\n"));
    }

    if (req->all_defaults || (conf.nas_ip_address_enable)) {
        if (conf.nas_ip_address_enable) {
            IC_RC(vtss_icfg_printf(result, "radius-server attribute 4 %s\n", misc_ipv4_txt(conf.nas_ip_address, buf)));
        } else {
            IC_RC(vtss_icfg_printf(result, "no radius-server attribute 4\n"));
        }
    }

#ifdef VTSS_SW_OPTION_IPV6
    if (req->all_defaults || (conf.nas_ipv6_address_enable)) {
        if (conf.nas_ipv6_address_enable) {
            IC_RC(vtss_icfg_printf(result, "radius-server attribute 95 %s\n", misc_ipv6_txt(&conf.nas_ipv6_address, buf)));
        } else {
            IC_RC(vtss_icfg_printf(result, "no radius-server attribute 95\n"));
        }
    }
#endif /* VTSS_SW_OPTION_IPV6 */

    if (req->all_defaults || (conf.nas_identifier[0])) {
        if (conf.nas_identifier[0]) {
            IC_RC(vtss_icfg_printf(result, "radius-server attribute 32 %s\n", conf.nas_identifier));
        } else {
            IC_RC(vtss_icfg_printf(result, "no radius-server attribute 32\n"));
        }
    }

    for (ix = 0; ix < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; ix++ ) {
        if ((vtss_appl_auth_radius_server_get(ix, &hc) == VTSS_RC_OK) && (strlen(hc.host) > 0)) {
            IC_RC(vtss_icfg_printf(result, "radius-server host %s", hc.host));
            if (req->all_defaults || (hc.auth_port != VTSS_APPL_AUTH_RADIUS_AUTH_PORT_DEFAULT)) {
                IC_RC(vtss_icfg_printf(result, " auth-port %u", hc.auth_port));
            }
            if (req->all_defaults || (hc.acct_port != VTSS_APPL_AUTH_RADIUS_ACCT_PORT_DEFAULT)) {
                IC_RC(vtss_icfg_printf(result, " acct-port %u", hc.acct_port));
            }
            if (hc.timeout) {
                IC_RC(vtss_icfg_printf(result, " timeout %u", hc.timeout));
            }
            if (hc.retransmit) {
                IC_RC(vtss_icfg_printf(result, " retransmit %u", hc.retransmit));
            }
            if (hc.key[0]) {
                IC_RC(vtss_icfg_printf(result, " key encrypted %s", hc.key));
            }
            // We're not adding 'key unencrypted ""' for all_defaults, but just omitting it.
            // We would add it if a kind of "no key" existed for a server, but, well, it doesn't.
            IC_RC(vtss_icfg_printf(result, "\n"));
        }
    }

    return VTSS_RC_OK;
}
#endif /* defined(VTSS_SW_OPTION_RADIUS) */

#if defined(VTSS_SW_OPTION_TACPLUS)
static mesa_rc VTSS_AUTH_ICFG_tacacs_conf(const vtss_icfg_query_request_t *req, vtss_icfg_query_result_t *result)
{
    vtss_appl_auth_tacacs_global_conf_t conf;
    vtss_appl_auth_tacacs_server_conf_t hc;
    vtss_auth_host_index_t              ix;

    IC_RC(vtss_appl_auth_tacacs_global_conf_get(&conf));

    if (req->all_defaults || (conf.timeout != VTSS_APPL_AUTH_TIMEOUT_DEFAULT)) {
        if (conf.timeout == VTSS_APPL_AUTH_TIMEOUT_DEFAULT) {
            IC_RC(vtss_icfg_printf(result, "no tacacs-server timeout\n"));
        } else {
            IC_RC(vtss_icfg_printf(result, "tacacs-server timeout %u\n", conf.timeout));
        }
    }

    if (req->all_defaults || (conf.deadtime != VTSS_APPL_AUTH_DEADTIME_DEFAULT)) {
        if (conf.deadtime == VTSS_APPL_AUTH_DEADTIME_DEFAULT) {
            IC_RC(vtss_icfg_printf(result, "no tacacs-server deadtime\n"));
        } else {
            IC_RC(vtss_icfg_printf(result, "tacacs-server deadtime %u\n", conf.deadtime));
        }
    }

    if (conf.key[0]) {
        IC_RC(vtss_icfg_printf(result, "tacacs-server key encrypted %s\n", conf.key));
    } else if (req->all_defaults) {
        IC_RC(vtss_icfg_printf(result, "no tacacs-server key\n"));
    }

    for (ix = 0; ix < VTSS_APPL_AUTH_NUMBER_OF_SERVERS; ix++ ) {
        if ((vtss_appl_auth_tacacs_server_get(ix, &hc) == VTSS_RC_OK) && (strlen(hc.host) > 0)) {
            IC_RC(vtss_icfg_printf(result, "tacacs-server host %s", hc.host));
            if (req->all_defaults || (hc.port != VTSS_APPL_AUTH_TACACS_PORT_DEFAULT)) {
                IC_RC(vtss_icfg_printf(result, " port %u", hc.port));
            }
            if (hc.timeout) {
                IC_RC(vtss_icfg_printf(result, " timeout %u", hc.timeout));
            }
            if (hc.key[0]) {
                IC_RC(vtss_icfg_printf(result, " key encrypted %s", hc.key));
            }
            // We're not adding 'key unencrypted ""' for all_defaults, but just omitting it.
            // We would add it if a kind of "no key" existed for a server, but, well, it doesn't.
            IC_RC(vtss_icfg_printf(result, "\n"));
        }
    }

    return VTSS_RC_OK;
}
#endif /* defined(VTSS_SW_OPTION_TACPLUS) */

/*
******************************************************************************

    Public functions

******************************************************************************
*/

/* Initialization function */
mesa_rc vtss_auth_icfg_init(void)
{
#if defined(VTSS_APPL_AUTH_ENABLE_CONSOLE) || defined(VTSS_SW_OPTION_CLI_TELNET) || defined(VTSS_SW_OPTION_SSH) || defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_FAST_CGI)
    IC_RC(vtss_icfg_query_register(VTSS_ICFG_AUTH_AGENT_CONF, "auth", VTSS_AUTH_ICFG_agent_conf));
#endif /* defined(VTSS_APPL_AUTH_ENABLE_CONSOLE) || defined(VTSS_SW_OPTION_CLI_TELNET) || defined(VTSS_SW_OPTION_SSH) || defined(VTSS_SW_OPTION_WEB) || defined(VTSS_SW_OPTION_FAST_CGI) */

#if defined(VTSS_SW_OPTION_RADIUS)
    IC_RC(vtss_icfg_query_register(VTSS_ICFG_AUTH_RADIUS_CONF, "auth", VTSS_AUTH_ICFG_radius_conf));
#endif /* defined(VTSS_SW_OPTION_RADIUS) */

#if defined(VTSS_SW_OPTION_TACPLUS)
    IC_RC(vtss_icfg_query_register(VTSS_ICFG_AUTH_TACACS_CONF, "auth", VTSS_AUTH_ICFG_tacacs_conf));
#endif /* defined(VTSS_SW_OPTION_TACPLUS) */
    return VTSS_RC_OK;
}
