/*
 Copyright (c) 2006-2023 Microsemi Corporation "Microsemi". All Rights Reserved.

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

#include "port_trace.h"
#include "main.h"
#include "icli_api.h"
#include "icli_porting_util.h"
#include "phy_icli_util.h"
#include "port_api.h"
#include "phy_api.h"
#include "misc_api.h"

static void phy_icli_port_iter_init(port_iter_t *pit)
{
    (void)icli_port_iter_init(pit, VTSS_ISID_START, PORT_ITER_FLAGS_NORMAL);
}

static void phy_icli_phy_reg(phy_icli_req_t *req, port_iter_t *pit, u8 addr)
{
    u32    session_id = req->session_id;
    u32    addr_page = ((req->page << 5) + addr);
    ushort value;
    int    i;

    /* Ignore non-PHY ports */
    if (!port_has_phy(pit->iport))
        return;

    if (req->write) {
        /* Write */
        if (meba_phy_clause22_write(board_instance, pit->iport, addr_page, req->value) != VTSS_RC_OK)
            req->count++;
    } else if (meba_phy_clause22_read(board_instance, pit->iport, addr_page, &value) != VTSS_RC_OK) {
        /* Read failure */
        req->count++;
    } else {
        if (req->header) {
            req->header = 0;
            ICLI_PRINTF("Port  Addr     Value   15      8 7       0\n");
        }
        ICLI_PRINTF("%-6u0x%02x/%-4u0x%04x  ", pit->uport, addr, addr, value);
        for (i = 15; i >= 0; i--) {
            ICLI_PRINTF("%u%s", value & (1<<i) ? 1 : 0, i == 0 ? "\n" : (i % 4) ? "" : ".");
        }
    }
}

#define PHY_ICLI_PHY_ADDR_MAX 32
#define PHY_ICLI_CLAUSE45_ADDR_MAX 65535

void phy_icli_debug_phy(phy_icli_req_t *req)
{
    u32                   session_id = req->session_id;
    icli_unsigned_range_t *list = req->addr_list;
    u8                    i, j, addr, addr_list[PHY_ICLI_PHY_ADDR_MAX];
    port_iter_t           pit;
    
    /* Build address list */
    for (addr = 0; addr < PHY_ICLI_PHY_ADDR_MAX; addr++) {
        addr_list[addr] = (list == NULL ? 1 : 0);
    }
    for (i = 0; list != NULL && i < list->cnt; i++) {
        for (j = list->range[i].min; j < PHY_ICLI_PHY_ADDR_MAX && j <= list->range[i].max; j++) {
            addr_list[j] = 1;
        }
    }

    if (req->addr_sort) {
        /* Iterate in (address, port) order */
        for (addr = 0; addr < PHY_ICLI_PHY_ADDR_MAX; addr++) {
            if (addr_list[addr]) {
                phy_icli_port_iter_init(&pit);
                while (icli_port_iter_getnext(&pit, req->port_list)) {
                    phy_icli_phy_reg(req, &pit, addr);
                }
            }
        }
    } else {
        /* Iterate in (port, address) order */
        phy_icli_port_iter_init(&pit);
        while (icli_port_iter_getnext(&pit, req->port_list)) {
            for (addr = 0; addr < PHY_ICLI_PHY_ADDR_MAX; addr++) {
                if (addr_list[addr]) {
                    phy_icli_phy_reg(req, &pit, addr);
                }
            }
        }
    }
    if (req->count) {
        ICLI_PRINTF("%u operations failed\n", req->count);
    }
}


mesa_rc phy_icli_debug_phy_mode_set(i32 session_id, icli_stack_port_range_t *plist, BOOL has_media_if, u8 media_if, BOOL has_conf_mode, u8 conf_mode, BOOL has_speed, BOOL has_1g, BOOL has_100M, BOOL has_10M)
{
    vtss_phy_reset_conf_t reset_conf;
    vtss_phy_conf_t       setup_conf;
    switch_iter_t         sit;
    port_iter_t           pit;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            VTSS_RC(vtss_phy_reset_get(NULL, pit.iport, &reset_conf));
            VTSS_RC(vtss_phy_conf_get(NULL, pit.iport, &setup_conf));

            if (has_media_if || has_conf_mode || has_speed ) { 
                if (has_media_if) {
                    reset_conf.media_if = (vtss_phy_media_interface_t)media_if;
                }
                VTSS_RC(vtss_phy_reset(NULL, pit.iport, &reset_conf));

                if (has_conf_mode) {
                    setup_conf.mode = (vtss_phy_mode_t)conf_mode;
                }
                if (conf_mode == VTSS_PHY_MODE_FORCED && has_speed) {
                    if (has_100M) {
                        ICLI_PRINTF("Speed 100M\n");
                        setup_conf.forced.speed = VTSS_SPEED_100M;                
                    } 

                    if (has_10M) {
                        ICLI_PRINTF("Speed 10M\n");
                        setup_conf.forced.speed = VTSS_SPEED_10M;              
                    }

                    if (has_1g) {
                        ICLI_PRINTF("Speed 1G\n");
                        setup_conf.forced.speed = VTSS_SPEED_1G;                
                    }
                } else if (!has_speed) {
                    ICLI_PRINTF("Setting default forced speed to 1G \n");
                    setup_conf.forced.speed = VTSS_SPEED_1G;
                }
                ICLI_PRINTF("conf_mode:%d media_if:%d speed %u \n", conf_mode, media_if,setup_conf.forced.speed);
                VTSS_RC(vtss_phy_conf_set(NULL, pit.iport, &setup_conf));
            } else {
                ICLI_PRINTF("conf_mode:%d media_if:%d speed %u \n", setup_conf.mode, reset_conf.media_if,setup_conf.forced.speed);
            }
        }
    }
    return VTSS_RC_OK;
}

mesa_rc phy_icli_debug_phy_pass_through_speed(i32 session_id, icli_stack_port_range_t *plist, BOOL has_1g, BOOL has_100M, BOOL has_10M)
{
    vtss_phy_reset_conf_t reset_conf;
    vtss_phy_conf_t       setup_conf;
    switch_iter_t         sit;
    port_iter_t           pit;
  
    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            VTSS_RC(vtss_phy_reset_get(NULL, pit.iport, &reset_conf));
            reset_conf.media_if = VTSS_PHY_MEDIA_IF_SFP_PASSTHRU;
            VTSS_RC(vtss_phy_reset(NULL, pit.iport, &reset_conf));

            VTSS_RC(vtss_phy_conf_get(NULL, pit.iport, &setup_conf));
            setup_conf.mode = VTSS_PHY_MODE_FORCED;
            if (has_100M) {
                setup_conf.forced.speed = VTSS_SPEED_100M;                
            } 

            if (has_10M) {
                setup_conf.forced.speed = VTSS_SPEED_10M;              
            }

            if (has_1g) {
                setup_conf.forced.speed = VTSS_SPEED_1G;                
            }
            
            VTSS_RC(vtss_phy_conf_set(NULL, pit.iport, &setup_conf));
        }
    }
    return VTSS_RC_OK;
}

mesa_rc phy_icli_debug_do_page_chk(i32 session_id, BOOL has_enable, BOOL has_disable) {
    if (has_enable) {
        VTSS_RC(vtss_phy_do_page_chk_set(NULL, TRUE));
    } else if (has_disable) {
        VTSS_RC(vtss_phy_do_page_chk_set(NULL, FALSE));
    } else {
        BOOL enabled;
        VTSS_RC(vtss_phy_do_page_chk_get(NULL, &enabled));
        ICLI_PRINTF("Do page check is %s \n", enabled ? "enabled" : "disabled");
    }
    return VTSS_RC_OK;
}

//  see phy_icli_functions.h
mesa_rc phy_icli_debug_phy_reset(i32 session_id, icli_stack_port_range_t *plist) {
    port_iter_t   pit;
    switch_iter_t sit;
    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            port_phy_reset(pit.iport);
        }
    }
    return VTSS_RC_OK;
}
//  see phy_icli_functions.h
mesa_rc phy_icli_debug_phy_loop(i32 session_id, icli_stack_port_range_t *plist, phy_icli_loopback_t cli_lb, BOOL no) {
    port_iter_t   pit;
    switch_iter_t sit;
    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            mepa_loopback_t lb;
            VTSS_RC(meba_phy_loopback_get(board_instance, pit.iport, &lb));
            if (cli_lb.near) {
                lb.near_end_ena = !no;
            }

            if (cli_lb.far) {
                lb.far_end_ena = !no;
            }
            if (cli_lb.connector) {
                lb.connector_ena = !no;
            }
            if (cli_lb.mac_serdes_input) {
                lb.mac_serdes_input_ena = !no;
            }
            if (cli_lb.mac_serdes_facility) {
                lb.mac_serdes_facility_ena = !no;
            }
            if (cli_lb.mac_serdes_equipment) {
                lb.mac_serdes_equip_ena = !no;
            }
            if (cli_lb.media_serdes_input) {
                lb.media_serdes_input_ena = !no;
            }
            if (cli_lb.media_serdes_facility) {
                lb.media_serdes_facility_ena = !no;
            }
            if (cli_lb.media_serdes_equipment) {
                lb.media_serdes_equip_ena = !no;
            }
            if (cli_lb.qsgmii_tbi) {
                lb.qsgmii_pcs_tbi_ena = !no;
            }
            if (cli_lb.qsgmii_gmi) {
                lb.qsgmii_pcs_gmii_ena = !no;
            }
            if (cli_lb.qsgmii_serdes) {
                lb.qsgmii_serdes_ena = !no;
            }

            T_I("far_end:%d, near:%d, has_far:%d, has_near:%d", lb.far_end_ena, lb.near_end_ena, cli_lb.far, cli_lb.near);
            VTSS_RC(meba_phy_loopback_set(board_instance, pit.iport, &lb));
        }
    }
    return VTSS_RC_OK;
}
static const char *ring_res_type2txt(const vtss_phy_ring_resiliency_conf_t *rrng_rsln)
{
    switch(rrng_rsln->ring_res_status) {
        case VTSS_PHY_TIMING_DEFAULT_SLAVE: return "DEFAULT-TIMING-SLAVE ";
        case VTSS_PHY_TIMING_SLAVE_AS_MASTER: return "TIMING-SLAVE-AS-MASTER";
        case VTSS_PHY_TIMING_DEFAULT_MASTER: return  "DEFAULT-TIMING-MASTER";
        case VTSS_PHY_TIMING_MASTER_AS_SLAVE: return "TIMING-MASTER-AS-SLAVE";
        case VTSS_PHY_TIMING_LP_NOT_RESILIENT_CAP: return "TIMING_LP_NOT_RESILIENT_CAP";
    }
    return "INVALID";
}
mesa_rc phy_icli_debug_phy_ring_resiliency_conf(i32 session_id, icli_stack_port_range_t *plist, BOOL has_enable,
                                                BOOL has_disable, BOOL has_get)
{

    port_iter_t   pit;
    switch_iter_t sit;
    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {

        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            vtss_phy_ring_resiliency_conf_t r_rrslnt;
            VTSS_RC(vtss_phy_ring_resiliency_conf_get(PHY_INST, pit.iport, &r_rrslnt));
            if(has_get) {
                ICLI_PRINTF("RING-RESILIENCY enabled :%s\n", r_rrslnt.enable_rrslnt ? "TRUE" : "FALSE");
                ICLI_PRINTF("RING-RESILIENCY node-state :%s\n", ring_res_type2txt(&r_rrslnt));
                ICLI_PRINTF("\n\n\n");
            }
            if (has_enable) {
                r_rrslnt.enable_rrslnt = TRUE;
            }
            if (has_disable) {
                r_rrslnt.enable_rrslnt = FALSE;
            }
            if(has_disable || has_enable ) {
                VTSS_RC(vtss_phy_ring_resiliency_conf_set(PHY_INST, pit.iport, &r_rrslnt));
            }
        }
    }
    return VTSS_RC_OK;

}
//  see phy_icli_functions.h
mesa_rc phy_icli_debug_phy_gpio(i32 session_id, icli_stack_port_range_t *plist, BOOL has_mode_output, BOOL has_mode_input, BOOL has_mode_alternative, BOOL has_gpio_get, BOOL has_gpio_set, BOOL value, u8 gpio_no) {
    port_iter_t   pit;
    switch_iter_t sit;
    mepa_gpio_conf_t gpio_conf;
    gpio_conf.mode = MEPA_GPIO_MODE_OUT;
    gpio_conf.gpio_no = gpio_no;
    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (has_mode_output) {
                gpio_conf.mode = MEPA_GPIO_MODE_OUT;
                VTSS_RC(meba_phy_gpio_mode_set(board_instance, pit.iport, &gpio_conf));
            }

            if (has_mode_input) {
                gpio_conf.mode = MEPA_GPIO_MODE_IN;
                VTSS_RC(meba_phy_gpio_mode_set(board_instance, pit.iport, &gpio_conf));
            }

            if (has_mode_alternative) {
                gpio_conf.mode = MEPA_GPIO_MODE_ALT;
                VTSS_RC(meba_phy_gpio_mode_set(board_instance, pit.iport, &gpio_conf));
            }
            
            if (has_gpio_get) {
                value = 0;
                VTSS_RC(meba_phy_gpio_in_get(board_instance, pit.iport, gpio_no, &value));
                ICLI_PRINTF("GPIO:%d is %s\n", gpio_no, value ? "high" : "low");
            }

            if (has_gpio_set) {
                VTSS_RC(meba_phy_gpio_out_set(board_instance, pit.iport, gpio_no, value));
            }
        }
    }
    return VTSS_RC_OK;
}

mesa_rc phy_icli_debug_phy_led_set(i32 session_id,
                                   icli_stack_port_range_t *plist,
                                   BOOL has_led_num,
                                   u8 led_num,
                                   BOOL has_led_mode,
                                   u8 led_mode)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_gpio_conf_t      gpio_conf;
    gpio_conf.mode = MEPA_GPIO_MODE_OUT;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (has_led_num || has_led_mode) {
                gpio_conf.led_num = MEPA_LED0;
                if (has_led_num) {
                    gpio_conf.led_num = (mepa_led_num_t) led_num;
                }

                if (has_led_mode) {
                    gpio_conf.mode = (mepa_gpio_mode_t) (MEPA_GPIO_MODE_LED_LINK_ACTIVITY + led_mode);
                    VTSS_RC(meba_phy_gpio_mode_set(board_instance, pit.iport, &gpio_conf));
                } else {
                    ICLI_PRINTF("Select valid LED Mode\n");
                    gpio_conf.mode = MEPA_GPIO_MODE_LED_DISABLE_EXTENDED;
                    VTSS_RC(meba_phy_gpio_mode_set(board_instance, pit.iport, &gpio_conf));
                    return VTSS_RC_ERROR;
                }
            }
        }
    }
    return VTSS_RC_OK;
}

mesa_rc phy_icli_dbg_phy_clause45_access(i32 session_id, icli_stack_port_range_t *plist, uint16_t page, uint16_t address, BOOL read, uint16_t value, uint16_t count)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    uint32_t addr_page = (page << 16) | address;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (read) {
                ICLI_PRINTF("Port  Addr     Value   15      8 7       0\n");
                while (count) {
                    if (meba_phy_clause45_read(board_instance, pit.iport, addr_page, &value) == MESA_RC_OK) {
                        ICLI_PRINTF("%-6u0x%02x/%-4u0x%04x  ", pit.uport, address, address, value);
                        for (int i = 15; i >= 0; i--) {
                            ICLI_PRINTF("%u%s", value & (1<<i) ? 1 : 0, i == 0 ? "\n" : (i % 4) ? "" : ".");
                        }
                    }
                    count--;
                    addr_page++;
                    address++;
                }
            } else {
               (void)meba_phy_clause45_write(board_instance, pit.iport, addr_page, value);
            }
        }
    }
    return MESA_RC_OK;
}

mesa_rc phy_icli_dbg_phy_qsgmii_aneg_set(i32 session_id, icli_stack_port_range_t *plist, BOOL enable)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_conf_t                conf;
    mepa_rc                    rc;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if ((rc = meba_phy_conf_get(board_instance, pit.iport, &conf)) == MEPA_RC_OK) {
                conf.mac_if_aneg_ena = enable;
                rc = meba_phy_conf_set(board_instance, pit.iport, &conf);
            }
            if (rc != MEPA_RC_OK) {
                ICLI_PRINTF("Could not configure qsgmii auto-negotiation\n");
            }
        }
    }
    return MESA_RC_OK;
}

mepa_rc phy_icli_dbg_event_conf_status(i32 session_id, icli_stack_port_range_t *plist, BOOL has_set, BOOL enable, uint32_t events, BOOL has_get, BOOL has_poll)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc = MEPA_RC_OK;
    mepa_event_t               status;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (has_set) {
                if ((rc = meba_phy_event_enable_set(board_instance, pit.iport, events, enable)) != MEPA_RC_OK) {
                    ICLI_PRINTF("Event could not be enabled\n");
                }
            }
            if (has_get) {
                if ((rc = meba_phy_event_enable_get(board_instance, pit.iport, &status)) == MEPA_RC_OK) {
                    ICLI_PRINTF("Events enabled : 0x%x\n", status);
                }
            }
            if (has_poll) {
                if ((rc = meba_phy_event_poll(board_instance, pit.iport, &status)) == MEPA_RC_OK) {
                    ICLI_PRINTF("Events generated : 0x%x\n", status);
                }
            }
        }
    }
    if (rc != MEPA_RC_OK && (has_get | has_poll)) {
        ICLI_PRINTF("Events status get failed\n");
    }
    return rc;
}

mesa_rc phy_ts_mepa_eth_class_set(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t flow_id, mepa_bool_t pbb_en, bool has_etype, uint16_t etype, bool has_tpid, uint16_t tpid, mepa_ts_mac_match_select_t mac_match_select, mepa_ts_mac_match_mode_t mac_match_mode, bool has_mac, mesa_mac_t mac_addr, mepa_bool_t vlan_chk, bool has_tag_cnt, uint8_t num_tag, bool has_out_tag, bool has_out_rng, uint16_t out_rng_low, uint16_t out_rng_hi, bool has_out_val, uint16_t out_val, uint16_t out_mask, bool has_in_tag, bool has_in_rng, uint16_t in_rng_low, uint16_t in_rng_hi, bool has_in_val, uint16_t in_val, uint16_t in_mask)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc = MEPA_RC_OK;
    mepa_ts_classifier_t       ts;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (ingr) {
                rc = meba_phy_ts_rx_classifier_conf_get(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get rx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            } else {
                rc = meba_phy_ts_tx_classifier_conf_get(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get tx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            }

            mepa_ts_classifier_eth_t *eth = &ts.eth_class_conf;
            eth->mac_match_mode = mac_match_mode;
            eth->mac_match_select = mac_match_select;
            memcpy(&eth->mac_addr[0], &mac_addr, sizeof(eth->mac_addr));
            eth->vlan_check = vlan_chk;
            eth->vlan_conf.pbb_en = pbb_en;
            eth->vlan_conf.tpid = tpid;
            eth->vlan_conf.etype = etype;
            eth->vlan_conf.num_tag = has_tag_cnt ? num_tag : 0;
            if (has_out_tag) {
                if (has_out_rng) {
                    eth->vlan_conf.outer_tag.mode = MEPA_TS_MATCH_MODE_RANGE;
                    eth->vlan_conf.outer_tag.match.range.upper = (uint16_t)out_rng_low;
                    eth->vlan_conf.outer_tag.match.range.lower = (uint16_t)out_rng_hi;
                } else if (has_out_val) {
                    eth->vlan_conf.outer_tag.mode = MEPA_TS_MATCH_MODE_VALUE;
                    eth->vlan_conf.outer_tag.match.value.val = out_val;
                    eth->vlan_conf.outer_tag.match.value.mask = out_mask;
                }
            } else if (has_in_tag) {
                if (has_in_rng) {
                    eth->vlan_conf.inner_tag.mode = MEPA_TS_MATCH_MODE_RANGE;
                    eth->vlan_conf.inner_tag.match.range.upper = in_rng_low;
                    eth->vlan_conf.inner_tag.match.range.lower = in_rng_hi;
                } else if (has_in_val) {
                    eth->vlan_conf.inner_tag.mode = MEPA_TS_MATCH_MODE_VALUE;
                    eth->vlan_conf.inner_tag.match.value.val = in_val;
                    eth->vlan_conf.inner_tag.match.value.mask = in_mask;
                }
            }
            if (ingr) {
                rc = meba_phy_ts_rx_classifier_conf_set(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not set rx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            } else {
                rc = meba_phy_ts_tx_classifier_conf_set(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not set tx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            }
        }
    }
    return MESA_RC_OK;
}

mesa_rc phy_ts_mepa_ip_class_set(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t flow_id, bool has_udp_src_port, uint16_t udp_src_port, bool has_udp_dst_port, uint16_t udp_dst_port, mepa_ts_ip_match_select_t ip_match_mode, bool has_ipv4, mesa_ipv4_t ipv4_addr,  mesa_ipv4_t ipv4_mask, BOOL has_ipv6, mesa_ipv6_t ipv6_addr, mesa_ipv6_t ipv6_mask)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc = MEPA_RC_OK;
    mepa_ts_classifier_t       ts;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (ingr) {
                rc = meba_phy_ts_rx_classifier_conf_get(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get rx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            } else {
                rc = meba_phy_ts_tx_classifier_conf_get(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get tx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            }

            mepa_ts_classifier_ip_t *ip = &ts.ip_class_conf;
            ip->ip_match_mode = ip_match_mode;
            if (has_ipv4) {
                ip->ip_ver = MEPA_TS_IP_VER_4;
                ip->ip_addr.ipv4.addr = ipv4_addr;
                ip->ip_addr.ipv4.mask = ipv4_mask;
            } else if (has_ipv6) {
                ip->ip_ver = MEPA_TS_IP_VER_6;
                ip->ip_addr.ipv6.addr[3] = ipv6_addr.addr[0] << 24 | ipv6_addr.addr[1] << 16 | ipv6_addr.addr[2] << 8 | ipv6_addr.addr[3];
                ip->ip_addr.ipv6.addr[2] = ipv6_addr.addr[4] << 24 | ipv6_addr.addr[5] << 16 | ipv6_addr.addr[6] << 8 | ipv6_addr.addr[7];
                ip->ip_addr.ipv6.addr[1] = ipv6_addr.addr[8] << 24 | ipv6_addr.addr[9] << 16 | ipv6_addr.addr[10] << 8 | ipv6_addr.addr[11];
                ip->ip_addr.ipv6.addr[0] = ipv6_addr.addr[12] << 24 | ipv6_addr.addr[13] << 16 | ipv6_addr.addr[14] << 8 | ipv6_addr.addr[15];
                ip->ip_addr.ipv6.mask[3] = ipv6_mask.addr[0] << 24 | ipv6_mask.addr[1] << 16 | ipv6_mask.addr[2] << 8 | ipv6_mask.addr[3];
                ip->ip_addr.ipv6.mask[2] = ipv6_mask.addr[4] << 24 | ipv6_mask.addr[5] << 16 | ipv6_mask.addr[6] << 8 | ipv6_mask.addr[7];
                ip->ip_addr.ipv6.mask[1] = ipv6_mask.addr[8] << 24 | ipv6_mask.addr[9] << 16 | ipv6_mask.addr[10] << 8 | ipv6_mask.addr[11];
                ip->ip_addr.ipv6.mask[0] = ipv6_mask.addr[12] << 24 | ipv6_mask.addr[13] << 16 | ipv6_mask.addr[14] << 8 | ipv6_mask.addr[15];
            }
            ip->udp_sport_en = has_udp_src_port ? TRUE : FALSE;
            ip->udp_sport = udp_src_port;
            ip->udp_dport_en = has_udp_dst_port ? TRUE : FALSE;
            ip->udp_dport = udp_dst_port;
            if (ingr) {
                rc = meba_phy_ts_rx_classifier_conf_set(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not set rx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            } else {
                rc = meba_phy_ts_tx_classifier_conf_set(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not set tx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            }
        }
    }
    return MESA_RC_OK;
}

mesa_rc phy_ts_mepa_class_comm_set(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t flow_id, mepa_ts_pkt_encap_t encap, uint8_t clock_id)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc = MEPA_RC_OK;
    mepa_ts_classifier_t       ts;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (ingr) {
                rc = meba_phy_ts_rx_classifier_conf_get(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get rx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            } else {
                rc = meba_phy_ts_tx_classifier_conf_get(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get tx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            }
            ts.pkt_encap_type = encap;
            ts.clock_id = clock_id;
            if (ingr) {
                rc = meba_phy_ts_rx_classifier_conf_set(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not set rx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            } else {
                rc = meba_phy_ts_tx_classifier_conf_set(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not set tx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            }
        }
    }

    return MESA_RC_OK;
}

mesa_rc phy_ts_mepa_clock_set(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t clock_id, BOOL enable, bool has_clk_type, mepa_ts_ptp_clock_mode_t clk_mode, bool has_delay_type, mepa_ts_ptp_delaym_type_t delaym_type, bool has_cf_update, mepa_bool_t cf_update, bool has_version_rng, uint8_t ver_low, uint8_t ver_hi, bool has_minor_ver, uint8_t min_ver_low, uint8_t min_ver_hi, bool has_domain, bool has_domain_range, uint8_t domain_rng_low, uint8_t domain_rng_hi, bool has_domain_val, uint8_t domain_val, uint8_t domain_mask, bool has_sdoid, bool has_sdoid_rng, uint8_t sdoid_rng_lo, uint8_t sdoid_rng_hi, bool has_sdoid_val, uint8_t sdoid_val, uint8_t sdoid_mask)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc = MEPA_RC_OK;
    mepa_ts_ptp_clock_conf_t   ts;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (ingr) {
                rc = meba_phy_ts_rx_clock_conf_get(board_instance, pit.iport, clock_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get rx clock conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            } else {
                rc = meba_phy_ts_tx_clock_conf_get(board_instance, pit.iport, clock_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get tx clock conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            }
            ts.enable = enable;
            if (has_clk_type) {
                ts.clk_mode = clk_mode;
            }
            if (has_delay_type) {
                ts.delaym_type = delaym_type;
            }
            if (has_cf_update) {
                ts.cf_update = cf_update;
            }
            if (has_version_rng) {
                ts.ptp_class_conf.version.upper = ver_hi;
                ts.ptp_class_conf.version.lower = ver_low;
            }
            if (has_minor_ver) {
                ts.ptp_class_conf.minor_version.upper = min_ver_hi;
                ts.ptp_class_conf.minor_version.lower = min_ver_low;
            }
            
            if (has_domain) {
                if (has_domain_range) {
                    ts.ptp_class_conf.domain.mode = MEPA_TS_MATCH_MODE_RANGE;
                    ts.ptp_class_conf.domain.match.range.upper = domain_rng_hi;
                    ts.ptp_class_conf.domain.match.range.lower = domain_rng_low;
                } else if (has_domain_val) {
                    ts.ptp_class_conf.domain.mode = MEPA_TS_MATCH_MODE_VALUE;
                    ts.ptp_class_conf.domain.match.value.val = domain_val;
                    ts.ptp_class_conf.domain.match.value.mask = domain_mask;
                }
            } else if (has_sdoid) {
                if (has_sdoid_rng) {
                    ts.ptp_class_conf.sdoid.mode = MEPA_TS_MATCH_MODE_RANGE;
                    ts.ptp_class_conf.sdoid.match.range.upper = sdoid_rng_hi;
                    ts.ptp_class_conf.sdoid.match.range.lower = sdoid_rng_lo;
                } else if (has_sdoid_val) {
                    ts.ptp_class_conf.sdoid.mode = MEPA_TS_MATCH_MODE_VALUE;
                    ts.ptp_class_conf.sdoid.match.value.val = sdoid_val;
                    ts.ptp_class_conf.sdoid.match.value.mask = sdoid_mask;
                }
            }
            if (ingr) {
                rc = meba_phy_ts_rx_clock_conf_set(board_instance, pit.iport, clock_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not set rx clock conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            } else {
                rc = meba_phy_ts_tx_clock_conf_set(board_instance, pit.iport, clock_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not set tx clock conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            }
        }
    }
    return MESA_RC_OK;
}

void phy_ts_mepa_eth_class_get(i32 session_id, const mepa_ts_classifier_eth_t *conf)
{
    ICLI_PRINTF("Ethernet classifier common configuration\n");
    ICLI_PRINTF("pbb_en %s\n", conf->vlan_conf.pbb_en ? "true" : "false");
    ICLI_PRINTF("tpid 0x%x\n", conf->vlan_conf.tpid);
    ICLI_PRINTF("etype 0x%x\n", conf->vlan_conf.etype);

    ICLI_PRINTF("EThernet flow specific configuration\n");
    ICLI_PRINTF("mac_match_mode : %s \n", conf->mac_match_mode == MEPA_TS_ETH_ADDR_MATCH_48BIT ? "MATCH_48BIT" :
                conf->mac_match_mode == MEPA_TS_ETH_ADDR_MATCH_ANY_UNICAST ? "MATCH_ANY_UNICAST" :
                conf->mac_match_mode == MEPA_TS_ETH_ADDR_MATCH_ANY_MULTICAST ? "MATCH_ANY_MULTICAST" : "MATCH_ANY");
    ICLI_PRINTF("mac_match_select : %s \n", conf->mac_match_select == MEPA_TS_ETH_MATCH_NONE? "MATCH_NONE" :
                conf->mac_match_select == MEPA_TS_ETH_MATCH_SRC_ADDR ? "MATCH_SRC" :
                conf->mac_match_select == MEPA_TS_ETH_MATCH_SRC_OR_DEST ? "MATCH_SRC_OR_DEST" : "MATCH_DEST");
    ICLI_PRINTF("mac_addr : 0x");
    for (int i = 0; i < 6; i++) {
        ICLI_PRINTF("%x%x%c", (conf->mac_addr[i]>>4), (conf->mac_addr[i] & 0xF), (i!=5 ? '-':' '));
    }
    ICLI_PRINTF("\n");
    ICLI_PRINTF("vlan_check : %s\n", conf->vlan_check ? "true" : "false");
    ICLI_PRINTF("number of tags : %d\n", conf->vlan_conf.num_tag);
    if (conf->vlan_conf.outer_tag.mode == MEPA_TS_MATCH_MODE_RANGE) {
        ICLI_PRINTF("outer tag : match mode is range\n");
        ICLI_PRINTF("            range is [%d, %d]\n", conf->vlan_conf.outer_tag.match.range.lower, conf->vlan_conf.outer_tag.match.range.upper);
    } else {
        ICLI_PRINTF("outer tag : match mode is value\n");
        ICLI_PRINTF("            value 0x%x mask 0x%x\n", conf->vlan_conf.outer_tag.match.value.val, conf->vlan_conf.outer_tag.match.value.mask);
    }
    if (conf->vlan_conf.inner_tag.mode == MEPA_TS_MATCH_MODE_RANGE) {
        ICLI_PRINTF("inner tag : match mode is range\n");
        ICLI_PRINTF("            range is [%d, %d]\n", conf->vlan_conf.inner_tag.match.range.lower, conf->vlan_conf.inner_tag.match.range.upper);
    } else {
        ICLI_PRINTF("inner tag : match mode is value\n");
        ICLI_PRINTF("            value 0x%x mask 0x%x\n", conf->vlan_conf.inner_tag.match.value.val, conf->vlan_conf.inner_tag.match.value.mask);
    }
}
void phy_ts_mepa_ip_class_get(i32 session_id, const mepa_ts_classifier_ip_t *conf)
{
    ICLI_PRINTF("IP classifier common configuration\n");
    ICLI_PRINTF("ip version : %s\n", conf->ip_ver == MEPA_TS_IP_VER_6 ? "ipv6" : "ipv4");
    ICLI_PRINTF("UDP source port enabled : %s\n", conf->udp_sport_en ? "true" : "false");
    ICLI_PRINTF("UDP destination port enabled : %s\n", conf->udp_dport_en ? "true" : "false");
    ICLI_PRINTF("UDP source port : %d\n", conf->udp_sport);
    ICLI_PRINTF("UDP destination port : %d\n", conf->udp_dport);

    ICLI_PRINTF("\n IP classifier flow specific configuration\n");
    ICLI_PRINTF("IP match mode : %s\n", conf->ip_match_mode == MEPA_TS_IP_MATCH_DEST ? "MATCH_DEST" :
                 conf->ip_match_mode == MEPA_TS_IP_MATCH_SRC ? "MATCH_SRC" :
                 conf->ip_match_mode == MEPA_TS_IP_MATCH_SRC_OR_DEST ? "MATCH_SRC_OR_DEST" : "MATCH_NONE");
    ICLI_PRINTF("IP address : ");
    if (conf->ip_ver == MEPA_TS_IP_VER_4) {
        for (int i = 3; i >= 0; i--) {
            ICLI_PRINTF("%d%c", conf->ip_addr.ipv4.addr >> i, i ? '.' : ' ');
        }
    } else {
        for (int i = 0; i < 4; i++) {
            ICLI_PRINTF("%x:%x%c", conf->ip_addr.ipv6.addr[i] & 0xFFFF, conf->ip_addr.ipv6.addr[i] >> 16, (i!=3) ? ':' : ' ');
        }
    }

    ICLI_PRINTF("\n");
}
mepa_rc phy_ts_mepa_class_get(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t flow_id, bool has_eth, bool has_ip)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc = MEPA_RC_OK;
    mepa_ts_classifier_t       ts;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (ingr) {
                rc = meba_phy_ts_rx_classifier_conf_get(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get rx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            } else {
                rc = meba_phy_ts_tx_classifier_conf_get(board_instance, pit.iport, flow_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get tx classifier conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            }
            ICLI_PRINTF("\n Classifier configuration for %s  port %d\n", ingr ? "ingress" : "egress", pit.iport);
            ICLI_PRINTF(" enable : %s \n", ts.enable ? "TRUE" : "FALSE");
            ICLI_PRINTF(" encapsulation : %s \n", ts.pkt_encap_type == MEPA_TS_ENCAP_ETH_PTP ? "eth" :
                          ts.pkt_encap_type == MEPA_TS_ENCAP_ETH_IP_PTP ? "ip" : "none");
            ICLI_PRINTF(" clock id associated : ");
            if (ts.clock_id <= 23) {
                ICLI_PRINTF(" %u\n", ts.clock_id);
            } else {
                ICLI_PRINTF(" Invalid\n");
            }
            if (has_eth) {
                phy_ts_mepa_eth_class_get(session_id, &ts.eth_class_conf);
            }
            if (has_ip) {
                phy_ts_mepa_ip_class_get(session_id, &ts.ip_class_conf);
            }
        }
    }
    return MEPA_RC_OK;
}
mepa_rc phy_ts_mepa_clock_get(i32 session_id, icli_stack_port_range_t *plist, bool ingr, uint8_t clock_id)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc = MEPA_RC_OK;
    mepa_ts_ptp_clock_conf_t   ts;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (ingr) {
                rc = meba_phy_ts_rx_clock_conf_get(board_instance, pit.iport, clock_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get rx clock conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            } else {
                rc = meba_phy_ts_tx_clock_conf_get(board_instance, pit.iport, clock_id, &ts);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get tx clock conf for port %d rc=%d\n", pit.iport, rc);
                    break;
                }
            }
            ICLI_PRINTF("Clock configuration for %s port %d id %d\n", ingr ? "ingress" : "egress", pit.iport, clock_id);
            ICLI_PRINTF(" clock mode : %s\n", ts.clk_mode == MEPA_TS_PTP_CLOCK_MODE_BC1STEP ? "BC 1-Step" :
                          ts.clk_mode == MEPA_TS_PTP_CLOCK_MODE_BC2STEP ? "BC 2-Step" :
                          ts.clk_mode == MEPA_TS_PTP_CLOCK_MODE_TC1STEP ? "TC 1-Step" :
                          ts.clk_mode == MEPA_TS_PTP_CLOCK_MODE_TC2STEP ? "TC 2-Step" : "None");
            ICLI_PRINTF(" delay method : %s\n", ts.delaym_type == MEPA_TS_PTP_DELAYM_E2E ? "E2E" : "P2P");
            ICLI_PRINTF(" Version range : [%d, %d]\n", ts.ptp_class_conf.version.lower, ts.ptp_class_conf.version.upper);
            ICLI_PRINTF(" Minor version range : [%d, %d]\n", ts.ptp_class_conf.minor_version.lower, ts.ptp_class_conf.minor_version.upper);
            if (ts.ptp_class_conf.domain.mode == MEPA_TS_MATCH_MODE_RANGE) {
                ICLI_PRINTF(" Domain range : [%d, %d]\n", ts.ptp_class_conf.domain.match.range.lower, ts.ptp_class_conf.domain.match.range.upper);
            } else {
                ICLI_PRINTF(" Domain value-mask : val 0x%x mask 0x%x\n", ts.ptp_class_conf.domain.match.value.val, ts.ptp_class_conf.domain.match.value.mask);
            }
        }
    }
    return MEPA_RC_OK;
}
mepa_rc phy_ts_mepa_port_mode_set(i32 session_id, icli_stack_port_range_t *plist, BOOL enable)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc = MEPA_RC_OK;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            rc = meba_phy_ts_mode_set(board_instance, pit.iport, enable);
            if (rc != MEPA_RC_OK) {
                ICLI_PRINTF("Mode set failed for port %d\n", pit.iport);
                break;
            }
        }
    }
    return MEPA_RC_OK;
}

mepa_rc phy_ts_mepa_fifo_empty(i32 session_id, icli_stack_port_range_t *plist, BOOL fifo_get)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc = MEPA_RC_OK;
    mepa_fifo_ts_entry_t       ts_list[8];
    uint32_t                   num;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (fifo_get) {
                rc = meba_phy_ts_fifo_get(board_instance, pit.iport, ts_list, 8, &num);
                for (int i = 0; i < num; i++) {
                    ICLI_PRINTF("seq[%d],msg[%d]: seconds high %d low %d nano 0x%x\n", ts_list[i].sig.sequence_id, ts_list[i].sig.msg_type, ts_list[i].ts.seconds.high, ts_list[i].ts.seconds.low, ts_list[i].ts.nanoseconds);
                }
            } else {
                rc = meba_phy_ts_fifo_empty(board_instance, pit.iport);
            }
            if (rc != MEPA_RC_OK) {
                ICLI_PRINTF("FIFO Empty failed for port %d\n", pit.iport);
                break;
            }
        }
    }
    return MEPA_RC_OK;
}

mepa_rc phy_icli_dbg_phy_ts_mepa_init_set( i32 session_id, icli_stack_port_range_t *plist,bool has_clk_freq, bool  has_clk_src, bool  has_rx_ts_pos, bool  has_rx_ts_len,bool  has_tx_ts_len,bool has_ls_clear , bool has_tc_mode, bool has_dly_req_rcv_10, mepa_ts_clock_freq_t clk_freq, mepa_ts_clock_src_t clk_src, mepa_ts_rx_timestamp_pos_t rx_ts_pos, mepa_ts_rx_timestamp_len_t rx_ts_len, mepa_ts_fifo_timestamp_len_t  tx_ts_len,mepa_bool_t clear, mepa_ts_tc_op_mode_t tc_op_mode, mepa_bool_t dly_req_recv_10byte_ts, mepa_bool_t auto_ts)
{

    switch_iter_t              sit; 
    port_iter_t                pit; 
    mepa_rc                    rc = MEPA_RC_OK;
    mepa_phy_info_t            info = {};
    mepa_ts_init_conf_t        phy_conf;
    
    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (meba_phy_info_get(board_instance, pit.iport, &info) == MESA_RC_OK) {
                if (info.cap & MEPA_CAP_TS_MASK_NONE) {
                    ICLI_PRINTF("Timestamping not supported on this port %d\n", pit.iport);
                    continue;
                }
            }
            rc = meba_phy_ts_init_conf_get(board_instance, pit.iport, &phy_conf);       
            if(rc != MEPA_RC_OK) {
                ICLI_PRINTF("Could not get init  for port %d\n", pit.iport);
                break;  
            }
	        if(has_clk_freq) {
	            phy_conf.clk_freq = clk_freq;
	        }
	        if(has_clk_src) {
	            phy_conf.clk_src = clk_src;
	        }
	        if(has_rx_ts_pos) {
	            phy_conf.rx_ts_pos =  rx_ts_pos;
	        }
	        if(has_rx_ts_len) {
	            phy_conf.rx_ts_len =  rx_ts_len;
	        }
	        if(has_tx_ts_len) {
	            phy_conf.tx_ts_len =  tx_ts_len;
	        }
            if(has_ls_clear) {
                phy_conf.auto_clear_ls  =  clear;
            }
	        if(has_tc_mode) {
	            phy_conf.tc_op_mode = tc_op_mode;
	        }
	        if(has_dly_req_rcv_10) {
	            phy_conf.dly_req_recv_10byte_ts = dly_req_recv_10byte_ts;
	        }
            phy_conf.tx_auto_followup_ts = auto_ts;
            if( phy_conf.clk_freq == 0 ) {
                ICLI_PRINTF("Reference clock frequency : 125 MHz\n ");
            }else if(phy_conf.clk_freq == 1 ) {
                ICLI_PRINTF("Reference clock frequency : 156.25 MHz\n ");
            }else if(phy_conf.clk_freq == 2 ) {
                ICLI_PRINTF("Reference clock frequency : 200 MHz\n ");
            }else if(phy_conf.clk_freq == 3 ) {
                ICLI_PRINTF("Reference clock frequency : 250 MHz\n ");
            }else if(phy_conf.clk_freq == 4 ) {
                ICLI_PRINTF("Reference clock frequency : 500 MHz\n ");
            }else {
                ICLI_PRINTF("Reference clock frequency : Max Frequency\n");
            }
            ICLI_PRINTF("Clock Source:%s\n", phy_conf.clk_src == 4 ? "125MHZ internal sys PLL ":(phy_conf.clk_src == 5?"125MHZ qsgmii rec clock":(phy_conf.clk_src == 6 ?"external 1588 clock reference":"Unknow")));
            ICLI_PRINTF("Rx timestamp position : %s\n",phy_conf.rx_ts_pos ? "4 bytes at the end of frame" : "4 bytes of PTP header");
            ICLI_PRINTF("Rx timestamp length : %s\n", phy_conf.rx_ts_len  ? "nanosecCounter + secCounter" : "The nanosecCounter");
            ICLI_PRINTF("Timestamp size in Tx TSFIFO : %s\n", phy_conf.tx_ts_len ? "10 byte Tx timestamp" : "4 byte Tx timestamp");
            ICLI_PRINTF("Auto clear ls : %s\n", phy_conf.auto_clear_ls ? "manual":"auto");
            ICLI_PRINTF("TC operating mode : %s\n", phy_conf.tc_op_mode ? (phy_conf.tc_op_mode == 1?"Mode B":(phy_conf.tc_op_mode == 2 ?"Mode C":"Unknow")):"Mode A");
            ICLI_PRINTF("auto delay req/response : %s\n", phy_conf.dly_req_recv_10byte_ts ? "True" : "False");
            ICLI_PRINTF("Automatic timestamp insertion in followup message : %s\n", phy_conf.tx_auto_followup_ts ? "True" : "False");
	        rc = meba_phy_ts_init_conf_set(board_instance, pit.iport, &phy_conf);
	        if(rc != MEPA_RC_OK) {
	            ICLI_PRINTF("Could not set successfully " );
	            break;
		    }
        }
    }
    return MEPA_RC_OK;    
}

mepa_rc phy_icli_show_sqi_value(i32 session_id, icli_stack_port_range_t *plist)
{

    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc = MEPA_RC_OK;
    uint32_t                   val;

    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            rc = meba_phy_sqi_read( board_instance, pit.iport, &val);
            if(rc != MEPA_RC_OK) {
                ICLI_PRINTF("Could not get SQI value for port %d\n", pit.iport);
            }
            else {
                ICLI_PRINTF("SQI value : %d\n", val);
            }
        }
    }
    return MEPA_RC_OK;
}

mesa_rc phy_icli_dbg_phy_mdi(i32 session_id, icli_stack_port_range_t *plist, bool has_mdi, bool has_mdix, bool has_auto)
{
    switch_iter_t              sit;
    mepa_media_mode_t          mode = MEPA_MEDIA_MODE_AUTO;
    port_iter_t                pit;
    mepa_conf_t                conf;
    mepa_rc                    rc;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            rc = meba_phy_conf_get(board_instance, pit.iport, &conf);
            if (rc != MEPA_RC_OK) {
                ICLI_PRINTF("Could not get mdi mode  for port %d\n", pit.iport);
                break;
            }
            if (has_mdi || has_mdix || has_auto) {
                mode = has_mdi ? MEPA_MEDIA_MODE_MDI :(has_mdix ? MEPA_MEDIA_MODE_MDIX : MEPA_MEDIA_MODE_AUTO);
                conf.mdi_mode = mode;
                rc = meba_phy_conf_set(board_instance, pit.iport, &conf);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not configure mdi mode for port %d \n", pit.iport);
                }
            }
            else {
                ICLI_PRINTF("MDI mode : %s \n",(conf.mdi_mode == 0)? "AUTO" :((conf.mdi_mode == 1)?"MDI":"MDIX"));
            }
        }
    }
    return MESA_RC_OK;
}

mepa_rc  phy_icli_dbg_phy_start_of_frame_pulses(i32 session_id, icli_stack_port_range_t *plist, mepa_bool_t ingress, bool has_dir, bool has_sofno, uint8_t sofno, bool has_start_of_frame_preemption_val, mepa_preemption_mode_t preemption)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc;
    mepa_start_of_frame_conf_t conf;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            rc = meba_phy_start_of_frame_conf_get(board_instance, pit.iport, &conf);
            if (rc != MEPA_RC_OK) {
                ICLI_PRINTF("Could not get SOF  for port %d\n", pit.iport);
                break;
            }
            if (has_dir || has_sofno || has_start_of_frame_preemption_val) {
                conf.ingress = ingress;
                conf.sof_no = sofno;
                conf.sof_preemption_mode = preemption;
                rc = meba_phy_start_of_frame_conf_set(board_instance, pit.iport, &conf);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not set SOF  for port %d\n", pit.iport);
                }
            }
            else {
                ICLI_PRINTF("Packet transmission direction : %s \n",(conf.ingress == 1)? "Receive" : "Trasmit");
                ICLI_PRINTF("SOF no : %d \n",conf.sof_no);
                ICLI_PRINTF("Preemption value : %d \n", conf.sof_preemption_mode);
            }
        }
    }
    return MESA_RC_OK;
}

mesa_rc phy_icli_dbg_phy_frame_preemption(i32 session_id, icli_stack_port_range_t *plist, bool has_enable, bool has_disable)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc;
    mepa_reset_param_t         reset;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            rc = meba_phy_framepreempt_get(board_instance, pit.iport, &(reset.framepreempt_en));
            if (rc != MEPA_RC_OK) {
                ICLI_PRINTF("Could not get frame preemption for port %d, err=%d\n", pit.iport, rc);
                break;
            }
            if (has_enable || has_disable) {
                reset.framepreempt_en = has_enable ? TRUE : FALSE;
                rc = meba_phy_reset(board_instance, pit.iport, &reset);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not configure frame preemption \n");
                }
            }
            else {
                ICLI_PRINTF("Frame preemption : %s \n",(reset.framepreempt_en == 0) ? "Disable" : "Enable");
            }
        }
    }
    return MESA_RC_OK;
}

mesa_rc phy_icli_dbg_phy_selftest(i32 session_id, icli_stack_port_range_t *plist, mepa_port_speed_t speed, mepa_media_mode_t mdi, uint32_t frames, bool has_start, bool has_read)
{
    switch_iter_t              sit;
    port_iter_t                pit;
    mepa_rc                    rc;

    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if (has_read) {
                mepa_selftest_info_t       inf;

                rc = meba_selftest_read(board_instance, pit.iport, &inf);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Failed to read selftest check for port %d \n", pit.iport);
                    break;
                }
                ICLI_PRINTF("Speed : %s \n",(inf.speed == MESA_SPEED_10M) ? "10M" : ((inf.speed == MESA_SPEED_100M) ? "100M" :((inf.speed == MESA_SPEED_1G) ? "1G" : "AUTO")));
                ICLI_PRINTF("MDI : %s \n",(inf.mdi == MEPA_MEDIA_MODE_AUTO) ? "Auto" : (inf.mdi == MEPA_MEDIA_MODE_MDI) ? "MDI" : "MDIX");
                ICLI_PRINTF("Frames : %d \n",inf.frames);
                ICLI_PRINTF("Good count : %d \n",inf.good_cnt);
                ICLI_PRINTF("Error count : %d \n",inf.err_cnt);
            } else if (has_start) {
                mepa_selftest_info_t       inf = {speed, mdi, frames, 0, 0};

                if (inf.speed == MESA_SPEED_1G && inf.mdi != MEPA_MEDIA_MODE_AUTO) {
                    ICLI_PRINTF("Invalid mode=%s for 1G speed for port %d \n",
                            (inf.mdi == MEPA_MEDIA_MODE_MDI ? "MDI" : "MDIX"), pit.iport);
                    break;
                } else if (inf.speed != MESA_SPEED_1G && inf.mdi == MEPA_MEDIA_MODE_AUTO) {
                    ICLI_PRINTF("Invalid mode=AUTO for speed=%s for port %d \n",
                            (inf.speed == MESA_SPEED_100M ? "100M" : "10M"), pit.iport);
                    break;
                }
                rc = meba_selftest_start(board_instance, pit.iport, &inf);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Failed to run selftest check for port %d \n", pit.iport);
                    break;
                }
                ICLI_PRINTF("Speed : %s \n",(inf.speed == MESA_SPEED_10M) ? "10M" : ((inf.speed == MESA_SPEED_100M) ? "100M" :((inf.speed == MESA_SPEED_1G) ? "1G" : "AUTO")));
                ICLI_PRINTF("MDI : %s \n",(inf.mdi == MEPA_MEDIA_MODE_AUTO) ? "Auto" : (inf.mdi == MEPA_MEDIA_MODE_MDI) ? "MDI" : "MDIX");
                ICLI_PRINTF("Frames : %d \n",inf.frames);
            }
        }
    }
    return MESA_RC_OK;
}

mesa_rc phy_icli_crc_gen(i32 session_id, char *input_clk, uint16_t port_id)
{
    int clk[8] = {};
    char crc[12] = {};
    char crc_input[10] = {};
    char byte, bit, invert;
    int i;
    uint16_t ret = 0;

    (void)sscanf(input_clk, "%x:%x:%x:%x:%x:%x:%x:%x", &clk[0], &clk[1], &clk[2], &clk[3], &clk[4], &clk[5], &clk[6], &clk[7]);
    for (i = 0; i < 8; i++) {
        crc_input[i] = (uint8_t)clk[i];
    }
    crc_input[8] = (port_id & 0xFF00) >> 8;
    crc_input[9] = port_id & 0xFF;

    // Initialize crc
    for (i = 0; i < 12; i++) {
        crc[i] = 1;
    }

    // Calculate crc
    for (i = 0; i < 10; i++) {
        byte = crc_input[i];

        // X^12 + X^11 + X^3 + X^2 + X^1 + 1
        for (int j = 0; j < 8; j++) {
            bit = byte & 0x1; //lowest order bit first processed
            byte = byte >> 1;
            invert = bit ^ crc[11];

            crc[11] = crc[10] ^ invert;
            crc[10] = crc[9];
            crc[9]  = crc[8];
            crc[8]  = crc[7];
            crc[7]  = crc[6];
            crc[6]  = crc[5];
            crc[5]  = crc[4];
            crc[4]  = crc[3];
            crc[3]  = crc[2] ^ invert;
            crc[2]  = crc[1] ^ invert;
            crc[1]  = crc[0] ^ invert;
            crc[0]  = invert;
        }
    }

    // Convert binary crc into number
    for (int j = 11; j > 0; j--) {
        ret |= crc[j];
        ret = ret << 1;
    }
    ret |= crc[0];
    ICLI_PRINTF("crc-12 calculated : 0x%x\n", ret);

    return MESA_RC_OK;
}

mepa_rc phy_icli_dbg_phy_prbs(i32 session_id, icli_stack_port_range_t *plist, bool enable, mepa_phy_prbs_type_t type, mepa_phy_prbs_direction_t direction, mepa_prbs_pattern_t prbsn_sel, bool is_set, mepa_prbs_clock_t clk, mepa_prbs_loopback_t loopback)
{
    switch_iter_t                   sit;
    port_iter_t                     pit;
    mepa_rc                         rc;
    mepa_phy_prbs_generator_conf_t  prbs_config;
    // Loop through all switches in a stack
    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop though the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            rc = meba_prbs_get(board_instance, pit.iport, type, direction, &prbs_config);
            if (rc != MEPA_RC_OK) {
                ICLI_PRINTF("Could not get PRBS for port %d\n", pit.iport);
                break;
            }
            if(is_set) {
                prbs_config.enable = enable;
                prbs_config.prbsn_sel = prbsn_sel;
                prbs_config.clk = clk;
                prbs_config.loopback = loopback;
                rc = meba_prbs_set(board_instance, pit.iport, type, direction, &prbs_config);
                if (rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Issue with clock settings \n");
                    break;
                }
                ICLI_PRINTF("Type : %s\n",type ? "PCS" : "Serdes");
                ICLI_PRINTF("Direction : %s\n",direction ? "Line" : "Host");
            }
            ICLI_PRINTF("PRBS : %s\n",(prbs_config.enable == 1)? "Enable":(prbs_config.enable == 2)? "Disable-reset" : "Disable");
            ICLI_PRINTF("PRBS : %s\n",(prbs_config.prbsn_sel == 0) ? "PRBS7" :((prbs_config.prbsn_sel == 1) ? "PRBS15" : "PRBS31"));
            ICLI_PRINTF("CLOCK : %s\n",(prbs_config.clk == 0) ? "25MHz" : "125MHz");
            ICLI_PRINTF("LOOPBACK : %s\n",(prbs_config.loopback == 0) ? "Internal" : "External");
        }
    }
    return MEPA_RC_OK;
}

mepa_rc phy_icli_show_prbs_checker_value(i32 session_id, icli_stack_port_range_t *plist, mepa_prbs_pattern_t prbsn_sel, bool has_inject, bool has_value)
{
    switch_iter_t                sit;
    port_iter_t                  pit;
    mepa_rc                      rc = MEPA_RC_OK;
    mepa_phy_prbs_monitor_conf_t error_conf;

    VTSS_RC(icli_switch_iter_init(&sit));
    while (icli_switch_iter_getnext(&sit, plist)) {
        // Loop through the ports
        VTSS_RC(icli_port_iter_init(&pit, sit.isid, PORT_ITER_FLAGS_FRONT));
        while (icli_port_iter_getnext(&pit, plist)) {
            if(has_inject){
                error_conf.prbsn_sel = prbsn_sel;
                rc = meba_prbs_monitor_set( board_instance, pit.iport, &error_conf);
                if(rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not inject %s  value for port %d\n", (error_conf.prbsn_sel == 0) ? "PRBS7" : ((error_conf.prbsn_sel == 1)? "PRBS15" : "PRBS31"), pit.iport);
                    break;
                }
                ICLI_PRINTF("PRBS : %s\n",(error_conf.prbsn_sel == 0) ? "PRBS7" : ((error_conf.prbsn_sel == 1)? "PRBS15" : "PRBS31"));
            }
            else if (has_value){
                error_conf.prbsn_sel = prbsn_sel;
                rc =  meba_prbs_monitor_get( board_instance, pit.iport, &error_conf);
                if(rc != MEPA_RC_OK) {
                    ICLI_PRINTF("Could not get %s value for port %d\n", (error_conf.prbsn_sel == 0) ? "PRBS7" : ((error_conf.prbsn_sel == 1)? "PRBS15" : "PRBS31"), pit.iport);
                    break;
                }
                ICLI_PRINTF("PRBS : %s\n",(error_conf.prbsn_sel == 0) ? "PRBS7" : ((error_conf.prbsn_sel == 1)? "PRBS15" : "PRBS31"));
                ICLI_PRINTF("Error count : %x\n",error_conf.no_of_errors);
            }
        }
    }
    return MEPA_RC_OK;
}
