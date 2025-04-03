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

#include "port_instance.hxx"
#include <vtss/appl/port.h>
#include "port_listener.hxx"
#include "port_trace.h"
#include "port_lock.hxx"                       // For PORT_LOCK_SCOPE()
#include "misc_api.h"                          // For iport2uport()
#include "conf_api.h"                          // For conf_mgmt_mac_addr_get()
#include <vtss/basics/expose/table-status.hxx> // For vtss::expose::TableStatus
#include <vtss/basics/memcmp-operator.hxx>     // For VTSS_BASICS_MEMCMP_OPERATOR

#if defined(VTSS_SW_OPTION_KR)
#include "kr_api.h"
#endif

#ifdef VTSS_SW_OPTION_SYSLOG
#include "syslog_api.h"
#endif

#if defined(VTSS_SW_OPTION_TOD)
#include "tod_api.h"
#endif

/* We have two different hardware layouts for dual media ports.
 * 1. Dual media with INTERNAL PHY:
 *    This is where the switch chip's internal PHY connects to both RJ45
 *    and SFP interfaces.
 *    Examples of this are:
 *      PCB116: Serval-T,  Gi 1/1,2
 *      PCB123: Ocelot-10, Gi 1/3,4
 *
 *    The media port on PCB116 uses S/W to switch between 2x1000Base-T and 2x1G
 *    Serdes:
 *
 *            VSC7437 (Switch chip with internal PHY)
 *                  |
 *    +---------------------------+
 *    |                           |
 *    2x1000Base-T (RJ45)    2x1G Serdes (SFP)
 *
 *   PORT_IS_DUAL_MEDIA_INTERNAL_PHY() cathes ports with this property.
 *
 * 2. Dual media with EXTERNAL PHY:
 *    This is where the switch chip connects to a PHY first and the PHY
 *    connects to both RJ45 and SFP interfaces.
 *    An example of this is:
 *       PCB090: Luton26/Caracal2, Gi 1/21-24
 *
 *    The media ports on PCB106 use SGMII to connect a VSC8574 (Quad PHY) first
 *    and then 4x1000Base-T and 4x1G Serdes:
 *
 *            VSC7418 (Switch chip)
 *                  |
 *            VSC8574(Quad PHY)
 *                  |
 *    +---------------------------+
 *    |                           |
 *    4x1000Base-T(RJ45)       4x1G Serdes(SFP)
 *
 *    PORT_IS_DUAL_MEDIA_EXTERNAL_PHY() cathes ports with this property.
 *
 * The Port software modules needs to switch the media port MAC interface
 * correctly according to the current port configuration and real connected
 * situation.
 *
 * Notes:
 * 1. If the dual media port is connected with an internal PHY, the port MAC
 *    interface should be based on the software configuration. For example,
 *    1.a Use "MESA_PORT_INTERFACE_SGMII" for copper mode.
 *    1.b Use "MESA_PORT_INTERFACE_SERDES" for 1G fiber mode.
 * 2. If the dual media port is connected with an external PHY, the port MAC
 *    interface always equals "MESA_PORT_INTERFACE_QSGMII" and use
 *    mepa_media_interface_t (see phy_reset->media_if in phy_fiber_media()) to
 *    set the speed on API layer.
 */

extern vtss_appl_port_capabilities_t PORT_cap;

// JSON notifications
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_port_status_t);
vtss::expose::TableStatus <vtss::expose::ParamKey<vtss_ifindex_t>, vtss::expose::ParamVal<vtss_appl_port_status_t *>> port_status_update("port_status_update", VTSS_MODULE_ID_PORT);

/******************************************************************************/
// port_instance_vol_user_txt()
/******************************************************************************/
const char *port_instance_vol_user_txt(port_user_t user)
{
    switch (user) {
    case PORT_USER_STATIC:
        return "Static";

    case PORT_USER_ACL:
        return "ACL";

    case PORT_USER_THERMAL_PROTECT:
        return "Thermal";

    case PORT_USER_LOOP_PROTECT:
        return "Loop Protect";

    case PORT_USER_UDLD:
        return "UDLD";

    case PORT_USER_PSEC:
        return "Port Security";

    case PORT_USER_ERRDISABLE:
        return "ErrDisable";

    case PORT_USER_AGGR:
        return "AGGR";

    case PORT_USER_CNT:
        return "ALL";

    default:
        return "Unknown";
    }
}

/******************************************************************************/
// pfc2txt()
/******************************************************************************/
static char *pfc2txt(const mesa_bool_t *pfc, char *buf)
{
    size_t      cnt = ARRSZ(mesa_port_flow_control_conf_t::pfc);
    mesa_prio_t prio;

    buf[0] = '"';

    for (prio = 0; prio < cnt; prio++) {
        buf[prio + 1] = pfc[prio] ? '1' : '0';
    }

    buf[cnt + 1] = '"';
    buf[cnt + 2] = '\0';

    return buf;
}

/******************************************************************************/
// serdes_media_type2txt()
/******************************************************************************/
static const char *serdes_media_type2txt(mesa_sd10g_media_type_t media_type)
{
    switch (media_type) {
    case MESA_SD10G_MEDIA_PR_NONE:
        return "No preset";

    case MESA_SD10G_MEDIA_SR:
        return "Short Range";

    case MESA_SD10G_MEDIA_ZR:
        return "Long Range";

    case MESA_SD10G_MEDIA_DAC:
        return "DAC (Direct attached copper) cable, unspecified length";

    case MESA_SD10G_MEDIA_DAC_1M:
        return "1m DAC";

    case MESA_SD10G_MEDIA_DAC_2M:
        return "2m DAC";

    case MESA_SD10G_MEDIA_DAC_3M:
        return "3m DAC";

    case MESA_SD10G_MEDIA_DAC_5M:
        return "5m DAC";

    case MESA_SD10G_MEDIA_BP:
        return "Backplane";

    case MESA_SD10G_MEDIA_B2B:
        return "Board to Board";

    case MESA_SD10G_MEDIA_10G_KR:
        return "10G Base KR";

    default:
        return "UNKNOWN!";
    }
}

/******************************************************************************/
// phy_media_if2txt()
/******************************************************************************/
static const char *phy_media_if2txt(mepa_media_interface_t phy_media_if)
{
    switch (phy_media_if) {
    case MESA_PHY_MEDIA_IF_CU:
        return "CU";

    case MESA_PHY_MEDIA_IF_SFP_PASSTHRU:
        return "SFP_PASSTHRU";

    case MESA_PHY_MEDIA_IF_FI_1000BX:
        return "FI_1000BX";

    case MESA_PHY_MEDIA_IF_FI_100FX:
        return "FI_100FX";

    case MESA_PHY_MEDIA_IF_AMS_CU_PASSTHRU:
        return "AMS_CU_PASSTHRU";

    case MESA_PHY_MEDIA_IF_AMS_FI_PASSTHRU:
        return "AMS_FI_PASSTHRU";

    case MESA_PHY_MEDIA_IF_AMS_CU_1000BX:
        return "AMS_CU_1000BX";

    case MESA_PHY_MEDIA_IF_AMS_FI_1000BX:
        return "AMS_FI_1000BX";

    case MESA_PHY_MEDIA_IF_AMS_CU_100FX:
        return "AMS_CU_100FX";

    case MESA_PHY_MEDIA_IF_AMS_FI_100FX:
        return "AMS_FI_100FX";

    default:
        return "UNKNOWN!";
    }
}

/******************************************************************************/
// phy_reset_point_to_str()
/******************************************************************************/
static const char *phy_reset_point_to_str(mepa_reset_point_t reset_point)
{
    switch (reset_point) {
    case MEPA_RESET_POINT_DEFAULT:
        return "Default";

    case MEPA_RESET_POINT_PRE:
        return "Pre";

    case MEPA_RESET_POINT_POST:
        return "Post";

    case MEPA_RESET_POINT_POST_MAC:
        return "Post MAC";

    default:
        return "UNKNOWN!";
    }
}

// Make sure we can make bitwise operations on this enumerated type to avoid
// compiler warnings.
VTSS_ENUM_BITWISE(mepa_phy_cap_t);

/******************************************************************************/
// phy_cap_to_str()
// Buf must be ~100 bytes long if all bits are set.
/******************************************************************************/
static char *phy_cap_to_str(char *buf, size_t size, mepa_phy_cap_t cap)
{
    int  s = 0;
    bool first = true;

#define P(...)                                              \
    if (size - s > 0) {                                     \
        int res = snprintf(buf + s, size - s, __VA_ARGS__); \
        if (res > 0) {                                      \
            s += res;                                       \
        }                                                   \
    }

#define F(X)                  \
    if (cap & MEPA_CAP_##X) { \
        cap &= ~MEPA_CAP_##X; \
        if (first) {          \
            first = false;    \
            P(#X);            \
        } else {              \
            P(" " #X);        \
        }                     \
    }

    *buf = 0;
    F(SPEED_MASK_1G);
    F(SPEED_MASK_2G5);
    F(SPEED_MASK_10G);
    F(TS_MASK_GEN_1);
    F(TS_MASK_GEN_2);
    F(TS_MASK_GEN_3);
    F(TS_MASK_NONE);

#undef F
#undef P

    if (cap != 0) {
        T_E("Not all PHY capabilities are handled. Missing = 0x%x", cap);
    }

    buf[MIN(size - 1, s)] = 0;
    return buf;
}

/******************************************************************************/
// mac_if2txt()
/******************************************************************************/
static const char *mac_if2txt(mesa_port_interface_t if_type)
{
    switch (if_type) {
    case MESA_PORT_INTERFACE_NO_CONNECTION:
        return "NO_CONNECTION";

    case MESA_PORT_INTERFACE_LOOPBACK:
        return "LOOPBACK";

    case MESA_PORT_INTERFACE_INTERNAL:
        return "INTERNAL";

    case MESA_PORT_INTERFACE_MII:
        return "MII";

    case MESA_PORT_INTERFACE_GMII:
        return "GMII";

    case MESA_PORT_INTERFACE_RGMII:
        return "RGMII";

    case MESA_PORT_INTERFACE_RGMII_ID:
        return "RGMII_ID";

    case MESA_PORT_INTERFACE_RGMII_TXID:
        return "RGMII_TXID";

    case MESA_PORT_INTERFACE_RGMII_RXID:
        return "RGMII_RXID";

    case MESA_PORT_INTERFACE_TBI:
        return "TBI";

    case MESA_PORT_INTERFACE_RTBI:
        return "RTBI";

    case MESA_PORT_INTERFACE_SGMII:
        return "SGMII";

    case MESA_PORT_INTERFACE_SGMII_2G5:
        return "SGMII_2G5";

    case MESA_PORT_INTERFACE_SGMII_CISCO:
        return "SGMII_CISCO";

    case MESA_PORT_INTERFACE_SERDES:
        return "SERDES";

    case MESA_PORT_INTERFACE_VAUI:
        return "VAUI";

    case MESA_PORT_INTERFACE_100FX:
        return "100FX";

    case MESA_PORT_INTERFACE_XAUI:
        return "XAUI";

    case MESA_PORT_INTERFACE_RXAUI:
        return "RXAUI";

    case MESA_PORT_INTERFACE_XGMII:
        return "XGMII";

    case MESA_PORT_INTERFACE_SPI4:
        return "SPI4";

    case MESA_PORT_INTERFACE_QSGMII:
        return "QSGMII";

    case MESA_PORT_INTERFACE_SFI:
        return "SFI";

    default:
        return "???";
    }
}

/******************************************************************************/
// power_mode2txt()
/******************************************************************************/
static const char *power_mode2txt(vtss_phy_power_mode_t power_mode)
{
    switch (power_mode) {
    case VTSS_PHY_POWER_NOMINAL:
        return "Nominal";

    case VTSS_PHY_POWER_ACTIPHY:
        return "ActiPHY";

    case VTSS_PHY_POWER_DYNAMIC:
        return "PerfectReach";

    case VTSS_PHY_POWER_ENABLED:
        return "ActiPHY + PerfectReach";

    default:
        return "UNKNOWN!";
    }
}

// #define PORT_INSTANCE_TRACE_RARELY_USED_FIELDS

#ifdef PORT_INSTANCE_TRACE_RARELY_USED_FIELDS
/******************************************************************************/
// mesa_port_frame_gaps_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_frame_gaps_t &conf)
{
    o << "{hdx_gap_1 = "  << conf.hdx_gap_1
      << ", hdx_gap_2 = " << conf.hdx_gap_2
      << ", fdx_gap = "   << conf.fdx_gap
      << "}";

    return o;
}
#endif

/******************************************************************************/
// mesa_port_flow_control_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_flow_control_conf_t &conf)
{
    char buf[20];

    o << "{obey = "  << conf.obey
      << ", gen = "  << conf.generate
      << ", pfc = "  << pfc2txt(conf.pfc, buf)
      << ", smac = " << conf.smac
      << "}";

    return o;
}

/******************************************************************************/
// mesa_port_serdes_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_serdes_conf_t &conf)
{
    o << "{media_type = " << serdes_media_type2txt(conf.media_type)
#ifdef PORT_INSTANCE_TRACE_RARELY_USED_FIELDS
      << ", sfp_dac = "   << conf.sfp_dac
      << ", rx_invert = " << conf.rx_invert
      << ", tx_invert = " << conf.tx_invert
#endif
      << "}";

    return o;
}

/******************************************************************************/
// meba_sfp_device_info_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const meba_sfp_device_info_t &info)
{
    if (info.vendor_pn[0] == '\0') {
        o << "{}";
        return o;
    }

    o << "{name = "  << info.vendor_name
      << ", pn = "   << info.vendor_pn
      << ", rev = "  << info.vendor_rev
      << ", sn = "   << info.vendor_sn
      << ", date = " << info.date_code
      << ", tr = "   << port_sfp_transceiver_to_txt(info.transceiver)
      << ", conn = " << port_sfp_connector_to_txt(info.connector)
      << "}";

    return o;
}

/******************************************************************************/
// meba_sfp_device_info_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const meba_sfp_device_info_t *info)
{
    o << *info;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_port_kr_aneg_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_kr_aneg_t &conf)
{
    o << "{enable = "      << conf.enable
      << ", adv_25g = "    << conf.adv_25g
      << ", adv_10g = "    << conf.adv_10g
      << ", adv_5g = "     << conf.adv_5g
      << ", adv_2g5 = "    << conf.adv_2g5
      << ", adv_1g = "     << conf.adv_1g
      << ", r_fec_req = "  << conf.r_fec_req
      << ", rs_fec_req = " << conf.rs_fec_req
      << ", next_page = "  << conf.next_page
      << ", no_pd = "      << conf.no_pd
      << "}";

    return o;
}

/******************************************************************************/
// mesa_port_kr_train_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_kr_train_t &conf)
{
    o << "{enable = "       << conf.enable
      << ", no_remote = "   << conf.no_remote
      << ", no_eq_apply = " << conf.no_eq_apply
      << ", use_ber_cnt = " << conf.use_ber_cnt
      << ", test_mode = "   << conf.test_mode
      << ", test_repeat = " << conf.test_repeat
      << "}";

    return o;
}

/******************************************************************************/
// mesa_port_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_conf_t &conf)
{
    o << "{enable = "             << !conf.power_down
      << ", if_type = "           << mac_if2txt(conf.if_type)
      << ", speed = "             << port_speed_to_txt(conf.speed)
      << ", fdx = "               << conf.fdx
      << ", flow_control = "      << conf.flow_control
      << ", mtu = "               << conf.max_frame_length
      << ", frame_length_chk = "  << conf.frame_length_chk
      << ", exc_col_cont = "      << conf.exc_col_cont
      << ", serdes = "            << conf.serdes
      << ", pcs = "               << conf.pcs

#ifdef PORT_INSTANCE_TRACE_RARELY_USED_FIELDS
      << ", sd_enable = "         << conf.sd_enable
      << ", sd_active_high = "    << conf.sd_active_high
      << ", sd_internal = "       << conf.sd_internal
      << ", frame_gaps = "        << conf.frame_gaps
      << ", max_tags = "          << conf.max_tags
      << ", xaui_rx_lane_flip = " << conf.xaui_rx_lane_flip
      << ", xaui_tx_lane_flip = " << conf.xaui_tx_lane_flip
      << ", loop = "              << conf.loop
#endif

      << "}";

    return o;
}

/******************************************************************************/
// mesa_aneg_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_aneg_t &aneg)
{
    o << "{obey= "  << aneg.obey_pause
      << ", gen = " << aneg.generate_pause
      << "}";

    return o;
}

/******************************************************************************/
// mepa_manual_neg_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mepa_manual_neg_t &man_neg)
{
    switch (man_neg) {
    case MEPA_MANUAL_NEG_DISABLED:
        o << "Auto";
        break;
    case MEPA_MANUAL_NEG_REF:
        o << "Reference";
        break;
    case MEPA_MANUAL_NEG_CLIENT:
        o << "Client";
        break;
    }

    return o;
}

#ifdef PORT_INSTANCE_TRACE_RARELY_USED_FIELDS
/******************************************************************************/
// vtss_appl_port_power_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_port_power_status_t &power)
{
    o << "{actiphy_capable = " << power.actiphy_capable
      << ", perfectreach_capable = " << power.perfectreach_capable
      << ", actiphy_power_savings = " << power.actiphy_power_savings
      << ", perfectreach_power_savings = " << power.perfectreach_power_savings
      << "}";

    return o;
}
#endif

/******************************************************************************/
// mepa_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mepa_conf_t &conf)
{
    char buf[100];

    o << "{enable = "        << conf.admin.enable
      << ", speed = "        << port_speed_to_txt(conf.speed)
      << ", fdx = "          << conf.fdx
      << ", flow_control = " << conf.flow_control
      << ", adv_dis = "      << '<' << port_adv_dis_to_txt(buf, sizeof(buf), (mepa_adv_dis_t)conf.adv_dis) << '>'
      << ", man_neg = "      << conf.man_neg
      << "}";

    return o;
}

/******************************************************************************/
// mepa_reset_param_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mepa_reset_param_t &reset_param)
{
    o << "{media_intf = "    << phy_media_if2txt(reset_param.media_intf)
      << ", reset_point = "  << phy_reset_point_to_str(reset_param.reset_point)
      << "}";

    return o;
}

/******************************************************************************/
// mepa_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mepa_status_t &status)
{
    o << "{link = "     << status.link
      << ", speed = "   << port_speed_to_txt(status.speed)
      << ", fdx = "     << status.fdx
      << ", aneg = "    << status.aneg
      << ", copper = "  << status.copper
      << ", fiber = "   << status.fiber
      << "}";

    return o;
}

/******************************************************************************/
// mepa_phy_info_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mepa_phy_info_t &info)
{
    char buf[100];

    // Print the part number both as decimal and hex, because the PHY guys tend
    // to forget that it's actually BCD.
    o << "{part_number = "   << info.part_number << " = 0x" << vtss::hex(info.part_number)
      << ", revision = "     << info.revision
      << ", cap = <"         << phy_cap_to_str(buf, sizeof(buf), info.cap) << ">"
      << ", ts_base_port = " << info.ts_base_port
      << "}";

    return o;
}

/******************************************************************************/
// meba_sfp_driver_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const meba_sfp_driver_conf_t &conf)
{
    o << "{enable = "        << conf.admin.enable
      << ", speed = "        << port_speed_to_txt(conf.speed)
      << ", flow_control = " << conf.flow_control
      << "}";

    return o;
}

/******************************************************************************/
// meba_sfp_driver_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const meba_sfp_driver_status_t &status)
{
    o << "{link = "   << status.link
      << ", speed = " << port_speed_to_txt(status.speed)
      << ", fdx = "   << status.fdx
      << ", los "     << status.los
      << ", aneg = "  << status.aneg // mesa_aneg_t
      << "}";

    return o;
}

/******************************************************************************/
// meba_sfp_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const meba_sfp_status_t &status)
{
    o << "{tx_fault = " << status.tx_fault
      << ", los = "     << status.los
      << ", present = " << status.present
      << "}";

    return o;
}

/******************************************************************************/
// mesa_port_kr_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_kr_conf_t &conf)
{
    o << "{aneg = "   << conf.aneg  // mesa_port_kr_aneg_t
      << ", train = " << conf.train // mesa_port_kr_train_t
      << "}";

    return o;
}

/******************************************************************************/
// mesa_port_kr_fec_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_kr_fec_t &conf)
{
    o << "{r_fec = "   << conf.r_fec
      << ", rs_fec = " << conf.rs_fec
      << "}";

    return o;
}

/******************************************************************************/
// mesa_port_kr_status_aneg_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_kr_status_aneg_t &aneg)
{
    o << "{complete = "        << aneg.complete
      << ", active = "         << aneg.active
      << ", speed_req = "      << port_speed_to_txt(aneg.speed_req)
      << ", fec_change_req = " << aneg.request_fec_change
      << ", r_fec_enable = "   << aneg.r_fec_enable;

    if (PORT_cap.has_kr_v3) {
        o << ", rs_fec_enable = " << aneg.rs_fec_enable;
    }

#ifdef PORT_INSTANCE_TRACE_RARELY_USED_FIELDS
    o << ", sm = "           << aneg.sm
      << ", lp_aneg_able = " << aneg.lp_aneg_able
      << ", block_lock = "   << aneg.block_lock;

    if (PORT_cap.has_kr_v3) {
        o << ", hist = "     << aneg.hist
          << ", lp_bp0 = "   << aneg.lp_bp0
          << ", lp_bp1 = "   << aneg.lp_bp1
          << ", lp_bp2 = "   << aneg.lp_bp2
          << ", lp_np0 = "   << aneg.lp_np0
          << ", lp_np1 = "   << aneg.lp_np1
          << ", lp_np2 = "   << aneg.lp_np2;
    }
#endif

    o << "}";

    return o;
}

/******************************************************************************/
// mesa_port_kr_status_train_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_kr_status_train_t &train)
{
    o << "{complete = " << train.complete
      << ", ob_tap_result = {"
      << "cm = "   << train.cm_ob_tap_result
      << ", cp = " << train.cp_ob_tap_result
      << ", c0 = " << train.c0_ob_tap_result
      << "}";

    if (PORT_cap.has_kr_v3) {
        o << ", frame_sent = "   << train.frame_sent
          << ", frame_errors = " << train.frame_errors;
    }

    o << "}";

    return o;
}

/******************************************************************************/
// mesa_port_kr_status_fec_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_kr_status_fec_t &fec)
{
    o << "{r_fec_enable = " << fec.r_fec_enable;

    if (PORT_cap.has_kr_v3) {
        o << ", rs_fec_enable = " << fec.rs_fec_enable;
    }

    o << ", corrected_block_cnt = "   << fec.corrected_block_cnt
      << ", uncorrected_block_cnt = " << fec.uncorrected_block_cnt
      << "}";

    return o;
}

/******************************************************************************/
// mesa_port_kr_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_kr_status_t &status)
{
    o << "{aneg = "   << status.aneg  // mesa_port_kr_status_aneg_t
      << ", train = " << status.train // mesa_port_kr_status_train_t
      << ", fec = "   << status.fec   // mesa_port_kr_status_fec_t
      << "}";

    return o;
}

/******************************************************************************/
// port_vol_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const port_vol_conf_t &vol_conf)
{
    o << "{disable = "              << vol_conf.disable
      << ", disable_adm_recover = " << vol_conf.disable_adm_recover
      << ", oper_down = "           << vol_conf.oper_down
      << ", oper_up = "             << vol_conf.oper_up
      << "}";

    return o;
}

/******************************************************************************/
// port_vol_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const port_vol_conf_t *vol_conf)
{
    o << *vol_conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_port_conf_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_port_conf_t &conf)
{
    char buf1[20];
    char buf2[100];

    o << "{enable = "            << conf.admin.enable
      << ", media_type = "       << port_media_type_to_txt(conf.media_type)
      << ", speed = "            << port_speed_to_txt(conf.speed)
      << ", adv_dis = "          << '<' << port_adv_dis_to_txt(buf2, sizeof(buf2), conf.adv_dis) << '>'
      << ", fdx = "              << conf.fdx
      << ", flow_control = "     << conf.flow_control
      << ", pfc = "              << pfc2txt(conf.pfc, buf1)
      << ", max_length = "       << conf.max_length
      << ", exc_col_cont = "     << conf.exc_col_cont
      << ", frame_length_chk = " << conf.frame_length_chk
      << ", force_clause_73 = "  << conf.force_clause_73
      << ", clause_73_pd = "     << conf.clause_73_pd
      << ", fec_mode = "         << port_fec_mode_to_txt(conf.fec_mode, false)
      << ", power_mode = "       << power_mode2txt(conf.power_mode)
      << ", dscr = "             << '"' << conf.dscr << '"'
      << "}";

    return o;
}

static bool PORT_INSTANCE_dont_trace_all;
/******************************************************************************/
// vtss_appl_port_status_t::operator<<()
// Used for tracing.
/******************************************************************************/
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_port_status_t &status)
{
    char buf1[200];
    char buf2[400];
    char buf3[400];

    o << "{link = "            << status.link
      << ", speed = "          << port_speed_to_txt(status.speed)
      << ", aneg_method = "    << port_aneg_method_to_txt(status.aneg_method)
      << ", fdx = "            << status.fdx
      << ", fiber = "          << status.fiber
      << ", sfp_type = "       << port_sfp_type_to_txt(status.sfp_type)
      << ", sfp_speeds = "     << port_speed_to_txt(status.sfp_speed_min) << " - " << port_speed_to_txt(status.sfp_speed_max)
      << ", aneg = "           << status.aneg     // mesa_aneg_t
      << ", has_kr = "         << status.has_kr
      << ", mac_if = "         << mac_if2txt(status.mac_if)
      << ", oper_warnings = <" << port_oper_warnings_to_txt(buf1, sizeof(buf1), status.oper_warnings) << ">";

    if (!PORT_INSTANCE_dont_trace_all) {
        o << ", sfp_info = "     << status.sfp_info // meba_sfp_device_info_t
          << ", static_caps = <" << port_cap_to_txt(buf2, sizeof(buf2), status.static_caps) << ">"
          << ", sfp_caps = <"    << port_cap_to_txt(buf3, sizeof(buf3), status.sfp_caps) << ">";
    }

#ifdef PORT_INSTANCE_TRACE_RARELY_USED_FIELDS
    o << ", power = "        << status.power // vtss_appl_port_power_status_t
      << ", loop_port = "    << status.loop_port;
#endif

    o << "}";

    return o;
}

/******************************************************************************/
// mesa_port_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_port_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_port_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_port_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mepa_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mepa_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mepa_reset_param_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mepa_reset_param_t *reset_param)
{
    o << *reset_param;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mepa_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mepa_status_t *status)
{
    o << *status;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mepa_phy_info_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mepa_phy_info_t *info)
{
    o << *info;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// meba_sfp_driver_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const meba_sfp_driver_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// meba_sfp_driver_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const meba_sfp_driver_status_t *status)
{
    o << *status;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_port_kr_conf_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_port_kr_conf_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_port_kr_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_port_kr_status_t *status)
{
    o << *status;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// mesa_port_kr_fec_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_port_kr_fec_t *conf)
{
    o << *conf;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

/******************************************************************************/
// vtss_appl_port_status_t::fmt()
// Used for tracing using T_x() macros
/******************************************************************************/
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_port_status_t *status)
{
    o << *status;

    // We should have returned number of bytes we have written, but we can't
    // find it. This, however, also works.
    return 0;
}

#define PORT_IS_DUAL_MEDIA()               ((_port_status.static_caps & (MEBA_PORT_CAP_DUAL_COPPER | MEBA_PORT_CAP_DUAL_FIBER)) != 0)
#define PORT_IS_DUAL_MEDIA_INTERNAL_PHY()  (PORT_IS_DUAL_MEDIA() && (_port_status.static_caps & MEBA_PORT_CAP_INT_PHY)          != 0)
#define PORT_IS_DUAL_MEDIA_EXTERNAL_PHY()  (PORT_IS_DUAL_MEDIA() && (_port_status.static_caps & MEBA_PORT_CAP_INT_PHY)          == 0)
#define PORT_IS_DUAL_MEDIA_NO_COPPER_SFP() (PORT_IS_DUAL_MEDIA() && (_port_status.static_caps & MEBA_PORT_CAP_DUAL_NO_COPPER)   != 0)

/******************************************************************************/
// PORT_INSTANCE_yes_no_str()
/******************************************************************************/
static const char *PORT_INSTANCE_yes_no_str(bool b)
{
    return b ? "Yes" : "No";
}

/******************************************************************************/
// port_instance::admin_enabled()
/******************************************************************************/
bool port_instance::admin_enabled(const vtss_appl_port_conf_t &conf, const port_vol_conf_t &vol_conf) const
{
    // Volatile configuration wins.
    if (vol_conf.disable) {
        return false;
    }

    return conf.admin.enable;
}

static mepa_port_interface_t rgmii_id_convert(mepa_port_interface_t interface)
{
    switch (interface) {
    case MESA_PORT_INTERFACE_RGMII:
        return MESA_PORT_INTERFACE_RGMII_ID;
    case MESA_PORT_INTERFACE_RGMII_ID:
        return MESA_PORT_INTERFACE_RGMII;
    case MESA_PORT_INTERFACE_RGMII_RXID:
        return MESA_PORT_INTERFACE_RGMII_TXID;
    case MESA_PORT_INTERFACE_RGMII_TXID:
        return MESA_PORT_INTERFACE_RGMII_RXID;
    default:
        return interface;
    }
}

/******************************************************************************/
// port_instance::mac_if_from_speed()
/******************************************************************************/
mesa_port_interface_t port_instance::mac_if_from_speed(mesa_port_speed_t &speed, const char **mac_if_came_from)
{
    mesa_port_interface_t mac_if;
    mesa_port_speed_t     orig_speed = speed;

    if (!_mac_if_use_port_custom_table) {
        if (!PORT_IS_DUAL_MEDIA_EXTERNAL_PHY()) {
            if (_port_status.fiber) {
                if (_sfp_dev) {
                    if (_sfp_dev->drv->meba_sfp_driver_if_get(_sfp_dev, speed, &mac_if) == VTSS_RC_OK) {
                        // Speed is supported.
                        *mac_if_came_from = "SFP driver";
                        goto do_exit;
                    }
                }
            } else {
                if (_phy_dev) {
                    if (meba_phy_if_get(board_instance, _port_no, speed, &mac_if) == VTSS_RC_OK) {
                        // Speed is supported.
                        *mac_if_came_from = "PHY driver";

                        mac_if = rgmii_id_convert(mac_if);

                        if (mac_if == MESA_PORT_INTERFACE_SFI) {
                            // Sometimes, the MAC I/F dictates another speed
                            // than what we think we should apply. This is the
                            // case when e.g. an AQR PHY is present on a JR2-24
                            // side board.
                            if (speed != MESA_SPEED_5G && speed != MESA_SPEED_10G && speed != MESA_SPEED_25G) {
                                speed = MESA_SPEED_10G;
                            }
                        }

                        goto do_exit;
                    }
                }
            }
        } else {
            // On dual media ports with external PHYs (currently only Gi 1/21-24
            // on Lu26), we need to use a fixed interface between MAC and PHY
            // and leave it up to the external PHY to set the output interface
            // (see phy_media_if_set()).
            if (_phy_dev) {
                if (meba_phy_if_get(board_instance, _port_no, speed, &mac_if) == VTSS_RC_OK) {
                    // Speed is supported.
                    *mac_if_came_from = "External PHY driver";
                    goto do_exit;
                }
            }
        }
    }

    // Use default interface. Since we don't have a PHY and don't have an SFP,
    // we cannot have link and therefore may have to modify the requested speed
    // so that the API doesn't complain that the speed doesn't match the
    // requested interface.
    *mac_if_came_from = "port_custom_table[]";
    mac_if = port_custom_table[_port_no].mac_if;

    switch (mac_if) {
    case MESA_PORT_INTERFACE_100FX:
        speed = MESA_SPEED_100M;
        break;

    case MESA_PORT_INTERFACE_RGMII:
    case MESA_PORT_INTERFACE_RGMII_ID:
    case MESA_PORT_INTERFACE_RGMII_RXID:
    case MESA_PORT_INTERFACE_RGMII_TXID:
    case MESA_PORT_INTERFACE_SGMII:
    case MESA_PORT_INTERFACE_SGMII_CISCO:
    case MESA_PORT_INTERFACE_SERDES:
    case MESA_PORT_INTERFACE_QSGMII:
        speed = MESA_SPEED_1G;
        break;

    case MESA_PORT_INTERFACE_SGMII_2G5:
    case MESA_PORT_INTERFACE_VAUI:
        speed = MESA_SPEED_2500M;
        break;

    case MESA_PORT_INTERFACE_XAUI:
    case MESA_PORT_INTERFACE_SFI:
        speed = MESA_SPEED_10G;
        break;

    default:
        T_EG_PORT(PORT_TRACE_GRP_CONF, _port_no, "Unable to deduce usable speed from port_custom_table[].mac_if (%s/%d). Please add it to switch (mac_if)", mac_if2txt(mac_if), mac_if);
        speed = MESA_SPEED_1G;
        break;
    }

    if (speed != orig_speed) {
        T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "Adjusted requested speed = %s to new speed = %s to satisfy MAC I/F = %s from port_custom_table[]", port_speed_to_txt(orig_speed), port_speed_to_txt(speed), mac_if2txt(mac_if));
    }

do_exit:
    T_NG_PORT(PORT_TRACE_GRP_CONF, _port_no, "speed = %s => mac_if = %s, which came from %s", port_speed_to_txt(speed), mac_if2txt(mac_if), *mac_if_came_from);
    return mac_if;
}

/*******************************************************************************/
// running_1g_aneg()
/*******************************************************************************/
bool port_instance::running_1g_aneg(void)
{
    // We return true if either using clause 28 (PHY/twisted pair aneg) or
    // clause 37 (SFP/serdes aneg).
    return _port_status.aneg_method == VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_28 || _port_status.aneg_method == VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_37;
}

/******************************************************************************/
// port_instance::port_conf_set()
/******************************************************************************/
void port_instance::port_conf_set(const char *func, int line, bool force_new_conf)
{
    mesa_port_conf_t  new_port_conf = _port_conf;
    bool              use_kr        = false;
    bool              forced_speed;
    const char        *mac_if_came_from;
    mesa_rc           rc;
    bool              mepa_cap_phy_ts = false, using_pfc;

    _port_status.oper_warnings &= ~VTSS_APPL_PORT_OPER_WARNING_CANNOT_OBEY_ANEG_FC_WHEN_PFC_ENABLED;

    if (_port_status.oper_warnings & VTSS_APPL_PORT_OPER_WARNING_PORT_DOES_NOT_SUPPORT_SFP) {
        // This port does not support the inserted SFP. Do not attempt to invoke
        // mesa_port_conf_set(), because that will/may result in a trace error.
        T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "Max SFP speed (%s) is lower than minimum port speed (%s). Skipping", port_speed_to_txt(_port_status.sfp_speed_max), port_speed_to_txt(_port_speed_min));
        return;
    }

    new_port_conf.power_down        = !admin_enabled(_conf, _vol_conf);
    new_port_conf.max_frame_length  = _conf.max_length;
    new_port_conf.frame_length_chk  = _conf.frame_length_chk;
    new_port_conf.exc_col_cont      = _conf.exc_col_cont;
    memcpy(new_port_conf.flow_control.pfc, _conf.pfc, sizeof(new_port_conf.flow_control.pfc));

#if defined(VTSS_SW_OPTION_KR)
    // If KRv3 is enabled, it's KR that determines the speed and the port module
    // should simply use whatever KR has set with its own call to
    // mesa_port_conf_set().
    if (_kr_conf.aneg.enable && _kr_initialized && PORT_cap.has_kr_v3) {
        T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "Using what KR has set");
        mesa_port_conf_t pconf = {};
        (void)mesa_port_conf_get(nullptr, _port_no, &pconf);
        new_port_conf.speed = pconf.speed;
        use_kr = true;
    }
#endif

    if (!use_kr) {
        // _sfp_speed and _phy_speed may be either of MESA_SPEED_UNDEFINED (if
        // e.g. an SFP port without an SFP plugged in) or MESA_SPEED_AUTO if
        // running clause 28/37 aneg or it may be any forced speed (see details
        // in speed_update()).
        new_port_conf.speed = _port_status.fiber ? _sfp_speed : _phy_speed;
        T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "new_port_conf.speed = %s", port_speed_to_txt(new_port_conf.speed));
    } else {
        T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "new_port_conf.speed = %s", port_speed_to_txt(new_port_conf.speed));
    }

    if (new_port_conf.speed == MESA_SPEED_UNDEFINED) {
        // Interface is not up, which may happen e.g. if the SFP is not plugged
        // in or if the SFP doesn't support the configured speed. Default to the
        // user-configured speed.
        new_port_conf.speed = _conf.speed;
        T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "new_port_conf.speed = %s", port_speed_to_txt(new_port_conf.speed));
    }

    forced_speed = true;
    if (new_port_conf.speed == MESA_SPEED_AUTO) {
        if (running_1g_aneg()) {
            forced_speed = false;

            // If no link:
            //   _port_status.speed               == MESA_SPEED_UNDEFINED
            //   _port_status.fdx                 == true
            //   _port_status.aneg.obey_pause     == false
            //   _port_status.aneg.generate_pause == false
            // Otherwise, it's what came out of the mesa_port_status_get(),
            // which is indirectly invoked by meba_phy_status_poll() or
            // meba_sfp_driver_poll()
            new_port_conf.speed                 = _port_status.speed;
            new_port_conf.fdx                   = _port_status.fdx;

            if (_port_status.aneg.obey_pause || _port_status.aneg.generate_pause) {
                if ((using_pfc = pfc_enabled(_conf))) {
                    // If PFC (Priority Flow Control) is enabled, we cannot run the
                    // anegged port flow control.
                    _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_CANNOT_OBEY_ANEG_FC_WHEN_PFC_ENABLED;
                }
            }

            new_port_conf.flow_control.obey     = using_pfc ? false : _port_status.aneg.obey_pause;
            new_port_conf.flow_control.generate = using_pfc ? false : _port_status.aneg.generate_pause;
            T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "new_port_conf.speed = %s", port_speed_to_txt(new_port_conf.speed));

            // If the port doesn't support hdx, but aneg - for some odd reason -
            // has negotiated hdx, we need to change that to fdx.
            if (!new_port_conf.fdx && (_port_status.static_caps & MEBA_PORT_CAP_HDX) == 0) {
                new_port_conf.fdx = true;
            }

            if (new_port_conf.speed == MESA_SPEED_UNDEFINED) {
                // Aneg not completed. Use the default of 1G.
                new_port_conf.speed = MESA_SPEED_1G;
                T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "new_port_conf.speed = %s", port_speed_to_txt(new_port_conf.speed));

                if (_port_status.fiber) {
                    if (_port_status.sfp_speed_max == MESA_SPEED_100M) {
                        // If, however, we are using an SFP and the maximum SFP
                        // speed is 100M, then use 100M
                        new_port_conf.speed = MESA_SPEED_100M;
                        T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "new_port_conf.speed = %s", port_speed_to_txt(new_port_conf.speed));
                    }
                }
            }

            if (_is_aqr_phy) {
                // On 2.5G, 5G, and 10G Cu ports (Aquantia), We always use 10G
                // Serdes (SFI) between the MAC and the 10G PHY. When the port
                // gets link, _phy_status also tells the switch to obey flow
                // control from the Aquantia PHY.
                new_port_conf.speed = MESA_SPEED_10G;
                T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "Aquantia: new_port_conf.speed = %s", port_speed_to_txt(new_port_conf.speed));
            }
        } else {
            // This is a 10G or 25G SFP interface, since we didn't get into the
            // running_1g_aneg(), while speed is MESA_SPEED_AUTO.
            // We only get here if an SFP is not plugged in (otherwise,
            // _sfp_speed would have a fixed speed corresponding to the minimum
            // of the port's max speed and the SFP's max speed, and would have
            // been set further up in this function). So since we get here,
            // there's no link and we set a speed corresponding to what the port
            // can handle.
            if (_port_status.static_caps & MEBA_PORT_CAP_25G_FDX) {
                new_port_conf.speed = MESA_SPEED_25G;
                T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "new_port_conf.speed = %s", port_speed_to_txt(new_port_conf.speed));
            } else if (_port_status.static_caps & MEBA_PORT_CAP_10G_FDX) {
                new_port_conf.speed = MESA_SPEED_10G;
                T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "new_port_conf.speed = %s", port_speed_to_txt(new_port_conf.speed));
            } else {
                T_EG_PORT(PORT_TRACE_GRP_CONF, _port_no, "What?!?");
                new_port_conf.speed = MESA_SPEED_1G;
                T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "new_port_conf.speed = %s", port_speed_to_txt(new_port_conf.speed));
            }
        }
    }

    if (forced_speed) {
        // Use configured flow control and duplex when speed is forced.
        new_port_conf.flow_control.obey     = _conf.flow_control;
        new_port_conf.flow_control.generate = _conf.flow_control;
        new_port_conf.fdx                   = _conf.fdx;

        if (_port_status.link) {
            _port_status.speed = new_port_conf.speed;
        }
    }

    // Get the interface to use between MAC and PHY/SFP
    _port_status.mac_if             = mac_if_from_speed(new_port_conf.speed, &mac_if_came_from);
    new_port_conf.if_type           = _port_status.mac_if;
    new_port_conf.pcs               = _pcs_conf;
    new_port_conf.serdes.media_type = serdes_media_type_get(new_port_conf.speed);
    T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "new_port_conf.speed = %s", port_speed_to_txt(new_port_conf.speed));

    if (!force_new_conf && memcmp(&new_port_conf, &_port_conf, sizeof(new_port_conf)) == 0) {
        // No changes.
        T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "%s#%d: No changes (%s)", func, line, new_port_conf);
        return;
    }

    T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "%s#%d: mac_if from %s: mesa_port_conf_set(%s)", func, line, mac_if_came_from, new_port_conf);
    if ((rc = mesa_port_conf_set(nullptr, _port_no, &new_port_conf)) != VTSS_RC_OK) {
        T_EG_PORT(PORT_TRACE_GRP_CONF, _port_no, "%s#%d: mac_if from %s: mesa_port_conf_set(%s) failed: %s", func, line, mac_if_came_from, new_port_conf, error_txt(rc));
        T_EG_PORT(PORT_TRACE_GRP_CONF, _port_no, "%s#%d. port_status = %s", func, line, _port_status);
    }

    // Reset the MAC side path in PHY for QSGMII when MAC is configured for the
    // first time.
    if (_phy_dev && _port_status.mac_if == MESA_PORT_INTERFACE_QSGMII && !_phy_host_side_reset) {
        (void)phy_reset(true);
        _phy_host_side_reset = true;
    }

    _port_conf = new_port_conf;

#if defined(VTSS_SW_OPTION_TOD)
    mepa_cap_phy_ts = mepa_phy_ts_cap();
#endif
    if (mepa_cap_phy_ts || fast_cap(MESA_CAP_TS)) {
        (void)mesa_ts_status_change(nullptr, _port_no);
    }
}

/******************************************************************************/
// port_instance::port_state_set()
/******************************************************************************/
void port_instance::port_state_set(const char *func, int line, bool enable)
{
    bool    up;
    mesa_rc rc;

    // When changing the volatile oper flags, we don't need to configure the
    // port's state.
    // I think oper_down weighs heavier than oper_up.
    if (_vol_conf.oper_down) {
        up = false;
    } else if (_vol_conf.oper_up || _port_status.loop_port) {
        // If either asked to set it volatilly operationally up or the port
        // is a loop port, set it operationally up
        up = true;
    } else {
        // Use the port's link state if not overriding.
        up = enable;
    }

    T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "%s#%d: mesa_port_state_set(%d)", func, line, up);
    if ((rc = mesa_port_state_set(nullptr, _port_no, up)) != VTSS_RC_OK) {
        T_EG_PORT(PORT_TRACE_GRP_CONF, _port_no, "%s#%d: mesa_port_state_set(%d) failed: %s", func, line, up, error_txt(rc));
    }

    _port_state = up;
}

/******************************************************************************/
// port_instance::phy_man_neg_set()
// Private
// In 1G aneg one end is reference and the other end is
// client. Usually it is random which end is reference and which end
// is client. However, when using syncE, the syncE information can
// only be transferred from the reference to the client, so when syncE
// choose to use a port for synchronization, it may have to change the
// direction of the link. There is no reason to specify the direction
// of the link in the running-config, depending on network topology,
// syncE may still have to change direction of a link. SyncE just
// needs to be able to read the current configuration and to be able
// to change it when needed.
/******************************************************************************/
mesa_rc port_instance::phy_man_neg_set(const char *func, int line, mepa_manual_neg_t man_neg)
{
    mepa_conf_t new_phy_conf = _phy_conf;
    mesa_rc     rc;

    if (!_phy_dev) {
        return VTSS_RC_ERROR;
    }

    if (man_neg == _phy_conf.man_neg) {
        // No changes
        return VTSS_RC_OK;
    }

    new_phy_conf.man_neg = man_neg;

    T_IG_PORT(PORT_TRACE_GRP_PHY, _port_no, "%s#%d: meba_phy_conf_set(%s)", func, line, new_phy_conf);
    if ((rc = meba_phy_conf_set(board_instance, _port_no, &new_phy_conf)) != VTSS_RC_OK) {
        T_EG_PORT(PORT_TRACE_GRP_PHY, _port_no, "meba_phy_conf_set(%s) failed: %s", new_phy_conf, error_txt(rc));
        return VTSS_RC_ERROR;
    }

    _phy_conf = new_phy_conf;
    return VTSS_RC_OK;
}

/******************************************************************************/
// port_instance::phy_man_neg_get()
// Private
/******************************************************************************/
mepa_manual_neg_t port_instance::phy_man_neg_get(const char *func, int line)
{
    return _phy_conf.man_neg;
}

/******************************************************************************/
// port_instance::phy_conf_set()
// Private
/******************************************************************************/
void port_instance::phy_conf_set(const char *func, int line, bool enable, bool force_new_conf)
{
    mepa_conf_t new_phy_conf = _phy_conf;
    mesa_rc     rc;

    if (!_phy_dev) {
        return;
    }

    // This one can accept speed = MESA_SPEED_AUTO
    if (force_new_conf) {
        new_phy_conf.speed        = _conf.speed;
        new_phy_conf.fdx          = _conf.fdx;          // Only used if speed is forced
        new_phy_conf.flow_control = _conf.flow_control; // Only used if speed is auto
        new_phy_conf.adv_dis      = _conf.adv_dis;      // Only used if speed is auto
    }

    new_phy_conf.admin.enable = enable;

    if (memcmp(&new_phy_conf, &_phy_conf, sizeof(new_phy_conf)) == 0) {
        // No changes
        return;
    }

    T_IG_PORT(PORT_TRACE_GRP_PHY, _port_no, "%s#%d: meba_phy_conf_set(%s)", func, line, new_phy_conf);
    if ((rc = meba_phy_conf_set(board_instance, _port_no, &new_phy_conf)) != VTSS_RC_OK) {
        T_EG_PORT(PORT_TRACE_GRP_PHY, _port_no, "meba_phy_conf_set(%s) failed: %s", new_phy_conf, error_txt(rc));
        return;
    }

    _phy_conf = new_phy_conf;
}

/******************************************************************************/
// port_instance::phy_media_if_get()
// Private
/******************************************************************************/
mepa_media_interface_t port_instance::phy_media_if_get(void)
{
    if (!_phy_dev) {
        // Return an otherwise unused value if we don't have a PHY.
        return MESA_PHY_MEDIA_IF_AMS_FI_PASSTHRU;
    }

    // Note, we don't use the AMS (Automatic Media Sense) modes, because we use
    // the presence or lack of presence of an SFP to determine the mode.
    if (_port_status.fiber) {
        if (_is_100m_sfp || _port_status.speed == MESA_SPEED_100M || _port_status.mac_if == MESA_PORT_INTERFACE_100FX) {
            // 100 Mbps fiber
            return MESA_PHY_MEDIA_IF_FI_100FX;
        }

        if (_port_status.mac_if == MESA_PORT_INTERFACE_SGMII_CISCO) {
            // CuSFP
            return MESA_PHY_MEDIA_IF_SFP_PASSTHRU;
        }

        return MESA_PHY_MEDIA_IF_FI_1000BX;
    }

    // Copper port.
    return MESA_PHY_MEDIA_IF_CU;
}

/******************************************************************************/
// port_instance::phy_media_if_set()
// Private
/******************************************************************************/
void port_instance::phy_media_if_set(void)
{
    mepa_media_interface_t new_phy_media_if;
    mesa_rc                rc;

    if (!PORT_IS_DUAL_MEDIA_EXTERNAL_PHY()) {
        // We only need to set the media-side (outbound) interface on external
        // PHYs.
        return;
    }

    if (!_phy_dev) {
        // PHY not present.
        return;
    }

    new_phy_media_if = phy_media_if_get();

    if (new_phy_media_if == _phy_media_if) {
        // No changes
        return;
    }

    T_IG_PORT(PORT_TRACE_GRP_PHY, _port_no, "meba_phy_media_set(%s)", phy_media_if2txt(new_phy_media_if));
    if ((rc = meba_phy_media_set(board_instance, _port_no, new_phy_media_if)) != VTSS_RC_OK && rc != MESA_RC_NOT_IMPLEMENTED) {
        T_EG_PORT(PORT_TRACE_GRP_PHY, _port_no, "meba_phy_media_set(%s) failed: %s", phy_media_if2txt(new_phy_media_if), error_txt(rc));
    }

    _phy_media_if = new_phy_media_if;
}

/******************************************************************************/
// port_instance::sfp_admin_state_set()
/******************************************************************************/
void port_instance::sfp_admin_state_set(const char *func, int line, bool enable)
{
    meba_port_admin_state_t state;
    mesa_rc                 rc;

    if (!_sfp_dev) {
        return;
    }

    if (_sfp_initialized && _sfp_turned_on == enable) {
        // Already has this value and it's been called at least once before.
        return;
    }

    state.enable = enable;

    T_IG_PORT(PORT_TRACE_GRP_SFP, _port_no, "%s#%d: meba_port_admin_state_set(%d)", func, line, enable);

    {
        // meba_port_admin_state_set() calls mesa_sgpio_conf_get()/set(), and
        // these two calls must be invoked without interference.
        VTSS_APPL_API_LOCK_SCOPE();

        if ((rc = meba_port_admin_state_set(board_instance, _port_no, &state)) != VTSS_RC_OK) {
            T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "meba_port_admin_state_set(%d) failed: %s", enable, error_txt(rc));
        }
    }

    if (!enable) {
        // In order for a possible link down to be propagated, we need to wait
        // for the new setting to be propagated.
        VTSS_MSLEEP(50);
    }

    _sfp_turned_on = enable;
}

/******************************************************************************/
// port_instance::sfp_conf_set()
// Private
/******************************************************************************/
void port_instance::sfp_conf_set(const char *func, int line, bool enable)
{
    meba_sfp_driver_conf_t new_sfp_conf;
    mesa_rc                rc;

    if (!_sfp_dev) {
        return;
    }

    // The call to meba_sfp_driver_conf_set() either becomes a call that sets
    // up clause 37 in the PCS or - in case of a CuSFP - a call that sets up
    // advertisement of all features in the PHY in the CuSFP.
    //
    // In the first case, speed, admin.enable, and flow_control is used as
    // follows:
    //    if speed == MESA_SPEED_AUTO *and* it's not a dual media port with
    //       external PHY:
    //        Enable aneg in PCS while using admin.enable to advertise whether
    //        to use remote_fault == RF_LINK_ON or RF_OFFLINE and flow_control
    //        to advertise symmetric_pause and asymmetric_pause.
    //        It always advertises FDX only (not HDX).
    //    else
    //       Disable aneg in PCS
    //
    // In the second case (CuSFP), only admin.enable is used.
    //
    // The rule is actually something like: If using _port_conf.if_type ==
    // MESA_PORT_INTERFACE_QSGMII, we cannot use clause 37 aneg in the PCS, but
    // I don't think we have computed if_type in all cases when we get here.
    //
    // RBNTBD: If using a CuSFP and _conf.flow_control == true, we should issue
    // an operational warning, that we cannot obey that, or...?

    vtss_clear(new_sfp_conf);
    // _conf.speed == MESA_SPEED_AUTO is also used on 10G and 25G ports, so we
    // cannot use that in new_sfp_conf.speed. running_1g_aneg() is accurate.
    new_sfp_conf.speed        = running_1g_aneg() && !PORT_IS_DUAL_MEDIA_EXTERNAL_PHY() ? MESA_SPEED_AUTO : MESA_SPEED_1G;
    new_sfp_conf.admin.enable = enable;
    new_sfp_conf.flow_control = _conf.flow_control;

    if (_sfp_initialized && memcmp(&new_sfp_conf, &_sfp_conf, sizeof(new_sfp_conf)) == 0) {
        // No changes
        return;
    }

    T_IG_PORT(PORT_TRACE_GRP_SFP, _port_no, "%s#%d: meba_sfp_driver_conf_set(%s)", func, line, new_sfp_conf);
    if ((rc = _sfp_dev->drv->meba_sfp_driver_conf_set(_sfp_dev, &new_sfp_conf)) == VTSS_RC_ERROR) {
        T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "%s#%d: meba_sfp_driver_conf_set(%s) failed: %s", func, line, new_sfp_conf, error_txt(rc));
        return;
    }

    _sfp_conf = new_sfp_conf;
}

/******************************************************************************/
// port_instance::turn_on_phy()
// The name of this function is somewhat misleading, because it doesn't
// necessarily turn on the PHY. If the administrative state is "shutdown", then
// the PHY is actually turned off.
// Nothing happens if we don't have a PHY device.
// Private
/******************************************************************************/
void port_instance::turn_on_phy(const char *func, int line, bool force_new_conf)
{
    phy_conf_set(func, line, admin_enabled(_conf, _vol_conf), force_new_conf);
}

/******************************************************************************/
// port_instance::turn_off_phy()
// Nothing happens if we don't have a PHY device.
// Private
/******************************************************************************/
void port_instance::turn_off_phy(const char *func, int line)
{
    phy_conf_set(func, line, false, true);
}

/******************************************************************************/
// port_instance::turn_on_sfp()
// The name of this function is somewhat misleading, because it doesn't
// necessarily turn on the SFP. If the administrative state is "shutdown", then
// the SFP is actually turned off.
// Also, if an SFP is inserted, but it's useless in the current configuration,
// it's also turned off.
// Nothing happens if we don't have an SFP device.
// Private
/******************************************************************************/
void port_instance::turn_on_sfp(const char *func, int line)
{
    bool turn_on = admin_enabled(_conf, _vol_conf) && sfp_usable();

    sfp_admin_state_set(func, line, turn_on);
    sfp_conf_set(       func, line, turn_on);
    _sfp_initialized = true;
}

/******************************************************************************/
// port_instance::turn_off_sfp()
// Nothing happens if we don't have an SFP device.
// Private
/******************************************************************************/
void port_instance::turn_off_sfp(const char *func, int line)
{
    sfp_admin_state_set(func, line, false);
    sfp_conf_set(       func, line, false);
    _sfp_initialized = true;
}

/******************************************************************************/
// port_instance::sfp_type_of_mac_to_mac()
// Private
/******************************************************************************/
vtss_appl_port_sfp_type_t port_instance::sfp_type_of_mac_to_mac(void)
{
    // This function gets invoked whenever we get instantiated with a MAC-to-MAC
    // SFP driver.

    // The MAC-to-MAC SFP driver gets instantiated in two cases:
    // 1) If it's true copper backplane (CuBP, no SFP, but serdes-to-serdes) and
    // 2) If an SFP is inserted, but the SFP's ROM cannot be read.
    //
    // For the sake of the user interface, we need to distinguish these two
    // cases.
    //
    // When MEBA claims that it's detectable when an SFP module is inserted,
    // it cannot be copper backplane.
    if (_port_status.static_caps & MEBA_PORT_CAP_SFP_DETECT ||
        _port_status.static_caps & MEBA_PORT_CAP_DUAL_SFP_DETECT) {
        // We couldn't read the SFP ROM.

        if ((_port_status.static_caps & MEBA_PORT_CAP_SFP_INACCESSIBLE) == 0) {
            // But we were not told that this was the case. Issue an error.
            // This error means one of two things:
            // 1) MEBA requires an update for this board if the SFP indeed is
            //    inaccessible.
            // 2) If the SFP *is* accessible through I2C, the SFP ROM read
            //    failed for some odd reason that needs to be investigated. This
            //    could be like prolonging the read timeouts in the API.
            // In case of 2) one occassion has been investigated: On Lu26, Gi
            // 1/21 is read through an external Atom12. This works in most
            // cases, but a few SFPs may not be readable in exactly this port,
            // but readable in other ports. Examples are Excom 1000BASE-ZX and
            // HP ProCurve 1000BASE-SX SFPs. Therefore, we cannot throw a trace
            // error, but must demote it to an info.
            T_IG_PORT(PORT_TRACE_GRP_SFP, _port_no, "SFP insertion is detectable, but we got instantiated with a MAC-to-MAC driver even though MEBA doesn't claim that the SFP ROM is not readable");
            _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_SFP_READ_FAILED;
        } else {
            _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_SFP_UNREADABLE_IN_THIS_PORT;
        }

        // Either way, tell everyone that this is an unknown SFP.
        return VTSS_APPL_PORT_SFP_TYPE_UNKNOWN;
    } else {
        // This SFP port does not support SFP detection, so it must be true
        // copper backplane
        return VTSS_APPL_PORT_SFP_TYPE_CU_BP;
    }
}

/******************************************************************************/
// port_instance::sfp_caps_update()
//
// Utilizes:
//   _sfp_dev.info.transceiver
//   _sfp_dev.info.pn_name (for Cu Backplane detection)
//   _conf.media_type
//   _port_status.has_kr
//   _port_status.static_caps
//
// Updates:
//   _port_status.oper_warnings
//   _port_status.sfp_caps
//   _port_status.sfp_type
//   _port_status.sfp_speed_min
//   _port_status.sfp_speed_max
//   _sfp_must_run_kr
//   _sfp_may_run_kr
//
// Private
/******************************************************************************/
void port_instance::sfp_caps_update(void)
{
    meba_port_cap_t dyn_cap;

    _port_status.oper_warnings &= ~VTSS_APPL_PORT_OPER_WARNING_PORT_DOES_NOT_SUPPORT_SFP;
    _port_status.oper_warnings &= ~VTSS_APPL_PORT_OPER_WARNING_SFP_UNREADABLE_IN_THIS_PORT;
    _port_status.oper_warnings &= ~VTSS_APPL_PORT_OPER_WARNING_SFP_READ_FAILED;
    _port_status.sfp_caps      = 0;
    _port_status.sfp_type      = VTSS_APPL_PORT_SFP_TYPE_NONE;
    _port_status.sfp_speed_min = MESA_SPEED_UNDEFINED;
    _port_status.sfp_speed_max = MESA_SPEED_UNDEFINED;
    _sfp_must_run_kr           = false;
    _sfp_may_run_kr            = false;
    _is_100m_sfp               = false;

    if (!_sfp_dev) {
        return;
    }

    // In dyn_cap, we set those *not* supported by the SFP and transform those
    // at the end of the function to those supported by it.
    switch (_sfp_dev->info.transceiver) {
    case MEBA_SFP_TRANSRECEIVER_100FX:
    case MEBA_SFP_TRANSRECEIVER_100BASE_LX:
    case MEBA_SFP_TRANSRECEIVER_100BASE_ZX:
    case MEBA_SFP_TRANSRECEIVER_100BASE_SX:
        _port_status.sfp_speed_min = MESA_SPEED_100M;
        _port_status.sfp_speed_max = MESA_SPEED_100M;
        _port_status.sfp_type      = VTSS_APPL_PORT_SFP_TYPE_OPTICAL;
        _is_100m_sfp               = true;

        dyn_cap = MEBA_PORT_CAP_AUTONEG  |
                  MEBA_PORT_CAP_10M_HDX  |
                  MEBA_PORT_CAP_10M_FDX  |
                  MEBA_PORT_CAP_100M_HDX |
                  MEBA_PORT_CAP_1G_FDX   |
                  MEBA_PORT_CAP_2_5G_FDX |
                  MEBA_PORT_CAP_5G_FDX   |
                  MEBA_PORT_CAP_10G_FDX  |
                  MEBA_PORT_CAP_25G_FDX;
        break;

    case MEBA_SFP_TRANSRECEIVER_1000BASE_T:
        _port_status.sfp_speed_min = MESA_SPEED_1G;
        _port_status.sfp_speed_max = MESA_SPEED_1G;
        _port_status.sfp_type      = VTSS_APPL_PORT_SFP_TYPE_CU;
        dyn_cap = MEBA_PORT_CAP_10M_HDX  |
                  MEBA_PORT_CAP_10M_FDX  |
                  MEBA_PORT_CAP_100M_HDX |
                  MEBA_PORT_CAP_100M_FDX |
                  MEBA_PORT_CAP_1G_FDX   |
                  MEBA_PORT_CAP_2_5G_FDX |
                  MEBA_PORT_CAP_5G_FDX   |
                  MEBA_PORT_CAP_10G_FDX  |
                  MEBA_PORT_CAP_25G_FDX;
        break;

    case MEBA_SFP_TRANSRECEIVER_1000BASE_CX:
    case MEBA_SFP_TRANSRECEIVER_1000BASE_SX:
    case MEBA_SFP_TRANSRECEIVER_1000BASE_LX:
    case MEBA_SFP_TRANSRECEIVER_1000BASE_ZX:
    case MEBA_SFP_TRANSRECEIVER_1000BASE_LR:
    case MEBA_SFP_TRANSRECEIVER_1000BASE_X:
        _port_status.sfp_speed_min = MESA_SPEED_10M;
        _port_status.sfp_speed_max = MESA_SPEED_1G;

        // We can't distinguish Cu backplane from the others, so let's use the
        // driver name to find out whether it's CuBP or something else.
        if (strcmp(_sfp_dev->info.vendor_pn, "MAC-to-MAC-1G") == 0) {
            // sfp_driver.c uses MEBA_SFP_TRANSRECEIVER_1000BASE_SX for CuBP!
            _port_status.sfp_type = sfp_type_of_mac_to_mac();
        } else if (_sfp_dev->info.transceiver == MEBA_SFP_TRANSRECEIVER_1000BASE_CX || _cu_sfp) {
            _port_status.sfp_type = VTSS_APPL_PORT_SFP_TYPE_CU;
        } else {
            _port_status.sfp_type = VTSS_APPL_PORT_SFP_TYPE_OPTICAL;
        }

        dyn_cap = MEBA_PORT_CAP_2_5G_FDX |
                  MEBA_PORT_CAP_5G_FDX   |
                  MEBA_PORT_CAP_10G_FDX  |
                  MEBA_PORT_CAP_25G_FDX;

        if (_conf.media_type == VTSS_APPL_PORT_MEDIA_SFP) {
            _port_status.sfp_speed_min = MESA_SPEED_100M;
            dyn_cap |= MEBA_PORT_CAP_10M_HDX |
                       MEBA_PORT_CAP_10M_FDX |
                       MEBA_PORT_CAP_100M_HDX;
        }

        break;

    case MEBA_SFP_TRANSRECEIVER_2G5:
        _port_status.sfp_speed_min = MESA_SPEED_10M;
        _port_status.sfp_speed_max = MESA_SPEED_2500M;

        // We can't distinguish Cu backplane from the others, so let's use the
        // driver name to find out whether it's CuBP or fiber.
        if (strcmp(_sfp_dev->info.vendor_pn, "MAC-to-MAC-2.5G") == 0) {
            _port_status.sfp_type = sfp_type_of_mac_to_mac();
        } else {
            _port_status.sfp_type = VTSS_APPL_PORT_SFP_TYPE_OPTICAL;
        }

        dyn_cap = MEBA_PORT_CAP_5G_FDX  |
                  MEBA_PORT_CAP_10G_FDX |
                  MEBA_PORT_CAP_25G_FDX;
        break;

    case MEBA_SFP_TRANSRECEIVER_5G:
        _port_status.sfp_speed_min = MESA_SPEED_10M;
        _port_status.sfp_speed_max = MESA_SPEED_5G;

        // We can't distinguish Cu backplane from the others, so let's use the
        // driver name to find out whether it's CuBP or fiber.
        if (strcmp(_sfp_dev->info.vendor_pn, "MAC-to-MAC-5G") == 0) {
            // For future use. No such driver currently exists in API, so
            // port.cxx cannot and doesn't utilize it.
            _port_status.sfp_type = sfp_type_of_mac_to_mac();
        } else {
            _port_status.sfp_type = VTSS_APPL_PORT_SFP_TYPE_OPTICAL;
        }

        dyn_cap = MEBA_PORT_CAP_10G_FDX |
                  MEBA_PORT_CAP_25G_FDX;
        break;

    case MEBA_SFP_TRANSRECEIVER_10G:
    case MEBA_SFP_TRANSRECEIVER_10G_SR:
    case MEBA_SFP_TRANSRECEIVER_10G_LR:
    case MEBA_SFP_TRANSRECEIVER_10G_LRM:
    case MEBA_SFP_TRANSRECEIVER_10G_ER:
    case MEBA_SFP_TRANSRECEIVER_10G_DAC:
        _port_status.sfp_speed_min = MESA_SPEED_10M;
        _port_status.sfp_speed_max = MESA_SPEED_10G;

        // We can't distinguish Cu backplane from the others, so let's use the
        // driver name to find out whether it's CuBP or one of the others.
        if (strcmp(_sfp_dev->info.vendor_pn, "MAC-to-MAC-10G") == 0) {
            _port_status.sfp_type = sfp_type_of_mac_to_mac();
        } else if (_sfp_dev->info.transceiver == MEBA_SFP_TRANSRECEIVER_10G_DAC) {
            _port_status.sfp_type = VTSS_APPL_PORT_SFP_TYPE_DAC;
        } else {
            _port_status.sfp_type = VTSS_APPL_PORT_SFP_TYPE_OPTICAL;
        }

        if (_port_status.has_kr) {
            _sfp_may_run_kr = true;

            if (_port_status.sfp_type == VTSS_APPL_PORT_SFP_TYPE_CU_BP) {
                // It's a Cu backplane connection, which must run clause 73 aneg
                // (if configured speed is auto). port.cxx makes sure that this
                // driver can only be instantiated on a 10G port.
                _sfp_must_run_kr = true;
            }
        }

        dyn_cap = MEBA_PORT_CAP_25G_FDX;
        break;

    case MEBA_SFP_TRANSRECEIVER_25G_DAC:
    case MEBA_SFP_TRANSRECEIVER_25G:
    case MEBA_SFP_TRANSRECEIVER_25G_SR:
    case MEBA_SFP_TRANSRECEIVER_25G_LR:
    case MEBA_SFP_TRANSRECEIVER_25G_LRM:
    case MEBA_SFP_TRANSRECEIVER_25G_ER:
        _port_status.sfp_speed_min = MESA_SPEED_10M;
        _port_status.sfp_speed_max = MESA_SPEED_25G;

        // Both MAC-to-MAC-25G and 25GBASE-CR(-S) DAC cables return 25G_DAC
        // transceiver. Use the driver name to find out whether it's one or the
        // other.
        if (strcmp(_sfp_dev->info.vendor_pn, "MAC-to-MAC-25G") == 0) {
            // port.cxx makes sure that this can only be instantiated on a 25G
            // port.
            _port_status.sfp_type = sfp_type_of_mac_to_mac();
        } else if (_sfp_dev->info.transceiver == MEBA_SFP_TRANSRECEIVER_25G_DAC) {
            _port_status.sfp_type = VTSS_APPL_PORT_SFP_TYPE_DAC;
        } else {
            _port_status.sfp_type = VTSS_APPL_PORT_SFP_TYPE_OPTICAL;
        }

        if (_port_status.has_kr) {
            // These SFPs may run clause 73.
            _sfp_may_run_kr = true;

            if (_port_status.static_caps & MEBA_PORT_CAP_25G_FDX) {
                if (_port_status.sfp_type == VTSS_APPL_PORT_SFP_TYPE_CU_BP || _port_status.sfp_type == VTSS_APPL_PORT_SFP_TYPE_DAC) {
                    // These *must* run clause 73 on 25G ports (not necessarily
                    // on 10G ports).
                    _sfp_must_run_kr = true;
                }
            }
        }

        dyn_cap = 0;
        break;

    default:
        T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "Unknown SFP transceiver (%d). Code update required", _sfp_dev->info.transceiver);
        dyn_cap = 0;
        break;
    }

    if (_port_status.sfp_speed_max < _port_speed_min) {
        // The SFP's maximum speed is smaller than the minimum speed supported
        // by this port.
        _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_PORT_DOES_NOT_SUPPORT_SFP;
    }

    // Clear those bits set in dyn_cap, that is, those that this SFP don't
    // support.
    _port_status.sfp_caps = _port_status.static_caps & ~dyn_cap;
}

/******************************************************************************/
// port_instance::sfp_may_not_work_oper_warning_update()
/******************************************************************************/
void port_instance::sfp_may_not_work_oper_warning_update(void)
{
    _port_status.oper_warnings &= ~VTSS_APPL_PORT_OPER_WARNING_SFP_NOMINAL_SPEED_HIGHER_THAN_PORT_SPEED;

    if (!_sfp_dev) {
        return;
    }

    // If the port's maximum supported speed is lower than the SFP's nominal
    // speed, flag the warning.
    if (_port_speed_max < _port_status.sfp_speed_max) {
        _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_SFP_NOMINAL_SPEED_HIGHER_THAN_PORT_SPEED;
        return;
    }

    // If the port is configured to a forced speed that is lower than the SFP's
    // nominal speed, flag the warning.
    if (_conf.speed != MESA_SPEED_AUTO) {
        if (_conf.speed < _port_status.sfp_speed_max) {
            _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_SFP_NOMINAL_SPEED_HIGHER_THAN_PORT_SPEED;
        }

        return;
    }

    // Here, we may run some kind of aneg, so we need to use the _port_status's
    // speed, because that one shows the right speed.
    if (_port_status.speed == MESA_SPEED_UNDEFINED) {
        // Don't flag anything (yet).
        return;
    }

    if (_port_status.speed < _port_status.sfp_speed_max) {
        // The port has clause 28/37/73 anegged (or is forced into) a given
        // speed that is lower than the SFP's nominal speed. Flag the warning.
        _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_SFP_NOMINAL_SPEED_HIGHER_THAN_PORT_SPEED;
    }
}

/******************************************************************************/
// port_instance::speed_update()
//
// Utilizes:
//   prefer_fiber (argument)
//   _conf.speed
//   _conf.fdx
//   _conf.force_clause_73
//   _port_status.sfp_type
//   _port_status.static_caps
//   _port_status.has_kr
//   _port_status.sfp_speed_min
//   _port_status.sfp_speed_max
//   _port_status.sfp_caps
//   _sfp_must_run_kr
//   _sfp_may_run_kr
//
// Updates:
//   _port_status.oper_warnings
//   _port_status.aneg_method
//   _sfp_speed
//   _phy_speed
//
// Private
/******************************************************************************/
void port_instance::speed_update(bool prefer_fiber)
{
    vtss_appl_port_aneg_method_t new_aneg_method;
    bool                         use_sfp_dev = _sfp_dev != nullptr;

    _port_status.oper_warnings &= ~VTSS_APPL_PORT_OPER_WARNING_CU_SFP_SPEED_AUTO;
    _port_status.oper_warnings &= ~VTSS_APPL_PORT_OPER_WARNING_FORCED_SPEED_NOT_SUPPORTED_BY_SFP;
    _port_status.oper_warnings &= ~VTSS_APPL_PORT_OPER_WARNING_SFP_CANNOT_RUN_CLAUSE_73;
    _port_status.oper_warnings &= ~VTSS_APPL_PORT_OPER_WARNING_SFP_DOES_NOT_SUPPORT_HDX;

    _sfp_speed               = MESA_SPEED_UNDEFINED;
    _phy_speed               = MESA_SPEED_UNDEFINED;
    _port_status.aneg_method = VTSS_APPL_PORT_ANEG_METHOD_UNKNOWN;

    // The following code requires _port_status.sfp_speed_min and
    // _port_status.sfp_speed_max to be defined if an SFP is inserted.
    if (_sfp_dev) {
        if (_port_status.sfp_speed_min == MESA_SPEED_UNDEFINED || _port_status.sfp_speed_max == MESA_SPEED_UNDEFINED) {
            use_sfp_dev = false;
            T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "Min/Max SFP speeds not updated: %s/%s", port_speed_to_txt(_port_status.sfp_speed_min), port_speed_to_txt(_port_status.sfp_speed_max));
        }
    }

    if (_conf.speed != MESA_SPEED_AUTO) {
        // Not using aneg at all with forced speeds.
        _port_status.aneg_method = VTSS_APPL_PORT_ANEG_METHOD_NONE;

        // Forced speeds are used as they are on PHYs, but require us to know
        // the SFP type for SFPs.
        // We know that the PHY supports the forced speed already (from
        // _port_status.static_caps), but for SFPs, this is a dynamic feature,
        // depending on the SFP plugged in.
        if (_phy_dev) {
            _phy_speed = _conf.speed;
        }

        if (use_sfp_dev) {
            if (_port_status.sfp_type == VTSS_APPL_PORT_SFP_TYPE_CU) {
                // First a special note to those plugging in CuSFPs:
                // We only support auto (clause 28 aneg), not forced speeds.
                _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_CU_SFP_SPEED_AUTO;
            } else if (_conf.speed < _port_status.sfp_speed_min || _conf.speed > _port_status.sfp_speed_max) {
                // The SFP's speed is not within the limits of the configured,
                // forced speed. Raise operational warning (SFP will be turned
                // off in just a second, and on dual media interfaces, we will
                // use the PHY port instead.
                _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_FORCED_SPEED_NOT_SUPPORTED_BY_SFP;
            } else if (_is_100m_sfp && !_conf.fdx) {
                // The speed is forced and duplex is set to half, but the 100M
                // SFP does not support half duplex.
                _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_SFP_DOES_NOT_SUPPORT_HDX;
            } else {
                // This SFP *does* support the configured, forced speed.
                _sfp_speed = _conf.speed;
            }
        }
    } else {
        // Auto speed depends on port type and also the SFP type if using an
        // SFP.
        // We use _conf.speed == MESA_SPEED_AUTO if the user has forced clause
        // 73 aneg (KR), so we also get into this piece of the code in that
        // case, but we cannot have _conf.force_clause_73 == true if it's not a
        // 10G or 25G SFP-only port, so no need to check for that in the next
        // couple of ifs().
        if ((_port_status.static_caps & MEBA_PORT_CAP_COPPER) ||
            ((_port_status.static_caps & MEBA_PORT_CAP_10G_FDX) == 0 &&
             (_port_status.static_caps & MEBA_PORT_CAP_25G_FDX) == 0)) {
            // If it's a copper port or it's not a 10G and not a 25G port, we
            // run either clause 28 or clause 37 aneg.
            // A 10G Cu PHY could be Aquantia, which is handled here.

            // Assume we have an SFP. Later on, we will adjust this to match the
            // currently selected media mode, because we can't set it here if
            // the SFP for some reason renders unusable on dual media ports.
            _port_status.aneg_method = VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_37;

            if (_phy_dev) {
                // MESA_SPEED_AUTO
                _phy_speed = _conf.speed;
            }

            if (use_sfp_dev) {
                if (_is_100m_sfp) {
                    // This SFP does not support aneg (100Mbps SFP).
                    if (prefer_fiber) {
                        // Only change to no aneg if we're not running Cu on
                        // dual media ports.
                        _port_status.aneg_method = VTSS_APPL_PORT_ANEG_METHOD_NONE;
                    }

                    _sfp_speed = _port_status.sfp_speed_max;
                } else {
                    // MESA_SPEED_AUTO
                    _sfp_speed = _conf.speed;
                }
            }
        } else if (PORT_IS_DUAL_MEDIA() || _phy_dev) {
            // If we have 10G or 25G dual media ports, I think we should update
            // this code to also support PHYs.
            T_E_PORT(_port_no, "Code does not support 10G or 25G dual media ports.");
        } else {
            // We know that this is a 10G or 25G SFP-only port by now.
            // If we have an SFP device inserted, figure out what to do with it.
            if (use_sfp_dev) {
                // Assume we don't use any kind of aneg.
                new_aneg_method = VTSS_APPL_PORT_ANEG_METHOD_NONE;

                if (_port_status.has_kr) {
                    if (_sfp_must_run_kr) {
                        new_aneg_method = VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_73;
                    } else if (_conf.force_clause_73) {
                        if (_sfp_may_run_kr) {
                            // The SFP is capable of running KR. Then do it if
                            // we are asked to.
                            new_aneg_method = VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_73;
                        } else {
                            // Operational warning: SFP can't run KR, but user
                            // has asked us to.
                            new_aneg_method             = VTSS_APPL_PORT_ANEG_METHOD_NONE;
                            _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_SFP_CANNOT_RUN_CLAUSE_73;
                        }
                    }
                }

                if (_port_status.static_caps & MEBA_PORT_CAP_25G_FDX) {
                    // It's a 25G port. If it's a 10G or 25G SFP, use that speed
                    if (_port_status.sfp_speed_max >= MESA_SPEED_10G) {
                        _sfp_speed = _port_status.sfp_speed_max;
                    } else if (_port_status.sfp_caps & MEBA_PORT_CAP_AUTONEG) {
                        // It's an SFP that supports aneg, but less than 10G.
                        // Run clause 37 aneg
                        new_aneg_method = VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_37;
                        _sfp_speed      = _conf.speed; // MESA_SPEED_AUTO
                    } else {
                        // The SFP does not support aneg (it's a 100 Mbps SFP).
                        _sfp_speed = _port_status.sfp_speed_max;
                    }
                } else {
                    // It's a 10G port. If it's a 10G or 25G SFP, use 10G as the
                    // speed.
                    if (_port_status.sfp_speed_max >= MESA_SPEED_10G) {
                        _sfp_speed = MESA_SPEED_10G;
                    } else if (_is_100m_sfp) {
                        // This 100M SFP does not support aneg.
                        _sfp_speed = _port_status.sfp_speed_max;
                    } else {
                        // It's an SFP that supports aneg and less than 10G
                        // Run clause 37 aneg.
                        new_aneg_method = VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_37;
                        _sfp_speed      = _conf.speed;
                    }
                }

                if (prefer_fiber) {
                    // I haven't heard of 10G or 25G dual media ports, so
                    // currently, prefer_fiber will always be true.
                    _port_status.aneg_method = new_aneg_method;
                }
            } else {
                // We don't know yet whether we run clause 28/37/73 or none at
                // all. Leave aneg_method at VTSS_APPL_PORT_ANEG_METHOD_UNKNOWN.
            }
        }
    }
}

/******************************************************************************/
// port_instance::update_1g_aneg_type()
/******************************************************************************/
void port_instance::update_1g_aneg_type(bool prefer_fiber)
{
    // This function can only change _port_status.aneg_method from being clause
    // 28 to being clause 37 or vice versa. It is dynamic, because it may change
    // on dual media ports whenever an SFP is inserted or pulled out, and also
    // because it may change depending on whether it's a CuSFP (clause 37) or an
    // optical SFP (clause 28).
    // Changing to one or the other does not affect the remaining functionality,
    // because running_1g_aneg() will return true if either is set.
    if (!running_1g_aneg()) {
        return;
    }

    if (prefer_fiber) {
        // If using a CuSFP, we run PHY/twisted pair aneg (clause 28) on the
        // line (and SGMII_CISCO aneg on the serdes I/F between PCS and PHY)
        _port_status.aneg_method = _port_status.sfp_type == VTSS_APPL_PORT_SFP_TYPE_CU ? VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_28 : VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_37;
    } else {
        // This is a PHY, we prefer, so use clause 28
        _port_status.aneg_method = VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_28;
    }
}

/******************************************************************************/
// caps_update()
// Must be called if either of these changes:
//   _conf.speed
//   _conf.force_clause_73
//   _conf.media_type
//   _sfp_dev
//   _phy_dev
//
// Private
/******************************************************************************/
bool port_instance::caps_update(void)
{
    bool prefer_fiber = prefer_fiber_get(false);

    // Update
    //  - _port_status.sfp_caps
    //  - _port_status.sfp_type
    //  - _port_status.sfp_speed_min
    //  - _port_status.sfp_speed_max
    //  - _sfp_must_run_kr
    //  - _sfp_may_run_kr
    sfp_caps_update();

    // With the SFP capabilities in place, update the speed to apply to
    // phy_conf_set() and sfp_conf_set():
    // If _sfp_speed or _phy_speed is MESA_SPEED_AUTO, we apply 1G to
    // port_conf_set().
    // The function updates
    //  - _sfp_speed
    //  - _phy_speed
    //  - _port_status.aneg_method
    speed_update(prefer_fiber);

    // This may change _port_status.aneg_type from one 1G aneg type to another
    // (e.g. clause 28 to 37 or 37 to 28).
    update_1g_aneg_type(prefer_fiber);

    // This may add a flag indicating that the SFP may not work because the
    // actual speed is lower than the SFP's nominal (maximum) speed.
    sfp_may_not_work_oper_warning_update();

    T_I_PORT(_port_no, "prefer_fiber = %d, phy_speed = %s, sfp_speed = %s (%s-%s), aneg-method = %s, must_run_kr = %d, may_run_kr = %d",
             prefer_fiber, port_speed_to_txt(_phy_speed), port_speed_to_txt(_sfp_speed), port_speed_to_txt(_port_status.sfp_speed_min), port_speed_to_txt(_port_status.sfp_speed_max),
             port_aneg_method_to_txt(_port_status.aneg_method), _sfp_must_run_kr, _sfp_may_run_kr);

    return prefer_fiber;
}

/******************************************************************************/
// port_instance::media_type_check()
// Private
/******************************************************************************/
mesa_rc port_instance::media_type_check(const vtss_appl_port_conf_t &new_conf)
{
    switch (new_conf.media_type) {
    case VTSS_APPL_PORT_MEDIA_CU:
        if (_port_status.static_caps & MEBA_PORT_CAP_SFP_ONLY) {
            return VTSS_APPL_PORT_RC_MEDIA_TYPE_CU;
        }

        break;

    case VTSS_APPL_PORT_MEDIA_SFP:
        if (PORT_IS_DUAL_MEDIA()) {
            // This is OK.
            break;
        }

        if (_port_status.static_caps & MEBA_PORT_CAP_SFP_ONLY) {
            // This is also OK.
            break;
        }

        // Not an SFP port.
        return VTSS_APPL_PORT_RC_MEDIA_TYPE_SFP;

    case VTSS_APPL_PORT_MEDIA_DUAL:
        if (!PORT_IS_DUAL_MEDIA()) {
            // Port is not a dual media port.
            return VTSS_APPL_PORT_RC_MEDIA_TYPE_DUAL;
        }

        break;

    default:
        T_IG(PORT_TRACE_GRP_CONF, "%u: Unknown media type (%d)", _port_no, new_conf.media_type);
        return VTSS_APPL_PORT_RC_MEDIA_TYPE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// port_instance::speed_duplex_check()
// Private
/******************************************************************************/
mesa_rc port_instance::speed_duplex_check(const vtss_appl_port_conf_t &new_conf)
{
    bool     hdx_dis, fdx_dis;
    uint32_t port_adv_dis;

    // General check on forced modes
    if (new_conf.speed != MESA_SPEED_AUTO && (_port_status.static_caps & MEBA_PORT_CAP_NO_FORCE)) {
        T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "Port does not support forced mode (speed = %u)", new_conf.speed);
        return VTSS_APPL_PORT_RC_SPEED_NO_FORCE;
    }

    // General check on duplex for forced speeds (even though speed is
    // configured as auto, we cannot duplex to be set as half duplex on ports
    // that don't support it).
    if (!new_conf.fdx && (_port_status.static_caps & MEBA_PORT_CAP_HDX) == 0) {
        return VTSS_APPL_PORT_RC_DUPLEX_HALF;
    }

    switch (new_conf.speed) {
    case MESA_SPEED_10M:
        if (new_conf.fdx) {
            if ((_port_status.static_caps & MEBA_PORT_CAP_10M_FDX) == 0) {
                return VTSS_APPL_PORT_RC_SPEED_10M_FDX;
            }
        } else {
            if ((_port_status.static_caps & MEBA_PORT_CAP_10M_HDX) == 0) {
                return VTSS_APPL_PORT_RC_SPEED_10M_HDX;
            }
        }

        break;

    case MESA_SPEED_100M:
        if (new_conf.fdx) {
            if ((_port_status.static_caps & MEBA_PORT_CAP_100M_FDX) == 0) {
                return VTSS_APPL_PORT_RC_SPEED_100M_FDX;
            }
        } else {
            if ((_port_status.static_caps & MEBA_PORT_CAP_100M_HDX) == 0) {
                return VTSS_APPL_PORT_RC_SPEED_100M_HDX;
            }
        }

        break;

    case MESA_SPEED_1G:
        if ((_port_status.static_caps & MEBA_PORT_CAP_1G_FDX) == 0) {
            return VTSS_APPL_PORT_RC_SPEED_1G;
        } else if (!new_conf.fdx) {
            return VTSS_APPL_PORT_RC_SPEED_1G_HDX;
        }

        break;

    case MESA_SPEED_2500M:
        if ((_port_status.static_caps & MEBA_PORT_CAP_2_5G_FDX) == 0) {
            return VTSS_APPL_PORT_RC_SPEED_2G5;
        } else if (!new_conf.fdx) {
            return VTSS_APPL_PORT_RC_SPEED_2G5_HDX;
        }

        break;

    case MESA_SPEED_5G:
        if ((_port_status.static_caps & MEBA_PORT_CAP_5G_FDX) == 0) {
            return VTSS_APPL_PORT_RC_SPEED_5G;
        } else if (!new_conf.fdx) {
            return VTSS_APPL_PORT_RC_SPEED_5G_HDX;
        }

        break;

    case MESA_SPEED_10G:
        if ((_port_status.static_caps & MEBA_PORT_CAP_10G_FDX) == 0) {
            return VTSS_APPL_PORT_RC_SPEED_10G;
        } else if (!new_conf.fdx) {
            return VTSS_APPL_PORT_RC_SPEED_10G_HDX;
        }

        break;

    case MESA_SPEED_25G:
        if ((_port_status.static_caps & MEBA_PORT_CAP_25G_FDX) == 0) {
            return VTSS_APPL_PORT_RC_SPEED_25G;
        } else if (!new_conf.fdx) {
            return VTSS_APPL_PORT_RC_SPEED_25G_HDX;
        }

        break;

    case MESA_SPEED_AUTO:
        if ((_port_status.static_caps & MEBA_PORT_CAP_AUTONEG) == 0 && !new_conf.force_clause_73) {
            // Port is not aneg capable. If forcing clause 73, we use
            // MESA_SPEED_AUTO because there is no MESA_SPEED_CLAUSE_73.
            return VTSS_APPL_PORT_RC_SPEED_NO_AUTO;
        }

        // Time to check what not to advertise.

        // Check if bits other than valid adv_dis bits are set.
        // Do not use MEPA_ADV_DIS_ALL, because it covers a loop bit, that we
        // also don't support.
        if (new_conf.adv_dis != (new_conf.adv_dis & (MEPA_ADV_DIS_SPEED | MEPA_ADV_DIS_DUPLEX))) {
            T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "adv_dis bitmask (0x%x) has bits set outside of valid bitmask (0x%x)", new_conf.adv_dis, MEPA_ADV_DIS_SPEED | MEPA_ADV_DIS_DUPLEX);
            return VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_MASK;
        }

        // Get the port's disabled advertisements. These are what the port
        // can never support (e.g. 2.5G advertisements on a 1G port).
        port_adv_dis = port_cap_to_advertise_dis(_port_status.static_caps);

        // Check the advertisements. We only support change of the port's
        // default advertisements on a copper port, because we don't have the
        // hooks and handles to change SFP ports (including dual media ports to
        // avoid inter-configuration checks with the current media-type).
        if (_port_status.static_caps & MEBA_PORT_CAP_COPPER) {
            // If the port requires a certain aneg advertisement feature (e.g.
            // HDX) to be disabled, because the port doesn't support HDX, but
            // the user has enabled it, it's an error.
#define ADV_ENA_CHK(X)                                                                     \
    if ((port_adv_dis & MEPA_ADV_DIS_##X) && (new_conf.adv_dis & MEPA_ADV_DIS_##X) == 0) { \
        return VTSS_APPL_PORT_RC_ADVERTISE_ENABLE_##X;                                     \
    }
            ADV_ENA_CHK(HDX);
            ADV_ENA_CHK(10M);
            ADV_ENA_CHK(100M);
            ADV_ENA_CHK(1G);
            ADV_ENA_CHK(2500M);
            ADV_ENA_CHK(5G);
            ADV_ENA_CHK(10G);

            // More checks on duplex. Both half and full cannot be disabled at
            // the same time.
            hdx_dis = (new_conf.adv_dis & MEPA_ADV_DIS_HDX) != 0;
            fdx_dis = (new_conf.adv_dis & MEPA_ADV_DIS_FDX) != 0;

            if (fdx_dis && hdx_dis) {
                return VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_HDX_AND_FDX;
            }
#undef ADV_ENA_CHK
        } else {
            // On SFP ports and dual media ports, if the port supports a certain
            // feature, say HDX, and therefore can advertise HDX, the user must
            // not disable advertisement of HDX, because we don't have the hooks
            // and handles to change what it advertises.
            // Basically, this is the opposite check of when the port is a
            // copper port.
#define ADV_DIS_CHK(X)                                                                     \
    if ((port_adv_dis & MEPA_ADV_DIS_##X) == 0 && (new_conf.adv_dis & MEPA_ADV_DIS_##X)) { \
        return VTSS_APPL_PORT_RC_ADVERTISE_DISABLE_##X;                                    \
    }
            ADV_DIS_CHK(HDX);
            ADV_DIS_CHK(FDX);
            ADV_DIS_CHK(10M);
            ADV_DIS_CHK(100M);
            ADV_DIS_CHK(1G);
            ADV_DIS_CHK(2500M);
            ADV_DIS_CHK(5G);
            ADV_DIS_CHK(10G);
#undef ADV_DIS_CHK
        }

        break;

    default:
        T_IG(PORT_TRACE_GRP_CONF, "Unknown speed (%d)", new_conf.speed);
        return VTSS_APPL_PORT_RC_SPEED;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// port_instance::speed_on_dual_media_check()
// Private
/******************************************************************************/
mesa_rc port_instance::speed_on_dual_media_check(const vtss_appl_port_conf_t &new_conf)
{
    if (!PORT_IS_DUAL_MEDIA()) {
        return VTSS_RC_OK;
    }

    switch (new_conf.media_type) {
    case VTSS_APPL_PORT_MEDIA_CU:
        // User has force-selected copper interface.
        // All speeds are OK.
        break;

    case VTSS_APPL_PORT_MEDIA_SFP:
        // User has force-selected SFP interface.
        // Only speeds above 10 Mbps are OK.
        if (new_conf.speed == MESA_SPEED_10M) {
            return VTSS_APPL_PORT_RC_SPEED_DUAL_MEDIA_SFP;
        }

        break;

    case VTSS_APPL_PORT_MEDIA_DUAL:
        // User has selected to auto-select whether to use copper or SFP
        // interface.
        // In this mode, speed must be set to full aneg, that is, speed == Auto,
        // duplex == full, and advertise everything.
        if (new_conf.speed != MESA_SPEED_AUTO || new_conf.adv_dis != port_cap_to_advertise_dis(_port_status.static_caps)) {
            return VTSS_APPL_PORT_RC_SPEED_DUAL_MEDIA_AUTO;
        }

        break;

    default:
        T_IG(PORT_TRACE_GRP_CONF, "%u: Unknown media_type (%d)", _port_no, new_conf.media_type);
        return VTSS_APPL_PORT_RC_MEDIA_TYPE;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// kr_check()
// Private
/******************************************************************************/
mesa_rc port_instance::kr_check(const vtss_appl_port_conf_t &new_conf)
{
    if (new_conf.force_clause_73 || !new_conf.clause_73_pd) {
        if (!PORT_cap.has_kr) {
            return VTSS_APPL_PORT_RC_KR_NOT_SUPPORTED_ON_THIS_PLATFORM;
        }

        if (!_port_status.has_kr) {
            return VTSS_APPL_PORT_RC_KR_NOT_SUPPORTED_ON_THIS_INTERFACE;
        }

        if (new_conf.force_clause_73) {
            if (new_conf.speed != MESA_SPEED_AUTO) {
                return VTSS_APPL_PORT_RC_KR_FORCED_REQUIRES_SPEED_AUTO;
            }
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// fec_check()
// Private
/******************************************************************************/
mesa_rc port_instance::fec_check(const vtss_appl_port_conf_t &new_conf)
{
    if (new_conf.fec_mode == VTSS_APPL_PORT_FEC_MODE_AUTO ||
        new_conf.fec_mode == VTSS_APPL_PORT_FEC_MODE_NONE) {
        return VTSS_RC_OK;
    }

    if (new_conf.fec_mode != VTSS_APPL_PORT_FEC_MODE_R_FEC &&
        new_conf.fec_mode != VTSS_APPL_PORT_FEC_MODE_RS_FEC) {
        return VTSS_APPL_PORT_RC_FEC_ILLEGAL;
    }

    if (!PORT_cap.has_kr) {
        if (new_conf.fec_mode == VTSS_APPL_PORT_FEC_MODE_R_FEC) {
            return VTSS_APPL_PORT_RC_FEC_R_NOT_SUPPORTED_ON_THIS_PLATFORM;
        } else {
            return VTSS_APPL_PORT_RC_FEC_RS_NOT_SUPPORTED_ON_THIS_PLATFORM;
        }
    }

    if (!_port_status.has_kr) {
        if (new_conf.fec_mode == VTSS_APPL_PORT_FEC_MODE_R_FEC) {
            return VTSS_APPL_PORT_RC_FEC_R_NOT_SUPPORTED_ON_THIS_INTERFACE;
        } else {
            return VTSS_APPL_PORT_RC_FEC_RS_NOT_SUPPORTED_ON_THIS_INTERFACE;
        }
    }

    if (new_conf.fec_mode == VTSS_APPL_PORT_FEC_MODE_RS_FEC) {
        if (!PORT_cap.has_kr_v3 || (_port_status.static_caps & MEBA_PORT_CAP_25G_FDX) == 0) {
            return VTSS_APPL_PORT_RC_FEC_RS_NOT_SUPPORTED_ON_THIS_INTERFACE;
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// port_instance::pfc_enabled()
// Returns true if at least one priority has FC enabled.
/******************************************************************************/
bool port_instance::pfc_enabled(const vtss_appl_port_conf_t &conf)
{
    mesa_prio_t prio;

    for (prio = 0; prio < ARRSZ(conf.pfc); prio++) {
        if (conf.pfc[prio]) {
            return true;
        }
    }

    return false;
}

/******************************************************************************/
// conf_check()
// Private
/******************************************************************************/
mesa_rc port_instance::conf_check(const vtss_appl_port_conf_t &new_conf)
{
    // Check selected media type against port's capabilities.
    VTSS_RC(media_type_check(new_conf));

    // Check speed and duplex against port's capabilities.
    VTSS_RC(speed_duplex_check(new_conf));

    // Check speed and media type on dual media ports.
    VTSS_RC(speed_on_dual_media_check(new_conf));

    // Check forced KR
    VTSS_RC(kr_check(new_conf));

    // Check FEC mode
    VTSS_RC(fec_check(new_conf));

    // Check MTU
    if (new_conf.max_length < MESA_MAX_FRAME_LENGTH_STANDARD || new_conf.max_length > fast_cap(MESA_CAP_PORT_FRAME_LENGTH_MAX)) {
        return VTSS_APPL_PORT_RC_MTU;
    }

    // Check Flow Control
    if (new_conf.flow_control && !(_port_status.static_caps & MEBA_PORT_CAP_FLOW_CTRL)) {
        return VTSS_APPL_PORT_RC_FLOWCONTROL;
    }

    // Check per-priority flow control
    if (pfc_enabled(new_conf)) {
        if (!PORT_cap.has_pfc) {
            // Not supported on this chip
            return VTSS_APPL_PORT_RC_FLOWCONTROL_PFC;
        }

        if (new_conf.flow_control) {
            // Priority flow control and standard flow control cannot be enabled
            // simultaneously
            return VTSS_APPL_PORT_RC_FLOWCONTROL_WHILE_PFC;
        }
    }

    // Check excessive collision
    if (new_conf.exc_col_cont && !(_port_status.static_caps & MEBA_PORT_CAP_HDX)) {
        return VTSS_APPL_PORT_RC_EXCESSIVE_COLLISSION;
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// port_instance::kr_fec_update()
/******************************************************************************/
void port_instance::kr_fec_update(mesa_port_kr_fec_t &new_fec_conf)
{
#if defined(VTSS_SW_OPTION_KR)
    mesa_rc rc;

    if (memcmp(&new_fec_conf, &_fec_conf, sizeof(new_fec_conf)) == 0) {
        // No changes
        T_IG_PORT(PORT_TRACE_GRP_KR, _port_no, "No changes (%s)", new_fec_conf);
        return;
    }

    T_IG_PORT(PORT_TRACE_GRP_KR, _port_no, "mesa_port_kr_fec_set(%s)", new_fec_conf);
    if ((rc = mesa_port_kr_fec_set(nullptr, _port_no, &new_fec_conf)) != VTSS_RC_OK) {
        T_EG_PORT(PORT_TRACE_GRP_KR, _port_no, "mesa_port_kr_fec_set(%s) failed: %s", new_fec_conf, error_txt(rc));
    } else {
        _fec_conf = new_fec_conf;
    }
#endif
}

/******************************************************************************/
// port_instance::kr_about_to_be_disabled()
//
// Returns true if we are about to disable KR.
/******************************************************************************/
bool port_instance::kr_about_to_be_disabled(void)
{
    bool enabling;

    if (!_port_status.has_kr) {
        // This port doesn't support KR, so nothing to do.
        return false;
    }

    if (_kr_initialized) {
        // KR is already initialized. Nothing else to do.
        return false;
    }

    enabling = _port_status.aneg_method == VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_73 && admin_enabled(_conf, _vol_conf);

    if (_kr_conf.aneg.enable && !enabling) {
        return true;
    }

    return false;
}

/******************************************************************************/
// port_instance::kr_conf_update()
//
// The KR configuration can only change if _conf changes or if an SFP is plugged
// or unplugged, in which case _kr_initialized gets cleared and we get invoked.
/******************************************************************************/
void port_instance::kr_conf_update(void)
{
#if defined(VTSS_SW_OPTION_KR)
    mesa_port_kr_conf_t new_kr_conf;
    mesa_port_kr_fec_t  new_fec_conf;
    mesa_rc             rc;

    if (!_port_status.has_kr) {
        // This port doesn't support KR, so nothing to do.
        return;
    }

    if (_kr_initialized) {
        // KR is already initialized. Nothing else to do.
        return;
    }

    vtss_clear(new_kr_conf);
    new_kr_conf.aneg.enable = _port_status.aneg_method == VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_73 && admin_enabled(_conf, _vol_conf);

    if (new_kr_conf.aneg.enable) {
        // We advertise everything that both the port and the SFP supports.
        // Therefore, we use sfp_caps, which contains exactly that.
        new_kr_conf.aneg.adv_25g = (_port_status.sfp_caps & MEBA_PORT_CAP_25G_FDX)  != 0 && PORT_cap.has_kr_v3;
        new_kr_conf.aneg.adv_10g = (_port_status.sfp_caps & MEBA_PORT_CAP_10G_FDX)  != 0;
        new_kr_conf.aneg.adv_5g  = (_port_status.sfp_caps & MEBA_PORT_CAP_5G_FDX)   != 0 && PORT_cap.has_kr_v3;
        new_kr_conf.aneg.adv_2g5 = (_port_status.sfp_caps & MEBA_PORT_CAP_2_5G_FDX) != 0 && PORT_cap.has_kr_v3;
        new_kr_conf.aneg.adv_1g  = (_port_status.sfp_caps & MEBA_PORT_CAP_1G_FDX)   != 0 && PORT_cap.has_kr_v3;

        // Parallel-detect?
        new_kr_conf.aneg.no_pd = !_conf.clause_73_pd;

        switch (_conf.fec_mode) {
        case VTSS_APPL_PORT_FEC_MODE_AUTO:
            // We only request one of RS-FEC and R-FEC.
            // If we advertise 25G, we request RS-FEC, otherwise we request
            // R-FEC.
            if (new_kr_conf.aneg.adv_25g) {
                new_kr_conf.aneg.rs_fec_req = true;
            } else {
                new_kr_conf.aneg.r_fec_req = true;
            }

            break;

        case VTSS_APPL_PORT_FEC_MODE_R_FEC:
            new_kr_conf.aneg.r_fec_req = true;
            break;

        case VTSS_APPL_PORT_FEC_MODE_RS_FEC:
            if (new_kr_conf.aneg.adv_25g) {
                // Only request RS-FEC if we're also advertising 25G.
                new_kr_conf.aneg.rs_fec_req = true;
            }

            break;

        default:
            break;
        }

        // Don't touch aneg.next_page, since I don't know what it's used for.
        new_kr_conf.train.enable = true;
    }

    if (memcmp(&new_kr_conf, &_kr_conf, sizeof(new_kr_conf)) == 0) {
        // No changes.
        T_IG_PORT(PORT_TRACE_GRP_KR, _port_no, "No changes (%s)", new_kr_conf);
        goto check_fec;
    }

    if (new_kr_conf.aneg.enable && !_kr_conf.aneg.enable) {
        // Before enabling KR, make sure than any forced FEC is disabled.
        vtss_clear(new_fec_conf);
        kr_fec_update(new_fec_conf);

        // Also make sure not to link-flap during clause 73 aneg/training.
        _kr_complete = false;
    }

    T_IG_PORT(PORT_TRACE_GRP_KR, _port_no, "kr_mgmt_port_conf_set(%s)", new_kr_conf);
    if ((rc = kr_mgmt_port_conf_set(_port_no, &new_kr_conf)) != VTSS_RC_OK) {
        // Trace errors are already thrown if needed.
        T_EG_PORT(PORT_TRACE_GRP_KR, _port_no, "kr_mgmt_port_conf_set(%s) failed: %s", new_kr_conf, error_txt(rc));
    } else {
        _kr_conf = new_kr_conf;
    }

check_fec:
    _kr_initialized = true;

    if (_port_status.aneg_method == VTSS_APPL_PORT_ANEG_METHOD_CLAUSE_73) {
        // We are running clause 73 aneg. Nothing to be done for forced FEC.
        // When we enabled clause 73 aneg above, we also cleared any forced FEC.
        // FEC configuration will be updated by the Clause 73 aneg.
        return;
    }

    // Time to figure out whether we want to force-enable R-FEC or RS-FEC.
    // Assume we don't enable any.
    vtss_clear(new_fec_conf);

    // First of all, the current MAC I/F must be SFI for any FEC to work, and
    // FEC mode must be non-NONE in order for one of the FEC modes to be set.
    if (_port_conf.if_type == MESA_PORT_INTERFACE_SFI) {
        switch (_conf.fec_mode) {
        case VTSS_APPL_PORT_FEC_MODE_AUTO:
            // We only enable one of RS-FEC and R-FEC.
            if (_port_conf.speed == MESA_SPEED_25G) {
                // If the current speed is 25G, then we have a 25G port with
                // either a 25G SFP or no SFP is inserted at all. No matter
                // which SFP it is (CuBP, DAC, Optical), we must enable RS-FEC.
                new_fec_conf.rs_fec = true;
            } else if (_port_conf.speed == MESA_SPEED_5G || _port_conf.speed == MESA_SPEED_10G) {
                // We are running 5G forced or 10G forced on a 25G port or with
                // a 10G SFP on a 25G port or 10G forced or auto on a 10G port
                // or we have a 10G port without an SFP inserted.
                // In some of these cases, we must enable R-FEC, namely when it
                // should have run clause 73, but doesn't because the speed is
                // forced.
                if (_sfp_must_run_kr) {
                    // _sfp_must_run_kr is set if we have a 10G Cu backplane on
                    // a 10G port or 25G Cu backplane on a 25G port or
                    // 25GBASE-CR(-S) SFP on a 25G port, only!
                    new_fec_conf.r_fec = true;
                }
            }

            break;

        case VTSS_APPL_PORT_FEC_MODE_RS_FEC:
            // The user is only allowed to enable RS-FEC on 25G ports and only
            // if they run in 25G mode.
            if (_port_conf.speed == MESA_SPEED_25G) {
                new_fec_conf.rs_fec = true;
            }

            break;

        case VTSS_APPL_PORT_FEC_MODE_R_FEC:
            // The user can enable R-FEC on both 5G, 10G and 25G ports.
            if (_port_conf.speed == MESA_SPEED_5G || _port_conf.speed == MESA_SPEED_10G || _port_conf.speed == MESA_SPEED_25G) {
                new_fec_conf.r_fec = true;
            }

            break;

        default:
            break;
        }
    }

    kr_fec_update(new_fec_conf);
#endif
}

/******************************************************************************/
// port_instance::conf_update()
/******************************************************************************/
void port_instance::conf_update(const vtss_appl_port_conf_t &new_conf, const port_vol_conf_t &new_vol_conf)
{
    bool old_admin_enable      = admin_enabled(_conf, _vol_conf), new_admin_enable;
    bool admin_enable_changed  = false;
    bool speed_changed         = false;
    bool invoke_phy_conf_set   = false, reset_phy = false;
    bool invoke_sfp_conf_set   = false;

    T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "Old conf: %s", _conf);
    T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "New conf: %s", new_conf);

    new_admin_enable = admin_enabled(new_conf, new_vol_conf);

    if (old_admin_enable != new_admin_enable) {
        admin_enable_changed = true;

        // Shutting down (either administrative or volatile user).
        if (!new_admin_enable && _port_status.link) {
            // Call shutdown listeners right away
            T_DG_PORT(PORT_TRACE_GRP_CONF, _port_no, "port_listener_shutdown_notify()");
            port_listener_shutdown_notify(_port_no);

            // Let the poll() function get to know about this.
            _notify_shutdown = true;
        }
    }

    if (new_conf.speed != _conf.speed) {
        speed_changed = true;
    }

    if (speed_changed                                       ||
        admin_enable_changed                                ||
        new_conf.force_clause_73  != _conf.force_clause_73  ||
        new_conf.fec_mode         != _conf.fec_mode         ||
        new_conf.media_type       != _conf.media_type       ||
        new_conf.max_length       != _conf.max_length       ||
        new_conf.frame_length_chk != _conf.frame_length_chk ||
        new_conf.fdx              != _conf.fdx              ||
        new_conf.exc_col_cont     != _conf.exc_col_cont     ||
        new_conf.flow_control     != _conf.flow_control     ||
        (memcmp(new_conf.pfc, _conf.pfc, sizeof(new_conf.pfc)) != 0)) {

        // The next invocation of poll() should consult the port state and and
        // the port and KR configuration.
        _port_initialized = false;
        _kr_initialized = false;
    }

    if (speed_changed                               ||
        admin_enable_changed                        ||
        new_conf.flow_control != _conf.flow_control ||
        new_conf.adv_dis      != _conf.adv_dis      ||
        new_conf.fdx          != _conf.fdx) {
        invoke_phy_conf_set = true;
    }

    if (speed_changed                                                                      ||
        admin_enable_changed                                                               ||
        (new_conf.speed == MESA_SPEED_AUTO && new_conf.flow_control != _conf.flow_control) ||
        (new_conf.speed == MESA_SPEED_AUTO && new_conf.adv_dis      != _conf.adv_dis)      ||
        (new_conf.speed != MESA_SPEED_AUTO && new_conf.fdx          != _conf.fdx)) {
        // Don't reset the PHY unnecessarily
        reset_phy = true;
    }

    if (speed_changed                               ||
        admin_enable_changed                        ||
        new_conf.flow_control != _conf.flow_control) {
        invoke_sfp_conf_set = true;
    }

    // Now that we have what to invoke, save the new (volatile) configuration.
    _conf     = new_conf;
    _vol_conf = new_vol_conf;

    // This might have effected various properties.
    (void)caps_update();

    T_DG_PORT(PORT_TRACE_GRP_CONF, _port_no,
              "_port_initialized = %d, "
              "invoke_phy_conf_set = %d, "
              "invoke_sfp_conf_set = %d, "
              "_kr_initialized = %d",
              _port_initialized,
              invoke_phy_conf_set,
              invoke_sfp_conf_set,
              _kr_initialized);

    if (invoke_phy_conf_set && _phy_dev) {
        lu26_oos_fix(__FUNCTION__, __LINE__, false, MESA_SPEED_UNDEFINED, false);

        if (reset_phy) {
            (void)phy_reset();
        }

        // If we are shutting down, we need to set the port state to down before
        // we turn off the PHY, because otherwise we cannot pause/cancel AFI
        // frames in mesa_port_state_set(), because the switch core cannot get
        // rid of frames when the PHY is turned off.
        if (!new_admin_enable && _port_status.link) {
            port_state_set(__FUNCTION__, __LINE__, false);
        }

        // The following call doesn't necessarily turn on the PHY. It merely
        // changes its configuration.
        turn_on_phy(__FUNCTION__, __LINE__, true);
    }

    if (invoke_sfp_conf_set && _sfp_dev) {
        // Gotta turn off and on the SFP, because we need it to possibly restart
        // aneg. Here, we only turn it off. The poll() function will turn it
        // back on if needed.
        turn_off_sfp(__FUNCTION__, __LINE__);
    }

    // Tell listeners that the port configuration has changed
    port_listener_conf_change_notify(_port_no, _conf);
}

/******************************************************************************/
// port_instance::conf_default_get()
// Public
/******************************************************************************/
void port_instance::conf_default_get(vtss_appl_port_conf_t &conf)
{
    vtss_clear(conf);

    conf.admin.enable = true;
    conf.fdx = true;
    if (_port_status.static_caps & (MEBA_PORT_CAP_AUTONEG | MEBA_PORT_CAP_NO_FORCE)) {
        conf.speed = MESA_SPEED_AUTO;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_25G_FDX) {
        conf.speed = MESA_SPEED_25G;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_10G_FDX) {
        conf.speed = MESA_SPEED_10G;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_5G_FDX) {
        conf.speed = MESA_SPEED_5G;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_2_5G_FDX) {
        conf.speed = MESA_SPEED_2500M;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_1G_FDX) {
        conf.speed = MESA_SPEED_1G;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_100M_FDX) {
        conf.speed = MESA_SPEED_100M;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_100M_HDX) {
        conf.speed = MESA_SPEED_100M;
        conf.fdx = false;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_10M_FDX) {
        conf.speed = MESA_SPEED_10M;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_10M_HDX) {
        conf.speed = MESA_SPEED_10M;
        conf.fdx = false;
    } else {
        // Unknown speed capability
        T_E("%u: Unknown speed capability. Capabilities = 0x%x", _port_no, _port_status.static_caps);
        conf.speed = MESA_SPEED_1G;
        conf.fdx   = true;
    }

    if (PORT_IS_DUAL_MEDIA()) {
        conf.media_type = VTSS_APPL_PORT_MEDIA_DUAL;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_SFP_ONLY) {
        conf.media_type = VTSS_APPL_PORT_MEDIA_SFP;
    } else {
        conf.media_type = VTSS_APPL_PORT_MEDIA_CU;
    }

    conf.flow_control     = false;
    conf.max_length       = fast_cap(MESA_CAP_PORT_FRAME_LENGTH_MAX);
    conf.power_mode       = VTSS_PHY_POWER_NOMINAL;
    conf.exc_col_cont     = false;
    conf.frame_length_chk = false;
    conf.adv_dis          = port_cap_to_advertise_dis(_port_status.static_caps);
    conf.fec_mode         = VTSS_APPL_PORT_FEC_MODE_AUTO;
    conf.clause_73_pd     = true; // Default to running parallel-detect when running KR.

    // Update for Up-injection loop port
    if (_port_no == fast_cap(VTSS_APPL_CAP_LOOP_PORT_UP_INJ)) {
        _port_status.loop_port = true;
        conf.max_length       += MESA_PACKET_HDR_SIZE_BYTES + 4;
    }

#if defined(VTSS_SW_OPTION_MIRROR_LOOP_PORT)
    // Update for mirror loop port
    if (_port_no == VTSS_SW_OPTION_MIRROR_LOOP_PORT) {
        _port_status.loop_port = true;
    }
#endif /* VTSS_SW_OPTION_MIRROR_LOOP_PORT */

    T_DG_PORT(PORT_TRACE_GRP_CONF, _port_no, "conf = %s", conf);
}

/******************************************************************************/
// port_instance::conf_get()
/******************************************************************************/
void port_instance::conf_get(vtss_appl_port_conf_t &conf)
{
    conf = _conf;
}

/******************************************************************************/
// port_instance::conf_set()
// Public
/******************************************************************************/
mesa_rc port_instance::conf_set(const vtss_appl_port_conf_t &new_conf)
{
    port_vol_conf_t new_vol_conf;

    T_DG_PORT(PORT_TRACE_GRP_CONF, _port_no, "conf = %s", new_conf);

    if (_port_status.loop_port) {
        // Ports compile-time configured as loop ports are not configurable.
        return VTSS_APPL_PORT_RC_LOOP_PORT;
    }

    // Check configuration
    VTSS_RC(conf_check(new_conf));

    // If there are no changes, we are done.
    if (memcmp(&new_conf, &_conf, sizeof(new_conf)) == 0) {
        // No changes.
        return VTSS_RC_OK;
    }

    // If a port is being shut down, update volatile port configuration.
    new_vol_conf = _vol_conf;
    if (_conf.admin.enable && !new_conf.admin.enable) {
        // This call will update new_vol_conf and we are expected to set it
        // after we have checked for changes.
        vol_conf_shutdown(new_vol_conf);
    }

    // If we get here, the basic configuration is good.
    conf_update(new_conf, new_vol_conf);

    return VTSS_RC_OK;
}

/******************************************************************************/
// port_instance::shutdown_now()
// This should be the last call into this module prior to a reboot, because it
// leaves the configuration in a limbo state.
/******************************************************************************/
void port_instance::shutdown_now(void)
{
    T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "Shutting down");

    // Use _vol_conf to force the port down.
    _vol_conf.oper_down = true;
    _vol_conf.disable   = true;

    port_state_set(__FUNCTION__, __LINE__, false);
    port_conf_set(__FUNCTION__, __LINE__);

    turn_off_phy(__FUNCTION__, __LINE__);
    turn_off_sfp(__FUNCTION__, __LINE__);

    _port_status.link = false;
    led_update();
}

/******************************************************************************/
// port_instance::port_speed_min_max_update()
// Private
/******************************************************************************/
void port_instance::port_speed_min_max_update(void)
{
    // First find the minimum supported speed on this port.
    if ((_port_status.static_caps & MEBA_PORT_CAP_10M_FDX) || (_port_status.static_caps & MEBA_PORT_CAP_10M_HDX)) {
        _port_speed_min = MESA_SPEED_10M;
    } else if ((_port_status.static_caps & MEBA_PORT_CAP_100M_FDX) || (_port_status.static_caps & MEBA_PORT_CAP_100M_HDX)) {
        _port_speed_min = MESA_SPEED_100M;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_1G_FDX) {
        _port_speed_min = MESA_SPEED_1G;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_2_5G_FDX) {
        _port_speed_min = MESA_SPEED_2500M;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_5G_FDX) {
        _port_speed_min = MESA_SPEED_5G;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_10G_FDX) {
        _port_speed_min = MESA_SPEED_10G;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_25G_FDX) {
        _port_speed_min = MESA_SPEED_25G;
    } else {
        T_EG_PORT(PORT_TRACE_GRP_CONF, _port_no, "No speeds are defined for this port. Resorting to 1G");
        _port_speed_min = MESA_SPEED_1G;
    }

    // Then find the maximum supported speed on this port.
    if (_port_status.static_caps & MEBA_PORT_CAP_25G_FDX) {
        _port_speed_max = MESA_SPEED_25G;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_10G_FDX) {
        _port_speed_max = MESA_SPEED_10G;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_5G_FDX) {
        _port_speed_max = MESA_SPEED_5G;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_2_5G_FDX) {
        _port_speed_max = MESA_SPEED_2500M;
    } else if (_port_status.static_caps & MEBA_PORT_CAP_1G_FDX) {
        _port_speed_max = MESA_SPEED_1G;
    } else if ((_port_status.static_caps & MEBA_PORT_CAP_100M_FDX) || (_port_status.static_caps & MEBA_PORT_CAP_100M_HDX)) {
        _port_speed_max = MESA_SPEED_100M;
    } else if ((_port_status.static_caps & MEBA_PORT_CAP_10M_FDX) || (_port_status.static_caps & MEBA_PORT_CAP_10M_HDX)) {
        _port_speed_max = MESA_SPEED_10M;
    } else {
        T_EG_PORT(PORT_TRACE_GRP_CONF, _port_no, "No speeds are defined for this port. Resorting to 1G");
        _port_speed_max = MESA_SPEED_1G;
    }

    VTSS_ASSERT(_port_speed_min <= _port_speed_max);
}

/******************************************************************************/
// port_instance::init_members()
// Public
/******************************************************************************/
void port_instance::init_members(mesa_port_no_t port_no)
{
#if (VTSS_TRACE_LVL_MIN <= VTSS_TRACE_LVL_INFO)
    char    buf[400];
#endif
    bool    old_enable;
    mesa_rc rc;

    (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_no, &_ifindex);
    _port_no                 = port_no;
    _pcs_conf                = MESA_PORT_PCS_NORMAL;
    _port_status.static_caps = port_custom_table[_port_no].cap;
    _port_status.sfp_caps    = 0;
    _port_status.fdx         = true;
    _port_status.speed       = MESA_SPEED_UNDEFINED;

    // Make sure not to switch frames to this port until it's ready.
    port_state_set(__FUNCTION__, __LINE__, false);

    // Figure out the minimum and maximum speed this port supports.
    port_speed_min_max_update();

    if (PORT_cap.has_kr) {
        // Clause 73 is supported only on 10G and 25G SFP ports
        _port_status.has_kr =
            ( _port_status.static_caps & MEBA_PORT_CAP_SFP_ONLY) != 0 &&
            ((_port_status.static_caps & MEBA_PORT_CAP_10G_FDX)  != 0 ||
             (_port_status.static_caps & MEBA_PORT_CAP_25G_FDX)  != 0);
    } else {
        _port_status.has_kr = false;
    }

    T_DG_PORT(PORT_TRACE_GRP_CONF, _port_no, "_port_status.static_caps = <%s>", port_cap_to_txt(buf, sizeof(buf), _port_status.static_caps));

#if defined(VTSS_OOS_FIX)
    if (fast_cap(MESA_CAP_PORT_PCS_CONF)) {
        // Keep pcs disabled at init (Lu26/OOS issue)
        _pcs_conf = MESA_PORT_PCS_IGNORE;
    }
#endif

    conf_default_get(_conf);

    _port_status.fiber = prefer_fiber_get(false);

    // Set the members that never change in _port_conf.
    _port_conf.max_tags          = MESA_PORT_MAX_TAGS_NONE;
    _port_conf.sd_enable         = _port_status.static_caps & MEBA_PORT_CAP_SD_ENABLE        ? true : false;
    _port_conf.sd_active_high    = _port_status.static_caps & MEBA_PORT_CAP_SD_HIGH          ? true : false;
    _port_conf.sd_internal       = _port_status.static_caps & MEBA_PORT_CAP_SD_INTERNAL      ? true : false;
    _port_conf.xaui_rx_lane_flip = _port_status.static_caps & MEBA_PORT_CAP_XAUI_LANE_FLIP   ? true : false;
    _port_conf.xaui_tx_lane_flip = _port_status.static_caps & MEBA_PORT_CAP_XAUI_LANE_FLIP   ? true : false;
    _port_conf.serdes.rx_invert  = _port_status.static_caps & MEBA_PORT_CAP_SERDES_RX_INVERT ? true : false;
    _port_conf.serdes.tx_invert  = _port_status.static_caps & MEBA_PORT_CAP_SERDES_TX_INVERT ? true : false;
    _port_conf.loop              = _port_status.loop_port ? MESA_PORT_LOOP_PCS_HOST : MESA_PORT_LOOP_DISABLE;
    (void)conf_mgmt_mac_addr_get(_port_conf.flow_control.smac.addr, _port_no + 1);

    // Make a check of the default configuration so that we can see that the
    // created default configuration is actually valid if applied with
    // vtss_appl_port_conf_set()
    if ((rc = conf_check(_conf)) != VTSS_RC_OK) {
        // This needs to be fixed in code.
        T_EG_PORT(PORT_TRACE_GRP_CONF, _port_no, "Default configuration is not a valid configuration: Error: %s", error_txt(rc));
    }

    (void)caps_update();

    // Gotta initialize the API right away without enabling the port. The reason
    // is that we only get into poll() after ICFG has applied all configuration,
    // and if some other module (e.g. frame preemption) calls into the API
    // before ICFG has applied all configuration, and the API relies on proper
    // port configuration, that other function call will fail.
    // In fact, this module may also call into the API "out-of-band", namely
    // on Lu26, where lu26_oos_fix() may call mesa_port_conf_get() followed by
    // a failing mesa_port_conf_set() unless we invoked mesa_port_conf_set()
    // first here:
    old_enable = _conf.admin.enable;
    _conf.admin.enable = false;
    port_conf_set(__FUNCTION__, __LINE__, true);
    _conf.admin.enable = old_enable;
}

/******************************************************************************/
// port_instance::reload_defaults()
// Public
/******************************************************************************/
void port_instance::reload_defaults(mesa_port_no_t port_no)
{
    vtss_appl_port_conf_t new_conf;
    port_vol_conf_t       new_vol_conf = _vol_conf;

    conf_default_get(new_conf);

    // If a port is being shut down, update volatile port configuration.
    if (_conf.admin.enable && !new_conf.admin.enable) {
        // This call will update new_vol_conf and we are expected to set it
        // after we have checked for changes.
        // As long as the default port configuration is admin.enable == true,
        // we won't change the _vol_conf, so the following is currently never
        // getting executed, but betteer safe than sorry in case someone changes
        // the default.
        vol_conf_shutdown(new_vol_conf);
    }

    conf_update(new_conf, new_vol_conf);
}

/******************************************************************************/
// port_instance::phy_device_set()
// Public
/******************************************************************************/
void port_instance::phy_device_set(void)
{
    const char *board_name;
    mesa_rc    rc;

    if (_phy_dev) {
        T_EG_PORT(PORT_TRACE_GRP_PHY, _port_no, "Cannot set new PHY device, since a device is already set");
        return;
    }

    // Get the part number of the PHY, if possible.
    if ((rc = meba_phy_info_get(board_instance, _port_no, &_phy_info)) == VTSS_RC_OK) {
        T_IG_PORT(PORT_TRACE_GRP_PHY, _port_no, "Instantiating PHY: %s", _phy_info);
    } else {
        // It's not an error if the driver hasn't installed a info_get() call.
        T_IG_PORT(PORT_TRACE_GRP_PHY, _port_no, "Instantiating PHY: Unable to get info (rc = %s)", error_txt(rc));
    }

    // Some code requires special handling if it's an Aquantia PHY.
    _is_aqr_phy = _phy_info.part_number == 0xB552 ||
                  _phy_info.part_number == 0xB572 ||
                  _phy_info.part_number == 0xB581 ||
                  _phy_info.part_number == 0xB582 ||
                  _phy_info.part_number == 0xB692 ||
                  _phy_info.part_number == 0xB6E0 ||
                  _phy_info.part_number == 0xB6E2 ||
                  _phy_info.part_number == 0xB6F0 ||
                  _phy_info.part_number == 0xB6F2 ||
                  _phy_info.part_number == 0xB700 ||
                  _phy_info.part_number == 0xB710;

    // Going forward, it's much better to get the MAC I/F from MEBA
    // (port_custom_table[]) than from the PHY (meba_phy_if_get()). We can
    // handle this by looking at the board's name. If it contains LAN969x, we
    // use MEBA, otherwise we use the PHY's I/F.
    board_name = vtss_board_name();
    if (strstr(board_name, "LAN969") != nullptr) {
        // Let port_custom_table[] decide the mac_if
        _mac_if_use_port_custom_table = false;
    } else {
        // Let the PHY driver decide the mac_if
        _mac_if_use_port_custom_table = false;
    }

    _phy_dev = true;
    (void)caps_update();

    (void)phy_reset();

    // If a PHY covers more than one port, the call to meba_phy_info_get()
    // above only returned valid date for the base ports for that PHY. Only when
    // meba_phy_reset() is called the first time, will the PHY info be correct.
    // Therefore, we re-cache the PHY info here.
    if ((rc = meba_phy_info_get(board_instance, _port_no, &_phy_info)) == VTSS_RC_OK) {
        T_IG_PORT(PORT_TRACE_GRP_PHY, _port_no, "Instantiating PHY: %s", _phy_info);
    } else {
        // It's not an error if the driver hasn't installed a info_get() call.
        T_IG_PORT(PORT_TRACE_GRP_PHY, _port_no, "Instantiating PHY: Unable to get info (rc = %s)", error_txt(rc));
    }

    turn_on_phy(__FUNCTION__, __LINE__, true);

    // Force poll() to restart port.
    _port_initialized = false;
}

/******************************************************************************/
// port_instance::sfp_device_set()
// Public
/******************************************************************************/
void port_instance::sfp_device_set(meba_sfp_device_t *dev)
{
    mesa_port_interface_t mac_if;
    bool                  prefer_fiber;
    mesa_rc               rc;

    if (dev == nullptr) {
        T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "dev is NULL");
        return;
    }

    if (dev->drv == nullptr) {
        T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "dev->drv is NULL");
        return;
    }

    // A couple of methods that the driver must implement
    if (dev->drv->meba_sfp_driver_conf_set == nullptr) {
        T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "dev->drv->meba_sfp_driver_conf_set(%s) is NULL", dev->info);
        return;
    }

    if (dev->drv->meba_sfp_driver_poll == nullptr) {
        T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "dev->drv->meba_sfp_driver_poll(%s) is NULL", dev->info);
        return;
    }

    if (dev->drv->meba_sfp_driver_if_get == nullptr) {
        T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "dev->drv->meba_sfp_driver_if_get(%s) is NULL", dev->info);
        return;
    }

    if (_sfp_dev) {
        T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "Cannot set new device (%s). A device (%s) is already set", _sfp_dev->info, dev->info);
        return;
    }

    // Figure out whether this is a CuSFP.
    if ((rc = dev->drv->meba_sfp_driver_if_get(dev, MESA_SPEED_AUTO, &mac_if)) != VTSS_RC_OK) {
        T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "meba_sfp_driver_if_set(AUTO, %s) failed: %s", dev->info, error_txt(rc));
        return;
    }

    _cu_sfp = mac_if                == MESA_PORT_INTERFACE_SGMII_CISCO ||
              dev->info.connector   == MEBA_SFP_CONNECTOR_RJ45         ||
              dev->info.transceiver == MEBA_SFP_TRANSRECEIVER_1000BASE_T;

    // If this board does not support Cu SFP in dual media slots, don't use it.

    if (_cu_sfp && PORT_IS_DUAL_MEDIA_NO_COPPER_SFP()) {
        T_IG_PORT(PORT_TRACE_GRP_SFP, _port_no, "Copper SFP not supported in dual media slot (%s)", dev->info);
        _port_status.oper_warnings |= VTSS_APPL_PORT_OPER_WARNING_CU_SFP_IN_DUAL_MEDIA_SLOT;

        // Prevent the port module from loading this SFP again next time it
        // comes to this port in its poll function.
        _cu_sfp       = false;
        _may_load_sfp = false;

        // Keep a copy of the SFP's data to be shown in user interface
        _port_status.sfp_info = dev->info;
        return;
    }

    _sfp_dev              = dev;
    _port_status.sfp_info = _sfp_dev->info;

    // Let the SFP driver itself decide the mac_if...
    _mac_if_use_port_custom_table = false;

    // ... but not in all cases:
    if (strcmp(vtss_board_name(), "FPGA-LAN9698X-Sunrise") == 0) {
        // On the LAN9698x-Sunrise FPGA board, we can't read SFPs, so we need to
        // use the port_custom_table[]'s MAC I/F.
        _mac_if_use_port_custom_table = true;
    }

    T_IG_PORT(PORT_TRACE_GRP_SFP, _port_no, "Setting SFP device %s. is_CuSFP = %d", _sfp_dev->info, _cu_sfp);

    prefer_fiber = caps_update();

    // Force an update of KR conf upon the next invocation of poll
    _kr_initialized = false;

    // Force an update of the SFP driver in the next call to turn_off_sfp().
    _sfp_initialized = false;

    if (prefer_fiber) {
        // Force poll() to restart port
        _port_initialized = false;
    }

    // Force a reset of the SFP. Here, we only turn it off. The poll() function
    // will turn it back on if needed.
    turn_off_sfp(__FUNCTION__, __LINE__);
}

/******************************************************************************/
// port_instance::sfp_device_del()
// Public
/******************************************************************************/
void port_instance::sfp_device_del(void)
{
    bool    prefer_fiber;
    mesa_rc rc;

    if (_sfp_dev) {
        T_IG_PORT(PORT_TRACE_GRP_SFP, _port_no, "Unsetting SFP device %s", _sfp_dev->info);

        if (_sfp_dev->drv->meba_sfp_driver_delete) {
            T_DG_PORT(PORT_TRACE_GRP_SFP, _port_no, "meba_sfp_driver_delete()");
            if ((rc = _sfp_dev->drv->meba_sfp_driver_delete(_sfp_dev)) != VTSS_RC_OK) {
                T_EG_PORT(PORT_TRACE_GRP_SFP, _port_no, "meba_sfp_driver_delete() failed: %s", error_txt(rc));
            }
        }
    }

    vtss_clear(_port_status.sfp_info);
    _sfp_dev      = nullptr;
    _may_load_sfp = true;
    _cu_sfp       = false;

    // Since we don't have an SFP in a slot anymore, we must clear the following
    _port_status.oper_warnings &= ~VTSS_APPL_PORT_OPER_WARNING_CU_SFP_IN_DUAL_MEDIA_SLOT;

    prefer_fiber = caps_update();

    if (_port_status.fiber || prefer_fiber) {
        // If we preferred fiber before we pulled the SFP or we are starting to
        // prefer fiber, because we inserted a new SFP, we must ask the poll()
        // function to restart.
        // The other situation is: We preferred RJ45 before inserting the SFP,
        // and we still do, so don't do anything (dual media ports).

        // Force an update of KR conf upon the next invocation of poll
        _kr_initialized = false;

        // Force poll() to restart port
        _port_initialized = false;
    }

    // Do not do anything else. The next time poll() is invoked, it will take a
    // possible link down.
}

/******************************************************************************/
// port_instance::serdes_media_type_get()
// Returns the media_type by the 10G SFP.
// Private
/******************************************************************************/
mesa_sd10g_media_type_t port_instance::serdes_media_type_get(mesa_port_speed_t speed) const
{
    // Media type only applies for 10/25G SFP ports and backplane.
    // Otherwise no SerDes preset is needed.
    mesa_sd10g_media_type_t serdes_media_type;

    if (!_sfp_dev || !_sfp_dev->drv->meba_sfp_driver_mt_get) {
        return MESA_SD10G_MEDIA_PR_NONE;
    }

    // The SFP driver returns its media type based on contents of the
    // SFP ROM, or in the MAC-to-MAC case - based on the preprovisioned
    // "SFP" driver
    _sfp_dev->drv->meba_sfp_driver_mt_get(_sfp_dev, &serdes_media_type);

    return serdes_media_type;
}

/******************************************************************************/
// port_instance::start_veriphy()
/******************************************************************************/
mesa_rc port_instance::start_veriphy(int mode)
{
    return _phy_dev ? meba_phy_cable_diag_start(board_instance, _port_no, mode) : VTSS_RC_OK;
}

/******************************************************************************/
// port_instance::get_veriphy()
/******************************************************************************/
mesa_rc port_instance::get_veriphy(mepa_cable_diag_result_t *res)
{
    return _phy_dev ? meba_phy_cable_diag_get(board_instance, _port_no, res) : VTSS_RC_OK;
}

/******************************************************************************/
// port_instance::has_phy()
// Returns true if the current configuration is using a PHY
/******************************************************************************/
bool port_instance::has_phy(void) const
{
    return !_port_status.fiber;
}

/******************************************************************************/
// port_instance::is_phy()
// Returns true if the current configuration contains a PHY
/******************************************************************************/
bool port_instance::is_phy(void) const
{
    return !_port_status.fiber;
}

/******************************************************************************/
// port_instance::phy_reset()
/******************************************************************************/
mesa_rc port_instance::phy_reset(bool host_rst)
{
    mepa_reset_param_t rst_conf = {};
    mesa_rc            rc;

    if (!_phy_dev) {
        // PHY not present or it doesn't have a reset function. That's fine.
        return VTSS_RC_OK;
    }

    rst_conf.media_intf = phy_media_if_get();
    rst_conf.reset_point = host_rst ? MEPA_RESET_POINT_POST_MAC : MEPA_RESET_POINT_DEFAULT;

    T_IG_PORT(PORT_TRACE_GRP_PHY, _port_no, "meba_phy_reset(%s)", rst_conf);
    if ((rc = meba_phy_reset(board_instance, _port_no, &rst_conf)) != VTSS_RC_OK && rc != MESA_RC_NOT_IMPLEMENTED) {
        T_EG_PORT(PORT_TRACE_GRP_PHY, _port_no, "meba_phy_reset(%s) failed: %s", rst_conf, error_txt(rc));
    }

    // It's OK that this function is not implemented
    return rc == MESA_RC_NOT_IMPLEMENTED ? MESA_RC_OK : rc;
}

/******************************************************************************/
// port_instance::power_saving_status_update()
/******************************************************************************/
void port_instance::power_saving_status_update(void)
{
#ifdef VTSS_SW_OPTION_PHY_POWER_CONTROL
    vtss_clear(_port_status.power);

    if (!_phy_dev && !_sfp_dev) {
        return;
    }

    // When ActiPhy is enabled power is saved when the link is down
    if (has_phy()) {
        _port_status.power.actiphy_capable      = true;
        _port_status.power.perfectreach_capable = true;
    }

    if (has_phy() && !_port_status.link && (_conf.power_mode == VTSS_PHY_POWER_ACTIPHY || _conf.power_mode == VTSS_PHY_POWER_ENABLED)) {
        _port_status.power.actiphy_power_savings = true;
    }

    // When PerfectReach is enabled power is saved when the link is up
    if (_port_status.link && (_conf.power_mode == VTSS_PHY_POWER_DYNAMIC || _conf.power_mode == VTSS_PHY_POWER_ENABLED)) {
        _port_status.power.perfectreach_power_savings = true;
    }
#endif
}

/******************************************************************************/
// sfp_usable()
/******************************************************************************/
bool port_instance::sfp_usable(void)
{
    uint32_t all_flags_except_nominal_speed = 0xFFFFFFFF;

    // All operational warnings except three make the SFP unusable.
    // The SFP may still be usable if either of those two warnings is issued.
    all_flags_except_nominal_speed &= ~VTSS_APPL_PORT_OPER_WARNING_SFP_NOMINAL_SPEED_HIGHER_THAN_PORT_SPEED;
    all_flags_except_nominal_speed &= ~VTSS_APPL_PORT_OPER_WARNING_SFP_UNREADABLE_IN_THIS_PORT;
    all_flags_except_nominal_speed &= ~VTSS_APPL_PORT_OPER_WARNING_SFP_READ_FAILED;

    return (_port_status.oper_warnings & all_flags_except_nominal_speed) == 0;
}

/******************************************************************************/
// prefer_fiber_get()
/******************************************************************************/
bool port_instance::prefer_fiber_get(bool turn_on_off)
{
    bool use_fiber;

    switch (_conf.media_type) {
    case VTSS_APPL_PORT_MEDIA_CU:
        use_fiber = false;
        break;

    case VTSS_APPL_PORT_MEDIA_SFP:
        // Preferred mode is fiber
        use_fiber = true;
        break;

    case VTSS_APPL_PORT_MEDIA_DUAL:
        // In dual media mode, we need to consult the capabilities set by MEBA
        // to find the preferred media
        //
        // Note: Mostly, the API sets both MEBA_PORT_CAP_DUAL_COPPER and
        // MEBA_CAP_DUAL_FIBER, even though only one of them should be set, to
        // indicate the preferred media, so it's left up to the application
        // (this piece of code) to select the default mode.
        //
        // The thing is that if we select copper as the default mode, we won't
        // be able to detect link on the SFP, because we then read the PHY (in
        // mac_if_from_speed()) to get the MAC interface to use in the call to
        // mesa_port_conf_set(), and then nothing works. So we must use fiber
        // as the preferred interface.
        //
        // Another problem I've experienced on Gi 1/4 on Ocelot PCB123 is that
        // if I change meba/include/mscc/ethernet/board/api/types.h's
        // definition of MEBA_PORT_CAP_SPEED_DUAL_ANY_FIBER to only include
        // MEBA_PORT_CAP_DUAL_FIBER, then the PHY doesn't seem to be initialized
        // correctly, and subsequent temperature readings from the
        // thermal_protect module throws trace errors.
        //
        // However, if the preferred mode is fiber, but no SFP module is plugged
        // in, we must switch to the PHY. Otherwise the MAC I/F will remain with
        // the SFP, and the PHY won't get stable link.
        if (_port_status.static_caps & MEBA_PORT_CAP_DUAL_FIBER) {
            // Fiber is preferred.
            // However, if there's currently no SFP plugged in or if that SFP
            // is not compatible with the currently configured speed, we need
            // to turn off the SFP and use copper.
            if (_sfp_dev && sfp_usable()) {
                // Fiber is preferred and the currently installed SFP is usable
                use_fiber = true;
            } else {
                // Use copper
                use_fiber = false;
            }
        } else {
            // Copper is preferred.
            use_fiber = false;
        }

        break;

    default:
        T_E("Invalid media type (%d)", _conf.media_type);
        use_fiber = false;
    }

    if (turn_on_off) {
        if (use_fiber) {
            if (PORT_IS_DUAL_MEDIA_EXTERNAL_PHY()) {
                // On dual-media ports with external PHY, we need to keep the
                // PHY turned on even if we are using fiber, because the PHY
                // connects to the SFP.
                turn_on_phy(__FUNCTION__, __LINE__);
            } else {
                turn_off_phy(__FUNCTION__, __LINE__);
            }

            turn_on_sfp( __FUNCTION__, __LINE__);
        } else {
            turn_off_sfp(__FUNCTION__, __LINE__);
            turn_on_phy( __FUNCTION__, __LINE__);
        }
    }

    return use_fiber;
}

/******************************************************************************/
// port_instance::kr_status_update()
// Private
/******************************************************************************/
void port_instance::kr_status_update(void)
{
    mesa_port_kr_fec_t fec_conf;
    mesa_rc            rc;

    if (!_port_status.has_kr) {
        // All fields already cleared during instantiation.
        return;
    }

    if ((rc = mesa_port_kr_fec_get(nullptr, _port_no, &fec_conf)) != VTSS_RC_OK) {
        T_EG_PORT(PORT_TRACE_GRP_KR, _port_no, "mesa_port_kr_fec_get() failed: %s", error_txt(rc));
    } else {
        T_IG_PORT(PORT_TRACE_GRP_KR, _port_no, "mesa_port_kr_fec_get() => %s", fec_conf);
    }

    // Get the current FEC mode from our own configuration.
    if (fec_conf.r_fec) {
        _port_status.fec_mode = VTSS_APPL_PORT_FEC_MODE_R_FEC;
    } else if (fec_conf.rs_fec) {
        _port_status.fec_mode = VTSS_APPL_PORT_FEC_MODE_RS_FEC;
    } else {
        _port_status.fec_mode = VTSS_APPL_PORT_FEC_MODE_NONE;
    }
}

/******************************************************************************/
// port_instance::poll()
// Public
/******************************************************************************/
void port_instance::poll(vtss_appl_port_status_t &new_port_status, bool &link_down, bool &link_up)
{
    bool                     old_was_fiber   = _port_status.fiber;
    bool                     prefer_fiber    = prefer_fiber_get(true);
    bool                     call_port_conf_set;
    bool                     vol_conf_changed, new_port_state = false;
    mepa_status_t            old_phy_status  = _phy_status;
    meba_sfp_driver_status_t old_sfp_status  = _sfp_status;
    vtss_appl_port_status_t  old_port_status = _port_status;
    mesa_rc                  rc;

    link_down = false;
    link_up   = false;

    if (old_was_fiber != prefer_fiber) {
        // We possibly gotta change the aneg_method if changing dual media. This
        // only has an effect on what is displayed in CLI, and not the remaining
        // functionality inside this module, because running_1g_aneg() returns
        // the same whether running clause 28 or clause 37 aneg.
        T_IG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "prefer_fiber changed from %d to %d", old_was_fiber, prefer_fiber);
        update_1g_aneg_type(prefer_fiber);
    }

    // Check if the volatile user configuration has changed.
    vol_conf_changed = vol_conf_process();

    // Check if there was a shutdown command or a fast link-down interrupt and
    // the poll method hasn't managed to detect it.
    if (_notify_link_down_phy || _notify_link_down_sfp || _notify_shutdown || !_port_initialized) {

        T_IG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "_notify_link_down_phy = %d, _notify_link_down_sfp = %d, _notify_shutdown = %d, _port_initialized = %d", _notify_link_down_phy, _notify_link_down_sfp, _notify_shutdown, _port_initialized);

        // _notify_link_down_phy is set by port.cxx when a PHY fast link down
        // interrupt is seen.
        // _notify_link_down_sfp is set by port.cxx when an SFP LoS interrupt is
        // seen.
        // _notify_shutdown is set by ourselves when the user shuts down a port
        //
        // In either case, we know that the port is soon going down. However, if
        // we allow the PHY or SFP to be polled immediately after the fast link
        // down or LoS interrupt, the status may still be up.
        // This would in turn result in "down-up-down" events to the registered
        // modules of link change events.
        //
        // By postponing the polling for one poll-cycle, we are pretty sure that
        // after a fast-link-down or LoS interrupt, the PHY/SFP is actually
        // down, so that it will only result in *one* "down" event.
        if (_notify_link_down_phy || _notify_shutdown || !_port_initialized) {
            _dont_poll_phy_next_time = true;
            _phy_status.link         = false;
        }

        if (_notify_link_down_sfp || _notify_shutdown || !_port_initialized) {
            _dont_poll_sfp_next_time = true;
            _sfp_status.link         = false;
        }
    }

    if (!_dont_poll_phy_next_time) {
        if (_phy_dev && _phy_conf.admin.enable /* PHY turned on */) {
            if ((rc = meba_phy_status_poll(board_instance, _port_no, &_phy_status)) != VTSS_RC_OK) {
                T_EG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "meba_phy_status_poll() failed: %s", error_txt(rc));
            } else {
                T_DG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "PHY status = %s", _phy_status);
            }
        } else {
            _phy_status.link = false;
        }
    }

    if (!_dont_poll_sfp_next_time) {
        if (_sfp_dev && _sfp_turned_on && sfp_usable()) {
            if ((rc = _sfp_dev->drv->meba_sfp_driver_poll(_sfp_dev, &_sfp_status)) != VTSS_RC_OK) {
                T_EG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "meba_sfp_driver_poll() failed: %s", error_txt(rc));
            } else {
                if (_sfp_status.link) {
                    if (_sfp_status.los) {
                        // In some cases (e.g. 100BASE-SX/LX in JR2-24 10G
                        // port), the PCS/Serdes status is not good enough to
                        // indicate whether there's link or not, so we gate it
                        // with the SGPIO telling whether the SFP has Loss of
                        // Signal. If this SGPIO is not connected, los defaults
                        // to false, so it's safe to test for los == true.
                        _sfp_status.link = false;
                    } else if (_kr_conf.aneg.enable && _kr_initialized && PORT_cap.has_kr_v3 && !_kr_complete) {
                        // If we have started clause 73 but it hasn't yet
                        // completed aneg and possibly also training, we must not
                        // signal link up. The link can flap many times during
                        // aneg.
                        T_IG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "Got link-up, but clause 73 aneg/training not complete yet");
                        _sfp_status.link = false;
                    }
                }

                T_DG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "SFP status = %s", _sfp_status);
            }
        } else {
            _sfp_status.link = false;
        }
    }

    // Go through the new status
    if (_phy_status.link && _sfp_status.link) {
        // Link on both PHY and SFP port (must be a dual media port then).
        // Select the preferred one.
        _port_status.fiber = prefer_fiber;

        if (_port_status.fiber != old_was_fiber) {
            if (( old_was_fiber && old_sfp_status.link) ||
                (!old_was_fiber && old_phy_status.link)) {
                // Force a link down before changing media.
                T_IG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "Link down");
                link_down = true;

                // Force a link up on the new media.
                link_up = true;
            }
        } else {
            if (( _port_status.fiber && !old_sfp_status.link) ||
                (!_port_status.fiber && !old_phy_status.link)) {
                link_up = true;
            }
        }

        if (prefer_fiber) {
            // Fiber is preferred. No link on copper port.
            _phy_status.link   = false;
            _port_status.link  = true;
            _port_status.speed = _sfp_status.speed;
            _port_status.fdx   = _sfp_status.fdx;
            _port_status.aneg  = _sfp_status.aneg;
        } else {
            // Copper is preferred. No link on SFP port.
            _sfp_status.link   = false;
            _port_status.link  = true;
            _port_status.speed = _phy_status.speed;
            _port_status.fdx   = _phy_status.fdx;
            _port_status.aneg  = _phy_status.aneg;
        }
    } else if (_phy_status.link) {
        _port_status.fiber = false;

        if (old_was_fiber && old_sfp_status.link) {
            // Force a link down on the SFP interface.
            T_IG_PORT(PORT_TRACE_GRP_STATUS,  _port_no, "Link down");
            link_down = true;

            // Force a new link up on the Cu interface
            link_up = true;
        } else if (!old_phy_status.link) {
            link_up = true;
        }

        _port_status.link  = true;
        _port_status.speed = _phy_status.speed;
        _port_status.fdx   = _phy_status.fdx;
        _port_status.aneg  = _phy_status.aneg;
    } else if (_sfp_status.link) {
        _port_status.fiber = true;

        if (!old_was_fiber && old_phy_status.link) {
            // Force a link down on the Cu interface.
            T_IG_PORT(PORT_TRACE_GRP_STATUS,  _port_no, "Link down");
            link_down = true;

            // Force a new link up on the SFP interface
            link_up = true;
        } else if (!old_sfp_status.link) {
            link_up = true;
        }

        _port_status.link  = true;
        _port_status.speed = _sfp_status.speed;
        _port_status.fdx   = _sfp_status.fdx;
        _port_status.aneg  = _sfp_status.aneg;
    } else {
        // Not link on any.
        _port_status.fiber = prefer_fiber;

        if (( old_was_fiber && old_sfp_status.link) ||
            (!old_was_fiber && old_phy_status.link)) {
            T_IG_PORT(PORT_TRACE_GRP_STATUS,  _port_no, "Link down");
            link_down = true;
        }

        _port_status.link  = false;
        _port_status.speed = MESA_SPEED_UNDEFINED;
        _port_status.fdx   = true;
        vtss_clear(_port_status.aneg);
    }

    if (vol_conf_changed) {
        // The volatile configuration has changed. This can cause port state
        // changes. Operational down wins.
        if (_vol_conf.oper_down) {
            new_port_state   = false;
            vol_conf_changed = _port_state == true;
        } else if (_vol_conf.oper_up) {
            new_port_state   = true;
            vol_conf_changed = _port_state == false;
        } else {
            // No volatile configuration overrides anymore. Time to go back to
            // whatever it should be.
            new_port_state   = _port_status.link;
            vol_conf_changed = _port_status.link != _port_state;
        }
    }

    if (link_down || (vol_conf_changed && !new_port_state)) {
        if (link_down) {
            T_IG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "Link down");
        } else {
            T_IG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "Oper state down");
        }

        port_state_set(__FUNCTION__, __LINE__, false);

        if (link_down) {
            if (!old_was_fiber && _phy_dev) {
                lu26_oos_fix(__FUNCTION__, __LINE__, true, old_phy_status.speed, false);
            }

            // The fact that the port went down may clear a flag that indicates
            // that the SFP may not work, because its nominal (maximum) speed is
            // higher than the actual speed.
            sfp_may_not_work_oper_warning_update();
        }
    }

    // Change port conf if needed.
    call_port_conf_set = false;
    if (old_port_status.fiber != _port_status.fiber || !_port_initialized) {
        call_port_conf_set = true;
    } else if (_port_status.link &&
               (!old_port_status.link                                               ||
                _port_status.speed               != old_port_status.speed           ||
                _port_status.fdx                 != old_port_status.fdx             ||
                _port_status.aneg.obey_pause     != old_port_status.aneg.obey_pause ||
                _port_status.aneg.generate_pause != old_port_status.aneg.generate_pause)) {
        call_port_conf_set = true;
    }

    if (call_port_conf_set) {
        if (kr_about_to_be_disabled()) {
            // If we are disabling clause 73, we must do that before setting the
            // new port configuration with port_conf_set(), because if the new
            // port configuration contains a speed that is not supported by
            // clause 73, an API error will occur if clause 73 was still
            // enabled.
            (void)kr_conf_update();
        }

        port_conf_set(__FUNCTION__, __LINE__);

        // port_conf_set() sets (a.o.) the interface between the switch's MAC
        // and whatever interfaces to it, which can be one of three things:
        //  - a Cu port (1000BASE-T)
        //  - an SFP port (Serdes)
        //  - an external PHY (QSGMII on Lu26)
        //
        // When it's an external PHY, we must tell the PHY which interface to
        // use on its media side (outbound side) as well. We do that with
        // phy_media_if_set().
        phy_media_if_set();

        // While the dual mode phy needs to perform the oos_fix the dual mode fiber does not.
        // However the oos_fix must be disabled (pcs enabled) when the fiber is chosen
        if (_port_status.fiber && (old_port_status.fiber != _port_status.fiber)) {
            lu26_oos_fix(__FUNCTION__, __LINE__, false, _phy_status.speed, true);
        }

        _port_initialized = true;
    }

    if (link_up || (vol_conf_changed && new_port_state)) {
        if (link_up) {
            T_IG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "Link up");
        } else {
            T_IG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "Oper state up");
        }

        port_state_set(__FUNCTION__, __LINE__, true);

        if (link_up) {
            if (!_port_status.fiber && _phy_dev) {
                lu26_oos_fix(__FUNCTION__, __LINE__, true, _phy_status.speed, true);
            }

            // The fact that the port came up may set a flag that indicates that
            // the SFP may not work, because its nominal (maximum) speed is
            // higher than the actual speed.
            sfp_may_not_work_oper_warning_update();
        }
    }

    // Check if we need to update the KR conf. This only has effect if either
    // KR is already enabled or if we are about to enable it.
    // This must be done *after* the call to port_conf_set(), because if _conf
    // changes, we must set up the port configuration in the API before starting
    // clause 73, because after we have enabled clause 73, we may no longer call
    // mesa_port_conf_set(). And we won't do that as long as the old
    // mesa_port_conf equals the new. If we did re-invoke mesa_port_conf_set()
    // after we got link, we would end up overwriting the Serdes equalizer
    // settings, which are fine-tuned by the clause 73 process.
    // If KR is getting disabled, the following call has no effect, because it's
    // already handled in the kr_conf_update() call above.
    kr_conf_update();

    if (link_up || link_down) {
        kr_status_update();

        // Load TS latencies for every link up event.
        if (link_up && fast_cap(MESA_CAP_TS)) {
            (void)mesa_ts_status_change(nullptr, _port_no);
        }
    }

    if (link_up) {
        _port_status.link_up_cnt++;
    }

    if (link_down) {
        _port_status.link_down_cnt++;
    }

    // Update power saving status
    power_saving_status_update();

    // See comment at the top of this function.
    if (_notify_link_down_phy || _notify_link_down_sfp || _notify_shutdown) {
        _notify_link_down_phy = false;
        _notify_link_down_sfp = false;
        _notify_shutdown      = false;
    } else {
        _dont_poll_sfp_next_time = false;
        _dont_poll_phy_next_time = false;
    }

    // This may miss the link down event if a link up event happens at the same
    // time on dual media ports. Cannot do anything about it.
    port_status_update.set(_ifindex, &_port_status);

    new_port_status = _port_status;
}

/******************************************************************************/
// port_instance::port_status_get()
// Public
/******************************************************************************/
void port_instance::port_status_get(vtss_appl_port_status_t &port_status)
{
    // Get the latest Clause 73 status
    // kr_status_update();

    port_status = _port_status;
}

/******************************************************************************/
// port_instance::led_update()
// Public
/******************************************************************************/
mesa_rc port_instance::led_update(void)
{
    // Update LEDs
    mesa_port_counters_t    counters;
    mesa_port_status_t      mesa_port_status = {};
    meba_port_admin_state_t admin_state = {admin_enabled(_conf, _vol_conf)};

    mesa_port_status.link   =  _port_status.link;
    mesa_port_status.speed  =  _port_status.speed;
    mesa_port_status.fdx    =  _port_status.fdx;
    mesa_port_status.aneg   =  _port_status.aneg;
    mesa_port_status.copper = !_port_status.fiber;
    mesa_port_status.fiber  =  _port_status.fiber;

    if (mesa_port_counters_get(nullptr, _port_no, &counters) != VTSS_RC_OK) {
        vtss_clear(counters);
    }

    // meba_port_led_update() calls mesa_sgpio_conf_get()/set(), and these two
    // calls must be invoked without interference.
    VTSS_APPL_API_LOCK_SCOPE();
    return meba_port_led_update(board_instance, _port_no, &mesa_port_status, &counters, &admin_state);
}

/******************************************************************************/
// port_instance::vol_conf_merge()
// Private
/******************************************************************************/
void port_instance::vol_conf_merge(port_vol_conf_t &new_vol_conf)
{
    uint32_t user;

    // Merge all _vol_confs
    vtss_clear(new_vol_conf);

    for (user = PORT_USER_STATIC; user < PORT_USER_CNT; user++) {
        port_vol_conf_t &conf = _vol_confs[user];

        if (conf.disable || conf.disable_adm_recover) {
            new_vol_conf.disable = true;
        }

        if (conf.oper_up) {
            new_vol_conf.oper_up = true;
        }

        if (conf.oper_down) {
            new_vol_conf.oper_down = true;
        }
    }
}

/******************************************************************************/
// port_instance::vol_conf_shutdown()
// Private
/******************************************************************************/
void port_instance::vol_conf_shutdown(port_vol_conf_t &new_vol_conf)
{
    uint32_t user;

    // Volatile users that have set the disable_adm_recover flag
    // will get it reset as soon as the end-user performs a "shutdown".
    // If at the same time, the volatile user has specified a callback,
    // he will get notified about the change. This allows the user to
    // change her own state.
    for (user = 0; user < PORT_USER_CNT; user++) {
        port_vol_conf_t &vol_conf = _vol_confs[user];

        if (vol_conf.disable_adm_recover) {
            // Only call back when a state-change occurs.
            vol_conf.disable_adm_recover = false;
            if (vol_conf.on_adm_recover_clear) {
                vol_conf.on_adm_recover_clear(_port_no, &vol_conf);
            }
        }
    }

    // Get the new aggreate volatile configuration
    vol_conf_merge(new_vol_conf);
}

/******************************************************************************/
// port_instance::vol_conf_process()
// Returns true if either oper_up or oper_down has changed.
// Private
/******************************************************************************/
bool port_instance::vol_conf_process(void)
{
    port_vol_conf_t new_vol_conf;
    bool            vol_conf_changed;

    if (!_vol_config_changed) {
        return false;
    }

    _vol_config_changed = false;

    vol_conf_merge(new_vol_conf);

    if (memcmp(&new_vol_conf, &_vol_conf, sizeof(new_vol_conf)) == 0) {
        return false;
    }

    T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "Change disable: %d->%d, oper_down: %d->%d, oper_up: %d->%d",
              _vol_conf.disable,   new_vol_conf.disable,
              _vol_conf.oper_down, new_vol_conf.oper_down,
              _vol_conf.oper_up,   new_vol_conf.oper_up);

    vol_conf_changed = new_vol_conf.oper_down != _vol_conf.oper_down ||
                       new_vol_conf.oper_up   != _vol_conf.oper_up;

    // Handle the volatile disable stuff.
    conf_update(_conf, new_vol_conf);

    return vol_conf_changed;
}

/******************************************************************************/
// port_instance::vol_conf_get()
// Public
/******************************************************************************/
mesa_rc port_instance::vol_conf_get(port_user_t user, port_vol_conf_t &vol_conf)
{
    vol_conf = _vol_confs[user];
    return VTSS_RC_OK;
}

/******************************************************************************/
// port_instance::vol_conf_set()
// Public
/******************************************************************************/
mesa_rc port_instance::vol_conf_set(port_user_t user, const port_vol_conf_t &vol_conf)
{
    if (vol_conf.oper_up && vol_conf.oper_down) {
        // oper_up and oper_down cannot be enabled simulataneously
        T_EG_PORT(PORT_TRACE_GRP_CONF, _port_no, "User = %s: Both oper_up and oper_down set simultaneously", port_instance_vol_user_txt(user));
        return VTSS_APPL_PORT_RC_PARM;
    }

    PORT_INSTANCE_dont_trace_all = true;
    T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "%s: %s (port_status = %s)", port_instance_vol_user_txt(user), vol_conf, _port_status);
    PORT_INSTANCE_dont_trace_all = false;

    if (memcmp(&vol_conf, &_vol_confs[user], sizeof(vol_conf)) == 0) {
        // No changes
        return VTSS_RC_OK;
    }

    _vol_confs[user] = vol_conf;

    // Process it asynchronously to avoid mutex deadlocks (see
    // vol_conf_process())
    _vol_config_changed = true;

    return VTSS_RC_OK;
}

// Lu26 PTP out-of-sync work-around code

#if defined(VTSS_OOS_FIX)
/******************************************************************************/
// port_instance::lu26_oos_port_conf_set()
// Only applies to external SGMII/QSGMII ports
/******************************************************************************/
void port_instance::lu26_oos_port_conf_set(bool enable)
{
    mesa_port_conf_t tmp;

    T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "enable = %d", enable);

    (void)mesa_port_conf_get(nullptr, _port_no, &tmp);

    if (tmp.power_down) {
        return;
    }

    if ((tmp.pcs == MESA_PORT_PCS_ENABLE  &&  enable) ||
        (tmp.pcs == MESA_PORT_PCS_DISABLE && !enable) ||
        (tmp.pcs == MESA_PORT_PCS_IGNORE  && !enable)) {
        return; // Nothing to do
    }

    tmp.pcs = enable ? MESA_PORT_PCS_ENABLE : MESA_PORT_PCS_DISABLE;

    T_IG_PORT(PORT_TRACE_GRP_CONF, _port_no, "OOS issue: %s the PCS (conf = %s)", enable ? "enable" : "disable", tmp);
    (void)mesa_port_conf_set(nullptr, _port_no, &tmp);

    // Compute next state
    if (tmp.pcs == MESA_PORT_PCS_DISABLE) {
        _pcs_conf = MESA_PORT_PCS_IGNORE;
    } else {
        _pcs_conf = MESA_PORT_PCS_NORMAL;
    }
}
#endif

#if defined(VTSS_OOS_FIX)
/******************************************************************************/
// port_instance::lu26_oos_aneg_clear()
/******************************************************************************/
void port_instance::lu26_oos_aneg_clear(mesa_port_speed_t new_speed)
{
    mepa_adv_dis_t old_adv_dis = _conf.adv_dis;

    T_I_PORT(_port_no, "_conf.speed = %s, speed = %s", port_speed_to_txt(_conf.speed), port_speed_to_txt(new_speed));
    if (_conf.speed != MESA_SPEED_AUTO || new_speed == MESA_SPEED_1G) {
        return;
    }

    T_I_PORT(_port_no, "OOS issue: Clear PHY advertisement, but do not restart aneg");

    // Clear all PHY capabilities to ensure that a PHY speed change does not
    // occur without the application knowing.
    _conf.adv_dis |= MEPA_ADV_DIS_SPEED | MEPA_ADV_DIS_DUPLEX | MEPA_ADV_DIS_RESTART_ANEG;

    turn_on_phy(__FUNCTION__, __LINE__, true);

    // Restore original adv_dis flags.
    _conf.adv_dis = old_adv_dis;
}
#endif

#if defined(VTSS_OOS_FIX)
/******************************************************************************/
// port_instance::lu26_oos_aneg_restore()
/******************************************************************************/
void port_instance::lu26_oos_aneg_restore(mesa_port_speed_t old_speed)
{
    T_I_PORT(_port_no, "_conf.speed = %s, old_speed = %s", port_speed_to_txt(_conf.speed), port_speed_to_txt(old_speed));

    if (_conf.speed != MESA_SPEED_AUTO || old_speed == MESA_SPEED_1G) {
        return;
    }

    // Set the PHY capabilities back to user config
    T_I_PORT(_port_no, "OOS issue: Restore PHY advertisement and restart aneg");

    turn_on_phy(__FUNCTION__, __LINE__, true);
}
#endif

/******************************************************************************/
// port_instance::lu26_oos_fix()
/******************************************************************************/
void port_instance::lu26_oos_fix(const char *func, int line, bool update_phy_dev, mesa_port_speed_t speed, bool pcs_ena)
{
#if defined(VTSS_OOS_FIX)
    if (!_phy_dev) {
        // We don't have a PHY
        return;
    }

    if (!fast_cap(MESA_CAP_PORT_PCS_CONF)) {
        // Not vulnerable
        return;
    }

    if (port_custom_table[_port_no].map.chip_port < 10) {
        // Not vulnerable
        return;
    }

    T_I_PORT(_port_no, "%s#%d. speed = %s, update_phy_dev = %d, pcs_ena = %d", func, line, port_speed_to_txt(speed), update_phy_dev, pcs_ena);
    if (!pcs_ena) {
        lu26_oos_port_conf_set(pcs_ena);
    }

    if (update_phy_dev) {
        if (pcs_ena) {
            lu26_oos_aneg_clear(speed);
        } else {
            lu26_oos_aneg_restore(speed);
        }
    }

    if (pcs_ena) {
        lu26_oos_port_conf_set(pcs_ena);
    }
#endif
}

/******************************************************************************/
// port_instance::debug_state_dump()
/******************************************************************************/
void port_instance::debug_state_dump(uint32_t session_id, port_icli_pr_t pr)
{
    mesa_port_kr_status_t    local_kr_status;
    vtss_appl_port_conf_t    local_conf;
    vtss_appl_port_status_t  local_port_status;
    mepa_conf_t              local_phy_conf;
    meba_sfp_driver_conf_t   local_sfp_conf;
    mesa_port_conf_t         local_port_conf;
    bool                     local_port_state;
    port_vol_conf_t          local_vol_conf;
    mesa_port_kr_conf_t      local_kr_conf;
    mesa_port_kr_fec_t       local_fec_conf;
    mepa_status_t            local_phy_status;
    mepa_phy_info_t          local_phy_info;
    meba_sfp_driver_status_t local_sfp_status;
    meba_sfp_status_t        local_sfp_dev_status;
    mepa_media_interface_t   local_phy_media_if;
    bool                     local_phy_dev_exists;
    bool                     local_sfp_dev_exists;
    mesa_port_speed_t        local_phy_speed;
    mesa_port_speed_t        local_sfp_speed;
    mesa_port_speed_t        local_port_speed_min;
    mesa_port_speed_t        local_port_speed_max;
    bool                     local_sfp_must_run_kr;
    bool                     local_sfp_may_run_kr;
    bool                     local_sfp_turned_on;
    bool                     local_may_load_sfp;
    bool                     local_is_100m_sfp;
    bool                     local_is_aqr_phy;
    bool                     local_phy_host_side_reset;
    bool                     local_port_custom_table;
    char                     if_str[50], buf[400];
    vtss::StringStream       s;
    const int                width = 30;
    mesa_rc                  rc;

    {
        PORT_LOCK_SCOPE();
        // Gotta obtain a consistent state/conf into local variables before
        // letting go of the port lock. Otherwise, we keep it for too long -
        // especially if the user decides not to react on the
        // "-- more --, next page..." at the bottom of the console window.
        local_port_status     = _port_status;
        local_conf            = _conf;
        local_phy_conf        = _phy_conf;
        local_sfp_conf        = _sfp_conf;
        local_port_conf       = _port_conf;
        local_port_state      = _port_state;
        local_vol_conf        = _vol_conf;
        local_kr_conf         = _kr_conf;
        local_fec_conf        = _fec_conf;
        local_phy_status      = _phy_status;
        local_phy_info        = _phy_info;
        local_sfp_status      = _sfp_status;
        if (_sfp_dev) {
            local_sfp_dev_status  = _sfp_dev->sfp;
        } else {
            vtss_clear(local_sfp_dev_status);
        }

        local_phy_media_if        = _phy_media_if;
        local_phy_dev_exists      = _phy_dev;
        local_sfp_dev_exists      = _sfp_dev != nullptr;
        local_phy_speed           = _phy_speed;
        local_sfp_speed           = _sfp_speed;
        local_port_speed_min      = _port_speed_min;
        local_port_speed_max      = _port_speed_max;
        local_sfp_must_run_kr     = _sfp_must_run_kr;
        local_sfp_may_run_kr      = _sfp_may_run_kr;
        local_sfp_turned_on       = _sfp_turned_on;
        local_may_load_sfp        = _may_load_sfp;
        local_is_100m_sfp         = _is_100m_sfp;
        local_is_aqr_phy          = _is_aqr_phy;
        local_phy_host_side_reset = _phy_host_side_reset;
        local_port_custom_table   = _mac_if_use_port_custom_table;
    }

    pr(session_id, "%s:\n", icli_port_info_txt_short(VTSS_USID_START, iport2uport(_port_no), if_str));

    pr(session_id, " %-*s <%s>\n", width, "Static caps:", port_cap_to_txt(buf, sizeof(buf), local_port_status.static_caps));
    pr(session_id, " %-*s <%s>\n", width, "SFP caps:",    port_cap_to_txt(buf, sizeof(buf), local_port_status.sfp_caps));

    s.clear();
    s << local_conf;
    pr(session_id, " %-*s %s\n", width, "_conf:", s.cstring());

    s.clear();
    s << local_phy_conf;
    pr(session_id, " %-*s %s\n", width, "_phy_conf:", s.cstring());

    s.clear();
    s << local_sfp_conf;
    pr(session_id, " %-*s %s\n", width, "_sfp_conf:", s.cstring());

    s.clear();
    s << local_port_conf;
    pr(session_id, " %-*s %s\n", width, "_port_conf:", s.cstring());
    pr(session_id, " %-*s %s\n", width, "_port_state: ", local_port_state ? "Enabled" : "Disabled");

    s.clear();
    s << local_vol_conf;
    pr(session_id, " %-*s %s\n", width, "_vol_conf: ", s.cstring());

    s.clear();
    PORT_INSTANCE_dont_trace_all = true;
    s << local_port_status;
    PORT_INSTANCE_dont_trace_all = false;
    pr(session_id, " %-*s %s\n", width, "_port_status:", s.cstring());

    s.clear();
    s << local_phy_status;
    pr(session_id, " %-*s %s\n", width, "_phy_status:", s.cstring());

    s.clear();
    s << local_phy_info;
    pr(session_id, " %-*s %s\n", width, "_phy_info:", s.cstring());

    s.clear();
    s << local_sfp_status;
    pr(session_id, " %-*s %s\n", width, "_sfp_status:", s.cstring());

    s.clear();
    s << local_sfp_dev_status;
    pr(session_id, " %-*s %s\n", width, "_sfp_dev_status:", s.cstring());

    if (local_port_status.has_kr) {
        s.clear();
        s << local_kr_conf;
        pr(session_id, " %-*s %s\n", width, "_kr_conf:", s.cstring());

        s.clear();
        s << local_fec_conf;
        pr(session_id, " %-*s %s\n", width, "_fec_conf:", s.cstring());

        if ((rc = mesa_port_kr_status_get(nullptr, _port_no, &local_kr_status)) != VTSS_RC_OK) {
            T_EG_PORT(PORT_TRACE_GRP_STATUS, _port_no, "mesa_port_kr_status_get() failed: %s", error_txt(rc));
        } else {
            s.clear();
            s << local_kr_status;
            pr(session_id, " %-*s %s\n", width, "_kr_status:", s.cstring());
        }
    }

    pr(session_id, " %-*s %s\n", width, "PHY dev:", PORT_INSTANCE_yes_no_str(local_phy_dev_exists));

    if (local_phy_dev_exists) {
        pr(session_id, " %-*s %s\n", width, "_phy_media_if:", phy_media_if2txt(local_phy_media_if));
    } else {
        pr(session_id, " %-*s %s\n", width, "_phy_media_if:", "N/A");
    }

    if (local_sfp_dev_exists) {
        s.clear();
        s << local_port_status.sfp_info;
        pr(session_id, " %-*s %s (%s)\n", width, "SFP dev:", PORT_INSTANCE_yes_no_str(local_sfp_dev_exists), s.cstring());
    } else {
        pr(session_id, " %-*s %s\n", width, "SFP dev:", PORT_INSTANCE_yes_no_str(local_sfp_dev_exists));
    }

    pr(session_id, " %-*s %s\n",    width, "_phy_speed:",                    port_speed_to_txt(local_phy_speed));
    pr(session_id, " %-*s %s\n",    width, "_sfp_speed:",                    port_speed_to_txt(local_sfp_speed));
    pr(session_id, " %-*s %s-%s\n", width, "_port_speeds:",                  port_speed_to_txt(local_port_speed_min), port_speed_to_txt(local_port_speed_max));
    pr(session_id, " %-*s %s\n",    width, "_sfp_must_run_kr:",              PORT_INSTANCE_yes_no_str(local_sfp_must_run_kr));
    pr(session_id, " %-*s %s\n",    width, "_sfp_may_run_kr:",               PORT_INSTANCE_yes_no_str(local_sfp_may_run_kr));
    pr(session_id, " %-*s %s\n",    width, "_phy_turned_on:",                PORT_INSTANCE_yes_no_str(local_phy_conf.admin.enable));
    pr(session_id, " %-*s %s\n",    width, "_sfp_turned_on:",                PORT_INSTANCE_yes_no_str(local_sfp_turned_on));
    pr(session_id, " %-*s %s\n",    width, "_may_load_sfp:",                 PORT_INSTANCE_yes_no_str(local_may_load_sfp));
    pr(session_id, " %-*s %s\n",    width, "_is_100m_sfp:",                  PORT_INSTANCE_yes_no_str(local_is_100m_sfp));
    pr(session_id, " %-*s %s\n",    width, "_is_aqr_phy:",                   PORT_INSTANCE_yes_no_str(local_is_aqr_phy));
    pr(session_id, " %-*s %s\n",    width, "_phy_host_side_reset:",          PORT_INSTANCE_yes_no_str(local_phy_host_side_reset));
    pr(session_id, " %-*s %s\n",    width, "_mac_if_use_port_custom_table:", PORT_INSTANCE_yes_no_str(local_port_custom_table));
    pr(session_id, "\n");
}

