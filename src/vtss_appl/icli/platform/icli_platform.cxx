/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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
==============================================================================

    Revision history
    > CP.Wang, 2011/09/16 10:16
        - create

==============================================================================
*/

/*
==============================================================================

    Include File

==============================================================================
*/
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "vtss_icli.h"

#ifdef VTSS_SW_OPTION_AUTH
#include "icli_porting_trace.h" // for T_E
#endif

#ifdef ICLI_TARGET

#include "icli_porting_util.h"

#ifdef VTSS_SW_OPTION_AUTH
#include "vtss_auth_api.h"
#endif

#ifdef VTSS_SW_OPTION_MD5
#include "vtss_md5_api.h"
#endif

#include "led_api.h"
#include "msg_api.h"
#include "misc_api.h"
#include "topo_api.h"
#include "port_api.h"

#endif // ICLI_TARGET

#define ICLI_ENUM_INC(T)                                       \
static inline T& operator++(T& x) {                            \
    x = (T) (x + 1);                                           \
    return x;                                                  \
}

ICLI_ENUM_INC(icli_cmd_mode_t);

/*
==============================================================================

    Constant and Macro

==============================================================================
*/
#define _MAX_STR_BUF_SIZE       128

#define _SEQUENCE_INDICATOR     91  // [
#define _CURSOR_UP              65  // A
#define _CURSOR_DOWN            66  // B
#define _CURSOR_FORWARD         67  // C
#define _CURSOR_BACKWARD        68  // D

#define _CSI \
    (void)vtss_icli_sutil_usr_char_put(handle, ICLI_KEY_ESC); \
    (void)vtss_icli_sutil_usr_char_put(handle, _SEQUENCE_INDICATOR);

/*
==============================================================================

    Type Definition

==============================================================================
*/

/*
==============================================================================

    Static Variable

==============================================================================
*/
/* modify as icli_cmd_mode_t */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
/* The strings inside 'icli_cmd_mode_info_t' are simple char *
 * No guarantee on memory overwrites, so tread carefully and carry a small string */
static const icli_cmd_mode_info_t g_cmd_mode_info[ICLI_CMD_MODE_MAX] = {
    {
        ICLI_CMD_MODE_EXEC,     // mode
        "ICLI_CMD_MODE_EXEC",   // mode str
        "User EXEC Mode",       // description
        "",                     // prompt
        "exec",                 // name
        "",                     // ICLI command
        FALSE                   // b_mode_var
    },
    {
        ICLI_CMD_MODE_GLOBAL_CONFIG,    // mode
        "ICLI_CMD_MODE_GLOBAL_CONFIG",  // mode str
        "Global Configuration Mode",    // description
        "config",                       // prompt
        "configure",                    // name
        "configure terminal",           // ICLI command
        FALSE                           // b_mode_var
    },
    {
        ICLI_CMD_MODE_CONFIG_VLAN,      // mode
        "ICLI_CMD_MODE_CONFIG_VLAN",    // mode str
        "VLAN Configuration Mode",      // description
        "config-vlan",                  // prompt
        "config-vlan",                  // name
        "vlan <vlan_list>",             // ICLI command
        TRUE                            // b_mode_var
    },
    {
        ICLI_CMD_MODE_INTERFACE_PORT_LIST,          // mode
        "ICLI_CMD_MODE_INTERFACE_PORT_LIST",        // mode str
        "Port List Interface Mode",                 // description
        "config-if",                                // prompt
        "interface",                                // name
        "interface <port_type> <port_type_list>",   // ICLI command
        TRUE                                        // b_mode_var
    },
    {
        ICLI_CMD_MODE_INTERFACE_VLAN,   // mode
        "ICLI_CMD_MODE_INTERFACE_VLAN", // mode str
        "VLAN Interface Mode",          // description
        "config-if-vlan",               // prompt
        "if-vlan",                      // name
        "interface vlan <vlan_list>",   // ICLI command
        TRUE                            // b_mode_var
    },
    {
        ICLI_CMD_MODE_CONFIG_LINE,                  // mode
        "ICLI_CMD_MODE_CONFIG_LINE",                // mode str
        "Line Configuration Mode",                  // description
        "config-line",                              // prompt
        "line",                                     // name
        "line { <0~16> | console 0 | vty <0~15> }", // ICLI command
        TRUE                                        // b_mode_var
    },
    {
        ICLI_CMD_MODE_IPMC_PROFILE,     // mode
        "ICLI_CMD_MODE_IPMC_PROFILE",   // mode str
        "IPMC Profile Mode",            // description
        "config-ipmc-profile",          // prompt
        "ipmc-profile",                 // name
        "ipmc profile <word1-16>",      // ICLI command
        TRUE                            // b_mode_var
    },
    {
        ICLI_CMD_MODE_SNMPS_HOST,      // mode
        "ICLI_CMD_MODE_SNMPS_HOST",    // mode str
        "SNMP Server Host Mode",       // description
        "config-snmps-host",           // prompt
        "snmps-host",                  // name
        "snmp-server host <word32>",   // ICLI command
        TRUE                           // b_mode_var
    },
    {
        ICLI_CMD_MODE_STP_AGGR,         // mode
        "ICLI_CMD_MODE_STP_AGGR",       // mode str
        "STP Aggregation Mode",         // description
        "config-stp-aggr",              // prompt
        "stp-aggr",                     // name
        "spanning-tree aggregation",    // ICLI command
        FALSE                           // b_mode_var
    },
    {
        ICLI_CMD_MODE_DHCP_POOL,        // mode
        "ICLI_CMD_MODE_DHCP_POOL",      // mode str
        "DHCP Pool Configuration Mode", // description
        "config-dhcp-pool",             // prompt
        "dhcp-pool",                    // name
        "ip dhcp pool <word32>",        // ICLI command
        TRUE                            // b_mode_var
    },
    {
        ICLI_CMD_MODE_JSON_NOTI_HOST,       // mode
        "ICLI_CMD_MODE_JSON_NOTI_HOST",     // mode str
        "JSON Notification Host Mode",      // description
        "config-json-noti-host",            // prompt
        "json-noti-host",                   // name
        "json notification host <word32>",  // ICLI command
        TRUE                                // b_mode_var
    },
    {
        ICLI_CMD_MODE_LLAG,         // mode
        "ICLI_CMD_MODE_LLAG",       // mode str
        "LLAG Mode",                // description
        "config-llag",              // prompt
        "llag",                     // name
        "interface llag <uint>",    // ICLI command
        TRUE                        // b_mode_var
    },
    {
        ICLI_CMD_MODE_QOS_INGRESS_MAP,      // mode
        "ICLI_CMD_MODE_QOS_INGRESS_MAP",    // mode str
        "QoS Ingress Map Mode",             // description
        "config-qos-map-ingress",           // prompt
        "qos-map-ingress",                  // name
        "qos map ingress <uint>",           // ICLI command
        TRUE                                // b_mode_var
    },
    {
        ICLI_CMD_MODE_QOS_EGRESS_MAP,       // mode
        "ICLI_CMD_MODE_QOS_EGRESS_MAP",     // mode str
        "QoS Egress Map Mode",              // description
        "config-qos-map-egress",            // prompt
        "qos-map-egress",                   // name
        "qos map egress <uint>",            // ICLI command
        TRUE                                // b_mode_var
    },
    {
        ICLI_CMD_MODE_CFM_MD,            // mode
        "ICLI_CMD_MODE_CFM_MD",          // mode str
        "CFM Domain Configuration Mode", // description
        "config-cfm-dmn",                // prompt
        "cfm-dmn",                       // name
        "cfm domain <kword1-15>",        // ICLI command
        TRUE                             // b_mode_var
    },
    {
        ICLI_CMD_MODE_CFM_MA,             // mode
        "ICLI_CMD_MODE_CFM_MA",           // mode str
        "CFM Service Configuration Mode", // description
        "config-cfm-dmn-svc",             // prompt
        "cfm-dmn-svc",                    // name
        "service <kword1-15>",            // ICLI command
        TRUE                              // b_mode_var
    },
    {
        ICLI_CMD_MODE_CFM_MEP,            // mode
        "ICLI_CMD_MODE_CFM_MEP",          // mode str
        "CFM MEP Configuration Mode",     // description
        "config-cfm-dmn-svc-mep",         // prompt
        "cfm-dmn-svc-mep",                // name
        "mep <1-8192>",                   // ICLI command
        TRUE                              // b_mode_var
    },
    {
        ICLI_CMD_MODE_APS,               // mode
        "ICLI_CMD_MODE_APS",             // mode str
        "APS Mode",                      // description
        "config-aps",                    // prompt
        "aps",                           // name
        "aps <uint>",                    // ICLI command
        TRUE                             // b_mode_var
    },
    {
        ICLI_CMD_MODE_ERPS,              // mode
        "ICLI_CMD_MODE_ERPS",            // mode str
        "ERPS Mode",                     // description
        "config-erps",                   // prompt
        "erps",                          // name
        "erps <uint>",                   // ICLI command
        TRUE                             // b_mode_var
    },
    {
        ICLI_CMD_MODE_IEC_MRP,           // mode
        "ICLI_CMD_MODE_IEC_MRP",         // mode str
        "Media-redundancy Mode",         // description
        "config-media-redundancy",       // prompt
        "media-redundancy",              // name
        "media-redundancy <uint>",       // ICLI command
        TRUE                             // b_mode_var
    },
    {
        ICLI_CMD_MODE_REDBOX,            // mode
        "ICLI_CMD_MODE_REDBOX",          // mode str
        "Redbox Mode",                   // description
        "config-redbox",                 // prompt
        "redbox",                        // name
        "redbox <uint>",                 // ICLI command
        TRUE                             // b_mode_var
    },
    {
        ICLI_CMD_MODE_MULTILINE,         // mode
        "ICLI_CMD_MODE_MULTILINE",       // mode str
        "ICLI mode for multiline input", // description
        "multiline-input",               // prompt
        "multiline",                     // name
        "banner login <line>",           // ICLI command
        FALSE                            // b_mode_var
    },
    {
        ICLI_CMD_MODE_CONFIG_CPU_PORT,   // mode
        "ICLI_CMD_MODE_CONFIG_CPU_PORT", // mode str
        "CPU Port Configuration Mode",   // description
        "config-cpu-port",               // prompt
        "config-cpu-port",               // name
        "cpu <cpu_port_list>",           // ICLI command
        TRUE                             // b_mode_var
    },
    {
        ICLI_CMD_MODE_CONFIG_ROUTER_OSPF,   // mode
        "ICLI_CMD_MODE_CONFIG_ROUTER_OSPF", // mode str
        "OSPF Router Mode",                 // description
        "config-router",                    // prompt
        "router-ospf-if",                   // name
        "router ospf",                      // ICLI command
        FALSE                               // b_mode_var
        /* TODO: Need to be changed when multiple OSPF processes
         * seaching keyword: FRR_MGMT_OSPF_DEFAULT_INSTANCE_ID
         *
         * "router ospf <uint>",            // ICLI command
         * TRUE                             // b_mode_var
         */
    },
    {
        ICLI_CMD_MODE_CONFIG_ROUTER_OSPF6,   // mode
        "ICLI_CMD_MODE_CONFIG_ROUTER_OSPF6", // mode str
        "OSPFv3 Router Mode",                // description
        "config-router",                     // prompt
        "router-ospf6-if",                   // name
        "router ospf6",                      // ICLI command
        FALSE                                // b_mode_var
        /* TODO: Need to be changed when multiple OSPFv3 processes
         * seaching keyword: FRR_MGMT_OSPF6_DEFAULT_INSTANCE_ID
         *
         * "router ospf6 <uint>",           // ICLI command
         * TRUE                             // b_mode_var
         */
    },
    {
        ICLI_CMD_MODE_CONFIG_ROUTER_RIP,    // mode
        "ICLI_CMD_MODE_CONFIG_ROUTER_RIP",  // mode str
        "RIP Router Mode",                  // description
        "config-router",                    // prompt
        "router-rip-if",                    // name
        "router rip",                       // ICLI command
        FALSE                               // b_mode_var
    },
    {
        ICLI_CMD_MODE_CONFIG_ROUTER_KEYCHAIN,   // mode
        "ICLI_CMD_MODE_CONFIG_ROUTER_KEYCHAIN", // mode str
        "Router Keychain Mode",                 // description
        "config-keychain",                      // prompt
        "router-keychain",                      // name
        "key chain <word31>",                   // ICLI command
        TRUE                                    // b_mode_var
    },
    {
        ICLI_CMD_MODE_STREAM,            // mode
        "ICLI_CMD_MODE_STREAM",          // mode str
        "Stream Configuration Mode",     // description
        "config-stream",                 // prompt
        "stream",                        // name
        "stream <uint>",                 // ICLI command
        TRUE                             // b_mode_var
    },
    {
        ICLI_CMD_MODE_STREAM_COLLECTION,        // mode
        "ICLI_CMD_MODE_STREAM_COLLECTION",      // mode str
        "Stream-collection Configuration Mode", // description
        "config-stream-collection",             // prompt
        "stream-collection",                    // name
        "stream-collection <uint>",             // ICLI command
        TRUE                                    // b_mode_var
    },
    {
        ICLI_CMD_MODE_TSN_PSFP_FLOW_METER,   // mode
        "ICLI_CMD_MODE_TSN_PSFP_FLOW_METER", // mode str
        "TSN Flow Meter Configuration Mode", // description
        "config-flow-meter",                 // prompt
        "flow-meter",                        // name
        "tsn flow meter <uint>",             // ICLI command
        TRUE                                 // b_mode_var
    },
    {
        ICLI_CMD_MODE_TSN_PSFP_GATE,          // mode
        "ICLI_CMD_MODE_TSN_PSFP_GATE",        // mode str
        "TSN Stream Gate Configuration Mode", // description
        "config-stream-gate",                 // prompt
        "stream-gate",                        // name
        "tsn stream gate <uint>",             // ICLI command
        TRUE                                  // b_mode_var
    },
    {
        ICLI_CMD_MODE_TSN_PSFP_FILTER,          // mode
        "ICLI_CMD_MODE_TSN_PSFP_FILTER",        // mode str
        "TSN Stream Filter Configuration Mode", // description
        "config-stream-filter",                 // prompt
        "stream-filter",                        // name
        "tsn stream filter <uint>",             // ICLI command
        TRUE                                    // b_mode_var
    },
    {
        ICLI_CMD_MODE_TSN_FRER,               // mode
        "ICLI_CMD_MODE_TSN_FRER",             // mode str
        "TSN FRER configuration Mode",        // description
        "config-frer",                        // prompt
        "frer",                               // name
        "tsn frer <uint>",                    // ICLI command
        TRUE                                  // b_mode_var
    },
    {
        ICLI_CMD_MODE_MACSEC,              // mode
        "ICLI_CMD_MODE_MACSEC",            // mode str
        "MACsec Configuration Mode",       // description
        "config-if-macsec",                // prompt
        "macsec",                          // name
        "macsec",                          // ICLI command
        FALSE                              // b_mode_var
    },
    {
        ICLI_CMD_MODE_MACSEC_SECY,         // mode
        "ICLI_CMD_MODE_MACSEC_SECY",       // mode str
        "MACsec SecY Configuration Mode",  // description
        "config-if-macsec-secy",           // prompt
        "macsec-secy",                     // name
        "secy <uint>",                     // ICLI command
        TRUE                               // b_mode_var
    },
    {
        ICLI_CMD_MODE_MACSEC_TX_SC,           // mode
        "ICLI_CMD_MODE_MACSEC_TX_SC",         // mode str
        "MACsec SecY Tx Secure Channel Mode", // description
        "config-if-macsec-secy-tx-sc",        // prompt
        "macsec-secy-tx-sc",                  // name
        "sc <uint>",                          // ICLI command
        TRUE                                  // b_mode_var
    },
    {
        ICLI_CMD_MODE_MACSEC_RX_SC,           // mode
        "ICLI_CMD_MODE_MACSEC_RX_SC",         // mode str
        "MACsec SecY Rx Secure Channel Mode", // description
        "config-if-macsec-secy-rx-sc",        // prompt
        "macsec-secy-rx-sc",                  // name
        "sc <uint>",                          // ICLI command
        TRUE                                  // b_mode_var
    },
};

#pragma GCC diagnostic pop
/* modify as icli_privilege_t */
static const char *g_privilege_str[] = {
    "ICLI_PRIVILEGE_0",
    "ICLI_PRIVILEGE_1",
    "ICLI_PRIVILEGE_2",
    "ICLI_PRIVILEGE_3",
    "ICLI_PRIVILEGE_4",
    "ICLI_PRIVILEGE_5",
    "ICLI_PRIVILEGE_6",
    "ICLI_PRIVILEGE_7",
    "ICLI_PRIVILEGE_8",
    "ICLI_PRIVILEGE_9",
    "ICLI_PRIVILEGE_10",
    "ICLI_PRIVILEGE_11",
    "ICLI_PRIVILEGE_12",
    "ICLI_PRIVILEGE_13",
    "ICLI_PRIVILEGE_14",
    "ICLI_PRIVILEGE_15",
#if 0 /* Bugzilla#14129 - remove debug level */
    "ICLI_PRIVILEGE_DEBUG",
#endif
};

/*
==============================================================================

    Static Function

==============================================================================
*/

/*
==============================================================================

    Public Function

==============================================================================
*/
/*
    get command mode info by string

    INPUT
        mode_str : string of mode

    OUTPUT
        n/a

    RETURN
        icli_cmd_mode_info_t * : successful
        NULL                   : failed

    COMMENT
        n/a
*/
extern "C" const icli_cmd_mode_info_t *icli_platform_cmd_mode_info_get_by_str(
    IN  char    *mode_str
)
{
    icli_cmd_mode_t     mode;

    for ( mode = (icli_cmd_mode_t)0; mode < ICLI_CMD_MODE_MAX; ++mode ) {
        if ( vtss_icli_str_cmp(mode_str, g_cmd_mode_info[mode].str) == 0 ) {
            break;
        }
    }

    if ( mode == ICLI_CMD_MODE_MAX ) {
        return NULL;
    }

    return &g_cmd_mode_info[mode];
}

/*
    get command mode info by command mode

    INPUT
        mode : command mode

    OUTPUT
        n/a

    RETURN
        icli_cmd_mode_info_t * : successful
        NULL                   : failed

    COMMENT
        n/a
*/
extern "C" const icli_cmd_mode_info_t *icli_platform_cmd_mode_info_get_by_mode(
    IN  icli_cmd_mode_t     mode
)
{
    if ( mode >= ICLI_CMD_MODE_MAX ) {
        return NULL;
    }
    return &g_cmd_mode_info[mode];
}

/*
    get privilege by string

    INPUT
        priv_str : string of privilege

    OUTPUT
        n/a

    RETURN
        icli_privilege_t   : successful
        ICLI_PRIVILEGE_MAX : failed

    COMMENT
        n/a
*/
extern "C" icli_privilege_t icli_platform_privilege_get_by_str(
    IN char     *priv_str
)
{
    i32     priv;

    if ( vtss_icli_str_to_int(priv_str, &priv) == 0 ) {
        if ( priv >= 0 && priv < ICLI_PRIVILEGE_MAX ) {
            return (icli_privilege_t)priv;
        }
        return ICLI_PRIVILEGE_MAX;
    }

    for ( priv = 0; priv < ICLI_PRIVILEGE_MAX; ++priv ) {
        if ( vtss_icli_str_cmp(priv_str, g_privilege_str[priv]) == 0 ) {
            break;
        }
    }
    return (icli_privilege_t)priv;
}

#ifdef ICLI_TARGET

/*
    calculate MD5
*/
void icli_platform_hmac_md5(
    IN  const unsigned char *key,
    IN  size_t              key_len,
    IN  const unsigned char *data,
    IN  size_t              data_len,
    OUT unsigned char       *mac
)
{
#ifdef VTSS_SW_OPTION_MD5
    vtss_hmac_md5(key, key_len, data, data_len, mac);
#else
    memset(mac, 0, ICLI_HMAC_MD5_MAX_LEN);
#endif
}

/*
    cursor up
*/
void icli_platform_cursor_up(
    IN icli_session_handle_t    *handle
)
{
    _CSI;
    (void)vtss_icli_sutil_usr_char_put(handle, _CURSOR_UP);
}

/*
    cursor down
*/
void icli_platform_cursor_down(
    IN icli_session_handle_t    *handle
)
{
    _CSI;
    (void)vtss_icli_sutil_usr_char_put(handle, _CURSOR_DOWN);
}

/*
    cursor forward
*/
void icli_platform_cursor_forward(
    IN icli_session_handle_t    *handle
)
{
    i32     x, y;
    i32     max_x;

    // get current x,y
    vtss_icli_sutil_current_xy_get(handle, &x, &y);

    // get max x
    max_x = handle->runtime_data.width - 1;

    if ( x < max_x ) {
        // just go forward
        _CSI;
        (void)vtss_icli_sutil_usr_char_put(handle, _CURSOR_FORWARD);
    } else {
        // go to the beginning of next line
        icli_platform_cursor_offset(handle, 0 - max_x, 1);
    }
}

/*
    cursor backward
*/
void icli_platform_cursor_backward(
    IN icli_session_handle_t    *handle
)
{
    i32     x, y;

    // get current x,y
    vtss_icli_sutil_current_xy_get(handle, &x, &y);

    if ( x ) {
        // just backward
        _CSI;
        (void)vtss_icli_sutil_usr_char_put(handle, _CURSOR_BACKWARD);
    } else {
        // go to the end of previous line
        icli_platform_cursor_offset(handle, handle->runtime_data.width - 1, -1);
    }

}

/*
    cursor go to ( current_x + offser_x, current_y + offset_y )
*/
void icli_platform_cursor_offset(
    IN icli_session_handle_t    *handle,
    IN i32                      offset_x,
    IN i32                      offset_y
)
{
    char    s[32];

    if ( offset_x > 0 ) {
        // forward
        _CSI;
        icli_sprintf(s, "%d", offset_x);
        vtss_icli_session_str_put(handle, s);
        (void)vtss_icli_sutil_usr_char_put(handle, _CURSOR_FORWARD);
    } else if ( offset_x < 0 ) {
        // backward
        _CSI;
        icli_sprintf(s, "%d", 0 - offset_x);
        vtss_icli_session_str_put(handle, s);
        (void)vtss_icli_sutil_usr_char_put(handle, _CURSOR_BACKWARD);
    }

    if ( offset_y > 0 ) {
        // down
        _CSI;
        icli_sprintf(s, "%d", offset_y);
        vtss_icli_session_str_put(handle, s);
        (void)vtss_icli_sutil_usr_char_put(handle, _CURSOR_DOWN);
    } else if ( offset_y < 0 ) {
        // up
        _CSI;
        icli_sprintf(s, "%d", 0 - offset_y);
        vtss_icli_session_str_put(handle, s);
        (void)vtss_icli_sutil_usr_char_put(handle, _CURSOR_UP);
    }
}

/*
    cursor backward and delete that char
    update cursor_pos, but not cmd_pos and cmd_len
*/
void icli_platform_cursor_backspace(
    IN  icli_session_handle_t   *handle
)
{
    i32     x, y;

    icli_platform_cursor_backward(handle);
    ICLI_PUT_SPACE;

    /*
        if just go backward when at the end of postion,
        then the cursor will go up one line.
        the root cause is if put char at the end of position
        then the cursor is still there and will not go to next line.
    */

    // get current x,y
    vtss_icli_sutil_current_xy_get(handle, &x, &y);
    if ( x == 0 ) {
        ICLI_PUT_SPACE;
        ICLI_PUT_BACKSPACE;
    }

    icli_platform_cursor_backward(handle);

    _DEC_1( handle->runtime_data.cursor_pos );
}

/*
    get switch information
*/
void icli_platform_switch_info_get(OUT icli_switch_info_t *switch_info)
{
}

/*
    check if auth callback exists

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        TRUE  - yes, the callback exists
        FALSE - no, it is NULL
*/
BOOL icli_platform_has_user_auth(void)
{
#ifdef VTSS_SW_OPTION_AUTH
    return TRUE;
#else
    return FALSE;
#endif
}

/*
    authenticate user

    INPUT
        session_way  : way to access session
        username     : user name
        password     : password for the user

    OUTPUT
        privilege    : the privilege level for the authenticated user
        agent_id     : agent id assigned to the session

    RETURN
        icli_rc_t
*/
i32 icli_platform_user_auth(
    IN  icli_session_way_t  session_way,
    IN  char                *hostname,
    IN  char                *username,
    IN  char                *password,
    OUT u32                 *priv_lvl,
    OUT u32                 *agent_id
)
{
#ifdef VTSS_SW_OPTION_AUTH

    vtss_appl_auth_agent_t  agent;
    u8                      plvl;
    u16                     aid;
    mesa_rc                 rc;

    switch (session_way) {
    case ICLI_SESSION_WAY_CONSOLE:
        agent = VTSS_APPL_AUTH_AGENT_CONSOLE;
        break;

    case ICLI_SESSION_WAY_TELNET:
        agent = VTSS_APPL_AUTH_AGENT_TELNET;
        break;

    case ICLI_SESSION_WAY_SSH:
        agent = VTSS_APPL_AUTH_AGENT_SSH;
        break;

    case ICLI_SESSION_WAY_APP_EXEC:
        *priv_lvl = ICLI_PRIVILEGE_15;
        return ICLI_RC_OK;

    default:
        T_E("Invalid session way = %d\n", session_way);
        return ICLI_RC_ERR_PARAMETER;
    }

    // give semaphore to avoid lock too long
    _ICLI_SEMA_GIVE();

    rc = vtss_auth_login(agent, hostname, username, password, &plvl, &aid);

    // take again
    _ICLI_SEMA_TAKE();

    if ( rc == VTSS_RC_OK ) {
        if (priv_lvl) {
            *priv_lvl = plvl;
        }
        if (agent_id) {
            *agent_id = aid;
        }
        return ICLI_RC_OK;
    }

    return ICLI_RC_ERROR;

#else // VTSS_SW_OPTION_AUTH

    if (session_way) {}
    if (username) {}
    if (password) {}
    if (hostname) {}
    if (agent_id) {}

    if ( priv_lvl ) {
        *priv_lvl = ICLI_PRIVILEGE_MAX - 1;
    }

    if ( agent_id ) {
        *agent_id = 0;
    }
    return ICLI_RC_OK;

#endif // VTSS_SW_OPTION_AUTH
}

/*
    authorize command

    INPUT
        handle    : session handle
        command   : full command string
        cmd_priv  : highest command privilege
        b_execute : TRUE if <enter>, FALSE if <tab>, '?' or '??'

    OUTPUT
        n/a

    RETURN
        icli_rc_t
*/
i32 icli_platform_cmd_authorize(
    IN icli_session_handle_t    *handle,
    IN char                     *command,
    IN u32                      cmd_priv,
    IN BOOL                     b_execute
)
{
    i32                     rc = ICLI_RC_ERR_AAA_IGNORE;

#ifdef VTSS_SW_OPTION_AUTH

    vtss_appl_auth_agent_t  agent;
    char                    hostname[46 /* INET6_ADDRSTRLEN */];
    mesa_ip_addr_t          client_ip;
    char                    user_name[ICLI_USERNAME_MAX_LEN + 4];
    u8                      priv_lvl;
    u16                     agent_id;
    BOOL                    b_cfg_cmd;

    T_D("%s command: '%s'", b_execute ? "Execute" : "query", command);

    if (command == NULL || ICLI_IS_(EOS, (*command))) {
        T_W("command == NULL || *command = 0\n");
        goto _icli_cmd_authorize_end;
    }

    switch (handle->open_data.way) {
    case ICLI_SESSION_WAY_CONSOLE:
    case ICLI_SESSION_WAY_THREAD_CONSOLE:
        agent = VTSS_APPL_AUTH_AGENT_CONSOLE;
        break;

    case ICLI_SESSION_WAY_TELNET:
    case ICLI_SESSION_WAY_THREAD_TELNET:
        agent = VTSS_APPL_AUTH_AGENT_TELNET;
        break;

    case ICLI_SESSION_WAY_SSH:
    case ICLI_SESSION_WAY_THREAD_SSH:
        agent = VTSS_APPL_AUTH_AGENT_SSH;
        break;

    default:
        rc = ICLI_RC_ERR_AAA_IGNORE; /* Irrelevant session way */
        goto _icli_cmd_authorize_end;
    }

    client_ip = handle->open_data.client_ip;
    (void)vtss_icli_str_cpy(user_name, handle->runtime_data.user_name);
    priv_lvl = (u8)(handle->runtime_data.privilege);
    agent_id = (u16)(handle->runtime_data.agent_id);
    b_cfg_cmd = (handle->runtime_data.mode_para[handle->runtime_data.mode_level].mode > ICLI_CMD_MODE_EXEC);

    // give semaphore to avoid lock too long
    _ICLI_SEMA_GIVE();

    rc = vtss_auth_cmd(agent,
                       misc_ip_txt(&client_ip, hostname),
                       user_name,
                       command,
                       priv_lvl,
                       agent_id,
                       b_execute,
                       (u8)cmd_priv,
                       b_cfg_cmd);

    // take again
    _ICLI_SEMA_TAKE();

    switch (rc) {
    case VTSS_RC_OK:
        rc = ICLI_RC_OK;
        goto _icli_cmd_authorize_end;

    case VTSS_APPL_AUTH_ERROR_SERVER_REJECT:
        rc = ICLI_RC_ERR_AAA_REJECT;
        goto _icli_cmd_authorize_end;

    default:
        break;
    }

_icli_cmd_authorize_end:

#else // VTSS_SW_OPTION_AUTH

    if (handle) {}
    if (command) {}
    if (cmd_priv) {}
    if (b_execute) {}

#endif // VTSS_SW_OPTION_AUTH

    vtss_icli_exec_full_cmd_str_get_end(handle);
    return rc;
}

/*
    user logout

    INPUT
        handle    : session handle

    OUTPUT
        n/a

    RETURN
        icli_rc_t
*/
i32 icli_platform_user_logout(
    IN icli_session_handle_t    *handle
)
{
    i32                     rc = ICLI_RC_OK;

#ifdef VTSS_SW_OPTION_AUTH

    vtss_appl_auth_agent_t  agent;
    char                    hostname[46 /* INET6_ADDRSTRLEN */];
    mesa_ip_addr_t          client_ip;
    char                    user_name[ICLI_USERNAME_MAX_LEN + 4];
    u8                      priv_lvl;
    u16                     agent_id;

    switch (handle->open_data.way) {
    case ICLI_SESSION_WAY_CONSOLE:
        agent = VTSS_APPL_AUTH_AGENT_CONSOLE;
        break;

    case ICLI_SESSION_WAY_TELNET:
        agent = VTSS_APPL_AUTH_AGENT_TELNET;
        break;

    case ICLI_SESSION_WAY_SSH:
        agent = VTSS_APPL_AUTH_AGENT_SSH;
        break;

    default:
        T_E("Invalid session way = %d\n", handle->open_data.way);
        return ICLI_RC_ERR_PARAMETER;
    }

    client_ip = handle->open_data.client_ip;
    (void)vtss_icli_str_cpy(user_name, handle->runtime_data.user_name);
    priv_lvl = (u8)(handle->runtime_data.privilege);
    agent_id = (u16)(handle->runtime_data.agent_id);

    // give semaphore to avoid lock too long
    _ICLI_SEMA_GIVE();

    rc = vtss_auth_logout(agent,
                          misc_ip_txt(&client_ip, hostname),
                          user_name,
                          priv_lvl,
                          agent_id);

    // take again
    _ICLI_SEMA_TAKE();

#else // VTSS_SW_OPTION_AUTH

    if (handle) {}

#endif // VTSS_SW_OPTION_AUTH

    return rc;
}

u16 icli_platform_isid2usid(
    IN u16  isid
)
{
    return (u16)( topo_isid2usid((vtss_isid_t)isid) );
}

u16 icli_platform_iport2uport(
    IN u16  iport
)
{
    return (u16)( iport2uport((mesa_port_no_t)iport) );
}

/*
    get switch_id by usid

    INPUT
        usid

    OUTPUT
        n/a

    RETURN
        switch ID

    COMMENT
        n/a
*/
u16 icli_platform_usid2switchid(
    IN u16  usid
)
{
    return usid;
}

/*
    get switch_id by isid

    INPUT
        isid

    OUTPUT
        n/a

    RETURN
        switch ID

    COMMENT
        n/a
*/
u16 icli_platform_isid2switchid(
    IN u16  isid
)
{
    u16     usid;

    usid = icli_platform_isid2usid( isid );

    return icli_platform_usid2switchid( usid );
}

/*
    configure current port ranges including stacking into ICLI

    INPUT
        n/a

    OUTPUT
        n/a

    RETURN
        TRUE  - successful
        FALSE - failed

    COMMENT
        n/a
*/
BOOL icli_platform_port_range(
    void
)
{
    icli_stack_port_range_t *rng;
    switch_iter_t           s_iter;
    port_iter_t             p_iter;
    icli_port_type_t        type;
    mesa_rc                 rc;
    BOOL                    b_rc;
    u32                     r_idx = 0;

    T_D("entry");

    if (!msg_switch_is_primary()) {
        // Not primary switch => don't do anything
        return TRUE;
    }

    vtss_icli_port_range_reset();

    rng = (icli_stack_port_range_t *)icli_malloc(sizeof(icli_stack_port_range_t));
    if (!rng) {
        T_E("Cannot allocate memory for building port range");
        return FALSE;
    }
    memset(rng, 0, sizeof(icli_stack_port_range_t));

    if ((rc = switch_iter_init(&s_iter, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG)) != VTSS_RC_OK) {
        T_E("Cannot init switch iteration; rc=%d", rc);
        icli_free(rng);
        return FALSE;
    }

    while (switch_iter_getnext(&s_iter)) {
        u32  fe_cnt = 0, g_cnt = 0, g_2_5_cnt = 0, g_5_cnt = 0, g_10_cnt = 0, g_25_cnt = 0,
             cpu_cnt = 0, begin = 0;
        if ((rc = port_iter_init(&p_iter, NULL, s_iter.isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_CPU)) != VTSS_RC_OK) {
            T_E("Cannot init port iteration for isid=%u; rc=%d. Ignoring switch.", s_iter.isid, rc);
            continue;
        }
        while (port_iter_getnext(&p_iter)) {
            if (! p_iter.exists) {
                continue;
            }
            meba_port_cap_t cap = 0;
            VTSS_RC_ERR_PRINT(port_cap_get(p_iter.iport, &cap));
            if (cap & (MEBA_PORT_CAP_25G_FDX)) {
                type = ICLI_PORT_TYPE_25_GIGABIT_ETHERNET;
                begin = ++g_25_cnt;
            } else if (cap & (MEBA_PORT_CAP_10G_FDX)) {
                type = ICLI_PORT_TYPE_TEN_GIGABIT_ETHERNET;
                begin = ++g_10_cnt;
            } else if (cap & (MEBA_PORT_CAP_5G_FDX)) {
                type = ICLI_PORT_TYPE_FIVE_GIGABIT_ETHERNET;
                begin = ++g_5_cnt;
            } else if (cap & MEBA_PORT_CAP_2_5G_FDX) {
                type = ICLI_PORT_TYPE_2_5_GIGABIT_ETHERNET;
                begin = ++g_2_5_cnt;
            } else if (cap & MEBA_PORT_CAP_1G_FDX) {
                type = ICLI_PORT_TYPE_GIGABIT_ETHERNET;
                begin = ++g_cnt;
            } else if (cap & MEBA_PORT_CAP_100M_FDX) {
                type = ICLI_PORT_TYPE_FAST_ETHERNET;
                begin = ++fe_cnt;
            } else if ((cap & MEBA_PORT_CAP_CPU) == MEBA_PORT_CAP_CPU) {
                type = ICLI_PORT_TYPE_CPU;
                begin = ++cpu_cnt;
            } else {
                T_D("Unexpected port speed. Capabilities = 0x%08x", (unsigned int)cap);
                continue;
            }

            icli_port_iter_add_port(type, s_iter.isid, p_iter.iport, p_iter.uport);

            if (rng->switch_range[r_idx].port_cnt == 0) {  // First port, init range entry
                rng->switch_range[r_idx].port_type   = type;
                rng->switch_range[r_idx].switch_id   = s_iter.usid;
                rng->switch_range[r_idx].begin_port  = begin;
                rng->switch_range[r_idx].usid        = s_iter.usid;
                rng->switch_range[r_idx].begin_uport = p_iter.uport;
                rng->switch_range[r_idx].isid        = s_iter.isid;
                rng->switch_range[r_idx].begin_iport = p_iter.iport;
            }

            if (rng->switch_range[r_idx].port_type != type) {  // Type change, we need a new range entry
                if (r_idx == ICLI_RANGE_LIST_CNT - 1) {
                    T_E("ICLI port range enumeration full; remaining ports will be ignored");
                    goto done;
                }
                ++r_idx;
                rng->switch_range[r_idx].port_cnt    = 0;
                rng->switch_range[r_idx].port_type   = type;
                rng->switch_range[r_idx].switch_id   = s_iter.usid;
                rng->switch_range[r_idx].begin_port  = begin;
                rng->switch_range[r_idx].usid        = s_iter.usid;
                rng->switch_range[r_idx].begin_uport = p_iter.uport;
                rng->switch_range[r_idx].isid        = s_iter.isid;
                rng->switch_range[r_idx].begin_iport = p_iter.iport;
            }

            rng->switch_range[r_idx].port_cnt++;

        }  /* while (port_iter_getnext(...)) */

        r_idx += (rng->switch_range[0].port_cnt > 0) ? 1 : 0;
    }  /* while (switch_iter_getnext(...) */

done:
    rng->cnt = r_idx;
    b_rc = vtss_icli_port_range_set(rng);
    icli_free(rng);

    T_D("exit, cnt=%u; return=%d", r_idx, (int)b_rc);

    return b_rc;
}

#endif // ICLI_TARGET

/*
    get mode prompt

    INPUT
        mode : command mode

    OUTPUT
        n/a

    RETURN
        mode prompt
*/
const char *icli_platform_mode_prompt_get(
    IN  icli_cmd_mode_t     mode
)
{
    if ( mode >= ICLI_CMD_MODE_MAX ) {
        return "";
    }
    return ( g_cmd_mode_info[mode].prompt );
}

/*
    get mode name

    INPUT
        mode : command mode

    OUTPUT
        n/a

    RETURN
        mode name
*/
char *icli_platform_mode_name_get(
    IN  icli_cmd_mode_t     mode
)
{
    static char empty[1] = {0};
    if ( mode >= ICLI_CMD_MODE_MAX ) {
        return empty;
    }
    return ( g_cmd_mode_info[mode].name );
}

/*
    get mode by name

    INPUT
        name : name of command mode

    OUTPUT
        n/a

    RETURN
        mode : error if return ICLI_CMD_MODE_MAX

*/
icli_cmd_mode_t icli_platform_mode_get_by_name(
    IN  char    *name
)
{
    icli_cmd_mode_t     mode;

    if ( name == NULL ) {
        return ICLI_CMD_MODE_MAX;
    }

    for ( mode = ICLI_CMD_MODE_EXEC; mode < ICLI_CMD_MODE_MAX; ++mode ) {
        if ( vtss_icli_str_sub(name, g_cmd_mode_info[mode].name, 0, NULL) != -1 ) {
            return mode;
        }
    }

    return ICLI_CMD_MODE_MAX;
}

/*
    get mode string

    INPUT
        mode : command mode

    OUTPUT
        n/a

    RETURN
        mode string
*/
const char *icli_platform_mode_str_get(
    IN  icli_cmd_mode_t     mode
)
{
    if ( mode >= ICLI_CMD_MODE_MAX ) {
        return "";
    }
    return ( g_cmd_mode_info[mode].str );
}
