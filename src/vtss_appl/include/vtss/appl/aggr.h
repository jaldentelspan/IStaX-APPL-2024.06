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

/**
 * \file
 * \brief Public Aggregation API
 * \details This header file describes functions and types for static link aggregation
 */

#ifndef _VTSS_APPL_AGGR_H_
#define _VTSS_APPL_AGGR_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <microchip/ethernet/switch/api.h>

/**
 * vtss_appl_aggr_group_t hold the group membership information for all physical ports
 */
typedef struct {
    vtss_port_list_stackable_t member; /**< The member list for an aggregation group     */
} vtss_appl_aggr_group_t;

/**
 * Contains the entry (i.e. members' list) and aggregation number for an
 * aggregation entry
 */
typedef struct {
    vtss_appl_aggr_group_t    entry;        /**< Aggregation entry associated with the aggr_no */
    mesa_aggr_no_t       aggr_no;      /**< The number of this aggregation group          */
} vtss_appl_aggr_group_member_t;

/**
 * Contains the mode of the aggregation group
 */
typedef enum {
    VTSS_APPL_AGGR_GROUP_MODE_DISABLED,     /**< Group not used                                                 */
    VTSS_APPL_AGGR_GROUP_MODE_RESERVED,     /**< Group reserved, no members yet                                 */
    VTSS_APPL_AGGR_GROUP_MODE_STATIC_ON,    /**< Static aggregation enabled                                     */
    VTSS_APPL_AGGR_GROUP_MODE_LACP_ACTIVE,  /**< Active LACP, initiates negotiation with partner port           */
    VTSS_APPL_AGGR_GROUP_MODE_LACP_PASSIVE  /**< Passive LACP, responds to LACP packets but does not initiate   */
} vtss_appl_aggr_mode_t;

/**
 * Aggregation group configuration.
 */
typedef struct {
    vtss_appl_aggr_group_t     cfg_ports; /**< Port members of the aggregation group. */
    vtss_appl_aggr_mode_t      mode;      /**< Aggregation mode of the group          */
} vtss_appl_aggr_group_conf_t;


/**
 * Status of an aggregation group.
 */
typedef struct {

    /**
     * Configured Port members of the aggregation group.
     */
    vtss_appl_aggr_group_t   cfg_ports;

    /**
     * Aggregated Port members of the aggregation group.
     */
    vtss_appl_aggr_group_t   aggr_ports;

    /**
     * Aggregation Speed.
     */
    mesa_port_speed_t   speed;

    /**
     * Aggregation mode  :Disabled     ;Not active
                         :Reserved     ;Reserved with no port members
                         :Static       ;Static aggregation
                         :LACP Active  ;LACP in active mode
                         :LACP Passive ;LACP in passive mode
     */
    vtss_appl_aggr_mode_t mode;

    /**
     * Aggregation mode as a string.
     */

    char                type[15];

} vtss_appl_aggr_group_status_t;


/**
 * vtss_appl_aggregation_mode_get
 *
 * Purpose: Returns the aggregation mode (same for all switches in a stack)
 *
 * \param mode [OUT] the static aggregation traffic distribution settings'
 * modes.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_aggregation_mode_get(mesa_aggr_mode_t *const mode);

/**
 * vtss_aggregation_mode_set
 *
 * Purpose: Set the aggregation mode for all groups.
 *
 * \param mode [IN] the static aggregation traffic distribution settings' modes.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_aggregation_mode_set(const mesa_aggr_mode_t *const mode);

/**
 * vtss_appl_aggregation_port_members_get:
 *
 * Purpose: Get members of a aggr group (only for STATIC created groups).
 *
 * \param ifindex [IN] Interface index - the logical aggregation index/aggr_no.
 *
 * \param members [OUT] Points to the updated group and memberlist.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 *
 * \note: Ports which are members of a statically created group - but
 *        without a portlink will not be included in the returned portlist.
 */
mesa_rc vtss_appl_aggregation_port_members_get(vtss_ifindex_t ifindex,
                                               vtss_appl_aggr_group_member_t *const members);

/**
 * vtss_appl_aggregation_port_members_itr
 *
 * Purpose: To iterate through status entries of existing aggregation groups.
 *
 * \param  prev [IN] previous Interface index - the logical aggregation index.
 *
 * \param  next [OUT] next Interface index - the logical aggregation index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_aggregation_port_members_itr(const vtss_ifindex_t *const prev,
                                               vtss_ifindex_t       *const next);

/**
 * vtss_appl_aggregation_group_set
 *
 * Purpose: Reserves or sets the mode (LACP/STATIC) for a aggregation group.
 *
 * \param ifindex [IN] Interface index - the logical aggregation index/number.
 *
 * \param conf [IN] conf - the mode of the group.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_aggregation_group_set(const vtss_ifindex_t ifindex,
                                        const vtss_appl_aggr_group_conf_t *const conf);

/**
 * vtss_appl_aggregation_group_get
 *
 * Purpose: Gets the mode (Disabled/Reserved/LACP/Sstatic) of an aggregation group.
 *
 * \param ifindex [IN] Interface index - the logical aggregation index/number.
 *
 * \param conf [OUT] conf - the configuration of the group.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_aggregation_group_get(vtss_ifindex_t ifindex,
                                        vtss_appl_aggr_group_conf_t *const conf);

/**
 * vtss_appl_aggregation_status_get
 *
 * Purpose: To get the status of both Static and LACP aggregation groups.
 *
 * \param  ifindex [IN] Interface index - the logical aggregation index.
 *
 * \param  status [OUT]  Points to the struct containing Group status information.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_aggregation_status_get(vtss_ifindex_t ifindex,
                                         vtss_appl_aggr_group_status_t *const status);

/**
 * vtss_appl_aggregation_status_itr
 *
 * Purpose: To iterate through status entries of existing aggregation groups.
 *
 * \param  prev [IN] previous Interface index - the logical aggregation index.
 *
 * \param  next [OUT] next Interface index - the logical aggregation index.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_aggregation_status_itr(const vtss_ifindex_t *const prev,
                                         vtss_ifindex_t       *const next);
#endif  /* _VTSS_APPL_AGGR_H_ */
