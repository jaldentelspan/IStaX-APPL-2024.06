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

#ifndef _L2PROTO_API_H_
#define _L2PROTO_API_H_

#ifdef VTSS_SW_OPTION_MSTP
#include <vtss/appl/mstp.h>
#endif

#include "vtss_common_os.h"
#include "port_iter.hxx"        /* For port state structure */
#ifdef VTSS_SW_OPTION_AGGR
#include "aggr_api.h"           /* For AGGR_LLAG_CNT */
#else
#define AGGR_LLAG_CNT 0
#define AGGR_MGMT_GROUP_NO_START 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* L2_MAX_POAGS */
typedef unsigned int l2_port_no_t;

/* From old API */
typedef mesa_port_no_t vtss_poag_no_t;
#define L2_MAX_SWITCH_PORTS_     (fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT))
#define L2_MAX_PORTS_            (L2_MAX_SWITCH_PORTS_ * VTSS_ISID_CNT)
#define L2_MAX_LLAGS_            ((L2_MAX_SWITCH_PORTS_ / 2) * VTSS_ISID_CNT)
#define L2_MAX_GLAGS_            0
#define L2_MAX_POAGS_            (L2_MAX_PORTS_+L2_MAX_LLAGS_+L2_MAX_GLAGS_)

#define L2_MAX_SWITCH_PORTS      (VTSS_PORTS)
#define L2_MAX_PORTS             (L2_MAX_SWITCH_PORTS * VTSS_ISID_CNT)
#define L2_MAX_LLAGS             (AGGR_LLAG_CNT * VTSS_ISID_CNT)
#define L2_MAX_GLAGS             0
#define L2_MAX_POAGS             (L2_MAX_PORTS+L2_MAX_LLAGS+L2_MAX_GLAGS)

#define L2_PORT_NAME_MAX         sizeof("MultiGigabyteFastEthernet 999/9999")

/* l2port mapping functions */
BOOL l2port2port(l2_port_no_t, vtss_isid_t *, mesa_port_no_t *);
BOOL l2port2poag(l2_port_no_t, vtss_isid_t *, vtss_poag_no_t *);
BOOL l2port2glag(l2_port_no_t, mesa_glag_no_t *);
const char *l2port2str(l2_port_no_t l2port);
const char *l2port2str_icli(l2_port_no_t l2port, char *buf, size_t buflen);

BOOL l2port_is_valid(l2_port_no_t);
BOOL l2port_is_port(l2_port_no_t);
BOOL l2port_is_poag(l2_port_no_t);
BOOL l2port_is_glag(l2_port_no_t);

#define L2PORT2PORT(isid, port) (((isid - 1) * L2_MAX_SWITCH_PORTS_)    + (port))
#define L2LLAG2PORT(isid, llag) (L2_MAX_PORTS_ + ((isid - 1) * (L2_MAX_SWITCH_PORTS_ / 2)) + ((llag)))
#define L2GLAG2PORT(glag)       (L2_MAX_PORTS_ + L2_MAX_LLAGS_          + ((glag)))

void l2_receive_indication(vtss_module_id_t modid, const void *packet, size_t len, mesa_port_no_t switchport, mesa_vid_t vid, mesa_glag_no_t glag_no);

typedef void (*l2_stack_rx_callback_t)(const void *packet, size_t len, mesa_vid_t vid, l2_port_no_t l2port);
void l2_receive_register(vtss_module_id_t modid, l2_stack_rx_callback_t cb);

#define N_L2_MSTI_MAX   8

#if defined(VTSS_SW_OPTION_RSTP)
void l2_flush(l2_port_no_t, vtss_common_vlanid_t vlan_id, BOOL exclude, uchar reason);
#endif

#ifdef VTSS_SW_OPTION_MSTP
void l2_flush_port(l2_port_no_t l2port);
#endif

#ifdef VTSS_SW_OPTION_MSTP
void l2_flush_vlan_port(l2_port_no_t l2port, vtss_common_vlanid_t vlan_id);
#endif

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Set the MSTP MSTI mapping table
 */
mesa_rc l2_set_msti_map(BOOL all_to_cist, size_t maplen, const vtss_appl_mstp_mstid_t *map);
#endif

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Get the MSTP MSTI mapping table
 */
mesa_rc l2_get_msti_map(uchar *map, size_t map_size);
#endif

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Set the Spanning Tree state of all MSTIs for a specific port.
 */
void l2_set_msti_stpstate_all(vtss_common_port_t portno, mesa_stp_state_t new_state);
#endif

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Set the Spanning Tree state of a specific MSTI port.
 */
void l2_set_msti_stpstate(uchar msti, vtss_common_port_t portno, mesa_stp_state_t new_state);
#endif

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Get the Spanning Tree state of a specific MSTI port.
 */
mesa_stp_state_t l2_get_msti_stpstate(uchar msti, vtss_common_port_t portno);
#endif

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Set the Spanning Tree states (port, MSTIs) of a port to that of another.
 */
void l2_sync_stpstates(vtss_common_port_t copy, vtss_common_port_t port);
#endif

mesa_rc l2_init(vtss_init_data_t *data);

#ifdef VTSS_SW_OPTION_MSTP
/* STP state change callback */
typedef void (*l2_stp_state_change_callback_t)(vtss_common_port_t l2port, vtss_common_stpstate_t new_state);
#endif

#ifdef VTSS_SW_OPTION_MSTP
/* STP state change callback registration */
mesa_rc l2_stp_state_change_register(l2_stp_state_change_callback_t callback);
#endif

#ifdef VTSS_SW_OPTION_MSTP
/* STP MSTI state change callback */
typedef void (*l2_stp_msti_state_change_callback_t)(vtss_common_port_t l2port, uchar msti, vtss_common_stpstate_t new_state);
#endif

#ifdef VTSS_SW_OPTION_MSTP
/* STP MSTI state change callback registration */
mesa_rc l2_stp_msti_state_change_register(l2_stp_msti_state_change_callback_t callback);
#endif

typedef enum {
    L2PORT_ITER_TYPE_PHYS     = 0x01,                                           /**< This is a physical port.            */
    L2PORT_ITER_TYPE_LLAG     = 0x02,                                           /**< This is a local aggregation.        */
    L2PORT_ITER_TYPE_GLAG     = 0x04,                                           /**< This is a global aggregation.       */
    L2PORT_ITER_TYPE_AGGR     = L2PORT_ITER_TYPE_LLAG | L2PORT_ITER_TYPE_GLAG,  /**< This is any aggregation type.       */
    L2PORT_ITER_TYPE_ALL      = 0x07,                                           /**< All l2port types.                   */

    /* Flags */
    L2PORT_ITER_ISID_ALL      = 0x40,                                           /**< Iterate all isids - present or not. */
    L2PORT_ITER_PORT_ALL      = 0x80,                                           /**< Iterate all ports - present or not. */
    L2PORT_ITER_ISID_CFG      = 0x100,                                          /**< Iterate configurable ISIDs.         */
    L2PORT_ITER_USID_ORDER    = 0x200,                                          /**< Get ISIDs in USID order.            */
    L2PORT_ITER_ALL           = L2PORT_ITER_ISID_ALL | L2PORT_ITER_PORT_ALL,    /**< Iterate all - present or not.       */
    L2PORT_ITER_ALL_USID      =  L2PORT_ITER_TYPE_ALL | L2PORT_ITER_USID_ORDER, /**< Iterate all - USID order    .       */
} l2port_iter_type_t;

/**
 * l2port iterator structure - combines switch, port iterator into one
 * logical unit.
 **/
typedef struct {

    /* Public members */
    l2_port_no_t       l2port;        /**< The current l2port */
    l2port_iter_type_t type;          /**< The current l2port type (mask) */
    vtss_isid_t        isid;          /**< The current isid. */
    vtss_usid_t        usid;          /**< The current usid. */
    mesa_port_no_t     iport;         /**< The current iport */
    mesa_port_no_t     uport;         /**< The current uport */

    /* Private members - don't use! */
    l2port_iter_type_t       itertype_req, itertype_pend;
    vtss_isid_t              isid_req;
    int                      ix;
    switch_iter_sort_order_t s_order;
    port_iter_sort_order_t   p_order;
    switch_iter_t            sit;
    port_iter_t              pit;
} l2port_iter_t;

#define L2PIT_TYPE(p, t) (((p)->type) & (t))

/**
 * \brief Initialize a l2port iterator.
 *
 * \param l2pit      [IN] L2Port iterator.
 *
 * \param isid       [IN] ISID to iterate (or VTSS_ISID_GLOBAL)
 *
 * \param l2type     [IN] Types of l2ports to iterate
 *
 * \return Return code (always VTSS_RC_OK).
 **/
mesa_rc l2port_iter_init(l2port_iter_t *l2pit, vtss_isid_t isid, l2port_iter_type_t l2type);

/**
 * \brief Get the next l2port.
 *
 * \param l2pit      [IN] L2Port iterator.
 *
 * \return TRUE if a l2port is found, otherwise FALSE.
 **/
BOOL l2port_iter_getnext(l2port_iter_t *l2pit);

#ifdef VTSS_SW_OPTION_MSTP
/**
 * Convert MSTID to MSTI
 *
 * \param mstid [IN] The given MSTID
 *
 * \return The MSTI index this maps to
 */
vtss_appl_mstp_msti_t l2_mstid2msti(vtss_appl_mstp_mstid_t mstid);
#endif

#ifdef __cplusplus
}
#endif
#endif // _L2PROTO_API_H_

