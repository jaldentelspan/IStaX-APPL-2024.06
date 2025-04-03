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

#ifndef __VTSS_BASICS_ARITHMETIC_OVERFLOW_HXX__
#define __VTSS_BASICS_ARITHMETIC_OVERFLOW_HXX__

#include <vtss/basics/common.hxx>

namespace vtss {

uint8_t add_overflow(uint8_t a, uint8_t b, bool *overflow);
uint16_t add_overflow(uint16_t a, uint16_t b, bool *overflow);
uint32_t add_overflow(uint32_t a, uint32_t b, bool *overflow);
uint64_t add_overflow(uint64_t a, uint64_t b, bool *overflow);

int8_t  add_overflow(int8_t a, int8_t b, bool *overflow);
int16_t add_overflow(int16_t a, int16_t b, bool *overflow);
int32_t add_overflow(int32_t a, int32_t b, bool *overflow);
int64_t add_overflow(int64_t a, int64_t b, bool *overflow);

uint8_t sub_overflow(uint8_t a, uint8_t b, bool *overflow);
uint16_t sub_overflow(uint16_t a, uint16_t b, bool *overflow);
uint32_t sub_overflow(uint32_t a, uint32_t b, bool *overflow);
uint64_t sub_overflow(uint64_t a, uint64_t b, bool *overflow);

int8_t  sub_overflow(int8_t a, int8_t b, bool *overflow);
int16_t sub_overflow(int16_t a, int16_t b, bool *overflow);
int32_t sub_overflow(int32_t a, int32_t b, bool *overflow);
int64_t sub_overflow(int64_t a, int64_t b, bool *overflow);

uint8_t mult_overflow(uint8_t a, uint8_t b, bool *overflow);
uint16_t mult_overflow(uint16_t a, uint16_t b, bool *overflow);
uint32_t mult_overflow(uint32_t a, uint32_t b, bool *overflow);
uint64_t mult_overflow(uint64_t a, uint64_t b, bool *overflow);

int8_t mult_overflow(int8_t a, int8_t b, bool *overflow);
int16_t mult_overflow(int16_t a, int16_t b, bool *overflow);
int32_t mult_overflow(int32_t a, int32_t b, bool *overflow);
int64_t mult_overflow(int64_t a, int64_t b, bool *overflow);


}  // namespace vtss

#endif  // __VTSS_BASICS_ARITHMETIC_OVERFLOW_HXX__

