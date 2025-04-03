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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include "main.h"
#include "port_api.h"
#include "phy_api.h"
#include "phy.h"
#include "conf_api.h"
#include "critd_api.h"
#include "misc_api.h"
#include "phy_icfg.h"
#include "microchip/ethernet/switch/api.h"
#include "vtss/basics/enum_macros.hxx" /* For VTSS_ENUM_INC() */
#include "vtss_optional_modules.hxx"

#include <vtss_module_id.h>
#include <vtss_trace_lvl_api.h>

static vtss_trace_reg_t trace_reg =
{
    VTSS_TRACE_MODULE_ID, "phy", "PHY"
};

static vtss_trace_grp_t trace_grps[] =
{
    [VTSS_TRACE_GRP_DEFAULT] = { 
        "default",
        "Default",
        VTSS_TRACE_LVL_WARNING
    }
};

VTSS_TRACE_REGISTER(&trace_reg, trace_grps);

#define PHY_CRIT_ENTER() critd_enter(&phy.crit, __FILE__, __LINE__)
#define PHY_CRIT_EXIT()  critd_exit( &phy.crit, __FILE__, __LINE__)

/****************************************************************************/
/*  Management functions                                                                                                           */
/****************************************************************************/

vtss_inst_t phy_mgmt_inst_get(void)
{
    vtss_inst_t inst = nullptr;
    return inst;
}

mesa_rc phy_mgmt_failover_set(mesa_port_no_t port_no, vtss_phy_10g_failover_mode_t *failover)
{
    return VTSS_RC_OK;
}


mesa_rc phy_mgmt_failover_get(mesa_port_no_t port_no, vtss_phy_10g_failover_mode_t *failover)
{
    return VTSS_RC_OK;
}

#ifdef VTSS_SW_OPTION_JSON_RPC
VTSS_PRE_DECLS void vtss_appl_phy_json_init(void);
#endif

extern "C" int phy_icli_cmd_register();

mesa_rc phy_init(vtss_init_data_t *data)
{
    mesa_rc rc;

    switch (data->cmd) {
    case INIT_CMD_INIT:
#if defined(VTSS_SW_OPTION_ICFG)
        rc = phy_icfg_init();
        if (rc != VTSS_RC_OK) {
            T_E("%% Error fail to init phy icfg registration, rc = %s", error_txt(rc));
        }
#endif
        phy_icli_cmd_register();
        break;

    case INIT_CMD_START:
#if defined(VTSS_SW_OPTION_JSON_RPC)
        if (port_phy_cap_check(MEPA_CAP_SPEED_MASK_10G)) {
            vtss_appl_phy_json_init();
        }
#endif
        break;

    case INIT_CMD_CONF_DEF:
        break;

    default:
        break;
    }

    return VTSS_RC_OK;
}

