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

#ifndef LLDP_BASIC_TYPES_H
#define LLDP_BASIC_TYPES_H
#include "main.h"

typedef BOOL lldp_bool_t;
typedef ushort lldp_timer_t;
typedef char lldp_8_t;
typedef u8  lldp_u8_t;
typedef u16 lldp_u16_t;
typedef i16 lldp_16_t;
typedef u32 lldp_u32_t;
typedef i32 lldp_32_t;
typedef u64 lldp_u64_t;
typedef i64 lldp_64_t;

/* port number,  counting from 1..LLDP_PORTS */
typedef mesa_port_no_t lldp_port_t;

#endif  //LLDP_BASIC_TYPES_H
