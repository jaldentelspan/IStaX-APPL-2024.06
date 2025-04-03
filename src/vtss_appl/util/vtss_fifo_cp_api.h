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

#ifndef _VTSS_FIFO_CP_API_H_
#define _VTSS_FIFO_CP_API_H_

/******************************************************************************/
// The vtss_fifo is a dynamically growing list of any-sized user data, with a 
// write and read operator. The list cannot shrink.
// When you initialize the FIFO, you specify the initial number of items
// and its maximum number of items together with a growth size, which is used
// if an item is added to the FIFO when it's full.
// You can build up the items you write to the FIFO on the stack, since
// the data is memcpy'd into the FIFO. Likewise, when popping the FIFO, you
// can allocate the structure on the stack and call the pop operator with a
// pointer to the structure. It's not required that you use VTSS_MALLOC() to
// allocate the item (unless it's very large, in which case, I suggest that
// you use the vtss_fifo component, which stores pointers).
//
// NOTE: IF MORE THAN ONE THREAD ACCESSES THE FIFO, PROTECT IT WITH A MUTEX!!
/******************************************************************************/

/******************************************************************************/
// The user normally doesn't need to be concerned about this structure.
// In C++ the members would've been declared private.
/******************************************************************************/
typedef struct {
  u32 *items;
  u32 user_item_sz_bytes;
  u32 alloc_item_sz_dwords;
  u32 sz;
  u32 cnt;
  u32 max_sz;
  u32 growth;
  u32 wr_idx;
  u32 rd_idx;
  u32 keep_statistics;
  u32 max_cnt;   // The highest simultaneously seen number of items in the fifo
  u32 total_cnt; // The total number of items that have been added to the fifo.
  u32 overruns;  // The number of times calls to vtss_fifo_cp_wr() failed.
  u32 underruns; // The number of times calls to vtss_fifo_cp_rd() failed.
} vtss_fifo_cp_t;

/******************************************************************************/
/******************************************************************************/
mesa_rc vtss_fifo_cp_init(vtss_fifo_cp_t *fifo, u32 item_sz_bytes, u32 init_sz, u32 max_sz, u32 growth, u32 keep_statistics);
mesa_rc vtss_fifo_cp_wr(vtss_fifo_cp_t *fifo, void *item);
mesa_rc vtss_fifo_cp_rd(vtss_fifo_cp_t *fifo, void *item);
mesa_rc vtss_fifo_cp_clr(vtss_fifo_cp_t *fifo);
u32   vtss_fifo_cp_cnt(vtss_fifo_cp_t *fifo);
void    vtss_fifo_cp_get_statistics(vtss_fifo_cp_t *fifo, u32 *max_cnt, u32 *total_cnt, u32 *cur_cnt, u32 *cur_sz, u32 *overruns, u32 *underruns);
void    vtss_fifo_cp_clr_statistics(vtss_fifo_cp_t *fifo);

#endif // _VTSS_FIFO_CP_API_H_

