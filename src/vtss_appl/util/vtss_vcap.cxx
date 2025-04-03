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


/**
 * \file vtss_vcap.cxx
 * \brief This file defines the VCAP enum descriptor and mapping functions
 */
#include "main.h"
#include "vtss_vcap_api.h"
#include "vtss_vcap_serializer.hxx"
#include "vtss_os_wrapper_network.h" // ntohs()
#include "vtss_vcap_api.h"

/****************************************************************************
 * Enum descriptors
 ****************************************************************************/

/* The enum descriptor of vtss_appl_vcap_vlan_pri_type_t */
const vtss_enum_descriptor_t vtss_appl_vcap_vlan_pri_type_txt[] = {
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_ANY, "any"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_0,   "value0"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_1,   "value1"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_2,   "value2"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_3,   "value3"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_4,   "value4"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_5,   "value5"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_6,   "value6"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_7,   "value7"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_01,  "range0to1"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_23,  "range2to3"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_45,  "range4to5"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_67,  "range6to7"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_03,  "range0to3"},
    {VTSS_APPL_VCAP_VLAN_PRI_TYPE_47,  "range4to7"},
    {0, 0},
};

/* The enum descriptor of mesa_vcap_bit_t */
const vtss_enum_descriptor_t vtss_appl_vcap_bit_txt[] = {
    {MESA_VCAP_BIT_ANY, "any"},
    {MESA_VCAP_BIT_0,   "zero"},
    {MESA_VCAP_BIT_1,   "one"},
    {0, 0},
};

/* The enum descriptor of vtss_appl_vcap_as_type_t */
const vtss_enum_descriptor_t vtss_appl_vcap_as_type_txt[] = {
    {VTSS_APPL_VCAP_AS_TYPE_ANY,      "any"},
    {VTSS_APPL_VCAP_AS_TYPE_SPECIFIC, "specific"},
    {0, 0},
};

/* The enum descriptor of vtss_appl_vcap_asr_type_t */
const vtss_enum_descriptor_t vtss_appl_vcap_asr_type_txt[] = {
    {VTSS_APPL_VCAP_ASR_TYPE_ANY,      "any"},
    {VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC, "specific"},
    {VTSS_APPL_VCAP_ASR_TYPE_RANGE,    "range"},
    {0, 0},
};

/* The enum descriptor of vtss_appl_vcap_dmac_type_t */
const vtss_enum_descriptor_t vtss_appl_vcap_dmac_type_txt[] = {
    {VTSS_APPL_VCAP_DMAC_TYPE_ANY,       "any"},
    {VTSS_APPL_VCAP_DMAC_TYPE_UNICAST,   "unicast"},
    {VTSS_APPL_VCAP_DMAC_TYPE_MULTICAST, "multicast"},
    {VTSS_APPL_VCAP_DMAC_TYPE_BROADCAST, "broadcast"},
    {0, 0},
};

/* The enum descriptor of vtss_appl_vcap_adv_dmac_type_t */
const vtss_enum_descriptor_t vtss_appl_vcap_adv_dmac_type_txt[] = {
    {VTSS_APPL_VCAP_ADV_DMAC_TYPE_ANY,       "any"},
    {VTSS_APPL_VCAP_ADV_DMAC_TYPE_UNICAST,   "unicast"},
    {VTSS_APPL_VCAP_ADV_DMAC_TYPE_MULTICAST, "multicast"},
    {VTSS_APPL_VCAP_ADV_DMAC_TYPE_BROADCAST, "broadcast"},
    {VTSS_APPL_VCAP_ADV_DMAC_TYPE_SPECIFIC,  "specific"},
    {0, 0},
};

/* The enum descriptor of vtss_appl_vcap_vlan_tag_type_t */
const vtss_enum_descriptor_t vtss_appl_vcap_vlan_tag_type_txt[] = {
    {VTSS_APPL_VCAP_VLAN_TAG_TYPE_ANY,      "any"},
    {VTSS_APPL_VCAP_VLAN_TAG_TYPE_UNTAGGED, "untagged"},
    {VTSS_APPL_VCAP_VLAN_TAG_TYPE_TAGGED,   "tagged"},
    {VTSS_APPL_VCAP_VLAN_TAG_TYPE_C_TAGGED, "cTagged"},
    {VTSS_APPL_VCAP_VLAN_TAG_TYPE_S_TAGGED, "sTagged"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_vcap_key_type_txt[] = {
    {MESA_VCAP_KEY_TYPE_NORMAL,      "normal"},
    {MESA_VCAP_KEY_TYPE_DOUBLE_TAG,  "doubleTag"},
    {MESA_VCAP_KEY_TYPE_IP_ADDR,     "ipAddr"},
    {MESA_VCAP_KEY_TYPE_MAC_IP_ADDR, "macIpAddr"},
    {0, 0}
};

/****************************************************************************
 * Public functions
 ****************************************************************************/

/* Convert VCAP mesa_vcap_vid_t to mesa_vid_t. */
mesa_vid_t vtss_appl_vcap_vid_type2value(mesa_vcap_vid_t vcap_vid)
{
    return (mesa_vid_t) ((vcap_vid.mask & 0xFFF) ? (vcap_vid.value & 0xFFF) : 0);
}

/* Convert mesa_vid_t VCAP mesa_vcap_vid_t. */
mesa_rc vtss_appl_vcap_vid_value2type(mesa_vid_t vid_value, mesa_vcap_vid_t *const vcap_vid)
{
    /* Check illegal parameters */
    if (vcap_vid == NULL || vid_value > 4095) {
        return VTSS_RC_ERROR;
    }

    vcap_vid->value = (vid_value & 0xFFF) ? (vid_value & 0xFFF) : 0;
    vcap_vid->mask = vid_value ? 0xFFF : 0;

    return VTSS_RC_OK;
}

/* Convert VCAP mesa_vcap_u8_t (3 bits only) to vlan_pri_type. */
vtss_appl_vcap_vlan_pri_type_t vtss_appl_vcap_vlan_pri2pri_type(mesa_vcap_u8_t vcap_vlan_pri)
{
    vtss_appl_vcap_vlan_pri_type_t pri_type;

    if (vcap_vlan_pri.mask == 7) { /* 0-7 */
        pri_type = (vtss_appl_vcap_vlan_pri_type_t)(vcap_vlan_pri.value + 1);
    } else if (vcap_vlan_pri.mask == 6) { /* 0-1(9), 2-3(10), 4-5(11), 6-7(12) */
        pri_type = (vtss_appl_vcap_vlan_pri_type_t)((vcap_vlan_pri.value / 2) + 9);
    } else if (vcap_vlan_pri.mask == 4) { /* 0-3(13), 4-7(14) */
        pri_type = vcap_vlan_pri.value ? VTSS_APPL_VCAP_VLAN_PRI_TYPE_47 : VTSS_APPL_VCAP_VLAN_PRI_TYPE_03;
    } else { /* any(14) */
        pri_type = VTSS_APPL_VCAP_VLAN_PRI_TYPE_ANY;
    }

    return pri_type;
}

/* Convert vlan_pri_type to VCAP mesa_vcap_u8_t (3 bits only). */
void vtss_appl_vcap_pri_type2vlan_pri(vtss_appl_vcap_vlan_pri_type_t pri_type, mesa_vcap_u8_t *const vcap_vlan_pri)
{
    if (pri_type == VTSS_APPL_VCAP_VLAN_PRI_TYPE_ANY){ /* any(0) */
        vcap_vlan_pri->value = vcap_vlan_pri->mask = 0;
    } else if (pri_type < VTSS_APPL_VCAP_VLAN_PRI_TYPE_01) { /* 0-7 */
        vcap_vlan_pri->value = (u8)(pri_type - 1);
        vcap_vlan_pri->mask = 7;
    } else if (pri_type < VTSS_APPL_VCAP_VLAN_PRI_TYPE_03) { /* 0-1(9), 2-3(10), 4-5(11), 6-7(12) */
        vcap_vlan_pri->value = (pri_type - 9) * 2;
        vcap_vlan_pri->mask = 6;
    } else if (pri_type <= VTSS_APPL_VCAP_VLAN_PRI_TYPE_47) { /* 0-3(13), 4-7(14) */
        vcap_vlan_pri->value = pri_type == VTSS_APPL_VCAP_VLAN_PRI_TYPE_03 ? 0 : 4;
        vcap_vlan_pri->mask = 4;
    }
}

vtss_appl_vcap_vlan_tag_type_t vtss_appl_vcap_tag_bits2tag_type(mesa_vcap_bit_t tagged, mesa_vcap_bit_t s_tag)
{
    switch (tagged) {
    case MESA_VCAP_BIT_0:
        return VTSS_APPL_VCAP_VLAN_TAG_TYPE_UNTAGGED;
    case MESA_VCAP_BIT_1:
        switch (s_tag) {
        case MESA_VCAP_BIT_0:
            return VTSS_APPL_VCAP_VLAN_TAG_TYPE_C_TAGGED;
        case MESA_VCAP_BIT_1:
            return VTSS_APPL_VCAP_VLAN_TAG_TYPE_S_TAGGED;
        default:
            return VTSS_APPL_VCAP_VLAN_TAG_TYPE_TAGGED;
        }
    default:
        return VTSS_APPL_VCAP_VLAN_TAG_TYPE_ANY;
    }
}

void vtss_appl_vcap_tag_type2tag_bits(vtss_appl_vcap_vlan_tag_type_t tag_type, mesa_vcap_bit_t *tagged, mesa_vcap_bit_t *s_tag)
{
    switch (tag_type) {
    case VTSS_APPL_VCAP_VLAN_TAG_TYPE_UNTAGGED:
        *tagged = MESA_VCAP_BIT_0;
        *s_tag = MESA_VCAP_BIT_ANY;
        break;
    case VTSS_APPL_VCAP_VLAN_TAG_TYPE_TAGGED:
        *tagged = MESA_VCAP_BIT_1;
        *s_tag = MESA_VCAP_BIT_ANY;
        break;
    case VTSS_APPL_VCAP_VLAN_TAG_TYPE_C_TAGGED:
        *tagged = MESA_VCAP_BIT_1;
        *s_tag = MESA_VCAP_BIT_0;
        break;
    case VTSS_APPL_VCAP_VLAN_TAG_TYPE_S_TAGGED:
        *tagged = MESA_VCAP_BIT_1;
        *s_tag = MESA_VCAP_BIT_1;
        break;
    default:
        *tagged = *s_tag = MESA_VCAP_BIT_ANY;
        break;
    }
}

void vtss_appl_vcap_vr2asr(const mesa_vcap_vr_t *vr, vtss_appl_vcap_asr_t *asr)
{
    if (vr->type != MESA_VCAP_VR_TYPE_VALUE_MASK) {
        asr->match = VTSS_APPL_VCAP_ASR_TYPE_RANGE;
        asr->low = vr->vr.r.low;
        asr->high = vr->vr.r.high;
    } else if (vr->vr.v.mask) {
        asr->match = VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC;
        asr->low = vr->vr.v.value;
        asr->high = 0;
    } else {
        asr->match = VTSS_APPL_VCAP_ASR_TYPE_ANY;
        asr->low = asr->high = 0;
    }
}

mesa_rc vtss_appl_vcap_asr2vr(const vtss_appl_vcap_asr_t *asr, mesa_vcap_vr_t *vr, u16 mask)
{
    switch (asr->match) {
    case VTSS_APPL_VCAP_ASR_TYPE_ANY:
        vr->type = MESA_VCAP_VR_TYPE_VALUE_MASK;
        vr->vr.v.value = vr->vr.v.mask = 0;
        break;
    case VTSS_APPL_VCAP_ASR_TYPE_SPECIFIC:
        if (asr->low > mask) {
            return VTSS_RC_ERROR;
        }
        vr->type = MESA_VCAP_VR_TYPE_VALUE_MASK;
        vr->vr.v.value = asr->low;
        vr->vr.v.mask = mask;
        break;
    case VTSS_APPL_VCAP_ASR_TYPE_RANGE:
        if ((asr->low > asr->high) || (asr->high > mask)) {
            return VTSS_RC_ERROR;
        }
        vr->type = MESA_VCAP_VR_TYPE_RANGE_INCLUSIVE;
        vr->vr.r.low = asr->low;
        vr->vr.r.high = asr->high;
        break;
    default:
        return VTSS_RC_ERROR;
        break;
    }
    return VTSS_RC_OK;
}

void vtss_appl_vcap_vr2range(const mesa_vcap_vr_t *vr, vtss_appl_vcap_range_t *range, u16 max)
{
    if (vr->type == MESA_VCAP_VR_TYPE_VALUE_MASK) {
        /* Match any */
        *range = (max << 16);
    } else {
        /* Match range or specific value */
        *range = ((vr->vr.r.high << 16) + vr->vr.r.low);
    }
}

mesa_rc vtss_appl_vcap_range2vr(const vtss_appl_vcap_range_t *range, mesa_vcap_vr_t *vr, u16 max, BOOL no_range)
{
    mesa_rc rc = VTSS_RC_OK;
    u32     low, high, mask;

    low = (*range & 0xffff);
    high = ((*range >> 16) & 0xffff);
    if (low > high || high > max) {
        /* Illegal range */
        rc = VTSS_RC_ERROR;
    } else if (low == 0 && high == max) {
        /* Match any */
        vr->type = MESA_VCAP_VR_TYPE_VALUE_MASK;
        vr->vr.v.value = 0;
        vr->vr.v.mask = 0;
    } else {
        /* Match range, check if legal range */
        vr->type = MESA_VCAP_VR_TYPE_RANGE_INCLUSIVE;
        vr->vr.r.low = low;
        vr->vr.r.high = high;
        if (no_range) {
	    rc = VTSS_RC_ERROR;
	}
        for (mask = 0; mask <= max; mask = (mask * 2 + 1)) {
            if ((low & ~mask) == (high & ~mask) && /* Upper bits match */
                (low & mask) == 0 &&               /* Lower bits of 'low' are zero */
                (high | mask) == high) {           /* Lower bits of 'high are one */
	        rc = VTSS_RC_OK;
                break;
            }
        }
    }
    return rc;
}

/* Convert VCAP mesa_vcap_u16_t to mesa_etype_t. */
mesa_etype_t vtss_appl_vcap_etype_type2value(mesa_vcap_u16_t vcap_etype)
{
    mesa_etype_t temp;

    memcpy(&temp, vcap_etype.value, sizeof(temp));
    temp = ntohs(temp);

    return ((vcap_etype.mask[0] == 0 && vcap_etype.mask[1] == 0) ? 0 : temp);
}

/* Convert mesa_etype_t VCAP mesa_vcap_u16_t */
mesa_rc vtss_appl_vcap_etype_value2type(mesa_etype_t etype_value, mesa_vcap_u16_t *const vcap_etype)
{
    /* Check illegal parameters */
    if (vcap_etype == NULL) {
        return VTSS_RC_ERROR;
    }
    if (etype_value &&
        (etype_value < 0x600 || etype_value == 0x0800 || etype_value == 0x0806 || etype_value == 0x086DD)) {
        return VTSS_RC_ERROR;
    }

    if (etype_value) {
        mesa_etype_t temp = ntohs(etype_value);
        memcpy(vcap_etype->value, &temp, sizeof(temp));
        vcap_etype->mask[0] =  vcap_etype->mask[1] = 0xFF;
    } else {
        memset(vcap_etype, 0, sizeof(*vcap_etype));
    }

    return VTSS_RC_OK;
}
