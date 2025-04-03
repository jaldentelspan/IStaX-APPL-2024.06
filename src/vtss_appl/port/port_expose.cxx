/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "snmp.hxx"  // For CYG_HTTPD_STATE
#include <vtss/appl/port.h>
#include "port_expose.hxx"
#include "port_trace.h"
#include "port_api.h"
#include "port_iter.hxx"
#include "vtss_os_wrapper.h"
#include "misc_api.h" /* For iport2uport() */
#ifdef VTSS_SW_OPTION_PRIVATE_MIB_GEN
#include "web_api.h"
#endif
#ifdef VTSS_SW_OPTION_QOS
#include "qos_api.h"  // For VTSS_SW_OPTION_QOS_ADV
#endif

//
// Define the Entry names
//
vtss_enum_descriptor_t port_expose_speed_duplex_txt[] {
    {PORT_EXPOSE_SPEED_DUPLEX_10M_FDX,   "force10ModeFdx"},
    {PORT_EXPOSE_SPEED_DUPLEX_10M_HDX,   "force10ModeHdx"},
    {PORT_EXPOSE_SPEED_DUPLEX_100M_FDX,  "force100ModeFdx"},
    {PORT_EXPOSE_SPEED_DUPLEX_100M_HDX,  "force100ModeHdx"},
    {PORT_EXPOSE_SPEED_DUPLEX_1G_FDX,    "force1GModeFdx"},
    {PORT_EXPOSE_SPEED_DUPLEX_AUTO,      "autoNegMode"},
    {PORT_EXPOSE_SPEED_DUPLEX_2500M_FDX, "force2G5ModeFdx"},
    {PORT_EXPOSE_SPEED_DUPLEX_5G_FDX,    "force5GModeFdx"},
    {PORT_EXPOSE_SPEED_DUPLEX_10G_FDX,   "force10GModeFdx"},
    {PORT_EXPOSE_SPEED_DUPLEX_12G_FDX,   "force12GModeFdx"},
    {PORT_EXPOSE_SPEED_DUPLEX_25G_FDX,   "force25GModeFdx"},
    {0, 0},
};

vtss_enum_descriptor_t port_expose_media_txt[] {
    {VTSS_APPL_PORT_MEDIA_CU,   "rj45"},
    {VTSS_APPL_PORT_MEDIA_SFP,  "sfp"},
    {VTSS_APPL_PORT_MEDIA_DUAL, "dual"},
    {0, 0},
};

vtss_enum_descriptor_t port_expose_fc_txt[] {
    {PORT_EXPOSE_FC_OFF, "off"},
    {PORT_EXPOSE_FC_ON,  "on"},
    {0, 0},
};

vtss_enum_descriptor_t port_expose_fec_mode_txt[] {
    {VTSS_APPL_PORT_FEC_MODE_NONE,   "fecModeNone"},
    {VTSS_APPL_PORT_FEC_MODE_R_FEC,  "fecModeRFec"},
    {VTSS_APPL_PORT_FEC_MODE_RS_FEC, "fecModeRSFec"},
    {VTSS_APPL_PORT_FEC_MODE_AUTO,   "fecModeAuto"},
    {0, 0},
};

vtss_enum_descriptor_t port_expose_status_speed_txt[] {
    {MESA_SPEED_UNDEFINED, "undefined"},
    {MESA_SPEED_10M,       "speed10M"},
    {MESA_SPEED_100M,      "speed100M"},
    {MESA_SPEED_1G,        "speed1G"},
    {MESA_SPEED_2500M,     "speed2G5"},
    {MESA_SPEED_5G,        "speed5G" },
    {MESA_SPEED_10G,       "speed10G"},
    {MESA_SPEED_12G,       "speed12G"},
    {MESA_SPEED_25G,       "speed25G"},
    {0, 0},
};

vtss_enum_descriptor_t port_expose_phy_veriphy_status_txt[] {
    {MEPA_CABLE_DIAG_STATUS_OK,      "correctlyTerminatedPair"},
    {MEPA_CABLE_DIAG_STATUS_OPEN,    "openPair"},
    {MEPA_CABLE_DIAG_STATUS_SHORT,   "shortPair"},
    {MEPA_CABLE_DIAG_STATUS_ABNORM,  "abnormalTermination"},
    {MEPA_CABLE_DIAG_STATUS_SHORT_A, "crossPairShortToPairA"},
    {MEPA_CABLE_DIAG_STATUS_SHORT_B, "crossPairShortToPairB"},
    {MEPA_CABLE_DIAG_STATUS_SHORT_C, "crossPairShortToPairC"},
    {MEPA_CABLE_DIAG_STATUS_SHORT_D, "crossPairShortToPairD"},
    {MEPA_CABLE_DIAG_STATUS_COUPL_A, "abnormalCrossPairCouplingToPairA"},
    {MEPA_CABLE_DIAG_STATUS_COUPL_B, "abnormalCrossPairCouplingToPairB"},
    {MEPA_CABLE_DIAG_STATUS_COUPL_C, "abnormalCrossPairCouplingToPairC"},
    {MEPA_CABLE_DIAG_STATUS_COUPL_D, "abnormalCrossPairCouplingToPairD"},
    {MEPA_CABLE_DIAG_STATUS_UNKNOWN, "unknownResult"},
    {MEPA_CABLE_DIAG_STATUS_RUNNING, "veriPhyRunning"},
    {0, 0},
};

// Dummy function because the "Write Only" OID is implemented with a
// TableReadWrite
mesa_rc port_stats_dummy_get(vtss_ifindex_t ifindex, BOOL *const clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }

    return VTSS_RC_OK;
}

#ifdef VTSS_UI_OPT_VERIPHY
// Wrapper for starting VeriPHy, converting from ifindex to isid, port
// In : ifindex - OID ifindex
//      start   - Set to TRUE to start VeriPhy for the select ifindex interface
mesa_rc port_expose_veriphy_start_set(vtss_ifindex_t ifindex, const BOOL *start)
{
    vtss_ifindex_elm_t ife;

    if (start && *start) {  // Make sure the Start is not a NULL pointer
        CapArray<port_veriphy_mode_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> mode;

        if (!vtss_ifindex_is_port(ifindex)) {
            return VTSS_RC_ERROR;
        }

        VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

        // Start by setting all ports to NOT run veriphy, because the mgmt
        // veriphy function takes a port boolean array.
        port_iter_t pit;
        VTSS_RC(port_iter_init(&pit, NULL, ife.isid, PORT_ITER_SORT_ORDER_UPORT,
                               PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_NPI));

        while (port_iter_getnext(&pit)) {
            mode[pit.iport] = PORT_VERIPHY_MODE_NONE;
        }

        meba_port_cap_t port_cap = 0;
        VTSS_RC_ERR_PRINT(port_cap_get(ife.ordinal, &port_cap));
        // Start veriphy at the selected port.
        mode[ife.ordinal] = (port_cap == 0 || (port_cap & MEBA_PORT_CAP_1G_PHY) == 0) ? PORT_VERIPHY_MODE_NONE : PORT_VERIPHY_MODE_FULL;

        VTSS_RC(port_veriphy_start(mode.data()));
    }

    return VTSS_RC_OK;
}

// Wrapper for starting VeriPHy, converting from ifindex to isid, port
// In : ifindex - OID ifindex
//      start   - Set to TRUE to start VeriPhy for the select ifindex interface
mesa_rc port_expose_veriphy_result_get(vtss_ifindex_t ifindex, vtss_appl_port_veriphy_result_t *port_result)
{
    mepa_cable_diag_result_t phy_result;
    vtss_ifindex_elm_t       ife;

    if (!vtss_ifindex_is_port(ifindex)) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    (void)port_veriphy_result_get(ife.ordinal, &phy_result, 1);  // Run for maximum 1 sec.
    T_NG_PORT(PORT_TRACE_GRP_MIB, ife.ordinal,
              "Getting VeriPhy result, phy_result.status[0]:%d",
              phy_result.status[0]);
    T_NG_PORT(PORT_TRACE_GRP_MIB, ife.ordinal,
              "Getting VeriPhy result, phy_result.status[1]:%d",
              phy_result.status[1]);
    T_NG_PORT(PORT_TRACE_GRP_MIB, ife.ordinal,
              "Getting VeriPhy result, phy_result.status[2]:%d",
              phy_result.status[2]);
    T_NG_PORT(PORT_TRACE_GRP_MIB, ife.ordinal,
              "Getting VeriPhy result, phy_result.status[3]:%d",
              phy_result.status[3]);

    port_result->status_pair_a = phy_result.status[0];
    port_result->status_pair_b = phy_result.status[1];
    port_result->status_pair_c = phy_result.status[2];
    port_result->status_pair_d = phy_result.status[3];
    port_result->length_pair_a = phy_result.length[0];
    port_result->length_pair_b = phy_result.length[1];
    port_result->length_pair_c = phy_result.length[2];
    port_result->length_pair_d = phy_result.length[3];
    return VTSS_RC_OK;
}

#endif // VTSS_UI_OPT_VERIPHY

// Wrapper for clear counters, converting from ifindex to isid, port
// In : ifindex - OID ifindex
//      clear   - Set to TRUE to clear counters for the select ifindex interface
mesa_rc port_stats_clr_set(vtss_ifindex_t ifindex, const BOOL *const clear)
{
    if (clear && *clear) {  // Make sure the Clear is not a NULL pointer
        if (!vtss_ifindex_is_port(ifindex)) {
            return VTSS_RC_ERROR;
        }

        VTSS_RC(vtss_appl_port_statistics_clear(ifindex));
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// port_expose_port_conf_get()
// Converts from real port configuration to backwards-compatbile
// port_expose_port_conf_t.
/******************************************************************************/
mesa_rc port_expose_port_conf_get(vtss_ifindex_t ifindex, port_expose_port_conf_t *port_expose_conf)
{
    uint8_t               q;
    vtss_appl_port_conf_t port_conf;

    if (!port_expose_conf) {
        return VTSS_RC_ERROR;
    }

    // vtss_appl_port_conf_get() can both handle normal ports and CPU ports and
    // returns an error if ifindex is not of supported type
    VTSS_RC(vtss_appl_port_conf_get(ifindex, &port_conf));

    vtss_clear(*port_expose_conf);
    port_expose_conf->shutdown         = !port_conf.admin.enable;
    port_expose_conf->advertise_dis    = port_conf.adv_dis;
    port_expose_conf->media            = port_conf.media_type;
    port_expose_conf->fc               = port_conf.flow_control ? PORT_EXPOSE_FC_ON : PORT_EXPOSE_FC_OFF;
    port_expose_conf->mtu              = port_conf.max_length;
    port_expose_conf->excessive        = port_conf.exc_col_cont;
    port_expose_conf->frame_length_chk = port_conf.frame_length_chk;
    port_expose_conf->force_clause_73  = port_conf.force_clause_73;
    port_expose_conf->fec_mode         = port_conf.fec_mode;

    for (port_expose_conf->pfc_mask = 0, q = 0; q < ARRSZ(port_conf.pfc); q++) {
        port_expose_conf->pfc_mask |= port_conf.pfc[q] ? (1 << q) : 0;
    }

    if (port_conf.speed == MESA_SPEED_AUTO) {
        port_expose_conf->speed = PORT_EXPOSE_SPEED_DUPLEX_AUTO;
    } else if (port_conf.speed ==  MESA_SPEED_1G) {
        port_expose_conf->speed = PORT_EXPOSE_SPEED_DUPLEX_1G_FDX;
    } else if (port_conf.speed ==  MESA_SPEED_10M) {
        port_expose_conf->speed = port_conf.fdx ? PORT_EXPOSE_SPEED_DUPLEX_10M_FDX  : PORT_EXPOSE_SPEED_DUPLEX_10M_HDX;
    } else if (port_conf.speed ==  MESA_SPEED_100M) {
        port_expose_conf->speed = port_conf.fdx ? PORT_EXPOSE_SPEED_DUPLEX_100M_FDX : PORT_EXPOSE_SPEED_DUPLEX_100M_HDX;
    } else if (port_conf.speed ==  MESA_SPEED_2500M) {
        port_expose_conf->speed = PORT_EXPOSE_SPEED_DUPLEX_2500M_FDX;
    } else if (port_conf.speed ==  MESA_SPEED_5G) {
        port_expose_conf->speed = PORT_EXPOSE_SPEED_DUPLEX_5G_FDX;
    } else if (port_conf.speed ==  MESA_SPEED_10G) {
        port_expose_conf->speed = PORT_EXPOSE_SPEED_DUPLEX_10G_FDX;
    } else if (port_conf.speed ==  MESA_SPEED_12G) {
        port_expose_conf->speed = PORT_EXPOSE_SPEED_DUPLEX_12G_FDX;
    } else if (port_conf.speed ==  MESA_SPEED_25G) {
        port_expose_conf->speed = PORT_EXPOSE_SPEED_DUPLEX_25G_FDX;
    } else {
        T_EG(PORT_TRACE_GRP_MIB, "%s: Unexpected port configuration: speed = %d, duplex = %d, adv_dis = 0x%x", ifindex, port_conf.speed, port_conf.fdx, port_conf.adv_dis);
        return VTSS_RC_ERROR;
    }

    T_DG(PORT_TRACE_GRP_MIB, "%s: Port configuration: <speed = %d, duplex = %d> maps to speed = %d", ifindex, port_conf.speed, port_conf.fdx, port_expose_conf->speed);

    return VTSS_RC_OK;
}

/******************************************************************************/
// port_expose_port_conf_set()
// Converts from backwards-compatbile port_expose_port_conf_t to real port
// configuration.
/******************************************************************************/
mesa_rc port_expose_port_conf_set(vtss_ifindex_t ifindex, const port_expose_port_conf_t *port_expose_conf)
{
    vtss_appl_port_conf_t port_conf;
    vtss_ifindex_elm_t    ife;
    uint8_t               q;

    if (!port_expose_conf) {
        return VTSS_RC_ERROR;
    }

    // Get old conf.
    // vtss_appl_port_conf_get() can both handle normal ports and CPU ports and
    // returns an error if ifindex is not of supported type.
    VTSS_RC(vtss_appl_port_conf_get(ifindex, &port_conf));

    port_conf.admin.enable     = !port_expose_conf->shutdown;
    port_conf.fdx              = true; // For now. May be changed later.
    port_conf.flow_control     = port_expose_conf->fc == PORT_EXPOSE_FC_ON;
    port_conf.media_type       = port_expose_conf->media;
    port_conf.max_length       = port_expose_conf->mtu;
    port_conf.exc_col_cont     = port_expose_conf->excessive;
    port_conf.adv_dis          = port_expose_conf->advertise_dis;
    port_conf.frame_length_chk = port_expose_conf->frame_length_chk;
    port_conf.force_clause_73  = port_expose_conf->force_clause_73;
    port_conf.fec_mode         = port_expose_conf->fec_mode;

    switch (port_expose_conf->speed) {
    case PORT_EXPOSE_SPEED_DUPLEX_AUTO:
        port_conf.speed = MESA_SPEED_AUTO;
        break;

    case PORT_EXPOSE_SPEED_DUPLEX_1G_FDX:
        port_conf.speed = MESA_SPEED_1G;
        break;

    case PORT_EXPOSE_SPEED_DUPLEX_10M_FDX:
        port_conf.speed = MESA_SPEED_10M;
        break;

    case PORT_EXPOSE_SPEED_DUPLEX_10M_HDX:
        port_conf.speed = MESA_SPEED_10M;
        port_conf.fdx = false;
        break;

    case PORT_EXPOSE_SPEED_DUPLEX_100M_FDX:
        port_conf.speed = MESA_SPEED_100M;
        break;

    case PORT_EXPOSE_SPEED_DUPLEX_100M_HDX:
        port_conf.speed = MESA_SPEED_100M;
        port_conf.fdx = false;
        break;

    case PORT_EXPOSE_SPEED_DUPLEX_2500M_FDX:
        port_conf.speed = MESA_SPEED_2500M;
        break;

    case PORT_EXPOSE_SPEED_DUPLEX_5G_FDX:
        port_conf.speed = MESA_SPEED_5G;
        break;

    case PORT_EXPOSE_SPEED_DUPLEX_10G_FDX:
        port_conf.speed = MESA_SPEED_10G;
        break;

    case PORT_EXPOSE_SPEED_DUPLEX_12G_FDX:
        port_conf.speed = MESA_SPEED_12G;
        break;

    case PORT_EXPOSE_SPEED_DUPLEX_25G_FDX:
        port_conf.speed = MESA_SPEED_25G;
        break;
    }

    // Wrapper for Priority Flow control
    for (q = 0; q < ARRSZ(port_conf.pfc); q++) {
        port_conf.pfc[q] = (port_expose_conf->pfc_mask & (1 << q)) > 0 ? true : false;
    }

    VTSS_RC(vtss_ifindex_decompose(ifindex, &ife));

    T_IG(PORT_TRACE_GRP_MIB, "%s: speed = %d maps to <speed = %d, duplex = %d>", ifindex, port_expose_conf->speed, port_conf.speed, port_conf.fdx);

    // Set new configuration.
    // vtss_appl_port_conf_set() can handle both cpuports and normal ports.
    return vtss_appl_port_conf_set(ifindex, &port_conf);
}

// Getting the RFC2861 counter from the port counters
mesa_rc port_if_group_statistics_get(vtss_ifindex_t ifindex, mesa_port_if_group_counters_t *port_if_group_counters)
{
    mesa_port_counters_t counters;

    VTSS_RC(vtss_appl_port_statistics_get(ifindex, &counters));

    *port_if_group_counters = counters.if_group;
    return VTSS_RC_OK;
}

/**
 * \brief Getting bridge Interface counters */
mesa_rc port_bridge_statistics_get(vtss_ifindex_t ifindex, mesa_port_bridge_counters_t *bridge_counters)
{
    mesa_port_counters_t counters;

    VTSS_RC(vtss_appl_port_statistics_get(ifindex, &counters));

    *bridge_counters = counters.bridge;
    return VTSS_RC_OK;
}

/**
 * \brief Getting Ethernet-like Interface counters */
mesa_rc port_ethernet_like_statistics_get(vtss_ifindex_t ifindex, mesa_port_ethernet_like_counters_t *eth_counters)
{
    mesa_port_counters_t counters;

    VTSS_RC(vtss_appl_port_statistics_get(ifindex, &counters));

    *eth_counters = counters.ethernet_like;
    return VTSS_RC_OK;
}

/**
 * \brief Getting queues counters */
mesa_rc port_expose_qu_statistics_get(vtss_ifindex_t ifindex, mesa_prio_t prio, port_expose_prio_counter_t *prio_counters)
{
    mesa_port_counters_t counters;

    VTSS_RC(vtss_appl_port_statistics_get(ifindex, &counters));

    prio_counters->rx_prio = counters.prio[prio].rx;
    prio_counters->tx_prio = counters.prio[prio].tx;
    return VTSS_RC_OK;
}

// Getting the RMON part of the counter from the port counters
mesa_rc port_rmon_statistics_get(vtss_ifindex_t ifindex, mesa_port_rmon_counters_t *rmon_statistics)
{
    mesa_port_counters_t counters;

    VTSS_RC(vtss_appl_port_statistics_get(ifindex, &counters));

    *rmon_statistics = counters.rmon;
    return VTSS_RC_OK;
}

// Getting the 802.3br part of the counter from the port counters
mesa_rc port_dot3br_statistics_get(vtss_ifindex_t ifindex, mesa_port_dot3br_counters_t *dot3br_statistics)
{
#if defined(VTSS_SW_OPTION_QOS_ADV)
    mesa_port_counters_t counters;

    if (!fast_cap(MESA_CAP_QOS_FRAME_PREEMPTION)) {
        return VTSS_APPL_PORT_RC_GEN; // Feature not suported
    }

    VTSS_RC(vtss_appl_port_statistics_get(ifindex, &counters));

    *dot3br_statistics = counters.dot3br;
    return VTSS_RC_OK;
#else
    return VTSS_APPL_PORT_RC_GEN; // Feature not suported
#endif
}

mesa_rc port_expose_interface_portno_get(vtss_ifindex_t ifindex, mesa_port_no_t *port_no)
{
    mesa_rc rc;
    vtss_ifindex_elm_t ife;

    if (vtss_ifindex_is_port(ifindex)) {

        if ((rc = vtss_ifindex_decompose(ifindex, &ife)) != VTSS_RC_OK) {
            return rc;
        }

        *port_no = iport2uport(ife.ordinal);
        return VTSS_RC_OK;
    }

    if (vtss_ifindex_is_cpu(ifindex)) {

        if ((rc = vtss_ifindex_decompose(ifindex, &ife)) != VTSS_RC_OK) {
            return rc;
        }

        *port_no = port_count_max() + iport2uport(ife.ordinal);
        return VTSS_RC_OK;
    }

    return VTSS_RC_ERROR;
}

