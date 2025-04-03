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

#ifndef _VTSS_WEBSTAX_OPTIONS_H_
#define _VTSS_WEBSTAX_OPTIONS_H_

/* ================================================================= *
 *  Unmanaged module options
 * ================================================================= */

/* VeriPHY during initialization */
#ifndef VTSS_UNMGD_OPT_VERIPHY
#define VTSS_UNMGD_OPT_VERIPHY    0
#endif /* VTSS_UNMGD_OPT_VERIPHY */

/* Flow control setup via GPIO14 and LED control via GPIO8 */
#ifndef VTSS_UNMGD_OPT_FC_GPIO
#define VTSS_UNMGD_OPT_FC_GPIO    0
#endif /* VTSS_UNMGD_OPT_FC_GPIO */

/* Aggregation setup via GPIO12/GPIO13 and LED control via GPIO4/GPIO5/GPIO6 */
#ifndef VTSS_UNMGD_OPT_AGGR_GPIO
#define VTSS_UNMGD_OPT_AGGR_GPIO  0
#endif /* VTSS_UNMGD_OPT_AGGR_GPIO */

/* HDMI CABLE detect via GPIO2 and GPIO3 */
#ifndef VTSS_UNMGD_OPT_HDMI_GPIO
#define VTSS_UNMGD_OPT_HDMI_GPIO    0
#endif /* VTSS_UNMGD_OPT_FC_GPIO */


/* Serial LED for VeriPHY and port status */
#ifndef VTSS_UNMGD_OPT_SERIAL_LED
#define VTSS_UNMGD_OPT_SERIAL_LED 0
#endif /* VTSS_UNMGD_OPT_SERIAL_LED */

/* Firmware upgrade */
#ifndef VTSS_UNMGD_OPT_SWUP
#define VTSS_UNMGD_OPT_SWUP 0
#endif /* VTSS_UNMGD_OPT_SERIAL_LED */

/* ================================================================= *
 *  User interface module options
 * ================================================================= */

/* VeriPHY supported in user interface */
#ifndef VTSS_UI_OPT_VERIPHY
#define VTSS_UI_OPT_VERIPHY 1
#endif /* VTSS_UI_OPT_VERIPHY */

#endif /* _VTSS_WEBSTAX_OPTIONS_H_ */

