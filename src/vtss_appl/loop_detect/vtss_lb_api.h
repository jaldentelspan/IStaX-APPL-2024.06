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

#ifdef __cplusplus
extern "C" {
#endif

// Callback type
typedef void (*vtss_lb_callback_t)(void);

// Callback function for registering for "magic packet"
// Do not attempt to call any other functions from this module
// when inside the callback function.
void vtss_lb_callback_register(vtss_lb_callback_t cb, int vtss_type) ;

// Function for unregistering the callback function.
void vtss_lb_callback_unregister(int vtss_type);

/* Return whether port is (actively) being monitored by this module */
BOOL vtss_lb_port(mesa_port_no_t port_no);

// Function for transmitting a OSSP frame with user defined vtsss_type.
void vtss_lb_tx_ossp_pck(int vtss_type);

/* Initialize module */
mesa_rc vtss_lb_init(vtss_init_data_t *data);

#ifdef __cplusplus
}
#endif

