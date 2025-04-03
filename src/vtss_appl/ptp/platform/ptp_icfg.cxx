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

#ifdef VTSS_SW_OPTION_ICFG
#include "icfg_api.h"
#include "icli_porting_util.h"
#include "ptp_api.h"
#include "vtss_tod_api.h"
#include "ptp.h" // For Trace
#include "msg_api.h" // To check is an isid is valid
#include "misc_api.h"
#include "tod_api.h"
#include "ptp_1pps_sync.h"
#include "ptp_1pps_closed_loop.h"
#endif
#include "ptp_1pps_serial.h"
#include "ptp_local_clock.h"

/***************************************************************************/
/*  Internal types                                                         */
/***************************************************************************/

/***************************************************************************/
/*  Internal functions                                                     */
/***************************************************************************/

static const char *device_type_2_string(u8 type)
{
    switch (type) {
        case VTSS_APPL_PTP_DEVICE_NONE:            return "none";
        case VTSS_APPL_PTP_DEVICE_ORD_BOUND:       return "boundary";
        case VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT: return "p2ptransparent";
        case VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT: return "e2etransparent";
        case VTSS_APPL_PTP_DEVICE_SLAVE_ONLY:      return "slave";
        case VTSS_APPL_PTP_DEVICE_MASTER_ONLY:     return "master";
        case VTSS_APPL_PTP_DEVICE_BC_FRONTEND:     return "bcfrontend";
        case VTSS_APPL_PTP_DEVICE_AED_GM:          return "aedGm";
        case VTSS_APPL_PTP_DEVICE_INTERNAL:        return "internal";
        default:                         return "?";
    }
}

static const char *protocol_2_string(u8 p)
{
    switch (p) {
        case VTSS_APPL_PTP_PROTOCOL_ETHERNET:       return "ethernet";
        case VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED: return "ethernet-mixed";
        case VTSS_APPL_PTP_PROTOCOL_IP4MULTI:       return "ip4multi";
        case VTSS_APPL_PTP_PROTOCOL_IP4MIXED:       return "ip4mixed";
        case VTSS_APPL_PTP_PROTOCOL_IP4UNI:         return "ip4uni";
        case VTSS_APPL_PTP_PROTOCOL_IP6MIXED:       return "ip6mixed";
        case VTSS_APPL_PTP_PROTOCOL_OAM:            return "oam";
        case VTSS_APPL_PTP_PROTOCOL_ONE_PPS:        return "onepps";
        case VTSS_APPL_PTP_PROTOCOL_ANY:            return "ethip4ip6-combo";
        default:                          return "?";
    }
}

static const char *filter_type_2_string(u32 p)
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
#if defined(SW_OPTION_BASIC_PTP_SERVO)
        case PTP_FILTERTYPE_BASIC:            // Note: This is a special case. We use servo type
             return "basic";        //       PTP_FILTERTYPE_BASIC for 802.1AS profile and calibration purpose only.
#endif
        default: return "?";
    }
}

static const char *one_pps_mode_2_string(vtss_appl_ptp_ext_clock_1pps_t m)
{
    switch (m) {
        case VTSS_APPL_PTP_ONE_PPS_DISABLE: return "";
        case VTSS_APPL_PTP_ONE_PPS_OUTPUT: return "output";
        default: return "unknown";
    }
}

static const char *cli_rs422_baudrate_2_string(vtss_serial_baud_rate_t baudrate)
{
    switch(baudrate) {
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

static const char *cli_rs422_parity_2_string(vtss_serial_parity_t parity)
{
    switch(parity) {
        case VTSS_SERIAL_PARITY_NONE:  return "none";  break;
        case VTSS_SERIAL_PARITY_EVEN:  return "even";  break;
        case VTSS_SERIAL_PARITY_ODD:   return "odd";   break;
        case VTSS_SERIAL_PARITY_MARK:  return "mark";  break;
        case VTSS_SERIAL_PARITY_SPACE: return "space"; break;
        default: return "???";
    }
}

static const char *cli_rs422_word_length_2_string(vtss_serial_word_length_t word_length)
{
    switch(word_length) {
        case VTSS_SERIAL_WORD_LENGTH_5: return "5"; break;
        case VTSS_SERIAL_WORD_LENGTH_6: return "6"; break;
        case VTSS_SERIAL_WORD_LENGTH_7: return "7"; break;
        case VTSS_SERIAL_WORD_LENGTH_8: return "8"; break;
        default: return "???";
    }
}

static const char *cli_rs422_stop_2_string(vtss_serial_stop_bits_t stop)
{
    switch(stop) {
        case VTSS_SERIAL_STOP_1:   return "1";   break;
        case VTSS_SERIAL_STOP_2:   return "2";   break;
        default: return "???";
    }
}

static const char *cli_rs422_flags_2_string(u32 flags)
{
    if ((flags & VTSS_SERIAL_FLOW_RTSCTS_RX) && (flags & VTSS_SERIAL_FLOW_RTSCTS_TX)) {
        return "rtscts";
    }
    else if (flags == 0) {
        return "none";
    }
    else {
        return "???";
    }
}

static const char *cli_pps_sync_mode_2_string(vtss_1pps_sync_mode_t m)
{
    switch (m) {
        case VTSS_PTP_1PPS_SYNC_MAIN_MAN: return "main-man";
        case VTSS_PTP_1PPS_SYNC_MAIN_AUTO: return "main-auto";
        case VTSS_PTP_1PPS_SYNC_SUB: return "sub";
        case VTSS_PTP_1PPS_SYNC_DISABLE: return "disable";
        default: return "unknown";
    }
}

static const char *cli_pps_ser_tod_2_string(vtss_1pps_ser_tod_mode_t m)
{
    switch (m) {
        case VTSS_PTP_1PPS_SER_TOD_MAN: return "ser-man";
        case VTSS_PTP_1PPS_SER_TOD_AUTO: return "ser-auto";
        case VTSS_PTP_1PPS_SER_TOD_DISABLE: return " ";
        default: return "unknown";
    }
}

static const char *cli_adj_method_2_string(vtss_appl_ptp_preferred_adj_t m)
{
    switch (m) {
        case VTSS_APPL_PTP_PREFERRED_ADJ_LTC: return "ltc";
        case VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE: return "single";
        case VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT: return "independent";
        case VTSS_APPL_PTP_PREFERRED_ADJ_COMMON: return "common";
        case VTSS_APPL_PTP_PREFERRED_ADJ_AUTO: return "auto";
        default: return "unknown";
    }
}


static const char *rs422_protomode_2_string(vtss_ptp_appl_rs422_protocol_t proto)
{
    switch (proto) {
        case VTSS_PTP_APPL_RS422_PROTOCOL_SER_POLYT: return "ser proto polyt";
        case VTSS_PTP_APPL_RS422_PROTOCOL_SER_ZDA: return "ser proto zda";
        case VTSS_PTP_APPL_RS422_PROTOCOL_SER_RMC: return "ser proto rmc";
        default: return "";
   }
}

static const char *virtual_port_mode_2_string(vtss_appl_virtual_port_mode_t port_mode)
{
    switch (port_mode) {
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO: return "main-auto";
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB: return "sub";
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN: return "main-man";
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN:return "pps-in";
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT: return "pps-out";
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_FREQ_OUT: return "freq-out";
        default: return "";
    }
}

// returns true if time properties are different.
static bool cmp_time_prop(vtss_appl_ptp_clock_timeproperties_ds_t *pr_a, vtss_appl_ptp_clock_timeproperties_ds_t *pr_b)
{
    return pr_a->currentUtcOffset != pr_b->currentUtcOffset ||
           pr_a->currentUtcOffsetValid != pr_b->currentUtcOffsetValid ||
           pr_a->leap59 != pr_b->leap59 ||
           pr_a->leap61 != pr_b->leap61 ||
           pr_a->timeTraceable != pr_b->timeTraceable ||
           pr_a->frequencyTraceable != pr_b->frequencyTraceable ||
           pr_a->ptpTimescale != pr_b->ptpTimescale ||
           pr_a->timeSource != pr_b->timeSource ||
           pr_a->pendingLeap != pr_b->pendingLeap ||
           pr_a->leapDate != pr_b->leapDate ||
           pr_a->leapType != pr_b->leapType;
}

/***************************************************************************/
/* ICFG callback functions */
/****************************************************************************/
#ifdef VTSS_SW_OPTION_ICFG
static mesa_rc ptp_icfg_global_conf(const vtss_icfg_query_request_t *req,
                                    vtss_icfg_query_result_t *result)
{
    char str1 [40];
    char buf1[20];
    char mep_txt[20];
    char clock_domain_txt[20];
    char dscp_txt[20];
    uint ix;
    char buf[75];

    int inst;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_appl_ptp_clock_status_default_ds_t default_ds_status;

    vtss_appl_ptp_clock_config_default_ds_t default_default_ds;
    vtss_appl_ptp_clock_status_default_ds_t default_default_ds_status;
    vtss_appl_ptp_clock_timeproperties_ds_t prop;
    vtss_appl_ptp_clock_timeproperties_ds_t default_prop;
    vtss_appl_ptp_virtual_port_config_t virtual_port_cfg;
    vtss_appl_ptp_virtual_port_config_t default_virtual_port_cfg;
    vtss_appl_ptp_clock_filter_config_t filter_params;
    vtss_appl_ptp_clock_filter_config_t default_filter_params;
    vtss_appl_ptp_clock_servo_config_t servo;
    vtss_appl_ptp_clock_servo_config_t default_servo;
    vtss_appl_ptp_unicast_slave_config_t uni_slave_cfg;
    vtss_appl_ptp_ext_clock_mode_t ext_clk_mode;
    vtss_appl_ptp_ext_clock_mode_t default_ext_clk_mode;
    vtss_appl_ptp_clock_slave_config_t slave_cfg;
    vtss_appl_ptp_clock_slave_config_t default_slave_cfg;

    vtss_serial_info_t rs422_serial_info;
    vtss_serial_info_t default_rs422_serial_info;

    ptp_clock_default_timeproperties_ds_get(&default_prop);
    vtss_appl_ptp_filter_default_parameters_get(&default_filter_params, VTSS_APPL_PTP_PROFILE_NO_PROFILE);
    vtss_appl_ptp_clock_servo_default_parameters_get(&default_servo, VTSS_APPL_PTP_PROFILE_NO_PROFILE);
    vtss_ext_clock_out_default_get(&default_ext_clk_mode);
    vtss_appl_ptp_clock_slave_default_config_get(&default_slave_cfg);
    ptp_1pps_get_default_baudrate(&default_rs422_serial_info);

    /*Set internal TC mode */
    mesa_packet_internal_tc_mode_t tc_mode;
    if (!tod_tc_mode_get(&tc_mode)) {
        tc_mode = MESA_PACKET_INTERNAL_TC_MODE_30BIT;
    }
    if (tc_mode != MESA_PACKET_INTERNAL_TC_MODE_30BIT) {
        VTSS_RC(vtss_icfg_printf(result, "ptp tc-internal mode %d\n", tc_mode));
    }

    bool phy_ts_dis = tod_board_phy_ts_dis_get();
    if (req->all_defaults) {
        if (phy_ts_dis || mepa_phy_ts_cap()) {
            VTSS_RC(vtss_icfg_printf(result, "no ptp phy-ts dis\n"));
        } else {
            VTSS_RC(vtss_icfg_printf(result, "ptp phy-ts dis\n"));
        }
    } else if (phy_ts_dis) {
        VTSS_RC(vtss_icfg_printf(result, "ptp phy-ts dis\n"));
    }

    if (mepa_phy_ts_cap()) {
        if (fast_cap(MEBA_CAP_1588_REF_CLK_SEL)) {
            /* set 1588 referene clock */
            mepa_ts_clock_freq_t freq;
    
            if (tod_ref_clock_freg_get(&freq)) {
                if (freq != MEPA_TS_CLOCK_FREQ_250M) {
                VTSS_RC(vtss_icfg_printf(result, "ptp ref-clock %s\n",
                                         freq == MEPA_TS_CLOCK_FREQ_125M ? "mhz125" :
                                         freq == MEPA_TS_CLOCK_FREQ_15625M ? "mhz156p25" : "mhz250"));
                } else if(req->all_defaults) {
                    VTSS_RC(vtss_icfg_printf(result, "no ptp ref-clock\n"));
                }
            }
        }
    }

    /* show the local clock  */
    (void) vtss_appl_ptp_ext_clock_out_get(&ext_clk_mode);
    if (ext_clk_mode.one_pps_mode != default_ext_clk_mode.one_pps_mode ||
            ext_clk_mode.clock_out_enable != default_ext_clk_mode.clock_out_enable ||
            ext_clk_mode.adj_method != default_ext_clk_mode.adj_method ||
            ext_clk_mode.freq != default_ext_clk_mode.freq ||
            ext_clk_mode.clk_domain != default_ext_clk_mode.clk_domain) {
        char pps_txt[20] = {};
        if ((fast_cap(MESA_CAP_TS_DOMAIN_CNT) > 1) && ext_clk_mode.clk_domain) {
            sprintf(pps_txt, "clk-domain %d", ext_clk_mode.clk_domain);
        }
        if (ext_clk_mode.clock_out_enable) {
            VTSS_RC(vtss_icfg_printf(result, "ptp ext %s ext %u %s %s\n",  one_pps_mode_2_string(ext_clk_mode.one_pps_mode),
                                     ext_clk_mode.freq, cli_adj_method_2_string(ext_clk_mode.adj_method), pps_txt));
        } else {
            VTSS_RC(vtss_icfg_printf(result, "ptp ext %s %s %s\n", one_pps_mode_2_string(ext_clk_mode.one_pps_mode),
                                     cli_adj_method_2_string(ext_clk_mode.adj_method), pps_txt));
        }
    } else if (req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, "no ptp ext\n"));
    }

    for (inst = 0 ; inst < PTP_CLOCK_INSTANCES; inst++) {
        if ((vtss_appl_ptp_clock_config_default_ds_get(inst, &default_ds_cfg) == VTSS_RC_OK) &&
            (vtss_appl_ptp_clock_status_default_ds_get(inst, &default_ds_status) == VTSS_RC_OK))
        {
            ptp_get_default_clock_default_ds(&default_default_ds_status, &default_default_ds);

            if (default_ds_cfg.deviceType != default_default_ds.deviceType) {
                ptp_apply_profile_defaults_to_default_ds(&default_default_ds, default_ds_cfg.profile);
                sprintf(mep_txt, " mep %d", default_ds_cfg.mep_instance);
                sprintf(clock_domain_txt, " clock-domain %d", default_ds_cfg.clock_domain);
                sprintf(dscp_txt, " dscp %d", default_ds_cfg.dscp);
                VTSS_RC(vtss_icfg_printf(result, "ptp %d mode %s %s %s %s id %s vid %d %d%s%s%s%s%s\n",
                                             inst,
                                             device_type_2_string(default_ds_cfg.deviceType),
                                             default_ds_cfg.twoStepFlag ? "twostep" : "onestep",
                                             protocol_2_string(default_ds_cfg.protocol),
                                             default_ds_cfg.oneWay ? "oneway" : "twoway",
                                             ClockIdentityToString(default_ds_status.clockIdentity, str1),
                                             default_ds_cfg.configured_vid,
                                             default_ds_cfg.configured_pcp,
                                             default_ds_cfg.profile != VTSS_APPL_PTP_PROFILE_NO_PROFILE ? " profile " : "",
                                             default_ds_cfg.profile != VTSS_APPL_PTP_PROFILE_NO_PROFILE ? ClockProfileToString(default_ds_cfg.profile) : "",
                                             default_ds_cfg.mep_instance ? mep_txt : "",
                                             (default_ds_cfg.clock_domain != inst) ? clock_domain_txt : "",
                                             (default_ds_cfg.dscp != default_default_ds.dscp) ? dscp_txt : "" ));
                if ((default_ds_cfg.priority1 != default_default_ds.priority1) || (req->all_defaults)) {
                    VTSS_RC(vtss_icfg_printf(result, "ptp %d priority1 %d\n",
                                             inst, default_ds_cfg.priority1));
                }
                if ((default_ds_cfg.priority2 != default_default_ds.priority2) || (req->all_defaults)) {
                    VTSS_RC(vtss_icfg_printf(result, "ptp %d priority2 %d\n",
                                             inst, default_ds_cfg.priority2));
                }
                if ((default_ds_cfg.domainNumber != default_default_ds.domainNumber) || (req->all_defaults)) {
                    VTSS_RC(vtss_icfg_printf(result, "ptp %d domain %d\n",
                                             inst, default_ds_cfg.domainNumber));
                }
                if ((default_ds_cfg.localPriority != default_default_ds.localPriority) || (req->all_defaults)) {
                    if (default_ds_cfg.localPriority != default_default_ds.localPriority) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d localpriority %d\n", inst, default_ds_cfg.localPriority));
                    } else {
                        VTSS_RC(vtss_icfg_printf(result, "no ptp %d localpriority\n", inst));
                    }
                }
                {
                    VTSS_RC(vtss_icfg_printf(result, " ptp %d filter-type %s\n",
                                             inst,
                                             filter_type_2_string(default_ds_cfg.filter_type)));
                }
                if ((default_ds_cfg.path_trace_enable != default_default_ds.path_trace_enable) || (req->all_defaults)) {
                    if (default_ds_cfg.path_trace_enable) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d path-trace-enable\n", inst));
                    } else {
                        VTSS_RC(vtss_icfg_printf(result, "no ptp %d path-trace-enable\n", inst));
                    }
                }
                if (vtss_appl_ptp_clock_config_timeproperties_ds_get(inst, &prop) == VTSS_RC_OK) {
                    char leap_cfg_string[58];
                    if (prop.pendingLeap) {
                        struct tm* ptm;
                        struct tm timeinfo;
                        time_t rawtime = (time_t) prop.leapDate * 86400;
                        ptm = gmtime_r(&rawtime, &timeinfo);
                        snprintf(leap_cfg_string, sizeof(leap_cfg_string), " leap-pending %04d-%02d-%02d leap-%s",
                                 ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, (prop.leapType == VTSS_APPL_PTP_LEAP_SECOND_59) ? "59" : "61");
                    }
                    else {
                        strcpy(leap_cfg_string, "");
                    }
                    if (cmp_time_prop(&default_prop,&prop) || req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d time-property utc-offset %d %s%s%s%s%stime-source %u%s\n",
                                                 inst,
                                                 prop.currentUtcOffset,
                                                 prop.currentUtcOffsetValid ? "valid " : "",
                                                 prop.leap59 ? "leap-59 " : prop.leap61 ? "leap-61 " : "",
                                                 prop.timeTraceable ? "time-traceable " : "",
                                                 prop.frequencyTraceable ? "freq-traceable " : "",
                                                 prop.ptpTimescale ? "ptptimescale " : "",
                                                 prop.timeSource,
                                                 leap_cfg_string));
                    }
                }
                if (default_ds_cfg.deviceType == VTSS_APPL_PTP_DEVICE_INTERNAL) {
                    ptp_internal_mode_config_t in_cfg;
                    int srcClkDom = vtss_local_clock_src_clk_domain_get(inst);
                    if (srcClkDom != VTSS_PTP_SRC_CLK_DOMAIN_NONE) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d internal src-clk-domain %d\n", inst, srcClkDom));
                    }
                    ptp_internal_mode_config_get(inst, &in_cfg);
                    if (in_cfg.sync_rate != DEFAULT_INTERNAL_MODE_SYNC_RATE) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d internal sync-rate %d\n", inst, in_cfg.sync_rate));
                    }
                }
                if (ptp_clock_config_virtual_port_config_get(inst, &virtual_port_cfg) == VTSS_RC_OK) {
                    vtss_appl_ptp_clock_config_default_virtual_port_config_get(&default_virtual_port_cfg);
                    if (default_virtual_port_cfg.virtual_port_mode != virtual_port_cfg.virtual_port_mode) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port mode %s", inst, virtual_port_mode_2_string(virtual_port_cfg.virtual_port_mode)));
                        if (virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN ||virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT){
                            VTSS_RC(vtss_icfg_printf(result, " %d",(virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN) ? virtual_port_cfg.input_pin : virtual_port_cfg.output_pin));
                        }
                        if ((virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB ||
                             virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN)
                             && virtual_port_cfg.delay) {
                            VTSS_RC(vtss_icfg_printf(result, " pps-delay %d\n", virtual_port_cfg.delay));
                        } else {
                            VTSS_RC(vtss_icfg_printf(result, "\n"));
                        }
                    } else if (req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result,"no ptp %d virtual-port mode\n",inst));
                    }

                    if (default_virtual_port_cfg.proto != virtual_port_cfg.proto) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port tod",inst));
                        if ( virtual_port_cfg.proto == VTSS_PTP_APPL_RS422_PROTOCOL_PIM){
                             VTSS_RC(vtss_icfg_printf(result, " pim interface %s\n",icli_port_info_txt(VTSS_USID_START, iport2uport(virtual_port_cfg.portnum), buf)));
                        } else {
                             VTSS_RC(vtss_icfg_printf(result, " %s\n",rs422_protomode_2_string(virtual_port_cfg.proto)));
                        }
                    } else if (req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result,"no ptp %d virtual-port tod\n",inst));
                    }

                    if (default_virtual_port_cfg.clockQuality.clockClass != virtual_port_cfg.clockQuality.clockClass || req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port class %d\n", inst, virtual_port_cfg.clockQuality.clockClass));
                    }
                    if (default_virtual_port_cfg.clockQuality.clockAccuracy != virtual_port_cfg.clockQuality.clockAccuracy || req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port accuracy %d\n", inst, virtual_port_cfg.clockQuality.clockAccuracy));
                    }
                    if (default_virtual_port_cfg.clockQuality.offsetScaledLogVariance != virtual_port_cfg.clockQuality.offsetScaledLogVariance || req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port variance %d\n", inst, virtual_port_cfg.clockQuality.offsetScaledLogVariance));
                    }
                    if ((default_virtual_port_cfg.localPriority != virtual_port_cfg.localPriority) || req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port local-priority %d\n", inst, virtual_port_cfg.localPriority));
                    }
                    if ((default_virtual_port_cfg.priority1 != virtual_port_cfg.priority1) || req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port priority1 %d\n", inst, virtual_port_cfg.priority1));
                    }
                    if ((default_virtual_port_cfg.priority2 != virtual_port_cfg.priority2) || req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port priority2 %d\n", inst, virtual_port_cfg.priority2));
                    }
                    if ((default_virtual_port_cfg.steps_removed != virtual_port_cfg.steps_removed) || req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port steps-removed %d\n", inst, virtual_port_cfg.steps_removed));
					}
                    if (memcmp(&default_virtual_port_cfg.clock_identity , &virtual_port_cfg.clock_identity,sizeof(virtual_port_cfg.clock_identity)) || req->all_defaults) {
                        char clk_id[24]={'\0'};
                            sprintf(clk_id,"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                            virtual_port_cfg.clock_identity[0],virtual_port_cfg.clock_identity[1],
                            virtual_port_cfg.clock_identity[2],virtual_port_cfg.clock_identity[3],
                            virtual_port_cfg.clock_identity[4],virtual_port_cfg.clock_identity[5],
                            virtual_port_cfg.clock_identity[6],virtual_port_cfg.clock_identity[7]);

                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port clock-identity %s\n", inst, clk_id));
					}
                    if ((default_virtual_port_cfg.alarm != virtual_port_cfg.alarm) || req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port alarm enable\n", inst));
					}
                    if (cmp_time_prop(&default_virtual_port_cfg.timeproperties, &virtual_port_cfg.timeproperties) || req->all_defaults) {
                        char leap_cfg_string[58];
                        vtss_appl_ptp_clock_timeproperties_ds_t *prop_ptr = &virtual_port_cfg.timeproperties;
                        if (prop_ptr->pendingLeap) {
                            struct tm* ptm;
                            struct tm timeinfo;
                            time_t rawtime = (time_t) prop_ptr->leapDate * 86400;
                            ptm = gmtime_r(&rawtime, &timeinfo);
                            snprintf(leap_cfg_string, sizeof(leap_cfg_string), " leap-pending %04d-%02d-%02d leap-%s",
                                     ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, (prop_ptr->leapType == VTSS_APPL_PTP_LEAP_SECOND_59) ? "59" : "61");
                        }
                        else {
                            strcpy(leap_cfg_string, "");
                        }
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d virtual-port time-property utc-offset %d %s%s%s%s%stime-source %u%s\n",
                                                 inst,
                                                 prop_ptr->currentUtcOffset,
                                                 prop_ptr->currentUtcOffsetValid ? "valid " : "",
                                                 prop_ptr->leap59 ? "leap-59 " : prop_ptr->leap61 ? "leap-61 " : "",
                                                 prop_ptr->timeTraceable ? "time-traceable " : "",
                                                 prop_ptr->frequencyTraceable ? "freq-traceable " : "",
                                                 prop_ptr->ptpTimescale ? "ptptimescale " : "",
                                                 prop_ptr->timeSource,
                                                 leap_cfg_string));
                    }
                }
                if ((default_ds_cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_BASIC) && (vtss_appl_ptp_clock_filter_parameters_get(inst, &filter_params) == VTSS_RC_OK)) {
                    if (memcmp(&default_filter_params, &filter_params, sizeof(filter_params)) || req->all_defaults) {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d filter delay %d period %d dist %d\n",
                                                 inst,
                                                 filter_params.delay_filter,
                                                 filter_params.period,
                                                 filter_params.dist));
                    }
                }
                if (vtss_appl_ptp_clock_servo_parameters_get(inst, &servo) == VTSS_RC_OK) {
                    if (servo.display_stats != default_servo.display_stats || req->all_defaults) {
                        if (servo.display_stats) {
                            VTSS_RC(vtss_icfg_printf(result, "ptp %d servo displaystates\n", inst));
                        } else {
                            VTSS_RC(vtss_icfg_printf(result, "no ptp %d servo displaystates\n", inst));
                        }
                    }
                    if (servo.srv_option != default_servo.srv_option || servo.synce_threshold != default_servo.synce_threshold ||
                            servo.synce_ap != default_servo.synce_ap || req->all_defaults) {
                        if (servo.srv_option) {
                            VTSS_RC(vtss_icfg_printf(result, "ptp %d clk sync %d ap %d\n", inst, servo.synce_threshold, servo.synce_ap ));
                        } else {
                            VTSS_RC(vtss_icfg_printf(result, "no ptp %d clk\n", inst));
                        }
                    }
                    ix = 0;
                    while (vtss_appl_ptp_clock_config_unicast_slave_config_get(inst, ix++, &uni_slave_cfg) == VTSS_RC_OK) {
                        if (uni_slave_cfg.ip_addr) {
                            VTSS_RC(vtss_icfg_printf(result, "ptp %d uni %d duration %d %s\n", inst, ix-1, uni_slave_cfg.duration, misc_ipv4_txt(uni_slave_cfg.ip_addr, buf1) ));
                        } else if (req->all_defaults) {
                            VTSS_RC(vtss_icfg_printf(result, "no ptp %d uni %d\n", inst, ix-1));
                        }
                    }
                    if (default_ds_cfg.filter_type == VTSS_APPL_PTP_FILTER_TYPE_BASIC) {
                        if (servo.p_reg != default_servo.p_reg || servo.ap != default_servo.ap || req->all_defaults) {
                            if (servo.p_reg) {
                                VTSS_RC(vtss_icfg_printf(result, "ptp %d servo ap %d\n", inst, servo.ap));
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, "no ptp %d servo ap\n", inst));
                            }
                        }
                        if (servo.i_reg != default_servo.i_reg || servo.ai != default_servo.ai || req->all_defaults) {
                            if (servo.i_reg) {
                                VTSS_RC(vtss_icfg_printf(result, "ptp %d servo ai %d\n", inst, servo.ai));
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, "no ptp %d servo ai\n", inst));
                            }
                        }
                        if (servo.d_reg != default_servo.d_reg || servo.ad != default_servo.ad || req->all_defaults) {
                            if (servo.d_reg) {
                                VTSS_RC(vtss_icfg_printf(result, "ptp %d servo ad %d\n", inst, servo.ad));
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, "no ptp %d servo ad\n", inst));
                            }
                        }
                        if (servo.gain != default_servo.gain || req->all_defaults) {
                            VTSS_RC(vtss_icfg_printf(result, "ptp %d servo gain %d\n", inst, servo.gain));
                        }
                        if (servo.ho_filter != default_servo.ho_filter || servo.stable_adj_threshold != default_servo.stable_adj_threshold ||
                                req->all_defaults) {
                            VTSS_RC(vtss_icfg_printf(result, "ptp %d ho filter %d adj-threshold %d\n", inst, servo.ho_filter, (i32)servo.stable_adj_threshold ));
                        }
                    }
                }
                if (vtss_appl_ptp_clock_slave_config_get(inst, &slave_cfg) == VTSS_RC_OK) {
                    if (slave_cfg.offset_fail != default_slave_cfg.offset_fail ||
                        slave_cfg.offset_ok != default_slave_cfg.offset_ok ||
                        slave_cfg.stable_offset != default_slave_cfg.stable_offset ||
                        req->all_defaults)
                    {
                        VTSS_RC(vtss_icfg_printf(result, "ptp %d slave-cfg offset-fail %d offset-ok %d stable-offset %d\n", inst, slave_cfg.offset_fail, slave_cfg.offset_ok, slave_cfg.stable_offset));
                    }
                }
                
                if (fast_cap(MESA_CAP_SYNCE_ANN_AUTO_TRANSMIT)) {
                    bool afi_enable;
                    if (ptp_afi_mode_get(inst, true, &afi_enable) == VTSS_RC_OK) {
                        if (!afi_enable) {
                            VTSS_RC(vtss_icfg_printf(result, "no ptp %d afi-announce\n", inst));
                        }
                    }
                    if (ptp_afi_mode_get(inst, false, &afi_enable) == VTSS_RC_OK) {
                        if (!afi_enable) {
                            VTSS_RC(vtss_icfg_printf(result, "no ptp %d afi-sync\n", inst));
                        }
                    }
                }
                
            } else if (req->all_defaults) {
                VTSS_RC(vtss_icfg_printf(result, "no ptp %d mode %s\n",
                                         inst,
                                         device_type_2_string(default_ds_cfg.deviceType)));
            }
        }
    }

    /* holdover spec */
    {
        vtss_appl_ho_spec_conf_t ho_spec;
        vtss_ho_spec_conf_get(&ho_spec);
        if ((ho_spec.cat1 != 0 || ho_spec.cat2 != 0 || ho_spec.cat3 != 0) || (req->all_defaults)) {
            if (ho_spec.cat1 != 0 || ho_spec.cat2 != 0 || ho_spec.cat3 != 0) {
                VTSS_RC(vtss_icfg_printf(result, "ptp ho-spec cat1 %d cat2 %d cat3 %d\n", ho_spec.cat1, ho_spec.cat2, ho_spec.cat3));
            } else {
                VTSS_RC(vtss_icfg_printf(result, "no ptp ho-spec\n"));
            }
        }
    }

    /* global config */
    /* set System time <-> PTP time synchronization mode */
    vtss_appl_ptp_system_time_sync_conf_t conf;

    VTSS_RC(vtss_appl_ptp_system_time_sync_mode_get(&conf));
    if (conf.mode != VTSS_APPL_PTP_SYSTEM_TIME_NO_SYNC) {
        VTSS_RC(vtss_icfg_printf(result, "ptp system-time %s %d\n",
                                 conf.mode == VTSS_APPL_PTP_SYSTEM_TIME_SYNC_GET ? "get" : "set",
				 conf.clockinst)
);
    } else if(req->all_defaults) {
        VTSS_RC(vtss_icfg_printf(result, "no ptp system-time\n"));
    }

    if (fast_cap(MESA_CAP_TS_PTP_RS422)) {
        if (ptp_1pps_get_baudrate(&rs422_serial_info) == VTSS_RC_OK) {
            if (memcmp(&rs422_serial_info, &default_rs422_serial_info, sizeof(vtss_serial_info_t)) != 0 || req->all_defaults) {
                VTSS_RC(vtss_icfg_printf(result, "ptp rs422 baudrate %s parity %s wordlength %s stopbits %s flowctrl %s\n",
                                         cli_rs422_baudrate_2_string(rs422_serial_info.baud),
                                         cli_rs422_parity_2_string(rs422_serial_info.parity),
                                         cli_rs422_word_length_2_string(rs422_serial_info.word_length),
                                         cli_rs422_stop_2_string(rs422_serial_info.stop),
                                         cli_rs422_flags_2_string(rs422_serial_info.flags)));
            }
        }
    }

    return VTSS_RC_OK;
}

static mesa_rc ptp_icfg_interface_conf(const vtss_icfg_query_request_t *req,
                                       vtss_icfg_query_result_t *result)
{
    int inst;
    vtss_appl_ptp_config_port_ds_t ps;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_isid_t isid;
    mesa_port_no_t uport;
    vtss_ifindex_t ifindex;
    char int_txt[10];
    vtss_1pps_sync_conf_t pps_sync_mode;
    vtss_1pps_closed_loop_conf_t pps_cl_mode;
    mesa_port_no_t iport;
    char buf[75]; // Buffer for storage of string

    isid = req->instance_id.port.isid;
    uport = req->instance_id.port.begin_uport;
    (void) vtss_ifindex_from_port(isid, uport2iport(uport), &ifindex);
    T_IG(VTSS_TRACE_GRP_PTP_ICLI, "isid %d, port %d", isid, uport);
    if (msg_switch_configurable(isid)) {
        for (inst = 0 ; inst < PTP_CLOCK_INSTANCES; inst++) {
            if (vtss_appl_ptp_clock_config_default_ds_get(inst, &default_ds_cfg) == VTSS_RC_OK) {
                if (default_ds_cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) {
                    if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &ps) == VTSS_RC_OK) && (ps.enabled == 1)) {
                        VTSS_RC(vtss_icfg_printf(result, " ptp %d %s\n",
                                                 inst,
                                                 ps.portInternal ? "internal" : ""));
                        if (ps.logAnnounceInterval == 126) {
                            snprintf(int_txt, sizeof(int_txt), "default");
                        } else if (ps.logAnnounceInterval == 127) {
                            snprintf(int_txt, sizeof(int_txt), "stop");
                        } else {
                            snprintf(int_txt, sizeof(int_txt),"%d", ps.logAnnounceInterval);
                        }
                        VTSS_RC(vtss_icfg_printf(result, " ptp %d announce interval %s timeout %d\n",
                                                 inst,
                                                 int_txt,
                                                 ps.announceReceiptTimeout));
                        if (ps.logSyncInterval == 126) {
                            snprintf(int_txt, sizeof(int_txt), "default");
                        } else if (ps.logSyncInterval == 127) {
                            snprintf(int_txt, sizeof(int_txt), "stop");
                        } else {
                            snprintf(int_txt, sizeof(int_txt),"%hhd", ps.logSyncInterval);
                        }
                        VTSS_RC(vtss_icfg_printf(result, " ptp %d sync-interval %s\n",
                                                 inst,
                                                 int_txt));
                        VTSS_RC(vtss_icfg_printf(result, " ptp %d delay-mechanism %s\n",
                                                 inst,
                                                 ps.delayMechanism == VTSS_APPL_PTP_DELAY_MECH_E2E ? "e2e" : (ps.delayMechanism == VTSS_APPL_PTP_DELAY_MECH_P2P ? "p2p": "common-p2p")));
                        if (ps.logMinPdelayReqInterval == 126) {
                            snprintf(int_txt, sizeof(int_txt), "default");
                        } else if (ps.logMinPdelayReqInterval == 127) {
                            snprintf(int_txt, sizeof(int_txt), "stop");
                        } else {
                            snprintf(int_txt, sizeof(int_txt),"%d", ps.logMinPdelayReqInterval);
                        }
                        VTSS_RC(vtss_icfg_printf(result, " ptp %d delay-req interval %s\n",
                                                 inst,
                                                 int_txt));
                        VTSS_RC(vtss_icfg_printf(result, " ptp %d delay-asymmetry %d\n",
                                                 inst,
                                                 VTSS_INTERVAL_NS(ps.delayAsymmetry)));
                        VTSS_RC(vtss_icfg_printf(result, " ptp %d ingress-latency %d\n",
                                                 inst,
                                                 VTSS_INTERVAL_NS(ps.ingressLatency)));
                        VTSS_RC(vtss_icfg_printf(result, " ptp %d egress-latency %d\n",
                                                 inst,
                                                 VTSS_INTERVAL_NS(ps.egressLatency)));
                        if (ps.localPriority != 128 || req->all_defaults) {
                            if (ps.localPriority != 128) {
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d localpriority %d\n", inst, ps.localPriority));
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, " no ptp %d localpriority\n", inst));
                            }
                        }
                        if (ps.dest_adr_type != VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT || req->all_defaults) {
                            if (ps.dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_LINK_LOCAL) {
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d mcast-dest link-local\n", inst));
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d mcast-dest default\n", inst));
                            }
                        }
                        if (ps.masterOnly || req->all_defaults) {
                            if (ps.masterOnly) {
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d master-only\n", inst));
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, " no ptp %d master-only\n", inst));
                            }
                        }
                        if (ps.notMaster || req->all_defaults) {
                            if (ps.notMaster) {
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d not-master\n", inst));
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, " no ptp %d not-master\n", inst));
                            }
                        }
                        if (ps.twoStepOverride != VTSS_APPL_PTP_TWO_STEP_OVERRIDE_NONE || req->all_defaults) {
                            if (ps.twoStepOverride != VTSS_APPL_PTP_TWO_STEP_OVERRIDE_NONE) {
                                if (ps.twoStepOverride == VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE) {
                                    VTSS_RC(vtss_icfg_printf(result, " ptp %d two-step true\n", inst));
                                } else {
                                    VTSS_RC(vtss_icfg_printf(result, " ptp %d two-step false\n", inst));
                                }
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, " no ptp %d two-step\n", inst));
                            }
                        }
                        //port_delay_threshold
                        if ((default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) && (ps.c_802_1as.peer_d.meanLinkDelayThresh != (800LL<<16) || req->all_defaults)) {
                            if (ps.c_802_1as.peer_d.meanLinkDelayThresh != VTSS_MAX_TIMEINTERVAL) {
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d delay-thresh %d\n", inst, (u32)(ps.c_802_1as.peer_d.meanLinkDelayThresh>>16)));
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, " no ptp %d delay-thresh\n", inst));
                            }
                        }
                        //sync rx timeout
                        if ((default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) && (ps.c_802_1as.syncReceiptTimeout != 3 || req->all_defaults)) {
                            if (ps.c_802_1as.syncReceiptTimeout != 3) {
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d sync-rx-to %d\n", inst, ps.c_802_1as.syncReceiptTimeout));
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, " no ptp %d sync-rx-to\n", inst));
                            }
                        }
                        //max allowed lost PDelay responses
                        if ((default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) && (ps.c_802_1as.peer_d.allowedLostResponses != DEFAULT_MAX_CONTIGOUS_MISSED_PDELAY_RESPONSE || req->all_defaults)) {
                            if (ps.c_802_1as.peer_d.allowedLostResponses != DEFAULT_MAX_CONTIGOUS_MISSED_PDELAY_RESPONSE) {
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d allow-lost-resp %d\n", inst, ps.c_802_1as.peer_d.allowedLostResponses));
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, " no ptp %d allow-lost-resp\n", inst));
                            }
                        }
                        //management settable sync-interval
                        if (default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                            if (ps.c_802_1as.mgtSettableLogSyncInterval == 126) {
                                snprintf(int_txt, sizeof(int_txt), "default");
                            } else if (ps.c_802_1as.mgtSettableLogSyncInterval == 127) {
                                snprintf(int_txt, sizeof(int_txt), "stop");
                            } else {
                                snprintf(int_txt, sizeof(int_txt),"%d", ps.c_802_1as.mgtSettableLogSyncInterval);
                            }
                            if (ps.c_802_1as.useMgtSettableLogSyncInterval == TRUE)
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d mgtSettableLogSyncInterval %s\n",
                                                 inst,
                                                 int_txt));
                            if (ps.c_802_1as.mgtSettableLogAnnounceInterval == 126) {
                                snprintf(int_txt, sizeof(int_txt), "default");
                            } else if (ps.c_802_1as.mgtSettableLogAnnounceInterval == 127) {
                                snprintf(int_txt, sizeof(int_txt), "stop");
                            } else {
                                snprintf(int_txt, sizeof(int_txt),"%d", ps.c_802_1as.mgtSettableLogAnnounceInterval);
                            }
                            if (ps.c_802_1as.useMgtSettableLogAnnounceInterval == TRUE)
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d mgtSettableLogAnnounceInterval %s\n",
                                                 inst,
                                                 int_txt));
                            if (ps.c_802_1as.peer_d.mgtSettableLogPdelayReqInterval == 126) {
                                snprintf(int_txt, sizeof(int_txt), "default");
                            } else if (ps.c_802_1as.peer_d.mgtSettableLogPdelayReqInterval == 127) {
                                snprintf(int_txt, sizeof(int_txt), "stop");
                            } else {
                                snprintf(int_txt, sizeof(int_txt),"%d", ps.c_802_1as.peer_d.mgtSettableLogPdelayReqInterval);
                            }
                            if (ps.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval == TRUE)
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d mgtSettableLogPdelayReqInterval %s\n",
                                                 inst,
                                                 int_txt));
                            if (ps.c_802_1as.mgtSettableLogGptpCapableMessageInterval == 126) {
                                snprintf(int_txt, sizeof(int_txt), "default");
                            } else if (ps.c_802_1as.mgtSettableLogGptpCapableMessageInterval == 127) {
                                snprintf(int_txt, sizeof(int_txt), "stop");
                            } else {
                                snprintf(int_txt, sizeof(int_txt),"%d", ps.c_802_1as.mgtSettableLogGptpCapableMessageInterval);
                            }
                            if (ps.c_802_1as.useMgtSettableLogGptpCapableMessageInterval == TRUE)
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d mgtSettableLogGptpCapableMessageInterval %s\n",
                                                 inst,
                                                 int_txt));
                            if ((ps.c_802_1as.useMgtSettableLogSyncInterval == TRUE) || req->all_defaults) {
                                if (ps.c_802_1as.useMgtSettableLogSyncInterval == TRUE) {
                                    VTSS_RC(vtss_icfg_printf(result, " ptp %d usemgtSettableLogSyncInterval 1\n", inst));
                                } else {
                                    VTSS_RC(vtss_icfg_printf(result, " ptp %d usemgtSettableLogSyncInterval 0\n", inst));
                                }
                            }
                            if ((ps.c_802_1as.useMgtSettableLogAnnounceInterval == TRUE) || req->all_defaults) {
                                if (ps.c_802_1as.useMgtSettableLogAnnounceInterval == TRUE) {
                                    VTSS_RC(vtss_icfg_printf(result, " ptp %d usemgtSettableLogAnnounceInterval 1\n", inst));
                                } else {
                                    VTSS_RC(vtss_icfg_printf(result, " ptp %d usemgtSettableLogAnnounceInterval 0\n", inst));
                                }
                            }
                            if ((ps.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval == TRUE) || req->all_defaults) {
                                if (ps.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval == TRUE) {
                                    VTSS_RC(vtss_icfg_printf(result, " ptp %d usemgtSettableLogPdelayReqInterval 1\n", inst));
                                } else {
                                    VTSS_RC(vtss_icfg_printf(result, " ptp %d usemgtSettableLogPdelayReqInterval 0\n", inst));
                                }
                            }
                            if ((ps.c_802_1as.useMgtSettableLogGptpCapableMessageInterval == TRUE) || req->all_defaults) {
                                if (ps.c_802_1as.useMgtSettableLogGptpCapableMessageInterval == TRUE) {
                                    VTSS_RC(vtss_icfg_printf(result, " ptp %d useMgtSettableLogGptpCapableMessageInterval 1\n", inst));
                                } else {
                                    VTSS_RC(vtss_icfg_printf(result, " ptp %d useMgtSettableLogGptpCapableMessageInterval 0\n", inst));
                                }
                            }
                            if (ps.c_802_1as.initialLogGptpCapableMessageInterval == 126) {
                                snprintf(int_txt, sizeof(int_txt), "default");
                            } else if (ps.c_802_1as.initialLogGptpCapableMessageInterval == 127) {
                                snprintf(int_txt, sizeof(int_txt), "stop");
                            } else {
                                snprintf(int_txt, sizeof(int_txt),"%d", ps.c_802_1as.initialLogGptpCapableMessageInterval);
                            }
                            VTSS_RC(vtss_icfg_printf(result, " ptp %d gptp-interval %s\n",
                                                 inst,
                                                 int_txt));
                            if (ps.c_802_1as.gPtpCapableReceiptTimeout != DEFAULT_GPTP_CAPABLE_RECEIPT_TIMEOUT || req->all_defaults) {
                                if (ps.c_802_1as.gPtpCapableReceiptTimeout != DEFAULT_GPTP_CAPABLE_RECEIPT_TIMEOUT) {
                                    VTSS_RC(vtss_icfg_printf(result, " ptp %d gptp-to %d\n", inst, ps.c_802_1as.gPtpCapableReceiptTimeout));
                                } else {
                                    VTSS_RC(vtss_icfg_printf(result, " no ptp %d gptp-to\n", inst));
                                }
                            }
                            if (!ps.c_802_1as.as2020) {
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d 802.1as 2011\n", inst));
                            }
                            if (default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d aed-port-role %s\n",inst, ps.aedPortState == VTSS_APPL_PTP_PORT_STATE_AED_MASTER ? "master" : "slave"));
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d aed-intervals operPdelayReq %d\n", inst, ps.c_802_1as.peer_d.operLogPdelayReqInterval));
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d aed-intervals initSync %d\n", inst, ps.c_802_1as.initialLogSyncInterval));
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d aed-intervals operSync %d\n", inst, ps.c_802_1as.operLogSyncInterval));
                            }
                        }
                        //max allowed faults in 802.1as Pdelay mechanism
                        if ((default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) && 
                        (ps.c_802_1as.peer_d.allowedFaults != DEFAULT_MAX_PDELAY_ALLOWED_FAULTS || req->all_defaults)) {
                            if (ps.c_802_1as.peer_d.allowedFaults != DEFAULT_MAX_PDELAY_ALLOWED_FAULTS) {
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d allow-faults %d\n", inst, ps.c_802_1as.peer_d.allowedFaults));
                            } else {
                                VTSS_RC(vtss_icfg_printf(result, " no ptp %d allow-faults\n", inst));
                            }
                        }
                        if ((default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) && 
                        (!ps.c_802_1as.peer_d.mgtSettableComputeNeighborRateRatio || ps.c_802_1as.peer_d.useMgtSettableComputeNeighborRateRatio ||req->all_defaults)) {
                            if (!ps.c_802_1as.peer_d.mgtSettableComputeNeighborRateRatio) {
                                VTSS_RC(vtss_icfg_printf(result, " no ptp %d compute-neighbor-rate-ratio %s\n", inst, ps.c_802_1as.peer_d.useMgtSettableComputeNeighborRateRatio? "force":""));
                            } else{
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d compute-neighbor-rate-ratio %s\n", inst, ps.c_802_1as.peer_d.useMgtSettableComputeNeighborRateRatio? "force":""));
                            }
                        }
                        if ((default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_IEEE_802_1AS || default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) && 
                        (!ps.c_802_1as.peer_d.mgtSettableComputeMeanLinkDelay || ps.c_802_1as.peer_d.useMgtSettableComputeMeanLinkDelay || req->all_defaults)) {
                            if (!ps.c_802_1as.peer_d.mgtSettableComputeMeanLinkDelay) {
                                VTSS_RC(vtss_icfg_printf(result, " no ptp %d compute-meanlinkdelay %s\n", inst, ps.c_802_1as.peer_d.useMgtSettableComputeMeanLinkDelay ? "force":""));
                            } else{
                                VTSS_RC(vtss_icfg_printf(result, " ptp %d compute-meanlinkdelay %s\n", inst, ps.c_802_1as.peer_d.useMgtSettableComputeMeanLinkDelay ? "force":""));
                            }
                        }
                    }
                } else if (req->all_defaults) {
                    VTSS_RC(vtss_icfg_printf(result, " no ptp %d\n",
                                             inst));
                }
            }
        }
        if (mepa_phy_ts_cap()) {
            /* one pps configuration functions */
            iport = req->instance_id.port.begin_iport;
            VTSS_RC(ptp_1pps_sync_mode_get(iport, &pps_sync_mode));
            if (pps_sync_mode.mode != VTSS_PTP_1PPS_SYNC_DISABLE) {
                VTSS_RC(vtss_icfg_printf(result, " ptp pps-sync %s cable-asy %d pps-phase %d %s\n",
                                         cli_pps_sync_mode_2_string(pps_sync_mode.mode),
                                         pps_sync_mode.cable_asy,
                                         pps_sync_mode.pulse_delay,
                                         cli_pps_ser_tod_2_string(pps_sync_mode.serial_tod)));
            } else if (req->all_defaults) {
                VTSS_RC(vtss_icfg_printf(result, " no ptp pps-sync\n"));
            }
            VTSS_RC(ptp_1pps_closed_loop_mode_get(iport, &pps_cl_mode));
            if (pps_cl_mode.mode == VTSS_PTP_1PPS_CLOSED_LOOP_AUTO) {
                VTSS_RC(vtss_icfg_printf(result, " ptp pps-delay auto master-port interface %s\n",
                                         icli_port_info_txt(VTSS_USID_START, iport2uport(pps_cl_mode.master_port), buf)));
            } else if (pps_cl_mode.mode == VTSS_PTP_1PPS_CLOSED_LOOP_MAN) {
                    VTSS_RC(vtss_icfg_printf(result, " ptp pps-delay man cable-delay %d\n",
                                             pps_cl_mode.cable_delay));
            } else if (req->all_defaults) {
                VTSS_RC(vtss_icfg_printf(result, " no ptp pps-delay\n"));
            }
        }
#if defined (VTSS_SW_OPTION_P802_1_AS)
        vtss_appl_ptp_802_1as_cmlds_status_port_ds_t cmlds_ds;
        if ((vtss_appl_ptp_cmlds_port_status_get(uport, &cmlds_ds) == VTSS_RC_OK)
                                              && cmlds_ds.cmldsLinkPortEnabled) {
            vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t conf, def;
            VTSS_RC(vtss_appl_ptp_cmlds_port_conf_get(uport, &conf));
            vtss_appl_ptp_cmlds_conf_defaults_get(&def);
            if (conf.peer_d.meanLinkDelayThresh != def.peer_d.meanLinkDelayThresh || req->all_defaults) {
                if (conf.peer_d.meanLinkDelayThresh != VTSS_MAX_TIMEINTERVAL) {
                    VTSS_RC(vtss_icfg_printf(result, " ptp cmlds pdelay-thresh %d\n", (u32)conf.peer_d.meanLinkDelayThresh>>16));
                } else {
                    VTSS_RC(vtss_icfg_printf(result, " no ptp cmlds pdelay-thresh\n"));
                }
            }
            if (conf.peer_d.mgtSettableLogPdelayReqInterval != def.peer_d.mgtSettableLogPdelayReqInterval) {
                if (conf.peer_d.mgtSettableLogPdelayReqInterval == 126) {
                    snprintf(int_txt, sizeof(int_txt), "default");
                } else if (conf.peer_d.mgtSettableLogPdelayReqInterval == 127) {
                    snprintf(int_txt, sizeof(int_txt), "stop");
                } else {
                    snprintf(int_txt, sizeof(int_txt),"%d", conf.peer_d.mgtSettableLogPdelayReqInterval);
                }
                VTSS_RC(vtss_icfg_printf(result, " ptp cmlds pdelayreq-interval %s %s\n",
                        int_txt, conf.peer_d.useMgtSettableLogPdelayReqInterval?"force":""));
            } else if (req->all_defaults) {
                VTSS_RC(vtss_icfg_printf(result, " no ptp cmlds pdelayreq-interval\n"));
            }
            if ((conf.peer_d.mgtSettableComputeMeanLinkDelay != def.peer_d.mgtSettableComputeMeanLinkDelay) ||
                (conf.peer_d.useMgtSettableComputeMeanLinkDelay != def.peer_d.useMgtSettableComputeMeanLinkDelay) || req->all_defaults) {
                if (!conf.peer_d.mgtSettableComputeMeanLinkDelay) {
                    VTSS_RC(vtss_icfg_printf(result, " no ptp cmlds compute-meanlinkdelay %s\n",
                                             conf.peer_d.useMgtSettableComputeMeanLinkDelay?"force":""));
                } else if (conf.peer_d.useMgtSettableComputeMeanLinkDelay) {
                    VTSS_RC(vtss_icfg_printf(result, " ptp cmlds compute-meanlinkdelay force\n"));
                } else {
                    VTSS_RC(vtss_icfg_printf(result, " ptp cmlds compute-meanlinkdelay \n"));
                }
            }
            if ((conf.peer_d.mgtSettableComputeNeighborRateRatio != def.peer_d.mgtSettableComputeNeighborRateRatio) ||
                (conf.peer_d.useMgtSettableComputeNeighborRateRatio != def.peer_d.useMgtSettableComputeNeighborRateRatio) || req->all_defaults) {
                if (!conf.peer_d.mgtSettableComputeNeighborRateRatio) {
                    VTSS_RC(vtss_icfg_printf(result, " no ptp cmlds compute-neighbor-rate-ratio %s\n",
                                             conf.peer_d.useMgtSettableComputeNeighborRateRatio?"force":""));
                } else if (conf.peer_d.useMgtSettableComputeNeighborRateRatio) {
                    VTSS_RC(vtss_icfg_printf(result, " ptp cmlds compute-neighbor-rate-ratio force\n"));
                } else {
                    VTSS_RC(vtss_icfg_printf(result, " ptp cmlds compute-neighbor-rate-ratio \n"));
                }
            }
            if (conf.peer_d.allowedLostResponses != def.peer_d.allowedLostResponses) {
                VTSS_RC(vtss_icfg_printf(result, " ptp cmlds allow-lost-responses %d\n", conf.peer_d.allowedLostResponses));
            } else if (req->all_defaults) {
                VTSS_RC(vtss_icfg_printf(result, " no ptp cmlds allow-lost-responses 0 \n"));
            }
            if (conf.peer_d.allowedFaults != def.peer_d.allowedFaults) {
                VTSS_RC(vtss_icfg_printf(result, " ptp cmlds allow-faults %d\n", conf.peer_d.allowedFaults));
            } else if (req->all_defaults) {
                VTSS_RC(vtss_icfg_printf(result, " no ptp cmlds allow-faults 0 \n"));
            }
            if (conf.delayAsymmetry) {
                VTSS_RC(vtss_icfg_printf(result, " ptp cmlds delay-asymmetry %d\n", VTSS_INTERVAL_NS(conf.delayAsymmetry)));
            } else if (req->all_defaults) {
                VTSS_RC(vtss_icfg_printf(result, " no ptp cmlds delay-asymmetry \n"));
            }
        }
#endif
    }
    return VTSS_RC_OK;
}

/* ICFG Initialization function */
mesa_rc ptp_icfg_init(void)
{
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_PTP_GLOBAL_CONF, "ptp", ptp_icfg_global_conf));
    VTSS_RC(vtss_icfg_query_register(VTSS_ICFG_PTP_INTERFACE_CONF, "ptp", ptp_icfg_interface_conf));
    return VTSS_RC_OK;
}
#endif // VTSS_SW_OPTION_ICFG

