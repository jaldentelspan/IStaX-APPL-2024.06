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

/*  This header file is based on IANAifType-MIB to define the interface type which is used in ifType  */
#ifndef INET_ADDRESS_H
#define INET_ADDRESS_H

typedef enum {
    INET_ADDRESS_UNKNOWN = 0,
    INET_ADDRESS_IPV4,
    INET_ADDRESS_IPV6,
    INET_ADDRESS_IPV4Z,
    INET_ADDRESS_IPV6Z,
    INET_ADDRESS_DNS = 16,
} inet_address_type;

typedef enum {
    INET_VERSION_UNKNOWN = 0,
    INET_VERSION_IPV4,
    INET_VERSION_IPV6,
} inet_version;

#define INET_ADDRESS_IPV4_LEN 4
#define INET_ADDRESS_IPV6_LEN 16

BOOL prepare_get_next_inetAddress(long *type, char *addr, size_t addr_max_len, size_t *addr_len );
mesa_rc get_pre_inetAddress(char *addr, size_t *addr_len );

#endif

