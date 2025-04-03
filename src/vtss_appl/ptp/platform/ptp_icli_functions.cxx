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
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "icli_api.h"
#include "icli_porting_util.h"
#include "ptp.h" // For Trace
#include "ptp_api.h"
#include "ptp_icli_functions.h"
#include "ptp_icli_show_functions.h"
#include "ptp_1pps_sync.h"
#include "ptp_1pps_closed_loop.h"
#include "ptp_1pps_serial.h"
#include "vtss_ptp_types.h"

#if defined(VTSS_SW_OPTION_ZLS30387)
#include "zl_3038x_api_pdv_api.h"
#endif

#include "tod_api.h"
#include "vtss_tod_api.h"
#include "misc_api.h"
#include "vtss_ptp_local_clock.h"
#include "port_api.h"
#include "port_iter.hxx"
#include "ptp_pim_api.h"
#include "vlan_api.h"
#include "main_types.h"
#include "ptp_local_clock.h"

extern void ptp_debug_hybrid_mode_set(bool enable);
/***************************************************************************/
/*  Internal types                                                         */
/***************************************************************************/

/***************************************************************************/
/*  Internal functions                                                     */
/***************************************************************************/


static icli_rc_t ptp_icli_traverse_ports(i32 session_id, int clockinst,
                                    icli_stack_port_range_t *port_type_list_p,
                                    icli_rc_t (*show_function)(i32 session_id, int inst, mesa_port_no_t uport, BOOL first, vtss_ptp_cli_pr *pr))
{
    icli_rc_t rc =  ICLI_RC_OK;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    switch_iter_t   sit;
    port_iter_t     pit;
    BOOL            first = true;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                if (ICLI_RC_OK != show_function(session_id, clockinst, uport, first, icli_session_self_printf)) {
                    rc = ICLI_RC_ERROR;
                }
                first = false;
            }
        }
    } else { //show all port configuraton
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG);
        while (switch_iter_getnext(&sit)) {
            (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (ICLI_RC_OK != show_function(session_id, clockinst, pit.uport, first, icli_session_self_printf)) {
                    rc = ICLI_RC_ERROR;
                }
                first = false;
            }
        }
    }
    return rc;
}


static icli_rc_t ptp_icli_config_traverse_ports(i32 session_id, int clockinst,
                                           icli_stack_port_range_t *port_type_list_p, void *port_cfg,
                                           icli_rc_t (*cfg_function)(i32 session_id, int inst, mesa_port_no_t uport, void *cfg))
{
    icli_rc_t rc =  ICLI_RC_OK;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    switch_iter_t   sit;
    port_iter_t     pit;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                if (ICLI_RC_OK != cfg_function(session_id, clockinst, uport, port_cfg)) {
                    rc = ICLI_RC_ERROR;
                }
            }
        }
    } else { //show all port configuration
        (void)switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG);
        while (switch_iter_getnext(&sit)) {
            (void)port_iter_init(&pit, NULL, sit.isid, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);
            while (port_iter_getnext(&pit)) {
                if (ICLI_RC_OK != cfg_function(session_id, clockinst, pit.uport, port_cfg)) {
                    rc = ICLI_RC_ERROR;
                }
            }
        }
    }
    return rc;
}

static icli_rc_t icli_show_clock_port_state_ds(i32 session_id, int inst, mesa_port_no_t uport, BOOL first, vtss_ptp_cli_pr *pr)
{
    ptp_show_clock_port_state_ds(inst, uport, first, pr);
    return ICLI_RC_OK;
}

static icli_rc_t icli_show_clock_port_statistics(i32 session_id, int inst, mesa_port_no_t uport, BOOL first, vtss_ptp_cli_pr *pr)
{
    ptp_show_clock_port_statistics(inst, uport, first, pr, false);
    return ICLI_RC_OK;
}

static icli_rc_t icli_show_clock_port_ds(i32 session_id, int inst, mesa_port_no_t uport, BOOL first, vtss_ptp_cli_pr *pr)
{
    ptp_show_clock_port_ds(inst, uport, first, pr);
    return ICLI_RC_OK;
}

#if defined (VTSS_SW_OPTION_P802_1_AS)
static icli_rc_t icli_show_clock_port_802_1as_cfg(i32 session_id, int inst, mesa_port_no_t uport, BOOL first, vtss_ptp_cli_pr *pr)
{
    ptp_show_clock_port_802_1as_cfg(inst, uport, first, pr);
    return ICLI_RC_OK;
}

static icli_rc_t icli_show_clock_port_802_1as_status(i32 session_id, int inst, mesa_port_no_t uport, BOOL first, vtss_ptp_cli_pr *pr)
{
    ptp_show_clock_port_802_1as_status(inst, uport, first, pr);
    return ICLI_RC_OK;
}
#endif //defined (VTSS_SW_OPTION_P802_1_AS)

static icli_rc_t icli_show_clock_wireless_ds(i32 session_id, int inst, mesa_port_no_t uport, BOOL first, vtss_ptp_cli_pr *pr)
{
    ptp_show_clock_wireless_ds(inst, uport, first, pr);
    return ICLI_RC_OK;
}

static icli_rc_t icli_show_clock_foreign_master_record_ds(i32 session_id, int inst, mesa_port_no_t uport, BOOL first, vtss_ptp_cli_pr *pr)
{
    ptp_show_clock_foreign_master_record_ds(inst, uport, first, pr);
    return ICLI_RC_OK;
}

static icli_rc_t icli_wireless_mode_enable(i32 session_id, int inst, mesa_port_no_t uport, BOOL first, vtss_ptp_cli_pr *pr)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (!ptp_port_wireless_delay_mode_set(true, uport, inst)) {
        ICLI_PRINTF("Wireless mode not available for ptp instance %d, port %u\n", inst, uport);
        ICLI_PRINTF("Wireless mode requires a two-step or Oam based BC\n");
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

static icli_rc_t icli_wireless_mode_disable(i32 session_id, int inst, mesa_port_no_t uport, BOOL first, vtss_ptp_cli_pr *pr)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (!ptp_port_wireless_delay_mode_set(false, uport, inst)) {
        ICLI_PRINTF("Wireless mode not available for ptp instance %d, port %u\n", inst, uport);
        ICLI_PRINTF("Wireless mode requires a two-step or Oam based BC\n");
    }
    return rc;
}

static icli_rc_t icli_wireless_pre_not(i32 session_id, int inst, mesa_port_no_t uport, BOOL first, vtss_ptp_cli_pr *pr)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (!ptp_port_wireless_delay_pre_notif(uport, inst)) {
        ICLI_PRINTF("Wireless mode not available for ptp instance %d, port %u\n", inst, uport);
        ICLI_PRINTF("Wireless mode requires a two-step or Oam based BC\n");
    }
    return rc;
}


/***************************************************************************/
/*  Functions called by iCLI                                               */
/***************************************************************************/

//  see ptp_icli_functions.h
icli_rc_t ptp_icli_show(i32 session_id, int clockinst, BOOL has_default, BOOL has_current, BOOL has_parent, BOOL has_time_property,
                   BOOL has_filter, BOOL has_servo, BOOL has_clk, BOOL has_ho, BOOL has_uni,
                   BOOL has_master_table_unicast, BOOL has_slave, BOOL has_details, BOOL has_port_state, BOOL has_port_statistics, BOOL has_port_ds, BOOL has_wireless,
                   BOOL has_foreign_master_record, BOOL has_interface, icli_stack_port_range_t *port_type_list_p, BOOL has_log_mode)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (has_default) {
        ptp_show_clock_default_ds(clockinst, icli_session_self_printf);
    }
    if (has_current) {
        ptp_show_clock_current_ds(clockinst, icli_session_self_printf);
    }
    if (has_parent) {
        ptp_show_clock_parent_ds(clockinst, icli_session_self_printf);
    }
    if (has_time_property) {
        ptp_show_clock_time_property_ds(clockinst, icli_session_self_printf);
    }
    if (has_filter) {
        ptp_show_clock_filter_ds(clockinst, icli_session_self_printf);
    }
    if (has_servo) {
        ptp_show_clock_servo_ds(clockinst, icli_session_self_printf);
    }
    if (has_clk) {
        ptp_show_clock_clk_ds(clockinst, icli_session_self_printf);
    }
    if (has_ho) {
        ptp_show_clock_ho_ds(clockinst, icli_session_self_printf);
    }
    if (has_uni) {
        ptp_show_clock_uni_ds(clockinst, icli_session_self_printf);
    }
    if (has_master_table_unicast) {
        ptp_show_clock_master_table_unicast_ds(clockinst, icli_session_self_printf);
    }
    if (has_slave) {
        ptp_show_clock_slave_ds(clockinst, icli_session_self_printf, has_details);
    }
    if (has_log_mode) {
        ptp_show_log_mode(clockinst, icli_session_self_printf);
    }
    if (has_port_state) {
        rc = ptp_icli_traverse_ports(session_id, clockinst, port_type_list_p, icli_show_clock_port_state_ds);
        ptp_show_clock_virtual_port_state_ds(clockinst, fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT), true, icli_session_self_printf);
#if defined (VTSS_SW_OPTION_P802_1_AS)
        rc = ptp_icli_traverse_ports(session_id, clockinst, port_type_list_p, icli_show_clock_port_802_1as_status);
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
    }
    if (has_port_statistics) {
        rc = ptp_icli_traverse_ports(session_id, clockinst, port_type_list_p, icli_show_clock_port_statistics);
    }
    if (has_port_ds) {
        rc = ptp_icli_traverse_ports(session_id, clockinst, port_type_list_p, icli_show_clock_port_ds);
#if defined (VTSS_SW_OPTION_P802_1_AS)
        rc = ptp_icli_traverse_ports(session_id, clockinst, port_type_list_p, icli_show_clock_port_802_1as_cfg);
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
    }
    if (has_wireless) {
        rc = ptp_icli_traverse_ports(session_id, clockinst, port_type_list_p, icli_show_clock_wireless_ds);
    }
    if (has_foreign_master_record) {
        rc = ptp_icli_traverse_ports(session_id, clockinst, port_type_list_p, icli_show_clock_foreign_master_record_ds);
    }
    return rc;
}

icli_rc_t ptp_icli_ext_clock_mode_show(i32 session_id)
{
    ptp_show_ext_clock_mode(icli_session_self_printf);
    return ICLI_RC_OK;
}

const char *ptp_1pps_baudrate_to_str(vtss_serial_baud_rate_t b)
{
    switch(b) {
        case VTSS_SERIAL_BAUD_50:     return     "50"; break;
        case VTSS_SERIAL_BAUD_75:     return     "75"; break;
        case VTSS_SERIAL_BAUD_110:    return    "110"; break;
        case VTSS_SERIAL_BAUD_134_5:  return  "134.5"; break;
        case VTSS_SERIAL_BAUD_150:    return    "150"; break;
        case VTSS_SERIAL_BAUD_200:    return    "200"; break;
        case VTSS_SERIAL_BAUD_300:    return    "300"; break;
        case VTSS_SERIAL_BAUD_600:    return    "600"; break;
        case VTSS_SERIAL_BAUD_1200:   return   "1200"; break;
        case VTSS_SERIAL_BAUD_1800:   return   "1800"; break;
        case VTSS_SERIAL_BAUD_2400:   return   "2400"; break;
        case VTSS_SERIAL_BAUD_3600:   return   "3600"; break;
        case VTSS_SERIAL_BAUD_4800:   return   "4800"; break;
        case VTSS_SERIAL_BAUD_7200:   return   "7200"; break;
        case VTSS_SERIAL_BAUD_9600:   return   "9600"; break;
        case VTSS_SERIAL_BAUD_14400:  return  "14400"; break;
        case VTSS_SERIAL_BAUD_19200:  return  "19200"; break;
        case VTSS_SERIAL_BAUD_38400:  return  "38400"; break;
        case VTSS_SERIAL_BAUD_57600:  return  "57600"; break;
        case VTSS_SERIAL_BAUD_115200: return "115200"; break;
        case VTSS_SERIAL_BAUD_230400: return "230400"; break;
        case VTSS_SERIAL_BAUD_460800: return "460800"; break;
        case VTSS_SERIAL_BAUD_921600: return "921600"; break;
        default: return "???";
    }
}

const char *ptp_1pps_stop_bits_to_str(vtss_serial_stop_bits_t s)
{
    switch(s) {
        case VTSS_SERIAL_STOP_1:   return "1";   break;
        case VTSS_SERIAL_STOP_2:   return "2";   break;
        default: return "???";
    }
}

const char *ptp_1pps_parity_to_str(vtss_serial_parity_t p)
{
    switch(p) {
        case VTSS_SERIAL_PARITY_NONE:  return "none";  break;
        case VTSS_SERIAL_PARITY_EVEN:  return "even";  break;
        case VTSS_SERIAL_PARITY_ODD:   return "odd";   break;
        case VTSS_SERIAL_PARITY_MARK:  return "mark";  break;
        case VTSS_SERIAL_PARITY_SPACE: return "space"; break;
        default: return "???";
    }
}

const char *ptp_1pps_wordlength_to_str(vtss_serial_word_length_t w)
{
    switch(w) {
        case VTSS_SERIAL_WORD_LENGTH_5: return "5"; break;
        case VTSS_SERIAL_WORD_LENGTH_6: return "6"; break;
        case VTSS_SERIAL_WORD_LENGTH_7: return "7"; break;
        case VTSS_SERIAL_WORD_LENGTH_8: return "8"; break;
        default: return "???";
    }
}

static const char *filter_type_to_str(u32 p)
{
    switch (p) {
#if defined(VTSS_SW_OPTION_ZLS30387)
        case PTP_FILTERTYPE_ACI_DEFAULT:
            return "aci-default";
        case PTP_FILTERTYPE_ACI_FREQ_XO:
            return "aci-freq-xo";
        case PTP_FILTERTYPE_ACI_PHASE_XO:
            return "aci-phase-xo";
        case PTP_FILTERTYPE_ACI_FREQ_TCXO:
            return "aci-freq-tcxo";
        case PTP_FILTERTYPE_ACI_PHASE_TCXO:
            return "aci-phase-tcxo";
        case PTP_FILTERTYPE_ACI_FREQ_OCXO_S3E:
            return "aci-freq-ocxo-s3e";
        case PTP_FILTERTYPE_ACI_PHASE_OCXO_S3E:
            return "aci-phase-ocxo-s3e";
        case PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_FREQ:
            return "aci-bc-partial-on-path-freq";
        case PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_PHASE:
            return "aci-bc-partial-on-path-phase";
        case PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_FREQ:
            return "aci-bc-full-on-path-freq";
        case PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE:
            return "aci-bc-full-on-path-phase";
        case PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE:
            return "aci-bc-full-on-path-phase-faster-lock-low-pkt-rate";
        case PTP_FILTERTYPE_ACI_FREQ_ACCURACY_FDD:
            return "aci-freq-accuracy-fdd";
        case PTP_FILTERTYPE_ACI_FREQ_ACCURACY_XDSL:
            return "aci-freq-accuracy-xdsl";
        case PTP_FILTERTYPE_ACI_ELEC_FREQ:
            return "aci-elec-freq";
        case PTP_FILTERTYPE_ACI_ELEC_PHASE:
            return "aci-elec-phase";
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C60W:
            return "aci-phase-relaxed-c60w";
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C150:
            return "aci-phase-relaxed-c150";
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C180:
             return "aci-phase-relaxed-c180";
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C240:
             return "aci-phase-relaxed-c240";
        case PTP_FILTERTYPE_ACI_PHASE_OCXO_S3E_R4_6_1:
             return "aci-phase-ocxo-s3e-r4-6-1";
        case PTP_FILTERTYPE_ACI_BASIC_PHASE:
             return "aci-basic-phase";
        case PTP_FILTERTYPE_ACI_BASIC_PHASE_LOW:
             return "aci-basic-phase-low";
#endif
        case PTP_FILTERTYPE_BASIC:
             return "basic";
        default: return "?";
    }
}

icli_rc_t ptp_icli_rs422_clock_mode_show_baudrate(i32 session_id)
{
    static vtss_serial_info_t serial_info;

    if (ptp_1pps_get_baudrate(&serial_info) != VTSS_RC_OK) {
        return ICLI_RC_ERROR;
    }
    
    ICLI_PRINTF("Parameters of RS422 port are: baudrate = %s, parity = %s, wordlength = %s, stopbits = %s, flags = %08x\n",
                ptp_1pps_baudrate_to_str(serial_info.baud),
                ptp_1pps_parity_to_str(serial_info.parity),
                ptp_1pps_wordlength_to_str(serial_info.word_length),
                ptp_1pps_stop_bits_to_str(serial_info.stop),
                serial_info.flags);

    return ICLI_RC_OK;
}

icli_rc_t ptp_icli_cal_t_plane(i32 session_id, u32 iport, BOOL has_ext, BOOL has_int)
{
    icli_rc_t rc = ICLI_RC_OK;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_appl_ptp_clock_config_default_ds_t init_default_ds;
    vtss_appl_ptp_clock_status_default_ds_t init_default_ds_status;

    ICLI_PRINTF("Starting calibration of t-plane on port: %d with loopback type: %s\n", iport2uport(iport), has_int ? "Internal" : "External");

    // Remove any PTP instances already present.
    ICLI_PRINTF("Deleting any existing PTP instances\n");
    for (int clockinst = 0; clockinst < PTP_CLOCK_INSTANCES; clockinst++) {
        if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
            if (vtss_appl_ptp_clock_config_default_ds_del(clockinst) != VTSS_RC_OK) {
                ICLI_PRINTF("Cannot delete clock instance %d (may be that another clock instance depends on it)\n", clockinst);
                ICLI_PRINTF("Please try to delete all PTP instances before attempting a calibration.\n");
                rc = ICLI_RC_ERROR;
            }
        }
    }

    // Reset VLAN configuration so the switch in general is using VLAN 1 for all ports and all ports are configured as
    // access ports. VLAN 2 shall be used for port under calibration.
    if (rc == ICLI_RC_OK) {
        u8 access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];

        ICLI_PRINTF("Resetting VLAN configuration to default\n");
        vtss_init_data_t init_data{.cmd=INIT_CMD_CONF_DEF, .isid=VTSS_ISID_GLOBAL, .flags=0, .resume=0, .warmstart=0, .switch_info={}};
        if (vlan_init(&init_data) != MESA_RC_OK) {
            ICLI_PRINTF("Could not reset VLAN configuration.\n");
            rc = ICLI_RC_ERROR;
        } else {    
            ICLI_PRINTF("Adding VLAN ID 2 to allowed access VLANs\n");
            if (vtss_appl_vlan_access_vids_get(access_vids) != MESA_RC_OK) {  // Get current list of access VLANs
                ICLI_PRINTF("Unable to get current list of access VLANs.\n");
                rc = ICLI_RC_ERROR;
            } else {                                                          // Add VLAN IDs 2 and 3 to allowed access VLANs
                VTSS_BF_SET(access_vids, 2, 1);
                if (vtss_appl_vlan_access_vids_set(access_vids) != MESA_RC_OK) {
                    ICLI_PRINTF("Unable to add VLAN ID = 2  to alllowed VLANs\n");
                    rc = ICLI_RC_ERROR;
                } else {
                    vtss_appl_vlan_port_conf_t conf;
    
                    if (vlan_mgmt_port_conf_get(VTSS_ISID_START, iport, &conf, VTSS_APPL_VLAN_USER_STATIC, FALSE) != VTSS_RC_OK) {
                        ICLI_PRINTF("Unable to get port VLAN configuration for port %d\n", iport2uport(iport));
                        rc = ICLI_RC_ERROR;
                    } else {
                        conf.access_pvid = 2;
                        if (vlan_mgmt_port_conf_set(VTSS_ISID_START, iport, &conf, VTSS_APPL_VLAN_USER_STATIC) != VTSS_RC_OK) {
                            ICLI_PRINTF("Unable to set port VLAN configuration for port %d\n", iport2uport(iport));
                            rc = ICLI_RC_ERROR;
                        }
                    }
                }
            }
        }
    }

    // Setup a new PTP master and slave using the port specified by iport.
    // Note: Since only the Sync packets are needed, the PTP instances shall be configured as one-way.
    if (rc == ICLI_RC_OK) {
        mesa_rc my_rc;

        ICLI_PRINTF("Creating PTP master and slave used for calibration\n");

        ptp_get_default_clock_default_ds(&init_default_ds_status, &init_default_ds);
        ptp_apply_profile_defaults_to_default_ds(&init_default_ds, VTSS_APPL_PTP_PROFILE_NO_PROFILE);

        init_default_ds.deviceType = VTSS_APPL_PTP_DEVICE_MASTER_ONLY;
        init_default_ds.oneWay = true;

        if ((my_rc = ptp_clock_clockidentity_set(0, &init_default_ds_status.clockIdentity)) != VTSS_RC_OK) {
            ICLI_PRINTF("Cannot set clock identity of PTP master: %s\n", error_txt(my_rc));
            rc = ICLI_RC_ERROR;
        } else {
            init_default_ds.configured_vid = 2;
            if ((my_rc = vtss_appl_ptp_clock_config_default_ds_set(0, &init_default_ds)) != VTSS_RC_OK) {
                ICLI_PRINTF("Cannot create PTP master as instance 0: %s\n", error_txt(my_rc));
                rc = ICLI_RC_ERROR;
            } else {
                init_default_ds_status.clockIdentity[7] += 1;
                if ((my_rc = ptp_clock_clockidentity_set(1, &init_default_ds_status.clockIdentity)) != VTSS_RC_OK) {
                    ICLI_PRINTF("Cannot set clock identity of PTP slave: %s\n", error_txt(my_rc));
                    rc = ICLI_RC_ERROR;
                } else {
                    init_default_ds.deviceType = VTSS_APPL_PTP_DEVICE_SLAVE_ONLY;
                    if ((my_rc = vtss_appl_ptp_clock_config_default_ds_set(1, &init_default_ds)) != VTSS_RC_OK) {
                        ICLI_PRINTF("Cannot create PTP slave as instance 1: %s\n", error_txt(my_rc));
                        rc = ICLI_RC_ERROR;
                    }
                }
            }
        }

        // Enable port given by value of iport
        if (rc == ICLI_RC_OK) {
            vtss_ifindex_t ifindex;
            vtss_appl_ptp_config_port_ds_t ds_cfg;

            (void)vtss_ifindex_from_port(0, iport, &ifindex);
            if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_get(0, ifindex, &ds_cfg)) == VTSS_RC_OK) {
                ds_cfg.enabled = true;
                ds_cfg.logSyncInterval = -4;
                if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_set(0, ifindex, &ds_cfg)) != VTSS_RC_OK) {
                    ICLI_PRINTF("Error enabling port %d (%s) on PTP master\n", iport2uport(iport), error_txt(my_rc));
                    rc = ICLI_RC_ERROR;
                } else {
                    if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_get(1, ifindex, &ds_cfg)) == VTSS_RC_OK) {
                        ds_cfg.enabled = true;
                        ds_cfg.logSyncInterval = -4;
                        if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_set(1, ifindex, &ds_cfg)) != VTSS_RC_OK) {
                            ICLI_PRINTF("Error enabling port %d (%s) on PTP slave\n", iport2uport(iport), error_txt(my_rc));
                            rc = ICLI_RC_ERROR;
                        }
                    } else {
                        ICLI_PRINTF("Error getting port data for port %d on PTP slave\n", iport2uport(iport));
                        rc = ICLI_RC_ERROR;
                    }
                }
            } else {
                ICLI_PRINTF("Error getting port data for port %d on PTP master\n", iport2uport(iport));
                rc = ICLI_RC_ERROR;
            }
        }
    }

    // If has_int is true then configure an internal loopback (equipment loopback).    
    if (has_int && rc == ICLI_RC_OK) {
        mesa_port_test_conf_t conf;

        ICLI_PRINTF("Need to wait 5 seconds before setting up internal loopback\n");
        sleep(5);

        if (mesa_port_test_conf_get(NULL, iport, &conf) == MESA_RC_OK) {
            conf.loopback = MESA_PORT_LB_EQUIPMENT;  // MESA_PORT_LB_NEAR_END;
            if (meba_port_test_conf_set(NULL, iport, &conf) != MESA_RC_OK) {
                ICLI_PRINTF("Error enabling port loopback.\n");
                rc = ICLI_RC_ERROR;
            }
        } else {
            ICLI_PRINTF("Error getting configuration for loopback port.\n");
            rc = ICLI_RC_ERROR;
        }
    }

    // If spanning tree is configured, it shall be disabled temporarily for the port that is being calibrated.
    {
    }

    // Set flag that makes vtss_ptp_slave_sync do the calibration
    if (rc == ICLI_RC_OK) {
        calib_initiate = true;
        calib_t_plane_enabled = true;
    
        // Wait for vtss_ptp_slave_sync to clear the flag and report that calibration has been performed
        // If waiting time exceeds 30 seconds (value TBD) stop calibration and report that there is a problem
        // for instance an external loopback may not have been applied correctly. 
        ICLI_PRINTF("Now waiting up tp 30 seconds for calibration to be performed.\n");
        {
            int n = 0;
            while (calib_t_plane_enabled && n < 30) {
                sleep(1);
                n++;
            }
            if (n == 30) {
                calib_t_plane_enabled = false;
                rc = ICLI_RC_ERROR;
            }
        }

        // Remove internal loopback if one was set up
        if (has_int) {
            mesa_port_test_conf_t conf;
    
            if (mesa_port_test_conf_get(NULL, iport, &conf) == MESA_RC_OK) {
                conf.loopback = MESA_PORT_LB_DISABLED;
                if (meba_port_test_conf_set(NULL, iport, &conf) != MESA_RC_OK) {
                    ICLI_PRINTF("Error removing port loopback.\n");
                    rc = ICLI_RC_ERROR;
                }
            } else {
                ICLI_PRINTF("Error getting configuration for loopback port.\n");
                rc = ICLI_RC_ERROR;
            }
        }
    }

    // Report that the calibration has been completed
    if (rc == ICLI_RC_OK) {
        ICLI_PRINTF("Calibration completed.\n");
    } else {
        ICLI_PRINTF("Calibration aborted.\n");
    }

    return rc;
}

icli_rc_t ptp_icli_cal_p2p(i32 session_id, u32 ref_iport, u32 other_iport, i32 cable_latency)
{
    icli_rc_t rc = ICLI_RC_OK;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_appl_ptp_clock_config_default_ds_t init_default_ds;
    vtss_appl_ptp_clock_status_default_ds_t init_default_ds_status;

    ICLI_PRINTF("Starting port to port calibration of port: %d with reference port: %d and cable latency: %d)\n", iport2uport(other_iport), iport2uport(ref_iport), cable_latency);

    // Remove any PTP instances already present.
    ICLI_PRINTF("Deleting any existing PTP instances\n");
    for (int clockinst = 0; clockinst < PTP_CLOCK_INSTANCES; clockinst++) {
        if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
            if (vtss_appl_ptp_clock_config_default_ds_del(clockinst) != VTSS_RC_OK) {
                ICLI_PRINTF("Cannot delete clock instance %d (may be that another clock instance depends on it)\n", clockinst);
                ICLI_PRINTF("Please try to delete all PTP instances before attempting a calibration.\n");
                rc = ICLI_RC_ERROR;
            }
        }
    }

    // Reset VLAN configuration so the switch in general is using VLAN 1 for all ports and all ports are configured as
    // access ports. VLAN 2 shall be used for the master port and VLAN 3 shall be used for the slave port.
    if (rc == ICLI_RC_OK) {
        u8 access_vids[VTSS_APPL_VLAN_BITMASK_LEN_BYTES];

        ICLI_PRINTF("Resetting VLAN configuration to default\n");
        vtss_init_data_t init_data{.cmd=INIT_CMD_CONF_DEF, .isid=VTSS_ISID_GLOBAL, .flags=0, .resume=0, .warmstart=0, .switch_info={}};
        if (vlan_init(&init_data) != MESA_RC_OK) {
            ICLI_PRINTF("Could not reset VLAN configuration.\n");
            rc = ICLI_RC_ERROR;
        } else {
            ICLI_PRINTF("Adding VLAN IDs 2 and 3 to allowed access VLANs\n");
            if (vtss_appl_vlan_access_vids_get(access_vids) != MESA_RC_OK) {  // Get current list of access VLANs
                ICLI_PRINTF("Unable to get current list of access VLANs.\n");
                rc = ICLI_RC_ERROR;
            } else {                                                          // Add VLAN IDs 2 and 3 to allowed access VLANs
                VTSS_BF_SET(access_vids, 2, 1);
                VTSS_BF_SET(access_vids, 3, 1);   
                if (vtss_appl_vlan_access_vids_set(access_vids) != MESA_RC_OK) {
                    ICLI_PRINTF("Unable to add VLAN IDs = 2 and 3 to alllowed VLANs\n");
                    rc = ICLI_RC_ERROR;
                } else {
                    vtss_appl_vlan_port_conf_t conf;
    
                    if (vlan_mgmt_port_conf_get(VTSS_ISID_START, ref_iport, &conf, VTSS_APPL_VLAN_USER_STATIC, FALSE) != VTSS_RC_OK) {
                        ICLI_PRINTF("Unable to get port VLAN configuration for port %d\n", iport2uport(ref_iport));
                        rc = ICLI_RC_ERROR;
                    } else {
                        conf.access_pvid = 2;
                        if (vlan_mgmt_port_conf_set(VTSS_ISID_START, ref_iport, &conf, VTSS_APPL_VLAN_USER_STATIC) != VTSS_RC_OK) {
                            ICLI_PRINTF("Unable to set port VLAN configuration for port %d\n", iport2uport(ref_iport));
                            rc = ICLI_RC_ERROR;
                        }
                    }
    
                    if (vlan_mgmt_port_conf_get(VTSS_ISID_START, other_iport, &conf, VTSS_APPL_VLAN_USER_STATIC, FALSE) != VTSS_RC_OK) {
                        ICLI_PRINTF("Unable to get port VLAN configuration for port %d\n", other_iport);
                        rc = ICLI_RC_ERROR;
                    } else {
                        conf.access_pvid = 3;
                        if (vlan_mgmt_port_conf_set(VTSS_ISID_START, other_iport, &conf, VTSS_APPL_VLAN_USER_STATIC) != VTSS_RC_OK) {
                            ICLI_PRINTF("Unable to set port VLAN configuration for port %d\n", other_iport);
                            rc = ICLI_RC_ERROR;
                        }
                    }
                }
            }
        }
    }

    // Setup a new PTP master and slave using the ports specified by ref_iport and other_iport.
    // While setting up the PTP master and slave use the VLAN IDs configured above.
    // Note: Since only the Sync packets are needed, the PTP instances shall be configured as one-way.
    if (rc == ICLI_RC_OK) {
        mesa_rc my_rc;

        ICLI_PRINTF("Creating PTP master and slave used for calibration\n");

        ptp_get_default_clock_default_ds(&init_default_ds_status, &init_default_ds);
        ptp_apply_profile_defaults_to_default_ds(&init_default_ds, VTSS_APPL_PTP_PROFILE_NO_PROFILE);

        init_default_ds.deviceType = VTSS_APPL_PTP_DEVICE_MASTER_ONLY;
        init_default_ds.oneWay = true;

        if ((my_rc = ptp_clock_clockidentity_set(0, &init_default_ds_status.clockIdentity)) != VTSS_RC_OK) {
            ICLI_PRINTF("Cannot set clock identity of PTP master: %s\n", error_txt(my_rc));
            rc = ICLI_RC_ERROR;
        } else {
            init_default_ds.configured_vid = 2;
            if ((my_rc = vtss_appl_ptp_clock_config_default_ds_set(0, &init_default_ds)) != VTSS_RC_OK) {
                ICLI_PRINTF("Cannot create PTP master as instance 0: %s\n", error_txt(my_rc));
                rc = ICLI_RC_ERROR;
            } else {
                init_default_ds.configured_vid = 3;
                init_default_ds_status.clockIdentity[7] += 1;
                if ((my_rc = ptp_clock_clockidentity_set(1, &init_default_ds_status.clockIdentity)) != VTSS_RC_OK) {
                    ICLI_PRINTF("Cannot set clock identity of PTP slave: %s\n", error_txt(my_rc));
                    rc = ICLI_RC_ERROR;
                } else {
                    init_default_ds.deviceType = VTSS_APPL_PTP_DEVICE_SLAVE_ONLY;
                    if ((my_rc = vtss_appl_ptp_clock_config_default_ds_set(1, &init_default_ds)) != VTSS_RC_OK) {
                        ICLI_PRINTF("Cannot create PTP slave as instance 1: %s\n", error_txt(my_rc));
                        rc = ICLI_RC_ERROR;
                    }
                }
            }
        }

        // Enable master port given by value of ref_iport and slave port given by value of slave_iport
        if (rc == ICLI_RC_OK) {
            vtss_ifindex_t ifindex;
            vtss_appl_ptp_config_port_ds_t ds_cfg;

            (void)vtss_ifindex_from_port(0, ref_iport, &ifindex);
            if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_get(0, ifindex, &ds_cfg)) == VTSS_RC_OK) {
                ds_cfg.enabled = true;
                ds_cfg.logSyncInterval = -4;
                if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_set(0, ifindex, &ds_cfg)) != VTSS_RC_OK) {
                    ICLI_PRINTF("Error enabling port %d (%s) on PTP master\n", iport2uport(ref_iport), error_txt(my_rc));
                    rc = ICLI_RC_ERROR;
                } else {
                    (void)vtss_ifindex_from_port(0, other_iport, &ifindex);
                    if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_get(1, ifindex, &ds_cfg)) == VTSS_RC_OK) {
                        ds_cfg.enabled = true;
                        ds_cfg.logSyncInterval = -4;
                        if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_set(1, ifindex, &ds_cfg)) != VTSS_RC_OK) {
                            ICLI_PRINTF("Error enabling port %d (%s) on PTP slave\n", iport2uport(other_iport), error_txt(my_rc));
                            rc = ICLI_RC_ERROR;
                        }
                    } else {
                        ICLI_PRINTF("Error getting port data for port %d on PTP slave\n", iport2uport(other_iport));
                        rc = ICLI_RC_ERROR;
                    }
                }
            } else {
                ICLI_PRINTF("Error getting port data for port %d on PTP master\n", iport2uport(ref_iport));
                rc = ICLI_RC_ERROR;
            }
        }
    }

    // If spanning tree is configured, it shall be disabled temporarily for the port that is being calibrated.
    {
    }

    // Set flag that makes vtss_ptp_slave_sync do the calibration
    if (rc == ICLI_RC_OK) {
        calib_initiate = true;
        calib_cable_latency = cable_latency;
        calib_p2p_enabled = true;

        // Wait for vtss_ptp_slave_sync to clear the flag and report that calibration has been performed
        // If waiting time exceeds 30 seconds (value TBD) stop calibration and report that there is a problem
        // for instance an external loopback may not have been applied correctly. 
        ICLI_PRINTF("Now waiting up tp 30 seconds for calibration to be performed.\n");
        {
            int n = 0;
            while (calib_p2p_enabled && n < 30) {
                sleep(1);
                n++;
            }
            if (n == 30) {
                calib_p2p_enabled = false;
                rc = ICLI_RC_ERROR;
            }
        }
    }

    // Report that the calibration has been completed
    if (rc == ICLI_RC_OK) {
        ICLI_PRINTF("Calibration completed.\n");
    } else {
        ICLI_PRINTF("Calibration aborted.\n");
    }

    return rc;
}

icli_rc_t ptp_icli_cal_port_start(i32 session_id, u32 iport, BOOL has_synce)
{
    icli_rc_t rc = ICLI_RC_OK;
    vtss_ifindex_t ifindex;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_appl_ptp_clock_config_default_ds_t init_default_ds;
    vtss_appl_ptp_clock_status_default_ds_t init_default_ds_status;

   (void)vtss_ifindex_from_port(0, iport, &ifindex);


    ICLI_PRINTF("Starting calibration of port: %d using external reference.\n", iport2uport(iport));

    // Check that port has link status 'up'. A port without link cannot be calibrated.
    {
        vtss_appl_port_status_t port_status;

        // Determine port mode
        (void)vtss_appl_port_status_get(ifindex, &port_status);

        // Check that port has link status 'up'
        if (!port_status.link) {
            ICLI_PRINTF("Port link status is 'down' - cannot calibrate.\n");
            rc = ICLI_RC_ERROR;
        }
    }

    // Clear SyncE configuration
#if defined(VTSS_SW_OPTION_SYNCE)
    if (rc == ICLI_RC_OK) {
        ICLI_PRINTF("Resetting SyncE configuration\n");
        vtss_appl_synce_reset_configuration_to_default();
    }
#endif

    // If synce parameter is specified then configure SyncE on the port being calibrated
#if defined(VTSS_SW_OPTION_SYNCE)
    if (rc == ICLI_RC_OK) {
        if (has_synce) {
            mesa_rc my_rc;
            vtss_appl_synce_port_config_t port_config;

            ICLI_PRINTF("Enabling SyncE on port to be calibrated\n");

            // Enable SSM for port being calibrated
            if ((my_rc = vtss_appl_synce_port_config_get(ifindex, &port_config)) == VTSS_RC_OK) {
                port_config.ssm_enabled = true;
                if ((my_rc = vtss_appl_synce_port_config_set(ifindex, &port_config)) != VTSS_RC_OK) {
                    ICLI_PRINTF("Error setting SyncE port configuration for port %d\n", iport2uport(iport));
                    rc = ICLI_RC_ERROR;
                }
            } else {
                ICLI_PRINTF("Error getting SyncE port configuration for port %d\n", iport2uport(iport));
                rc = ICLI_RC_ERROR;
            }

            // Set clock source nomination
            if (rc == ICLI_RC_OK) {
                vtss_appl_synce_clock_source_nomination_config_t clock_source_nomination_config;

                if ((my_rc = vtss_appl_synce_clock_source_nomination_config_get(1, &clock_source_nomination_config)) == VTSS_RC_OK) {
                    clock_source_nomination_config.nominated = true;
                    clock_source_nomination_config.network_port = ifindex;
                    clock_source_nomination_config.clk_in_port = 0;
                    clock_source_nomination_config.priority = 0;
                    if ((my_rc = vtss_appl_synce_clock_source_nomination_config_set(1, &clock_source_nomination_config)) != VTSS_RC_OK) {
                        ICLI_PRINTF("Error setting SyncE clock source nomination configuration for source 1\n");
                        rc = ICLI_RC_ERROR;
                    }
                } else {
                    ICLI_PRINTF("Error getting SyncE clock source nomination configuration for source 1\n");
                    rc = ICLI_RC_ERROR;
                }
            }

            // Set clock selection mode
            if (rc == ICLI_RC_OK) {
                vtss_appl_synce_clock_selection_mode_config_t clock_selection_mode_config;

                if ((my_rc = vtss_appl_synce_clock_selection_mode_config_get(&clock_selection_mode_config)) == VTSS_RC_OK) {
                    clock_selection_mode_config.selection_mode = VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL;
                    clock_selection_mode_config.source = 1;
                    clock_selection_mode_config.wtr_time = 0;
                    clock_selection_mode_config.eec_option = VTSS_APPL_SYNCE_EEC_OPTION_1;
                    if ((my_rc = vtss_appl_synce_clock_selection_mode_config_set(&clock_selection_mode_config)) != VTSS_RC_OK) {
                        ICLI_PRINTF("Error setting SyncE clock selection mode configuration\n");
                        rc = ICLI_RC_ERROR;
                    }
                } else {
                    ICLI_PRINTF("Error getting SyncE clock selection mode configuration\n");
                    rc = ICLI_RC_ERROR;
                }
            }
        }
    }
#endif

    // Setup PTP adjustment method
    if (rc == ICLI_RC_OK) {
        vtss_appl_ptp_ext_clock_mode_t ext_clock_mode;

        ext_clock_mode.one_pps_mode = VTSS_APPL_PTP_ONE_PPS_OUTPUT;
        ext_clock_mode.clock_out_enable =  false;
        ext_clock_mode.adj_method = VTSS_APPL_PTP_PREFERRED_ADJ_AUTO;
        ext_clock_mode.freq = 1;
        ext_clock_mode.clk_domain = 0;

        if (vtss_appl_ptp_ext_clock_out_set(&ext_clock_mode) != VTSS_RC_OK) {
            ICLI_PRINTF("Could not set PTP adjustment method.\n");
            rc = ICLI_RC_ERROR;
        }
    }

    // Remove any PTP instances already present.
    if (rc == ICLI_RC_OK) {
        ICLI_PRINTF("Deleting any existing PTP instances\n");
        for (int clockinst = 0; clockinst < PTP_CLOCK_INSTANCES; clockinst++) {
            if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
                if (vtss_appl_ptp_clock_config_default_ds_del(clockinst) != VTSS_RC_OK) {
                    ICLI_PRINTF("Cannot delete clock instance %d (may be that another clock instance depends on it)\n", clockinst);
                    ICLI_PRINTF("Please try to delete all PTP instances before attempting a calibration.\n");
                    rc = ICLI_RC_ERROR;
                }
            }
        }
    }

    // Reset VLAN configuration so the switch in general is using VLAN 1 for all ports and all ports are configured as
    // access ports. VLAN 2 shall be used for the master port and VLAN 3 shall be used for the slave port.
    if (rc == ICLI_RC_OK) {
        ICLI_PRINTF("Resetting VLAN configuration to default\n");
        vtss_init_data_t init_data{.cmd=INIT_CMD_CONF_DEF, .isid=VTSS_ISID_GLOBAL, .flags=0, .resume=0, .warmstart=0, .switch_info={}};
        if (vlan_init(&init_data) != MESA_RC_OK) {
            ICLI_PRINTF("Could not reset VLAN configuration.\n");
            rc = ICLI_RC_ERROR;
        } 
    }

    // Setup a new PTP slave using the port specified by iport.
    if (rc == ICLI_RC_OK) {
        mesa_rc my_rc;

        ICLI_PRINTF("Creating PTP slave used for calibration\n");

        // Create a PTP instance
        ptp_get_default_clock_default_ds(&init_default_ds_status, &init_default_ds);
        ptp_apply_profile_defaults_to_default_ds(&init_default_ds, VTSS_APPL_PTP_PROFILE_NO_PROFILE);
        init_default_ds.deviceType = VTSS_APPL_PTP_DEVICE_SLAVE_ONLY;
        init_default_ds.filter_type = VTSS_APPL_PTP_FILTER_TYPE_BASIC;

        if ((my_rc = ptp_clock_clockidentity_set(0, &init_default_ds_status.clockIdentity)) != VTSS_RC_OK) {
            ICLI_PRINTF("Cannot set clock identity of PTP slave: %s\n", error_txt(my_rc));
            rc = ICLI_RC_ERROR;
        } else {
            if ((my_rc = vtss_appl_ptp_clock_config_default_ds_set(0, &init_default_ds)) != VTSS_RC_OK) {
                ICLI_PRINTF("Cannot create PTP slave as instance 0: %s\n", error_txt(my_rc));
                rc = ICLI_RC_ERROR;
            }
        }

        // Enable slave port given by value of iport
        if (rc == ICLI_RC_OK) {
            vtss_ifindex_t ifindex;
            vtss_appl_ptp_config_port_ds_t ds_cfg;

            (void)vtss_ifindex_from_port(0, iport, &ifindex);
            if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_get(0, ifindex, &ds_cfg)) == VTSS_RC_OK) {
                ds_cfg.enabled = true;
                ds_cfg.logSyncInterval = -3;
                ds_cfg.logMinPdelayReqInterval = -3;
                if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_set(0, ifindex, &ds_cfg)) != VTSS_RC_OK) {
                    ICLI_PRINTF("Error enabling port %d (%s) on PTP slave\n", iport2uport(iport), error_txt(my_rc));
                    rc = ICLI_RC_ERROR;
                }
            } else {
                ICLI_PRINTF("Error getting port data for port %d on PTP master\n", iport2uport(iport));
                rc = ICLI_RC_ERROR;
            }
        }

        // Change filter parameters
        if (rc == ICLI_RC_OK) {
            vtss_appl_ptp_clock_filter_config_t filter_params;

            if ((my_rc = vtss_appl_ptp_clock_filter_parameters_get(0, &filter_params)) == VTSS_RC_OK) {
                filter_params.delay_filter = 1;
                filter_params.period = 1;
                filter_params.dist = 0;

                if ((my_rc = vtss_appl_ptp_clock_filter_parameters_set(0, &filter_params)) != VTSS_RC_OK) {
                    ICLI_PRINTF("Error setting filter parameters for PTP instance 0\n");
                    rc = ICLI_RC_ERROR;
                } 
            } else {
                ICLI_PRINTF("Error getting filter parameters for PTP instance 0\n");
                rc = ICLI_RC_ERROR;
            }
        }

        // Change servo parameters
        if (rc == ICLI_RC_OK) {
            vtss_appl_ptp_clock_servo_config_t servo_params;

            if ((my_rc = vtss_appl_ptp_clock_servo_parameters_get(0, &servo_params)) == VTSS_RC_OK) {
                servo_params.p_reg = true;
                servo_params.i_reg = true;
                servo_params.d_reg = true;
                servo_params.ap = DEFAULT_AP;
                servo_params.ai = DEFAULT_AI;
                servo_params.ad = DEFAULT_AD;
                servo_params.gain = 1;

                if ((my_rc = vtss_appl_ptp_clock_servo_parameters_set(0, &servo_params)) != VTSS_RC_OK) {
                    ICLI_PRINTF("Error setting servo parameters for PTP instance 0\n");
                    rc = ICLI_RC_ERROR;
                } 
            } else {
                ICLI_PRINTF("Error getting servo parameters for PTP instance 0\n");
                rc = ICLI_RC_ERROR;
            }
        }
    }

    if (rc == ICLI_RC_OK) {
        ICLI_PRINTF("------------------------------------------------------------------------------------------\n");
        ICLI_PRINTF("A PTP slave was setup for calibration.\n");
        ICLI_PRINTF("Please wait for servo to lock. Then measure 1PPS difference between PTP master (reference)\n");
        ICLI_PRINTF("and PTP slave (device under calibration) the continue calibration i.e. issue command:\n");
        ICLI_PRINTF("\n");
        ICLI_PRINTF("    ptp cal port <port> offset <-100000-100000> cable-latency <-100000-100000>\n");
        ICLI_PRINTF("\n");
        ICLI_PRINTF("------------------------------------------------------------------------------------------\n");
    }

    return rc;
}

icli_rc_t ptp_icli_cal_port_offset(i32 session_id, u32 iport, i32 pps_offset, i32 cable_latency)
{
    icli_rc_t rc = ICLI_RC_OK;
    ICLI_PRINTF("Performing calibration of port: %d using external reference with measured 1PPS offset: %d and specified cable latency: %d\n", iport2uport(iport), pps_offset, cable_latency);

    // Check that the PTP setup is the same as set up by the "cal port start" command.
    // I.e. that there is only a single PTP slave instance and that it is in two-way mode.
    for (int clockinst = 0; clockinst < PTP_CLOCK_INSTANCES; clockinst++) {
       vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg; 

       if (clockinst == 0) {
           if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) != VTSS_RC_OK) {
               ICLI_PRINTF("Cannot get defaultDS for PTP instance 0\n");
               rc = ICLI_RC_ERROR;
           } else {
               if (default_ds_cfg.deviceType != VTSS_APPL_PTP_DEVICE_SLAVE_ONLY) {
                   ICLI_PRINTF("PTP instance 0 is supposed to be a 'slave only' device.\n");
                   rc = ICLI_RC_ERROR;
               } else if (default_ds_cfg.oneWay) {
                   ICLI_PRINTF("PTP instance 0 is supposed to be configured as 'two-way'\n");
                   rc = ICLI_RC_ERROR;
               } else {
                   port_iter_t pit;
                   vtss_ifindex_t ifindex;
                   uint port_no;
                   vtss_appl_ptp_config_port_ds_t port_cfg;

                   // Check that PTP instance 0 has only a single port enabled and that the port is the one specified by iport
                   port_iter_init_local(&pit);
                   while (port_iter_getnext(&pit)) {
                       (void)vtss_ifindex_from_port(0, pit.iport, &ifindex);
                       (void)ptp_ifindex_to_port(ifindex, &port_no);
       
                       if (vtss_appl_ptp_config_clocks_port_ds_get(clockinst, ifindex, &port_cfg) == VTSS_RC_OK) {
                           if (!port_cfg.enabled & (port_no == iport)) {
                               ICLI_PRINTF("PTP instance 0 is supposed to have port %d enabled (as the only one)\n", iport2uport(iport));
                               rc = ICLI_RC_ERROR;
                           } else if (port_cfg.enabled & (port_no != iport)) {
                               ICLI_PRINTF("PTP instance 0 is not supposed to have other ports enabled than port %d. Port %d is enabled.\n", iport2uport(iport), iport2uport(port_no));
                               rc = ICLI_RC_ERROR;
                           }
                       }
                   }
               }
           }
       } else {
           if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {       
               ICLI_PRINTF("Extra PTP instances found. Only PTP instance 0 is supposed to exist.\n");
               rc = ICLI_RC_ERROR;
           }
       }
    }

    // Check that the servo is locked
    if (rc == ICLI_RC_OK) {
        vtss_appl_ptp_clock_slave_ds_t ss;
    
        if (vtss_appl_ptp_clock_status_slave_ds_get(0, &ss) == VTSS_RC_OK) {
            if (ss.slave_state != VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKED) {
                ICLI_PRINTF("PTP slave not phase locked.\n");
                rc = ICLI_RC_ERROR;
            }
        } else {
            ICLI_PRINTF("Could not get state of PTP slave\n");
            rc = ICLI_RC_ERROR;
        }
    }

    // Check that offset from master is below 5 ns
    if (rc == ICLI_RC_OK) {
        vtss_appl_ptp_clock_current_ds_t status;

        if (vtss_appl_ptp_clock_status_current_ds_get(0, &status) == VTSS_RC_OK) {
            if (llabs(status.offsetFromMaster) > (5LL << 16)) {
                ICLI_PRINTF("Offset from master must be in the range from -5ns to 5ns to perform calibration. Calibration not performed.\n");
                rc = ICLI_RC_ERROR;
            }
        } else {
            ICLI_PRINTF("Could not get current DS of PTP slave\n");
            rc = ICLI_RC_ERROR;
        }
    }

    // Set flag that makes vtss_ptp_slave_sync do the calibration
    if (rc == ICLI_RC_OK) {
        calib_initiate = true;
        calib_cable_latency = cable_latency;
        calib_pps_offset = pps_offset;
        calib_port_enabled = true;

        // Wait for vtss_ptp_slave_sync to clear the flag and report that calibration has been performed
        // If waiting time exceeds 30 seconds (value TBD) stop calibration and report that there is a problem
        // for instance an external loopback may not have been applied correctly. 
        ICLI_PRINTF("Now waiting up tp 30 seconds for calibration to be performed.\n");
        {
            int n = 0;
            while (calib_port_enabled && n < 30) {
                sleep(1);
                n++;
            }
            if (n == 30) {
                calib_port_enabled = false;
                rc = ICLI_RC_ERROR;
            }
        }
    }

    // Report that the calibration has been completed
    if (rc == ICLI_RC_OK) {
        ICLI_PRINTF("Calibration completed.\n");
    } else {
        ICLI_PRINTF("Calibration aborted.\n");
    }

    return rc;
}

icli_rc_t ptp_icli_cal_port_reset(i32 session_id, u32 iport, BOOL has_10m_cu, BOOL has_100m_cu, BOOL has_1g_cu,
                                  BOOL has_1g, BOOL has_2g5, BOOL has_5g, BOOL has_10g, BOOL has_25g_nofec, BOOL has_25g_rsfec, BOOL has_all)
{
    icli_rc_t rc = ICLI_RC_OK;

    auto specified_mode = [has_10m_cu, has_100m_cu, has_1g_cu, has_1g, has_2g5, has_5g, has_10g, has_25g_nofec, has_25g_rsfec, has_all]() {
                              if (has_10m_cu) return "10M_CU";
                              else if (has_100m_cu) return "100M_CU";
                              else if (has_1g_cu) return "1G_CU";
                              else if (has_1g)  return "1G";
                              else if (has_2g5) return "2G5";
                              else if (has_5g)  return "5G";
                              else if (has_10g) return "10G";
                              else if (has_25g_nofec) return "25G";
                              else if (has_25g_rsfec) return "25G RS-FEC";
                              else if (has_all) return "ALL";
                              else              return "?";
                          };

    if (!has_1g && !has_2g5 && !has_10g && !has_25g_nofec && !has_25g_rsfec && !has_all) {
        ICLI_PRINTF("Resetting calibration of the current mode for port: %d.\n", iport2uport(iport));

        // Check that port has link status 'up'. A port without link cannot be calibrated.
        {
            vtss_appl_port_status_t port_status;
            vtss_ifindex_t ifindex;
    
            // Determine port mode
            (void)vtss_ifindex_from_port(0, iport, &ifindex);
            (void)vtss_appl_port_status_get(ifindex, &port_status);
    
            // Check that port has link status 'up'
            if (!port_status.link) {
                ICLI_PRINTF("Port link status is 'down' - cannot determine port mode and hence not reset calibration.\n");
                rc = ICLI_RC_ERROR;
            } else {
                switch (port_status.speed) {
                    case MESA_SPEED_10M:    has_10m_cu  = TRUE; break;
                    case MESA_SPEED_100M:   has_100m_cu = TRUE; break;
                    case MESA_SPEED_1G:     if (port_status.fiber) {
                                                has_1g  = TRUE;
                                                break;
                                            } else {
                                                has_1g_cu  = TRUE;
                                                break;
                                            }
                    case MESA_SPEED_2500M:  has_2g5 = TRUE; break;
                    case MESA_SPEED_5G:     has_5g = TRUE; break;
                    case MESA_SPEED_10G:    has_10g = TRUE; break;
                    case MESA_SPEED_25G:    if (port_status.fec_mode == VTSS_APPL_PORT_FEC_MODE_RS_FEC ||
                                                port_status.fec_mode == VTSS_APPL_PORT_FEC_MODE_AUTO) {
                                                has_25g_rsfec = TRUE;
                                            } else {
                                                has_25g_nofec = TRUE;
                                            }
                                            break;
                    default:;
                }
            }
        }
    } else {
        bool explicit_mode_supported = false;

        if (!has_all) {
            /* Check that explicitly specified port mode is actually supported */
            vtss_ifindex_t          ifindex;
            vtss_appl_port_status_t port_status;
            meba_port_cap_t         caps;

            if (vtss_ifindex_from_port(0, iport, &ifindex) != VTSS_RC_OK) {
                T_W("Could not get ifindex");
            } else {
                if (vtss_appl_port_status_get(ifindex, &port_status) != VTSS_RC_OK) {
                    T_W("Could not get port status");
                } else {
                    caps = port_status.fiber && port_status.sfp_type != VTSS_APPL_PORT_SFP_TYPE_NONE ? port_status.sfp_caps : port_status.static_caps;
                    if ((has_10m_cu  && (caps & (MEBA_PORT_CAP_10M_HDX | MEBA_PORT_CAP_10M_FDX))) ||
                        (has_100m_cu && (caps & (MEBA_PORT_CAP_100M_HDX | MEBA_PORT_CAP_100M_FDX))) ||
                        (has_1g_cu   && (caps & (MEBA_PORT_CAP_1G_FDX)) && (caps & (MEBA_PORT_CAP_COPPER | MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER))) ||
                        (has_1g      && ((caps & (MEBA_PORT_CAP_2_5G_FDX | MEBA_PORT_CAP_10G_FDX | MEBA_PORT_CAP_25G_FDX)) ||
                                        ((caps & MEBA_PORT_CAP_1G_FDX) && (caps & (MEBA_PORT_CAP_SFP_ONLY | MEBA_PORT_CAP_FIBER | MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER))))) ||
                        (has_2g5     && (caps & MEBA_PORT_CAP_2_5G_FDX)) ||
                        (has_5g      && (caps & MEBA_PORT_CAP_5G_FDX)) ||
                        (has_10g     && (caps & MEBA_PORT_CAP_10G_FDX)) ||
                        (has_25g_rsfec && (caps & MEBA_PORT_CAP_25G_FDX)) ||
                        (has_25g_nofec && (caps & MEBA_PORT_CAP_25G_FDX)))
                    {
                        explicit_mode_supported = true;
                    }
                }
            }
        }

        if (has_all || explicit_mode_supported) {
            ICLI_PRINTF("Resetting calibration of %s mode(s) for port: %d.\n", specified_mode(), iport2uport(iport));
        } else {
            ICLI_PRINTF("Port %d does not support %s mode.\n", iport2uport(iport), specified_mode());
            rc = ICLI_RC_ERROR;
        }
    }

    if (rc == ICLI_RC_OK) {
        if (has_10m_cu || has_all) {
            ptp_port_calibration.port_calibrations[iport].port_latencies_10m_cu.ingress_latency = 0;
            ptp_port_calibration.port_calibrations[iport].port_latencies_10m_cu.egress_latency = 0;
        }

        if (has_100m_cu || has_all) {
            ptp_port_calibration.port_calibrations[iport].port_latencies_100m_cu.ingress_latency = 0;
            ptp_port_calibration.port_calibrations[iport].port_latencies_100m_cu.egress_latency = 0;
        }

        if (has_1g_cu || has_all) {
            ptp_port_calibration.port_calibrations[iport].port_latencies_1g_cu.ingress_latency = 0;
            ptp_port_calibration.port_calibrations[iport].port_latencies_1g_cu.egress_latency = 0;
        }

        if (has_1g || has_all) {
            ptp_port_calibration.port_calibrations[iport].port_latencies_1g.ingress_latency = 0;
            ptp_port_calibration.port_calibrations[iport].port_latencies_1g.egress_latency = 0;
        }
    
        if (has_2g5 || has_all) {
            ptp_port_calibration.port_calibrations[iport].port_latencies_2g5.ingress_latency = 0;
            ptp_port_calibration.port_calibrations[iport].port_latencies_2g5.egress_latency = 0;
        }

        if (has_5g || has_all) {
            ptp_port_calibration.port_calibrations[iport].port_latencies_5g.ingress_latency = 0;
            ptp_port_calibration.port_calibrations[iport].port_latencies_5g.egress_latency = 0;
        }
    
        if (has_10g || has_all) {
            ptp_port_calibration.port_calibrations[iport].port_latencies_10g.ingress_latency = 0;
            ptp_port_calibration.port_calibrations[iport].port_latencies_10g.egress_latency = 0;
        }

        if (has_25g_rsfec || has_all) {
            ptp_port_calibration.port_calibrations[iport].port_latencies_25g_rsfec.ingress_latency = 0;
            ptp_port_calibration.port_calibrations[iport].port_latencies_25g_rsfec.egress_latency = 0;
        }

        if (has_25g_nofec || has_all) {
            ptp_port_calibration.port_calibrations[iport].port_latencies_25g_nofec.ingress_latency = 0;
            ptp_port_calibration.port_calibrations[iport].port_latencies_25g_nofec.egress_latency = 0;
        }

        // Write PTP port calibration to file on Linux filesystem
        {
            int ptp_port_calib_file = open(PTP_CALIB_FILE_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
            if (ptp_port_calib_file != -1) {
                ptp_port_calibration.version = PTP_CURRENT_CALIB_FILE_VER;
                u32 crc32 = vtss_crc32((const unsigned char*)&ptp_port_calibration.version, sizeof(u32));
                crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)&ptp_port_calibration.rs422_pps_delay, sizeof(u32));
                crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)&ptp_port_calibration.sma_pps_delay, sizeof(u32));
                crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)ptp_port_calibration.port_calibrations.data(), fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * sizeof(port_calibrations_s));
                ptp_port_calibration.crc32 = crc32;
     
                ssize_t numwritten = write(ptp_port_calib_file, &ptp_port_calibration.version, sizeof(u32));
                numwritten += write(ptp_port_calib_file, &ptp_port_calibration.crc32, sizeof(u32));
                numwritten += write(ptp_port_calib_file, &ptp_port_calibration.rs422_pps_delay, sizeof(u32));
                numwritten += write(ptp_port_calib_file, &ptp_port_calibration.sma_pps_delay, sizeof(u32));
                numwritten += write(ptp_port_calib_file, ptp_port_calibration.port_calibrations.data(), fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * sizeof(port_calibrations_s));
     
                if (numwritten != 4 * sizeof(u32) + fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * sizeof(port_calibrations_s)) {
                    T_W("Problem writing PTP port calibration data file.");
                    rc = ICLI_RC_ERROR;
                } else {
                    // Update ingrees and egress latencies in the hardware
                    {
                        mesa_timeinterval_t egress_latency;
    
                        vtss_1588_egress_latency_get(iport2uport(iport), &egress_latency);
                        vtss_1588_egress_latency_set(iport2uport(iport), egress_latency);
                    }
                    {
                        mesa_timeinterval_t ingress_latency;
    
                        vtss_1588_ingress_latency_get(iport2uport(iport), &ingress_latency);
                        vtss_1588_ingress_latency_set(iport2uport(iport), ingress_latency);
                    }
                }

                if (close(ptp_port_calib_file) == -1) {
                    T_W("Could not close PTP port calibration data file.");
                    rc = ICLI_RC_ERROR;
                }
            } else {
                T_W("Could not create/open PTP port calibration data file");
                rc = ICLI_RC_ERROR;
            }
        }
    }

    return rc;
}


icli_rc_t ptp_icli_cal_1pps(i32 session_id, i32 cable_latency, bool sma_pps, bool has_reset)
{
    icli_rc_t rc = ICLI_RC_OK;
    int instance = 0;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_appl_ptp_clock_config_default_ds_t init_default_ds;
    vtss_appl_ptp_clock_status_default_ds_t init_default_ds_status;
    vtss_ptp_rs422_conf_t mode;
    vtss_appl_ptp_virtual_port_config_t cfg;
    uint32_t pps_delay = 0;

    if (!has_reset) {
        ICLI_PRINTF("Calibration of 1PPS input (cable_latency = %d)\n", cable_latency);

        // Remove any PTP instances already present.
        if (rc == ICLI_RC_OK) {
            ICLI_PRINTF("Deleting any existing PTP instances\n");
            for (int clockinst = 0; clockinst < PTP_CLOCK_INSTANCES; clockinst++) {
                if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
                    if (vtss_appl_ptp_clock_config_default_ds_del(clockinst) != VTSS_RC_OK) {
                        ICLI_PRINTF("Cannot delete clock instance %d (may be that another clock instance depends on it)\n", clockinst);
                        ICLI_PRINTF("Please try to delete all PTP instances before attempting a calibration.\n");
                        rc = ICLI_RC_ERROR;
                    }
                }
            }
        }
        if (rc == ICLI_RC_OK) {
            mesa_rc my_rc;

            ICLI_PRINTF("Creating PTP clock used for calibration\n");

            // Create a PTP instance
            ptp_get_default_clock_default_ds(&init_default_ds_status, &init_default_ds);
            ptp_apply_profile_defaults_to_default_ds(&init_default_ds, VTSS_APPL_PTP_PROFILE_NO_PROFILE);
            init_default_ds.deviceType = VTSS_APPL_PTP_DEVICE_MASTER_ONLY;
            init_default_ds.filter_type = VTSS_APPL_PTP_FILTER_TYPE_BASIC;

            if ((my_rc = ptp_clock_clockidentity_set(instance, &init_default_ds_status.clockIdentity)) != VTSS_RC_OK) {
                ICLI_PRINTF("Cannot set clock identity of PTP clock: %s\n", error_txt(my_rc));
                rc = ICLI_RC_ERROR;
            } else {
                if ((my_rc = vtss_appl_ptp_clock_config_default_ds_set(instance, &init_default_ds)) != VTSS_RC_OK) {
                    ICLI_PRINTF("Cannot create PTP clock instance 0: %s\n", error_txt(my_rc));
                    rc = ICLI_RC_ERROR;
                }
            }
        }
        if (sma_pps) {
            if (ptp_1pps_sma_calibrate_virtual_port(instance, true) != VTSS_RC_OK) {
                return ICLI_RC_ERROR;
            }
            ICLI_PRINTF("Now waiting up to 20 seconds for measurement to be performed.\n");
            int n = 0;
            while (n < 20) {
                sleep(1);
                n++;
            }
            pps_delay = ptp_1pps_calibrated_delay_get();
            if (pps_delay == 0) {
                ICLI_PRINTF("could not measure 1pps delay through SMA connectors\n");
                rc = ICLI_RC_ERROR;
            } else {
                pps_delay = pps_delay - cable_latency;
                ICLI_PRINTF("Measured 1pps one-way delay through SMA connectors : %d\n\n", pps_delay);
                if (ptp_1pps_sma_calibrate_virtual_port(instance, false) != VTSS_RC_OK) {
                    return ICLI_RC_ERROR;
                }
            }
        } else {
            // Enable virtual port for calibration.
            if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &cfg) == MESA_RC_OK) {
                cfg.virtual_port_mode = VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO;
                cfg.delay = 0;
                cfg.enable = true;
                (void)vtss_appl_ptp_clock_config_virtual_port_config_set(instance, &cfg);
            } else {
                rc = ICLI_RC_ERROR;
            }

            ICLI_PRINTF("Now waiting up to 20 seconds for calibration to be performed.\n");
            {
                int n = 0;
                while (n < 20) {
                    sleep(1);
                    n++;
                }
                vtss_ext_clock_rs422_conf_get(&mode);
                if (mode.delay == 0) {
                    ICLI_PRINTF("could not measure 1pps signal through RS-422 interface\n");
                    rc = ICLI_RC_ERROR;
                } else {
                    mode.delay = mode.delay - PTP_VIRTUAL_PORT_SUB_DELAY; //SUB delay is added in auto_handler for sending it to SUB.
                    pps_delay = mode.delay - cable_latency;
                    ICLI_PRINTF("Measured 1PPS through RS-422 interface system delay is %d ns\n\n", pps_delay);
                }
            }

            // Disable virtual port
            if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &cfg) == MESA_RC_OK) {
                cfg.virtual_port_mode = VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE;
                cfg.delay = 0;
                cfg.enable = false;
                (void)vtss_appl_ptp_clock_config_virtual_port_config_set(instance, &cfg);
            } else {
                rc = ICLI_RC_ERROR;
            }
        }

        // Delete PTP instance created in this function
        if (vtss_appl_ptp_clock_config_default_ds_del(instance) != VTSS_RC_OK) {
            ICLI_PRINTF("Cannot delete clock instance %d (may be that another clock instance depends on it)\n", instance);
            rc = ICLI_RC_ERROR;
        }
    }

    if (rc != ICLI_RC_ERROR) {
        int ptp_port_calib_file = open(PTP_CALIB_FILE_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
        if (ptp_port_calib_file != -1) {
            ptp_port_calibration.version = PTP_CURRENT_CALIB_FILE_VER;
            if (sma_pps) {
                ptp_port_calibration.sma_pps_delay = pps_delay;
            } else {
                ptp_port_calibration.rs422_pps_delay = pps_delay;
            }
            u32 crc32 = vtss_crc32((const unsigned char*)&ptp_port_calibration.version, sizeof(u32));
            crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)&ptp_port_calibration.rs422_pps_delay, sizeof(u32));
            crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)&ptp_port_calibration.sma_pps_delay, sizeof(u32));
            crc32 = vtss_crc32_accumulate(crc32, (const unsigned char*)ptp_port_calibration.port_calibrations.data(), fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * sizeof(port_calibrations_s));
            ptp_port_calibration.crc32 = crc32;

            ssize_t numwritten = write(ptp_port_calib_file, &ptp_port_calibration.version, sizeof(u32));
            numwritten += write(ptp_port_calib_file, &ptp_port_calibration.crc32, sizeof(u32));
            numwritten += write(ptp_port_calib_file, &ptp_port_calibration.rs422_pps_delay, sizeof(u32));
            numwritten += write(ptp_port_calib_file, &ptp_port_calibration.sma_pps_delay, sizeof(u32));
            numwritten += write(ptp_port_calib_file, ptp_port_calibration.port_calibrations.data(), fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * sizeof(port_calibrations_s));

            if (numwritten != 4 * sizeof(u32) + fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) * sizeof(port_calibrations_s)) {
                T_W("Problem writing PTP port calibration data file.");
                rc = ICLI_RC_ERROR;
            }

            if (close(ptp_port_calib_file) == -1) {
                T_W("Could not close PTP port calibration data file.");
                rc = ICLI_RC_ERROR;
            }
        } else {
            T_W("Could not create/open PTP port calibration data file");
            rc = ICLI_RC_ERROR;
        }
    }
    return rc;
}


icli_rc_t ptp_icli_ptp_cal_show(i32 session_id)
{
    ICLI_PRINTF("PTP Port Calibration\n\n");
    for (int n = 0; n < 9; n++) {
        switch (n) {
            case 0: ICLI_PRINTF("Mode: 10M\n");
                    break;
            case 1: ICLI_PRINTF("Mode: 100M\n");
                    break;
            case 2: ICLI_PRINTF("Mode: 1G (CU)\n");
                    break;                   
            case 3: ICLI_PRINTF("Mode: 1G (SFP)\n");
                    break;
            case 4: ICLI_PRINTF("Mode: 2.5G\n");
                    break;
            case 5: ICLI_PRINTF("Mode: 5G\n");
                    break;
            case 6: ICLI_PRINTF("Mode: 10G\n");
                    break;
            case 7: ICLI_PRINTF("Mode: 25G\n");
                    break;
            case 8: ICLI_PRINTF("Mode: 25G RS-FEC\n");
                    break;
        } 
        ICLI_PRINTF("Port        Ingress latency      Egress latency   \n");
        ICLI_PRINTF("--------------------------------------------------\n");

        for (int q = 0; q < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); q++) {
            vtss_ifindex_t          ifindex;
            vtss_appl_port_status_t port_status;
            meba_port_cap_t         caps;

            if (vtss_ifindex_from_port(0, q, &ifindex) != VTSS_RC_OK) {
                T_W("Could not get ifindex");
            } else {
                if (vtss_appl_port_status_get(ifindex, &port_status) != VTSS_RC_OK) {
                    T_W("Could not get port status");
                } else {
                    caps = port_status.fiber && port_status.sfp_type != VTSS_APPL_PORT_SFP_TYPE_NONE ? port_status.sfp_caps : port_status.static_caps;
                    if ((n == 0 && (caps & (MEBA_PORT_CAP_10M_HDX | MEBA_PORT_CAP_10M_FDX))) ||
                        (n == 1 && (caps & (MEBA_PORT_CAP_100M_HDX | MEBA_PORT_CAP_100M_FDX))) ||
                        (n == 2 && (caps & (MEBA_PORT_CAP_1G_FDX)) && (caps & (MEBA_PORT_CAP_COPPER | MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER))) ||
                        (n == 3 && ((caps & (MEBA_PORT_CAP_2_5G_FDX | MEBA_PORT_CAP_10G_FDX | MEBA_PORT_CAP_25G_FDX)) ||
                                    ((caps & MEBA_PORT_CAP_1G_FDX) && (caps & (MEBA_PORT_CAP_SFP_ONLY | MEBA_PORT_CAP_FIBER | MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER))))) ||
                        (n == 4 && (caps & MEBA_PORT_CAP_2_5G_FDX)) ||
                        (n == 5 && (caps & MEBA_PORT_CAP_5G_FDX)) ||
                        (n == 6 && (caps & MEBA_PORT_CAP_10G_FDX)) ||
                        (n == 7 && (caps & MEBA_PORT_CAP_25G_FDX)) ||
                        (n == 8 && (caps & MEBA_PORT_CAP_25G_FDX)))
                    {
                        char buf[32];          
                    
                        auto mesa_timeinterval_to_str = [&buf](mesa_timeinterval_t t) {
                             snprintf(buf, sizeof(buf), VPRI64Fd("10") "." VPRI64Fd("03"), t >> 16, t >= 0 ? (((t & 0xffff) * 1000) >> 16) : (((-t & 0xffff) * 1000) >> 16));
                             return buf;
                        };
            
                        auto ptp_icli_ptp_cal_show_port = [session_id, mesa_timeinterval_to_str](u32 uport, port_latencies_s port_latencies) {
                            ICLI_PRINTF(" %2d      %s", iport2uport(uport), mesa_timeinterval_to_str(port_latencies.ingress_latency));                           
                            ICLI_PRINTF("      %s\n", mesa_timeinterval_to_str(port_latencies.egress_latency));
                        };
            
                        switch (n) {
                            case 0: ptp_icli_ptp_cal_show_port(q, ptp_port_calibration.port_calibrations[q].port_latencies_10m_cu);  break;
                            case 1: ptp_icli_ptp_cal_show_port(q, ptp_port_calibration.port_calibrations[q].port_latencies_100m_cu); break;
                            case 2: ptp_icli_ptp_cal_show_port(q, ptp_port_calibration.port_calibrations[q].port_latencies_1g_cu); break;
                            case 3: ptp_icli_ptp_cal_show_port(q, ptp_port_calibration.port_calibrations[q].port_latencies_1g);  break;
                            case 4: ptp_icli_ptp_cal_show_port(q, ptp_port_calibration.port_calibrations[q].port_latencies_2g5); break;
                            case 5: ptp_icli_ptp_cal_show_port(q, ptp_port_calibration.port_calibrations[q].port_latencies_5g); break;
                            case 6: ptp_icli_ptp_cal_show_port(q, ptp_port_calibration.port_calibrations[q].port_latencies_10g); break;
                            case 7: ptp_icli_ptp_cal_show_port(q, ptp_port_calibration.port_calibrations[q].port_latencies_25g_nofec); break;
                            case 8: ptp_icli_ptp_cal_show_port(q, ptp_port_calibration.port_calibrations[q].port_latencies_25g_rsfec); break;
                        }
                    }
                }    
            }
        }
        ICLI_PRINTF("\n");
    }
    ICLI_PRINTF("1PPS one-way delay through RS-422 interface: %d \n", ptp_port_calibration.rs422_pps_delay);
    ICLI_PRINTF("1PPS one-way delay through SMA connectors: %d \n\n", ptp_port_calibration.sma_pps_delay);

    return ICLI_RC_OK;
}

icli_rc_t ptp_icli_local_clock_set(i32 session_id, int clockinst, BOOL has_update, BOOL has_ratio, i32 ratio)
{
    mesa_timestamp_t t;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "clockinst %d has_update %d has_ratio %d ratio  %d", clockinst, has_update, has_ratio, ratio);

    if (has_update) {
        /* update the local clock to the system clock */
        t.sec_msb = 0;
        t.seconds = time(NULL);
        t.nanoseconds = 0;
        ptp_local_clock_time_set(&t, ptp_instance_2_timing_domain(clockinst));
    }
    if (has_ratio) {
        /* set the local clock master ratio */
        if (ratio == 0) {
            vtss_local_clock_ratio_clear(clockinst);
        } else {
            vtss_local_clock_ratio_set((ratio<<16)/10, clockinst);
        }
    }
    return ICLI_RC_OK;
}

icli_rc_t ptp_icli_show_filter_type(i32 session_id, int clockinst)
{
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    
    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
        ICLI_PRINTF("Clockinst: %d, filter type: %s\n", clockinst, filter_type_to_str(default_ds_cfg.filter_type));
    } else {
        ICLI_PRINTF("Failed reading filter type for clockinst %d\n", clockinst);
        return ICLI_RC_ERROR;
    }
    return ICLI_RC_OK;
}

icli_rc_t ptp_icli_local_clock_show(i32 session_id, int clockinst)
{
    ptp_show_local_clock(clockinst, icli_session_self_printf);
    return ICLI_RC_OK;
}

icli_rc_t ptp_icli_slave_cfg_set(i32 session_id, int clockinst, BOOL has_stable_offset, u32 stable_offset, BOOL has_offset_ok, u32 offset_ok, BOOL has_offset_fail, u32 offset_fail)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_appl_ptp_clock_slave_config_t slave_cfg;

    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "clockinst %d, has_stable_offset %d, stable_offset %d, has_offset_ok %d, offset_ok %d, has_offset_fail %d, offset_fail %d", clockinst, has_stable_offset, stable_offset, has_offset_ok, offset_ok, has_offset_fail, offset_fail);
    if (vtss_appl_ptp_clock_slave_config_get(clockinst, &slave_cfg) == VTSS_RC_OK) {
        if (has_stable_offset) slave_cfg.stable_offset = stable_offset;
        if (has_offset_ok) slave_cfg.offset_ok = offset_ok;
        if (has_offset_fail) slave_cfg.offset_fail = offset_fail;
        if (vtss_appl_ptp_clock_slave_config_set(clockinst, &slave_cfg) != VTSS_RC_OK) {
            ICLI_PRINTF("Failed setting slave-cfg\n");
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
    }
    return rc;
}

icli_rc_t ptp_icli_slave_cfg_show(i32 session_id, int clockinst)
{
    ptp_show_slave_cfg(clockinst, icli_session_self_printf);
    return ICLI_RC_OK;
}

icli_rc_t ptp_icli_slave_table_unicast_show(i32 session_id, int clockinst)
{
    ptp_show_clock_slave_table_unicast_ds(clockinst, icli_session_self_printf);
    return ICLI_RC_OK;
}

icli_rc_t ptp_icli_deb_send_unicast_cancel(i32 session_id, int clockinst, int slave_idx, BOOL has_ann, BOOL has_sync, BOOL has_del)
{
    u8 msg_type = has_ann ? 0xb : (has_sync ? 0x0 : 0x9);
    return (MESA_RC_OK == ptp_clock_send_unicast_cancel(clockinst, slave_idx, msg_type) ? ICLI_RC_OK : ICLI_RC_ERR_PARAMETER);
}

icli_rc_t ptp_icli_virtual_port_show(i32 session_id, int clockinst)
{
    icli_rc_t rc = ICLI_RC_ERROR;

    if (clockinst < PTP_CLOCK_INSTANCES) {
        vtss_appl_ptp_virtual_port_config_t c;

        if (ptp_clock_config_virtual_port_config_get(clockinst, &c) == VTSS_RC_OK) {

			char clk_id[24]={'\0'};
			sprintf(clk_id,"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
					c.clock_identity[0],c.clock_identity[1],
					c.clock_identity[2],c.clock_identity[3],
					c.clock_identity[4],c.clock_identity[5],
					c.clock_identity[6],c.clock_identity[7]);
                    ICLI_PRINTF("clockinst		:%d\nclass			:%d\naccuracy 		:%s\nvariance 		:%d\nlocalPriority		:%d\npriority1 		:%d\npriority2 		:%d\nio-pin			:%d\nenable			:%s\nsteps-removed		:%d\nclock-identity		:%s\n",
                    clockinst, c.clockQuality.clockClass, ClockAccuracyToString(c.clockQuality.clockAccuracy),
                    c.clockQuality.offsetScaledLogVariance, c.localPriority, c.priority1,
                    c.priority2, c.input_pin, c.enable ? "TRUE" : "FALSE",c.steps_removed,clk_id);

            if (c.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN ||
                c.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO) {
                ICLI_PRINTF("alarm                   :%s\n", c.alarm ? "enabled" : "disabled");
            }
            rc = ICLI_RC_OK;
        }
    } else {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
    }
    return rc;
}

//  see ptp_icli_function
icli_rc_t ptp_icli_wireless_mode_set(i32 session_id, int clockinst, BOOL enable, icli_stack_port_range_t *port_type_list_p)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Wireless mode set clockinst %d, enable %d", clockinst, enable);
    if (enable) {
        rc = ptp_icli_traverse_ports(session_id, clockinst, port_type_list_p, icli_wireless_mode_enable);
    } else {
        rc = ptp_icli_traverse_ports(session_id, clockinst, port_type_list_p, icli_wireless_mode_disable);
    }
    return rc;
}

icli_rc_t ptp_icli_wireless_pre_notification(i32 session_id, int clockinst, icli_stack_port_range_t *port_type_list_p)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Wireless mode pre clockinst %d", clockinst);
    rc = ptp_icli_traverse_ports(session_id, clockinst, port_type_list_p, icli_wireless_pre_not);
    return rc;
}

static icli_rc_t my_port_wireless_delay_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_ptp_delay_cfg_t *my_cfg = (vtss_ptp_delay_cfg_t *)cfg;

    if (!ptp_port_wireless_delay_set(my_cfg, uport, inst)) {
        ICLI_PRINTF("Wireless mode not available for ptp instance %d, port %u\n", inst, uport);
        ICLI_PRINTF("Wireless mode requires a two-step or Oam based BC\n");
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_wireless_delay(i32 session_id, int clockinst, i32 base_delay, i32 incr_delay, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_ptp_delay_cfg_t delay_cfg;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "clockinst %d, base_delay %d, incr_delay %d", clockinst, base_delay, incr_delay);
    delay_cfg.base_delay = ((mesa_timeinterval_t)base_delay)*0x10000/1000;
    delay_cfg.incr_delay = ((mesa_timeinterval_t)incr_delay)*0x10000/1000;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &delay_cfg, my_port_wireless_delay_set);
    return rc;
}

icli_rc_t ptp_icli_mode(i32 session_id, int clockinst,
                        BOOL has_boundary, BOOL has_e2etransparent, BOOL has_p2ptransparent, BOOL has_master, BOOL has_slave, BOOL has_bcfrontend, BOOL has_aedgm, BOOL has_internal,
                        BOOL has_onestep, BOOL has_twostep,
                        BOOL has_ethernet, BOOL has_ethernet_mixed, BOOL has_ip4multi, BOOL has_ip4mixed, BOOL has_ip4unicast, BOOL has_ip6mixed, BOOL has_oam, BOOL has_onepps, BOOL has_any_ptp,
                        BOOL has_oneway, BOOL has_twoway,
                        BOOL has_id, icli_clock_id_t *v_clock_id,
                        BOOL has_vid, u32 vid, u32 prio, BOOL has_mep, u32 mep_id,
                        BOOL has_profile, BOOL has_ieee1588, BOOL has_g8265_1, BOOL has_g8275_1, BOOL has_g8275_2, BOOL has_802_1as,
                        BOOL has_802_1as_aed, BOOL has_clock_domain, u32 clock_domain, BOOL has_dscp, u32 dscp_id)
{
    icli_rc_t rc = ICLI_RC_ERROR;
    vtss_appl_ptp_clock_config_default_ds_t init_default_ds;
    vtss_appl_ptp_clock_status_default_ds_t init_default_ds_status;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_appl_ptp_clock_status_default_ds_t default_ds_status;
    mesa_rc my_rc;
    vtss_appl_ptp_config_port_ds_t port_cfg;
    vtss_appl_ptp_config_port_ds_t new_port_cfg;
    vtss_ifindex_t ifindex;
    port_iter_t pit;
    u32 port_no;
    
    if (!fast_cap(MESA_CAP_TS_MISSING_PTP_ON_INTERNAL_PORTS)) {
        if (has_oam) {
            ICLI_PRINTF("OAM encapsulation is only possible in Serval\n");
            return rc;
        }
    }

    vtss_appl_ptp_profile_t profile = has_profile ? (has_ieee1588 ?  VTSS_APPL_PTP_PROFILE_1588 :
                                      has_g8265_1 ? VTSS_APPL_PTP_PROFILE_G_8265_1 :
                                      has_g8275_1 ? VTSS_APPL_PTP_PROFILE_G_8275_1 :
                                      has_g8275_2 ? VTSS_APPL_PTP_PROFILE_G_8275_2 :
                                      has_802_1as_aed ? VTSS_APPL_PTP_PROFILE_AED_802_1AS :
                                      has_802_1as ? VTSS_APPL_PTP_PROFILE_IEEE_802_1AS : VTSS_APPL_PTP_PROFILE_NO_PROFILE) : VTSS_APPL_PTP_PROFILE_NO_PROFILE;

    ptp_get_default_clock_default_ds(&init_default_ds_status, &init_default_ds);
    ptp_apply_profile_defaults_to_default_ds(&init_default_ds, profile);

    init_default_ds.deviceType = has_boundary ? VTSS_APPL_PTP_DEVICE_ORD_BOUND : has_e2etransparent ? VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT :
                             has_p2ptransparent ? VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT : has_master ? VTSS_APPL_PTP_DEVICE_MASTER_ONLY :
                             has_slave ? VTSS_APPL_PTP_DEVICE_SLAVE_ONLY : has_bcfrontend ? VTSS_APPL_PTP_DEVICE_BC_FRONTEND :
                             has_aedgm ? VTSS_APPL_PTP_DEVICE_AED_GM : has_internal ? VTSS_APPL_PTP_DEVICE_INTERNAL : VTSS_APPL_PTP_DEVICE_NONE;

    if (init_default_ds.deviceType == VTSS_APPL_PTP_DEVICE_INTERNAL) {
        init_default_ds.filter_type = VTSS_APPL_PTP_FILTER_TYPE_BASIC;
    }

    init_default_ds.oneWay = has_oneway ? true : has_twoway ? false : init_default_ds.oneWay;
    init_default_ds.twoStepFlag = has_onestep ? false : has_twostep ? true : init_default_ds.twoStepFlag;

    init_default_ds.protocol = has_ethernet ? VTSS_APPL_PTP_PROTOCOL_ETHERNET : has_ethernet_mixed ? VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED :
                           has_ip4multi ? VTSS_APPL_PTP_PROTOCOL_IP4MULTI : has_ip4mixed ? VTSS_APPL_PTP_PROTOCOL_IP4MIXED :
                           has_ip4unicast ? VTSS_APPL_PTP_PROTOCOL_IP4UNI : has_ip6mixed ? VTSS_APPL_PTP_PROTOCOL_IP6MIXED :
                           has_oam ? VTSS_APPL_PTP_PROTOCOL_OAM : has_onepps ? VTSS_APPL_PTP_PROTOCOL_ONE_PPS : has_any_ptp ? VTSS_APPL_PTP_PROTOCOL_ANY :
                           init_default_ds.protocol;

    /* PTP_ANY encapsulation is allowed only for Transparent clocks */
    if (init_default_ds.protocol == VTSS_APPL_PTP_PROTOCOL_ANY && init_default_ds.deviceType != VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT &&
            init_default_ds.deviceType != VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT) {
        ICLI_PRINTF("Cannot create clock instance : As any_ptp encapsulation is allowed only incase of transparent clocks\n");
        return rc;
    }

    /* Currently, IPv6 can be configured only for one step E2E transparent clock. */
    if ((init_default_ds.protocol == VTSS_APPL_PTP_PROTOCOL_IP6MIXED || init_default_ds.protocol == VTSS_APPL_PTP_PROTOCOL_ANY) &&
       ((init_default_ds.deviceType != VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT) || (init_default_ds.twoStepFlag))) {
        ICLI_PRINTF("Currently, PTP over IPv6 encapsulation is supported for Onestep E2E Transparent clock configuration only\n");
        return rc;
    }
    init_default_ds.configured_vid = has_vid ? vid : init_default_ds.configured_vid;
    init_default_ds.configured_pcp = has_vid ? prio : init_default_ds.configured_pcp;
    init_default_ds.mep_instance = has_mep ? mep_id : init_default_ds.mep_instance;
    init_default_ds.dscp = has_dscp ? dscp_id : init_default_ds.dscp;
    init_default_ds.clock_domain = has_clock_domain ? clock_domain : init_default_ds.clock_domain + clockinst;
    //init_ds.cfg.clock_domain = init_default_ds.clock_domain;
    init_default_ds.profile = profile;
    
    if (has_id) {
        memcpy(init_default_ds_status.clockIdentity, v_clock_id, sizeof(init_default_ds_status.clockIdentity));
    } else {
        init_default_ds_status.clockIdentity[7] += clockinst;
    }

    if ((vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) &&
        (vtss_appl_ptp_clock_status_default_ds_get(clockinst, &default_ds_status) == VTSS_RC_OK))
    {
        // clock instance already exists
        if (default_ds_cfg.deviceType != init_default_ds.deviceType) {
            ICLI_PRINTF("Cannot create clock instance %d. A clock of another type (%s) already exists with that instance number.\n", clockinst, DeviceTypeToString(default_ds_cfg.deviceType));
        } else if (default_ds_cfg.profile != init_default_ds.profile) {
            ICLI_PRINTF("Cannot set profile on clock instance %d. Another profile (%s) already configured with that instance number.\n", clockinst, ClockProfileToString(default_ds_cfg.profile));
        }
        else {
            if (memcmp(init_default_ds_status.clockIdentity, default_ds_status.clockIdentity, 8) == 0) {
                if (vtss_appl_ptp_clock_config_default_ds_set(clockinst, &init_default_ds) != VTSS_RC_OK) {
                    ICLI_PRINTF("There was a problem updating the clock.\n");
                }
                else rc = ICLI_RC_OK;
            }
            else {
                ICLI_PRINTF("Cannot modify the ID of an existing clock - please delete the clock and create it again!\n");
            }
        }
    }
    else {
        // clock instance does not exist: create a new
        //init_ds.cfg = init_default_ds.cfg;
        //memcpy(init_ds.clockIdentity, init_default_ds_status.clockIdentity, sizeof(init_ds.clockIdentity));
        if ((my_rc = ptp_clock_clockidentity_set(clockinst, &init_default_ds_status.clockIdentity)) != VTSS_RC_OK) {
            ICLI_PRINTF("Cannot set clock identity: %s\n", error_txt(my_rc));
        }
        T_IG(VTSS_TRACE_GRP_PTP_ICLI, "clockinst %d, device_type %s", clockinst, DeviceTypeToString(init_default_ds.deviceType));
        if ((my_rc = vtss_appl_ptp_clock_config_default_ds_set(clockinst, &init_default_ds)) != VTSS_RC_OK) {
        //if ((my_rc = ptp_clock_create(&init_ds, clockinst)) != VTSS_RC_OK) {
            ICLI_PRINTF("Cannot create clock: %s\n", error_txt(my_rc));
        }
        else rc = ICLI_RC_OK;
    }
    if (rc == ICLI_RC_OK) {
        /* Apply profile defaults to all ports of this clock (whether enabled or not) */
        port_iter_init_local(&pit);
        while (port_iter_getnext(&pit)) {
            (void)vtss_ifindex_from_port(0, pit.iport, &ifindex);
            (void)ptp_ifindex_to_port(ifindex, &port_no);
    
            if (vtss_appl_ptp_config_clocks_port_ds_get(clockinst, ifindex, &port_cfg) == VTSS_RC_OK) {
                new_port_cfg = port_cfg;
                ptp_apply_profile_defaults_to_port_ds(&new_port_cfg, profile);
    
                if (memcmp(&new_port_cfg, &port_cfg, sizeof(vtss_appl_ptp_config_port_ds_t)) != 0) {
                    if (vtss_appl_ptp_config_clocks_port_ds_set(clockinst, ifindex, &new_port_cfg) == VTSS_RC_ERROR) {
                        T_D("Clock instance %d : does not exist", clockinst);
                    }
                }
            }
        }
    }
    return rc;
}

BOOL ptp_icli_runtime_synce_present(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_SYNCE)
            runtime->present = TRUE;
#else
            runtime->present = FALSE;
#endif
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

//  see ptp_icli_functions.h

BOOL ptp_icli_runtime_zls30380_present(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    vtss_zarlink_servo_t servo_type = (vtss_zarlink_servo_t)fast_cap(VTSS_APPL_CAP_ZARLINK_SERVO_TYPE);
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (servo_type == VTSS_ZARLINK_SERVO_ZLS30380) {
                runtime->present = TRUE;
            } else {
                runtime->present = FALSE;
            }
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

BOOL ptp_icli_runtime_zls30380_or_zls30387_present(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
#if defined(VTSS_SW_OPTION_ZLS30387)
            runtime->present = TRUE;
#else
            runtime->present = FALSE;
#endif // defined(VTSS_SW_OPTION_ZLS30387)
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

BOOL ptp_icli_runtime_telecom_profile_present(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_LAN966X) {
                runtime->present = FALSE;
            } else {
                runtime->present = TRUE;
            }
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

BOOL ptp_icli_runtime_802_1as_present(u32                session_id,
                                        icli_runtime_ask_t ask,
                                        icli_runtime_t     *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
#if defined (VTSS_SW_OPTION_P802_1_AS)
            runtime->present = TRUE;
#else
            runtime->present = FALSE;
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

BOOL ptp_icli_runtime_has_phy_timestamp_capability(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (mepa_phy_ts_cap()) {
                runtime->present = TRUE;
            } else {
                runtime->present = FALSE;
            }
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

BOOL ptp_icli_runtime_has_single_dpll_mode_capability(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MEBA_CAP_SYNCE_DPLL_MODE_SINGLE)) {
                runtime->present = TRUE;
            } else {
                runtime->present = FALSE;
            }
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

BOOL ptp_icli_runtime_has_dual_dpll_mode_capability(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MEBA_CAP_SYNCE_DPLL_MODE_DUAL)) {
                runtime->present = TRUE;
            } else {
                runtime->present = FALSE;
            }
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

BOOL ptp_icli_runtime_has_multiple_clock_domains(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_DOMAIN_CNT) > 1) {
                runtime->present = TRUE;
            } else {
                runtime->present = FALSE;
            }
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

icli_rc_t ptp_icli_no_mode(i32 session_id, int clockinst, BOOL has_boundary, BOOL has_e2etransparent, BOOL has_p2ptransparent, BOOL has_master, BOOL has_slave, BOOL has_bcfrontend, BOOL has_aedgm, BOOL has_internal)
{
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_appl_ptp_device_type_t device_type = has_boundary ? VTSS_APPL_PTP_DEVICE_ORD_BOUND : has_e2etransparent ? VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT :
                                              has_p2ptransparent ? VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT : has_master ? VTSS_APPL_PTP_DEVICE_MASTER_ONLY :
                                              has_slave ? VTSS_APPL_PTP_DEVICE_SLAVE_ONLY : has_bcfrontend ? VTSS_APPL_PTP_DEVICE_BC_FRONTEND :
                                              has_aedgm ? VTSS_APPL_PTP_DEVICE_AED_GM : has_internal ? VTSS_APPL_PTP_DEVICE_INTERNAL : VTSS_APPL_PTP_DEVICE_NONE;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "clockinst %d, device_type %s", clockinst, DeviceTypeToString(device_type));
    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
        if (default_ds_cfg.deviceType == device_type) {
            if (vtss_appl_ptp_clock_config_default_ds_del(clockinst) != VTSS_RC_OK) {
                ICLI_PRINTF("Cannot delete clock instance %d (may be that another clock instance depends on it)\n", clockinst);
                rc = ICLI_RC_ERROR;
            }
        } else {
            ICLI_PRINTF("Cannot delete clock instance %d : it is not a %s clock type\n", clockinst, DeviceTypeToString(device_type));
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_virtual_port_class_set(i32 session_id, int clockinst, u8 ptpclass)
{
    return ptp_set_virtual_port_clock_class(clockinst, ptpclass) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_accuracy_set(i32 session_id, int clockinst, u8 ptpaccuracy)
{
    return ptp_set_virtual_port_clock_accuracy(clockinst, ptpaccuracy) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_variance_set(i32 session_id, int clockinst, u16 ptpvariance)
{
    return ptp_set_virtual_port_clock_variance(clockinst, ptpvariance) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_local_priority_set(i32 session_id, int clockinst, u8 localPriority)
{
    return ptp_set_virtual_port_local_priority(clockinst, localPriority) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_priority1_set(i32 session_id, int clockinst, u8 priority1)
{
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;

    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
        if ((default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_1 ||
             default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_G_8275_2) && (priority1 != 128)) {
            ICLI_PRINTF("priority1 for 8275 profiles can be 128 only\n");
            return ICLI_RC_ERROR;
        }
    }
    return ptp_set_virtual_port_priority1(clockinst, priority1) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_priority2_set(i32 session_id, int clockinst, u8 priority2)
{
    return ptp_set_virtual_port_priority2(clockinst, priority2) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_clock_identity_set(i32 session_id, int clockinst, char *clock_identity, bool enable)
{
    return ptp_set_virtual_port_clock_identity(clockinst, clock_identity,enable) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}
icli_rc_t ptp_icli_virtual_port_steps_removed_set(i32 session_id, int clockinst, uint16_t steps_removed, bool enable)
{
    return ptp_set_virtual_port_steps_removed(clockinst, steps_removed, enable) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}
icli_rc_t ptp_icli_virtual_port_class_clear(i32 session_id, int clockinst)
{
    return ptp_clear_virtual_port_clock_class(clockinst) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_accuracy_clear(i32 session_id, int clockinst)
{
    return ptp_clear_virtual_port_clock_accuracy(clockinst) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_variance_clear(i32 session_id, int clockinst)
{
    return ptp_clear_virtual_port_clock_variance(clockinst) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_local_priority_clear(i32 session_id, int clockinst)
{
    return ptp_clear_virtual_port_local_priority(clockinst) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_priority1_clear(i32 session_id, int clockinst)
{
    return ptp_clear_virtual_port_priority1(clockinst) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_priority2_clear(i32 session_id, int clockinst)
{
    return ptp_clear_virtual_port_priority2(clockinst) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_virtual_port_alarm_conf_set(i32 session_id, int clock_inst, BOOL enable)
{
    return ptp_virtual_port_alarm_set(clock_inst, enable) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_debug_class_set(i32 session_id, int clockinst, u8 ptpclass)
{
    return ptp_set_clock_class(clockinst, ptpclass) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_debug_accuracy_set(i32 session_id, int clockinst, u8 ptpaccuracy)
{
    return ptp_set_clock_accuracy(clockinst, ptpaccuracy) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_debug_variance_set(i32 session_id, int clockinst, u16 ptpvariance)
{
    return ptp_set_clock_variance(clockinst, ptpvariance) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_debug_class_clear(i32 session_id, int clockinst)
{
    return ptp_clear_clock_class(clockinst) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_debug_accuracy_clear(i32 session_id, int clockinst)
{
    return ptp_clear_clock_accuracy(clockinst) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_debug_variance_clear(i32 session_id, int clockinst)
{
    return ptp_clear_clock_variance(clockinst) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
}

//  see ptp_icli_functions.h
icli_rc_t ptp_icli_priority1_set(i32 session_id, int clockinst, u8 priority1)
{
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    icli_rc_t rc =  ICLI_RC_OK;

    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Priority1 set clockinst %d, priority1 %d", clockinst, priority1);
    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
        default_ds_cfg.priority1 = priority1;

        if (vtss_appl_ptp_clock_config_default_ds_set(clockinst, &default_ds_cfg) != VTSS_RC_OK) {
            T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Clock instance %d : does not exist", clockinst);
            rc = ICLI_RC_ERROR;
        }
    }
    return rc;
}

icli_rc_t ptp_icli_priority2_set(i32 session_id, int clockinst, u8 priority2)
{
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    icli_rc_t rc =  ICLI_RC_OK;

    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Priority2 set clockinst %d, priority2 %d", clockinst, priority2);
    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
        default_ds_cfg.priority2 = priority2;

        if (vtss_appl_ptp_clock_config_default_ds_set(clockinst, &default_ds_cfg) != VTSS_RC_OK) {
            T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Clock instance %d : does not exist", clockinst);
            rc = ICLI_RC_ERROR;
        }
    }
    return rc;
}

//  see ptp_icli_functions.h
icli_rc_t ptp_icli_local_priority_set(i32 session_id, int clockinst, u8 localpriority)
{
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    icli_rc_t rc =  ICLI_RC_OK;

    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Local Priority set clockinst %d, priority %d", clockinst, localpriority);
    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
        default_ds_cfg.localPriority = localpriority;

        if (vtss_appl_ptp_clock_config_default_ds_set(clockinst, &default_ds_cfg) != VTSS_RC_OK) {
            T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Clock instance %d : does not exist", clockinst);
            rc = ICLI_RC_ERROR;
        }
    }
    return rc;
}

//  see ptp_icli_functions.h
icli_rc_t ptp_icli_path_trace_set(i32 session_id, int clockinst, BOOL enable)
{
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    icli_rc_t rc =  ICLI_RC_OK;

    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Path trace set clockinst %d, enable %s", clockinst, enable ? "TRUE" : "FALSE");
    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
        default_ds_cfg.path_trace_enable = enable;

        if (vtss_appl_ptp_clock_config_default_ds_set(clockinst, &default_ds_cfg) != VTSS_RC_OK) {
            T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Clock instance %d : does not exist", clockinst);
            rc = ICLI_RC_ERROR;
        }
    }
    return rc;
}

icli_rc_t ptp_icli_domain_set(i32 session_id, int clockinst, u8 domain)
{
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    icli_rc_t rc =  ICLI_RC_OK;

    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Domain set clockinst %d, domain %d", clockinst, domain);
    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &default_ds_cfg) == VTSS_RC_OK) {
        default_ds_cfg.domainNumber = domain;

        if (vtss_appl_ptp_clock_config_default_ds_set(clockinst, &default_ds_cfg) != VTSS_RC_OK) {
            T_IG(VTSS_TRACE_GRP_PTP_ICLI, "Clock instance %d : does not exist", clockinst);
            rc = ICLI_RC_ERROR;
        }
    }
    return rc;
}

icli_rc_t ptp_icli_time_property_set(i32 session_id, int clockinst, BOOL has_utc_offset, i32 utc_offset, BOOL has_valid,
                                     BOOL has_leapminus_59, BOOL has_leapminus_61, BOOL has_time_traceable,
                                     BOOL has_freq_traceable, BOOL has_ptptimescale,
                                     BOOL has_time_source, u8 time_source,
                                     BOOL has_leap_pending, char *date_string, BOOL has_leaptype_59, BOOL has_leaptype_61)
{
    vtss_appl_ptp_clock_timeproperties_ds_t prop;
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "icli timeproperty instance %d", clockinst);
    ptp_clock_default_timeproperties_ds_get(&prop);
    if (has_utc_offset)  prop.currentUtcOffset = utc_offset;
    prop.currentUtcOffsetValid = has_valid;
    prop.leap59 = has_leapminus_59;
    prop.leap61 = has_leapminus_61;
    prop.timeTraceable = has_time_traceable;
    prop.frequencyTraceable = has_freq_traceable;
    prop.ptpTimescale = has_ptptimescale;
    if (has_time_source) prop.timeSource = time_source;
    if (has_leap_pending) {
        prop.pendingLeap = true;
        (void)extract_date(date_string, &prop.leapDate);
        prop.leapType = has_leaptype_59 ? VTSS_APPL_PTP_LEAP_SECOND_59 : VTSS_APPL_PTP_LEAP_SECOND_61;
    }
    if (vtss_appl_ptp_clock_config_timeproperties_ds_set(clockinst, &prop) != VTSS_RC_OK) {
        ICLI_PRINTF("Error writing timing properties instance %d\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_filter_type_set(i32 session_id, int clockinst, BOOL has_aci_default, BOOL has_aci_freq_xo,
                                   BOOL has_aci_phase_xo, BOOL has_aci_freq_tcxo, BOOL has_aci_phase_tcxo,
                                   BOOL has_aci_freq_ocxo_s3e, BOOL has_aci_phase_ocxo_s3e, BOOL has_aci_bc_partial_on_path_freq,
                                   BOOL has_aci_bc_partial_on_path_phase, BOOL has_aci_bc_full_on_path_freq, BOOL has_aci_bc_full_on_path_phase,
                                   BOOL has_aci_bc_full_on_path_phase_faster_lock_low_pkt_rate,
                                   BOOL has_aci_freq_accuracy_fdd, BOOL has_aci_freq_accuracy_xdsl, BOOL has_aci_elec_freq, BOOL has_aci_elec_phase,
                                   BOOL has_aci_phase_relaxed_c60w, BOOL has_aci_phase_relaxed_c150,
                                   BOOL has_aci_phase_relaxed_c180, BOOL has_aci_phase_relaxed_c240,
                                   BOOL has_aci_phase_ocxo_s3e_r4_6_1, BOOL has_aci_basic_phase, BOOL has_aci_basic_phase_low, BOOL has_basic)
{
    vtss_appl_ptp_clock_config_default_ds_t clock_config;
    
    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &clock_config) == VTSS_RC_ERROR) {
        return ICLI_RC_ERROR;
    } else {
        u32 new_filter_type = PTP_FILTERTYPE_ACI_BASIC_PHASE;

        if (has_aci_default)
            new_filter_type = PTP_FILTERTYPE_ACI_DEFAULT;
        else if (has_aci_freq_xo)
            new_filter_type = PTP_FILTERTYPE_ACI_FREQ_XO;
        else if (has_aci_phase_xo)
            new_filter_type = PTP_FILTERTYPE_ACI_PHASE_XO;
        else if (has_aci_freq_tcxo)
            new_filter_type = PTP_FILTERTYPE_ACI_FREQ_TCXO;
        else if (has_aci_phase_tcxo)
            new_filter_type = PTP_FILTERTYPE_ACI_PHASE_TCXO;
        else if (has_aci_freq_ocxo_s3e)
            new_filter_type = PTP_FILTERTYPE_ACI_FREQ_OCXO_S3E;
        else if (has_aci_phase_ocxo_s3e)
            new_filter_type = PTP_FILTERTYPE_ACI_PHASE_OCXO_S3E;
        else if (has_aci_bc_partial_on_path_freq)
            new_filter_type = PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_FREQ;
        else if (has_aci_bc_partial_on_path_phase)
            new_filter_type = PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_PHASE;
        else if (has_aci_bc_full_on_path_freq)
            new_filter_type = PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_FREQ;
        else if (has_aci_bc_full_on_path_phase)
            new_filter_type = PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE;
        else if (has_aci_bc_full_on_path_phase_faster_lock_low_pkt_rate)
            new_filter_type = PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE;
        else if (has_aci_freq_accuracy_fdd)
            new_filter_type = PTP_FILTERTYPE_ACI_FREQ_ACCURACY_FDD;
        else if (has_aci_freq_accuracy_xdsl)
            new_filter_type = PTP_FILTERTYPE_ACI_FREQ_ACCURACY_XDSL;
        else if (has_aci_elec_freq)
            new_filter_type = PTP_FILTERTYPE_ACI_ELEC_FREQ;
        else if (has_aci_elec_phase)
            new_filter_type = PTP_FILTERTYPE_ACI_ELEC_PHASE;
        else if (has_aci_phase_relaxed_c60w)
            new_filter_type = PTP_FILTERTYPE_ACI_PHASE_RELAXED_C60W;
        else if (has_aci_phase_relaxed_c150)
            new_filter_type = PTP_FILTERTYPE_ACI_PHASE_RELAXED_C150;
        else if (has_aci_phase_relaxed_c180)
            new_filter_type = PTP_FILTERTYPE_ACI_PHASE_RELAXED_C180;
        else if (has_aci_phase_relaxed_c240)
            new_filter_type = PTP_FILTERTYPE_ACI_PHASE_RELAXED_C240;
        else if (has_aci_phase_ocxo_s3e_r4_6_1)
            new_filter_type = PTP_FILTERTYPE_ACI_PHASE_OCXO_S3E_R4_6_1;
        else if (has_aci_basic_phase)
            new_filter_type = PTP_FILTERTYPE_ACI_BASIC_PHASE;
        else if (has_aci_basic_phase_low)
            new_filter_type = PTP_FILTERTYPE_ACI_BASIC_PHASE_LOW;
        else if (has_basic)
            new_filter_type = PTP_FILTERTYPE_BASIC;

        if (new_filter_type == clock_config.filter_type) {
            return ICLI_RC_OK;
        }
        if (new_filter_type == PTP_FILTERTYPE_BASIC && ((clock_config.profile != VTSS_APPL_PTP_PROFILE_NO_PROFILE) &&
            (clock_config.profile != VTSS_APPL_PTP_PROFILE_1588) && (clock_config.profile != VTSS_APPL_PTP_PROFILE_IEEE_802_1AS)
            && (clock_config.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS))) {
            ICLI_PRINTF("Basic Servo can be used only for 802.1AS and default profiles \n");
            return ICLI_RC_ERROR;
        }
        clock_config.filter_type = vtss_appl_ptp_filter_type_t(new_filter_type);
        return vtss_appl_ptp_clock_config_default_ds_set(clockinst, &clock_config) == VTSS_RC_OK ? ICLI_RC_OK : ICLI_RC_ERROR;
    }
}

icli_rc_t ptp_icli_servo_clear(i32 session_id, int clockinst)
{
    vtss_appl_ptp_clock_servo_config_t default_servo;
    icli_rc_t rc =  ICLI_RC_OK;

    if (vtss_appl_ptp_clock_servo_parameters_get(clockinst, &default_servo) == VTSS_RC_OK) {
        if (vtss_appl_ptp_clock_servo_clear(clockinst) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error resetting servo, instance %d\n", clockinst);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_servo_displaystate_set(i32 session_id, int clockinst, BOOL enable)
{
    vtss_appl_ptp_clock_servo_config_t default_servo;
    icli_rc_t rc =  ICLI_RC_OK;

    if (vtss_appl_ptp_clock_servo_parameters_get(clockinst, &default_servo) == VTSS_RC_OK) {
        default_servo.display_stats = enable;
        if (vtss_appl_ptp_clock_servo_parameters_set(clockinst, &default_servo) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error writing servo parameters, instance %d\n", clockinst);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_clock_servo_options_set(i32 session_id, int clockinst, BOOL synce, u32 threshold, u32 ap)
{
    vtss_appl_ptp_clock_servo_config_t default_servo;
    icli_rc_t rc =  ICLI_RC_OK;

    if (vtss_appl_ptp_clock_servo_parameters_get(clockinst, &default_servo) == VTSS_RC_OK) {
        default_servo.srv_option = (synce == 1) ? VTSS_APPL_PTP_CLOCK_SYNCE : VTSS_APPL_PTP_CLOCK_FREE;
        if (synce != 0) {  // Only update synce related parameters when servo mode is set to synce (i.e. ignore values when mode is being set to free running).
            default_servo.synce_threshold = threshold;
            default_servo.synce_ap = ap;
        }    
        if (vtss_appl_ptp_clock_servo_parameters_set(clockinst, &default_servo) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error writing servo parameters, instance %d\n", clockinst);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_servo_ap_set(i32 session_id, int clockinst, BOOL enable, u32 ap)
{
    vtss_appl_ptp_clock_servo_config_t default_servo;
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_appl_ptp_clock_config_default_ds_t clock_config;

    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &clock_config) == VTSS_RC_ERROR) {
        return ICLI_RC_ERROR;
    } else if (clock_config.filter_type != VTSS_APPL_PTP_FILTER_TYPE_BASIC) {
        ICLI_PRINTF("Basic Servo parameters can be modified only with basic filter\n");
        return ICLI_RC_ERROR;
    }

    if (vtss_appl_ptp_clock_servo_parameters_get(clockinst, &default_servo) == VTSS_RC_OK) {
        if (enable == 1) default_servo.ap = ap;  // <- Dont change the ap value when ap parameter is being disabled.
        default_servo.p_reg = enable;
        if (vtss_appl_ptp_clock_servo_parameters_set(clockinst, &default_servo) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error writing servo parameters, instance %d\n", clockinst);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_servo_ai_set(i32 session_id, int clockinst, BOOL enable, u32 ai)
{
    vtss_appl_ptp_clock_servo_config_t default_servo;
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_appl_ptp_clock_config_default_ds_t clock_config;

    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &clock_config) == VTSS_RC_ERROR) {
        return ICLI_RC_ERROR;
    } else if (clock_config.filter_type != VTSS_APPL_PTP_FILTER_TYPE_BASIC) {
        ICLI_PRINTF("Basic Servo parameters can be modified only with basic filter\n");
        return ICLI_RC_ERROR;
    }

    if (vtss_appl_ptp_clock_servo_parameters_get(clockinst, &default_servo) == VTSS_RC_OK) {
        if (enable == 1) default_servo.ai = ai;  // <- Dont change the ai value when ai parameter is being disabled.
        default_servo.i_reg = enable;
        if (vtss_appl_ptp_clock_servo_parameters_set(clockinst, &default_servo) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error writing servo parameters, instance %d\n", clockinst);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_servo_ad_set(i32 session_id, int clockinst, BOOL enable, u32 ad)
{
    vtss_appl_ptp_clock_servo_config_t default_servo;
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_appl_ptp_clock_config_default_ds_t clock_config;

    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &clock_config) == VTSS_RC_ERROR) {
        return ICLI_RC_ERROR;
    } else if (clock_config.filter_type != VTSS_APPL_PTP_FILTER_TYPE_BASIC) {
        ICLI_PRINTF("Basic Servo parameters can be modified only with basic filter\n");
        return ICLI_RC_ERROR;
    }

    if (vtss_appl_ptp_clock_servo_parameters_get(clockinst, &default_servo) == VTSS_RC_OK) {
        if (enable == 1) default_servo.ad = ad;  // <- Dont change the ad value when ad parameter is being disabled.
        default_servo.d_reg = enable;
        if (vtss_appl_ptp_clock_servo_parameters_set(clockinst, &default_servo) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error writing servo parameters, instance %d\n", clockinst);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_servo_gain_set(i32 session_id, int clockinst, u32 gain)
{
    vtss_appl_ptp_clock_servo_config_t default_servo;
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_appl_ptp_clock_config_default_ds_t clock_config;

    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &clock_config) == VTSS_RC_ERROR) {
        return ICLI_RC_ERROR;
    } else if (clock_config.filter_type != VTSS_APPL_PTP_FILTER_TYPE_BASIC) {
        ICLI_PRINTF("Basic Servo parameters can be modified only with basic filter\n");
        return ICLI_RC_ERROR;
    }

    if (vtss_appl_ptp_clock_servo_parameters_get(clockinst, &default_servo) == VTSS_RC_OK) {
        default_servo.gain = gain;
        if (vtss_appl_ptp_clock_servo_parameters_set(clockinst, &default_servo) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error writing servo parameters, instance %d\n", clockinst);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_clock_slave_holdover_set(i32 session_id, int clockinst, BOOL has_filter, u32 ho_filter, BOOL has_adj_threshold, u32 adj_threshold)
{
    vtss_appl_ptp_clock_servo_config_t default_servo;
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_appl_ptp_clock_config_default_ds_t clock_config;

    if (vtss_appl_ptp_clock_config_default_ds_get(clockinst, &clock_config) == VTSS_RC_ERROR) {
        return ICLI_RC_ERROR;
    } else if (clock_config.filter_type != VTSS_APPL_PTP_FILTER_TYPE_BASIC) {
        ICLI_PRINTF("Basic Servo parameters can be modified only with basic filter\n");
        return ICLI_RC_ERROR;
    }

    if (vtss_appl_ptp_clock_servo_parameters_get(clockinst, &default_servo) == VTSS_RC_OK) {
        default_servo.ho_filter = has_filter ? ho_filter : default_servo.ho_filter;
        default_servo.stable_adj_threshold = has_adj_threshold ? adj_threshold : default_servo.stable_adj_threshold;

        if (vtss_appl_ptp_clock_servo_parameters_set(clockinst, &default_servo) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error writing servo parameters, instance %d\n", clockinst);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_filter_set(i32 session_id, int clockinst, BOOL has_delay, u32 delay, BOOL has_period, u32 period, BOOL has_dist, u32 dist)
{
    vtss_appl_ptp_clock_filter_config_t filter_params;
    vtss_appl_ptp_clock_config_default_ds_t clock_config;
    icli_rc_t rc =  ICLI_RC_OK;

    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "icli filter instance %d ", clockinst);
    if ((vtss_appl_ptp_clock_config_default_ds_get(clockinst, &clock_config) == VTSS_RC_ERROR) ||
       (clock_config.filter_type != VTSS_APPL_PTP_FILTER_TYPE_BASIC)) {
        ICLI_PRINTF("Basic Servo parameters can be modified only with basic filter\n");
        rc = ICLI_RC_ERROR;
    } else {
        vtss_appl_ptp_filter_default_parameters_get(&filter_params, clock_config.profile);
        if (has_delay) filter_params.delay_filter = delay;
        if (has_period) filter_params.period = period;
        if (has_dist) filter_params.dist = dist;
        if (vtss_appl_ptp_clock_filter_parameters_set(clockinst, &filter_params) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error writing filter parameters, instance %d\n", clockinst);
            rc = ICLI_RC_ERROR;
        }
    }
    return rc;
}

icli_rc_t ptp_icli_clock_unicast_conf_set(i32 session_id, int clockinst, int idx, BOOL has_duration, u32 duration, u32 ip)
{
    vtss_appl_ptp_unicast_slave_config_t uni_slave;
    icli_rc_t rc =  ICLI_RC_OK;

    uni_slave.duration = has_duration ? duration : 100;
    uni_slave.ip_addr = ip;
    if (vtss_appl_ptp_clock_config_unicast_slave_config_set(clockinst, idx, &uni_slave) == VTSS_RC_ERROR) {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_ext_clock_set(i32 session_id, BOOL has_output, BOOL has_ext, u32 clockfreq,
                                 BOOL has_ltc, BOOL has_single, BOOL has_independent, BOOL has_common, BOOL has_auto, u32 clk_domain)
{
    vtss_appl_ptp_ext_clock_mode_t mode;
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_rc my_rc = VTSS_RC_OK;

    /* update the local clock to the system clock */
    mode.one_pps_mode = has_output ? VTSS_APPL_PTP_ONE_PPS_OUTPUT : VTSS_APPL_PTP_ONE_PPS_DISABLE;
    mode.clock_out_enable = has_ext;
    mode.adj_method = has_ltc ? VTSS_APPL_PTP_PREFERRED_ADJ_LTC :
                      has_single ? VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE :
                      has_independent ? VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT :
                      has_common ? VTSS_APPL_PTP_PREFERRED_ADJ_COMMON :
                      VTSS_APPL_PTP_PREFERRED_ADJ_AUTO;
    mode.freq = clockfreq;
    mode.clk_domain = clk_domain;
    if (!clockfreq) {
        mode.freq = 1;
    }
    if (fast_cap(MESA_CAP_TS_PPS_OUT_OVERRULES_CLK_OUT)) {
        if ((mode.one_pps_mode == VTSS_APPL_PTP_ONE_PPS_OUTPUT) && mode.clock_out_enable) {
            ICLI_PRINTF("One_pps_output overrules clock_out_enable, i.e. clock_out_enable is set to false\n");
            mode.clock_out_enable = false;
        }
    }
    if (VTSS_RC_OK != (my_rc = vtss_appl_ptp_ext_clock_out_set(&mode))) {
        ICLI_PRINTF("Could not set ext clock, ptp returned error: %s\n", error_txt(my_rc));
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_preferred_adj_set(i32 session_id, BOOL has_ltc, BOOL has_single, BOOL has_independent, BOOL has_common, BOOL has_auto)
{
    vtss_appl_ptp_ext_clock_mode_t mode;
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_rc my_rc = VTSS_RC_OK;
    if (VTSS_RC_OK == (my_rc = vtss_appl_ptp_ext_clock_out_get(&mode))) {

        mode.adj_method = has_ltc ? VTSS_APPL_PTP_PREFERRED_ADJ_LTC :
                          has_single ? VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE :
                          has_independent ? VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT :
                          has_common ? VTSS_APPL_PTP_PREFERRED_ADJ_COMMON :
                          VTSS_APPL_PTP_PREFERRED_ADJ_AUTO;
        if (VTSS_RC_OK != (my_rc = vtss_appl_ptp_ext_clock_out_set(&mode))) {
            ICLI_PRINTF("Could not set ext clock, ptp returned error: %s\n", error_txt(my_rc));
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Could not get ext clock, ptp returned error: %s\n", error_txt(my_rc));
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_rs422_set_baudrate(i32 session_id, u32 baudrate, BOOL has_parity, BOOL has_none, BOOL has_even, BOOL has_odd, BOOL has_wordlength, u32 wordlength, BOOL has_stopbits, u32 stopbits, BOOL has_flowctrl, BOOL has_noflow, BOOL has_rtscts)
{
    vtss_serial_info_t serial_info;

    switch(baudrate) {
        case  9600: serial_info.baud = VTSS_SERIAL_BAUD_9600;  break;
        case 19200: serial_info.baud = VTSS_SERIAL_BAUD_19200; break;
        case 38400: serial_info.baud = VTSS_SERIAL_BAUD_38400; break;
        default   : serial_info.baud = VTSS_SERIAL_BAUD_115200;
    }
    if (has_parity && has_even) {
        serial_info.parity = VTSS_SERIAL_PARITY_EVEN;
    } else if (has_parity && has_odd)  {
        serial_info.parity = VTSS_SERIAL_PARITY_ODD;
    } else {
        serial_info.parity = VTSS_SERIAL_PARITY_NONE;
    }
    if (has_wordlength) {
        switch(wordlength) {
            case  5: serial_info.word_length = VTSS_SERIAL_WORD_LENGTH_5; break;
            case  6: serial_info.word_length = VTSS_SERIAL_WORD_LENGTH_6; break;
            case  7: serial_info.word_length = VTSS_SERIAL_WORD_LENGTH_7; break;
            default: serial_info.word_length = VTSS_SERIAL_WORD_LENGTH_8;
        }
    } else {
        serial_info.word_length = VTSS_SERIAL_WORD_LENGTH_8;
    }
    if (has_stopbits) {
       switch(stopbits) {
           case  2: serial_info.stop = VTSS_SERIAL_STOP_2;   break;
           default: serial_info.stop = VTSS_SERIAL_STOP_1;
       }
    } else {
        serial_info.stop = VTSS_SERIAL_STOP_1;
    }
    if (has_flowctrl && has_rtscts) {
        serial_info.flags = VTSS_SERIAL_FLOW_RTSCTS_RX | VTSS_SERIAL_FLOW_RTSCTS_TX;
    } else {
        serial_info.flags = 0;
    }
    if (ptp_1pps_set_baudrate(&serial_info) != VTSS_RC_OK) {
        return ICLI_RC_ERROR;
    }

    return ICLI_RC_OK;
}

icli_rc_t ptp_icli_ho_spec_set(i32 session_id, BOOL has_cat1, u32 cat1, BOOL has_cat2, u32 cat2, BOOL has_cat3, u32 cat3)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_appl_ho_spec_conf_t ho_spec;

    vtss_ho_spec_conf_get(&ho_spec);
    /* update the local clock to the system clock */
    ho_spec.cat1 = has_cat1 ? cat1 : ho_spec.cat1;
    ho_spec.cat2 = has_cat2 ? cat2 : ho_spec.cat2;
    ho_spec.cat3 = has_cat3 ? cat3 : ho_spec.cat3;
    vtss_ho_spec_conf_set(&ho_spec);
    return rc;
}

icli_rc_t ptp_icli_debug_log_mode_set(i32 session_id, int clockinst, u32 debug_mode, BOOL has_log_to_file, BOOL has_control, BOOL has_max_time, u32 max_time)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u32 log_time = 4000;
    if (has_max_time) {
        log_time = max_time;
    }
    if (!ptp_debug_mode_set(debug_mode, clockinst, has_log_to_file, has_control, log_time)) {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_log_delete(i32 session_id, int clockinst)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (!ptp_log_delete(clockinst)) {
        ICLI_PRINTF("Could not delete log file for PTP instance %d - Did one exist at all?\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_afi_mode_set(i32 session_id, int clockinst, bool ann, bool enable)
{
    icli_rc_t rc =  ICLI_RC_OK;
    if (VTSS_RC_OK != ptp_afi_mode_set(clockinst, ann, enable)) {
        ICLI_PRINTF("Clock instance %d : does not exist\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}
typedef struct ptp_icli_port_state_s {
    BOOL enable;
    BOOL internal;
} ptp_icli_port_state_t;

static icli_rc_t my_port_state_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_port_state_t  *my_cfg = (ptp_icli_port_state_t *)cfg;
    mesa_rc my_rc;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);
    if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg)) == VTSS_RC_OK) {
        ds_cfg.enabled = my_cfg->enable;
        ds_cfg.portInternal = my_cfg->internal;
        if ((my_rc = vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg)) != VTSS_RC_OK) {
            ICLI_PRINTF("Error enabling or disabling inst %d port %d (%s)\n", inst, uport, error_txt(my_rc));
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error getting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_state_set(i32 session_id, int clockinst, BOOL enable, BOOL has_internal, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_port_state_t port_cfg;
    port_cfg.enable = enable;
    port_cfg.internal = has_internal;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &port_cfg, my_port_state_set);
    return rc;
}

typedef struct ptp_icli_announce_state_s {
    BOOL has_interval;
    i8 interval;
    BOOL has_timeout;
    u8 timeout;
} ptp_icli_announce_state_t;

static icli_rc_t my_port_ann_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_announce_state_t *my_cfg = (ptp_icli_announce_state_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1)) {
        if (my_cfg->has_interval) ds_cfg.logAnnounceInterval = my_cfg->interval;
        if (my_cfg->has_timeout) ds_cfg.announceReceiptTimeout = my_cfg->timeout;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) != VTSS_RC_OK) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_announce_set(i32 session_id, int clockinst, BOOL has_interval, i8 interval, BOOL has_stop, BOOL has_default, BOOL has_timeout, u8 timeout, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_announce_state_t ann_cfg;
    ann_cfg.has_interval = has_interval;
    ann_cfg.interval = interval;
    ann_cfg.has_timeout = has_timeout;
    ann_cfg.timeout = timeout;
    if (has_interval) {
        if (has_stop) {
            ann_cfg.interval = 127;
        } else if (has_default) {
            ann_cfg.interval = 126;
        }
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &ann_cfg, my_port_ann_set);
    return rc;
}

static icli_rc_t my_port_sync_interval_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    i8 *interval = (i8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        ds_cfg.logSyncInterval = *interval;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

static icli_rc_t my_port_mgtSettable_announce_interval_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    i8 *interval = (i8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        if(ds_cfg.c_802_1as.mgtSettableLogAnnounceInterval == *interval) {
            return ICLI_RC_OK;
        }
        ds_cfg.c_802_1as.mgtSettableLogAnnounceInterval = *interval;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

static icli_rc_t my_port_usemgtSettable_announce_interval_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    i8 *interval = (i8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {    if(ds_cfg.c_802_1as.useMgtSettableLogAnnounceInterval == (*interval == 1 ? TRUE:FALSE)){
           return ICLI_RC_OK;
         }
        if(*interval == 1)
            ds_cfg.c_802_1as.useMgtSettableLogAnnounceInterval = TRUE;
        else
            ds_cfg.c_802_1as.useMgtSettableLogAnnounceInterval = FALSE;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
   } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

static icli_rc_t my_port_mgtSettable_sync_interval_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    i8 *interval = (i8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        if(ds_cfg.c_802_1as.mgtSettableLogSyncInterval == *interval) {
            return ICLI_RC_OK;
        }
        ds_cfg.c_802_1as.mgtSettableLogSyncInterval = *interval;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

static icli_rc_t my_port_usemgtSettable_sync_interval_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    i8 *interval = (i8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {   
        if(ds_cfg.c_802_1as.useMgtSettableLogSyncInterval == (*interval == 1 ? TRUE:FALSE)){
           return ICLI_RC_OK;
        }
        if(*interval == 1){
            ds_cfg.c_802_1as.useMgtSettableLogSyncInterval = TRUE;
        } else {
            ds_cfg.c_802_1as.useMgtSettableLogSyncInterval = FALSE;
        }
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

static icli_rc_t my_port_usemgtSettable_pdelayReq_interval_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    i8 *interval = (i8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        if(ds_cfg.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval == (*interval == 1 ? TRUE:FALSE)){
           return ICLI_RC_OK;
        }
        if(*interval == 1)
            ds_cfg.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval = TRUE;
        else
            ds_cfg.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval = FALSE;

        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
   } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}
static icli_rc_t my_port_gptp_mgt_interval_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    i8 *interval = (i8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        if(ds_cfg.c_802_1as.mgtSettableLogGptpCapableMessageInterval == *interval) {
            return ICLI_RC_OK;
        }
        ds_cfg.c_802_1as.mgtSettableLogGptpCapableMessageInterval = *interval == 126 ? ds_cfg.c_802_1as.initialLogGptpCapableMessageInterval : *interval;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}
static icli_rc_t my_port_usemgtSettable_gptp_interval_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    i8 *interval = (i8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        if(ds_cfg.c_802_1as.useMgtSettableLogGptpCapableMessageInterval == (*interval == 1 ? TRUE:FALSE)){
           return ICLI_RC_OK;
        }
        if(*interval == 1)
            ds_cfg.c_802_1as.useMgtSettableLogGptpCapableMessageInterval = TRUE;
        else
            ds_cfg.c_802_1as.useMgtSettableLogGptpCapableMessageInterval = FALSE;

        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
   } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}
icli_rc_t ptp_icli_port_mgtSettable_gptp_interval_set (i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "interval %d, has_stop %d, has_default %d", interval, has_stop, has_default);
    if (has_stop) {
        interval = 127;
    } else if (has_default) {
        interval = 126;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &interval, my_port_gptp_mgt_interval_set);
    return rc;
}
 
icli_rc_t ptp_icli_port_usemgtSettable_gptp_interval_set(i32 session_id, int clockinst, i8 usemgtSettableLogSyncInterval, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "usemgtSettableLogSyncInterval =  %d", usemgtSettableLogSyncInterval);
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &usemgtSettableLogSyncInterval, my_port_usemgtSettable_gptp_interval_set);
    return rc;
}

icli_rc_t ptp_icli_port_sync_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "interval %hhd, has_stop %d, has_default %d", interval, has_stop, has_default);
    if (has_stop) {
        interval = 127;
    } else if (has_default) {
        interval = 126;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &interval, my_port_sync_interval_set);
    return rc;
}

icli_rc_t ptp_icli_port_mgtSettable_sync_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "interval %d, has_stop %d, has_default %d", interval, has_stop, has_default);
    if (has_stop) {
        interval = 127;
    } else if (has_default) {
        interval = 126;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &interval, my_port_mgtSettable_sync_interval_set);
    return rc;
}
 
icli_rc_t ptp_icli_port_usemgtSettable_sync_interval_set(i32 session_id, int clockinst, i8 usemgtSettableLogSyncInterval, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "usemgtSettableLogSyncInterval =  %d", usemgtSettableLogSyncInterval);
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &usemgtSettableLogSyncInterval, my_port_usemgtSettable_sync_interval_set);
    return rc;
}

icli_rc_t ptp_icli_port_usemgtSettable_pdelay_req_interval_set(i32 session_id, int clockinst, i8 usemgtSettableLogPdelayReqInterval, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "usemgtSettableLogPdelayReqInterval =  %d", usemgtSettableLogPdelayReqInterval);
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &usemgtSettableLogPdelayReqInterval, my_port_usemgtSettable_pdelayReq_interval_set);
    return rc;
}


icli_rc_t ptp_icli_port_mgtSettable_announce_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "interval %d, has_stop %d, has_default %d", interval, has_stop, has_default);
    if (has_stop) {
        interval = 127;
    } else if (has_default) {
        interval = 126;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &interval, my_port_mgtSettable_announce_interval_set);
    return rc;
}

icli_rc_t ptp_icli_port_usemgtSettable_announce_interval_set(i32 session_id, int clockinst, i8 usemgtSettableLogAnnounceInterval, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "usemgtSettableLogAnnounceInterval =  %d", usemgtSettableLogAnnounceInterval);
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &usemgtSettableLogAnnounceInterval, my_port_usemgtSettable_announce_interval_set);
    return rc;
}

BOOL ptp_icli_log_sync_interval_check(u32                session_id,
                                      icli_runtime_ask_t ask,
                                      icli_runtime_t     *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            runtime->present = TRUE;
            return TRUE;
        case ICLI_ASK_BYWORD:
            icli_sprintf(runtime->byword, "<Interval : -7-4>");
            return TRUE;
        case ICLI_ASK_RANGE:
            runtime->range.type = ICLI_RANGE_TYPE_SIGNED;
            runtime->range.u.sr.cnt = 1;
            runtime->range.u.sr.range[0].min = -7;
            runtime->range.u.sr.range[0].max = 4;
            return TRUE;
        case ICLI_ASK_HELP:
            icli_sprintf(runtime->help, "logSyncInterval");
            return TRUE;
        default:
            break;
    }
    return FALSE;
}


static icli_rc_t my_port_delay_mechanism_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u8 *dly = (u8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        ds_cfg.delayMechanism = vtss_appl_ptp_delay_mechanism_t(*dly);
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("1 Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("2 Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_delay_mechanism_set(i32 session_id, int clockinst, BOOL has_e2e, BOOL has_p2p, BOOL has_common_p2p, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u8 dly;
    dly = has_e2e ? DELAY_MECH_E2E : (has_p2p ? DELAY_MECH_P2P : DELAY_MECH_COMMON_P2P);
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &dly, my_port_delay_mechanism_set);
    return rc;
}

static icli_rc_t my_port_min_pdelay_interval_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    i8 *interval = (i8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
        (ds_cfg.enabled == 1))
    {
        ds_cfg.logMinPdelayReqInterval = *interval;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

static icli_rc_t  my_port_mgtsettable_pdelay_interval_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{

   icli_rc_t rc =  ICLI_RC_OK;
    i8 *interval = (i8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
        (ds_cfg.enabled == 1))
    {
        if(ds_cfg.c_802_1as.mgtSettableLogSyncInterval == *interval) {
            return ICLI_RC_OK;
        }
        ds_cfg.c_802_1as.peer_d.mgtSettableLogPdelayReqInterval = *interval == 126 ? ds_cfg.c_802_1as.peer_d.initialLogPdelayReqInterval : *interval;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}
icli_rc_t ptp_icli_port_min_pdelay_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "interval %d, has_stop %d, has_default %d", interval, has_stop, has_default);
    if (has_stop) {
        interval = 127;
    } else if (has_default) {
        interval = 126;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &interval, my_port_min_pdelay_interval_set);
    return rc;
}

icli_rc_t ptp_icli_port_mgtsettable_pdelay_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "interval %d, has_stop %d, has_default %d", interval, has_stop, has_default);
    if (has_stop) {
        interval = 127;
    } else if (has_default) {
        interval = 126;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &interval, my_port_mgtsettable_pdelay_interval_set);
    return rc;
}


static icli_rc_t my_port_delay_asymmetry_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_timeinterval_t *asy = (mesa_timeinterval_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
        (ds_cfg.enabled == 1))
    {
        ds_cfg.delayAsymmetry = *asy;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_delay_asymmetry_set(i32 session_id, int clockinst, i32 delay_asymmetry, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_timeinterval_t asy = ((mesa_timeinterval_t)delay_asymmetry)<<16;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &asy, my_port_delay_asymmetry_set);
    return rc;
}

static icli_rc_t my_port_ingress_latency_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_timeinterval_t *igr = (mesa_timeinterval_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
        (ds_cfg.enabled == 1))
    {
        ds_cfg.ingressLatency = *igr;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_ingress_latency_set(i32 session_id, int clockinst, i32 ingress_latency, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_timeinterval_t igr = ((mesa_timeinterval_t)ingress_latency)<<16;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &igr, my_port_ingress_latency_set);
    return rc;
}

static icli_rc_t my_port_egress_latency_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_timeinterval_t *egr = (mesa_timeinterval_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
        (ds_cfg.enabled == 1))
    {
        ds_cfg.egressLatency = *egr;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_egress_latency_set(i32 session_id, int clockinst, i32 egress_latency, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_timeinterval_t egr = ((mesa_timeinterval_t)egress_latency)<<16;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &egr, my_port_egress_latency_set);
    return rc;
}

static icli_rc_t my_port_local_priority_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u8* local_pri = (u8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
            (ds_cfg.enabled == 1))
    {
        ds_cfg.localPriority = *local_pri;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_local_priority_set(i32 session_id, int clockinst, u8 localpriority, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &localpriority, my_port_local_priority_set);
    return rc;
}

static icli_rc_t my_port_master_only_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    bool *master_only = (bool *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
            (ds_cfg.enabled == 1))
    {
        ds_cfg.masterOnly = *master_only;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_master_only_set(i32 session_id, int clockinst, bool masterOnly, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &masterOnly, my_port_master_only_set);
    return rc;
}

typedef struct ptp_icli_destaddr_type_s {
    bool has_default;
    bool has_link_local;
} ptp_icli_destaddr_type_t;

typedef struct ptp_icli_override_type_s {
    bool set_value;
    bool value;
} ptp_icli_override_type_t;

static icli_rc_t my_port_mcast_dest_adr_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_destaddr_type_t* p = (ptp_icli_destaddr_type_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
            (ds_cfg.enabled == 1))
    {
        ds_cfg.dest_adr_type = p->has_default ? VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT :
                              (p->has_link_local ? VTSS_APPL_PTP_PROTOCOL_SELECT_LINK_LOCAL : ds_cfg.dest_adr_type);
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

static icli_rc_t my_port_two_step_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_override_type_t *p = (ptp_icli_override_type_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
            (ds_cfg.enabled == 1))
    {
        if (p->set_value) {
            ds_cfg.twoStepOverride = p->value ? VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE : VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE;
        }
        else {
            ds_cfg.twoStepOverride = VTSS_APPL_PTP_TWO_STEP_OVERRIDE_NONE;
        }
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_mcast_dest_adr_set(i32 session_id, int clockinst, bool has_default, bool has_link_local, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_destaddr_type_t p;
    p.has_default = has_default;
    p.has_link_local = has_link_local;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &p, my_port_mcast_dest_adr_set);
    return rc;
}

icli_rc_t ptp_icli_port_two_step_set(i32 session_id, int clockinst, bool set_value, bool value, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_override_type_t p;
    p.set_value = set_value;
    p.value = value;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &p, my_port_two_step_set);
    return rc;
}

static icli_rc_t my_port_as2020_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_bool_t *as2020 = (mesa_bool_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);
    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        ds_cfg.c_802_1as.as2020 = *as2020;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_as2020_set(i32 session_id, int clockinst, icli_stack_port_range_t *port_list, BOOL as2020)
{
    icli_rc_t rc =  ICLI_RC_OK;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, port_list, &as2020, my_port_as2020_set);
    return rc;
}

static icli_rc_t my_aed_port_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_appl_ptp_802_1as_aed_port_state_t *aedMaster = (vtss_appl_ptp_802_1as_aed_port_state_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);
    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        ds_cfg.aedPortState = *aedMaster;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_aed_port_set(i32 session_id, int clockinst, icli_stack_port_range_t *port_list, uint aedMaster)
{
   icli_rc_t rc =  ICLI_RC_OK;
   rc = ptp_icli_config_traverse_ports(session_id, clockinst, port_list, &aedMaster, my_aed_port_set);
   return rc;
}

typedef struct ptp_icli_aed_intervals_s {
    BOOL has_oper_pdelay;
    i8 oper_pdelay_val;
    BOOL has_init_sync;
    u8 init_sync_val;
    BOOL has_oper_sync;
    u8 oper_sync_val;
} ptp_icli_aed_intervals_t;

static icli_rc_t my_aed_intervals_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_aed_intervals_s *aed_cfg = (ptp_icli_aed_intervals_s *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);
    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        if (aed_cfg->has_oper_pdelay) {
            ds_cfg.c_802_1as.peer_d.operLogPdelayReqInterval = aed_cfg->oper_pdelay_val;
        } else if (aed_cfg->has_init_sync) {
            ds_cfg.c_802_1as.initialLogSyncInterval = aed_cfg->init_sync_val;
        } else if (aed_cfg->has_oper_sync) {
            ds_cfg.c_802_1as.operLogSyncInterval = aed_cfg->oper_sync_val;
        }
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_aed_interval_set(i32 session_id, int clockinst, BOOL has_oper_pdelay, i8 oper_pdelay_val, BOOL has_init_sync,
                                    i8 init_sync_val, BOOL has_oper_sync, i8 oper_sync_val, icli_stack_port_range_t *port_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_aed_intervals_s aed_cfg;
    aed_cfg.has_oper_pdelay = has_oper_pdelay;
    aed_cfg.oper_pdelay_val = oper_pdelay_val;
    aed_cfg.has_init_sync = has_init_sync;
    aed_cfg.init_sync_val = init_sync_val;
    aed_cfg.has_oper_sync = has_oper_sync;
    aed_cfg.oper_sync_val = oper_sync_val;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, port_list, &aed_cfg, my_aed_intervals_set);
    return rc;
}

static icli_rc_t my_port_delay_thresh_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_timeinterval_t *thr = (mesa_timeinterval_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
            (ds_cfg.enabled == 1))
    {
        ds_cfg.c_802_1as.peer_d.meanLinkDelayThresh = *thr;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error getting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_delay_thresh_set(i32 session_id, int clockinst, BOOL set, u32 delay_thresh, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_timeinterval_t thr;
    if (set) {
        thr = ((mesa_timeinterval_t)delay_thresh)<<16;
    } else {
        thr = VTSS_MAX_TIMEINTERVAL;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &thr, my_port_delay_thresh_set);
    return rc;
}

static icli_rc_t my_port_sync_rx_to_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u8 *to = (u8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
            (ds_cfg.enabled == 1))
    {
        ds_cfg.c_802_1as.syncReceiptTimeout = *to;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error getting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_sync_rx_to_set(i32 session_id, int clockinst, BOOL set, u8 sync_rx_to, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u8 to;
    if (set) {
        to = sync_rx_to;
    } else {
        to = 3;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &to, my_port_sync_rx_to_set);
    return rc;
}

static icli_rc_t my_port_statistics_clear(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    BOOL * clear = (BOOL*)cfg;
    ptp_show_clock_port_statistics(inst, uport, false, icli_session_self_printf, *clear);
    return rc;
}

icli_rc_t ptp_icli_port_statistics_clear(i32 session_id, int clockinst, BOOL has_clear, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &has_clear, my_port_statistics_clear);
    return rc;
}

static icli_rc_t my_port_allow_lost_resp_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u16 *resp = (u16 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
            (ds_cfg.enabled == 1))
    {
        ds_cfg.c_802_1as.peer_d.allowedLostResponses = *resp;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error getting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_allow_lost_resp_set(i32 session_id, int clockinst, BOOL set, u16 allow_lost_resp, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u16 resp;
    if (set) {
        resp = allow_lost_resp;
    } else {
        resp = 3;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &resp, my_port_allow_lost_resp_set);
    return rc;
}

static icli_rc_t my_port_allowed_faults_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u16 *allowed_faults = (u16 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
            (ds_cfg.enabled == 1))
    {
        ds_cfg.c_802_1as.peer_d.allowedFaults = *allowed_faults;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error getting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;

}

icli_rc_t ptp_icli_port_allow_faults_set(i32 session_id, int clockinst, BOOL set, u16 allowed_faults, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u16 faults;
    if (set) {
        faults = allowed_faults;
    } else {
        faults = DEFAULT_MAX_PDELAY_ALLOWED_FAULTS;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &faults, my_port_allowed_faults_set);
    return rc;
}

typedef struct ptp_icli_comp_nbr_rate_ratio_s {
    mesa_bool_t mgtSettableComputeNeighborRateRatio;
    mesa_bool_t useMgtSettableComputeNeighborRateRatio;
} ptp_icli_comp_nbr_rate_ratio_t;

typedef struct ptp_icli_comp_mean_link_delay_s {
    mesa_bool_t mgtSettableComputeMeanLinkDelay;
    mesa_bool_t useMgtSettableComputeMeanLinkDelay;
} ptp_icli_comp_mean_link_delay_t;

static icli_rc_t my_port_comp_rate_ratio_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_comp_nbr_rate_ratio_t *comp_rr = (ptp_icli_comp_nbr_rate_ratio_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
            (ds_cfg.enabled == 1))
    {
        ds_cfg.c_802_1as.peer_d.mgtSettableComputeNeighborRateRatio = comp_rr->mgtSettableComputeNeighborRateRatio;
        ds_cfg.c_802_1as.peer_d.useMgtSettableComputeNeighborRateRatio = comp_rr->useMgtSettableComputeNeighborRateRatio;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error getting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}
icli_rc_t ptp_icli_port_compute_nbr_rate_ratio_set(i32 session_id, int clockinst, BOOL compNbrRateRatio, BOOL useMgmtNbrRateRatio, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_comp_nbr_rate_ratio_t c_rate_ratio;
    c_rate_ratio.mgtSettableComputeNeighborRateRatio = compNbrRateRatio ? true : false; 
    c_rate_ratio.useMgtSettableComputeNeighborRateRatio = useMgmtNbrRateRatio ? true : false; 
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &c_rate_ratio, my_port_comp_rate_ratio_set);
    return rc;
}
static icli_rc_t my_port_comp_mean_link_delay_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_comp_mean_link_delay_t *comp_mld = (ptp_icli_comp_mean_link_delay_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
            (ds_cfg.enabled == 1))
    {
        ds_cfg.c_802_1as.peer_d.mgtSettableComputeMeanLinkDelay = comp_mld->mgtSettableComputeMeanLinkDelay;
        ds_cfg.c_802_1as.peer_d.useMgtSettableComputeMeanLinkDelay = comp_mld->useMgtSettableComputeMeanLinkDelay;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error getting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}
icli_rc_t ptp_icli_port_compute_mean_link_delay_set(i32 session_id, int clockinst, BOOL compMeanLinkDelay, BOOL useMgmtCompMeanLinkDelay, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    ptp_icli_comp_mean_link_delay_t c_link_delay;
    c_link_delay.mgtSettableComputeMeanLinkDelay = compMeanLinkDelay;
    c_link_delay.useMgtSettableComputeMeanLinkDelay = useMgmtCompMeanLinkDelay;
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &c_link_delay, my_port_comp_mean_link_delay_set);
    return rc;
}

static icli_rc_t my_port_not_master_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_bool_t *not_master = (mesa_bool_t *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        ds_cfg.notMaster = *not_master;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}
icli_rc_t ptp_icli_port_not_master_set(i32 session_id, int clockinst, icli_stack_port_range_t *port_list, BOOL not_master)
{
    icli_rc_t rc =  ICLI_RC_OK;

    rc = ptp_icli_config_traverse_ports(session_id, clockinst, port_list, &not_master, my_port_not_master_set);
    return rc;
}
static icli_rc_t my_port_1pps_mode_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_1pps_sync_conf_t *pps_conf = (vtss_1pps_sync_conf_t *)cfg;
    mesa_rc my_rc;

    if (VTSS_RC_OK != (my_rc = ptp_1pps_sync_mode_set(uport2iport(uport), pps_conf))) {
        ICLI_PRINTF("Error setting 1pps sync for port %d (%s)\n", uport, error_txt(my_rc));
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_1pps_mode_set(i32 session_id, BOOL has_main_auto, BOOL has_main_man, BOOL has_sub, BOOL has_pps_phase, i32 pps_phase,
                                 BOOL has_cable_asy, i32 cable_asy, BOOL has_ser_man, BOOL has_ser_auto, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_1pps_sync_conf_t pps_conf;
    pps_conf.mode = has_main_auto ? VTSS_PTP_1PPS_SYNC_MAIN_AUTO : has_main_man ? VTSS_PTP_1PPS_SYNC_MAIN_MAN :
                    has_sub ? VTSS_PTP_1PPS_SYNC_SUB : VTSS_PTP_1PPS_SYNC_DISABLE;
    pps_conf.pulse_delay = has_pps_phase ? pps_phase : 0;
    pps_conf.cable_asy = has_cable_asy ? cable_asy : 0;
    pps_conf.serial_tod = has_ser_man ? VTSS_PTP_1PPS_SER_TOD_MAN : has_ser_auto ? VTSS_PTP_1PPS_SER_TOD_AUTO : VTSS_PTP_1PPS_SER_TOD_DISABLE;
    rc = ptp_icli_config_traverse_ports(session_id, 0, v_port_type_list, &pps_conf, my_port_1pps_mode_set);
    return rc;
}

static icli_rc_t my_port_1pps_delay_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_1pps_closed_loop_conf_t *pps_delay = (vtss_1pps_closed_loop_conf_t *)cfg;
    mesa_rc my_rc;

    if (VTSS_RC_OK != (my_rc = ptp_1pps_closed_loop_mode_set(uport2iport(uport), pps_delay))) {
        ICLI_PRINTF("Error setting 1pps delay for port %d (%s)\n", uport, error_txt(my_rc));
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_1pps_delay_set(i32 session_id, BOOL has_auto, u32 master_port, BOOL has_man, u32 cable_delay, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_1pps_closed_loop_conf_t pps_delay;
    if (mepa_phy_ts_cap()) {
        pps_delay.mode = has_auto ? VTSS_PTP_1PPS_CLOSED_LOOP_AUTO : has_man ? VTSS_PTP_1PPS_CLOSED_LOOP_MAN : VTSS_PTP_1PPS_CLOSED_LOOP_DISABLE;
        pps_delay.master_port = has_auto ? master_port : 0;
        pps_delay.cable_delay = has_man ? cable_delay : 0;
        rc = ptp_icli_config_traverse_ports(session_id, 0, v_port_type_list, &pps_delay, my_port_1pps_delay_set);
    }
    return rc;
}

icli_rc_t ptp_icli_ms_pdv_show_apr_server_status(i32 session_id, u16 cguId, u16 serverId)
{
    icli_rc_t rc =  ICLI_RC_OK;

#if defined(VTSS_SW_OPTION_ZLS30387)
    char *s;
    if (zl_30380_pdv_apr_server_status_get(cguId, serverId, &s) != VTSS_RC_OK) {
        rc = ICLI_RC_ERROR;
    } else ICLI_PRINTF(s);
#else
    ICLI_PRINTF("External PDV filter function not defined\n");
#endif // defined(VTSS_SW_OPTION_ZLS30387)
    return rc;
}

icli_rc_t ptp_icli_ms_pdv_show_apr_config(i32 session_id, u16 cguId)
{
    icli_rc_t rc =  ICLI_RC_OK;

#if defined(VTSS_SW_OPTION_ZLS30387)
    if (zl_30380_apr_config_dump(cguId) == FALSE) {
        rc = ICLI_RC_ERROR;
    }
#else
    ICLI_PRINTF("External PDV filter function not defined\n");
#endif // defined(VTSS_SW_OPTION_ZLS30387)
    return rc;
}

static const char *tc_int_mode_disp(mesa_packet_internal_tc_mode_t m)
{
    switch (m) {
        case MESA_PACKET_INTERNAL_TC_MODE_30BIT: return "MODE_30BIT";
        case MESA_PACKET_INTERNAL_TC_MODE_32BIT: return "MODE_32BIT";
        case MESA_PACKET_INTERNAL_TC_MODE_44BIT: return "MODE_44BIT";
        case MESA_PACKET_INTERNAL_TC_MODE_48BIT: return "MODE_48BIT";
        default: return "unknown";
    }
}

icli_rc_t ptp_icli_tc_internal_set(i32 session_id, BOOL has_mode, u32 mode)
{
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_packet_internal_tc_mode_t my_mode;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "tc_internal_set, has_mode: %d, mode %d", has_mode, mode);
    if(has_mode) {
        my_mode = (mesa_packet_internal_tc_mode_t)mode;
        if(!tod_tc_mode_set(&my_mode)) {
            ICLI_PRINTF("Failed to set the TC internal mode to %d\n", my_mode);
            rc = ICLI_RC_ERROR;
        } else {
            ICLI_PRINTF("\nSuccessfully set the TC internal mode...\n");
            if (tod_ready()) {
                ICLI_PRINTF("Internal TC mode Configuration has been set, you need to reboot to activate the changed conf.\n");
            }
        }
    } else {
         if(!tod_tc_mode_get(&my_mode)) {
              ICLI_PRINTF("Failed to get the TC internal mode\n");
              rc = ICLI_RC_ERROR;
         } else {
              ICLI_PRINTF("TC internal mode %s\n", tc_int_mode_disp(my_mode));
         }
    }
    return rc;
}

icli_rc_t ptp_icli_phy_timestamp_dis_set(i32 session_id, BOOL phy_ts_dis)
{
    icli_rc_t rc =  ICLI_RC_OK;

    if (tod_board_phy_ts_dis_set(phy_ts_dis)) {
        ICLI_PRINTF(" PHY timestamping mode %s\n", phy_ts_dis ? "disabled" : "enabled");
    } else {
        rc = ICLI_RC_ERROR;
        ICLI_PRINTF(" Failed to disable PHY timestamping mode\n");
    }
    return rc;
}

static const char *sys_clock_sync_mode_txt(vtss_appl_ptp_system_time_sync_mode_t mode)
{
    switch (mode) {
        case VTSS_APPL_PTP_SYSTEM_TIME_NO_SYNC:  return "No System clock to PTP Sync";
        case VTSS_APPL_PTP_SYSTEM_TIME_SYNC_SET: return "Set System time from PTP time";
        case VTSS_APPL_PTP_SYSTEM_TIME_SYNC_GET: return "Get PTP time from System time";
    }
    return "INVALID";
}

icli_rc_t ptp_icli_system_time_sync_set(i32 session_id, BOOL has_get, BOOL has_set, int clockinst)
{
    vtss_appl_ptp_system_time_sync_conf_t conf;
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_rc my_rc;

    if (VTSS_RC_OK != (my_rc = vtss_appl_ptp_system_time_sync_mode_get(&conf))) {
        ICLI_PRINTF("Error getting system clock synch mode (%s)\n", error_txt(my_rc));
        rc = ICLI_RC_ERROR;
    }
    conf.mode = has_get ? VTSS_APPL_PTP_SYSTEM_TIME_SYNC_GET :
                has_set ? VTSS_APPL_PTP_SYSTEM_TIME_SYNC_SET : VTSS_APPL_PTP_SYSTEM_TIME_NO_SYNC;
    conf.clockinst = clockinst;
    if (has_get) {
        mesa_timestamp_t t;
        t.sec_msb = 0;
        t.seconds = time(NULL);
        t.nanoseconds = 0;
        ptp_local_clock_time_set(&t, ptp_instance_2_timing_domain(clockinst));
    }
    // NOTE : System clock can only be set to the ptp clock with HW domain 0.
    else if (has_set) {
        vtss_appl_ptp_clock_control_t port_control;
        port_control.syncToSystemClock = true;
        memset(&port_control.syncToSystemClock, clockinst, sizeof(vtss_appl_ptp_clock_control_t));
    }
    if (VTSS_RC_OK != (my_rc = vtss_appl_ptp_system_time_sync_mode_set(&conf))) {
        ICLI_PRINTF("Error setting system clock synch mode (%s)\n", error_txt(my_rc));
        rc = ICLI_RC_ERROR;
    } else {
        ICLI_PRINTF("System clock synch mode (%s)\n", sys_clock_sync_mode_txt(conf.mode));
    }
    return rc;

}

icli_rc_t ptp_icli_system_time_sync_show(i32 session_id)
{
    vtss_appl_ptp_system_time_sync_conf_t conf;
    icli_rc_t rc =  ICLI_RC_OK;
    mesa_rc my_rc;

    if (VTSS_RC_OK != (my_rc = vtss_appl_ptp_system_time_sync_mode_get(&conf))) {
        ICLI_PRINTF("Error getting system clock synch mode (%s)\n", error_txt(my_rc));
        rc = ICLI_RC_ERROR;
    } else {
        ICLI_PRINTF("System clock synch mode (%s)\n", sys_clock_sync_mode_txt(conf.mode));
    }
    return rc;
}

static const char *ref_clock_freq_txt(mepa_ts_clock_freq_t freq)
{
    switch (freq) {
        case MEPA_TS_CLOCK_FREQ_125M:  return "125 MHz ";
        case MEPA_TS_CLOCK_FREQ_15625M: return "156.25 MHz";
        case MEPA_TS_CLOCK_FREQ_250M: return "250 MHz";
        default: return "INVALID";
    }
}

icli_rc_t ptp_icli_ptp_ref_clock_set(i32 session_id, BOOL has_mhz125, BOOL has_mhz156p25, BOOL has_mhz250)
{
    icli_rc_t rc =  ICLI_RC_OK;

    if (mepa_phy_ts_cap()) {
        mepa_ts_clock_freq_t freq = MEPA_TS_CLOCK_FREQ_250M;

        if (!tod_ref_clock_freg_get(&freq)) {
            ICLI_PRINTF("Error getting ref clock frequency\n");
            rc = ICLI_RC_ERROR;
        }
        if (has_mhz125) {
            freq = MEPA_TS_CLOCK_FREQ_125M;
        } else if (has_mhz156p25) {
            freq = MEPA_TS_CLOCK_FREQ_15625M;
        } else if (has_mhz250) {
            freq = MEPA_TS_CLOCK_FREQ_250M;
        }

        if (!tod_ref_clock_freg_set(&freq)) {
            ICLI_PRINTF("Error setting ref clock frequency\n");
            rc = ICLI_RC_ERROR;
        } else {
            ICLI_PRINTF("System ref clock frequency (%s)\n", ref_clock_freq_txt(freq));
        }
    }

    return rc;
}

static icli_rc_t my_port_ptp_icli_debug_pim_statistics(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    BOOL *clear = (BOOL *)cfg;
    ptp_pim_frame_statistics_t stati;
    
    /* show the statistics  */
    ptp_pim_statistics_get(uport2iport(uport), &stati, *clear);
    if (!*clear) {
        ICLI_PRINTF("Port %d Statistics:\n", uport);
        ICLI_PRINTF("%-24s%12u\n", "Requests", stati.request);
        ICLI_PRINTF("%-24s%12u\n", "Replies", stati.reply);
        ICLI_PRINTF("%-24s%12u\n", "Events", stati.event);
        ICLI_PRINTF("%-24s%12u\n", "RX-Dropped", stati.dropped);
        ICLI_PRINTF("%-24s%12u\n", "TX-Dropped", stati.tx_dropped);
        ICLI_PRINTF("%-24s%12u\n", "Errors", stati.errors);
    }
    return rc;
}

icli_rc_t ptp_icli_debug_pim_statistics(i32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL clear)
{
    icli_rc_t rc =  ICLI_RC_OK;
    rc = ptp_icli_config_traverse_ports(session_id, 0, v_port_type_list, &clear, my_port_ptp_icli_debug_pim_statistics);
    return rc;
}

icli_rc_t ptp_icli_debug_egress_latency_statistics(i32 session_id, BOOL clear)
{
    icli_rc_t rc =  ICLI_RC_OK;
    observed_egr_lat_t lat;
    char str1[30];
    char str2[30];
    char str3[30];
    if (clear) {
        /* clear the Observed Egress latency */
        ptp_clock_egress_latency_clear();
        ICLI_PRINTF("Observed Egress Latency counters cleared\n");
    } else {
        int ti_width = ptp_cap_sub_nano_second() ? 20 : 16;
        std::string hdr = "min             ";

        if (ptp_cap_sub_nano_second()) {
            hdr += "    ";
        }
        hdr += "mean            ";
        if (ptp_cap_sub_nano_second()) {
            hdr += "    ";
        }
        hdr += "max             ";
        if (ptp_cap_sub_nano_second()) {
            hdr += "    ";
        }
        hdr += "count ";
        ptp_cli_table_header(hdr.c_str(), icli_session_self_printf);
        ptp_clock_egress_latency_get(&lat);
        ICLI_PRINTF("%-*s%-*s%-*s%-5d\n",
                ti_width,
                vtss_tod_TimeInterval_To_String(&lat.min,str1,','),
                ti_width,
                vtss_tod_TimeInterval_To_String(&lat.mean,str2,','),
                ti_width,
                vtss_tod_TimeInterval_To_String(&lat.max,str3,','),
                lat.cnt);
    }
    return rc;
}

icli_rc_t ptp_icli_debug_slave_statistics(i32 session_id, int clockinst, BOOL has_enable, BOOL has_disable, BOOL has_clear)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_ptp_slave_statistics_t stati;
    char str1[30];
    char str2[30];
    char str3[30];
    char str4[30];
    char str5[30];
    char str6[30];
    char str7[30];
    char str8[30];
    if (has_enable || has_disable) {
        if (VTSS_RC_OK != ptp_clock_slave_statistics_enable(clockinst, has_enable)) {
            rc = ICLI_RC_ERR_PARAMETER;
        }
    } else {
        if (VTSS_RC_OK != ptp_clock_slave_statistics_get(clockinst, &stati, has_clear)) {
            rc = ICLI_RC_ERR_PARAMETER;
        } else {
            if (stati.enable) {
                mesa_timeinterval_t master_to_slave_mean = stati.master_to_slave_mean / MAX(stati.master_to_slave_mean_count, 1);
                mesa_timeinterval_t slave_to_master_mean = stati.slave_to_master_mean / MAX(stati.slave_to_master_mean_count, 1);

                ICLI_PRINTF(
                        "master_to_slave_max     : %18s sec\n"
                        "master_to_slave_min     : %18s sec\n"
                        "master_to_slave_mean    : %18s sec\n"
                        "master_to_slave_cur     : %18s sec\n"
                        "slave_to_master_max     : %18s sec\n"
                        "slave_to_master_min     : %18s sec\n"
                        "slave_to_master_mean    : %18s sec\n"
                        "slave_to_master_cur     : %18s sec\n"
                        "sync_pack_rx_cnt        : %18u\n"
                        "sync_pack_timeout_cnt   : %18u\n"
                        "delay_req_pack_tx_cnt   : %18u\n"
                        "delay_resp_pack_rx_cnt  : %18u\n"
                        "sync_pack_seq_err_cnt   : %18u\n"
                        "follow_up_pack_loss_cnt : %18u\n"
                        "delay_resp_seq_err_cnt  : %18u\n"
                        "delay_req_not_saved_cnt : %18u\n"
                        "delay_req_no_intr_cnt   : %18u\n",
                        vtss_tod_TimeInterval_To_String(&stati.master_to_slave_max,str1,','),
                        vtss_tod_TimeInterval_To_String(&stati.master_to_slave_min,str2,','),
                        vtss_tod_TimeInterval_To_String(&master_to_slave_mean,str7,','),
                        vtss_tod_TimeInterval_To_String(&stati.master_to_slave_cur,str3,','),
                        vtss_tod_TimeInterval_To_String(&stati.slave_to_master_max,str4,','),
                        vtss_tod_TimeInterval_To_String(&stati.slave_to_master_min,str5,','),
                        vtss_tod_TimeInterval_To_String(&slave_to_master_mean,str8,','),
                        vtss_tod_TimeInterval_To_String(&stati.slave_to_master_cur,str6,','),
                        stati.sync_pack_rx_cnt,
                        stati.sync_pack_timeout_cnt,
                        stati.delay_req_pack_tx_cnt,
                        stati.delay_resp_pack_rx_cnt,
                        stati.sync_pack_seq_err_cnt,
                        stati.follow_up_pack_loss_cnt,
                        stati.delay_resp_seq_err_cnt,
                        stati.delay_req_not_saved_cnt,
                        stati.delay_req_intr_not_rcvd_cnt);
                if (has_clear) {
                    ICLI_PRINTF("Slave statistics cleared\n");
                }
            } else {
                ICLI_PRINTF("Slave statistics not enabled\n");
            }
        }
    }
    return rc;
}

icli_rc_t ptp_icli_debug_one_pps_tod_statistics(i32 session_id, BOOL has_clear)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_ptp_one_pps_tod_statistics_t stati;
    if (VTSS_RC_OK != ptp_clock_one_pps_tod_statistics_get(&stati, has_clear)) {
        rc = ICLI_RC_ERR_PARAMETER;
    } else {
        if (has_clear) {
            ICLI_PRINTF("Slave statistics cleared\n");
        } else {
            ICLI_PRINTF(
                "one_tod_cnt            : %14u\n"
                "one_pps_cnt            : %14u\n"
                "missed_one_pps_cnt     : %14u\n"
                "missed_tod_rx_cnt      : %14u\n",
                stati.tod_cnt,stati.one_pps_cnt,stati.missed_one_pps_cnt,
                stati.missed_tod_rx_cnt);
        }
    }
    return rc;
}

BOOL ptp_icli_ref_clk_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MEBA_CAP_1588_REF_CLK_SEL)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}

BOOL ptp_icli_rs_422_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_PTP_RS422)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}
BOOL ptp_pps_inp_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            runtime->present = false;
            for (int pin = 0; pin < MAX_VTSS_TS_IO_ARRAY_SIZE; pin++) {
                if (pin >= fast_cap(MESA_CAP_TS_IO_CNT)) {
                    break;
                }
                runtime->present = runtime->present || VTSS_IS_IO_PIN_IN(ptp_io_pin[pin].board_assignments);
            }
            return true;
        default:
            break;
    }
    return false;
}
BOOL ptp_inp_0_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 0 && VTSS_IS_IO_PIN_IN(ptp_io_pin[0].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}
BOOL ptp_inp_1_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 1 && VTSS_IS_IO_PIN_IN(ptp_io_pin[1].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}
BOOL ptp_inp_2_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 2 && VTSS_IS_IO_PIN_IN(ptp_io_pin[2].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}
BOOL ptp_inp_3_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 3 && VTSS_IS_IO_PIN_IN(ptp_io_pin[3].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}

BOOL ptp_inp_4_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 4 && VTSS_IS_IO_PIN_IN(ptp_io_pin[4].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}

BOOL ptp_inp_5_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 5 && VTSS_IS_IO_PIN_IN(ptp_io_pin[5].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}

BOOL ptp_pps_out_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            runtime->present = false;
            for (int pin = 0; pin < MAX_VTSS_TS_IO_ARRAY_SIZE; pin++) {
                if (pin >= fast_cap(MESA_CAP_TS_IO_CNT)) {
                    break;
                }
                runtime->present = runtime->present || VTSS_IS_IO_PIN_OUT(ptp_io_pin[pin].board_assignments);
            }
            return true;
        default:
            break;
    }
    return false;
}
BOOL ptp_out_0_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 0 && VTSS_IS_IO_PIN_OUT(ptp_io_pin[0].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}
BOOL ptp_out_1_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 1 && VTSS_IS_IO_PIN_OUT(ptp_io_pin[1].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}
BOOL ptp_out_2_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 2 && VTSS_IS_IO_PIN_OUT(ptp_io_pin[2].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}
BOOL ptp_out_3_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 3 && VTSS_IS_IO_PIN_OUT(ptp_io_pin[3].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}

BOOL ptp_out_4_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 4 && VTSS_IS_IO_PIN_OUT(ptp_io_pin[4].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}

BOOL ptp_out_5_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_TS_IO_CNT) > 5 && VTSS_IS_IO_PIN_OUT(ptp_io_pin[5].board_assignments)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}

#define HI_BIT_U8(val) ((u8)val & 0xF0) ? ((val & 0xC0) ? ((val & 0x80) ? 8 : 7) : ((val & 0x20) ? 6 : 5)) : \
                         (((val & 0xC) ? ((val & 8) ? 4 : 3) : ((val & 2) ? 2 : ((val & 1) ? 1 : 0))))

BOOL port_icli_runtime_tc_internal_range(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
             if (fast_cap(MESA_CAP_TS_TOD_INTERNAL_TC_MODE)) {
                 runtime->present = true;
             } else {
                 runtime->present = false;
             }
             return true;
        case ICLI_ASK_HELP:
            if (!mepa_phy_ts_cap() && (((HI_BIT_U8(fast_cap(MESA_CAP_TS_TOD_INTERNAL_TC_MODE))) - 1) <= 2)) {
                icli_sprintf(runtime->help, "0 = MODE_30BIT, 1 = MODE_32BIT, 2 = MODE_44BIT");
            } else {
                icli_sprintf(runtime->help, "0 = MODE_30BIT, 1 = MODE_32BIT, 2 = MODE_44BIT, 3 = MODE_48BIT");
            }
            return TRUE;
        case ICLI_ASK_RANGE:
            runtime->range.type = ICLI_RANGE_TYPE_UNSIGNED;
            runtime->range.u.sr.cnt = 1;
            runtime->range.u.sr.range[0].min = 0;
            runtime->range.u.sr.range[0].max = mepa_phy_ts_cap() ? 3 : ((HI_BIT_U8(fast_cap(MESA_CAP_TS_TOD_INTERNAL_TC_MODE))) - 1);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

BOOL ptp_icli_sync_ann_auto_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
            if (fast_cap(MESA_CAP_SYNCE_ANN_AUTO_TRANSMIT)) {
                runtime->present = true;
            } else {
                runtime->present = false;
            }
            return true;
        default:
            break;
    }
    return false;
}

BOOL ptp_icli_filter_type_basic_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
    switch (ask) {
        case ICLI_ASK_PRESENT:
#ifdef SW_OPTION_BASIC_PTP_SERVO
            runtime->present = true;
#else
            runtime->present = false;
#endif
            return true;
        default:
            break;
    }
    return false;
}

BOOL ptp_icli_wireless_option(u32 session_id, icli_runtime_ask_t ask, icli_runtime_t *runtime)
{
 switch (ask) {
        case ICLI_ASK_PRESENT:
            runtime->present = false;
            return true;
        default:
            break;
    }
    return false;
}

icli_rc_t ptp_icli_debug_servo_best_master(i32 session_id, int clockinst)
{
    icli_rc_t rc = ICLI_RC_OK;

//    rc = (vtss_ptp_servo_set_best_master(clockinst) == VTSS_RC_OK) ? ICLI_RC_OK : ICLI_RC_ERROR;
    ICLI_PRINTF("Servo best master is not supported\n");
    return rc;
}

icli_rc_t ptp_icli_show_servo_source(i32 session_id)
{
    vtss_ptp_synce_src_t src;
    if (ptp_get_selected_src(&src) == VTSS_RC_OK) {
        bool generic = false;
#if defined(VTSS_SW_OPTION_ZLS30387)
        generic = zl_3038x_servo_dpll_config_generic();
#endif //VTSS_SW_OPTION_ZLS30387

        ICLI_PRINTF("Servo current source is type %s ref %d, DPLL_type %s\n", sync_src_type_2_txt(src.type), src.ref, generic ? "Generic" : "DPLLSpecific");
        return ICLI_RC_OK;
    } else {
        return ICLI_RC_ERROR;
    }
}

icli_rc_t ptp_icli_show_servo_mode_ref(i32 session_id)
{
    int inst = 0;
    vtss_ptp_servo_mode_ref_t mode_ref;
    while (VTSS_RC_OK == vtss_ptp_get_servo_mode_ref(inst, &mode_ref)) {
        ICLI_PRINTF("Servo [%d] mode %s ref %d\n", inst, sync_servo_mode_2_txt(mode_ref.mode), mode_ref.ref);
        inst++;
    }
    return ICLI_RC_OK;
}


icli_rc_t ptp_icli_debug_path_trace(i32 session_id, int clockinst)
{
    icli_rc_t rc;
    ptp_path_trace_t trace;
    int i;
    char str1 [40];

    rc = (ptp_clock_path_trace_get(clockinst, &trace) == VTSS_RC_OK) ? ICLI_RC_OK : ICLI_RC_ERROR;
    if (rc == ICLI_RC_OK) {
        ICLI_PRINTF("path_trace size: %d\n", trace.size);
        for (i = 0; i < trace.size && i < PTP_PATH_TRACE_MAX_SIZE; i++) {
            ICLI_PRINTF("path_trace[%d] %s\n", i, ClockIdentityToString(trace.pathSequence[i], str1));

        }
    }
    return rc;
}

#if defined (VTSS_SW_OPTION_P802_1_AS)
icli_rc_t ptp_icli_debug_802_1as_status(i32 session_id, int clockinst)
{
    icli_rc_t rc;
    vtss_ptp_clock_802_1as_bmca_t status;
    char str1 [300];

    rc = (ptp_clock_path_802_1as_status_get(clockinst, &status) == VTSS_RC_OK) ? ICLI_RC_OK : ICLI_RC_ERROR;
    if (rc == ICLI_RC_OK) {
        ICLI_PRINTF("\n802.1AS clock inst[%d] status:\n", clockinst);
        ICLI_PRINTF("  timeProperties:\n    %s\n", vtss_ptp_TimePropertiesToString(&status.timeProperties, str1, sizeof(str1)));
        ICLI_PRINTF("  systimeProperties:\n    %s\n", vtss_ptp_TimePropertiesToString(&status.sysTimeProperties, str1, sizeof(str1)));
        ICLI_PRINTF("  systemPriorityVector:\n%s\n", vtss_ptp_PriorityVectorToString(&status.systemPriorityVector, str1, sizeof(str1)));
        ICLI_PRINTF("  gmPriorityVector:\n%s\n", vtss_ptp_PriorityVectorToString(&status.gmPriorityVector, str1, sizeof(str1)));
        ICLI_PRINTF("  gmPresent: %s\n", status.gmPresent ? "TRUE" : "FALSE");
   }
   return rc;
}

icli_rc_t my_ptp_icli_debug_802_1as_port_status(i32 session_id, int clockinst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc;
    vtss_ptp_port_802_1as_bmca_t status;
    char str1 [300];

    rc = (ptp_port_path_802_1as_status_get(clockinst, uport2iport(uport), &status) == VTSS_RC_OK) ? ICLI_RC_OK : ICLI_RC_ERROR;
    if (rc == ICLI_RC_OK) {
        ICLI_PRINTF("\n802.1AS clock inst[%d], port[%d] status:\n", clockinst, uport);
        ICLI_PRINTF("  masterPriority:\n%s\n", vtss_ptp_PriorityVectorToString(&status.masterPriority, str1, sizeof(str1)));
        ICLI_PRINTF("  portPriority:\n%s\n", vtss_ptp_PriorityVectorToString(&status.portPriority, str1, sizeof(str1)));
        ICLI_PRINTF("  annTimeProperties:\n    %s\n", vtss_ptp_TimePropertiesToString(&status.annTimeProperties, str1, sizeof(str1)));
        ICLI_PRINTF("  infoIs: %s\n", vtss_ptp_InfoIsToString(status.infoIs));
        ICLI_PRINTF("  portStepsRemoved: %d\n", status.portStepsRemoved);
    }
    return rc;
}

icli_rc_t ptp_icli_debug_802_1as_port_status(i32 session_id, int clockinst, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    rc = ptp_icli_config_traverse_ports(session_id, 0, v_port_type_list, 0, my_ptp_icli_debug_802_1as_port_status);
    return rc;
}
#endif //(VTSS_SW_OPTION_P802_1_AS)

#if defined(VTSS_SW_OPTION_ZLS30387)
icli_rc_t ptp_icli_debug_mspdv_loglevel(i32 session_id, u32 loglevel)
{
    return zl_30380_apr_set_log_level(loglevel) ? ICLI_RC_OK : ICLI_RC_ERROR;
}
#endif

#if defined(VTSS_SW_OPTION_ZLS30387)
icli_rc_t ptp_icli_show_psl_fcl_config(i32 session_id, u16 cguId)
{
    return zl_30380_apr_show_psl_fcl_config(cguId) ? ICLI_RC_OK : ICLI_RC_ERROR;
}

icli_rc_t ptp_icli_show_apr_statistics(i32 session_id, u16 cguId, u32 stati)
{
    return zl_30380_apr_show_statistics(cguId, stati) ? ICLI_RC_OK : ICLI_RC_ERROR;
}



#endif

icli_rc_t ptp_icli_virtual_port_mode_set(i32 session_id, uint instance, BOOL has_main_auto, BOOL has_main_man, BOOL has_sub, BOOL has_pps_delay, u32 pps_delay, BOOL has_pps_in, u32 input_pin, BOOL has_pps_out, u32 output_pin, BOOL has_freq_out, u32 freq_output_pin, u32 freq)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_appl_ptp_virtual_port_config_t cfg;

    if (fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_CARACAL) {
        ICLI_PRINTF("Virtual port not supported on caracal platform\n");
        return ICLI_RC_ERROR;
    }
    if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &cfg) == MESA_RC_OK) {
        cfg.virtual_port_mode = has_main_auto ? VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO : has_main_man ? VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN : 
                                has_sub ? VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB : has_pps_in ? VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN :
                                has_pps_out ? VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT : has_freq_out ? VTSS_PTP_APPL_VIRTUAL_PORT_MODE_FREQ_OUT :
                                VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE;
        T_IG(VTSS_TRACE_GRP_PTP_ICLI, "mode %d\n", cfg.virtual_port_mode);
        cfg.delay = has_pps_delay ? pps_delay : VTSS_PTP_VIRTUAL_PORT_DEFAULT_DELAY;
        if (has_pps_in) { cfg.input_pin = input_pin;}
        if (has_pps_out) { cfg.output_pin = output_pin;}
             
        if (has_freq_out) { 
            cfg.output_pin = freq_output_pin;
            cfg.freq = freq;
        }
        cfg.enable = (cfg.virtual_port_mode != VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE) ? true : false;
        if (vtss_appl_ptp_clock_config_virtual_port_config_set(instance, &cfg) != MESA_RC_OK) {
            rc = ICLI_RC_ERROR;
        }
    } else {
        T_IG(VTSS_TRACE_GRP_PTP_ICLI, "error\n");
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_virtual_port_time_property_set(i32 session_id, int clockinst, BOOL has_utc_offset, i32 utc_offset, BOOL has_valid,
                                    BOOL has_leapminus_59, BOOL has_leapminus_61, BOOL has_time_traceable,
                                    BOOL has_freq_traceable, BOOL has_ptptimescale,
                                    BOOL has_time_source, u8 time_source,
                                    BOOL has_leap_pending, char *date_string, BOOL has_leaptype_59, BOOL has_leaptype_61)
{
    vtss_appl_ptp_clock_timeproperties_ds_t prop;
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "icli timeproperty instance %d", clockinst);
    if (ptp_get_virtual_port_time_property(clockinst, &prop) != VTSS_RC_OK) {
        return ICLI_RC_ERROR;
    }
    if (has_utc_offset)  prop.currentUtcOffset = utc_offset;
    prop.currentUtcOffsetValid = has_valid;
    prop.leap59 = has_leapminus_59;
    prop.leap61 = has_leapminus_61;
    prop.timeTraceable = has_time_traceable;
    prop.frequencyTraceable = has_freq_traceable;
    prop.ptpTimescale = has_ptptimescale;
    if (has_time_source) prop.timeSource = time_source;
    if (has_leap_pending) {
        prop.pendingLeap = true;
        (void)extract_date(date_string, &prop.leapDate);
        prop.leapType = has_leaptype_59 ? VTSS_APPL_PTP_LEAP_SECOND_59 : VTSS_APPL_PTP_LEAP_SECOND_61;
    }
    if (ptp_set_virtual_port_time_property(clockinst, &prop) != VTSS_RC_OK) {
        ICLI_PRINTF("Error writing virtual port's timing properties instance %d\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_virtual_port_default_time_property_set(i32 session_id, int clockinst)
{
    icli_rc_t rc =  ICLI_RC_OK;
    vtss_appl_ptp_virtual_port_config_t cfg;
    vtss_appl_ptp_clock_timeproperties_ds_t prop;

    vtss_appl_ptp_clock_config_default_virtual_port_config_get(&cfg);
    memcpy(&prop, &cfg.timeproperties, sizeof(prop));
    if (ptp_set_virtual_port_time_property(clockinst, &prop) != VTSS_RC_OK) {
        ICLI_PRINTF("Error clearing virtual port's timing properties instance %d\n", clockinst);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_virtual_port_tod_set(i32 session_id, uint instance, BOOL has_ser, BOOL has_proto, BOOL has_polyt, BOOL has_zda, BOOL has_rmc, BOOL has_pim, u32 pim_port)
{
    icli_rc_t rc =  ICLI_RC_OK;

    vtss_appl_ptp_virtual_port_config_t cfg;

    if (vtss_appl_ptp_clock_config_virtual_port_config_get(instance, &cfg) == MESA_RC_OK) {
        cfg.proto = has_ser ? (has_proto ? (has_rmc ? VTSS_PTP_APPL_RS422_PROTOCOL_SER_RMC : (has_zda ? VTSS_PTP_APPL_RS422_PROTOCOL_SER_ZDA : VTSS_PTP_APPL_RS422_PROTOCOL_SER_POLYT)) : VTSS_PTP_APPL_RS422_PROTOCOL_SER_POLYT) : (has_pim ? VTSS_PTP_APPL_RS422_PROTOCOL_PIM : VTSS_PTP_APPL_RS422_PROTOCOL_NONE);
        cfg.portnum = pim_port;
        if (vtss_appl_ptp_clock_config_virtual_port_config_set(instance, &cfg) != MESA_RC_OK) {
            rc = ICLI_RC_ERROR;
        }
    } else {
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_debug_set_ptp_delta_time(i32 session_id, u32 domain, u16 sec_msb, u32 sec_lsb, u32 nanosec, BOOL has_neg)
{
    icli_rc_t rc = ICLI_RC_OK;
    mesa_timestamp_t  ts;
    ts.sec_msb = sec_msb;
    ts.seconds = sec_lsb;
    ts.nanoseconds = nanosec;
    
    vtss_local_clock_time_set_delta(&ts, domain, has_neg);
    return rc;
}

static icli_rc_t my_port_gptp_to_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u8 *to = (u8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) &&
            (ds_cfg.enabled == 1))
    {
        ds_cfg.c_802_1as.gPtpCapableReceiptTimeout = *to;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error getting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_gptp_to_set(i32 session_id, int clockinst, BOOL set, u8 gptp_to, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    u8 to;
    if (set) {
        to = gptp_to;
    } else {
        to = 3;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &to, my_port_gptp_to_set);
    return rc;
}

static icli_rc_t my_port_gptp_interval_set(i32 session_id, int inst, mesa_port_no_t uport, void *cfg)
{
    icli_rc_t rc =  ICLI_RC_OK;
    i8 *interval = (i8 *)cfg;
    vtss_appl_ptp_config_port_ds_t ds_cfg;
    vtss_ifindex_t ifindex;

    (void)vtss_ifindex_from_port(0, uport2iport(uport), &ifindex);

    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ds_cfg) == VTSS_RC_OK) && (ds_cfg.enabled == 1))
    {
        ds_cfg.c_802_1as.initialLogGptpCapableMessageInterval = *interval;
        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &ds_cfg) == VTSS_RC_ERROR) {
            ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
            rc = ICLI_RC_ERROR;
        }
    } else {
        ICLI_PRINTF("Error setting port data instance %d port %d\n", inst, uport);
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

icli_rc_t ptp_icli_port_gptp_interval_set(i32 session_id, int clockinst, i8 interval, BOOL has_stop, BOOL has_default, icli_stack_port_range_t *v_port_type_list)
{
    icli_rc_t rc =  ICLI_RC_OK;
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "interval %d, has_stop %d, has_default %d", interval, has_stop, has_default);
    if (has_stop) {
        interval = 127;
    } else if (has_default) {
        interval = 126;
    }
    rc = ptp_icli_config_traverse_ports(session_id, clockinst, v_port_type_list, &interval, my_port_gptp_interval_set);
    return rc;
}

icli_rc_t ptp_icli_cmlds_port_data_show(i32 session_id, BOOL has_port_status, BOOL has_port_conf, BOOL has_port_statistics, icli_stack_port_range_t *port_type_list_p)
{
    icli_rc_t rc = ICLI_RC_OK;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    BOOL            first = true;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                if (has_port_status) {
                    ptp_show_cmlds_port_status(uport, first, icli_session_self_printf);
                }
                if (has_port_conf) {
                    ptp_show_cmlds_port_conf(uport, first, icli_session_self_printf);
                }
                if (has_port_statistics) {
                    ptp_show_cmlds_port_statistics(uport, first, icli_session_self_printf, false);
                }
                first = false;
            }
        }
    }

    return rc;
}

icli_rc_t ptp_icli_cmlds_default_ds_show(i32 session_id)
{
    vtss_appl_ptp_802_1as_cmlds_default_ds_t def_ds;
    char str[30];

    if (vtss_appl_ptp_cmlds_default_ds_get(&def_ds) == VTSS_RC_OK) {
        ICLI_PRINTF("ClockIdentity  : %s \n", ClockIdentityToString(def_ds.clockIdentity, str));
        ICLI_PRINTF("numberLinkPorts: %u \n", def_ds.numberLinkPorts);
        ICLI_PRINTF("sdoId          : 0x%x \n", def_ds.sdoId);
    }
    return ICLI_RC_OK;
}

icli_rc_t ptp_icli_cmlds_pdelay_thresh_set(i32 session_id, BOOL set, u32 pdelay_thresh, icli_stack_port_range_t *port_type_list_p)
{
    icli_rc_t rc = ICLI_RC_OK;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t conf;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                if (vtss_appl_ptp_cmlds_port_conf_get(uport, &conf) == VTSS_RC_OK) {
                    if (set) {
                        conf.peer_d.meanLinkDelayThresh = ((mesa_timeinterval_t)pdelay_thresh)<<16;
                    } else {
                        conf.peer_d.meanLinkDelayThresh = VTSS_MAX_TIMEINTERVAL;
                    }
                }
                vtss_appl_ptp_cmlds_port_conf_set(uport, &conf);
            }
        }
    }
    return rc;
}
                    
icli_rc_t ptp_icli_cmlds_mean_link_delay(i32 session_id, BOOL mgtSettableComputeMeanLinkDelay, BOOL useMgtSettableComputeMeanLinkDelay, icli_stack_port_range_t *port_type_list_p)
{
    icli_rc_t rc = ICLI_RC_OK;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t conf, def;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                vtss_appl_ptp_cmlds_conf_defaults_get(&def);
                if (vtss_appl_ptp_cmlds_port_conf_get(uport, &conf) == VTSS_RC_OK) {
                    conf.peer_d.mgtSettableComputeMeanLinkDelay= mgtSettableComputeMeanLinkDelay;
                    if (useMgtSettableComputeMeanLinkDelay) {
                        conf.peer_d.useMgtSettableComputeMeanLinkDelay= true;
                    } else {
                        conf.peer_d.useMgtSettableComputeMeanLinkDelay = def.peer_d.useMgtSettableComputeMeanLinkDelay;
                    }
                }
                vtss_appl_ptp_cmlds_port_conf_set(uport, &conf);
            }
        }
    }

    return rc;
}
icli_rc_t ptp_icli_cmlds_pdelay_req_int_set(i32 session_id, BOOL enable, i8 pdelay_req_int, BOOL has_stop, BOOL has_default, BOOL UseMgtSettableLogPdelayReqInterval, icli_stack_port_range_t *port_type_list_p)
{
    icli_rc_t rc = ICLI_RC_OK;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t conf, def;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                vtss_appl_ptp_cmlds_conf_defaults_get(&def);
                if (vtss_appl_ptp_cmlds_port_conf_get(uport, &conf) == VTSS_RC_OK) {
                    if (has_stop) {
                        conf.peer_d.mgtSettableLogPdelayReqInterval = 127;
                    } else if (has_default) {
                        conf.peer_d.mgtSettableLogPdelayReqInterval = 126;
                    } else if (enable) {
                        conf.peer_d.mgtSettableLogPdelayReqInterval = pdelay_req_int;
                    } else {
                        conf.peer_d.mgtSettableLogPdelayReqInterval = def.peer_d.mgtSettableLogPdelayReqInterval;
                    }

                    if (UseMgtSettableLogPdelayReqInterval) {
                        conf.peer_d.useMgtSettableLogPdelayReqInterval = true;
                    } else {
                        conf.peer_d.useMgtSettableLogPdelayReqInterval = def.peer_d.useMgtSettableLogPdelayReqInterval;
                    }
                }
                vtss_appl_ptp_cmlds_port_conf_set(uport, &conf);
            }
        }
    }
    return rc;
}
icli_rc_t ptp_icli_cmlds_allow_lost_resp_set(i32 session_id, BOOL enable, u8 allow_lost_resp, icli_stack_port_range_t *port_type_list_p)
{
    icli_rc_t rc = ICLI_RC_OK;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t conf;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                if (vtss_appl_ptp_cmlds_port_conf_get(uport, &conf) == VTSS_RC_OK) {
                    if (enable) {
                        conf.peer_d.allowedLostResponses = allow_lost_resp;
                    } else {
                        conf.peer_d.allowedLostResponses = DEFAULT_MAX_CONTIGOUS_MISSED_PDELAY_RESPONSE;
                    }
                }
                vtss_appl_ptp_cmlds_port_conf_set(uport, &conf);
            }
        }
    }

    return rc;
}
icli_rc_t ptp_icli_cmlds_allow_faults_set(i32 session_id, BOOL enable, u8 allow_faults, icli_stack_port_range_t *port_type_list_p)
{
    icli_rc_t rc = ICLI_RC_OK;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t conf;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                if (vtss_appl_ptp_cmlds_port_conf_get(uport, &conf) == VTSS_RC_OK) {
                    if (enable) {
                        conf.peer_d.allowedFaults = allow_faults;
                    } else {
                        conf.peer_d.allowedFaults = DEFAULT_MAX_PDELAY_ALLOWED_FAULTS;
                    }
                }
                vtss_appl_ptp_cmlds_port_conf_set(uport, &conf);
            }
        }
    }

    return rc;
}

icli_rc_t ptp_icli_cmlds_delay_asym_set(i32 session_id, i32 delay_asymmetry, icli_stack_port_range_t *port_type_list_p)
{
    icli_rc_t rc = ICLI_RC_OK;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t conf;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                if (vtss_appl_ptp_cmlds_port_conf_get(uport, &conf) == VTSS_RC_OK) {
                    conf.delayAsymmetry = ((mesa_timeinterval_t)delay_asymmetry)<<16;
                }
                vtss_appl_ptp_cmlds_port_conf_set(uport, &conf);
            }
        }
    }

    return rc;
}

icli_rc_t ptp_icli_cmlds_nbr_rate_ratio(i32 session_id, BOOL compute_neighbor_rate_ratio, BOOL useMgtSettableComputeNeighborRateRatio, icli_stack_port_range_t *port_type_list_p)
{
    icli_rc_t rc = ICLI_RC_OK;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t conf, def;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                vtss_appl_ptp_cmlds_conf_defaults_get(&def);
                if (vtss_appl_ptp_cmlds_port_conf_get(uport, &conf) == VTSS_RC_OK) {
                    conf.peer_d.mgtSettableComputeNeighborRateRatio = compute_neighbor_rate_ratio;
                    if (useMgtSettableComputeNeighborRateRatio) {
                        conf.peer_d.useMgtSettableComputeNeighborRateRatio = true;
                    } else {
                        conf.peer_d.useMgtSettableComputeNeighborRateRatio = def.peer_d.useMgtSettableComputeNeighborRateRatio;
                    }
                }
                vtss_appl_ptp_cmlds_port_conf_set(uport, &conf);
            }
        }
    }

    return rc;
}

icli_rc_t ptp_icli_cmlds_statistics_clear(i32 session_id, BOOL has_clear, icli_stack_port_range_t *port_type_list_p)
{
    icli_rc_t rc = ICLI_RC_OK;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;

    if (port_type_list_p) { //at least one range input
        for (range_idx = 0; range_idx < port_type_list_p->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < port_type_list_p->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = port_type_list_p->switch_range[range_idx].begin_uport + cnt_idx;

                ptp_show_cmlds_port_statistics(uport, true, icli_session_self_printf, has_clear);
            }
        }
    }

    return rc;
}

void ptp_icli_debug_basic_servo_hybrid_enable(i32 session_id, bool enable)
{
    vtss_appl_ptp_clock_config_default_ds_t clock_config;

    if ((vtss_appl_ptp_clock_config_default_ds_get(0, &clock_config) == VTSS_RC_OK) &&
        (clock_config.profile == VTSS_APPL_PTP_PROFILE_G_8275_1 ||
        clock_config.profile == VTSS_APPL_PTP_PROFILE_1588)) {
        clock_config.filter_type =  enable ? VTSS_APPL_PTP_FILTER_TYPE_BASIC : VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE;
        if (vtss_appl_ptp_clock_config_default_ds_set(0, &clock_config) == VTSS_RC_OK) {
            ptp_debug_hybrid_mode_set(enable);
        }
    } else {
        ICLI_PRINTF("Debug mode not executed\n");
    }
}

icli_rc_t ptp_icli_software_clock_pps_domain_set(int32_t session_id, uint32_t instance, uint32_t pps_domain, bool set)
{
    icli_rc_t rc = ICLI_RC_OK;

    if (!vtssLocalClockPpsConfSet(instance, pps_domain, set)) {
        ICLI_PRINTF("Could not configure 1pps domain for software clock\n");
        rc = ICLI_RC_ERROR;
    }
    return rc;
}

void ptp_icli_software_clock_show_data(i32 session_id, uint32_t instance)
{
    char str[100];
    localClockData_t data = {};

    vtss_local_clock_soft_data_get(instance, &data);
    vtss_tod_timestamp_to_string(&data.t0, sizeof(str), str);
    ICLI_PRINTF("t0 : %s\n", str);
    ICLI_PRINTF("drift : %s\n", vtss_tod_TimeInterval_To_String(&data.drift, str, 0));
    ICLI_PRINTF("ratio : %s\n", vtss_tod_TimeInterval_To_String(&data.ratio, str, 0));
    ICLI_PRINTF("ptp_offset : %d\n", data.ptp_offset);
    ICLI_PRINTF("adj : " VPRI64d "\n", data.adj);
    ICLI_PRINTF("set_time_count : %u\n", data.set_time_count);
    ICLI_PRINTF("ppsDomain : %d\n", data.ppsDomain);
    ICLI_PRINTF("ppsProcDelay : " VPRI64d " ns\n", data.ppsProcDelay);
}

void ptp_icli_internal_mode_sync_rate_set(i32 session, uint32_t instance, int32_t sync_rate, bool enable)
{
    ptp_internal_mode_config_t in_cfg;

    ptp_internal_mode_config_get(instance, &in_cfg);
    if (enable) {
        in_cfg.sync_rate = sync_rate;
    } else {
        in_cfg.sync_rate = DEFAULT_INTERNAL_MODE_SYNC_RATE;
    }
    ptp_internal_mode_config_set(instance, in_cfg);
}
