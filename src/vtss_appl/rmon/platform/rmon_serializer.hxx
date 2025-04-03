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

#ifndef __VTSS_RMON_SERIALIZER_HXX__
#define __VTSS_RMON_SERIALIZER_HXX__

#include "vtss/appl/rmon.h"
#include "vtss_appl_serialize.hxx"

extern vtss_enum_descriptor_t rmon_event_log_trap_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_event_t,
                         "RmonEventType",
                         rmon_event_log_trap_type_txt,
                         "This enumeration is to select a perticular event type.");

extern vtss_enum_descriptor_t rmon_if_counter_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_rmon_alarm_if_t,
                         "RmonIfCounterType",
                         rmon_if_counter_type_txt,
                         "This enumeration is to select a perticular counter type on an interface.");

extern vtss_enum_descriptor_t rmon_alarm_sample_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_alarm_sample_t,
                         "RmonSampleType",
                         rmon_alarm_sample_type_txt,
                         "This enumeration is to select the rmon sample type.");

extern vtss_enum_descriptor_t rmon_alarm_startup_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_alarm_t,
                         "RmonStartupType",
                         rmon_alarm_startup_type_txt,
                         "This enumeration is to select the rmon startup type.");

VTSS_SNMP_TAG_SERIALIZE(rmon_entry_index, u32, a, s) {
    a.add_leaf(
        s.inner,
        vtss::tag::Name("RmonEntryIndex"),
        vtss::tag::Description("RMON entry index number.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(event_last_time_sent, u32, a, s) {
    a.add_leaf(
        s.inner,
        vtss::tag::Name("RmonEventLastTimeSent"),
        vtss::tag::Description("RMON event last time sent.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_stats_ctrl_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_stats_ctrl_entry_t"));
    m.add_leaf(
        vtss::AsInterfaceIndex(s.ifIndex),
        vtss::tag::Name("Ifindex"),
        vtss::tag::Description("Interface index")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_stats_statistics_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_stats_statistics_entry_t"));

    m.add_leaf(
        s.etherStatsCreateTime,
        vtss::tag::Name("etherStatsCreateTime"),
        vtss::tag::Description("The time when the entry is activated.")
    );

    m.add_leaf(
        s.eth.drop_events,
        vtss::tag::Name("etherStatsDropEvents"),
        vtss::tag::Description("Dropped frames counter.")
    );

    m.add_leaf(
        s.eth.octets,
        vtss::tag::Name("etherStatsOctets"),
        vtss::tag::Description("Number of octets/bytes.")
    );

    m.add_leaf(
        s.eth.packets,
        vtss::tag::Name("etherStatsPkts"),
        vtss::tag::Description("Packets counter.")
    );

    m.add_leaf(
        s.eth.bcast_pkts,
        vtss::tag::Name("etherStatsBroadcastPkts"),
        vtss::tag::Description("Broadcoast packets count")
    );

    m.add_leaf(
        s.eth.mcast_pkts,
        vtss::tag::Name("etherStatsMulticastPkts"),
        vtss::tag::Description("Multicast packets count")
    );

    m.add_leaf(
        s.eth.crc_align,
        vtss::tag::Name("etherStatsCRCAlignErrors"),
        vtss::tag::Description("CRC error count")
    );

    m.add_leaf(
        s.eth.undersize,
        vtss::tag::Name("etherStatsUndersizePkts"),
        vtss::tag::Description("Undersized packets count")
    );

    m.add_leaf(
        s.eth.oversize,
        vtss::tag::Name("etherStatsOversizePkts"),
        vtss::tag::Description("Oversized packets count")
    );

    m.add_leaf(
        s.eth.fragments,
        vtss::tag::Name("etherStatsFragments"),
        vtss::tag::Description("Fragments count")
    );

    m.add_leaf(
        s.eth.jabbers,
        vtss::tag::Name("etherStatsJabbers"),
        vtss::tag::Description("Jabber frame count")
    );

    m.add_leaf(
        s.eth.collisions,
        vtss::tag::Name("etherStatsCollisions"),
        vtss::tag::Description("Collision error count")
    );

    m.add_leaf(
        s.eth.pkts_64,
        vtss::tag::Name("etherStatsPkts64Octets"),
        vtss::tag::Description("pkts sized less than 64 bytes")
    );

    m.add_leaf(
        s.eth.pkts_65_127,
        vtss::tag::Name("etherStatsPkts65to127Octets"),
        vtss::tag::Description("pkts sized from 64 to 128")
    );

    m.add_leaf(
        s.eth.pkts_128_255,
        vtss::tag::Name("etherStatsPkts128to255Octets"),
        vtss::tag::Description("pkts sized from 128 to 255")
    );

    m.add_leaf(
        s.eth.pkts_256_511,
        vtss::tag::Name("etherStatsPkts256to511Octets"),
        vtss::tag::Description("pkts sized from 256 to 511")
    );

    m.add_leaf(
        s.eth.pkts_512_1023,
        vtss::tag::Name("etherStatsPkts512to1023Octets"),
        vtss::tag::Description("pkts sized from 512 to 1023")
    );

    m.add_leaf(
        s.eth.pkts_1024_1518,
        vtss::tag::Name("etherStatsPkts1024to1518Octets"),
        vtss::tag::Description("pkts sized from 1024 to 1518")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_history_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_history_entry_t"));

    m.add_leaf(
        vtss::AsInterfaceIndex(s.ifIndex),
        vtss::tag::Name("Ifindex"),
        vtss::tag::Description("Interface index")
    );

    m.add_leaf(
        s.interval,
        vtss::expose::snmp::RangeSpec<u32>(1, 1800),
        vtss::tag::Name("Interval"),
        vtss::tag::Description("Sampling interval")
    );

    m.add_leaf(
        s.data_requested,
        vtss::expose::snmp::RangeSpec<u32>(1, RMON_BUCKET_CNT_MAX),
        vtss::tag::Name("BucketSize"),
        vtss::tag::Description("Requested data bucket size")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_history_status_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_history_status_t"));

    m.add_leaf(
        s.data_granted,
        vtss::tag::Name("DataGranted"),
        vtss::tag::Description("Size of data bucket granted in added history entry")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_history_statistics_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_history_statistics_t"));

    m.add_leaf(
        s.start_interval,
        vtss::tag::Name("etherHistoryIntervalStart"),
        vtss::tag::Description("Data entry startup time")
    );

    m.add_leaf(
        s.EthData.drop_events,
        vtss::tag::Name("etherHistoryDropEvents"),
        vtss::tag::Description("Dropped frames counter")
    );

    m.add_leaf(
        s.EthData.octets,
        vtss::tag::Name("etherHistoryOctets"),
        vtss::tag::Description("Number of octets/bytes")
    );

    m.add_leaf(
        s.EthData.packets,
        vtss::tag::Name("etherHistoryPkts"),
        vtss::tag::Description("Packets counter")
    );

    m.add_leaf(
        s.EthData.bcast_pkts,
        vtss::tag::Name("etherHistoryBroadcastPkts"),
        vtss::tag::Description("Broadcoast packets count")
    );

    m.add_leaf(
        s.EthData.mcast_pkts,
        vtss::tag::Name("etherHistoryMulticastPkts"),
        vtss::tag::Description("Multicast packets count")
    );

    m.add_leaf(
        s.EthData.crc_align,
        vtss::tag::Name("etherHistoryCRCAlignErrors"),
        vtss::tag::Description("CRC error count")
    );

    m.add_leaf(
        s.EthData.undersize,
        vtss::tag::Name("etherHistoryUndersizePkts"),
        vtss::tag::Description("Undersized packets count")
    );

    m.add_leaf(
        s.EthData.oversize,
        vtss::tag::Name("etherHistoryOversizePkts"),
        vtss::tag::Description("Oversized packets count")
    );

    m.add_leaf(
        s.EthData.fragments,
        vtss::tag::Name("etherHistoryFragments"),
        vtss::tag::Description("Fragments count")
    );

    m.add_leaf(
        s.EthData.jabbers,
        vtss::tag::Name("etherHistoryJabbers"),
        vtss::tag::Description("Jabber frame count")
    );

    m.add_leaf(
        s.EthData.collisions,
        vtss::tag::Name("etherHistoryCollisions"),
        vtss::tag::Description("Collision error count")
    );

    m.add_leaf(
        s.utilization,
        vtss::tag::Name("etherHistoryUtilization"),
        vtss::tag::Description("The untilization")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_alarm_ctrl_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_alarm_ctrl_entry_t"));

    m.add_leaf(
        s.interval,
        vtss::expose::snmp::RangeSpec<u32>(1, 2147483647),
        vtss::tag::Name("Interval"),
        vtss::tag::Description("Sampling interval")
    );

    m.add_leaf(
        s.var_name,
        vtss::tag::Name("VarName"),
        vtss::tag::Description("The source data to be compared")
    );

    m.add_leaf(
        s.sample_type,
        vtss::tag::Name("SampleType"),
        vtss::tag::Description("The source data to be computed with absolute value or delta value")
    );

    m.add_leaf(
        s.startup_type,
        vtss::tag::Name("StartupType"),
        vtss::tag::Description("Trigger rising or falling alarm when startup")
    );

    m.add_leaf(
        vtss::AsInterfaceIndex(s.ifIndex),
        vtss::tag::Name("Ifindex"),
        vtss::tag::Description("Interface index")
    );

    m.add_leaf(
        s.rising_threshold,
        vtss::tag::Name("RisingThreshold"),
        vtss::tag::Description("Rising threshold")
    );

    m.add_leaf(
        s.falling_threshold,
        vtss::tag::Name("FallingThreshold"),
        vtss::tag::Description("Falling threshold")
    );

    m.add_leaf(
        s.rising_event_index,
        vtss::expose::snmp::RangeSpec<u32>(0, 65535),
        vtss::tag::Name("RisingEventIndex"),
        vtss::tag::Description("Rising event index")
    );

    m.add_leaf(
        s.falling_event_index,
        vtss::expose::snmp::RangeSpec<u32>(0, 65535),
        vtss::tag::Name("FallingEventIndex"),
        vtss::tag::Description("Falling event index")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_event_entry_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_event_entry_t"));

    m.add_leaf(
        vtss::AsDisplayString(s.event_description, VTSS_APPL_RMON_LOG_STRING_LENGTH),
        vtss::tag::Name("EventDescription"),
        vtss::tag::Description("Event description")
    );

    m.add_leaf(
        s.event_type,
        vtss::tag::Name("EventType"),
        vtss::tag::Description("RMON event stored type")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_event_data_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_event_data_t"));

    m.add_leaf(
        s.log_time,
        vtss::tag::Name("LogTime"),
        vtss::tag::Description("Time when this log entry was created")
    );

    m.add_leaf(
        vtss::AsDisplayString(s.log_description, VTSS_APPL_RMON_LOG_STRING_LENGTH),
        vtss::tag::Name("LogDescription"),
        vtss::tag::Description("Log description string")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_alarm_value_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_alarm_value_t"));

    m.add_leaf(
        s.value,
        vtss::tag::Name("Value"),
        vtss::tag::Description("RMON alarm trigger count value")
    );
}

namespace vtss {
namespace appl {
namespace rmon {
namespace interfaces {
struct RmonstatisticsEntryAddDeleteTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_stats_ctrl_entry_t *>
    > P;

    static constexpr const char *table_description =
        "The table is RMON statistics add/delete table entry.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_stats_ctrl_entry_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_rmon_statistics_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rmon_statistics_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_rmon_statistics_entry_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_rmon_statistics_entry_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_rmon_statistics_entry_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};

struct RmonStatisticsDataTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_stats_statistics_entry_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to show statistics data";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_stats_statistics_entry_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_rmon_statistics_stats_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rmon_statistics_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};
struct RmonHistoryEntryAddDeleteTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_history_entry_t *>
    > P;

    static constexpr const char *table_description =
        "The table is RMON history add/delete table entry.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_history_entry_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_rmon_history_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rmon_history_entry_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_rmon_history_entry_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_rmon_history_entry_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_rmon_history_entry_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};
struct RmonHistoryStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_history_status_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to history entry status";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_history_status_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_rmon_history_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rmon_history_entry_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};
struct RmonHistoryStatisticsTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_history_statistics_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to show statistics data";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_history_statistics_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_rmon_history_stats_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rmon_history_stats_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};
struct RmonAlarmEntryAddDeleteTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_alarm_ctrl_entry_t *>
    > P;

    static constexpr const char *table_description =
        "The table is RMON statistics add/delete table entry.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_alarm_ctrl_entry_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_rmon_alarm_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rmon_alarm_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_rmon_alarm_entry_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_rmon_alarm_entry_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_rmon_alarm_entry_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};
struct RmonEventEntryAddDeleteTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_event_entry_t *>
    > P;

    static constexpr const char *table_description =
        "The table is RMON statistics add/delete table entry.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_event_entry_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_rmon_event_entry_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rmon_event_entry_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_rmon_event_entry_add);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_rmon_event_entry_add);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_rmon_event_entry_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};
struct RmonEventStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_event_data_t *>
    > P;

    static constexpr const char *table_description =
        "The table is RMON event status table entry.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_event_data_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_rmon_event_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rmon_event_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};
struct RmonEventLastSentStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<u32 *>
    > P;

    static constexpr const char *table_description =
        "The table is RMON event last sent status table entry.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u32 &i) {
        serialize(h, event_last_time_sent(i));
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_rmon_event_status_last_sent);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rmon_event_entry_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};
struct RmonAlarmStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_alarm_value_t *>
    > P;

    static constexpr const char *table_description =
        "The table is RMON event status table entry.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, rmon_entry_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_alarm_value_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_rmon_alarm_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_rmon_alarm_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SECURITY_NETWORK);
};
}  // namespace interfaces
}  // namespace rmon
}  // namespace appl
}  // namespace vtss
 
#endif /* __VTSS_RMON_SERIALIZER_HXX__ */  
