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

#include <stdint.h>
#include "vtss/basics/api_types.h"

#ifndef __EXAMPLE_TEST_API_H__
#define __EXAMPLE_TEST_API_H__

#ifdef __cplusplus
extern "C" {
#endif

struct ab_t {
    int32_t a;
    int32_t b;
};

struct xyz_t {
    ab_t x;
    ab_t y;
    ab_t z;
};

typedef enum {
    val_a,
    val_b = 3
} enumTestType_t;

typedef enum {
    val_x,
    val_y 
} enumTableType_t;

typedef enum {
    val_1,
    val_2 
} enumTable2Type_t;

typedef struct {
    int32_t sub1;
    int32_t sub2;
    enumTestType_t sub3;
    bool aBoolean;
    uint32_t asBoolean;
} SubConf;

typedef struct {
    uint32_t a;
    int32_t c;
}SomeConf;

typedef struct {
    int32_t d;
    enumTestType_t e;
    int32_t f;
} SomeStatus;

typedef struct {
    int32_t g;
    int32_t h;
    int32_t i;
} SomeDetailedStatus;

typedef struct {
    mesa_ipv4_t anIpAddress;
    char ds[32];
    uint8_t bitMappedString[30];
    uint8_t octetstring[32];
    mesa_mac_t macAddress;
} Abc_t;

typedef struct {
    int32_t d;
    enumTable2Type_t e;
    int32_t f;
} Table2Status;

mesa_rc vtss_test_xyz_get(xyz_t *conf);
mesa_rc vtss_test_xyz_set(const xyz_t *conf);

mesa_rc vtss_test_some_conf_get(SomeConf *conf);
mesa_rc vtss_test_some_conf_set(const SomeConf *conf);

mesa_rc vtss_test_sub_conf_get(SubConf *conf);
mesa_rc vtss_test_sub_conf_set(const SubConf *conf);

mesa_rc vtss_test_some_status_get(SomeStatus *status);
mesa_rc vtss_test_some_status2_get(SomeDetailedStatus *status);

mesa_rc vtss_abc_get(Abc_t *abc);
mesa_rc vtss_abc_set(const Abc_t *abc);

// TABLE EXAMPLE 0 /////////////////////////////////////////////////////////////
mesa_rc vtss_table0_get(mesa_vid_t vid,        // TABLE KEY
                        SomeStatus *s);        // TABLE VALUE

mesa_rc vtss_table0_set(mesa_vid_t vid,        // TABLE KEY
                        const SomeStatus *s);  // TABLE VALUE


// TABLE EXAMPLE 1 /////////////////////////////////////////////////////////////
mesa_rc vtss_table1_get(mesa_vid_t         vid,    // TABLE KEY
                        SomeStatus        *s);     // TABLE VALUE

mesa_rc vtss_table1_set(mesa_vid_t         vid,    // TABLE KEY
                        const SomeStatus  *s);     // TABLE VALUE

mesa_rc vtss_table1_add(mesa_vid_t         vid,    // TABLE KEY
                        const SomeStatus  *s);     // TABLE VALUE

mesa_rc vtss_table1_del(mesa_vid_t         vid);   // TABLE KEY

mesa_rc vtss_table1_itr(const mesa_vid_t *last,    // KEY LAST
                        mesa_vid_t       *next);   // KEY NEXT

// TABLE EXAMPLE 2 /////////////////////////////////////////////////////////////
mesa_rc vtss_table2_get(mesa_vid_t vid,        // TABLE KEY
                        mesa_vid_t vid_inner,  // TABLE KEY
                        Table2Status *s,       // TABLE VALUE
                        int *i);               // TABLE VALUE

mesa_rc vtss_table2_set(mesa_vid_t vid,        // TABLE KEY
                        mesa_vid_t vid_inner,  // TABLE KEY
                        const Table2Status *s, // TABLE VALUE
                        const int *i);         // TABLE VALUE

mesa_rc vtss_table2_add(mesa_vid_t vid,        // TABLE KEY
                        mesa_vid_t vid_inner,  // TABLE KEY
                        const Table2Status *s, // TABLE VALUE
                        const int *i);         // TABLE VALUE

mesa_rc vtss_table2_del(mesa_vid_t vid,        // TABLE KEY
                        mesa_vid_t vid_inner); // TABLE KEY

mesa_rc vtss_table2_itr(const mesa_vid_t *last_vid1, mesa_vid_t *next_vid1,
                        const mesa_vid_t *last_vid2, mesa_vid_t *next_vid2);

// TABLE EXAMPLE 3 /////////////////////////////////////////////////////////////
mesa_rc vtss_table3_get(mesa_vid_t         vid,    // TABLE KEY
                        SomeStatus        *s);     // TABLE VALUE

mesa_rc vtss_table3_set(mesa_vid_t         vid,    // TABLE KEY
                        const SomeStatus  *s);     // TABLE VALUE

mesa_rc vtss_table3_del(mesa_vid_t         vid);   // TABLE KEY

mesa_rc vtss_table3_itr(const mesa_vid_t *last,    // KEY LAST
                        mesa_vid_t       *next);   // KEY NEXT

mesa_rc leaf_12_get(uint32_t *a, uint32_t *b);
mesa_rc leaf_13_get(uint32_t *a, uint32_t *b);
mesa_rc leaf_13_set(uint32_t  a, uint32_t  b);

#ifdef __cplusplus
}
#endif

#endif  // __EXAMPLE_TEST_API_H__

