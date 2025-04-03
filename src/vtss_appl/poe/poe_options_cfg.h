/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _POE_OPTIONS_CFG_H_
#define _POE_OPTIONS_CFG_H_

#include "poe_api.h"

// *  Least port power priority:     VTSS_APPL_POE_PORT_POWER_PRIORITY_LOW,
// *  Medium port power priority:    VTSS_APPL_POE_PORT_POWER_PRIORITY_HIGH,
// *  Highest port power priority:   VTSS_APPL_POE_PORT_POWER_PRIORITY_CRITICAL
#define POE_PRIORITY_DEFAULT                 VTSS_APPL_POE_PORT_POWER_PRIORITY_LOW


// *  VTSS_APPL_POE_PSE_PORT_TYPE3_15W:  limit up to class 3 15W
// *  VTSS_APPL_POE_PSE_PORT_TYPE3_30W:  limit up to class 4 30W
// *  VTSS_APPL_POE_PSE_PORT_TYPE3_60W:  limit up to class 6 60W
// *  VTSS_APPL_POE_PSE_PORT_TYPE4_90W:  limit up to class 4 90W
#define POE_TYPE_DEFAULT                     VTSS_APPL_POE_PSE_PORT_TYPE4_90W


// *  Disable PoE data in LLDP PDUs: VTSS_APPL_POE_PORT_LLDP_DISABLED
// *  Enable PoE data in LLDP PDUs:  VTSS_APPL_POE_PORT_LLDP_ENABLED 
#define POE_LLDP_DISABLE_DEFAULT             VTSS_APPL_POE_PORT_LLDP_ENABLED


// * PoE functionality is disabled:   
//      VTSS_APPL_POE_MODE_DISABLED
// * Enables PoE based on IEEE 802.3af standard, and provides power up to 15.4W(or 154 deciwatt) of DC power to powered device:
//      VTSS_APPL_POE_MODE_POE
// * Enabled PoE based on IEEE 802.3at standard,and provides power up to 30W(or 300 deciwatt) of DC power to powered device:
//      VTSS_APPL_POE_MODE_POE_PLUS
#define POE_MODE_DEFAULT                     VTSS_APPL_POE_MODE_POE_PLUS


// * The port power that is used for power management purposes is dynamic (Iport x Vmain):
//      VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC
// * The port power that is used for power management purposes is port TPPL_BT:
//      VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_STATIC
// * The port power that is used for power management purposes is dynamic for non LLDP/CDP/Autoclass ports and TPPL_BT for LLDP/CDP/Autoclass ports:
//      VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_HYBRID
#define VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DEFAULT        VTSS_APPL_POE_BT_PORT_POWER_MANAGEMENT_DYNAMIC


// Respect PD report for supporting Auto Class and perform power measurement or ignore and use PoE-BT class
// * 0 = Set port max power as per PD class (0-8)
// * 1 = Set port max power as per PD max power consumption during negotiation
#define POE_PD_AUTO_CLASS_REQUEST_DEFAULT    0


// Should IStaX PoE module uppon start-up perform 2 Sec RESET (PoE power off) for all ports or leave PoE power unchanged
// * 0 = Leave PoE power unchanged upon power up
// * 1 = Reset PoE port power for 2 Sec uppon power up
#define POE_PD_INTERRUPTIBLE_POWER_DEFAULT   0
          

//    /** legacy . */
//    VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_STANDARD,
//    /** Medium port power priority. */
//    VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_POH,
//    /** Highest port power priority. */
//    VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_IGNORE_PD_CLASS
#define POE_LEGACY_PD_CLASS_MODE_DEFAULT     VTSS_APPL_POE_LEGACY_PD_CLASS_MODE_STANDARD



//    /** cable length till 10 meters. */
//    VTSS_APPL_POE_PORT_CABLE_LENGTH_10 = 0,
//    /** cable length till 30 meters.  */
//    VTSS_APPL_POE_PORT_CABLE_LENGTH_30,
//    /** cable length till 60 meters.  */
//    VTSS_APPL_POE_PORT_CABLE_LENGTH_60,
//    /** cable length till 100 meters. */
//    VTSS_APPL_POE_PORT_CABLE_LENGTH_100
#define POE_CABLE_LENGTH_DEFAULT             VTSS_APPL_POE_PORT_CABLE_LENGTH_100



// would the disabled link operation will disable the PoE power as well
// * 0 = Leave PoE power unchanged
// * 1 = disable the PoE power if link was disabled
#define DISABLE_LINK_ALSO_DISABLE_POE         0



// use parameters: Mumber of PoE ports, power_supply_max_power_w and eMeba_poe_firmware_type (PREBT/BT) from h file or from Product process
#define USE_POE_STATIC_PARAMETERS


#endif //_POE_OPTIONS_CFG_H_

