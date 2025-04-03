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

#include "poe_serializer.hxx"

const vtss_enum_descriptor_t vtss_appl_poe_management_mode_txt[] = {
    {VTSS_APPL_POE_CLASS_RESERVED,       "classReservedPower"},
    {VTSS_APPL_POE_CLASS_CONSUMP,        "classConsumption"},
    {VTSS_APPL_POE_ALLOCATED_RESERVED,   "allocatedReservedPower"},
    {VTSS_APPL_POE_ALLOCATED_CONSUMP,    "allocatedConsumption"},
    {VTSS_APPL_POE_LLDPMED_RESERVED,     "lldpReservedPower"},
    {VTSS_APPL_POE_LLDPMED_CONSUMP,      "lldpConsumption"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_poebt_port_type_txt[] = {
    {VTSS_APPL_POE_PSE_PORT_TYPE3_15W,        "type3pwr15w"},
    {VTSS_APPL_POE_PSE_PORT_TYPE3_30W,        "type3pwr30w"},
    {VTSS_APPL_POE_PSE_PORT_TYPE3_60W,        "type3pwr60w"},
    {VTSS_APPL_POE_PSE_PORT_TYPE4_90W,        "type4pwr90w"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_poebt_port_mode_txt[] = {
    {VTSS_APPL_POE_MODE_DISABLED,        "disable"},
    {VTSS_APPL_POE_MODE_POE,             "standard"},
    {VTSS_APPL_POE_MODE_POE_PLUS,        "plus"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_poebt_port_pm_txt[] = {
    {VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC,       "dynamic"},
    {VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_STATIC,        "static"},
    {VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_HYBRID,        "hybrid"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_poe_port_power_priority_txt[] = {
    {VTSS_APPL_POE_PORT_POWER_PRIORITY_LOW,         "low"},
    {VTSS_APPL_POE_PORT_POWER_PRIORITY_HIGH,        "high"},
    {VTSS_APPL_POE_PORT_POWER_PRIORITY_CRITICAL,    "critical"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_poe_port_cable_length_txt[] = {
    {VTSS_APPL_POE_PORT_CABLE_LENGTH_10,     "max10"},
    {VTSS_APPL_POE_PORT_CABLE_LENGTH_30,     "max30"},
    {VTSS_APPL_POE_PORT_CABLE_LENGTH_60,     "max60"},
    {VTSS_APPL_POE_PORT_CABLE_LENGTH_100,    "max100"},
    {0, 0},
};

extern const vtss_enum_descriptor_t vtss_appl_poe_port_lldp_disable_txt[] = {
    {VTSS_APPL_POE_PORT_LLDP_ENABLED,        "enable"},
    {VTSS_APPL_POE_PORT_LLDP_DISABLED,       "disable"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_poe_status_txt[] = {
    {VTSS_APPL_POE_UNKNOWN_STATE,               "unknownState"  },
    {VTSS_APPL_POE_POWER_BUDGET_EXCEEDED,       "budgetExceeded"},
    {VTSS_APPL_POE_NO_PD_DETECTED,              "noPdDetected"  },
    {VTSS_APPL_POE_PD_ON,                       "pdOn"          },
    {VTSS_APPL_POE_PD_OVERLOAD,                 "pdOverloaded"  },
    {VTSS_APPL_POE_NOT_SUPPORTED,               "notSupported"  },
    {VTSS_APPL_POE_DISABLED,                    "disabled"      },
    {VTSS_APPL_POE_DISABLED_INTERFACE_SHUTDOWN, "shutdown"      },
    {VTSS_APPL_POE_PD_FAULT,                    "pdFault"       },
    {VTSS_APPL_POE_PSE_FAULT,                   "pseFault"      },
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_poe_led_txt[] = {
    {VTSS_APPL_POE_LED_NULL,                             "Unknown"},
    {VTSS_APPL_POE_LED_OFF,                              "Off"},
    {VTSS_APPL_POE_LED_GREEN,                            "Green"},
    {VTSS_APPL_POE_LED_RED,                              "Red"},
    {VTSS_APPL_POE_LED_BLINK_RED,                        "BlinkRed"},
    {0, 0},
};

extern const vtss_enum_descriptor_t vtss_appl_poe_pd_structure_txt[] = {
    {VTSS_APPL_POE_PD_STRUCTURE_NOT_PERFORMED,               "notDetected"},
    {VTSS_APPL_POE_PD_STRUCTURE_OPEN,                        "open"},
    {VTSS_APPL_POE_PD_STRUCTURE_INVALID_SIGNATURE,           "invalidSignature"},
    {VTSS_APPL_POE_PD_STRUCTURE_4P_SINGLE_IEEE,              "ieee4PairSingleSig"},
    {VTSS_APPL_POE_PD_STRUCTURE_4P_SINGLE_LEGACY,            "legacy4PairSingleSig"},
    {VTSS_APPL_POE_PD_STRUCTURE_4P_DUAL_IEEE,                "ieee4PairDualSig"},
    {VTSS_APPL_POE_PD_STRUCTURE_2P_DUAL_4P_CANDIDATE_FALSE,  "p2p4CandidateFalse"},
    {VTSS_APPL_POE_PD_STRUCTURE_2P_IEEE,                     "ieee2Pair"},
    {VTSS_APPL_POE_PD_STRUCTURE_2P_LEGACY,                   "legacy2Pair"},
    {0, 0},
};
