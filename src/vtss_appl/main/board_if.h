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
#ifndef _MSCC_APPL_BOARD_IF_H_
#define _MSCC_APPL_BOARD_IF_H_

#include "meba.h"

extern CapArray<meba_port_entry_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> port_custom_table;

#ifdef __cplusplus
extern "C" {
#endif

extern meba_inst_t board_instance;

// DEPRECATED - at some time...
//static inline vtss_board_type_t vtss_board_type(void) __attribute__ ((deprecated));
static inline vtss_board_type_t vtss_board_type(void)
{
    return board_instance->props.board_type;
}

/* Chip Compile Target */
static inline mesa_target_type_t vtss_api_chipid(void)
{
    return board_instance->props.target;
}

static inline const char *vtss_board_name(void)
{
    return board_instance->props.name;
}

#ifdef __cplusplus
}
#endif

#endif /* _MSCC_APPL_BOARD_IF_H_ */
