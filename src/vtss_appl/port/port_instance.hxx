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

#ifndef _PORT_INSTANCE_HXX_
#define _PORT_INSTANCE_HXX_

#include <vtss/appl/port.h>
#include <microchip/ethernet/board/api.h>
#include <vtss/basics/type_traits.hxx>
#include "port_api.h"

#if defined(VTSS_SW_OPTION_PTP)
// If we include the PTP module, we must fix the out-of-sync issue on Lu26.
// See MESA-404 and Bugzilla#9251
#define VTSS_OOS_FIX
#endif

const char *port_instance_vol_user_txt(port_user_t user);

typedef struct {
    bool                     running;     /* Running or idle                           */
    bool                     valid;       /* Result valid                              */
    mepa_cable_diag_result_t result;      /* Result                                    */
    port_veriphy_mode_t      mode;        /* The mode to run VeriPhy.                  */
    uint8_t                  repeat_cnt;  /* Number of time VeriPhy should be repeated */
    uint8_t                  variate_cnt; /* Number of time the result is not the same */
} port_veriphy_t;

typedef struct port_instance {
    port_instance(void) {}

    port_instance(const struct port_instance &rhs) = delete;
    port_instance &operator=(const struct port_instance &rhs) = delete;
    port_instance(struct port_instance &&rhs) = delete;
    port_instance &operator=(struct port_instance &&rhs) = delete;

    void phy_device_set(void);
    void sfp_device_set(meba_sfp_device_t *dev);

    void init_members(mesa_port_no_t port_no);
    void reload_defaults(mesa_port_no_t port_no);

    void conf_default_get(vtss_appl_port_conf_t &conf);
    void conf_get(vtss_appl_port_conf_t &conf);
    mesa_rc conf_set(const vtss_appl_port_conf_t &new_conf);

    void shutdown_now(void);

    mesa_rc vol_conf_get(port_user_t user,       port_vol_conf_t &vol_conf);
    mesa_rc vol_conf_set(port_user_t user, const port_vol_conf_t &vol_conf);
    void poll(vtss_appl_port_status_t &new_port_status, bool &link_down, bool &link_up);
    void port_status_get(vtss_appl_port_status_t &port_status);
    mesa_rc led_update(void);

    void link_up_down_cnt_clear(void)
    {
        _port_status.link_up_cnt   = 0;
        _port_status.link_down_cnt = 0;
    }

    void sfp_device_del(void);

    meba_port_cap_t static_caps_get(void)
    {
        return _port_status.static_caps;
    }

    meba_port_cap_t sfp_caps_get(void)
    {
        return _port_status.sfp_caps;
    }

    port_veriphy_t &get_veriphy(void)
    {
        return _veriphy;
    }

    bool has_link(void)
    {
        return _port_status.link;
    }

    mesa_rc start_veriphy(int mode);
    mesa_rc get_veriphy(mepa_cable_diag_result_t *res);

    bool has_phy(void) const;
    bool is_phy(void) const;
    mesa_rc phy_reset(bool host_rst = false);

    bool has_sfp_dev(void) const
    {
        return _sfp_dev != nullptr;
    }

    bool has_phy_dev(void) const
    {
        return _phy_dev;
    }

    bool may_load_sfp(void) const
    {
        return _may_load_sfp;
    }

    void notify_link_down_set(bool phy_link_down, bool sfp_link_down)
    {
        if (phy_link_down) {
            _notify_link_down_phy = true;
        }

        if (sfp_link_down) {
            _notify_link_down_sfp = true;
        }
    }

    // Called by KR module when clause 73 is enabled on a port and a state
    // change occurs. Used by us to figure out when it's safe to signal link up,
    // because the port may link flap during aneg and training.
    void kr_completeness_set(bool complete)
    {
        _kr_complete = complete;
    }

    void debug_state_dump(uint32_t session_id, port_icli_pr_t pr);

    mesa_rc phy_man_neg_set(const char *func, int line, mepa_manual_neg_t man_neg);
    mepa_manual_neg_t phy_man_neg_get(const char *func, int line);

private:
    void conf_update(const vtss_appl_port_conf_t &new_conf, const port_vol_conf_t &new_vol_conf);
    void kr_conf_update(void);
    bool kr_about_to_be_disabled(void);
    void kr_status_update(void);
    void kr_fec_update(mesa_port_kr_fec_t &new_fec_conf);

    // Configuration check functions
    mesa_rc media_type_check(const vtss_appl_port_conf_t &new_conf);
    mesa_rc speed_duplex_check(const vtss_appl_port_conf_t &new_conf);
    mesa_rc speed_on_dual_media_check(const vtss_appl_port_conf_t &new_conf);
    mesa_rc kr_check(const vtss_appl_port_conf_t &new_conf);
    mesa_rc fec_check(const vtss_appl_port_conf_t &new_conf);
    mesa_rc conf_check(const vtss_appl_port_conf_t &new_conf);

    bool admin_enabled(const vtss_appl_port_conf_t &conf, const port_vol_conf_t &vol_conf) const;

    mesa_sd10g_media_type_t serdes_media_type_get(mesa_port_speed_t speed) const;
    void port_state_set(const char *func, int line, bool enable);
    void port_conf_set(const char *func, int line, bool force_new_conf = false);

    void vol_conf_merge(port_vol_conf_t &new_vol_conf);
    void vol_conf_shutdown(port_vol_conf_t &new_vol_conf);
    bool vol_conf_process(void);

    mepa_media_interface_t phy_media_if_get(void);
    void phy_media_if_set(void);
    void phy_conf_set(const char *func, int line, bool enable, bool force_new_conf);
    void turn_off_phy(const char *func, int line);
    void turn_on_phy( const char *func, int line, bool force_new_conf = false);

    void sfp_admin_state_set(const char *func, int line, bool enable);
    void sfp_conf_set(const char *func, int line, bool enable);
    void turn_off_sfp(const char *func, int line);
    void turn_on_sfp( const char *func, int line);

    void port_speed_min_max_update(void);
    vtss_appl_port_sfp_type_t sfp_type_of_mac_to_mac(void);
    void sfp_caps_update(void);
    void speed_update(bool prefer_fiber);
    bool caps_update(void);
    mesa_port_interface_t mac_if_from_speed(mesa_port_speed_t &speed, const char **mac_if_came_from);

    bool sfp_usable(void);
    bool prefer_fiber_get(bool turn_on_off);
    bool running_1g_aneg(void);
    void update_1g_aneg_type(bool prefer_fiber);
    void sfp_may_not_work_oper_warning_update(void);

    bool pfc_enabled(const vtss_appl_port_conf_t &conf);

    // Power saving status
    void power_saving_status_update(void);

    // Luton26 OOS work around
    void lu26_oos_fix(const char *func, int line, bool update_phy_dev, mesa_port_speed_t speed, bool pcs_ena);

#if defined(VTSS_OOS_FIX)
    // Helper functionsm
    void lu26_oos_port_conf_set(bool enable);
    void lu26_oos_aneg_clear(mesa_port_speed_t new_speed);
    void lu26_oos_aneg_restore(mesa_port_speed_t old_speed);
#endif

    // Data
    vtss_appl_port_status_t _port_status = {};

    bool               _phy_dev; // Indication that the port has a PHY
    meba_sfp_device_t *_sfp_dev;

    // Obtaining the mac_if can be a pain, and sometimes the code does it
    // wrongly (by using the driver's meba_sfp_driver_if_get() or
    // meba_phy_if_get() rather than port_custom_table[]'s mac_if.
    // The following boolean controls per-port whether to force it to use
    // port_custom_table[]'s interface
    bool _mac_if_use_port_custom_table;

    // Configuration
    vtss_appl_port_conf_t _conf = {};     // User's config
    port_vol_conf_t       _vol_conf = {}; // Volatile overrides (merged)
    CapArray<port_vol_conf_t, VTSS_APPL_CAP_PORT_USER_CNT> _vol_confs; // Per-user volatile overrides

    mesa_port_no_t _port_no = MESA_PORT_NO_NONE;
    vtss_ifindex_t _ifindex = VTSS_IFINDEX_NONE;

    mepa_media_interface_t   _phy_media_if       = MESA_PHY_MEDIA_IF_AMS_FI_PASSTHRU; // Unused value to force a call
    mepa_status_t            _phy_status         = {};
    mepa_phy_info_t          _phy_info           = {};
    meba_sfp_driver_status_t _sfp_status         = {};
    mesa_port_pcs_conf_t     _pcs_conf           = {};
    mepa_conf_t              _phy_conf           = {};
    meba_sfp_driver_conf_t   _sfp_conf           = {};
    mesa_port_kr_conf_t      _kr_conf            = {};
    mesa_port_kr_fec_t       _fec_conf           = {};
    mesa_port_conf_t         _port_conf          = {};
    mesa_port_speed_t        _phy_speed          = MESA_SPEED_UNDEFINED;
    mesa_port_speed_t        _sfp_speed          = MESA_SPEED_UNDEFINED;
    mesa_port_speed_t        _port_speed_min     = MESA_SPEED_UNDEFINED;
    mesa_port_speed_t        _port_speed_max     = MESA_SPEED_UNDEFINED;
    bool                     _sfp_must_run_kr    = false;
    bool                     _sfp_may_run_kr     = false;
    bool                     _vol_config_changed = false;
    bool                     _sfp_turned_on      = false;
    bool                     _sfp_initialized    = false;
    bool                     _kr_initialized     = false;
    bool                     _port_initialized   = false;
    bool                     _cu_sfp             = false;
    bool                     _is_100m_sfp        = false;
    bool                     _is_aqr_phy         = false;
    bool                     _kr_complete        = false;
    bool                     _port_state         = false;

    // Veriphy
    port_veriphy_t _veriphy = {};

    bool _may_load_sfp = true;

    bool _notify_link_down_phy = false;
    bool _notify_link_down_sfp = false;
    bool _notify_shutdown      = false;

    bool _dont_poll_phy_next_time = false;
    bool _dont_poll_sfp_next_time = false;

    // This variable keeps track of whether MAC serdes is configured at least
    // once on the PHY side. In Indy PHY's QSGMII serdes, the host side QSGMII
    // must be reset after the MAC is configured the first time.
    bool _phy_host_side_reset = false;
} port_instance_t;

#endif

