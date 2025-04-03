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

#ifndef __PORT_ITER_HXX__
#define __PORT_ITER_HXX__

#include "main_types.h"

/* ================================================================= *
 *  Switch iteration.
 * ================================================================= */
/**
 * \brief Private declaration used internally
 **/
typedef enum {
    SWITCH_ITER_STATE_FIRST, /**< This is the first call of switch_iter_getnext(). */
    SWITCH_ITER_STATE_NEXT,  /**< This is one of the following calls of switch_iter_getnext(). */
    SWITCH_ITER_STATE_LAST,  /**< This is the last call of switch_iter_getnext(). Next time switch_iter_getnext() returns FALSE. */
    SWITCH_ITER_STATE_DONE   /**< switch_iter_getnext() has returned FALSE and we are done. */
} switch_iter_state_t;

typedef enum {
    SWITCH_ITER_SORT_ORDER_ISID,        /**< Return the existing switches in isid order. */
    SWITCH_ITER_SORT_ORDER_USID,        /**< Return the existing switches in usid order. */
    SWITCH_ITER_SORT_ORDER_ISID_CFG,    /**< Return the configurable switches in isid order. */
    SWITCH_ITER_SORT_ORDER_USID_CFG,    /**< Return the configurable switches in usid order. */
    SWITCH_ITER_SORT_ORDER_ISID_ALL,    /**< Return the existing and non-existing switches in isid order. */
    SWITCH_ITER_SORT_ORDER_END = SWITCH_ITER_SORT_ORDER_ISID_ALL  /* Must be last value in enum */
} switch_iter_sort_order_t;

typedef struct {
    // private: Do not use these variables!
    uint32_t                 m_switch_mask;        /**< Bitmask of switches indexed with isid or usid. */
    uint32_t                 m_exists_mask;        /**< Bitmask of existing switches indexed with isid or usid. */
    switch_iter_state_t      m_state;              /**< Internal state of iterator. */
    switch_iter_sort_order_t m_order;              /**< Configured sort_order. */
    vtss_isid_t              m_sid;                /**< Internal sid variable. */

    // public: These variables are read-only and are valid after each switch_iter_getnext()
    vtss_isid_t              isid;                 /**< The current isid. */
    vtss_usid_t              usid;                 /**< The current usid. Not valid with SWITCH_ITER_SORT_ORDER_ISID_ALL (always zero). */
    bool                     first;                /**< The current switch is the first one. */
    bool                     last;                 /**< The current switch is the last one. The next call to switch_iter_getnext() will return FALSE. */
    bool                     exists;               /**< The current switch exists. */
    uint                     remaining;            /**< The remaining number of times a call to switch_iter_getnext() will return TRUE. */
} switch_iter_t;

/**
 * \brief Initialize a switch iterator.
 *
 * If any of the parameters are invalid, it'll return VTSS_INVALID_PARAMETER.
 * Otherwise it'll return VTSS_RC_OK.
 *
 * \param sit        [IN] Switch iterator.
 * \param isid       [IN] isid selector. Valid values are VTSS_ISID_START to (VTSS_ISID_END - 1) or VTSS_ISID_GLOBAL.
 * \param sort_order [IN] Sorting order.
 *
 * \return Return code.
 **/
mesa_rc switch_iter_init(switch_iter_t *sit, vtss_isid_t isid, switch_iter_sort_order_t sort_order);

/**
 * \brief Get the next switch.
 *
 * The number of times switch_iter_getnext() returns a switch is dependant on how the iterator was initialized.
 *
 * If isid == VTSS_ISID_GLOBAL, the returned number of switches is 0 to VTSS_ISID_CNT:
 *   If sort_order != SWITCH_ITER_SORT_ORDER_ISID_ALL, all existing (or configurable) switches are returned by switch_iter_getnext().
 *   If sort_order == SWITCH_ITER_SORT_ORDER_ISID_ALL, all existing and non-existing switches are returned by switch_iter_getnext().
 *
 * If isid != VTSS_ISID_GLOBAL, the returned number of switches is 0 to 1:
 *   If sort_order != SWITCH_ITER_SORT_ORDER_ISID_ALL and the switch exists (or is configurable) it is returned by switch_iter_getnext().
 *   If sort_order == SWITCH_ITER_SORT_ORDER_ISID_ALL the switch is always returned by switch_iter_getnext().
 *
 * If switch_iter_getnext() is called on a secondary switch or if switch_iter_init() is called with invalid parameters, it silently returns 0 switches.
 *
 * \param sit  [IN] Switch iterator.
 *
 * \return TRUE if a switch is found, otherwise FALSE.
 **/
bool switch_iter_getnext(switch_iter_t *sit);

/* ================================================================= *
 *  Port iteration.
 * ================================================================= */
/**
 * \brief Private declaration used internally
 **/
typedef enum {
    PORT_ITER_STATE_FIRST, /**< This is the first call of port_iter_getnext(). */
    PORT_ITER_STATE_NEXT,  /**< This is one of the following calls of port_iter_getnext(). */
    PORT_ITER_STATE_LAST,  /**< This is the last call of port_iter_getnext(). Next time port_iter_getnext() returns FALSE. */
    PORT_ITER_STATE_DONE,  /**< port_iter_getnext() has returned FALSE and we are done. */
    PORT_ITER_STATE_INIT   /**< Get next switch and reinitialize port iterator with the new isid. */
} port_iter_state_t;

typedef enum {
    PORT_ITER_SORT_ORDER_IPORT,    /**< Return the existing ports in iport order */
    PORT_ITER_SORT_ORDER_UPORT,    /**< Return the existing ports in uport order */
    PORT_ITER_SORT_ORDER_IPORT_ALL /**< Return the existing and non-existing ports in iport order */
} port_iter_sort_order_t;

typedef enum {
    PORT_ITER_TYPE_FRONT,    /**< This is a front port. */
    PORT_ITER_TYPE_LOOPBACK, /**< This is a loopback port. */
    PORT_ITER_TYPE_TRUNK,    /**< This is a trunk port. */
    PORT_ITER_TYPE_NPI,      /**< This is a NPI port. */
    PORT_ITER_TYPE_CPU,      /**< This is a CPU port. */
    PORT_ITER_TYPE_LAST      /**< This must always be the last one. */
} port_iter_type_t;

typedef enum {
    PORT_ITER_FLAGS_FRONT    = (1 << PORT_ITER_TYPE_FRONT),      /**< Return front ports. */
    PORT_ITER_FLAGS_LOOPBACK = (1 << PORT_ITER_TYPE_LOOPBACK),   /**< Return loopback ports. */
    PORT_ITER_FLAGS_TRUNK    = (1 << PORT_ITER_TYPE_TRUNK),      /**< Return trunk ports. */
    PORT_ITER_FLAGS_NPI      = (1 << PORT_ITER_TYPE_NPI),        /**< Return NPI ports. */
    PORT_ITER_FLAGS_CPU      = (1 << PORT_ITER_TYPE_CPU),        /**< Return CPU ports */
    PORT_ITER_FLAGS_NORMAL   =  PORT_ITER_FLAGS_FRONT,           /**< This is for normal use. */
    PORT_ITER_FLAGS_NORMAL_CPU =  PORT_ITER_FLAGS_FRONT |
                                  PORT_ITER_FLAGS_CPU,            /**< This is for normal use when cpu ports are included. */
    PORT_ITER_FLAGS_ALL      = (PORT_ITER_FLAGS_FRONT    |
                                PORT_ITER_FLAGS_LOOPBACK |
                                PORT_ITER_FLAGS_TRUNK    |
                                PORT_ITER_FLAGS_NPI),            /**< All port types. */
    PORT_ITER_FLAGS_ALL_CPU  = (PORT_ITER_FLAGS_FRONT    |
                                PORT_ITER_FLAGS_LOOPBACK |
                                PORT_ITER_FLAGS_TRUNK    |
                                PORT_ITER_FLAGS_NPI      |
                                PORT_ITER_FLAGS_CPU ),            /**< All port types including cpu ports. */
    PORT_ITER_FLAGS_UP       = (1 << PORT_ITER_TYPE_LAST),       /**< Only return ports with link. */
    PORT_ITER_FLAGS_DOWN     = (1 << (PORT_ITER_TYPE_LAST + 1))  /**< Only return ports without link. */
} port_iter_flags_t;

typedef struct {
// private: Do not use these variables!
    uint64_t               m_port_mask;        /**< Bitmask of ports indexed with iport or uport. */
    uint64_t               m_exists_mask;      /**< Bitmask of existing ports indexed with iport or uport. */
    port_iter_state_t      m_state;            /**< Internal state of iterator. */
    switch_iter_t         *m_sit;              /**< Configured switch iterator. */
    vtss_isid_t            m_isid;             /**< Configured isid. */
    port_iter_sort_order_t m_order;            /**< Configured sort_order. */
    port_iter_flags_t      m_flags;            /**< Configured port flags. */
    mesa_port_no_t         m_port;             /**< Internal port variable. */
// public: These variables are read-only and are valid after each port_iter_getnext()
    mesa_port_no_t         iport;              /**< The current iport */
    mesa_port_no_t         uport;              /**< The current uport */
    bool                   first;              /**< The current port is the first one */
    bool                   last;               /**< The current port is the last one. The next call to port_iter_getnext() will return FALSE. */
    bool                   exists;             /**< The current port exists. */
    bool                   link;               /**< The current link state. */
    port_iter_type_t       type;               /**< The current port type. Will never contain more than one type (only one bit set). */
    uint32_t               port_type;
} port_iter_t;

/**
 * \brief Initialize a port iterator.
 *
 * If this function is called on a secondary switch and sit == NULL and isid != VTSS_ISID_LOCAL, it'll return VTSS_UNSPECIFIED_ERROR.
 * If this function is called on a secondary switch and sit != NULL, it'll return VTSS_UNSPECIFIED_ERROR.
 * If any of the parameters are invalid, it'll return VTSS_INVALID_PARAMETER.
 * Otherwise it'll return VTSS_RC_OK.
 *
 * \param pit        [IN] Port iterator.
 * \param sit        [IN] Switch iterator. If sit != NULL, the port iterator iterates over all isid's returned from the switch iterator.
 *                        If sit == NULL, the port iterator iterates over the isid in the isid selector parameter.
 * \param isid       [IN] isid selector. Valid values are VTSS_ISID_START to (VTSS_ISID_END - 1) or VTSS_ISID_LOCAL when sit == NULL.
 *                        Not used if sit != NULL.
 * \param sort_order [IN] Sorting order.
 * \param flags      [IN] Port type(s) to be returned. Several port flags can be or'ed together.
 *
 * \return Return code.
 **/
mesa_rc port_iter_init(port_iter_t *pit, switch_iter_t *sit, vtss_isid_t isid, port_iter_sort_order_t sort_order, uint32_t flags);

/* Initialize port iterator for VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT and PORT_ITER_FLAGS_NORMAL */
void port_iter_init_local(port_iter_t *pit);

/* Initialize port iterator for VTSS_ISID_LOCAL, PORT_ITER_SORT_ORDER_IPORT_ALL and PORT_ITER_FLAGS_NORMAL */
void port_iter_init_local_all(port_iter_t *pit);

/**
 * \brief Get the next port.
 *
 * The number of times port_iter_getnext() returns a port is dependant on how the iterator was initialized.
 *
 * If isid == VTSS_ISID_LOCAL, we iterate over the local ports. This is valid on both primary and secondary switch.
 * If isid != VTSS_ISID_LOCAL, we iterate over the ports on the specific switch. This is only valid on the primary switch.
 *
 * If sort_order == PORT_ITER_SORT_ORDER_IPORT_ALL, we iterate over all ports. Even if we know that the actual port count is
 * less than the maximum port count or the switch doesn't exist, we will return them all.
 * If sort_order != PORT_ITER_SORT_ORDER_IPORT_ALL, we iterate over the actual port count if the switch exist. Otherwise we
 * return the maximum number of ports.
 *
 * If PORT_ITER_TYPE_FRONT is excluded from type, port_iter_getnext() may return 0 ports.
 *
 * \param pit  [IN] Port iterator.
 *
 * \return TRUE if a port is found, otherwise FALSE.
 **/
bool port_iter_getnext(port_iter_t *pit);


#endif /* __PORT_ITER_HXX__ */
