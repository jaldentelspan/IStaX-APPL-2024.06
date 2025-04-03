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
#include "gvrp_serializer.hxx"
#include <vtss_trace_api.h>
#include "../base/vtss_gvrp.h"
#include "xxrp_api.h"
#include "vtss_xxrp_callout.h"
#include "misc_api.h"
#include "xxrp_trace.h"
#include "critd_api.h"

VTSS_CRIT_SCOPE_CLASS_EXTERN(xxrp_appl_crit, VtssXxrpApplCritdGuard);

#define XXRP_APPL_CRIT_SCOPE() VtssXxrpApplCritdGuard __lock_guard__(__LINE__)

/**
 * \brief Get GVRP global configuration.
 *
 * \param config [OUT] The data pointer of configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_gvrp_config_globals_get(
    vtss_appl_gvrp_config_globals_t     *const config
)
{
    /* Check illegal parameters */
    if (config == NULL) {
        T_W("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }

    config->mode           = vtss_gvrp_is_enabled() ? TRUE : FALSE;
    config->leave_all_time = (u32)vtss_gvrp_get_timer(GARP_TC__leavealltimer);
    config->leave_time     = (u32)vtss_gvrp_get_timer(GARP_TC__leavetimer);
    config->join_time      = (u32)vtss_gvrp_get_timer(GARP_TC__transmitPDU);
    config->max_vlans      = (u32)vtss_gvrp_max_vlans();

    return VTSS_RC_OK;
}

/**
 * \brief Set GVRP global configuration.
 *
 * \param config [IN] The data pointer of configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_gvrp_config_globals_set(
    const vtss_appl_gvrp_config_globals_t   *const config
)
{
    mesa_rc rc;
    int  appl_exclusive = 1;

    /* Check illegal parameters */
    if (config == NULL) {
        T_W("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }

    if (config->mode != TRUE && config->mode != FALSE) {
        return VTSS_RC_ERROR;
    }

    XXRP_APPL_CRIT_SCOPE();
    /* Check that no other MRP/GARP application is currently enabled */
    if (xxrp_mgmt_appl_exclusion(VTSS_GARP_APPL_GVRP)) {
        appl_exclusive = 0;
    }

    // --- Check timers
    rc = vtss_gvrp_chk_timer(GARP_TC__transmitPDU, config->join_time);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    rc = vtss_gvrp_chk_timer(GARP_TC__leavetimer, config->leave_time);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    rc = vtss_gvrp_chk_timer(GARP_TC__leavealltimer, config->leave_all_time);
    if (rc != VTSS_RC_OK) {
        return rc;
    }

    // --- GVRP is disabled, then set the 4 parameters
    //     join_timer, leave_timer, leave_all_timer and max_vlans first.
    //     If GVRP is about to be disabled, then disable first, and then
    //     set the 4 parameters.
    if ((! config->mode) && vtss_gvrp_is_enabled()) {
        // --- Disable
        (void)xxrp_mgmt_global_enabled_set(VTSS_GARP_APPL_GVRP, FALSE);
        GVRP_CRIT_ENTER();
        vtss_gvrp_destruct(FALSE);
        GVRP_CRIT_EXIT();
    }

    // --- Check max-vlans changed or not
    //     if changed, but GVRP is enabled, max-vlans cannot apply,
    //     max-vlans can apply only when GVRP is disabled.
    if (config->max_vlans != vtss_gvrp_max_vlans()) {
        if (!vtss_gvrp_is_enabled()) {
            if (VTSS_RC_OK != (rc = vtss_gvrp_max_vlans_set(config->max_vlans))) {
                return rc;
            }
        } else {
            /* max-vlans setting only apply when GVRP is disabled */
            return VTSS_RC_ERROR;
        }
    }

    // --- We do not check the error code below,
    //     since we have done that already.
    vtss_gvrp_set_timer(GARP_TC__transmitPDU,   config->join_time);
    vtss_gvrp_set_timer(GARP_TC__leavetimer,    config->leave_time);
    vtss_gvrp_set_timer(GARP_TC__leavealltimer, config->leave_all_time);

    // --- Turn GVRP on now if needed.
    if (config->mode && !vtss_gvrp_is_enabled()) {
        if (appl_exclusive) {
            (void)vtss_gvrp_construct(-1, vtss_gvrp_max_vlans());
            (void)xxrp_mgmt_global_enabled_set(VTSS_GARP_APPL_GVRP, TRUE);
        }
    }

    return rc;
}

/**
 * \brief Iterate function of interface table.
 *
 * \param prev_ifindex [IN]  ifindex of previous port.
 * \param next_ifindex [OUT] ifindex of next port.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_gvrp_config_interface_itr(
    const vtss_ifindex_t    *prev_ifindex,
    vtss_ifindex_t          *next_ifindex
)
{
    return vtss_appl_iterator_ifindex_front_port(prev_ifindex, next_ifindex);
}

/**
 * \brief Get GVRP interface configuration.
 *
 * \param ifindex [IN]  ifindex of port.
 * \param config  [OUT] The data pointer of interface configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_gvrp_config_interface_get(
    vtss_ifindex_t                      ifindex,
    vtss_appl_gvrp_config_interface_t   *const config
)
{
    mesa_rc                 rc;
    vtss_ifindex_elm_t      ife;

    /* Check illegal parameters */
    if (config == NULL) {
        T_W("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    rc = xxrp_mgmt_enabled_get(ife.isid, (mesa_port_no_t)ife.ordinal, VTSS_GARP_APPL_GVRP, &(config->mode));
    return rc;
}


/**
 * \brief Set GVRP interface configuration.
 *
 * \param ifindex [IN]  ifindex of port.
 * \param config  [OUT] The data pointer of interface configuration.
 *
 * \return VTSS_RC_OK if the operation succeeded.
 */
mesa_rc vtss_appl_gvrp_config_interface_set(
    vtss_ifindex_t                              ifindex,
    const vtss_appl_gvrp_config_interface_t     *const config
)
{
    mesa_rc                 rc;
    vtss_ifindex_elm_t      ife;

    /* Check illegal parameters */
    if (config == NULL) {
        T_W("Input parameter is NULL");
        return VTSS_RC_ERROR;
    }

    /* get isid/iport from ifindex and validate them */
    if (vtss_appl_ifindex_port_configurable(ifindex, &ife) != VTSS_RC_OK) {
        return VTSS_RC_ERROR;
    }

    rc = xxrp_mgmt_enabled_set(ife.isid, (mesa_port_no_t)ife.ordinal, VTSS_GARP_APPL_GVRP, config->mode);
    return rc;
}
