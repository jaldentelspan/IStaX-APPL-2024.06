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
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"

vtss_enum_descriptor_t vtss_auth_authen_method_txt[] {
    {VTSS_APPL_AUTH_AUTHEN_METHOD_NONE,   "none"},
    {VTSS_APPL_AUTH_AUTHEN_METHOD_LOCAL,  "local"},
    {VTSS_APPL_AUTH_AUTHEN_METHOD_RADIUS, "radius"},
    {VTSS_APPL_AUTH_AUTHEN_METHOD_TACACS, "tacacs"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_auth_author_method_txt[] {
    {VTSS_APPL_AUTH_AUTHOR_METHOD_NONE,   "none"},
    {VTSS_APPL_AUTH_AUTHOR_METHOD_TACACS, "tacacs"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_auth_acct_method_txt[] {
    {VTSS_APPL_AUTH_ACCT_METHOD_NONE,   "none"},
    {VTSS_APPL_AUTH_ACCT_METHOD_TACACS, "tacacs"},
    {0, 0},
};

static mesa_rc authen_get(vtss_appl_auth_agent_t agent, int ix,
                          vtss_appl_auth_authen_method_t *method) {
    mesa_rc rc = VTSS_RC_ERROR;
    if (ix <= VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX) {
        vtss_appl_auth_authen_agent_conf_t conf;
        if ((rc = vtss_appl_auth_authen_agent_conf_get(agent, &conf)) == VTSS_RC_OK) {
            *method = conf.method[ix];
        }
    }
    return rc;
}

static mesa_rc authen_set(vtss_appl_auth_agent_t agent, int ix,
                          const vtss_appl_auth_authen_method_t *method) {
    mesa_rc rc = VTSS_RC_ERROR;
    if (ix <= VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX) {
        vtss_appl_auth_authen_agent_conf_t conf;
        if ((rc = vtss_appl_auth_authen_agent_conf_get(agent, &conf)) == VTSS_RC_OK) {
            conf.method[ix] = *method;
            rc = vtss_appl_auth_authen_agent_conf_set(agent, &conf);
        }
    }
    return rc;
}

mesa_rc authen_console_get(uint32_t ix,
                           vtss_appl_auth_authen_method_t *method) {
    return authen_get(VTSS_APPL_AUTH_AGENT_CONSOLE, ix, method);
}

mesa_rc authen_console_set(uint32_t ix,
                           const vtss_appl_auth_authen_method_t *method) {
    return authen_set(VTSS_APPL_AUTH_AGENT_CONSOLE, ix, method);
}

mesa_rc authen_telnet_get(uint32_t ix, vtss_appl_auth_authen_method_t *method) {
    return authen_get(VTSS_APPL_AUTH_AGENT_TELNET, ix, method);
}

mesa_rc authen_telnet_set(uint32_t ix,
                          const vtss_appl_auth_authen_method_t *method) {
    return authen_set(VTSS_APPL_AUTH_AGENT_TELNET, ix, method);
}

mesa_rc authen_ssh_get(uint32_t ix, vtss_appl_auth_authen_method_t *method) {
    return authen_get(VTSS_APPL_AUTH_AGENT_SSH, ix, method);
}

mesa_rc authen_ssh_set(uint32_t ix,
                       const vtss_appl_auth_authen_method_t *method) {
    return authen_set(VTSS_APPL_AUTH_AGENT_SSH, ix, method);
}

mesa_rc authen_http_get(uint32_t ix, vtss_appl_auth_authen_method_t *method) {
    return authen_get(VTSS_APPL_AUTH_AGENT_HTTP, ix, method);
}

mesa_rc authen_http_set(uint32_t ix,
                        const vtss_appl_auth_authen_method_t *method) {
    return authen_set(VTSS_APPL_AUTH_AGENT_HTTP, ix, method);
}

mesa_rc authen_method_itr(const uint32_t *prev, uint32_t *next) {
    vtss::expose::snmp::IteratorComposeRange<uint32_t> itr(
            0, VTSS_APPL_AUTH_METHOD_PRI_IDX_MAX);
    return itr(prev, next);
}

mesa_rc acct_console_get(vtss_appl_auth_acct_agent_conf_t *conf) {
    return vtss_appl_auth_acct_agent_conf_get(VTSS_APPL_AUTH_AGENT_CONSOLE,
                                              conf);
}

mesa_rc acct_console_set(const vtss_appl_auth_acct_agent_conf_t *conf) {
    return vtss_appl_auth_acct_agent_conf_set(VTSS_APPL_AUTH_AGENT_CONSOLE,
                                              conf);
}

mesa_rc acct_telnet_get(vtss_appl_auth_acct_agent_conf_t *conf) {
    return vtss_appl_auth_acct_agent_conf_get(VTSS_APPL_AUTH_AGENT_TELNET,
                                              conf);
}

mesa_rc acct_telnet_set(const vtss_appl_auth_acct_agent_conf_t *conf) {
    return vtss_appl_auth_acct_agent_conf_set(VTSS_APPL_AUTH_AGENT_TELNET,
                                              conf);
}

mesa_rc acct_ssh_get(vtss_appl_auth_acct_agent_conf_t *conf) {
    return vtss_appl_auth_acct_agent_conf_get(VTSS_APPL_AUTH_AGENT_SSH, conf);
}

mesa_rc acct_ssh_set(const vtss_appl_auth_acct_agent_conf_t *conf) {
    return vtss_appl_auth_acct_agent_conf_set(VTSS_APPL_AUTH_AGENT_SSH, conf);
}

mesa_rc author_console_get(vtss_appl_auth_author_agent_conf_t *conf) {
    return vtss_appl_auth_author_agent_conf_get(VTSS_APPL_AUTH_AGENT_CONSOLE,
                                                conf);
}

mesa_rc author_console_set(const vtss_appl_auth_author_agent_conf_t *conf) {
    return vtss_appl_auth_author_agent_conf_set(VTSS_APPL_AUTH_AGENT_CONSOLE,
                                                conf);
}

mesa_rc author_telnet_get(vtss_appl_auth_author_agent_conf_t *conf) {
    return vtss_appl_auth_author_agent_conf_get(VTSS_APPL_AUTH_AGENT_TELNET,
                                                conf);
}

mesa_rc author_telnet_set(const vtss_appl_auth_author_agent_conf_t *conf) {
    return vtss_appl_auth_author_agent_conf_set(VTSS_APPL_AUTH_AGENT_TELNET,
                                                conf);
}

mesa_rc author_ssh_get(vtss_appl_auth_author_agent_conf_t *conf) {
    return vtss_appl_auth_author_agent_conf_get(VTSS_APPL_AUTH_AGENT_SSH, conf);
}

mesa_rc author_ssh_set(const vtss_appl_auth_author_agent_conf_t *conf) {
    return vtss_appl_auth_author_agent_conf_set(VTSS_APPL_AUTH_AGENT_SSH, conf);
}

mesa_rc auth_host_itr(const vtss_auth_host_index_t *prev_host, vtss_auth_host_index_t *next_host)
{
    uint32_t host;
    if (!prev_host) {
        *next_host = 0;
        return VTSS_RC_OK;
    }
    host = *prev_host;
    if (host >= (VTSS_APPL_AUTH_NUMBER_OF_SERVERS-1)) return VTSS_RC_ERROR;
    *next_host = host+1;
    return VTSS_RC_OK;
}

