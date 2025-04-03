/*
 Copyright (c) 2006-2024 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#ifndef __PORT_API_H__
#define __PORT_API_H__

#include "main.h"
#include <vtss/appl/port.h>

// Get port capability for a particular port
mesa_rc port_cap_get(mesa_port_no_t port_no, meba_port_cap_t *cap);

// Check for PHY capabilities
bool port_phy_cap_check(mepa_phy_cap_t cap);

// Get port advertise_disable bitmask based on port capabilities
mepa_adv_dis_t port_cap_to_advertise_dis(meba_port_cap_t port_cap);

// Determine if port has a PHY
bool port_has_phy(mesa_port_no_t port_no);

// Returns if a port is PHY (if the PHY is in pass through mode, it shall act a
// s it is not a PHY)
bool port_is_phy(mesa_port_no_t port_no);

// Determine if port has 10g Cu PHY
bool port_is_10g_copper_phy(mesa_port_no_t port_no);

/* Wait until all PHYs have been reset */
void port_phy_wait_until_ready(void);

// Returns true if the port is a front port
bool port_is_front_port(mesa_port_no_t port_no);

/* Get current maximum port count */
uint32_t port_count_max(void);

/* ================================================================= *
 *  Local port change events
 * ================================================================= */

/* Port change callback */
typedef void (*port_change_callback_t)(mesa_port_no_t port_no, const vtss_appl_port_status_t *status);

// Port change callback registration
// After successful registration, it is guaranteed that the registrant will be
// called back once per port, so there's usually no need to pre-cache the
// current link status.
mesa_rc port_change_register(vtss_module_id_t module_id, port_change_callback_t callback);

/* Port shutdown callback */
typedef void (*port_shutdown_callback_t)(mesa_port_no_t port_no);

/* Port shutdown registration */
mesa_rc port_shutdown_register(vtss_module_id_t module_id, port_shutdown_callback_t callback);

/* Port configuration change callback */
typedef void (*port_conf_change_callback_t)(mesa_port_no_t port_no, const vtss_appl_port_conf_t *conf);

/* Port configuration change registration */
mesa_rc port_conf_change_register(vtss_module_id_t module_id, port_conf_change_callback_t callback);

/* ================================================================= *
 *  Volatile port configuration APIs
 * ================================================================= */

/* Volatile port configuration users */
typedef enum {
    PORT_USER_STATIC,
    PORT_USER_ACL,
    PORT_USER_THERMAL_PROTECT,
    PORT_USER_LOOP_PROTECT,
    PORT_USER_UDLD,
    PORT_USER_PSEC,
    PORT_USER_ERRDISABLE,
    PORT_USER_AGGR,

    PORT_USER_CNT // Must come last
} port_user_t;

/* Volatile port configuration */
typedef struct port_vol_conf_s {
    bool disable;             /* Administrative disable port */
    bool oper_up;             /* Force operational mode up */
    bool oper_down;           /* Force operational mode down, i.e. no forwarding to/from this port */
    bool disable_adm_recover; /* Administrative disable port, but let "shutdown/no shutdown" re-open it (also clears this bool) */
    void (*on_adm_recover_clear)(mesa_port_no_t port_no, const struct port_vol_conf_s *const new_conf); /* Callback invoked when port-module resets disable_adm_recover. May be NULL */
} port_vol_conf_t;

typedef struct {
    port_vol_conf_t conf;
    port_user_t     user;
    char            name[64];
} port_vol_status_t;

mesa_rc port_vol_conf_get(port_user_t user, mesa_port_no_t port_no, port_vol_conf_t *conf);
mesa_rc port_vol_conf_set(port_user_t user, mesa_port_no_t port_no, const port_vol_conf_t *conf);

/* Get volatile port status (PORT_USER_CNT means combined) */
mesa_rc port_vol_status_get(port_user_t user, mesa_port_no_t port_no, port_vol_status_t *status);

const char *port_media_type_to_txt(vtss_appl_port_media_t media_type, bool capitals = true);
const char *port_fec_mode_to_txt(vtss_appl_port_fec_mode_t fec_mode, bool capitals = true);
const char *port_speed_to_txt(mesa_port_speed_t speed, bool capitals = true);
const char *port_aneg_method_to_txt(vtss_appl_port_aneg_method_t aneg_method, bool use_consolidated_name = false);
const char *port_speed_duplex_to_txt_from_conf(const vtss_appl_port_conf_t &conf);
const char *port_speed_duplex_to_txt_from_status(const vtss_appl_port_status_t &status);
const char *port_speed_duplex_link_to_txt(const vtss_appl_port_status_t &status);
const char *port_veriphy_status_to_txt(mepa_cable_diag_status_t status);
const char *port_sfp_transceiver_to_txt(meba_sfp_transreceiver_t tr);
const char *port_sfp_connector_to_txt(meba_sfp_connector_t conn);
const char *port_sfp_type_to_txt(vtss_appl_port_sfp_type_t sfp_type);
char *port_sfp_type_speed_to_txt(char *buf, size_t size, vtss_appl_port_status_t &status);              // sizeof(buf) >=  13 bytes
char *port_cap_to_txt(           char *buf, size_t size, meba_port_cap_t cap);                          // sizeof(buf) >= 400 bytes
char *port_adv_dis_to_txt(       char *buf, size_t size, mepa_adv_dis_t adv_dis);                       // sizeof(buf) >=  50 bytes
char *port_oper_warnings_to_txt( char *buf, size_t size, vtss_appl_port_oper_warnings_t oper_warnings); // sizeof(buf) >= 200 bytes

/* VeriPHY operation mode */
typedef enum {
    PORT_VERIPHY_MODE_NONE,      /* No VeriPHY at all */
    PORT_VERIPHY_MODE_BASIC,     /* No length or cross pair search */
    PORT_VERIPHY_MODE_NO_LENGTH, /* No length search */
    PORT_VERIPHY_MODE_FULL       /* Full VeriPHY process */
} port_veriphy_mode_t;

/* Start VeriPHY */
mesa_rc port_veriphy_start(port_veriphy_mode_t *mode);

/* Get VeriPHY result */
mesa_rc port_veriphy_result_get(mesa_port_no_t port_no, mepa_cable_diag_result_t *result, unsigned int timeout);

// Debug function for resetting the PHY for a specific port.
void port_phy_reset(mesa_port_no_t port_no);

/**
 * \brief Checks if an interface index is a valid switch/port interface
 * \param ifindex [IN] Interface number to be checked.
 * \return VTSS_RC_OK if interface contains a valid switch id and port number else error code
 */
mesa_rc port_ifindex_valid(vtss_ifindex_t ifindex);

/**
 * In 1G aneg one end is reference and the other end is
 * client. Usually it is random which end is reference and which end
 * is client. However, when using syncE, the syncE information can
 * only be transferred from the reference to the client, so when syncE
 * choose to use a port for synchronization, it may have to change the
 * direction of the link. There is no reason to specify the direction
 * of the link in the running-config, depending on network topology,
 * syncE may still have to change direction of a link. SyncE just
 * needs to be able to read the current configuration and to be able
 * to change it when needed.
 */
mesa_rc port_man_neg_set(mesa_port_no_t port_no, mepa_manual_neg_t man_neg);
mesa_rc port_man_neg_get(mesa_port_no_t port_no, mepa_manual_neg_t *man_neg);

// For tracing of vtss_appl_port_status_t
vtss::ostream &operator<<(vtss::ostream &o, const vtss_appl_port_status_t &status);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const vtss_appl_port_status_t *status);

// For tracing of mesa_port_conf_t
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_conf_t &conf);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_port_conf_t *conf);

// For tracing of mesa_port_kr_conf_t
vtss::ostream &operator<<(vtss::ostream &o, const mesa_port_kr_conf_t &conf);
size_t fmt(vtss::ostream &o, const vtss::Fmt &fmt, const mesa_port_kr_conf_t *conf);

// For debugging
typedef int32_t (*port_icli_pr_t)(uint32_t session_id, const char *fmt, ...) __attribute__((__format__(__printf__, 2, 3)));

const char *port_error_txt(mesa_rc rc);
mesa_rc port_init(vtss_init_data_t *data);

#endif /* __PORT_API_H__ */

