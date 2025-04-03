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

#ifndef _VTSS_PERSONALITY_H_
#define _VTSS_PERSONALITY_H_

#define VTSS_SWITCH_STANDALONE 1
#define VTSS_STACK_TYPE        "standalone"

// Define a default production name in case that it isn't define by the make system
#ifdef VTSS_PRODUCT_NAME
#else
#define VTSS_PRODUCT_NAME "E-Stax-34"
#endif

/* LINTLIBRARY */

#ifndef __ASSEMBLER__

/*
 * Inline Personality Functions
 */
static __inline__ int vtss_switch_stackable(void) __attribute__ ((const));
static __inline__ int vtss_switch_stackable(void)
{
    return 0;
}

static __inline__ int vtss_switch_standalone(void) __attribute__ ((const));
static __inline__ int vtss_switch_standalone(void)
{
    return VTSS_SWITCH_STANDALONE;
}

#endif  /* __ASSEMBLER__ */

#endif /* _VTSS_PERSONALITY_H_ */

