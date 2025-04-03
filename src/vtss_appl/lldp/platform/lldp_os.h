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

/******************************************************************************
 * This header file defines types and macros that are OS and application      *
 * specific.                                                                  *
 ******************************************************************************/

#ifndef LLDP_OS_H
#define LLDP_OS_H

//
// Types and macros for Microsemi WebStaX platform
//
#include "lldp_basic_types.h"
#include "vtss_common_os.h"
#include "main.h"
#include "vtss/appl/lldp.h"
#include "lldp_tlv.h"

#define LLDP_FALSE 0
#define LLDP_TRUE  1


#ifdef VTSS_SW_OPTION_LLDP_MED
#include "lldpmed_shared.h"
#endif

VTSS_BEGIN_HDR
lldp_timer_t lldp_os_get_msg_tx_interval (void);
lldp_timer_t lldp_os_get_msg_tx_hold (void);
lldp_timer_t lldp_os_get_reinit_delay (void);
lldp_timer_t lldp_os_get_tx_delay (void);

void lldp_os_set_msg_tx_interval (lldp_timer_t val);
void lldp_os_set_msg_tx_hold (lldp_timer_t val);
void lldp_os_set_reinit_delay (lldp_timer_t val);
void lldp_os_set_tx_delay (lldp_timer_t val);

lldp_u8_t *lldp_os_get_frame_storage (lldp_bool_t init);
vtss_appl_lldp_admin_state_t lldp_os_get_admin_status (lldp_port_t port);
void lldp_os_set_admin_status (lldp_port_t port, vtss_appl_lldp_admin_state_t admin_state);
void lldp_os_tx_frame (lldp_port_t port_no, lldp_u8_t *frame, lldp_u16_t len);
void lldp_os_get_if_descr(lldp_port_t uport, lldp_8_t *dest, u8 buf_len);
void lldp_os_get_system_name (lldp_8_t *dest);
void lldp_os_get_system_descr (lldp_8_t *dest);
void lldp_os_get_ip_address (lldp_u8_t *dest, lldp_u8_t port);

lldp_u8_t lldp_os_get_ip_enabled (void);
lldp_u8_t lldp_os_get_optional_tlv_enabled(lldp_tlv_t tlv, lldp_u8_t port);
void lldp_os_print_remotemib (void);
void lldp_os_set_tx_in_progress (lldp_bool_t tx_in_progress);
vtss_common_macaddr_t lldp_os_get_primary_switch_mac(void);
lldp_u32_t lldp_os_get_sys_up_time (void);
void lldp_os_ip_ulong2char_array(lldp_u8_t *ip_char_array, ulong ip_ulong);
VTSS_END_HDR

typedef struct {
    /* tlv_optionals_enabled uses bits 0-5 of the octet:
    ** Port Description:    bit 0
    ** System Name:         bit 1
    ** System Description:  bit 2
    ** System Capabilities: bit 3
    ** Management Address:  bit 4
    */

    lldp_u16_t       reInitDelay;
    lldp_u16_t       msgTxHold;
    lldp_u16_t       msgTxInterval;
    lldp_u16_t       txDelay;
    lldp_8_t         timingChanged; // set to one when reInitDelay,msgTxHold,msgTxInterval or txDelay have changed.

    /* interpretation of admin_state is as follows:
    ** (must match vtss_appl_lldp_admin_state_t)
    ** 0 = disabled
    ** 1 = rx_tx
    ** 2 = tx
    ** 3 = rx only
    */
    CapArray<vtss_appl_lldp_admin_state_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> admin_state;
    CapArray<uchar, MEBA_CAP_BOARD_PORT_MAP_COUNT> optional_tlv;
#ifdef VTSS_SW_OPTION_CDP
    CapArray<lldp_bool_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> cdp_aware;
#endif
    vtss_common_macaddr_t mac_addr ; /* Primary switch's mac address*/

#ifdef VTSS_SW_OPTION_LLDP_MED
    vtss_appl_lldp_med_policy_t         policies_table[LLDPMED_POLICIES_CNT]; // Table with all policies
    vtss_appl_lldp_med_location_info_t location_info; // Location information
    lldp_u8_t medFastStartRepeatCount; // Fast Start Repeat Count, See TIA1057
    CapArray<lldp_u8_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldpmed_optional_tlv; // Enable/disable of the LLDP-MED optional TLVs ( See TIA-1057, MIB LLDPXMEDPORTCONFIGTLVSTXENABLE).
#ifdef VTSS_SW_OPTION_LLDP_MED_TYPE
    CapArray<vtss_appl_lldp_med_device_type_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> lldpmed_device_type_array; // Used to select operation mode (connectivity/end-point)
#endif
#endif

} lldp_struc_0_t;

VTSS_PRE_DECLS void lldp_os_set_optional_tlv(lldp_tlv_t tlv, lldp_u8_t enabled, u32 *tlv_enabled, lldp_u8_t port);

#ifndef LOW_BYTE
#define LOW_BYTE(v) ((lldp_u8_t) (v))
#endif
#ifndef HIGH_BYTE
#define HIGH_BYTE(v) ((lldp_u8_t) (((lldp_u16_t) (v)) >> 8))
#endif


//
// Define limits and default (only non-zero) for configuration settings
//


#ifdef VTSS_SW_OPTION_LLDP_MED

#define LLDP_ADMIN_STATE_DEFAULT VTSS_APPL_LLDP_ENABLED_RX_TX // Section 8, TIA1057, Bullet b.1
#else
// For LLDP default enabled is only recommended (for LLDP-MED is "shall") (Section 10.5.1 Bullet 1. in 802.1AB-20005)
// but this were discussed when implemented and decided to have it disabled because we like to have all protocols disabled as default.
#define LLDP_ADMIN_STATE_DEFAULT VTSS_APPL_LLDP_DISABLED
#endif // VTSS_SW_OPTION_LLDP_MED
#endif
