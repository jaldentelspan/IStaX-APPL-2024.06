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

#ifndef VTSS_ALARM_ICLI_FUNCTIONS_H
#define VTSS_ALARM_ICLI_FUNCTIONS_H

#include "icli_api.h"
// #include "alarm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

icli_rc_t alarm_create_alarm(u32 session_id, const char *alarm_name,
                             const char *alarm_expression);

// note alarm_name is optional in alarm_delete_alarm
icli_rc_t alarm_delete_alarm(u32 session_id, const char *alarm_name);

icli_rc_t alarm_suppress_alarm(u32 session_id, const char *alarm_name);

icli_rc_t alarm_no_suppress_alarm(u32 session_id, const char *alarm_name);

icli_rc_t alarm_show_alarm_sources(u32 session_id, char *filter);

// note alarm_name is optional in alarm_show_alarm_status
icli_rc_t alarm_show_alarm_status(u32 session_id,
                                  const char *alarm_name);

#ifdef __cplusplus
}
#endif

#endif /* VTSS_ALARM_ICLI_FUNCTIONS_H */

