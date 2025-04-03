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

#ifndef _SYMREG_STANDALONE_H_
#define _SYMREG_STANDALONE_H_

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <main_types.h>
#include <vtss/appl/module_id.h>

#define VTSS_BF_SIZE(n)      ((n + 7) / 8)
#define VTSS_BF_GET(a, n)    (((a)[(n) / 8] & (1 << ((n) % 8))) ? 1 : 0)
#define VTSS_BF_SET(a, n, v) { if (v) { a[(n) / 8] |= (1U << ((n) % 8)); } else { a[(n) / 8] &= ~(1U << ((n) % 8)); }}
#define VTSS_BF_CLR(a, n)    (memset(a, 0, VTSS_BF_SIZE(n)))
#define VTSS_MALLOC(_sz_) malloc(_sz_)
#define VTSS_FREE(_ptr_)  free(_ptr_)
#define ARRSZ(_x_)  (sizeof(_x_) / sizeof((_x_)[0]))

void symreg_printf(BOOL debug, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
#define T_N(_fmt, ...) symreg_printf(TRUE,  _fmt, ##__VA_ARGS__)
#define T_D(_fmt, ...) symreg_printf(TRUE,  _fmt, ##__VA_ARGS__)
#define T_E(_fmt, ...) symreg_printf(FALSE, _fmt, ##__VA_ARGS__)

mesa_rc mgmt_txt2bf(char *buf, BOOL *list, u32 min, u32 max, BOOL def);
char *mgmt_bf2txt(BOOL *list, int min, int max, char *buf);

#endif /* _SYMREG_STANDALONE_H_ */

