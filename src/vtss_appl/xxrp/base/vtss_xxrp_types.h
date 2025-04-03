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

#ifndef _VTSS_XXRP_TYPES_H_
#define _VTSS_XXRP_TYPES_H_

#include "vtss/basics/enum_macros.hxx"

/* Types part of platform code */
#include "main.h"
#include "vtss/appl/mstp.h"
#include "vtss_xxrp_util.h"
#include "vtss/basics/map.hxx"
#include "vtss_xxrp_types.hxx"

#define VTSS_XXRP_ETH_TYPE_LEN                 2
#define VTSS_XXRP_MAC_ADDR_LEN                 6
#define VTSS_MRP_PEER_MAC_ADDRESS_SIZE         6
#define XXRP_MAX_ATTRS                         4096 /* Maximum attributes supported by MRP. This should be equal
to the max attributes required by any MRP application    */
#define XXRP_MAX_ATTRS_BF        (XXRP_MAX_ATTRS / 8) /* Bitfield for maximum attributes supported by MRP */
#define XXRP_MAX_ATTR_REG_STATE  (XXRP_MAX_ATTRS / 4) /* Number of bytes required to store all reg states, i.e. NORMAL, FIXED, FORBIDDEN */
#define XXRP_MAX_ATTR_EVENTS_ARR (XXRP_MAX_ATTRS / 2) /* Maximum attribute events array */
#define XXRP_MAX_ATTR_TYPES      1                    /* For MVRP, this is 1, this will change when a new MRP application is added */

#define MRP_MSTI_MAX VTSS_APPL_MSTP_MAX_MSTI

#define VTSS_XXRP_REG_ADMIN_STATUS_GET(mad, indx) ((mad->registrar_admin_status[indx / 4] >> (2 * (indx % 4))) & 3U)
#define VTSS_XXRP_REG_ADMIN_STATUS_SET(mad, indx, t) (mad->registrar_admin_status[indx / 4] |= ((u8)t << (2 * (indx % 4))))
#define VTSS_XXRP_REG_ADMIN_STATUS_CLEAR(mad, indx) (mad->registrar_admin_status[indx / 4] &= ~(3U << (2 * (indx % 4))))

/***************************************************************************************************
* MRP application types
*
* Additional types such as MMRP, GVRP etc. can be defined here
**************************************************************************************************/
/**
 * \brief enumeration to list MRP applications.
 **/
typedef enum {
#ifdef VTSS_SW_OPTION_MVRP
    VTSS_MRP_APPL_MVRP,
#endif
#ifdef VTSS_SW_OPTION_GVRP
    VTSS_GARP_APPL_GVRP,
#endif
    VTSS_MRP_APPL_MAX  /* Always last entry! */
} vtss_mrp_appl_t;

/*lint -save -e19 */
VTSS_ENUM_INC(vtss_mrp_appl_t);
/*lint -restore */

/**
 * \brief structure to hold the timer values
 **/
typedef struct {
    u32 join_timer;         /**< Join timer     */
    u32 leave_timer;        /**< Leave timer    */
    u32 leave_all_timer;    /**< LeaveAll timer */
} vtss_mrp_timer_conf_t;

#ifdef VTSS_SW_OPTION_MVRP
/**
 * \brief MVRP attribute types.
 **/
typedef enum {
    VTSS_MVRP_VLAN_ATTRIBUTE,
} vtss_mvrp_attribute_type_t;
#endif /* VTSS_SW_OPTION_MVRP */

/**
 * \brief Depending on the application, the following union will be parsed.
 **/
typedef union {
    u32                        dummy; /* At least one member in the union */
#ifdef VTSS_SW_OPTION_MVRP
    vtss_mvrp_attribute_type_t mvrp_attr_type;
#endif /* VTSS_SW_OPTION_MVRP */
} vtss_mrp_attribute_type_t;

/**
 * \brief MSTP port state change enumeration.
 **/
typedef enum {
    VTSS_MRP_MSTP_PORT_ADD = 0,    /**< Port moving to FORWARDING state       */
    VTSS_MRP_MSTP_PORT_DELETE      /**< Port moving out of FORWARDING state   */
} vtss_mrp_mstp_port_state_change_type_t;

/**
 * \brief MSTP port role change enumeration.
 **/
typedef enum {
    VTSS_MRP_MSTP_PORT_ROOT_OR_ALTERNATE_TO_DESIGNATED, /**< Port role to designated      */
    VTSS_MRP_MSTP_PORT_DEESIGNATED_TO_ROOT_OR_ALTERNATE /**< Port role to non-designated  */
} vtss_mrp_mstp_port_role_change_type_t;

/**
 * \brief MRP statistics enumeration.
 **/
typedef enum {
    VTSS_MRP_RX_PKTS,
    VTSS_MRP_RX_DROPPED_PKTS,
    VTSS_MRP_RX_NEW,
    VTSS_MRP_RX_JOININ,
    VTSS_MRP_RX_IN,
    VTSS_MRP_RX_JOINMT,
    VTSS_MRP_RX_MT,
    VTSS_MRP_RX_LV,
    VTSS_MRP_RX_LA,
    VTSS_MRP_TX_PKTS,
    VTSS_MRP_TX_NEW,
    VTSS_MRP_TX_JOININ,
    VTSS_MRP_TX_IN,
    VTSS_MRP_TX_JOINMT,
    VTSS_MRP_TX_MT,
    VTSS_MRP_TX_LV,
    VTSS_MRP_TX_LA,
    VTSS_MRP_STAT_MAX
} vtss_mrp_stat_type_t;

/**
 * \brief structure to denote MRP statistics.
 **/
typedef struct {
    /* Rx statistics */
    u32  pkts_rx;         /* Counter of total received packets   */
    u32  pkts_dropped_rx; /* Counter of packets dropped          */
    u32  new_rx;          /* Counter of new events received      */
    u32  joinin_rx;       /* Counter of joinin events received   */
    u32  in_rx;           /* Counter of in received              */
    u32  joinmt_rx;       /* Counter of joinmt events received   */
    u32  mt_rx;           /* Counter of mt events received       */
    u32  leave_rx;        /* Counter of leave events received    */
    u32  leaveall_rx;     /* Counter of leaveall events received */
    /* Tx statistics */
    u32  pkts_tx;     /* Counter of packets transmitted         */
    u32  new_tx;      /* Counter of new events transmitted      */
    u32  joinin_tx;   /* Counter of joinin events transmitted   */
    u32  in_tx;       /* Counter of in events transmitted       */
    u32  joinmt_tx;   /* Counter of joinmt events transmitted   */
    u32  mt_tx;       /* Counter of mt events transmitted       */
    u32  leave_tx;    /* Counter of leave events transmitted    */
    u32  leaveall_tx; /* Counter of leaveall events transmitted */
} vtss_mrp_stats_t;

/**
 *  \brief Structure to represent Applicant and Registrar states of an attribute.
 */
typedef struct {
    u8 applicant; /**<  Current Applicant state for an attribute */
    u8 registrar; /**<  Current Registrar state for an attribute */
} vtss_mrp_mad_machine_t;

/**
 *  \brief Structure to represent MRP MAD for a port.
 */
typedef struct { /* mad */
    u32                    port_no;                                 /**< L2 port number                                             */
    i32                    join_timeout;                            /**< Configured join timeout                                    */
    i32                    leave_timeout;                           /**< Configure leave timeout                                    */
    i32                    leaveall_timeout;                        /**< Configured leaveall timeout                                */
    i32                    periodic_tx_timeout;                     /**< Configured periodic tx timeout                             */
    vtss_mrp_mad_machine_t *machines;                               /**< Applicant and registrar states for all the attributes      */
    u8                     peer_mac_address[VTSS_MRP_PEER_MAC_ADDRESS_SIZE];/**< Peer MAC address                                   */
    BOOL                   peer_mac_updated;                        /**< Flag to indicate whether peer_mac_address is updated       */
    BOOL                   join_timer_running;                      /**< Flag to indicate join timer is running                     */
    BOOL                   leaveall_timer_running;                  /**< Flag to indicate leaveall timer is running                 */
    BOOL                   periodic_timer_running;                  /**< Flag to indicate periodic timer is running                 */
    u8                     leave_timer_running[XXRP_MAX_ATTRS_BF];  /**< Flag for each attribute to indicate leave timer is running */
    BOOL                   periodic_timer_control_status;           /**< Periodic timer control status                              */
    BOOL                   leaveall_stm_current_state;              /**< Current FSM state of leaveall timer                        */
    BOOL                   periodic_stm_current_state;              /**< Current FSM state of periodic timer                        */
    i16                    join_timer_count;                        /**< Current join timer expiry in cs. This is valid only when
                                                                      join_timer_running = TRUE                                     */
    i16                    leave_timer_count[XXRP_MAX_ATTRS];       /**< Current leave timer expiry in cs for all the attributes,
                                                                      valid when leave_timer_running = TRUE for an attribute        */
    i16                    leaveall_timer_count;                    /**< Current leaveall timer expiry in cs, valid only when
                                                                      leaveall_timer_running = TRUE                                 */
    i16                    periodic_timer_count;                    /**< Current periodic tx timer expiry in cs. This is valid only
                                                                      when periodic_timer_running = TRUE                            */
    BOOL                   join_timer_kick;
    u8                     leave_timer_kick[XXRP_MAX_ATTRS_BF];
    BOOL                   leaveall_timer_kick;
    BOOL                   periodic_timer_kick;
    BOOL                   attr_type_admin_status[XXRP_MAX_ATTR_TYPES]; /**< Attribute type control status for each attribute type  */
    u8                     registrar_admin_status[XXRP_MAX_ATTR_REG_STATE]; /* Table keepin all registrars' administrative status */
    vtss_mrp_stats_t       stats;                                   /**< MRP statistics                                             */
} vtss_mrp_mad_t;

/**
 *  \brief Structure to represent MRP MAP for a port.
 */
typedef struct {
    u32          port;                                    /* Port number                                         */
    BOOL         tc_detected[MRP_MSTI_MAX];               /* tc_detected flag for each MSTI instance             */
unsigned int is_connected :
    MRP_MSTI_MAX;             /* each bit represents an MSTI instance. If it is set,
                                                                       this port is connected in that MSTI       */
    void         *next_in_port_ring;                      /* pointer to connect all the ports for which MRP is
                                                                       enabled and port is in FORWARDING state   */
    void         *next_in_connected_ring[MRP_MSTI_MAX];   /* pointer to connect all the ports of a MSTI instance */
} vtss_mrp_map_t;

/**
 *  \brief Structure to represent MRP application.
 *  Each MRP, i.e., each instance of an application that uses the MRP
 *  protocol, is represented as a struct or control block with common
 *  initial fields. These comprise pointers to application-specific
 *  functions that are by the MAD and MAP components to signal protocol
 *  events to the application, and other controls common to all
 *  applications. The pointers include a pointer to the instances of MAD
 *  (one per port) for the application,and to MAP (one per application).
 *  The signaling functions include the addition and removal of ports,
 *  which the application should use to initialize port attributes with
 *  any management state required.
 */
typedef struct {
    vtss_mrp_appl_t appl_type;                                                          /**< Application type MVRP or MMRP  */
    vtss_mrp_mad_t  **mad;                                                              /**< pointer to MAD pointer array   */
    vtss_mrp_map_t  **map;                                                              /**< pointer to MAP pointer array   */
    u32             max_mad_index;                                                      /**< Maximum attributes             */
    u32             last_mad_used;                                                      /**< TODO                           */
    void (*join_indication_fn)(u32 port_no,
                               u16 joining_mad_index, BOOL new_);                       /**< Join indication function       */
    void (*leave_indication_fn)(u32 port_no, u32 leaving_mad_index);                                 /**< Leave indication function      */
    void (*join_propagated_fn)(void *, void *mad, u32 joining_mad_index, BOOL new_);    /**< Join propagation function      */
    void (*leave_propagated_fn)(void *, void *mad, u32 leaving_mad_index);              /**< Leave propagation function     */
    u32  (*transmit_fn)(u32 port_no, u8 *all_attr_events, u32 no_events, BOOL la_flag); /**< PDU transmit function          */
    BOOL (*receive_fn)(u32 port_no, const u8 *pdu, u32 length);                         /**< PDU receive function           */
    void (*added_port_fn)(void *, u32 port_no);                                         /**< Port add function              */
    void (*removed_port_fn)(void *, u32 port_no);                                       /**< Port remove function           */
    void *appl_glob_data;      /* Used to Pass the Application Global Data */           /**< TODO                           */
} vtss_mrp_t;

#endif /* _VTSS_XXRP_TYPES_H_ */
