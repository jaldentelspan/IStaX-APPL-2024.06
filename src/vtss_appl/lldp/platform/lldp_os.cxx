/*

 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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


#include "lldp_os.h"
#include "main.h"
#include "vtss_common_os.h"
#include "lldp_sm.h"
#include "lldp_api.h"
#include "lldp_trace.h"
#include "lldp_private.h"
#include "misc_api.h"
#include "msg_api.h"
#include "lldp_remote.h"
#ifdef VTSS_SW_OPTION_ICLI
#include "icli_porting_util.h"
#include "icli_api.h"
#endif

static lldp_bool_t use_tx_shutdown_buffer = LLDP_FALSE;
static lldp_u8_t tx_shutdown_buffer[64];
#define SHARED_BUFFER_SIZE 1518
lldp_u8_t shared_buffer [SHARED_BUFFER_SIZE];


/* from SNMP */
extern lldp_u32_t time_since_boot;


lldp_timer_t lldp_os_get_msg_tx_interval (void)
{
    vtss_appl_lldp_common_conf_t conf;
    (void) lldp_common_local_conf_get(&conf);

    // According to the standard the msgmsgTxInterval shall not be below 5 sec. ( This should never happen since the cli code also should do this check. )
    if (conf.tx_sm.msgTxInterval < 5) {
        conf.tx_sm.msgTxInterval = 5;
    }
    return conf.tx_sm.msgTxInterval;

}

lldp_timer_t lldp_os_get_msg_tx_hold (void)
{
    vtss_appl_lldp_common_conf_t conf;
    (void) lldp_common_local_conf_get(&conf);
    return conf.tx_sm.msgTxHold;
}

lldp_timer_t lldp_os_get_reinit_delay (void)
{
    vtss_appl_lldp_common_conf_t conf;
    (void) lldp_common_local_conf_get(&conf);
    return conf.tx_sm.reInitDelay;
}

lldp_timer_t lldp_os_get_tx_delay (void)
{
    lldp_timer_t tx_interval = lldp_os_get_msg_tx_interval();

    vtss_appl_lldp_common_conf_t conf;
    (void) lldp_common_local_conf_get(&conf);
    lldp_timer_t tx_delay = conf.tx_sm.txDelay;

    /* must not be larger that 0.25 * msgTxInterval*/
    if (tx_delay > (tx_interval / 4)) {
        return (tx_interval / 4);
    }
    return tx_delay;
}


vtss_appl_lldp_admin_state_t lldp_os_get_admin_status (lldp_port_t port)
{
    mesa_rc rc;
    CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> conf;
    if ((rc = lldp_conf_get(&conf[0])) != VTSS_RC_OK) {
        T_E("%s", error_txt(rc));
    };
    T_RG_PORT(TRACE_GRP_CONF, port, "admin:%d", conf[port].admin_states);
    return conf[port].admin_states;
}


/* return some memory we can use to build up frame in */
lldp_u8_t *lldp_os_get_frame_storage (lldp_bool_t init)
{
    if (use_tx_shutdown_buffer) {
        T_R("Returning tx_shutdown_buffer");
        return tx_shutdown_buffer;
    }
    if (init) {
        memset(&shared_buffer[0], 0, SHARED_BUFFER_SIZE);
    }

    T_RG(TRACE_GRP_TX, "Returning shared_buffer");
    return shared_buffer;
}

void lldp_os_set_tx_in_progress (lldp_bool_t tx_in_progress)
{
    use_tx_shutdown_buffer = tx_in_progress;
}

// Get the interface description as something like "GigbitEthernet 1/23"
// IN:  usid, uport - User switch and port ids.
// OUT: str_buf_p - Pointer to where to put the string result.
void lldp_os_get_if_descr(lldp_port_t uport, char *str_buf_p, u8 buf_len)
{
    // This is a little ugly bugly, and should have been fixed in the topo module, so
    // we could get the correct usid in the interface name by calling icli_port_info_txt, but
    // since that doesn't work, we have to make the interface description this was. See Bugzilla#16893
    // and Bugzilla#16526.
    str_buf_p[0] = '\0';
#ifdef VTSS_SW_OPTION_ICLI
    icli_switch_port_range_t switch_range;
    char *type;
    switch_range.usid = VTSS_USID_START;
    switch_range.begin_uport = uport;
    switch_range.port_cnt = 1;
    if (icli_port_from_usid_uport(&switch_range)) {
        type = icli_port_type_get_name(switch_range.port_type);
        (void) snprintf(str_buf_p, buf_len, "%s %u/%u",
                        type,
                        switch_range.usid,
                        switch_range.begin_port);
    }
#endif

}

void lldp_os_get_system_name (lldp_8_t   *dest)
{
    lldp_system_name(dest, 1); // lldp_os_get_system_name is the "old" function, so we pass it on to the "new" function lldp_get_system_name.
}

void lldp_os_get_system_descr (lldp_8_t   *dest)
{
    char          buf[VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH];
#ifdef CUSTOMIZED_SYSTEM_DESCRIPTION
    snprintf(buf, VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH, "%s", TO_STR(CUSTOMIZED_SYSTEM_DESCRIPTION));
#else
    strncpy(buf, misc_software_version_txt(), VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH - 1);
    strncat(buf, " ", VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH - strlen(buf) - 1);
    strncat(buf, misc_software_date_txt(), VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH - strlen(buf) - 1);
    buf[VTSS_APPL_MAX_SYSTEM_DESCR_LENGTH - 1] = '\0';
    T_NG(TRACE_GRP_TX, "buf = %s", buf);
#endif
    strcpy(&dest[0], buf);
}

// Converting IP as ulong to a 4 bytes char array.
//
// In : ip_ulong - IP address as ulong type
//
// In/Out : ip_char_array - IP as a 4 bytes char array
//
void lldp_os_ip_ulong2char_array (lldp_u8_t *ip_char_array, ulong ip_ulong)
{
    memcpy(ip_char_array, &ip_ulong, 4);

    // Swap bytes
    lldp_u8_t temp;
    temp = *ip_char_array;
    *ip_char_array = *(ip_char_array + 3);
    *(ip_char_array + 3) = temp;

    temp = *(ip_char_array + 1) ;
    *(ip_char_array + 1) = *(ip_char_array + 2);
    *(ip_char_array + 2) = temp;
}

// Geting the IP address
void lldp_os_get_ip_address (lldp_u8_t *dest, lldp_u8_t port)
{
#if defined(VTSS_SW_OPTION_IP)
    lldp_os_ip_ulong2char_array(dest, lldp_ip_addr_get(port));
#endif
}

void lldp_os_set_optional_tlv (lldp_tlv_t tlv_t, lldp_u8_t enabled, u32 *tlv_enabled, lldp_u8_t port)
{
    lldp_u8_t tlv_u8 = (lldp_u8_t) tlv_t;
    /* adjust from TLV value to bits */
    tlv_u8 -= 4;
    if (enabled) {
        *tlv_enabled = *tlv_enabled |  (1 << tlv_u8);
        T_D("Set TLV %u to %u, tlv_enabled= %d", tlv_u8, (unsigned)enabled, *tlv_enabled);
    } else {
        *tlv_enabled = *tlv_enabled &  ~((lldp_u8_t)(1 << tlv_u8));
    }
}

lldp_u8_t lldp_os_get_optional_tlv_enabled(lldp_tlv_t tlv, lldp_u8_t port)
{
    CapArray<vtss_appl_lldp_port_conf_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> conf;
    (void) lldp_conf_get(&conf[0]);
    BOOL enabled;

    switch (tlv) {
    case LLDP_TLV_BASIC_MGMT_PORT_DESCR:
        enabled = conf[port].optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_PORT_DESCR_BIT ? TRUE : FALSE;
        break;
    case  LLDP_TLV_BASIC_MGMT_SYSTEM_NAME:
        enabled = conf[port].optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_NAME_BIT ? TRUE : FALSE;
        break;

    case LLDP_TLV_BASIC_MGMT_SYSTEM_DESCR:
        enabled = conf[port].optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_DESCR_BIT ? TRUE : FALSE;
        break;

    case LLDP_TLV_BASIC_MGMT_SYSTEM_CAPA:
        enabled = conf[port].optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_SYSTEM_CAPA_BIT ? TRUE : FALSE;
        break;

    case LLDP_TLV_BASIC_MGMT_MGMT_ADDR:
        enabled = conf[port].optional_tlvs_mask & VTSS_APPL_LLDP_TLV_OPTIONAL_MGMT_ADDR_BIT ? TRUE : FALSE;
        break;
    default:
        enabled = FALSE;
    }

    return enabled;
}

/* send a frame on external port number */
void lldp_os_tx_frame (lldp_port_t port_no, lldp_u8_t *frame, lldp_u16_t len)
{
    lldp_send_frame(port_no, frame, len);
}

lldp_u32_t lldp_os_get_sys_up_time (void)
{
    T_DG(TRACE_GRP_RX, "Getting entry time to %s", misc_time2str(msg_uptime_get(VTSS_ISID_LOCAL)));
    return msg_uptime_get(VTSS_ISID_LOCAL);
}
