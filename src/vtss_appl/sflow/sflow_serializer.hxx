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
#ifndef __VTSS_SFLOW_SERIALIZER_HXX__
#define __VTSS_SFLOW_SERIALIZER_HXX__

#include "vtss/appl/sflow.h"
#include "vtss_appl_serialize.hxx"

mesa_rc sflow_rcvr_statistics_clr_dummy_get(u32 rcvr_idx, BOOL *const clear);
mesa_rc sflow_rcvr_statistics_clr_set(u32 rcvr_idx, const BOOL *const clear);
mesa_rc sflow_instance_statistics_clr_dummy_get(vtss_ifindex_t ifindex, u16 instance, BOOL *const clear);
mesa_rc sflow_instance_statistics_clr_set(vtss_ifindex_t ifindex, u16 instance, const BOOL *const clear);

extern vtss_enum_descriptor_t sflow_flow_sampling_type_txt[];
VTSS_XXXX_SERIALIZE_ENUM(mesa_sflow_type_t,
                         "SflowFlowSamplingDirection",
                         sflow_flow_sampling_type_txt,
                         "This enumeration control the agent IP Address type.");

VTSS_SNMP_TAG_SERIALIZE(sflow_instance_index, u16, a, s) {
    a.add_leaf(
        s.inner,
        vtss::tag::Name("SflowInstance"),
        vtss::tag::Description("sFlow instance number.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(sflow_rcvr_index, u32, a, s) {
    a.add_leaf(
        s.inner,
        vtss::tag::Name("SflowRcvrIndex"),
        vtss::tag::Description("sFlow receiver index number.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(sflow_ifindex_index, vtss_ifindex_t, a, s ) {
    a.add_leaf(
        vtss::AsInterfaceIndex(s.inner),
        vtss::tag::Name("IfIndex"),
        vtss::tag::Description("Logical interface number of the physical port.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(vtss_sflow_ctrl_bool_t, BOOL, a, s) {
    a.add_leaf(
        vtss::AsBool(s.inner),
        vtss::tag::Name("RcvrStatisticsClear"),
        vtss::tag::Description("Set to true to clear the statistics of a sflow receiver.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sflow_agent_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sflow_agent_t"));

    m.add_leaf(
        s.agent_ip_addr,
        vtss::tag::Name("AgentIpAddress"),
        vtss::tag::Description("This switch's IP address, or rather,\
            by default the IPv4 loopback address, but it can be changed\
            from CLI and Web. Will not get persisted. The aggregate type\
            allows for holding either an IPv4 and IPv6 address.")
    );
}


template<typename T>
void serialize(T &a, vtss_appl_sflow_rcvr_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sflow_rcvr_t"));

    m.add_leaf(
        vtss::AsDisplayString(s.owner, VTSS_APPL_SFLOW_OWNER_LEN),
        vtss::tag::Name("OwnerOfReceiverEntry"),
        vtss::tag::Description("The entity making use of this receiver table entry.")
    );

    m.add_leaf(
        s.timeout,
        vtss::tag::Name("ReceiverTimeout"),
        vtss::tag::Description("The time (in seconds) remaining before the sampler is\
            released and stops sampling.")
    );

    m.add_leaf(
        s.max_datagram_size,
        vtss::tag::Name("ReceiverMaxDatagramSize"),
        vtss::tag::Description("The maximum number of data bytes that can be sent in\
            a single datagram.")
    );

    m.add_leaf(
        s.hostname,
        vtss::tag::Name("ReceiverHostname"),
        vtss::tag::Description("IPv4/IPv6 address or hostname of receiver to which\
            the datagrams are sent.")
    );

    m.add_leaf(
        s.udp_port,
        vtss::tag::Name("ReceiverUdpPortNumber"),
        vtss::tag::Description("UDP port number on the receiver to send the datagrams to.")
    );

    m.add_leaf(
        s.datagram_version,
        vtss::tag::Name("ReceiverUdpDatagramVersion"),
        vtss::tag::Description("The version of sFlow datagrams that we should send.\
            Valid values [5; 5] (we only support v. 5). Default: 5.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sflow_rcvr_info_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sflow_rcvr_info_t"));

    m.add_leaf(
        s.ip_addr,
        vtss::tag::Name("RcvrIpAddress"),
        vtss::tag::Description("The IP address of the current receiver. If there is no current receiver, this is set to <NONE>")
    );

    m.add_leaf(
        s.timeout_left,
        vtss::tag::Name("RcvrTimeLeft"),
        vtss::tag::Description("The number of seconds left for this receiver.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sflow_fs_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sflow_fs_t"));

    m.add_leaf(
        vtss::AsBool(s.enabled),
        vtss::tag::Name("FlowSamplingMode"),
        vtss::tag::Description("Enable or disable this entry.")
    );

    m.add_leaf(
        s.receiver,
        vtss::tag::Name("FlowSamplingRcvrIndex"),
        vtss::tag::Description("One-based index into the sflow receiver table.\
            0 means that this entry is free. Valid values:\
            [0; VTSS_APPL_SFLOW_RECEIVER_CNT]. Default: 0.")
    );

    m.add_leaf(
        s.sampling_rate,
        vtss::tag::Name("FlowSamplingRate"),
        vtss::tag::Description("The statistical sampling rate for packet\
            sampling from this source.")
    );

    m.add_leaf(
        s.max_header_size,
        vtss::tag::Name("FlowSamplingMaxHeaderSize"),
        vtss::tag::Description("The maximum number of bytes that should be\
            copied from a sampled packet. Valid values:\
            [VTSS_APPL_SFLOW_FLOW_HEADER_SIZE_MIN; VTSS_APPL_SFLOW_FLOW_HEADER_SIZE_MAX].\
            Default: VTSS_APPL_SFLOW_FLOW_HEADER_SIZE_DEFAULT.")
    );

    m.add_leaf(
        s.type,
        vtss::tag::Name("FlowSamplingDirection"),
        vtss::tag::Description("Flow sampling direction. Valid values:\
            {NONE, RX, TX, ALL}. Default: TX")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sflow_cp_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sflow_cp_t"));

    m.add_leaf(
        vtss::AsBool(s.enabled),
        vtss::tag::Name("CounterSamplingMode"),
        vtss::tag::Description("Enable or disable this entry.")
    );

    m.add_leaf(
        s.receiver,
        vtss::tag::Name("CounterSamplingRcvrIndex"),
        vtss::tag::Description("One-based index into the sflow receiver table.\
            0 means that this entry is free. Valid values:\
            [0; VTSS_APPL_SFLOW_RECEIVER_CNT]. Default: 0.")
    );

    m.add_leaf(
        s.interval,
        vtss::tag::Name("CounterSamplingInterval"),
        vtss::tag::Description("The maximum number of seconds between sampling\
            of counters for this port. 0 disables sampling. Valid values:\
            [VTSS_APPL_SFLOW_POLLING_INTERVAL_MIN; VTSS_APPL_SFLOW_POLLING_INTERVAL_MAX].\
            Default: 0")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sflow_rcvr_statistics_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sflow_rcvr_statistics_t"));

    m.add_leaf(
        vtss::AsCounter(s.dgrams_ok),
        vtss::tag::Name("DatagramOK"),
        vtss::tag::Description("Counting number of times datagrams were sent successfully.")
    );

    m.add_leaf(
        vtss::AsCounter(s.dgrams_err),
        vtss::tag::Name("DatagramError"),
        vtss::tag::Description("Counting number of times datagram transmission failed\
            (for instance because the receiver could not be reached).")
    );

    m.add_leaf(
        vtss::AsCounter(s.fs),
        vtss::tag::Name("FlowSamplesCount"),
        vtss::tag::Description("Counting number of attempted transmitted flow samples.\
            If #dgrams_err is 0, this corresponds to the actual number\
            of transmitted flow samples.")
    );

    m.add_leaf(
        vtss::AsCounter(s.cp),
        vtss::tag::Name("CounterSamplesCount"),
        vtss::tag::Description("Counting number of attempted transmitted counter samples.\
            If #dgrams_err is 0, this corresponds to the actual number\
            of transmitted counter samples.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_sflow_instance_statistics_t &s) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_sflow_instance_statistics_t"));

    m.add_leaf(
        vtss::AsCounter(s.fs_rx),
        vtss::tag::Name("FsRx"),
        vtss::tag::Description("Number of flow samples received on the primary\
            switch that were Rx sampled.")
    );

    m.add_leaf(
        vtss::AsCounter(s.fs_tx),
        vtss::tag::Name("FsTx"),
        vtss::tag::Description("Number of flow samples received on the primary\
            switch that were Tx sampled.")
    );

    m.add_leaf(
        vtss::AsCounter(s.cp),
        vtss::tag::Name("CpCount"),
        vtss::tag::Description("Number of counter samples received on the primary switch.")
    );
}

namespace vtss {
namespace appl {
namespace sflow {
namespace interfaces {
struct SflowAgentConfigGlobals {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamVal<vtss_appl_sflow_agent_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_sflow_agent_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sflow_agent_cfg_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_sflow_agent_cfg_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SFLOW);
};

struct SflowRcvrConfTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_sflow_rcvr_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "The table is sFlow receiver configuration table. The index is receiver index number (unsigned integer).";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, sflow_rcvr_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_sflow_rcvr_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sflow_rcvr_cfg_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sflow_rcvr_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_sflow_rcvr_cfg_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_sflow_rcvr_cfg_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_sflow_rcvr_cfg_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SFLOW);
};
struct SflowRcvrStatusTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_sflow_rcvr_info_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of the Sflow receiver status";

    static constexpr const char *index_description =
        "Each receiver has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, sflow_rcvr_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_sflow_rcvr_info_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sflow_rcvr_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sflow_rcvr_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SFLOW);
};

struct SflowRcvrStatisticsTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<vtss_appl_sflow_rcvr_statistics_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table of the Sflow receiver status";

    static constexpr const char *index_description =
        "Each receiver has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, sflow_rcvr_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_sflow_rcvr_statistics_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sflow_rcvr_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sflow_rcvr_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SFLOW);
};

struct SflowFlowSamConfTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<u16>,
        vtss::expose::ParamVal<vtss_appl_sflow_fs_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "The table is sFlow flow sampling configuration table. The indexes are ifindex & instance number.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, sflow_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u16 &i) {
        serialize(h, sflow_instance_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_sflow_fs_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sflow_flow_sampler_cfg_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sflow_instance_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_sflow_flow_sampler_cfg_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_sflow_flow_sampler_cfg_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_sflow_flow_sampler_cfg_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SFLOW);
};

struct SflowCpConfTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<u16>,
        vtss::expose::ParamVal<vtss_appl_sflow_cp_t *>
    > P;

    static constexpr uint32_t snmpRowEditorOid = 100;
    static constexpr const char *table_description =
        "The table is sFlow flow sampling configuration table. The indexes are ifindex & instance number.";

    static constexpr const char *index_description =
        "Each entry has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, sflow_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u16 &i) {
        serialize(h, sflow_instance_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_sflow_cp_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sflow_counter_poller_cfg_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sflow_instance_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_sflow_counter_poller_cfg_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_sflow_counter_poller_cfg_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_sflow_counter_poller_cfg_del);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_SFLOW);
};

struct SflowRcvrStatsClearTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<u32>,
        vtss::expose::ParamVal<BOOL *>
    > P;

    static constexpr const char *table_description =
        "This is a table to clear SFLOW receiver statistics";

    static constexpr const char *index_description =
        "Each receiver has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(u32 &i) {
        serialize(h, sflow_rcvr_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i) {
        serialize(h, vtss_sflow_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_GET_PTR(sflow_rcvr_statistics_clr_dummy_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sflow_rcvr_itr);
    VTSS_EXPOSE_SET_PTR(sflow_rcvr_statistics_clr_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SFLOW);
};

struct SflowInstanceStatisticsTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<u16>,
        vtss::expose::ParamVal<vtss_appl_sflow_instance_statistics_t *>
    > P;

    static constexpr const char *table_description =
        "This is a table to clear SFLOW instance statistics";

    static constexpr const char *index_description =
        "Each instance has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, sflow_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u16 &i) {
        serialize(h, sflow_instance_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_sflow_instance_statistics_t &i) {
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_sflow_instance_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sflow_instance_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SFLOW);
};

struct SflowInstanceStatsClearTable {
    typedef vtss::expose::ParamList<
        vtss::expose::ParamKey<vtss_ifindex_t>,
        vtss::expose::ParamKey<u16>,
        vtss::expose::ParamVal<BOOL *>
    > P;

    static constexpr const char *table_description =
        "This is a table to clear SFLOW instance statistics";

    static constexpr const char *index_description =
        "Each instance has a set of parameters";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        serialize(h, sflow_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(u16 &i) {
        serialize(h, sflow_instance_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(BOOL &i) {
        serialize(h, vtss_sflow_ctrl_bool_t(i));
    }

    VTSS_EXPOSE_GET_PTR(sflow_instance_statistics_clr_dummy_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_sflow_instance_itr);
    VTSS_EXPOSE_SET_PTR(sflow_instance_statistics_clr_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_SFLOW);
};

}//namespace interfaces
}//namespace vtss
}//namespace appl
}//namespance sflow

#endif /* __VTSS_SFLOW_SERIALIZER_HXX__ */
