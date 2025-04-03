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

#ifndef __PORT_EXPOSE_HXX__
#define __PORT_EXPOSE_HXX__

#include <vtss/basics/expose.hxx>
#include <vtss/basics/enum-descriptor.h>

extern vtss_enum_descriptor_t port_expose_speed_duplex_txt[];
extern vtss_enum_descriptor_t port_expose_fc_txt[];
extern vtss_enum_descriptor_t port_expose_media_txt[];
extern vtss_enum_descriptor_t port_expose_fec_mode_txt[];
extern vtss_enum_descriptor_t port_expose_status_speed_txt[];
extern vtss_enum_descriptor_t port_expose_phy_veriphy_status_txt[];
extern vtss::expose::TableStatus<vtss::expose::ParamKey<vtss_ifindex_t>, vtss::expose::ParamVal<vtss_appl_port_status_t *>> port_status_update;

// Speed and duplex in one enumeration
typedef enum {
    PORT_EXPOSE_SPEED_DUPLEX_10M_FDX,   /**< Forced 10M mode, full duplex  */
    PORT_EXPOSE_SPEED_DUPLEX_10M_HDX,   /**< Forced 10M mode, half duplex  */
    PORT_EXPOSE_SPEED_DUPLEX_100M_FDX,  /**< Forced 100M mode, full duplex */
    PORT_EXPOSE_SPEED_DUPLEX_100M_HDX,  /**< Forced 100M mode, half duplex */
    PORT_EXPOSE_SPEED_DUPLEX_1G_FDX,    /**< Forced 1G mode, full duplex   */
    PORT_EXPOSE_SPEED_DUPLEX_AUTO,      /**< Auto-negotiation mode         */
    PORT_EXPOSE_SPEED_DUPLEX_2500M_FDX, /**< Forced 2.5G mode, full duplex */
    PORT_EXPOSE_SPEED_DUPLEX_5G_FDX,    /**< Forced 5G mode, full duplex   */
    PORT_EXPOSE_SPEED_DUPLEX_10G_FDX,   /**< Forced 10G mode, full duplex  */
    PORT_EXPOSE_SPEED_DUPLEX_12G_FDX,   /**< Forced 12G mode, full duplex  */
    PORT_EXPOSE_SPEED_DUPLEX_25G_FDX,   /**< Forced 25G mode, full duplex  */
} port_expose_speed_duplex_t;

typedef enum {
    PORT_EXPOSE_FC_OFF, /**< Flow control off */
    PORT_EXPOSE_FC_ON,  /**< Flow control on */
} port_expose_fc_t;

// Once upon a time, there was a public structure called
// vtss_appl_port_mib_conf_t, from which JSON and the Port MIB were created.
// This no longer exists, because it clutters the port.h public header file and
// confuses people, but but we still use it in the serializer, in order to keep
// it backwards compatible.
// In order not have people think that this is to be used, the various
// VTSS_APPL_PORT_/vtss_appl_port_ prefixes are replaced by
// PORT_EXPOSE_/port_expose_ prefixes.
typedef struct {
    mesa_bool_t                shutdown;         /**< Shutdown port                                                                                                  */
    port_expose_speed_duplex_t speed;            /**< Port speed and duplex mode (Full or Half)                                                                      */
    mepa_adv_dis_t             advertise_dis;    /**< In auto mode, bitmask that allows features not to be advertised                                                */
    vtss_appl_port_media_t     media;            /**< Media type                                                                                                     */
    port_expose_fc_t           fc;               /**< Flow control                                                                                                   */
    uint8_t                    pfc_mask;         /**< 802.1Qbb Priority Flow Control bitmask. One bit for each prio. 0x01 = prio 0, 0x80 = prio 7, 0xFF = prio 0-7   */
    uint32_t                   mtu;              /**< Maximum Transmission Unit                                                                                      */
    mesa_bool_t                excessive;        /**< TRUE to restart half-duplex back-off algorithm after 16 collisions. FALSE to discard frame after 16 collisions */
    mesa_bool_t                frame_length_chk; /**< TRUE to Enforce 802.3 frame length check (from ethertype field)                                                */
    mesa_bool_t                force_clause_73;  /**< TRUE to enforce 802.3 clause 73 aneg ("KR"). speed must be PORT_EXPOSE_SPEED_DUPLEX_AUTO in that case          */
    vtss_appl_port_fec_mode_t  fec_mode;         /**< May be used to force a particular Forward Error Correction mode                                                */
} port_expose_port_conf_t;

// Likewise, this structure is only (and should only be) used by the serializer.
typedef struct {
    mesa_port_counter_t rx_prio; /**< Rx frames for this queue */
    mesa_port_counter_t tx_prio; /**< Tx frames for this queue */
} port_expose_prio_counter_t;

mesa_rc port_expose_port_conf_get(        vtss_ifindex_t ifindex, port_expose_port_conf_t                      *port_expose_conf);
mesa_rc port_expose_port_conf_set(        vtss_ifindex_t ifindex, const port_expose_port_conf_t                *port_expose_conf);
mesa_rc port_expose_qu_statistics_get(    vtss_ifindex_t ifindex, mesa_prio_t prio, port_expose_prio_counter_t *prio_counters);
mesa_rc port_rmon_statistics_get(         vtss_ifindex_t ifindex, mesa_port_rmon_counters_t                    *rmon_statistics);
mesa_rc port_if_group_statistics_get(     vtss_ifindex_t ifindex, mesa_port_if_group_counters_t                *port_if_group_counters);
mesa_rc port_bridge_statistics_get(       vtss_ifindex_t ifindex, mesa_port_bridge_counters_t                  *bridge_counters);
mesa_rc port_ethernet_like_statistics_get(vtss_ifindex_t ifindex, mesa_port_ethernet_like_counters_t           *eth_counters);
mesa_rc port_dot3br_statistics_get(       vtss_ifindex_t ifindex, mesa_port_dot3br_counters_t                  *dot3br_statistics);
mesa_rc port_stats_dummy_get(             vtss_ifindex_t ifindex, BOOL                                         *const clear);
mesa_rc port_stats_clr_set(               vtss_ifindex_t ifindex, const BOOL                                   *const clear);
mesa_rc port_expose_veriphy_start_set(    vtss_ifindex_t ifindex, const BOOL                                   *start);
mesa_rc port_expose_veriphy_result_get(   vtss_ifindex_t ifindex, vtss_appl_port_veriphy_result_t              *port_result);
mesa_rc port_expose_interface_portno_get( vtss_ifindex_t ifindex, mesa_port_no_t                               *port_no);

#endif /* __PORT_EXPOSE_HXX__ */

