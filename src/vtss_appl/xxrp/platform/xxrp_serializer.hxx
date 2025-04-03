/*

 Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.

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
#ifndef _XXRP_SERIALIZER_HXX_
#define _XXRP_SERIALIZER_HXX_

#if defined(VTSS_SW_OPTION_MRP)
#include "vtss/appl/mrp.h"
#include "vtss_xxrp_api.h"
#endif
#if defined(VTSS_SW_OPTION_MVRP)
#include "vtss/appl/mvrp.h"
#endif
#include "vtss_appl_serialize.hxx"
#include "vtss/appl/module_id.h"
#include "vtss/basics/expose.hxx"
#include "vtss_appl_formatting_tags.hxx"
#include "vtss/appl/types.hxx"

/****************************************************************************
 * Capabilities
 ****************************************************************************/
/* Maximum numbers */
struct MrpCapJoinTimeoutMin {
    static constexpr const char *json_ref = "vtss_appl_mrp_capabilities_t";
    static constexpr const char *name = "JoinTimeoutMin";
    static constexpr const char *desc = "Minimum value of MRP Join timeout "
                                        "in centiseconds.";
    static uint32_t get();
};

struct MrpCapJoinTimeoutMax {
    static constexpr const char *json_ref = "vtss_appl_mrp_capabilities_t";
    static constexpr const char *name = "JoinTimeoutMax";
    static constexpr const char *desc = "Maximum value of MRP Join timeout "
                                        "in centiseconds.";
    static uint32_t get();
};

struct MrpCapLeaveTimeoutMin {
    static constexpr const char *json_ref = "vtss_appl_mrp_capabilities_t";
    static constexpr const char *name = "LeaveTimeoutMin";
    static constexpr const char *desc = "Minimum value of MRP Leave timeout "
                                        "in centiseconds.";
    static uint32_t get();
};

struct MrpCapLeaveTimeoutMax {
    static constexpr const char *json_ref = "vtss_appl_mrp_capabilities_t";
    static constexpr const char *name = "LeaveTimeoutMax";
    static constexpr const char *desc = "Maximum value of MRP Leave timeout "
                                        "in centiseconds.";
    static uint32_t get();
};

struct MrpCapLeaveAllTimeoutMin {
    static constexpr const char *json_ref = "vtss_appl_mrp_capabilities_t";
    static constexpr const char *name = "LeaveAllTimeoutMin";
    static constexpr const char *desc = "Minimum value of MRP LeaveAll timeout "
                                        "in centiseconds.";
    static uint32_t get();
};

struct MrpCapLeaveAllTimeoutMax {
    static constexpr const char *json_ref = "vtss_appl_mrp_capabilities_t";
    static constexpr const char *name = "LeaveAllTimeoutMax";
    static constexpr const char *desc = "Maximum value of MRP LeaveAll timeout "
                                        "in centiseconds.";
    static uint32_t get();
};

//******************************************************************************
// Tag serializers
//******************************************************************************
VTSS_SNMP_TAG_SERIALIZE(vtss_appl_xxrp_ifindex_index, vtss_ifindex_t, a, s) {
    a.add_leaf(vtss::AsInterfaceIndex(s.inner),
               vtss::tag::Name("ifIndex"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(0),
               vtss::tag::Description("Interface index number."));
}

//******************************************************************************
// Struct serializers
//******************************************************************************
#if defined(VTSS_SW_OPTION_MRP)
template <typename T>
void serialize(T &a, vtss_appl_mrp_config_interface_t &p) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mrp_config_interface_t"));

    m.add_leaf(
        vtss::AsInt(p.join_timeout),
        vtss::tag::Name("JoinTimeout"),
        vtss::expose::snmp::RangeSpec<u32>(1, 20),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Join-timeout protocol parameter. Range [1, 20]cs.")
    );

    m.add_leaf(
        vtss::AsInt(p.leave_timeout),
        vtss::tag::Name("LeaveTimeout"),
        vtss::expose::snmp::RangeSpec<u32>(60, 300),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(1),
        vtss::tag::Description("Leave-timeout protocol parameter. Range [60, 300]cs.")
    );

    m.add_leaf(
        vtss::AsInt(p.leave_all_timeout),
        vtss::tag::Name("LeaveAllTimeout"),
        vtss::expose::snmp::RangeSpec<u32>(1000, 5000),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(2),
        vtss::tag::Description("LeaveAll-timeout protocol parameter. Range [1000, 5000] cs.")
    );

    m.add_leaf(
        vtss::AsBool(p.periodic_transmission),
        vtss::tag::Name("PeriodicTransmission"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(3),
        vtss::tag::Description("PeriodicTransmission state of MRP. "
                               "TRUE - enable PeriodicTransmission, "
                               "FALSE - disable PeriodicTransmission."));
}
#endif

#if defined(VTSS_SW_OPTION_MVRP)
template <typename T>
void serialize(T &a, vtss_appl_mvrp_config_global_t &p) {
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mvrp_config_global_t"));

    m.add_leaf(
        vtss::AsBool(p.state),
        vtss::tag::Name("GlobalState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Global state of MVRP. "
                               "TRUE - enable MVRP, FALSE - disable MVRP."));

    
    m.add_rpc_leaf(
        vtss::AsVlanList(p.vlans.data, 512),
        vtss::tag::Name("ManagedVlans"),
        vtss::tag::Description("MVRP-managed VLANs.")
    );

    m.add_snmp_leaf(
        vtss::AsVlanListQuarter(p.vlans.data +   0, 128),
        vtss::tag::Name("ManagedVlans0KTo1K"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(1),
        vtss::tag::Description("First quarter of bit-array indicating the MVRP-managed VLANs.")
    );

    m.add_snmp_leaf(
        vtss::AsVlanListQuarter(p.vlans.data + 128, 128),
        vtss::tag::Name("ManagedVlans1KTo2K"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(2),
        vtss::tag::Description("Second quarter of bit-array indicating the MVRP-managed VLANs.")
    );

    m.add_snmp_leaf(
        vtss::AsVlanListQuarter(p.vlans.data + 256, 128),
        vtss::tag::Name("ManagedVlans2KTo3K"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(3),
        vtss::tag::Description("Third quarter of bit-array indicating the MVRP-managed VLANs.")
    );

    m.add_snmp_leaf(
        vtss::AsVlanListQuarter(p.vlans.data + 384, 128),
        vtss::tag::Name("ManagedVlans3KTo4K"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(4),
        vtss::tag::Description("Last quarter of bit-array indicating the MVRP-managed VLANs.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_mvrp_config_interface_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mvrp_config_interface_t"));

    m.add_leaf(
        vtss::AsBool(p.state),
        vtss::tag::Name("PortState"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Per-interface state of MVRP. "
                               "TRUE - enable MVRP on the interface, "
                               "FALSE - disable MVRP on the interface.")
    );
}

template<typename T>
void serialize(T &a, vtss_appl_mvrp_stat_interface_t &p)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_mvrp_stat_interface_t"));

    m.add_leaf(vtss::AsCounter(p.failed_registrations),
        vtss::tag::Name("FailedRegistrations"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Number of failed VLAN registrations.")
    );

    m.add_leaf(
        p.last_pdu_origin,
        vtss::tag::Name("LastPduOrigin"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(1),
        vtss::tag::Description("Source MAC Address of the last MVRPDU received.")
    );
}
#endif // defined(VTSS_SW_OPTION_MVRP)

namespace vtss {
namespace appl {
namespace xxrp {
namespace interfaces {

#if defined(VTSS_SW_OPTION_MRP)
struct MrpCapabilities {
    typedef vtss::expose::ParamList<
            vtss::expose::ParamVal<vtss_appl_mrp_capabilities_t *>
    > P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mrp_capabilities_t &s) {
        typename HANDLER::Map_t m = 
            h.as_map(vtss::tag::Typename("vtss_appl_mrp_capabilities_t"));

        m.template capability<MrpCapJoinTimeoutMin>(
            vtss::expose::snmp::OidElementValue(1));

        m.template capability<MrpCapJoinTimeoutMax>(
            vtss::expose::snmp::OidElementValue(2));

        m.template capability<MrpCapLeaveTimeoutMin>(
            vtss::expose::snmp::OidElementValue(3));

        m.template capability<MrpCapLeaveTimeoutMax>(
            vtss::expose::snmp::OidElementValue(4));

        m.template capability<MrpCapLeaveAllTimeoutMin>(
            vtss::expose::snmp::OidElementValue(5));

        m.template capability<MrpCapLeaveAllTimeoutMax>(
            vtss::expose::snmp::OidElementValue(6));

        // Add more capabilities here if needed
    }

    // The dummy get function pointer. Empty implementation is enough for this.
    VTSS_EXPOSE_GET_PTR(vtss_appl_mrp_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MRP);
};

struct MrpInterfaceConfigurationTable {
    typedef expose::ParamList <
            expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<vtss_appl_mrp_config_interface_t *>> P;


    static constexpr const char *table_description =
            "This is the MRP interface configuration table. "
            "The number of interfaces is the total number of ports "
            "available on the switch/stack. MRP timer values and "
            "the state of the PeriodicTransmission STM can be "
            "configured for each interface.";

    static constexpr const char *index_description =
            "Each entry has a set of parameters.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(tag::Name("ifidx"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_xxrp_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mrp_config_interface_t &i) {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mrp_config_interface_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_mrp_config_interface_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mrp_config_interface_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MRP);
};
#endif // defined(VTSS_SW_OPTION_MRP)

#if defined(VTSS_SW_OPTION_MVRP)
struct MvrpGlobalsLeaf {
    typedef expose::ParamList<
            expose::ParamVal<vtss_appl_mvrp_config_global_t *>>  P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_mvrp_config_global_t &i) {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mvrp_config_global_get);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mvrp_config_global_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MRP);
};

struct MvrpInterfaceConfigurationTable {
    typedef expose::ParamList <
            expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<vtss_appl_mvrp_config_interface_t *>> P;

    static constexpr const char *table_description =
            "This is the MVRP interface configuration table. "
            "The number of interfaces is the total number of ports "
            "available on the switch/stack. Each one of these "
            "interfaces can be set to either MVRP enabled "
            "or MVRP disabled.";

    static constexpr const char *index_description =
            "Entries in this table represent switch interfaces "
            "and their corresponding MVRP state";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(tag::Name("ifidx"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_xxrp_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mvrp_config_interface_t &i) {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mvrp_config_interface_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_mvrp_config_interface_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_mvrp_config_interface_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_MRP);
};

struct MvrpInterfaceStatisticsTable {
    typedef expose::ParamList <
            expose::ParamKey<vtss_ifindex_t>,
            expose::ParamVal<vtss_appl_mvrp_stat_interface_t *>> P;

    static constexpr const char *table_description =
            "This is the MVRP interface statistics table. "
            "The number of interfaces is the total number of ports "
            "available on the switch/stack. ";

    static constexpr const char *index_description =
            "Each entry has a counter and a MAC address.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i) {
        h.argument_properties(tag::Name("ifidx"));
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_xxrp_ifindex_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_mvrp_stat_interface_t &i) {
        h.argument_properties(tag::Name("conf"));
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_mvrp_stat_interface_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_mvrp_config_interface_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_MRP);
};
#endif // defined(VTSS_SW_OPTION_MVRP)

}  // namespace interfaces
}  // namespace mvrp
}  // namespace appl
}  // namespace vtss

#endif  // _XXRP_SERIALIZER_HXX_
