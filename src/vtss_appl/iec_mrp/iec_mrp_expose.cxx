/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "iec_mrp_serializer.hxx"
#include <vtss/appl/iec_mrp.h>

vtss_enum_descriptor_t vtss_appl_iec_mrp_oper_state_txt[] {
    {VTSS_APPL_IEC_MRP_OPER_STATE_ADMIN_DISABLED, "disabled"},
    {VTSS_APPL_IEC_MRP_OPER_STATE_ACTIVE,         "active"},
    {VTSS_APPL_IEC_MRP_OPER_STATE_INTERNAL_ERROR, "internalError"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_iec_mrp_recovery_profile_txt[]  {
    {VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_10MS,  "ms10"},
    {VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_30MS,  "ms30"},
    {VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_200MS, "ms200"},
    {VTSS_APPL_IEC_MRP_RECOVERY_PROFILE_500MS, "ms500"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_iec_mrp_role_txt[] {
    {VTSS_APPL_IEC_MRP_ROLE_MRC, "mrc"},
    {VTSS_APPL_IEC_MRP_ROLE_MRM, "mrm"},
    {VTSS_APPL_IEC_MRP_ROLE_MRA, "mra"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_iec_mrp_in_role_txt[] {
    {VTSS_APPL_IEC_MRP_IN_ROLE_NONE,  "none"},
    {VTSS_APPL_IEC_MRP_IN_ROLE_MIC,   "mic"},
    {VTSS_APPL_IEC_MRP_IN_ROLE_MIM,  "mim"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_iec_mrp_port_type_txt[] {
    {VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT1,      "port1"},
    {VTSS_APPL_IEC_MRP_PORT_TYPE_RING_PORT2,      "port2"},
    {VTSS_APPL_IEC_MRP_PORT_TYPE_INTERCONNECTION, "interconnection"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_iec_mrp_sf_trigger_txt[] {
    {VTSS_APPL_IEC_MRP_SF_TRIGGER_LINK, "link"},
    {VTSS_APPL_IEC_MRP_SF_TRIGGER_MEP,  "mep"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_iec_mrp_ring_state_txt[] {
    {VTSS_APPL_IEC_MRP_RING_STATE_DISABLED,  "disabled"},
    {VTSS_APPL_IEC_MRP_RING_STATE_OPEN,      "open"},
    {VTSS_APPL_IEC_MRP_RING_STATE_CLOSED,    "closed"},
    {VTSS_APPL_IEC_MRP_RING_STATE_UNDEFINED, "undefined"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_iec_mrp_in_mode_txt[] {
    {VTSS_APPL_IEC_MRP_IN_MODE_LC, "linkCheck"},
    {VTSS_APPL_IEC_MRP_IN_MODE_RC, "ringCheck"},
    {0, 0},
};

vtss_enum_descriptor_t vtss_appl_iec_mrp_oui_type_txt[] {
    {VTSS_APPL_IEC_MRP_OUI_TYPE_DEFAULT, "default"},
    {VTSS_APPL_IEC_MRP_OUI_TYPE_SIEMENS, "siemens"},
    {VTSS_APPL_IEC_MRP_OUI_TYPE_CUSTOM,  "custom"},
    {0, 0}
};

// Wrappers
mesa_rc vtss_iec_mrp_create_conf_default(uint32_t *instance, vtss_appl_iec_mrp_conf_t *s)
{
    vtss_appl_iec_mrp_conf_t  def_conf;
    if (s) {
        VTSS_RC(vtss_appl_iec_mrp_conf_default_get(&def_conf));
        *s = def_conf;
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_iec_mrp_statistics_clear(uint32_t instance, const BOOL *clear)
{
    if (clear && *clear) {
        VTSS_RC(vtss_appl_iec_mrp_statistics_clear(instance));
    }
    return VTSS_RC_OK;
}

mesa_rc vtss_iec_mrp_statistics_clear_dummy(uint32_t instance,  BOOL *clear)
{
    if (clear && *clear) {
        *clear = FALSE;
    }
    return VTSS_RC_OK;
}

