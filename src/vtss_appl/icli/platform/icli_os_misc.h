/*

 Copyright (c) 2006-2018 Microsemi Corporation "Microsemi". All Rights Reserved.

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
    > CP.Wang, 2015-01-27 17:42
        - create

==============================================================================
*/
#ifndef __VTSS_ICLI_OS_MISC_H__
#define __VTSS_ICLI_OS_MISC_H__
//****************************************************************************
/*
==============================================================================

    Include File

==============================================================================
*/

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

/*
==============================================================================

    Type

==============================================================================
*/

/*
==============================================================================

    Macro Definition

==============================================================================
*/
#define icli_sprintf        sprintf
#define icli_snprintf       snprintf
#define icli_vsnprintf      vsnprintf
#define icli_close_socket   close
#ifdef ICLI_TARGET
#define icli_malloc(_sz_)   VTSS_MALLOC_MODID(VTSS_MODULE_ID_ICLI, _sz_, __FILE__, __LINE__)
#define icli_calloc(_n_, _sz_) VTSS_CALLOC_MODID(VTSS_MODULE_ID_ICLI, _n_, _sz_, __FILE__, __LINE__)
#define icli_free           VTSS_FREE
#else
#define icli_malloc         malloc
#define icli_calloc         calloc
#define icli_free           free
#endif

/*
==============================================================================

    Public Function

==============================================================================
*/

//****************************************************************************
#endif //__VTSS_ICLI_OS_MISC_H__

