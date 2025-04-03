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

#ifndef RFC_2819_RMON_H
#define RFC_2819_RMON_H

#define RFC2819_SUPPORTED_STATISTICS   1
#define RFC2819_SUPPORTED_HISTORY      1
#define RFC2819_SUPPORTED_AlARM        1
#define RFC2819_SUPPORTED_EVENT        1

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Function declarations
 */
#if RFC2819_SUPPORTED_STATISTICS
/* statistics ----------------------------------------------------------*/
void init_rmon_statisticsMIB(void);
#endif /* RFC2819_SUPPORTED_STATISTICS */

#if RFC2819_SUPPORTED_HISTORY
/* history ----------------------------------------------------------*/
void init_rmon_historyMIB(void);
int write_historyControl(int action, u_char *var_val, u_char var_val_type,
                         size_t var_val_len, u_char *statP,
                         oid *name, size_t name_len);
#endif /* RFC2819_SUPPORTED_HISTORY */

#if RFC2819_SUPPORTED_AlARM
/* alram ----------------------------------------------------------*/
void init_rmon_alarmMIB(void);
#endif /* RFC2819_SUPPORTED_AlARM */

#if RFC2819_SUPPORTED_EVENT
/* event ----------------------------------------------------------*/
void init_rmon_eventMIB(void);

int write_eventControl(int action, u_char *var_val, u_char var_val_type,
                       size_t var_val_len, u_char *statP,
                       oid *name, size_t name_len);
#endif /* RFC2819_SUPPORTED_EVENT */

#ifdef __cplusplus
}
#endif
#endif /* RFC_2819_RMON_H */

