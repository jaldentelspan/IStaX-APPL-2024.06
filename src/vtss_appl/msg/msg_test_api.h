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

#ifndef _VTSS_MSG_TEST_API_H_
#define _VTSS_MSG_TEST_API_H_

#include "main.h"

/****************************************************************************/
//
//                   MESSAGE MODULE TEST API.
//
/****************************************************************************/

/****************************************************************************/
// msg_test_suite()
// If @num == 0 or out of bounds, it prints the defined tests using
// @print_function..
// If @num > 0 and within bounds, it executes the test with the argument
// given by @arg.
// This should be called from CLI, only, and is (currently) independent of
// the current stack state, i.e. the same messages are sent whether or not
// the switch is the primary swiuch.
// Only one test can be in progress at a time.
/****************************************************************************/
void msg_test_suite(u32 num, u32 arg0, u32 arg1, u32 arg2, int (*print_function)(const char *fmt, ...));

/****************************************************************************/
// msg_test_init()
// Message Test Module initialization function.
/****************************************************************************/
mesa_rc msg_test_init(vtss_init_data_t *data);

#endif // _VTSS_MSG_TEST_API_H_

