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

#ifndef IFINDEX_H
#define IFINDEX_H

/* interfaces ----------------------------------------------------------*/
#include "topo_api.h"
#include "msg_api.h"
#include "vlan_api.h"
#ifdef VTSS_SW_OPTION_AGGR
#include "aggr_api.h"
#endif /* VTSS_SW_OPTION_AGGR */
#ifdef VTSS_SW_OPTION_IP
#include "ip_api.h"
#endif /* VTSS_SW_OPTION_IP */

#ifdef __cplusplus
extern "C" {
#endif

#define IFTABLE_IFINDEX_END     0xFFFFFFFF

/* mib2/ifTable - ifIndex type */
typedef enum {
    IFTABLE_IFINDEX_TYPE_PORT,
    IFTABLE_IFINDEX_TYPE_LLAG,
    IFTABLE_IFINDEX_TYPE_GLAG,
    IFTABLE_IFINDEX_TYPE_VLAN,
    IFTABLE_IFINDEX_TYPE_UNDEF
} ifIndex_type_t;

#define IFTABLE_IFINDEX_TYPE_FLAG_SET(f, x)         (f |= (0x1 << x))
#define IFTABLE_IFINDEX_TYPE_FLAG_CHK(f, x)         (f & (0x1 << x))

typedef u32  ifIndex_id_t;

typedef struct {
    ifIndex_id_t    ifIndex;
    ifIndex_type_t  type;
    vtss_isid_t     isid;
    u32             if_id;
} iftable_info_t;

/**
  * \brief Get the existent IfIndex.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [IN] ifIndex: The ifIndex
  *                       [OUT] type: The interface type
  *                       [OUT] isid: The ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get( iftable_info_t *info );

/**
  * \brief Get the valid IfIndex.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [IN] ifIndex: The ifIndex
  *                       [OUT] type: The interface type
  *                       [OUT] isid: The ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_valid( iftable_info_t *info );

/**
  * \brief Get the next existent IfIndex.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [INOUT] ifIndex: The next ifIndex
  *                       [OUT] type: The next interface type
  *                       [OUT] isid: The next ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: the next interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_next( iftable_info_t *info );

/**
  * \brief Get the next valid IfIndex.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [INOUT] ifIndex: The next ifIndex
  *                       [OUT] type: The next interface type
  *                       [OUT] isid: The next ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: the next interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_next_valid( iftable_info_t *info );

/**
  * \brief Get the first existent IfIndex in the specific type.
  *
  * \param info [INOUT]:    The info parameter has following members\n
  *                       [IN] type: The next interface type
  *                       [OUT] ifIndex: The next ifIndex
  *                       [OUT] isid: The next ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The next interface ID
  * \return
  *    FALSE if there is no available ifIndex.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_first_by_type( iftable_info_t *info );

/**
  * \brief Get the next existent IfIndex in the specific type.
  *
  * \param info [INOUT]:    The info parameter has following members\n
  *                       [INOUT] ifIndex: The next ifIndex, if any
  *                       [OUT] type: The interface type
  *                       [OUT] isid: The first ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The first interface ID
  * \param if_types [IN]:   Using macro IFTABLE_IFINDEX_TYPE_FLAG_SET(if_types, IFTABLE_IFINDEX_TYPE_XXX) to specify interface types for filtering the get next result.
  * \return
  *    FALSE if there is no available ifIndex.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_next_by_type( iftable_info_t *info, u32 if_types );

/**
  * \brief Get the existent IfIndex in the specific type.
  *
  * \param info [INOUT]:    The info parameter has following members\n
  *                       [IN] type: The interface type
  *                       [IN] if_id: The interface ID
  *                       [IN] isid: The ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] ifIndex: The ifIndex
  * \return
  *    FALSE if there is no available ifIndex.\n
  *     Otherwise, return TRUE.
  */
BOOL ifIndex_get_by_interface( iftable_info_t *info );

#ifdef VTSS_SW_OPTION_IP
BOOL get_next_ip(mesa_ip_addr_t *ip_addr, mesa_vid_t *if_id);
#endif /* VTSS_SW_OPTION_IP */

/* ifIndex initial function
   return 0 when operation is success, - 1 otherwize. */
int ifIndex_init(void);

#ifdef __cplusplus
}
#endif
#endif                          /* IFINDEX_H */

