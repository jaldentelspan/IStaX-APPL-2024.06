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
 * \file
 * \brief Public Energy Efficient Ethernet(EEE) APIs.
 * \details This header file describes public energy efficient ethernet APIs.
 *          EEE is a power saving option that reduces the switch power consumption 
 *          when there is little or no traffic on a switch port.
 */

#ifndef _VTSS_APPL_EEE_H_
#define _VTSS_APPL_EEE_H_

#include <vtss/appl/types.h>
#include <vtss/appl/interface.h>
#include <vtss/basics/enum-descriptor.h>    // For vtss_enum_descriptor_t

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief Types of optimization preference
 *  EEE can be configured either to give maximum power savings or low traffic latency.
 */
typedef enum {
    /** EEE optimize for low traffic latency */
    VTSS_APPL_EEE_OPTIMIZATION_PREFERENCE_LATENCY,
    /** EEE optimize for maximum power saving  */
    VTSS_APPL_EEE_OPTIMIZATION_PREFERENCE_POWER
} vtss_appl_eee_optimization_preference_t;

/**
 * \brief EEE global configuration.
 */
typedef struct {
    /** EEE optimization preferences, either maximum power saving or low traffic latency. */
    vtss_appl_eee_optimization_preference_t preference;
} vtss_appl_eee_global_conf_t;

/**
 *  \brief EEE port configurable parameters.
 */
typedef struct {
    /** Enable EEE (IEEE 802.3az) feature on a port. */
    mesa_bool_t eee_enable;
} vtss_appl_eee_port_conf_t;

/**
 * \brief Egress port queues type. 
 */
typedef enum {
    /** Normal queue, en-queued data will be transmitted with latency depending upon traffic utilization. */
    VTSS_APPL_EEE_QUEUE_TYPE_NORMAL,
    /** Urgent queue, en-queued data will be transmitted with minimum latency. */
    VTSS_APPL_EEE_QUEUE_TYPE_URGENT
} vtss_appl_eee_queue_type_t;

/**
 *  \brief EEE egress port queues.
 *  \details Each port have set of egress queues. 
 *   Each queue is configured either as urgent queue or normal queue.
 *   Queues configured as urgent, en-queued data will be transmitted with minimum latency. 
 *   Queue configured as normal, en-queued data will be transmitted with latency 
 *   depending upon traffic utilization. 
 */
typedef struct {
    /** Whether queue is urgent queue or normal queue. */
    vtss_appl_eee_queue_type_t queue_type; 
} vtss_appl_eee_port_queue_t;
    
/**
 * \brief Egress port queue number for transmitting en-queued data.
 */
typedef  uint8_t vtss_appl_eee_port_queue_index_t;

/**
 * \brief Port status. 
 */
typedef enum {
    /** Functionality is enabled. */
    VTSS_APPL_EEE_STATUS_DISABLE,
    /** Functionality is disabled. */
    VTSS_APPL_EEE_STATUS_ENABLE,
    /** Functionality is not supported. */
    VTSS_APPL_EEE_STATUS_NOT_SUPPORTED
} vtss_appl_eee_status_t;

/**
 *  \brief Energy Efficient Ethernet port status.
 */
typedef struct {
    /** Whether partner link advertising EEE(IEEE 802.3az) capability. */
    vtss_appl_eee_status_t link_partner_eee;         
    /** Whether port rx path currently in power save state. */
    vtss_appl_eee_status_t rx_in_power_save_state;   
} vtss_appl_eee_port_status_t;

/**
 *   \brief Energy Efficient Ethernet platform specific global capabilities.
 */
typedef struct {
    /** Whether maximum power savings or low traffic latency optimization preference capable . */
    mesa_bool_t optimization_preference_capable;            
} vtss_appl_eee_global_capabilities_t;

/**
 * \brief Get global capabilities
 *
 * \param capabilities [OUT]: EEE platform specific global capabilities
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_eee_global_capabilities_get(vtss_appl_eee_global_capabilities_t *const capabilities);

/**
 *   \brief Energy Efficient Ethernet platform specific port capabilities.
 */
typedef struct {
    /** Maximum number of supported egress port queues. */
    uint8_t  queue_count; 
    /** Whether port is EEE(IEEE 802.3az) capable or not.*/
    mesa_bool_t eee_capable;            
} vtss_appl_eee_port_capabilities_t;

/**
 * \brief Get port capabilities
 *
 * \param ifIndex       [IN]: Interface index
 * \param capabilities [OUT]: EEE platform specific port capabilities
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_eee_port_capabilities_get(vtss_ifindex_t                    ifIndex, 
                                             vtss_appl_eee_port_capabilities_t *const capabilities);


/**
 * \brief Set Energy Efficient Ethernet global parameters.
 *
 * \param conf  [IN]: EEE global configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_eee_global_conf_set(const vtss_appl_eee_global_conf_t  *const conf);

/**
 * \brief Get Energy Efficient Ethernet global parameters.
 *
 * \param conf  [OUT]: EEE global configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_eee_global_conf_get(vtss_appl_eee_global_conf_t  *const conf);

/**
 * \brief Set Energy Efficient Ethernet port configuration.
 *
 * \param ifIndex  [IN]: Interface index
 * \param conf     [IN]: EEE port configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_eee_port_conf_set(vtss_ifindex_t ifIndex,
                                     const vtss_appl_eee_port_conf_t  *const conf);

/**
 * \brief Get Energy Efficient Ethernet port configuration.
 *
 * \param ifIndex  [IN] : Interface index
 * \param conf     [OUT]: EEE port configurable parameters
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_eee_port_conf_get(vtss_ifindex_t            ifIndex,
                                     vtss_appl_eee_port_conf_t *const conf);

/**
 * \brief Set whether given egress port queue type is urgent queue or normal queue. 
 *
 * \param ifIndex     [IN]: Interface index
 * \param queueIndex  [IN]: Queue index value 
 * \param type        [IN]: Egress port queue type
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_eee_port_queue_type_set(vtss_ifindex_t                   ifIndex,
                                           vtss_appl_eee_port_queue_index_t queueIndex,
                                           const vtss_appl_eee_port_queue_t *const type);

/**
 * \brief Get whether given egress port queue type is urgent queue or normal queue. 
 *
 * \param ifIndex      [IN]: Interface index
 * \param queueIndex   [IN]: Queue index value
 * \param type        [OUT]: Egress port queue type
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_eee_port_queue_type_get(vtss_ifindex_t                   ifIndex,
                                           vtss_appl_eee_port_queue_index_t queueIndex,
                                           vtss_appl_eee_port_queue_t       *const type);
/**
 * \brief Egress port queue iterate function. 
 * \details It is used to get first and get next egress port queue indexes.
 *
 * \param prevIfindex     [IN]: Ifindex of previous port
 * \param nextIfindex    [OUT]: Ifindex of next port
 * \param prevQueueIndex  [IN]: Previous port queue index
 * \param nextQueueIndex [OUT]: Next port queue index

 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_eee_port_queue_iterator(const vtss_ifindex_t                   *const prevIfindex,
                                           vtss_ifindex_t                         *const nextIfindex,
                                           const vtss_appl_eee_port_queue_index_t *const prevQueueIndex,
                                           vtss_appl_eee_port_queue_index_t       *const nextQueueIndex);

/**
 * \brief Get the port current status 
 *
 * \param ifIndex     [IN]: Interface index
 * \param status     [OUT]: Port status data
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
 mesa_rc vtss_appl_eee_port_status_get(vtss_ifindex_t              ifIndex,
                                       vtss_appl_eee_port_status_t *const status);

#ifdef __cplusplus
}  // extern "C"
#endif
#endif  // _VTSS_APPL_EEE_H_
