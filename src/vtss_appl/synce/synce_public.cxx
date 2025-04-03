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

#include "synce.h"
#include "synce_api.h"
#include "vtss_common_iterator.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-range.hxx"
#include "vtss/basics/expose/snmp/iterator-compose-N.hxx"

// Enum descriptors for elements of FIXME:

const vtss_enum_descriptor_t vtss_appl_synce_selection_mode_txt[] = {
    {VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL, "manual"},
    {VTSS_APPL_SYNCE_SELECTOR_MODE_MANUEL_TO_SELECTED, "manualToSelected"},
    {VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_NONREVERTIVE, "autoNonrevertive"},
    {VTSS_APPL_SYNCE_SELECTOR_MODE_AUTOMATIC_REVERTIVE, "autoRevertive"},
    {VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_HOLDOVER, "forcedHoldover"},
    {VTSS_APPL_SYNCE_SELECTOR_MODE_FORCED_FREE_RUN, "forcedFreeRun"},
    {}};

const vtss_enum_descriptor_t vtss_appl_synce_quality_level_txt[] = {
    {VTSS_APPL_SYNCE_QL_NONE, "qlNone"},
    {VTSS_APPL_SYNCE_QL_PRC,  "qlPrc"},
    {VTSS_APPL_SYNCE_QL_SSUA, "qlSsua"},
    {VTSS_APPL_SYNCE_QL_SSUB, "qlSsub"},
    {VTSS_APPL_SYNCE_QL_EEC1, "qlEec1"},
    {VTSS_APPL_SYNCE_QL_DNU,  "qlDnu"},
    {VTSS_APPL_SYNCE_QL_INV,  "qlInv"},
    {VTSS_APPL_SYNCE_QL_FAIL, "qlFail"},
    {VTSS_APPL_SYNCE_QL_LINK, "qlLink"},
    {VTSS_APPL_SYNCE_QL_PRS,  "qlPrs"},
    {VTSS_APPL_SYNCE_QL_STU,  "qlStu"},
    {VTSS_APPL_SYNCE_QL_ST2,  "qlSt2"},
    {VTSS_APPL_SYNCE_QL_TNC,  "qlTnc"},
    {VTSS_APPL_SYNCE_QL_ST3E, "qlSt3e"},
    {VTSS_APPL_SYNCE_QL_EEC2, "qlEec2"},
    {VTSS_APPL_SYNCE_QL_SMC,  "qlSmc"},
    {VTSS_APPL_SYNCE_QL_PROV, "qlProv"},
    {VTSS_APPL_SYNCE_QL_DUS,  "qlDus"},
    {}};

const vtss_enum_descriptor_t vtss_appl_synce_eec_option_txt[] = {
    {VTSS_APPL_SYNCE_EEC_OPTION_1, "eecOption1"},
    {VTSS_APPL_SYNCE_EEC_OPTION_2, "eecOption2"},    
    {}};

const vtss_enum_descriptor_t vtss_appl_synce_aneg_mode_txt[] = {
    {VTSS_APPL_SYNCE_ANEG_NONE, "none"},
    {VTSS_APPL_SYNCE_ANEG_PREFERED_SLAVE, "preferedSlave"},
    {VTSS_APPL_SYNCE_ANEG_PREFERED_MASTER, "preferedMaster"},
    {VTSS_APPL_SYNCE_ANEG_FORCED_SLAVE, "forcedSlave"},
    {}};

const vtss_enum_descriptor_t vtss_appl_synce_selector_state_txt[] = {
    {VTSS_APPL_SYNCE_SELECTOR_STATE_LOCKED, "locked"},
    {VTSS_APPL_SYNCE_SELECTOR_STATE_HOLDOVER, "holdover"},
    {VTSS_APPL_SYNCE_SELECTOR_STATE_FREERUN, "freerun"},
    {VTSS_APPL_SYNCE_SELECTOR_STATE_PTP, "ptp"},
    {VTSS_APPL_SYNCE_SELECTOR_STATE_REF_FAILED, "refFailed"},
    {VTSS_APPL_SYNCE_SELECTOR_STATE_ACQUIRING, "acquiring"},
    {}};

const vtss_enum_descriptor_t vtss_appl_synce_frequency_txt[] = {
    {VTSS_APPL_SYNCE_STATION_CLK_DIS, "disabled"},
    {VTSS_APPL_SYNCE_STATION_CLK_1544_KHZ, "freq1544kHz"},
    {VTSS_APPL_SYNCE_STATION_CLK_2048_KHZ, "freq2048kHz"},
    {VTSS_APPL_SYNCE_STATION_CLK_10_MHZ, "freq10MHz"},
    {VTSS_APPL_SYNCE_STATION_CLK_MAX, "freqMax"},
    {}};

const vtss_enum_descriptor_t vtss_appl_synce_ptp_ptsf_state_txt[] = {
    {SYNCE_PTSF_NONE, "none"},
    {SYNCE_PTSF_UNUSABLE, "unusable"},
    {SYNCE_PTSF_LOSS_OF_SYNC, "lossSync"},
    {SYNCE_PTSF_LOSS_OF_ANNOUNCE, "lossAnnounce"},
    {}};

const vtss_enum_descriptor_t vtss_appl_synce_lol_alarm_state_txt[] = {
    {VTSS_APPL_SYNCE_LOL_ALARM_STATE_FALSE, "false"},
    {VTSS_APPL_SYNCE_LOL_ALARM_STATE_TRUE, "true"},
    {VTSS_APPL_SYNCE_LOL_ALARM_STATE_NA, "na"},
    {}};

const vtss_enum_descriptor_t vtss_appl_synce_clock_hw_id_txt[] = {
    {MEBA_SYNCE_CLOCK_HW_NONE,     "hwNone"},
    {MEBA_SYNCE_CLOCK_HW_SI_5326,  "hwSI5326"},
    {MEBA_SYNCE_CLOCK_HW_SI_5328,  "hwSI5328"},
    {MEBA_SYNCE_CLOCK_HW_ZL_30343, "hwZL30343"},
    {MEBA_SYNCE_CLOCK_HW_ZL_30363, "hwZL30363"},
    {MEBA_SYNCE_CLOCK_HW_OMEGA,    "hwOmega"},
    {MEBA_SYNCE_CLOCK_HW_ZL_30771, "hwZL30771"},
    {MEBA_SYNCE_CLOCK_HW_ZL_30772, "hwZL30772"},
    {MEBA_SYNCE_CLOCK_HW_ZL_30773, "hwZL30773"},
    {MEBA_SYNCE_CLOCK_HW_ZL_30731, "hwZL30731"},
    {MEBA_SYNCE_CLOCK_HW_ZL_30732, "hwZL30732"},
    {MEBA_SYNCE_CLOCK_HW_ZL_30733, "hwZL30733"},
    {MEBA_SYNCE_CLOCK_HW_ZL_30734, "hwZL30734"},
    {MEBA_SYNCE_CLOCK_HW_ZL_30735, "hwZL30735"},
    {}};

mesa_rc vtss_appl_synce_capabilities_global_get(vtss_appl_synce_capabilities_t *const c)
{
    meba_synce_clock_hw_id_t hw_id;
    c->synce_source_count = synce_my_nominated_max;
    clock_hardware_id_get(&hw_id);
    c->dpll_type = hw_id;
    clock_dpll_fw_ver_get(&c->dpll_fw_ver);
    clock_station_clock_type_get(&c->clock_type);

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_synce_clock_source_control_get(uint sourceId, vtss_appl_synce_clock_source_control_t *const port_control)
{
    if ((0 < sourceId) && (sourceId <= synce_my_nominated_max)) {
        memset(port_control, 0, sizeof(vtss_appl_synce_clock_source_control_t));
        return VTSS_RC_OK;
    }
    else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc vtss_appl_synce_clock_source_control_set(uint sourceId, const vtss_appl_synce_clock_source_control_t *const port_control)
{
    if ((0 < sourceId) && (sourceId <= synce_my_nominated_max)) {
        if (port_control->clearWtr == TRUE) {
            synce_mgmt_wtr_clear_set(sourceId - 1);
        }
        return VTSS_RC_OK;
    }
    else {
        return VTSS_RC_ERROR;
    }
}

mesa_rc vtss_appl_synce_source_itr(const uint *const prev, uint *const next)
{
    vtss::expose::snmp::IteratorComposeRange<uint> i(1, synce_my_nominated_max);
    return i(prev, next);
}

mesa_rc vtss_appl_synce_port_itr(const vtss_ifindex_t *const prev, vtss_ifindex_t *const next)
{
    return vtss_appl_iterator_ifindex_front_port_exist(prev, next);
}

mesa_rc vtss_appl_synce_ptp_source_itr(const uint *const prev, uint *const next)
{
    vtss::expose::snmp::IteratorComposeRange<uint> i(1, PTP_CLOCK_INSTANCES);
    return i(prev, next);
}
