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

#include "aps_serializer.hxx"
#include <vtss/appl/aps.h>

vtss_enum_descriptor_t aps_sf_trigger_txt[] {
    {VTSS_APPL_APS_SF_TRIGGER_LINK,                         "link"},
    {VTSS_APPL_APS_SF_TRIGGER_MEP,                          "mep"},
    {0, 0},
};

vtss_enum_descriptor_t aps_mode_txt[] {
    {VTSS_APPL_APS_MODE_ONE_FOR_ONE,                        "oneForOne"},
    {VTSS_APPL_APS_MODE_ONE_PLUS_ONE_UNIDIRECTIONAL,        "onePlusOneUniDir"},
    {VTSS_APPL_APS_MODE_ONE_PLUS_ONE_BIDIRECTIONAL,         "onePlusOneBiDir"},
    {0, 0},
};

vtss_enum_descriptor_t aps_command_txt[] {
    {VTSS_APPL_APS_COMMAND_NR,                              "noRequest"},
    {VTSS_APPL_APS_COMMAND_LO,                              "lockout"},
    {VTSS_APPL_APS_COMMAND_FS,                              "forceSwitch"},
    {VTSS_APPL_APS_COMMAND_MS_TO_W,                         "manualSwitchToWorking"},
    {VTSS_APPL_APS_COMMAND_MS_TO_P,                         "manualSwitchToProtecting"},
    {VTSS_APPL_APS_COMMAND_EXER,                            "exercise"},
    {VTSS_APPL_APS_COMMAND_CLEAR,                           "clear"},
    {VTSS_APPL_APS_COMMAND_FREEZE,                          "freeze"},
    {VTSS_APPL_APS_COMMAND_FREEZE_CLEAR,                    "freezeClear"},
    {0, 0},
};

vtss_enum_descriptor_t aps_oper_state_txt [] {
    {VTSS_APPL_APS_OPER_STATE_ADMIN_DISABLED,               "disabled"},
    {VTSS_APPL_APS_OPER_STATE_ACTIVE,                       "active"},
    {VTSS_APPL_APS_OPER_STATE_INTERNAL_ERROR,               "internalError"},
    {0, 0},
};

vtss_enum_descriptor_t aps_oper_warning_txt [] {
    {VTSS_APPL_APS_OPER_WARNING_NONE,                         "none"},
    {VTSS_APPL_APS_OPER_WARNING_WMEP_NOT_FOUND,               "wMEPNotFound"},
    {VTSS_APPL_APS_OPER_WARNING_PMEP_NOT_FOUND,               "pMEPNotFound"},
    {VTSS_APPL_APS_OPER_WARNING_WMEP_ADMIN_DISABLED,          "wMEPAdminDisabled"},
    {VTSS_APPL_APS_OPER_WARNING_PMEP_ADMIN_DISABLED,          "pMEPAdminDisabled"},
    {VTSS_APPL_APS_OPER_WARNING_WMEP_NOT_DOWN_MEP,            "wMEPNotDownMEP"},
    {VTSS_APPL_APS_OPER_WARNING_PMEP_NOT_DOWN_MEP,            "pMEPNotDownMEP"},
    {VTSS_APPL_APS_OPER_WARNING_WMEP_AND_PORT_IFINDEX_DIFFER, "wMEPDiffersFromIfindex"},
    {VTSS_APPL_APS_OPER_WARNING_PMEP_AND_PORT_IFINDEX_DIFFER, "pMEPDiffersFromIfindex"},
    {0, 0},
};

vtss_enum_descriptor_t aps_prot_state_txt[] {
    {VTSS_APPL_APS_PROT_STATE_NR_W,                         "noRequestWorking"},
    {VTSS_APPL_APS_PROT_STATE_NR_P,                         "noRequestProtect"},
    {VTSS_APPL_APS_PROT_STATE_LO,                           "lockout"},
    {VTSS_APPL_APS_PROT_STATE_FS,                           "forcedSwitch"},
    {VTSS_APPL_APS_PROT_STATE_SF_W,                         "signalFailWorking"},
    {VTSS_APPL_APS_PROT_STATE_SF_P,                         "signalFailProtect"},
    {VTSS_APPL_APS_PROT_STATE_MS_TO_P,                      "manualSwitchtoProtect"},
    {VTSS_APPL_APS_PROT_STATE_MS_TO_W,                      "manualSwitchtoWorking"},
    {VTSS_APPL_APS_PROT_STATE_WTR,                          "waitToRestore"},
    {VTSS_APPL_APS_PROT_STATE_DNR,                          "doNotRevert"},
    {VTSS_APPL_APS_PROT_STATE_EXER_W,                       "exerciseWorking"},
    {VTSS_APPL_APS_PROT_STATE_EXER_P,                       "exerciseProtect"},
    {VTSS_APPL_APS_PROT_STATE_RR_W,                         "reverseRequestWorking"},
    {VTSS_APPL_APS_PROT_STATE_RR_P,                         "reverseRequestProtect"},
    {VTSS_APPL_APS_PROT_STATE_SD_W,                         "signalDegradeWorking"},
    {VTSS_APPL_APS_PROT_STATE_SD_P,                         "signalDegradeProtect"},
    {0, 0},
};

vtss_enum_descriptor_t aps_defect_state_txt[] {
    {VTSS_APPL_APS_DEFECT_STATE_OK,                         "ok"},
    {VTSS_APPL_APS_DEFECT_STATE_SD,                         "sd"},
    {VTSS_APPL_APS_DEFECT_STATE_SF,                         "sf"},
    {0, 0},
};

vtss_enum_descriptor_t aps_request_txt[] {
    {VTSS_APPL_APS_REQUEST_NR,                              "nr"},
    {VTSS_APPL_APS_REQUEST_DNR,                             "dnr"},
    {VTSS_APPL_APS_REQUEST_RR,                              "rr"},
    {VTSS_APPL_APS_REQUEST_EXER,                            "exer"},
    {VTSS_APPL_APS_REQUEST_WTR,                             "wtr"},
    {VTSS_APPL_APS_REQUEST_MS,                              "ms"},
    {VTSS_APPL_APS_REQUEST_SD,                              "sd"},
    {VTSS_APPL_APS_REQUEST_SF_W,                            "sfW"},
    {VTSS_APPL_APS_REQUEST_FS,                              "fs"},
    {VTSS_APPL_APS_REQUEST_SF_P,                            "sfP"},
    {VTSS_APPL_APS_REQUEST_LO,                              "lo"},
    {0, 0},
};

// Wrappers
mesa_rc vtss_aps_create_conf_default(uint32_t *instance, vtss_appl_aps_conf_t *s)
{
    vtss_appl_aps_conf_t  def_conf;
    if (s) {
        VTSS_RC(vtss_appl_aps_conf_default_get(&def_conf));
        *s = def_conf;
    }
    return VTSS_RC_OK;
};

mesa_rc vtss_aps_statistics_clear(uint32_t instance, const BOOL *clear)
{
    if (clear && *clear) {
        VTSS_RC(vtss_appl_aps_statistics_clear(instance));
    }
    return VTSS_RC_OK;
};

mesa_rc vtss_aps_statistics_clear_dummy(uint32_t instance,  BOOL *clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }
    return VTSS_RC_OK;
};
