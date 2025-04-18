/*
 Copyright (c) 2006-2021 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "eee.h"                       /* For internal defs and vars */
#include "eee_api.h"                   /* For our own public defs    */
#include <microchip/ethernet/switch/api.h>  /* For vtss_phy_eee_ena       */

//************************************************
// Global Variables
//************************************************
static mesa_port_list_t conf_changed;
static volatile BOOL conf_ready;

/******************************************************************************/
// EEE_switch_api_set();
// Function for setting EEE configuration in the switch API.
//
// In : eee_conf - the EEE configuration
//      iport    - The port that shall be configured
//
// Return : If API access went well then return VTSS_RC_OK, else return error code
/******************************************************************************/
static mesa_rc eee_switch_api_set(mesa_port_no_t iport)
{
    mesa_eee_port_conf_t eee_port_conf;

    eee_port_conf.eee_ena          = EEE_local_conf.port[iport].eee_ena;
    eee_port_conf.optimized_for_power = EEE_global_conf.optimized_for_power;
#if EEE_FAST_QUEUES_CNT > 0
    eee_port_conf.eee_fast_queues  = EEE_local_conf.port[iport].eee_fast_queues;
#endif
    eee_port_conf.tx_tw            = EEE_local_state.port[iport].LocResolvedTxSystemValue;
    eee_port_conf.lp_advertisement = EEE_local_state.port[iport].link_partner_eee_capable;

    // Setup EEE wake-up times etc. in switch API
    VTSS_RC(mesa_eee_port_conf_set(NULL, iport, &eee_port_conf));

    return VTSS_RC_OK;
}

/******************************************************************************/
// EEE_platform_thread()
/******************************************************************************/
void EEE_platform_thread(void)
{
    mesa_port_no_t iport;
    mesa_port_list_t enable_lpi;

    // ***** Go into loop **** //
    T_R("Entering eee_thread");

    // Initialise conf_changed/enable_lpi
    for (iport = VTSS_PORT_NO_START; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
        conf_changed[iport] = TRUE;
        enable_lpi[iport]   = FALSE;
    }

    // First time we exit our mutex.
    /*lint -e(455) */
    EEE_CRIT_EXIT();

    while (!conf_ready) {
        VTSS_OS_MSLEEP(1000);
    }

    while (1) {
        for (iport = VTSS_PORT_NO_START; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
            // If port supports EEE then set enable according to user settings else disable.
            if (!EEE_local_state.port[iport].eee_capable) {
                continue;
            }

            EEE_CRIT_ENTER();
            if (conf_changed[iport] && EEE_local_state.port[iport].running) {
                conf_changed[iport] = FALSE;
                // If link has just come up, then configure EEE for the port with default configuration
                if (eee_switch_api_set(iport) != VTSS_RC_OK) {
                    T_E("Could not set EEE in swtich API");
                }

                // See comment at the 1 second sleep below.
                // PHY register 17E2 (EEE Control):
                // bit 3 == 1: Disable transmission of EEE LPI on transmit path MDI in 100BASE-TX mode when receiving LPI from MAC.
                // bit 1 == 1: Disable transmission of EEE LPI on transmit path MDI in 1000BASE-T mode when receiving LPI from MAC.
                (void)vtss_phy_write_masked_page(NULL, iport, 2, 17, 0xA, 0x000A);
                enable_lpi[iport] = TRUE;
            }
            EEE_CRIT_EXIT();
        }

        // Going into LPI will stress the DSP startup as it is not entirely complete when link status becomes good and continues to train afterward.
        // This requirement is noted in clause 22.6a.1 (for 100BASE-TX), 35.3a.1 (for 1000BASE-T), and 78.1.2.1.2 (for EEE as a whole).
        // For EEE and it is highly desirable to have the 1 second delay from link coming up prior to sending LPI.
        // This can be done using PHY registers to block sending LPI when the link is down and only unblock it 1 second after
        // the link comes up and then reblock it when the link goes down.
        // This can be blocked using extended-page 2, register 17 by setting bits 1 and 3. Then clear bits 1 and 3 a second after the link comes up.
        VTSS_OS_MSLEEP(1000);
        for (iport = VTSS_PORT_NO_START; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
            EEE_CRIT_ENTER();
            if (EEE_local_state.port[iport].running) {
                if (enable_lpi[iport]) {
                    enable_lpi[iport] = FALSE;
                    // PHY register 17E2 (EEE Control):
                    // bit 3 == 0: Enable transmission of EEE LPI on transmit path MDI in 100BASE-TX mode when receiving LPI from MAC.
                    // bit 1 == 0: Enable transmission of EEE LPI on transmit path MDI in 1000BASE-T mode when receiving LPI from MAC.
                    (void)vtss_phy_write_masked_page(NULL, iport, 2, 17, 0x0, 0x000A);
                }
            } else if (EEE_local_state.port[iport].eee_capable)  {
                (void)vtss_phy_write_masked_page(NULL, iport, 2, 17, 0xA, 0x000A);
            }
            EEE_CRIT_EXIT();
        }
    }
}

/******************************************************************************/
//
// SEMI-PUBLIC FUNCTIONS, I.E. INTERFACE TOWARDS PLATFORM-INDEPENDENT CODE.
//
/******************************************************************************/

// Callback function for when a port changes state.
void EEE_platform_port_link_change_event(mesa_port_no_t iport)
{
    EEE_CRIT_ASSERT_LOCKED();
    conf_changed[iport] = TRUE;
}

/******************************************************************************/
// EEE_platform_local_conf_set()
// Configuration received from primary switch.
// PHY advertisement already configured.
/******************************************************************************/
void EEE_platform_local_conf_set(mesa_port_list_t &port_changes)
{
    mesa_port_no_t iport;

    EEE_CRIT_ENTER();
    for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
        if (port_changes[iport]) {
            conf_changed[iport] = TRUE;

            // Setup switch API
            (void)eee_switch_api_set(iport);
        }
    }
    conf_ready = TRUE;
    EEE_CRIT_EXIT();
}

/******************************************************************************/
// EEE_platform_conf_reset()
/******************************************************************************/
void EEE_platform_conf_reset(eee_switch_conf_t *conf)
{
    // Nothing to be done (#conf already reset by caller)
}

/******************************************************************************/
// EEE_platform_conf_valid()
/******************************************************************************/
BOOL EEE_platform_conf_valid(eee_switch_conf_t *conf)
{
    // FJTBD
    return TRUE;
}

/******************************************************************************/
// EEE_platform_tx_wakeup_time_changed()
/******************************************************************************/
void EEE_platform_tx_wakeup_time_changed(mesa_port_no_t port, u16 LocResolvedTxSystemValue)
{
    EEE_CRIT_ASSERT_LOCKED();
    (void)eee_switch_api_set(port);
}

/******************************************************************************/
// EEE_platform_init()
/******************************************************************************/
void EEE_platform_init(vtss_init_data_t *data)
{
    // Nothing to be done additionally on this platform.
}

