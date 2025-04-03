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

/**
 * \file frr_ospf_api.h
 * \brief This file contains the definitions of OSPF internal API functions,
 * including the application public header APIs(naming start with
 * vtss_appl_ospf) and the internal module APIs(naming start with frr_ospf)
 */

#ifndef _FRR_OSPF_API_HXX_
#define _FRR_OSPF_API_HXX_

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr.hxx"
#include <vtss/appl/ospf.h>

/*
 * Notice!!!
 *
 * Although FRR 2.0 does not yet support multiple OSPF processes. We still
 * reserve the parameter (instance ID) for future usage.
 * In the current stage, only value 1 is acceptable. And API caller should
 * use the definition 'FRR_OSPF_DEFAULT_INSTANCE_ID' instead of hard code value.
 * When the multiple OSPF process is supported in the future, it can be a
 * serching
 * keyword to indicate where need to be changed.
 */
#define FRR_OSPF_DEFAULT_INSTANCE_ID VTSS_APPL_OSPF_INSTANCE_ID_START

/**  The backbone area ID */
#define FRR_OSPF_BACKBONE_AREA_ID 0

/******************************************************************************/
/** Module initialization                                                     */
/******************************************************************************/
mesa_rc frr_ospf_init(vtss_init_data_t *data);
const char *frr_ospf_error_txt(mesa_rc rc);

/******************************************************************************/
/** Module internal APIs                                                      */
/******************************************************************************/
#define FRR_OSPF_DEF_PRIORITY 1
#define FRR_OSPF_DEF_FAST_HELLO_PKTS \
    2 /* Hello packet will be sent every 500ms */
#define FRR_OSPF_DEF_HELLO_INTERVAL 10     /* in seconds */
#define FRR_OSPF_DEF_DEAD_INTERVAL 40      /* in seconds */
#define FRR_OSPF_DEF_RETRANSMIT_INTERVAL 5 /* in seconds */
#define FRR_OSPF_DEF_TRANSMIT_DELAY 1      /* in seconds */

/**
 * \brief Get the OSPF default instance for clearing OSPF routing process.
 * \param id [OUT] OSPF instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf_def(vtss_appl_ospf_id_t *const id);

/**
 * \brief Get the OSPF area default configuration.
 * \param id [OUT] OSPF instance ID.
 * \param network [OUT] OSPF area network.
 * \param area_id [OUT] OSPF area ID.
 * \return Error code.
 */
mesa_rc frr_ospf_area_conf_def(vtss_appl_ospf_id_t *const id,
                               mesa_ipv4_network_t *const network,
                               vtss_appl_ospf_area_id_t *const area_id);

/**
 * \brief Get the OSPF router default configuration.
 * \param id   [IN] OSPF instance ID.
 * \param conf [OUT] OSPF router configuration.
 * \return Error code.
 */
mesa_rc frr_ospf_router_conf_def(vtss_appl_ospf_id_t *const id,
                                 vtss_appl_ospf_router_conf_t *const conf);

/**
 * \brief Get the default configuration for a specific stub areas.
 * \param id        [IN]  OSPF instance ID.
 * \param area_id   [IN]  OSPF area ID.
 * \param auth_type [OUT] The authentication type.
 * \return Error code.
 */
mesa_rc frr_ospf_area_auth_conf_def(vtss_appl_ospf_id_t *const id,
                                    vtss_appl_ospf_area_id_t *const area_id,
                                    vtss_appl_ospf_auth_type_t *const auth_type);

/**
 * \brief Get Get the default configuration for message digest key.
 * \param ifindex       [IN]  The index of VLAN interface.
 * \param key_id        [IN]  The key ID.
 * \param digest_key    [OUT] The digest key.
 * \return Error code.
 */
mesa_rc frr_ospf_intf_auth_digest_key_def(
    vtss_ifindex_t *const ifindex, uint8_t *const key_id,
    vtss_appl_ospf_auth_digest_key_t *const digest_key);

/**
 * \brief Set the digest key in the specific interface.
 *        It is a dummy function for SNMP serialzer only.
 * \param ifindex       [IN] The index of VLAN interface.
 * \param key_id        [IN] The key ID.
 * \param digest_key    [IN] The digest key.
 * \return Error code.
 */
mesa_rc frr_ospf_intf_auth_digest_key_dummy_set(
    const vtss_ifindex_t ifindex, const uint8_t key_id,
    const vtss_appl_ospf_auth_digest_key_t *const digest_key);

/**
 * \brief Get the OSPF area range default configuration.
 * \param id      [IN]  OSPF instance ID.
 * \param area_id [IN]  OSPF area ID.
 * \param network [IN]  OSPF area range network.
 * \param conf    [OUT] OSPF area range configuration.
 * \return Error code.
 */
mesa_rc frr_ospf_area_range_conf_def(vtss_appl_ospf_id_t *const id,
                                     vtss_appl_ospf_area_id_t *const area_id,
                                     mesa_ipv4_network_t *const network,
                                     vtss_appl_ospf_area_range_conf_t *const conf);

/**
 * \brief Get the default configuration of OSPF virtual link.
 * \param id        [IN]  OSPF instance ID.
 * \param area_id   [IN]  OSPF area ID.
 * \param router_id [IN]  OSPF area destination router id of virtual link.
 * \param conf      [OUT] OSPF virtual link configuration.
 * \return Error code.
 */
mesa_rc frr_ospf_vlink_conf_def(vtss_appl_ospf_id_t *const id,
                                vtss_appl_ospf_area_id_t *const area_id,
                                vtss_appl_ospf_router_id_t *const router_id,
                                vtss_appl_ospf_vlink_conf_t *const conf);

/**
 * \brief Get the default configuration of message digest key for the specific
 * virtual
 * link.
 * \param id        [IN] OSPF instance ID.
 * \param area_id   [IN] OSPF area ID.
 * \param router_id [IN] OSPF router ID.
 * \param key_id    [IN] The message digest key ID.
 * \param md_key    [IN] The message digest key configuration.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_TOO_LONG means the password
 *  is too long.
 *  VTSS_APPL_FRR_OSPF_ERROR_AUTH_KEY_INVALID means the key is not a valid
 *  key for AES256.
 */
mesa_rc frr_ospf_vlink_md_key_conf_def(
    vtss_appl_ospf_id_t *const id, vtss_appl_ospf_area_id_t *const area_id,
    vtss_appl_ospf_router_id_t *const router_id,
    vtss_appl_ospf_md_key_id_t *const key_id,
    vtss_appl_ospf_auth_digest_key_t *const md_key);

/**
 * \brief Set the message digest key for the specific virtual link.
 * It is a dummy function for SNMP serialzer only.
 */
mesa_rc frr_ospf_vlink_md_key_conf_dummy_set(
    const vtss_appl_ospf_id_t id, const vtss_appl_ospf_area_id_t area_id,
    const vtss_appl_ospf_router_id_t router_id,
    const vtss_appl_ospf_md_key_id_t key_id,
    const vtss_appl_ospf_auth_digest_key_t *const md_key);

/**
 * \brief Get the OSPF VLAN interface default configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF VLAN interface configuration.
 * \return Error code.
 */
mesa_rc frr_ospf_intf_conf_def(vtss_ifindex_t *const ifindex,
                               vtss_appl_ospf_intf_conf_t *const conf);

/**
 * \brief Get OSPF control of global options.
 * It is a dummy function for SNMP serialzer only.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc frr_ospf_control_globals_dummy_get(
    vtss_appl_ospf_control_globals_t *const control);

/**
 * \brief Get the default configuration for a specific stub areas.
 * \param id      [IN] OSPF instance ID.
 * \param area_id [IN]  The area ID of the stub area configuration.
 * \param conf    [OUT] The stub area configuration.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc frr_ospf_stub_area_conf_def(vtss_appl_ospf_id_t *const id,
                                    vtss_appl_ospf_area_id_t *const area_id,
                                    vtss_appl_ospf_stub_area_conf_t *const conf);

#endif /* _FRR_OSPF_API_HXX_ */

