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

#include "web_api.h"
#include "conf_api.h"
#include "port_iter.hxx"
#include "ptp_api.h"
#include "vtss_ptp_types.h"
#include "vtss_ptp_local_clock.h"
#include "vtss_tod_api.h"
#include "vtss_tod_mod_man.h"
#include "tod_api.h"
#include "time.h"

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

#if defined(VTSS_SW_OPTION_ZLS30387)
#include "zl_3038x_api_pdv_api.h"
#endif

/* =================
 * Trace definitions
 * -------------- */
#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
#include <vtss_trace_api.h>
/* ============== */

#define PTP_WEB_BUF_LEN 300

const char *ptp_profiles[] = { "No Profile", "1588", "G8265.1", "G8275.1", "G8275.2", "802.1AS", "AED 802.1AS" };

static uint32_t ptp_web_profile_map(uint32_t in, bool html_to_cpp)
{
    uint32_t ret = in;

    // telecom profiles are disabled on maserati platform.
    // so, profile strings in ptp_profiles[2] to ptp_profiles[4] are not available on maserati platform.
    if (fast_cap(MESA_CAP_MISC_CHIP_FAMILY) == MESA_CHIP_FAMILY_LAN966X &&
        in > 1) {
        if (html_to_cpp) {
            ret = (vtss_appl_ptp_profile_t)(in + 3);
        } else {
            ret = in - 3;
        }
    }
    return ret;
}

static const char *ptp_two_step_override_disp(vtss_appl_ptp_twostep_override_t m)
{
    switch (m) {
        case VTSS_APPL_PTP_TWO_STEP_OVERRIDE_NONE:
            return "Clock Def.";
        case VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE:
            return "False";
        case VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE:
            return "True";
        default:
            return "unknown";
    }
}

static const char *ptp_one_pps_mode_disp(u8 m)
{
    switch (m) {
        case VTSS_APPL_PTP_ONE_PPS_DISABLE:
            return "Disable";
        case VTSS_APPL_PTP_ONE_PPS_OUTPUT:
            return "Output";
        default:
            return "unknown";
    }
}

static bool ptp_get_one_pps_mode_from_str(char *one_pps_mode_str, u8 *one_pps_mode)
{
    if(!strcmp(one_pps_mode_str, "Disable")) {
        *one_pps_mode = VTSS_APPL_PTP_ONE_PPS_DISABLE;
        return true;
    }
    if(!strcmp(one_pps_mode_str,"Output")) {
        *one_pps_mode = VTSS_APPL_PTP_ONE_PPS_OUTPUT;
        return true;
    }
    return false;
}

static const char *ptp_bool_disp(bool b)
{
    return (b ? "True" : "False");
}

static int ptp_get_bool_value(char *bool_var)
{
    if (!strcmp(bool_var,"True"))
        return true;
    else
        return false;
}

static vtss_appl_ptp_leap_second_type_t ptp_get_leap_type_value(char *leap_type_var)
{
    if (!strcmp(leap_type_var,"leap59"))
        return VTSS_APPL_PTP_LEAP_SECOND_59;
    else
        return VTSS_APPL_PTP_LEAP_SECOND_61;
}

static const char *ptp_state_disp(u8 b)
{
    switch (b) {
        case VTSS_APPL_PTP_COMM_STATE_IDLE:
            return "IDLE";
        case VTSS_APPL_PTP_COMM_STATE_INIT:
            return "INIT";
        case VTSS_APPL_PTP_COMM_STATE_CONN:
            return "CONN";
        case VTSS_APPL_PTP_COMM_STATE_SELL:
            return "SELL";
        case VTSS_APPL_PTP_COMM_STATE_SYNC:
            return "SYNC";
        default:
            return "?";
    }
}

static const char *ptp_protocol_disp(u8 p)
{
    switch (p) {
        case VTSS_APPL_PTP_PROTOCOL_ETHERNET:
            return "Ethernet";
        case VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED:
            return "EthernetMixed";
        case VTSS_APPL_PTP_PROTOCOL_IP4MULTI:
            return "IPv4Multi";
        case VTSS_APPL_PTP_PROTOCOL_IP4MIXED:
            return "IPv4Mixed";
        case VTSS_APPL_PTP_PROTOCOL_IP4UNI:
            return "IPv4Uni";
        case VTSS_APPL_PTP_PROTOCOL_OAM:
            return "Oam";
        case VTSS_APPL_PTP_PROTOCOL_ONE_PPS:
            return "OnePPS";
        case VTSS_APPL_PTP_PROTOCOL_IP6MIXED:
            return "IPv6Mixed";
        case VTSS_APPL_PTP_PROTOCOL_ANY:
            return "EthIPv4IPv6Combo";
        default:
            return "?";
    }
}

static const char *ptp_virtual_port_mode_disp(u32 p)
{
    switch (p) {
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_FREQ_OUT:
            return "freq-out";
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO:
            return "main-auto";
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN:
            return "main-man";
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN:
            return "pps-in";
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT:
            return "pps-out";
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB:
            return "sub";
        case VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE:
            return "none";
        default:
            return "?";
    }
}

static const char *ptp_virtual_port_tod_protocol_disp(u32 p)
{
    switch (p) {
        case VTSS_PTP_APPL_RS422_PROTOCOL_SER_POLYT:
            return "polyt";
        case VTSS_PTP_APPL_RS422_PROTOCOL_SER_RMC:
            return "rmc";
        case VTSS_PTP_APPL_RS422_PROTOCOL_SER_ZDA:
            return "zda";
        case VTSS_PTP_APPL_RS422_PROTOCOL_PIM:
            return "pim";
        case VTSS_PTP_APPL_RS422_PROTOCOL_NONE:
            return "none";
        default:
            return "?";
    }
}
static vtss_appl_ptp_protocol_t ptp_get_protocol(char *prot)
{
    if (!strcmp(prot,"Ethernet")) {
        return VTSS_APPL_PTP_PROTOCOL_ETHERNET;
    }
    else if (!strcmp(prot,"EthernetMixed")) {
        return VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED;
    }
    else if (!strcmp(prot,"IPv4Multi")) {
        return VTSS_APPL_PTP_PROTOCOL_IP4MULTI;
    }
    else if (!strcmp(prot,"IPv4Mixed")) {
        return VTSS_APPL_PTP_PROTOCOL_IP4MIXED;
    }
    else if (!strcmp(prot,"IPv4Uni")) {
        return VTSS_APPL_PTP_PROTOCOL_IP4UNI;
    }
    else if (!strcmp(prot,"Oam")) {
        return VTSS_APPL_PTP_PROTOCOL_OAM;
    }
    else if (!strcmp(prot,"OnePPS")) {
        return VTSS_APPL_PTP_PROTOCOL_ONE_PPS;
    }
    else if (!strcmp(prot,"IPv6Mixed")) {
        return VTSS_APPL_PTP_PROTOCOL_IP6MIXED;
    }
    else if (!strcmp(prot,"EthIPv4IPv6Combo")) {
        return VTSS_APPL_PTP_PROTOCOL_ANY;
    }
    return VTSS_APPL_PTP_PROTOCOL_ETHERNET;
}
static vtss_appl_virtual_port_mode_t ptp_get_virtual_port_mode(char *mode)
{
    if (!strcmp(mode,"freq-out")) {
        return VTSS_PTP_APPL_VIRTUAL_PORT_MODE_FREQ_OUT;
    }
    else if (!strcmp(mode,"main-auto")) {
        return VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO;
    }
    else if (!strcmp(mode,"main-man")) {
        return VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN;
    }
    else if (!strcmp(mode,"pps-in")) {
        return VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN;
    }
    else if (!strcmp(mode,"pps-out")) {
        return VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT;
    }
    else if (!strcmp(mode,"sub")) {
        return VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB;
    }
    return VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE;
}
static vtss_ptp_appl_rs422_protocol_t ptp_get_virtual_port_tod_protocol(char *prot)
{
    if (!strcmp(prot,"polyt")) {
        return VTSS_PTP_APPL_RS422_PROTOCOL_SER_POLYT;
    }
    else if (!strcmp(prot,"rmc")) {
        return VTSS_PTP_APPL_RS422_PROTOCOL_SER_RMC;
    }
    else if (!strcmp(prot,"zda")) {
        return VTSS_PTP_APPL_RS422_PROTOCOL_SER_ZDA;
    }
    else if (!strcmp(prot,"pim")) {
        return VTSS_PTP_APPL_RS422_PROTOCOL_PIM;
    }
    return VTSS_PTP_APPL_RS422_PROTOCOL_NONE;
}
static const char *ptp_filter_type_disp(u8 p)
{
    switch (p) {
#if defined(VTSS_SW_OPTION_ZLS30387)
        case PTP_FILTERTYPE_ACI_DEFAULT:
            return "ACI_DEFAULT";
        case PTP_FILTERTYPE_ACI_FREQ_XO:
            return "ACI_FREQ_XO";
        case PTP_FILTERTYPE_ACI_PHASE_XO:
            return "ACI_PHASE_XO";
        case PTP_FILTERTYPE_ACI_FREQ_TCXO:
            return "ACI_FREQ_TCXO";
        case PTP_FILTERTYPE_ACI_PHASE_TCXO:
            return "ACI_PHASE_TCXO";
        case PTP_FILTERTYPE_ACI_FREQ_OCXO_S3E:
            return "ACI_FREQ_OCXO_S3E";
        case PTP_FILTERTYPE_ACI_PHASE_OCXO_S3E:
            return "ACI_PHASE_OCXO_S3E";
        case PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_FREQ:
            return "ACI_BC_PARTIAL_ON_PATH_FREQ";
        case PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_PHASE:
            return "ACI_BC_PARTIAL_ON_PATH_PHASE";
        case PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_FREQ:
            return "ACI_BC_FULL_ON_PATH_FREQ";
        case PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE:
            return "ACI_BC_FULL_ON_PATH_PHASE";
        case PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE:
            return "ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE";
        case PTP_FILTERTYPE_ACI_FREQ_ACCURACY_FDD:
            return "ACI_FREQ_ACCURACY_FDD";
        case PTP_FILTERTYPE_ACI_FREQ_ACCURACY_XDSL:
            return "ACI_FREQ_ACCURACY_XDSL";
        case PTP_FILTERTYPE_ACI_ELEC_FREQ:
            return "ACI_ELEC_FREQ";
        case PTP_FILTERTYPE_ACI_ELEC_PHASE:
            return "ACI_ELEC_PHASE";
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C60W:
            return "ACI_PHASE_RELAXED_C60W";
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C150:
            return "ACI_PHASE_RELAXED_C150";
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C180:
             return "ACI_PHASE_RELAXED_C180";
        case PTP_FILTERTYPE_ACI_PHASE_RELAXED_C240:
             return "ACI_PHASE_RELAXED_C240";
        case PTP_FILTERTYPE_ACI_PHASE_OCXO_S3E_R4_6_1:
             return "ACI_PHASE_OCXO_S3E_R4_6_1";
        case PTP_FILTERTYPE_ACI_BASIC_PHASE:
             return "ACI_BASIC_PHASE";
        case PTP_FILTERTYPE_ACI_BASIC_PHASE_LOW:
             return "ACI_BASIC_PHASE_LOW";
#endif // defined(VTSS_SW_OPTION_ZLS30387)
        case PTP_FILTERTYPE_BASIC:
             return "BASIC";
        default:
            return "?";
    }
}

static int ptp_get_filter_type(char *filt)
{
#if defined(VTSS_SW_OPTION_ZLS30387)
    if (!strcmp(filt, "ACI_DEFAULT"))
        return PTP_FILTERTYPE_ACI_DEFAULT;
    if (!strcmp(filt, "ACI_FREQ_XO"))
        return PTP_FILTERTYPE_ACI_FREQ_XO;
    if (!strcmp(filt, "ACI_PHASE_XO"))
        return PTP_FILTERTYPE_ACI_PHASE_XO;
    if (!strcmp(filt, "ACI_FREQ_TCXO"))
        return PTP_FILTERTYPE_ACI_FREQ_TCXO;
    if (!strcmp(filt, "ACI_PHASE_TCXO"))
        return PTP_FILTERTYPE_ACI_PHASE_TCXO;
    if (!strcmp(filt, "ACI_FREQ_OCXO_S3E"))
        return PTP_FILTERTYPE_ACI_FREQ_OCXO_S3E;
    if (!strcmp(filt, "ACI_PHASE_OCXO_S3E"))
        return PTP_FILTERTYPE_ACI_PHASE_OCXO_S3E;
    if (!strcmp(filt, "ACI_BC_PARTIAL_ON_PATH_FREQ"))
        return PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_FREQ;
    if (!strcmp(filt, "ACI_BC_PARTIAL_ON_PATH_PHASE"))
        return PTP_FILTERTYPE_ACI_BC_PARTIAL_ON_PATH_PHASE;
    if (!strcmp(filt, "ACI_BC_FULL_ON_PATH_FREQ"))
        return PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_FREQ;
    if (!strcmp(filt, "ACI_BC_FULL_ON_PATH_PHASE"))
        return PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE;
    if (!strcmp(filt, "ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE"))
        return PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE;
    if (!strcmp(filt, "ACI_FREQ_ACCURACY_FDD"))
        return PTP_FILTERTYPE_ACI_FREQ_ACCURACY_FDD;
    if (!strcmp(filt, "ACI_FREQ_ACCURACY_XDSL"))
        return PTP_FILTERTYPE_ACI_FREQ_ACCURACY_XDSL;
    if (!strcmp(filt, "ACI_ELEC_FREQ"))
        return PTP_FILTERTYPE_ACI_ELEC_FREQ;
    if (!strcmp(filt, "ACI_ELEC_PHASE"))
        return PTP_FILTERTYPE_ACI_ELEC_PHASE;
    if (!strcmp(filt, "ACI_PHASE_RELAXED_C60W"))
        return PTP_FILTERTYPE_ACI_PHASE_RELAXED_C60W;
    if (!strcmp(filt, "ACI_PHASE_RELAXED_C150"))
        return PTP_FILTERTYPE_ACI_PHASE_RELAXED_C150;
    if (!strcmp(filt, "ACI_PHASE_RELAXED_C180"))
        return PTP_FILTERTYPE_ACI_PHASE_RELAXED_C180;
    if (!strcmp(filt, "ACI_PHASE_RELAXED_C240"))
        return PTP_FILTERTYPE_ACI_PHASE_RELAXED_C240;
    if (!strcmp(filt, "ACI_PHASE_OCXO_S3E_R4_6_1"))
        return PTP_FILTERTYPE_ACI_PHASE_OCXO_S3E_R4_6_1;
    if (!strcmp(filt, "ACI_BASIC_PHASE"))
        return PTP_FILTERTYPE_ACI_BASIC_PHASE;
    if (!strcmp(filt, "ACI_BASIC_PHASE_LOW"))
        return PTP_FILTERTYPE_ACI_BASIC_PHASE_LOW;
#endif // defined(VTSS_SW_OPTION_ZLS30387)
    if (!strcmp(filt, "BASIC"))
        return PTP_FILTERTYPE_BASIC;
    return PTP_FILTERTYPE_ACI_BC_FULL_ON_PATH_PHASE;
}

static const char *ptp_delaymechanism_disp(uchar d)
{
    switch (d) {
        case 1:
            return "e2e";
        case 2:
            return "p2p";
        case 3:
            return "cp2p";
        default:
            return "?\?\?";
    }
}

static bool ptp_get_delaymechanism_type(char *dmStr,u8 *mechType)
{
    bool rc = false;
    if ( 0 == strcmp(dmStr, "e2e")) {
        *mechType = 1;
        rc = true;
    } else if ( 0 == strcmp(dmStr, "p2p")) {
        *mechType = 2;
        rc =  true;
    } else if ( 0 == strcmp(dmStr, "cp2p")) {
        *mechType = 3;
        rc = true;
    }
    return rc;
}

static const char *clock_adjustment_method_txt(int m)
{
    switch (m) {
        case 0:
            return "Internal Timer";
        case 1:
            return "PTP DPLL";
        case 2:
            return "DAC option";
        case 3:
            return "Software";
        case 4:
            return "Synce DPLL";
        default:
            return "Unknown";
    }
}

static bool ptp_get_devtype_from_str(char * deviceTypeStr, vtss_appl_ptp_device_type_t *devType)
{
    bool rc = false;
    *devType = VTSS_APPL_PTP_DEVICE_NONE;
    if (0 == strcmp(deviceTypeStr,"Inactive")) {
        *devType = VTSS_APPL_PTP_DEVICE_NONE;
        rc = true;
    } else if (0 == strcmp(deviceTypeStr,"Ord-Bound")) {
        *devType = VTSS_APPL_PTP_DEVICE_ORD_BOUND;
        rc = true;
    } else if (0 == strcmp(deviceTypeStr,"P2pTransp")) {
        *devType = VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT;
        rc = true;
    } else if (0 == strcmp(deviceTypeStr,"E2eTransp")) {
        *devType = VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT;
        rc = true;
    } else if (0 == strcmp(deviceTypeStr,"Mastronly")) {
        *devType = VTSS_APPL_PTP_DEVICE_MASTER_ONLY;
        rc = true;
    } else if (0 == strcmp(deviceTypeStr,"Slaveonly")) {
        *devType = VTSS_APPL_PTP_DEVICE_SLAVE_ONLY;
        rc = true;
    } else if (0 == strcmp(deviceTypeStr,"BC-frontend")) {
        *devType = VTSS_APPL_PTP_DEVICE_BC_FRONTEND;
        rc = true;
    } else if (0 == strcmp(deviceTypeStr,"AED-GM")) {
        *devType = VTSS_APPL_PTP_DEVICE_AED_GM;
        rc = true;
    } else if (0 == strcmp(deviceTypeStr,"Internal")) {
        *devType = VTSS_APPL_PTP_DEVICE_INTERNAL;
        rc = true;
    }

    return rc;
}
static const char *ptp_adj_method_disp(vtss_appl_ptp_preferred_adj_t m)
{
    switch (m) {
        case VTSS_APPL_PTP_PREFERRED_ADJ_LTC: return "LTC";
        case VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE: return "Single";
        case VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT: return "Independent";
        case VTSS_APPL_PTP_PREFERRED_ADJ_COMMON: return "Common";
        case VTSS_APPL_PTP_PREFERRED_ADJ_AUTO: return "Auto";
        default: return "unknown";
    }
}

static bool ptp_get_adj_method_from_str(char * preferred_adjStr, vtss_appl_ptp_preferred_adj_t *m)
{
    bool rc = false;
    *m = VTSS_APPL_PTP_PREFERRED_ADJ_AUTO;
    if (0 == strcmp(preferred_adjStr,"LTC")) {
        *m = VTSS_APPL_PTP_PREFERRED_ADJ_LTC;
        rc = true;
    } else if (0 == strcmp(preferred_adjStr,"Single")) {
        *m = VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE;
        rc = true;
    } else if (0 == strcmp(preferred_adjStr,"Independent")) {
        *m = VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT;
        rc = true;
    } else if (0 == strcmp(preferred_adjStr,"Common")) {
        *m = VTSS_APPL_PTP_PREFERRED_ADJ_COMMON;
        rc = true;
    } else if (0 == strcmp(preferred_adjStr,"Auto")) {
        *m = VTSS_APPL_PTP_PREFERRED_ADJ_AUTO;
        rc = true;
    }
    return rc;
}

#if defined (VTSS_SW_OPTION_P802_1_AS)
static const char *web_port_role_disp(vtss_appl_ptp_802_1as_port_role_t r)
{
    switch (r) {
        case VTSS_APPL_PTP_PORT_ROLE_DISABLED_PORT:
            return "Disabled";
        case VTSS_APPL_PTP_PORT_ROLE_MASTER_PORT:
            return "Master";
        case VTSS_APPL_PTP_PORT_ROLE_PASSIVE_PORT:
            return "Passive";
        case VTSS_APPL_PTP_PORT_ROLE_SLAVE_PORT:
            return "Slave";
        default:
            return "?\?\?";
    }
}

static bool ptp_get_aed_port_state(char *dmStr,u8 *aedState)
{
    bool rc = false;
    if ( 0 == strcmp(dmStr, "AED-Master")) {
        *aedState = 0;
        rc = true;
    } else if ( 0 == strcmp(dmStr, "AED-Slave")) {
        *aedState = 1;
        rc =  true;
    }
    return rc;
}

static const char *ptp_aed_port_state_disp(uchar d)
{
    switch (d) {
        case 0:
            return "AED-Master";
        case 1:
            return "AED-Slave";
        default:
            return "?\?\?";
    }
}

// str1 is input string.
static bool ptp_extract_clock_id(const char *str1, vtss_appl_clock_identity *clk_id)
{
    uint32_t id[8];
    if (strlen(str1) > 23) {
        return false;
    }

    for (auto i = 0; i < strlen(str1); i++) {
        if ((str1[i] >= 'A' && str1[i] <= 'F') || (str1[i] >= 'a' && str1[i] <= 'f') ||
            (str1[i] >= '0' && str1[i] <= '9') || (str1[i] == ':')) {
            continue;
        }
        return false;
    }

    sscanf(str1, "%x:%x:%x:%x:%x:%x:%x:%x", &id[0], &id[1], &id[2], &id[3], &id[4], &id[5], &id[6], &id[7]);
    for (auto i = 0; i < 8; i++) {
        (*clk_id)[i] = (uint8_t)id[i];
    }
    return true;
}
#endif //defined (VTSS_SW_OPTION_P802_1_AS)


/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/

static i32 handler_config_ptp(CYG_HTTPD_STATE* p)
{
    i32    ct;
    vtss_appl_ptp_device_type_t dev_type;
    u8     one_pps_mode;
    int    var_int;
    ulong  var_value;
    vtss_appl_ptp_preferred_adj_t adj_method;
    mesa_mac_t my_sysmac;
    uint inst;
    uint new_clock_inst;
    ptp_clock_default_ds_t default_ds;
    vtss_appl_ptp_clock_status_default_ds_t default_ds_status;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;

    ptp_init_clock_ds_t clock_init_bs;
    vtss_appl_ptp_config_port_ds_t port_cfg;
    vtss_appl_ptp_ext_clock_mode_t mode;
    vtss_appl_ptp_config_port_ds_t new_port_cfg;
    vtss_ifindex_t ifindex;
    switch_iter_t sit;
    u32 port_no;
    bool dataSet = false;
    const char *var_string;
    size_t len = 0;
    const char *str;
    char search_str[32];
    char str2[40];
    port_iter_t       pit;

    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG);

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP))
        return -1;
#endif

    memset(&clock_init_bs,0,sizeof(clock_init_bs));

    memset(&default_ds, 0, sizeof(default_ds));
    memset(&port_cfg, 0, sizeof(port_cfg));
    if (p->method == CYG_HTTPD_METHOD_POST) {
        /* show the local clock  */
        (void) vtss_appl_ptp_ext_clock_out_get(&mode);
        var_string = cyg_httpd_form_varable_string(p, "one_pps_mode", &len);
        if (len > 0) {
           (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
           /* Change the Device Type of Clock Instance */
           if (ptp_get_one_pps_mode_from_str(str2, &one_pps_mode)) {
               mode.one_pps_mode = (vtss_appl_ptp_ext_clock_1pps_t) one_pps_mode;
           }
        }
        var_string = cyg_httpd_form_varable_string(p, "external_enable", &len);
        if (len > 0) {
           (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
           mode.clock_out_enable = ptp_get_bool_value(str2);
        }
        var_value = 0;
        if (cyg_httpd_form_varable_long_int(p, "clock_freq", &var_value)) {
            mode.freq = var_value;
        }

        var_value = 0;
        if (cyg_httpd_form_varable_long_int(p, "pps_domain", &var_value)) {
            mode.clk_domain = var_value;
        }

        var_string = cyg_httpd_form_varable_string(p, "adj_method", &len);
        if (len > 0) {
           (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
           if (ptp_get_adj_method_from_str(str2, &adj_method)) {
               mode.adj_method = adj_method;
           }
        }

        if (fast_cap(MESA_CAP_TS_PPS_MODE_OVERRULES_CLK_OUT)) {
            if (mode.one_pps_mode != VTSS_APPL_PTP_ONE_PPS_DISABLE && mode.clock_out_enable) {
                mode.clock_out_enable = false;
            }
        }

        if (fast_cap(MESA_CAP_TS_PPS_OUT_OVERRULES_CLK_OUT)) {
            if (mode.one_pps_mode == VTSS_APPL_PTP_ONE_PPS_OUTPUT && mode.clock_out_enable) {
                mode.clock_out_enable = false;
            }
        }

        if (mode.clock_out_enable && !mode.freq) {
            mode.freq = 1;
        }

        if (VTSS_RC_OK != vtss_appl_ptp_ext_clock_out_set(&mode)) {
            T_W("The preferred adjust method is not supported");
        }

        var_string = cyg_httpd_form_varable_string(p, "phy_ts_mode", &len);
        if (len > 0) {
            BOOL phy_ts_dis = FALSE;
            (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
            phy_ts_dis = ptp_get_bool_value(str2) ? FALSE : TRUE;
            (void)tod_board_phy_ts_dis_set(phy_ts_dis);
        }

        for (inst = 0; inst < PTP_CLOCK_INSTANCES; inst++)
        {
            // Note: Delete/update of clock needs to come before creation of new clocks to avoid that clocks are erroneously modified right after creation
            sprintf(search_str, "delete_%d", inst);
            if (cyg_httpd_form_varable_find(p, search_str)){
                /* Delete the Clock Instance */
                (void) vtss_appl_ptp_clock_config_default_ds_del(inst);

                port_iter_init_local(&pit);
                while (port_iter_getnext(&pit)) {
                    (void) vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex);
                    (void) ptp_ifindex_to_port(ifindex, &port_no);
                    if (vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &port_cfg) == VTSS_RC_OK) {
                        port_cfg.enabled = false;
                        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &port_cfg) == VTSS_RC_ERROR) {
                            T_D("Clock instance %d : does not exist", inst);
                        }
                    }
                }
            }
        }

        for (inst = 0; inst < PTP_CLOCK_INSTANCES; inst++)
        {
            sprintf(search_str, "clock_inst_new_%d", inst);
            if (cyg_httpd_form_varable_int(p, search_str, &var_int)) {
                new_clock_inst = var_int;
                T_D(" Creating a new clock_instance-[%d]", new_clock_inst);

                vtss_appl_ptp_profile_t profile;
                sprintf(search_str, "ptp_profile_new_%d", inst);
                if (cyg_httpd_form_varable_int(p, search_str, &var_int)) {
                    profile = (vtss_appl_ptp_profile_t)ptp_web_profile_map(var_int, true);
                }
                else {
                    profile = VTSS_APPL_PTP_PROFILE_NO_PROFILE;
                }
                clock_init_bs.cfg.profile = profile;

                (void) ptp_get_default_clock_default_ds(&default_ds_status, &default_ds_cfg);
                (void) ptp_apply_profile_defaults_to_default_ds(&default_ds_cfg, profile);
                
                clock_init_bs.cfg = default_ds_cfg;

                sprintf(search_str, "devType_new_%d", inst);
                var_string = cyg_httpd_form_varable_string(p, search_str, &len);
                if (len > 0) {
                    (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    /* Change the Device Type of Clock Instance */
                    if (ptp_get_devtype_from_str(str2, &dev_type)) {
                        T_D("dev_type-[%d]", dev_type);
                        clock_init_bs.cfg.deviceType = dev_type;
                        if (clock_init_bs.cfg.deviceType == VTSS_APPL_PTP_DEVICE_INTERNAL) {
                            clock_init_bs.cfg.filter_type = VTSS_APPL_PTP_FILTER_TYPE_BASIC;
                        }
                    }
                }

                // Vlan id
                sprintf(search_str, "vid_new_%d", inst);
                if (cyg_httpd_form_varable_int(p, search_str, &var_int)) {
                    clock_init_bs.cfg.configured_vid = var_int ? var_int : 1;
                }

                clock_init_bs.cfg.mep_instance = default_ds_cfg.mep_instance;
                
                // sprintf(search_str, "enforce_profile_new_%d", inst);
                // if (cyg_httpd_form_varable_int(p, search_str, &var_int)) {
                //     clock_init_bs.cfg.enforceProfile = var_int;
                // }
                T_D(" protocol-[%s (%d)]",str2, clock_init_bs.cfg.protocol);

                // Generate clock identity from clock instance number and base MAC address
                (void) conf_mgmt_mac_addr_get(my_sysmac.addr, 0);
                clock_init_bs.clockIdentity[0] = my_sysmac.addr[0];
                clock_init_bs.clockIdentity[1] = my_sysmac.addr[1];
                clock_init_bs.clockIdentity[2] = my_sysmac.addr[2];
                clock_init_bs.clockIdentity[3] = 0xff;
                clock_init_bs.clockIdentity[4] = 0xfe;
                clock_init_bs.clockIdentity[5] = my_sysmac.addr[3];
                clock_init_bs.clockIdentity[6] = my_sysmac.addr[4];
                clock_init_bs.clockIdentity[7] = my_sysmac.addr[5]+new_clock_inst;

                // clock domain
                sprintf(search_str, "clk_dom_new_%d", inst);
                if (cyg_httpd_form_varable_int(p, search_str, &var_int)) {
                    clock_init_bs.cfg.clock_domain = var_int;
                }
                T_D(" clock_domain-[%d]",var_int);

                // The creation will fail if it is a transparent clock and a transparent clock already exists.
                if (ptp_clock_clockidentity_set(new_clock_inst, &clock_init_bs.clockIdentity) != VTSS_RC_OK) {
                    T_D("Cannot set clock identity");
                }
                if (vtss_appl_ptp_clock_config_default_ds_set(new_clock_inst, &clock_init_bs.cfg) != VTSS_RC_OK) {
                //if (ptp_clock_create(&clock_init_bs, new_clock_inst) != VTSS_RC_OK) {
                    T_D(" Cannot Create Clock: Tried to Create more than one transparent clock ");
                }

                /* Apply profile defaults to all ports of this clock (whether enabled or not) */
                port_iter_init_local(&pit);
                while (port_iter_getnext(&pit)) {
                    (void) vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex);
                    (void) ptp_ifindex_to_port(ifindex, &port_no);

                    if (vtss_appl_ptp_config_clocks_port_ds_get(new_clock_inst, ifindex, &port_cfg) == VTSS_RC_OK) {
                        new_port_cfg = port_cfg;
                        ptp_apply_profile_defaults_to_port_ds(&new_port_cfg, profile);
    
                        if (memcmp(&new_port_cfg, &port_cfg, sizeof(vtss_appl_ptp_config_port_ds_t)) != 0) {
                            if (vtss_appl_ptp_config_clocks_port_ds_set(new_clock_inst, ifindex, &new_port_cfg) == VTSS_RC_ERROR) {
                                T_D("Clock instance %d : does not exist", new_clock_inst);
                            }
                        }
                    }
                }
            } 
        }
        redirect(p, "/ptp_config.htm");
    } else {
        if(!cyg_httpd_start_chunked("html"))
            return -1;

        T_D("Inside Else Part");
        /* default clock id */
        memset(str2, 0, sizeof(str2));

        /* Send the Dynamic Parameters */
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^Disable/Output");
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        (void)snprintf(p->outbuffer, sizeof(p->outbuffer), "^%s", ptp_adj_method_disp(VTSS_APPL_PTP_PREFERRED_ADJ_LTC));
        if (fast_cap(MEBA_CAP_SYNCE_DPLL_MODE_SINGLE)) {
            (void)strncat(p->outbuffer, "/", sizeof(p->outbuffer) - strlen(p->outbuffer) - 1);
            (void)strncat(p->outbuffer, ptp_adj_method_disp(VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE), sizeof(p->outbuffer) - strlen(p->outbuffer) - 1);
        }
        if (fast_cap(MEBA_CAP_SYNCE_DPLL_MODE_DUAL)) {
            (void)strncat(p->outbuffer, "/", sizeof(p->outbuffer) - strlen(p->outbuffer) - 1);
            (void)strncat(p->outbuffer, ptp_adj_method_disp(VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT), sizeof(p->outbuffer) - strlen(p->outbuffer) - 1);
        }
        if (fast_cap(MEBA_CAP_SYNCE_DPLL_MODE_SINGLE)) {
            (void)strncat(p->outbuffer, "/", sizeof(p->outbuffer) - strlen(p->outbuffer) - 1);
            (void)strncat(p->outbuffer, ptp_adj_method_disp(VTSS_APPL_PTP_PREFERRED_ADJ_COMMON), sizeof(p->outbuffer) - strlen(p->outbuffer) - 1);
        }
        (void)strncat(p->outbuffer, "/", sizeof(p->outbuffer) - strlen(p->outbuffer) - 1);
        (void)strncat(p->outbuffer, ptp_adj_method_disp(VTSS_APPL_PTP_PREFERRED_ADJ_AUTO), sizeof(p->outbuffer) - strlen(p->outbuffer) - 1);
        (void)cyg_httpd_write_chunked(p->outbuffer, strlen(p->outbuffer));

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), (tod_board_phy_ts_dis_get() || mepa_phy_ts_cap()) ? "^True/False" : "^False");
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^0");
        if (fast_cap(MESA_CAP_TS_DOMAIN_CNT) > 1) {
            for (auto cnt = 1; cnt < fast_cap(MESA_CAP_TS_DOMAIN_CNT); cnt++) {
                ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, "/%d", cnt);
            }
        }
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        /* show the local clock  */
        (void) vtss_appl_ptp_ext_clock_out_get(&mode);

        (void)cyg_httpd_write_chunked("^", 1);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|%s|%d|%s|%d",
                                      ptp_one_pps_mode_disp(mode.one_pps_mode),
                                      ptp_bool_disp(mode.clock_out_enable),
                                      mode.freq,
                                      ptp_adj_method_disp(mode.adj_method),
                                      mode.clk_domain);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        (void)cyg_httpd_write_chunked("^", 1);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", ptp_bool_disp(mepa_phy_ts_cap()));
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        (void)cyg_httpd_write_chunked("^", 1);

        dev_type = VTSS_APPL_PTP_DEVICE_NONE;
        str = DeviceTypeToString(dev_type);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s", str);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        while (1) {
            dev_type = (vtss_appl_ptp_device_type_t)(dev_type + 1);
            str = DeviceTypeToString(dev_type);
            if (strcmp(str,"?") != 0 ) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%s", str);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            } else {
                break;
            }
        }

        (void)cyg_httpd_write_chunked("#", 1);
        for (inst = 0; inst < PTP_CLOCK_INSTANCES; inst++)
        {
            T_D("Inst - %d",inst);
            if (vtss_appl_ptp_clock_config_default_ds_get(inst, &default_ds_cfg) == VTSS_RC_OK)
            {
                dataSet = true;
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%u", inst);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", default_ds_cfg.clock_domain);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", default_ds_cfg.configured_vid ? default_ds_cfg.configured_vid : 1); // default vlan id is 1
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s", DeviceTypeToString(default_ds_cfg.deviceType));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", ptp_web_profile_map((uint32_t)default_ds_cfg.profile, false));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);

//                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", default_ds_cfg.enforceProfile);
//                (void)cyg_httpd_write_chunked(p->outbuffer, ct);

//                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s", ClockIdentityToString(default_ds_status.clockIdentity, str1));
//                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        if (!dataSet)
            (void)cyg_httpd_write_chunked("",1);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^%s/%s", ptp_profiles[0], ptp_profiles[1]);

        // telecom profile are disabled on Lan9668 platform.
        if (fast_cap(MESA_CAP_MISC_CHIP_FAMILY) != MESA_CHIP_FAMILY_LAN966X) {
            ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, "/%s/%s/%s", ptp_profiles[2], ptp_profiles[3], ptp_profiles[4]);
        }
        ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, "%s%s%s%s",
#if defined (VTSS_SW_OPTION_P802_1_AS)
                "/", ptp_profiles[5],
                "/", ptp_profiles[6]
#else
                "", ""
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
              );
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

#define HAS_SERVO_380(expr)     { \
                                    if (servo_type == VTSS_ZARLINK_SERVO_ZLS30380) { \
                                        ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, "%s", expr); \
                                    } \
                                }
#define HAS_ANY_SERVO(expr)     { \
                                    if (servo_type == VTSS_ZARLINK_SERVO_ZLS30380 || servo_type == VTSS_ZARLINK_SERVO_ZLS30387) { \
                                        ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, "%s", expr); \
                                    } \
                                }

static i32 handler_clock_config_ptp(CYG_HTTPD_STATE* p)
{
    i32    ct;
    char   str [14];
    size_t len;
    mesa_timestamp_t t;
    uint inst = 0;
    int    var_value = 0;
    int    var_int;    
    mesa_ipv4_t ip_address;
    vtss_appl_ptp_clock_status_default_ds_t default_ds_status;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_appl_ptp_clock_current_ds_t clock_current_bs;
    vtss_appl_ptp_clock_parent_ds_t clock_parent_bs;
    vtss_appl_ptp_clock_timeproperties_ds_t clock_time_properties_bs;
    vtss_appl_ptp_clock_config_default_ds_t new_default_ds_cfg;
    vtss_appl_ptp_config_port_ds_t new_port_cfg;
    vtss_appl_ptp_virtual_port_config_t virtual_port_cfg;
    vtss_appl_ptp_clock_timeproperties_ds_t *virtual_time_prop = &virtual_port_cfg.timeproperties;
#ifdef  PTP_OFFSET_FILTER_CUSTOM
    ptp_clock_servo_con_ds_t servo;
#else
    vtss_appl_ptp_clock_servo_config_t default_servo;
#endif /* PTP_OFFSET_FILTER_CUSTOM */
    vtss_appl_ptp_clock_filter_config_t filter_params;
    vtss_appl_ptp_unicast_slave_config_t uni_slave_cfg;
    vtss_appl_ptp_unicast_slave_table_t uni_slave_status;
    bool dataSet = false;
    bool sync_2_sys_clock = false;
    bool old_ptp_port_state = false;
    const char *var_string;
    char search_str[32];    
    char str1[40];
    char str2[40];
    char str3[60]; /* Change this */
    u64 hw_time;
    u32 ix;
    vtss_appl_ptp_config_port_ds_t port_cfg;
    vtss_ifindex_t ifindex;
    uint port_no;
    port_iter_t pit;
    int first_port;
    ptp_internal_mode_config_t internal_cfg = {};

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP))
        return -1;
#endif
    if (cyg_httpd_form_varable_int(p, "clock_inst", &var_value)) {
        inst = var_value;
    }

    // bool="true" || "false"
    if ((var_string = cyg_httpd_form_varable_string(p, "bool", &len)) != NULL && len >= 4) {
      if (strncasecmp(var_string, "true", 4) == 0) {
        sync_2_sys_clock = true;
      } else if (strncasecmp(var_string, "false", 5) == 0) {
        sync_2_sys_clock = false;
      }
    }

    if (sync_2_sys_clock == true) {
        vtss_appl_ptp_system_time_sync_conf_t sync_conf;
        vtss_appl_ptp_system_time_sync_mode_get(&sync_conf);
        sync_conf.clockinst = inst;
        sync_conf.mode = VTSS_APPL_PTP_SYSTEM_TIME_SYNC_GET;
        vtss_appl_ptp_system_time_sync_mode_set(&sync_conf);
        t.sec_msb = 0;
        t.seconds = time(NULL);
        t.nanoseconds = 0;
        ptp_local_clock_time_set(&t, ptp_instance_2_timing_domain(inst));
        T_D("True is set,clock_instance-[%d] ",inst);
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        T_D("Inside CYG_HTTPD_METHOD_POST post_data-[%s]",p->post_data);
        if ((vtss_appl_ptp_clock_config_default_ds_get(inst, &default_ds_cfg) == VTSS_RC_OK) &&
            (vtss_appl_ptp_clock_status_default_ds_get(inst, &default_ds_status) == VTSS_RC_OK))
        {
            new_default_ds_cfg = default_ds_cfg;

            // sprintf(search_str, "ptp_profile_%d", inst);
            // if (cyg_httpd_form_varable_int(p, search_str, &var_int))
            //     new_default_ds_cfg.profile = var_int;
    
            // sprintf(search_str, "enforce_profile_%d", inst);
            // if (cyg_httpd_form_varable_int(p, search_str, &var_int))
            //     new_default_ds_cfg.enforceProfile = var_int;

            sprintf(str1,"2_step_flag_%d",inst);
            var_string = cyg_httpd_form_varable_string(p, str1, &len);
            if (len > 0)
                (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
            new_default_ds_cfg.twoStepFlag = ptp_get_bool_value(str2);

            port_iter_init_local(&pit);
            while (port_iter_getnext(&pit)) {
                (void) vtss_ifindex_from_port(0, pit.iport, &ifindex);
                (void) ptp_ifindex_to_port(ifindex, &port_no);

                if (vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &port_cfg) == VTSS_RC_OK) {
                    old_ptp_port_state = port_cfg.enabled;
                    sprintf(search_str, "mask_%d", (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_find(p, search_str)){
                        port_cfg.enabled = true;
                    } else {
                        port_cfg.enabled = false;
                    }
                    // Only update if port state changes enabled <--> disabled
                    if (old_ptp_port_state != port_cfg.enabled) {
                        if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &port_cfg) == VTSS_RC_ERROR) {
                            T_D("Clock instance %d : does not exist", inst);
                        }
                    }
                    if (new_default_ds_cfg.twoStepFlag && port_cfg.enabled) {
                        tod_mod_man_tx_ts_queue_init(port_no);
                    }
                }
            }

            sprintf(str1,"domain_%d",inst);
            if (cyg_httpd_form_varable_int(p, str1, &var_value) )
                new_default_ds_cfg.domainNumber = var_value;

            sprintf(str1,"prio_1_%d",inst);
            if (cyg_httpd_form_varable_int(p, str1, &var_value))
                new_default_ds_cfg.priority1 = var_value;

            sprintf(str1,"prio_2_%d",inst);
            if (cyg_httpd_form_varable_int(p, str1, &var_value))
                new_default_ds_cfg.priority2 = var_value;

            sprintf(str1,"local_prio_%d",inst);
            if (cyg_httpd_form_varable_int(p, str1, &var_value))
                new_default_ds_cfg.localPriority = var_value;

            sprintf(str1,"protocol_method_%d",inst);
            var_string = cyg_httpd_form_varable_string(p, str1, &len);
            if (len > 0)
                (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
            new_default_ds_cfg.protocol = ptp_get_protocol(str2);

            sprintf(str1,"one_way_%d",inst);
            var_string = cyg_httpd_form_varable_string(p, str1, &len);
            if (len > 0)
                (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
            new_default_ds_cfg.oneWay =  ptp_get_bool_value(str2);

            sprintf(search_str, "pcp_%d", inst);
            if (cyg_httpd_form_varable_int(p, search_str, &var_int)) {
                new_default_ds_cfg.configured_pcp = var_int;
            }

            sprintf(search_str, "dscp_%d", inst);
            if (cyg_httpd_form_varable_int(p, search_str, &var_int)) {
                new_default_ds_cfg.dscp = var_int;
            }

            sprintf(str1, "filter_type_%d", inst);
            var_string = cyg_httpd_form_varable_string(p, str1, &len);
            if (len > 0)
                (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
            new_default_ds_cfg.filter_type = vtss_appl_ptp_filter_type_t(ptp_get_filter_type(str2));

            if (vtss_appl_ptp_clock_config_default_ds_set(inst, &new_default_ds_cfg) != VTSS_RC_OK) {
                T_D("Clock instance %d : does not exist", inst);
            }

            if (default_ds_cfg.deviceType != VTSS_APPL_PTP_DEVICE_INTERNAL &&
                ptp_clock_config_virtual_port_config_get(inst, &virtual_port_cfg) == VTSS_RC_OK) {

                sprintf(str1, "virtual_port_enable_%d", inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    virtual_port_cfg.enable = ptp_get_bool_value(str2);
                }

                sprintf(str1, "virtual_port_class_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    virtual_port_cfg.clockQuality.clockClass = var_value;

                sprintf(str1, "virtual_port_accuracy_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    virtual_port_cfg.clockQuality.clockAccuracy = var_value;

                sprintf(str1, "virtual_port_variance_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    virtual_port_cfg.clockQuality.offsetScaledLogVariance = var_value;

                sprintf(str1, "virtual_port_prio1_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    virtual_port_cfg.priority1 = var_value;

                sprintf(str1, "virtual_port_prio2_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value)){
                    virtual_port_cfg.priority2 = var_value;
                }

                sprintf(str1, "virtual_port_local_prio_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    virtual_port_cfg.localPriority = var_value;

                sprintf(str1,"virtual_port_mode_%d",inst);
                var_string = cyg_httpd_form_varable_string(p, str1, &len);
                if (len > 0) 
                    (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                virtual_port_cfg.virtual_port_mode = ptp_get_virtual_port_mode(str2);
                
                sprintf(str1, "virtual_port_inp_pin_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value)) 
                    virtual_port_cfg.input_pin = var_value;

                sprintf(str1, "virtual_port_out_pin_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    virtual_port_cfg.output_pin = var_value;
                
	        sprintf(str1,"virtual_port_tod_proto_%d",inst);
                var_string = cyg_httpd_form_varable_string(p, str1, &len);
                if (len > 0)
                    (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                virtual_port_cfg.proto = ptp_get_virtual_port_tod_protocol(str2);
                
                sprintf(str1, "virtual_port_pim_interface_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                     virtual_port_cfg.portnum = uport2iport(var_value);

                // pps-delay need to be updated.
                sprintf(str1, "virtual_port_pps_delay_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    virtual_port_cfg.delay = var_value;

                sprintf(str1, "virtual_port_alarm_%d", inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    virtual_port_cfg.alarm = ptp_get_bool_value(str2);
                }

                sprintf(str1, "v_clock_id_%d", inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0) {
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                        if (ptp_extract_clock_id(str2, &virtual_port_cfg.clock_identity) == false) {
                            T_W("Could not extract virtual port clock identity");
                        }
                    }
                }

                sprintf(str1, "v_steps_rmvd_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value)){
                    virtual_port_cfg.steps_removed = var_value;
                }

                sprintf(str1,"v_uct_offset_%d",inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    virtual_time_prop->currentUtcOffset = var_value;


                sprintf(str1,"v_valid_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    virtual_time_prop->currentUtcOffsetValid = ptp_get_bool_value(str2);
                }
                sprintf(str1,"v_leap59_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    virtual_time_prop->leap59 = ptp_get_bool_value(str2);
                }

                sprintf(str1,"v_leap61_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    virtual_time_prop->leap61 = ptp_get_bool_value(str2);
                }

                sprintf(str1,"v_time_trac_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    virtual_time_prop->timeTraceable = ptp_get_bool_value(str2);
                }

                sprintf(str1,"v_freq_trac_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    virtual_time_prop->frequencyTraceable = ptp_get_bool_value(str2);
                }

                sprintf(str1,"v_ptp_time_scale_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    virtual_time_prop->ptpTimescale = ptp_get_bool_value(str2);
                }

                sprintf(str1,"v_time_source_%d",inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    virtual_time_prop->timeSource = var_value;

                sprintf(str1,"v_leap_pending_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    virtual_time_prop->pendingLeap = ptp_get_bool_value(str2);
                }

                sprintf(str1,"v_leap_date_%d",inst);
                var_string = cyg_httpd_form_varable_string(p, str1, &len);
                if (len > 0) {
                    (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    (void)extract_date(str2, &virtual_time_prop->leapDate);
                }

                sprintf(str1,"v_leap_type_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    virtual_time_prop->leapType = ptp_get_leap_type_value(str2);
                }

                if (vtss_appl_ptp_clock_config_virtual_port_config_set(inst, &virtual_port_cfg) != VTSS_RC_OK) {
                    T_D("Could not update virtual port data set of clock instance %d", inst);
                }
   
            }

            if (vtss_appl_ptp_clock_config_timeproperties_ds_get(inst, &clock_time_properties_bs) == VTSS_RC_OK) {

                sprintf(str1,"uct_offset_%d",inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    clock_time_properties_bs.currentUtcOffset = var_value;


                sprintf(str1,"valid_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    clock_time_properties_bs.currentUtcOffsetValid = ptp_get_bool_value(str2);
                }
                sprintf(str1,"leap59_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    clock_time_properties_bs.leap59 = ptp_get_bool_value(str2);
                }

                sprintf(str1,"leap61_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    clock_time_properties_bs.leap61 = ptp_get_bool_value(str2);
                }

                sprintf(str1,"time_trac_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    clock_time_properties_bs.timeTraceable = ptp_get_bool_value(str2);
                }

                sprintf(str1,"freq_trac_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    clock_time_properties_bs.frequencyTraceable = ptp_get_bool_value(str2);
                }

                sprintf(str1,"ptp_time_scale_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    clock_time_properties_bs.ptpTimescale = ptp_get_bool_value(str2);
                }

                sprintf(str1,"time_source_%d",inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    clock_time_properties_bs.timeSource = var_value;

                sprintf(str1,"leap_pending_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    clock_time_properties_bs.pendingLeap = ptp_get_bool_value(str2);
                }

                sprintf(str1,"leap_date_%d",inst);
                var_string = cyg_httpd_form_varable_string(p, str1, &len);
                if (len > 0) {
                    (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    (void)extract_date(str2, &clock_time_properties_bs.leapDate);
                }

                sprintf(str1,"leap_type_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    clock_time_properties_bs.leapType = ptp_get_leap_type_value(str2);
                }

                if (vtss_appl_ptp_clock_config_timeproperties_ds_set(inst, &clock_time_properties_bs) != VTSS_RC_OK) {
                    T_D("Clock instance %d : does not exist", inst);
                }
            }

            if (vtss_appl_ptp_clock_servo_parameters_get(inst, &default_servo) == VTSS_RC_OK) {
                sprintf(str1,"display_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    default_servo.display_stats =  ptp_get_bool_value(str2);
                }

                sprintf(str1,"p_enable_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    default_servo.p_reg =  ptp_get_bool_value(str2);
                }
                sprintf(str1,"i_enable_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    default_servo.i_reg =  ptp_get_bool_value(str2);
                }
                sprintf(str1,"d_enable_%d",inst);
                if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                    if (len > 0)
                        (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                    default_servo.d_reg =  ptp_get_bool_value(str2);
                }
                sprintf(str1,"p_const_%d",inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    default_servo.ap = var_value;

                sprintf(str1,"i_const_%d",inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    default_servo.ai = var_value;

                sprintf(str1,"d_const_%d",inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    default_servo.ad = var_value;

                sprintf(str1,"gain_%d",inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value))
                    default_servo.gain = var_value;

                if (vtss_appl_ptp_clock_servo_parameters_set(inst, &default_servo) == VTSS_RC_ERROR) {
                    T_D("Clock instance %d : does not exist", inst);
                }
            }
            if (vtss_appl_ptp_clock_filter_parameters_get(inst, &filter_params) == VTSS_RC_OK) {
                sprintf(str1,"delay_filter_%d",inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                    filter_params.delay_filter = var_value;
                }

                sprintf(str1,"period_%d",inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                    filter_params.period = var_value;
                }

                sprintf(str1,"dist_%d",inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                    filter_params.dist = var_value;
                }

                if (vtss_appl_ptp_clock_filter_parameters_set(inst, &filter_params) == VTSS_RC_ERROR) {
                    T_D("Clock instance %d : does not exist", inst);
                }
            }
            ix = 0;
            while (vtss_appl_ptp_clock_config_unicast_slave_config_get(inst, ix++, &uni_slave_cfg) == VTSS_RC_OK) {
                sprintf(str1,"uc_dura_%d_%d",inst,(int)(ix-1));
                if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                    uni_slave_cfg.duration = var_value;
                }

                sprintf(str1,"uc_ip_%d_%d",inst,(int)(ix-1));
                if (cyg_httpd_form_varable_ipv4(p, str1, &ip_address)) {
                    uni_slave_cfg.ip_addr = ip_address;
                }

                if (vtss_appl_ptp_clock_config_unicast_slave_config_set(inst, (ix-1), &uni_slave_cfg) == VTSS_RC_ERROR) {
                    T_D("Clock instance %d : does not exist",inst);
                }
            }

            if (default_ds_cfg.deviceType == VTSS_APPL_PTP_DEVICE_INTERNAL &&
                ptp_internal_mode_config_get(inst, &internal_cfg) == true) {
                sprintf(str1, "inter_src_clk_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                    internal_cfg.srcClkDomain = var_value;
                }

                sprintf(str1, "inter_sync_rate_%d", inst);
                if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                    internal_cfg.sync_rate = var_value;
                }
                ptp_internal_mode_config_set(inst, internal_cfg);
            }
        }
        sprintf(str1, "/ptp_clock_config.htm?clock_inst=%d", inst);
        redirect(p, str1);
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        T_D("Get Method");
        if(!cyg_httpd_start_chunked("html"))
            return -1;

        if ((vtss_appl_ptp_clock_config_default_ds_get(inst, &default_ds_cfg) == VTSS_RC_OK) &&
            (vtss_appl_ptp_clock_status_default_ds_get(inst, &default_ds_status) == VTSS_RC_OK))
        {
            if ((var_string = cyg_httpd_form_varable_string(p, "apply_profile_defaults", &len)) != NULL && len >= 4) {
                if (strncasecmp(var_string, "true", 4) == 0) {
                    ptp_apply_profile_defaults_to_default_ds(&default_ds_cfg, default_ds_cfg.profile);
                    if (vtss_appl_ptp_clock_config_default_ds_set(inst, &default_ds_cfg) != VTSS_RC_OK) {
                        T_D("Clock instance %d : does not exist", inst);
                    }

                    /* Look for all the ports that have been configured
                       for this new Clock */
                    port_iter_init_local(&pit);
                    while (port_iter_getnext(&pit)) {
                        (void) vtss_ifindex_from_port(0, pit.iport, &ifindex);
                        (void) ptp_ifindex_to_port(ifindex, &port_no);  
                        if (vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &port_cfg) == VTSS_RC_OK) {
                            new_port_cfg = port_cfg;
                            ptp_apply_profile_defaults_to_port_ds(&new_port_cfg, default_ds_cfg.profile);
        
                            if (memcmp(&new_port_cfg, &port_cfg, sizeof(vtss_appl_ptp_config_port_ds_t)) != 0) {
                                if (vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &new_port_cfg) == VTSS_RC_ERROR) {
                                    T_D("Clock instance %d : does not exist", inst);
                                }
                            }
                        }
                    }

                    if (vtss_appl_ptp_clock_config_default_ds_get(inst, &default_ds_cfg) != VTSS_RC_OK) {
                        T_D("Clock instance %d : does not exist", inst);
                    }
                }
            }
            dataSet = true;

            /* Send the Dynamic Parameters */
            if (default_ds_cfg.deviceType == VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT || default_ds_cfg.deviceType == VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT) {
#if defined(VTSS_SW_OPTION_MEP)
            if (fast_cap(MESA_CAP_TS_MISSING_PTP_ON_INTERNAL_PORTS)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^Ethernet/EthernetMixed/IPv4Multi/IPv4Mixed/IPv4Uni/Oam/OnePPS/IPv6Mixed/EthIPv4IPv6Combo");
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^Ethernet/EthernetMixed/IPv4Multi/IPv4Mixed/IPv4Uni/OnePPS/IPv6Mixed/EthIPv4IPv6Combo");
            }
#else
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^Ethernet/EthernetMixed/IPv4Multi/IPv4Mixed/IPv4Uni/OnePPS/IPv6Mixed/EthIPv4IPv6Combo");
#endif
            } else {
#if defined(VTSS_SW_OPTION_MEP)
            if (fast_cap(MESA_CAP_TS_MISSING_PTP_ON_INTERNAL_PORTS)) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^Ethernet/EthernetMixed/IPv4Multi/IPv4Mixed/IPv4Uni/Oam/OnePPS/EthIPv4IPv6Combo");
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^Ethernet/EthernetMixed/IPv4Multi/IPv4Mixed/IPv4Uni/OnePPS/EthIPv4IPv6Combo");
            }
#else
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^Ethernet/EthernetMixed/IPv4Multi/IPv4Mixed/IPv4Uni/OnePPS/EthIPv4IPv6Combo");
#endif
            }

            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            vtss_zarlink_servo_t servo_type = (vtss_zarlink_servo_t)fast_cap(VTSS_APPL_CAP_ZARLINK_SERVO_TYPE);
            /* Send the Dynamic Parameters relating to servo/filter types */
            if (servo_type == VTSS_ZARLINK_SERVO_NONE) {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^");
#if defined(SW_OPTION_BASIC_PTP_SERVO) && defined(VTSS_SW_OPTION_P802_1_AS)
                ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, "%s", "BASIC");
#endif
            } else {
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^%s",
                                                                  "ACI_BASIC_PHASE"
                                                                  "/ACI_BASIC_PHASE_LOW");
                HAS_SERVO_380(
                                                  "/ACI_DEFAULT"
                                                  "/ACI_FREQ_XO"
                                                  "/ACI_PHASE_XO"
                                                  "/ACI_FREQ_TCXO"
                                                  "/ACI_PHASE_TCXO"
                                                  "/ACI_FREQ_OCXO_S3E"
                                                  "/ACI_PHASE_OCXO_S3E"
                )
                HAS_SERVO_380(     
                                                  "/ACI_BC_PARTIAL_ON_PATH_FREQ"
                                                  "/ACI_BC_PARTIAL_ON_PATH_PHASE"
                )
                HAS_ANY_SERVO(
                                                  "/ACI_BC_FULL_ON_PATH_FREQ"
                )
                HAS_SERVO_380(    
                                                  "/ACI_BC_FULL_ON_PATH_PHASE"
                                                  "/ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE"
                )
                HAS_SERVO_380(
                                                  "/ACI_FREQ_ACCURACY_FDD"
                                                  "/ACI_FREQ_ACCURACY_XDSL"
                )
                HAS_SERVO_380(     
                                                  "/ACI_ELEC_FREQ"
                                                  "/ACI_ELEC_PHASE"
                )
                HAS_SERVO_380(     
                                                  "/ACI_PHASE_RELAXED_C60W"
                                                  "/ACI_PHASE_RELAXED_C150"
                                                  "/ACI_PHASE_RELAXED_C180"
                                                  "/ACI_PHASE_RELAXED_C240"
                )
                HAS_SERVO_380(
                                                  "/ACI_PHASE_OCXO_S3E_R4_6_1"
                )
#if defined(SW_OPTION_BASIC_PTP_SERVO) && defined(VTSS_SW_OPTION_P802_1_AS)
                ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, "%s", "/BASIC");
#endif
            }
            if (vtss_appl_ptp_clock_filter_parameters_get(inst, &filter_params) != VTSS_RC_OK) {
                T_W("Could not get filter parameters for PTP clock instance.");
            } else {
                for (int i = 0; i < PTP_CLOCK_INSTANCES; i++) {
                    vtss_appl_ptp_clock_config_default_ds_t tmp_default_ds_cfg;
    
                    if ((i != inst) && (vtss_appl_ptp_clock_config_default_ds_get(i, &tmp_default_ds_cfg) == VTSS_RC_OK)) {
                        if ((tmp_default_ds_cfg.deviceType != VTSS_APPL_PTP_DEVICE_NONE) && (tmp_default_ds_cfg.clock_domain == default_ds_cfg.clock_domain)) {
                            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^%s", ptp_filter_type_disp(default_ds_cfg.filter_type));
                            break;
                        }
                    }
                }
            }
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            (void)cyg_httpd_write_chunked("^", 1);

            /* send the local clock  */
            vtss_local_clock_time_get(&t, inst, &hw_time);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%s %s", misc_time2str(t.seconds), vtss_tod_ns2str(t.nanoseconds, str,','));
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s", clock_adjustment_method_txt(vtss_ptp_adjustment_method(inst)));
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            /* Send the port configuration */
            first_port = 1;
            port_iter_init_local(&pit);
            while (port_iter_getnext(&pit)) {
                (void) vtss_ifindex_from_port(0, pit.iport, &ifindex);

                if (vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &port_cfg) == VTSS_RC_OK) {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), first_port ? "#%u" : "/%u", port_cfg.enabled);
                } else {
                    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), first_port ? "#%u" : "/%u", 0);
                }
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                first_port = 0;
            }
            
            /* Send the virtual port configuration */
            (void)ptp_clock_config_virtual_port_config_get(inst, &virtual_port_cfg);
            struct tm* v_ptm;
            struct tm v_timeinfo;
            time_t rawtime = (time_t) virtual_port_cfg.timeproperties.leapDate * 86400;
            v_ptm = gmtime_r(&rawtime, &v_timeinfo);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%s/%d/%d/%d/%d/%d/%d/%s/%s/%d/%d/%d/%d/%s/%d/%s/%s/%s/%s/%s/%s/%d/%s/%04d-%02d-%02d/%s",
                          (virtual_port_cfg.enable ? "True" : "False"),
                          virtual_port_cfg.clockQuality.clockClass,
                          virtual_port_cfg.clockQuality.clockAccuracy,
                          virtual_port_cfg.clockQuality.offsetScaledLogVariance,
                          virtual_port_cfg.priority1,
                          virtual_port_cfg.priority2,
                          virtual_port_cfg.localPriority, 
                          ptp_virtual_port_mode_disp(virtual_port_cfg.virtual_port_mode),
                          ptp_virtual_port_tod_protocol_disp(virtual_port_cfg.proto),
                          (uint)iport2uport(virtual_port_cfg.portnum),
                          (uint)virtual_port_cfg.input_pin,
                          (uint)virtual_port_cfg.output_pin,
                          (uint)virtual_port_cfg.delay,
                          virtual_port_cfg.alarm ? "True" : "False",
                          virtual_time_prop->currentUtcOffset,
                          ptp_bool_disp(virtual_time_prop->currentUtcOffsetValid),
                          ptp_bool_disp(virtual_time_prop->leap59),
                          ptp_bool_disp(virtual_time_prop->leap61),
                          ptp_bool_disp(virtual_time_prop->timeTraceable),
                          ptp_bool_disp(virtual_time_prop->frequencyTraceable),
                          ptp_bool_disp(virtual_time_prop->ptpTimescale),
                          virtual_time_prop->timeSource,
                          virtual_time_prop->pendingLeap ? "True" : "False",
                          v_ptm->tm_year + 1900,
                          v_ptm->tm_mon + 1,
                          v_ptm->tm_mday,
                          (virtual_time_prop->leapType == VTSS_APPL_PTP_LEAP_SECOND_59) ? "leap59" : "leap61");
            vtss_appl_clock_identity *c_id = &virtual_port_cfg.clock_identity;
            ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, "/%x", (*c_id)[0]);
            for (auto i = 1; i < 8; i++) {
                ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, ":%x", (*c_id)[i]);
            }
            ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, "/%d", virtual_port_cfg.steps_removed);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);

            /* Send the Default clock Data Set */
            if (default_ds_cfg.deviceType == VTSS_APPL_PTP_DEVICE_NONE) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        " #%d /%s", inst,
                        DeviceTypeToString(default_ds_cfg.deviceType));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            } else {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "#%d/%d/%s/%s/%d/%s/%d",
                        inst, default_ds_cfg.clock_domain, DeviceTypeToString(default_ds_cfg.deviceType),
                        (default_ds_cfg.twoStepFlag) ? "True" : "False",
                         default_ds_status.numberPorts,
                         ClockIdentityToString(default_ds_status.clockIdentity, str1),
                         default_ds_cfg.domainNumber);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);

                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "/%s/%d/%d",
                        ClockQualityToString(&default_ds_status.clockQuality, str2),
                        default_ds_cfg.priority1,
                        default_ds_cfg.priority2);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "/%s/%s", ptp_protocol_disp(default_ds_cfg.protocol),
                        (default_ds_cfg.oneWay) ? "True" : "False");
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/%s/%d",
                         "False",   // was the tagging enable parameter, that is not used any more (the value is kept in the list, otherwise all the indexed in the command string must be changed in the HTM files
                         (default_ds_cfg.configured_pcp));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/%d",
                              (default_ds_cfg.dscp));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/%d", default_ds_cfg.profile);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/%d",
                         default_ds_cfg.localPriority);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
#if defined (VTSS_SW_OPTION_P802_1_AS)
                if (default_ds_cfg.profile == 4) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/%s",
                    (default_ds_status.s_802_1as.gmCapable) ? "True" : "False");
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/0x%03x",
                    default_ds_status.s_802_1as.sdoId);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }
                if (default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                    ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/%s",
                    (default_ds_cfg.deviceType == VTSS_APPL_PTP_DEVICE_AED_GM) ? "True" : "False");
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
            }

            /* Send the current Clock Data Set */
            if (vtss_appl_ptp_clock_status_current_ds_get(inst, &clock_current_bs) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
#if defined (VTSS_SW_OPTION_P802_1_AS)
                        "#%d/%s/%s/%s/%lf/%d/%d/%d/%d/%d",
#else
                        "#%d/%s/%s",
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                        clock_current_bs.stepsRemoved, vtss_tod_TimeInterval_To_String(&clock_current_bs.offsetFromMaster, str1,','),
                        vtss_tod_TimeInterval_To_String(&clock_current_bs.meanPathDelay, str2,',')
#if defined (VTSS_SW_OPTION_P802_1_AS)
                        , vtss_tod_ScaledNs_To_String(&clock_current_bs.cur_802_1as.lastGMPhaseChange, str3,','),
                        clock_current_bs.cur_802_1as.lastGMFreqChange,
                        clock_current_bs.cur_802_1as.gmTimeBaseIndicator,
                        clock_current_bs.cur_802_1as.gmChangeCount,
                        clock_current_bs.cur_802_1as.timeOfLastGMChangeEvent,
                        clock_current_bs.cur_802_1as.timeOfLastGMPhaseChangeEvent,
                        clock_current_bs.cur_802_1as.timeOfLastGMFreqChangeEvent
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                        );
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            if (vtss_appl_ptp_clock_status_parent_ds_get(inst, &clock_parent_bs) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "#%s/%d/%s/%d/%d",
                        ClockIdentityToString(clock_parent_bs.parentPortIdentity.clockIdentity, str1), clock_parent_bs.parentPortIdentity.portNumber,
                        ptp_bool_disp(clock_parent_bs.parentStats),
                        clock_parent_bs.observedParentOffsetScaledLogVariance,
                        clock_parent_bs.observedParentClockPhaseChangeRate);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);

                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
#if defined (VTSS_SW_OPTION_P802_1_AS)
                        "/%s/%s/%d/%d/%d",
#else
                        "/%s/%s/%d/%d",
#endif //defined (VTSS_SW_OPTION_P802_1_AS)

                        ClockIdentityToString(clock_parent_bs.grandmasterIdentity, str2),
                        ClockQualityToString(&clock_parent_bs.grandmasterClockQuality, str3),
                        clock_parent_bs.grandmasterPriority1,
                        clock_parent_bs.grandmasterPriority2
#if defined (VTSS_SW_OPTION_P802_1_AS)
                        , clock_parent_bs.par_802_1as.cumulativeRateRatio
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                        );
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            if (vtss_appl_ptp_clock_config_timeproperties_ds_get(inst, &clock_time_properties_bs) == VTSS_RC_OK) {
                struct tm* ptm;
                struct tm timeinfo;
                time_t rawtime = (time_t) clock_time_properties_bs.leapDate * 86400;
                ptm = gmtime_r(&rawtime, &timeinfo);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "#%d/%s/%s/%s/%s/%s/%s/%d/%s/%04d-%02d-%02d/%s",
                        clock_time_properties_bs.currentUtcOffset,
                        ptp_bool_disp(clock_time_properties_bs.currentUtcOffsetValid),
                        ptp_bool_disp(clock_time_properties_bs.leap59),
                        ptp_bool_disp(clock_time_properties_bs.leap61),
                        ptp_bool_disp(clock_time_properties_bs.timeTraceable),
                        ptp_bool_disp(clock_time_properties_bs.frequencyTraceable),
                        ptp_bool_disp(clock_time_properties_bs.ptpTimescale),
                        clock_time_properties_bs.timeSource,
                        clock_time_properties_bs.pendingLeap ? "True" : "False",
                        ptm->tm_year + 1900,
                        ptm->tm_mon + 1,
                        ptm->tm_mday,
                        (clock_time_properties_bs.leapType == VTSS_APPL_PTP_LEAP_SECOND_59) ? "leap59" : "leap61");
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            /* send the servo parameters  */
            if (vtss_appl_ptp_clock_servo_parameters_get(inst, &default_servo) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "#%s/%s/%s/%s/%d/%d/%d/%d",
                        ptp_bool_disp(default_servo.display_stats),
                        ptp_bool_disp(default_servo.p_reg),
                        ptp_bool_disp(default_servo.i_reg),
                        ptp_bool_disp(default_servo.d_reg),
                        default_servo.ap,
                        default_servo.ai,
                        default_servo.ad,
                        default_servo.gain);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            /* send the filter parameters  */
            if (vtss_appl_ptp_clock_filter_parameters_get(inst, &filter_params) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "#%d/%s/%d/%d",
                        filter_params.delay_filter,
                        ptp_filter_type_disp(default_ds_cfg.filter_type),
                        filter_params.period,
                        filter_params.dist);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }

            /* send the unicast slave parameters */
            (void)cyg_httpd_write_chunked("#", 1);
            ix = 0;
            while ((vtss_appl_ptp_clock_config_unicast_slave_config_get(inst, ix, &uni_slave_cfg) == VTSS_RC_OK) &&
                   (vtss_appl_ptp_clock_status_unicast_slave_table_get(inst, ix++, &uni_slave_status) == VTSS_RC_OK))
            {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "%u/%d/%s/%hhd/%s|",
                        (ix-1),
                        uni_slave_cfg.duration,
                        misc_ipv4_txt(uni_slave_cfg.ip_addr, str1),
                        uni_slave_status.log_msg_period,
                        ptp_state_disp(uni_slave_status.comm_state));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        if (!dataSet)
            (void)cyg_httpd_write_chunked("",1);

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^%s/%s/%s%s%s%s%s%s%s%s%s",
                ptp_profiles[0],
                ptp_profiles[1],
                ptp_profiles[2],
                "/", ptp_profiles[3],
                "/", ptp_profiles[4],
#if defined (VTSS_SW_OPTION_P802_1_AS)
                "/", ptp_profiles[5],
                "/", ptp_profiles[6]
#else
                "", ""
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                );
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

         /* send the virtual-port mode  parameters  */
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^");
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        if (MESA_CAP(MESA_CAP_TS_PTP_RS422)){
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "main-auto/main-man/sub/");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }
        for(int i =0; i<fast_cap(MESA_CAP_TS_IO_CNT); i++){
             if(VTSS_IS_IO_PIN_IN(ptp_io_pin[i].board_assignments)){
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "pps-in/");
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
             }
             break;
        }
        for(int i =0; i<fast_cap(MESA_CAP_TS_IO_CNT); i++){
             if (VTSS_IS_IO_PIN_OUT(ptp_io_pin[i].board_assignments)){
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "pps-out/freq-out/");
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
             }
             break;
        }
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "none");
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
         /* send the virtual-port tod protocol  parameters  */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^polyt/rmc/zda/pim/none");

            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
      
         /* send pim port nos */
         first_port = 1;
         port_iter_init_local(&pit);
         while (port_iter_getnext(&pit)) {
            (void) vtss_ifindex_from_port(0, pit.iport, &ifindex);
            (void) ptp_ifindex_to_port(ifindex, &port_no);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), first_port? "^%d":"/%d",((uint) iport2uport(port_no)));
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            first_port =0;
         }
        
         /* send I/O PIN numbers in pps-in mode*/
         first_port = 1;
         ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^");
         (void)cyg_httpd_write_chunked(p->outbuffer, ct);
         for(int i =0; i<fast_cap(MESA_CAP_TS_IO_CNT); i++){
             if(VTSS_IS_IO_PIN_IN(ptp_io_pin[i].board_assignments)){
                 ct = snprintf(p->outbuffer, sizeof(p->outbuffer), first_port? "%d":"/%d", i);
                 (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                 first_port = 0;
             }
         }

        /* send I/O PIN numbers in pps-out/freq-out mode*/
         first_port = 1; 
         ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^");
         (void)cyg_httpd_write_chunked(p->outbuffer, ct);
         for(int i =0; i<fast_cap(MESA_CAP_TS_IO_CNT); i++){
             if(VTSS_IS_IO_PIN_OUT(ptp_io_pin[i].board_assignments)){
                 ct = snprintf(p->outbuffer, sizeof(p->outbuffer), first_port? "%d":"/%d", i);
                 (void)cyg_httpd_write_chunked(p->outbuffer, ct); 
                 first_port = 0; 
             }
         }

         // Internal Mode Configuration.
         ptp_internal_mode_config_get(inst, &internal_cfg);
         ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^");
         (void)cyg_httpd_write_chunked(p->outbuffer, ct);
         ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%d", internal_cfg.srcClkDomain, internal_cfg.sync_rate);
         (void)cyg_httpd_write_chunked(p->outbuffer, ct);

         cyg_httpd_end_chunked();

    }
    return -1; // Do not further search the file system.
}


static i32 handler_clock_ports_config_ptp(CYG_HTTPD_STATE* p)
{
    i32    ct;
    size_t len;
    u8 mech_type;
    uint inst = 0;
    int    var_value;
    mesa_timeinterval_t latency;
    vtss_appl_ptp_clock_status_default_ds_t default_ds_status;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_appl_ptp_config_port_ds_t port_cfg;
    vtss_appl_ptp_config_port_ds_t new_port_cfg;
    vtss_appl_ptp_status_port_ds_t port_status;
    vtss_ifindex_t ifindex;
    switch_iter_t sit;
    u32 port_no;
    bool dataSet = false;
    const char *var_string;
    char str1[50];
    char pmpd[50];
#if defined (VTSS_SW_OPTION_P802_1_AS)
    char srti[50];
    vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t cmlds_port_cfg;
    vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t new_cmlds_port_cfg = {};
    u8 aedState;
#endif
    char str2[40];
    port_iter_t       pit;
    mesa_rc port_rc;
    static mesa_rc error_no = VTSS_RC_OK;
    static vtss_uport_no_t error_port = 0;
    char err_str[60];

    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG);

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PTP))
        return -1;
#endif

    if (cyg_httpd_form_varable_int(p, "clock_inst", &var_value)) {
        inst = var_value;
    }
    if (p->method == CYG_HTTPD_METHOD_POST) {
        T_D("Inside CYG_HTTPD_METHOD_POST post_data-[%s]",p->post_data);
        if ((vtss_appl_ptp_clock_config_default_ds_get(inst, &default_ds_cfg) == VTSS_RC_OK) &&
            (vtss_appl_ptp_clock_status_default_ds_get(inst, &default_ds_status) == VTSS_RC_OK))
        {

            error_no = VTSS_RC_OK;
            error_port = 0;
            port_iter_init_local(&pit);
            while (port_iter_getnext(&pit)) {
                (void) vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex);
                (void) ptp_ifindex_to_port(ifindex, &port_no);
                if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &port_cfg) == VTSS_RC_OK) &&
#if defined (VTSS_SW_OPTION_P802_1_AS)
                    (vtss_appl_ptp_cmlds_port_conf_get(pit.uport, &cmlds_port_cfg) == VTSS_RC_OK) &&
#endif
                    (port_cfg.enabled == 1))
                {
                    new_port_cfg = port_cfg;

                    sprintf(str1, "anv_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                        new_port_cfg.logAnnounceInterval = var_value;
                    }

                    sprintf(str1, "ato_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                        new_port_cfg.announceReceiptTimeout = var_value;
                    }

                    sprintf(str1, "syv_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                        new_port_cfg.logSyncInterval = var_value;
                    }

                    sprintf(str1, "dlm_%d_%d", inst, (uint) iport2uport(port_no));
                    if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                        if (len > 0)
                            (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                        if (ptp_get_delaymechanism_type(str2, &mech_type)) {
                            new_port_cfg.delayMechanism = vtss_appl_ptp_delay_mechanism_t(mech_type);
                        }
                    }

                    sprintf(str1, "mpr_%d_%d", inst, (uint)iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                        new_port_cfg.logMinPdelayReqInterval = var_value;
                    }
                    sprintf(str1,"delay_assymetry_%d_%d", inst, (uint) iport2uport(port_no));
                    var_value = 0;
                    if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                        latency = 0;
                        latency = (mesa_timeinterval_t) var_value << 16;
                        new_port_cfg.delayAsymmetry = latency;
                    }
                    sprintf(str1,"ingress_latency_%d_%d", inst, (uint) iport2uport(port_no));
                    var_value = 0;
                    if (cyg_httpd_form_varable_int(p, str1, &var_value) ) {
                        latency = 0;
                        latency = (mesa_timeinterval_t) var_value << 16;
                        new_port_cfg.ingressLatency = latency;
                    }
                    sprintf(str1,"egress_latency_%d_%d",inst,(uint)iport2uport(port_no));
                    var_value = 0;
                    if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                        latency = 0;
                        latency = (mesa_timeinterval_t) var_value << 16;
                        new_port_cfg.egressLatency = latency;
                    }
                    sprintf(str1, "mcast_addr_%d_%d", inst, (uint) iport2uport(port_no));
                    if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                        if (len > 0) {
                            (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                            if (!strcmp(str2, "Link-local")) {
                                new_port_cfg.dest_adr_type = VTSS_APPL_PTP_PROTOCOL_SELECT_LINK_LOCAL;
                            } else {
                                new_port_cfg.dest_adr_type = VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT;
                            }
                        }
                    }
                    sprintf(str1, "master_only_%d_%d", inst, (uint) iport2uport(port_no));
                    if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                        if (len > 0) {
                            (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                            if (!strcmp(str2, "True")) {
                                new_port_cfg.masterOnly = true;
                            } else {
                                new_port_cfg.masterOnly = false;
                            }
                        }
                    }
                    sprintf(str1, "local_prio_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_port_cfg.localPriority = var_value;
                    sprintf(str1, "2_step_flag_%d_%d", inst, (uint) iport2uport(port_no));
                    if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                        if (len > 0) {
                            new_port_cfg.twoStepOverride = VTSS_APPL_PTP_TWO_STEP_OVERRIDE_NONE;
                            (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                            if (!strcmp(str2, "False")) {
                                new_port_cfg.twoStepOverride = VTSS_APPL_PTP_TWO_STEP_OVERRIDE_FALSE;
                            } else if (!strcmp(str2, "True")) {
                                new_port_cfg.twoStepOverride = VTSS_APPL_PTP_TWO_STEP_OVERRIDE_TRUE;
                            }
                        }
                    }
                    sprintf(str1, "not_mstr_%d_%d", inst, (uint) iport2uport(port_no));
                    if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                        if (len > 0) {
                            (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                            if (!strcmp(str2, "True")) {
                                new_port_cfg.notMaster = true;
                            } else {
                                new_port_cfg.notMaster = false;
                            }
                        }
                    }
#if defined (VTSS_SW_OPTION_P802_1_AS)
                    sprintf(str1, "as_2020_%d_%d", inst, (uint) iport2uport(port_no));
                    if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                        if (len > 0) {
                            (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                            if (!strcmp(str2, "True")) {
                                new_port_cfg.c_802_1as.as2020 = true;
                            } else {
                                new_port_cfg.c_802_1as.as2020 = false;
                            }
                        }
                    }
                    sprintf(str1, "meanLinkDelayThresh_%d_%d", inst, (uint) iport2uport(port_no));
                    var_value = 0;
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_port_cfg.c_802_1as.peer_d.meanLinkDelayThresh = (mesa_timeinterval_t) var_value << 16;
                    sprintf(str1, "syncReceiptTimeout_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_port_cfg.c_802_1as.syncReceiptTimeout = var_value;
                    sprintf(str1, "allowedLostResponses_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_port_cfg.c_802_1as.peer_d.allowedLostResponses = var_value;
                    sprintf(str1, "allowedFaults_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_port_cfg.c_802_1as.peer_d.allowedFaults = var_value;
                    sprintf(str1, "useMgmtSyncIntrvl_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_find(p, str1)){
                        new_port_cfg.c_802_1as.useMgtSettableLogSyncInterval = true;
                    } else {
                        new_port_cfg.c_802_1as.useMgtSettableLogSyncInterval = false;
                    }
                    sprintf(str1, "MgmtSyncIntrvl_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_port_cfg.c_802_1as.mgtSettableLogSyncInterval = var_value;
                    
                    sprintf(str1, "useMgmtAnnIntrvl_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_find(p, str1)){
                        new_port_cfg.c_802_1as.useMgtSettableLogAnnounceInterval = true;
                    } else {
                        new_port_cfg.c_802_1as.useMgtSettableLogAnnounceInterval = false;
                    }
                    sprintf(str1, "MgmtAnnIntrvl_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_port_cfg.c_802_1as.mgtSettableLogAnnounceInterval = var_value;
                    
                    sprintf(str1, "useMgmtPdlyIntrvl_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_find(p, str1)){
                        new_port_cfg.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval = true;
                    } else {
                        new_port_cfg.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval = false;
                    }
                    sprintf(str1, "MgmtPdlyIntrvl_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_port_cfg.c_802_1as.peer_d.mgtSettableLogPdelayReqInterval = var_value;
                    sprintf(str1, "uMSCNRR_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_find(p, str1)){
                        new_port_cfg.c_802_1as.peer_d.useMgtSettableComputeNeighborRateRatio = true;
                    } else {
                        new_port_cfg.c_802_1as.peer_d.useMgtSettableComputeNeighborRateRatio = false;
                    }
                    sprintf(str1, "MSCNRR_%d_%d", inst, (uint) iport2uport(port_no));
                    if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                        if (len > 0) {
                            (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                            if (!strcmp(str2, "True")) {
                                new_port_cfg.c_802_1as.peer_d.mgtSettableComputeNeighborRateRatio = true;
                            } else {
                                new_port_cfg.c_802_1as.peer_d.mgtSettableComputeNeighborRateRatio = false;
                            }
                        }
                    }
                    sprintf(str1, "uMSCMLD_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_find(p, str1)){
                        new_port_cfg.c_802_1as.peer_d.useMgtSettableComputeMeanLinkDelay = true;
                    } else {
                        new_port_cfg.c_802_1as.peer_d.useMgtSettableComputeMeanLinkDelay = false;
                    }
                    sprintf(str1, "MSCMLD_%d_%d", inst, (uint) iport2uport(port_no));
                    if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                        if (len > 0) {
                            (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                            if (!strcmp(str2, "True")) {
                                new_port_cfg.c_802_1as.peer_d.mgtSettableComputeMeanLinkDelay = true;
                            } else {
                                new_port_cfg.c_802_1as.peer_d.mgtSettableComputeMeanLinkDelay = false;
                            }
                        }
                    }
                    
                    sprintf(str1, "useMgmtGptpCapIntrvl_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_find(p, str1)){
                        new_port_cfg.c_802_1as.useMgtSettableLogGptpCapableMessageInterval = true;
                    } else {
                        new_port_cfg.c_802_1as.useMgtSettableLogGptpCapableMessageInterval = false;
                    }
                    sprintf(str1, "MgmtGptpCapIntrvl_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_port_cfg.c_802_1as.mgtSettableLogGptpCapableMessageInterval = var_value;

                    sprintf(str1, "GptpCapableReceiptTimeout_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_port_cfg.c_802_1as.gPtpCapableReceiptTimeout = var_value;

                    sprintf(str1, "GptpCapableLogInterval_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_port_cfg.c_802_1as.initialLogGptpCapableMessageInterval = var_value;
                    sprintf(str1, "MLDT_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value)) {
                        new_cmlds_port_cfg.peer_d.meanLinkDelayThresh = (mesa_timeinterval_t) var_value << 16;
                    }
                    sprintf(str1, "DA_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_cmlds_port_cfg.delayAsymmetry = (mesa_timeinterval_t) var_value << 16;
                    sprintf(str1, "uMSLPDRv_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_find(p, str1)){
                        new_cmlds_port_cfg.peer_d.useMgtSettableLogPdelayReqInterval = true;
                    } else {
                        new_cmlds_port_cfg.peer_d.useMgtSettableLogPdelayReqInterval = false;
                    }
                    sprintf(str1, "MSLPDRv_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_cmlds_port_cfg.peer_d.mgtSettableLogPdelayReqInterval = var_value;
                    sprintf(str1, "cm_uMSCNRR_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_find(p, str1)){
                        new_cmlds_port_cfg.peer_d.useMgtSettableComputeNeighborRateRatio = true;
                    } else {
                        new_cmlds_port_cfg.peer_d.useMgtSettableComputeNeighborRateRatio = false;
                    }
                    sprintf(str1, "cm_MSCNRR_%d_%d", inst, (uint) iport2uport(port_no));
                    if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                        if (len > 0) {
                            (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                            if (!strcmp(str2, "True")) {
                                new_cmlds_port_cfg.peer_d.mgtSettableComputeNeighborRateRatio = true;
                            } else {
                                new_cmlds_port_cfg.peer_d.mgtSettableComputeNeighborRateRatio = false;
                            }
                        }
                    }
                    sprintf(str1, "cm_uMSCMLD_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_find(p, str1)){
                        new_cmlds_port_cfg.peer_d.useMgtSettableComputeMeanLinkDelay = true;
                    } else {
                        new_cmlds_port_cfg.peer_d.useMgtSettableComputeMeanLinkDelay = false;
                    }
                    sprintf(str1, "cm_MSCMLD_%d_%d", inst, (uint) iport2uport(port_no));
                    if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                        if (len > 0) {
                            (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                            if (!strcmp(str2, "True")) {
                                new_cmlds_port_cfg.peer_d.mgtSettableComputeMeanLinkDelay = true;
                            } else {
                                new_cmlds_port_cfg.peer_d.mgtSettableComputeMeanLinkDelay = false;
                            }
                        }
                    }
                    sprintf(str1, "cm_ALR_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_cmlds_port_cfg.peer_d.allowedLostResponses = var_value;
                    sprintf(str1, "cm_AFs_%d_%d", inst, (uint) iport2uport(port_no));
                    if (cyg_httpd_form_varable_int(p, str1, &var_value))
                        new_cmlds_port_cfg.peer_d.allowedFaults = var_value;
                    /* The below variables cannot be modified but displayed in web. For the sake of comparision, these values are filled. */
                    new_cmlds_port_cfg.peer_d.initialLogPdelayReqInterval = cmlds_port_cfg.peer_d.initialLogPdelayReqInterval;
                    new_cmlds_port_cfg.peer_d.initialComputeNeighborRateRatio = cmlds_port_cfg.peer_d.initialComputeNeighborRateRatio;
                    new_cmlds_port_cfg.peer_d.initialComputeMeanLinkDelay = cmlds_port_cfg.peer_d.initialComputeMeanLinkDelay;
                    if (memcmp(&new_cmlds_port_cfg, &cmlds_port_cfg, sizeof(vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t)) != 0) {
                        port_rc = vtss_appl_ptp_cmlds_port_conf_set(pit.uport, &new_cmlds_port_cfg);
                        if (port_rc != VTSS_RC_OK) {
                            error_no = port_rc;
                            error_port = port_no;
                            T_I(", port %u : error setting cmlds port data set", pit.uport);
                        }
                    }
                    // 802.1as-AED specific configuration
                    if (default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                        sprintf(str1, "aed_role_%d_%d", inst, (uint) iport2uport(port_no));
                        if ((var_string = cyg_httpd_form_varable_string(p, str1, &len))) {
                            if (len > 0)
                                (void)cgi_unescape(var_string, &str2[0], len, sizeof(str2));
                            if (ptp_get_aed_port_state(str2, &aedState)) {
                                new_port_cfg.aedPortState = vtss_appl_ptp_802_1as_aed_port_state_t(aedState);
                            }
                        }
                        sprintf(str1, "operLogPdelayReqInterval_%d_%d", inst, (uint) iport2uport(port_no));
                        if (cyg_httpd_form_varable_int(p, str1, &var_value))
                            new_port_cfg.c_802_1as.peer_d.operLogPdelayReqInterval = var_value;
                        sprintf(str1, "initialLogSyncInterval_%d_%d", inst, (uint) iport2uport(port_no));
                        if (cyg_httpd_form_varable_int(p, str1, &var_value))
                            new_port_cfg.c_802_1as.initialLogSyncInterval = var_value;
                        sprintf(str1, "operLogSyncInterval_%d_%d", inst, (uint) iport2uport(port_no));
                        if (cyg_httpd_form_varable_int(p, str1, &var_value))
                            new_port_cfg.c_802_1as.operLogSyncInterval = var_value;
                    }
#endif //defined (VTSS_SW_OPTION_P802_1_AS)

                    if (memcmp(&new_port_cfg, &port_cfg, sizeof(vtss_appl_ptp_config_port_ds_t)) != 0) {
                        port_rc = vtss_appl_ptp_config_clocks_port_ds_set(inst, ifindex, &new_port_cfg);
                        if (port_rc != VTSS_RC_OK) {
                            error_no = port_rc;
                            error_port = port_no;
                            T_I("Clock instance %d, port %u : error setting port data set", inst, iport2uport(port_no));
                        }
                    }
                }
            }
        }
        sprintf(str1, "/ptp_clock_ports_config.htm?clock_inst=%d", inst);
        redirect(p, str1);
    } else {                    /* CYG_HTTPD_METHOD_GET (+HEAD) */
        T_D("Get Method");
        if(!cyg_httpd_start_chunked("html"))
            return -1;
        if ((vtss_appl_ptp_clock_config_default_ds_get(inst, &default_ds_cfg) == VTSS_RC_OK) &&
            (vtss_appl_ptp_clock_status_default_ds_get(inst, &default_ds_status) == VTSS_RC_OK))
        {
            dataSet = true;
            /* Delay Mechanism */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^e2e/p2p");
#if defined (VTSS_SW_OPTION_P802_1_AS)
            ct += snprintf(p->outbuffer + ct, sizeof(p->outbuffer) - ct, "/cp2p");
#endif
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            /* Multicast address option */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^Default/Link-local");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            /* 2-step option */
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^Clock Def./False/True");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            ct = default_ds_cfg.deviceType == VTSS_APPL_PTP_DEVICE_AED_GM ? snprintf(p->outbuffer, sizeof(p->outbuffer), "^AED-Master") : snprintf(p->outbuffer, sizeof(p->outbuffer), "^AED-Master/AED-Slave");
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            (void)cyg_httpd_write_chunked("^", 1);
            (void)cyg_httpd_write_chunked("#", 1);
            port_iter_init_local(&pit);
            while (port_iter_getnext(&pit)) {
                (void) vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex);
                (void) ptp_ifindex_to_port(ifindex, &port_no);
                if ((vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &port_cfg) == VTSS_RC_OK) &&
                    (vtss_appl_ptp_status_clocks_port_ds_get(inst, ifindex, &port_status) == VTSS_RC_OK) &&
#if defined (VTSS_SW_OPTION_P802_1_AS)
                    (vtss_appl_ptp_cmlds_port_conf_get(pit.uport ,&cmlds_port_cfg) == VTSS_RC_OK) &&
#endif
                    (port_cfg.enabled == 1))
                {
                    if (error_no != VTSS_RC_OK && port_no == error_port) {
                        (void)snprintf(err_str, sizeof(err_str), "Error setting parameter for port no %d", iport2uport(error_port));
                        T_I("Clock instance %d, error 0x%x", inst, error_no);
                        error_no = VTSS_RC_OK;
                    } else {
                        err_str[0] = 0;
                    }
                    if (default_ds_cfg.profile != VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                    ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
#if defined (VTSS_SW_OPTION_P802_1_AS)
                            "$%d/%s/%d/%s/%d/%d/%d/%s/%d/" VPRI64d "/" VPRI64d "/" VPRI64d "/%d/%s/%s/%d/%s/%s/%s/%s/%s/%s/%d/%d/%d/%s/%d/%s/%d/%s/" VPRI64d "/%d/%d/%d/%u/%d/%u/%d/%u/%d/%u/%s/%u/%s/%u/%d/%d/%d/" VPRI64d "/" VPRI64d "/%d/%u/%d/%s/%u/%s/%s/%u/%s/%d/%d",
#else
                            "$%d/%s/%d/%s/%d/%d/%d/%s/%d/" VPRI64d "/" VPRI64d "/" VPRI64d "/%d/%s/%s/%d/%s/%s",
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                            port_status.portIdentity.portNumber,
                            PortStateToString(port_status.portState),
                            port_status.logMinDelayReqInterval,
                            vtss_tod_TimeInterval_To_String(&port_status.peerMeanPathDelay, pmpd,','),
                            port_cfg.logAnnounceInterval,
                            port_cfg.announceReceiptTimeout,
                            port_cfg.logSyncInterval,
                            ptp_delaymechanism_disp(port_cfg.delayMechanism),
                            port_cfg.logMinPdelayReqInterval,
                            (port_cfg.delayAsymmetry >> 16),
                            (port_cfg.ingressLatency >> 16),
                            (port_cfg.egressLatency >> 16),
                            port_cfg.versionNumber,
                            (port_cfg.dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_LINK_LOCAL) ? "Link-local" : "Default",
                            port_cfg.masterOnly ? "True" : "False",
                            port_cfg.localPriority,
                            ptp_two_step_override_disp(port_cfg.twoStepOverride),
                            port_cfg.notMaster ? "True" : "False",
                            err_str
#if defined (VTSS_SW_OPTION_P802_1_AS)
                            , web_port_role_disp(port_status.s_802_1as.portRole),
                            port_status.s_802_1as.peer_d.isMeasuringDelay ? "True" : "False",
                            port_status.s_802_1as.asCapable ? "True" : "False",
                            port_status.s_802_1as.peer_d.neighborRateRatio,
                            port_status.s_802_1as.currentLogAnnounceInterval,
                            port_status.s_802_1as.currentLogSyncInterval,
                            vtss_tod_TimeInterval_To_String(&port_status.s_802_1as.syncReceiptTimeInterval, srti, ','),
                            port_status.s_802_1as.peer_d.currentLogPDelayReqInterval,
                            port_status.s_802_1as.acceptableMasterTableEnabled ? "TRUE" : "FALSE",
                            port_status.s_802_1as.peer_d.versionNumber,
                            port_cfg.c_802_1as.as2020 ? "True" : "False",
                            (port_cfg.c_802_1as.peer_d.meanLinkDelayThresh >> 16),
                            port_cfg.c_802_1as.syncReceiptTimeout,
                            port_cfg.c_802_1as.peer_d.allowedLostResponses,
                            port_cfg.c_802_1as.peer_d.allowedFaults,
                            port_cfg.c_802_1as.useMgtSettableLogSyncInterval? 1 : 0,
                            port_cfg.c_802_1as.mgtSettableLogSyncInterval,
                            port_cfg.c_802_1as.useMgtSettableLogAnnounceInterval? 1 : 0,
                            port_cfg.c_802_1as.mgtSettableLogAnnounceInterval,
                            port_cfg.c_802_1as.peer_d.useMgtSettableLogPdelayReqInterval? 1 : 0,
                            port_cfg.c_802_1as.peer_d.mgtSettableLogPdelayReqInterval,
                            port_cfg.c_802_1as.peer_d.useMgtSettableComputeNeighborRateRatio? 1 : 0,
                            port_cfg.c_802_1as.peer_d.mgtSettableComputeNeighborRateRatio ? "True" : "False",
                            port_cfg.c_802_1as.peer_d.useMgtSettableComputeMeanLinkDelay? 1 : 0,
                            port_cfg.c_802_1as.peer_d.mgtSettableComputeMeanLinkDelay ? "True" : "False",
                            port_cfg.c_802_1as.useMgtSettableLogGptpCapableMessageInterval? 1 : 0,
                            port_cfg.c_802_1as.mgtSettableLogGptpCapableMessageInterval,
                            port_cfg.c_802_1as.gPtpCapableReceiptTimeout,
                            port_cfg.c_802_1as.initialLogGptpCapableMessageInterval,
                            (cmlds_port_cfg.peer_d.meanLinkDelayThresh >> 16),
                            (cmlds_port_cfg.delayAsymmetry >> 16),
                            cmlds_port_cfg.peer_d.initialLogPdelayReqInterval,
                            cmlds_port_cfg.peer_d.useMgtSettableLogPdelayReqInterval? 1 : 0,
                            cmlds_port_cfg.peer_d.mgtSettableLogPdelayReqInterval,
                            cmlds_port_cfg.peer_d.initialComputeNeighborRateRatio ? "True" : "False",
                            cmlds_port_cfg.peer_d.useMgtSettableComputeNeighborRateRatio? 1 : 0,
                            cmlds_port_cfg.peer_d.mgtSettableComputeNeighborRateRatio ? "True" : "False",
                            cmlds_port_cfg.peer_d.initialComputeMeanLinkDelay? "True" : "False",
                            cmlds_port_cfg.peer_d.useMgtSettableComputeMeanLinkDelay? 1 : 0,
                            cmlds_port_cfg.peer_d.mgtSettableComputeMeanLinkDelay ? "True" : "False",
                            cmlds_port_cfg.peer_d.allowedLostResponses,
                            cmlds_port_cfg.peer_d.allowedFaults
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                            );
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                    } else {
                        ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                            "$%d/%s/%d/%s/%d/%d/%d/%s/%d/" VPRI64d "/" VPRI64d "/" VPRI64d "/%d/%s/%s/%d/%s/%s/%s/%s/%s/%s/%d/%d/%s/%d/%s/%d/" VPRI64d "/%d/%d/%d/%d/%d/%d/%d",
                            port_status.portIdentity.portNumber,
                            PortStateToString(port_status.portState),
                            port_status.logMinDelayReqInterval,
                            vtss_tod_TimeInterval_To_String(&port_status.peerMeanPathDelay, pmpd,','),
                            port_cfg.logAnnounceInterval,
                            port_cfg.announceReceiptTimeout,
                            port_cfg.logSyncInterval,
                            ptp_delaymechanism_disp(port_cfg.delayMechanism),
                            port_cfg.logMinPdelayReqInterval,
                            (port_cfg.delayAsymmetry >> 16),
                            (port_cfg.ingressLatency >> 16),
                            (port_cfg.egressLatency >> 16),
                            port_cfg.versionNumber,
                            (port_cfg.dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_LINK_LOCAL) ? "Link-local" : "Default",
                            port_cfg.masterOnly ? "True" : "False",
                            port_cfg.localPriority,
                            ptp_two_step_override_disp(port_cfg.twoStepOverride),
                            port_cfg.notMaster ? "True" : "False",
                            err_str,
                            ptp_aed_port_state_disp(port_cfg.aedPortState),
                            port_status.s_802_1as.peer_d.isMeasuringDelay ? "True" : "False",
                            port_status.s_802_1as.asCapable ? "True" : "False",
                            port_status.s_802_1as.peer_d.neighborRateRatio,
                            port_status.s_802_1as.currentLogSyncInterval,
                            vtss_tod_TimeInterval_To_String(&port_status.s_802_1as.syncReceiptTimeInterval, srti, ','),
                            port_status.s_802_1as.peer_d.currentLogPDelayReqInterval,
                            port_status.s_802_1as.acceptableMasterTableEnabled ? "TRUE" : "FALSE",
                            port_status.s_802_1as.peer_d.versionNumber,
                            (port_cfg.c_802_1as.peer_d.meanLinkDelayThresh >> 16),
                            port_cfg.c_802_1as.syncReceiptTimeout,
                            port_cfg.c_802_1as.peer_d.allowedLostResponses,
                            port_cfg.c_802_1as.peer_d.allowedFaults,
                            port_cfg.c_802_1as.peer_d.initialLogPdelayReqInterval,
                            port_cfg.c_802_1as.peer_d.operLogPdelayReqInterval,
                            port_cfg.c_802_1as.initialLogSyncInterval,
                            port_cfg.c_802_1as.operLogSyncInterval);
                            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                    }
                }
            }
        }
        if (!dataSet)
            (void)cyg_httpd_write_chunked("",1);

        if ((vtss_appl_ptp_clock_config_default_ds_get(inst, &default_ds_cfg) == VTSS_RC_OK)) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "^%s",
                    ptp_profiles[default_ds_cfg.profile]);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        cyg_httpd_end_chunked();

    }
    return -1; // Do not further search the file system.
}


static i32 handler_stat_ptp(CYG_HTTPD_STATE* p)
{
    i32         ct;
    uint inst;
    vtss_appl_ptp_clock_status_default_ds_t default_ds_status;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_appl_ptp_ext_clock_mode_t mode;
    vtss_appl_ptp_config_port_ds_t port_cfg;
    vtss_ifindex_t ifindex;
    switch_iter_t sit;
    bool dataSet = false;
    port_iter_t       pit;

    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG);

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP))
        return -1;
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {
       T_D("Inside CYG_HTTPD_METHOD_POST ");
       redirect(p, "/ptp.htm");
    } else {
        if(!cyg_httpd_start_chunked("html"))
            return -1;
        /* show the local clock  */
        (void) vtss_appl_ptp_ext_clock_out_get(&mode);

        (void)cyg_httpd_write_chunked("^", 1);
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|%s|%d|%s|%d",
                                      ptp_one_pps_mode_disp(mode.one_pps_mode),
                                      ptp_bool_disp(mode.clock_out_enable),
                                      mode.freq,
                                      ptp_adj_method_disp(mode.adj_method),
                                      mode.clk_domain);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);

        (void)cyg_httpd_write_chunked("^", 1);
        for (inst = 0; inst < PTP_CLOCK_INSTANCES; inst++)
        {
            T_D("Inst - %d",inst);
            if ((vtss_appl_ptp_clock_config_default_ds_get(inst, &default_ds_cfg) == VTSS_RC_OK) &&
                (vtss_appl_ptp_clock_status_default_ds_get(inst, &default_ds_status) == VTSS_RC_OK))
            {
                dataSet = true;
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "|%u", inst);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", default_ds_cfg.clock_domain);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s", DeviceTypeToString(default_ds_cfg.deviceType));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                port_iter_init_local(&pit);
                while (port_iter_getnext(&pit)) {
                    (void) vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex);
                    if (vtss_appl_ptp_config_clocks_port_ds_get(inst, ifindex, &port_cfg) == VTSS_RC_OK) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", port_cfg.enabled);
                    } else {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%u", 0);
                    }
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            }
        }
        if (!dataSet)
            (void)cyg_httpd_write_chunked("No entries",10);
        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

static i32 handler_clock_stat_ptp(CYG_HTTPD_STATE* p)
{
    i32         ct;
    uint  request_inst = 0;
    mesa_timestamp_t t;
    char str[14];
    int    var_value;
    vtss_appl_ptp_clock_status_default_ds_t default_ds_status;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_appl_ptp_clock_current_ds_t clock_current_bs;
    vtss_appl_ptp_clock_slave_ds_t clock_slave_bs;

    vtss_appl_ptp_clock_parent_ds_t clock_parent_bs;
    vtss_appl_ptp_clock_timeproperties_ds_t clock_time_properties_bs;
#ifdef  PTP_OFFSET_FILTER_CUSTOM
    ptp_clock_servo_con_ds_t servo;
#else
    vtss_appl_ptp_clock_servo_config_t default_servo;
#endif /* PTP_OFFSET_FILTER_CUSTOM */
    vtss_appl_ptp_clock_filter_config_t filter_params;
    vtss_appl_ptp_unicast_slave_config_t uni_slave_cfg;
    vtss_appl_ptp_unicast_slave_table_t uni_slave_status;
    bool dataSet = false;
    char str1[40];
    char str2[40];
    char str3[60];
    u64 hw_time;
    i32 ix;

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP))
        return -1;
#endif
    if (cyg_httpd_form_varable_int(p, "clock_inst", &var_value)) {
        request_inst = var_value;
    }

    if (p->method == CYG_HTTPD_METHOD_POST) {
        T_D("Inside CYG_HTTPD_METHOD_POST ");
        redirect(p, "/ptp_clock.htm");
    } else {
        if(!cyg_httpd_start_chunked("html"))
            return -1;
        if ((vtss_appl_ptp_clock_config_default_ds_get(request_inst, &default_ds_cfg) == VTSS_RC_OK) &&
            (vtss_appl_ptp_clock_status_default_ds_get(request_inst, &default_ds_status) == VTSS_RC_OK))
        {
            dataSet = true;
            /* send the local clock  */
            vtss_local_clock_time_get(&t, request_inst, &hw_time);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%s %s", misc_time2str(t.seconds), vtss_tod_ns2str(t.nanoseconds, str,','));
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "/%s", clock_adjustment_method_txt(vtss_ptp_adjustment_method(request_inst)));
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            /* Send the Default clock Data Set */
            if (default_ds_cfg.deviceType == VTSS_APPL_PTP_DEVICE_NONE) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        " #%d /%s", request_inst,
                        DeviceTypeToString(default_ds_cfg.deviceType));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            } else {
                if (vtss_appl_ptp_clock_filter_parameters_get(request_inst, &filter_params) != VTSS_RC_OK) {
                    T_W("Could not read filter parameters");
                }
                vtss_ptp_servo_mode_ref_t mode_ref;
                if (vtss_ptp_get_servo_mode_ref(request_inst, &mode_ref) != VTSS_RC_OK) {
                    T_W("Could not read servo mode");
                }
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "#%d/%d/%s/%s/%s/%s",
                        request_inst,
                        default_ds_cfg.clock_domain,
                        DeviceTypeToString(default_ds_cfg.deviceType),
                        ptp_profiles[default_ds_cfg.profile],
                        ptp_filter_type_disp(default_ds_cfg.filter_type),
                        sync_servo_mode_2_txt(mode_ref.mode));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "#%s/%s/%s/%d/%s/%d",
                        DeviceTypeToString(default_ds_cfg.deviceType),
                        (default_ds_cfg.oneWay) ? "True" : "False",
                        (default_ds_cfg.twoStepFlag) ? "True" : "False",
                         default_ds_status.numberPorts,
                         ClockIdentityToString(default_ds_status.clockIdentity, str1),
                         default_ds_cfg.domainNumber);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "/%s/%d/%d/%d",
                        ClockQualityToString(&default_ds_status.clockQuality, str2),
                        default_ds_cfg.priority1,
                        default_ds_cfg.priority2,
                        default_ds_cfg.localPriority);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "/%s", ptp_protocol_disp(default_ds_cfg.protocol));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/%d/%d",
                         (default_ds_cfg.configured_vid),
                         (default_ds_cfg.configured_pcp));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/%d",
                              (default_ds_cfg.dscp));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
#if defined (VTSS_SW_OPTION_P802_1_AS)
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/%s",
                              (default_ds_status.s_802_1as.gmCapable) ? "True" : "False");
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/0x%03x",
                              default_ds_status.s_802_1as.sdoId);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                if (default_ds_cfg.profile == VTSS_APPL_PTP_PROFILE_AED_802_1AS) {
                    ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "/%s",
                    (default_ds_cfg.deviceType == VTSS_APPL_PTP_DEVICE_AED_GM) ? "True" : "False");
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
            }
            /* Send the current Clock Data Set */
            if (vtss_appl_ptp_clock_status_current_ds_get(request_inst, &clock_current_bs) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
#if defined (VTSS_SW_OPTION_P802_1_AS)
                        "#%d/%s/%s/%s/%lf/%d/%d/%d/%d/%d",
#else
                        "#%d/%s/%s/%s/%s/%s/%s/%s/%s/%s",
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                        clock_current_bs.stepsRemoved, vtss_tod_TimeInterval_To_String(&clock_current_bs.offsetFromMaster, str1,','),
                        vtss_tod_TimeInterval_To_String(&clock_current_bs.meanPathDelay, str2,',')
#if defined (VTSS_SW_OPTION_P802_1_AS)
                        , vtss_tod_ScaledNs_To_String(&clock_current_bs.cur_802_1as.lastGMPhaseChange, str3,','),
                        clock_current_bs.cur_802_1as.lastGMFreqChange,
                        clock_current_bs.cur_802_1as.gmTimeBaseIndicator,
                        clock_current_bs.cur_802_1as.gmChangeCount,
                        clock_current_bs.cur_802_1as.timeOfLastGMChangeEvent,
                        clock_current_bs.cur_802_1as.timeOfLastGMPhaseChangeEvent,
                        clock_current_bs.cur_802_1as.timeOfLastGMFreqChangeEvent
#else
                        , "", "", "", "", "", "", ""
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                        );
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            /* Send the Clock Slave Data Set */
            if (vtss_appl_ptp_clock_status_slave_ds_get(request_inst, &clock_slave_bs) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                              "/%d/%s/%s",
                              clock_slave_bs.port_number,
                              vtss_ptp_slave_state_2_text(clock_slave_bs.slave_state),
                              vtss_ptp_ho_state_2_text(clock_slave_bs.holdover_stable, clock_slave_bs.holdover_adj, str1, false));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            if (vtss_appl_ptp_clock_status_parent_ds_get(request_inst, &clock_parent_bs) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "#%s/%d/%s/%d/%d",
                        ClockIdentityToString(clock_parent_bs.parentPortIdentity.clockIdentity, str1), clock_parent_bs.parentPortIdentity.portNumber,
                        ptp_bool_disp(clock_parent_bs.parentStats),
                        clock_parent_bs.observedParentOffsetScaledLogVariance,
                        clock_parent_bs.observedParentClockPhaseChangeRate);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
#if defined (VTSS_SW_OPTION_P802_1_AS)
                        "/%s/%s/%d/%d/%d",
#else
                        "/%s/%s/%d/%d",
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                        ClockIdentityToString(clock_parent_bs.grandmasterIdentity, str2),
                        ClockQualityToString(&clock_parent_bs.grandmasterClockQuality, str3),
                        clock_parent_bs.grandmasterPriority1,
                        clock_parent_bs.grandmasterPriority2
#if defined (VTSS_SW_OPTION_P802_1_AS)
                        , clock_parent_bs.par_802_1as.cumulativeRateRatio
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                        );
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            if (vtss_appl_ptp_clock_status_timeproperties_ds_get(request_inst, &clock_time_properties_bs) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "#%d/%s/%s/%s/%s/%s/%s/%d",
                        clock_time_properties_bs.currentUtcOffset,
                        ptp_bool_disp(clock_time_properties_bs.currentUtcOffsetValid),
                        ptp_bool_disp(clock_time_properties_bs.leap59),
                        ptp_bool_disp(clock_time_properties_bs.leap61),
                        ptp_bool_disp(clock_time_properties_bs.timeTraceable),
                        ptp_bool_disp(clock_time_properties_bs.frequencyTraceable),
                        ptp_bool_disp(clock_time_properties_bs.ptpTimescale),
                        clock_time_properties_bs.timeSource);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            /* send the filter parameters  */
            if (vtss_appl_ptp_clock_filter_parameters_get(request_inst, &filter_params) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "#%d/%d/%d",
                        filter_params.delay_filter,
                        filter_params.period,
                        filter_params.dist);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            /* send the servo parameters  */
            if (vtss_appl_ptp_clock_servo_parameters_get(request_inst, &default_servo) == VTSS_RC_OK) {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "#%s/%s/%s/%s/%d/%d/%d/%d",
                        ptp_bool_disp(default_servo.display_stats),
                        ptp_bool_disp(default_servo.p_reg),
                        ptp_bool_disp(default_servo.i_reg),
                        ptp_bool_disp(default_servo.d_reg),
                        default_servo.ap,
                        default_servo.ai,
                        default_servo.ad,
                        default_servo.gain);
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
            /* send the unicast slave parameters */
            (void)cyg_httpd_write_chunked("#", 1);
            ix = 0;
            while ((vtss_appl_ptp_clock_config_unicast_slave_config_get(request_inst, ix, &uni_slave_cfg) == VTSS_RC_OK) &&
                   (vtss_appl_ptp_clock_status_unicast_slave_table_get(request_inst, ix++, &uni_slave_status) == VTSS_RC_OK))
             {
                ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
                        "%u/%d/%s/%hhd/%s|",
                        (ix-1),
                        uni_slave_cfg.duration,
                        misc_ipv4_txt(uni_slave_cfg.ip_addr, str1),
                        uni_slave_status.log_msg_period,
                        ptp_state_disp(uni_slave_status.comm_state));
                (void)cyg_httpd_write_chunked(p->outbuffer, ct);
            }
        }
        if (!dataSet)
            (void)cyg_httpd_write_chunked("",1);
        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

#if defined (VTSS_SW_OPTION_P802_1_AS)
static i32 handler_ptp_as_statistics(CYG_HTTPD_STATE* p)
{
    i32                                              ct;
    uint                                             request_inst = 0;
    vtss_appl_ptp_status_port_statistics_t           port_stati;
    vtss_ifindex_t                                   ifindex;
    port_iter_t                                      pit;
    switch_iter_t                                    sit;
    int                                              var_value;
    bool                                             clear = (cyg_httpd_form_varable_find(p, "clear") != NULL);
    mesa_rc                                          rc;
    vtss_appl_ptp_802_1as_cmlds_status_port_ds_t     cmlds_port_status;
    vtss_appl_ptp_802_1as_cmlds_port_statistics_ds_t cmlds_statis;
    bool                                             cmlds_port_enabled = false;

    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG);

    if (cyg_httpd_form_varable_int(p, "id", &var_value)) {
        request_inst = var_value;
    }
    if (p->method == CYG_HTTPD_METHOD_POST) {
        T_D("Inside CYG_HTTPD_METHOD_POST ");
        redirect(p, "/ptp_as_statistics.htm");
    } else {
        if(!cyg_httpd_start_chunked("html"))
            return -1;

        (void)cyg_httpd_write_chunked("$", 1);
        /* CMLDS statistics are identified with id '4' in the html files.*/
        if (request_inst != 4) {
            port_iter_init_local(&pit);
            while (port_iter_getnext(&pit)) {
                (void) vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex);
                if (clear) {     /* Clear? */
                    rc = vtss_appl_ptp_status_clocks_port_statistics_get_clear(request_inst, ifindex, &port_stati);
                } else {
                    rc = vtss_appl_ptp_status_clocks_port_statistics_get(request_inst, ifindex, &port_stati);
                }
                if (rc == VTSS_RC_OK) {
                    ct = snprintf(p->outbuffer,sizeof(p->outbuffer), "%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d/%d#",
                            pit.iport+1,
                            port_stati.rxSyncCount,
                            port_stati.txSyncCount,
                            port_stati.rxFollowUpCount,
                            port_stati.txFollowUpCount,
                            port_stati.peer_d.rxPdelayRequestCount,
                            port_stati.peer_d.txPdelayRequestCount,
                            port_stati.peer_d.rxPdelayResponseCount,
                            port_stati.peer_d.txPdelayResponseCount,
                            port_stati.peer_d.rxPdelayResponseFollowUpCount,
                            port_stati.peer_d.txPdelayResponseFollowUpCount,
                            port_stati.rxDelayRequestCount,
                            port_stati.txDelayRequestCount,
                            port_stati.rxDelayResponseCount,
                            port_stati.txDelayResponseCount,
                            port_stati.rxAnnounceCount,
                            port_stati.txAnnounceCount,
                            port_stati.rxPTPPacketDiscardCount,
                            port_stati.syncReceiptTimeoutCount,
                            port_stati.announceReceiptTimeoutCount,
                            port_stati.peer_d.pdelayAllowedLostResponsesExceededCount
                            );
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            }
        } else {
            /* Display CMLDS statistics if it is enabled for atleast one port. */
            port_iter_init_local(&pit);
            while (port_iter_getnext(&pit)) {
                rc = vtss_appl_ptp_cmlds_port_status_get(pit.uport, &cmlds_port_status);
                if ((rc == VTSS_RC_OK) && cmlds_port_status.cmldsLinkPortEnabled) {
                    cmlds_port_enabled = true;
                    break;
                }
            }
            if (cmlds_port_enabled != false) {
                port_iter_init_local(&pit);
                while (port_iter_getnext(&pit)) {
                    if (clear) {     /* Clear? */
                        rc = vtss_appl_ptp_cmlds_port_statistics_get_clear(pit.uport, &cmlds_statis);
                    } else {
                        rc = vtss_appl_ptp_cmlds_port_statistics_get(pit.uport, &cmlds_statis);
                    }
                    if (rc == VTSS_RC_OK) {
                        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d/%d/%d/%d/%d/%d/%d/%d/%d#",
                                      pit.iport,
                                      cmlds_statis.peer_d.rxPdelayRequestCount,
                                      cmlds_statis.peer_d.txPdelayRequestCount,
                                      cmlds_statis.peer_d.rxPdelayResponseCount,
                                      cmlds_statis.peer_d.txPdelayResponseCount,
                                      cmlds_statis.peer_d.rxPdelayResponseFollowUpCount,
                                      cmlds_statis.peer_d.txPdelayResponseFollowUpCount,
                                      cmlds_statis.rxPTPPacketDiscardCount,
                                      cmlds_statis.peer_d.pdelayAllowedLostResponsesExceededCount);
                        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                    }
                }
            }
        }
        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}
#endif

static i32 handler_clock_ports_stat_ptp(CYG_HTTPD_STATE* p)
{
    i32         ct;
    int   var_value;
    uint  request_inst = 0;
    vtss_appl_ptp_clock_status_default_ds_t default_ds_status;
    vtss_appl_ptp_clock_config_default_ds_t default_ds_cfg;
    vtss_appl_ptp_config_port_ds_t port_cfg;
    vtss_appl_ptp_status_port_ds_t port_status;
    vtss_appl_ptp_802_1as_cmlds_status_port_ds_t cmlds_port_status;
    vtss_ifindex_t ifindex;
    switch_iter_t sit;
    bool dataSet = false;
    char pmpd[50];
#if defined (VTSS_SW_OPTION_P802_1_AS)
    char srti[50];
    char str1[40];
    char str5[40];
    char str6[40];
#endif
    char str2[40];
    char str3[40];
    char str4[40];
    port_iter_t       pit;

	vtss_appl_ptp_status_port_ds_t virtual_port_status;
	vtss_appl_ptp_virtual_port_config_t  virtual_port_cfg;
	u32 pin;
	u32 virtual_port = fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT);

    (void) switch_iter_init(&sit, VTSS_ISID_GLOBAL, SWITCH_ITER_SORT_ORDER_USID_CFG);

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP))
        return -1;
#endif
    if (cyg_httpd_form_varable_int(p, "clock_inst", &var_value)) {
        request_inst = var_value;
    }
    if (p->method == CYG_HTTPD_METHOD_POST) {
        T_D("Inside CYG_HTTPD_METHOD_POST ");
        redirect(p, "/ptp_clock_ports.htm");
    } else {
        if(!cyg_httpd_start_chunked("html"))
            return -1;

        if ((vtss_appl_ptp_clock_config_default_ds_get(request_inst, &default_ds_cfg) == VTSS_RC_OK) &&
            (vtss_appl_ptp_clock_status_default_ds_get(request_inst, &default_ds_status) == VTSS_RC_OK))
        {
            dataSet = true;
            (void) cyg_httpd_write_chunked("#", 1);
            port_iter_init_local(&pit);
            while (port_iter_getnext(&pit)) {
                (void) vtss_ifindex_from_port(sit.isid, pit.iport, &ifindex);
                if ((vtss_appl_ptp_config_clocks_port_ds_get(request_inst, ifindex, &port_cfg) == VTSS_RC_OK) &&
                    (port_cfg.enabled == 1) &&
                    (vtss_appl_ptp_status_clocks_port_ds_get(request_inst, ifindex, &port_status) == VTSS_RC_OK) &&
                    (vtss_appl_ptp_cmlds_port_status_get(pit.uport, &cmlds_port_status) == VTSS_RC_OK))
                {
                    ct = snprintf(p->outbuffer,sizeof(p->outbuffer),
#if defined (VTSS_SW_OPTION_P802_1_AS)
                            "$%d/%s/%d/%s/%d/%d/%d/%s/%d/%s/%s/%s/%d/%s/%s/%d^%s/%s/%s/%d/%d/%d/%s/%d/%s/%s/%s/%d/%s/%d/%s/%d/%d/%d/%d/%d/%d/%d^%s/%s/%s/%s/%s/%d/%d/%s/%s/%d/%d",
#else
                            "$%d/%s/%d/%s/%d/%d/%d/%s/%d/%s/%s/%s/%d/%s/%s/%d^",
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                            port_status.portIdentity.portNumber,
                            PortStateToString(port_status.portState),
                            port_status.logMinDelayReqInterval,
                            vtss_tod_TimeInterval_To_String(&port_status.peerMeanPathDelay, pmpd, ','),
                            port_cfg.logAnnounceInterval,
                            port_cfg.announceReceiptTimeout,port_cfg.logSyncInterval,ptp_delaymechanism_disp(port_cfg.delayMechanism),
                            port_cfg.logMinPdelayReqInterval,
                            vtss_tod_TimeInterval_To_String(&port_cfg.delayAsymmetry, str2,','),
                            vtss_tod_TimeInterval_To_String(&port_cfg.ingressLatency, str3,','),
                            vtss_tod_TimeInterval_To_String(&port_cfg.egressLatency, str4,','),
                            port_cfg.versionNumber,
                            (port_cfg.dest_adr_type == VTSS_APPL_PTP_PROTOCOL_SELECT_LINK_LOCAL) ? "Link-local" : "Default",
                            port_cfg.masterOnly ? "True" : "False",
                            port_cfg.localPriority
#if defined (VTSS_SW_OPTION_P802_1_AS)
                            ,web_port_role_disp(port_status.s_802_1as.portRole),
                            port_status.s_802_1as.peer_d.isMeasuringDelay ? "True" : "False",
                            port_status.s_802_1as.asCapable ? "True" : "False",
                            port_status.s_802_1as.peer_d.neighborRateRatio,
                            port_status.s_802_1as.currentLogAnnounceInterval,
                            port_status.s_802_1as.currentLogSyncInterval,
                            vtss_tod_TimeInterval_To_String(&port_status.s_802_1as.syncReceiptTimeInterval, srti, ','),
                            port_status.s_802_1as.peer_d.currentLogPDelayReqInterval,
                            port_status.s_802_1as.acceptableMasterTableEnabled ? "True" : "False",
                            port_status.s_802_1as.peer_d.currentComputeNeighborRateRatio ? "True" : "False",
                            port_status.s_802_1as.peer_d.currentComputeMeanLinkDelay ? "True" : "False",
                            port_status.s_802_1as.peer_d.versionNumber,
                            port_cfg.c_802_1as.as2020 ? "True" : "False",
                            port_status.s_802_1as.currentLogGptpCapableMessageInterval,
                            vtss_tod_TimeInterval_To_String(&port_cfg.c_802_1as.peer_d.meanLinkDelayThresh, str1, ','),
                            port_cfg.c_802_1as.syncReceiptTimeout,
                            port_cfg.c_802_1as.peer_d.allowedLostResponses,
                            port_cfg.c_802_1as.peer_d.allowedFaults,
                            port_cfg.c_802_1as.peer_d.initialLogPdelayReqInterval,
                            port_cfg.c_802_1as.peer_d.operLogPdelayReqInterval,
                            port_cfg.c_802_1as.initialLogSyncInterval,
                            port_cfg.c_802_1as.operLogSyncInterval,
                            ClockIdentityToString(cmlds_port_status.portIdentity.clockIdentity, str5),
                            cmlds_port_status.cmldsLinkPortEnabled ? "True" : "False",
                            cmlds_port_status.peer_d.isMeasuringDelay ? "True" : "False",
                            cmlds_port_status.asCapableAcrossDomains ? "True" : "False",
                            vtss_tod_TimeInterval_To_String(&cmlds_port_status.meanLinkDelay, str6,','),
                            cmlds_port_status.peer_d.neighborRateRatio,
                            cmlds_port_status.peer_d.currentLogPDelayReqInterval,
                            cmlds_port_status.peer_d.currentComputeNeighborRateRatio ? "True" : "False",
                            cmlds_port_status.peer_d.currentComputeMeanLinkDelay ? "True" : "False",
                            cmlds_port_status.peer_d.versionNumber,
                            cmlds_port_status.peer_d.minorVersionNumber
#endif //defined (VTSS_SW_OPTION_P802_1_AS)
                            );
                    (void)cyg_httpd_write_chunked(p->outbuffer, ct);
                }
            } //while

        }

	/* virtual port configuration */
	if ((ptp_clock_config_virtual_port_config_get(request_inst, &virtual_port_cfg) == VTSS_RC_OK) &&
            (vtss_appl_ptp_status_clocks_virtual_port_ds_get(request_inst, virtual_port, &virtual_port_status) == VTSS_RC_OK)) {
				if(virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO ||
				   virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN ||
				   virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT ||
				   virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_FREQ_OUT){
						pin = virtual_port_cfg.output_pin;
				} else if (virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB ||
						virtual_port_cfg.virtual_port_mode == VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN){
						pin = virtual_port_cfg.input_pin;
				} else {
						pin = 99999;
				}

				ct = snprintf (p->outbuffer, sizeof(p->outbuffer) ,
								  "#V_PORT/%d/%s/%s/%d",
								  iport2uport(virtual_port),
								  virtual_port_cfg.enable ? "TRUE" : "FALSE",
								  PortStateToString(virtual_port_status.portState),
								  pin);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
	}

        if (!dataSet)
            (void)cyg_httpd_write_chunked("",1);
        if ((vtss_appl_ptp_clock_config_default_ds_get(request_inst, &default_ds_cfg) == VTSS_RC_OK)) {
            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "#%s",
                    ptp_profiles[default_ds_cfg.profile]);
            (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        }

        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

#if defined (VTSS_SW_OPTION_P802_1_AS)
static i32 handler_ptp_cmlds_ds(CYG_HTTPD_STATE* p)
{
    i32 ct;
    vtss_appl_ptp_802_1as_cmlds_default_ds_t def_ds;
    char str1[40];

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PTP))
        return -1;
#endif
    if (p->method == CYG_HTTPD_METHOD_POST) {
        T_D("Inside CYG_HTTPD_METHOD_POST ");
        redirect(p, "/ptp_cmlds_ds.htm");
    } else {
        if(!cyg_httpd_start_chunked("html"))
            return -1;

        if (vtss_appl_ptp_cmlds_default_ds_get(&def_ds) != VTSS_RC_OK) {
            return -1;
        }
        (void) cyg_httpd_write_chunked("#", 1);
        ct = snprintf(p->outbuffer,sizeof(p->outbuffer),"$%s/%d/%d",
                      ClockIdentityToString(def_ds.clockIdentity, str1),
                      def_ds.numberLinkPorts,
                      def_ds.sdoId);
        (void)cyg_httpd_write_chunked(p->outbuffer, ct);
        cyg_httpd_end_chunked();
    }
    return -1;
}
#endif //VTSS_SW_OPTION_P802_1_AS

static i32 handler_logs_ptp(CYG_HTTPD_STATE* p, int instance)
{
    if (p->method == CYG_HTTPD_METHOD_POST) {
        // We do not support HTTP POST request (only GET is supported)
    } else {
        if (!cyg_httpd_start_chunked("txt"))
            return -1;

        char log_file_name[20];
        snprintf(log_file_name, sizeof(log_file_name), "/tmp/ptp_log_%d.tpk", instance);

        FILE *log_file;
        if ((log_file = fopen(log_file_name, "r")) != NULL) {
            char c[1];
            while ((c[0] = (char)fgetc(log_file)) != EOF) {
                if (c[0] == '\n')
                    (void)cyg_httpd_write_chunked("\r", sizeof(char));
                (void)cyg_httpd_write_chunked(c, sizeof(c));
            }
            fclose(log_file);
        } else {
            char buf[] = "No PTP log file to get.\n";
            (void)cyg_httpd_write_chunked(buf, strlen(buf));
        }

        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}

static i32 handler_logs_ptp_0(CYG_HTTPD_STATE* p)
{
    return handler_logs_ptp(p, 0);
}

static i32 handler_logs_ptp_1(CYG_HTTPD_STATE* p)
{
    return handler_logs_ptp(p, 1);
}

static i32 handler_logs_ptp_2(CYG_HTTPD_STATE* p)
{
    return handler_logs_ptp(p, 2);
}

static i32 handler_logs_ptp_3(CYG_HTTPD_STATE* p)
{
    return handler_logs_ptp(p, 3);
}

/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t ptp_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    vtss_zarlink_servo_t servo_type = (vtss_zarlink_servo_t)fast_cap(VTSS_APPL_CAP_ZARLINK_SERVO_TYPE);
    char buff[PTP_WEB_BUF_LEN]="";
    int cnt = 0;
#if !defined (VTSS_SW_OPTION_P802_1_AS)
    cnt += snprintf(buff + cnt, PTP_WEB_BUF_LEN - cnt, ".has_ptp_1as { display: none; }\r\n");
#endif //defined (SW_OPTION_P802_1_AS)
#if !defined(SW_OPTION_BASIC_PTP_SERVO) || !defined(VTSS_SW_OPTION_P802_1_AS)
    cnt += snprintf(buff + cnt, PTP_WEB_BUF_LEN - cnt, ".has_ptp_basic { display: none;}\r\n");
#endif
    if (servo_type != VTSS_ZARLINK_SERVO_ZLS30380 && servo_type != VTSS_ZARLINK_SERVO_ZLS30387) {
        cnt += snprintf(buff + cnt , PTP_WEB_BUF_LEN - cnt, ".has_ptp_zls30380_or_zls30387 { display: none; }\r\n");
    }
    if (servo_type != VTSS_ZARLINK_SERVO_ZLS30380) {
        cnt += snprintf(buff + cnt , PTP_WEB_BUF_LEN - cnt, ".has_ptp_zls30380 { display: none; }\r\n");
    }
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(ptp_lib_filter_css);

/****************************************************************************/
/*  Module JS lib config routine                                            */
/****************************************************************************/

static size_t ptp_lib_config_js(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[PTP_WEB_BUF_LEN];
    int maxSyncInt = -7;
    int maxDelayInt = -7;
#if defined (__MIPSEL__)
    maxSyncInt = -3;
    maxDelayInt = 0;
#endif
    (void) snprintf(buff, PTP_WEB_BUF_LEN,
                    "var maxSyncInt = %d;\n"
                    "var maxDelayInt = %d;\n",
                    maxSyncInt, maxDelayInt);

    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}

/****************************************************************************/
/*  JS lib config table entry                                               */
/****************************************************************************/

web_lib_config_js_tab_entry(ptp_lib_config_js);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/

CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_ptp, "/config/ptp_config", handler_config_ptp);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_clock_config_ptp, "/config/ptp_clock_config", handler_clock_config_ptp);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_clock_ports_config_ptp, "/config/ptp_clock_ports_config", handler_clock_ports_config_ptp);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_ptp, "/stat/ptp", handler_stat_ptp);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_ptp_clock, "/stat/ptp_clock", handler_clock_stat_ptp);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_ptp_clock_ports, "/stat/ptp_clock_ports", handler_clock_ports_stat_ptp);
#if defined (VTSS_SW_OPTION_P802_1_AS)
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_ptp_as_statistics, "/stat/ptp_as_statistics", handler_ptp_as_statistics);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_stat_ptp_cmlds_ds, "/stat/ptp_cmlds_ds", handler_ptp_cmlds_ds);
#endif
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_logs_ptp_0, "/logs/ptp_log_0.tpk", handler_logs_ptp_0);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_logs_ptp_1, "/logs/ptp_log_1.tpk", handler_logs_ptp_1);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_logs_ptp_2, "/logs/ptp_log_2.tpk", handler_logs_ptp_2);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_logs_ptp_3, "/logs/ptp_log_3.tpk", handler_logs_ptp_3);

