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
 * \file frr_ospf6_api.h
 * \brief This file contains the definitions of OSPF6 internal API functions,
 * including the application public header APIs(naming start with
 * vtss_appl_ospf6) and the internal module APIs(naming start with frr_ospf6)
 */

#ifndef _FRR_OSPF6_API_HXX_
#define _FRR_OSPF6_API_HXX_

/******************************************************************************/
/** Includes                                                                  */
/******************************************************************************/
#include "frr.hxx"
#include <vtss/appl/ospf6.h>

/*
 * Notice!!!
 *
 * Although FRR 2.0 does not yet support multiple OSPF6 processes. We still
 * reserve the parameter (instance ID) for future usage.
 * In the current stage, only value 1 is acceptable. And API caller should
 * use the definition 'FRR_OSPF6_DEFAULT_INSTANCE_ID' instead of hard code value.
 * When the multiple OSPF6 process is supported in the future, it can be a
 * serching
 * keyword to indicate where need to be changed.
 */
#define FRR_OSPF6_DEFAULT_INSTANCE_ID VTSS_APPL_OSPF6_INSTANCE_ID_START

/**  The backbone area ID */
#define FRR_OSPF6_BACKBONE_AREA_ID 0

/******************************************************************************/
/** Module initialization                                                     */
/******************************************************************************/
mesa_rc frr_ospf6_init(vtss_init_data_t *data);
const char *frr_ospf6_error_txt(mesa_rc rc);

/******************************************************************************/
/** Module internal APIs                                                      */
/******************************************************************************/
#define FRR_OSPF6_DEF_PRIORITY 1
#define FRR_OSPF6_DEF_FAST_HELLO_PKTS \
    2 /* Hello packet will be sent every 500ms */
#define FRR_OSPF6_DEF_HELLO_INTERVAL 10     /* in seconds */
#define FRR_OSPF6_DEF_DEAD_INTERVAL 40      /* in seconds */
#define FRR_OSPF6_DEF_RETRANSMIT_INTERVAL 5 /* in seconds */
#define FRR_OSPF6_DEF_TRANSMIT_DELAY 1      /* in seconds */

/**
 * \brief Get the OSPF6 default instance for clearing OSPF6 routing process.
 * \param id [OUT] OSPF6 instance ID.
 * \return Error code.
 */
mesa_rc vtss_appl_ospf6_def(vtss_appl_ospf6_id_t *const id);

/**
 * \brief Get the OSPF6 area default configuration.
 * \param id [OUT] OSPF6 instance ID.
 * \param network [OUT] OSPF6 area network.
 * \param area_id [OUT] OSPF6 area ID.
 * \return Error code.
 */
mesa_rc frr_ospf6_area_conf_def(vtss_appl_ospf6_id_t *const id,
                                mesa_ipv6_network_t *const network,
                                vtss_appl_ospf6_area_id_t *const area_id);

/**
 * \brief Get the OSPF6 router default configuration.
 * \param id   [IN] OSPF6 instance ID.
 * \param conf [OUT] OSPF6 router configuration.
 * \return Error code.
 */
mesa_rc frr_ospf6_router_conf_def(vtss_appl_ospf6_id_t *const id,
                                  vtss_appl_ospf6_router_conf_t *const conf);

/**
 * \brief Get the OSPF6 area range default configuration.
 * \param id      [IN]  OSPF6 instance ID.
 * \param area_id [IN]  OSPF6 area ID.
 * \param network [IN]  OSPF6 area range network.
 * \param conf    [OUT] OSPF6 area range configuration.
 * \return Error code.
 */
mesa_rc frr_ospf6_area_range_conf_def(vtss_appl_ospf6_id_t *const id,
                                      vtss_appl_ospf6_area_id_t *const area_id,
                                      mesa_ipv6_network_t *const network,
                                      vtss_appl_ospf6_area_range_conf_t *const conf);

/**
 * \brief Get the OSPF6 VLAN interface default configuration.
 * \param ifindex [IN]  The index of VLAN interface.
 * \param conf    [OUT] OSPF6 VLAN interface configuration.
 * \return Error code.
 */
mesa_rc frr_ospf6_intf_conf_def(vtss_ifindex_t *const ifindex,
                                vtss_appl_ospf6_intf_conf_t *const conf);

/**
 * \brief Get OSPF6 control of global options.
 * It is a dummy function for SNMP serialzer only.
 * \param control [in] Pointer to the control global options.
 * \return Error code.
 */
mesa_rc frr_ospf6_control_globals_dummy_get(
    vtss_appl_ospf6_control_globals_t *const control);

/**
 * \brief Get the default configuration for a specific stub areas.
 * \param id      [IN] OSPF6 instance ID.
 * \param area_id [IN]  The area ID of the stub area configuration.
 * \param conf    [OUT] The stub area configuration.
 * \return Error code.
 *  VTSS_APPL_FRR_OSPF6_ERROR_STUB_AREA_NOT_FOR_BACKBONE means the area is
 *  backbone area.
 */
mesa_rc frr_ospf6_stub_area_conf_def(vtss_appl_ospf6_id_t *const id,
                                     vtss_appl_ospf6_area_id_t *const area_id,
                                     vtss_appl_ospf6_stub_area_conf_t *const conf);

#endif /* _FRR_OSPF6_API_HXX_ */

