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

/*
==============================================================================

    Revision history
    > CP.Wang, 06/19/2013 15:58
        - create

==============================================================================
*/
#ifndef __VTSS_ICLI_OS_H__
#define __VTSS_ICLI_OS_H__
//****************************************************************************
/*lint --e{429,454,455,456,459} */
/*lint -sem(icli_os_sema_take, thread_lock)   */
/*lint -sem(icli_os_sema_give, thread_unlock) */
/*
==============================================================================

    Include File

==============================================================================
*/
#include "icli_os_misc.h"

/*
==============================================================================

    Constant

==============================================================================
*/


/*
==============================================================================

    Macro

==============================================================================
*/
#define _ICLI_SEMA_TAKE()     icli_os_sema_take(__FILE__, __LINE__)
#define _ICLI_SEMA_GIVE()     icli_os_sema_give(__FILE__, __LINE__)

/*
==============================================================================

    Type

==============================================================================
*/
typedef i32 icli_thread_entry_cb_t(
    IN i32  data
);

typedef enum {
    ICLI_THREAD_PRIORITY_NORMAL,
    ICLI_THREAD_PRIORITY_HIGH,
} icli_thread_priority_t;

/*
==============================================================================

    Macro Definition

==============================================================================
*/

/*
==============================================================================

    Public Function

==============================================================================
*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef ICLI_TARGET

/**
 * \brief
 *      initialization
 *
 * \param
 *      n/a
 *
 * \return
 *      TRUE  - successful
 *      FALSE - failed
 */
BOOL icli_os_init(
    void
);

/*
    create thread
*/
BOOL icli_os_thread_create(
    IN  i32                     session_id,
    IN  const char              *name,
    IN  icli_thread_priority_t  priority,
    IN  icli_thread_entry_cb_t  *entry_cb,
    IN  i32                     entry_data
);

/*
    get my thread ID
*/
u32 icli_os_thread_id_get(
    void
);

/*
    sleep in milli-seconds
*/
void icli_os_sleep(
    IN u32  t
);

/**
 * \brief
 *      take semaphore
 *
 * \param
 *      file [IN]: file name
 *      line [IN]: line number
 *
 * \return
 *      n/a.
 */
void icli_os_sema_take(
    const char *const   file,
    const int           line
);

/**
 * \brief
 *      give semaphore
 *
 * \param
 *      file [IN]: file name
 *      line [IN]: line number
 *
 * \return
 *      n/a.
 */
void icli_os_sema_give(
    const char *const   file,
    const int           line
);

#endif // ICLI_TARGET

/*
    get the time elapsed from system start in milli-seconds
*/
u32 icli_os_current_time_get(
    void
);

#ifdef __cplusplus
}
#endif

//****************************************************************************
#endif //__VTSS_ICLI_OS_H__

