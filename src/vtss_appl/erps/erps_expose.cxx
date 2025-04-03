/*
 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "erps_serializer.hxx"
#include <vtss/appl/erps.h>


vtss_enum_descriptor_t vtss_appl_erps_ring_type_txt[] {
    {VTSS_APPL_ERPS_RING_TYPE_MAJOR,                                            "major"},
    {VTSS_APPL_ERPS_RING_TYPE_SUB,                                              "sub"},
    {VTSS_APPL_ERPS_RING_TYPE_INTERCONNECTED_SUB,                               "interconnectedSub"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_erps_command_txt[] {
    {VTSS_APPL_ERPS_COMMAND_NR,                                                 "noRequest"},
    {VTSS_APPL_ERPS_COMMAND_FS_TO_PORT0,                                        "forceSwitchToPort0"},
    {VTSS_APPL_ERPS_COMMAND_FS_TO_PORT1,                                        "forceSwitchToPort1"},
    {VTSS_APPL_ERPS_COMMAND_MS_TO_PORT0,                                        "manualSwitchToPort0"},
    {VTSS_APPL_ERPS_COMMAND_MS_TO_PORT1,                                        "manualSwitchToPort1"},
    {VTSS_APPL_ERPS_COMMAND_CLEAR,                                              "clear"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_erps_version_txt[] {
    {VTSS_APPL_ERPS_VERSION_V1,                                                 "v1"},
    {VTSS_APPL_ERPS_VERSION_V2,                                                 "v2"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_erps_rpl_mode_txt[] {
    {VTSS_APPL_ERPS_RPL_MODE_NONE,                                              "none"},
    {VTSS_APPL_ERPS_RPL_MODE_OWNER,                                             "owner"},
    {VTSS_APPL_ERPS_RPL_MODE_NEIGHBOR,                                          "neighbor"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_erps_sf_trigger_txt[] {
    {VTSS_APPL_ERPS_SF_TRIGGER_LINK,                                            "link"},
    {VTSS_APPL_ERPS_SF_TRIGGER_MEP,                                             "mep"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_erps_request_txt[] {
    {VTSS_APPL_ERPS_REQUEST_NR,                                                 "noRequest"},
    {VTSS_APPL_ERPS_REQUEST_MS,                                                 "manualSwitch"},
    {VTSS_APPL_ERPS_REQUEST_SF,                                                 "signalFailed"},
    {VTSS_APPL_ERPS_REQUEST_FS,                                                 "forceSwitch"},
    {VTSS_APPL_ERPS_REQUEST_EVENT,                                              "event"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_erps_ring_port_txt[] {
    {VTSS_APPL_ERPS_RING_PORT0,                                                 "ringPort0"},
    {VTSS_APPL_ERPS_RING_PORT1,                                                 "ringPort1"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_erps_oper_state_txt[] {
    {VTSS_APPL_ERPS_OPER_STATE_ADMIN_DISABLED,                                  "disabled"},
    {VTSS_APPL_ERPS_OPER_STATE_ACTIVE,                                          "active"},
    {VTSS_APPL_ERPS_OPER_STATE_INTERNAL_ERROR,                                  "internalError"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_erps_node_state_txt[] {
    {VTSS_APPL_ERPS_NODE_STATE_INIT,                                            "init"},
    {VTSS_APPL_ERPS_NODE_STATE_IDLE,                                            "idle"},
    {VTSS_APPL_ERPS_NODE_STATE_PROTECTION,                                      "protection"},
    {VTSS_APPL_ERPS_NODE_STATE_MS,                                              "ms"},
    {VTSS_APPL_ERPS_NODE_STATE_FS,                                              "fs"},
    {VTSS_APPL_ERPS_NODE_STATE_PENDING,                                         "pending"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_erps_oper_warning_txt[] {
    {VTSS_APPL_ERPS_OPER_WARNING_NONE,                                          "none"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT0_NOT_MEMBER_OF_CONTROL_VLAN,              "port0NotMemberOfControlVlan"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT1_NOT_MEMBER_OF_CONTROL_VLAN,              "port1NotMemberOfControlVlan"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT0_UNTAGS_CONTROL_VLAN,                     "port0UntagsControlVlan"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT1_UNTAGS_CONTROL_VLAN,                     "port1UntagsControlVlan"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_NOT_FOUND,                           "port0MepNotFound"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_NOT_FOUND,                           "port1MepNotFound"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_ADMIN_DISABLED,                      "port0MepAdminDisabled"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_ADMIN_DISABLED,                      "port1MepAdminDisabled"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT0_MEP_NOT_DOWN_MEP,                        "port0MepNotDownMep"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT1_MEP_NOT_DOWN_MEP,                        "port1MepNotDownMep"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT0_AND_MEP_IFINDEX_DIFFER,                  "port0AndMepIfindexDiffer"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT1_AND_MEP_IFINDEX_DIFFER,                  "port1AndMepIfindexDiffer"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT_MEP_SHADOWS_PORT0_MIP,                    "portMepShadowsPort0Mip"},
    {VTSS_APPL_ERPS_OPER_WARNING_PORT_MEP_SHADOWS_PORT1_MIP,                    "portMepShadowsPort1Mip"},
    {VTSS_APPL_ERPS_OPER_WARNING_MEP_SHADOWS_PORT0_MIP,                         "mepShadowsPort0Mip"},
    {VTSS_APPL_ERPS_OPER_WARNING_MEP_SHADOWS_PORT1_MIP,                         "mepShadowsPort1Mip"},
    {VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_DOESNT_EXIST,                   "connectedRingDoesntExist"},
    {VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_IS_AN_INTERCONNECTED_SUB_RING,  "connectedRingIsAnInterconnectedSubRing"},
    {VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_IS_NOT_OPERATIVE,               "connectedRingIsNotOperative"},
    {VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_INTERFACE_CONFLICT,             "connectedRingInterfaceConflict"},
    {VTSS_APPL_ERPS_OPER_WARNING_CONNECTED_RING_DOESNT_PROTECT_CONTROL_VLAN,    "connectedRingDoesntProtectControlVlan"},
    {0, 0},
};

// Wrappers
mesa_rc vtss_erps_create_conf_default(uint32_t *instance, vtss_appl_erps_conf_t *s)
{
    vtss_appl_erps_conf_t  def_conf;
    if (s) {
        VTSS_RC(vtss_appl_erps_conf_default_get(&def_conf));
        *s = def_conf;
    }
    return VTSS_RC_OK;
};

mesa_rc vtss_erps_statistics_clear(uint32_t instance, const BOOL *clear)
{
    if (clear && *clear) {
        VTSS_RC(vtss_appl_erps_statistics_clear(instance));
    }
    return VTSS_RC_OK;
};

mesa_rc vtss_erps_statistics_clear_dummy(uint32_t instance,  BOOL *clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }
    return VTSS_RC_OK;
};
