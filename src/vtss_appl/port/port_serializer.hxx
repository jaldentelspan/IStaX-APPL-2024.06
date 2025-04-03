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

#ifndef __VTSS_PORT_SERIALIZER_HXX__
#define __VTSS_PORT_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/port.h"
#include "vtss/appl/interface.h"
#include "port_expose.hxx"

//
// Type serialization descriptions
//
VTSS_XXXX_SERIALIZE_ENUM(port_expose_speed_duplex_t, "PortSpeed",
                         port_expose_speed_duplex_txt,
                         "This enumeration controls the interface speed. "
                         "E.g force10ModeFdx means force 10Mbs full duplex.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_port_media_t, "PortMedia",
                         port_expose_media_txt,
                         "This enumeration controls the interface media type.");

VTSS_XXXX_SERIALIZE_ENUM(port_expose_fc_t, "PortFc",
                         port_expose_fc_txt,
                         "This enumeration controls the interface flow control.");

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_port_fec_mode_t, "PortFecMode",
                         port_expose_fec_mode_txt,
                         "This enumeration controls/shows the Forward Error Correction mode.");

VTSS_XXXX_SERIALIZE_ENUM(mepa_cable_diag_status_t, "PortPhyVeriPhyStatus",
                         port_expose_phy_veriphy_status_txt,
                         "This enumerations show the VeriPhy status.");

extern vtss_enum_descriptor_t port_expose_status_speed_txt[];
VTSS_XXXX_SERIALIZE_ENUM_SHARED(mesa_port_speed_t, "PortStatusSpeed",
                                port_expose_status_speed_txt,
                                "This enumerations show the current interface speed.");

//
// Serialization
//
// Generic serializer for mesa_prio_t used as queue index
VTSS_SNMP_TAG_SERIALIZE(vtss_port_queue_index_t, mesa_prio_t, a, s)
{
    a.add_leaf(vtss::AsInt(s.inner),
               vtss::tag::Name("Queue"),
               vtss::expose::snmp::RangeSpec<u32>(0, VTSS_PRIOS - 1),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Queue index."));
}

VTSS_SNMP_TAG_SERIALIZE(vtss_port_ctrl_bool_t, BOOL, a, s)
{
    a.add_leaf(vtss::AsBool(s.inner),
               vtss::tag::Name("StatisticsClear"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Set to TRUE to clear the statistics of an interface."));
}

#ifdef VTSS_UI_OPT_VERIPHY
VTSS_SNMP_TAG_SERIALIZE(vtss_port_veriphy_ctrl_bool_t, BOOL, a, s)
{
    a.add_leaf(vtss::AsBool(s.inner),
               vtss::tag::Name("Start"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Set to TRUE to start VeriPHy for the interface.\n"
                                      "When running 10 and 100 Mbps, ports will be linked down "
                                      "while running VeriPHY. Therefore, running VeriPHY on a 10 "
                                      "or 100 Mbps management port will cause the switch to stop "
                                      "responding until VeriPHY is complete."));
}

#define VERIPHY_STATUS_HELP_TXT \
    "VeriPhy status for the cable pair\n          \
                                  0  - Cable is Correctly terminated pair\n    \
                                  1  - Open pair\n                             \
                                  2  - Shorted pair\n                          \
                                  4  - Abnormal termination\n                  \
                                  8  - Cross-pair short to pair A\n            \
                                  9  - Cross-pair short to pair B\n            \
                                  10 - Cross-pair short to pair C\n            \
                                  11 - Cross-pair short to pair D\n            \
                                  12 - Abnormal cross-pair coupling - pair A\n \
                                  13 - Abnormal cross-pair coupling - pair B\n \
                                  14 - Abnormal cross-pair coupling - pair C\n \
                                  15 - Abnormal cross-pair coupling - pair D"

#define VERIPHY_LENGTH_HELP_TXT \
    "VeriPhy status cable length i meters for the cable pair. \
                                 When VeriPhy is completed, the cable diagnostics results is shown in the VeriPhy status table. Note that VeriPHY is only accurate for cables of length 7 - 140 meters.\n \
                                 The resolution is 3 meters"

template <typename T>
void serialize(T &a, vtss_appl_port_veriphy_result_t &s)
{
    int ix = 0;
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_port_veriphy_result_t"));

    m.add_leaf(s.status_pair_a, vtss::tag::Name("VeriPhyStatusPairA"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(VERIPHY_STATUS_HELP_TXT));

    m.add_leaf(s.status_pair_b, vtss::tag::Name("VeriPhyStatusPairB"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(VERIPHY_STATUS_HELP_TXT));

    m.add_leaf(s.status_pair_c, vtss::tag::Name("VeriPhyStatusPairC"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(VERIPHY_STATUS_HELP_TXT));

    m.add_leaf(s.status_pair_d, vtss::tag::Name("VeriPhyStatusPairD"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(VERIPHY_STATUS_HELP_TXT));

    m.add_leaf(s.length_pair_a, vtss::tag::Name("VeriPhyLengthStatusPairA"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(VERIPHY_LENGTH_HELP_TXT));

    m.add_leaf(s.length_pair_b, vtss::tag::Name("VeriPhyLengthStatusPairB"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(VERIPHY_LENGTH_HELP_TXT));

    m.add_leaf(s.length_pair_c, vtss::tag::Name("VeriPhyLengthStatusPairC"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(VERIPHY_LENGTH_HELP_TXT));

    m.add_leaf(s.length_pair_d, vtss::tag::Name("VeriPhyLengthStatusPairD"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(VERIPHY_LENGTH_HELP_TXT));
}
#endif

VTSS_SNMP_TAG_SERIALIZE(PORT_ifindex_index, vtss_ifindex_t, a, s)
{
    a.add_leaf(vtss::AsInterfaceIndex(s.inner), vtss::tag::Name("IfIndex"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Logical interface number."));
}

VTSS_SNMP_TAG_SERIALIZE(PORT_SERIALIZER_portno_t, mesa_port_no_t, a, s)
{
    a.add_leaf(s.inner,
               vtss::tag::Name("PortNo"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(1),
               vtss::tag::Description("Port number for ifIndex."));
}

/****************************************************************************
 * Capabilities
 ****************************************************************************/

template<typename T>
void serialize(T &a, vtss_appl_port_capabilities_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_port_capabilities_t"));
    int ix = 1;

    m.add_leaf(
        s.last_pktsize_threshold,
        vtss::tag::Name("LastFrameLenThreshold"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The max length of the frames counted in the last range of the frame counter group.")
    );

    m.add_leaf(
        s.has_kr,
        vtss::tag::Name("HasKR"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("If HasKRv2 or HasKRv3 is true, so is this")
    );

    m.add_leaf(
        s.frame_length_max_min,
        vtss::tag::Name("FrameLengthMaxMin"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The smallest maximum acceptable ingress frame length that can be configured.")
    );

    m.add_leaf(
        s.frame_length_max_max,
        vtss::tag::Name("FrameLengthMaxMax"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The largest maximum acceptable ingress frame length that can be configured.")
    );

    m.add_leaf(
        s.has_kr_v2,
        vtss::tag::Name("HasKRv2"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates whether at least one of the ports supports 10G BASE-KR.")
    );

    m.add_leaf(
        s.has_kr_v3,
        vtss::tag::Name("HasKRv3"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("Indicates whether at least one of the ports supports 10G or 25G BASE-KR.")
    );

    m.add_leaf(
        s.aggr_caps,
        vtss::tag::Name("AggrCaps"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The aggregated capability flags for all ports on this platform.")
    );

    m.add_leaf(
        s.port_cnt,
        vtss::tag::Name("PortCnt"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("The number of ports on this device")
    );

    m.add_leaf(
        s.has_pfc,
        vtss::tag::Name("HasPFC"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(ix++),
        vtss::tag::Description("True if this platform supports priority-based flow control")
    );
}

template <typename T>
void serialize(T &a, mesa_port_rmon_counters_t &s)
{
    int ix = 0;
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("mesa_port_rmon_counters_t"));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsDropEvents),
               vtss::tag::Name("RxDropEvents"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of frames discarded due to ingress "
                                      "congestion."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsOctets),
               vtss::tag::Name("RxOctets"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of received (good and bad) bytes. "
                                      "Includes FCS, but excludes framing bits."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsPkts),
               vtss::tag::Name("RxPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of received (good and bad) packets."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsBroadcastPkts),
               vtss::tag::Name("RxBroadcastPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of received (good and bad) broadcast "
                                      "packets."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsMulticastPkts),
               vtss::tag::Name("RxMulticastPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of received (good and bad) multicast "
                                      "packets."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsCRCAlignErrors),
               vtss::tag::Name("RxCrcAlignErrPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of frames received with CRC or "
                                      "alignment errors."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsUndersizePkts),
               vtss::tag::Name("RxUndersizePkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of short frames (frames that are "
                                      "smaller than 64 bytes) received with valid CRC."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsOversizePkts),
               vtss::tag::Name("RxOversizePkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of long frames received with valid "
                                      "CRC."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsFragments),
               vtss::tag::Name("RxFragmentsPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of short frames (frames that are "
                                      "smaller than 64 bytes) received with invalid CRC."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsJabbers),
               vtss::tag::Name("RxJabbersPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of long frames (frames that are longer than "
                                      "the configured maximum frame length for this "
                                      "interface) received with invalid CRC."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsPkts64Octets),
               vtss::tag::Name("Rx64Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of 64 bytes frames received."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsPkts65to127Octets),
               vtss::tag::Name("Rx65to127Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of received frames with size within 65 to "
                                      "127 bytes."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsPkts128to255Octets),
               vtss::tag::Name("Rx128to255Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of received frames with size within 128 to "
                                      "255 bytes."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsPkts256to511Octets),
               vtss::tag::Name("Rx256to511Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of received frames with size within 256 to "
                                      "511 bytes."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsPkts512to1023Octets),
               vtss::tag::Name("Rx512to1023Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of received frames with size within 512 to "
                                      "1023 bytes."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsPkts1024to1518Octets),
               vtss::tag::Name("Rx1024to1518Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of received frames with size within 1024 to "
                                      "nn bytes where nn is varying between platforms. The actual value of nn "
                                      "is shown in the capabilities:LastFrameLenThreshold parameter."));

    m.add_leaf(vtss::AsCounter(s.rx_etherStatsPkts1519toMaxOctets),
               vtss::tag::Name("Rx1519PktsToMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of received frames with size larger than "
                                      "nn bytes where nn is varying between platforms. The actual value of nn "
                                      "is shown in the capabilities:LastFrameLenThreshold parameter."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsDropEvents),
               vtss::tag::Name("TxDropEvents"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of frames discarded due to egress "
                                      "congestion."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsOctets),
               vtss::tag::Name("TxOctets"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of transmitted (good and bad) bytes. "
                                      "Includes FCS, but excludes framing bits."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsPkts),
               vtss::tag::Name("TxPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of transmitted (good and bad) packets."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsBroadcastPkts),
               vtss::tag::Name("TxBroadcastPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of transmitted (good and bad) "
                                      "broadcast packets."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsMulticastPkts),
               vtss::tag::Name("TxMulticastPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of transmitted (good and bad) "
                                      "multicast packets."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsPkts64Octets),
               vtss::tag::Name("Tx64Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of 64 bytes frames transmitted."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsPkts65to127Octets),
               vtss::tag::Name("Tx65to127Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of transmitted frames with size within 65 "
                                      "to 127 bytes."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsPkts128to255Octets),
               vtss::tag::Name("Tx128to255Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of transmitted frames with size within 128 "
                                      "to 255 bytes."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsPkts256to511Octets),
               vtss::tag::Name("Tx256to511Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of transmitted frames with size within 256 "
                                      "to 511 bytes."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsPkts512to1023Octets),
               vtss::tag::Name("Tx512to1023Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of transmitted frames with size within 512 "
                                      "to 1023 bytes."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsPkts1024to1518Octets),
               vtss::tag::Name("Tx1024to1518Pkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of transmitted frames with size within 1024 "
                                      "to nn bytes where nn is varying between platforms. The actual value of nn "
                                      "is shown in the capabilities:LastFrameLenThreshold parameter."));

    m.add_leaf(vtss::AsCounter(s.tx_etherStatsPkts1519toMaxOctets),
               vtss::tag::Name("Tx1519PktsToMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of transmitted frames with size larger than "
                                      "nn bytes where nn is varying between platforms. The actual value of nn "
                                      "is shown in the capabilities:LastFrameLenThreshold parameter."));
}

template <typename T>
void serialize(T &a, mesa_port_ethernet_like_counters_t &s)
{
    int ix = 0;
    typename T::Map_t m = a.as_map(vtss::tag::Typename("mesa_port_ethernet_like_counters_t"));

    m.add_leaf(vtss::AsCounter(s.dot3InPauseFrames),
               vtss::tag::Name("RxPauseFrames"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of pause frames received."));

    m.add_leaf(vtss::AsCounter(s.dot3OutPauseFrames),
               vtss::tag::Name("TxPauseFrames"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of pause frames transmitted."));
}

template <typename T>
void serialize(T &a, mesa_port_bridge_counters_t &s)
{
    int ix = 0;
    typename T::Map_t m = a.as_map(vtss::tag::Typename("mesa_port_bridge_counters_t"));

    m.add_leaf(vtss::AsCounter(s.dot1dTpPortInDiscards),
               vtss::tag::Name("RxBridgeDiscard"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of bridge frames discarded."));
}

template <typename T>
void serialize(T &a, port_expose_prio_counter_t &s)
{
    int ix = 0;

    // Back in the days, this was called vtss_appl_port_prio_counter_t.
    // See also comment in port_expose.hxx.
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_port_prio_counter_t"));

    m.add_leaf(vtss::AsCounter(s.rx_prio), vtss::tag::Name("RxPrio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of frames received for the queue."));

    m.add_leaf(vtss::AsCounter(s.tx_prio), vtss::tag::Name("TxPrio"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The number of frames transmitted for the queue."));
}

template <typename T>
void serialize(T &a, mesa_port_if_group_counters_t &s)
{
    int ix = 0;
    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("mesa_port_if_group_counters_t"));

    m.add_leaf(vtss::AsCounter(s.ifInOctets), vtss::tag::Name("RxOctets"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of bytes received."));

    m.add_leaf(vtss::AsCounter(s.ifInUcastPkts),
               vtss::tag::Name("RxUnicastPkts"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of uni-cast frames received."));

    m.add_leaf(vtss::AsCounter(s.ifInMulticastPkts),
               vtss::tag::Name("RxMulticastPkts"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of multi-cast frames received."));

    m.add_leaf(vtss::AsCounter(s.ifInBroadcastPkts),
               vtss::tag::Name("RxBroadcastPkts"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of broad-cast frames received."));

    m.add_leaf(vtss::AsCounter(s.ifInNUcastPkts),
               vtss::tag::Name("RxNonUnicastPkts"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of non uni-cast frames received."));

    m.add_leaf(vtss::AsCounter(s.ifInDiscards), vtss::tag::Name("RxDiscards"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of received frames discarded."));

    m.add_leaf(vtss::AsCounter(s.ifInErrors), vtss::tag::Name("RxErrors"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of frames with errors received."));

    m.add_leaf(vtss::AsCounter(s.ifOutOctets),
               vtss::tag::Name("TxOctets"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of bytes transmitted."));

    m.add_leaf(vtss::AsCounter(s.ifOutUcastPkts),
               vtss::tag::Name("TxUnicastPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of uni-cast frames transmitted."));

    m.add_leaf(vtss::AsCounter(s.ifOutMulticastPkts),
               vtss::tag::Name("TxMulticastPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of multi-cast frames transmitted."));

    m.add_leaf(vtss::AsCounter(s.ifOutBroadcastPkts),
               vtss::tag::Name("TxBroadcastPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of broad-cast frames transmitted."));

    m.add_leaf(vtss::AsCounter(s.ifOutNUcastPkts),
               vtss::tag::Name("TxNonUnicastPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of non uni-cast frames transmitted."));

    m.add_leaf(vtss::AsCounter(s.ifOutDiscards),
               vtss::tag::Name("TxDiscardPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of discard frames which should been "
                                      "transmitted."));

    m.add_leaf(vtss::AsCounter(s.ifOutErrors),
               vtss::tag::Name("TxErrorPkts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the number of frames transmit with error."));
}

template <typename T>
void serialize(T &a, mesa_port_dot3br_counters_t &s)
{
    int ix = 0;
    typename T::Map_t m = a.as_map(vtss::tag::Typename("mesa_port_dot3br_counters_t"));

    m.add_leaf(vtss::AsCounter(s.aMACMergeFrameAssErrorCount),
               vtss::tag::Name("aMACMergeFrameAssErrorCount"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("A count of MAC frames with reassembly errors."));

    m.add_leaf(vtss::AsCounter(s.aMACMergeFrameSmdErrorCount),
               vtss::tag::Name("aMACMergeFrameSmdErrorCount"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("A count of received MAC frames / MAC frame fragments rejected due to "
                                      "unknown SMD value or arriving with an SMD-C when no frame is in progress."));

    m.add_leaf(vtss::AsCounter(s.aMACMergeFrameAssOkCount),
               vtss::tag::Name("aMACMergeFrameAssOkCount"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("A count of MAC frames that were successfully reassembled and delivered to MAC."));

    m.add_leaf(vtss::AsCounter(s.aMACMergeFragCountRx),
               vtss::tag::Name("aMACMergeFragCountRx"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("A count of the number of additional mPackets received due to preemption."));

    m.add_leaf(vtss::AsCounter(s.aMACMergeFragCountTx),
               vtss::tag::Name("aMACMergeFragCountTx"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("A count of the number of additional mPackets transmitted due to preemption."));

    m.add_leaf(vtss::AsCounter(s.aMACMergeHoldCount),
               vtss::tag::Name("aMACMergeHoldCount"), vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("A count of the number of times the variable hold (see 802.3br, section 99.4.7.3) transitions from FALSE to TRUE."));
}

template <typename T>
void serialize(T &a, vtss_appl_port_status_t &s)
{
    int ix = 0;
    mesa_bool_t b;

    typename T::Map_t m =
        a.as_map(vtss::tag::Typename("vtss_appl_port_status_t"));

    m.add_leaf(vtss::AsBool(s.link),
               vtss::tag::Name("Link"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows whether interface has link."));

    m.add_leaf(vtss::AsBool(s.fdx),
               vtss::tag::Name("Fdx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows whether interface is running in full duplex."));

    m.add_leaf(vtss::AsBool(s.fiber),
               vtss::tag::Name("Fiber"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows whether interface is an SFP interface or RJ45."));

    m.add_leaf(s.speed,
               vtss::tag::Name("Speed"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the current interface speed."));

    m.add_leaf(s.sfp_info.transceiver,
               vtss::tag::Name("SFPType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the current interface SFP type."));

    m.add_leaf(vtss::AsDisplayString(s.sfp_info.vendor_name, sizeof(s.sfp_info.vendor_name)),
               vtss::tag::Name("SFPVendorName"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the SFP vendor name."));

    m.add_leaf(vtss::AsDisplayString(s.sfp_info.vendor_pn, sizeof(s.sfp_info.vendor_pn)),
               vtss::tag::Name("SFPVendorPN"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the SFP vendor Product Number."));

    m.add_leaf(vtss::AsDisplayString(s.sfp_info.vendor_rev, sizeof(s.sfp_info.vendor_rev)),
               vtss::tag::Name("SFPVendorRev"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the SFP vendor Revision."));

    b = s.fiber && !s.link;
    m.add_rpc_leaf(vtss::AsBool(b),
                   vtss::tag::Name("LossOfSignal"),
                   vtss::tag::Description("SFP Loss Of Signal."));

    b = false;
    m.add_rpc_leaf(vtss::AsBool(b),
                   vtss::tag::Name("TxFault"),
                   vtss::tag::Description("SFP Transmit Fault."));

    b = s.sfp_info.vendor_pn[0] != '\0';
    m.add_rpc_leaf(vtss::AsBool(b),
                   vtss::tag::Name("Present"),
                   vtss::tag::Description("SFP module present."));

    m.add_leaf(vtss::AsDisplayString(s.sfp_info.vendor_sn, sizeof(s.sfp_info.vendor_sn)),
               vtss::tag::Name("SFPVendorSN"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the SFP vendor Serial Number."));

    m.add_leaf(vtss::AsDisplayString(s.sfp_info.date_code, sizeof(s.sfp_info.date_code)),
               vtss::tag::Name("SFPDateCode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Shows the SFP Date Code."));

    m.add_leaf(s.fec_mode,
               vtss::tag::Name("FecMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Tells the current FEC mode used on the port."));

    m.add_leaf(s.link_up_cnt,
               vtss::tag::Name("LinkUpCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates number of times this port has gotten link up."));

    m.add_leaf(s.link_down_cnt,
               vtss::tag::Name("LinkDownCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates number of times this port has gotten link down."));
}

template <typename T>
void serialize(T &a, port_expose_port_conf_t &s)
{
    int ix = 0;
    // Back in the days, this was called vtss_appl_port_mib_conf_t.
    // See also comment in port_expose.hxx.
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_port_mib_conf_t"));

    m.add_leaf(vtss::AsBool(s.shutdown),
               vtss::tag::Name("Shutdown"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls whether interface is shutdown or powered up. "
                                      "Set to TRUE in order to power down the interface."));

    m.add_leaf(s.speed, vtss::tag::Name("Speed"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls the port speed and duplex."));

    m.add_leaf((uint32_t)s.advertise_dis,
               vtss::tag::Name("AdvertiseDisabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("In auto mode, bitmask that allows features not \
                                       to be advertised.\n \
                                       Bit 0: Disable half duplex advertising.\n\
                                       Bit 1: Disable full duplex advertising.\n\
                                       Bit 3: Disable 2.5G advertising.\n\
                                       Bit 4: Disable 1G advertising.\n\
                                       Bit 6: Disable 100M advertising.\n\
                                       Bit 7: Disable 10M advertising.\n\
                                       Bit 8: Disable 5G advertising.\n\
                                       Bit 9: Disable 10G advertising.\n\
                                       When not in auto mode, the value shall be zero."));

    m.add_leaf(s.media,
               vtss::tag::Name("MediaType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls the port media type."));

    m.add_leaf(s.fc,
               vtss::tag::Name("FC"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls the port flow control mode."));

    m.add_leaf(s.mtu,
               vtss::tag::Name("MTU"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls the port's Maximum Transmission Unit."));

    m.add_leaf(vtss::AsBool(s.excessive),
               vtss::tag::Name("ExcessiveRestart"),
               vtss::expose::snmp::Status::Current, vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("TRUE to restart half-duplex back-off algorithm after 16 "
                                      "collisions. FALSE to discard frame after 16 collisions"));

    m.add_leaf(s.pfc_mask,
               vtss::tag::Name("PFC"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("802.1Qbb Priority Flow Control bitmask, one bit for each priority."
                                      "E.g. 0x01 = prio 0, 0x80 = prio 7, 0xFF = prio 0-7"));

    m.add_leaf(vtss::AsBool(s.frame_length_chk),
               vtss::tag::Name("FrameLengthCheck"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("TRUE to enforce 802.3 frame length check (from Ethertype field). If enabled frames with length that doesn't match the frame length field will be dropped."));

    m.add_leaf(vtss::AsBool(s.force_clause_73),
               vtss::tag::Name("ForceClause73"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("TRUE to enforce 802.3 clause 73 aneg. Speed must be set to 'AUTO' in that case."));

    m.add_leaf(s.fec_mode,
               vtss::tag::Name("FECMode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls whether to force a given Forward Error Correction mode on the port."));
}

namespace vtss
{
namespace appl
{
namespace port
{
namespace interfaces
{
struct PortCapabilities {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamVal<vtss_appl_port_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_port_capabilities_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(0));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_port_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PORT);
};

struct PortParams {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<port_expose_port_conf_t *>> P;

    static constexpr const char *table_description =
        "This is a table of the port interface parameters";

    static constexpr const char *index_description =
        "Each port interface has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(port_expose_port_conf_t &i)
    {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_GET_PTR(port_expose_port_conf_get);
    VTSS_EXPOSE_SET_PTR(port_expose_port_conf_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PORT);
};

struct PortStatusRmonStatistics {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<mesa_port_rmon_counters_t *>> P;

    static constexpr const char *table_description =
        "This table represents the port interface RMON statistics "
        "counters";

    static constexpr const char *index_description =
        "Each port interface has a set of statistics counters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_port_rmon_counters_t &i)
    {
        h.argument_properties(tag::Name("statistics"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(port_rmon_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT);
};

struct PortStatusIfGroupStatistics {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<mesa_port_if_group_counters_t>> P;

    static constexpr const char *table_description =
        R"(This table represents the port interfaces group (RFC 2863) )"
        R"(counters)";

    static constexpr const char *index_description =
        R"(Each port interface has a set of statistics counters)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_port_if_group_counters_t &i)
    {
        h.argument_properties(tag::Name("counters"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(port_if_group_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT);
};

struct PortStatusEthernetLikeStatstics {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<mesa_port_ethernet_like_counters_t *>> P;

    static constexpr const char *table_description =
        R"(This table represents the port Ethernet-like interfaces )"
        R"(counters)";

    static constexpr const char *index_description =
        R"(Each port interface has a set of statistics counters)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_port_ethernet_like_counters_t &i)
    {
        h.argument_properties(tag::Name("counters"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(port_ethernet_like_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT);
};

struct PortStatusBridgeStatistics {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<mesa_port_bridge_counters_t *>> P;

    static constexpr const char *table_description =
        R"(This table represents the port interface bridge counters)";

    static constexpr const char *index_description =
        R"(Each port interface has a set of statistics counters)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_port_bridge_counters_t &i)
    {
        h.argument_properties(tag::Name("counters"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(port_bridge_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT);
};

struct PortStatusQueuesStatistics {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>, expose::ParamKey<mesa_prio_t>,
            expose::ParamVal<port_expose_prio_counter_t *>> P;

    static constexpr const char *table_description =
        R"(This table represents the port interfaces queues counters)";

    static constexpr const char *index_description =
        R"(Each port interface has a set of statistics counters)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_prio_t &i)
    {
        h.argument_properties(tag::Name("prio"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, vtss_port_queue_index_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(port_expose_prio_counter_t &i)
    {
        h.argument_properties(tag::Name("counter"));
        h.argument_properties(expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(port_expose_qu_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_ifindex_getnext_port_queue);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT);
};

struct PortStatusDot3brStatistics {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<mesa_port_dot3br_counters_t *>> P;

    static constexpr const char *table_description =
        R"(This table represents the port 802.3br counters)";

    static constexpr const char *index_description =
        R"(Each port interface has a set of statistics counters)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_port_dot3br_counters_t &i)
    {
        h.argument_properties(tag::Name("counters"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(port_dot3br_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT);
};

struct PortStatusParams {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<vtss_appl_port_status_t *>> P;

    static constexpr const char *table_description =
        R"(This table represents the status of the ports)";

    static constexpr const char *index_description =
        R"(Each port interface has a set of status parameters)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_port_status_t &i)
    {
        h.argument_properties(tag::Name("status"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_port_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT);
};

struct PortStatsClear {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<BOOL *>> P;

    static constexpr const char *table_description =
        R"(This is a table to clear port Interface statistics)";

    static constexpr const char *index_description =
        R"(Each port has a set of control parameters)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(tag::Name("clear"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, vtss_port_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_GET_PTR(port_stats_dummy_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_SET_PTR(port_stats_clr_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT);
};

#ifdef VTSS_UI_OPT_VERIPHY
struct PortVerifyStart {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<BOOL *>> P;

    static constexpr const char *table_description =
        R"(This is a table to start VeriPhy for the interface)";

    static constexpr const char *index_description =
        R"(Each port has a set of control parameters)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(tag::Name("ctrl"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, vtss_port_veriphy_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_GET_PTR(port_stats_dummy_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_SET_PTR(port_expose_veriphy_start_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_PORT);
};

struct PortVeriphyResult {
    typedef expose::ParamList <expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<vtss_appl_port_veriphy_result_t *>> P;

    static constexpr const char *table_description =
        R"(This table represents the VeriPhy result from the last VeriPhy )"
        R"(run for the interface)";

    static constexpr const char *index_description =
        R"(Each port has a set of VeriPhy results)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_port_veriphy_result_t &i)
    {
        h.argument_properties(tag::Name("res"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(port_expose_veriphy_result_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port_exist);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT);
};
#endif  // VTSS_UI_OPT_VERIPHY

/**
 * A map between port ifindex string values and the external port number (uport).
 * The JSON interface encodes the port ifindex value as a string on the form
 * "Gi 1/1" (for GigabitEthernet 1/1). This map allows the JSON client to obtain
 * the uport number for a port given the encoded ifindex value.
 */
struct PortNameMap {
    typedef expose::ParamList<expose::ParamKey<vtss_ifindex_t>, expose::ParamVal<mesa_port_no_t>> P;

    static constexpr const char *table_description = R"(Table of mapping between port ifIndex name and port number)";
    static constexpr const char *index_description = R"(Each row contains the status for one interface-to-port number mapping)";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(tag::Name("ifindex"));
        h.argument_properties(expose::snmp::OidOffset(0));
        serialize(h, PORT_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_port_no_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, PORT_SERIALIZER_portno_t(i));
    }

    VTSS_EXPOSE_GET_PTR(port_expose_interface_portno_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_iterator_ifindex_front_port);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_PORT);
};

}  // namespace interfaces
}  // namespace port
}  // namespace appl
}  // namespace vtss

#endif
