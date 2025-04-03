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

#ifndef DOT1PORT_API_H
#define DOT1PORT_API_H

#include "l2proto_api.h"

#define DOT1PORT_NO_START 1
#define DOT1PORT_NO_NONE  0

/*
   PORTLIST type is described in RFC2674, the description is as following:

   Each octet within this value specifies a set of eight
   ports, with the first octet specifying ports 1 through
   8, the second octet specifying ports 9 through 16, etc.
   Within each octet, the most significant bit represents
   the lowest numbered port, and the least significant bit
   represents the highest numbered port.  Thus, each port
   of the bridge is represented by a single bit within the
   value of this object.  If that bit has a value of '1'
   then that port is included in the set of ports; the port
   is not included if its bit has a value of '0'.
*/

// Avoid "Warning -- Constant value Boolean" Lint error
/*lint -emacro(506,VTSS_PORTLIST_BF_SET) */
#define VTSS_PORTLIST_BF_SIZE(count)      (((count)+7)/8)
#define VTSS_PORTLIST_BF_GET(a, n)    ((a[((n)-1)/8] & (1<<(7-((n)-1)%8))) ? 1 : 0)
#define VTSS_PORTLIST_BF_SET(a, n, v) { if (v) { a[((n)-1)/8] |= (1U<<(7-((n)-1)%8)); } else { a[((n)-1)/8] &= ~(1U<<(7-((n)-1)%8)); }}
//#define VTSS_PORTLIST_BF_SET(a, n, v) { if (v) { a[((n)-1)/8] |= (1U<<(7-((n)-1)%8)); } else { a[((n)-1)/8] &= ~(1U<<(7-((n)-1)%8)); }}
#define VTSS_PORTLIST_BF_CLR(a, count)    (memset(a, 0, VTSS_PORTLIST_BF_SIZE(count)))


typedef enum {
    DOT1PORT_TYPE_PORT,
    DOT1PORT_TYPE_LLAG,
    DOT1PORT_TYPE_GLAG,
    DOT1PORT_TYPE_UNDEF
} dot1Port_type_t;

typedef struct {
    l2_port_no_t    dot1port; /* l2port + DOT1PORT_NO_START */
    dot1Port_type_t type;
    vtss_isid_t     isid;
    u_long          if_id;
} dot1Port_info_t;

/**
  * \brief Get the existent dot1Port.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [IN]  l2port: l2port
  *                       [OUT] type: The interface type
  *                       [OUT] isid: The ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL dot1Port_get( dot1Port_info_t *info );

/**
  * \brief Get the next existent dot1Port.
  *
  * \param info [INOUT]:    The information parameter has following members\n
  *                       [IN]  l2port: l2port
  *                       [OUT] type: The interface type
  *                       [OUT] isid: The ISID, if the output type is neither IFINDEX_TYPE_PORT nor IFINDEX_TYPE_LLAG interface, the isid shouldn't be modified.
  *                       [OUT] if_id: The interface ID
  * \return
  *    FALSE if the IfIndex is not available.\n
  *     Otherwise, return TRUE.
  */
BOOL dot1Port_get_next( dot1Port_info_t *info );


/**
  * \brief Get the existent dot1Port in the specific type.
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
BOOL dot1Port_get_by_interface( dot1Port_info_t *info );


#endif

