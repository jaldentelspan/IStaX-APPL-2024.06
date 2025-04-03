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

#ifndef _REDBOX_SERIALIZER_HXX_
#define _REDBOX_SERIALIZER_HXX_

#include <vtss/appl/redbox.h>
#include <vtss/appl/module_id.h>
#include <vtss/appl/types.hxx>
#include "vtss_appl_serialize.hxx"
#include "redbox_api.h" // For redbox_util_oper_warnings_to_str()
#include "port_iter.hxx"

/******************************************************************************/
// Helper functions and wrappers
/******************************************************************************/

struct redbox_serializer_capabilities_interface_list_t {
    // port_list converted to PortListStackable
    vtss_port_list_stackable_t port_list_stackable;
};

/******************************************************************************/
// redbox_serializer_capabilities_port_list_itr()
// Iterates across all possible RedBox instances
/******************************************************************************/
static inline mesa_rc redbox_serializer_capabilities_port_list_itr(const uint32_t *prev_inst, uint32_t *next_inst)
{
    vtss_appl_redbox_capabilities_t cap;

    if (!next_inst) {
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_appl_redbox_capabilities_get(&cap));

    *next_inst = prev_inst ? *prev_inst : 0;

    if (*next_inst < 1) {
        *next_inst = 1;
    } else {
        (*next_inst)++;
    }

    return *next_inst <= cap.inst_cnt_max ? VTSS_RC_OK : VTSS_RC_ERROR;
}

/******************************************************************************/
// redbox_serializer_capabilities_port_list_get()
// Convert a mesa_port_list_t to a vtss_port_list_stackable_t
/******************************************************************************/
static inline mesa_rc redbox_serializer_capabilities_port_list_get(uint32_t inst, redbox_serializer_capabilities_interface_list_t *l)
{
    mesa_port_no_t   port_no;
    port_iter_t      pit;
    mesa_port_list_t port_list;

    if (!l) {
        return VTSS_RC_ERROR;
    }

    vtss_clear(l->port_list_stackable);
    VTSS_RC(vtss_appl_redbox_capabilities_port_list_get(inst, &port_list));

    (void)port_iter_init(&pit, NULL, VTSS_ISID_START, PORT_ITER_SORT_ORDER_UPORT, PORT_ITER_FLAGS_NORMAL);
    while (port_iter_getnext(&pit)) {
        port_no = pit.iport;
        if (port_list[port_no]) {
            // The following are defined in .../vtss_appl/mac/mac.cxx
            BOOL portlist_index_set(u32 i, vtss_port_list_stackable_t *pl);
            uint32_t isid_port_to_index(vtss_isid_t i,  mesa_port_no_t p);
            (void)portlist_index_set(isid_port_to_index(VTSS_ISID_START, port_no), &l->port_list_stackable);
        }
    }

    return VTSS_RC_OK;
}

/******************************************************************************/
// redbox_serializer_conf_default_get()
/******************************************************************************/
static inline mesa_rc redbox_serializer_conf_default_get(uint32_t *inst, vtss_appl_redbox_conf_t *conf)
{
    if (!inst || !conf) {
        return VTSS_RC_ERROR;
    }

    *inst = 0;
    return vtss_appl_redbox_conf_default_get(conf);
}

/******************************************************************************/
// redbox_serializer_nt_mac_status_get()
/******************************************************************************/
static inline mesa_rc redbox_serializer_nt_mac_status_get(uint32_t inst, mesa_mac_t mac, vtss_appl_redbox_nt_mac_status_t *s)
{
    return vtss_appl_redbox_nt_mac_status_get(inst, &mac, s);
}

/******************************************************************************/
// redbox_serializer_pnt_mac_status_get()
/******************************************************************************/
static inline mesa_rc redbox_serializer_pnt_mac_status_get(uint32_t inst, mesa_mac_t mac, vtss_appl_redbox_pnt_mac_status_t *s)
{
    return vtss_appl_redbox_pnt_mac_status_get(inst, &mac, s);
}

/******************************************************************************/
// redbox_serializer_clear_get()
/******************************************************************************/
static inline mesa_rc redbox_serializer_clear_get(uint32_t inst, BOOL *clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }

    return VTSS_RC_OK;
};

/******************************************************************************/
// redbox_serializer_nt_clear_set()
/******************************************************************************/
static inline mesa_rc redbox_serializer_nt_clear_set(uint32_t inst, const BOOL *clear)
{
    if (clear && *clear) {
        return vtss_appl_redbox_nt_clear(inst);
    }

    return VTSS_RC_OK;
};

/******************************************************************************/
// redbox_serializer_pnt_clear_set()
/******************************************************************************/
static inline mesa_rc redbox_serializer_pnt_clear_set(uint32_t inst, const BOOL *clear)
{
    if (clear && *clear) {
        return vtss_appl_redbox_pnt_clear(inst);
    }

    return VTSS_RC_OK;
};

/******************************************************************************/
// redbox_serializer_statistics_clear_set()
/******************************************************************************/
static inline mesa_rc redbox_serializer_statistics_clear_set(uint32_t inst, const BOOL *clear)
{
    if (clear && *clear) {
        return vtss_appl_redbox_statistics_clear(inst);
    }

    return VTSS_RC_OK;
};

VTSS_SNMP_TAG_SERIALIZE(redbox_serializer_inst_index, uint32_t, a, s)
{
    a.add_leaf(
        s.inner,
        vtss::tag::Name("Instance"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("RedBox instance number.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(redbox_serializer_mac_index, mesa_mac_t, a, s)
{
    a.add_leaf(
        s.inner,
        vtss::tag::Name("MacAddress"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("MAC address in NodesTable or ProxyNodeTable.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(redbox_serializer_nt_clear_value, BOOL, a, s)
{
    a.add_leaf(
        vtss::AsBool(s.inner),
        vtss::tag::Name("Clear"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Set to TRUE to clear the NodesTable of a RedBox instance.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(redbox_serializer_pnt_clear_value, BOOL, a, s)
{
    a.add_leaf(
        vtss::AsBool(s.inner),
        vtss::tag::Name("Clear"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Set to TRUE to clear the ProxyNodeTable of a RedBox instance.")
    );
}

VTSS_SNMP_TAG_SERIALIZE(redbox_serializer_statistics_clear_value, BOOL, a, s)
{
    a.add_leaf(
        vtss::AsBool(s.inner),
        vtss::tag::Name("Clear"),
        vtss::expose::snmp::Status::Current,
        vtss::expose::snmp::OidElementValue(0),
        vtss::tag::Description("Set to TRUE to clear the counters of a RedBox instance.")
    );
}

static inline const vtss_enum_descriptor_t redbox_serializer_mode_txt[] = {
    {VTSS_APPL_REDBOX_MODE_PRP_SAN, "prpSan"},
    {VTSS_APPL_REDBOX_MODE_HSR_SAN, "hsrSan"},
    {VTSS_APPL_REDBOX_MODE_HSR_PRP, "hsrPrp"},
    {VTSS_APPL_REDBOX_MODE_HSR_HSR, "hsrHsr"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_redbox_mode_t,
                         "RedBoxMode",
                         redbox_serializer_mode_txt,
                         "This enumeration defines the RedBox' mode of operation.");

static inline const vtss_enum_descriptor_t redbox_serializer_lan_id_txt[] = {
    {VTSS_APPL_REDBOX_LAN_ID_A, "lanA"},
    {VTSS_APPL_REDBOX_LAN_ID_B, "lanB"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_redbox_lan_id_t,
                         "RedBoxLanId",
                         redbox_serializer_lan_id_txt,
                         "This enumeration defines whether the RedBox connects to LanA or LanB.");

static inline const vtss_enum_descriptor_t redbox_serializer_oper_state_txt[] = {
    {VTSS_APPL_REDBOX_OPER_STATE_ADMIN_DISABLED,  "disabled"},
    {VTSS_APPL_REDBOX_OPER_STATE_ACTIVE,          "active"},
    {VTSS_APPL_REDBOX_OPER_STATE_INTERNAL_ERROR,  "internalError"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_redbox_oper_state_t,
                         "RedBoxOperState",
                         redbox_serializer_oper_state_txt,
                         "Operational state of a RedBox instance");

static inline const vtss_enum_descriptor_t redbox_serializer_node_type_txt[] = {
    {VTSS_APPL_REDBOX_NODE_TYPE_DANP,    "danP"},
    {VTSS_APPL_REDBOX_NODE_TYPE_DANP_RB, "danPRedBox"},
    {VTSS_APPL_REDBOX_NODE_TYPE_VDANP,   "vDanP"},
    {VTSS_APPL_REDBOX_NODE_TYPE_DANH,    "danH"},
    {VTSS_APPL_REDBOX_NODE_TYPE_DANH_RB, "danHRedBox"},
    {VTSS_APPL_REDBOX_NODE_TYPE_VDANH,   "vDanH"},
    {VTSS_APPL_REDBOX_NODE_TYPE_SAN,     "san"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_redbox_node_type_t,
                         "RedBoxNodeType",
                         redbox_serializer_node_type_txt,
                         "Node type");

static inline const vtss_enum_descriptor_t redbox_serializer_sv_type_txt[] = {
    {VTSS_APPL_REDBOX_SV_TYPE_PRP_DD, "prpDd"},
    {VTSS_APPL_REDBOX_SV_TYPE_PRP_DA, "prpDa"},
    {VTSS_APPL_REDBOX_SV_TYPE_HSR,    "hsr"},
    {0, 0}
};

VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_redbox_sv_type_t,
                         "RedBoxSvType",
                         redbox_serializer_sv_type_txt,
                         "Supervision frame type");

/******************************************************************************/
// Capabilities
/******************************************************************************/
template<typename T>
void serialize(T &a, vtss_appl_redbox_capabilities_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_redbox_capabilities_t"));
    int               ix = 0;

    m.add_leaf(s.inst_cnt_max,
               vtss::tag::Name("InstCntMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of configurable RedBox instances."));

    m.add_leaf(s.nt_pnt_size,
               vtss::tag::Name("NtPntSize"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum number of entries in the combined NodesTable/ProxyNodeTable."));

    m.add_leaf(s.nt_age_time_secs_min,
               vtss::tag::Name("NtAgeTimeMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum allowed NodesTable age time in seconds."));

    m.add_leaf(s.nt_age_time_secs_max,
               vtss::tag::Name("NtAgeTimeMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum allowed NodesTable age time in seconds."));

    m.add_leaf(s.pnt_age_time_secs_min,
               vtss::tag::Name("PntAgeTimeMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum allowed ProxyNodeTable age time in seconds."));

    m.add_leaf(s.pnt_age_time_secs_max,
               vtss::tag::Name("PntAgeTimeMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum allowed ProxyNodeTable age time in seconds."));

    m.add_leaf(s.duplicate_discard_age_time_msecs_min,
               vtss::tag::Name("DdAgeTimeMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum allowed Duplicate Discard age time in milliseconds."));

    m.add_leaf(s.duplicate_discard_age_time_msecs_max,
               vtss::tag::Name("DdAgeTimeMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum allowed Duplicate Discard age time in milliseconds."));

    m.add_leaf(s.sv_frame_interval_secs_min,
               vtss::tag::Name("SvFrameIntervalMin"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Minimum allowed supervision frame interval in seconds."));

    m.add_leaf(s.sv_frame_interval_secs_max,
               vtss::tag::Name("SvFrameIntervalMax"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Maximum allowed supervision frame interval in seconds."));

    m.add_leaf(s.statistics_poll_interval_secs,
               vtss::tag::Name("StatisticsPollInterval"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of seconds between statistics polling."));

    m.add_leaf(s.alarm_raised_time_secs,
               vtss::tag::Name("AlarmRaisedTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of seconds after a frame error condition is no longer detected until the notification/alarm disappears."));
};

template <typename T>
void serialize(T &a, redbox_serializer_capabilities_interface_list_t &s)
{
    typename T::Map_t       m = a.as_map(vtss::tag::Typename("vtss_appl_redbox_capabilities_port_list_t"));
    int                     ix = 0;
    vtss::PortListStackable &ports = (vtss::PortListStackable &)s.port_list_stackable;

    m.add_leaf(ports,
               vtss::tag::Name("Interfaces"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("List of valid ports for this RedBox instance."));

};

/******************************************************************************/
// Configuration
/******************************************************************************/
template <typename T>
void serialize(T &a, vtss_appl_redbox_conf_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_redbox_conf_t"));
    int               ix = 0;

    // This is converted to text with redbox_serializer_mode_txt[]
    m.add_leaf(s.mode,
               vtss::tag::Name("Mode"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Mode that this instance is running in."));

    m.add_leaf(vtss::AsInterfaceIndex(s.port_a),
               vtss::tag::Name("PortA"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Interface index of port A."));

    m.add_leaf(vtss::AsInterfaceIndex(s.port_b),
               vtss::tag::Name("PortB"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Interface index of port B."));

    // Make room for possible future support of hsr_mode.
    ix++;

    // Make room for possible future support of duplicate_discard
    ix++;

    m.add_leaf(s.net_id,
               vtss::tag::Name("NetId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("NetId used to filter frames in HSR-PRP and HSR-HSR modes."));

    // This is converted to text with redbox_serializer_lan_id_txt[]
    m.add_leaf(s.lan_id,
               vtss::tag::Name("LanId"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("LanId used to filter frames in HSR-PRP mode."));

    m.add_leaf(s.nt_age_time_secs,
               vtss::tag::Name("NtAgeTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of seconds without activity before a remote node is removed from the NodesTable."));

    m.add_leaf(s.pnt_age_time_secs,
               vtss::tag::Name("PntAgeTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of seconds without activity before a proxy node is removed from the ProxyNodeTable."));

    m.add_leaf(s.duplicate_discard_age_time_msecs,
               vtss::tag::Name("DdAgeTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of milliseconds that an entry is considered a duplicate."));

    m.add_leaf(vtss::AsVlan(s.sv_vlan),
               vtss::tag::Name("SvVlan"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("VLAN ID on which supervision PDUs are transmitted on port A and port B. Use 0 to indicate the interlink port's native VLAN (Port VLAN ID)"));

    m.add_leaf(s.sv_pcp,
               vtss::tag::Name("SvPcp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("PCP value used in the VLAN tag of supervision PDUs transmitted on port A and port B."));

    m.add_leaf(s.sv_dmac_lsb,
               vtss::tag::Name("SvDmacLsb"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Least significant byte of destination MAC address used in supervision frames transmitted on port A and port B."));

    m.add_leaf(s.sv_frame_interval_secs,
               vtss::tag::Name("SvFrameInterval"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of seconds between transmission of supervision frames."));

    m.add_leaf(vtss::AsBool(s.sv_xlat_prp_to_hsr),
               vtss::tag::Name("SvXlatPrpToHsr"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Enable proxy-translation of supervision frames from HSR ring to PRP network (HSR-PRP mode only)."));

    m.add_leaf(vtss::AsBool(s.sv_xlat_hsr_to_prp),
               vtss::tag::Name("SvXlatHsrToPrp"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Enable proxy-translation of supervision frames from PRP network to HSR ring (HSR-PRP mode only)."));

    m.add_leaf(vtss::AsBool(s.admin_active),
               vtss::tag::Name("AdminActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Controls whether this instance is active or not."));
};

/******************************************************************************/
// Status
/******************************************************************************/
template <typename T>
void serialize_notif_status(T &m, vtss_appl_redbox_notification_status_t &s, int &ix)
{
    vtss_appl_redbox_port_type_t port_type;
    char                         port_name1[20], port_name2[20], name_buf[50], dscr_buf[100];

    m.add_leaf(vtss::AsBool(s.nt_pnt_full),
               vtss::tag::Name("NtPntFull"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The combined NodesTable/ProxyNodeTable is full"));

    // Make room for future not-per-port notifications.
    ix += 10;

    for (port_type = (vtss_appl_redbox_port_type_t)0; port_type < ARRSZ(s.port); port_type++) {
        vtss_appl_redbox_port_notification_status_t &p = s.port[port_type];

        // Get a contracted port name in capitals, e.g. "PortA".
        sprintf(port_name1, "%s", redbox_util_port_type_to_str(port_type, true /* capital */, true  /* contract */));

        // Get a non-contracted port name in capitals, e.g. "Port A".
        sprintf(port_name2, "%s", redbox_util_port_type_to_str(port_type, true /* capital */, false /* don't contract */));

        sprintf(name_buf, "%sWrongLan",                                port_name1);
        sprintf(dscr_buf, "%s has received traffic with wrong Lan ID", port_name2);
        m.add_leaf(vtss::AsBool(p.cnt_err_wrong_lan),
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sHsrUntaggedRx",                                       port_name1);
        sprintf(dscr_buf, "%s has received traffic without HSR-tag from HSR ring", port_name2);
        m.add_leaf(vtss::AsBool(p.hsr_untagged_rx),
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        if (port_type != VTSS_APPL_REDBOX_PORT_TYPE_C) {
            sprintf(name_buf, "%sDown",           port_name1);
            sprintf(dscr_buf, "%s has link down", port_name2);
            m.add_leaf(vtss::AsBool(p.down),
                       vtss::tag::Name(name_buf),
                       vtss::expose::snmp::Status::Current,
                       vtss::expose::snmp::OidElementValue(ix++),
                       vtss::tag::Description(dscr_buf));
        } else {
            // Perhaps Port C needs this in the future?
            ix += 1;
        }

        // Make room for future per-port notifications.
        ix += 10;
    }
};

template <typename T>
void serialize(T &a, vtss_appl_redbox_notification_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_redbox_notification_status_t"));
    int               ix = 0;

    // Use common serializer
    serialize_notif_status(m, s, ix);
};

template <typename T>
void serialize(T &a, vtss_appl_redbox_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_redbox_status_t"));
    vtss_appl_redbox_oper_warnings_t w;
    mesa_bool_t                      b;
    char                             buf[400];
    int                              ix = 0;

    // This is converted to text with redbox_serializer_oper_state_txt[]
    m.add_leaf(s.oper_state,
               vtss::tag::Name("OperState"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Operational state of this RedBox instance."));

    m.add_leaf(vtss::AsInterfaceIndex(s.port_c),
               vtss::tag::Name("PortC"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Interface index of port C (the interlink port)."));

    // Use common serializer in order to get ix updated.
    serialize_notif_status(m, s.notif_status, ix);

    b = s.oper_warnings == VTSS_APPL_REDBOX_OPER_WARNING_NONE;
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNone"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("No configurational warnings found."));

    w = VTSS_APPL_REDBOX_OPER_WARNING_MTU_TOO_HIGH_LRE_PORTS;
    b = (s.oper_warnings & w) != 0;
    (void)redbox_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningMtuTooHighLrePorts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    w = VTSS_APPL_REDBOX_OPER_WARNING_MTU_TOO_HIGH_NON_LRE_PORTS;
    b = (s.oper_warnings & w) != 0;
    (void)redbox_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningMtuTooHighNonLrePorts"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    w = VTSS_APPL_REDBOX_OPER_WARNING_INTERLINK_NOT_C_TAGGED;
    b = (s.oper_warnings & w) != 0;
    (void)redbox_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningInterlinkNotCTagged"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    w = VTSS_APPL_REDBOX_OPER_WARNING_INTERLINK_NOT_MEMBER_OF_VLAN;
    b = (s.oper_warnings & w) != 0;
    (void)redbox_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningInterlinkNotMemberOfVlan"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    w = VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_NOT_CONFIGURED;
    b = (s.oper_warnings & w) != 0;
    (void)redbox_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNeighborRedBoxNotConfigured"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    w = VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_NOT_ACTIVE;
    b = (s.oper_warnings & w) != 0;
    (void)redbox_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNeighborRedBoxNotActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    w = VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_PORT_A_NOT_SET_TO_NEIGHBOR;
    b = (s.oper_warnings & w) != 0;
    (void)redbox_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNeighborRedBoxPortANotSetToNeighbor"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    w = VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_PORT_B_NOT_SET_TO_NEIGHBOR;
    b = (s.oper_warnings & w) != 0;
    (void)redbox_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNeighborRedBoxPortBNotSetToNeighbor"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    w = VTSS_APPL_REDBOX_OPER_WARNING_NEIGHBOR_REDBOX_OVERLAPPING_VLANS;
    b = (s.oper_warnings & w) != 0;
    (void)redbox_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningNeighborRedBoxOverlappingVlans"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));

    w = VTSS_APPL_REDBOX_OPER_WARNING_STP_ENABLED_INTERLINK;
    b = (s.oper_warnings & w) != 0;
    (void)redbox_util_oper_warnings_to_str(buf, sizeof(buf), w);
    m.add_leaf(vtss::AsBool(b),
               vtss::tag::Name("WarningStpEnabledInterlink"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(buf));
};

template <typename T>
void serialize(T &a, vtss_appl_redbox_nt_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_redbox_nt_status_t"));
    int               ix = 0;

    m.add_leaf(s.mac_cnt,
               vtss::tag::Name("MacCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of MAC addresses in NodesTable"));

    m.add_leaf(vtss::AsBool(s.wrong_lan),
               vtss::tag::Name("WrongLan"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("True if at least one MAC address in the NodesTable has a non-zero Rx Wrong LAN count (PRP-SAN mode only)"));
};

template <typename T>
void serialize_mac_port_status(T &m, vtss_appl_redbox_mac_port_status_t &p, int &ix, vtss_appl_redbox_port_type_t port_type)
{
    char port_name1[20], port_name2[20], name_buf[50], dscr_buf[200];

    // Get a contracted port name in capitals, e.g. "PortA".
    sprintf(port_name1, "%s", redbox_util_port_type_to_str(port_type, true /* capital */, true  /* contract */));

    // Get a non-contracted port name in capitals, e.g. "Port A".
    sprintf(port_name2, "%s", redbox_util_port_type_to_str(port_type, true /* capital */, false /* don't contract */));

    sprintf(name_buf, "%sRxCnt",                         port_name1);
    sprintf(dscr_buf, "Number of frames received on %s", port_name2);
    m.add_leaf(p.rx_cnt,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(dscr_buf));

    sprintf(name_buf, "%sLastSeen",                                                   port_name1);
    sprintf(dscr_buf, "Number of seconds since this MAC address was last seen on %s", port_name2);
    m.add_leaf(p.last_seen_secs,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(dscr_buf));

    sprintf(name_buf, "%sSvRxCnt",                                         port_name1);
    sprintf(dscr_buf, "Number of valid supervision frames received on %s", port_name2);
    m.add_leaf(p.sv_rx_cnt,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(dscr_buf));

    sprintf(name_buf, "%sSvLastSeen",                                                    port_name1);
    sprintf(dscr_buf, "Number of seconds since a supervision frame was last seen on %s", port_name2);
    m.add_leaf(p.sv_last_seen_secs,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(dscr_buf));

    sprintf(name_buf, "%sSvLastType",                                   port_name1);
    sprintf(dscr_buf, "The supervision frame type last received on %s", port_name2);
    // This is converted to text with redbox_serializer_sv_type_txt[]
    m.add_leaf(p.sv_last_type,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(dscr_buf));

    sprintf(name_buf, "%sRxWrongLanCnt",                                                 port_name1);
    sprintf(dscr_buf, "The number of frames received with wrong LanId on %s in %s mode", port_name2, port_type == VTSS_APPL_REDBOX_PORT_TYPE_C ? "HSR-PRP" : "PRP-SAN");
    m.add_leaf(p.rx_wrong_lan_cnt,
               vtss::tag::Name(name_buf),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description(dscr_buf));

    if (port_type != VTSS_APPL_REDBOX_PORT_TYPE_C) {
        // Only exists on Port A/B.
        sprintf(name_buf, "%sFwd",                                                                                      port_name1);
        sprintf(dscr_buf, "Indicates whether the a frame from switch core will be forwarded on %s (PRP-SAN mode only)", port_name2);
        m.add_leaf(vtss::AsBool(p.fwd),
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));
    }
}

template <typename T>
void serialize(T &a, vtss_appl_redbox_nt_mac_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_redbox_nt_mac_status_t"));
    vtss_appl_redbox_port_type_t port_type;
    int                          ix = 0;

    // This is converted to text with redbox_serializer_node_type_txt[]
    m.add_leaf(s.node_type,
               vtss::tag::Name("NodeType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Type of node"));

    // Make room for more non-per-port NT status
    ix += 10;

    for (port_type = (vtss_appl_redbox_port_type_t)0; port_type < ARRSZ(s.port); port_type++) {
        serialize_mac_port_status(m, s.port[port_type], ix, port_type);

        // Make room for more per-port status
        ix += 10;
    }
};

template <typename T>
void serialize(T &a, vtss_appl_redbox_pnt_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_redbox_pnt_status_t"));
    int               ix = 0;

    m.add_leaf(s.mac_cnt,
               vtss::tag::Name("MacCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of MAC addresses in ProxyNodeTable"));

    m.add_leaf(vtss::AsBool(s.wrong_lan),
               vtss::tag::Name("WrongLan"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("True if at least one MAC address in the ProxyNodeTable has a non-zero Rx Wrong LAN count (HSR-PRP mode only)"));
};

template <typename T>
void serialize(T &a, vtss_appl_redbox_pnt_mac_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_redbox_pnt_mac_status_t"));
    int               ix = 0;

    // This is converted to text with redbox_serializer_node_type_txt[]
    m.add_leaf(s.node_type,
               vtss::tag::Name("NodeType"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Type of node"));

    serialize_mac_port_status(m, s.port, ix, VTSS_APPL_REDBOX_PORT_TYPE_C);

    // Make room for more per-port status
    ix += 10;

    m.add_leaf(s.sv_tx_cnt,
               vtss::tag::Name("SvTxCnt"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Number of proxied supervision frames transmitted towards LRE ports on behalf of this VDAN."));

    m.add_leaf(vtss::AsBool(s.locked),
               vtss::tag::Name("Locked"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("Indicates whether the node is locked in the ProxyNodeTable. Only entries added by the RedBox itself are locked."));
};

/******************************************************************************/
// Statistics
/******************************************************************************/

template <typename T>
void serialize(T &a, vtss_appl_redbox_statistics_t &s)
{
    typename T::Map_t            m = a.as_map(vtss::tag::Typename("vtss_appl_redbox_statistics_t"));
    vtss_appl_redbox_port_type_t port_type;
    vtss_appl_redbox_sv_type_t   sv_type;
    char                         port_name1[10], port_name2[10];
    char                         name_buf[50], dscr_buf[150];
    char                         sv_type_buf1[10], sv_type_buf2[10];
    int                          ix = 0;

    for (port_type = (vtss_appl_redbox_port_type_t)0; port_type < ARRSZ(s.port); port_type++) {
        vtss_appl_redbox_port_statistics_t &p = s.port[port_type];

        // Get a contracted port name in capitals, e.g. "PortA".
        sprintf(port_name1, "%s", redbox_util_port_type_to_str(port_type, true /* capital */, true  /* contract */));

        // Get a non-contracted port name in capitals, e.g. "Port A".
        sprintf(port_name2, "%s", redbox_util_port_type_to_str(port_type, true /* capital */, false /* don't contract */));

        sprintf(name_buf, "%sRxTaggedCnt",                                          port_name1);
        sprintf(dscr_buf, "Number of HSR- and/or PRP-tagged frames received on %s", port_name2);
        m.add_leaf(p.rx_tagged_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sTxTaggedCnt",                                          port_name1);
        sprintf(dscr_buf, "Number of HSR- and/or PRP-tagged frames received on %s", port_name2);
        m.add_leaf(p.tx_tagged_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sRxUntaggedCnt",                                                      port_name1);
        sprintf(dscr_buf, "Number of frames received on %s that are neither HSR- nor PRP-tagged", port_name2);
        m.add_leaf(p.rx_untagged_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sTxUntaggedCnt",                                                         port_name1);
        sprintf(dscr_buf, "Number of frames transmitted on %s that are neither HSR- nor PRP-tagged", port_name2);
        m.add_leaf(p.tx_untagged_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sRxLinkLocalCnt",                            port_name1);
        sprintf(dscr_buf, "Number of link-local (BPDUs) received on %s", port_name2);
        m.add_leaf(p.rx_bpdu_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sTxLinkLocalCnt",                               port_name1);
        sprintf(dscr_buf, "Number of link-local (BPDUs) transmitted on %s", port_name2);
        m.add_leaf(p.tx_bpdu_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sRxOwnCnt",                                                                           port_name1);
        sprintf(dscr_buf, "Number of HSR-tagged frames received on %s whose SMAC appears in the ProxyNodeTable)", port_name2);
        m.add_leaf(p.rx_own_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sRxWrongLanCnt",                                  port_name1);
        sprintf(dscr_buf, "Number of frames received on %s with wrong LanId", port_name2);
        m.add_leaf(p.rx_wrong_lan_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sTxDuplZeroCnt",                                                 port_name1);
        sprintf(dscr_buf, "Number of frames transmitted on %s without any duplicates seen.", port_name2);
        m.add_leaf(p.tx_dupl_zero_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sTxDuplOneCnt",                                                   port_name1);
        sprintf(dscr_buf, "Number of frames transmitted on %s with one duplicate discarded.", port_name2);
        m.add_leaf(p.tx_dupl_one_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sTxDuplMultiCnt",                                                          port_name1);
        sprintf(dscr_buf, "Number of frames transmitted on %s with two or more duplicates discarded.", port_name2);
        m.add_leaf(p.tx_dupl_multi_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        // Now, it's time to add SV counters.
        sprintf(name_buf, "%sRxSvErrCnt",                                           port_name1);
        sprintf(dscr_buf, "Number of erroneous supervision frames received on %s.", port_name2);
        m.add_leaf(p.sv_rx_err_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        sprintf(name_buf, "%sRxSvFilteredCnt",                                     port_name1);
        sprintf(dscr_buf, "Number of filtered supervision frames received on %s.", port_name2);
        m.add_leaf(p.sv_rx_filtered_cnt,
                   vtss::tag::Name(name_buf),
                   vtss::expose::snmp::Status::Current,
                   vtss::expose::snmp::OidElementValue(ix++),
                   vtss::tag::Description(dscr_buf));

        for (sv_type = (vtss_appl_redbox_sv_type_t)0; sv_type < ARRSZ(p.sv_rx_cnt); sv_type++) {
            // Get a contracted SV type, e.g. "PrpDd"
            sprintf(sv_type_buf1, "%s", redbox_util_sv_type_to_str(sv_type, true  /* contract */));

            // Get a non-contracted SV type, e.g. "PRP-DD"
            sprintf(sv_type_buf2, "%s", redbox_util_sv_type_to_str(sv_type, false /* don't contract */));

            sprintf(name_buf, "%sRxSv%sCnt",                                    port_name1, sv_type_buf1);
            sprintf(dscr_buf, "Number of %s supervision frames received on %s", sv_type_buf2, port_name2);
            m.add_leaf(p.sv_rx_cnt[sv_type],
                       vtss::tag::Name(name_buf),
                       vtss::expose::snmp::Status::Current,
                       vtss::expose::snmp::OidElementValue(ix++),
                       vtss::tag::Description(dscr_buf));

            // Make room for future SV types
            ix += 2;
        }

        for (sv_type = (vtss_appl_redbox_sv_type_t)0; sv_type < ARRSZ(p.sv_tx_cnt); sv_type++) {
            // Get a contracted SV type, e.g. "PrpDd"
            sprintf(sv_type_buf1, "%s", redbox_util_sv_type_to_str(sv_type, true  /* contract */));

            // Get a non-contracted SV type, e.g. "PRP-DD"
            sprintf(sv_type_buf2, "%s", redbox_util_sv_type_to_str(sv_type, false /* don't contract */));

            sprintf(name_buf, "%sTxSv%sCnt",                                       port_name1, sv_type_buf1);
            sprintf(dscr_buf, "Number of %s supervision frames transmitted on %s", sv_type_buf2, port_name2);
            m.add_leaf(p.sv_tx_cnt[sv_type],
                       vtss::tag::Name(name_buf),
                       vtss::expose::snmp::Status::Current,
                       vtss::expose::snmp::OidElementValue(ix++),
                       vtss::tag::Description(dscr_buf));

            // Make room for future SV types
            ix += 2;
        }

        // Make room for additional per-port counters
        ix += 10;
    }
};

namespace vtss
{
namespace appl
{
namespace redbox
{
namespace interfaces
{

/******************************************************************************/
// RedBoxCapabilities
/******************************************************************************/
struct RedBoxCapabilities {
    typedef expose::ParamList <expose::ParamVal<vtss_appl_redbox_capabilities_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_redbox_capabilities_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_redbox_capabilities_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxCapabilitiesInterfaces
// Lists - per RedBox instance - valid ports
/******************************************************************************/
struct RedBoxCapabilitiesInterfaces {
    typedef expose::ParamList<expose::ParamKey<uint32_t>, expose::ParamVal<redbox_serializer_capabilities_interface_list_t *>> P;

    static constexpr const char *table_description =
        "This table shows - per RedBox instance number - which interfaces "
        "can be configured on that instance";

    static constexpr const char *index_description =
        "Each RedBox has a list of valid interfaces that can be used along "
        "with that instance";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(redbox_serializer_capabilities_interface_list_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(redbox_serializer_capabilities_port_list_get);
    VTSS_EXPOSE_ITR_PTR(redbox_serializer_capabilities_port_list_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxDefaultConf
/******************************************************************************/
struct RedBoxDefaultConf {
    typedef expose::ParamList <expose::ParamVal<vtss_appl_redbox_conf_t *>> P;

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_appl_redbox_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_redbox_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxConf
/******************************************************************************/
struct RedBoxConf {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamVal<vtss_appl_redbox_conf_t *>> P;

    static constexpr uint32_t snmpRowEditorOid = 100;

    static constexpr const char *table_description =
        "RedBox instance configuration table.";

    static constexpr const char *index_description =
        "Each entry in this table represents a RedBox instance's configuration.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_redbox_conf_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_redbox_conf_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_redbox_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_redbox_conf_set);
    VTSS_EXPOSE_ADD_PTR(vtss_appl_redbox_conf_set);
    VTSS_EXPOSE_DEL_PTR(vtss_appl_redbox_conf_del);
    VTSS_EXPOSE_DEF_PTR(redbox_serializer_conf_default_get);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxStatus
/******************************************************************************/
struct RedBoxStatus {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamVal<vtss_appl_redbox_status_t *>> P;

    static constexpr const char *table_description =
        "Redbox status table.";

    static constexpr const char *index_description =
        "Each entry in this table represents a RedBox instance's status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_redbox_status_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_redbox_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_redbox_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxNodesTableStatus
/******************************************************************************/
struct RedBoxNodesTableStatus {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamVal<vtss_appl_redbox_nt_status_t *>> P;

    static constexpr const char *table_description =
        "Redbox NodesTable status.";

    static constexpr const char *index_description =
        "Each entry in this table represents the status of the NodesTable for a given RedBox instance.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_redbox_nt_status_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_redbox_nt_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_redbox_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxNodesTableMacStatus
/******************************************************************************/
struct RedBoxNodesTableMacStatus {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamKey<mesa_mac_t>, expose::ParamVal<vtss_appl_redbox_nt_mac_status_t *>> P;

    static constexpr const char *table_description =
        "Redbox NodesTable MAC status.";

    static constexpr const char *index_description =
        "Each entry in this table represents a MAC address in the NodesTable for a given RedBox instance.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_mac_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, redbox_serializer_mac_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_redbox_nt_mac_status_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(redbox_serializer_nt_mac_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_redbox_nt_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxProxyNodeTableStatus
/******************************************************************************/
struct RedBoxProxyNodeTableStatus {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamVal<vtss_appl_redbox_pnt_status_t *>> P;

    static constexpr const char *table_description =
        "Redbox ProxyNodeTable status.";

    static constexpr const char *index_description =
        "Each entry in this table represents the status of the ProxyNodeTable for a given RedBox instance.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_redbox_pnt_status_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_redbox_pnt_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_redbox_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxProxyNodeTableMacStatus
/******************************************************************************/
struct RedBoxProxyNodeTableMacStatus {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamKey<mesa_mac_t>, expose::ParamVal<vtss_appl_redbox_pnt_mac_status_t *>> P;

    static constexpr const char *table_description =
        "Redbox ProxyNodeTable MAC status.";

    static constexpr const char *index_description =
        "Each entry in this table represents a MAC address in the ProxyNodeTable for a given RedBox instance.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(mesa_mac_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, redbox_serializer_mac_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_3(vtss_appl_redbox_pnt_mac_status_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(3));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(redbox_serializer_pnt_mac_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_redbox_pnt_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxNotifStatus
/******************************************************************************/
struct RedBoxNotifStatus {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamVal<vtss_appl_redbox_notification_status_t *>> P;

    static constexpr const char *table_description =
        "RedBox notification status table.";

    static constexpr const char *index_description =
        "Each entry in this table represents a RedBox instance's notification status.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_redbox_notification_status_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_redbox_notification_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_redbox_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxStatistics
/******************************************************************************/
struct RedBoxStatistics {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamVal<vtss_appl_redbox_statistics_t *>> P;

    static constexpr const char *table_description =
        "Redbox statistics table.";

    static constexpr const char *index_description =
        "Each entry in this table represents a RedBox instance's statistics.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_redbox_statistics_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_redbox_statistics_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_redbox_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxNtClear()
/******************************************************************************/
struct RedBoxNtClear {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamVal<BOOL *>> P;

    static constexpr const char *table_description =
        "Clear RedBox' NodesTable.";

    static constexpr const char *index_description =
        "Each entry in this table represents a RedBox whose NodesTable may be cleared.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, redbox_serializer_nt_clear_value(i));
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_redbox_itr);
    VTSS_EXPOSE_GET_PTR(redbox_serializer_clear_get);
    VTSS_EXPOSE_SET_PTR(redbox_serializer_nt_clear_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxPntClear()
/******************************************************************************/
struct RedBoxPntClear {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamVal<BOOL *>> P;

    static constexpr const char *table_description =
        "Clear RedBox' ProxyNodeTable.";

    static constexpr const char *index_description =
        "Each entry in this table represents a RedBox whose ProxyNodeTable may be cleared.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, redbox_serializer_pnt_clear_value(i));
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_redbox_itr);
    VTSS_EXPOSE_GET_PTR(redbox_serializer_clear_get);
    VTSS_EXPOSE_SET_PTR(redbox_serializer_pnt_clear_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

/******************************************************************************/
// RedBoxStatisticsClear()
/******************************************************************************/
struct RedBoxStatisticsClear {
    typedef expose::ParamList <expose::ParamKey<uint32_t>, expose::ParamVal<BOOL *>> P;

    static constexpr const char *table_description =
        "Clear RedBox statistics table.";

    static constexpr const char *index_description =
        "Each entry in this table represents a RedBox whose statistics may be cleared.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(uint32_t &i)
    {
        h.argument_properties(expose::snmp::OidOffset(1));
        serialize(h, redbox_serializer_inst_index(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(BOOL &i)
    {
        h.argument_properties(expose::snmp::OidOffset(2));
        serialize(h, redbox_serializer_statistics_clear_value(i));
    }

    VTSS_EXPOSE_ITR_PTR(vtss_appl_redbox_itr);
    VTSS_EXPOSE_GET_PTR(redbox_serializer_clear_get);
    VTSS_EXPOSE_SET_PTR(redbox_serializer_statistics_clear_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_REDBOX);
};

}  // namespace interfaces
}  // namespace redbox
}  // namespace appl
}  // namespace vtss
#endif // _REDBOX_SERIALIZER_HXX_

