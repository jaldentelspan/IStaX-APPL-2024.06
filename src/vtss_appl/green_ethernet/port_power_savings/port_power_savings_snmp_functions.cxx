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

#include "vtss/appl/port_power_savings.h"
#include "vtss/appl/port.h"

#undef VTSS_TRACE_MODULE_ID // We use the port module trace in this file
#include "port_trace.h"     // For trace

/**
 * Set Port power saving configuration.
 * ifIndex  [IN]: Interface index
 * conf     [IN]: Port power saving configurable parameters
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_port_power_saving_conf_set(vtss_ifindex_t                            ifIndex,
                                             const vtss_appl_port_power_saving_conf_t  *const conf)
{
#ifdef VTSS_SW_OPTION_PHY_POWER_CONTROL
    vtss_appl_port_conf_t    port_conf;
    vtss_appl_port_status_t  port_status;

    if (conf == NULL) {
        T_I("conf == NULL");
        return VTSS_RC_ERROR;
    }

    // Get port configuration
    VTSS_RC(vtss_appl_port_conf_get(ifIndex, &port_conf));

    VTSS_RC(vtss_appl_port_status_get(ifIndex, &port_status));

    if (conf->energy_detect) {
        //Set the actiphy bit
        port_conf.power_mode = (vtss_phy_power_mode_t)(VTSS_PHY_POWER_ACTIPHY | port_conf.power_mode);
    } else {
        //Clear the actiphy bit
        port_conf.power_mode = (vtss_phy_power_mode_t)(port_conf.power_mode & (~VTSS_PHY_POWER_ACTIPHY));
    }

    if (!port_status.power.actiphy_capable) {
        if ((port_conf.power_mode & VTSS_PHY_POWER_ACTIPHY) >> VTSS_PHY_POWER_ACTIPHY_BIT) {
            T_D("Error, link partner detection not supported");
            return VTSS_RC_ERROR;
        }
    }

    if (conf->short_reach) {
        //Set the PerfectReach bit
        port_conf.power_mode = (vtss_phy_power_mode_t)(port_conf.power_mode | VTSS_PHY_POWER_DYNAMIC);
    } else {
        //Clear the PerfectReach bit
        port_conf.power_mode = (vtss_phy_power_mode_t)(port_conf.power_mode & (~VTSS_PHY_POWER_DYNAMIC));
    }

    if (!port_status.power.perfectreach_capable) {
        if ((port_conf.power_mode & VTSS_PHY_POWER_DYNAMIC) >> VTSS_PHY_POWER_DYNAMIC_BIT) {
            T_D("Error, short reach not supported on port: %u", VTSS_IFINDEX_PRINTF_ARG(ifIndex));
            return VTSS_RC_ERROR;
        }
    }
    VTSS_RC(vtss_appl_port_conf_set(ifIndex, &port_conf));
    return VTSS_RC_OK;
#endif
    return VTSS_RC_ERROR;
}
/**
 * Get Port power saving configuration.
 * ifIndex  [IN] : Interface index
 * conf     [OUT]: Port power saving configurable parameters
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_port_power_saving_conf_get(vtss_ifindex_t                     ifIndex,
                                             vtss_appl_port_power_saving_conf_t *const conf)
{
#ifdef VTSS_SW_OPTION_PHY_POWER_CONTROL
    vtss_appl_port_conf_t    port_conf;

    if (conf == NULL) {
        T_I("conf == NULL");
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_appl_port_conf_get(ifIndex, &port_conf));

    conf->energy_detect = (port_conf.power_mode & VTSS_PHY_POWER_ACTIPHY) >> VTSS_PHY_POWER_ACTIPHY_BIT;
    conf->short_reach   = (port_conf.power_mode & VTSS_PHY_POWER_DYNAMIC) >> VTSS_PHY_POWER_DYNAMIC_BIT;
    return VTSS_RC_OK;
#endif
    return VTSS_RC_ERROR;
}
/**
 * Get Port power saving current status
 * ifIndex     [IN]: Interface index
 * status     [OUT]: Port power saving status
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_port_power_saving_status_get(vtss_ifindex_t                       ifIndex,
                                               vtss_appl_port_power_saving_status_t *const status)
{
#ifdef VTSS_SW_OPTION_PHY_POWER_CONTROL
    vtss_appl_port_status_t  port_status;

    if (status == NULL) {
        T_I("status == NULL");
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_appl_port_status_get(ifIndex, &port_status));

    if (port_status.power.actiphy_capable) {
        status->energy_detect_power_savings = port_status.power.actiphy_power_savings ?
                                              VTSS_APPL_PORT_POWER_SAVING_STATUS_YES :
                                              VTSS_APPL_PORT_POWER_SAVING_STATUS_NO;
    } else {
        status->energy_detect_power_savings = VTSS_APPL_PORT_POWER_SAVING_NOT_SUPPORTED;
    }

    if (port_status.power.perfectreach_capable) {
        status->short_reach_power_savings = port_status.power.perfectreach_power_savings ?
                                            VTSS_APPL_PORT_POWER_SAVING_STATUS_YES :
                                            VTSS_APPL_PORT_POWER_SAVING_STATUS_NO;
    } else {
        status->short_reach_power_savings = VTSS_APPL_PORT_POWER_SAVING_NOT_SUPPORTED;
    }
    return VTSS_RC_OK;
#endif
    return VTSS_RC_ERROR;
}
/**
 * Get Platform specific port power saving capabilities
 * ifIndex       [IN]: Interface index
 * capabilities [OUT]: Platform specific port power saving capabilities
 * return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_port_power_saving_capabilities_get(vtss_ifindex_t                             ifIndex,
                                                     vtss_appl_port_power_saving_capabilities_t *const cap)
{
#ifdef VTSS_SW_OPTION_PHY_POWER_CONTROL
    vtss_appl_port_status_t  port_status;

    if (cap == NULL) {
        T_I("cap == NULL");
        return VTSS_RC_ERROR;
    }

    VTSS_RC(vtss_appl_port_status_get(ifIndex, &port_status));

    cap->energy_detect_capable = port_status.power.actiphy_capable;
    cap->short_reach_capable   = port_status.power.perfectreach_capable;
    return VTSS_RC_OK;
#endif
    return VTSS_RC_ERROR;
}
/****************************************************************************/
