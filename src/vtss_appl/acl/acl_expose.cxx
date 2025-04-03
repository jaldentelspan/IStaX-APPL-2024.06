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
#include "main.h" 
#include "misc_api.h"
#include "acl_serializer.hxx"

const vtss_enum_descriptor_t vtss_appl_acl_ace_frame_type_txt[] = {
    {VTSS_APPL_ACL_ACE_FRAME_TYPE_ANY,    "any"},
    {VTSS_APPL_ACL_ACE_FRAME_TYPE_ETYPE,  "etype"},
    {VTSS_APPL_ACL_ACE_FRAME_TYPE_ARP,    "arp"},
    {VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV4,   "ipv4"},
    {VTSS_APPL_ACL_ACE_FRAME_TYPE_IPV6,   "ipv6"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_acl_ace_ingress_mode_txt[] = {
    {VTSS_APPL_ACL_ACE_INGRESS_MODE_ANY,        "any"},
    {VTSS_APPL_ACL_ACE_INGRESS_MODE_SPECIFIC,   "specific"},
    {VTSS_APPL_ACL_ACE_INGRESS_MODE_SWITCH,     "switch"},
    {VTSS_APPL_ACL_ACE_INGRESS_MODE_SWITCHPORT, "switchport"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_acl_hit_action_txt[] = {
    {VTSS_APPL_ACL_HIT_ACTION_PERMIT,   "permit"},
    {VTSS_APPL_ACL_HIT_ACTION_DENY,     "deny"},
    {VTSS_APPL_ACL_HIT_ACTION_REDIRECT, "redirect"},
    {VTSS_APPL_ACL_HIT_ACTION_EGRESS,   "egress"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_acl_ace_vlan_tagged_txt[] = {
    {VTSS_APPL_ACL_ACE_VLAN_TAGGED_ANY, "any"},
    {VTSS_APPL_ACL_ACE_VLAN_UNTAGGED,   "untagged"},
    {VTSS_APPL_ACL_ACE_VLAN_TAGGED,     "tagged"},
    {0, 0},
};

const vtss_enum_descriptor_t vtss_appl_acl_ace_arp_op_txt[] = {
    {VTSS_APPL_ACL_ACE_ARP_OP_ANY,      "any"},
    {VTSS_APPL_ACL_ACE_ARP_OP_ARP,      "arp"},
    {VTSS_APPL_ACL_ACE_ARP_OP_RARP,     "rarp"},
    {VTSS_APPL_ACL_ACE_ARP_OP_OTHER,    "other"},
    {0, 0},
};

/**
 * \brief Get ACL ACE event status.
 *
 * \param usid       [IN]  switch ID for user view (The value starts from 1).
 * \param status     [OUT] ACE event status.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_acl_status_ace_event_get(
    vtss_usid_t                         usid,
    vtss_appl_acl_status_ace_event_t    *const status
)
{
    return acl_status_ace_event_update.get(usid, status);
}

