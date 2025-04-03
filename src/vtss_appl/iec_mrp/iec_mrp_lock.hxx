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

#ifndef _IEC_MRP_LOCK_HXX_
#define _IEC_MRP_LOCK_HXX_

#include "critd_api.h"
extern critd_t IEC_MRP_crit;

struct MRP_Lock {
    MRP_Lock(const char *file, int line)
    {
        critd_enter(&IEC_MRP_crit, file, line);
    }

    ~MRP_Lock()
    {
        critd_exit(&IEC_MRP_crit, __FILE__, 0);
    }
};

#define MRP_LOCK_SCOPE() MRP_Lock __mrp_lock_guard__(__FILE__, __LINE__)
#define MRP_LOCK_ASSERT_LOCKED(_fmt_, ...) if (!critd_is_locked(&IEC_MRP_crit)) {T_E(_fmt_, ##__VA_ARGS__);}

#endif /* _IEC_MRP_LOCK_HXX_ */

