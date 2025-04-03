/*
 Copyright (c) 2006-2022 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef _MSTP_API_H_
#define _MSTP_API_H_

#include "vtss/appl/mstp.h"
#include "vtss_mstp_api.h"
#include "vtss_mstp_callout.h"
#include "l2proto_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(VTSS_SW_OPTION_BUILD_SMB) || defined(VTSS_BUILD_CONFIG_BRINGUP)
#define VTSS_MSTP_FULL  1
#endif

#define L2_NULL         0xffff /* Layer 2 invalid port */

mesa_rc mstp_init(vtss_init_data_t *data);
u8 vtss_mstp_msti_priority_get(mstp_msti_t msti);
mesa_rc vtss_mstp_msti_priority_set(mstp_msti_t msti, u8 priority);
mesa_rc vtss_mstp_port_config_set(vtss_isid_t isid, mesa_port_no_t port_no, BOOL enable, const mstp_port_param_t *pconf);
mesa_rc vtss_mstp_port_config_get(vtss_isid_t isid, mesa_port_no_t port_no, BOOL *enable, mstp_port_param_t *pconf);
mesa_rc vtss_mstp_msti_port_config_get(vtss_isid_t isid, mstp_msti_t msti, mesa_port_no_t port_no, mstp_msti_port_param_t *pconf);
mesa_rc vtss_mstp_msti_port_config_set(vtss_isid_t isid, mstp_msti_t msti, mesa_port_no_t port_no, const mstp_msti_port_param_t *pconf);
BOOL mstp_get_port_status(mstp_msti_t msti, l2_port_no_t l2port, mstp_port_mgmt_status_t *status);
BOOL mstp_get_port_vectors(mstp_msti_t msti, l2_port_no_t l2port, mstp_port_vectors_t *vectors);
BOOL mstp_get_port_statistics(l2_port_no_t l2port, mstp_port_statistics_t *stats, BOOL clear);

static inline int mstp_bridge2str(void *buffer, size_t size, const u8 *bridgeid)
{
    return vtss_mstp_bridge2str(buffer, size, bridgeid);
}

BOOL mstp_set_port_mcheck(l2_port_no_t l2port);

typedef void (*mstp_trap_sink_t)(u32 eventmask);

/** mstp_register_trap_sink() - (Un)-register SNMP MSTP trap event
 * callback.
 *
 * \parm cb Trap callback function to register. NULL unregister any
 * previous callback.
 *
 * \return TRUE if (un)-registration succeeded.
 *
 * \note The callback will be called at periodic intervals when an
 * MSTP event has occurred, typically within the last second. The \e
 * eventmask argument will have a bit set for each (1 <<
 * mstp_trap_event_t) having occurred.
 */
BOOL mstp_register_trap_sink(mstp_trap_sink_t cb);

typedef void (*mstp_config_change_cb_t)(void);

/** mstp_register_config_change_cb() - Register MSTP
 * configuration change callback.
 *
 * Currently not possible to unregister.
 *
 * \parm cb Configuration change callback function to register.
 *
 * \return TRUE if registration succeeded.
 *
 * \note The callback will be called when the MSTP configuration
 * changes.
 */
BOOL mstp_register_config_change_cb(mstp_config_change_cb_t cb);

const char *msti_name(vtss_appl_mstp_mstid_t msti);

char *mstp_mstimap2str(const mstp_msti_config_t *conf, vtss_appl_mstp_mstid_t msti, char *buf, size_t bufsize);

#ifdef __cplusplus
}
#endif

#endif // _MSTP_API_H_

