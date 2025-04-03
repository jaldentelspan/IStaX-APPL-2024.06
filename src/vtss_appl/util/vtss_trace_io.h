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

#ifndef _VTSS_TRACE_IO_H_
#define _VTSS_TRACE_IO_H_

#include "vtss_trace.h"

#ifdef __cplusplus
extern "C" {
#endif
int trace_vprintf(   char *err_buf, const char *fmt, va_list ap);
int trace_rb_vprintf(char *err_buf, const char *fmt, va_list ap);

void trace_write_string(   char *err_buf, const char *str);
void trace_rb_write_string(char *err_buf, const char *str);

void trace_write_string_len(   char *err_buf, const char *str, unsigned length);
void trace_rb_write_string_len(char *err_buf, const char *str, unsigned length);

void trace_write_char(char *err_buf, char c);
void trace_rb_write_char(char *err_buf, char c);

void trace_flush(void);

#ifdef __cplusplus
}
#endif
#endif /* _VTSS_TRACE_IO_H_ */

