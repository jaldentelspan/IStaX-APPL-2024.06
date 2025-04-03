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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "ptp_api.h"
#include "vtss_common_iterator.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"
#include "misc_api.h"        /* For iport2uport()         */

// Enum descriptors for elements of vtss_appl_ptp_ext_clock_mode_t structure

const vtss_enum_descriptor_t vtss_appl_ptp_ext_clock_1pps_txt[] = {
    {VTSS_APPL_PTP_ONE_PPS_DISABLE, "onePpsDisable"},
    {VTSS_APPL_PTP_ONE_PPS_OUTPUT, "onePpsOutput"},
    {}};

const vtss_enum_descriptor_t vtss_appl_ptp_preferred_adj_txt[] = {
    {VTSS_APPL_PTP_PREFERRED_ADJ_LTC, "preferredAdjLtc"},
    {VTSS_APPL_PTP_PREFERRED_ADJ_SINGLE, "preferredAdjSingle"},
    {VTSS_APPL_PTP_PREFERRED_ADJ_INDEPENDENT, "preferredAdjIndependent"},
    {VTSS_APPL_PTP_PREFERRED_ADJ_COMMON, "preferredAdjCommon"},
    {VTSS_APPL_PTP_PREFERRED_ADJ_AUTO, "preferredAdjAuto"},
    {}};

// Enum descriptor for vtss_appl_ptp_system_time_sync_conf_t

const vtss_enum_descriptor_t vtss_appl_ptp_system_time_sync_mode_txt[] = {
    {VTSS_APPL_PTP_SYSTEM_TIME_NO_SYNC, "systemTimeNoSync"},
    {VTSS_APPL_PTP_SYSTEM_TIME_SYNC_GET, "systemTimeSyncGet"},
    {VTSS_APPL_PTP_SYSTEM_TIME_SYNC_SET, "systemTimeSyncSet"},
    {}};

// Enum descriptor for vtss_appl_ptp_device_type_t

const vtss_enum_descriptor_t vtss_appl_ptp_device_type_txt[] = {
    {VTSS_APPL_PTP_DEVICE_NONE, "none"},
    {VTSS_APPL_PTP_DEVICE_ORD_BOUND, "ordBound"},
    {VTSS_APPL_PTP_DEVICE_P2P_TRANSPARENT, "p2pTransparent"},
    {VTSS_APPL_PTP_DEVICE_E2E_TRANSPARENT, "e2eTransparent"},
    {VTSS_APPL_PTP_DEVICE_MASTER_ONLY, "masterOnly"},
    {VTSS_APPL_PTP_DEVICE_SLAVE_ONLY, "slaveOnly"},
    {VTSS_APPL_PTP_DEVICE_BC_FRONTEND, "bcFrontend"},
    {VTSS_APPL_PTP_DEVICE_AED_GM, "aedGm"},
    {VTSS_APPL_PTP_DEVICE_INTERNAL, "internal"},
    {}};

// Enum descriptor for vtss_appl_ptp_protocol_t

const vtss_enum_descriptor_t vtss_appl_ptp_protocol_txt[] = {
    {VTSS_APPL_PTP_PROTOCOL_ETHERNET, "ethernet"},
    {VTSS_APPL_PTP_PROTOCOL_ETHERNET_MIXED, "ethernetMixed"},
    {VTSS_APPL_PTP_PROTOCOL_IP4MULTI, "ip4multi"},
    {VTSS_APPL_PTP_PROTOCOL_IP4MIXED, "ip4mixed"},
    {VTSS_APPL_PTP_PROTOCOL_IP4UNI, "ip4uni"},
    {VTSS_APPL_PTP_PROTOCOL_OAM, "oam"},
    {VTSS_APPL_PTP_PROTOCOL_ONE_PPS, "onePps"},
    {VTSS_APPL_PTP_PROTOCOL_IP6MIXED, "ip6mixed"},
    {VTSS_APPL_PTP_PROTOCOL_ANY, "ethip4ip6combo"},
    {}};

// Enum descriptor for vtss_appl_ptp_profile_t

const vtss_enum_descriptor_t vtss_appl_ptp_profile_txt[] = {
    {VTSS_APPL_PTP_PROFILE_NO_PROFILE, "noProfile"},
    {VTSS_APPL_PTP_PROFILE_1588, "ieee1588"},
    {VTSS_APPL_PTP_PROFILE_G_8265_1, "g8265"},
    {VTSS_APPL_PTP_PROFILE_G_8275_1, "g8275d1"},
    {VTSS_APPL_PTP_PROFILE_G_8275_2, "g8275d2"},
    {VTSS_APPL_PTP_PROFILE_IEEE_802_1AS, "ieee802d1as"},
    {}};

// Enum descriptor for vtss_apppl_ptp_filter_type_t
const vtss_enum_descriptor_t vtss_appl_ptp_filter_type_txt[] = {
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_DEFAULT, "default"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_XO, "freqXo"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_XO, "phaseXo"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_TCXO, "freqTxco"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_TCXO, "phaseTxco"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_OCXO_S3E, "freqOcxoS3e"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E, "phaseOcxoS3e"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_FREQ, "partialOnPathFreq"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_PARTIAL_ON_PATH_PHASE, "partialOnPathPhase"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_FREQ, "fullOnPathFreq"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE, "fullOnPathPhase"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_BC_FULL_ON_PATH_PHASE_FASTER_LOCK_LOW_PKT_RATE,"fullOnPathPhaseFasterLockLowPktRate"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_FDD, "freqAccuracyFdd"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_FREQ_ACCURACY_XDSL, "freqAccuracyXdsl"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_FREQ, "elecFreq"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_ELEC_PHASE, "elecPhase"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C60W, "phaseRelaxedC60w"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C150, "phaseRelaxedC150"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C180, "phaseRelaxedC180"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_RELAXED_C240, "phaseRelaxedC240"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_PHASE_OCXO_S3E_R4_6_1, "phaseOcx0S3eR461"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE, "basicPhase"},
    {VTSS_APPL_PTP_FILTER_TYPE_ACI_BASIC_PHASE_LOW, "basicPhaseLow"},
    {VTSS_APPL_PTP_FILTER_TYPE_BASIC, "basic"},
    {VTSS_APPL_PTP_FILTER_TYPE_MAX_TYPE, "maxType"},
   {}};  

// Enum descriptor for vtss_appl_virtual_port_mode_t
const vtss_enum_descriptor_t  vtss_appl_virtual_port_mode_txt[] = {
    {VTSS_PTP_APPL_VIRTUAL_PORT_MODE_DISABLE, "noVirtualPortMode"},
    {VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_AUTO, "mainAuto"},
    {VTSS_PTP_APPL_VIRTUAL_PORT_MODE_SUB, "sub"},
    {VTSS_PTP_APPL_VIRTUAL_PORT_MODE_MAIN_MAN, "mainMan"},
    {VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_IN, "ppsIn"},
    {VTSS_PTP_APPL_VIRTUAL_PORT_MODE_PPS_OUT, "ppsOut"},
    {VTSS_PTP_APPL_VIRTUAL_PORT_MODE_FREQ_OUT, "freqOut"},
    {}};

// Enum descriptor for vtss_appl_virtual_port_mode_t
const vtss_enum_descriptor_t vtss_ptp_appl_rs422_protocol_txt[] = {
    {VTSS_PTP_APPL_RS422_PROTOCOL_SER_POLYT, "nmeaProprietaryPolytFormat"},
    {VTSS_PTP_APPL_RS422_PROTOCOL_SER_ZDA, "nmeaZdaFormat"},
    {VTSS_PTP_APPL_RS422_PROTOCOL_SER_GGA, "nmeaGgaFormat"},
    {VTSS_PTP_APPL_RS422_PROTOCOL_SER_RMC, "nmeaRmcFormat"},
    {VTSS_PTP_APPL_RS422_PROTOCOL_PIM, "ptpPimProtocol"},
    {VTSS_PTP_APPL_RS422_PROTOCOL_NONE, "noTodProtocol"},
    {}};
// Enum descriptor for vtss_appl_ptp_leap_second_type_t

const vtss_enum_descriptor_t vtss_appl_ptp_leap_second_type_txt[] = {
    {VTSS_APPL_PTP_LEAP_SECOND_59, "leap59"},
    {VTSS_APPL_PTP_LEAP_SECOND_61, "leap61"},
    {}};

// Enum descriptor for vtss_appl_ptp_srv_clock_option_t

const vtss_enum_descriptor_t vtss_appl_ptp_srv_clock_option_txt[] = {
    {VTSS_APPL_PTP_CLOCK_FREE, "clockFree"},
    {VTSS_APPL_PTP_CLOCK_SYNCE, "clockSyncE"},
    {}};

// Enum descriptor for vtss_appl_ptp_dest_adr_type_t

const vtss_enum_descriptor_t vtss_appl_ptp_dest_adr_type_txt[] = {
    {VTSS_APPL_PTP_PROTOCOL_SELECT_DEFAULT, "default"},
    {VTSS_APPL_PTP_PROTOCOL_SELECT_LINK_LOCAL, "linkLocal"},
    {}};

// Enum descriptor for vtss_appl_ptp_delay_mechanism_t
const vtss_enum_descriptor_t vtss_appl_ptp_delay_mechanism_txt[] = {
   {VTSS_APPL_PTP_DELAY_MECH_E2E, "e2e"},
   {VTSS_APPL_PTP_DELAY_MECH_P2P, "p2p"},
   {VTSS_APPL_PTP_DELAY_MECH_COMMON_P2P, "commonP2P"},
   {VTSS_APPL_PTP_DELAY_MECH_DISABLED, "disabled"},
   {}};

// Enum descriptor for vtss_appl_ptp_slave_clock_state_t

const vtss_enum_descriptor_t vtss_appl_ptp_slave_clock_state_txt[] = {
    {VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREERUN, "slaveClockStateFreerun"},
    {VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREQ_LOCKING, "slaveClockStateFreqLocking"},
    {VTSS_APPL_PTP_SLAVE_CLOCK_STATE_FREQ_LOCKED, "slaveClockStateFreqLocked"},
    {VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKING, "slaveClockStatePhaseLocking"},
    {VTSS_APPL_PTP_SLAVE_CLOCK_STATE_PHASE_LOCKED, "slaveClockStatePhaseLocked"},
    {VTSS_APPL_PTP_SLAVE_CLOCK_STATE_HOLDOVER, "slaveClockStateHoldover"},
    {VTSS_APPL_PTP_SLAVE_CLOCK_STATE_INVALID, "slaveClockStateInvalid"},
    {}};

const vtss_enum_descriptor_t vtss_appl_ptp_unicast_comm_state_txt[] = {
    {VTSS_APPL_PTP_COMM_STATE_IDLE, "idle"},
    {VTSS_APPL_PTP_COMM_STATE_INIT, "initializing"},
    {VTSS_APPL_PTP_COMM_STATE_CONN, "connected"},
    {VTSS_APPL_PTP_COMM_STATE_SELL, "selected"},
    {VTSS_APPL_PTP_COMM_STATE_SYNC, "synced"},
    {}
};

const vtss_enum_descriptor_t vtss_appl_ptp_clock_port_state_txt[] = {
    {VTSS_APPL_PTP_INITIALIZING, "initializing"},
    {VTSS_APPL_PTP_FAULTY, "faulty"},
    {VTSS_APPL_PTP_DISABLED, "disabled"},
    {VTSS_APPL_PTP_LISTENING, "listening"},
    {VTSS_APPL_PTP_PRE_MASTER, "preMaster"},
    {VTSS_APPL_PTP_MASTER, "master"},
    {VTSS_APPL_PTP_PASSIVE, "passive"},
    {VTSS_APPL_PTP_UNCALIBRATED, "uncalibrated"},
    {VTSS_APPL_PTP_SLAVE, "slave"},
    {VTSS_APPL_PTP_P2P_TRANSPARENT, "p2pTransparent"},
    {VTSS_APPL_PTP_E2E_TRANSPARENT, "e2eTransparent"},
    {VTSS_APPL_PTP_FRONTEND, "frontend"},
    {}
};

const vtss_enum_descriptor_t vtss_appl_ptp_802_1as_port_role_txt[] = {
    {VTSS_APPL_PTP_PORT_ROLE_DISABLED_PORT, "disabledPort"},
    {VTSS_APPL_PTP_PORT_ROLE_MASTER_PORT, "masterPort"},
    {VTSS_APPL_PTP_PORT_ROLE_PASSIVE_PORT, "passivePort"},
    {VTSS_APPL_PTP_PORT_ROLE_SLAVE_PORT, "slavePort"},
    {}
};

mesa_rc vtss_appl_ptp_capabilities_global_get(vtss_appl_ptp_capabilities_t *const c)
{
    c->ptp_clock_max = PTP_CLOCK_INSTANCES;

    return VTSS_RC_OK;
}

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_ext_clock_out_get(vtss_appl_ptp_ext_clock_mode_t *const mode)   
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_ext_clock_out_set(const vtss_appl_ptp_ext_clock_mode_t *const mode)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_system_time_sync_mode_set(const vtss_appl_ptp_system_time_sync_conf_t *const conf)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_system_time_sync_mode_get(vtss_appl_ptp_system_time_sync_conf_t *const conf)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_config_default_ds_get(uint instance, vtss_appl_ptp_clock_config_default_ds_t *const clock_config)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_config_default_ds_set(uint instance, const vtss_appl_ptp_clock_config_default_ds_t *clock_config)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_config_default_ds_del(uint instance)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_config_timeproperties_ds_get(uint instance, vtss_appl_ptp_clock_timeproperties_ds_t *const timeproperties_ds)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_config_timeproperties_ds_set(uint instance, const vtss_appl_ptp_clock_timeproperties_ds_t *const timeproperties_ds)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_filter_parameters_get(uint instance, vtss_appl_ptp_clock_filter_config_t *const c)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_filter_parameters_set(uint instance, const vtss_appl_ptp_clock_filter_config_t *const c)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_servo_parameters_get(uint instance, vtss_appl_ptp_clock_servo_config_t *const c)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_servo_parameters_set(uint instance, const vtss_appl_ptp_clock_servo_config_t *const c)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_slave_config_get(uint instance, vtss_appl_ptp_clock_slave_config_t *const cfg)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_slave_config_set(uint instance, const vtss_appl_ptp_clock_slave_config_t *const cfg)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_config_unicast_slave_config_get(uint instance, uint idx, vtss_appl_ptp_unicast_slave_config_t *const c)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_config_unicast_slave_config_set(uint instance, uint idx, const vtss_appl_ptp_unicast_slave_config_t *const c)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_config_port_ds_get(uint instance, uint portnum, vtss_appl_ptp_config_port_ds_t *port_ds)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_config_port_ds_set(uint instance, uint portnum, const vtss_appl_ptp_config_port_ds_t *port_ds)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_status_default_ds_get(uint instance, vtss_appl_ptp_clock_status_default_ds_t *const clock_status)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_status_current_ds_get(uint instance, vtss_appl_ptp_clock_current_ds_t *const status)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_status_parent_ds_get(uint instance, vtss_appl_ptp_clock_parent_ds_t *const status)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_status_timeproperties_ds_get(uint instance, vtss_appl_ptp_clock_timeproperties_ds_t *timeproperties_ds)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_status_slave_ds_get(uint instance, vtss_appl_ptp_clock_slave_ds_t *const status)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_status_unicast_master_table_get(uint instance, vtss_appl_ptp_protocol_adr_t slave, vtss_appl_ptp_unicast_master_table_t *const uni_master_table)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_status_unicast_slave_table_get(uint instance, uint ix, vtss_appl_ptp_unicast_slave_table_t *const uni_slave_table)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clocks_status_port_ds_get(uint instance, uint portnum, vtss_appl_ptp_status_port_ds_t *const port_ds)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_control_get(uint instance, vtss_appl_ptp_clock_control_t *const port_control)
// {
//     return VTSS_RC_ERROR;
// }

// Defined in ptp.c
//
// mesa_rc vtss_appl_ptp_clock_control_set(uint instance, const vtss_appl_ptp_clock_control_t *const port_control)
// {
//     return VTSS_RC_ERROR;
// }

mesa_rc vtss_appl_ptp_clock_itr(const uint *const prev, uint *const next)
{
    vtss::expose::snmp::IteratorComposeRange<uint> i(0, PTP_CLOCK_INSTANCES - 1);
    return i(prev, next);
}

mesa_rc vtss_appl_ptp_master_itr(const uint *const prev, uint *const next)
{
    vtss::expose::snmp::IteratorComposeRange<uint> i(0, 4);
    return i(prev, next);
}

// mesa_rc vtss_appl_ptp_port_itr(const uint *const prev, uint *const next)
// { 
//     vtss::expose::snmp::IteratorComposeRange<uint> i(1, port_count_max());
//     return i(prev, next);
// }

mesa_rc vtss_appl_ptp_clock_master_itr(const uint *const clock_prev, uint *const clock_next, const uint *const master_prev, uint *const master_next)
{
    vtss::IteratorComposeN<uint, uint> i(vtss_appl_ptp_clock_itr, vtss_appl_ptp_master_itr);
    return i(clock_prev, clock_next, master_prev, master_next);
}

//
//
// mesa_rc vtss_appl_ptp_clock_slave_itr(const uint *const clock_prev, uint *const clock_next, const uint *const slave_prev, uint *const slave_next)
// {
//     return VTSS_RC_ERROR;
// }

mesa_rc vtss_appl_ptp_clock_port_itr(const uint *const clock_prev, uint *const clock_next, const vtss_ifindex_t *const port_prev, vtss_ifindex_t *const port_next)
{
    vtss::IteratorComposeN<uint, vtss_ifindex_t> i(vtss_appl_ptp_clock_itr, vtss_appl_iterator_ifindex_front_port_exist);
    return i(clock_prev, clock_next, port_prev, port_next);
}

mesa_rc vtss_appl_ptp_port_itr(const vtss_uport_no_t *port_prev, vtss_uport_no_t *const port_next)
{
    vtss_ifindex_t if_prev, if_next;
    u32 v;

    if (!port_prev) {
        *port_next = 1;
        return VTSS_RC_OK;
    }
    vtss_ifindex_from_usid_uport(VTSS_ISID_START, *port_prev, &if_prev);
    VTSS_RC(vtss_appl_iterator_ifindex_front_port_exist(&if_prev, &if_next));
    VTSS_RC(ptp_ifindex_to_port(if_next, &v));
    *port_next = iport2uport(v);
    return VTSS_RC_OK;
}
