/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "mstp_serializer.hxx"
#include "vtss/appl/mstp.h"
#include "vtss/appl/vlan.h"  // VTSS_APPL_VLAN_ID_MAX

vtss_enum_descriptor_t mstp_forceVersion_txt[] {
    {VTSS_APPL_MSTP_PROTOCOL_VERSION_COMPAT, "stp"},
    {VTSS_APPL_MSTP_PROTOCOL_VERSION_RSTP,   "rstp"},
    {VTSS_APPL_MSTP_PROTOCOL_VERSION_MSTP,   "mstp"},
    {0, 0},
};

vtss_enum_descriptor_t mstp_portstate_txt[] {
    {VTSS_APPL_MSTP_PORTSTATE_DISABLED,   "disabled"},
    {VTSS_APPL_MSTP_PORTSTATE_DISCARDING, "discarding"},
    {VTSS_APPL_MSTP_PORTSTATE_LEARNING,   "learning"},
    {VTSS_APPL_MSTP_PORTSTATE_FORWARDING, "forwarding"},
    {0, 0},
};

vtss_enum_descriptor_t mstp_p2p_txt[] {
    {VTSS_APPL_MSTP_P2P_FORCETRUE,  "forceTrue"},
    {VTSS_APPL_MSTP_P2P_FORCEFALSE, "forceFalse"},
    {VTSS_APPL_MSTP_P2P_AUTO,       "auto"},
    {0, 0},
};

mesa_rc mstp_msti_itr(const vtss_appl_mstp_msti_t *prev_msti, vtss_appl_mstp_msti_t *next_msti)
{
    if (prev_msti == NULL) {
        *next_msti = VTSS_MSTI_CIST;
        return VTSS_RC_OK;
    }

    if (*prev_msti >= (VTSS_APPL_MSTP_MAX_MSTI - 1)) {
        return VTSS_RC_ERROR;
    }

    *next_msti = *prev_msti + 1;
    return VTSS_RC_OK;
}

mesa_rc mstp_msti_port_exist_itr(const vtss_ifindex_t  *prev_ifindex, vtss_ifindex_t *next_ifindex, const vtss_appl_mstp_msti_t *prev_msti, vtss_appl_mstp_msti_t *next_msti)
{
    mesa_rc rc = VTSS_RC_OK;

    if (prev_ifindex == NULL) {
        // Get first port
        rc = vtss_appl_iterator_ifindex_front_port_exist(prev_ifindex, next_ifindex);
        *next_msti = VTSS_MSTI_CIST;
    } else {
        // Have previous port, get next MSTI
        if (prev_msti == NULL) {
            *next_msti = VTSS_MSTI_CIST;
            *next_ifindex = *prev_ifindex;
        } else {
            if (*prev_msti >= (VTSS_APPL_MSTP_MAX_MSTI - 1)) {
                *next_msti = VTSS_MSTI_CIST;
                rc = vtss_appl_iterator_ifindex_front_port_exist(prev_ifindex, next_ifindex);
            } else {
                *next_msti = *prev_msti + 1;
                *next_ifindex = *prev_ifindex;
            }
        }
    }

    return rc;
}

static vtss_appl_mstp_msti_config_t the_mstp_msti_config;

mesa_rc vtss_appl_mstp_msti_config_name_and_rev_get(vtss_appl_mstp_msti_config_name_and_rev_t *conf)
{
    mesa_rc rc = vtss_appl_mstp_msti_config_get(&the_mstp_msti_config, nullptr);

    if (rc != VTSS_RC_OK) {
        return rc;
    }

    memcpy(conf->configname, the_mstp_msti_config.configname, VTSS_APPL_MSTP_CONFIG_NAME_MAXLEN);
    conf->revision = the_mstp_msti_config.revision;
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mstp_msti_config_name_and_rev_set(const vtss_appl_mstp_msti_config_name_and_rev_t *conf)
{
    mesa_rc rc = vtss_appl_mstp_msti_config_get(&the_mstp_msti_config, nullptr);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    memcpy(the_mstp_msti_config.configname, conf->configname, VTSS_APPL_MSTP_CONFIG_NAME_MAXLEN);
    the_mstp_msti_config.revision = conf->revision;
    return vtss_appl_mstp_msti_config_set(&the_mstp_msti_config);
}

/**
 * msti config table with vlan id as index (iteration key)
 */
mesa_rc vtss_appl_mstp_msti_config_get_table(mesa_vid_t vid, vtss_appl_mstp_vlan_msti_config *conf)
{
    u8 cfg_digest[VTSS_APPL_MSTP_DIGEST_LEN];

    if (vid == 0 || vid >= VTSS_APPL_MSTP_MAX_VID - 1) {
        return VTSS_RC_ERROR;
    }

    vtss_appl_mstp_msti_config_get(&the_mstp_msti_config, cfg_digest);
    conf->msti = the_mstp_msti_config.map.map[vid];
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mstp_msti_config_table_itr(const mesa_vid_t *prev_ifindex, mesa_vid_t *next_ifindex)
{
    mesa_vid_t vid = 0;

    if (prev_ifindex) {
        vid = *prev_ifindex;
    }

    if (vid + 1 >= VTSS_APPL_MSTP_MAX_VID - 1) {
        return VTSS_RC_ERROR;
    }

    *next_ifindex = vid + 1;
    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mstp_msti_config_set_table(mesa_vid_t vid, const vtss_appl_mstp_vlan_msti_config *conf)
{
    u8 cfg_digest[VTSS_APPL_MSTP_DIGEST_LEN];

    if (vid == 0 || vid >= VTSS_APPL_MSTP_MAX_VID - 1) {
        return VTSS_RC_ERROR;
    }

    if (conf->msti >= VTSS_APPL_MSTP_MAX_MSTI && conf->msti != VTSS_MSTID_TE) {
        return VTSS_RC_ERROR;
    }

    vtss_appl_mstp_msti_config_get(&the_mstp_msti_config, cfg_digest);
    the_mstp_msti_config.map.map[vid] = conf->msti;
    vtss_appl_mstp_msti_config_set(&the_mstp_msti_config);
    return VTSS_RC_OK;
}

/**
 * msti config table with msti value as index (iteration key)
 */
mesa_rc vtss_appl_mstp_msti_table_get(vtss_appl_mstp_mstid_t msti_value, vtss_appl_mstp_vlan_bitmap_t *vlan_bitmap)
{
    u8  cfg_digest[VTSS_APPL_MSTP_DIGEST_LEN];
    int vid;

    // Sanity check
    if ((msti_value >= VTSS_APPL_MSTP_MAX_MSTI && msti_value != VTSS_MSTID_TE) || !vlan_bitmap) {
        return VTSS_RC_ERROR;
    }

    vtss_appl_mstp_msti_config_get(&the_mstp_msti_config, cfg_digest);
    for (vid = 1; vid < VTSS_APPL_VLAN_ID_MAX; ++vid) {
        // vid 0 , 4095 are reserved
        if (msti_value == the_mstp_msti_config.map.map[vid]) {
            VTSS_BF_SET(vlan_bitmap->vlan_bitmap, vid, TRUE);
        } else {
            VTSS_BF_SET(vlan_bitmap->vlan_bitmap, vid, FALSE);
        }
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mstp_msti_table_itr(const vtss_appl_mstp_mstid_t *const prev_msti, vtss_appl_mstp_mstid_t *const next_msti)
{
    vtss_appl_mstp_mstid_t tmp_msti = 0;

    if (prev_msti == NULL) {
        *next_msti = 0;
    } else {
        tmp_msti = *prev_msti;
        if (tmp_msti == VTSS_MSTID_TE) {
            return VTSS_RC_ERROR;
        }
        if (tmp_msti + 1 > VTSS_APPL_MSTP_MAX_MSTI) {
            *next_msti = VTSS_MSTID_TE;
        } else {
            *next_msti = tmp_msti + 1;
        }

        *next_msti = tmp_msti + 1;
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_appl_mstp_msti_table_set(vtss_appl_mstp_mstid_t msti_value, const vtss_appl_mstp_vlan_bitmap_t *vlan_bitmap)
{
    u8  cfg_digest[VTSS_APPL_MSTP_DIGEST_LEN];
    int vid;

    // Sanity check
    if ((msti_value >= VTSS_APPL_MSTP_MAX_MSTI && msti_value != VTSS_MSTID_TE) || !vlan_bitmap) {
        return VTSS_RC_ERROR;
    }

    vtss_appl_mstp_msti_config_get(&the_mstp_msti_config, cfg_digest);
    for (vid = 1; vid < VTSS_APPL_VLAN_ID_MAX; ++vid) {
        if (VTSS_BF_GET(vlan_bitmap->vlan_bitmap, vid)) {
            the_mstp_msti_config.map.map[vid] = msti_value;
        }
    }

    vtss_appl_mstp_msti_config_set(&the_mstp_msti_config);
    return VTSS_RC_OK;
}

