/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _VTSS_FIFO_API_H_
#define _VTSS_FIFO_API_H_

#include <microchip/ethernet/switch/api.h>

/******************************************************************************/
// The vtss_fifo is a dynamically growing list of pointers, with write and 
// read operators. The list cannot shrink.
// When you initialize the FIFO, you specify the initial number of items
// and its maximum number of items together with a growth size, which is used
// if an item is added to the FIFO when it's full.
//
// NOTE: IF MORE THAN ONE THREAD ACCESSES THE FIFO, PROTECT IT WITH A MUTEX!!
//
// If you need to store more data than just a pointer, use the vtss_fifo_cp
// variant, which memcpy's your any-sized data into the FIFO when writing, 
// and memcpy's it out again when reading.
/******************************************************************************/

/******************************************************************************/
// The user normally doesn't need to be concerned about this structure.
// In C++ the members would've been declared private.
/******************************************************************************/
typedef struct {
  u32 sz;
  u32 cnt;
  u32 max_sz;
  u32 growth;
  void **items;
  u32 wr_idx;
  u32 rd_idx;
  u32 keep_statistics;
  u32 max_cnt;   // The highest simultaneously seen number of items in the fifo
  u32 total_cnt; // The total number of items that have been added to the fifo.
  u32 overruns;  // The number of times calls to vtss_fifo_cp() failed.
  u32 underruns; // The number of times calls to vtss_fifo_cp() failed.
} vtss_fifo_t;

/******************************************************************************/
// RBNTBD: API usage to be added.
/******************************************************************************/
mesa_rc vtss_fifo_init(vtss_fifo_t *fifo, u32 init_sz, u32 max_sz, u32 growth, u32 keep_statistics);
mesa_rc vtss_fifo_wr(vtss_fifo_t *fifo, void *item);
void    *vtss_fifo_rd(vtss_fifo_t *fifo);
u32   vtss_fifo_cnt(vtss_fifo_t *fifo);
void    vtss_fifo_get_statistics(vtss_fifo_t *fifo, u32 *max_cnt, u32 *total_cnt, u32 *cur_cnt, u32 *cur_sz, u32 *overruns, u32 *underruns);
void    vtss_fifo_clr_statistics(vtss_fifo_t *fifo);

#endif // _VTSS_FIFO_API_H_

