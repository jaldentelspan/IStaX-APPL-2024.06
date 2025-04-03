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

/**
 * \file
 * \brief Public LACP API
 * \details This header file describes LACP functions and types
 */

#ifndef _VTSS_APPL_LACP_H_
#define _VTSS_APPL_LACP_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>

/**
 * Minimum valid value of LACP priority.
 */
#define VTSS_APPL_LACP_PRIORITY_MINIMUM 1

/**
 * Maximum valid value of LACP priority.
 */
#define VTSS_APPL_LACP_PRIORITY_MAXIMUM 65535

/**
 * Default valid value of LACP priority.
 */
#define VTSS_APPL_LACP_PRIORITY_DEFAULT 32768

/**
 * Minimum valid value of LACP per-port key.
 * By default port key is 0; Which means key will be automatically selected,
 * based upon link speed.
 */
#define VTSS_APPL_LACP_PORT_KEY_MINIMUM      0

/**
 * Maximum valid value of LACP per-port key.
 */
#define VTSS_APPL_LACP_PORT_KEY_MAXIMUM      65535

/**
 * LACP global configuration(s).
 */
typedef struct {
    /**
     * System priority is any value which ranges from VTSS_APPL_LACP_PRIORITY_MINIMUM to
     * VTSS_APPL_LACP_PRIORITY_MAXIMUM. Default value is VTSS_APPL_LACP_PRIORITY_DEFAULT.
     */
    uint32_t     system_prio;
} vtss_appl_lacp_globals_t;

/**
 * LACP port configurations comprises of various LACP per port settings.
 */
typedef struct {
    /**
     * The priority of the port. If the LACP partner wants to form a larger
     * group than is supported by this device then this parameter will control
     * which ports will be active and which ports will be in a backup role.
     * Lower number means greater priority.
     * A valid value of priority lies in range from VTSS_APPL_LACP_PRIORITY_MINIMUM to
     * VTSS_APPL_LACP_PRIORITY_MAXIMUM. Default value is VTSS_APPL_LACP_PRIORITY_DEFAULT.
     */
    uint32_t                 port_prio;

    /**
     * A physical switch port with fast_xmit_mode=true will transmit LACP
     * packets each second, while one with fast_xmit_mode=false will wait
     * for 30 seconds before sending a LACP packet.
     */
    mesa_bool_t               fast_xmit_mode;

} vtss_appl_lacp_port_conf_t;

/**
 * The LACP system status information structure.
 */
typedef struct {

    /**
     * A unique number used to identify a aggregation group
     */
    uint16_t             aggr_id;

    /**
     * MAC Address of the aggregation partner
     */
    mesa_mac_t      partner_oper_system;

    /**
     * The system priority value which is assigned to this aggregation group by
     * aggregation partner
     */
    uint16_t             partner_oper_system_priority;

    /**
     * The key value assigned to this aggregation group by aggregation partner
     */
    uint16_t             partner_oper_key;

    /**
     * The time in seconds since the aggregation last changed
     */
    uint32_t             secs_since_last_change;

    /**
     * The bitmap of local switch ports which are member of this aggregation
     * for this switch.
     */
    vtss_port_list_stackable_t  port_list;

} vtss_appl_lacp_aggregator_status_t;


/**
 * LACP operational port status information table
 */
typedef struct {
    /**
     * The LACP mode status on a given port.
     * When port_enabled=true then it means LACP is enabled on that port
     */
    mesa_bool_t lacp_mode;

    /**
     * The configured port priority
     */
    uint16_t actor_port_prio;

    /**
     * The configured port key
     */
    uint16_t actor_oper_port_key;

    /**
     * The current LACP port Actor_State
     */
    uint8_t actor_lacp_state;

    /**
     * The partner port number
     */
    uint16_t partner_oper_port_number;

    /**
     * The partner priority
     */
    uint16_t partner_oper_port_priority;

    /**
     * The partner key
     */
    uint16_t partner_oper_port_key;

    /**
     * The current LACP port partner state
     */
    uint8_t partner_lacp_state;

} vtss_appl_lacp_port_status_t;


/**
 * The port statistics counters of received and transmitted
 * LACP frames.
 */
typedef struct {

    uint64_t lacp_frame_tx;   /*!< LACP frames transmitted */

    uint64_t lacp_frame_rx;   /*!< LACP frames received */

    uint64_t unknown_frame_rx;/*!< Unknown frames received and discarded in error */

    uint64_t illegal_frame_rx;/*!< Illegal frames received and discarded in error */

} vtss_appl_lacp_port_stats_t;

/**
 * The LACP group properties.
 * The group is identified by the key id.
 */
typedef struct {
    /**
     * Revertive (TRUE) port will change back to active if it comes back up.  Non-revertive (FALSE) will remain standby.
     * All ports in the group are revertive as default.
     */
    mesa_bool_t revertive;
    /**
     * Max number of ports that can bundle up in an aggregation. Remaining ports will go into standby mode.
     * There is no limit for max bundle at default.
     */
    uint32_t max_bundle;
} vtss_appl_lacp_group_conf_t;

#ifdef __cplusplus
extern "C" {
#endif
/*************** LACP Application Management APIs ****************/

/**
 * vtss_appl_lacp_globals_get
 *
 * Purpose: To read the LACP global configuration(s).
 *
 * \param conf [OUT] the LACP global configuration(s).
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_lacp_globals_get(vtss_appl_lacp_globals_t *const conf);

/**
 * vtss_appl_lacp_globals_set
 *
 * Purpose: To write the LACP global configuration(s).
 *
 * \param conf [IN] the LACP global configuration(s).
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_lacp_globals_set(const vtss_appl_lacp_globals_t *const conf);

/**
 * vtss_lacp_port_conf_get
 *
 * Purpose: To read the LACP port configuration.
 *
 * \param ifindex [IN] the logical interface index/number.
 *
 * \param pconf [OUT] the LACP port configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_lacp_port_conf_get(vtss_ifindex_t ifindex, vtss_appl_lacp_port_conf_t *const pconf);

/**
 * vtss_lacp_port_conf_set
 *
 * Purpose: To set the LACP port configuration.
 *
 * \param ifindex [IN] the logical interface index/number.
 *
 * \param pconf [IN] the reference to LACP port configuration to be set.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_lacp_port_conf_set(vtss_ifindex_t ifindex, const vtss_appl_lacp_port_conf_t *const pconf);

/**
 * vtss_appl_lacp_group_if_conf_set
 *
 * Purpose: To set the LACP group properties using an ifindex as the key.
 *
 * \param ifindex [IN] the ifindex of the group.
 *
 * \param gconf [IN] the properties of the group.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_lacp_group_if_conf_set(const vtss_ifindex_t ifindex, const vtss_appl_lacp_group_conf_t *const gconf);

/**
 * vtss_appl_lacp_group_conf_set
 *
 * Purpose: To set the LACP group properties using a simple id as the key.
 *
 * \param key [IN] the key of the group. This equals the port_key of a port.
 *
 * \param gconf [IN] the properties of the group.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_lacp_group_conf_set(uint16_t key, const vtss_appl_lacp_group_conf_t *const gconf);

/**
 * vtss_appl_lacp_group_if_conf_get
 *
 * Purpose: To get the LACP group properties using an ifindex as the key.
 *
 * \param ifindex [IN] the ifindex of the group.
 *
 * \param gconf [OUT] the properties of the group.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_lacp_group_if_conf_get(const vtss_ifindex_t ifindex, vtss_appl_lacp_group_conf_t *const gconf);

/**
 * vtss_appl_lacp_group_conf_get
 *
 * Purpose: To get the LACP group properties using a simple id as the key.
 *
 * \param key [IN] the key of the group. This equals the port_key of a port.
 *
 * \param gconf [OUT] the properties of the group.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_lacp_group_conf_get(uint16_t key, vtss_appl_lacp_group_conf_t *const gconf);

/**
 * vtss_lacp_system_status_get
 *
 * Purpose: To set the LACP port configuration.
 *
 * \param ifindex [IN] the logical aggregation index/number.
 *
 * \param stat [OUT] the reference to LACP port status data.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_lacp_system_status_get(vtss_ifindex_t ifindex, vtss_appl_lacp_aggregator_status_t *const stat);

/**
 * vtss_appl_lacp_port_status_get
 *
 * Purpose: To read the LACP port configuration.
 *
 * \param ifindex [IN] the physical interface index/number.
 *
 * \param status [OUT] the reference to LACP port status data.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_lacp_port_status_get(vtss_ifindex_t ifindex, vtss_appl_lacp_port_status_t *const status);


/**
 * vtss_appl_lacp_port_stats_get
 *
 * Purpose: To read the LACP port statistics.
 *
 * \param ifindex [IN] the physical interface index/number.
 *
 * \param stats [OUT] the reference to LACP port statistics.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_lacp_port_stats_get(vtss_ifindex_t ifindex, vtss_appl_lacp_port_stats_t *const stats);

/**
 * vtss_lacp_port_stats_clr
 *
 * Purpose: To clear the LACP port statistics.
 *
 * \param ifindex [IN] the logical interface index/number. 
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_lacp_port_stats_clr(vtss_ifindex_t ifindex);

#ifdef __cplusplus
}
#endif

#endif  /* _VTSS_APPL_LACP_H_ */

