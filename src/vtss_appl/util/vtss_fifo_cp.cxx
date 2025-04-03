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

#include <main.h>
#include <vtss_fifo_cp_api.h>
#include <stdlib.h>
#include "control_api.h"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_NONE /* Should be the caller's module ID */

// #define RBN_DEBUG

/******************************************************************************/
/******************************************************************************/
mesa_rc vtss_fifo_cp_init(vtss_fifo_cp_t *fifo, u32 item_sz_bytes, u32 init_sz, u32 max_sz, u32 growth, u32 keep_statistics)
{
  VTSS_ASSERT(item_sz_bytes > 0);
  VTSS_ASSERT(max_sz >= init_sz);
  VTSS_ASSERT((max_sz>init_sz && growth>0) || max_sz==init_sz);
  VTSS_ASSERT(init_sz>1 || max_sz>0);

  // Allocate a multiplum of four bytes per item to be able to index it.
  // Also allocate a placeholder for a next pointer, so that the array can grow
  // dynamically.
  fifo->alloc_item_sz_dwords = ((item_sz_bytes+3)/4) + 1; // + 1 for the next pointer
  fifo->user_item_sz_bytes   = item_sz_bytes;

  if(init_sz>0) {
    int i;
    if((VTSS_MALLOC_CAST(fifo->items, init_sz * 4 * fifo->alloc_item_sz_dwords))==NULL) {
      return VTSS_UNSPECIFIED_ERROR;
    }

#ifdef RBN_DEBUG
    memset(fifo->items, 0xFF, init_sz * 4 * fifo->alloc_item_sz_dwords);
#endif

    // Link the items together in a circular list. Don't use pointers, but merely indices, since
    // the array is dynamically resized, whereby the absolute memory locations may change.
    for(i = 0; i < init_sz - 1; i++)
      fifo->items[i * fifo->alloc_item_sz_dwords] = (i + 1) * fifo->alloc_item_sz_dwords;
    fifo->items[(init_sz - 1) * fifo->alloc_item_sz_dwords] = 0;
  }

  fifo->sz      = init_sz;
  fifo->max_sz  = max_sz;
  fifo->growth  = growth;
  fifo->wr_idx = fifo->rd_idx = fifo->cnt = fifo->max_cnt = fifo->total_cnt = fifo->overruns = fifo->underruns = 0;
  fifo->keep_statistics = keep_statistics;
  return VTSS_RC_OK;
}

/******************************************************************************/
/******************************************************************************/
mesa_rc vtss_fifo_cp_wr(vtss_fifo_cp_t *fifo, void *item)
{
  if(fifo->cnt==fifo->sz) {
    // FIFO full. Reallocate memory, if we haven't reached the upper limit.
    if(fifo->sz < fifo->max_sz) {
      u32 new_sz = (fifo->sz + fifo->growth > fifo->max_sz) ? fifo->max_sz : fifo->sz + fifo->growth;
      u32 *new_items = (u32 *)VTSS_REALLOC(fifo->items, new_sz * 4 * fifo->alloc_item_sz_dwords);
      if(new_items==NULL) {
        return VTSS_UNSPECIFIED_ERROR;
      }

#ifdef RBN_DEBUG
      memset(&new_items[fifo->sz * fifo->alloc_item_sz_dwords], 0xFF, (new_sz - fifo->sz) * 4 * fifo->alloc_item_sz_dwords);
#endif

      // Link the new items into the existing circular list. Ideally, we would use the rd_idx's prev-idx,
      // but to save space, we don't have such an entry into the array, and since we don't grow the array very
      // often (at least this is not supposed to happen very often), I think it's OK to search through all items.
      // Remember to use the new_items array, since fifo->items is no longer valid!!
      {
        u32 i, idx = new_items[fifo->rd_idx]; // Way to say "->next" is to dereference the items array
        
        while(new_items[idx] != fifo->rd_idx)
          idx = new_items[idx];
        fifo->wr_idx = new_items[idx] = fifo->sz * fifo->alloc_item_sz_dwords; // Link tail of old list into beginning of new items

        // Initialize added items.
        for(i = fifo->sz; i < new_sz - 1; i++)
          new_items[i * fifo->alloc_item_sz_dwords] = (i + 1) * fifo->alloc_item_sz_dwords;
        new_items[(new_sz - 1) * fifo->alloc_item_sz_dwords] = fifo->rd_idx;
      }

      fifo->items = new_items;
      fifo->sz = new_sz;
    } else {
      // FIFO full and we're not allowed to allocate anymore memory.
      if (fifo->keep_statistics) {
        fifo->overruns++;
      }
      return VTSS_UNSPECIFIED_ERROR;
    }
  }

  memcpy(&fifo->items[fifo->wr_idx + 1], item, fifo->user_item_sz_bytes);
  fifo->wr_idx = fifo->items[fifo->wr_idx];
  fifo->cnt++;
  if(fifo->keep_statistics) {
    if(fifo->cnt > fifo->max_cnt)
      fifo->max_cnt = fifo->cnt;
    fifo->total_cnt++;
  }
  return VTSS_RC_OK;
}

/******************************************************************************/
/******************************************************************************/
mesa_rc vtss_fifo_cp_rd(vtss_fifo_cp_t *fifo, void *item)
{
  if(fifo->cnt==0 || item==NULL) {
    if (fifo->keep_statistics) {
      fifo->underruns++;
    }
    return VTSS_UNSPECIFIED_ERROR;
  }

  fifo->cnt--;
  memcpy(item, &fifo->items[fifo->rd_idx + 1], fifo->user_item_sz_bytes);
#ifdef RBN_DEBUG
  memset(&fifo->items[fifo->rd_idx + 1], 0xFF, 4 * (fifo->alloc_item_sz_dwords - 1));
#endif
  fifo->rd_idx = fifo->items[fifo->rd_idx];
  return VTSS_RC_OK;
}

/******************************************************************************/
/******************************************************************************/
mesa_rc vtss_fifo_cp_clr(vtss_fifo_cp_t *fifo)
{
  while (fifo->cnt) {
    fifo->cnt--;
#ifdef RBN_DEBUG
    memset(&fifo->items[fifo->rd_idx + 1], 0xFF, 4 * (fifo->alloc_item_sz_dwords - 1));
#endif
    fifo->rd_idx = fifo->items[fifo->rd_idx];
  }

  return VTSS_RC_OK;
}

/******************************************************************************/
/******************************************************************************/
u32 vtss_fifo_cp_cnt(vtss_fifo_cp_t *fifo)
{
  return fifo->cnt;
}

/******************************************************************************/
/******************************************************************************/
void vtss_fifo_cp_get_statistics(vtss_fifo_cp_t *fifo, u32 *max_cnt, u32 *total_cnt, u32 *cur_cnt, u32 *cur_sz, u32 *overruns, u32 *underruns)
{
  *max_cnt   = fifo->max_cnt;
  *total_cnt = fifo->total_cnt;
  *cur_cnt   = fifo->cnt;
  *cur_sz    = fifo->sz;
  *overruns  = fifo->overruns;
  *underruns = fifo->underruns;
}

/******************************************************************************/
/******************************************************************************/
void vtss_fifo_cp_clr_statistics(vtss_fifo_cp_t *fifo)
{
  fifo->max_cnt   = 0;
  fifo->total_cnt = 0;
  fifo->overruns  = 0;
  fifo->underruns = 0;
}

