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
#ifndef __VTSS_TSN_FP_SERIALIZER_HXX__
#define __VTSS_TSN_FP_SERIALIZER_HXX__

#include "vtss_appl_serialize.hxx"
#include "vtss/appl/tsn.h"
#include "vtss/basics/expose/json.hxx"
#include "vtss_appl_formatting_tags.hxx" // for AsInterfaceIndex
#include <vtss/appl/types.hxx>


/*****************************************************************************
    Enum serializer
*****************************************************************************/
extern const vtss_enum_descriptor_t vtss_appl_tsn_mm_status_verify_txt[];
VTSS_XXXX_SERIALIZE_ENUM(vtss_appl_tsn_mm_status_verify_t,
                         "MACMergeStatusVerify",
                         vtss_appl_tsn_mm_status_verify_txt,
                         "MAC Merge Status Verify (aMACMergeStatusVerify in 802.3br).");


template<typename T>
void serialize(T &a, vtss_appl_tsn_fp_cfg_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_tsn_fp_cfg_t"));

    int ix = 0;

    m.add_leaf(vtss::AsBool(s.enable_tx),
               vtss::tag::Name("EnableTx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br aMACMergeEnableTx parameter for the port. "
                                      "This value determines whether frame preemption is enabled (TRUE) or "
                                      "disabled (FALSE) in the MAC Merge sublayer in the transmit direction."));

    m.add_leaf(vtss::AsBool(s.ignore_lldp_tx),
               vtss::tag::Name("IgnoreLldpTx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("If this value is enabled (TRUE) the enable_tx parameter will determine if preemption "
                                      "is enabled without considering the state of potentially received lldp information. Default value false."));

    m.add_leaf(vtss::AsBool(s.verify_disable_tx),
               vtss::tag::Name("VerifyDisableTx"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br aMACMergeVerifyDisableTx parameter for the port. "
                                      "This value determines whether the verifiy function is disabled (TRUE) or "
                                      "enabled (FALSE) in the MAC Merge sublayer in the transmit direction."));

    m.add_leaf(s.verify_time,
               vtss::tag::Name("VerifyTime"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br aMACMergeVerifyTime parameter for the port. "
                                      "This value determines the nominal wait time between verification attempts in "
                                      "milliseconds. Valid range is 1 to 128 inclusive. The default value is 10."));

    m.add_leaf(vtss::AsBool(s.admin_status[0]),
               vtss::tag::Name("AdminQ0"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IEEE802.1Qbu: FramePreemptionStatus value priority 0. If value is TRUE, "
                                      "the service is express, if FALSE and EnableTx is TRUE the service is preemptable."));

    m.add_leaf(vtss::AsBool(s.admin_status[1]),
               vtss::tag::Name("AdminQ1"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IEEE802.1Qbu: FramePreemptionStatus value priority 1. If value is TRUE, "
                                      "the service is express, if FALSE and EnableTx is TRUE the service is preemptable."));

    m.add_leaf(vtss::AsBool(s.admin_status[2]),
               vtss::tag::Name("AdminQ2"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IEEE802.1Qbu: FramePreemptionStatus value priority 2. If value is TRUE, "
                                      "the service is express, if FALSE and EnableTx is TRUE the service is preemptable."));

    m.add_leaf(vtss::AsBool(s.admin_status[3]),
               vtss::tag::Name("AdminQ3"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IEEE802.1Qbu: FramePreemptionStatus value priority 3. If value is TRUE, "
                                      "the service is express, if FALSE and EnableTx is TRUE the service is preemptable."));

    m.add_leaf(vtss::AsBool(s.admin_status[4]),
               vtss::tag::Name("AdminQ4"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IEEE802.1Qbu: FramePreemptionStatus value priority 4. If value is TRUE, "
                                      "the service is express, if FALSE and EnableTx is TRUE the service is preemptable."));

    m.add_leaf(vtss::AsBool(s.admin_status[5]),
               vtss::tag::Name("AdminQ5"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IEEE802.1Qbu: FramePreemptionStatus value priority 5. If value is TRUE, "
                                      "the service is express, if FALSE and EnableTx is TRUE the service is preemptable."));

    m.add_leaf(vtss::AsBool(s.admin_status[6]),
               vtss::tag::Name("AdminQ6"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IEEE802.1Qbu: FramePreemptionStatus value priority 6. If value is TRUE, "
                                      "the service is express, if FALSE and EnableTx is TRUE the service is preemptable."));

    m.add_leaf(vtss::AsBool(s.admin_status[7]),
               vtss::tag::Name("AdminQ7"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("IEEE802.1Qbu: FramePreemptionStatus value priority 7. If value is TRUE, "
                                      "the service is express, if FALSE and EnableTx is TRUE the service is preemptable."));

}


template<typename T>
void serialize(T &a, vtss_appl_tsn_fp_status_t &s)
{
    typename T::Map_t m = a.as_map(vtss::tag::Typename("vtss_appl_tsn_fp_status_t"));

    int ix = 0;

    m.add_leaf(s.hold_advance,
               vtss::tag::Name("HoldAdvance"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the holdAdvance parameter for the port in nanoseconds."));

    m.add_leaf(s.release_advance,
               vtss::tag::Name("ReleaseAdvance"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the releaseAdvance parameter for the port in nanoseconds."));

    m.add_leaf(vtss::AsBool(s.preemption_active),
               vtss::tag::Name("PreemptionActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value is active (TRUE) when preemption is operationally "
                                      "active for the port, and idle (FALSE) otherwise."));

    m.add_leaf(vtss::AsBool(s.hold_request),
               vtss::tag::Name("HoldRequest"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value is hold (TRUE) when the sequence of gate operations "
                                      "for the port has executed a Set-And-Hold-MAC operation, "
                                      "and release (FALSE) when the sequence of gate operations has "
                                      "executed a Set-And-Release-MAC operation.\n"
                                      "The value of this object is release (FALSE) on system initialization."));

    m.add_leaf(s.status_verify,
               vtss::tag::Name("StatusVerify"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The status of the MAC Merge sublayer verification for the given device."));

    m.add_leaf(vtss::AsBool(s.loc_preempt_supported),
               vtss::tag::Name("LocPreemptSupported"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value is TRUE when preemption is supported "
                                      "on the port, and FALSE otherwise."));

    m.add_leaf(vtss::AsBool(s.loc_preempt_enabled),
               vtss::tag::Name("LocPreemptEnabled"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value is TRUE when preemption is enabled "
                                      "on the port, and FALSE otherwise."));

    m.add_leaf(vtss::AsBool(s.loc_preempt_active),
               vtss::tag::Name("LocPreemptActive"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value is TRUE when preemption is operationally "
                                      "active on the port, and FALSE otherwise."));

    m.add_leaf(s.loc_add_frag_size,
               vtss::tag::Name("LocAddFragSize"),
               vtss::expose::snmp::Status::Current,
               vtss::expose::snmp::OidElementValue(ix++),
               vtss::tag::Description("The value of the 802.3br LocAddFragSize parameter for the port. "
                                      "The minimum size of non-final fragments supported by the "
                                      "receiver on the local port. This value is expressed in units "
                                      "of 64 octets of additional fragment length. "
                                      "The minimum non-final fragment size is: (LocAddFragSize + 1) * 64 octets."));

}


namespace vtss
{
namespace appl
{
namespace tsn
{
namespace interfaces
{

struct TsnFpCfgTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_tsn_fp_cfg_t *>
         > P;

    static constexpr const char *table_description =
        "A table that contains the per-port frame preemption configuration.";

    static constexpr const char *index_description =
        "A list of objects that contains the frame preemption configuration for a port.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_tsn_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_tsn_fp_cfg_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_tsn_fp_cfg_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_tsn_fp_cfg_itr);
    VTSS_EXPOSE_SET_PTR(vtss_appl_tsn_fp_cfg_set);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_TSN);
};

struct TsnFpStatusTable {
    typedef vtss::expose::ParamList <
    vtss::expose::ParamKey<vtss_ifindex_t>,
         vtss::expose::ParamVal<vtss_appl_tsn_fp_status_t *>
         > P;

    static constexpr const char *table_description =
        "A table that contains the per-port frame preemption status.";

    static constexpr const char *index_description =
        "A list of objects that contains the frame preemption status for a port.";

    VTSS_EXPOSE_SERIALIZE_ARG_1(vtss_ifindex_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(1));
        serialize(h, vtss_appl_tsn_ifindex_t(i));
    }

    VTSS_EXPOSE_SERIALIZE_ARG_2(vtss_appl_tsn_fp_status_t &i)
    {
        h.argument_properties(vtss::expose::snmp::OidOffset(2));
        serialize(h, i);
    }

    VTSS_EXPOSE_GET_PTR(vtss_appl_tsn_fp_status_get);
    VTSS_EXPOSE_ITR_PTR(vtss_appl_tsn_fp_status_itr);
    VTSS_EXPOSE_WEB_PRIV(VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_TSN);
};

}  // namespace interfaces
}  // namespace tsn
}  // namespace appl
}  // namespace vtss

#endif /* __VTSS_TSN_FP_SERIALIZER_HXX__ */
