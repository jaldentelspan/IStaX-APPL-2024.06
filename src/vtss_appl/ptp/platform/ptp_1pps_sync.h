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

#ifndef _PTP_1PPS_SYNC_
#define _PTP_1PPS_SYNC_
/******************************************************************************/
//
// This header file contains various definitions and functions for
// implementing the 1pps synchronization feature supported bu the Gen2
// PHY's.
//
//
/******************************************************************************/



/*
 * 1PPS synchronization mode.
 */
typedef enum {
    VTSS_PTP_1PPS_SYNC_MAIN_MAN,    /* Main function with manual setting of the pulse delay*/
    VTSS_PTP_1PPS_SYNC_MAIN_AUTO,   /* Main function with cable delay measurement*/
    VTSS_PTP_1PPS_SYNC_SUB,         /* Sub function  */
    VTSS_PTP_1PPS_SYNC_DISABLE,
} vtss_1pps_sync_mode_t;

/*
 * Serial TOD mode.
 */
typedef enum {
    VTSS_PTP_1PPS_SER_TOD_MAN,      /* Enable Serial TOD without auto load, i.e. phase adjustment is done in SW */
    VTSS_PTP_1PPS_SER_TOD_AUTO,     /* Enable Serial TOD with auto load */
    VTSS_PTP_1PPS_SER_TOD_DISABLE,  /* Disable Serial TOD */
} vtss_1pps_ser_tod_mode_t;

/*
 * 1PPS synchronization configuration.
 */
typedef struct {
    vtss_1pps_sync_mode_t    mode;           /* 1pps sync mode */
    u32                      pulse_delay;    /* manually entered pulse delay used in manual mode */
    u32                      cable_asy;      /* manually entered cable asymmetry used in auto mode */
    vtss_1pps_ser_tod_mode_t serial_tod;     /* serial TOD mode */
} vtss_1pps_sync_conf_t;


/******************************************************************************/
// Interface functions.
/******************************************************************************/

/**
 * \brief Function for configuring the 1PPS node synchronization using the Gen2 PHY's
 *
 * \param port_no       [IN]  Interface port number to be configured.
 * \param conf          [IN]  The configuration parameters.
 * \return Errorcode.   PTP_RC_MISSING_PHY_TIMESTAMP_RESOURCE (itf the port does not support Gen2 features
 *                      Errorcode from the phy_ts_api
 **/
mesa_rc ptp_1pps_sync_mode_set(mesa_port_no_t port_no, const vtss_1pps_sync_conf_t *conf);

/**
 * \brief Function to get the configuration of the 1PPS node synchronization using the Gen2 PHY's
 *
 * \param port_no       [IN]  Interface port number to be configured.
 * \param conf          [OUT]  The configuration parameters.
 * \return Errorcode.   PTP_RC_MISSING_PHY_TIMESTAMP_RESOURCE (itf the port does not support Gen2 features
 *                      Errorcode from the phy_ts_api
 **/
mesa_rc ptp_1pps_sync_mode_get(mesa_port_no_t port_no, vtss_1pps_sync_conf_t *conf);

/**
 * \brief Function for initializing the 1PPS node synchronization using the Gen2 PHY's
 *
 * \return none
 **/
mesa_rc ptp_1pps_sync_init(void);

#endif /* _PTP_1PPS_SYNC_ */

