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

#include "main.h"
#include "qos_api.h"
#include "mgmt_api.h"
#include "misc_api.h"

const vtss_enum_descriptor_t vtss_appl_qos_wred_max_txt[] = {
    {VTSS_APPL_QOS_WRED_MAX_DP, "maximumDropProbability"},
    {VTSS_APPL_QOS_WRED_MAX_FL, "maximumFillLevel"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_qos_tag_remark_mode_txt[] = {
    {VTSS_APPL_QOS_TAG_REMARK_MODE_CLASSIFIED, "classified"},
    {VTSS_APPL_QOS_TAG_REMARK_MODE_DEFAULT,    "default"},
    {VTSS_APPL_QOS_TAG_REMARK_MODE_MAPPED,     "mapped"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_qos_dscp_mode_txt[] = {
    {VTSS_APPL_QOS_DSCP_MODE_NONE, "none"},
    {VTSS_APPL_QOS_DSCP_MODE_ZERO, "zero"},
    {VTSS_APPL_QOS_DSCP_MODE_SEL,  "selected"},
    {VTSS_APPL_QOS_DSCP_MODE_ALL,  "all"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_qos_dscp_emode_txt[] = {
    {VTSS_APPL_QOS_DSCP_EMODE_DISABLE,   "disabled"},
    {VTSS_APPL_QOS_DSCP_EMODE_REMARK,    "rewrite"},
    {VTSS_APPL_QOS_DSCP_EMODE_REMAP,     "remap"},
    {VTSS_APPL_QOS_DSCP_EMODE_REMAP_DPA, "remapDp"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_qos_qcl_user_txt[] = {
    {VTSS_APPL_QOS_QCL_USER_STATIC,      "admin"},
    {VTSS_APPL_QOS_QCL_USER_VOICE_VLAN,  "voiceVlan"},
    {VTSS_APPL_QOS_QCL_USER_DHCP6_SNOOP, "dhcp6Snoop"},
    {VTSS_APPL_QOS_QCL_USER_IPV6_SOURCE_GUARD, "ipv6SourceGuard"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_qos_qce_type_txt[] = {
    {VTSS_APPL_QOS_QCE_TYPE_ANY,   "any"},
    {VTSS_APPL_QOS_QCE_TYPE_ETYPE, "etype"},
    {VTSS_APPL_QOS_QCE_TYPE_LLC,   "llc"},
    {VTSS_APPL_QOS_QCE_TYPE_SNAP,  "snap"},
    {VTSS_APPL_QOS_QCE_TYPE_IPV4,  "ipv4"},
    {VTSS_APPL_QOS_QCE_TYPE_IPV6,  "ipv6"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_qos_shaper_mode_txt[] = {
    {VTSS_APPL_QOS_SHAPER_MODE_LINE, "line"},
    {VTSS_APPL_QOS_SHAPER_MODE_DATA, "data"},
    {VTSS_APPL_QOS_SHAPER_MODE_FRAME, "frame"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_qos_ingress_map_key_txt[] = {
    {VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP,              "pcp"},
    {VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP_DEI,          "pcpDei"},
    {VTSS_APPL_QOS_INGRESS_MAP_KEY_DSCP,             "dscp"},
    {VTSS_APPL_QOS_INGRESS_MAP_KEY_DSCP_PCP_DEI,     "dscpPcpDei"},
    {VTSS_APPL_QOS_INGRESS_MAP_KEY_DSCP_PCP_DEI + 1, "tcObsolete"},
    {0, 0}
};

const vtss_enum_descriptor_t vtss_appl_qos_egress_map_key_txt[] = {
    {VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID,        "cosid"},
    {VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID_DPL,    "cosidDpl"},
    {VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP,         "dscp"},
    {VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP_DPL,     "dscpDpl"},
    {VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP_DPL + 1, "tcOsolete"},
    {VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP_DPL + 2, "tcDplObsolete"},
    {0, 0}
};

/* QOS error text */
const char *vtss_appl_qos_error_txt(mesa_rc rc)
{
    switch (rc) {
    case VTSS_APPL_QOS_ERROR_GEN:
        return "QOS generic error";
    case VTSS_APPL_QOS_ERROR_PARM:
        return "QOS parameter error";
    case VTSS_APPL_QOS_ERROR_FEATURE:
        return "QOS feature not present";
    case VTSS_APPL_QOS_ERROR_QCE_NOT_FOUND:
        return "QCE not found";
    case VTSS_APPL_QOS_ERROR_QCE_TABLE_FULL:
        return "QCE is full";
    case VTSS_APPL_QOS_ERROR_QCL_USER_NOT_FOUND:
        return "QoS QCL User not found";
    case VTSS_APPL_QOS_ERROR_STACK_STATE:
        return "QoS stack state error";
    case VTSS_APPL_QOS_ERROR_UNHANDLED_QUEUES:
        return "All queues must be open during cycle";
    case VTSS_APPL_QOS_ERROR_INVALID_CYCLE_TIME:
        return "CycleTime less than sum of TimeIntervals";
    case VTSS_APPL_QOS_ERROR_SFI_SAME_PORT_OR_DIFFERENT_PRIO:
        return "Ports must be non wild card and different and priorities must be equal in SFIs that references the same SID";
    case VTSS_APPL_QOS_ERROR_SID_IN_USE_OR_INVALID_OR_TOO_MANY:
        return "SID or SID + 1 is in use, invalid or has too many references.";
    default:
        return "QOS unknown error";
    }
}

static const char *const qos_dscp_names[64] = {
    "0  (BE)",  "1",  "2",         "3",  "4",         "5",  "6",         "7",
    "8  (CS1)", "9",  "10 (AF11)", "11", "12 (AF12)", "13", "14 (AF13)", "15",
    "16 (CS2)", "17", "18 (AF21)", "19", "20 (AF22)", "21", "22 (AF23)", "23",
    "24 (CS3)", "25", "26 (AF31)", "27", "28 (AF32)", "29", "30 (AF33)", "31",
    "32 (CS4)", "33", "34 (AF41)", "35", "36 (AF42)", "37", "38 (AF43)", "39",
    "40 (CS5)", "41", "42",        "43", "44",        "45", "46 (EF)",   "47",
    "48 (CS6)", "49", "50",        "51", "52",        "53", "54",        "55",
    "56 (CS7)", "57", "58",        "59", "60",        "61", "62",        "63"
};

const char *vtss_appl_qos_dscp2str(mesa_dscp_t dscp)
{
    return (dscp > 63) ? "?" : qos_dscp_names[dscp];
}

u32 vtss_appl_qos_rate_min(u32 r1, u32 r2)
{
    u32 min = 0xffffffff;

    if (r1) {
        min = r1;
    }
    if (r2) {
        min = MIN(min, r2);
    }
    return min;
}

u32 vtss_appl_qos_rate_max(u32 r1, u32 r2)
{
    u32 max = 0;

    if (r1) {
        max = r1;
    }
    if (r2) {
        max = MAX(max, r2);
    }
    return max;
}

const char *vtss_appl_qos_rate2txt(u32 rate, BOOL frame_rate, char *buf)
{
    if (rate >= 1000 && (rate % 1000) == 0) {
        rate /= 1000;
        sprintf(buf, "%u %s", rate, (frame_rate) ? "kfps" : "mbps");
    } else {
        sprintf(buf, "%u %s", rate, (frame_rate) ? "fps"  : "kbps");
    }
    return buf;
}

const char *vtss_appl_qos_ingress_map_key2txt(vtss_appl_qos_ingress_map_key_t key, BOOL upper)
{
    switch (key) {
    case VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP:
        return (upper) ? "PCP" : "pcp";
    case VTSS_APPL_QOS_INGRESS_MAP_KEY_PCP_DEI:
        return (upper) ? "PCP and DEI" : "pcp-dei";
    case VTSS_APPL_QOS_INGRESS_MAP_KEY_DSCP:
        return (upper) ? "DSCP" : "dscp";
    case VTSS_APPL_QOS_INGRESS_MAP_KEY_DSCP_PCP_DEI:
        return (upper) ? "DSCP, PCP and DEI" : "dscp-pcp-dei";
    default:
        return (upper) ? "Invalid" : "invalid";
    }
}

const char *vtss_appl_qos_egress_map_key2txt(vtss_appl_qos_egress_map_key_t key, BOOL upper)
{
    switch (key) {
    case VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID:
        return (upper) ? "CLASS" : "class";
    case VTSS_APPL_QOS_EGRESS_MAP_KEY_COSID_DPL:
        return (upper) ? "CLASS and DPL" : "class-dpl";
    case VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP:
        return (upper) ? "DSCP" : "dscp";
    case VTSS_APPL_QOS_EGRESS_MAP_KEY_DSCP_DPL:
        return (upper) ? "DSCP and DPL" : "dscp-dpl";
    default:
        return (upper) ? "Invalid" : "invalid";
    }
}

const char *vtss_appl_qos_shaper_mode2txt(vtss_appl_qos_shaper_mode_t mode, char *buf)
{
    switch (mode) {
    case VTSS_APPL_QOS_SHAPER_MODE_LINE:
        sprintf(buf, "line-rate");
        break;
    case VTSS_APPL_QOS_SHAPER_MODE_DATA:
        sprintf(buf, "data-rate");
        break;
    case VTSS_APPL_QOS_SHAPER_MODE_FRAME:
        sprintf(buf, "frame-rate");
        break;
    default:
        sprintf(buf, "invalid rate");
        break;
    }
    return buf;
}

const char *vtss_appl_qos_qcl_key_type2txt(mesa_vcap_key_type_t key_type, BOOL upper)
{
    switch (key_type) {
    case MESA_VCAP_KEY_TYPE_NORMAL:
        return (upper) ? "Normal" : "normal";
    case MESA_VCAP_KEY_TYPE_DOUBLE_TAG:
        return (upper) ? "Double Tag" : "double-tag";
    case MESA_VCAP_KEY_TYPE_IP_ADDR:
        return (upper) ? "IP Address" : "ip-addr";
    case MESA_VCAP_KEY_TYPE_MAC_IP_ADDR:
        return (upper) ? "MAC and IP Address" : "mac-ip-addr";
    default:
        return (upper) ? "Invalid" : "invalid";
    }
}

const char *vtss_appl_qos_qcl_user2txt(vtss_appl_qos_qcl_user_t user, BOOL upper)
{
    switch (user) {
    case VTSS_APPL_QOS_QCL_USER_STATIC:
        return (upper) ? "Static" : "static";
#ifdef VTSS_SW_OPTION_VOICE_VLAN
    case VTSS_APPL_QOS_QCL_USER_VOICE_VLAN:
        return (upper) ? "Voice VLAN" : "voice vlan";
#endif

#ifdef VTSS_SW_OPTION_DHCP6_SNOOPING
    case VTSS_APPL_QOS_QCL_USER_DHCP6_SNOOP:
        return (upper) ? "DHCP6 Snooping" : "dhcp6 snooping";
#endif

#ifdef VTSS_SW_OPTION_IPV6_SOURCE_GUARD
    case VTSS_APPL_QOS_QCL_USER_IPV6_SOURCE_GUARD:
        return (upper) ? "Ipv6 Source Guard" : "ipv6 source guard";
#endif

    default:
        return (upper) ? "Unknown User" : "unknown user";
    }
}

const char *vtss_appl_qos_qcl_tag_type2txt(mesa_vcap_bit_t tagged, mesa_vcap_bit_t s_tag, BOOL upper)
{
    switch (tagged) {
    case MESA_VCAP_BIT_0:
        return (upper) ? "Untagged" : "untagged";
    case MESA_VCAP_BIT_1:
        switch (s_tag) {
        case MESA_VCAP_BIT_0:
            return (upper) ? "C-Tagged" : "c-tagged";
        case MESA_VCAP_BIT_1:
            return (upper) ? "S-Tagged" : "s-tagged";
        default:
            return (upper) ? "Tagged" : "tagged";
        }
    default:
        return (upper) ? "Any" : "any";
    }
}

const char *vtss_appl_qos_qcl_port_list2txt(mesa_port_list_t &port_list, char *buf, BOOL upper)
{
    char *result;

    result = mgmt_iport_list2txt(port_list, buf);
    if (strlen(result) == 0) {
        strcpy(buf, (upper) ? "None" : "none");
    }
    return buf;
}

const char *vtss_appl_qos_qcl_dmactype2txt(mesa_vcap_bit_t bc, mesa_vcap_bit_t mc, BOOL upper)
{
    if (bc == MESA_VCAP_BIT_1) {
        return (upper) ? "Broadcast" : "broadcast";
    } else if (mc == MESA_VCAP_BIT_1) {
        return (upper) ? "Multicast" : "multicast";
    } else if (mc == MESA_VCAP_BIT_0) {
        return (upper) ? "Unicast" : "unicast";
    } else {
        return (upper) ? "Any" : "any";
    }
}

const char *vtss_appl_qos_qcl_proto2txt(mesa_vcap_u8_t *proto, char *buf)
{
    if (proto->mask) {
        if (proto->value == 6) {
            sprintf(buf, "tcp");
        } else if (proto->value == 17) {
            sprintf(buf, "udp");
        } else {
            sprintf(buf, "%u", proto->value);
        }
    } else {
        sprintf(buf, "any");
    }
    return (buf);
}

/* Convert an ipv4 address to the 32 LSB in an ipv6 address */
void vtss_appl_qos_qcl_ipv42ipv6(mesa_vcap_ip_t *ipv4, mesa_vcap_u128_t *ipv6)
{
    u32 i, j;

    for (i = 0; i < 4; i++) {
        j = ((3 - i) * 8);
        ipv6->value[i + 12] = (ipv4->value >> j) & 0xFF;
        ipv6->mask[i + 12] = (ipv4->mask >> j) & 0xFF;
    }
}

/* Convert the 32 LSB in an ipv6 address to an ipv4 address */
void vtss_appl_qos_qcl_ipv62ipv4(mesa_vcap_u128_t *ipv6, mesa_vcap_ip_t *ipv4)
{
    u32 i, j;

    ipv4->value = 0;
    ipv4->mask = 0;
    for (i = 0; i < 4; i++) {
        j = ((3 - i) * 8);
        ipv4->value += (ipv6->value[i + 12] << j);
        ipv4->mask += (ipv6->mask[i + 12] << j);
    }
}

const char *vtss_appl_qos_qcl_ipv42txt(mesa_vcap_ip_t *ipv4, char *buf, BOOL upper)
{
    u32 i, n = 0;

    if (ipv4->mask == 0) {
        strcpy(buf, (upper) ? "Any" : "any");
    } else {
        for (i = 0; i < 32; i++) {
            if (ipv4->mask & (1 << i)) {
                n++;
            }
        }
        (void)misc_ipv4_txt(ipv4->value, buf);
        sprintf(&buf[strlen(buf)], "/%d", n);
    }
    return buf;
}

const char *vtss_appl_qos_qcl_ipv62txt(mesa_vcap_u128_t *ipv6, char *buf, BOOL upper)
{
    /*
     * Bug APPL-1244: QCE rules support full IPv6 addresses since they can use a full key
     */
    const int acount = sizeof(ipv6->value);
    int prefix_size = 0;
    bool mask_non_zero = false;
    mesa_ipv6_t ipv6_val;

    // loop through address bytes
    for (int i = 0; i < acount; i++) {
        // collect prefix size
        for (int j = 0; j < 8; j++) {
            if (ipv6->mask[i] & (1 << j)) {
                prefix_size++;
            }
        }

        // check if mask is non-zero
        if (ipv6->mask[i] != 0) {
            mask_non_zero = true;
        }

        // only collect masked part of address byte
        ipv6_val.addr[i] = ipv6->value[i] & ipv6->mask[i];
    }

    if (mask_non_zero) {
        misc_ipv6_txt(&ipv6_val, buf);
        sprintf(buf + strlen(buf), "/%d", prefix_size);

    } else {
        strcpy(buf, (upper) ? "Any" : "any");
    }

    return buf;
}

const char *vtss_appl_qos_qcl_range2txt(mesa_vcap_vr_t *range, char *buf, BOOL upper)
{
    if (range->type != MESA_VCAP_VR_TYPE_VALUE_MASK) {
        sprintf(buf, "%u-%u", range->vr.r.low, range->vr.r.high);
    } else if (range->vr.v.mask) {
        sprintf(buf, "%u", range->vr.v.value);
    } else {
        sprintf(buf, "%s", (upper) ? "Any" : "any");
    }
    return (buf);
}

// if min, max and mask is 0 it's 'any'
// if min >= max it's specific value/mask else it's range
void vtss_appl_qos_qcl_range_set(mesa_vcap_vr_t *dest, u16 min, u16 max, u16 mask)
{
    if (!min && !max && !mask) {
        dest->type = MESA_VCAP_VR_TYPE_VALUE_MASK;
        dest->vr.v.value = 0;
        dest->vr.v.mask = 0;
    } else if (min >= max) {
        dest->type = MESA_VCAP_VR_TYPE_VALUE_MASK;
        dest->vr.v.value = min;
        dest->vr.v.mask = mask;
    } else {
        dest->type = MESA_VCAP_VR_TYPE_RANGE_INCLUSIVE;
        dest->vr.r.low = min;
        dest->vr.r.high = max;
    }
}

