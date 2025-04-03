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
#include "../base/vtss_xxrp_callout.h"
#include "vtss_xxrp_api.h" /* For definition of VTSS_MRP_APPL_MAX */
#include "xxrp_api.h"
#include "xxrp_trace.h"

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_XXRP

static xxrp_mgmt_mem_stat_t xxrp_mem_stat;

void *xxrp_sys_malloc(u32 size, const char *file, const char *function, u32 line)
{
    void *ptr = VTSS_MALLOC(size);
    if (ptr != NULL) {
        xxrp_mem_stat.alloc_cnt++;
        xxrp_mem_stat.alloc_size += size;
    }

    return ptr;
}

mesa_rc xxrp_sys_free(void *ptr, const char *file, const char *function, u32 line)
{
    if (!ptr) {
        return XXRP_ERROR_INVALID_PARAMETER;
    }
    VTSS_FREE(ptr);
    xxrp_mem_stat.free_cnt++;

    return VTSS_RC_OK;
}

void xxrp_mgmt_mem_stat_get(xxrp_mgmt_mem_stat_t &mem_stat)
{
    mem_stat = xxrp_mem_stat;

    return;
}
