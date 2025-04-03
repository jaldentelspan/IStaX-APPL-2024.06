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

#include <main.h>
#include "inet_address.h"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_SNMP

BOOL prepare_get_next_inetAddress(long *type, char *addr, size_t addr_max_len, size_t *addr_len )
{
    if (addr_max_len < INET_ADDRESS_IPV6_LEN) {
        return FALSE;
    }

    switch (*type) {
    case INET_ADDRESS_UNKNOWN:
        *type = INET_ADDRESS_IPV4;
        *addr_len = INET_ADDRESS_IPV4_LEN;
        memset( addr, 0, *addr_len);
        break;
    case INET_ADDRESS_IPV4:
        if (*addr_len < INET_ADDRESS_IPV4_LEN) {
            *addr_len = INET_ADDRESS_IPV4_LEN;
        } else if (*addr_len > INET_ADDRESS_IPV4_LEN) {
            *type = INET_ADDRESS_IPV6;
            *addr_len = INET_ADDRESS_IPV6_LEN;
        } else {
            break;
        }
        memset( addr, 0, *addr_len);
        break;
    case INET_ADDRESS_IPV6:
        if (*addr_len < INET_ADDRESS_IPV6_LEN) {
            *addr_len = INET_ADDRESS_IPV6_LEN;
        } else if (*addr_len > INET_ADDRESS_IPV6_LEN) {
            return FALSE;
        } else {
            break;
        }
        memset( addr, 0, *addr_len);
        break;
    default:
        return FALSE;
    }

    return TRUE;
}


mesa_rc get_pre_inetAddress(char *addr, size_t *addr_len )
{
    int i = 0;
    BOOL overflow = TRUE;
    u8 *tmp = NULL;
    u8 *ptr = tmp;

    VTSS_MALLOC_CAST(tmp, sizeof(char) * (*addr_len));

    if ( NULL == tmp) {
        return VTSS_RC_INCOMPLETE;
    }

    memcpy(tmp, addr, *addr_len);
    for (i = *addr_len - 1, ptr = (u8 *)(tmp + *addr_len - 1); i >= 0; i--, ptr-- ) {
        if (*ptr != 0x0 ) {
            --*ptr;
            overflow = FALSE;
            break;
        } else {
            *ptr = 0xff;
        }
    }

    if ( FALSE == overflow) {
        memcpy(addr, tmp, *addr_len);
    }
    VTSS_FREE(tmp);
    return overflow ? VTSS_RC_ERROR : VTSS_RC_OK;
}

