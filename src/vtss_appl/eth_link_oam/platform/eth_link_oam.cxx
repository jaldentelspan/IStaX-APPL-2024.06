/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "main.h"                       /* For Link OAM module initialization hookups */
#include "conf_api.h"                   /* Get the system MAC address */
#include "port_api.h"                   /* For port events callbacks */
#include "packet_api.h"                 /* For OAM frames handlers   */
#include "eth_link_oam_api.h"           /* For ETH Link OAM module   */
#include "main_types.h"
#include "acl_api.h"                    /* For ACE management APIs   */
#include "misc_api.h"                   /* instantiate MAC */
#ifdef VTSS_SW_OPTION_ICFG
#include "eth_link_oam_icfg.h"
#endif
#include "netdb.h"                      /* For byte-order swap functions */
#include "microchip/ethernet/switch/api.h"

#include "vtss_common_iterator.hxx"
#include "vtss/basics/expose/table-status.hxx" // For vtss::expose::TableStatus
#include "vtss/basics/memcmp-operator.hxx"  // For VTSS_BASICS_MEMCMP_OPERATOR
#include "vtss_safe_queue.hxx"

#include "control_api.h"        /* To be able to register callback for system reset */

/*lint -sem( vtss_eth_link_oam_crit_oper_data_lock, thread_lock ) */
/*lint -sem( vtss_eth_link_oam_crit_oper_data_unlock, thread_unlock ) */

/* ================================================================= *
 *  Trace definitions
 * ================================================================= */

#include <vtss_module_id.h>             /* For Link OAM module ID */
#include <vtss_trace_lvl_api.h>

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_ETH_LINK_OAM
#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_ETH_LINK_OAM

#include <vtss_trace_api.h>
#ifdef VTSS_SW_OPTION_SNMP
#include "vtss_snmp_api.h"
#endif

#include "ip_dying_gasp_api.hxx"

/*****************************************************************************/
/*  Static declarations                                                      */
/*****************************************************************************/
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
static void eth_link_oam_port_local_lost_link_timer_done(mesa_port_no_t port_no);
#endif

/****************************************************************************/
/*  Global variables                                                        */
/****************************************************************************/

// Filter id for subscribing Link OAM frame.
void                          *eth_link_oam_filter_id = NULL;

/* Global module/API Lock for Critical OAM data protection */
critd_t                            oam_data_lock;

/* Module Lock for critical OAM operaional data(OAM client data) protection */
critd_t                            oam_oper_lock;

/* Module Lock for critical OAM control layer date(OAM control layer) protection */
critd_t                            oam_control_layer_lock;

static vtss_trace_reg_t trace_reg = {
    VTSS_TRACE_MODULE_ID, "LINK_OAM", "Eth Link OAM module."
};

static vtss_trace_grp_t trace_grps[] = {
    [VTSS_TRACE_GRP_DEFAULT] = {
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define CRIT_ENTER(crit) critd_enter(&crit, __FILE__, __LINE__)
#define CRIT_EXIT(crit)  critd_exit(&crit,  __FILE__, __LINE__)

static u8 npi_encap[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0x88, 0x80, 0x00, 0x00
};

#define VTSS_PROTO    0x8880            // IFH 'long' encapsulation
#define NPI_ENCAP_LEN sizeof(npi_encap) // DA = BC, SA, ETYPE = 88:80, 00:05

/****************************************************************************/
/*  Various local functions                                                 */
/****************************************************************************/
static vtss_handle_t   eth_link_oam_control_timer_thread_handle;
static vtss_thread_t   eth_link_oam_control_timer_thread_block;

static vtss_handle_t   eth_link_oam_client_thread_handle;
static vtss_thread_t   eth_link_oam_client_thread_block;

/* Queue Variables */
static vtss::SafeQueue eth_link_oam_queue;
static vtss_sem_t      eth_link_oam_var_sem;
static vtss_sem_t      eth_link_oam_rlb_sem;

static u8              ETH_LINK_OAM_MULTICAST_MACADDR[] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x02 };
static u8              sys_mac_addr[VTSS_ETH_LINK_OAM_MAC_LEN];
static CapArray<u8 *, MEBA_CAP_BOARD_PORT_MAP_COUNT> oam_dying_gasp_pdu;

/* JSON notifications  */
VTSS_BASICS_MEMCMP_OPERATOR(vtss_appl_eth_link_oam_crit_link_event_statistics_t);
vtss::expose::TableStatus <
vtss::expose::ParamKey<vtss_ifindex_t>,
     vtss::expose::ParamVal<vtss_appl_eth_link_oam_crit_link_event_statistics_t *>
     > eth_link_oam_crit_event_update("eth_link_oam_crit_event_update", VTSS_MODULE_ID_ETH_LINK_OAM);

static mesa_rc rc_conv(u32 oam_rc)
{
    mesa_rc rc = VTSS_RC_ERROR;

    switch (oam_rc) {
    case VTSS_ETH_LINK_OAM_RC_OK:
        rc = VTSS_RC_OK;
        break;
    case VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER:
        rc = ETH_LINK_OAM_RC_INVALID_PARAMETER;
        break;
    case VTSS_ETH_LINK_OAM_RC_NOT_ENABLED:
        rc = ETH_LINK_OAM_RC_NOT_ENABLED;
        break;
    case VTSS_ETH_LINK_OAM_RC_ALREADY_CONFIGURED:
        rc = ETH_LINK_OAM_RC_ALREADY_CONFIGURED;
        break;
    case VTSS_ETH_LINK_OAM_RC_NO_MEMORY:
        rc = ETH_LINK_OAM_RC_NO_MEMORY;
        break;
    case VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED:
        rc = ETH_LINK_OAM_RC_NOT_SUPPORTED;
        break;
    case VTSS_ETH_LINK_OAM_RC_NO_SUFFIFIENT_MEMORY:
        rc = ETH_LINK_OAM_RC_NO_SUFFIFIENT_MEMORY;
        break;
    case VTSS_ETH_LINK_OAM_RC_INVALID_STATE:
        rc = ETH_LINK_OAM_RC_INVALID_STATE;
        break;
    case VTSS_ETH_LINK_OAM_RC_INVALID_FLAGS:
        rc = ETH_LINK_OAM_RC_INVALID_FLAGS;
        break;
    case VTSS_ETH_LINK_OAM_RC_INVALID_CODES:
        rc = ETH_LINK_OAM_RC_INVALID_CODES;
        break;
    case VTSS_ETH_LINK_OAM_RC_INVALID_PDU_CNT:
        rc = ETH_LINK_OAM_RC_INVALID_PDU_CNT;
        break;
    case VTSS_ETH_LINK_OAM_RC_TIMED_OUT:
        rc = ETH_LINK_OAM_RC_TIMED_OUT;
        break;
    default:
        T_E("Invalid OAM module error is noticed:- %u", oam_rc);
        break;
    }
    return rc;
}

/* Enables/Disables the OAM Control on the port                               */
mesa_rc eth_link_oam_mgmt_port_control_conf_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const vtss_eth_link_oam_control_t conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_control_conf_set(l2port, conf));
    return rc;
}

/* Retrieve the OAM Control on the port                                     */
mesa_rc eth_link_oam_mgmt_port_control_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_control_t *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_control_conf_get(l2port, conf));
    return rc;
}

/* Configures the Port's OAM mode                                   */
mesa_rc eth_link_oam_mgmt_port_mode_conf_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const vtss_eth_link_oam_mode_t conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_mode_conf_set(l2port, conf, FALSE));
    return rc;
}

/* Retrieves the Link OAM port mode                                   */
mesa_rc eth_link_oam_mgmt_port_mode_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_mode_t *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(vtss_eth_link_oam_mgmt_port_mode_conf_get(l2port, conf));
    return rc;
}

/* Configures the Port's MIB retrieval support */
mesa_rc eth_link_oam_mgmt_port_mib_retrieval_conf_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const BOOL conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(vtss_eth_link_oam_mgmt_port_mib_retrieval_conf_set(l2port, conf));
    return rc;
}

/* Retrieves the port's MIB retrieval support                                   */
mesa_rc eth_link_oam_mgmt_port_mib_retrieval_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, BOOL *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(vtss_eth_link_oam_mgmt_port_mib_retrieval_conf_get(l2port, conf));
    return rc;
}

/* Configures the Port's remote loopback support */
mesa_rc eth_link_oam_mgmt_port_remote_loopback_conf_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const BOOL conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_remote_loopback_conf_set(l2port, conf, FALSE));
    return rc;
}

/* Retrieves the port's remote loopback  support                                   */
mesa_rc eth_link_oam_mgmt_port_remote_loopback_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, BOOL *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(vtss_eth_link_oam_mgmt_port_remote_loopback_conf_get(l2port, conf));
    return rc;
}

/* Configures the Port's link monitoring support */
mesa_rc eth_link_oam_mgmt_port_link_monitoring_conf_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const BOOL conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(vtss_eth_link_oam_mgmt_port_link_monitoring_conf_set(l2port, conf));
    return rc;
}

/* Retrieves the port's remote loopback  support                                   */
mesa_rc eth_link_oam_mgmt_port_link_monitoring_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, BOOL *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(vtss_eth_link_oam_mgmt_port_link_monitoring_conf_get(
                     l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Event Window                           */
/* Configures the Error Frame Event Window Configuration                      */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_window_set (const vtss_isid_t isid, const u32  port_no, const u16 conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_error_frame_window_set(
                 l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Event Window                           */
/* Retrieves the Error Frame Event Window Configuration                       */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_window_get (const vtss_isid_t isid, const u32  port_no, u16 *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_error_frame_window_get(
                 l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Event Threshold                        */
/* Configures the Error Frame Event Threshold Configuration                   */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_threshold_set(const vtss_isid_t isid, const u32  port_no, const u32 conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(vtss_eth_link_oam_mgmt_port_link_error_frame_threshold_set(
                     l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Event Threshold                        */
/* Retrieves the Error Frame Event Threshold Configuration                    */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_threshold_get (const vtss_isid_t isid, const u32  port_no, u32 *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_error_frame_threshold_get(
                 l2port, conf));
    return rc;

}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error Window                         */
/* Configures the Symbol Period Error Window Configuration                    */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_window_set (const vtss_isid_t isid, const u32  port_no, const u64  conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_symbol_period_error_window_set(
                 l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error Window                         */
/* Retrieves the Symbol Period Error Window Configuration                     */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_window_get (const vtss_isid_t isid, const u32  port_no, u64 *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_symbol_period_error_window_get(
                 l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error Period Threshold               */
/* Configures the Symbol Period Error Period Threshold Configuration          */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_threshold_set (const vtss_isid_t isid, const u32  port_no, const u64  conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_symbol_period_error_threshold_set(
                 l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error Period Threshold               */
/* Retrieves the Symbol Period Error Period Threshold Configuration           */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_threshold_get (const vtss_isid_t isid, const u32  port_no, u64 *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_symbol_period_error_threshold_get(
                 l2port, conf));
    return rc;

}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error RxPackets Threshold            */
/* Configures the Symbol Period Error RxPackets Threshold Configuration       */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_rxpackets_threshold_set (const vtss_isid_t isid, const u32  port_no, const u64  conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_symbol_period_error_rxpackets_threshold_set(
                 l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Symbol Period Error RxPackets Threshold            */
/* Retreives the Symbol Period Error RxPackets Threshold Configuration        */
mesa_rc eth_link_oam_mgmt_port_link_symbol_period_error_rxpackets_threshold_get (const vtss_isid_t isid, const u32  port_no, u64 *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_symbol_period_error_rxpackets_threshold_get(
                 l2port, conf));
    return rc;
}

/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error Window                          */
/* Configures the Frame Period Error Window Configuration                     */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_window_set (const vtss_isid_t isid, const u32  port_no, const u32 conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_frame_period_error_window_set(
                 l2port, conf));
    return rc;
}

/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error Window                          */
/* Retrieves the Frame Period Error Window Configuration                      */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_window_get (const vtss_isid_t isid, const u32  port_no, u32 *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_frame_period_error_window_get(
                 l2port, conf));
    return rc;
}

/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error Period Threshold                */
/* Configures the Frame Period Error Period Threshold Configuration           */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_threshold_set (const vtss_isid_t isid, const u32  port_no, const u32  conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_frame_period_error_threshold_set(
                 l2port, conf));
    return rc;
}

/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error Period Threshold                */
/* Retrieves the Frame Period Error Period Threshold Configuration            */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_threshold_get (const vtss_isid_t isid, const u32  port_no, u32        *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_frame_period_error_threshold_get(
                 l2port, conf));
    return rc;
}

/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error RxPackets Threshold             */
/* Configures the Frame Period Error RxPackets Threshold Configuration        */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_rxpackets_threshold_set (const vtss_isid_t isid, const u32  port_no, const u64  conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_frame_period_error_rxpackets_threshold_set(
                 l2port, conf));
    return rc;
}

/* isid   :    slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Frame Period Error RxPackets Threshold             */
/* Retrieves the Frame Period Error RxPackets Threshold Configuration         */
mesa_rc eth_link_oam_mgmt_port_link_frame_period_error_rxpackets_threshold_get (const vtss_isid_t isid, const u32  port_no, u64 *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_frame_period_error_rxpackets_threshold_get(
                 l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Seconds Summary Event Window           */
/* Configures the Error Frame Seconds Summary Event Window Configuration      */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_set (const vtss_isid_t isid, const u32  port_no, const u16 conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_set(
                 l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Seconds Summary Event Window           */
/* Retrieves the Error Frame Seconds Summary Event Window Configuration       */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_get (const vtss_isid_t isid, const u16  port_no, u16 *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_get(
                 l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Seconds Summary Event Threshold        */
/* Configures the Error Frame Seconds Summary Event Threshold Configuration   */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_secs_summary_threshold_set(const vtss_isid_t isid, const u32  port_no, const u16 conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_error_frame_secs_summary_threshold_set(
                 l2port, conf));
    return rc;
}

/* isid   :    Slot Number                                                    */
/* port_no:    l2 port number                                                 */
/* conf:       Port's link Error Frame Seconds Summary Event Threshold        */
/* Retrieves the Error Frame Seconds Summary Event Threshold Configuration    */
mesa_rc eth_link_oam_mgmt_port_link_error_frame_secs_summary_threshold_get (const vtss_isid_t isid, const u16  port_no, u16 *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_port_link_error_frame_secs_summary_threshold_get(
                 l2port, conf));
    return rc;

}
/* End */
/* Configures the Port's MIB retrieval support */
mesa_rc eth_link_oam_mgmt_port_mib_retrieval_oper_set (const vtss_isid_t isid, const mesa_port_no_t port_no, const u8 conf)

{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;
    u8                       oam_pdu[VTSS_ETH_LINK_OAM_PDU_MIN_LEN];
    u8                       var_branch = VTSS_ETH_LINK_OAM_VAR_ATTRIBUTE;
    u16                      var_leaf[5];
    u16                      current_len = VTSS_ETH_LINK_OAM_PDU_HDR_LEN;
    u8                       tmp_index = 0, total_leaves = 0;
    vtss_eth_link_oam_mode_t oam_conf;
    BOOL                     mib_ret_support = FALSE;

    do {

        rc = vtss_eth_link_oam_mgmt_port_mode_conf_get(l2port, &oam_conf);

        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_E("Unable to retrieve the mode of the port(%d/%u)", isid, port_no);
            break;
        }

        if (oam_conf == VTSS_APPL_ETH_LINK_OAM_MODE_PASSIVE) {
            rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
            break;
        }

        rc = vtss_eth_link_oam_mgmt_port_mib_retrieval_conf_get(l2port,
                                                                &mib_ret_support);

        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_E("Unable to retrieve the configuration of the port(%d/%u)", isid, port_no);
            break;
        }

        if (mib_ret_support == FALSE) {
            T_D("MIB retrieval support is not supported on port(%d/%u)", isid, port_no);
            rc = VTSS_ETH_LINK_OAM_RC_NOT_ENABLED;
            break;
        }

        if (conf == 1) { /* Local_info */
            var_leaf[0] = VTSS_ETH_LINK_OAM_ID;
            var_leaf[1] = VTSS_ETH_LINK_OAM_LOCAL_CONF;
            var_leaf[2] = VTSS_ETH_LINK_OAM_LOCAL_PDU_CONF;
            var_leaf[3] = VTSS_ETH_LINK_OAM_LOCAL_REVISION_CONF;
            var_leaf[4] = VTSS_ETH_LINK_OAM_LOCAL_STATE;
            total_leaves = 5;
        } else { /* remote info */
            var_leaf[0] = VTSS_ETH_LINK_OAM_REMOTE_CONF;
            var_leaf[1] = VTSS_ETH_LINK_OAM_REMOTE_PDU_CONF;
            var_leaf[2] = VTSS_ETH_LINK_OAM_REMOTE_REVISION;
            var_leaf[3] = VTSS_ETH_LINK_OAM_REMOTE_STATE;
            total_leaves = 4;
        }

        memset(oam_pdu, '\0', VTSS_ETH_LINK_OAM_PDU_MIN_LEN);

        for (tmp_index = 0; tmp_index < total_leaves; tmp_index++) {

            rc = vtss_eth_link_oam_client_port_build_var_descriptor(l2port,
                                                                    oam_pdu,
                                                                    var_branch,
                                                                    var_leaf[tmp_index],
                                                                    &current_len,
                                                                    VTSS_ETH_LINK_OAM_PDU_MIN_LEN);
            if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
                T_E("Error:%u occured while building the MIB variable of the port(%d/%u)",
                    rc, isid, port_no);
                break;
            }
        }
        rc = vtss_eth_link_oam_mgmt_control_port_non_info_pdu_xmit(port_no,
                                                                   oam_pdu,
                                                                   VTSS_ETH_LINK_OAM_PDU_MIN_LEN,
                                                                   VTSS_ETH_LINK_OAM_CODE_TYPE_VAR_REQ);

        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_E("Error:%u occured while building MIB variable on port(%d/%u)",
                rc, isid, port_no);
            break;
        }

    } while (VTSS_ETH_LINK_OAM_NULL);

    return (rc_conv(rc));
}

mesa_rc eth_link_oam_mgmt_port_remote_loopback_oper_conf_set(const vtss_isid_t isid, const mesa_port_no_t port_no, const BOOL conf)
{

    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(vtss_eth_link_oam_mgmt_port_remote_loopback_oper_conf_set(l2port, conf));

    if (rc == VTSS_RC_OK) {
        T_D("Remote loopback operation is enabled on port(%u/%u)", isid, port_no);
        if (conf == FALSE) {
            rc = rc_conv(vtss_eth_link_oam_mgmt_port_state_conf_set(l2port,
                                                                    VTSS_ETH_LINK_OAM_MUX_FWD_STATE,
                                                                    VTSS_ETH_LINK_OAM_PARSER_FWD_STATE));
        }
    } else if (rc == ETH_LINK_OAM_RC_TIMED_OUT) {
        T_D("Remote loopback operation is timed out on port(%u/%u)", isid, port_no);
    }
    return rc;
}

/*****************************************************************************/
/* New public APIs based on public header                                    */
/*****************************************************************************/

/**
 * Set Link OAM port configuration.
 * ifIndex  [IN]: Interface index.
 * conf     [IN]: Link OAM port configurable parameters.
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_conf_set(
    vtss_ifindex_t                            ifIndex,
    const vtss_appl_eth_link_oam_port_conf_t  *const conf)
{
    vtss_ifindex_elm_t                 ife;
    vtss_appl_eth_link_oam_port_conf_t old_conf;

    if (conf == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }
    memset(&old_conf, 0, sizeof(vtss_appl_eth_link_oam_port_conf_t));

    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    VTSS_RC(eth_link_oam_mgmt_port_control_conf_get(ife.isid, ife.ordinal, &old_conf.admin_state))
    if (old_conf.admin_state != conf->admin_state) {
        VTSS_RC(eth_link_oam_mgmt_port_control_conf_set(ife.isid, ife.ordinal, conf->admin_state))
    }

    VTSS_RC(eth_link_oam_mgmt_port_mode_conf_get(ife.isid, ife.ordinal, &old_conf.mode))
    if (old_conf.mode != conf->mode) {
        VTSS_RC(eth_link_oam_mgmt_port_mode_conf_set(ife.isid, ife.ordinal, conf->mode))
    }

    VTSS_RC(eth_link_oam_mgmt_port_mib_retrieval_conf_get(
                ife.isid, ife.ordinal, &old_conf.mib_retrieval_support))
    if (old_conf.mib_retrieval_support != conf->mib_retrieval_support) {
        VTSS_RC(eth_link_oam_mgmt_port_mib_retrieval_conf_set(
                    ife.isid, ife.ordinal, conf->mib_retrieval_support))
    }

    VTSS_RC(eth_link_oam_mgmt_port_remote_loopback_conf_get(
                ife.isid, ife.ordinal, &old_conf.remote_loopback_support))
    if (old_conf.remote_loopback_support != conf->remote_loopback_support) {
        VTSS_RC(eth_link_oam_mgmt_port_remote_loopback_conf_set(
                    ife.isid, ife.ordinal, conf->remote_loopback_support))
    }

    VTSS_RC(eth_link_oam_mgmt_port_link_monitoring_conf_get(
                ife.isid, ife.ordinal, &old_conf.link_monitoring_support))
    if (old_conf.link_monitoring_support != conf->link_monitoring_support) {
        VTSS_RC(eth_link_oam_mgmt_port_link_monitoring_conf_set(
                    ife.isid, ife.ordinal, conf->link_monitoring_support))
    }

    return VTSS_RC_OK;
}

/**
 * Get Link OAM port configuration.
 * ifIndex   [IN]: Interface index.
 * conf     [OUT]: Link OAM port configurable parameters.
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_conf_get(
    vtss_ifindex_t                     ifIndex,
    vtss_appl_eth_link_oam_port_conf_t *const conf
)
{
    vtss_ifindex_elm_t  ife;

    if (conf == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    VTSS_RC(eth_link_oam_mgmt_port_control_conf_get(ife.isid, ife.ordinal, &conf->admin_state))

    VTSS_RC(eth_link_oam_mgmt_port_mode_conf_get(ife.isid, ife.ordinal, &conf->mode))

    VTSS_RC(eth_link_oam_mgmt_port_mib_retrieval_conf_get(
                ife.isid, ife.ordinal, &conf->mib_retrieval_support))

    VTSS_RC(eth_link_oam_mgmt_port_remote_loopback_conf_get(
                ife.isid, ife.ordinal, &conf->remote_loopback_support))

    VTSS_RC(eth_link_oam_mgmt_port_link_monitoring_conf_get(
                ife.isid, ife.ordinal, &conf->link_monitoring_support))

    return VTSS_RC_OK;
}

/**
 * Set Link OAM port event configuration.
 * ifIndex       [IN]: Interface index.
 * eventConf     [IN]: Link OAM port event configurable parameters.
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_event_conf_set(
    vtss_ifindex_t                                  ifIndex,
    const vtss_appl_eth_link_oam_port_event_conf_t  *const eventConf
)
{
    vtss_ifindex_elm_t  ife;

    if (eventConf == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    VTSS_RC(eth_link_oam_mgmt_port_link_error_frame_window_set(
                ife.isid, ife.ordinal, eventConf->error_frame_window))

    VTSS_RC(eth_link_oam_mgmt_port_link_error_frame_threshold_set(
                ife.isid, ife.ordinal, eventConf->error_frame_threshold))

    VTSS_RC(eth_link_oam_mgmt_port_link_symbol_period_error_window_set(
                ife.isid, ife.ordinal, eventConf->symbol_period_error_window))

    VTSS_RC(eth_link_oam_mgmt_port_link_symbol_period_error_threshold_set(
                ife.isid, ife.ordinal, eventConf->symbol_period_error_threshold))

    VTSS_RC(eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_set(
                ife.isid, ife.ordinal, eventConf->error_frame_second_summary_window))

    VTSS_RC(eth_link_oam_mgmt_port_link_error_frame_secs_summary_threshold_set(
                ife.isid, ife.ordinal, eventConf->error_frame_second_summary_threshold))

    return VTSS_RC_OK;
}

/**
 * Get Link OAM port event configuration.
 * ifIndex        [IN]: Interface index.
 * eventConf     [OUT]: Link OAM port event configurable parameters.
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_event_conf_get(
    vtss_ifindex_t                           ifIndex,
    vtss_appl_eth_link_oam_port_event_conf_t *const eventConf
)
{
    vtss_ifindex_elm_t  ife;

    if (eventConf == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    VTSS_RC(eth_link_oam_mgmt_port_link_error_frame_window_get(
                ife.isid, ife.ordinal, &eventConf->error_frame_window))

    VTSS_RC(eth_link_oam_mgmt_port_link_error_frame_threshold_get(
                ife.isid, ife.ordinal, &eventConf->error_frame_threshold))

    VTSS_RC(eth_link_oam_mgmt_port_link_symbol_period_error_window_get(
                ife.isid, ife.ordinal, &eventConf->symbol_period_error_window))

    VTSS_RC(eth_link_oam_mgmt_port_link_symbol_period_error_threshold_get(
                ife.isid, ife.ordinal, &eventConf->symbol_period_error_threshold))

    VTSS_RC(eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_get(
                ife.isid, ife.ordinal, &eventConf->error_frame_second_summary_window))

    VTSS_RC(eth_link_oam_mgmt_port_link_error_frame_secs_summary_threshold_get(
                ife.isid, ife.ordinal, &eventConf->error_frame_second_summary_threshold))

    return VTSS_RC_OK;
}

/**
 * Set Link OAM remote loopback test parameter.
 * ifIndex       [IN]: Interface index.
 * loopbackTest  [IN]: Link OAM remote loopback test parameters.
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_remote_loopback_test_set(
    vtss_ifindex_t                                       ifIndex,
    const vtss_appl_eth_link_oam_remote_loopback_test_t  *const loopbackTest
)
{
    vtss_ifindex_elm_t           ife;
    vtss_eth_link_oam_info_tlv_t local_info;
    BOOL                         loopback_test;

    if (loopbackTest == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    VTSS_RC(eth_link_oam_client_port_local_info_get(ife.isid, ife.ordinal, &local_info))

    loopback_test = (local_info.state & 3) ? TRUE : FALSE;

    if (loopback_test != loopbackTest->loopback_test) {
        VTSS_RC(eth_link_oam_mgmt_port_remote_loopback_oper_conf_set(
                    ife.isid, ife.ordinal, loopbackTest->loopback_test))
    }
    return VTSS_RC_OK;
}

/**
 * Get Link OAM remote loopback test parameter.
 * ifIndex        [IN]: Interface index.
 * loopbackTest  [OUT]: Link OAM remote loopback test parameters.
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_remote_loopback_test_get(
    vtss_ifindex_t                                ifIndex,
    vtss_appl_eth_link_oam_remote_loopback_test_t *const loopbackTest
)
{
    vtss_eth_link_oam_info_tlv_t local_info;
    vtss_ifindex_elm_t           ife;

    if (loopbackTest == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    VTSS_RC(eth_link_oam_client_port_local_info_get(ife.isid, ife.ordinal, &local_info))

    loopbackTest->loopback_test = (local_info.state & 3) ? TRUE : FALSE;

    return VTSS_RC_OK;
}

/**
 * Get Link OAM port statistics.
 * ifIndex     [IN]: Interface index.
 * stats      [OUT]: Port statistics.
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_statistics_get(
    vtss_ifindex_t                      ifIndex,
    vtss_appl_eth_link_oam_statistics_t *const stats
)
{
    vtss_ifindex_elm_t           ife;

    if (stats == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    VTSS_RC(eth_link_oam_control_layer_port_pdu_stats_get(
                ife.isid, ife.ordinal, stats))

    return VTSS_RC_OK;
}
/**
 * Function clearing Link OAM statistic counter for a specific port
 * ifIndex [IN]  The logical interface index/number.
 * return VTSS_RC_OK if counter was cleared else error code
 */
mesa_rc vtss_appl_eth_link_oam_counters_clear(vtss_ifindex_t ifIndex)
{
    vtss_ifindex_elm_t           ife;

    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    return (vtss_eth_link_oam_mgmt_control_clear_statistics(ife.ordinal) == VTSS_ETH_LINK_OAM_RC_OK
            ? VTSS_RC_OK : VTSS_RC_ERROR);
}
/**
 * Get Link OAM port critical link event statistics.
 * ifIndex               [IN]: Interface index.
 * statsCritLinkEvent   [OUT]: Port critical link event statistics.
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_eth_link_oam_port_critical_link_event_statistics_get(
    vtss_ifindex_t                                      ifIndex,
    vtss_appl_eth_link_oam_crit_link_event_statistics_t *const statsCritLinkEvent
)
{
    T_D("Getting link critical events");
    return eth_link_oam_crit_event_update.get(ifIndex, statsCritLinkEvent);
}

/** Get Link OAM port status.
  * ifIndex     [IN]: Interface index.
  * status     [OUT]: Port status.
  * return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_eth_link_oam_port_status_get(
    vtss_ifindex_t                       ifIndex,
    vtss_appl_eth_link_oam_port_status_t *const status
)
{
    vtss_ifindex_elm_t              ife;
    vtss_eth_link_oam_info_tlv_t    local_info;
    u16                             temp;
    char                            buf[80] = {0};
    if (status == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    VTSS_RC(eth_link_oam_control_layer_port_pdu_control_status_get(
                ife.isid, ife.ordinal, (vtss_eth_link_oam_pdu_control_t *)&status->pdu_control))

    VTSS_RC(eth_link_oam_control_layer_port_discovery_state_get(
                ife.isid, ife.ordinal, (vtss_eth_link_oam_discovery_state_t *)&status->discovery_state))

    VTSS_RC(eth_link_oam_client_port_local_info_get(
                ife.isid, ife.ordinal, &local_info))

    if (IS_CONF_ACTIVE(local_info.oam_conf, VTSS_ETH_LINK_OAM_CONF_UNI_DIRECTIONAL_SUPPORT)) {
        status->uni_dir_support = TRUE;
    } else {
        status->uni_dir_support = FALSE;
    }
    //MTU
    memcpy(&temp, local_info.oampdu_conf, sizeof(temp));
    status->mtu_size =  NET2HOSTS(temp);

    //mux state
    status->multiplexer_state = (vtss_appl_eth_link_oam_mux_state_t)((local_info.state & 4) >> 2);

    //parser state
    status->parser_state      = (vtss_appl_eth_link_oam_parser_state_t)(local_info.state & 3);

    //revision
    memcpy(&temp, local_info.revision, sizeof(temp));
    status->revision          = NET2HOSTS(temp);

    //oui
    sprintf(buf, "%02x:%02x:%02x", local_info.oui[0], local_info.oui[1], local_info.oui[2]);
    memcpy(&status->oui[0], &buf[0], VTSS_APPL_ETH_LINK_OAM_OUI_LEN);
    status->oui[8] = '\0';

    return VTSS_RC_OK;
}

/** Get Link OAM peer port status.
  * ifIndex     [IN]: Interface index.
  * peerStatus [OUT]: Peer port status.
  * return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_eth_link_oam_port_peer_status_get(
    vtss_ifindex_t                            ifIndex,
    vtss_appl_eth_link_oam_port_peer_status_t *const peerStatus
)
{
    vtss_ifindex_elm_t              ife;
    vtss_eth_link_oam_info_tlv_t    remote_info;
    u16                             remote_temp;
    char                            buf[80] = {0};
    u8                              remote_mac_addr[VTSS_COMMON_MACADDR_SIZE];

    if (peerStatus == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    VTSS_RC(eth_link_oam_client_port_remote_info_get(
                ife.isid, ife.ordinal, &remote_info))

    //remote mac addr
    VTSS_RC(eth_link_oam_client_port_remote_mac_addr_info_get(
                ife.isid, ife.ordinal, remote_mac_addr))

    memcpy(&peerStatus->peer_mac.addr[0], &remote_mac_addr[0], VTSS_COMMON_MACADDR_SIZE);
    //oam mode
    if (IS_CONF_ACTIVE(remote_info.oam_conf, VTSS_ETH_LINK_OAM_CONF_MODE)) {
        peerStatus->peer_oam_mode = VTSS_APPL_ETH_LINK_OAM_MODE_ACTIVE;
    } else {
        peerStatus->peer_oam_mode = VTSS_APPL_ETH_LINK_OAM_MODE_PASSIVE;
    }
    //uni dir support
    if (IS_CONF_ACTIVE(remote_info.oam_conf, VTSS_ETH_LINK_OAM_CONF_UNI_DIRECTIONAL_SUPPORT)) {
        peerStatus->peer_uni_dir_support = TRUE;
    } else {
        peerStatus->peer_uni_dir_support = FALSE;
    }
    //peer loopback support
    if (IS_CONF_ACTIVE(remote_info.oam_conf, VTSS_ETH_LINK_OAM_CONF_REMOTE_LOOP_BACK_CONTROL_SUPPORT)) {
        peerStatus->peer_loopback_support = TRUE;
    } else {
        peerStatus->peer_loopback_support = FALSE;
    }
    //link monitoring support
    if (IS_CONF_ACTIVE(remote_info.oam_conf, VTSS_ETH_LINK_OAM_CONF_LINK_EVENTS_SUPPORT)) {
        peerStatus->peer_link_monitoring_support = TRUE;
    } else {
        peerStatus->peer_link_monitoring_support = FALSE;
    }
    //mib retrieval support
    if (IS_CONF_ACTIVE(remote_info.oam_conf, VTSS_ETH_LINK_OAM_CONF_VARIABLE_RETRIEVAL_SUPPORT)) {
        peerStatus->peer_mib_retrieval_support = TRUE;
    } else {
        peerStatus->peer_mib_retrieval_support = FALSE;
    }
    //mtu size
    memcpy(&remote_temp, remote_info.oampdu_conf, sizeof(remote_temp));
    peerStatus->peer_mtu_size = NET2HOSTS(remote_temp);
    //revision
    memcpy(&remote_temp, remote_info.revision, sizeof(remote_temp));
    peerStatus->peer_pdu_revision = NET2HOSTS(remote_temp);
    //mux state
    peerStatus->peer_multiplexer_state = (vtss_appl_eth_link_oam_mux_state_t)((remote_info.state & 4) >> 2);
    //parser state
    peerStatus->peer_parser_state      = (vtss_appl_eth_link_oam_parser_state_t)(remote_info.state & 3);
    //oui
    sprintf(buf, "%02x:%02x:%02x", remote_info.oui[0], remote_info.oui[1], remote_info.oui[2]);
    memcpy(&peerStatus->peer_oui[0], &buf[0], VTSS_APPL_ETH_LINK_OAM_OUI_LEN);
    peerStatus->peer_oui[8] = '\0';

    return VTSS_RC_OK;
}
/** Get Link OAM port event link status.
  * ifIndex              [IN]: Interface index.
  * eventLinkStatus     [OUT]: Port event link status.
  * return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_eth_link_oam_port_link_event_status_get(
    vtss_ifindex_t                                  ifIndex,
    vtss_appl_eth_link_oam_port_link_event_status_t *const eventLinkStatus
)
{
    vtss_ifindex_elm_t                                      ife;
    vtss_eth_link_oam_error_frame_period_event_tlv_t        error_frame_period_tlv;
    vtss_eth_link_oam_error_frame_period_event_tlv_t        remote_error_frame_period_tlv;

    vtss_eth_link_oam_error_symbol_period_event_tlv_t       error_symbol_period_tlv;
    vtss_eth_link_oam_error_symbol_period_event_tlv_t       remote_error_symbol_period_tlv;

    vtss_eth_link_oam_error_frame_secs_summary_event_tlv_t  error_frame_secs_summary_tlv;
    vtss_eth_link_oam_error_frame_secs_summary_event_tlv_t  remote_frame_secs_summary_tlv;
    vtss_eth_link_oam_error_frame_event_tlv_t               error_frame_tlv;
    vtss_eth_link_oam_error_frame_event_tlv_t               remote_error_frame_tlv;
    u16                                                     temp16 = 0;
    u32                                                     temp32 = 0;
    u64                                                     temp64 = 0;

    if (eventLinkStatus == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    eventLinkStatus->sequence_number = 0;
    /*
    VTSS_RC(eth_link_oam_client_port_remote_seq_num_get(
                ife.isid, ife.ordinal, &eventLinkStatus->sequence_number))
    */
    VTSS_RC(eth_link_oam_client_port_symbol_period_error_info_get(
                ife.isid, ife.ordinal, &error_symbol_period_tlv, &remote_error_symbol_period_tlv))

    memcpy(&temp16, error_symbol_period_tlv.event_time_stamp,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_TIME_STAMP_LEN);
    eventLinkStatus->symbol_period_error_event_timestamp = NET2HOSTS(temp16);

    memcpy(&temp64, error_symbol_period_tlv.error_symbol_window,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_WINDOW_LEN);
    eventLinkStatus->symbol_period_error_event_window = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp64, error_symbol_period_tlv.error_symbol_threshold,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_THRESHOLD_LEN);
    eventLinkStatus->symbol_period_error_event_threshold = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp64, error_symbol_period_tlv.error_symbols,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_ERROR_SYMBOLS_LEN);
    eventLinkStatus->symbol_period_error = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp64, error_symbol_period_tlv.error_running_total,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_TOTAL_ERROR_SYMBOLS_LEN);
    eventLinkStatus->total_symbol_period_error = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp32, error_symbol_period_tlv.event_running_total,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_TOTAL_ERROR_EVENTS_LEN);
    eventLinkStatus->total_symbol_period_error_events = NET2HOSTL(temp32);

    VTSS_RC(eth_link_oam_client_port_frame_error_info_get(
                ife.isid, ife.ordinal, &error_frame_tlv, &remote_error_frame_tlv));

    memcpy(&temp16, error_frame_tlv.event_time_stamp,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_TIME_STAMP_LEN);
    eventLinkStatus->frame_error_event_timestamp = NET2HOSTS(temp16);

    memcpy(&temp16, error_frame_tlv.error_frame_window,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_WINDOW_LEN);
    eventLinkStatus->frame_error_event_window = NET2HOSTS(temp16);

    memcpy(&temp32, error_frame_tlv.error_frame_threshold,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_THRESHOLD_LEN);
    eventLinkStatus->frame_error_event_threshold  = NET2HOSTL(temp32);

    memcpy(&temp32, error_frame_tlv.error_frames,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_ERROR_FRAMES_LEN);
    eventLinkStatus->frame_error = NET2HOSTL(temp32);

    memcpy(&temp64, error_frame_tlv.error_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_TOTAL_ERROR_FRAMES_LEN);
    eventLinkStatus->total_frame_errors = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp32, error_frame_tlv.event_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_TOTAL_ERROR_EVENTS_LEN);
    eventLinkStatus->total_frame_errors_events  = NET2HOSTL(temp32);

    VTSS_RC(eth_link_oam_client_port_frame_period_error_info_get(
                ife.isid, ife.ordinal, &error_frame_period_tlv, &remote_error_frame_period_tlv))

    memcpy(&temp16, error_frame_period_tlv.event_time_stamp,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_TIME_STAMP_LEN);
    eventLinkStatus->frame_period_error_event_timestamp = NET2HOSTS(temp16);

    memcpy(&temp32, error_frame_period_tlv.error_frame_period_window,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_WINDOW_LEN);
    eventLinkStatus->frame_period_error_event_window = NET2HOSTL(temp32);

    memcpy(&temp32, error_frame_period_tlv.error_frame_threshold,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_THRESHOLD_LEN);
    eventLinkStatus->frame_period_error_event_threshold = NET2HOSTL(temp32);

    memcpy(&temp32, error_frame_period_tlv.error_frames,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_ERROR_FRAMES_LEN);
    eventLinkStatus->frame_period_errors = NET2HOSTL(temp32);

    memcpy(&temp64, error_frame_period_tlv.error_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_TOTAL_ERROR_FRAMES_LEN);
    eventLinkStatus->total_frame_period_errors = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp32, error_frame_period_tlv.event_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_TOTAL_ERROR_EVENTS_LEN);
    eventLinkStatus->total_frame_period_error_event = NET2HOSTL(temp32);

    VTSS_RC(eth_link_oam_client_port_error_frame_secs_summary_info_get(
                ife.isid, ife.ordinal, &error_frame_secs_summary_tlv, &remote_frame_secs_summary_tlv))

    memcpy(&temp16, error_frame_secs_summary_tlv.event_time_stamp,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_TIME_STAMP_LEN);
    eventLinkStatus->error_frame_seconds_summary_event_timestamp = NET2HOSTS(temp16);

    memcpy(&temp16, error_frame_secs_summary_tlv.secs_summary_window,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_SECS_SUMMARY_EVENT_WINDOW_LEN);
    eventLinkStatus->error_frame_seconds_summary_event_window = NET2HOSTS(temp16);

    memcpy(&temp16, error_frame_secs_summary_tlv.secs_summary_threshold,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_SECS_SUMMARY_EVENT_THRESHOLD_LEN);
    eventLinkStatus->error_frame_seconds_summary_event_threshold = NET2HOSTS(temp16);

    memcpy(&temp16, error_frame_secs_summary_tlv.secs_summary_events,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_SECS_SUMMARY_EVENT_ERROR_FRAMES_LEN);
    eventLinkStatus->error_frame_seconds_summary_errors = NET2HOSTS(temp16);

    memcpy(&temp32, error_frame_secs_summary_tlv.error_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_SECS_SUMMARY_EVENT_TOTAL_FRAMES_LEN);
    eventLinkStatus->total_error_frame_seconds_summary_errors = NET2HOSTL(temp32);

    memcpy(&temp32, error_frame_secs_summary_tlv.event_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_SECS_SUMMARY_EVENT_TOTAL_EVENTS_LEN);
    eventLinkStatus->total_error_frame_seconds_summary_events = NET2HOSTL(temp32);

    return VTSS_RC_OK;
}

/** Get Link OAM port peer event link status.
  * ifIndex              [IN]: Interface index.
  * peerEventLinkStatus  [OUT]: Port peer event link status.
  * return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_eth_link_oam_port_peer_link_event_status_get(
    vtss_ifindex_t                                  ifIndex,
    vtss_appl_eth_link_oam_port_link_event_status_t *const peerEventLinkStatus
)
{

    vtss_ifindex_elm_t                                      ife;
    vtss_eth_link_oam_error_frame_period_event_tlv_t        error_frame_period_tlv;
    vtss_eth_link_oam_error_frame_period_event_tlv_t        remote_error_frame_period_tlv;

    vtss_eth_link_oam_error_symbol_period_event_tlv_t       error_symbol_period_tlv;
    vtss_eth_link_oam_error_symbol_period_event_tlv_t       remote_error_symbol_period_tlv;

    vtss_eth_link_oam_error_frame_secs_summary_event_tlv_t  error_frame_secs_summary_tlv;
    vtss_eth_link_oam_error_frame_secs_summary_event_tlv_t  remote_frame_secs_summary_tlv;

    vtss_eth_link_oam_error_frame_event_tlv_t               error_frame_tlv;
    vtss_eth_link_oam_error_frame_event_tlv_t               remote_error_frame_tlv;
    u16                                                     temp16 = 0;
    u32                                                     temp32 = 0;
    u64                                                     temp64 = 0;

    if (peerEventLinkStatus == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }
    VTSS_RC(vtss_appl_ifindex_port_configurable(ifIndex, &ife))

    VTSS_RC(eth_link_oam_client_port_remote_seq_num_get(
                ife.isid, ife.ordinal, &peerEventLinkStatus->sequence_number));

    VTSS_RC(eth_link_oam_client_port_symbol_period_error_info_get(
                ife.isid, ife.ordinal, &error_symbol_period_tlv, &remote_error_symbol_period_tlv))

    memcpy(&temp16, remote_error_symbol_period_tlv.event_time_stamp,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_TIME_STAMP_LEN);
    peerEventLinkStatus->symbol_period_error_event_timestamp = NET2HOSTS(temp16);

    memcpy(&temp64, remote_error_symbol_period_tlv.error_symbol_window,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_WINDOW_LEN);
    peerEventLinkStatus->symbol_period_error_event_window = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp64, remote_error_symbol_period_tlv.error_symbol_threshold,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_THRESHOLD_LEN);
    peerEventLinkStatus->symbol_period_error_event_threshold = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp64, remote_error_symbol_period_tlv.error_symbols,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_ERROR_SYMBOLS_LEN);
    peerEventLinkStatus->symbol_period_error = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp64, remote_error_symbol_period_tlv.error_running_total,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_TOTAL_ERROR_SYMBOLS_LEN);
    peerEventLinkStatus->total_symbol_period_error = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp32, remote_error_symbol_period_tlv.event_running_total,
           VTSS_ETH_LINK_OAM_ERROR_SYMBOL_PERIOD_EVENT_TOTAL_ERROR_EVENTS_LEN);
    peerEventLinkStatus->total_symbol_period_error_events = NET2HOSTL(temp32);

    VTSS_RC(eth_link_oam_client_port_frame_error_info_get(
                ife.isid, ife.ordinal, &error_frame_tlv, &remote_error_frame_tlv))

    memcpy(&temp16, remote_error_frame_tlv.event_time_stamp,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_TIME_STAMP_LEN);
    peerEventLinkStatus->frame_error_event_timestamp = NET2HOSTS(temp16);

    memcpy(&temp16, remote_error_frame_tlv.error_frame_window,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_WINDOW_LEN);
    peerEventLinkStatus->frame_error_event_window = NET2HOSTS(temp16);

    memcpy(&temp32, remote_error_frame_tlv.error_frame_threshold,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_THRESHOLD_LEN);
    peerEventLinkStatus->frame_error_event_threshold  = NET2HOSTL(temp32);

    memcpy(&temp32, remote_error_frame_tlv.error_frames,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_ERROR_FRAMES_LEN);
    peerEventLinkStatus->frame_error = NET2HOSTL(temp32);

    memcpy(&temp64, remote_error_frame_tlv.error_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_TOTAL_ERROR_FRAMES_LEN);
    peerEventLinkStatus->total_frame_errors = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp32, remote_error_frame_tlv.event_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_EVENT_TOTAL_ERROR_EVENTS_LEN);
    peerEventLinkStatus->total_frame_errors_events  = NET2HOSTL(temp32);

    VTSS_RC(eth_link_oam_client_port_frame_period_error_info_get(
                ife.isid, ife.ordinal, &error_frame_period_tlv, &remote_error_frame_period_tlv));

    memcpy(&temp16, remote_error_frame_period_tlv.event_time_stamp,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_TIME_STAMP_LEN);
    peerEventLinkStatus->frame_period_error_event_timestamp = NET2HOSTS(temp16);

    memcpy(&temp32, remote_error_frame_period_tlv.error_frame_period_window,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_WINDOW_LEN);
    peerEventLinkStatus->frame_period_error_event_window = NET2HOSTL(temp32);

    memcpy(&temp32, remote_error_frame_period_tlv.error_frame_threshold,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_THRESHOLD_LEN);
    peerEventLinkStatus->frame_period_error_event_threshold = NET2HOSTL(temp32);

    memcpy(&temp32, remote_error_frame_period_tlv.error_frames,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_ERROR_FRAMES_LEN);
    peerEventLinkStatus->frame_period_errors = NET2HOSTL(temp32);

    memcpy(&temp64, remote_error_frame_period_tlv.error_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_TOTAL_ERROR_FRAMES_LEN);
    peerEventLinkStatus->total_frame_period_errors = vtss_eth_link_oam_swap64(temp64);

    memcpy(&temp32, remote_error_frame_period_tlv.event_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_TOTAL_ERROR_EVENTS_LEN);
    peerEventLinkStatus->total_frame_period_error_event = NET2HOSTL(temp32);

    VTSS_RC(eth_link_oam_client_port_error_frame_secs_summary_info_get(
                ife.isid, ife.ordinal, &error_frame_secs_summary_tlv, &remote_frame_secs_summary_tlv))

    memcpy(&temp16, remote_frame_secs_summary_tlv.event_time_stamp,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_TIME_STAMP_LEN);
    peerEventLinkStatus->error_frame_seconds_summary_event_timestamp = NET2HOSTS(temp16);

    memcpy(&temp16, remote_frame_secs_summary_tlv.secs_summary_window,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_SECS_SUMMARY_EVENT_WINDOW_LEN);
    peerEventLinkStatus->error_frame_seconds_summary_event_window = NET2HOSTS(temp16);

    memcpy(&temp16, remote_frame_secs_summary_tlv.secs_summary_threshold,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_SECS_SUMMARY_EVENT_THRESHOLD_LEN);
    peerEventLinkStatus->error_frame_seconds_summary_event_threshold = NET2HOSTS(temp16);

    memcpy(&temp16, remote_frame_secs_summary_tlv.secs_summary_events,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_SECS_SUMMARY_EVENT_ERROR_FRAMES_LEN);
    peerEventLinkStatus->error_frame_seconds_summary_errors = NET2HOSTS(temp16);

    memcpy(&temp32, remote_frame_secs_summary_tlv.error_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_SECS_SUMMARY_EVENT_TOTAL_FRAMES_LEN);
    peerEventLinkStatus->total_error_frame_seconds_summary_errors = NET2HOSTL(temp32);

    memcpy(&temp32, remote_frame_secs_summary_tlv.event_running_total,
           VTSS_ETH_LINK_OAM_ERROR_FRAME_SECS_SUMMARY_EVENT_TOTAL_EVENTS_LEN);
    peerEventLinkStatus->total_error_frame_seconds_summary_events = NET2HOSTL(temp32);

    return VTSS_RC_OK;
}

/**
 * Get Link OAM port capabilities
 * ifIndex       [IN]: Interface index
 * capabilities [OUT]: Link OAM platform specific port capabilities
 * return VTSS_RC_OK if the operation succeeded.
*/
mesa_rc vtss_appl_eth_link_oam_port_capabilities_get(
    vtss_ifindex_t                             ifIndex,
    vtss_appl_eth_link_oam_port_capabilities_t *const capabilities
)
{
    if (capabilities == NULL) {
        T_I("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }
    capabilities->eth_link_oam_capable = TRUE;
    return VTSS_RC_OK;
}

/******************************************************************************/
/* Link OAM's utility functions                                               */
/******************************************************************************/

u16 vtss_eth_link_oam_htons(u16 num)
{
    return HOST2NETS(num);
}

u32 vtss_eth_link_oam_htonl(u32 num)
{
    return HOST2NETL(num);
}

u16 vtss_eth_link_oam_ntohs(u16 num)
{
    return NET2HOSTS(num);
}

u32 vtss_eth_link_oam_ntohl(u32 num)
{
    return NET2HOSTL(num);
}

u16 vtss_eth_link_oam_ntohs_from_bytes(u8 tmp[])
{
    u16 num;

    memcpy(&num, tmp, sizeof(u16));
    return NET2HOSTS(num);
}

u32 vtss_eth_link_oam_ntohl_from_bytes(u8 tmp[])
{
    u32 num;

    memcpy(&num, tmp, sizeof(u32));
    return NET2HOSTL(num);
}

u64 vtss_eth_link_oam_swap64(u64 num)
{
    num = ( ( (num & 0xff00000000000000ULL) >> 56 ) |
            ( (num & 0x00000000000000ffULL) << 56 ) |
            ( (num & 0x00ff000000000000ULL) >> 40 ) |
            ( (num & 0x000000000000ff00ULL) << 40 ) |
            ( (num & 0x0000ff0000000000ULL) >> 24 ) |
            ( (num & 0x0000000000ff0000ULL) << 24 ) |
            ( (num & 0x000000ff00000000ULL) >> 8 ) |
            ( (num & 0x00000000ff000000ULL) << 8 )
          );

    return num;
}

u64 vtss_eth_link_oam_swap64_from_bytes(u8 tmp_num[])
{
    u64 num;
    memcpy (&num, tmp_num, 8);
    num = ( ( (num & 0xff00000000000000ULL) >> 56 ) |
            ( (num & 0x00000000000000ffULL) << 56 ) |
            ( (num & 0x00ff000000000000ULL) >> 40 ) |
            ( (num & 0x000000000000ff00ULL) << 40 ) |
            ( (num & 0x0000ff0000000000ULL) >> 24 ) |
            ( (num & 0x0000000000ff0000ULL) << 24 ) |
            ( (num & 0x000000ff00000000ULL) >> 8 ) |
            ( (num & 0x00000000ff000000ULL) << 8 )
          );

    return num;
}

void *vtss_eth_link_oam_malloc(size_t sz)
{
    return VTSS_MALLOC(sz);
}

u32 vtss_is_port_info_get(const u32 port_no, BOOL  *is_up)
{
    vtss_appl_port_status_t port_status;
    vtss_ifindex_t          ifindex;
    u32                     rc = VTSS_ETH_LINK_OAM_RC_OK;

    if (is_up == NULL) {
        return VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER;
    }

    (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_no, &ifindex);
    if (vtss_appl_port_status_get(ifindex, &port_status) == VTSS_RC_OK) {
        *is_up = port_status.link;
    } else {
        rc = VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER;
    }

    return rc;
}

void vtss_eth_link_oam_trace(const vtss_eth_link_oam_trace_level_t trace_level, const char  *string, const u32   param1, const u32   param2, const u32   param3, const u32   param4)

{
    do {
        switch (trace_level) {
        case VTSS_ETH_LINK_OAM_TRACE_LEVEL_ERROR:
            T_E("%s - port(%u), error(%u), %u, %u", string,
                param1, param2, param3, param4);
            break;
        case VTSS_ETH_LINK_OAM_TRACE_LEVEL_INFO:
            T_I("%s - %u, %u, %u, %u", string,
                param1, param2, param3, param4);
            break;
        case VTSS_ETH_LINK_OAM_TRACE_LEVEL_DEBUG:
            T_D("%s - port(%u), %u, %u, %u", string,
                param1, param2, param3, param4);
            break;
        case VTSS_ETH_LINK_OAM_TRACE_LEVEL_NOISE:
            T_N("%s - %u, %u, %u, %u", string,
                param1, param2, param3, param4);
            break;
        default:
            T_N("Invalid trace level %s - %u, %u, %u, %u", string,
                param1, param2, param3, param4);
            break;
        }
    } while (VTSS_ETH_LINK_OAM_NULL);

    return;
}

void vtss_eth_link_oam_crit_event_stats_update(const mesa_port_no_t port_no)
{
    vtss_ifindex_t                                      ifindex;
    vtss_appl_eth_link_oam_crit_link_event_statistics_t crit_link_event;
    u32                                                 rc = VTSS_ETH_LINK_OAM_RC_OK;

    if (vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_no, &ifindex) == VTSS_RC_OK) {
        memset(&crit_link_event, 0, sizeof(vtss_appl_eth_link_oam_crit_link_event_statistics_t));
        rc = vtss_eth_link_oam_control_layer_port_critical_event_pdu_stats_get(
                 port_no, &crit_link_event);
        if (rc == VTSS_ETH_LINK_OAM_RC_OK) {
            eth_link_oam_crit_event_update.set(ifindex, &crit_link_event);
        }
    }
}

/* Retrieve the OAM Configuration for the port                               */
mesa_rc eth_link_oam_mgmt_port_conf_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_conf_t *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;
    BOOL                     ready;
    if (rc_conv(vtss_eth_link_oam_ready_conf_get(&ready)) == VTSS_RC_OK && ready != FALSE) {
        if (rc_conv(vtss_eth_link_oam_mgmt_port_control_conf_get(
                        l2port, &conf->oam_control)) != VTSS_RC_OK ||
            rc_conv(vtss_eth_link_oam_mgmt_port_mode_conf_get(
                        l2port, &conf->oam_mode)) != VTSS_RC_OK  ||
            rc_conv(vtss_eth_link_oam_mgmt_port_remote_loopback_conf_get(
                        l2port, &conf->oam_remote_loop_back_support)) != VTSS_RC_OK ||
            rc_conv(vtss_eth_link_oam_mgmt_port_link_monitoring_conf_get(
                        l2port, &conf->oam_link_monitoring_support)) != VTSS_RC_OK  ||
            rc_conv(vtss_eth_link_oam_mgmt_port_link_error_frame_window_get(
                        l2port, &conf->oam_error_frame_event_conf.window)) != VTSS_RC_OK  ||
            rc_conv(vtss_eth_link_oam_mgmt_port_link_error_frame_threshold_get(
                        l2port, &conf->oam_error_frame_event_conf.threshold)) != VTSS_RC_OK  ||
            rc_conv(vtss_eth_link_oam_mgmt_port_link_symbol_period_error_window_get(
                        l2port, &conf->oam_symbol_period_event_conf.window)) != VTSS_RC_OK  ||
            rc_conv(vtss_eth_link_oam_mgmt_port_link_symbol_period_error_threshold_get(
                        l2port, &conf->oam_symbol_period_event_conf.threshold)) != VTSS_RC_OK  ||
            rc_conv(vtss_eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_get(
                        l2port, &conf->oam_error_frame_secs_summary_event_conf.window)) != VTSS_RC_OK  ||
            rc_conv(vtss_eth_link_oam_mgmt_port_mib_retrieval_conf_get(
                        l2port, &conf->oam_mib_retrieval_support)) != VTSS_RC_OK ) {

            T_E("Unable to retrieve the configuration of port(%d/%u)",
                isid, port_no);
            rc = VTSS_RC_ERROR;
        }
    }
    return rc;
}

const char *pdu_tx_control_to_str(vtss_eth_link_oam_pdu_control_t tx_control)
{
    switch (tx_control) {
    case VTSS_ETH_LINK_OAM_PDU_CONTROL_RX_INFO:
        return "Receive only";
    case VTSS_ETH_LINK_OAM_PDU_CONTROL_LF_INFO:
        return "Link fault";
    case VTSS_ETH_LINK_OAM_PDU_CONTROL_INFO:
        return "Info exchange";
    case VTSS_ETH_LINK_OAM_PDU_CONTROL_ANY:
        return "Any";
    default:
        return "";
    }
}

const char *discovery_state_to_str(vtss_eth_link_oam_discovery_state_t discovery_state)
{
    switch (discovery_state) {
    case VTSS_ETH_LINK_OAM_DISCOVERY_STATE_FAULT:
        return "Fault state";
    case VTSS_ETH_LINK_OAM_DISCOVERY_STATE_ACTIVE_SEND_LOCAL:
        return "Active state";
    case VTSS_ETH_LINK_OAM_DISCOVERY_STATE_PASSIVE_WAIT:
        return "Passive state";
    case VTSS_ETH_LINK_OAM_DISCOVERY_STATE_SEND_LOCAL_REMOTE:
        return "SEND_LOCAL_REMOTE_STATE";
    case VTSS_ETH_LINK_OAM_DISCOVERY_STATE_SEND_LOCAL_REMOTE_OK:
        return "SEND_LOCAL_REMOTE_OK_STATE";
    case VTSS_ETH_LINK_OAM_DISCOVERY_STATE_SEND_ANY:
        return "SEND_ANY_STATE";
    default:
        return "";
    }
}
const char *mux_state_to_str(vtss_eth_link_oam_mux_state_t mux_state)
{
    switch (mux_state) {
    case VTSS_ETH_LINK_OAM_MUX_FWD_STATE:
        return "Forwarding ";
    case VTSS_ETH_LINK_OAM_MUX_DISCARD_STATE:
        return "Discarding ";
    default:
        return "";
    }
}

const char *parser_state_to_str(vtss_eth_link_oam_parser_state_t parse_state)
{
    switch (parse_state) {
    case VTSS_ETH_LINK_OAM_PARSER_FWD_STATE:
        return "Forwarding ";
    case VTSS_ETH_LINK_OAM_PARSER_LB_STATE:
        return "Loop back ";
    case VTSS_ETH_LINK_OAM_PARSER_DISCARD_STATE:
        return "Discarding ";
    default:
        return "";
    }
}

void vtss_eth_link_oam_send_response_to_cli(char *var_response)
{
    T_D("Unlocking the MIB semaphore");
    vtss_eth_link_oam_mib_retrieval_opr_unlock();
}

/******************************************************************************/
/* Link OAM Port's Control status reterival functions                         */
/******************************************************************************/
mesa_rc eth_link_oam_control_layer_port_pdu_control_status_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_pdu_control_t *pdu_control)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);

    rc = vtss_eth_link_oam_mgmt_control_port_pdu_control_status_get(l2port,
                                                                    pdu_control);

    return (rc_conv(rc));
}

mesa_rc eth_link_oam_control_layer_port_discovery_state_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_discovery_state_t *state)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);

    rc = vtss_eth_link_oam_mgmt_control_port_discovery_state_get(l2port, state);

    return (rc_conv(rc));
}

mesa_rc eth_link_oam_control_layer_port_pdu_stats_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_pdu_stats_t *pdu_stats)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);

    rc = vtss_eth_link_oam_mgmt_control_port_pdu_stats_get(l2port, pdu_stats);

    return (rc_conv(rc));
}
mesa_rc eth_link_oam_control_layer_port_critical_event_pdu_stats_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_critical_event_pdu_stats_t *ce_pdu_stats)

{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);

    rc = vtss_eth_link_oam_mgmt_control_port_critical_event_pdu_stats_get(l2port,
                                                                          ce_pdu_stats);

    return (rc_conv(rc));
}
mesa_rc eth_link_oam_control_port_flags_conf_set(const vtss_isid_t isid, const mesa_port_no_t port_no, const u8 flag, const BOOL enable_flag)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);

    rc = vtss_eth_link_oam_mgmt_control_port_flags_conf_set(l2port,
                                                            flag,
                                                            enable_flag);
    if ( (rc == VTSS_ETH_LINK_OAM_RC_OK) &&
         (flag == VTSS_ETH_LINK_OAM_FLAG_LINK_FAULT)) {
        rc = vtss_eth_link_oam_mgmt_port_fault_conf_set(l2port, enable_flag);
    }

    return rc_conv(rc);

}

mesa_rc eth_link_oam_control_port_flags_conf_get(const vtss_isid_t isid, const mesa_port_no_t port_no, u8 *flag)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);

    rc = vtss_eth_link_oam_mgmt_control_port_flags_conf_get(l2port,
                                                            flag);
    return rc_conv(rc);
}

/*Clear the Link OAM statistics*/
mesa_rc eth_link_oam_clear_statistics(l2_port_no_t l2port)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;

    rc = vtss_eth_link_oam_mgmt_control_clear_statistics(l2port);
    return rc_conv(rc);
}

/******************************************************************************/
/* Link OAM Port's Client status reterival functions                          */
/******************************************************************************/

mesa_rc eth_link_oam_client_port_control_conf_get (const vtss_isid_t isid, const mesa_port_no_t port_no, u8 *conf)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_client_port_control_conf_get(
                 l2port, conf));
    return rc;
}

mesa_rc eth_link_oam_client_port_local_info_get (const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_info_tlv_t *local_info)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_client_port_local_info_get(
                 l2port, local_info));
    return rc;
}

mesa_rc eth_link_oam_client_port_remote_info_get (const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_info_tlv_t *remote_info)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_client_port_remote_info_get(
                 l2port, remote_info));
    return rc;
}

mesa_rc eth_link_oam_client_port_remote_seq_num_get (const vtss_isid_t isid, const mesa_port_no_t port_no, u16 *remote_info)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_client_port_remote_seq_num_get(
                 l2port, remote_info));
    return rc;
}

mesa_rc eth_link_oam_client_port_remote_mac_addr_info_get (const vtss_isid_t isid, const mesa_port_no_t port_no, u8 *remote_mac_addr)
{
    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_client_port_remote_mac_addr_info_get(l2port,
                                                                         remote_mac_addr));
    return rc;
}

mesa_rc eth_link_oam_client_port_frame_error_info_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_error_frame_event_tlv_t  *local_error_info, vtss_eth_link_oam_error_frame_event_tlv_t  *remote_error_info)

{

    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_client_port_frame_error_info_get(l2port,
                                                                     local_error_info,
                                                                     remote_error_info));
    return rc;
}

mesa_rc eth_link_oam_client_port_frame_period_error_info_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_error_frame_period_event_tlv_t  *local_info, vtss_eth_link_oam_error_frame_period_event_tlv_t   *remote_info)

{

    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_client_port_frame_period_error_info_get(l2port,
                                                                            local_info,
                                                                            remote_info));
    return rc;
}

mesa_rc eth_link_oam_client_port_symbol_period_error_info_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_error_symbol_period_event_tlv_t  *local_info, vtss_eth_link_oam_error_symbol_period_event_tlv_t   *remote_info)

{

    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_client_port_symbol_period_error_info_get(l2port,
                                                                             local_info,
                                                                             remote_info));
    return rc;
}

mesa_rc eth_link_oam_client_port_error_frame_secs_summary_info_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_error_frame_secs_summary_event_tlv_t   *local_info, vtss_eth_link_oam_error_frame_secs_summary_event_tlv_t  *remote_info)

{

    l2_port_no_t             l2port = L2PORT2PORT(isid, port_no);
    mesa_rc                  rc = VTSS_RC_OK;

    rc = rc_conv(
             vtss_eth_link_oam_mgmt_client_port_error_frame_secs_summary_info_get(
                 l2port,
                 local_info,
                 remote_info));
    return rc;
}

/****************************************************************************/
/*  ETH Link OAM Control layer interface                                    */
/****************************************************************************/
u32 vtss_eth_link_oam_control_port_conf_init(const u32 port_no, const u8 *data)
{
    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_conf_init(port_no, data);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif
    return rc;
}

u32 vtss_eth_link_oam_control_port_oper_init(const u32 port_no, BOOL is_port_active)
{
    u32 rc;
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_oper_init(port_no, is_port_active);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif
    return rc;
}

u32 vtss_eth_link_oam_control_port_control_conf_set(const u32 port_no, const u8 oam_control)
{
    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_control_conf_set(port_no, oam_control);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_control_conf_get(const u32 port_no, u8 *oam_control)
{
    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    if (oam_control == NULL) {
        return VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER;
    }
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_control_conf_get(port_no,
                                                               oam_control);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_flags_conf_set(const u32 port_no, const u8 flag, const BOOL enable_flag)
{
    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_flags_conf_set(
             port_no, flag, enable_flag);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_flags_conf_get(const u32 port_no, u8 *flag)
{
    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_flags_conf_get(port_no, flag);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_supported_code_conf_set (const u32 port_no, const u8 oam_code, const BOOL support_enable)
{
    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_supported_codes_conf_set(
             port_no,
             oam_code,
             support_enable);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_supported_code_conf_get (const u32 port_no, const u8 oam_code, BOOL *support_enable)
{
    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    if (support_enable == NULL) {
        return VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER;
    }
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_supported_codes_conf_get(port_no, oam_code,
                                                                  support_enable);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_data_set(const u32 port_no, const u8 *oam_data, BOOL  reset_port_oper, BOOL  is_port_active)
{
    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    if (oam_data == NULL) {
        return VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER;
    }
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_data_set(port_no,
                                                       oam_data, reset_port_oper, is_port_active);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_remote_state_valid_set(const u32 port_no)
{

    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_remote_state_valid_set(port_no);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_local_satisfied_set(const u32 port_no, const BOOL is_local_satisfied)
{
    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_local_satisfied_set(port_no,
                                                                  is_local_satisfied);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_remote_stable_set(const u32 port_no, const BOOL is_remote_stable)
{
    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_remote_stable_set(port_no,
                                                                is_remote_stable);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_pdu_control_status_get(const u32 port_no, vtss_eth_link_oam_pdu_control_t *pdu_control)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_pdu_control_status_get(port_no,
                                                                     pdu_control);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_local_lost_timer_conf_set(const u32 port_no)
{
    u32 rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_local_lost_timer_conf_set(port_no);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_discovery_state_get(const u32 port_no, vtss_eth_link_oam_discovery_state_t *state)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_discovery_state_get(
             port_no, state);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_pdu_stats_get(const u32 port_no, vtss_eth_link_oam_pdu_stats_t *pdu_stats)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_pdu_stats_get(port_no,
                                                            pdu_stats);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

u32 vtss_eth_link_oam_control_port_critical_event_pdu_stats_get(const u32 port_no, vtss_eth_link_oam_critical_event_pdu_stats_t *pdu_stats)

{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_critical_event_pdu_stats_get(
             port_no,
             pdu_stats);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

mesa_rc vtss_eth_link_oam_control_layer_port_mac_conf_get(const u32 port_no, u8 *port_mac_addr)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;
    vtss_isid_t isid;
    mesa_port_no_t switch_port;

    do {
        if (port_mac_addr == NULL) {
            rc = VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER;
            break;
        }
        if (l2port2port(port_no, &isid, &switch_port) == FALSE) {
            rc = VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER;
            break;
        }
        misc_instantiate_mac(port_mac_addr,
                             sys_mac_addr, switch_port + 1 - VTSS_PORT_NO_START);

    } while (VTSS_ETH_LINK_OAM_NULL);

    return rc;

}

mesa_rc vtss_eth_link_oam_control_port_mux_parser_conf_set(const u32 port_no, const vtss_eth_link_oam_mux_state_t  mux_state, const vtss_eth_link_oam_parser_state_t parser_state, u32   *oam_ace_id, u32   *lb_ace_id, const BOOL update_only_mux_state)
{
    u32            rc = VTSS_ETH_LINK_OAM_RC_OK;
    mesa_port_no_t iport;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)

    acl_entry_conf_t         lb_ace_conf;
    acl_entry_conf_t         oam_ace_conf;

    u8                       dmac[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x02};
    u8                       dmac_mask[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    u8                       eth_type[2] = {0x88, 0x09};
    u8                       eth_type_mask[2] = {0xff, 0xff};
    u8                       eth_data[2] = {0x03, 0x00};
    u8                       eth_data_mask[2] = {0xff, 0x00};

    do {
        if ( (lb_ace_id == NULL) ||
             (oam_ace_id == NULL) ) {
            rc = VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER;
            break;
        }

        if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ANY, &lb_ace_conf)) != VTSS_RC_OK) {
            return rc;
        }
        if ((rc = acl_mgmt_ace_init(MESA_ACE_TYPE_ETYPE, &oam_ace_conf)) != VTSS_RC_OK) {
            return rc;
        }
        /* Set the ACE's and ACE action's port list to FALSE */
        for (iport = VTSS_PORT_NO_START; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
            lb_ace_conf.port_list[iport] = FALSE;
            lb_ace_conf.action.port_list[iport] = FALSE;
            oam_ace_conf.port_list[iport] = FALSE;
            oam_ace_conf.action.port_list[iport] = FALSE;
        }

        lb_ace_conf.isid = VTSS_ISID_LOCAL;
        lb_ace_conf.id = ACL_MGMT_ACE_ID_NONE;//MAX;
        lb_ace_conf.action.policer = MESA_ACL_POLICY_NO_NONE;
        lb_ace_conf.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
        lb_ace_conf.port_list[port_no] = TRUE;

        oam_ace_conf.isid = VTSS_ISID_LOCAL;
        oam_ace_conf.id = ACL_MGMT_ACE_ID_NONE;//MAX;
        oam_ace_conf.action.policer = MESA_ACL_POLICY_NO_NONE;
        oam_ace_conf.action.port_action = MESA_ACL_PORT_ACTION_NONE;
        for (iport = VTSS_PORT_NO_START; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
            oam_ace_conf.action.port_list[iport] = TRUE;
        }
        oam_ace_conf.port_list[port_no] = TRUE;

        memcpy(oam_ace_conf.frame.etype.dmac.value, dmac, 6);
        memcpy(oam_ace_conf.frame.etype.dmac.mask, dmac_mask, 6);
        memcpy(oam_ace_conf.frame.etype.etype.value, eth_type, 2);
        memcpy(oam_ace_conf.frame.etype.etype.mask, eth_type_mask, 2);
        memcpy(oam_ace_conf.frame.etype.data.value, eth_data, 2);
        memcpy(oam_ace_conf.frame.etype.data.mask, eth_data_mask, 2);

        switch (mux_state) {
        case VTSS_ETH_LINK_OAM_MUX_FWD_STATE:
            rc = mesa_port_forward_state_set(NULL, port_no, MESA_PORT_FORWARD_ENABLED);
            if (rc != VTSS_RC_OK) {
                rc = VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER;
                break;
            }
            break;
        case VTSS_ETH_LINK_OAM_MUX_DISCARD_STATE:
            rc = mesa_port_forward_state_set(NULL, port_no, MESA_PORT_FORWARD_DISABLED);
            if (rc != VTSS_RC_OK) {
                rc = VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER;
                break;
            }
            break;
        }
        if (update_only_mux_state == TRUE) {
            /* Update only mux state */
            break;
        }
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            break;
        }
        switch (parser_state) {

        case VTSS_ETH_LINK_OAM_PARSER_FWD_STATE:
            rc = acl_mgmt_ace_del(ACL_USER_LINK_OAM, *lb_ace_id);
            if (rc == VTSS_RC_OK) {
                *lb_ace_id = 0;
            } else {
                rc = VTSS_ETH_LINK_OAM_RC_INVALID_PARAMETER;
                break;
            }
            rc = acl_mgmt_ace_del(ACL_USER_LINK_OAM, *oam_ace_id);
            if (rc == VTSS_RC_OK) {
                *oam_ace_id = 0;
            }
            break;

        case VTSS_ETH_LINK_OAM_PARSER_DISCARD_STATE:
            rc = acl_mgmt_ace_add(ACL_USER_LINK_OAM,
                                  ACL_MGMT_ACE_ID_NONE,
                                  &oam_ace_conf);
            if (rc == VTSS_RC_OK) {
                *oam_ace_id = oam_ace_conf.id;
            } else {
                rc = VTSS_ETH_LINK_OAM_RC_NOT_ENABLED;
                break;
            }

            rc = acl_mgmt_ace_add(ACL_USER_LINK_OAM,
                                  ACL_MGMT_ACE_ID_NONE,
                                  &lb_ace_conf);
            if (rc == VTSS_RC_OK) {
                *lb_ace_id = lb_ace_conf.id;
            } else {
                rc = VTSS_ETH_LINK_OAM_RC_NOT_ENABLED;
                break;
            }
            break;

        case VTSS_ETH_LINK_OAM_PARSER_LB_STATE:
            /* Install remote loopback specific rules */
            lb_ace_conf.action.port_action = MESA_ACL_PORT_ACTION_REDIR;
            lb_ace_conf.action.port_list[port_no] = TRUE;
            lb_ace_conf.port_list[port_no] = TRUE;

            rc = acl_mgmt_ace_add(ACL_USER_LINK_OAM,
                                  ACL_MGMT_ACE_ID_NONE,
                                  &oam_ace_conf);
            if (rc == VTSS_RC_OK) {
                *oam_ace_id = oam_ace_conf.id;
            } else {
                rc = VTSS_ETH_LINK_OAM_RC_NOT_ENABLED;
                break;
            }

            rc = acl_mgmt_ace_add(ACL_USER_LINK_OAM,
                                  ACL_MGMT_ACE_ID_NONE,
                                  &lb_ace_conf);
            if (rc == VTSS_RC_OK) {
                *lb_ace_id = lb_ace_conf.id;
            } else {
                rc = VTSS_ETH_LINK_OAM_RC_NOT_ENABLED;
                break;
            }
            break;

        default :
            break;
        }
    } while (VTSS_ETH_LINK_OAM_NULL);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return (rc);
}

mesa_rc vtss_eth_link_oam_control_port_parser_conf_get(const u32 port_no, vtss_eth_link_oam_parser_state_t *parser_state)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_port_parser_conf_get(
             port_no, parser_state);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return (rc);
}

u32 vtss_eth_link_oam_control_port_info_pdu_xmit(const u32 port_no)
{
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    u8  *oam_pdu;
    u32 rc       = VTSS_ETH_LINK_OAM_RC_OK;

    if ((oam_pdu = (uint8_t *)vtss_os_alloc_xmit(port_no, VTSS_ETH_LINK_OAM_PDU_MIN_LEN)) == nullptr) {
        return VTSS_ETH_LINK_OAM_RC_NO_MEMORY; // Should be fatal
    }

    memset(oam_pdu, 0, VTSS_ETH_LINK_OAM_PDU_MIN_LEN);
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_fill_info_data(port_no, oam_pdu);
    CRIT_EXIT(oam_control_layer_lock);
    if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
        // Frame not freed if vtss_os_xmit() doesn't get called.
        packet_tx_free(oam_pdu);
        return rc;
    }

    rc = vtss_os_xmit(port_no, oam_pdu, VTSS_ETH_LINK_OAM_PDU_MIN_LEN);
    if (rc != VTSS_COMMON_CC_OK) {
        packet_tx_free(oam_pdu);
        /* LOG the message */
    }

    return  rc;
#else
    return VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif
}

u32 vtss_eth_link_oam_control_port_non_info_pdu_xmit(const u32 port_no, u8  *oam_client_pdu, const u16 oam_pdu_len, const u8 oam_pdu_code)
{
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    u8                                  *oam_pdu;
    vtss_eth_link_oam_discovery_state_t state;
    uint32_t                            rc = VTSS_ETH_LINK_OAM_RC_OK;

    CRIT_ENTER(oam_control_layer_lock);

    if ((rc = vtss_eth_link_oam_control_layer_port_discovery_state_get(port_no, &state)) != VTSS_ETH_LINK_OAM_RC_OK) {
        T_D("Error:%u occured while retrieving the port(%u) configuration", rc, port_no);
        goto do_exit;
    }

    if (state != VTSS_ETH_LINK_OAM_DISCOVERY_STATE_SEND_ANY) {
        rc = VTSS_ETH_LINK_OAM_RC_INVALID_STATE;
        goto do_exit;
    }

    if ((oam_pdu = (uint8_t *)vtss_os_alloc_xmit(port_no, oam_pdu_len)) == nullptr) {
        rc = VTSS_ETH_LINK_OAM_RC_NO_MEMORY;
        goto do_exit;
    }

    memcpy(oam_pdu, oam_client_pdu, oam_pdu_len);
    if ((rc = vtss_eth_link_oam_control_layer_fill_header(port_no, oam_pdu, oam_pdu_code)) == VTSS_ETH_LINK_OAM_RC_OK) {
        rc = vtss_os_xmit(port_no, oam_pdu, oam_pdu_len);
    } else {
        T_D("Error: %u occured while sending the OAM PDU on port(%u)", rc, port_no);

        // Frame not freed if vtss_os_xmit() doesn't get called.
        packet_tx_free(oam_pdu);
    }

do_exit:
    CRIT_EXIT(oam_control_layer_lock);
    return rc;
#else
    return VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif
}

/* port_no:         port number                                               */
/* Resets the port counters                                                   */
u32 vtss_eth_link_oam_control_clear_statistics(const u32 port_no)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_clear_statistics(port_no);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return (rc);
}

u32 vtss_eth_link_oam_control_rx_pdu_handler(const u32 port_no, const u8 *pdu, const u16 pdu_len, const u8  oam_code)
{
    u32                      rc = VTSS_ETH_LINK_OAM_RC_OK;
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    CRIT_ENTER(oam_control_layer_lock);
    rc = vtss_eth_link_oam_control_layer_rx_pdu_handler(port_no,
                                                        pdu,
                                                        pdu_len,
                                                        oam_code);
    CRIT_EXIT(oam_control_layer_lock);
#else
    rc = VTSS_ETH_LINK_OAM_RC_NOT_SUPPORTED;
#endif

    return rc;
}

/****************************************************************************/
/*  ETH Link OAM platform interface                                         */
/****************************************************************************/
void vtss_eth_link_oam_crit_data_lock(void)
{
    CRIT_ENTER(oam_data_lock);
}

void vtss_eth_link_oam_crit_data_unlock(void)
{
    CRIT_EXIT(oam_data_lock);
}

void vtss_eth_link_oam_crit_oper_data_lock(void)
{
    CRIT_ENTER(oam_oper_lock);
}

void vtss_eth_link_oam_crit_oper_data_unlock(void)
{
    CRIT_EXIT(oam_oper_lock);
}

BOOL vtss_eth_link_oam_mib_retrieval_opr_lock(void)
{

    BOOL rc = FALSE;
    T_D("Locking the MIB retrieval operation semaphore");
    rc = vtss_sem_timed_wait(&eth_link_oam_var_sem, vtss_current_time() + VTSS_OS_MSEC2TICK(VTSS_ETH_LINK_OAM_SLEEP_TIME));

    T_D("Result of the MIB retrieval operation is %u", rc);
    return rc;
}

void vtss_eth_link_oam_mib_retrieval_opr_unlock(void)
{

    T_D("Unlocking the MIB retrieval operation semaphore");
    vtss_sem_post(&eth_link_oam_var_sem);
}

BOOL vtss_eth_link_oam_rlb_opr_lock(void)
{

    BOOL rc = FALSE;
    T_D("Locking the remote loopback operation semaphore");
    rc = vtss_sem_timed_wait(&eth_link_oam_rlb_sem, vtss_current_time() + VTSS_OS_MSEC2TICK(1000));
    T_D("Result of the remote loopback operation is %u", rc);
    return rc;
}

void vtss_eth_link_oam_rlb_opr_unlock(void)
{
    T_D("Unlocking the remote loopback operation semaphore");
    vtss_sem_post(&eth_link_oam_rlb_sem);
}

void vtss_eth_link_oam_message_post(vtss_eth_link_oam_message_t *message)
{
    T_D("Post OAM event:%u on port(%u)", message->event_code,
        message->event_on_port);

    if (eth_link_oam_queue.vtss_safe_queue_put(message) != TRUE) {
        VTSS_FREE(message);
    }
}

static void eth_link_oam_client_thread(vtss_addrword_t data)
{
    vtss_eth_link_oam_message_t *event_message;
    u32                         rc = VTSS_ETH_LINK_OAM_RC_OK;

    while (1) {
        event_message = (vtss_eth_link_oam_message_t *)eth_link_oam_queue.vtss_safe_queue_get();
        T_D("Get OAM event:%u on port(%u)", event_message->event_code, event_message->event_on_port);

        vtss_eth_link_oam_crit_oper_data_lock();

        rc = vtss_eth_link_oam_message_handler(event_message);
        if ( (rc != VTSS_ETH_LINK_OAM_RC_OK) &&
             (rc != VTSS_ETH_LINK_OAM_RC_ALREADY_CONFIGURED) ) {
            /* LOG-Message */
            T_D(":%u occured while processing the OAM event:%u on port(%u)",
                rc, event_message->event_code, event_message->event_on_port);
        }

        VTSS_FREE(event_message);
        vtss_eth_link_oam_crit_oper_data_unlock();
    }
}

static void eth_link_oam_control_error_frame_event_send(const u32 port_no, BOOL *is_error_frame_xmit_needed, mesa_port_counters_t *port_counters)
{
    BOOL                          is_timer_expired  = FALSE;
    u32                           rc;

    vtss_eth_link_oam_client_link_event_oper_conf_t link_event_oper_conf;

    /* Check if timer has expired */
    memset(&link_event_oper_conf, 0,
           sizeof(vtss_eth_link_oam_client_link_event_oper_conf_t));
    rc = vtss_eth_link_oam_client_is_error_frame_window_expired(
             port_no, &is_timer_expired);
    if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
        T_D("Error:%u occured while calculating port(%u) timer values",
            rc, port_no);
        return;
    }
    if (is_timer_expired == TRUE) {
        /* Update the PDU Error Events if Any */
        link_event_oper_conf.ifInErrors_at_timeout =
            (  port_counters->rmon.rx_etherStatsUndersizePkts +
               port_counters->rmon.rx_etherStatsOversizePkts  +
               port_counters->rmon.rx_etherStatsCRCAlignErrors );

        /* update the oper_event_window field from the mgmt
         * structure */
        rc = vtss_eth_link_oam_client_error_frame_oper_conf_update(
                 port_no, &link_event_oper_conf,
                 VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_EVENT_WINDOW_SET |
                 VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_ERROR_AT_TIMEOUT_SET);
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_D("Error:%u occured while updating port(%u) timer configurations",
                rc, port_no);
            return;
        }
        rc = vtss_eth_link_oam_client_error_frame_event_conf_set (port_no,
                                                                  is_error_frame_xmit_needed);
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_D("Error:%u occured while updating port(%u) timer configurations",
                rc, port_no);
            return;
        }
        /* After Sending Update the Errors at Start */
        rc = vtss_eth_link_oam_client_error_frame_oper_conf_update(port_no
                                                                   , &link_event_oper_conf,
                                                                   VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_ERROR_AT_START_SET);
    } else {

        link_event_oper_conf.oper_event_window = VTSS_ETH_LINK_OAM_SLEEP_TIME_IN_100_MS;
        /* Reduce the oper time period */
        rc = vtss_eth_link_oam_client_error_frame_oper_conf_update(
                 port_no, &link_event_oper_conf,
                 VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_EVENT_WINDOW_DELETE_OFFSET);
    }
    if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
        T_D("Error:%u occured while updating port(%u) timer configurations",
            rc, port_no);
        return;
    }
}

static void eth_link_oam_control_symbol_period_error_event_send(const u32 port_no, BOOL *is_symbol_period_xmit_needed, mesa_port_counters_t *port_counters)
{
    BOOL                          is_timer_expired  = FALSE;
    u32                           rc;
    vtss_eth_link_oam_client_link_error_symbol_period_event_oper_conf_t symbol_period_oper_conf;

    /* Send Symbol Period Error Events Here */
    /* Check if timer has expired */
    memset(&symbol_period_oper_conf, 0,
           sizeof(vtss_eth_link_oam_client_link_error_symbol_period_event_oper_conf_t));
    rc = vtss_eth_link_oam_client_is_symbol_period_frame_window_expired (port_no,
                                                                         &is_timer_expired);
    if (rc != VTSS_ETH_LINK_OAM_RC_OK) {

        T_D("Error:%u occured while calculating port(%u) timer configurations",
            rc, port_no);
        return;
    }
    if ( is_timer_expired == TRUE)  {
        /* Update the PDU Error Events if Any */
        if (fast_cap(MESA_CAP_PORT_CNT_ETHER_LIKE)) {
            symbol_period_oper_conf.symbolErrors_at_timeout = port_counters->ethernet_like.dot3StatsSymbolErrors;
        } else {
            symbol_period_oper_conf.symbolErrors_at_timeout = port_counters->rmon.rx_etherStatsCRCAlignErrors;
        }
        /* update the oper_event_window field from the mgmt structure */
        rc = vtss_eth_link_oam_client_symbol_period_error_oper_conf_update(port_no, &symbol_period_oper_conf,
                                                                           VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_EVENT_WINDOW_SET |
                                                                           VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_ERROR_AT_TIMEOUT_SET);
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_D("Error:%u occured while updating port(%u) timer configurations",
                rc, port_no);
            return;
        }
        rc = vtss_eth_link_oam_client_symbol_period_error_conf_set(port_no, is_symbol_period_xmit_needed);
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_D("Error:%u occured while updating port(%u) timer configurations",
                rc, port_no);
            return;
        }
        /* After Sending Update the Errors at Start */
        rc = vtss_eth_link_oam_client_symbol_period_error_oper_conf_update(port_no,
                                                                           &symbol_period_oper_conf,
                                                                           VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_ERROR_AT_START_SET);
    } else {
        /* Reduce the oper time period */
        symbol_period_oper_conf.oper_event_window     = VTSS_ETH_LINK_OAM_SLEEP_TIME_IN_100_MS;
        rc = vtss_eth_link_oam_client_symbol_period_error_oper_conf_update(port_no, &symbol_period_oper_conf,
                                                                           VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_EVENT_WINDOW_DELETE_OFFSET);
    }
    if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
        T_D("Error:%u occured while updating port(%u) timer configurations",
            rc, port_no);
        return;
    }
}

static void eth_link_oam_control_frame_period_error_event_send(const u32 port_no, BOOL *is_frame_period_xmit_needed, mesa_port_counters_t *port_counters)
{
    BOOL                          is_timer_expired  = FALSE;
    u32                           rc;
    vtss_eth_link_oam_client_link_error_frame_period_event_oper_conf_t frame_period_oper_conf;

    /* Send Frame Period Error Events Here */
    /* Check if timer has expired */
    memset(&frame_period_oper_conf, 0,
           sizeof(vtss_eth_link_oam_client_link_error_frame_period_event_oper_conf_t));

    rc = vtss_eth_link_oam_client_is_frame_period_frame_window_expired(port_no, &is_timer_expired);
    if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
        T_D("Error:%u occured while updating port(%u) timer configurations", rc, port_no);
        return;
    }
    if (is_timer_expired == TRUE)  {
        /* Update the PDU Error Events if Any */
        frame_period_oper_conf.frameErrors_at_timeout =
            (  port_counters->rmon.rx_etherStatsUndersizePkts +
               port_counters->rmon.rx_etherStatsOversizePkts  +
               port_counters->rmon.rx_etherStatsCRCAlignErrors );

        /* update the oper_event_window field from the mgmt structure */
        rc = vtss_eth_link_oam_client_frame_period_error_oper_conf_update(
                 port_no, &frame_period_oper_conf,
                 VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_EVENT_WINDOW_SET |
                 VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_ERROR_AT_TIMEOUT_SET);
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_D("Error:%u occured while updating port(%u) timer configurations",
                rc, port_no);
            return;
        }
        rc = vtss_eth_link_oam_client_frame_period_error_conf_set(port_no,
                                                                  is_frame_period_xmit_needed);
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_D("Error:%u occured while updating port(%u) timer configurations",
                rc, port_no);
            return;
        }
        /* After Sending Update the Errors at Start */
        rc = vtss_eth_link_oam_client_frame_period_error_oper_conf_update(port_no, &frame_period_oper_conf,
                                                                          VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_ERROR_AT_START_SET);
    } else {
        /* Reduce the oper time period */
        frame_period_oper_conf.oper_event_window     = VTSS_ETH_LINK_OAM_SLEEP_TIME_IN_100_MS;
        rc = vtss_eth_link_oam_client_frame_period_error_oper_conf_update(port_no, &frame_period_oper_conf,
                                                                          VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_EVENT_WINDOW_DELETE_OFFSET);
    }
    if (rc != VTSS_ETH_LINK_OAM_RC_OK) {

        T_I("Error:%u occured while updating port(%u) timer configurations",
            rc, port_no);
        return;
    }
}

static void eth_link_oam_control_error_frame_secs_summary_event_send(const u32 port_no, BOOL *is_error_frame_secs_summary_xmit_needed, mesa_port_counters_t *port_counters)
{
    BOOL                          is_timer_expired  = FALSE;
    u32                           rc;
    vtss_eth_link_oam_client_error_frame_secs_summary_event_oper_conf_t
    link_event_oper_conf;

    /* Check if timer has expired */
    memset(&link_event_oper_conf, 0,
           sizeof(
               vtss_eth_link_oam_client_error_frame_secs_summary_event_oper_conf_t));

    link_event_oper_conf.secErrors_at_timeout = (  port_counters->rmon.rx_etherStatsUndersizePkts +
                                                   port_counters->rmon.rx_etherStatsOversizePkts  +
                                                   port_counters->rmon.rx_etherStatsCRCAlignErrors );

    rc = vtss_eth_link_oam_client_is_error_frame_secs_summary_window_expired(
             port_no, &is_timer_expired);
    if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
        T_I("Error:%u occured while updating port(%u) timer configurations",
            rc, port_no);
        return;
    }

    if (is_timer_expired == TRUE) {
        /* update the oper_event_window field from the mgmt
         * structure */
        rc = vtss_eth_link_oam_client_error_frame_secs_summary_oper_conf_update(
                 port_no, &link_event_oper_conf,
                 VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_EVENT_WINDOW_SET |
                 VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_ERROR_AT_TIMEOUT_SET);
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_D("Error:%u occured while updating port(%u) timer configurations",
                rc, port_no);
            return;
        }
        rc = vtss_eth_link_oam_client_error_frame_secs_summary_event_conf_set (port_no, TRUE,
                                                                               is_error_frame_secs_summary_xmit_needed);
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_D("Error:%u occured while updating port(%u) timer configurations",
                rc, port_no);
            return;
        }
        /* After Sending Update the Error Frame Events at Start */
        rc = vtss_eth_link_oam_client_error_frame_secs_summary_oper_conf_update(port_no
                                                                                , &link_event_oper_conf,
                                                                                VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_ERROR_AT_START_SET);
    } else {

        /* update the oper_event_window field from the mgmt
         * structure */
        rc = vtss_eth_link_oam_client_error_frame_secs_summary_oper_conf_update(
                 port_no, &link_event_oper_conf,
                 VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_ERROR_AT_TIMEOUT_SET);
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_D("Error:%u occured while updating port(%u) timer configurations",
                rc, port_no);
            return;
        }
        rc = vtss_eth_link_oam_client_error_frame_secs_summary_event_conf_set (port_no, FALSE,
                                                                               is_error_frame_secs_summary_xmit_needed);
        /* After Sending Update the Error Frame Events at Start */
        rc = vtss_eth_link_oam_client_error_frame_secs_summary_oper_conf_update(port_no
                                                                                , &link_event_oper_conf,
                                                                                VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_ERROR_AT_START_SET);

        link_event_oper_conf.oper_event_window = VTSS_ETH_LINK_OAM_SLEEP_TIME_IN_100_MS;
        /* Reduce the oper time period */
        rc = vtss_eth_link_oam_client_error_frame_secs_summary_oper_conf_update(
                 port_no, &link_event_oper_conf,
                 VTSS_ETH_LINK_OAM_CLIENT_LINK_OPER_CONF_EVENT_WINDOW_DELETE_OFFSET);
    }
    if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
        T_I("Error:%u occured while updating port(%u) timer configurations",
            rc, port_no);
        return;
    }
}

static int vtss_dying_gasp_link_oam_pdu_send(mesa_port_no_t port, u8 *frame, vtss_common_framelen_t len, BOOL release_frame_buffers)
{
    T_I("Transmitting Dying-Gasp Link-OAM PDU of length = %u to iport = %u", len, port);
    T_I_HEX(frame, len);
#ifdef VTSS_SW_OPTION_PACKET
    packet_tx_props_t tx_props;
    packet_tx_props_init(&tx_props);
    tx_props.packet_info.modid     = VTSS_MODULE_ID_ETH_LINK_OAM;
    tx_props.packet_info.frm       = frame;
    tx_props.packet_info.len       = len;
    tx_props.packet_info.no_free   = !release_frame_buffers;
    tx_props.tx_info.dst_port_mask = VTSS_BIT64(port);
    tx_props.tx_info.cos           = MESA_PRIO_SUPER;
    if (packet_tx(&tx_props) == VTSS_RC_OK) {
        return VTSS_COMMON_CC_OK;
    }
#endif
    return VTSS_COMMON_CC_GENERR;
}

static void vtss_eth_link_oam_fill_dying_gasp_header_info(const u32 port_no, u8 *const pdu, const u8 code)
{
    vtss_eth_link_oam_frame_header_t         *oam_header;
    u16                                      eth_type;
    u16                                      flags_local = 0x0052;
    u16                                      tmp_flags = 0;
    u32                                      rc;
    do {
        if (!pdu) {
            T_D("pdu is null");
            return;
        }
        eth_type = vtss_eth_link_oam_htons(VTSS_ETH_LINK_OAM_ETH_TYPE);
        oam_header = (vtss_eth_link_oam_frame_header_t *)pdu;
        memcpy(oam_header->dst_mac, ETH_LINK_OAM_MULTICAST_MACADDR, VTSS_ETH_LINK_OAM_MAC_LEN);
        rc = vtss_eth_link_oam_control_layer_port_mac_conf_get(port_no, oam_header->src_mac);
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_D("failed to get mac address, port_no: %u", port_no);
            return;
        }
        memcpy(oam_header->eth_type, &eth_type, VTSS_ETH_LINK_OAM_ETH_TYPE_LEN);
        oam_header->subtype = VTSS_ETH_LINK_OAM_SUB_TYPE;
        tmp_flags = vtss_eth_link_oam_htons(flags_local);
        memcpy(oam_header->flags, &tmp_flags, (VTSS_ETH_LINK_OAM_FLAGS_LEN));
        oam_header->code = code;
    } while (VTSS_ETH_LINK_OAM_NULL); /* end of the do-while */
}

/*
 * This function adds npi_encap, ifh_encap based on port_no to the given pdu
 * then send it to the kernel that will reserve a link list for saving all
 * pdus, and send them out in stack manner (LIFO) upon power failure.
 * returns the id of pdu in LIFO.
 */
static int
vtss_eth_link_oam_add_dying_gasp_pdu_private(const u32     prt_no_or_vid,
                                             u8            *frame,
                                             size_t        len,
                                             BOOL          switch_frm = FALSE)
{
    /**
     * dying gasp pdu resides in kernel as stack (LIFO), not FIFO
     * hence if we push the dying gasp pdus of all ports to kernel, then
     * kernel starts to send pdu from the very last port, since it is
     * the very last pdu push into kernel.
     *
     * Based on some tests, kernel is only capable for sending out one
     * pdu out upon power failure! */

    packet_tx_props_t tx_props;
    CapArray<u8, MESA_CAP_PACKET_HDR_SIZE> ifh;
    u32               ifh_len;
    u8                buf[4096];
    u32               buf_len;
    mesa_rc           rc;
    int               id;

    memset(buf, 0x0, sizeof(buf));

    packet_tx_props_init(&tx_props);
    tx_props.packet_info.modid  = VTSS_MODULE_ID_ETH_LINK_OAM;
    tx_props.packet_info.frm    = frame;
    tx_props.packet_info.len    = len;
    tx_props.tx_info.cos        = MESA_PRIO_SUPER;
    tx_props.tx_info.switch_frm = switch_frm;

    if (switch_frm) {
        tx_props.tx_info.tag.vid    = prt_no_or_vid;
    } else {
        tx_props.tx_info.dst_port_mask = VTSS_BIT64(prt_no_or_vid);
    }

    /* encode ifh */
    ifh_len = ifh.size();
    if ((mesa_packet_tx_hdr_encode(NULL, &tx_props.tx_info, ifh_len, ifh.data(),
                                   &ifh_len)) != VTSS_RC_OK) {
        T_E("mesa_packet_tx_hdr_encode() failed!");
        return -1;
    }

    /* Frame layout:
     * [ENCAP DMAC][ENCAP SMAC][0x8880][0x00][0x05][IFH][DMAC][SMAC][ETYPE]...
     *
     * Format the msg and sent it to kernel via generic netlink */
    memcpy(buf, npi_encap, NPI_ENCAP_LEN);
    buf_len = NPI_ENCAP_LEN;
    memcpy(buf + buf_len, ifh.data(), ifh_len);
    buf_len += ifh_len;
    memcpy(buf + buf_len, tx_props.packet_info.frm, tx_props.packet_info.len);
    buf_len += tx_props.packet_info.len;  /* buf_len of oam pdu is 96 */

    /* ready to be sent to the kernel */
    rc = vtss::appl::ip::dying_gasp::vtss_dying_gasp_add("vtss.ifh",
                                                         (const u8 *)buf,
                                                         (size_t)buf_len,
                                                         &id);
    if (rc != VTSS_RC_OK) {
        T_E("Sending eth_link_oam dying gasp pdu failed!\n");
    }
    return id;
}

void vtss_eth_link_oam_add_dying_gasp_pdu(const u32 port_no)
{
    vtss_eth_link_oam_crit_data_lock();
    vtss_eth_link_oam_add_dying_gasp_pdu_private(port_no, (u8 *)oam_dying_gasp_pdu[port_no], VTSS_ETH_LINK_OAM_PDU_MIN_LEN);
    vtss_eth_link_oam_crit_data_unlock();
}

int vtss_eth_link_oam_add_dying_gasp_trap(const u32 vid, u8 *frame, size_t len)
{
    T_I("Adding a new buff to kernel for DG Trap\n");
    int id = vtss_eth_link_oam_add_dying_gasp_pdu_private(vid, frame, len, TRUE);
    T_I("Added a buf with id:%d\n", id);
    return id;
}

/* Example code */
void vtss_eth_link_oam_add_all_dying_gasp_pdu(void)
{
    u32     port_no = 0;
    u32     port_no_max = VTSS_PORT_NO_START + port_count_max();

    for (port_no = VTSS_PORT_NO_START;
         port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) && port_no < port_no_max; port_no++) {
        vtss_eth_link_oam_add_dying_gasp_pdu(port_no);
    }
}

/* Example code */
void vtss_eth_link_oam_del_dying_gasp_pdu(const int id)
{
    /* a unique id is returned after calling vtss_dying_gasp_add(...)
     * NOTE: id is not necessarily equal to port_no !
     */
    mesa_rc rc;
    rc = vtss::appl::ip::dying_gasp::vtss_dying_gasp_delete(id);

    if (rc != VTSS_RC_OK) {
        T_W("Deleting eth_link_oam dying gasp pdu failed!\n");
    } else {
        T_D("Dying gasp pdu ID:%d deleted!\n", id);
    }
}

/* Example code */
void vtss_eth_link_oam_del_all_dying_gasp_pdu(void)
{
    mesa_rc rc;
    rc = vtss::appl::ip::dying_gasp::vtss_dying_gasp_delete_all();

    if (rc != VTSS_RC_OK) {
        T_W("Cannot delete all eth_link_oam dying gasp pdu!\n");
    } else {
        T_D("All eth_link_oam dying gasp pdus deleted!\n");
    }
}

static void vtss_eth_link_oam_create_dying_gasp_pdu(void)
{
    u32     port_no = 0;
    u32     port_no_max = VTSS_PORT_NO_START + port_count_max();
    vtss_eth_link_oam_crit_data_lock();
    for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) && port_no < port_no_max; port_no++) {
        oam_dying_gasp_pdu[port_no] = (u8 *)packet_tx_alloc(VTSS_ETH_LINK_OAM_PDU_MIN_LEN);
        memset(oam_dying_gasp_pdu[port_no], '\0', VTSS_ETH_LINK_OAM_PDU_MIN_LEN);
        vtss_eth_link_oam_fill_dying_gasp_header_info(port_no, oam_dying_gasp_pdu[port_no], VTSS_ETH_LINK_OAM_CODE_TYPE_INFO);
    }
    vtss_eth_link_oam_crit_data_unlock();

}

void vtss_eth_link_oam_mgmt_sys_reboot_action_handler(mesa_restart_t restart)
{
    u32                                  port_no = 0;
    u32                                  port_no_max = VTSS_PORT_NO_START + port_count_max();
    T_D("enter");
    do {
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
        for (port_no = VTSS_PORT_NO_START; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT) && port_no < port_no_max; port_no++) {
            vtss_eth_link_oam_mgmt_sys_send_dying_gasp(port_no);
        }
#endif
    } while (0);
    T_D("exit");
}

void vtss_eth_link_oam_mgmt_sys_send_dying_gasp(mesa_port_no_t port_no)
{
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    vtss_eth_link_oam_discovery_state_t  state;
    u8                                   *tmp_frame;
    char                                 disc_buf[32] = {0};
    T_D("enter");
    CRIT_ENTER(oam_control_layer_lock);
    (void)vtss_eth_link_oam_control_layer_port_discovery_state_get(port_no, &state);

    T_I("iport %u: state = %u", port_no, state);
    CRIT_EXIT(oam_control_layer_lock);
    strncpy(disc_buf, discovery_state_to_str(state), sizeof(disc_buf));
    T_D("Port_no = %u. State = %s", port_no, disc_buf);
    if (state == VTSS_ETH_LINK_OAM_DISCOVERY_STATE_FAULT) {
        T_D("Link-oam disabled. Dying gasp not sent. Port_no = %u", port_no);
        return;
    }
    vtss_eth_link_oam_crit_data_lock();
    // Copying frame because same port might have to send dying gasp more than once in same session.
    tmp_frame = (u8 *)packet_tx_alloc(VTSS_ETH_LINK_OAM_PDU_MIN_LEN);
    memcpy(tmp_frame, oam_dying_gasp_pdu[port_no], VTSS_ETH_LINK_OAM_PDU_MIN_LEN);
    (void)vtss_dying_gasp_link_oam_pdu_send(port_no, tmp_frame, VTSS_ETH_LINK_OAM_PDU_MIN_LEN, true);
    // Updating pdu tx stats.
    vtss_eth_link_oam_control_increment_dying_gasp_pdu_tx(port_no);
    vtss_eth_link_oam_crit_event_stats_update(port_no);
    vtss_eth_link_oam_crit_data_unlock();
#endif
}

static void eth_link_oam_control_timer_thread(vtss_addrword_t data)
{
    BOOL                        is_update_needed  = FALSE;
    u16                         pdu_max_len = 0;
    u32                         rc;
    u32                         port_no = 0;
    u8                          *oam_pdu = NULL, pkt_buffer_length = 0;
    BOOL                        is_error_frame_xmit_needed = FALSE;
    BOOL                        is_symbol_period_xmit_needed = FALSE;
    BOOL                        is_frame_period_xmit_needed = FALSE;
    BOOL                        is_error_frame_secs_summary_xmit_needed = FALSE;
    mesa_port_counters_t        port_counters;
#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
    BOOL                        is_xmit_needed  = FALSE;
    BOOL                        is_local_lost_link_timer_done = FALSE;
#endif

    while (1) {
        VTSS_OS_MSLEEP(1000);

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
        for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            vtss_eth_link_oam_crit_oper_data_lock();
            rc = vtss_eth_link_oam_control_layer_port_pdu_cnt_conf_set(port_no);
            rc = vtss_eth_link_oam_control_layer_is_periodic_xmit_needed(port_no, &is_xmit_needed);
            vtss_eth_link_oam_crit_oper_data_unlock();
            if ( (rc == VTSS_ETH_LINK_OAM_RC_OK) &&
                 (is_xmit_needed == TRUE)
               ) {

                oam_pdu = (u8 *)vtss_os_alloc_xmit(port_no, VTSS_ETH_LINK_OAM_PDU_MIN_LEN);
                if (oam_pdu == NULL) {
                    continue; //It's should be fatal
                }
                memset(oam_pdu, '\0', VTSS_ETH_LINK_OAM_PDU_MIN_LEN);
                vtss_eth_link_oam_crit_oper_data_lock();
                rc = vtss_eth_link_oam_control_layer_fill_info_data(port_no, oam_pdu);
                vtss_eth_link_oam_crit_oper_data_unlock();
                if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
                    // Frame not freed if vtss_os_xmit() doesn't get called.
                    packet_tx_free(oam_pdu);
                    continue;
                }
                rc = vtss_os_xmit(port_no, oam_pdu, VTSS_ETH_LINK_OAM_PDU_MIN_LEN);
                if (rc != VTSS_COMMON_CC_OK) {
                    /* LOG the message */
                }
                vtss_eth_link_oam_crit_oper_data_lock();
                rc = vtss_eth_link_oam_control_layer_is_local_lost_link_timer_done(port_no, &is_local_lost_link_timer_done);
                vtss_eth_link_oam_crit_oper_data_unlock();

                if (rc == VTSS_ETH_LINK_OAM_RC_OK && is_local_lost_link_timer_done) {
                    eth_link_oam_port_local_lost_link_timer_done(port_no);
                }
            }
        }
#endif /* end of periodic transmission of OAM info PDUs */

        for (port_no = 0; port_no < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); port_no++) {
            vtss_eth_link_oam_crit_oper_data_lock();
            rc = vtss_eth_link_oam_client_is_link_event_stats_update_needed(port_no, &is_update_needed);
            vtss_eth_link_oam_crit_oper_data_unlock();
            if (rc == VTSS_ETH_LINK_OAM_RC_OK && is_update_needed) {
                vtss_ifindex_t ifindex;
                is_error_frame_xmit_needed = is_symbol_period_xmit_needed = is_frame_period_xmit_needed = FALSE;
                /* Read the Statistics and Update */
                /* Since the port thread updates the counters , it is
                 * better to read it using port_stack_couters_get */

                (void)vtss_ifindex_from_port(VTSS_ISID_LOCAL, port_no, &ifindex);
                if ((rc = vtss_appl_port_statistics_get(ifindex, &port_counters)) != VTSS_RC_OK) {
                    T_I("Error:%u occured while getting port(%u) counters", rc, port_no);
                    continue; /* It should be fatal */
                }
                vtss_eth_link_oam_crit_oper_data_lock();
                rc = vtss_eth_link_oam_client_pdu_max_len(port_no, &pdu_max_len);

                if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
                    T_I("Unable to retrieve the PDU max length of port(%u)",
                        port_no);
                    vtss_eth_link_oam_crit_oper_data_unlock();
                    continue;
                }

                (void) eth_link_oam_control_error_frame_event_send(port_no,
                                                                   &is_error_frame_xmit_needed, &port_counters);
                (void) eth_link_oam_control_symbol_period_error_event_send(port_no,
                                                                           &is_symbol_period_xmit_needed, &port_counters);
                (void) eth_link_oam_control_frame_period_error_event_send(port_no,
                                                                          &is_frame_period_xmit_needed, &port_counters);
                (void) eth_link_oam_control_error_frame_secs_summary_event_send(port_no,
                                                                                &is_error_frame_secs_summary_xmit_needed, &port_counters);
                vtss_eth_link_oam_crit_oper_data_unlock();
                pkt_buffer_length = VTSS_ETH_LINK_OAM_PDU_HDR_LEN +
                                    VTSS_ETH_LINK_OAM_ERROR_FRAME_PERIOD_EVENT_SEQUENCE_NUMBER_LEN;

                if (is_error_frame_xmit_needed == TRUE) {
                    pkt_buffer_length +=
                        VTSS_ETH_LINK_OAM_LINK_MONITORING_ERROR_FRAME_EVENT_LEN;
                }
                if (is_symbol_period_xmit_needed == TRUE) {
                    pkt_buffer_length +=
                        VTSS_ETH_LINK_OAM_LINK_MONITORING_SYMBOL_PERIOD_EVENT_LEN;
                }
                if (is_frame_period_xmit_needed == TRUE) {
                    pkt_buffer_length +=
                        VTSS_ETH_LINK_OAM_LINK_MONITORING_FRAME_PERIOD_EVENT_LEN;
                }
                if (is_error_frame_secs_summary_xmit_needed == TRUE) {
                    pkt_buffer_length +=
                        VTSS_ETH_LINK_OAM_LINK_MONITORING_ERROR_FRAME_SECS_SUM_EVENT_LEN;
                }

                if (pkt_buffer_length < VTSS_ETH_LINK_OAM_PDU_MIN_LEN) {
                    pkt_buffer_length = VTSS_ETH_LINK_OAM_PDU_MIN_LEN;
                }

                if (pdu_max_len < pkt_buffer_length) {
                    T_D("Frame length exceeds the max PDU length on port(%u)",
                        port_no);
                    continue;
                }

                if ((is_error_frame_xmit_needed == TRUE)   ||
                    (is_symbol_period_xmit_needed == TRUE) ||
                    (is_frame_period_xmit_needed == TRUE)  ||
                    (is_error_frame_secs_summary_xmit_needed == TRUE)) {
                    /* Construct the PDU */
                    if ((oam_pdu = (uint8_t *)vtss_os_alloc_xmit(port_no, pkt_buffer_length)) == nullptr) {
                        T_I("Unable to allocate memory to send out frame on port(%u)", port_no);
                        continue; // Should be fatal
                    }

                    memset(oam_pdu, 0, pkt_buffer_length);
                    vtss_eth_link_oam_crit_oper_data_lock();
                    rc = vtss_eth_link_oam_control_layer_fill_header(port_no, oam_pdu, VTSS_ETH_LINK_OAM_CODE_TYPE_EVENT);
                    vtss_eth_link_oam_crit_oper_data_unlock();

                    if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
                        if (rc != VTSS_ETH_LINK_OAM_RC_INVALID_STATE) {
                            T_E("Error %u occured while sending out OAM frame on port(%u)", rc, port_no);
                        }

                        // Frame not freed if vtss_os_xmit() doesn't get called.
                        packet_tx_free(oam_pdu);
                        continue;
                    }
                    vtss_eth_link_oam_crit_oper_data_lock();
                    rc = vtss_eth_link_oam_client_link_monitoring_pdu_fill_info_data(
                             port_no, oam_pdu,
                             is_error_frame_xmit_needed, is_symbol_period_xmit_needed,
                             is_frame_period_xmit_needed,
                             is_error_frame_secs_summary_xmit_needed);
                    vtss_eth_link_oam_crit_oper_data_unlock();

                    if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
                        T_E("Error %u occured while sending out OAM frame on port(%u)", rc, port_no);
                        // Frame not freed if vtss_os_xmit() doesn't get called.
                        packet_tx_free(oam_pdu);
                        continue;
                    }

                    /* Send the PDU to the client */
                    rc = vtss_os_xmit(port_no, oam_pdu, pkt_buffer_length);
                    if (rc != VTSS_COMMON_CC_OK) {
                        T_E("Error %u occured while sending out OAM frame on port(%u)", rc, port_no);
                        packet_tx_free(oam_pdu);
                        continue;
                    }
                }
            }
        }
    }
}

static void eth_link_oam_restore_to_default(void)
{
    u32   i = 0;
    u32  rc = 0;

    for (i = 0; i < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); ++i) {
        rc += vtss_eth_link_oam_mgmt_port_conf_init(i);
        rc += vtss_eth_link_oam_mgmt_port_control_conf_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_PORT_CONTROL);
        rc += vtss_eth_link_oam_mgmt_port_mode_conf_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_PORT_MODE, TRUE);
        rc += vtss_eth_link_oam_mgmt_port_mib_retrieval_conf_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_PORT_MIB_RETRIEVAL_SUPPORT);
        rc += vtss_eth_link_oam_mgmt_port_remote_loopback_conf_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_PORT_REMOTE_LOOPBACK_SUPPORT, TRUE);
        rc += vtss_eth_link_oam_mgmt_port_link_monitoring_conf_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_PORT_LINK_MONITORING_SUPPORT);
        rc += vtss_eth_link_oam_mgmt_port_link_error_frame_window_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_EVENT_WINDOW_CONF);
        rc += vtss_eth_link_oam_mgmt_port_link_error_frame_threshold_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_EVENT_THRESHOLD_CONF);
        rc += vtss_eth_link_oam_mgmt_port_link_symbol_period_error_window_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_EVENT_WINDOW_CONF);
        rc += vtss_eth_link_oam_mgmt_port_link_symbol_period_error_threshold_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_EVENT_THRESHOLD_CONF);
        rc += vtss_eth_link_oam_mgmt_port_link_symbol_period_error_rxpackets_threshold_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_EVENT_RXTHRESHOLD_CONF);
        rc += vtss_eth_link_oam_mgmt_port_link_frame_period_error_window_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_EVENT_WINDOW_CONF);
        rc += vtss_eth_link_oam_mgmt_port_link_frame_period_error_threshold_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_EVENT_THRESHOLD_CONF);
        rc += vtss_eth_link_oam_mgmt_port_link_frame_period_error_rxpackets_threshold_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_EVENT_RXTHRESHOLD_CONF);
        rc += vtss_eth_link_oam_mgmt_port_link_error_frame_secs_summary_window_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_EVENT_SECS_SUMMARY_WINDOW_MIN);
        rc += vtss_eth_link_oam_mgmt_port_link_error_frame_secs_summary_threshold_set(
                  i, VTSS_ETH_LINK_OAM_DEFAULT_EVENT_SECS_SUMMARY_THRESHOLD_MIN);

        if (rc) {
            T_D("Error:%u occured during applying the configuration on port(%u)", rc, i);
        }
        /* Reset the statistics */
        rc = vtss_eth_link_oam_mgmt_control_clear_statistics(i);
        if (rc != VTSS_ETH_LINK_OAM_RC_OK) {
            T_D("Error:%u occured during clear statistics on port(%u)", rc, i);
        }
    }
}

static void eth_link_oam_port_change_callback(mesa_port_no_t port_no, const vtss_appl_port_status_t *status)
{
    vtss_eth_link_oam_message_t       *message = NULL;
    T_D("Port %u Link %u\n", port_no, status->link);

    message = (vtss_eth_link_oam_message_t *)VTSS_MALLOC(sizeof(vtss_eth_link_oam_message_t));
    if (message != NULL) {
        if (status->link) {
            message->event_code = VTSS_ETH_LINK_OAM_PORT_UP_EVENT;
        } else {
            message->event_code = VTSS_ETH_LINK_OAM_PORT_DOWN_EVENT;
        }
        message->event_on_port = port_no;
        vtss_eth_link_oam_message_post(message);
    } else {
        T_E("Unable to allocate the memory to handle the received frame on port(%u)",
            port_no);
    }
    return;
}

static void port_shutdown_callback(mesa_port_no_t port_no)
{
    T_D("enter");
    vtss_eth_link_oam_mgmt_sys_send_dying_gasp(port_no);
    T_D("exit");
}

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
static void eth_link_oam_port_local_lost_link_timer_done(mesa_port_no_t port_no)
{
    vtss_eth_link_oam_message_t       *message = NULL;

    message = (vtss_eth_link_oam_message_t *)VTSS_MALLOC(sizeof(vtss_eth_link_oam_message_t));
    if (message != NULL) {
        message->event_code = VTSS_ETH_LINK_OAM_PDU_LOCAL_LOST_LINK_TIMER_EVENT;
        message->event_on_port = port_no;
        vtss_eth_link_oam_message_post(message);
    } else {
        T_E("Unable to allocate the memory to handle the port(%u) timer expiry",
            port_no);
    }
    return;
}
#endif

/* ETH Link OAM frame handler */
static BOOL rx_eth_link_oam(void *contxt, const uchar *const frm, const mesa_packet_rx_info_t *const rx_info)
{
    BOOL rc = FALSE; // Allow other subscribers to receive the packet
    vtss_eth_link_oam_message_t        *message = NULL;
    u8                                 conf;
    u32   rc1 = VTSS_ETH_LINK_OAM_RC_OK;

    do {
        if (frm[14] != VTSS_ETH_LINK_OAM_SUB_TYPE) {  //Verify that PDU is for Link OAM
            T_I("Other slow protocol PDU is received on port(%u)", rx_info->port_no);
            break;
        }
        if (vtss_eth_link_oam_mgmt_client_port_control_conf_get(rx_info->port_no, &conf) ==
            VTSS_ETH_LINK_OAM_RC_OK) {
            if (conf == FALSE) {
                //OAM operation is disabled on this port. Ignore the PDUs.
                T_I("OAM PDU is received on OAM disabled port(%u)", rx_info->port_no);
                break;
            }
        } else {
            T_E("Error occured while getting the port(%u) control configuration",
                rx_info->port_no);
            break;
        }
        if (frm[14] == VTSS_ETH_LINK_OAM_SUB_TYPE) {
            if (rx_info->tag_type != MESA_TAG_TYPE_UNTAGGED) {
                //Verify that PDU is not tagged
                T_D("Tagged OAM PDU is received on port(%u)", rx_info->port_no);
                break;
            }
        }
        /* Needs to consider the untagged frames */
        if ( (rx_info->length < (VTSS_ETH_LINK_OAM_PDU_MIN_LEN - 4)) ||
             (rx_info->length > (VTSS_ETH_LINK_OAM_PDU_MAX_LEN + VTSS_ETH_LINK_OAM_PDU_HDR_LEN))
           ) {
            //Verify that PDU came with sufficient length
            T_E("Invalid length:(%u) OAM frame recived on port(%u)", rx_info->length,
                rx_info->port_no);
            break;
        }
        rc1 = vtss_eth_link_oam_mgmt_control_rx_pdu_handler(rx_info->port_no, frm, rx_info->length, frm[17]);
        if ( (rc1 == VTSS_ETH_LINK_OAM_RC_OK) ||
             (rc1 == VTSS_ETH_LINK_OAM_RC_ALREADY_CONFIGURED)) {
            message = (vtss_eth_link_oam_message_t *)VTSS_MALLOC(sizeof(vtss_eth_link_oam_message_t));
            if (message != NULL) {
                message->event_code      = VTSS_ETH_LINK_OAM_PDU_RX_EVENT;
                message->event_on_port   = rx_info->port_no;
                message->event_data_len  = rx_info->length;
                memcpy(message->event_data, frm, rx_info->length);
                vtss_eth_link_oam_message_post(message);
                break;
            } else {
                T_E("Unable to allocate memory to handle the received frame on port(%u)", rx_info->port_no);
                break;
            }
        }
    } while (VTSS_ETH_LINK_OAM_NULL);

    return rc;
}

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_eth_link_oam_json_init(void);
#endif
extern "C" int eth_link_oam_icli_cmd_register();

/* Initialize module */
mesa_rc eth_link_oam_init(vtss_init_data_t *data)
{
    mesa_rc                vtssrc = VTSS_RC_OK;
    packet_rx_filter_t     rx_filter;
    vtss_isid_t            isid = data->isid;

    switch (data->cmd) {
    case INIT_CMD_INIT:
        T_D("INIT");
        // Initialize the OAM Module elements
        critd_init(&oam_data_lock,          "loam-data", VTSS_MODULE_ID_ETH_LINK_OAM, CRITD_TYPE_MUTEX);
        critd_init(&oam_oper_lock,          "loam-oper", VTSS_MODULE_ID_ETH_LINK_OAM, CRITD_TYPE_MUTEX);
        critd_init(&oam_control_layer_lock, "loam-ctrl", VTSS_MODULE_ID_ETH_LINK_OAM, CRITD_TYPE_MUTEX);

        vtss_sem_init(&eth_link_oam_var_sem, 0);
        vtss_sem_init(&eth_link_oam_rlb_sem, 0);

#ifdef VTSS_SW_OPTION_JSON_RPC
        vtss_appl_eth_link_oam_json_init();
#endif

#if defined(VTSS_SW_OPTION_ETH_LINK_OAM_CONTROL)
        vtss_eth_link_oam_control_init();
#endif
#ifdef VTSS_SW_OPTION_ICFG
        vtssrc = eth_link_oam_icfg_init();
        if (vtssrc != VTSS_RC_OK) {
            T_D("fail to init link oam icfg registration, rc = %s", error_txt(vtssrc));
        }
#endif
        eth_link_oam_icli_cmd_register();
        vtssrc = rc_conv(vtss_eth_link_oam_default_set());

        // NPI encapsulation EPID
        npi_encap[NPI_ENCAP_LEN - 1] = fast_cap(MESA_CAP_PACKET_IFH_EPID);
        break;

    case INIT_CMD_START:
        T_D("START");
        //Start OAM module threads
        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           eth_link_oam_control_timer_thread,
                           0,
                           "ETH LINK OAM TIMER",
                           nullptr,
                           0,
                           &eth_link_oam_control_timer_thread_handle,
                           &eth_link_oam_control_timer_thread_block);

        vtss_thread_create(VTSS_THREAD_PRIO_DEFAULT,
                           eth_link_oam_client_thread,
                           0,
                           "ETH LINK OAM CLIENT",
                           nullptr,
                           0,
                           &eth_link_oam_client_thread_handle,
                           &eth_link_oam_client_thread_block);

        vtssrc = vtss_eth_link_oam_ready_conf_set(TRUE);
        break;

    case INIT_CMD_CONF_DEF:
        T_D("CONF_DEF");
        //Construct the OAM configurations with factory defaults
        if (isid == VTSS_ISID_LOCAL) {
            eth_link_oam_restore_to_default();
        }

        break;

    case INIT_CMD_ICFG_LOADING_PRE:
        T_D("ICFG_LOADING_PRE");
        //Get the system MAC address
        (void)conf_mgmt_mac_addr_get(sys_mac_addr, 0);
        vtss_eth_link_oam_create_dying_gasp_pdu();
        if (fast_cap(MEBA_CAP_DYING_GASP)) {
            vtss_eth_link_oam_add_all_dying_gasp_pdu();  /* example code */
        }

        eth_link_oam_restore_to_default();
        break;

    case INIT_CMD_ICFG_LOADING_POST:
        T_D("ICFG_LOADING_POST");
        // Registration for OAM frames
        packet_rx_filter_init(&rx_filter);
        rx_filter.modid                 = VTSS_MODULE_ID_ETH_LINK_OAM;
        rx_filter.match                 = PACKET_RX_FILTER_MATCH_DMAC | PACKET_RX_FILTER_MATCH_ETYPE;
        rx_filter.cb                    = rx_eth_link_oam;
        rx_filter.prio                  = PACKET_RX_FILTER_PRIO_NORMAL;
        memcpy(rx_filter.dmac, ETH_LINK_OAM_MULTICAST_MACADDR, sizeof(rx_filter.dmac));
        rx_filter.etype                 = VTSS_ETH_LINK_OAM_ETH_TYPE; // 0x8809

        vtssrc = packet_rx_filter_register(&rx_filter, &eth_link_oam_filter_id);

        if (vtssrc != VTSS_RC_OK) {
            T_E("Unable to register with Packet module");
            // Can we break here?
            break;
        }
        vtssrc = port_change_register(VTSS_MODULE_ID_ETH_LINK_OAM, eth_link_oam_port_change_callback);

        if ((vtssrc = port_shutdown_register(VTSS_MODULE_ID_ETH_LINK_OAM, port_shutdown_callback)) != VTSS_RC_OK) {
            T_E("port_shutdown_register failed %s", error_txt(vtssrc));
        }

        // Registering a call for a dying gasp to be sent in case of a system reset.
        control_system_reset_register(vtss_eth_link_oam_mgmt_sys_reboot_action_handler, VTSS_MODULE_ID_ETH_LINK_OAM);

        if (vtssrc != VTSS_RC_OK) {
            T_E("Unable to register with Port module");
        }

        break;

    default:
        break;
    }

    return vtssrc;
}

mesa_rc eth_link_oam_port_loopback_oper_status_get(const vtss_isid_t isid, const mesa_port_no_t port_no, vtss_eth_link_oam_loopback_status_t *loopback_status)
{
    l2_port_no_t l2port = L2PORT2PORT (isid, port_no);
    mesa_rc rc = VTSS_RC_OK;

    rc =
        vtss_eth_link_oam_mgmt_loopback_oper_status_get (l2port, loopback_status);

    return (rc_conv (rc));
}

