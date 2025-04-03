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


/******************************************************************************
* Types / functions defintions for LLDP-MED shared by RX and TX
******************************************************************************/

#ifndef LLDPMED_SHARED_H
#define LLDPMED_SHARED_H
#include "lldp_basic_types.h"
#include "vtss/appl/lldp.h"

#define TUDE_MULTIPLIER 10000 // Defines the number of digits for latitude, longitude ( 10 = 1 digit, 100 = 2 digits and so on ).
#define TUDE_DIGIT 4     // Keep this in sync with TUDE_MULTIPLIER
#define ALTITUDE_DIGIT 1 // Number of digits for altitude parameter. 

#define LLDPMED_LATITUDE_VALUE_MIN 0 // Defines lowest allowed value for latitude
#define LLDPMED_LATITUDE_VALUE_MAX 90 * TUDE_MULTIPLIER // Defines highest allowed value for latitude
#define LLDPMED_LONGITUDE_VALUE_MIN 0 // Defines lowest allowed value for longitude
#define LLDPMED_LONGITUDE_VALUE_MAX 180 * TUDE_MULTIPLIER // Defines highest allowed value for longitude

#define LLDPMED_ALTITUDE_VALUE_MIN_FLOAT -2097151.9
#define LLDPMED_ALTITUDE_VALUE_MAX_FLOAT 2097151.9


// Defines lowest allowed value for altitude as number with one digit (which is why we multiply with 10).
#define LLDPMED_ALTITUDE_VALUE_MIN -20971519
// Defines highest allowed value for altitude as number with one digit (which is why we multiply with 10).
#define LLDPMED_ALTITUDE_VALUE_MAX 20971519


#define LLDPMED_LONGITUDE_BIT_MASK 0x3FFFFFFFF  /* Number of valid bits in the 2's complement, TIA1057, Figure 9*/
#define LLDPMED_LATITUDE_BIT_MASK  0x3FFFFFFFF  /* Number of valid bits in the 2's complement, TIA1057, Figure 9*/
#define LLDPMED_ALTITUDE_BIT_MASK  0x3FFFFFFF   /* Number of valid bits in the 2's complement, TIA1057, Figure 9*/

// Table 12, TIA1057
typedef enum {
    LLDPMED_LOCATION_INVALID,
    LLDPMED_LOCATION_COORDINATE,
    LLDPMED_LOCATION_CIVIC,
    LLDPMED_LOCATION_ECS
} lldpmed_location_type_t;

// Location Data Format, Table 14, TIA1057
typedef enum {
    COORDINATE_BASED = 1,
    CIVIC = 2,
    ECS = 3
} lldpmed_location_data_format_t;

// LLDP-MED TLV subtype for a network connectivity, Table 5, TIA1057
typedef enum {
    LLDPMED_TLV_SUBTYPE_CAPABILITIIES  = 1,
    LLDPMED_TLV_SUBTYPE_NETWORK_POLICY = 2,
    LLDPMED_TLV_SUBTYPE_LOCATION_ID    = 3,
    LLDPMED_TLV_SUBTYPE_POE            = 4
} lldpmed_tlv_subtype_t;

// Define Policy min and max
#define LLDPMED_POLICY_NONE -1 // Defines that no policy is defined


// Defines possible actions for "global" medFastStart timer
typedef enum {
    SET,
    DECREMENT,
} lldpmed_fast_start_repeat_count_t;


const lldp_8_t *lldpmed_catype2str(vtss_appl_lldp_med_catype_t ca_type);


#define LLDPMED_POLICY_MIN 0 /**< Defines the lowest policy number*/

/** Defines the highest policy number. (I have not found anywhere where it is specified how many we shall support,
    so I have chosen 32, and the code only support lower numbers = LLDPMED_POLICY_MAX SHALL NEVER BE SET ABOVE 31).*/
#define LLDPMED_POLICY_MAX 31

u64 lldp_power_ten(u8 power);

/** Defines how many different policies the device supports.*/
#define LLDPMED_POLICIES_CNT LLDPMED_POLICY_MAX - LLDPMED_POLICY_MIN + 1

vtss_appl_lldp_med_catype_t lldpmed_index2catype(lldp_u8_t index);
lldp_u8_t   lldpmed_catype2index(vtss_appl_lldp_med_catype_t ca_type);
#endif



