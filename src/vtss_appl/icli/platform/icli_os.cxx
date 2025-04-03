/*

 Copyright (c) 2006-2019 Microsemi Corporation "Microsemi". All Rights Reserved.

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
    > CP.Wang, 06/19/2013 15:43
        - create

==============================================================================
*/

/*
==============================================================================

    Include File

==============================================================================
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "icli_def.h"
#include "icli_os.h"
#include "icli_porting_trace.h"
#ifdef ICLI_TARGET
#include "vtss_os_wrapper.h"
#endif

#ifdef ICLI_TARGET
#include "critd_api.h"
#endif

/*
==============================================================================

    Constant and Macro

==============================================================================
*/
#ifdef ICLI_TARGET
#define ICLI_THREAD_CNT         ICLI_SESSION_CNT
#endif

/*
==============================================================================

    Type Definition

==============================================================================
*/

/*
==============================================================================

    Static Variable

==============================================================================
*/
#ifdef ICLI_TARGET

/* thread */
static vtss_handle_t          g_thread_handle[ICLI_THREAD_CNT];
static vtss_thread_t          g_thread_block[ICLI_THREAD_CNT];

/* semaphore */
static critd_t      g_critd;

#endif // ICLI_TARGET

/*
==============================================================================

    Static Function

==============================================================================
*/

/*
==============================================================================

    Public Function

==============================================================================
*/
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
)
{
    /* create semaphore */
    critd_init(&g_critd, "ICLI", VTSS_MODULE_ID_ICLI, CRITD_TYPE_MUTEX);

    return TRUE;
}
/*
    create thread
*/
BOOL icli_os_thread_create(
    IN  i32                     session_id,
    IN  const char              *name,
    IN  icli_thread_priority_t  priority,
    IN  icli_thread_entry_cb_t  *entry_cb,
    IN  i32                     entry_data
)
{
    vtss_thread_prio_t thread_prio;

    if ( session_id < 0 || session_id >= ICLI_THREAD_CNT ) {
        return FALSE;
    }

    thread_prio = VTSS_THREAD_PRIO_DEFAULT;
    if (priority == ICLI_THREAD_PRIORITY_HIGH) {
        thread_prio = VTSS_THREAD_PRIO_HIGHER;
    }

    vtss_thread_create(thread_prio,
                       (vtss_thread_entry_f *)entry_cb,
                       (vtss_addrword_t)((u64)entry_data),
                       name,
                       nullptr,
                       0,
                       &(g_thread_handle[session_id]),
                       &(g_thread_block[session_id]));

    return TRUE;
}

/*
    get my thread ID
*/
u32 icli_os_thread_id_get(
    void
)
{
    return (u32)vtss_thread_self();
}

/*
    sleep in milli-seconds
*/
void icli_os_sleep(
    IN u32  t
)
{
    VTSS_OS_MSLEEP( t );
}

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
void icli_os_sema_take(const char *const file, const int line)
{
    critd_enter(&g_critd, file, line);
}

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
void icli_os_sema_give( const char *const file, const int line)
{
    critd_exit(&g_critd, file, line);
}

#endif // ICLI_TARGET

/*
    get the time elapsed from system start in milli-seconds
*/
u32 icli_os_current_time_get(
    void
)
{
    struct timespec     tp;

    if ( clock_gettime(CLOCK_MONOTONIC, &tp) == -1 ) {
        T_E("failed to get system up time\n");
        return 0;
    }
    return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

