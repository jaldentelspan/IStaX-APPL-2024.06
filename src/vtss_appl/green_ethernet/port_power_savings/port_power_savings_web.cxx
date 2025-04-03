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

#include "web_api.h"
#include "port_api.h"
#include "port_iter.hxx"
#ifdef VTSS_SW_OPTION_EEE
#include "eee_api.h"
#endif

#ifdef VTSS_SW_OPTION_PRIV_LVL
#include "vtss_privilege_api.h"
#include "vtss_privilege_web_api.h"
#endif

// I know that err_msg isn't protected, but this is only in case of failures (which should not happen), so we'll live with it.
/*lint -esym(459,err_msg)*/

/****************************************************************************/
/*  Web Handler  functions                                                  */
/****************************************************************************/
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_WEB
//
// config handler
//
static i32 handler_config_port_power_savings_config(CYG_HTTPD_STATE *p)
{
    vtss_isid_t           isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                   ct;
    static char           err_msg[100]; // Need to be static because it is set when posting data, but used getting data.
    char                  form_name[32];
#ifdef VTSS_SW_OPTION_EEE
    eee_switch_conf_t     switch_conf, oldconf;
    eee_switch_global_conf_t     switch_global_conf, old_global_conf;
    eee_switch_state_t    state;
#if EEE_FAST_QUEUES_CNT > 0
    uint                  queue;
#endif
#endif
    mesa_rc               rc;
    port_iter_t           pit;
    vtss_appl_port_conf_t port_conf;
    BOOL                  actiphy;
    BOOL                  perfectreach;
    vtss_appl_port_status_t port_status;

//    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
//        return -1;
//    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_CONFIG_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET)) {
        return -1;
    }
#endif

#ifdef VTSS_SW_OPTION_EEE
    // Get configuration for the selected switch
    (void)eee_mgmt_switch_conf_get(isid, &oldconf); // Get current configuration
    (void)eee_mgmt_switch_global_conf_get(&old_global_conf); // Get current configuration
    switch_conf = oldconf;
#endif
    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);

    // Setting new configuration
    if (p->method == CYG_HTTPD_METHOD_POST) {

        // Parameters
        while (port_iter_getnext(&pit)) {

#ifdef VTSS_SW_OPTION_EEE
            // Enabled checkbox, htm-checkbox returns 1 if set else 0
            sprintf(form_name, "eee_enabled_%u", pit.uport);
            if (cyg_httpd_form_varable_find(p, form_name)) {
                switch_conf.port[pit.iport].eee_ena = TRUE;
            } else {
                switch_conf.port[pit.iport].eee_ena = FALSE;
            }

            // The optimize for power drop down list
            sprintf(form_name, "OptimizeForPower"); // Set form name
            switch_global_conf.optimized_for_power = httpd_form_get_value_int(p, form_name, 0, 1);
            T_I("Form:%s=%d, switch_conf.optimized_for_power:%d",
                form_name, httpd_form_get_value_int(p, form_name, 0, 1), switch_global_conf.optimized_for_power);


#if EEE_FAST_QUEUES_CNT > 0
            // Fast queues
            switch_conf.port[pit.iport].eee_fast_queues = 0;
            for (queue = 0; queue < EEE_FAST_QUEUES_CNT; queue++) {
                // Fast queue checkbox, htm-checkbox returns 1 if set else 0
                sprintf(form_name, "queue_%u_%u", queue, pit.uport);
                if (cyg_httpd_form_varable_find(p, form_name)) {
                    switch_conf.port[pit.iport].eee_fast_queues |= 1 << queue;
                }
            }
#endif
#endif
            // Find power mode
            sprintf(form_name, "actiPhy_%u", pit.uport);
            if (cyg_httpd_form_varable_find(p, form_name)) {
                actiphy = TRUE;
            } else {
                actiphy = FALSE;
            }

            sprintf(form_name, "perfectReach_%u", pit.uport);
            if (cyg_httpd_form_varable_find(p, form_name)) {
                perfectreach = TRUE;
            } else {
                perfectreach = FALSE;
            }

            vtss_ifindex_t ifindex;
            if (vtss_ifindex_from_port(isid, pit.iport, &ifindex) != VTSS_RC_OK) {
                T_E("Could not get ifindex");
            }

            if ((rc = vtss_appl_port_conf_get(ifindex, &port_conf)) != VTSS_RC_OK) {
                T_E("Could not get port conf for port:%u, rc:%s", pit.iport, error_txt(rc));
            }

            port_conf.power_mode = VTSS_PHY_POWER_NOMINAL;

            if (actiphy) {
                port_conf.power_mode = VTSS_PHY_POWER_ACTIPHY;
            }
            if (perfectreach) {
                port_conf.power_mode = VTSS_PHY_POWER_DYNAMIC;
            }

            if (actiphy && perfectreach) {
                port_conf.power_mode = VTSS_PHY_POWER_ENABLED;
            }

            if ((rc = vtss_appl_port_conf_set(ifindex, &port_conf)) != VTSS_RC_OK) {
                T_E("Could not set port conf for port:%u, isid:%d, rc:%s", pit.iport, isid, error_txt(rc));
            }

            T_I_PORT(pit.iport, "actiphy:%d, perfectreach%d:, mode:%d", actiphy, perfectreach, port_conf.power_mode);
        }

#ifdef VTSS_SW_OPTION_EEE
        // Set current configuration
        if (vtss_memcmp(switch_conf, oldconf)) {
            if ((rc = eee_mgmt_switch_conf_set(isid, &switch_conf)) != VTSS_RC_OK) {
                misc_strncpyz(err_msg, error_txt(rc), 100); // Set error message used in get method.
                goto err_msg_exit;
            } else {
                err_msg[0] = '\0';
            }
        }

        if (memcmp(&switch_global_conf, &old_global_conf, sizeof(switch_global_conf))) {
            if ((rc = eee_mgmt_switch_global_conf_set(&switch_global_conf)) != VTSS_RC_OK) {
                misc_strncpyz(err_msg, error_txt(rc), 100); // Set error message used in get method.
                goto err_msg_exit;
            } else {
                err_msg[0] = '\0';
            }
        }

err_msg_exit:
#endif
        redirect(p, "/port_power_savings_config.htm");

    } else {
        // Getting the configuration.
        cyg_httpd_start_chunked("html");

        // The data is split like this: err_msg|queue_cnt|port_1_configuration&port_2_configuration&......&port_n_configuration
        // The port configuration data is split like this: port_no/eee_capable/eee_enabled/fast_queue_mask

        // Apply error message (if any)
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", err_msg);
        cyg_httpd_write_chunked(p->outbuffer, ct);
        err_msg[0] = '\0'; // Clear error message

#ifdef VTSS_SW_OPTION_EEE
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|", TRUE);
#else
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|", FALSE);
#endif
        cyg_httpd_write_chunked(p->outbuffer, ct);

#ifdef VTSS_SW_OPTION_EEE
        // Apply number of fast queues
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|", EEE_FAST_QUEUES_CNT);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        // Apply the optimized_for_power
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|", old_global_conf.optimized_for_power);
        cyg_httpd_write_chunked(p->outbuffer, ct);

        (void)eee_mgmt_switch_state_get(isid, &state);
#else
        // Apply number of fast queues
        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "0|0|");
        cyg_httpd_write_chunked(p->outbuffer, ct);
#endif

        while (port_iter_getnext(&pit)) {
            vtss_ifindex_t ifindex;
            if (vtss_ifindex_from_port(isid, pit.iport, &ifindex) != VTSS_RC_OK) {
                T_E("Could not get ifindex");
            };

            if (vtss_appl_port_conf_get(ifindex, &port_conf)) {
                T_E("Could not get port conf for port:%u", pit.iport);
            }

            if (vtss_appl_port_status_get(ifindex, &port_status) != VTSS_RC_OK) {
                T_E("Could not get port status for port:%u", pit.iport);
            }

            ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%d/%d/%d/%d/%d/%d/%d&",
                          pit.uport,
#ifdef VTSS_SW_OPTION_EEE
                          state.port[pit.iport].eee_capable,
                          switch_conf.port[pit.iport].eee_ena,
#if EEE_FAST_QUEUES_CNT == 0
                          0,
#else
                          switch_conf.port[pit.iport].eee_fast_queues,
#endif
#else
                          0,
                          0,
                          0,
#endif
                          port_status.power.actiphy_capable,
                          port_conf.power_mode == VTSS_PHY_POWER_ACTIPHY || port_conf.power_mode == VTSS_PHY_POWER_ENABLED, // Actiphy
                          port_status.power.perfectreach_capable,
                          port_conf.power_mode == VTSS_PHY_POWER_DYNAMIC || port_conf.power_mode == VTSS_PHY_POWER_ENABLED  // Perfectreach
                         );
            T_I_PORT(pit.iport, "mode:%d, %d", port_conf.power_mode, port_conf.power_mode == VTSS_PHY_POWER_DYNAMIC || port_conf.power_mode == VTSS_PHY_POWER_ENABLED);

            cyg_httpd_write_chunked(p->outbuffer, ct);
        } // End iport for loop

        cyg_httpd_end_chunked();
    }
    return -1; // Do not further search the file system.
}


//
// EEE Status handler
//
static i32 handler_config_port_power_savings_status(CYG_HTTPD_STATE *p)
{
    vtss_isid_t           isid = web_retrieve_request_sid(p); /* Includes USID = ISID */
    int                   ct;
    static char           err_msg[100]; // Need to be static because it is set when posting data, but used getting data.
    port_iter_t           pit;
#ifdef VTSS_SW_OPTION_EEE
    eee_switch_state_t    switch_state;
    eee_switch_conf_t     switch_conf;
#endif
    vtss_appl_port_status_t port_status;

    if (redirectUnmanagedOrInvalid(p, isid)) { /* Redirect unmanaged/invalid access to handler */
        return -1;
    }

#ifdef VTSS_SW_OPTION_PRIV_LVL
    if (web_process_priv_lvl(p, VTSS_PRIV_LVL_STAT_TYPE, VTSS_MODULE_ID_GREEN_ETHERNET)) {
        return -1;
    }
#endif

#ifdef VTSS_SW_OPTION_EEE
    // Get configuration for the selected switch
    (void)eee_mgmt_switch_conf_get(isid, &switch_conf); // Get current configuration

    // Get configuration for the selected switch
    (void)eee_mgmt_switch_state_get(isid, &switch_state); // Get current state
#endif
    (void)port_iter_init(&pit, NULL, isid, PORT_ITER_SORT_ORDER_IPORT, PORT_ITER_FLAGS_NORMAL);

    // Getting the status.
    cyg_httpd_start_chunked("html");


    // Apply error message (if any)
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%s|", err_msg);
    cyg_httpd_write_chunked(p->outbuffer, ct);
    err_msg[0] = '\0'; // Clear error message

#ifdef VTSS_SW_OPTION_EEE
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|", TRUE);
#else
    ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%d|", FALSE);
#endif
    cyg_httpd_write_chunked(p->outbuffer, ct);

    while (port_iter_getnext(&pit)) {
        vtss_ifindex_t ifindex;
        if (vtss_ifindex_from_port(isid, pit.iport, &ifindex) != VTSS_RC_OK) {
            T_E("Could not get ifindex");
        };

        mesa_rc rc;

        if ((rc = vtss_appl_port_status_get(ifindex, &port_status)) != VTSS_RC_OK) {
            T_E("%s", error_txt(rc));
        }

        ct = snprintf(p->outbuffer, sizeof(p->outbuffer), "%u/%d/%d/%d/%d/%d/%d/%d/%d&",
                      pit.uport,
#ifdef VTSS_SW_OPTION_EEE
                      switch_state.port[pit.iport].eee_capable,
                      switch_conf.port[pit.iport].eee_ena,
                      switch_state.port[pit.iport].link,
                      switch_state.port[pit.iport].link_partner_eee_capable,
                      switch_state.port[pit.iport].rx_in_power_save_state,
                      switch_state.port[pit.iport].tx_in_power_save_state,
#else
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
#endif
                      port_status.power.actiphy_capable ? port_status.power.actiphy_power_savings : 0, // The image is "inverted in status.htm"
                      port_status.power.perfectreach_capable ? port_status.power.perfectreach_power_savings : 0 // The image is "inverted in status.htm"
                     );

        cyg_httpd_write_chunked(p->outbuffer, ct);
    } // End iport for loop

    cyg_httpd_end_chunked();

    return -1; // Do not further search the file system.
}





/****************************************************************************/
/*  Module Filter CSS routine                                               */
/****************************************************************************/
static size_t EEE_lib_filter_css(char **base_ptr, char **cur_ptr, size_t *length)
{
    char buff[100];

    (void) snprintf(buff, 100,
#if !defined(EEE_FAST_QUEUES_CNT) || EEE_FAST_QUEUES_CNT == 0
                    ".EEE_HAS_URGENT_QUEUES {display:none;}\r\n"
#else
                    " "
#endif

#if !defined(EEE_OPTIMIZE_SUPPORT) || (EEE_OPTIMIZE_SUPPORT == 0)
                    ".EEE_OPTIMIZE_SUPPORT {display:none;}\r\n"
#else
                    " "
#endif
                   );
    return webCommonBufferHandler(base_ptr, cur_ptr, length, buff);
}
/****************************************************************************/
/*  Filter CSS table entry                                                  */
/****************************************************************************/
web_lib_filter_css_tab_entry(EEE_lib_filter_css);

/****************************************************************************/
/*  HTTPD Table Entries                                                     */
/****************************************************************************/
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_config_port_power_savings_config, "/config/port_power_savings_config", handler_config_port_power_savings_config);
CYG_HTTPD_HANDLER_TABLE_ENTRY(get_cb_status_port_power_savings_status, "/stat/port_power_savings_status", handler_config_port_power_savings_status);

