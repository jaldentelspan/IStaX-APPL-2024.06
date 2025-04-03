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

#ifndef _TOD_H_
#define _TOD_H_

#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_TOD
#define VTSS_TRACE_GRP_DEFAULT 0
#define VTSS_TRACE_GRP_CLOCK   1
#define VTSS_TRACE_GRP_MOD_MAN 2
#define VTSS_TRACE_GRP_PHY_TS  3
#define VTSS_TRACE_GRP_PHY_ENG 4

#define _C VTSS_TRACE_GRP_CLOCK

#define TOD_CONF_VERSION    1

#define TOD_DATA_LOCK()   critd_enter(&tod_global.datamutex, __FILE__, __LINE__)
#define TOD_DATA_UNLOCK() critd_exit (&tod_global.datamutex, __FILE__, __LINE__)

#define TOD_RC(expr) { mesa_rc _rc_ = (expr); if (_rc_ < VTSS_RC_OK) { \
        T_I("Error code: %x", _rc_); }}

#define TOD_PICO_PER_NANO 1000          //Number of pico seconds per nano second
#define TOD_FRAC_PER_NANO_SECOND 256    //Number of fractional nano seconds per nano second. In fireAnt, resolution of time is 1/256th of nano second.
#define TOD_FRAC_NANO_LIMIT 0x10000     //total 16-bits

void tod_capability_sub_nano_set(bool set);
#endif /* _TOD_H_ */

