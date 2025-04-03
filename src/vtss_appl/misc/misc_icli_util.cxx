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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#include <fcntl.h>
#include "main.h"
#include "misc_api.h"
#include "icli_api.h"
#include "icli_porting_util.h"
#include "misc_icli_util.h"
#include "port_api.h"
#include "port_iter.hxx"
#include <vtss/basics/map.hxx>
#if defined(VTSS_SW_OPTION_PHY)
#include "phy_api.h"
#endif
#if defined(VTSS_OPT_TS_SPI_FPGA) && defined(VTSS_SW_OPTION_SYNCE)
#include "../synce/pcb107_cpld.h"
#endif
#include "interrupt_api.h"
#define MAX_SPI_DATA  50
#define API_INST_DEFAULT PHY_INST
#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
#include "thread_load_monitor_api.hxx"
#endif
#ifdef VTSS_SW_OPTION_ACL
#include "acl_api.h"
#endif
#if defined(VTSS_SW_OPTION_TOD)
#include "tod_api.h"
#endif

#define MAX_CFG_PADDING_BYTES 3
#define MAX_VIPER_SPI_STREAM  7 + MAX_CFG_PADDING_BYTES

/**********************************************************************************/
// Lint Stuff
/**********************************************************************************/
/*lint -sem(misc_icli_spi_transaction, thread_protected) */

#define MAX_VENICE_SPI_STREAM 7
mesa_rc spi_read_write(const mesa_inst_t inst, const mesa_port_no_t port_no,
                       const u8 size, u8 *const bitstream);

#define VTSS_ALLOC_MODULE_ID VTSS_MODULE_ID_MISC
#define VTSS_TRACE_MODULE_ID VTSS_MODULE_ID_MISC

void misc_icli_chip(misc_icli_req_t *req)
{
    u32            session_id = req->session_id;
    mesa_chip_no_t chip_no;

    if (req->chip_no == MISC_CHIP_NO_NONE) {
        if (misc_chip_no_get(&chip_no) == VTSS_RC_OK) {
            ICLI_PRINTF("Chip Number: ");
            if (chip_no == VTSS_CHIP_NO_ALL) {
                ICLI_PRINTF("All\n");
            } else {
                ICLI_PRINTF("%u\n", chip_no);
            }
        } else {
            ICLI_PRINTF("GET failed");
        }
    } else if (misc_chip_no_set(req->chip_no) != VTSS_RC_OK) {
        ICLI_PRINTF("SET failed");
    }
}

#define MISC_ICLI_SESSION_NONE 0xffffffff

/* Global variable for session ID used by debug api print */
/*lint -esym(459, misc_icli_session_id) */
static u32 misc_icli_session_id = MISC_ICLI_SESSION_NONE;
static BOOL misc_icli_print_phy;

static int misc_icli_printf(const char *fmt, ...)
{
    u32               session_id = misc_icli_session_id;
    va_list           args;
    char              buf[256];
    mesa_debug_lock_t lock;
    mepa_lock_t       mepa_lock;

    /*lint --e{454,455,456} ... We are called within a critical region */
    va_start(args, fmt);
    (void)vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    if (misc_icli_print_phy) {
        // Unlock PHY API before printing
        mepa_lock.function = __FUNCTION__;
        mepa_lock.file = __FILE__;
        mepa_lock.line = (__LINE__ + 1);
        mepa_callout_unlock(&mepa_lock);
        ICLI_PRINTF("%s", buf);
        mepa_lock.line = (__LINE__ + 1);
        mepa_callout_lock(&mepa_lock);
    } else if (mesa_debug_unlock(NULL, &lock) == VTSS_RC_OK) {
        /* We need to unlock the API when printing to avoid critd timeout for the API mutex.
           This is needed, if ICLI paging is done. The following lock will restore the API context (chip_no) */
        ICLI_PRINTF("%s", buf);
        (void)mesa_debug_lock(NULL, &lock);
    }
    va_end(args);
    return 0; // Actual return value doesn't matter
}

void misc_icli_debug_api(misc_icli_req_t *req)
{
    u32               session_id = req->session_id;
    port_iter_t       pit;
    mesa_debug_info_t *info = &req->debug_info;
    mesa_port_no_t    iport;

    (void)icli_port_iter_init(&pit, VTSS_ISID_START, PORT_ITER_FLAGS_NORMAL);
    if (req->port_list == NULL) {
        /* Include all ports if no ports were specified (e.g. hidden loop ports) */
        for (iport = 0; iport < fast_cap(MEBA_CAP_BOARD_PORT_MAP_COUNT); iport++) {
            info->port_list[iport] = 1;
        }
    } else {
        while (icli_port_iter_getnext(&pit, req->port_list)) {
            info->port_list[pit.iport] = 1;
        }
    }

    if (misc_chip_no_get(&info->chip_no) == VTSS_RC_OK) {
        if (misc_icli_session_id == MISC_ICLI_SESSION_NONE) {
            misc_icli_session_id = session_id;
            misc_icli_print_phy = FALSE;
            if (mesa_debug_info_print(NULL, misc_icli_printf, info) != VTSS_RC_OK) {
                ICLI_PRINTF("mesa debug print failed\n");
            }
            misc_icli_print_phy = TRUE;
            if (meba_phy_debug_info_print(board_instance, misc_icli_printf, info) != MESA_RC_OK) {
                ICLI_PRINTF("meba debug print failed\n");
            }
            misc_icli_session_id = MISC_ICLI_SESSION_NONE;
        } else {
            ICLI_PRINTF("debug api is already running\n");
        }
    }
}

void misc_icli_suspend_resume(misc_icli_req_t *req)
{
    vtss_init_data_t data = {};

    data.cmd = INIT_CMD_SUSPEND_RESUME;
    data.resume = req->resume;
    (void)init_modules(&data);
}

void phy_1g_token_ring_read(u32 session_id, icli_stack_port_range_t *v_port_type_list, u32 value)
{
    u32                 range_idx, cnt_idx;
    mesa_port_no_t      uport;
    vtss_phy_type_t     id1g;
    u16                 value1, value0;
    mesa_rc rc = VTSS_RC_ERROR;

    if (v_port_type_list) { //at least one ranae input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                if (vtss_phy_id_get(PHY_INST, uport2iport(uport), &id1g) == VTSS_RC_OK) {
                    vtss_phy_write(PHY_INST, uport2iport(uport), 31, VTSS_PHY_PAGE_TR);
                    if (VTSS_RC_OK == (rc = vtss_phy_write(PHY_INST, uport2iport(uport), 16, 0xa000 | value))) {
                        vtss_phy_read(PHY_INST, uport2iport(uport), 18, &value1);
                        vtss_phy_read(PHY_INST, uport2iport(uport), 17, &value0);
                        ICLI_PRINTF("value0 :: 0x%04x.%04x\n", value1, value0);
                    }

                    vtss_phy_write(PHY_INST, uport2iport(uport), 31, VTSS_PHY_PAGE_STANDARD);
                } else {
                    ICLI_PRINTF("Error getting PHY ID port %d (%s)\n", uport, error_txt(rc));
                }
            }
        }
    }
}

#define MESA_RC(expr) { mesa_rc _rc = (expr); if (_rc != MESA_RC_OK) { return _rc; } }

static mesa_rc ptp_tc(mesa_port_list_t *port_list)
{
#ifdef VTSS_SW_OPTION_ACL
    acl_entry_conf_t ace;
    mesa_ace_ptp_t   *ptp = &ace.frame.etype.ptp;
    int              i;

    for (i = 1; i < 5; i++) {
        MESA_RC(acl_mgmt_ace_init(MESA_ACE_TYPE_ETYPE, &ace));
        ace.isid = VTSS_ISID_LOCAL;
        ace.id = i;
        ace.frame.etype.etype.value[0] = 0x88;
        ace.frame.etype.etype.value[1] = 0xf7;
        ace.frame.etype.etype.mask[0] = 0xff;
        ace.frame.etype.etype.mask[1] = 0xff;
        ptp->enable = 1;
        ptp->header.mask[1] = 0x0f;
        ptp->header.value[1] = 0x02; /* versionPTP = 2 */
        if (i == 1) {
            ptp->header.mask[0] = 0x0f;
            ptp->header.value[0] = 0x00; /* messageType = 0 */
            ace.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_2;
        } else if (i == 2) {
            ptp->header.mask[0] = 0x0f;
            ptp->header.value[0] = 0x03; /* messageType = 3 */
            ace.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP_SUB_DELAY_1;
        } else if (i == 3) {
            ptp->header.mask[0] = 0x0c;
            ptp->header.value[0] = 0x00; /* messageType = 1/2 */
            ace.action.ptp_action = MESA_ACL_PTP_ACTION_ONE_STEP_ADD_DELAY;
        } else {
            ptp->header.mask[0] = 0x08;
            ptp->header.value[0] = 0x08; /* messageType = 8-15 */
        }

        if (port_list != NULL) {
            ace.port_list = *port_list;
            ace.action.port_list = *port_list;
        }

        ace.action.port_action = MESA_ACL_PORT_ACTION_FILTER;
        MESA_RC(acl_mgmt_ace_add(ACL_USER_TEST, MESA_ACE_ID_LAST, &ace));
    }
#endif
    return MESA_RC_OK;
}

static mesa_rc tsn_demo(void)
{
    mesa_vid_t               vid_psfp = 20; // PSFP VLAN (VCE only)
    mesa_vid_t               vid_frer = 30; // FRER VLAN
    mesa_port_no_t           port_no, port_rx = 0, port_tx = 1;
    mesa_port_list_t         port_list, rx_list, tx_list;
    mesa_psfp_gce_t          psfp_gcl[2];
    int                      i, q, max_cnt = 2;
    mesa_psfp_gate_id_t      gate_id = 0;
    mesa_psfp_gate_conf_t    gate_conf;
    mesa_psfp_filter_id_t    filter_id = 0;
    mesa_psfp_filter_conf_t  filter_conf;
    mesa_iflow_id_t          iflow_id;
    mesa_iflow_conf_t        iflow_conf;
    mesa_vce_t               vce;
    mesa_tce_t               tce;
    mesa_frer_mstream_id_t   mstream_id;
    mesa_frer_cstream_id_t   cstream_id = 0;
    mesa_frer_stream_conf_t  frer_conf;
    mesa_qos_tas_port_conf_t tas_conf;
    mesa_qos_tas_gce_t       tas_gcl[2];
    uint64_t                 tc;

    // Include Rx and Tx ports in FRER VLAN
    port_list.clear_all();
    for (port_no = 0; port_no < 3; port_no++) {
        port_list[port_no] = 1;
        if (port_no == port_rx) {
            rx_list[port_no] = 1;
        } else {
            tx_list[port_no] = 1;
        }
    }

    /*** PTP Transparent Clock **************************************************************/
    MESA_RC(ptp_tc(&port_list));

    /*** PSFP *******************************************************************************/

    // Setup PSFP gate control list
    MESA_RC(mesa_psfp_gate_conf_get(NULL, gate_id, &gate_conf));
    for (i = 0; i < max_cnt; i++) {
        mesa_psfp_gce_t *gce = &psfp_gcl[i];
        memset(gce, 0, sizeof(*gce));
        /* Open for 1 msec, closed for 9 msec */
        gce->gate_open = (i == 0 ? 1 : 0);
        gce->time_interval = ((i == 0 ? 1 : 9) * 1000000);
        gate_conf.config.cycle_time += gce->time_interval;
    }

    MESA_RC(mesa_psfp_gcl_conf_set(NULL, gate_id, max_cnt, psfp_gcl));
    gate_conf.enable = 1;
    gate_conf.config_change = 1;
    MESA_RC(mesa_psfp_gate_conf_set(NULL, gate_id, &gate_conf));

    // Map filter to gate
    MESA_RC(mesa_psfp_filter_conf_get(NULL, filter_id, &filter_conf));
    filter_conf.gate_enable = 1;
    filter_conf.gate_id = gate_id;
    MESA_RC(mesa_psfp_filter_conf_set(NULL, filter_id, &filter_conf));

    // Allocate iflow for PSFP and map to filter
    MESA_RC(mesa_iflow_alloc(NULL, &iflow_id));
    MESA_RC(mesa_iflow_conf_get(NULL, iflow_id, &iflow_conf));
    iflow_conf.psfp.filter_enable = 1;
    iflow_conf.psfp.filter_id = filter_id;
    iflow_conf.cut_through_disable = 1;
    MESA_RC(mesa_iflow_conf_set(NULL, iflow_id, &iflow_conf));

    // On Rx port, map PSFP VLAN to iflow
    MESA_RC(mesa_vce_init(NULL, MESA_VCE_TYPE_ANY, &vce));
    vce.id = 1;
    vce.key.port_list = rx_list;
    vce.key.tag.tagged = MESA_VCAP_BIT_1;
    vce.key.tag.s_tag = MESA_VCAP_BIT_0;
    vce.key.tag.vid.value = vid_psfp;
    vce.key.tag.vid.mask = 0xfff;
    vce.action.flow_id = iflow_id;
    MESA_RC(mesa_vce_add(NULL, MESA_VCE_ID_LAST, &vce));

    /*** FRER (sequence generation) *********************************************************/

    // On Rx port, map FRER VLAN to iflow with sequence generation
    MESA_RC(mesa_iflow_alloc(NULL, &iflow_id));
    MESA_RC(mesa_iflow_conf_get(NULL, iflow_id, &iflow_conf));
    iflow_conf.frer.generation = 1;
    iflow_conf.cut_through_disable = 1;
    MESA_RC(mesa_iflow_conf_set(NULL, iflow_id, &iflow_conf));
    vce.id = 2;
    vce.key.tag.vid.value = vid_frer;
    vce.action.vid = vid_frer;
    vce.action.pop_enable = 1;
    vce.action.pop_cnt = 1;
    vce.action.flow_id = iflow_id;
    MESA_RC(mesa_vce_add(NULL, MESA_VCE_ID_LAST, &vce));

    // On Tx ports, R-tag push enabled for FRER VLAN
    MESA_RC(mesa_tce_init(NULL, &tce));
    tce.id = 1;
    tce.key.port_list = tx_list;
    tce.key.vid = vid_frer;
    tce.action.tag.tpid = MESA_TPID_SEL_C;
    tce.action.tag.vid = vid_frer;
    tce.action.rtag.sel = MESA_RTAG_SEL_INNER;
    MESA_RC(mesa_tce_add(NULL, MESA_TCE_ID_LAST, &tce));

    /*** FRER (sequence recovery) ***********************************************************/

    // Enable sequence recovery for compound stream
    MESA_RC(mesa_frer_cstream_conf_get(NULL, cstream_id, &frer_conf));
    frer_conf.recovery = 1;
    frer_conf.alg = MESA_FRER_RECOVERY_ALG_VECTOR;
    frer_conf.hlen = 8;
    frer_conf.reset_time = 1000;
    MESA_RC(mesa_frer_cstream_conf_set(NULL, cstream_id, &frer_conf));

    // Allocate member stream on Rx port and map to compound stream
    MESA_RC(mesa_frer_mstream_alloc(NULL, &rx_list, &mstream_id));
    MESA_RC(mesa_frer_mstream_conf_get(NULL, mstream_id, port_rx, &frer_conf));
    frer_conf.cstream_id = cstream_id;
    MESA_RC(mesa_frer_mstream_conf_set(NULL, mstream_id, port_rx, &frer_conf));

    // On Tx ports, map FRER VLAN to iflow mapping to member stream
    MESA_RC(mesa_iflow_alloc(NULL, &iflow_id));
    MESA_RC(mesa_iflow_conf_get(NULL, iflow_id, &iflow_conf));
    iflow_conf.frer.mstream_enable = 1;
    iflow_conf.frer.mstream_id = mstream_id;
    iflow_conf.cut_through_disable = 1;
    MESA_RC(mesa_iflow_conf_set(NULL, iflow_id, &iflow_conf));
    vce.id = 3;
    vce.key.port_list = tx_list;
    vce.action.flow_id = iflow_id;
    MESA_RC(mesa_vce_add(NULL, MESA_VCE_ID_LAST, &vce));

    // On Rx port, R-tag pop enabled for FRER VLAN
    tce.id = 2;
    tce.key.port_list = rx_list;
    tce.action.rtag.sel = MESA_RTAG_SEL_NONE;
    tce.action.rtag.pop = 1;
    MESA_RC(mesa_tce_add(NULL, MESA_TCE_ID_LAST, &tce));

    /*** TAS ********************************************************************************/

    // Setup TAS gate control list for priority 5 on egress port
    MESA_RC(mesa_qos_tas_port_conf_get(NULL, port_tx, &tas_conf));
    for (i = 0; i < max_cnt; i++) {
        mesa_qos_tas_gce_t *gce = &tas_gcl[i];
        memset(gce, 0, sizeof(*gce));
        for (q = 0; q < MESA_QUEUE_ARRAY_SIZE; q++) {
            /* Open for 1 msec, closed for 9 msec */
            gce->gate_open[q] = (i == 1 && q == 5 ? 0 : 1);
        }

        gce->time_interval = ((i == 0 ? 1 : 9) * 1000000);
        tas_conf.cycle_time += gce->time_interval;
    }

    MESA_RC(mesa_qos_tas_port_gcl_conf_set(NULL, port_tx, max_cnt, tas_gcl));
    tas_conf.gate_enabled = 1;
    for (q = 0; q < MESA_QUEUE_ARRAY_SIZE; q++) {
        tas_conf.gate_open[q] = 1;
    }

    tas_conf.config_change = 1;
    MESA_RC(mesa_ts_timeofday_get(NULL, &tas_conf.base_time, &tc));
    tas_conf.base_time.seconds++;
    MESA_RC(mesa_qos_tas_port_conf_set(NULL, port_tx, &tas_conf));

    return MESA_RC_OK;
}

icli_rc_t misc_icli_test(misc_icli_req_t *req)
{
    u32 session_id = req->session_id;

    switch (req->value) {
    case 1:
        ICLI_PRINTF("Test case 1: Set forward mode for the first port to Rx forwarding only\n");
        if (mesa_port_forward_state_set(NULL, 0, MESA_PORT_FORWARD_INGRESS) != VTSS_RC_OK) {
            return ICLI_RC_ERROR;
        }

        break;
    case 2:
        ICLI_PRINTF("TSN demo command\n");
        if (tsn_demo() != VTSS_RC_OK) {
            return ICLI_RC_ERROR;
        }

        break;
    case 3:
        ICLI_PRINTF("PTP TC setup\n");
        if (ptp_tc(NULL) != VTSS_RC_OK) {
            return ICLI_RC_ERROR;
        }

        break;
    default:
        ICLI_PRINTF("Test case not implemented\n");
        break;
    }

    return ICLI_RC_OK;
}
#if 0
mesa_rc phy_fifo_sync(u32 session_id, icli_stack_port_range_t *v_port_type_list)
{
    u32                 range_idx, cnt_idx;
    mesa_port_no_t      uport;
    vtss_phy_oos_mitigation_mode_t operation_mode;

    if (v_port_type_list) { //at least one ranae input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                operation_mode = VTSS_PHY_OOS_MITIGATION_MODE_NORMAL_OPERATION;
                if (vtss_phy_ts_viper_fifo_reset(PHY_INST, uport2iport(uport), operation_mode) == VTSS_RC_OK) {
                    ICLI_PRINTF("VIPER B FIFO RESET ALGORITHM SUCCESSFUL\n");
                } else {
                    ICLI_PRINTF("VIPER B FIFO RESET ALGO FAILED\n");
                }
            }
        }
    }

    return ICLI_RC_OK;
}
#endif
#if defined(VTSS_OPT_TS_SPI_FPGA)
mesa_rc misc_icli_spi_daisy_chaining_timestamp(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_output,
                                               BOOL has_output_enable, BOOL has_output_disable, BOOL has_input,
                                               BOOL has_input_enable, BOOL has_input_disable)
{
    u32                 range_idx, cnt_idx;
    mesa_port_no_t      uport;
    mesa_rc             rc = VTSS_RC_ERROR;
    vtss_phy_type_t     id1g;
    vtss_phy_daisy_chain_conf_t   daisy_chain;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                if (vtss_phy_id_get(PHY_INST, uport2iport(uport), &id1g) == VTSS_RC_OK) {
                    if (VTSS_RC_OK != (rc = vtss_phy_daisy_conf_get(PHY_INST, uport2iport(uport), &daisy_chain))) {
                        ICLI_PRINTF("daisy-chaining get failed Port:: %d\n", uport);
                    }

                    if (has_output || has_input) {
                        daisy_chain.spi_daisy_input = (has_input_enable == TRUE) ? TRUE : daisy_chain.spi_daisy_input;
                        daisy_chain.spi_daisy_output = (has_output_enable == TRUE) ? TRUE : daisy_chain.spi_daisy_output;
                        if (VTSS_RC_OK == (rc = vtss_phy_daisy_conf_set(PHY_INST, uport2iport(uport), &daisy_chain))) {
                            ICLI_PRINTF("daisy-chaining done Port:: %d\n", uport);
                        }
                    } else {
                        ICLI_PRINTF("SPI-daisy-input %s\t SPI-daisy-output %s\n  port:: %d\n",
                                    (daisy_chain.spi_daisy_input == TRUE) ? "enabled" : "disabled",
                                    (daisy_chain.spi_daisy_output == TRUE) ? "enabled" : "disabled", uport);
                    }
                } else {
                    ICLI_PRINTF("Error getting PHY ID port %d (%s)\n", uport, error_txt(rc));
                }
            }
        }
    }

    return rc;
}
#if defined(VTSS_OPT_TS_SPI_FPGA) && defined(VTSS_SW_OPTION_SYNCE)
void misc_icli_cmd_debug_phy_1588_timestamp_show(u32 session_id, icli_stack_port_range_t *plist, u16 no_of_ts)
{
    port_iter_t pit;
    mesa_port_no_t port_no;
    unsigned char ts_cnt = 0, *for_ip;
    vtss_phy_timestamp_t ts;
    vtss_phy_ts_fifo_sig_t signature;
    int status, i;
    vtss_phy_ts_fifo_sig_mask_t sig_mask;
    u16 port = 0;

    (void)icli_port_iter_init(&pit, VTSS_ISID_START, PORT_ITER_FLAGS_NORMAL);
    while (icli_port_iter_getnext(&pit, plist)) {
        port_no = iport2uport(pit.iport);
        while (ts_cnt++ < no_of_ts) {
            /* Read the time stamp */
            vtss_ts_spi_fpga_read(&ts, &signature, &status, &port, pit.iport);
            if (status != -1) {
                sig_mask = signature.sig_mask;
                ICLI_PRINTF("Time Stamp :: %d  Port Number:: %d\n", ts_cnt, port);
                ICLI_PRINTF("    Seconds High :: %x\n", ts.seconds.high);
                ICLI_PRINTF("    Seconds  Low :: %x \n", (unsigned int)ts.seconds.low);
                ICLI_PRINTF("    Nano Seconds :: %x \n", (unsigned int)ts.nanoseconds);
                ICLI_PRINTF("Signature  :: ");
                if (sig_mask & VTSS_PHY_TS_FIFO_SIG_MSG_TYPE) {
                    ICLI_PRINTF("    Message Type  :: %.2x \n", signature.msg_type);
                }

                ICLI_PRINTF("Port: %d Host Mac\n", port_no);
                if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DOMAIN_NUM) {
                    ICLI_PRINTF("    Domain Number :: %d \n", signature.domain_num);
                }

                if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SOURCE_PORT_ID) {
                    ICLI_PRINTF("    Source portId :: %.2x::%.2x::%.2x::%.2x::%.2x::%.2x::%.2x::%.2x::%.2x::%.2x\n", signature.src_port_identity[0], signature.src_port_identity[1],
                                signature.src_port_identity[2], signature.src_port_identity[3], signature.src_port_identity[4],
                                signature.src_port_identity[5], signature.src_port_identity[6], signature.src_port_identity[7],
                                signature.src_port_identity[8], signature.src_port_identity[9]);
                }

                if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SEQ_ID) {
                    ICLI_PRINTF("    SequenceId    :: %x\n", signature.sequence_id);
                }

                if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SRC_IP) {
                    ICLI_PRINTF("    Source IP     :: ");
                    for_ip = (u8 *)(&(signature.src_ip));
                    for (i = 0; i < 4; i++) {
                        ICLI_PRINTF("%u ", for_ip[i]);
                    }

                    ICLI_PRINTF("\n");
                }

                if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DEST_IP) {
                    ICLI_PRINTF("    Destination IP:: ");
                    for_ip = (u8 *)(&(signature.dest_ip));
                    for (i = 0; i < 4; i++) {
                        ICLI_PRINTF("%u ", for_ip[i]);
                    }

                    ICLI_PRINTF("\n");

                }

                if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DEST_MAC) {
                    ICLI_PRINTF("    DMAC :: %.2x::%.2x::%.2x::%.2x::%.2x::%.2x\n", signature.dest_mac[5], signature.dest_mac[4], signature.dest_mac[3], signature.dest_mac[2], signature.dest_mac[1], signature.dest_mac[0]);
                }

                ICLI_PRINTF("\n\n");
                VTSS_OS_MSLEEP(10);
            } else {
                ICLI_PRINTF("\nFIFO is EMPTY to read timestamps\n");
                break;
            }
        }
    }
}

void misc_icli_cmd_debug_phy_1588_fifo_clear(u32 session_id)
{
    (void)pcb107_fifo_clear();
    ICLI_PRINTF("Cleared the FIFO\n");
}
#endif //(VTSS_SW_OPTION_SYNCE)

void misc_icli_cmd_debug_phy_1588_fifo_status(u32 session_id)
{
//#if 0
//      /* TODO */
//              vtss_ts_spi_status status = 0;
//
//          if (vtss_ts_spi_get_status(&status) != VTSS_RC_OK) {
//                      ICLI_PRINTF("Failed to read the status\n");
//                          }
//              ICLI_PRINTF("Empty  Dropped  Full\n-----  -------  ----\n");
//                  ICLI_PRINTF("%-5s  %-7s  %s\n", ((status & VTSS_TS_SPI_FIFO_EMPTY) ? "Yes" : "No"), ((status & VTSS_TS_SPI_FIFO_DROP) ? "Yes" : "No"), ((status & VTSS_TS_SPI_FIFO_FULL) ? "Yes" : "No"));
//#endif
}

#endif /* VTSS_OPT_TS_SPI_FPGA */

mesa_rc misc_icli_debug_macsec_sd6g_csr_read_write(u32 session_id, icli_stack_port_range_t *v_port_type_list, u16 target, u32 addr, u32 value, BOOL is_read)
{

    u32             range_idx, cnt_idx, rd_value;
    mesa_port_no_t  uport;
    mesa_rc rc = VTSS_RC_ERROR;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                if (is_read) {
                    if ((rc = vtss_phy_macsec_csr_sd6g_rd(misc_phy_inst_get(), uport2iport(uport), target, addr, &rd_value)) != VTSS_RC_OK) {
                        ICLI_PRINTF("Read failed on port %u \n", uport2iport(uport));
                    } else {
                        ICLI_PRINTF("port %u 0x%0x.%08x = 0x%08x \n", uport, target, addr, rd_value);
                    }
                } else {
                    if ((rc = vtss_phy_macsec_csr_sd6g_wr(misc_phy_inst_get(), uport2iport(uport), target, addr, value)) != VTSS_RC_OK) {
                        ICLI_PRINTF("Write failed on port %u \n", uport2iport(uport));
                    }

                }
            }
        }
    }

    return rc;
}

static const char *slewrate2txt(u32 value)
{
    switch (value) {
    case VTSS_SLEWRATE_25PS:
        return "25PS";
    case VTSS_SLEWRATE_35PS:
        return "35PS";
    case VTSS_SLEWRATE_55PS:
        return "55PS";
    case VTSS_SLEWRATE_70PS:
        return "70PS";
    case VTSS_SLEWRATE_120PS:
        return "120PS";
    }

    return "INVALID";
}

void misc_icli_10g_kr_conf(u32 session_id, BOOL has_cm1, i32 cm_1, BOOL has_c0, i32 c_0, BOOL has_cp1, i32 c_1,
                           BOOL has_ampl, u32 amp_val, BOOL has_ps25, BOOL has_ps35, BOOL has_ps55, BOOL has_ps70,
                           BOOL has_ps120, BOOL has_en_ob, BOOL has_dis_ob, BOOL has_ser_inv, BOOL has_ser_no_inv,
                           icli_stack_port_range_t *v_port_type_list, BOOL has_host)
{
    vtss_phy_10g_base_kr_conf_t kr_conf;
    vtss_phy_10g_ob_status_t ob_status;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    vtss_phy_10g_id_t phy_id;
    mesa_rc rc;
    float r, v1, v2, v3;
    u32 max_dac;
    BOOL is_set = FALSE;

    if (has_cm1 || has_c0 || has_cp1 || has_ampl || has_ps25 || has_ps35 || has_ps55 || has_ps70 || has_ps120 || has_en_ob || has_dis_ob || has_ser_inv || has_ser_no_inv ) {
        is_set = TRUE;
    } else {
        is_set = FALSE;
    }

    memset(&kr_conf, 0, sizeof(vtss_phy_10g_base_kr_conf_t));
    memset(&ob_status, 0, sizeof(vtss_phy_10g_ob_status_t));

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                if (VTSS_RC_OK != (rc = vtss_phy_10g_id_get(misc_phy_inst_get(), uport2iport(uport), &phy_id))) {
                    ICLI_PRINTF("Error in getting the chip_id of port %d (%s),Fun %s,line %u\n", uport, error_txt(rc), __FUNCTION__, __LINE__);
                    continue;
                }

                if (phy_id.family == VTSS_PHY_FAMILY_MALIBU) {
                    if (is_set == TRUE) {
                        if (has_host == TRUE) {
                            if (VTSS_RC_OK != (rc = vtss_phy_10g_base_kr_host_conf_get(misc_phy_inst_get(), uport2iport(uport), &kr_conf))) {
                                ICLI_PRINTF("Error getting KR configuration for port %d (%s),Fun %s,line %u\n", uport, error_txt(rc), __FUNCTION__, __LINE__);
                            }
                        } else {
                            if (VTSS_RC_OK != (rc = vtss_phy_10g_base_kr_conf_get(misc_phy_inst_get(), uport2iport(uport), &kr_conf))) {
                                ICLI_PRINTF("Error getting KR configuration for port %d (%s),Fun %s,line %u\n", uport, error_txt(rc), __FUNCTION__, __LINE__);
                            }
                        }
                    } else {
                        ob_status.is_host = has_host;
                        vtss_phy_10g_ob_status_get(misc_phy_inst_get(), uport2iport(uport), &ob_status);
                        ICLI_PRINTF("%-32s :: %u\n", "R-control", ob_status.r_ctrl);
                        ICLI_PRINTF("%-32s :: %u\n", "C-control", ob_status.c_ctrl);
                        ICLI_PRINTF("%-32s :: %u\n", "slew rate", ob_status.slew);
                        ICLI_PRINTF("%-32s :: %u\n", "Levn", ob_status.levn);
                        ICLI_PRINTF("%-32s :: 0x%08x\n", "d-filter value", ob_status.d_fltr);
                        ICLI_PRINTF("%-32s :: %d\n", "kr-tap v3", ob_status.v3);
                        ICLI_PRINTF("%-32s :: %d\n", "kr-tap vp", ob_status.vp);
                        ICLI_PRINTF("%-32s :: %d\n", "kr-tap v4", ob_status.v4);
                        ICLI_PRINTF("%-32s :: %d\n", "kr-tap v5", ob_status.v5);
                    }

                } else {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_base_kr_conf_get(misc_phy_inst_get(), uport2iport(uport), &kr_conf))) {
                        ICLI_PRINTF("Error getting KR configuration for port %d (%s)\n", uport, error_txt(rc));

                    }
                }

                if (has_cm1) {
                    kr_conf.cm1 = cm_1;
                }

                if (has_c0) {
                    kr_conf.c0 = c_0;
                }

                if (has_cp1) {
                    kr_conf.c1 = c_1;
                }

                if (has_ampl) {
                    kr_conf.ampl = amp_val;
                }

                kr_conf.slewrate = has_ps25 ? VTSS_SLEWRATE_25PS : has_ps35 ? VTSS_SLEWRATE_35PS : has_ps55 ? VTSS_SLEWRATE_55PS :
                                   has_ps70 ? VTSS_SLEWRATE_70PS : has_ps120 ? VTSS_SLEWRATE_120PS : kr_conf.slewrate;
                kr_conf.en_ob = has_en_ob ? TRUE : has_dis_ob ? FALSE : kr_conf.en_ob;
                kr_conf.ser_inv = has_ser_inv ? TRUE : has_ser_no_inv ? FALSE : kr_conf.ser_inv;
                r = kr_conf.ampl / 62.0;
                v1 = kr_conf.c0 - kr_conf.c1 + kr_conf.cm1;
                v2 = kr_conf.c0 + kr_conf.c1 + kr_conf.cm1;
                v3 = kr_conf.c0 + kr_conf.c1 - kr_conf.cm1;
                max_dac = labs(kr_conf.c0) + labs(kr_conf.c1) + labs(kr_conf.cm1);

                if (phy_id.family == VTSS_PHY_FAMILY_MALIBU) {
                    if (is_set == TRUE) {
                        if (has_host == TRUE) {
                            if (VTSS_RC_OK != (rc = vtss_phy_10g_base_kr_host_conf_set(misc_phy_inst_get(), uport2iport(uport), &kr_conf))) {
                                ICLI_PRINTF("Error setting KR configuration for port %d (%s),Fun %s,line %u\n", uport, error_txt(rc), __FUNCTION__, __LINE__);
                            }
                        } else {
                            if (VTSS_RC_OK != (rc = vtss_phy_10g_base_kr_conf_set(misc_phy_inst_get(), uport2iport(uport), &kr_conf))) {
                                ICLI_PRINTF("Error setting KR configuration for port %d (%s),Fun %s,line %u\n", uport, error_txt(rc), __FUNCTION__, __LINE__);
                            }
                        }
                    }
                } else {
                    ICLI_PRINTF("KR configuration for port %d\n", uport);
                    ICLI_PRINTF("----------------------------\n");
                    ICLI_PRINTF("cm1     = %3d\n", kr_conf.cm1);
                    ICLI_PRINTF("c0      = %3d\n", kr_conf.c0);
                    ICLI_PRINTF("cp1     = %3d\n", kr_conf.c1);
                    ICLI_PRINTF("Slewrate= %s\n", slewrate2txt(kr_conf.slewrate));
                    ICLI_PRINTF("Rpre    = %1.3f\n", v3 / v2);
                    ICLI_PRINTF("Rpst    = %1.3f\n", v1 / v2);
                    ICLI_PRINTF("Ampl    = %4d   mVppd\n", kr_conf.ampl);
                    ICLI_PRINTF("max     = %5.1f mVppd\n", 2.0 * r * max_dac);
                    ICLI_PRINTF("v2      = %5.1f mV\n", r * v2);
                    ICLI_PRINTF("maxdac  = %3u\n", max_dac);
                    ICLI_PRINTF("enable  = %s\n", kr_conf.en_ob ? "TRUE" : "FALSE");
                    ICLI_PRINTF("invert  = %s\n", kr_conf.ser_inv ? "TRUE" : "FALSE");

                    if (VTSS_RC_OK != (rc = vtss_phy_10g_base_kr_conf_set(misc_phy_inst_get(), uport2iport(uport), &kr_conf))) {
                        ICLI_PRINTF("Error setting KR configuration for port %d (%s),Fun %s,line %u\n", uport, error_txt(rc), __FUNCTION__, __LINE__);
                    }
                }

            }
        }
    }
}

void print_kr_status(u32 session_id, mesa_port_no_t port_no, vtss_phy_10g_base_kr_status_t *kr_status)
{
    ICLI_PRINTF(" aneg complete - %s \n", kr_status->aneg.complete ? "TRUE" : "FALSE");
    ICLI_PRINTF(" aneg active - %s \n", kr_status->aneg.active ? "TRUE" : "FALSE");
    ICLI_PRINTF(" aneg lp request_10g - %s \n", kr_status->aneg.request_10g ? "TRUE" : "FALSE");
    ICLI_PRINTF(" aneg lp request_1g - %s \n", kr_status->aneg.request_1g ? "TRUE" : "FALSE");
    ICLI_PRINTF(" aneg request_fec_change - %s \n", kr_status->aneg.request_fec_change ? "TRUE" : "FALSE");
    ICLI_PRINTF(" aneg fec_enable - %s \n", kr_status->aneg.fec_enable ? "TRUE" : "FALSE");
    ICLI_PRINTF(" aneg sm - %u \n", kr_status->aneg.sm );
    ICLI_PRINTF(" aneg lp_aneg_able - %s \n", kr_status->aneg.lp_aneg_able ? "TRUE" : "FALSE");
    ICLI_PRINTF(" pcs block_lock - %s \n", kr_status->aneg.block_lock ? "TRUE" : "FALSE");
    ICLI_PRINTF(" training complete - %s \n", kr_status->train.complete ? "TRUE" : "FALSE");
    ICLI_PRINTF(" training cm_ob_tap_result - %u \n", kr_status->train.cm_ob_tap_result );
    ICLI_PRINTF(" training cp_ob_tap_result - %u \n", kr_status->train.cp_ob_tap_result );
    ICLI_PRINTF(" training c0_ob_tap_result - %u \n", kr_status->train.c0_ob_tap_result );

    ICLI_PRINTF(" fec enable - %s \n", kr_status->fec.enable ? "TRUE" : "FALSE");
    ICLI_PRINTF(" fec corrected_block_cnt - %u \n", kr_status->fec.corrected_block_cnt);
    ICLI_PRINTF(" fec uncorrected_block_cnt - %u \n", kr_status->fec.uncorrected_block_cnt);
}

void misc_icli_10g_phy_kr_train_autoneg(u32 session_id, BOOL has_enable, BOOL has_disable,                                                                        BOOL has_line, BOOL has_host, vtss_phy_10g_base_kr_ld_adv_abil_t *adv_abil, vtss_phy_10g_base_kr_training_t *training,
                                        icli_stack_port_range_t *v_port_type_list)

{
    vtss_phy_10g_base_kr_train_aneg_t tr_autoneg, tr_autoneg_old;
    vtss_phy_10g_base_kr_status_t kr_status;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    mesa_rc rc;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                if (vtss_phy_10g_base_kr_train_aneg_get(misc_phy_inst_get(), uport2iport(uport), &tr_autoneg_old) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error getting kr_train_autoneg configuration for port %s %u/%u\n",
                                icli_port_type_get_name(v_port_type_list->switch_range[range_idx].port_type),
                                v_port_type_list->switch_range[range_idx].switch_id, uport);
                }

                tr_autoneg.autoneg.an_enable = has_enable ? TRUE : has_disable ? FALSE : tr_autoneg_old.autoneg.an_enable;
                //tr_autoneg.autoneg.an_restart = TRUE;
                tr_autoneg.autoneg.an_restart = FALSE;
                tr_autoneg.training.enable = has_enable ? TRUE : has_disable ? FALSE : tr_autoneg_old.training.enable;
                tr_autoneg.ld_abil.fec_req = FALSE;
                tr_autoneg.host_kr = has_host ? TRUE : FALSE;
                tr_autoneg.line_kr = has_line ? TRUE : FALSE;
                tr_autoneg.ld_abil = *adv_abil;
                tr_autoneg.training = *training;

                ICLI_PRINTF("Port %d, KR Training Autoneg:%s\n", uport, tr_autoneg.autoneg.an_enable ? "enable" : "disable");
                if (has_enable || has_disable) {
                    if (has_host) {
                        if (VTSS_RC_OK != (rc = vtss_phy_10g_base_host_kr_train_aneg_set(misc_phy_inst_get(), uport2iport(uport), &tr_autoneg))) {
                            ICLI_PRINTF("Error setting KR train Autoneg configuration for port %d (%s)\n", uport, error_txt(rc));
                        }

                    } else {
                        if (VTSS_RC_OK != (rc = vtss_phy_10g_base_kr_train_aneg_set(misc_phy_inst_get(), uport2iport(uport), &tr_autoneg))) {
                            ICLI_PRINTF("Error setting KR train Autoneg configuration for port %d (%s)\n", uport, error_txt(rc));
                        }
                    }

                } else {
                    if (has_host) {
                        ICLI_PRINTF("KR training and aneg status on port %u HOST\n", uport);
                        vtss_phy_10g_kr_status_get(misc_phy_inst_get(), uport2iport(uport), !has_host, &kr_status);
                        print_kr_status(session_id, uport2iport(uport), &kr_status);
                    }

                    if (has_line) {
                        ICLI_PRINTF("KR training and aneg status on port %u LINE\n", uport);
                        vtss_phy_10g_kr_status_get(misc_phy_inst_get(), uport2iport(uport), has_line, &kr_status);
                        print_kr_status(session_id, uport2iport(uport), &kr_status);
                    }
                }

            }
        }
    }
}

mesa_rc vtss_phy_10g_vscope_scan_icli(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_fast_scan, BOOL has_fast_scan_plus, BOOL has_host, BOOL has_line, u32 v_0_to_10000, BOOL has_enable, BOOL has_disable, BOOL has_xy_scan, BOOL has_host_1, BOOL has_line_1, BOOL has_enable_1, u32 v_0_to_127, u32 v_0_to_63, u32 v_0_to_127_1, u32 v_0_to_63_1, u32 v_0_to_10, u32 v_0_to_10_1, u32 v_0_to_64, u32 v_0_to_10000_1, BOOL has_disable_1)
{
    u32                         range_idx = 0, cnt_idx = 0;
    i32                         i, j;
    mesa_port_no_t              uport;
    vtss_phy_10g_vscope_conf_t conf;
    vtss_phy_10g_vscope_scan_conf_t conf_scan;
    vtss_phy_10g_vscope_scan_status_t  conf1;
    mesa_rc                     rc = VTSS_RC_ERROR;
    vtss_phy_10g_id_t           chip_id;
    memset(&conf_scan, 0, sizeof(conf_scan));
    memset(&conf, 0, sizeof(conf));
    memset(&conf1, 0, sizeof(conf1));
    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (VTSS_RC_OK != (rc = vtss_phy_10g_id_get(misc_phy_inst_get(), uport2iport(uport), &chip_id))) {
                    ICLI_PRINTF("Error in getting the chip_id of port %d \n", uport);
                    continue;
                }

                if (has_fast_scan) {
                    conf.scan_type = VTSS_PHY_10G_FAST_SCAN;
                    if (has_line) {
                        conf.line = TRUE;
                    } else {
                        if (chip_id.family == VTSS_PHY_FAMILY_VENICE) {
                            ICLI_PRINTF("NO SUPPORT for venice HOST side\n");
                            return VTSS_RC_ERROR;
                        }

                        conf.line = FALSE;
                    }

                    if (has_enable) {
                        conf.enable = TRUE;
                        conf.error_thres = v_0_to_10000;
                    } else {
                        conf.enable = FALSE;
                    }
                }

                if (has_xy_scan) {
                    conf.scan_type = VTSS_PHY_10G_FULL_SCAN;
                    if (has_line_1) {
                        conf.line = TRUE;
                    } else {
                        conf.line = FALSE;
                    }

                    if (has_enable_1) {
                        conf.error_thres = v_0_to_10000_1;
                        conf.enable = TRUE;
                        if (has_line_1) {
                            conf_scan.line = TRUE;
                        } else {
                            conf_scan.line = FALSE;
                        }

                        conf_scan.x_start = v_0_to_127;
                        conf_scan.y_start = v_0_to_63;
                        conf_scan.x_count = v_0_to_127_1;
                        conf_scan.y_count = v_0_to_63_1;
                        conf_scan.x_incr = v_0_to_10;
                        conf_scan.y_incr = v_0_to_10_1;
                        conf_scan.ber = v_0_to_64;
                        conf1.scan_conf = conf_scan;
                    } else {
                        conf.enable = FALSE;
                    }
                }

                if (has_enable_1) {
                    if (conf_scan.x_count + conf_scan.x_start > 127) {
                        ICLI_PRINTF("The points exceed bounds, make sure that x-count + x-start <= 127\n");
                        return VTSS_RC_ERROR;
                    }

                    if (conf_scan.y_count + conf_scan.y_start > 63) {
                        ICLI_PRINTF("The points exceed bounds, make sure that y-count + y-start <= 63\n");
                        return VTSS_RC_ERROR;
                    }
                }

                if ((rc = vtss_phy_10g_vscope_conf_set(misc_phy_inst_get(), uport2iport(uport), &conf)) != VTSS_RC_OK) {
                    ICLI_PRINTF("ERROR: VSCOPE configuration failed\n");
                    return VTSS_RC_ERROR;
                } else {
                    ICLI_PRINTF("VSCOPE successfully configured\n");
                }

                if (has_enable | has_enable_1) {
                    if ((rc = vtss_phy_10g_vscope_scan_status_get(misc_phy_inst_get(), uport2iport(uport), &conf1)) != VTSS_RC_OK) {
                        ICLI_PRINTF("ERROR: Fetching of VSCOPE data failed\n");
                        return VTSS_RC_ERROR;
                    } else {
                        if (has_fast_scan) {
                            ICLI_PRINTF("error free x points: %d\n", conf1.error_free_x);
                            ICLI_PRINTF("error free y points: %d\n", conf1.error_free_y);
                            ICLI_PRINTF("amplitude range: %d\n", conf1.amp_range);
                        } else if (has_xy_scan) {
                            ICLI_PRINTF("error counter values for the full scan are\n");
                            for (j = conf_scan.y_start; j < (conf_scan.y_start + conf_scan.y_count); j = j + conf_scan.y_incr + 1) {
                                if (conf1.errors[0][j] == 0) {
                                    j++;
                                }

                                ICLI_PRINTF("\n");
                                for (i = conf_scan.x_start; i < (conf_scan.x_start + conf_scan.x_count) ; i = i + conf_scan.x_incr + 1) {
                                    if (conf1.errors[i][j] == 0) {
                                        ICLI_PRINTF("X");
                                    } else {
                                        ICLI_PRINTF(".");
                                    }
                                }
                            }
                        } else {
                            ICLI_PRINTF("Scan Type not supported\n");
                        }
                    }
                }
            }
        }
    }

    return rc;
}

mesa_rc misc_phy_10g_prbs_generator_conf ( u32 session_id, icli_stack_port_range_t *v_port_type_list, mesa_bool_t set, vtss_phy_10g_prbs_type_t type,  vtss_phy_10g_direction_t direction,  vtss_phy_10g_prbs_generator_conf_t  *const conf)
{
    u32                         range_idx = 0, cnt_idx = 0;
    mesa_port_no_t              uport;
    vtss_phy_10g_prbs_generator_conf_t  prbs_conf;
    mesa_rc       rc = VTSS_RC_ERROR;

    memset(&prbs_conf, 0, sizeof(prbs_conf));
    ICLI_PRINTF("direction %s : type %s , Generator configuration\n", ((direction == VTSS_PHY_10G_DIRECTION_LINE) ? "LINE" : (direction == VTSS_PHY_10G_DIRECTION_HOST ? "HOST" : "not selected")), (type == VTSS_PHY_10G_PRBS_TYPE_SERDES ? "serdes" : ((type == VTSS_PHY_10G_PRBS_TYPE_PCS) ? "pcs" : "not selected")));

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (set) {
                    /* Enable or disable pcs prbs generator */
                    if ((rc = vtss_phy_10g_prbs_generator_conf_set(misc_phy_inst_get(), uport2iport(uport), type, direction, conf)) != VTSS_RC_OK) {
                        ICLI_PRINTF("ERROR: prbs generator config failed\n");
                    } else {
                        ICLI_PRINTF("prbs generator successfully configured!\n");
                    }
                }

                if (vtss_phy_10g_prbs_generator_conf_get(misc_phy_inst_get(), uport2iport(uport), type, direction, &prbs_conf) == VTSS_RC_OK) {
                    if (type == VTSS_PHY_10G_PRBS_TYPE_SERDES) {
                        ICLI_PRINTF("SERDES PRBS %s port %u \n ", prbs_conf.enable ? "ENABLED" : "DISABLED", uport);
                    }

                    if (type == VTSS_PHY_10G_PRBS_TYPE_PCS) {
                        ICLI_PRINTF("PCS PRBS %s \n ", prbs_conf.prbs_gen_pcs ? "ENABLED" : "DISABLED");
                    }
                } else {
                    ICLI_PRINTF("vtss_phy_10g_prbs_generator_conf_get() failed on port %u\n", uport);
                }
            }
        }
    }

    return rc;
}

mesa_rc misc_phy_10g_prbs_monitor_conf ( u32 session_id, icli_stack_port_range_t *v_port_type_list, mesa_bool_t set, vtss_phy_10g_prbs_type_t type, vtss_phy_10g_direction_t direction, vtss_phy_10g_prbs_monitor_conf_t *const conf_prbs)
{
    u32                         range_idx = 0, cnt_idx = 0;
    mesa_port_no_t              uport;
    vtss_phy_10g_prbs_monitor_conf_t mon_conf;
    mesa_rc       rc = VTSS_RC_ERROR;

    memset(&mon_conf, 0, sizeof(mon_conf));
    ICLI_PRINTF("direction %s : type %s , monitor configuration\n", ((direction == VTSS_PHY_10G_DIRECTION_LINE) ? "LINE" : (direction == VTSS_PHY_10G_DIRECTION_HOST ? "HOST" : "not selected")), (type == VTSS_PHY_10G_PRBS_TYPE_SERDES ? "serdes" : ((type == VTSS_PHY_10G_PRBS_TYPE_PCS) ? "pcs" : "not selected")));

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (set) {
                    /* enable or disable PCS prbs monitor */
                    if ((rc = vtss_phy_10g_prbs_monitor_conf_set(misc_phy_inst_get(), uport2iport(uport), type, direction, conf_prbs)) != VTSS_RC_OK) {
                        ICLI_PRINTF("ERROR: prbs monitor config failed\n");
                    } else {
                        ICLI_PRINTF("prbs monitor successfully configured!\n");
                    }
                }

                if (vtss_phy_10g_prbs_monitor_conf_get(misc_phy_inst_get(), uport2iport(uport), type, direction, &mon_conf ) == VTSS_RC_OK) {
                    if (type == VTSS_PHY_10G_PRBS_TYPE_SERDES) {
                        ICLI_PRINTF("SERDES PRBS %s \n ", mon_conf.enable ? "ENABLED" : "DISABLED");
                    }

                    if (type == VTSS_PHY_10G_PRBS_TYPE_PCS) {
                        ICLI_PRINTF("PCS PRBS %s \n ", mon_conf.prbs_mon_pcs ? "ENABLED" : "DISABLED");
                    }
                } else {
                    ICLI_PRINTF("vtss_phy_10g_prbs_monitor_conf_get failed on port %u\n", uport);
                }
            }

        }
    }

    return rc;
}

mesa_rc misc_phy_10g_monitor_status( u32 session_id, icli_stack_port_range_t *v_port_type_list, vtss_phy_10g_prbs_type_t type, vtss_phy_10g_direction_t direction, mesa_bool_t reset)
{
    u32                          range_idx = 0, cnt_idx = 0;
    mesa_port_no_t               uport;
    vtss_phy_10g_prbs_monitor_status_t mon_status;
    mesa_rc                      rc = VTSS_RC_ERROR;

    memset(&mon_status, 0, sizeof(mon_status));
    ICLI_PRINTF("direction %s : type %s , monitor status\n", ((direction == VTSS_PHY_10G_DIRECTION_LINE) ? "LINE" : (direction == VTSS_PHY_10G_DIRECTION_HOST ? "HOST" : "not selected")), (type == VTSS_PHY_10G_PRBS_TYPE_SERDES ? "serdes" : ((type == VTSS_PHY_10G_PRBS_TYPE_PCS) ? "pcs" : "not selected")));
    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //ignore if not 10G port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if ((rc = vtss_phy_10g_prbs_monitor_status_get(misc_phy_inst_get(), uport2iport(uport), type, direction, reset, &mon_status)) != VTSS_RC_OK) {
                    ICLI_PRINTF("could not display prbs counters");
                } else {
                    ICLI_PRINTF("port no: %d\n", uport);
                    if (type == VTSS_PHY_10G_PRBS_TYPE_SERDES) {
                        ICLI_PRINTF("error counter for prbs: %u\n", mon_status.error_status);
                        ICLI_PRINTF("prbs status (PRBS data related to 1st sync lost event): %u\n", mon_status.prbs_status);
                        ICLI_PRINTF("main status register for prbs:%u\n", mon_status.main_status);
                        ICLI_PRINTF("stuck at par: %s\nstuck at 0/1: %s\n", mon_status.stuck_at_par ? "true" : "false", mon_status.stuck_at_01 ? "true" : "false");
                        ICLI_PRINTF("no sync: %s\ninstable:%s\n", mon_status.no_sync ? "true" : "false", mon_status.instable ? "true" : "false");
                        ICLI_PRINTF("incomplete: %s\nactive: %s\n", mon_status.incomplete ? "true" : "false", mon_status.active ? "true" : "false");
                    } else if (type == VTSS_PHY_10G_PRBS_TYPE_PCS)  {
                        ICLI_PRINTF("pcs-prbs counters: %u\n", mon_status.prbs_error_count_pcs);
                    }
                }
            }
        }
    }

    return rc;
}

mesa_rc vtss_phy_10g_prbs_generator ( u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_line,
                                      BOOL has_host, u32 prbsn, BOOL has_enable, BOOL has_disable)
{
    u32                         range_idx = 0, cnt_idx = 0;
    mesa_port_no_t              uport;
    vtss_phy_10g_prbs_gen_conf_t conf;
    mesa_rc       rc = VTSS_RC_ERROR;
    memset(&conf, 0, sizeof(conf));
    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (has_line) {
                    conf.line = TRUE;
                } else {
                    conf.line = FALSE;
                }

                if (has_enable) {
                    conf.enable = TRUE;
                    conf.prbsn_sel = prbsn;
                } else {
                    conf.enable = FALSE;
                }

                if ((rc = vtss_phy_10g_prbs_gen_conf(misc_phy_inst_get(), uport2iport(uport), &conf)) != VTSS_RC_OK) {
                    ICLI_PRINTF("ERROR: prbs generator config failed\n");
                } else {
                    ICLI_PRINTF("prbs generator successfully configured!\n");
                }
            }
        }
    }

    return rc;
}

mesa_rc vtss_phy_10g_prbs_counters( u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_line)
{
    u32                          range_idx = 0, cnt_idx = 0;
    mesa_port_no_t               uport;
    vtss_phy_10g_prbs_mon_conf_t conf;
    mesa_rc                      rc = VTSS_RC_ERROR;

    memset(&conf, 0, sizeof(conf));
    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //ignore if not 10G port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (has_line) {
                    conf.line = TRUE;
                } else {
                    conf.line = FALSE;
                }

                if ((rc = vtss_phy_10g_prbs_mon_conf_get(misc_phy_inst_get(), uport2iport(uport), &conf)) != VTSS_RC_OK) {
                    ICLI_PRINTF("could not display prbs counters");
                } else {
                    ICLI_PRINTF("port no: %d\n", uport);
                    ICLI_PRINTF("error counter for prbs: %u\n", conf.error_status);
                    ICLI_PRINTF("prbs status (PRBS data related to 1st sync lost event): %u\n", conf.PRBS_status);
                    ICLI_PRINTF("main status register for prbs:%u\n", conf.main_status);
                    ICLI_PRINTF("stuck at par: %s\nstuck at 0/1: %s\n", conf.stuck_at_par ? "true" : "false", conf.stuck_at_01 ? "true" : "false");
                    ICLI_PRINTF("no sync: %s\ninstable:%s\n", conf.no_sync ? "true" : "false", conf.instable ? "true" : "false");
                    ICLI_PRINTF("incomplete: %s\nactive: %s\n", conf.incomplete ? "true" : "false", conf.active ? "true" : "false");
                }
            }
        }
    }

    return rc;
}

mesa_rc vtss_phy_10g_prbs_monitor( u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_line,
                                   BOOL has_host, u16 prbsn, u16 max_bist_frames, u16 error_states, u16 des_interface_width,
                                   BOOL has_input_invert, u16 no_of_errors, u16 bist_mode, BOOL has_enable, BOOL has_disable)
{
    u32                          range_idx = 0, cnt_idx = 0;
    mesa_port_no_t               uport;
    vtss_phy_10g_prbs_mon_conf_t conf;
    mesa_rc                      rc = VTSS_RC_ERROR;

    memset(&conf, 0, sizeof(conf));
    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //ignore if not 10G port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (has_line) {
                    conf.line = TRUE;
                } else {
                    conf.line = FALSE;
                }

                if (has_enable) {
                    conf.enable = TRUE;
                    conf.max_bist_frames = max_bist_frames;
                    conf.error_states = error_states;
                    conf.des_interface_width = des_interface_width;
                    conf.prbsn_sel = prbsn;
                    if ( has_input_invert) {
                        conf.prbs_check_input_invert = TRUE;
                    }

                    conf.no_of_errors = no_of_errors;
                    conf.bist_mode = bist_mode;
                } else {
                    conf.enable = FALSE;
                }

                if ((rc = vtss_phy_10g_prbs_mon_conf(misc_phy_inst_get(), uport2iport(uport), &conf)) != VTSS_RC_OK) {
                    ICLI_PRINTF(" ERROR: prbs monitor configuration failed\n");
                } else {
                    ICLI_PRINTF("prbs monitor successfully configured \n");
                }
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_10g_phy_pkt_generator(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_enable,
                                        BOOL has_ingress, BOOL has_egress, BOOL has_frames,
                                        BOOL has_ptp, u8 ts_sec, u8 ts_ns, u8 srate, BOOL has_stand, u16 ethtype, u8 pktlen, u32 intpktgap, mesa_mac_t srcmac,
                                        mesa_mac_t desmac, BOOL has_idle, BOOL has_disable)
{
    u32                         range_idx = 0, cnt_idx = 0;
    mesa_port_no_t              uport;
    vtss_phy_10g_pkt_gen_conf_t conf;
    mesa_rc                     rc = VTSS_RC_ERROR;

    memset(&conf, 0, sizeof(conf));
    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (has_enable) {
                    conf.enable = TRUE;
                    if (has_ingress) {
                        conf.ingress = TRUE;
                    } else {
                        conf.ingress = FALSE;
                    }

                    if (has_frames) {
                        conf.frames = TRUE;
                        if (has_ptp) {
                            conf.ptp = TRUE;
                            conf.ptp_ts_sec = ts_sec;
                            conf.ptp_ts_ns  = ts_ns;
                            conf.srate      = srate;
                        } else {
                            conf.ptp = FALSE;
                            conf.ptp_ts_sec = 0;
                            conf.ptp_ts_ns  = 0;
                            conf.srate      = 0;
                        }

                        conf.etype  =  ethtype;
                        conf.pkt_len = pktlen;
                        conf.ipg_len = intpktgap;
                        memcpy(conf.smac, srcmac.addr, 6);
                        memcpy(conf.dmac, desmac.addr, 6);
                    } else if (has_idle) {
                        conf.frames = FALSE;
                    }
                } else if (has_disable) {
                    conf.enable = FALSE;
                }

                ICLI_PRINTF(" smac = 0x%x-%x-%x-%x-%x-%x\n dmac = 0x%x-%x-%x-%x-%x-%x\n conf.ingress = %s\n conf.enable = %s\n conf.ptp =%s   Timestamp(sec) = %d Timestamp(ns) = %d\n conf.frames =%s\n conf.etype = 0x%x\n conf.pkt_len = %d\n conf.ipg_len = %d\n ", conf.smac[0], conf.smac[1],
                            conf.smac[2], conf.smac[3], conf.smac[4], conf.smac[5], conf.dmac[0], conf.dmac[1],
                            conf.dmac[2], conf.dmac[3], conf.dmac[4], conf.dmac[5],
                            ((conf.ingress == TRUE) ? "TRUE" : "FALSE"),
                            ((conf.enable == TRUE) ? "TRUE" : "FALSE"),
                            ((conf.ptp == TRUE) ? "TRUE" : "FALSE"),
                            conf.ptp_ts_sec, conf.ptp_ts_ns,
                            ((conf.frames == TRUE) ? "TRUE" : "FALSE"),
                            conf.etype, conf.pkt_len, conf.ipg_len);

                if ( VTSS_RC_OK != ( rc = vtss_phy_10g_pkt_gen_conf(misc_phy_inst_get(), uport2iport(uport), &conf))) {
                    ICLI_PRINTF("Error: Packet Generator conf failed\n");
                } else {
                    ICLI_PRINTF("Success: Packet Generator conf\n");
                }
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_10g_phy_pkt_monitor_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_enable, BOOL has_update, BOOL has_reset, vtss_phy_10g_pkt_mon_rst_t mon_rst, BOOL has_disable, BOOL has_timestamp)
{
    u32                         range_idx = 0, cnt_idx = 0, value = 0;
    mesa_port_no_t              uport;
    vtss_phy_10g_pkt_mon_conf_t conf;
    mesa_rc       rc = VTSS_RC_ERROR;
    vtss_phy_10g_timestamp_val_t  conf_ts;
    vtss_phy_10g_id_t           chip_id;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (VTSS_RC_OK != (rc = vtss_phy_10g_id_get(misc_phy_inst_get(), uport2iport(uport), &chip_id))) {
                    ICLI_PRINTF("Error in getting the chip_id of port %d (%s)\n", uport, error_txt(rc));
                    continue;
                }

                memset(&conf, 0, sizeof(conf));
                if (has_enable) {
                    conf.enable = TRUE;
                    if (has_update) {
                        conf.update = TRUE;
                    } else if (has_reset) {
                        conf.reset = mon_rst;
                    }
                } else if (has_disable) {
                    conf.update = FALSE;
                }

                if ( VTSS_RC_OK != ( rc = vtss_phy_10g_pkt_mon_conf(misc_phy_inst_get(), uport2iport(uport), has_timestamp, &conf, &conf_ts))) {
                    ICLI_PRINTF("Error: Packet monitor conf failed\n");
                } else if (has_update || has_reset) {
                    ICLI_PRINTF("Good CRC packet count:: %d\n", conf.good_crc);
                    ICLI_PRINTF("Bad CRC packet count:: %d\n", conf.bad_crc);
                    ICLI_PRINTF("Fragmented packet count:: %d\n", conf.frag);
                    ICLI_PRINTF("Local fault packet count:: %d\n", conf.lfault);
                    ICLI_PRINTF("B-errored packet count:: %d\n", conf.ber);
                    /* packet generator count */
                    vtss_phy_10g_csr_read(NULL, uport2iport(uport), 0x4, 0xe931, &value);
                    ICLI_PRINTF("generator count:: 0x%04x", value);
                    vtss_phy_10g_csr_read(NULL, uport2iport(uport), 0x4, 0xe930, &value);
                    ICLI_PRINTF("%04x\n", value);
                }

                if ( has_timestamp ) {
                    if (chip_id.family == VTSS_PHY_FAMILY_MALIBU) {
                        ICLI_PRINTF("Timestamp 0  = %04x  %04x%04x  %04x%04x\n", conf_ts.timestamp[0][4], conf_ts.timestamp[0][3], conf_ts.timestamp[0][2], conf_ts.timestamp[0][1], conf_ts.timestamp[0][0]);
                        ICLI_PRINTF("Timestamp 1  = %04x  %04x%04x  %04x%04x\n", conf_ts.timestamp[1][4], conf_ts.timestamp[1][3], conf_ts.timestamp[1][2], conf_ts.timestamp[1][1], conf_ts.timestamp[1][0]);
                        ICLI_PRINTF("Timestamp 2  = %04x  %04x%04x  %04x%04x\n", conf_ts.timestamp[2][4], conf_ts.timestamp[2][3], conf_ts.timestamp[2][2], conf_ts.timestamp[2][1], conf_ts.timestamp[2][0]);
                        ICLI_PRINTF("Timestamp 3  = %04x  %04x%04x  %04x%04x\n", conf_ts.timestamp[3][4], conf_ts.timestamp[3][3], conf_ts.timestamp[3][2], conf_ts.timestamp[3][1], conf_ts.timestamp[3][0]);
                        ICLI_PRINTF("Timestamp 4  = %04x  %04x%04x  %04x%04x\n", conf_ts.timestamp[4][4], conf_ts.timestamp[4][3], conf_ts.timestamp[4][2], conf_ts.timestamp[4][1], conf_ts.timestamp[4][0]);
                        ICLI_PRINTF("Timestamp 5  = %04x  %04x%04x  %04x%04x\n", conf_ts.timestamp[5][4], conf_ts.timestamp[5][3], conf_ts.timestamp[5][2], conf_ts.timestamp[5][1], conf_ts.timestamp[5][0]);
                        ICLI_PRINTF("Timestamp 6  = %04x  %04x%04x  %04x%04x\n", conf_ts.timestamp[6][4], conf_ts.timestamp[6][3], conf_ts.timestamp[6][2], conf_ts.timestamp[6][1], conf_ts.timestamp[6][0]);
                        ICLI_PRINTF("Timestamp 7  = %04x  %04x%04x  %04x%04x\n", conf_ts.timestamp[7][4], conf_ts.timestamp[7][3], conf_ts.timestamp[7][2], conf_ts.timestamp[7][1], conf_ts.timestamp[7][0]);
                        ICLI_PRINTF("Timestamp 8  = %04x  %04x%04x  %04x%04x\n", conf_ts.timestamp[8][4], conf_ts.timestamp[8][3], conf_ts.timestamp[8][2], conf_ts.timestamp[8][1], conf_ts.timestamp[8][0]);
                        ICLI_PRINTF("Timestamp 9  = %04x  %04x%04x  %04x%04x\n", conf_ts.timestamp[9][4], conf_ts.timestamp[9][3], conf_ts.timestamp[9][2], conf_ts.timestamp[9][1], conf_ts.timestamp[9][0]);
                    } else if (chip_id.family == VTSS_PHY_FAMILY_VENICE) {
                        ICLI_PRINTF("Timestamp   = %04x  %04x%04x  %04x%04x\n", conf_ts.timestamp[0][4], conf_ts.timestamp[0][3], conf_ts.timestamp[0][2], conf_ts.timestamp[0][1], conf_ts.timestamp[0][0]);
                    }
                }
            }
        }
    }

    return rc;
}

static void display_phy_10g_auto_failover_conf(u32 session_id, vtss_phy_10g_auto_failover_conf_t *fail_conf)
{
    ICLI_PRINTF("Cross connect %s \n", fail_conf->enable ? "ENABLED" : "DISABLED");
    ICLI_PRINTF("Event choosen : %u (0-pcs_link_status, 1-los ,2-lof ,3-gpio ,4-manual/none)\n", fail_conf->evnt);
    ICLI_PRINTF("Trigger channel : %u \n", fail_conf->trig_ch_id);
    ICLI_PRINTF("Destination channel : %u (expecting traffic from this channel) \n", fail_conf->channel_id);
    ICLI_PRINTF("Virtual GPIO %u, actual GPIO %u ( make sense only if GPIO event is choosen \n", fail_conf->v_gpio, fail_conf->a_gpio);
    ICLI_PRINTF("Filter type : %u (0-None,1-B2316,2-B70,3-A2316,4-A70)\n", fail_conf->filter);
    ICLI_PRINTF("Filter value : 0x%0x (valid only if filter is choosen)\n", fail_conf->fltr_val);

}

mesa_rc misc_icli_auto_failover(u32 session_id, icli_stack_port_range_t *plist, u16 des_ch, u16 source_ch, BOOL has_host, vtss_phy_10g_auto_failover_event_t event, BOOL has_enable, BOOL is_set, BOOL has_filter, u8 filter_type, u16 filter_val, u16 actual_pin, u16 virtual_pin)
{
    u32                               range_idx = 0, cnt_idx = 0, value = 0;
    mesa_port_no_t                    uport, port_no;
    mesa_rc                           rc = VTSS_RC_ERROR;
    vtss_phy_10g_auto_failover_conf_t fail_conf;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;
                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                port_no = uport2iport(uport);
                if (is_set) {
                    //compulsory components....
                    fail_conf.port_no = port_no;
                    fail_conf.trig_ch_id = source_ch;
                    fail_conf.channel_id = des_ch;
                    fail_conf.is_host_side = has_host;
                    fail_conf.enable = has_enable ? TRUE : FALSE ;
                    fail_conf.evnt = event;

                    //optional inputs.....
                    fail_conf.v_gpio = virtual_pin;
                    fail_conf.a_gpio = actual_pin;

                    if (has_filter) {
                        switch (filter_type) {
                        case 0 :
                            fail_conf.filter = VTSS_PHY_10G_AUTO_FAILOVER_FILTER_NONE;
                            break;
                        case 1 :
                            fail_conf.filter = VTSS_PHY_10G_AUTO_FAILOVER_FILTER_CNT_B2316;
                            break;
                        case 2 :
                            fail_conf.filter = VTSS_PHY_10G_AUTO_FAILOVER_FILTER_CNT_B70;
                            break;
                        case 3 :
                            fail_conf.filter = VTSS_PHY_10G_AUTO_FAILOVER_FILTER_CNT_A2316;
                            break;
                        case 4 :
                            fail_conf.filter = VTSS_PHY_10G_AUTO_FAILOVER_FILTER_CNT_A70;
                        default :
                            ;
                        }

                        fail_conf.fltr_val = filter_val;
                    } else {
                        fail_conf.filter = VTSS_PHY_10G_AUTO_FAILOVER_FILTER_NONE;
                        fail_conf.fltr_val = 0;
                    }

                    if (VTSS_RC_OK != (rc = vtss_phy_10g_auto_failover_set(misc_phy_inst_get(), &fail_conf))) {
                        ICLI_PRINTF("DBG:: auto-failover set :: failed on port %u\n", uport);
                        rc = VTSS_RC_ERROR;
                    }
                } else {
                    ICLI_PRINTF("DBG:: auto-failover set :: success on port %u\n", uport);
                    fail_conf.is_host_side = FALSE;
                    fail_conf.port_no = uport2iport(uport);
                    if (VTSS_RC_OK == (rc = vtss_phy_10g_auto_failover_get(misc_phy_inst_get(), &fail_conf))) {
                        ICLI_PRINTF("LINE Auto failover configuration on port %u \n", uport);
                        ICLI_PRINTF("--------------------------------------------\n");
                        display_phy_10g_auto_failover_conf(session_id, &fail_conf);
                    } else {
                        ICLI_PRINTF("LINE auto-failover get :: failed on port %u\n", uport);
                        rc = VTSS_RC_ERROR;
                    }

                    fail_conf.is_host_side = TRUE;
                    fail_conf.port_no = uport2iport(uport);
                    if (VTSS_RC_OK == (rc = vtss_phy_10g_auto_failover_get(misc_phy_inst_get(), &fail_conf))) {
                        ICLI_PRINTF("HOST Auto failover configuration on port %u \n", uport);
                        ICLI_PRINTF("--------------------------------------------\n");
                        display_phy_10g_auto_failover_conf(session_id, &fail_conf);
                    } else {
                        ICLI_PRINTF("HOST Auto-failover get :: failed on port %u\n", uport);
                        rc = VTSS_RC_ERROR;
                    }

                    if (vtss_phy_10g_csr_read(misc_phy_inst_get(), uport2iport(uport), 0x1e, 0xf102, &value) ==  VTSS_RC_OK) {
                        ICLI_PRINTF("Current cross connect configuration = 0x%0x \n", value);
                        ICLI_PRINTF("--------------------------------------------\n");
                        ICLI_PRINTF("LINE channel_3 <- channel_%u \n", (3) & (value >> 14));
                        ICLI_PRINTF("LINE channel_2 <- channel_%u \n", (3) & (value >> 12));
                        ICLI_PRINTF("LINE channel_1 <- channel_%u \n", (3) & (value >> 10));
                        ICLI_PRINTF("LINE channel_0 <- channel_%u \n", (3) & (value >> 8));
                        ICLI_PRINTF("HOST channel_3 <- channel_%u \n", (3) & (value >> 6));
                        ICLI_PRINTF("HOST channel_2 <- channel_%u \n", (3) & (value >> 4));
                        ICLI_PRINTF("HOST channel_1 <- channel_%u \n", (3) & (value >> 2));
                        ICLI_PRINTF("HOST channel_0 <- channel_%u \n", (3) & (value));

                    }
                }
            } //ending loop of ports...
        }//ending loop of switches....
    }// end of if (plist)
    return rc;
}

mesa_rc misc_icli_gpio_set_get(u32 session_id, icli_stack_port_range_t *plist, u16 gpio_no, BOOL has_mode, u8 mode, BOOL has_port_no, u32 port_no, u8 led_mode, BOOL blink_time, BOOL has_pgpio_no, u16 int_gpio, BOOL has_internal_signal, u8 int_sign, BOOL has_source, u16 source, BOOL has_channel_interrupt, u8 c_intrp, BOOL has_gpio_port_interrupt, BOOL has_set, BOOL has_unset, BOOL has_aggr_interrupt, u32 aggr_mask, BOOL has_gpio_input, u8 input, BOOL use_for_GPIO_interrupt)
{
    u32                       value;
    u32                       range_idx = 0, cnt_idx = 0;
    mesa_port_no_t            uport;
    vtss_gpio_10g_gpio_mode_t gpio_mode;
    BOOL                      cond_flag;
    const char               *mode_print;
    const char               *c_intr_print;
    const char               *sig_print;
    mesa_rc                   rc = VTSS_RC_ERROR;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                gpio_mode.port = uport2iport(uport);
                if ( VTSS_RC_OK != ( rc = vtss_phy_10g_gpio_mode_get(misc_phy_inst_get(), uport2iport(uport), gpio_no, &gpio_mode))) {
                    ICLI_PRINTF("error in retriving mode of gpio");
                }

                if (!(has_mode || has_port_no || has_pgpio_no || has_source || has_channel_interrupt || has_aggr_interrupt  || has_internal_signal || has_gpio_input)) {
                    // print the parameters that are fetched --- this is get mode no extra inputs.....
                    // getting stucture ready to print.
                    switch (gpio_mode.mode) {

                    case VTSS_10G_PHY_GPIO_NOT_INITIALIZED:
                        mode_print = "VTSS_10G_PHY_GPIO_NOT_INITIALIZED";
                        break;
                    case VTSS_10G_PHY_GPIO_OUT:
                        mode_print = "VTSS_10G_PHY_GPIO_OUT";
                        break;
                    case VTSS_10G_PHY_GPIO_IN:
                        mode_print = "VTSS_10G_PHY_GPIO_IN";
                        break;
                    case VTSS_10G_PHY_GPIO_WIS_INT:
                        mode_print = "VTSS_10G_PHY_GPIO_WIS_INT";
                        break;
                    case VTSS_10G_PHY_GPIO_1588_LOAD_SAVE:
                        mode_print = "VTSS_10G_PHY_GPIO_1588_LOAD_SAVE";
                        break;
                    case VTSS_10G_PHY_GPIO_1588_1PPS_0:
                        mode_print = "VTSS_10G_PHY_GPIO_1588_1PPS_0";
                        break;
                    case VTSS_10G_PHY_GPIO_1588_1PPS_1:
                        mode_print = "VTSS_10G_PHY_GPIO_1588_1PPS_1";
                        break;
                    case VTSS_10G_PHY_GPIO_1588_1PPS_2:
                        mode_print = "VTSS_10G_PHY_GPIO_1588_1PPS_2";
                        break;
                    case VTSS_10G_PHY_GPIO_1588_1PPS_3:
                        mode_print = "VTSS_10G_PHY_GPIO_1588_1PPS_3";
                        break;
                    case VTSS_10G_PHY_GPIO_PCS_RX_FAULT:
                        mode_print = "VTSS_10G_PHY_GPIO_PCS_RX_FAULT";
                        break;
                    case VTSS_10G_PHY_GPIO_SET_I2C_MASTER:
                        mode_print = "VTSS_10G_PHY_GPIO_SET_I2C_MASTER";
                        break;
                    case VTSS_10G_PHY_GPIO_TX_ENABLE:
                        mode_print = "VTSS_10G_PHY_GPIO_TX_ENABLE";
                        break;
                    case VTSS_10G_PHY_GPIO_LINE_PLL_STATUS:
                        mode_print = "VTSS_10G_PHY_GPIO_LINE_PLL_STATUS";
                        break;
                    case VTSS_10G_PHY_GPIO_HOST_PLL_STATUS:
                        mode_print = "VTSS_10G_PHY_GPIO_HOST_PLL_STATUS";
                        break;
                    case VTSS_10G_PHY_GPIO_RCOMP_BUSY:
                        mode_print = "VTSS_10G_PHY_GPIO_RCOMP_BUSY";
                        break;
                    case VTSS_10G_PHY_GPIO_CHAN_INT_0:
                        mode_print = "VTSS_10G_PHY_GPIO_CHAN_INT_0";
                        break;
                    case VTSS_10G_PHY_GPIO_CHAN_INT_1:
                        mode_print = "VTSS_10G_PHY_GPIO_CHAN_INT_1";
                        break;
                    case VTSS_10G_PHY_GPIO_1588_INT:
                        mode_print = "VTSS_10G_PHY_GPIO_1588_INT";
                        break;
                    case VTSS_10G_PHY_GPIO_TS_FIFO_EMPTY:
                        mode_print = "VTSS_10G_PHY_GPIO_TS_FIFO_EMPTY";
                        break;
                    case VTSS_10G_PHY_GPIO_AGG_INT_0:
                        mode_print = "VTSS_10G_PHY_GPIO_AGG_INT_0";
                        break;
                    case VTSS_10G_PHY_GPIO_AGG_INT_1:
                        mode_print = "VTSS_10G_PHY_GPIO_AGG_INT_1";
                        break;
                    case VTSS_10G_PHY_GPIO_AGG_INT_2:
                        mode_print = "VTSS_10G_PHY_GPIO_AGG_INT_2";
                        break;
                    case VTSS_10G_PHY_GPIO_AGG_INT_3:
                        mode_print = "VTSS_10G_PHY_GPIO_AGG_INT_3";
                        break;
                    case VTSS_10G_PHY_GPIO_PLL_INT_0:
                        mode_print = "VTSS_10G_PHY_GPIO_PLL_INT_0";
                        break;
                    case VTSS_10G_PHY_GPIO_PLL_INT_1:
                        mode_print = "VTSS_10G_PHY_GPIO_PLL_INT_1";
                        break;
                    case VTSS_10G_PHY_GPIO_SET_I2C_SLAVE:
                        mode_print = "VTSS_10G_PHY_GPIO_SET_I2C_SLAVE";
                        break;
                    case VTSS_10G_PHY_GPIO_CRSS_INT:
                        mode_print = "VTSS_10G_PHY_GPIO_CRSS_INT";
                        break;
                    case VTSS_10G_PHY_GPIO_LED:
                        mode_print = "VTSS_10G_PHY_GPIO_LED";
                        break;
                    case VTSS_10G_PHY_GPIO_DRIVE_LOW:
                        mode_print = "VTSS_10G_PHY_GPIO_DRIVE_LOW";
                        break;
                    case VTSS_10G_PHY_GPIO_DRIVE_HIGH:
                        mode_print = "VTSS_10G_PHY_GPIO_DRIVE_HIGH";
                        break;
                    default:
                        mode_print = "mode not found";
                    }

                    switch (gpio_mode.in_sig) {
                    case VTSS_10G_GPIO_INTR_SGNL_I2C_MSTR_DATA_OUT :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_I2C_MSTR_DATA_OUT";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_I2C_MSTR_CLK_OUT :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_I2C_MSTR_CLK_OUT";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LED_TX :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LED_TX";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LED_RX :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LED_RX" ;
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_RX_ALARM :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_RX_ALARM";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_TX_ALARM :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_TX_ALARM";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_HOST_LINK :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_HOST_LINK";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINE_LINK :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINE_LINK";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINE_KR_8b10b_2GPIO :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINE_KR_8b10b_2GPIO";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINE_KR_10b_2GPIO :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINE_KR_10b_2GPIO" ;
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_ROSI_PULSE :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_ROSI_PULSE" ;
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_ROSI_SDATA :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_ROSI_SDATA" ;
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_ROSI_SCLK :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_ROSI_SCLK" ;
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_TOSI_PULSE :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_TOSI_PULSE" ;
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_TOSI_SCLK :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_TOSI_SCLK" ;
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINE_PCS1G_LINK :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINE_PCS1G_LINK" ;
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINE_PCS_RX_STAT :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINE_PCS_RX_STAT" ;
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_CLIENT_PCS1G_LINK :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_CLIENT_PCS1G_LINK";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_HOST_PCS_RX_STAT :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_HOST_PCS_RX_STAT" ;
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_HOST_SD10G_IB_SIG :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_HOST_SD10G_IB_SIG";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINE_SD10G_IB_SIG :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINE_SD10G_IB_SIG";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_HPCS_INTR :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_HPCS_INTR";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LPCS_INTR :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LPCS_INTR";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_CLIENT_PCS1G_INTR :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_CLIENT_PCS1G_INTR";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINE_PCS1G_INTR :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINE_PCS1G_INTR";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_WIS_INT0 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_WIS_INT0";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_HOST_PMA_INT :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_HOST_PMA_INT";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINE_PMA_INT :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINE_PMA_INT";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_DATA_ACT_TX :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_DATA_ACT_TX";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_DATA_ACT_RX :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_DATA_ACT_RX";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_HDATA_ACT_TX :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_HDATA_ACT_TX";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_HDATA_ACT_RX :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_HDATA_ACT_RX";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_XGMII_PAUS_EGR :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_XGMII_PAUS_EGR";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_XGMII_PAUS_ING :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_XGMII_PAUS_ING";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_RX_PCS_PAUS :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_RX_PCS_PAUS";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_TX_PCS_PAUS :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_TX_PCS_PAUS";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_RX_WIS_PAUS :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_RX_WIS_PAUS";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_TX_WIS_PAUS :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_TX_WIS_PAUS";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_ETH_CHAN_DIS :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_ETH_CHAN_DIS";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_MACSEC_1588_SFD_LANE :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_MACSEC_1588_SFD_LANE";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINE_S_TX_FAULT :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINE_S_TX_FAULT";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LPCS1G_LATENCY0_OR_EWIS_BIT0 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LPCS1G_LATENCY0_OR_EWIS_BIT0";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LPCS1G_LATENCY1_OR_EWIS_BIT1 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LPCS1G_LATENCY1_OR_EWIS_BIT1";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS0_OR_EWIS_BIT2 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS0_OR_EWIS_BIT2";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS1_OR_EWIS_WORD0 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS1_OR_EWIS_WORD0";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS2_OR_EWIS_WORD1 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS2_OR_EWIS_WORD1";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS3_OR_EWIS_WORD2 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS3_OR_EWIS_WORD2";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_MACSEC_IGR_PRED_VAR0 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_MACSEC_IGR_PRED_VAR0";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_MACSEC_IGR_PRED_VAR1 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_MACSEC_IGR_PRED_VAR1";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_KR_ACTV_2GPIO :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_KR_ACTV_2GPIO";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_DFT_TX_2GPIO :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_DFT_TX_2GPIO";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_RESERVED :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_RESERVED";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_0 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_0";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_1 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_1";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_2 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_2";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_3 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_3";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_4 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_4";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINK_HCD_2GPIO_0 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINK_HCD_2GPIO_0";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINK_HCD_2GPIO_1 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINK_HCD_2GPIO_1";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_LINK_HCD_2GPIO_2 :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_LINK_HCD_2GPIO_2";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_ETH_1G_ENA :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_ETH_1G_ENA";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_H_KR_8b10b_2GIPO :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_H_KR_8b10b_2GIPO";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_H_KR_10Gb_2GPIO :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_H_KR_10Gb_2GPIO";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_H_KR_ACTV_2GPIO :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_H_KR_ACTV_2GPIO";
                        break;
                    case VTSS_10G_GPIO_INTR_SGNL_NONE :
                        sig_print = "VTSS_10G_GPIO_INTR_SGNL_NONE";
                        break;
                    default :
                        sig_print = "signal value not found";
                    }

                    switch (gpio_mode.c_intrpt) {
                    case VTSS_10G_GPIO_INTRPT_WIS0 :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_WIS0";
                        break;
                    case VTSS_10G_GPIO_INTRPT_WIS1 :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_WIS1";
                        break;
                    case VTSS_10G_GPIO_INTRPT_LPCS10G :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_LPCS10G";
                        break;
                    case VTSS_10G_GPIO_INTRPT_HPCS10G :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_HPCS10G";
                        break;
                    case VTSS_10G_GPIO_INTRPT_LPCS1G :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_LPCS1G";
                        break;
                    case VTSS_10G_GPIO_INTRPT_HPCS1G :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_HPCS1G";
                        break;
                    case VTSS_10G_GPIO_INTRPT_MSEC_EGR :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_MSEC_EGR";
                        break;
                    case VTSS_10G_GPIO_INTRPT_MSEC_IGR :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_MSEC_IGR";
                        break;
                    case VTSS_10G_GPIO_INTRPT_LMAC :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_LMAC";
                        break;
                    case VTSS_10G_GPIO_INTRPT_HMAC :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_HMAC";
                        break;
                    case VTSS_10G_GPIO_INTRPT_FCBUF :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_FCBUF";
                        break;
                    case VTSS_10G_GPIO_INTRPT_LIGR_FIFO :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_LIGR_FIFO";
                        break;
                    case VTSS_10G_GPIO_INTRPT_LEGR_FIFO :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_LEGR_FIFO";
                        break;
                    case VTSS_10G_GPIO_INTRPT_HEGR_FIFO :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_HEGR_FIFO";
                        break;
                    case VTSS_10G_GPIO_INTRPT_LPMA :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_LPMA";
                        break;
                    case VTSS_10G_GPIO_INTRPT_HPMA :
                        c_intr_print = "VTSS_10G_GPIO_INTRPT_HPMA";
                        break;
                    default :
                        c_intr_print = "channel interrupt not found";
                    }
                    //printing the structure...
                    ICLI_PRINTF("PORT NO.               : %d\n", uport);
                    ICLI_PRINTF("Mode                   : %s\n", mode_print);
                    ICLI_PRINTF("GPIO                   : %d\n", gpio_no);
                    ICLI_PRINTF("Port no.               : %d\n", gpio_mode.port);
                    ICLI_PRINTF("Internal signal        : %s\n", sig_print);
                    ICLI_PRINTF("Per channel GPIO No.   : %d\n", gpio_mode.p_gpio);
                    ICLI_PRINTF("Per channel interrupt  : %s\n", c_intr_print);
                    ICLI_PRINTF("Source                 : %d\n", gpio_mode.source);
                    ICLI_PRINTF("AGGR interrupt bitmask : 0x%0x\n", gpio_mode.aggr_intrpt);
                    ICLI_PRINTF("GPIO Input             : %u\n", gpio_mode.input);
                    ICLI_PRINTF("\n\n");
                    // Pin status
                    ICLI_PRINTF("GPIO pin status: \n");
                    vtss_phy_10g_gpio_read(NULL, uport2iport(uport), gpio_no, &cond_flag);
                    ICLI_PRINTF("Status of pin bit is : %d\n", cond_flag ? 1 : 0);
                    // Channel interrupt status
                    vtss_phy_10g_csr_read(NULL, uport2iport(uport), 0x1, 0xc01a, &value);
                    ICLI_PRINTF("Channel interrupt 0 status    0x%08x\n", value);
                    vtss_phy_10g_csr_read(NULL, uport2iport(uport), 0x1, 0xc01b, &value);
                    ICLI_PRINTF("Channel interrupt 1 status    0x%08x\n", value);
                    vtss_phy_10g_csr_read(NULL, uport2iport(uport), 0x1e, 0xf260, &value);
                    ICLI_PRINTF("Aggregated interrupt 0 status 0x%08x\n", value);
                    vtss_phy_10g_csr_read(NULL, uport2iport(uport), 0x1e, 0xf261, &value);
                    ICLI_PRINTF("Aggregated interrupt 1 status 0x%08x\n", value);
                    vtss_phy_10g_csr_read(NULL, uport2iport(uport), 0x1e, 0xf262, &value);
                    ICLI_PRINTF("Aggregated interrupt 2 status 0x%08x\n", value);
                    vtss_phy_10g_csr_read(NULL, uport2iport(uport), 0x1e, 0xf263, &value);
                    ICLI_PRINTF("Aggregated interrupt 3 status 0x%08x\n", value);
                    ICLI_PRINTF("Being used as GPIO interrupt source %s\n", gpio_mode.use_as_intrpt ? "YES" : "NO");

                } else {
                    //settting if any of the optional field is entered.
                    //converting the integers entered to the enum strings.
                    switch (mode) {
                    case 0 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_NOT_INITIALIZED;
                        break;
                    case 1 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_OUT;
                        break;
                    case 2 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_IN;
                        break;
                    case 3 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_WIS_INT;
                        break;
                    case 4 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_1588_LOAD_SAVE;
                        break;
                    case 5 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_1588_1PPS_0;
                        break;
                    case 6 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_1588_1PPS_1;
                        break;
                    case 7 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_1588_1PPS_2;
                        break;
                    case 8 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_1588_1PPS_3;
                        break;
                    case 9 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_PCS_RX_FAULT;
                        break;
                    case 10 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_SET_I2C_MASTER;
                        break;
                    case 11 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_TX_ENABLE;
                        break;
                    case 12 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_LINE_PLL_STATUS;
                        break;
                    case 13 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_HOST_PLL_STATUS;
                        break;
                    case 14 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_RCOMP_BUSY;
                        break;
                    case 15 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_CHAN_INT_0;
                        break;
                    case 16 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_CHAN_INT_1;
                        break;
                    case 17 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_1588_INT;
                        break;
                    case 18 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_TS_FIFO_EMPTY;
                        break;
                    case 19 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_AGG_INT_0;
                        break;
                    case 20 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_AGG_INT_1;
                        break;
                    case 21 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_AGG_INT_2;
                        break;
                    case 22 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_AGG_INT_3;
                        break;
                    case 23 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_PLL_INT_0;
                        break;
                    case 24 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_PLL_INT_1;
                        break;
                    case 25 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_SET_I2C_SLAVE;
                        break;
                    case 26 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_CRSS_INT;
                        break;
                    case 27 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_LED;
                        break;
                    case 28 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_DRIVE_LOW;
                        break;
                    case 29 :
                        gpio_mode.mode = VTSS_10G_PHY_GPIO_DRIVE_HIGH;
                        break;
                    default :
                        ;
                    }

                    if (!has_internal_signal) {
                        int_sign = 64;
                    }

                    switch (int_sign) {
                    case 0 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_I2C_MSTR_DATA_OUT;
                        break;
                    case 1 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_I2C_MSTR_CLK_OUT;
                        break;
                    case 2 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LED_TX;
                        break;
                    case 3 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LED_RX;
                        break;
                    case 4 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_RX_ALARM;
                        break;
                    case 5 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_TX_ALARM;
                        break;
                    case 6 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_HOST_LINK;
                        break;
                    case 7 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINE_LINK;
                        break;
                    case 8 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINE_KR_8b10b_2GPIO;
                        break;
                    case 9 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINE_KR_10b_2GPIO;
                        break;
                    case 10 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_ROSI_PULSE;
                        break;
                    case 11 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_ROSI_SDATA;
                        break;
                    case 12 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_ROSI_SCLK;
                        break;
                    case 13 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_TOSI_PULSE;
                        break;
                    case 14 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_TOSI_SCLK;
                        break;
                    case 15 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINE_PCS1G_LINK;
                        break;
                    case 16 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINE_PCS_RX_STAT;
                        break;
                    case 17 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_CLIENT_PCS1G_LINK;
                        break;
                    case 18 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_HOST_PCS_RX_STAT;
                        break;
                    case 19 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_HOST_SD10G_IB_SIG;
                        break;
                    case 20 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINE_SD10G_IB_SIG;
                        break;
                    case 21 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_HPCS_INTR;
                        break;
                    case 22 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LPCS_INTR;
                        break;
                    case 23 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_CLIENT_PCS1G_INTR;
                        break;
                    case 24 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINE_PCS1G_INTR;
                        break;
                    case 25 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_WIS_INT0;
                        break;
                    case 26 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_HOST_PMA_INT;
                        break;
                    case 27 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINE_PMA_INT;
                        break;
                    case 28 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_DATA_ACT_TX;
                        break;
                    case 29 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_DATA_ACT_RX;
                        break;
                    case 30 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_HDATA_ACT_TX;
                        break;
                    case 31 :
                        gpio_mode.in_sig =  VTSS_10G_GPIO_INTR_SGNL_HDATA_ACT_RX;
                        break;
                    case 32 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_XGMII_PAUS_EGR;
                        break;
                    case 33 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_XGMII_PAUS_ING;
                        break;
                    case 34 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_RX_PCS_PAUS;
                        break;
                    case 35 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_TX_PCS_PAUS;
                        break;
                    case 36 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_RX_WIS_PAUS;
                        break;
                    case 37 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_TX_WIS_PAUS;
                        break;
                    case 38 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_ETH_CHAN_DIS;
                        break;
                    case 39 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_MACSEC_1588_SFD_LANE;
                        break;
                    case 40 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINE_S_TX_FAULT;
                        break;
                    case 41 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LPCS1G_LATENCY0_OR_EWIS_BIT0;
                        break;
                    case 42 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LPCS1G_LATENCY1_OR_EWIS_BIT1;
                        break;
                    case 43 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS0_OR_EWIS_BIT2;
                        break;
                    case 44 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS1_OR_EWIS_WORD0;
                        break;
                    case 45 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS2_OR_EWIS_WORD1;
                        break;
                    case 46 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LPCS1G_CHAR_POS3_OR_EWIS_WORD2;
                        break;
                    case 47 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_MACSEC_IGR_PRED_VAR0;
                        break;
                    case 48 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_MACSEC_IGR_PRED_VAR1;
                        break;
                    case 49 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_KR_ACTV_2GPIO;
                        break;
                    case 50 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_DFT_TX_2GPIO;
                        break;
                    case 51 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_RESERVED;
                        break;
                    case 52 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_0;
                        break;
                    case 53 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_1;
                        break;
                    case 54 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_2;
                        break;
                    case 55 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_3;
                        break;
                    case 56 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_EXE_LST_2GPIO_4;
                        break;
                    case 57 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINK_HCD_2GPIO_0;
                        break;
                    case 58 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINK_HCD_2GPIO_1;
                        break;
                    case 59 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_LINK_HCD_2GPIO_2;
                        break;
                    case 60 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_ETH_1G_ENA;
                        break;
                    case 61 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_H_KR_8b10b_2GIPO;
                        break;
                    case 62 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_H_KR_10Gb_2GPIO;
                        break;
                    case 63 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_H_KR_ACTV_2GPIO;
                        break;
                    case 64 :
                        gpio_mode.in_sig = VTSS_10G_GPIO_INTR_SGNL_NONE;
                        break;
                    default:
                        ;
                    }

                    switch (c_intrp) {
                    case 0 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_WIS0;
                        break;
                    case 1 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_WIS1;
                        break;
                    case 2 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_LPCS10G;
                        break;
                    case 3 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_HPCS10G;
                        break;
                    case 4 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_LPCS1G;
                        break;
                    case 5 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_HPCS1G;
                        break;
                    case 6 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_MSEC_EGR;
                        break;
                    case 7 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_MSEC_IGR;
                        break;
                    case 8 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_LMAC;
                        break;
                    case 9 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_HMAC;
                        break;
                    case 10 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_FCBUF;
                        break;
                    case 11 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_LIGR_FIFO;
                        break;
                    case 12 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_LEGR_FIFO;
                        break;
                    case 13 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_HEGR_FIFO;
                        break;
                    case 14 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_LPMA;
                        break;
                    case 15 :
                        gpio_mode.c_intrpt = VTSS_10G_GPIO_INTRPT_HPMA;
                        break;
                    default :
                        ;
                    }

                    if (has_gpio_input) {
                        gpio_mode.input = (vtss_gpio_10g_input_t)input;
                    } else {
                        gpio_mode.input = VTSS_10G_GPIO_INPUT_NONE;
                    }

                    if (has_gpio_port_interrupt) {
                        if (has_set) {
                            ICLI_PRINTF("Setting Port GPIO interrupt change\n");
                            gpio_mode.p_gpio_intrpt = TRUE;
                        }
                    }

                    switch (led_mode) {
                    case 0 :
                        gpio_mode.led_conf.mode = VTSS_10G_GPIO_LED_NONE;
                        break;
                    case 1 :
                        gpio_mode.led_conf.mode = VTSS_10G_GPIO_LED_TX_LINK_STATUS;
                        break;
                    case 3 :
                        gpio_mode.led_conf.mode = VTSS_10G_GPIO_LED_TX_LINK_TX_DATA;
                        break;
                    case 4 :
                        gpio_mode.led_conf.mode = VTSS_10G_GPIO_LED_TX_LINK_TX_RX_DATA;
                        break;
                    case 5 :
                        gpio_mode.led_conf.mode = VTSS_10G_GPIO_LED_RX_LINK_STATUS;
                        break;
                    case 7 :
                        gpio_mode.led_conf.mode = VTSS_10G_GPIO_LED_RX_LINK_RX_DATA;
                        break;
                    case 8 :
                        gpio_mode.led_conf.mode = VTSS_10G_GPIO_LED_RX_LINK_TX_RX_DATA;
                        break;
                    default :
                        ;
                    }

                    switch (blink_time) {
                    case 0 :
                        gpio_mode.led_conf.blink = VTSS_10G_GPIO_LED_BLINK_50MS;
                        break;
                    case 1 :
                        gpio_mode.led_conf.blink = VTSS_10G_GPIO_LED_BLINK_100MS;
                        break;
                    }

                    gpio_mode.port = port_no;
                    gpio_mode.p_gpio = int_gpio;
                    gpio_mode.source = source;
                    gpio_mode.aggr_intrpt |= 1 << aggr_mask;
                    gpio_mode.use_as_intrpt = use_for_GPIO_interrupt;
                    if ( VTSS_RC_OK != ( rc = vtss_phy_10g_gpio_mode_set(misc_phy_inst_get(), uport2iport(uport), gpio_no, &gpio_mode))) {
                        ICLI_PRINTF("error in setting mode of gpio");
                    }
                }
            }//looping through ports.
        }//looping through switches in stack.
    }//if(plist)
    return rc;
}

static const char *lb_type2txt(const vtss_phy_10g_loopback_t *lb)
{
    switch (lb->lb_type) {
    case VTSS_LB_NONE:
        return "No looback   ";
    case VTSS_LB_SYSTEM_XS_SHALLOW:
        return "System Loopback B,  XAUI -> XS -> XAUI   4x800E.13";
    case VTSS_LB_SYSTEM_XS_DEEP:
        return "System Loopback C,  XAUI -> XS -> XAUI   4x800F.2";
    case VTSS_LB_SYSTEM_PCS_SHALLOW:
        return "System Loopback E,  XAUI -> PCS FIFO -> XAUI 3x8005.2";
    case VTSS_LB_SYSTEM_PCS_DEEP:
        return "System Loopback G,  XAUI -> PCS -> XAUI  3x0000.14";
    case VTSS_LB_SYSTEM_PMA:
        return "System Loopback J,  XAUI -> PMA -> XAUI  1x0000.0";
    case VTSS_LB_NETWORK_XS_SHALLOW:
        return "Network Loopback D,  XFI -> XS -> XFI   4x800F.1";
    case VTSS_LB_NETWORK_XS_DEEP:
        return "Network Loopback A,  XFI -> XS -> XFI   4x0000.1  4x800E.13=0";
    case VTSS_LB_NETWORK_PCS:
        return "Network Loopback F,  XFI -> PCS -> XFI  3x8005.3";
    case VTSS_LB_NETWORK_WIS:
        return "Network Loopback H,  XFI -> WIS -> XFI  2xE600.0";
    case VTSS_LB_NETWORK_PMA:
        return "Network Loopback K,  XFI -> PMA -> XFI  1x8000.8";
    /* Venice specific loopbacks, the Venice implementation is different, and therefore the loopbacks are not exactly the same */
    case VTSS_LB_H2:
        return "Host Loopback 2, 40-bit XAUI-PHY interface Mirror XAUI data";
    case VTSS_LB_H3:
        return "Host Loopback 3, 64-bit PCS after the gearbox FF00 repeating IEEE PCS system loopback";
    case VTSS_LB_H4:
        return "Host Loopback 4, 64-bit WIS FF00 repeating IEEE WIS system loopback";
    case VTSS_LB_H5:
        return "Host Loopback 5, 1-bit SFP+ after SerDes Mirror XAUI data IEEE PMA system loopback";
    case VTSS_LB_H6:
        return "Host Loopback 6, 32-bit XAUI-PHY interface Mirror XAUI data";
    case VTSS_LB_L0:
        return "Line Loopback 0, 4-bit XAUI before SerDes Mirror SFP+ data";
    case VTSS_LB_L1:
        return "Line Loopback 1, 4-bit XAUI after SerDes Mirror SFP+ data IEEE PHY-XS network loopback";
    case VTSS_LB_L2:
        return "Line Loopback 2, 64-bit XGMII after FIFO Mirror SFP+ data";
    case VTSS_LB_L3:
        return "Loopback 3, 64-bit PMA interface Mirror SFP+ data";
    case VTSS_LB_L2C:
        return "Loopback l2c, 64-bit XGMII interface Mirror SFP+ data";
    }

    return "INVALID";
}

void misc_icli_10g_phy_loopback(u32 session_id, BOOL has_a, BOOL has_b, BOOL has_c, BOOL has_d, BOOL has_e, BOOL has_f,
                                BOOL has_g, BOOL has_h, BOOL has_j, BOOL has_k, BOOL has_h2, BOOL has_h3, BOOL has_h4,
                                BOOL has_h5, BOOL has_h6, BOOL has_l0, BOOL has_l1, BOOL has_l2, BOOL has_l3, BOOL has_l2c,
                                BOOL has_enable, BOOL has_disable,
                                icli_stack_port_range_t *v_port_type_list)
{
    vtss_phy_10g_loopback_t lb;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    mesa_rc rc;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                if (VTSS_RC_OK != (rc = vtss_phy_10g_loopback_get(misc_phy_inst_get(), uport2iport(uport), &lb))) {
                    ICLI_PRINTF("Error getting loopback configuration for port %d (%s)\n", uport, error_txt(rc));
                } else {
                    lb.enable = has_enable ? TRUE : has_disable ? FALSE : lb.enable;
                    lb.lb_type = has_a ? VTSS_LB_NETWORK_XS_DEEP :
                                 has_b ? VTSS_LB_SYSTEM_XS_SHALLOW :
                                 has_c ? VTSS_LB_SYSTEM_XS_DEEP :
                                 has_d ? VTSS_LB_NETWORK_XS_SHALLOW :
                                 has_e ? VTSS_LB_SYSTEM_PCS_SHALLOW :
                                 has_f ? VTSS_LB_NETWORK_PCS :
                                 has_g ? VTSS_LB_SYSTEM_PCS_DEEP :
                                 has_h ? VTSS_LB_NETWORK_WIS :
                                 has_j ? VTSS_LB_SYSTEM_PMA :
                                 has_k ? VTSS_LB_NETWORK_PMA :
                                 has_h2 ? VTSS_LB_H2 :
                                 has_h3 ? VTSS_LB_H3 :
                                 has_h4 ? VTSS_LB_H4 :
                                 has_h5 ? VTSS_LB_H5 :
                                 has_h6 ? VTSS_LB_H6 :
                                 has_l0 ? VTSS_LB_L0 :
                                 has_l1 ? VTSS_LB_L1 :
                                 has_l2 ? VTSS_LB_L2 :
                                 has_l3 ? VTSS_LB_L3 :
                                 has_l2c ? VTSS_LB_L2C :
                                 lb.lb_type;
                    ICLI_PRINTF("Port %d, loopback %s : %s\n", uport, lb.enable ? "enable" : "disable", lb.enable ? lb_type2txt(&lb) : "");
                    if (has_enable || has_disable) {
                        if (VTSS_RC_OK != (rc = vtss_phy_10g_loopback_set(misc_phy_inst_get(), uport2iport(uport), &lb))) {
                            ICLI_PRINTF("Error setting loopback configuration for port %d (%s)\n", uport, error_txt(rc));
                        }
                    }
                }
            }
        }
    }
}

static const char *mode_type2txt(const vtss_phy_10g_mode_t *mode)
{
    switch (mode->oper_mode) {
    case VTSS_PHY_LAN_MODE:
        return "lan";
    case VTSS_PHY_WAN_MODE:
        return "wan";
    case VTSS_PHY_1G_MODE:
        return "1g";
    case VTSS_PHY_LAN_SYNCE_MODE:
        return "LAN SyncE";
    case VTSS_PHY_WAN_SYNCE_MODE:
        return "WAN SyncE";
    case VTSS_PHY_LAN_MIXED_SYNCE_MODE:
        return "LAN SyncE for Venice , Mixed LAN mode for 8488";
    case VTSS_PHY_WAN_MIXED_SYNCE_MODE:
        return "WAN SyncE for Venice , Mixed WAN mode for 8488";
    case VTSS_PHY_REPEATER_MODE:
        return "Repeater Mode";
    }

    return "INVALID";
}

static const char *interface_type2txt(const vtss_phy_10g_mode_t *mode)
{
    switch (mode->interface) {
    case VTSS_PHY_XAUI_XFI:
        return "XAUI";
    case VTSS_PHY_RXAUI_XFI:
        return "RXAUI";
    case VTSS_PHY_XGMII_XFI:
        return "XGMII";
    case VTSS_PHY_SGMII_LANE_0_XFI:
        return "SGMII LANE 0";
    case VTSS_PHY_SGMII_LANE_3_XFI:
        return "SGMII LANE 3";
    case VTSS_PHY_SFI_XFI:
        return "SFI-XFI";
    }

    return "INVALID";
}

static const char *get_apc_op_mode_txt(vtss_phy_10g_ib_apc_op_mode_t opmode)
{
    switch (opmode) {
    case 0:
        return "AUTO";
    case 1:
        return "MANUAL";
    case 2:
        return "FREEZE";
    case 3:
        return "RESET";
    case 4:
        return "RESTART";
    default:
        return "Invalid";
    }
}

void misc_icli_10g_phy_apc_conf(u32 session_id, vtss_phy_10g_apc_conf_t *apc_conf, icli_stack_port_range_t *v_port_type_list, BOOL is_set, BOOL is_host)
{
    u32                       range_idx, cnt_idx;
    mesa_port_no_t            uport;
    vtss_phy_10g_apc_status_t apc_status;

    if (v_port_type_list) { //at least one range input
        //Loop through all switches in the stack
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            //Loop through all ports in the switch
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(PHY_INST, uport2iport(uport))) {
                    continue;
                }

                if (is_set == TRUE) {
                    if (vtss_phy_10g_apc_conf_set(PHY_INST, uport2iport(uport), apc_conf, is_host) == VTSS_RC_OK) {
                        ICLI_PRINTF("Configuration is Successfull\n");
                    }
                } else {
                    if (vtss_phy_10g_apc_conf_get(PHY_INST, uport2iport(uport), is_host, apc_conf) == VTSS_RC_OK) {
                        ICLI_PRINTF("Current mode of operation %s\n", get_apc_op_mode_txt(apc_conf->op_mode));
                    }

                    if (vtss_phy_10g_apc_status_get(PHY_INST, uport2iport(uport), is_host, &apc_status) == VTSS_RC_OK) {
                        ICLI_PRINTF("APC freeze %s\n", apc_status.freeze ? "TRUE" : "FALSE");
                        ICLI_PRINTF("APC reset  %s\n", apc_status.reset ? "TRUE" : "FALSE");
                    }
                }
            }
        }
    }
}

void misc_icli_10g_phy_serdes_status(u32 session_id, vtss_phy_10g_serdes_status_t *serdes, icli_stack_port_range_t *v_port_type_list)
{
    u32                       range_idx, cnt_idx;
    mesa_port_no_t            uport;

    if (v_port_type_list) { //at least one range input
        //Loop through all switches in the stack
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            //Loop through all ports in the switch
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(PHY_INST, uport2iport(uport))) {
                    continue;
                }

                if (vtss_phy_10g_serdes_status_get(PHY_INST, uport2iport(uport), serdes) == VTSS_RC_OK) {
                    ICLI_PRINTF(">>>>>>>>>>>>>>>>>>> SERDES Status <<<<<<<<<<<<<<<<<<<<<<\n");
                    ICLI_PRINTF(" %-32s :: %u \n", "RCOMP value", serdes->rcomp);
                    ICLI_PRINTF(">> HOST PLL Status\n");
                    ICLI_PRINTF(" %-32s :: %s\n", "PLL5G Lock status", serdes->h_pll5g_lock_status ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %s\n", "FSM lock status", serdes->h_pll5g_fsm_lock ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %u \n", "FSM status", serdes->h_pll5g_fsm_stat);
                    ICLI_PRINTF(" %-32s :: %u \n", "Gain", serdes->h_pll5g_gain);

                    ICLI_PRINTF(">> LINE PLL Status\n");
                    ICLI_PRINTF(" %-32s :: %s\n", "PLL5G Lock status", serdes->l_pll5g_lock_status ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %s\n", "FSM lock status", serdes->l_pll5g_fsm_lock ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %u \n", "FSM status", serdes->l_pll5g_fsm_stat);
                    ICLI_PRINTF(" %-32s :: %u \n", "Gain", serdes->l_pll5g_gain);

                    ICLI_PRINTF(">> HOST RX-RCPLL Status\n");
                    ICLI_PRINTF(" %-32s :: %s \n", "LOCK status", serdes->h_rx_rcpll_lock_status ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %u \n", "RC-Pll range", serdes->h_rx_rcpll_range);
                    ICLI_PRINTF(" %-32s :: %u \n", "VCO Load", serdes->h_rx_rcpll_vco_load);
                    ICLI_PRINTF(" %-32s :: %u \n", "FSM status", serdes->h_rx_rcpll_fsm_status);
                    ICLI_PRINTF(">> LINE RX-RCPLL Status\n");
                    ICLI_PRINTF(" %-32s :: %s \n", "LOCK status", serdes->l_rx_rcpll_lock_status ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %u \n", "RC-Pll range", serdes->l_rx_rcpll_range);
                    ICLI_PRINTF(" %-32s :: %u \n", "VCO Load", serdes->l_rx_rcpll_vco_load);
                    ICLI_PRINTF(" %-32s :: %u \n", "FSM status", serdes->l_rx_rcpll_fsm_status);

                    ICLI_PRINTF(">> HOST TX-RCPLL Status\n");
                    ICLI_PRINTF(" %-32s :: %s \n", "LOCK status", serdes->h_tx_rcpll_lock_status ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %u \n", "RC-Pll range", serdes->h_tx_rcpll_range);
                    ICLI_PRINTF(" %-32s :: %u \n", "VCO Load", serdes->h_tx_rcpll_vco_load);
                    ICLI_PRINTF(" %-32s :: %u \n", "FSM status", serdes->h_tx_rcpll_fsm_status);
                    ICLI_PRINTF(">> LINE TX-RCPLL Status\n");
                    ICLI_PRINTF(" %-32s :: %s \n", "LOCK status", serdes->l_tx_rcpll_lock_status ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %u \n", "RC-Pll range", serdes->l_tx_rcpll_range);
                    ICLI_PRINTF(" %-32s :: %u \n", "VCO Load", serdes->l_tx_rcpll_vco_load);
                    ICLI_PRINTF(" %-32s :: %u \n", "FSM status", serdes->l_tx_rcpll_fsm_status);

                    ICLI_PRINTF(">> HOST PMA Status\n");
                    ICLI_PRINTF(" %-32s :: %s \n", "Link", serdes->h_pma.rx_link ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %s \n", "RX-fault", serdes->h_pma.rx_fault ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %s \n", "Tx-fault", serdes->h_pma.tx_fault ? "TRUE" : "FALSE");
                    ICLI_PRINTF(">> LINE PMA Status\n");
                    ICLI_PRINTF(" %-32s :: %s \n", "Link", serdes->l_pma.rx_link ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %s \n", "RX-fault", serdes->l_pma.rx_fault ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %s \n", "Tx-fault", serdes->l_pma.tx_fault ? "TRUE" : "FALSE");
                    ICLI_PRINTF(">> HOST PCS Status\n");
                    ICLI_PRINTF(" %-32s :: %s \n", "Link", serdes->h_pcs.rx_link ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %s \n", "RX-fault", serdes->h_pcs.rx_fault ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %s \n", "Tx-fault", serdes->h_pcs.tx_fault ? "TRUE" : "FALSE");
                    ICLI_PRINTF(">> LINE PCS Status\n");
                    ICLI_PRINTF(" %-32s :: %s \n", "Link", serdes->l_pcs.rx_link ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %s \n", "RX-fault", serdes->l_pcs.rx_fault ? "TRUE" : "FALSE");
                    ICLI_PRINTF(" %-32s :: %s \n", "Tx-fault", serdes->l_pcs.tx_fault ? "TRUE" : "FALSE");
                    ICLI_PRINTF(">> WIS Status\n");
                    ICLI_PRINTF(" %-32s :: %s \n", "Link", serdes->wis.rx_link ? "TRUE" : "FALSE");

                }
            }
        }
    }
}

void misc_icli_10g_phy_ib_conf(u32 session_id, vtss_phy_10g_ib_conf_t *ib_conf, icli_stack_port_range_t *v_port_type_list, BOOL is_set, BOOL is_host, BOOL has_prbs)
{
    u32                       range_idx, cnt_idx;
    mesa_port_no_t            uport;
    vtss_phy_10g_ib_status_t  ib_status;
    vtss_phy_10g_prbs_mon_conf_t prbs_mon_conf;
    vtss_phy_10g_ib_conf_t ib_get;

    if (v_port_type_list) { //at least one range input
        //Loop through all switches in the stack
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            //Loop through all ports in the switch
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(PHY_INST, uport2iport(uport))) {
                    continue;
                }

                if (is_set == TRUE) {
                    ib_conf->is_host = is_host;
                    vtss_phy_10g_ib_conf_get(PHY_INST, uport2iport(uport), is_host, &ib_get);
                    ib_conf->apc_bit_mask &= ib_get.apc_bit_mask;
                    ib_conf->freeze_bit_mask &= ib_get.freeze_bit_mask;
                    vtss_phy_10g_ib_conf_set(PHY_INST, uport2iport(uport), ib_conf, is_host);
                    if (has_prbs) {
                        prbs_mon_conf.enable = ((ib_conf->prbs == 6) ? FALSE : TRUE);
                        prbs_mon_conf.line = ((ib_conf->is_host == TRUE) ? FALSE : TRUE);
                        /* Setting default values to rest of parameters */
                        prbs_mon_conf.max_bist_frames = 0xffff;
                        prbs_mon_conf.error_states = 3;
                        prbs_mon_conf.des_interface_width = 4;
                        prbs_mon_conf.prbsn_sel = ib_conf->prbs;
                        prbs_mon_conf.no_of_errors = 7;
                        prbs_mon_conf.bist_mode = 3;
                        prbs_mon_conf.prbs_check_input_invert = ib_conf->prbs_inv;
                        if (vtss_phy_10g_prbs_mon_conf(PHY_INST, uport2iport(uport), &prbs_mon_conf) != VTSS_RC_OK) {
                            ICLI_PRINTF("vtss_phy_10g_prbs_mon_conf failed port_no %d", uport2iport(uport));
                        }
                    }
                } else {
                    ib_status.ib_conf.is_host = is_host;
                    vtss_phy_10g_ib_status_get(PHY_INST, uport2iport(uport), &ib_status);
                    ICLI_PRINTF("==================================================================\n");
                    ICLI_PRINTF(" PORT %u    %-4s  IB Parameters                                   \n", uport, is_host ? "HOST" : "LINE");
                    ICLI_PRINTF("==================================================================\n");
                    ICLI_PRINTF("offs    value =%8u min = %8u max = %8u\n", ib_status.ib_conf.offs.value, ib_status.ib_conf.offs.min, ib_status.ib_conf.offs.max);
                    ICLI_PRINTF("gain    value =%8u min = %8u max = %8u\n", ib_status.ib_conf.gain.value, ib_status.ib_conf.gain.min, ib_status.ib_conf.gain.max);
                    ICLI_PRINTF("gainadj value =%8u min = %8u max = %8u\n", ib_status.ib_conf.gainadj.value, ib_status.ib_conf.gainadj.min, ib_status.ib_conf.gainadj.max);
                    ICLI_PRINTF("l       value =%8u min = %8u max = %8u\n", ib_status.ib_conf.l.value, ib_status.ib_conf.l.min, ib_status.ib_conf.l.max);
                    ICLI_PRINTF("c       value =%8u min = %8u max = %8u\n", ib_status.ib_conf.c.value, ib_status.ib_conf.c.min, ib_status.ib_conf.c.max);
                    ICLI_PRINTF("agc     value =%8u min = %8u max = %8u\n", ib_status.ib_conf.agc.value, ib_status.ib_conf.agc.min, ib_status.ib_conf.agc.max);
                    ICLI_PRINTF("dfe1    value =%8u min = %8u max = %8u\n", ib_status.ib_conf.dfe1.value, ib_status.ib_conf.dfe1.min, ib_status.ib_conf.dfe1.max);
                    ICLI_PRINTF("dfe2    value =%8u min = %8u max = %8u\n", ib_status.ib_conf.dfe2.value, ib_status.ib_conf.dfe2.min, ib_status.ib_conf.dfe2.max);
                    ICLI_PRINTF("dfe3    value =%8u min = %8u max = %8u\n", ib_status.ib_conf.dfe3.value, ib_status.ib_conf.dfe3.min, ib_status.ib_conf.dfe3.max);
                    ICLI_PRINTF("dfe4    value =%8u min = %8u max = %8u\n", ib_status.ib_conf.dfe4.value, ib_status.ib_conf.dfe4.min, ib_status.ib_conf.dfe4.max);
                    ICLI_PRINTF("ld  = %u\nprbs %s\napc_bit_mask 0x%04x\nfreeze_bit_mask 0x%04x\n", ib_status.ib_conf.ld, ib_status.ib_conf.prbs ? "TRUE" : "FALSE", ib_status.ib_conf.apc_bit_mask, ib_status.ib_conf.freeze_bit_mask);
                    ICLI_PRINTF("Signal detect %s\n", ib_status.sig_det == TRUE ? "TRUE" : "FALSE");
                }
            }
        }
    }
}

char const *media_type2txt(vtss_phy_10g_media_t media)
{
    switch (media) {
    case VTSS_MEDIA_TYPE_SR :
        return "VTSS_MEDIA_TYPE_SR";
    case VTSS_MEDIA_TYPE_SR2 :
        return "VTSS_MEDIA_TYPE_SR2";
    case VTSS_MEDIA_TYPE_DAC:
        return "VTSS_MEDIA_TYPE_DAC";
    case VTSS_MEDIA_TYPE_ZR :
        return "VTSS_MEDIA_TYPE_ZR";
    case VTSS_MEDIA_TYPE_KR :
        return "VTSS_MEDIA_TYPE_KR";
    case VTSS_MEDIA_TYPE_SR_SC :
        return "VTSS_MEDIA_TYPE_SR_SC";
    case VTSS_MEDIA_TYPE_SR2_SC :
        return "VTSS_MEDIA_TYPE_SR2_SC";
    case VTSS_MEDIA_TYPE_DAC_SC:
        return "VTSS_MEDIA_TYPE_DAC_SC";
    case VTSS_MEDIA_TYPE_ZR_SC :
        return "VTSS_MEDIA_TYPE_ZR_SC";
    case VTSS_MEDIA_TYPE_ZR2_SC :
        return "VTSS_MEDIA_TYPE_ZR2_SC";
    case VTSS_MEDIA_TYPE_KR_SC :
        return "VTSS_MEDIA_TYPE_KR_SC";
    default:
        return "Invalid media type";
    }
}

void misc_icli_10g_phy_pcs_sticky_poll(u32 session_id, icli_stack_port_range_t *v_port_type_list)
{
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    vtss_phy_10g_extnd_event_t ex_events;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                if (vtss_phy_10g_pcs_status_get(misc_phy_inst_get(), uport2iport(uport), &ex_events) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error getting clause37 ability configuration for port %s %u/%u\n",
                                icli_port_type_get_name(v_port_type_list->switch_range[range_idx].port_type),
                                v_port_type_list->switch_range[range_idx].switch_id, uport);
                }

                ICLI_PRINTF(" PCS1G Line sticky set:%s\n", (ex_events & VTSS_PHY_1G_LINE_AUTONEG_RESTART_EV) ? "TRUE" : "FALSE");

            }
        }
    }
}

void misc_icli_10g_phy_clause37_abil(u32 session_id, BOOL has_host, BOOL has_line, BOOL has_aneg_enable,
                                     BOOL has_aneg_disable,  icli_stack_port_range_t *v_port_type_list)
{
    vtss_phy_10g_clause_37_control_t ctrl;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    mesa_rc rc;
    vtss_phy_10g_clause_37_cmn_status_t clause_37_status;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                if (has_aneg_enable || has_aneg_disable) {
                    memset(&ctrl, 0, sizeof(vtss_phy_10g_clause_37_control_t));
                    if (has_aneg_enable) {
                        ctrl.enable = TRUE;
                    }

                    if (has_aneg_disable) {
                        ctrl.enable = FALSE;
                    }

                    ctrl.advertisement.fdx = 1;
                    ctrl.advertisement.symmetric_pause = TRUE;
                    ctrl.advertisement.asymmetric_pause = TRUE;
                    ctrl.advertisement.next_page = TRUE;
                    if (has_line) {
                        ctrl.line = TRUE;
                    }

                    if (has_host) {
                        ctrl.host = TRUE;
                    }

                    if (has_host && has_line) {
                        ctrl.l_h = TRUE;
                    }

                    if (VTSS_RC_OK != (rc = vtss_phy_10g_clause_37_control_set(misc_phy_inst_get(), uport2iport(uport), &ctrl))) {
                        ICLI_PRINTF("Error setting clause37 Abil configuration for port %d (%s)\n", uport, error_txt(rc));
                    }
                } else {
                    if (has_line) {
                        memset(&ctrl, 0, sizeof(vtss_phy_10g_clause_37_control_t));
                        ctrl.line = TRUE;
                        vtss_phy_10g_clause_37_control_get(misc_phy_inst_get(), uport2iport(uport), &ctrl);
                        ICLI_PRINTF(" Line side Autoneg(sw) port %u\n", uport);
                        ICLI_PRINTF(" ==============================\n");
                        ICLI_PRINTF(" aneg enable      :%5s\n", ctrl.enable ? "TRUE" : "FALSE");
                        ICLI_PRINTF(" fdx              :%5s\n", ctrl.advertisement.fdx ? "YES" : "NO");
                        ICLI_PRINTF(" hdx              :%5s\n", ctrl.advertisement.hdx ? "YES" : "NO");
                        ICLI_PRINTF(" symmetric_pause  :%5s\n", ctrl.advertisement.symmetric_pause ? "YES" : "NO");
                        ICLI_PRINTF(" asymmetric_pause :%5s\n", ctrl.advertisement.asymmetric_pause ? "YES" : "NO");
                    }

                    if (has_host) {
                        memset(&ctrl, 0, sizeof(vtss_phy_10g_clause_37_control_t));
                        ctrl.host = TRUE;
                        vtss_phy_10g_clause_37_control_get(misc_phy_inst_get(), uport2iport(uport), &ctrl);
                        ICLI_PRINTF(" Host side Autoneg(sw) port %u\n", uport);
                        ICLI_PRINTF(" ==============================\n");
                        ICLI_PRINTF(" aneg enable      :%5s\n", ctrl.enable ? "TRUE" : "FALSE");
                        ICLI_PRINTF(" fdx              :%5s\n", ctrl.advertisement.fdx ? "YES" : "NO");
                        ICLI_PRINTF(" hdx              :%5s\n", ctrl.advertisement.hdx ? "YES" : "NO");
                        ICLI_PRINTF(" symmetric_pause  :%5s\n", ctrl.advertisement.symmetric_pause ? "YES" : "NO");
                        ICLI_PRINTF(" asymmetric_pause :%5s\n", ctrl.advertisement.asymmetric_pause ? "YES" : "NO");

                    }

                    if (vtss_phy_10g_clause_37_status_get(misc_phy_inst_get(), uport2iport(uport), &clause_37_status) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error getting clause37 ability configuration for port %s %u/%u\n",
                                    icli_port_type_get_name(v_port_type_list->switch_range[range_idx].port_type),
                                    v_port_type_list->switch_range[range_idx].switch_id, uport);
                    }

                    if (has_line) {
                        ICLI_PRINTF(" Line side Autoneg(hw) port %u\n", uport);
                        ICLI_PRINTF(" ===============================\n");
                        ICLI_PRINTF(" aneg complete    :%5s\n", clause_37_status.line.autoneg.complete ? "TRUE" : "FALSE");
                        ICLI_PRINTF(" link_status      :%5s\n **link partner adv abil**\n", clause_37_status.line.link ? "UP" : "DOWN");
                        ICLI_PRINTF(" fdx              :%5s\n", clause_37_status.line.autoneg.partner_advertisement.fdx ? "YES" : "NO");
                        ICLI_PRINTF(" hdx              :%5s\n", clause_37_status.line.autoneg.partner_advertisement.hdx ? "YES" : "NO");
                        ICLI_PRINTF(" symmetric_pause  :%5s\n", clause_37_status.line.autoneg.partner_advertisement.symmetric_pause ? "YES" : "NO");
                        ICLI_PRINTF(" asymmetric_pause :%5s\n", clause_37_status.line.autoneg.partner_advertisement.asymmetric_pause ? "YES" : "NO");
                        ICLI_PRINTF(" remote fault     :%5u(0-link_ok :: 1-offline :: 2-link_failure :: 3-aneg_error) \n", clause_37_status.line.autoneg.partner_advertisement.remote_fault);
                        ICLI_PRINTF(" acknowledge      :%5s\n", clause_37_status.line.autoneg.partner_advertisement.acknowledge ? "YES" : "NO");
                        ICLI_PRINTF(" next_page        :%5s\n", clause_37_status.line.autoneg.partner_advertisement.next_page ? "YES" : "NO");
                    }

                    if (has_host) {
                        ICLI_PRINTF(" Host side Autoneg(hw) port %u\n", uport);
                        ICLI_PRINTF(" ==============================\n");
                        ICLI_PRINTF(" aneg complete    :%5s\n", clause_37_status.host.autoneg.complete ? "TRUE" : "FALSE");
                        ICLI_PRINTF(" link_status      :%5s\ni **link partner adv abil**\n", clause_37_status.host.link ? "UP" : "DOWN");
                        ICLI_PRINTF(" fdx              :%5s\n", clause_37_status.host.autoneg.partner_advertisement.fdx ? "YES" : "NO");
                        ICLI_PRINTF(" hdx              :%5s\n", clause_37_status.host.autoneg.partner_advertisement.hdx ? "YES" : "NO");
                        ICLI_PRINTF(" symmetric_pause  :%5s\n", clause_37_status.host.autoneg.partner_advertisement.symmetric_pause ? "YES" : "NO");
                        ICLI_PRINTF(" asymmetric_pause :%5s\n", clause_37_status.host.autoneg.partner_advertisement.asymmetric_pause ? "YES" : "NO");
                        ICLI_PRINTF(" remote fault     :%5u(0-link_ok :: 1-offline :: 2-link_failure :: 3-aneg_error) \n", clause_37_status.host.autoneg.partner_advertisement.remote_fault);
                        ICLI_PRINTF(" acknowledge      :%5s\n", clause_37_status.host.autoneg.partner_advertisement.acknowledge ? "YES" : "NO");
                        ICLI_PRINTF(" next_page        :%5s\n", clause_37_status.host.autoneg.partner_advertisement.next_page ? "YES" : "NO");
                    }

                }
            }
        }
    }
}

void misc_icli_10g_phy_mode(u32 session_id, BOOL has_mode, u8 mode_conf, BOOL lane_conf, u8 drate, BOOL has_host_intf, u8 host_if,
                            u8 ddr_mode, BOOL has_host, BOOL has_clk_src, BOOL has_href, BOOL has_media_type,
                            u8 media_type, BOOL has_sgmii_pass_thru, BOOL has_enable, BOOL has_amp_tol, BOOL has_high,
                            icli_stack_port_range_t *v_port_type_list, BOOL has_link_6g_distance, BOOL has_long_range, BOOL has_apc_line_ld_ctrl,
                            BOOL has_ld_lev_ini, u8 v_0_0xff, BOOL has_apc_offs_ctrl, BOOL has_eqz_offs_par_cfg, u32 v_0_0xffffffff)
{
    vtss_phy_10g_mode_t       mode;
    vtss_phy_10g_id_t         chip_id;
    u32                       range_idx, cnt_idx;
    mesa_port_no_t            uport;
    mesa_rc                   rc;
    BOOL                      set = FALSE;
    bool                      mepa_cap_phy_ts = false;

    if (v_port_type_list) { //at least one range input
        //Loop through all switches in the stack
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            //Loop through all ports in the switch
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(PHY_INST, uport2iport(uport))) {
                    continue;
                }

                if (VTSS_RC_OK != (rc = vtss_phy_10g_id_get(PHY_INST, uport2iport(uport), &chip_id))) {
                    ICLI_PRINTF("Error in getting the chip_id of port %d (%s)\n", uport, error_txt(rc));
                    continue;
                }

                if (has_sgmii_pass_thru) {
                    set = TRUE;
                    mode.enable_pass_thru = has_enable;
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_sgmii_mode_set(PHY_INST, uport2iport(uport), has_enable))) {
                        ICLI_PRINTF("Error in setting the sgmii mode for port %d (%s)\n", uport, error_txt(rc));
                        continue;
                    }
                }

                if (has_mode || has_host_intf || has_clk_src || has_media_type || has_amp_tol || has_link_6g_distance) {
                    set = TRUE;
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_mode_get(PHY_INST, uport2iport(uport), &mode))) {
                        ICLI_PRINTF("Error in getting the mode of port %d (%s)\n", uport, error_txt(rc));
                        continue;
                    }

                    switch (mode_conf) {
                    case 1 :
                        /* LAN */
                        mode.oper_mode = VTSS_PHY_LAN_MODE;
                        mode.xaui_lane_flip = 0;
                        if (chip_id.family == VTSS_PHY_FAMILY_VENICE) {
                            mode.interface = VTSS_PHY_XAUI_XFI;
                        }

                        break;
                    case 2 :
                        /* WAN */
                        mode.oper_mode = VTSS_PHY_WAN_MODE;
                        mode.xaui_lane_flip = 0;
                        if (chip_id.family == VTSS_PHY_FAMILY_VENICE) {
                            mode.interface = VTSS_PHY_XAUI_XFI;
                        }

                        break;
                    case 3 :
                        /* 1G */
                        mode.oper_mode = VTSS_PHY_1G_MODE;
                        if (lane_conf == 0) {
                            mode.interface = VTSS_PHY_SGMII_LANE_0_XFI;
                        } else {
                            mode.interface = VTSS_PHY_SGMII_LANE_3_XFI;
                        }
                        /* The Serdes XAUI lane 0 must match phy lane 0 */
                        mode.xaui_lane_flip = 1;
                        break;
                    case 4 :
                        /* Repeater */
                        mode.oper_mode = VTSS_PHY_REPEATER_MODE;
                        mode.rate = (vtss_rptr_rate_t)drate;
                        break;
                    default:
                        /* LAN */
                        ICLI_PRINTF("Mode being configured on port %u is %s\n", uport, mode_type2txt(&mode));
                        break;
                    }

                    if (chip_id.family == VTSS_PHY_FAMILY_MALIBU) {
                        mode.interface = VTSS_PHY_SFI_XFI;
                    } else if (has_host_intf == TRUE) {
                        switch (host_if) {
                        case 0:
                            mode.interface = VTSS_PHY_XAUI_XFI;
                            break;
                        case 1:
                            mode.interface = VTSS_PHY_RXAUI_XFI;
                            switch (drate) {
                            case 0:
                                mode.ddr_mode = VTSS_DDR_MODE_A;
                                break;
                            case 1:
                                mode.ddr_mode = VTSS_DDR_MODE_K;
                                break;
                            case 2:
                                mode.ddr_mode = VTSS_DDR_MODE_M;
                                break;
                            default:
                                mode.ddr_mode = VTSS_DDR_MODE_A;
                                break;
                            }

                            break;
                        default:
                            mode.interface = VTSS_PHY_XAUI_XFI;
                            break;
                        }
                    }

                    if (has_clk_src) {
                        if (has_host) {
                            if (has_href) {
                                if (mode.lref_for_host != FALSE ) {
                                    mode.is_init =  TRUE;
                                }

                                mode.lref_for_host = FALSE;
                                if (has_amp_tol) {
                                    mode.h_clk_src.is_high_amp = mode.l_clk_src.is_high_amp = has_high;
                                } else {
                                    mode.h_clk_src.is_high_amp = mode.l_clk_src.is_high_amp = TRUE;
                                }
                            } else {
                                if (mode.lref_for_host != TRUE ) {
                                    mode.is_init =  TRUE;
                                }

                                mode.lref_for_host = TRUE;
                                if (has_amp_tol) {
                                    mode.h_clk_src.is_high_amp = mode.l_clk_src.is_high_amp = has_high;
                                } else {
                                    mode.h_clk_src.is_high_amp = mode.l_clk_src.is_high_amp = FALSE;
                                }
                            }
                        } else {
                            if (!has_href) {
                                if (has_amp_tol) {
                                    mode.h_clk_src.is_high_amp = mode.l_clk_src.is_high_amp = has_high;
                                } else {
                                    mode.h_clk_src.is_high_amp = mode.l_clk_src.is_high_amp = TRUE;
                                }
                            }
                        }
                    }

                    if (has_media_type) {
                        if (has_host) {
                            ICLI_PRINTF(" host media_type:%d\n", media_type);
                            switch (media_type) {
                            case 0:
                                mode.h_media = VTSS_MEDIA_TYPE_SR;
                                break;
                            case 1:
                                mode.h_media = VTSS_MEDIA_TYPE_SR2;
                                break;
                            case 2:
                                mode.h_media = VTSS_MEDIA_TYPE_DAC;
                                break;
                            case 3:
                                mode.h_media = VTSS_MEDIA_TYPE_ZR;
                                break;
                            case 4:
                                mode.h_media = VTSS_MEDIA_TYPE_KR;
                                break;
                            case 5:
                                mode.h_media = VTSS_MEDIA_TYPE_SR_SC;
                                break;
                            case 6:
                                mode.h_media = VTSS_MEDIA_TYPE_SR2_SC;
                                break;
                            case 7:
                                mode.h_media = VTSS_MEDIA_TYPE_DAC_SC;
                                break;
                            case 8:
                                mode.h_media = VTSS_MEDIA_TYPE_ZR_SC;
                                break;
                            case 9:
                                mode.h_media = VTSS_MEDIA_TYPE_ZR2_SC;
                                break;
                            case 10:
                                mode.h_media = VTSS_MEDIA_TYPE_KR_SC;
                                break;
                            default:
                                mode.h_media = VTSS_MEDIA_TYPE_SR;
                                break;
                            }
                        } else {
                            ICLI_PRINTF(" Line media_type:%d\n", media_type);
                            switch (media_type) {
                            case 0:
                                mode.l_media = VTSS_MEDIA_TYPE_SR;
                                break;
                            case 1:
                                mode.l_media = VTSS_MEDIA_TYPE_SR2;
                                break;
                            case 2:
                                mode.l_media = VTSS_MEDIA_TYPE_DAC;
                                break;
                            case 3:
                                mode.l_media = VTSS_MEDIA_TYPE_ZR;
                                break;
                            case 4:
                                mode.l_media = VTSS_MEDIA_TYPE_KR;
                                break;
                            case 5:
                                mode.l_media = VTSS_MEDIA_TYPE_SR_SC;
                                break;
                            case 6:
                                mode.l_media = VTSS_MEDIA_TYPE_SR2_SC;
                                break;
                            case 7:
                                mode.l_media = VTSS_MEDIA_TYPE_DAC_SC;
                                break;
                            case 8:
                                mode.l_media = VTSS_MEDIA_TYPE_ZR_SC;
                                break;
                            case 9:
                                mode.l_media = VTSS_MEDIA_TYPE_ZR2_SC;
                                break;
                            case 10:
                                mode.l_media = VTSS_MEDIA_TYPE_KR_SC;
                                break;

                            default:
                                mode.l_media = VTSS_MEDIA_TYPE_SR;
                                break;
                            }
                        }
                    }

                    if (has_link_6g_distance) {
                        if (has_long_range) {
                            mode.link_6g_distance = VTSS_6G_LINK_LONG_RANGE;
                        } else {
                            mode.link_6g_distance = VTSS_6G_LINK_SHORT_RANGE;
                        }
                    }

                    if (has_apc_line_ld_ctrl) {
                        if (has_ld_lev_ini) {
                            ICLI_PRINTF("Setting ld_lev_ini:%d\n", v_0_0xff);
                            mode.serdes_conf.apc_line_ld_ctrl = TRUE;
                            mode.serdes_conf.apc_line_eqz_ld_ctrl = v_0_0xff;
                        }
                    }

                    if (has_apc_offs_ctrl) {
                        if (has_eqz_offs_par_cfg) {
                            ICLI_PRINTF("Setting eqz_offs_par_cfg:%d\n", v_0_0xffffffff);
                            mode.serdes_conf.apc_offs_ctrl = TRUE;
                            mode.serdes_conf.apc_eqz_offs_par_cfg = v_0_0xffffffff;
                        }
                    }

                    if (VTSS_RC_OK != (rc = vtss_phy_10g_mode_set(PHY_INST, uport2iport(uport), &mode))) {
                        ICLI_PRINTF("Error in setting the mode for port %d (%s)\n", uport, error_txt(rc));
                        continue;
                    }
#ifdef VTSS_SW_OPTION_TOD
                    mepa_cap_phy_ts = mepa_phy_ts_cap();
#endif //VTSS_SW_OPTION_TOD
                    if (mepa_cap_phy_ts) {
                        if (chip_id.part_number == 0x8488 || chip_id.part_number == 0x8487 || chip_id.family == VTSS_PHY_FAMILY_MALIBU || chip_id.family == VTSS_PHY_FAMILY_VENICE) {
                            if (mode.oper_mode != VTSS_PHY_REPEATER_MODE) {
                                if (VTSS_RC_OK != (rc = vtss_phy_ts_phy_oper_mode_change(PHY_INST, uport2iport(uport)))) {
                                    ICLI_PRINTF("Error in changing the mode for %d (%s)\n", uport, error_txt(rc));
                                    continue;
                                }
                            }
                        }
                    }
                }

                if (set == FALSE) {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_mode_get(PHY_INST, uport2iport(uport), &mode))) {
                        ICLI_PRINTF("Error in getting the mode for port %d (%s)\n", uport, error_txt(rc));
                        continue;
                    }
                    // the internal api call still shows XAUI interface here this is just a temporary fix.
                    if (chip_id.part_number == 0x8256 || chip_id.part_number == 0x8257 || chip_id.part_number == 0x8258) {
                        mode.interface = VTSS_PHY_SFI_XFI;
                    }

                    ICLI_PRINTF("Port : %u, Mode : %s, Wrefclk : %s, Interface : %s\n", uport, mode_type2txt(&mode),
                                mode.oper_mode == VTSS_PHY_WAN_MODE ? (mode.wrefclk == VTSS_WREFCLK_155_52 ? "155.02" : "622.08") : "-",
                                mode.oper_mode == VTSS_PHY_1G_MODE ? "-" : interface_type2txt(&mode));
                    ICLI_PRINTF("Host media type %s\n", media_type2txt(mode.h_media));
                    ICLI_PRINTF("Line media type %s\n", media_type2txt(mode.l_media));
                    ICLI_PRINTF("Host Pll clock source %s\n", mode.lref_for_host ? "LREF" : "HREF");
                    ICLI_PRINTF("Host clock amplitude tolerance %s\n", mode.h_clk_src.is_high_amp ? "HIGH" : "LOW");
                    ICLI_PRINTF("Line clock amplitude tolerance %s\n", mode.l_clk_src.is_high_amp ? "HIGH" : "LOW");
                }
            }
        }
    }
}

BOOL misc_icli_10g_phy_fw_status(u32 session_id, icli_stack_port_range_t *plist)
{

    vtss_phy_10g_fw_status_t    status;
    u32                         range_idx = 0, cnt_idx = 0;
    mesa_port_no_t              uport;
    vtss_phy_10g_id_t           chip_id;
    BOOL                        first = TRUE;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (vtss_phy_10g_id_get(misc_phy_inst_get(), uport2iport(uport), &chip_id) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error getting chip id for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                    continue;
                }

                if ((chip_id.part_number != 0x8488) && (chip_id.part_number != 0x8487) && (chip_id.part_number != 0x8484)) {
                    ICLI_PRINTF("%% Error: Not supported on this chip\n");
                    continue;
                }

                if (chip_id.channel_id != 0) {
                    ICLI_PRINTF("%% Error: This command is only supported through a port which represent Phy channel 0\n");
                    continue;
                }

                if (vtss_phy_10g_edc_fw_status_get(misc_phy_inst_get(), uport2iport(uport), &status) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error getting firmware status for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                } else {
                    if (first) {
                        ICLI_PRINTF("%-10s %-10s %-10s %-16s %-10s %-10s %-10s\n", "CLI Port", "API Port", "Channel", "Loaded via API", "FW-Rev", "Chksum", "CPU activity");
                        ICLI_PRINTF("%-10s %-10s %-10s %-16s %-10s %-10s %-10s\n", "--------", "--------", "-------", "-------------", "------", "------", "-----------");
                        first = FALSE;
                    }

                    ICLI_PRINTF("%-10u %-10u %-10u %-16s %-10x %-10s %-10s\n", uport, uport - 1,
                                chip_id.channel_id, status.edc_fw_api_load ? "Yes" : "No",
                                status.edc_fw_rev, status.edc_fw_chksum ? "Pass" : "Fail", status.icpu_activity ? "Yes" : "No");
                }
            } //end of cnt_idx loop
        } // end of range_idx loop
        return TRUE;
    } else { // end of if plist
        ICLI_PRINTF("%% Error Null Port List\n");
        return FALSE;
    }
}

BOOL misc_icli_10g_phy_reset(u32 session_id, icli_stack_port_range_t *plist)
{
    u32                              range_idx = 0, cnt_idx = 0;
    mesa_port_no_t                   uport = 0;
    vtss_phy_10g_mode_t              oper_mode;

    /* Configure the 10G phy operating mode */
    memset(&oper_mode, 0, sizeof(oper_mode));

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (vtss_phy_10g_reset(misc_phy_inst_get(), uport2iport(uport)) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error in Performing phy Reset for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                    continue;
                } else {
                    ICLI_PRINTF("%% PHY Reset is done for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                }

                if (vtss_phy_10g_mode_set(0, uport2iport(uport), &oper_mode) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error in setting mode for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                    continue;
                }
            } //end of cnt_idx for loop
        } // end of range_idx for loop
        return TRUE;
    } else { // end of if plist
        ICLI_PRINTF("%% Error Null Port List\n");
        return FALSE;
    }
}

BOOL misc_icli_10g_phy_failover(u32 session_id, BOOL has_a, BOOL has_b, BOOL has_c, BOOL has_d,
                                BOOL has_e, BOOL has_f, BOOL has_get,
                                icli_stack_port_range_t *plist)
{
    vtss_phy_10g_failover_mode_t     mode;
    u32                              range_idx = 0, cnt_idx = 0;
    mesa_port_no_t                   uport = 0;
    BOOL                             first = TRUE;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (has_get) {
                    if (vtss_phy_10g_failover_get(misc_phy_inst_get(), uport2iport(uport), &mode) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in getting failover configuration for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport);
                        continue;
                    }

                    if (first) {
                        ICLI_PRINTF("%-12s %-12s\n", "Port", "Failover");
                        ICLI_PRINTF("%-12s %-12s\n", "----", "--------");
                        first = FALSE;
                    }

                    ICLI_PRINTF("%-12u %-12s\n", uport, mode == VTSS_PHY_10G_PMA_TO_FROM_XAUI_NORMAL ? "a" :
                                mode == VTSS_PHY_10G_PMA_TO_FROM_XAUI_CROSSED ? "b" :
                                mode == VTSS_PHY_10G_PMA_0_TO_FROM_XAUI_0_TO_XAUI_1 ? "c" :
                                mode == VTSS_PHY_10G_PMA_0_TO_FROM_XAUI_1_TO_XAUI_0 ? "d" :
                                mode == VTSS_PHY_10G_PMA_1_TO_FROM_XAUI_0_TO_XAUI_1 ? "e" :
                                mode == VTSS_PHY_10G_PMA_1_TO_FROM_XAUI_1_TO_XAUI_0 ? "f" : "dunno");
                } else {
                    mode = has_a ? VTSS_PHY_10G_PMA_TO_FROM_XAUI_NORMAL :
                           has_b ? VTSS_PHY_10G_PMA_TO_FROM_XAUI_CROSSED :
                           has_c ? VTSS_PHY_10G_PMA_0_TO_FROM_XAUI_0_TO_XAUI_1 :
                           has_d ? VTSS_PHY_10G_PMA_0_TO_FROM_XAUI_1_TO_XAUI_0 :
                           has_e ? VTSS_PHY_10G_PMA_1_TO_FROM_XAUI_0_TO_XAUI_1 :
                           has_f ? VTSS_PHY_10G_PMA_1_TO_FROM_XAUI_1_TO_XAUI_0 :
                           VTSS_PHY_10G_PMA_TO_FROM_XAUI_NORMAL;

                    if (vtss_phy_10g_failover_set(misc_phy_inst_get(), uport2iport(uport), &mode) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in setting failover configuration for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport);
                        continue;
                    }
                }
            } //end of for loop
        } // end of for loop
        return TRUE;
    } else { // end of if plist
        ICLI_PRINTF("%% Error Null Port List\n");
        return FALSE;
    }
}

BOOL misc_icli_10g_phy_power(u32 session_id, BOOL has_enable, BOOL has_no,
                             BOOL has_show, icli_stack_port_range_t *plist)
{
    u32                             range_idx = 0, cnt_idx = 0;
    mesa_port_no_t                  uport = 0;
    vtss_phy_10g_power_t            power = VTSS_PHY_10G_POWER_ENABLE;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (has_enable || has_no) {
                    if (has_enable) {
                        power = VTSS_PHY_10G_POWER_ENABLE;
                    } else if (has_no) {
                        power = VTSS_PHY_10G_POWER_DISABLE;
                    }

                    if (vtss_phy_10g_power_set(misc_phy_inst_get(), uport2iport(uport), &power) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error Setting Power configuration for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport);
                        continue;
                    }
                } else if (has_show) {
                    if (vtss_phy_10g_power_get(misc_phy_inst_get(), uport2iport(uport), &power) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error getting Power status for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport);
                    }

                    ICLI_PRINTF("Port : %u, Power : %s\n", uport, power == VTSS_PHY_10G_POWER_ENABLE ? "Enabled" : "Disabled");
                }
            } //End of for loop for ports
        } //End of for loop for switches
        return TRUE;
    } else {
        ICLI_PRINTF("%% Error NULL Port List");
        return FALSE;
    }
}

BOOL misc_icli_10g_phy_status(u32 session_id, icli_stack_port_range_t *plist)
{
    vtss_phy_10g_status_t   status;
    vtss_phy_10g_mode_t     mode;
    u32                     range_idx, cnt_idx;
    mesa_port_no_t          uport;
    vtss_phy_10g_cnt_t      cnt;
    BOOL                    first = TRUE;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (vtss_phy_10g_status_get(misc_phy_inst_get(), uport2iport(uport), &status) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error getting phy status for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                    continue;
                }

                if (vtss_phy_10g_mode_get(misc_phy_inst_get(), uport2iport(uport), &mode) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error getting phy mode for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                    continue;
                }

                ICLI_PRINTF("Port: %u\n", uport);
                ICLI_PRINTF("--------\n");
                if (first) {
                    ICLI_PRINTF("%-12s %-12s %-16s %-12s %-12s\n", "", "Link", "Link-down-event", "Rx-Fault-Sticky", "Tx-Fault-Sticky");
                    first = 0;
                }

                ICLI_PRINTF("%-12s %-12s %-16s %-12s %-12s\n", "Line PMA:", status.pma.rx_link ? "Yes" : "No",
                            status.pma.link_down ? "Yes" : "No", status.pma.rx_fault ? "Yes" : "No", status.pma.tx_fault ? "Yes" : "No");
                ICLI_PRINTF("%-12s %-12s %-16s %-12s %-12s\n", "Host PMA:", status.hpma.rx_link ? "Yes" : "No",
                            status.hpma.link_down ? "Yes" : "No", status.hpma.rx_fault ? "Yes" : "No", status.hpma.tx_fault ? "Yes" : "No");
                ICLI_PRINTF("%-12s %-12s %-16s %-12s %-12s\n", "WIS:", mode.oper_mode == VTSS_PHY_WAN_MODE ? status.wis.rx_link ? "Yes" : "No" : "-",
                            mode.oper_mode == VTSS_PHY_WAN_MODE ? status.wis.link_down ? "Yes" : "No" : "-",
                            mode.oper_mode == VTSS_PHY_WAN_MODE ? status.wis.rx_fault ? "Yes" : "No" : "-",
                            mode.oper_mode == VTSS_PHY_WAN_MODE ? status.wis.tx_fault ? "Yes" : "No" : "-");
                ICLI_PRINTF("%-12s %-12s %-16s %-12s %-12s\n", "Line PCS:", status.pcs.rx_link ? "Yes" : "No",
                            status.pcs.link_down ? "Yes" : "No", status.pcs.rx_fault ? "Yes" : "No", status.pcs.tx_fault ? "Yes" : "No");
                ICLI_PRINTF("%-12s %-12s %-16s %-12s %-12s\n", "Host PCS:", status.hpcs.rx_link ? "Yes" : "No",
                            status.hpcs.link_down ? "Yes" : "No", status.hpcs.rx_fault ? "Yes" : "No", status.hpcs.tx_fault ? "Yes" : "No");
                ICLI_PRINTF("%-12s %-12s %-16s %-12s %-12s\n", "XAUI Status:", status.xs.rx_link ? "Yes" : "No",
                            status.xs.link_down ? "Yes" : "No", status.xs.rx_fault ? "Yes" : "No", status.xs.tx_fault ? "Yes" : "No");
                ICLI_PRINTF("%-12s %-12s \n", "Line 1G PCS:", status.lpcs_1g ? "Yes" : "No");
                ICLI_PRINTF("%-12s %-12s \n", "Host 1G PCS:", status.hpcs_1g ? "Yes" : "No");
                ICLI_PRINTF("%-12s %-12s \n", "Link UP:", status.status ? "Yes" : "No");
                ICLI_PRINTF("%-12s %-12s \n", "Block lock:", status.block_lock ? "Yes" : "No");
                ICLI_PRINTF("%-12s %-12s \n", "LOPC status:", status.lopc_stat ? "Yes" : "No");

                if (vtss_phy_10g_cnt_get(misc_phy_inst_get(), uport2iport(uport), &cnt) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error getting counters for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                    continue;
                }

                ICLI_PRINTF("\n");
                ICLI_PRINTF("PCS counters:\n");
                ICLI_PRINTF("%-20s %-12s\n", "  Block_lacthed:", cnt.pcs.block_lock_latched ? "Yes" : "No");
                ICLI_PRINTF("%-20s %-12s\n", "  High_ber_latched:", cnt.pcs.high_ber_latched ? "Yes" : "No");
                ICLI_PRINTF("%-20s %-12d\n", "  Ber_cnt:", cnt.pcs.ber_cnt);
                ICLI_PRINTF("%-20s %-12d\n", "  Err_blk_cnt:", cnt.pcs.err_blk_cnt);

                vtss_phy_10g_debug_register_dump(misc_phy_inst_get(), (mesa_debug_printf_t)printf, FALSE, uport2iport(uport));
            } // end of cnt_idx
        } // end of range_idx
        return TRUE;
    } else {
        ICLI_PRINTF("%% Error NULL Port List");
        return FALSE;
    }
}

void misc_icli_phy_10g_jitter_conf(u32 session_id, BOOL set, BOOL has_host, u8 levn_val, BOOL has_yes, u8 vtail_val, icli_stack_port_range_t *plist)
{
    u32                        range_idx = 0, cnt_idx = 0;
    mesa_port_no_t             uport;
    vtss_phy_10g_jitter_conf_t jitter_conf;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (set) {
                    jitter_conf.levn = levn_val;
                    jitter_conf.incr_levn = has_yes;
                    jitter_conf.vtail = vtail_val;
                    vtss_phy_10g_jitter_conf_set (misc_phy_inst_get(), uport2iport(uport), &jitter_conf, has_host);
                } else {
                    vtss_phy_10g_jitter_status_get (misc_phy_inst_get(), uport2iport(uport), &jitter_conf, has_host);
                    ICLI_PRINTF("Port: %u\n", uport);
                    ICLI_PRINTF("--------\n");
                    ICLI_PRINTF("levn\t\t :: 0x%0x\n", jitter_conf.levn);
                    ICLI_PRINTF("incr_levn\t\t :: %s\n", jitter_conf.incr_levn ? "YES" : "NO");
                    ICLI_PRINTF("vtail\t\t :: 0x%0x\n", jitter_conf.vtail);
                }
            }
        }
    }
}

BOOL misc_icli_10g_phy_gpio(u32 session_id, icli_stack_port_range_t *plist, BOOL has_mode_output,
                            BOOL has_mode_input, BOOL has_mode_alternative, BOOL has_gpio_get,
                            BOOL has_gpio_set, BOOL value, u8 gpio_no)
{
    u32                        range_idx = 0, cnt_idx = 0;
    mesa_port_no_t             uport;
    vtss_gpio_10g_gpio_mode_t  gpio_mode;
    BOOL                       first = TRUE;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                gpio_mode.port = uport2iport(uport);
                if (has_mode_output) {
                    gpio_mode.mode = VTSS_10G_PHY_GPIO_OUT;
                    if (vtss_phy_10g_gpio_mode_set(misc_phy_inst_get(), uport2iport(uport), gpio_no, &gpio_mode) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in performing GPIO operation for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport2iport(uport));
                        continue;
                    }
                }

                if (has_mode_input) {
                    gpio_mode.mode = VTSS_10G_PHY_GPIO_IN;
                    if (vtss_phy_10g_gpio_mode_set(misc_phy_inst_get(), uport2iport(uport), gpio_no, &gpio_mode) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in performing GPIO operation for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport2iport(uport));
                        continue;
                    }
                }

                if (has_mode_alternative) {
                    gpio_mode.mode = VTSS_10G_PHY_GPIO_PCS_RX_FAULT;
                    if (vtss_phy_10g_gpio_mode_set(misc_phy_inst_get(), uport2iport(uport), gpio_no, &gpio_mode) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in performing GPIO operation for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport2iport(uport));
                        continue;
                    }
                }

                if (has_gpio_get) {
                    if (vtss_phy_10g_gpio_read(misc_phy_inst_get(), uport2iport(uport), gpio_no, &value) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in performing GPIO operation for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport2iport(uport));
                        continue;
                    }

                    if (first) {
                        ICLI_PRINTF("Port  GPIO  Value\n");
                        first = FALSE;
                    }

                    ICLI_PRINTF("%-4d  %-4u  %u\n", uport, gpio_no, value ? 1 : 0);
                }

                if (has_gpio_set) {
                    if (vtss_phy_10g_gpio_write(misc_phy_inst_get(), uport2iport(uport), gpio_no, value) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in performing GPIO operation for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport2iport(uport));
                        continue;
                    }
                }
            } // end of cnt_idx
        } // end of range_idx
        return TRUE;
    } else {
        ICLI_PRINTF("%% Error NULL Port List");
        return FALSE;
    }
}

BOOL misc_icli_10g_phy_rd_wr_i2c(u32 session_id, icli_stack_port_range_t *plist,
                                 u8  address, u16 address_1, BOOL has_read_i2c,
                                 BOOL has_write_i2c, u8 val)
{
    u32                          range_idx = 0, cnt_idx = 0;
    mesa_port_no_t               uport;
    u8                           value = 0;
    mesa_rc                      rc = VTSS_RC_OK;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error in setting GPIO pins for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport2iport(uport));
                    continue;
                }

                if (has_read_i2c) {
                    if (vtss_phy_10g_i2c_read(misc_phy_inst_get(), uport2iport(uport), address, &value) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error reading from i2c bus for port %s %u/%u,address %x\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport2iport(uport), address);
                        continue;
                    }

                    ICLI_PRINTF("Port: %d Value at address %x :: 0x%x\n", uport2iport(uport), address, value);
                }

                if (has_write_i2c) {
                    if (vtss_phy_10g_i2c_write(misc_phy_inst_get(), uport2iport(uport), address, &val) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error writing to i2c bus for port %s %u/%u,address %x\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport2iport(uport), address);
                        continue;
                    }
                }
            } // end of cnt_idx
        } // end of range_idx
        return TRUE;
    } else {
        ICLI_PRINTF("%% Error NULL Port List");
        return FALSE;
    }
}

static const char *rcvrdclk_type2txt(vtss_recvrd_clkout_t r)
{
    switch (r) {
    case VTSS_RECVRD_CLKOUT_DISABLE:
        return "Disable";
    case VTSS_RECVRD_CLKOUT_LINE_SIDE_RX_CLOCK:
        return "Line Side Rx Clock";
    case VTSS_RECVRD_CLKOUT_LINE_SIDE_TX_CLOCK:
        return "Line Side Tx Clock";
    }

    return "INVALID";
}

void misc_icli_10g_phy_sckout(u32 session_id, BOOL has_enable, BOOL has_disable, BOOL has_frequency,
                              vtss_phy_10g_sckout_freq_t freq, BOOL has_use_squelch_src_as_is,
                              BOOL has_invert_squelch_src, BOOL has_Squelch_src,
                              vtss_phy_10g_squelch_src_t Squelch_src, BOOL has_sckout_clkout_sel,
                              vtss_phy_10g_clk_sel_t sckout_clkout_sel,
                              icli_stack_port_range_t *v_port_type_list)
{
    vtss_phy_10g_sckout_conf_t sckout;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    mesa_rc rc;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                ICLI_PRINTF("has_enable:%d has_disable:%d has_freq:%d freq:%d has_use_squelch_src_as_is:%d has_invert_squelch_src:%d has_Squelch_src:%d Squelch_src:%d has_sckout_clkout_sel:%d sckout_clkout_sel:%d \n", has_enable,
                            has_disable, has_frequency, freq, has_use_squelch_src_as_is, has_invert_squelch_src,
                            has_Squelch_src, Squelch_src,
                            has_sckout_clkout_sel, sckout_clkout_sel);

                sckout.enable = (has_enable ? 1 : 0);
                sckout.squelch_inv = (has_invert_squelch_src ? 1 : 0);
                sckout.src = Squelch_src;
                sckout.mode = sckout_clkout_sel;
                sckout.freq = freq;
                ICLI_PRINTF("Port%d sckout%d sckout_enable:%s sckout_freq:%s sckout.squelch_inv:%s sckout_squelch_src:%d sckout_mode:%d\n", uport,
                            uport,
                            sckout.enable ? "enable" : "disable",
                            sckout.freq ? "125MHz" : "156.25MHz",
                            sckout.squelch_inv ? "Invert Squelch SRC" : "Use_as_is",
                            sckout.src,
                            sckout.mode);
                if (has_disable || has_enable || has_use_squelch_src_as_is ||
                    has_invert_squelch_src ||
                    has_sckout_clkout_sel || has_Squelch_src) {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_sckout_conf_set(misc_phy_inst_get(), uport2iport(uport), &sckout))) {
                        ICLI_PRINTF("Malibu Error setting ckout configuration for port %d (%s)\n", uport, error_txt(rc));
                    }
                }
            }
        }
    }
}

void misc_icli_10g_phy_ckout(u32 session_id, BOOL has_enable, BOOL has_disable, BOOL has_use_squelch_src_as_is,
                             BOOL has_invert_squelch_src, BOOL has_Squelch_src,
                             vtss_phy_10g_squelch_src_t Squelch_src, BOOL has_full_rate_clk,
                             BOOL has_div_by_2_clk, BOOL has_clk_out_src,
                             vtss_ckout_data_sel_t clk_out_src,
                             icli_stack_port_range_t *v_port_type_list)
{
    vtss_phy_10g_ckout_conf_t ckout;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    mesa_rc rc;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                ICLI_PRINTF("has_enable:%d has_disable:%d has_use_squelch_src_as_is:%d has_invert_squelch_src:%d has_Squelch_src:%d Squelch_src:%d has_full_rate_clk:%d has_div_by_2_clk:%d has_clk_out_src:%d clk_out_src:%d\n", has_enable,
                            has_disable, has_use_squelch_src_as_is, has_invert_squelch_src, has_Squelch_src, Squelch_src,
                            has_full_rate_clk, has_div_by_2_clk, has_clk_out_src, clk_out_src);

                ckout.enable = (has_enable ? 1 : 0);
                ckout.squelch_inv = (has_invert_squelch_src ? 1 : 0);
                ckout.freq = has_full_rate_clk ? VTSS_PHY_10G_CLK_FULL_RATE : VTSS_PHY_10G_CLK_DIVIDE_BY_2;
                ckout.src = Squelch_src;
                ckout.mode = clk_out_src;
                ckout.ckout_sel = VTSS_CKOUT0;
                ICLI_PRINTF("Port%d ckout%d ckout_enable:%s ckout.squelch_inv:%s ckout_squelch_src:%d ckout_mode:%d\n", uport,
                            uport,
                            ckout.enable ? "enable" : "disable",
                            ckout.squelch_inv ? "Invert Squelch SRC" : "Use_as_is",
                            ckout.src,
                            ckout.mode);
                if (has_disable || has_enable || has_use_squelch_src_as_is ||
                    has_invert_squelch_src || has_full_rate_clk || has_div_by_2_clk ||
                    has_clk_out_src || has_Squelch_src) {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_ckout_conf_set(misc_phy_inst_get(), uport2iport(uport), &ckout))) {
                        ICLI_PRINTF("Malibu Error setting ckout configuration for port %d (%s)\n", uport, error_txt(rc));
                    }

                    ckout.ckout_sel = VTSS_CKOUT1;
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_ckout_conf_set(misc_phy_inst_get(), uport2iport(uport), &ckout))) {
                        ICLI_PRINTF("Malibu Error setting ckout configuration for port %d (%s)\n", uport, error_txt(rc));
                    }

                    ckout.ckout_sel = VTSS_CKOUT2;
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_ckout_conf_set(misc_phy_inst_get(), uport2iport(uport), &ckout))) {
                        ICLI_PRINTF("Malibu Error setting ckout configuration for port %d (%s)\n", uport, error_txt(rc));
                    }

                    ckout.ckout_sel = VTSS_CKOUT3;
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_ckout_conf_set(misc_phy_inst_get(), uport2iport(uport), &ckout))) {
                        ICLI_PRINTF("Malibu Error setting ckout configuration for port %d (%s)\n", uport, error_txt(rc));
                    }
                }
            }
        }
    }
}

void misc_icli_10g_phy_clk_sel(u32 session_id, BOOL has_line, BOOL has_host, BOOL has_clk_sel_no,
                               u8 clk_sel_no, BOOL has_clk_sel_val,
                               vtss_phy_10g_clk_sel_t clk_sel_val,
                               icli_stack_port_range_t *v_port_type_list)
{
    vtss_phy_10g_host_clk_conf_t host_clk;
    vtss_phy_10g_line_clk_conf_t line_clk;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    mesa_rc rc;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                ICLI_PRINTF("has_line:%d has_host:%d has_clk_sel_no:%d clk_sel_no:%d has_clk_sel_val:%d clk_sel_val:%d \n", has_line,
                            has_host, has_clk_sel_no, clk_sel_no,
                            has_clk_sel_val, clk_sel_val);

                if (has_line) {
                    line_clk.clk_sel_no = has_clk_sel_no ? clk_sel_no : 0;
                    line_clk.mode = has_clk_sel_val ? clk_sel_val : VTSS_PHY_10G_SYNC_DISABLE;
                    ICLI_PRINTF("Port%d Line%d line_clk_sel_no%d clk_sel_mode:%d\n",
                                uport, has_line,
                                line_clk.clk_sel_no,
                                line_clk.mode);
                } else if (has_host) {
                    host_clk.clk_sel_no = has_clk_sel_no ? clk_sel_no : 0;
                    host_clk.mode = has_clk_sel_val ? clk_sel_val : VTSS_PHY_10G_SYNC_DISABLE;
                    ICLI_PRINTF("Port%d Host%d host_clk_sel_no%d clk_sel_mode:%d\n",
                                uport, has_host,
                                host_clk.clk_sel_no,
                                host_clk.mode);
                } else {
                    line_clk.clk_sel_no = has_clk_sel_no ? clk_sel_no : 0;
                    line_clk.mode = has_clk_sel_val ? clk_sel_val : VTSS_PHY_10G_SYNC_DISABLE;
                    host_clk.clk_sel_no = has_clk_sel_no ? clk_sel_no : 0;
                    host_clk.mode = has_clk_sel_val ? clk_sel_val : VTSS_PHY_10G_SYNC_DISABLE;
                    ICLI_PRINTF("Port%d Line%d line_clk_sel_no%d clk_sel_mode:%d\n",
                                uport, has_line,
                                line_clk.clk_sel_no,
                                line_clk.mode);
                    ICLI_PRINTF("Port%d Host%d host_clk_sel_no%d clk_sel_mode:%d\n",
                                uport, has_host,
                                host_clk.clk_sel_no,
                                host_clk.mode);
                }

                if (has_line) {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_line_clk_conf_set(misc_phy_inst_get(), uport2iport(uport), &line_clk))) {
                        ICLI_PRINTF("Malibu Error setting line_clk_sel for port %d (%s)\n", uport, error_txt(rc));
                    }
                } else if (has_host) {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_host_clk_conf_set(misc_phy_inst_get(), uport2iport(uport), &host_clk))) {
                        ICLI_PRINTF("Malibu Error setting host_clk_sel for port %d (%s)\n", uport, error_txt(rc));
                    }
                } else {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_line_clk_conf_set(misc_phy_inst_get(), uport2iport(uport), &line_clk))) {
                        ICLI_PRINTF("Malibu Error setting line_clk_sel for port %d (%s)\n", uport, error_txt(rc));
                    }

                    if (VTSS_RC_OK != (rc = vtss_phy_10g_host_clk_conf_set(misc_phy_inst_get(), uport2iport(uport), &host_clk))) {
                        ICLI_PRINTF("Malibu Error setting host_clk_sel for port %d (%s)\n", uport, error_txt(rc));
                    }

                }
            }
        }
    }
}

void misc_icli_10g_phy_rxckout(u32 session_id, BOOL has_disable, BOOL has_rx_clock, BOOL has_tx_clock,
                               BOOL has_pcs_fault_squelch, BOOL has_no_pcs_fault_squelch,
                               BOOL has_lopc_squelch, BOOL has_no_lopc_squelch,
                               icli_stack_port_range_t *v_port_type_list)
{
    vtss_phy_10g_rxckout_conf_t ckout;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    mesa_rc rc;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                if (VTSS_RC_OK != (rc = vtss_phy_10g_rxckout_get(misc_phy_inst_get(), uport2iport(uport), &ckout))) {
                    ICLI_PRINTF("Error getting rxckout configuration for port %d (%s)\n", uport, error_txt(rc));
                } else {
                    ckout.mode = has_disable ? VTSS_RECVRD_CLKOUT_DISABLE : has_rx_clock ? VTSS_RECVRD_CLKOUT_LINE_SIDE_RX_CLOCK :
                                 has_tx_clock ? VTSS_RECVRD_CLKOUT_LINE_SIDE_TX_CLOCK : ckout.mode;
                    ckout.squelch_on_pcs_fault = has_pcs_fault_squelch ? TRUE :
                                                 has_no_pcs_fault_squelch ? FALSE : ckout.squelch_on_pcs_fault;
                    ckout.squelch_on_lopc = has_lopc_squelch ? TRUE :
                                            has_no_lopc_squelch ? FALSE :  ckout.squelch_on_lopc;
                    ICLI_PRINTF("Port %d, rxckout %s, squelch_on_pcs_fault %s, squelch_on_lopc %s\n", uport,
                                rcvrdclk_type2txt(ckout.mode),
                                ckout.squelch_on_pcs_fault ? "enable" : "disable",
                                ckout.squelch_on_lopc ? "enable" : "disable");
                    if (has_disable || has_rx_clock || has_tx_clock || has_pcs_fault_squelch ||
                        has_no_pcs_fault_squelch || has_lopc_squelch || has_no_lopc_squelch) {
                        if (VTSS_RC_OK != (rc = vtss_phy_10g_rxckout_set(misc_phy_inst_get(), uport2iport(uport), &ckout))) {
                            ICLI_PRINTF("Error setting rxckout configuration for port %d (%s)\n", uport, error_txt(rc));
                        }
                    }
                }
            }
        }
    }
}

void misc_icli_10g_phy_txckout(u32 session_id, BOOL has_disable, BOOL has_rx_clock, BOOL has_tx_clock,
                               icli_stack_port_range_t *v_port_type_list)
{
    vtss_phy_10g_txckout_conf_t ckout;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    mesa_rc rc;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                if (VTSS_RC_OK != (rc = vtss_phy_10g_txckout_get(misc_phy_inst_get(), uport2iport(uport), &ckout))) {
                    ICLI_PRINTF("Error getting txckout configuration for port %d (%s)\n", uport, error_txt(rc));
                } else {
                    ckout.mode = has_disable ? VTSS_RECVRD_CLKOUT_DISABLE : has_rx_clock ? VTSS_RECVRD_CLKOUT_LINE_SIDE_RX_CLOCK :
                                 has_tx_clock ? VTSS_RECVRD_CLKOUT_LINE_SIDE_TX_CLOCK : ckout.mode;
                    ICLI_PRINTF("Port %d, txckout %s\n", uport,
                                rcvrdclk_type2txt(ckout.mode));
                    if (has_disable || has_rx_clock || has_tx_clock) {
                        if (VTSS_RC_OK != (rc = vtss_phy_10g_txckout_set(misc_phy_inst_get(), uport2iport(uport), &ckout))) {
                            ICLI_PRINTF("Error setting txckout configuration for port %d (%s)\n", uport, error_txt(rc));
                        }
                    }
                }
            }
        }
    }
}

void misc_icli_10g_phy_lane_sync(u32 session_id, BOOL has_enable, BOOL has_disable,
                                 BOOL has_rx_macro, vtss_phy_10g_rx_macro_t rx_macro,
                                 BOOL has_tx_macro, vtss_phy_10g_tx_macro_t tx_macro,
                                 u8 rx_chid, u8 tx_chid,
                                 icli_stack_port_range_t *v_port_type_list)
{
    vtss_phy_10g_lane_sync_conf_t lane_sync;
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    mesa_rc rc;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                ICLI_PRINTF("has_enable:%d has_disable:%d has_rx_macro:%d rx_macro:%d has_tx_macro:%d tx_macro:%d \n",
                            has_enable, has_disable, has_rx_macro, rx_macro, has_tx_macro, tx_macro);

                lane_sync.enable = (has_enable ? 1 : 0);
                lane_sync.rx_macro = has_rx_macro ? rx_macro : VTSS_PHY_10G_RX_MACRO_LINE;
                lane_sync.tx_macro = has_tx_macro ? tx_macro : VTSS_PHY_10G_TX_MACRO_LINE;
                lane_sync.rx_ch = rx_chid;
                lane_sync.tx_ch = tx_chid;

                ICLI_PRINTF("Port%d lane_sync enable:%s rx_macro:%d tx_macro:%d\n",
                            uport, lane_sync.enable ? "enable" : "disable",
                            lane_sync.rx_macro, lane_sync.tx_macro);
                if (has_disable || has_enable || has_rx_macro || has_tx_macro) {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_lane_sync_set(misc_phy_inst_get(), uport2iport(uport), &lane_sync))) {
                        ICLI_PRINTF("Malibu Error setting lane sync configuration for port %d (%s)\n", uport, error_txt(rc));
                    }
                }
            }
        }
    }
}

void misc_icli_phy_10g_fc_buffer_reset(u32 session_id, icli_stack_port_range_t *v_port_type_list)
{
    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                if (vtss_phy_10g_fc_buffer_reset(misc_phy_inst_get(), uport2iport(uport)) != MESA_RC_OK) {
                    ICLI_PRINTF("%% Error MAC FC Buffer reset  for port %s %u/%u\n", icli_port_type_get_name(v_port_type_list->switch_range[range_idx].port_type), v_port_type_list->switch_range[range_idx].switch_id, uport);
                }

                ICLI_PRINTF(" MAC FC Buffer Reset\n");
            }
        }
    }
}

void misc_icli_spi_transaction(u32 session_id, u32 spi_cs, u32 cs_active_high, u32 gpio_mask, u32 gpio_value, u32 no_of_bytes, icli_hexval_t *v_hexval)
{
}

#define DUMP_AQR_PHY_REG_VALUE(a, b, c) \
for (reg_addr = b; reg_addr <= c; reg_addr++) { \
    if (VTSS_RC_OK != (rc = mesa_mmd_read(NULL, chip_no, miim_ctrl, miim_addr, a, reg_addr , &value))) { \
        ICLI_PRINTF(" 0x%04x   Fail   | ", reg_addr); \
    } else { \
        ICLI_PRINTF(" 0x%04x   0x%04x | ", reg_addr, value); \
    } \
    cnt ++; \
    if (cnt == 4) { \
        cnt = 0; \
        ICLI_PRINTF("\n"); \
    } \
}

mesa_rc misc_icli_phy_aqr_mmd_dump_direct(u32 session_id, mesa_miim_controller_t miim_ctrl, u8 miim_addr)
{
    u8 cnt = 0;
    u16 value, reg_addr = 0x0;
    mesa_rc rc = VTSS_RC_ERROR;
    mesa_chip_no_t chip_no;
    chip_no = 0; // Chip number used for targets with multiple chips

    ICLI_PRINTF("[PMA Registers]\n");
    ICLI_PRINTF(" Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |\n");
    DUMP_AQR_PHY_REG_VALUE(0x1, 0x0, 0x18);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0x81, 0x93);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0x708, 0x710);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0x8000, 0x8106);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0x9000, 0x9006);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xa000, 0xa007);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xa060, 0xa074);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xc142, 0xc142);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xc412, 0xc413);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xcc00, 0xcc02);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xd000, 0xd001);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xd400, 0xd402);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xd800, 0xd800);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xe400, 0xe400);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xe800, 0xe800);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xe810, 0xe811);
    DUMP_AQR_PHY_REG_VALUE(0x1, 0xfc00, 0xfc00);
    cnt = 0;
    ICLI_PRINTF("\n[PCS Registers]\n");
    ICLI_PRINTF(" Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |\n");
    DUMP_AQR_PHY_REG_VALUE(0x3, 0x0, 0x31);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0x708, 0x710);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc400, 0xc401);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc410, 0xc410);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc455, 0xc45a);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc568, 0xc56a);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc620, 0xc63f);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc646, 0xc646);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc820, 0xc823);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc860, 0xc863);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc870, 0xc873);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc880, 0xc884);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc900, 0xc91c);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc930, 0xc93a);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc980, 0xc982);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc9a4, 0xc9a5);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xc9b0, 0xc9b1);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xcc00, 0xcc03);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xd000, 0xd002);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xd400, 0xd403);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xd800, 0xd800);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xe400, 0xe400);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xe460, 0xe461);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xe470, 0xe471);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xe600, 0xe626);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xe800, 0xe800);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xe808, 0xe821);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xe840, 0xe84e);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xe860, 0xe866);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xe870, 0xe876);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xe900, 0xe906);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xec00, 0xec09);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xf400, 0xf406);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xf800, 0xf800);
    DUMP_AQR_PHY_REG_VALUE(0x3, 0xfc00, 0xfc02);
    cnt = 0;
    ICLI_PRINTF("\n[PHY XS Registers]\n");
    ICLI_PRINTF(" Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |\n");
    DUMP_AQR_PHY_REG_VALUE(0x4, 0x0, 0x1d);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0x708, 0x710);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xc180, 0xc180);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xc1c0, 0xc1c0);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xc1d0, 0xc1d0);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xc1e0, 0xc1e0);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xc1f0, 0xc1f0);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xc200, 0xc200);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xc440, 0xc449);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xc802, 0xc805);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xc820, 0xc822);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xcc00, 0xcc02);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xd000, 0xd001);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xd400, 0xd402);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xd800, 0xd801);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xd810, 0xd818);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xd820, 0xd824);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xd840, 0xd840);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xe410, 0xe41f);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xe802, 0xe812);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xec00, 0xec01);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xf400, 0xf401);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xf800, 0xf802);
    DUMP_AQR_PHY_REG_VALUE(0x4, 0xfc00, 0xfc00);
    cnt = 0;
    ICLI_PRINTF("\n[Autonegotiation Registers]\n");
    ICLI_PRINTF(" Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |\n");
    DUMP_AQR_PHY_REG_VALUE(0x7, 0x0, 0x23);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0x31, 0x32);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0x3c, 0x45);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xc200, 0xc201);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xc210, 0xc21b);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xc300, 0xc301);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xc310, 0xc31b);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xc400, 0xc400);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xc410, 0xc410);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xc800, 0xc800);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xc810, 0xc814);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xcc00, 0xcc01);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xd000, 0xd001);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xd400, 0xd402);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xe410, 0xe411);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xe820, 0xe823);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xe830, 0xe832);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xec00, 0xec03);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xf400, 0xf403);
    DUMP_AQR_PHY_REG_VALUE(0x7, 0xfc00, 0xfc00);
    cnt = 0;
    ICLI_PRINTF("\n[100BASE-TX and 1000BASE-T Registers]\n");
    ICLI_PRINTF(" Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |\n");
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0x2, 0xf);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xc282, 0xc282);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xc300, 0xc33b);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xc420, 0x3d);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xc500, 0xc501);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xd280, 0xd288);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xd290, 0xd298);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xd302, 0xd30c);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xd312, 0xd31c);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xd322, 0xd322);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xec10, 0xec10);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xec20, 0xec20);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xf410, 0xf410);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xf420, 0xf420);
    DUMP_AQR_PHY_REG_VALUE(0x1d, 0xfc00, 0xfc00);
    cnt = 0;
    ICLI_PRINTF("\n[Global Registers]\n");
    ICLI_PRINTF(" Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |  Reg_addr Value  |\n");
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0x0, 0xf);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0x20, 0x20);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0x80, 0x8f);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0x100, 0x105);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0x200, 0x206);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0x300, 0x301);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0x31a, 0x31f);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0x380, 0x39f);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc000, 0xc001);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc006, 0xc006);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc400, 0xc400);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc420, 0xc453);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc470, 0xc47f);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc485, 0xc485);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc495, 0xc495);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc4a0, 0xc4a0);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc800, 0xc807);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc820, 0xc821);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc830, 0xc831);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc840, 0xc842);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc850, 0xc850);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xc880, 0xc88f);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xcc00, 0xcc02);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xd400, 0xd402);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xfc00, 0xfc01);
    DUMP_AQR_PHY_REG_VALUE(0x1e, 0xff00, 0xff01);
    ICLI_PRINTF("\n");
    return rc;
}

mesa_rc misc_icli_phy_mmd_read_direct(u32 session_id, mesa_miim_controller_t miim_ctrl, u8 miim_addr,                                                   u16 dev_addr, u16 mmd_reg_addr)
{
    u16 value;
    mesa_rc rc = VTSS_RC_ERROR;
    mesa_chip_no_t chip_no;
    chip_no = 0; // Chip number used for targets with multiple chips
    if (VTSS_RC_OK != (rc = mesa_mmd_read(NULL, chip_no, miim_ctrl, miim_addr, dev_addr,
                                          mmd_reg_addr, &value))) {
        ICLI_PRINTF("%% Error: mmd read failed\n");
        return rc;
    } else {
        ICLI_PRINTF("miim_addr: %u, dev_addr: %u, mmd_reg_addr: 0x%04x, value: 0x%04x\n", miim_addr, dev_addr,
                    mmd_reg_addr, value);
        return rc;
    }
}

mesa_rc misc_icli_phy_mmd_write_direct(u32 session_id, mesa_miim_controller_t miim_ctrl, u32 miim_addr,                                                  u32 dev_addr, u32 mmd_reg_addr, u16 value)
{
    mesa_rc rc = VTSS_RC_ERROR;
    mesa_chip_no_t chip_no;
    chip_no = 0; // Chip number used for targets with multiple chips

    if (VTSS_RC_OK != (rc = mesa_mmd_write(NULL, chip_no, miim_ctrl, miim_addr, dev_addr,
                                           mmd_reg_addr, value))) {
        ICLI_PRINTF("%% Error: mmd write failed\n");
        return rc;
    } else {
        ICLI_PRINTF("miim_addr: %u, dev_addr: %u, addr: 0x%04x, value: 0x%04x\n", miim_addr, dev_addr,
                    mmd_reg_addr, value);
        return rc;
    }
}

mesa_rc misc_icli_phy_csr_read_write(u32 session_id, icli_stack_port_range_t *plist, u8 dev, u16 addr, u32 value, BOOL is_read, icli_unsigned_range_t *addr_list)
{
    u32                        range_idx = 0, cnt_idx = 0;
    i8 tcount = 0;
    mesa_port_no_t             uport;
    u16                        l_addr;
    mesa_rc rc = VTSS_RC_OK;

    if (is_read) {
        ICLI_PRINTF("%-4s %-4s %-6s %-10s %-40s\n", "Port", "Dev", "Addr", "Value(hex)", "----.--24.----.--16.----.---8.----.---0");
        ICLI_PRINTF("%-4s %-4s %-6s %-10s %-40s\n", "----", "---", "------", "----------", "----------------------------------------");
    }

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                if (is_read && addr_list != NULL) {
                    for (l_addr = addr_list->range[0].min; l_addr <= addr_list->range[0].max; l_addr++) {
                        rc = vtss_phy_10g_csr_read( PHY_INST, uport2iport(uport), dev, l_addr, &value);
                        ICLI_PRINTF("%-4u %-4u 0x%04x 0x%08x ", uport, dev, l_addr, value);
                        for (tcount = 31; tcount >= 0; --tcount) {
                            ICLI_PRINTF("%u%s", value & (1 << tcount) ? 1 : 0, tcount == 0 ? "\n" : (tcount % 4) ? "" : ".");
                        }
                    }
                } else {
                    rc = vtss_phy_10g_csr_write(PHY_INST, uport2iport(uport), dev, addr, value);
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error: register %s failed\n", is_read ? "READ" : "WRITE");
                }
            }
        }
    }

    return rc;
}

BOOL misc_icli_10g_phy_srefclk(u32 session_id, BOOL has_enable, BOOL has_no,
                               BOOL has_show, icli_stack_port_range_t *plist, BOOL has_freq, BOOL freq125, BOOL freq156_25, BOOL freq_155_52)
{
    vtss_phy_10g_srefclk_mode_t     srefclk;
    u32                             range_idx = 0, cnt_idx = 0;
    mesa_port_no_t                  uport = 0;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (vtss_phy_10g_srefclk_conf_get(misc_phy_inst_get(), uport2iport(uport), &srefclk) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error getting srefclk configuration for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                }

                if (has_show) {
                    ICLI_PRINTF("Port : %u, SREFCLK : %s\n", uport, srefclk.enable ? "Enabled" : "Disabled");
                }

                if (has_freq) {
                    if (freq156_25) {
                        srefclk.freq = VTSS_PHY_10G_SREFCLK_156_25;
                    } else if (freq125) {
                        srefclk.freq = VTSS_PHY_10G_SREFCLK_125_00;
                    } else if (freq_155_52) {
                        srefclk.freq = VTSS_PHY_10G_SREFCLK_155_52;
                    }
                }

                if (has_enable || has_no) {
                    if (srefclk.enable == has_enable) {
                        continue;
                    }

                    srefclk.enable = has_enable;
                    if (vtss_phy_10g_srefclk_conf_set(misc_phy_inst_get(), uport2iport(uport), &srefclk) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error setting srefclk configuration for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport);
                        continue;
                    }
                }
            } //End of for loop for ports
        } //End of for loop for switches
        return TRUE;
    } else {
        ICLI_PRINTF("%% Error NULL Port List");
        return FALSE;
    }
}

BOOL misc_icli_10g_phy_synce_mode(u32 session_id, BOOL has_mode, u8 mode, BOOL has_clk_out, u8 clk_out,
                                  BOOL has_hitless, u8 hitless, BOOL has_rclk_div, u8 rclk_div,
                                  BOOL has_sref_div, u8 sref_div, BOOL has_wref_div, u8 wref_div,
                                  icli_stack_port_range_t *plist)
{
    u32                      range_idx = 0, cnt_idx = 0;
    mesa_port_no_t           uport;
    vtss_phy_10g_mode_t      synce_mode;
    bool                     mepa_cap_phy_ts = false;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (vtss_phy_10g_mode_get(misc_phy_inst_get(), uport2iport(uport), &synce_mode) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error getting synce mode for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                    continue;
                }

                if (has_mode || has_clk_out || has_hitless || has_rclk_div || has_sref_div || has_wref_div) {
                    if (has_mode) {
                        synce_mode.oper_mode = (vtss_oper_mode_t)mode;
                    }

                    if (has_clk_out) {
                        synce_mode.rcvrd_clk = (vtss_recvrd_t)clk_out;
                    }

                    if (has_hitless) {
                        synce_mode.hl_clk_synth = hitless;
                    }

                    if (has_rclk_div) {
                        synce_mode.rcvrd_clk_div = (vtss_recvrdclk_cdr_div_t)rclk_div;
                    }

                    if (has_sref_div) {
                        synce_mode.sref_clk_div = (vtss_srefclk_div_t)sref_div;
                    }

                    if (has_wref_div) {
                        synce_mode.wref_clk_div = (vtss_wref_clk_div_t)wref_div;
                    }

                    if (vtss_phy_10g_reset(misc_phy_inst_get(), uport2iport(uport)) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in performing PHY reset for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport);
                        continue;
                    }

                    if (vtss_phy_10g_mode_set(misc_phy_inst_get(), uport2iport(uport), &synce_mode) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in configuring synce mode for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport);
                        continue;
                    }

#ifdef VTSS_SW_OPTION_TOD
                    mepa_cap_phy_ts = mepa_phy_ts_cap();
#endif //VTSS_SW_OPTION_TOD
                    if (mepa_cap_phy_ts) {
                        if (VTSS_RC_OK != vtss_phy_ts_phy_oper_mode_change(PHY_INST, uport2iport(uport))) {
                            ICLI_PRINTF("Error in changing the mode for %d \n", uport);
                            continue;
                        }
                    }
                } else {
                    if (vtss_phy_10g_mode_get(misc_phy_inst_get(), uport2iport(uport), &synce_mode) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in getting synce mode for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport);
                        continue;
                    }

                    ICLI_PRINTF("Port: %u\n", uport);
                    ICLI_PRINTF("--------\n");
                    ICLI_PRINTF("SyncE oper mode      :%d  \n", synce_mode.oper_mode);
                    ICLI_PRINTF("SyncE hl clk synth   :%d  \n", synce_mode.hl_clk_synth);
                    ICLI_PRINTF("SyncE rcvrd clk      :%d  \n", synce_mode.rcvrd_clk);
                    ICLI_PRINTF("SyncE rcvrd  clk div :%d  \n", synce_mode.rcvrd_clk_div);
                    ICLI_PRINTF("SyncE sref clk div   :%d  \n", synce_mode.sref_clk_div);
                    ICLI_PRINTF("SyncE wref clk div   :%d  \n", synce_mode.wref_clk_div);
                }
            } // end of cnt_idx
        } // end of range_idx
        return TRUE;
    } else {
        ICLI_PRINTF("%% Error NULL Port List");
        return FALSE;
    }
}

BOOL misc_icli_10g_phy_synce_clkout(u32 session_id, BOOL has_enable, BOOL has_disable, icli_stack_port_range_t *plist)
{
    u32                      range_idx = 0, cnt_idx = 0;
    mesa_port_no_t           uport;
    BOOL                     synce_clkout;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (vtss_phy_10g_synce_clkout_get(misc_phy_inst_get(), uport2iport(uport), &synce_clkout) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error in Getting synce clock out for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                    continue;
                }

                if (has_enable || has_disable) {
                    if (synce_clkout == has_enable) {
                        continue;
                    }

                    synce_clkout = has_enable;
                    if (vtss_phy_10g_synce_clkout_set(misc_phy_inst_get(), uport2iport(uport), synce_clkout) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in Setting synce clock out for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport);
                        continue;
                    }
                } else {
                    ICLI_PRINTF("Port: %u\n", uport);
                    ICLI_PRINTF("--------\n");
                    ICLI_PRINTF("SyncE Clkout :%-12s  \n", synce_clkout ? "Enabled" : "Disabled");
                }
            } // end of cnt_idx
        } // end of range_idx
        return TRUE;
    } else {
        ICLI_PRINTF("%% Error NULL Port List");
        return FALSE;
    }
}

BOOL misc_icli_10g_phy_xfp_clkout(u32 session_id, BOOL has_enable, BOOL has_disable, icli_stack_port_range_t *plist)
{
    u32                      range_idx = 0, cnt_idx = 0;
    mesa_port_no_t           uport;
    BOOL                     xfp_clkout;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;
                // Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                if (vtss_phy_10g_xfp_clkout_get(misc_phy_inst_get(), uport2iport(uport), &xfp_clkout) != VTSS_RC_OK) {
                    ICLI_PRINTF("%% Error in Getting clock out for xfp for port %s %u/%u\n",
                                icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                plist->switch_range[range_idx].switch_id, uport);
                    continue;
                }

                if (has_enable || has_disable) {
                    if (xfp_clkout == has_enable) {
                        continue;
                    }

                    xfp_clkout = has_enable;
                    if (vtss_phy_10g_xfp_clkout_set(misc_phy_inst_get(), uport2iport(uport), xfp_clkout) != VTSS_RC_OK) {
                        ICLI_PRINTF("%% Error in Setting xfp clockout for port %s %u/%u\n",
                                    icli_port_type_get_name(plist->switch_range[range_idx].port_type),
                                    plist->switch_range[range_idx].switch_id, uport);
                        continue;
                    }
                } else {
                    ICLI_PRINTF("Port: %u\n", uport);
                    ICLI_PRINTF("--------\n");
                    ICLI_PRINTF("XFP Clkout :%-12s  \n", xfp_clkout ? "Enabled" : "Disabled");
                }
            } // end of cnt_idx
        } // end of range_idx
        return TRUE;
    } else {
        ICLI_PRINTF("%% Error NULL Port List");
        return FALSE;
    }
}

mesa_rc phy_icli_10g_event_enable_set(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                      u32 phy_10g_events, BOOL has_enable, BOOL has_disable)
{
    u32                       range_idx, cnt_idx;
    mesa_port_no_t            uport;
    vtss_phy_10g_event_t      phy_10g_wis_events;
    mesa_rc rc = VTSS_RC_ERROR;
    if (v_port_type_list) { //at least one range input
        //Loop through all switches in the stack
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            //Loop through all ports in the switch
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                phy_10g_wis_events = 1 << (phy_10g_events - 1) ; //events supported for Macros defined after 0x40
                if (has_enable) {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_event_enable_set(misc_phy_inst_get(), uport2iport(uport),
                                                                          phy_10g_wis_events, TRUE)))  {
                        ICLI_PRINTF("Failed to set 10g event::port-no = %d\n", uport);
                    } else {
                        ICLI_PRINTF("Successfull\n");
                    }
                } else if (has_disable) {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_event_enable_set(misc_phy_inst_get(), uport2iport(uport),
                                                                          phy_10g_wis_events, FALSE)))  {
                        ICLI_PRINTF("Failed to disable 10g event::port-no = %d\n", uport);
                    } else {
                        ICLI_PRINTF("Successfull\n");
                    }
                }
            }
        }
    }

    return rc;
}

static const char *event_10g_type2txt(vtss_phy_10g_event_t  phy_10g_events)
{
    switch (phy_10g_events) {
    case 0x1:
        return "LINK_LOS";
    case 0x2:
        return "RX_LOL";
    case 0x4:
        return "TX_LOL";
    case 0x8:
        return "LOPC";
    case 0x10:
        return "HIGH_BER";
    case 0x20:
        return "MODULE_STAT";
    case 0x40:
        return "PCS_RECEIVE_FAULT";
    case 0x80:
        return "SEF";
    case 0x100:
        return "far-end (PLM-P)";
    case 0x200:
        return "far-end (AIS-P)";
    case 0x400:
        return "Loss of Frame (LOF)";
    case 0x800:
        return "Line Remote Defect Indication";
    case 0x1000:
        return "Line Alarm Indication Signal";
    case 0x2000:
        return "Loss of Code-group Delineation";
    case 0x4000:
        return "Path Label Mismatch (PLMP)";
    case 0x8000:
        return "Path Alarm Indication Signal";
    case 0x10000:
        return "Path Loss of Pointer";
    case 0x20000:
        return "Unequiped Path";
    case 0x40000:
        return "Far-end Unequiped";
    case 0x80000:
        return "Far-end Path Remote Defect";
    case 0x100000:
        return "Line Remote Error Indication";
    case 0x200000:
        return "Path Remote Error Indication";
    case 0x400000:
        return "PMTICK B1 BIP";
    case 0x800000:
        return "PMTICK B2 BIP";
    case 0x1000000:
        return "PMTICK B3 BIP";
    case 0x2000000:
        return "PMTICK REI-L";
    case 0x4000000:
        return "PMTICK REI-P";
    case 0x8000000:
        return "B1 THRESH ERR";
    case 0x10000000:
        return "B2 THRESH ERR";
    case 0x20000000:
        return "B3 THRESH ERR";
    case 0x40000000:
        return "REIL THRESH ERR";
    case 0x80000000:
        return "REIP THRESH_ERR";
    }

    return "NO EVENT DETECTED";
}

mesa_rc phy_icli_10g_event_enable_poll(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                       u32 phy_10g_events)
{
    u32                       range_idx, cnt_idx;
    mesa_port_no_t            uport;
    vtss_phy_10g_event_t      phy_10g_wis_events;
    mesa_rc rc = VTSS_RC_ERROR;
    if (v_port_type_list) { //at least one range input
        //Loop through all switches in the stack
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            //Loop through all ports in the switch
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                phy_10g_wis_events = (1 << (phy_10g_events - 1)) ; //events supported for Macros defined after 0x40
                if (VTSS_RC_OK != (rc = vtss_phy_10g_event_poll(misc_phy_inst_get(), uport2iport(uport),
                                                                &phy_10g_wis_events))) {
                    ICLI_PRINTF("Failed to poll 10g event::port-no = %d\n", uport);
                } else {
                    ICLI_PRINTF("Events  %s\n", event_10g_type2txt(phy_10g_wis_events));
                }
            }
        }
    }

    return rc;
}

void misc_icli_cmd_apc_restart(BOOL has_host, icli_stack_port_range_t *v_port_type_list)
{
    u32                 range_idx, cnt_idx;
    mesa_port_no_t      uport;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                vtss_phy_10g_apc_restart (misc_phy_inst_get(), uport2iport(uport), has_host);
            }
        }
    }

    return;
}

mesa_rc phy_icli_10g_extended_event_enable_set(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                               u32 xtnd_events, BOOL has_enable, BOOL has_disable)
{
    u32                       range_idx, cnt_idx;
    mesa_port_no_t            uport;
    vtss_phy_10g_extnd_event_t phy_10g_extend_events;
    mesa_rc rc = VTSS_RC_ERROR;
    if (v_port_type_list) { //at least one range input
        //Loop through all switches in the stack
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            //Loop through all ports in the switch
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                phy_10g_extend_events = (1 << (xtnd_events - 1)); //events supported for Macros defined after 0x01
                if (has_enable) {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_extended_event_enable_set(misc_phy_inst_get(), uport2iport(uport),
                                                                                   phy_10g_extend_events, has_enable)))  {
                        ICLI_PRINTF("Failed to set 10g event::port-no = %d\n", uport);
                    } else {
                        ICLI_PRINTF("Successfull\n");
                    }
                } else if (has_disable) {
                    if (VTSS_RC_OK != (rc = vtss_phy_10g_extended_event_enable_set(misc_phy_inst_get(), uport2iport(uport),
                                                                                   phy_10g_extend_events, FALSE)))  {
                        ICLI_PRINTF("Failed to disable 10g event::port-no = %d\n", uport);
                    } else {
                        ICLI_PRINTF("Successfull\n");
                    }
                }
            }
        }
    }

    return rc;
}

static const char *extend_event_10g_type2txt(vtss_phy_10g_extnd_event_t phy_10g_extend_events)
{
    switch (phy_10g_extend_events) {
    case 0x01:
        return "RX LOS";
    case 0x02:
        return "RX LOL";
    case 0x04:
        return "TX LOL";
    /*case 0x08:
        return "None";*/
    case 0x10:
        return "RX character decode error";
    case 0x20:
        return "TX character encode error";
    case 0x40:
        return "RX block decode error count";
    case 0x80:
        return "TX block encode error count";
    case 0x100:
        return "RX sequencing error count";
    case 0x200:
        return "TX sequencing error count";
    case 0x400:
        return "KR-FEC uncorrectable";
    case 0x800:
        return "KR-FEC corrected";
    case 0x1000:
        return "high bit Error";
    case 0x2000:
        return "Link status up/down";
    }

    return "NO EVENT DETECTED";
}

mesa_rc phy_icli_10g_extended_event_enable_poll(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                                u32  xtnd_events)
{
    u32                       range_idx, cnt_idx;
    mesa_port_no_t            uport;
    vtss_phy_10g_extnd_event_t phy_10g_extend_events;
    mesa_rc rc = VTSS_RC_ERROR;
    if (v_port_type_list) { //at least one range input
        //Loop through all switches in the stack
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            //Loop through all ports in the switch
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                phy_10g_extend_events = (1 << (xtnd_events - 1)) ; //events supported for Macros defined after 0x01
                if (VTSS_RC_OK != (rc = vtss_phy_10g_extended_event_poll(misc_phy_inst_get(), uport2iport(uport),
                                                                         &phy_10g_extend_events))) {
                    ICLI_PRINTF("Failed to poll 10g event::port-no = %d\n", uport);
                } else {
                    ICLI_PRINTF("Event occurred :: %s\n", extend_event_10g_type2txt(phy_10g_extend_events));
                }
            }
        }
    }

    return rc;
}

static const char *event_phy_xtnd_type2txt(vtss_phy_event_t phy_xtnd_intrpt)
{
    switch (phy_xtnd_intrpt) {
    case 0x20000:
        return "EEE_WAKE_ERR";
    case 0x40000:
        return "EEE_WAIT_TS";
    case 0x80000:
        return "EEE_WAIT_RX_TQ";
    case 0x100000:
        return "EEE_LINKFAIL";
    case 0x200000:
        return "RR_SW_COMPL";
    case 0x400000:
        return "MACSEC_HOST_MAC";
    case 0x800000:
        return "MACSEC_LINE_MAC";
    case 0x1000000:
        return "MACSEC_FC_BUFF";
    case 0x2000000:
        return "MACSEC_INGRESS";
    case 0x4000000:
        return "MACSEC_EGRESS";
    case 0x8000000:
        return "MEM_INT_RING";

    }

    return "NO EVENT DETECTED";
}

mesa_rc phy_icli_1g_phy_extended_event_set(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                           BOOL has_poll, BOOL has_set, BOOL has_clear, u32 phy_xtnd_event)
{

    u32                range_idx, cnt_idx;
    mesa_port_no_t     uport;
    vtss_phy_event_t   phy_xtnd_intrpt;
    mesa_rc rc = VTSS_RC_ERROR;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                phy_xtnd_intrpt = (1 << (phy_xtnd_event + 16));
                if (has_set) {
                    if (VTSS_RC_OK != (rc = vtss_phy_event_enable_set(misc_phy_inst_get(),
                                                                      uport2iport(uport), phy_xtnd_intrpt, has_set))) {
                        ICLI_PRINTF("Failed to set event::port-no = %d\n", uport);
                    } else {
                        ICLI_PRINTF("Successfull\n");
                    }
                } else if (has_clear) {
                    phy_xtnd_intrpt = ~(1 << (phy_xtnd_event + 16));
                    if (VTSS_RC_OK != (rc = vtss_phy_event_enable_set(misc_phy_inst_get(),
                                                                      uport2iport(uport), phy_xtnd_intrpt, has_clear))) {
                        ICLI_PRINTF("Failed to set event::port-no = %d\n", uport);
                    } else {
                        ICLI_PRINTF("Successfull\n");
                    }
                } else if (has_poll) {
                    if (VTSS_RC_OK != (rc = vtss_phy_event_poll(misc_phy_inst_get(), uport2iport(uport), &phy_xtnd_intrpt))) {
                        ICLI_PRINTF("Failed to set event::port-no = %d\n", uport);
                    } else {
                        ICLI_PRINTF("Event occurred::%s\n", event_phy_xtnd_type2txt(phy_xtnd_intrpt));
                    }
                }
            }
        }
    }

    return rc;
}

static const char *event_ts_type2txt(vtss_phy_ts_event_t ts_interrupt)
{
    switch (ts_interrupt) {
    case 0x01:
        return "INGR_ENGINE_ERR";
    case 0x02:
        return "";
    case 0x04:
        return "INGR_RW_FCS_ERR";
    case 0x08:
        return "EGR_ENGINE_ERR";
    case 0x10:
        return "EGR_RW_FCS_ERR";
    case 0x20:
        return "EGR_TIMESTAMP_CAPTURED";
    case 0x40:
        return "EGR_FIFO_OVERFLOW";
    case 0x80:
        return "DATA_IN_RSRVD_FIELD";
    case 0x100:
        return "LTC_NEW_PPS_INTRPT";
    case 0x200:
        return "LOAD_SAVE_NEW_TOD";
    }

    return "NO EVENT DETECTED";
}

mesa_rc phy_icli_1588_event_enable_set(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_get,
                                       BOOL has_set, BOOL has_clear, u16 ts_event)
{

    u32             range_idx, cnt_idx;
    mesa_port_no_t  uport;
    vtss_phy_ts_event_t ts_interrupt;
    mesa_rc rc = VTSS_RC_ERROR;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                ts_interrupt =  (1 << (ts_event - 1));
                if (has_set) {
                    if (VTSS_RC_OK != (rc = vtss_phy_ts_event_enable_set(misc_phy_inst_get(),
                                                                         uport2iport(uport), has_set, ts_interrupt))) {
                        ICLI_PRINTF("Failed to set event::port-no = %d\n", uport);
                    } else {
                        ICLI_PRINTF("Successfull\n");
                    }
                } else if (has_get) {
                    if (VTSS_RC_OK != (rc = vtss_phy_ts_event_poll(misc_phy_inst_get(), uport2iport(uport), &ts_interrupt))) {
                        ICLI_PRINTF("Failed to poll event::port-no = %d\n", uport);
                    } else {
                        ICLI_PRINTF("Event occurred :: %s\n", event_ts_type2txt(ts_interrupt));
                    }
                } else if (has_clear) {
                    ts_interrupt  = ((1 << (ts_event - 1)) & 0);
                    if (VTSS_RC_OK != (rc = vtss_phy_ts_event_enable_set(misc_phy_inst_get(),
                                                                         uport2iport(uport), has_clear, ts_interrupt))) {
                        ICLI_PRINTF("Failed to clear event::port-no = %d\n", uport);
                    } else {
                        ICLI_PRINTF("Cleared\n");
                    }
                }
            }
        }
    }

    return rc;
}

mesa_rc phy_icli_cmd_ltc_freq_synth(i32 session_id, icli_stack_port_range_t *plist, BOOL has_enable, BOOL has_disable,
                                    u8 High_duty_cycle, u8 Low_duty_cycle)
{

    u32                        range_idx = 0, cnt_idx = 0;
    mesa_port_no_t             uport;
    vtss_phy_ltc_freq_synth_t  lfs;

    mesa_rc rc = VTSS_RC_ERROR;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;

                if (has_enable) {
                    lfs.high_duty_cycle = High_duty_cycle;
                    lfs.low_duty_cycle =  Low_duty_cycle;
                }

                if (has_enable || has_disable) {
                    lfs.enable = has_enable;
                    if ((rc = vtss_phy_ts_ltc_freq_synth_pulse_set(misc_phy_inst_get(),
                                                                   uport2iport(uport), &lfs)) != VTSS_RC_ERROR) {
                        ICLI_PRINTF("LTC frequency %s\t port_no = %d\n", has_enable ? "enabled" : "disbaled", uport);
                        continue;
                    } else {
                        ICLI_PRINTF("%% Error: Failed to set LTC frequency port_no %d\n", uport);
                    }
                } else {
                    if ((rc = vtss_phy_ts_ltc_freq_synth_pulse_get(misc_phy_inst_get(),
                                                                   uport2iport(uport), &lfs)) != VTSS_RC_ERROR) {
                        ICLI_PRINTF("High time :%u\t", lfs.high_duty_cycle);
                        ICLI_PRINTF("Low time :%u\n", lfs.low_duty_cycle);
                        continue;
                    } else {
                        ICLI_PRINTF("%% Error: Failed to get LTC frequency port_no %d\n", uport);
                    }
                }
            }
        }
    } else {
        ICLI_PRINTF("%% Error NULL Port List");
        return rc;
    }

    return rc;
}

mesa_rc misc_icli_ts_latency(i32 session_id, BOOL has_ingress, BOOL has_egress, BOOL has_latency, u32 value, icli_stack_port_range_t *plist)
{
    u8 uport = 0, range_idx = 0, cnt_idx = 0, port_no = 0;
    BOOL error = FALSE;
    u32 temp = 0;
    mesa_timeinterval_t  latency = value;
    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;
                port_no = uport2iport(uport);
                if (has_latency) {
                    if (has_ingress) {
                        if (vtss_phy_ts_ingress_latency_set(misc_phy_inst_get(), port_no, &latency) != VTSS_RC_OK) {
                            error = TRUE;
                        }
                    } else {
                        if (vtss_phy_ts_egress_latency_set(misc_phy_inst_get(), port_no, &latency) != VTSS_RC_OK) {
                            error = TRUE;
                        }
                    }
                } else {
                    ICLI_PRINTF(" show information\n");
                    if (has_ingress) {

                        if (vtss_phy_ts_ingress_latency_get(misc_phy_inst_get(), port_no, &latency) != VTSS_RC_OK) {
                            error = TRUE;
                            ICLI_PRINTF("Failed...! Get Operation is not processed\n");
                        }
                    } else {
                        if (vtss_phy_ts_egress_latency_get(misc_phy_inst_get(), port_no, &latency) != VTSS_RC_OK) {
                            error = TRUE;
                            ICLI_PRINTF("Failed...! Get Operation is not processed\n");
                        }
                    }

                    if (!error) {
                        //latency = latency >> 16;
                        temp = (u32) latency;
                        ICLI_PRINTF("Port  Ingress/Egress  Latency\n----  --------------  -------\n");
                        ICLI_PRINTF("%-4d  %-14s  %u\n", uport, has_ingress ? "Ingress" : "Egress", temp);
                    }
                }
            }
        }
    }

    return error;
}

mesa_rc misc_icli_ts_nphase(i32 session_id, BOOL has_nphase, u8 value, icli_stack_port_range_t *plist)
{
    vtss_phy_ts_nphase_status_t status;
    mesa_port_no_t             port_no;
    u8 icount = 0, range_idx = 0, cnt_idx = 0, uport = 0;
    BOOL error = FALSE;
    char sampler_str[5][10] = {"PPS_O", "PPS_RI", "EGR_SOF", "ING_SOF", "LS"};

    ICLI_PRINTF("Port  Sampler  %-12s\n", "Calibration");
    ICLI_PRINTF("%-4s  %-7s  %-5s  %-5s\n", "", "", "Done", "Error");
    ICLI_PRINTF("%-4s  %-7s  %-5s  %-5s\n", "---", "-------", "-----", "-----");
    if (plist) {
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;
                port_no = uport2iport(uport);
                if (has_nphase && value) {
                    //Shows NPhase sampler value of the needed
                    if (vtss_phy_ts_nphase_status_get(misc_phy_inst_get(), port_no, (vtss_phy_ts_nphase_sampler_t)(value - 1), &status) == VTSS_RC_OK) {
                        ICLI_PRINTF("%-4u  %-7s  %-5s  %-5s\n", uport, sampler_str[value - 1], status.CALIB_DONE ? "YES" : "FALSE", status.CALIB_ERR ? "YES" : "FALSE");
                    } else {
                        error = TRUE;
                    }
                } else {
                    //Show NPhase sampler value
                    for (icount = VTSS_PHY_TS_NPHASE_PPS_O; icount < VTSS_PHY_TS_NPHASE_MAX; icount++) {
                        if (vtss_phy_ts_nphase_status_get(misc_phy_inst_get(), port_no, (vtss_phy_ts_nphase_sampler_t)icount, &status) == VTSS_RC_OK) {
                            ICLI_PRINTF("%-4u  %-7s  %-5s  %-5s\n", uport, sampler_str[icount], status.CALIB_DONE ? "YES" : "FALSE", status.CALIB_ERR ? "YES" : "FALSE");
                        } else {
                            error = TRUE;
                            return error;
                        }
                    }
                }
            }
        }
    }

    return error;
}

void misc_icli_deb_phy_ts_statistics(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 time_sec)
{
    u8 i = 0, j = 0;
    CapArray<mesa_port_no_t, MEBA_CAP_BOARD_PORT_MAP_COUNT> ts_port_no;
    vtss_phy_ts_stats_t ts_stats;
    u8  num_ports = 0 ;
    u32 range_idx, cnt_idx;
    mesa_port_no_t port_no, uport;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);

                if ( vtss_phy_ts_stats_get(API_INST_DEFAULT, port_no, &ts_stats) == VTSS_RC_OK) {
                    ts_port_no[num_ports] = port_no;
                    num_ports++;
                } else {
                    ICLI_PRINTF("Cannot retrieve for port:%d\n", uport);
                    continue;
                }

                ICLI_PRINTF("Port : %u\n", iport2uport(port_no));
                ICLI_PRINTF("Frames with preambles too short to shrink  Ingress        : %u\n", ts_stats.ingr_pream_shrink_err);
                ICLI_PRINTF("Frames with preambles too short to shrink  Egress         : %u\n", ts_stats.egr_pream_shrink_err );
                ICLI_PRINTF("Timestamp block received frame with FCS error in ingress  : %u\n", ts_stats.ingr_fcs_err );
                ICLI_PRINTF("Timestamp block received frame with FCS error in egress   : %u\n", ts_stats.egr_fcs_err );
                ICLI_PRINTF("No of frames modified by timestamp block in ingress       : %u\n", ts_stats.ingr_frm_mod_cnt );
                ICLI_PRINTF("No of frames modified by timestamp block in egress        : %u\n", ts_stats.egr_frm_mod_cnt );
                ICLI_PRINTF("The number of timestamps transmitted to the SPI interface : %u\n", ts_stats.ts_fifo_tx_cnt );
                ICLI_PRINTF("Count of dropped Timestamps not enqueued to the Tx TSFIFO : %u\n", ts_stats.ts_fifo_drop_cnt );
                ICLI_PRINTF("\n\n");
            }
        }
    }

    for (j = 0; j < time_sec; j++) {
        for (i = 0; i < num_ports; i++) {
            if ( vtss_phy_ts_stats_get(API_INST_DEFAULT, ts_port_no[i], &ts_stats) == VTSS_RC_OK) {
                ICLI_PRINTF("Port : %u\n", iport2uport(ts_port_no[i]));
                ICLI_PRINTF("Frames with preambles too short to shrink  Ingress        : %u\n", ts_stats.ingr_pream_shrink_err);
                ICLI_PRINTF("Frames with preambles too short to shrink  Egress         : %u\n", ts_stats.egr_pream_shrink_err );
                ICLI_PRINTF("Timestamp block received frame with FCS error in ingress  : %u\n", ts_stats.ingr_fcs_err );
                ICLI_PRINTF("Timestamp block received frame with FCS error in egress   : %u\n", ts_stats.egr_fcs_err );
                ICLI_PRINTF("No of frames modified by timestamp block in ingress       : %u\n", ts_stats.ingr_frm_mod_cnt );
                ICLI_PRINTF("No of frames modified by timestamp block in egress        : %u\n", ts_stats.egr_frm_mod_cnt );
                ICLI_PRINTF("The number of timestamps transmitted to the SPI interface : %u\n", ts_stats.ts_fifo_tx_cnt );
                ICLI_PRINTF("Count of dropped Timestamps not enqueued to the Tx TSFIFO : %u\n", ts_stats.ts_fifo_drop_cnt );
                ICLI_PRINTF("\n\n");
            }
        }

        VTSS_OS_MSLEEP(1000);
    }
}

#define MISC_ICLI_PHY_TS_SIG_MSG_TYPE_LEN       1
#define MISC_ICLI_PHY_TS_SIG_DOMAIN_NUM_LEN     1
#define MISC_ICLI_PHY_TS_SIG_SOURCE_PORT_ID_LEN 10
#define MISC_ICLI_PHY_TS_SIG_SEQUENCE_ID_LEN    2
#define MISC_ICLI_PHY_TS_SIG_DEST_IP_LEN        4
#define MISC_ICLI_PHY_TS_SIG_SRC_IP_LEN         4
#define MISC_ICLI_PHY_TS_SIG_DEST_MAC_LEN       6

void misc_icli_deb_phy_ts_signature(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_sig_mask, u16 sig_mask)
{
    u32 range_idx, cnt_idx;
    u16 sig_mask_get = 0, len = 0;
    mesa_port_no_t port_no, uport;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                if (has_sig_mask) {
                    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_MSG_TYPE) {
                        len += MISC_ICLI_PHY_TS_SIG_MSG_TYPE_LEN;
                        ICLI_PRINTF("Msg Type |");
                    }

                    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DOMAIN_NUM) {
                        len += MISC_ICLI_PHY_TS_SIG_DOMAIN_NUM_LEN;
                        ICLI_PRINTF("Domain Num |");
                    }

                    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SOURCE_PORT_ID) {
                        len += MISC_ICLI_PHY_TS_SIG_SOURCE_PORT_ID_LEN;
                        ICLI_PRINTF("Source Port ID |");
                    }

                    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SEQ_ID) {
                        len += MISC_ICLI_PHY_TS_SIG_SEQUENCE_ID_LEN ;
                        ICLI_PRINTF("Sequence ID |");
                    }

                    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DEST_IP) {
                        len += MISC_ICLI_PHY_TS_SIG_DEST_IP_LEN;
                        ICLI_PRINTF("Dest IP |");
                    }

                    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_SRC_IP) {
                        len += MISC_ICLI_PHY_TS_SIG_SRC_IP_LEN;
                        ICLI_PRINTF("Src IP |");
                    }

                    if (sig_mask & VTSS_PHY_TS_FIFO_SIG_DEST_MAC) {
                        len += MISC_ICLI_PHY_TS_SIG_DEST_MAC_LEN;
                        ICLI_PRINTF("Dest MAC |");
                    }

                    ICLI_PRINTF("\nLength : %d \n", len);
                    if (vtss_phy_ts_fifo_sig_set(API_INST_DEFAULT, port_no, sig_mask) != VTSS_RC_OK) {
                        ICLI_PRINTF("Could not perform vtss_phy_ts_fifo_sig_set() operation \n");
                        return;
                    }
                } else {
                    if (vtss_phy_ts_fifo_sig_get(API_INST_DEFAULT, port_no,
                                                 (vtss_phy_ts_fifo_sig_mask_t *)&sig_mask_get) != VTSS_RC_OK) {
                        ICLI_PRINTF("Could not perform vtss_phy_ts_fifo_sig_get() operation \n");
                        return;
                    }

                    ICLI_PRINTF("Port : %d Signature : %x \n", port_no, sig_mask_get);
                }
            }
        }
    }
}

void misc_icli_deb_phy_ts_block_init(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_clk_freq, u8 clk_freq, BOOL has_clk_src, u8 clk_src, BOOL has_rx_ts_pos, u8 rx_ts_pos, BOOL has_tx_fifo_mode, u8 tx_fifo_mode, BOOL has_tx_fifo_spi_conf, BOOL has_tx_fifo_hi_clk_cycs, u8 hi_clk_value, BOOL has_tx_fifo_lo_clk_cycs, u8 lo_clk_value, BOOL has_modify_frm, u8 modify_frm)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t port_no, uport;
    mesa_rc rc;
#ifndef VTSS_FEATURE_PTP_DELAY_COMP_ENGINE
    vtss_phy_ts_init_conf_t tod_phy_init = {VTSS_PHY_TS_CLOCK_FREQ_250M, VTSS_PHY_TS_CLOCK_SRC_EXTERNAL, VTSS_PHY_TS_RX_TIMESTAMP_POS_IN_PTP, VTSS_PHY_TS_RX_TIMESTAMP_LEN_30BIT, VTSS_PHY_TS_FIFO_MODE_SPI, VTSS_PHY_TS_FIFO_TIMESTAMP_LEN_10BYTE};

    tod_phy_init.tc_op_mode = VTSS_PHY_TS_TC_OP_MODE_B;
#endif

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
#ifndef VTSS_FEATURE_PTP_DELAY_COMP_ENGINE
                if (has_clk_freq) {
                    tod_phy_init.clk_freq = (vtss_phy_ts_clockfreq_t)clk_freq;
                }

                if (has_clk_src) {
                    tod_phy_init.clk_src = (vtss_phy_ts_clock_src_t)clk_src;
                }

                if (has_rx_ts_pos) {
                    tod_phy_init.rx_ts_pos = (vtss_phy_ts_rxtimestamp_pos_t)rx_ts_pos;
                }

                if (has_tx_fifo_mode) {
                    tod_phy_init.tx_fifo_mode = (vtss_phy_ts_fifo_mode_t)tx_fifo_mode;
                }

                if (has_tx_fifo_spi_conf) {
                    tod_phy_init.tx_fifo_spi_conf = TRUE;
                    if (has_tx_fifo_hi_clk_cycs) {
                        tod_phy_init.tx_fifo_hi_clk_cycs = hi_clk_value;
                    } else {
                        tod_phy_init.tx_fifo_hi_clk_cycs = 2;
                    }

                    if (has_tx_fifo_lo_clk_cycs) {
                        tod_phy_init.tx_fifo_lo_clk_cycs = lo_clk_value;
                    } else {
                        tod_phy_init.tx_fifo_lo_clk_cycs = 2;
                    }
                }

                if (has_modify_frm) {
                    tod_phy_init.chk_ing_modified = modify_frm;
                }
#endif
#ifdef VTSS_FEATURE_PTP_DELAY_COMP_ENGINE
                rc = vtss_phy_dce_init(API_INST_DEFAULT, port_no);
#else
                rc = vtss_phy_ts_init(API_INST_DEFAULT, port_no, &tod_phy_init);
#endif
                if (rc == VTSS_RC_OK) {
                    ICLI_PRINTF("PHY TS Init Success\n");
                } else {
                    ICLI_PRINTF("PHY TS Init Failed!\n");
                    return;
                }

                rc = vtss_phy_ts_mode_set(API_INST_DEFAULT, port_no, TRUE);
                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("PHY TS Block Enable Failed\n");
                }
            }
        }
    }
}

mesa_rc misc_icli_ts_asym_delay(i32 session_id, BOOL has_asym, i32 value, icli_stack_port_range_t *plist)
{
    mesa_port_no_t             port_no;
    u8 range_idx = 0, cnt_idx = 0, uport = 0;
    BOOL error = FALSE;
    mesa_timeinterval_t        asym_val;
    i32 tmp = 0;

    if (plist) {
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;
                port_no = uport2iport(uport);
                if (has_asym) {
                    asym_val = (mesa_timeinterval_t) value ;
                    //asym_val = asym_val << 16;
                    if (vtss_phy_ts_delay_asymmetry_set(misc_phy_inst_get(), port_no, &asym_val) != VTSS_RC_OK) {
                        ICLI_PRINTF("Failed to set the Delay Asymmetry for the port %d\n", uport);
                        error = TRUE;
                    } else {
                        ICLI_PRINTF("Successfully set the delay asymmetry\n");
                    }
                } else {
                    if (vtss_phy_ts_delay_asymmetry_get(misc_phy_inst_get(), port_no, &asym_val) != VTSS_RC_OK) {
                        ICLI_PRINTF("Failed to get the asymmetry delay for the port %d\n", uport);
                        error = TRUE;
                    } else {
                        //asym_val = asym_val >> 16;
                        tmp = (i32) asym_val;
                        ICLI_PRINTF("Port  DelayAsym\n----  ---------\n");
                        ICLI_PRINTF("%-4d  %d\n", uport, tmp);
                    }
                }
            }
        }
    }

    return error;
}

mesa_rc misc_icli_ts_path_delay(u32 session_id, BOOL has_delay, mesa_timeinterval_t value, icli_stack_port_range_t *plist)
{
    mesa_port_no_t             port_no;
    u8 range_idx = 0, cnt_idx = 0, uport = 0;
    BOOL error = FALSE;
    mesa_timeinterval_t        delay_val;

    if (plist) {
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;
                port_no = uport2iport(uport);
                if (has_delay) {
                    if (vtss_phy_ts_path_delay_set(misc_phy_inst_get(), port_no, &value) != VTSS_RC_OK)  {
                        ICLI_PRINTF("Failed to set the port delay for the port %d\n", uport);
                        error = TRUE;
                    } else {
                        ICLI_PRINTF("Successfully set the port delay..\n");
                    }
                } else {
                    if (vtss_phy_ts_path_delay_get(misc_phy_inst_get(), port_no, &delay_val) != VTSS_RC_OK)  {
                        ICLI_PRINTF("Failed to get the port delay for the port %d\n", uport);
                        error = TRUE;
                    } else {
                        //temp = (u32)(delay_val >> 16);
                        ICLI_PRINTF("Port  PathDelay\n----  ---------\n");
                        //ICLI_PRINTF("%-4d  %-u\n", uport, temp);
                        ICLI_PRINTF("%-4d  %-u\n", uport, (u32)delay_val);
                    }
                }
            }
        }
    }

    return error;
}

mesa_rc misc_phy_icli_cmd_tod_time(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_time_sec, u8 v_0_to_255)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_timestamp_t ts;
    mesa_rc rc = VTSS_RC_ERROR;

    memset(&ts, 0, sizeof(vtss_phy_timestamp_t));
    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                if (has_time_sec) {
                    ts.seconds.high = 0;
                    ts.seconds.low = v_0_to_255;
                    ts.nanoseconds = 0;
                    if (VTSS_RC_OK != (rc = vtss_phy_ts_ptptime_set(API_INST_DEFAULT, port_no, &ts))) {
                        ICLI_PRINTF("Error..! vtss_phy_ts_ptptime_set\n");
                    }

                    VTSS_OS_MSLEEP(1000);
                    if (VTSS_RC_OK != (rc = vtss_phy_ts_ptptime_set_done(API_INST_DEFAULT, port_no))) {
                        ICLI_PRINTF("Error..! vtss_phy_ts_ptptime_set_done\n");
                    }
                } else {
                    if (VTSS_RC_OK != (rc = vtss_phy_ts_ptptime_arm(API_INST_DEFAULT, port_no))) {
                        ICLI_PRINTF("Error..! vtss_phy_ts_ptptime_arm\n");
                    }

                    VTSS_OS_MSLEEP(1000);
                    if (VTSS_RC_OK != (rc = vtss_phy_ts_ptptime_get(API_INST_DEFAULT, port_no, &ts))) {
                        ICLI_PRINTF("Error..! vtss_phy_ts_ptptime_get\n");
                    }

                    ICLI_PRINTF("PTP Time Get::\n");
                    ICLI_PRINTF("    Seconds High::%u\n", ts.seconds.high);
                    ICLI_PRINTF("    Seconds Low::%u\n", ts.seconds.low);
                    ICLI_PRINTF("    Nano Seconds:: %u\n", ts.nanoseconds);
                }
            }
        }
    }

    return rc;
}

void misc_icli_deb_phy_ts_mmd_read(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 blk_id, u16 address)
{
    u32 range_idx, cnt_idx, value;
    mesa_port_no_t uport, port_no;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                if (vtss_phy_1588_csr_reg_read(PHY_INST, port_no, blk_id,
                                               address, &value) != VTSS_RC_OK) {
                    ICLI_PRINTF("Could not perform vtss_phy_1588_csr_reg_read() operation \n");
                    return;
                }

                ICLI_PRINTF("%-12s %-12s %-12s %-12s \n", "Port", "Blk-Id", "CSR-Offset", "Value");
                ICLI_PRINTF("%-12s %-12s %-12s %-12s \n", "----", "------", "----------", "-----");
                ICLI_PRINTF("%-12u %-12u %-12x %-12x\n", port_no, blk_id, address, value);
            }
        }
    }
}

void misc_icli_deb_phy_ts_mmd_write(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 blk_id, u16 address, u32 value)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                if (vtss_phy_1588_csr_reg_write(NULL, port_no, blk_id,
                                                address, &value) != VTSS_RC_OK) {
                    ICLI_PRINTF("Could not perform vtss_phy_1588_csr_reg_write() operation \n");
                    return;
                }
            }
        }
    }
}

void misc_icli_deb_phy_ts_engine_init(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 engine_id, BOOL ingress, u8 encap_type, u8 flow_st_index, u8 flow_end_index, u8 flow_match)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport;
    vtss_phy_ts_engine_t eng_id = (vtss_phy_ts_engine_t)engine_id;
    vtss_phy_ts_encap_t encap = (vtss_phy_ts_encap_t) encap_type;
    vtss_phy_ts_engine_flow_match_t flow_match_mode = (vtss_phy_ts_engine_flow_match_t)flow_match;
    mesa_rc rc;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_init(API_INST_DEFAULT, uport2iport(uport),
                                                         eng_id, encap, flow_st_index, flow_end_index,
                                                         flow_match_mode);
                } else {
                    rc = vtss_phy_ts_egress_engine_init(API_INST_DEFAULT, uport2iport(uport),
                                                        (vtss_phy_ts_engine_t)eng_id, (vtss_phy_ts_encap_t) encap, flow_st_index, flow_end_index,
                                                        (vtss_phy_ts_engine_flow_match_t) flow_match_mode);
                }

                if (rc == VTSS_RC_OK) {
                    ICLI_PRINTF("Engine init Success\n");
                } else {
                    ICLI_PRINTF("Engine init Failed\n");
                }
            }
        }
    }
}

void misc_icli_deb_phy_ts_engine_clear(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    mesa_rc rc = VTSS_RC_OK;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_clear(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id);
                } else {
                    rc = vtss_phy_ts_egress_engine_clear(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id);
                }
            }
        }
    }

    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Engine cleared\n");
    }
}

void misc_icli_deb_phy_ts_correction_field_clear(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 action_id, u8 msg_type)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                ICLI_PRINTF("CF clear being attempted on port %u \n", port_no);
                ICLI_PRINTF(" engine id %u, ingress %s, action %u , ptp msg type %u \n",
                            eng_id, ingress ? "TRUE" : "FALSE", action_id, msg_type);
                if (vtss_phy_ts_flow_clear_cf_set(PHY_INST, port_no, ingress, (vtss_phy_ts_engine_t)eng_id, action_id, (vtss_phy_ts_ptp_message_type_t)msg_type) != VTSS_RC_OK) {
                    ICLI_PRINTF(" vtss_phy_ts_flow_clear_cf_set on port %u failed \n", port_no);
                } else {
                    ICLI_PRINTF(" vtss_phy_ts_flow_clear_cf_set on port %u success \n", port_no);
                }
            }
        }
    }
}

void misc_icli_deb_phy_ts_engine_mode(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL enable)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                conf_ptr = flow_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Engine configuration does not exist port=%d engine=%d\n", port_no, eng_id);
                    continue;
                }

                flow_conf->eng_mode = enable;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Engine mode set failed for port=%d engine=%d\n", port_no, eng_id);
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_ts_engine_chan_map(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_flow, u8 flow_id, BOOL has_mask, u8 mask)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_eng_init_conf_t *eng_conf, *conf_eng;
    u8 flow_st_index, flow_end_index, i;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    eng_conf = (vtss_phy_ts_eng_init_conf_t *)VTSS_MALLOC(sizeof(vtss_phy_ts_eng_init_conf_t));
    if (eng_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        VTSS_FREE(flow_conf);
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                conf_ptr = flow_conf;
                conf_eng = eng_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_init_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_eng);
                    if (rc != VTSS_RC_OK) {
                        ICLI_PRINTF("Could not get engine init config\n");
                        VTSS_FREE(eng_conf);
                        VTSS_FREE(flow_conf);
                        return;
                    }

                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_init_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_eng);
                    if (rc != VTSS_RC_OK) {
                        ICLI_PRINTF("Could not get engine init config\n");
                        VTSS_FREE(eng_conf);
                        VTSS_FREE(flow_conf);
                        return;
                    }

                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Engine configuration does not exist port=%d engine=%d\n", port_no, eng_id);
                    VTSS_FREE(eng_conf);
                    VTSS_FREE(flow_conf);
                    return;
                }

                if (has_flow) {
                    flow_conf->channel_map[flow_id] = has_mask ? mask : 0;
                } else {
                    flow_st_index = eng_conf->flow_st_index;
                    flow_end_index = eng_conf->flow_end_index;
                    for (i = flow_st_index; i <= flow_end_index; i++) {
                        flow_conf->channel_map[i] = has_mask ? mask : 0;
                    }
                }

                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed!\n");
                    VTSS_FREE(eng_conf);
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(eng_conf);
    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    }
}

void misc_icli_ts_engine_eth1_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_pbb, BOOL pbb_en, BOOL has_etype, u16 etype, BOOL has_tpid, u16 tpid)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_eth_conf_t *eth1_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                if (eng_id < VTSS_PHY_TS_OAM_ENGINE_ID_2A) {
                    eth1_conf = &flow_conf->flow_conf.ptp.eth1_opt;
                } else {
                    eth1_conf = &flow_conf->flow_conf.oam.eth1_opt;
                }

                conf_ptr = flow_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc == VTSS_RC_OK) {
                    eth1_conf->comm_opt.pbb_en = has_pbb ? pbb_en : eth1_conf->comm_opt.pbb_en;
                    eth1_conf->comm_opt.etype = (has_etype ? etype : eth1_conf->comm_opt.etype);
                    eth1_conf->comm_opt.tpid = (has_tpid ? tpid : eth1_conf->comm_opt.tpid);
                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed\n");
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_deb_phy_ts_engine_eth1_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, BOOL has_mac_match_mode, u8 mac_match_mode, BOOL has_address, mesa_mac_t mac_addr, BOOL has_match_addr_types, u8 src_dest_match, BOOL has_vlan_chk, BOOL vlan_chk, BOOL has_tag_rng, u8 tag_rng, u8 has_num_tag, u8 num_tag, BOOL has_tag1_type, u8 tag1_type, BOOL has_tag2_type, u8 tag2_type, BOOL has_tag1_low, u16 tag1_low, BOOL has_tag1_up, u16 tag1_up, BOOL has_tag2_low, u16 tag2_low, BOOL has_tag2_up, u16 tag2_up)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_eth_conf_t *eth1_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                if (eng_id < VTSS_PHY_TS_OAM_ENGINE_ID_2A) {
                    eth1_conf = &flow_conf->flow_conf.ptp.eth1_opt;
                } else {
                    eth1_conf = &flow_conf->flow_conf.oam.eth1_opt;
                }

                conf_ptr = flow_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc == VTSS_RC_OK) {
                    eth1_conf->flow_opt[flow_id].flow_en = enable;
                    if (has_mac_match_mode) {
                        eth1_conf->flow_opt[flow_id].addr_match_mode = mac_match_mode;
                    }

                    if (has_address) {
                        memcpy((void *)eth1_conf->flow_opt[flow_id].mac_addr, (void *)mac_addr.addr, 6);
                    }

                    if (has_match_addr_types) {
                        eth1_conf->flow_opt[flow_id].addr_match_select = (src_dest_match == 0) ? VTSS_PHY_TS_ETH_MATCH_SRC_ADDR :
                                                                         ((src_dest_match == 1) ? VTSS_PHY_TS_ETH_MATCH_DEST_ADDR :
                                                                          VTSS_PHY_TS_ETH_MATCH_SRC_OR_DEST);
                    }

                    if (has_vlan_chk) {
                        eth1_conf->flow_opt[flow_id].vlan_check = vlan_chk;
                    }

                    if (has_num_tag) {
                        eth1_conf->flow_opt[flow_id].num_tag = num_tag;
                    }

                    if (has_tag1_type) {
                        eth1_conf->flow_opt[flow_id].outer_tag_type = tag1_type;
                    }

                    if (has_tag2_type) {
                        eth1_conf->flow_opt[flow_id].inner_tag_type = tag2_type;
                    }

                    if (has_tag_rng && tag_rng == VTSS_PHY_TS_TAG_RANGE_OUTER) {
                        if (has_tag1_low) {
                            eth1_conf->flow_opt[flow_id].outer_tag.range.lower = tag1_low;
                        }

                        if (has_tag1_up) {
                            eth1_conf->flow_opt[flow_id].outer_tag.range.upper = tag1_up;
                        }
                    } else {
                        eth1_conf->flow_opt[flow_id].outer_tag.value.val = tag1_low;
                        eth1_conf->flow_opt[flow_id].outer_tag.value.mask = tag1_up;
                    }

                    if (has_tag_rng && tag_rng == VTSS_PHY_TS_TAG_RANGE_INNER) {
                        if (has_num_tag && num_tag == 1) {
                            if (has_tag1_low) {
                                eth1_conf->flow_opt[flow_id].inner_tag.range.lower = tag1_low;
                            }

                            if (has_tag1_up) {
                                eth1_conf->flow_opt[flow_id].inner_tag.range.upper = tag1_up;
                            }
                        }

                        if (has_tag2_low) {
                            eth1_conf->flow_opt[flow_id].inner_tag.range.lower = tag2_low;
                        }

                        if (has_tag2_up) {
                            eth1_conf->flow_opt[flow_id].inner_tag.range.upper = tag2_up;
                        }
                    } else {
                        eth1_conf->flow_opt[flow_id].inner_tag.value.val = tag1_low;
                        eth1_conf->flow_opt[flow_id].inner_tag.value.mask = tag1_up;
                    }

                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Flow-conf for eth1 Failed on port %d engine %d\n", port_no, eng_id);
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_ts_engine_eth2_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_etype, u16 etype, BOOL has_tpid, u16 tpid)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_eth_conf_t *eth2_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                if (eng_id < VTSS_PHY_TS_OAM_ENGINE_ID_2A) {
                    eth2_conf = &flow_conf->flow_conf.ptp.eth2_opt;
                } else {
                    eth2_conf = &flow_conf->flow_conf.oam.eth2_opt;
                }

                conf_ptr = flow_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc == VTSS_RC_OK) {
                    eth2_conf->comm_opt.etype = (has_etype ? etype : eth2_conf->comm_opt.etype);
                    eth2_conf->comm_opt.tpid = (has_tpid ? tpid : eth2_conf->comm_opt.tpid);
                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed\n");
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_deb_phy_ts_engine_eth2_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, BOOL has_mac_match_mode, u8 mac_match_mode, BOOL has_address, mesa_mac_t mac_addr, BOOL has_match_addr_types, u8 src_dest_match, BOOL has_vlan_chk, BOOL vlan_chk, BOOL has_tag_rng, u8 tag_rng, u8 has_num_tag, u8 num_tag, BOOL has_tag1_type, u8 tag1_type, BOOL has_tag2_type, u8 tag2_type, BOOL has_tag1_low, u16 tag1_low, BOOL has_tag1_up, u16 tag1_up, BOOL has_tag2_low, u16 tag2_low, BOOL has_tag2_up, u16 tag2_up)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_eth_conf_t *eth2_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                if (eng_id < VTSS_PHY_TS_OAM_ENGINE_ID_2A) {
                    eth2_conf = &flow_conf->flow_conf.ptp.eth2_opt;
                } else {
                    eth2_conf = &flow_conf->flow_conf.oam.eth2_opt;
                }

                conf_ptr = flow_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc == VTSS_RC_OK) {
                    eth2_conf->flow_opt[flow_id].flow_en = enable;
                    if (has_mac_match_mode) {
                        eth2_conf->flow_opt[flow_id].addr_match_mode = mac_match_mode;
                    }

                    if (has_address) {
                        memcpy((void *)eth2_conf->flow_opt[flow_id].mac_addr, (void *)mac_addr.addr, 6);
                    }

                    if (has_match_addr_types) {
                        eth2_conf->flow_opt[flow_id].addr_match_select = (src_dest_match == 0) ? VTSS_PHY_TS_ETH_MATCH_SRC_ADDR :
                                                                         ((src_dest_match == 1) ? VTSS_PHY_TS_ETH_MATCH_DEST_ADDR :
                                                                          VTSS_PHY_TS_ETH_MATCH_SRC_OR_DEST);
                    }

                    if (has_vlan_chk) {
                        eth2_conf->flow_opt[flow_id].vlan_check = vlan_chk;
                    }

                    if (has_num_tag) {
                        eth2_conf->flow_opt[flow_id].num_tag = num_tag;
                    }

                    if (has_tag1_type) {
                        eth2_conf->flow_opt[flow_id].outer_tag_type = tag1_type;
                    }

                    if (has_tag2_type) {
                        eth2_conf->flow_opt[flow_id].inner_tag_type = tag2_type;
                    }

                    if (has_tag_rng && tag_rng == VTSS_PHY_TS_TAG_RANGE_OUTER) {
                        if (has_tag1_low) {
                            eth2_conf->flow_opt[flow_id].outer_tag.range.lower = tag1_low;
                        }

                        if (has_tag1_up) {
                            eth2_conf->flow_opt[flow_id].outer_tag.range.upper = tag1_up;
                        }
                    } else {
                        eth2_conf->flow_opt[flow_id].outer_tag.value.val = tag1_low;
                        eth2_conf->flow_opt[flow_id].outer_tag.value.mask = tag1_up;
                    }

                    if (has_tag_rng && tag_rng == VTSS_PHY_TS_TAG_RANGE_INNER) {
                        if (has_num_tag && num_tag == 1) {
                            if (has_tag1_low) {
                                eth2_conf->flow_opt[flow_id].inner_tag.range.lower = tag1_low;
                            }

                            if (has_tag1_up) {
                                eth2_conf->flow_opt[flow_id].inner_tag.range.upper = tag1_up;
                            }
                        }

                        if (has_tag2_low) {
                            eth2_conf->flow_opt[flow_id].inner_tag.range.lower = tag2_low;
                        }

                        if (has_tag2_up) {
                            eth2_conf->flow_opt[flow_id].inner_tag.range.upper = tag2_up;
                        }
                    } else {
                        eth2_conf->flow_opt[flow_id].inner_tag.value.val = tag1_low;
                        eth2_conf->flow_opt[flow_id].inner_tag.value.mask = tag1_up;
                    }

                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Flow-conf for eth2 Failed on port %d engine %d\n", port_no, eng_id);
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_deb_phy_ts_engine_ip1_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_ipmode, u8 ip_mode, u16 sport_val, u16 sport_mask, u16 dport_val, u16 dport_mask)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_ip_conf_t *ip1_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                ip1_conf = &flow_conf->flow_conf.ptp.ip1_opt;
                conf_ptr = flow_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc == VTSS_RC_OK) {
                    if (has_ipmode) {
                        ip1_conf->comm_opt.ip_mode = ip_mode;
                    }

                    ip1_conf->comm_opt.sport_val = sport_val;
                    ip1_conf->comm_opt.sport_mask = sport_mask;
                    ip1_conf->comm_opt.dport_val = dport_val;
                    ip1_conf->comm_opt.dport_mask = dport_mask;
                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed\n");
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_deb_phy_ts_ip1_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, BOOL has_match_addr_types, u8 src_dest_match, BOOL has_ipv4, mesa_ipv4_t ipv4_addr, mesa_ipv4_t ipv4_mask, BOOL has_ipv6, mesa_ipv6_t ipv6_addr, mesa_ipv6_t ipv6_mask)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_ip_conf_t *ip1_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                ip1_conf = &flow_conf->flow_conf.ptp.ip1_opt;
                conf_ptr = flow_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc == VTSS_RC_OK) {
                    ip1_conf->flow_opt[flow_id].flow_en = enable;
                    if (has_match_addr_types) {
                        ip1_conf->flow_opt[flow_id].match_mode = src_dest_match;
                    }

                    if (ip1_conf->comm_opt.ip_mode == VTSS_PHY_TS_IP_VER_4) {
                        if (has_ipv4) {
                            ip1_conf->flow_opt[flow_id].ip_addr.ipv4.addr = ipv4_addr;
                            ip1_conf->flow_opt[flow_id].ip_addr.ipv4.mask = ipv4_mask;
                        }
                    } else {
                        if (has_ipv6) {
                            ip1_conf->flow_opt[flow_id].ip_addr.ipv6.addr[3] = ipv6_addr.addr[0] << 24 | ipv6_addr.addr[1] << 16 | ipv6_addr.addr[2] << 8 | ipv6_addr.addr[3];
                            ip1_conf->flow_opt[flow_id].ip_addr.ipv6.addr[2] = ipv6_addr.addr[4] << 24 | ipv6_addr.addr[5] << 16 | ipv6_addr.addr[6] << 8 | ipv6_addr.addr[7];
                            ip1_conf->flow_opt[flow_id].ip_addr.ipv6.addr[1] = ipv6_addr.addr[8] << 24 | ipv6_addr.addr[9] << 16 | ipv6_addr.addr[10] << 8 | ipv6_addr.addr[11];
                            ip1_conf->flow_opt[flow_id].ip_addr.ipv6.addr[0] = ipv6_addr.addr[12] << 24 | ipv6_addr.addr[13] << 16 | ipv6_addr.addr[14] << 8 | ipv6_addr.addr[15];
                            ip1_conf->flow_opt[flow_id].ip_addr.ipv6.mask[3] = ipv6_mask.addr[0] << 24 | ipv6_mask.addr[1] << 16 | ipv6_mask.addr[2] << 8 | ipv6_mask.addr[3];
                            ip1_conf->flow_opt[flow_id].ip_addr.ipv6.mask[2] = ipv6_mask.addr[4] << 24 | ipv6_mask.addr[5] << 16 | ipv6_mask.addr[6] << 8 | ipv6_mask.addr[7];
                            ip1_conf->flow_opt[flow_id].ip_addr.ipv6.mask[1] = ipv6_mask.addr[8] << 24 | ipv6_mask.addr[9] << 16 | ipv6_mask.addr[10] << 8 | ipv6_mask.addr[11];
                            ip1_conf->flow_opt[flow_id].ip_addr.ipv6.mask[0] = ipv6_mask.addr[12] << 24 | ipv6_mask.addr[13] << 16 | ipv6_mask.addr[14] << 8 | ipv6_mask.addr[15];
                        }
                    }

                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed\n");
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_deb_phy_ts_engine_ip2_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_ipmode, u8 ip_mode, u16 sport_val, u16 sport_mask, u16 dport_val, u16 dport_mask)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_ip_conf_t *ip2_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                ip2_conf = &flow_conf->flow_conf.ptp.ip2_opt;
                conf_ptr = flow_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc == VTSS_RC_OK) {
                    if (has_ipmode) {
                        ip2_conf->comm_opt.ip_mode = ip_mode;
                    }

                    ip2_conf->comm_opt.sport_val = sport_val;
                    ip2_conf->comm_opt.sport_mask = sport_mask;
                    ip2_conf->comm_opt.dport_val = dport_val;
                    ip2_conf->comm_opt.dport_mask = dport_mask;
                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed\n");
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_deb_phy_ts_ip2_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, BOOL has_match_addr_types, u8 src_dest_match, BOOL has_ipv4, mesa_ipv4_t ipv4_addr, mesa_ipv4_t ipv4_mask, BOOL has_ipv6, mesa_ipv6_t ipv6_addr, mesa_ipv6_t ipv6_mask)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_ip_conf_t *ip2_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                ip2_conf = &flow_conf->flow_conf.ptp.ip2_opt;
                conf_ptr = flow_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc == VTSS_RC_OK) {
                    ip2_conf->flow_opt[flow_id].flow_en = enable;
                    if (has_match_addr_types) {
                        ip2_conf->flow_opt[flow_id].match_mode = src_dest_match;
                    }

                    if (ip2_conf->comm_opt.ip_mode == VTSS_PHY_TS_IP_VER_4) {
                        if (has_ipv4) {
                            ip2_conf->flow_opt[flow_id].ip_addr.ipv4.addr = ipv4_addr;
                            ip2_conf->flow_opt[flow_id].ip_addr.ipv4.mask = ipv4_mask;
                        }
                    } else {
                        if (has_ipv6) {
                            ip2_conf->flow_opt[flow_id].ip_addr.ipv6.addr[3] = ipv6_addr.addr[0] << 24 | ipv6_addr.addr[1] << 16 | ipv6_addr.addr[2] << 8 | ipv6_addr.addr[3];
                            ip2_conf->flow_opt[flow_id].ip_addr.ipv6.addr[2] = ipv6_addr.addr[4] << 24 | ipv6_addr.addr[5] << 16 | ipv6_addr.addr[6] << 8 | ipv6_addr.addr[7];
                            ip2_conf->flow_opt[flow_id].ip_addr.ipv6.addr[1] = ipv6_addr.addr[8] << 24 | ipv6_addr.addr[9] << 16 | ipv6_addr.addr[10] << 8 | ipv6_addr.addr[11];
                            ip2_conf->flow_opt[flow_id].ip_addr.ipv6.addr[0] = ipv6_addr.addr[12] << 24 | ipv6_addr.addr[13] << 16 | ipv6_addr.addr[14] << 8 | ipv6_addr.addr[15];
                            ip2_conf->flow_opt[flow_id].ip_addr.ipv6.mask[3] = ipv6_mask.addr[0] << 24 | ipv6_mask.addr[1] << 16 | ipv6_mask.addr[2] << 8 | ipv6_mask.addr[3];
                            ip2_conf->flow_opt[flow_id].ip_addr.ipv6.mask[2] = ipv6_mask.addr[4] << 24 | ipv6_mask.addr[5] << 16 | ipv6_mask.addr[6] << 8 | ipv6_mask.addr[7];
                            ip2_conf->flow_opt[flow_id].ip_addr.ipv6.mask[1] = ipv6_mask.addr[8] << 24 | ipv6_mask.addr[9] << 16 | ipv6_mask.addr[10] << 8 | ipv6_mask.addr[11];
                            ip2_conf->flow_opt[flow_id].ip_addr.ipv6.mask[0] = ipv6_mask.addr[12] << 24 | ipv6_mask.addr[13] << 16 | ipv6_mask.addr[14] << 8 | ipv6_mask.addr[15];
                        }
                    }

                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed\n");
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_deb_phy_ts_mpls_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL cw_en)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_eng_init_conf_t *eng_conf, *conf_eng;
    vtss_phy_ts_mpls_conf_t *mpls_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    eng_conf = (vtss_phy_ts_eng_init_conf_t *)VTSS_MALLOC(sizeof(vtss_phy_ts_eng_init_conf_t));
    if (eng_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        VTSS_FREE(flow_conf);
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                conf_ptr = flow_conf;
                conf_eng = eng_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_init_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_eng);
                    if (rc != VTSS_RC_OK) {
                        ICLI_PRINTF("Could not get engine init config\n");
                        VTSS_FREE(eng_conf);
                        VTSS_FREE(flow_conf);
                        return;
                    }

                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_init_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_eng);
                    if (rc != VTSS_RC_OK) {
                        ICLI_PRINTF("Could not get engine init config\n");
                        VTSS_FREE(eng_conf);
                        VTSS_FREE(flow_conf);
                        return;
                    }

                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Engine configuration does not exist port=%d engine=%d\n", port_no, eng_id);
                    VTSS_FREE(eng_conf);
                    VTSS_FREE(flow_conf);
                    return;
                }

                if (rc == VTSS_RC_OK) {
                    if (eng_conf->encap_type == VTSS_PHY_TS_ENCAP_ETH_MPLS_ETH_OAM ||
                        eng_conf->encap_type == VTSS_PHY_TS_ENCAP_ETH_MPLS_ACH_OAM) {
                        mpls_conf = &flow_conf->flow_conf.oam.mpls_opt;
                    } else {
                        mpls_conf = &flow_conf->flow_conf.ptp.mpls_opt;
                    }

                    if (mpls_conf->comm_opt.cw_en == cw_en) {
                        VTSS_FREE(flow_conf);
                        VTSS_FREE(eng_conf);
                        ICLI_PRINTF("Success\n");
                        return;
                    }

                    mpls_conf->comm_opt.cw_en = cw_en;
                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed\n");
                    VTSS_FREE(flow_conf);
                    VTSS_FREE(eng_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    VTSS_FREE(eng_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_deb_phy_ts_engine_mpls_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, u8 stk_depth, BOOL has_stk_ref, u8 stk_ref_point, BOOL has_stk_lvl_0, icli_unsigned_range_t *stk_lvl_0, BOOL has_stk_lvl_1, icli_unsigned_range_t *stk_lvl_1, BOOL has_stk_lvl_2, icli_unsigned_range_t *stk_lvl_2, BOOL has_stk_lvl_3, icli_unsigned_range_t *stk_lvl_3)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_eng_init_conf_t *eng_conf = NULL, *conf_eng;
    vtss_phy_ts_mpls_conf_t *mpls_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    eng_conf = (vtss_phy_ts_eng_init_conf_t *)VTSS_MALLOC(sizeof(vtss_phy_ts_eng_init_conf_t));
    if (eng_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        VTSS_FREE(flow_conf);
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                conf_ptr = flow_conf;
                conf_eng = eng_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_init_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_eng);
                    if (rc != VTSS_RC_OK) {
                        ICLI_PRINTF("Could not get engine init config\n");
                        VTSS_FREE(eng_conf);
                        VTSS_FREE(flow_conf);
                        return;
                    }

                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_init_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_eng);
                    if (rc != VTSS_RC_OK) {
                        ICLI_PRINTF("Could not get engine init config\n");
                        VTSS_FREE(eng_conf);
                        VTSS_FREE(flow_conf);
                        return;
                    }

                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Engine configuration does not exist port=%d engine=%d\n", port_no, eng_id);
                    VTSS_FREE(eng_conf);
                    VTSS_FREE(flow_conf);
                    return;
                }

                if (rc == VTSS_RC_OK) {
                    if (eng_conf->encap_type == VTSS_PHY_TS_ENCAP_ETH_MPLS_ETH_OAM ||
                        eng_conf->encap_type == VTSS_PHY_TS_ENCAP_ETH_MPLS_ACH_OAM) {
                        mpls_conf = &flow_conf->flow_conf.oam.mpls_opt;
                    } else {
                        mpls_conf = &flow_conf->flow_conf.ptp.mpls_opt;
                    }

                    mpls_conf->flow_opt[flow_id].flow_en = enable;
                    mpls_conf->flow_opt[flow_id].stack_depth = stk_depth;
                    if (has_stk_ref) {
                        mpls_conf->flow_opt[flow_id].stack_ref_point = stk_ref_point;
                    }

                    if (has_stk_lvl_0) {
                        if (mpls_conf->flow_opt[flow_id].stack_ref_point == VTSS_PHY_TS_MPLS_STACK_REF_POINT_TOP) {
                            mpls_conf->flow_opt[flow_id].stack_level.top_down.top.lower = stk_lvl_0 ? stk_lvl_0->range[0].min : 0;
                            mpls_conf->flow_opt[flow_id].stack_level.top_down.top.upper =  stk_lvl_0 ? stk_lvl_0->range[0].max : 0;
                        } else {
                            mpls_conf->flow_opt[flow_id].stack_level.bottom_up.end.lower = stk_lvl_0 ? stk_lvl_0->range[0].min : 0;
                            mpls_conf->flow_opt[flow_id].stack_level.bottom_up.end.upper = stk_lvl_0 ? stk_lvl_0->range[0].max : 0;
                        }
                    }

                    if (has_stk_lvl_1) {
                        if (mpls_conf->flow_opt[flow_id].stack_ref_point == VTSS_PHY_TS_MPLS_STACK_REF_POINT_TOP) {
                            mpls_conf->flow_opt[flow_id].stack_level.top_down.frst_lvl_after_top.lower = stk_lvl_1 ? stk_lvl_1->range[0].min : 0;
                            mpls_conf->flow_opt[flow_id].stack_level.top_down.frst_lvl_after_top.upper = stk_lvl_1 ? stk_lvl_1->range[0].max : 0;
                        } else {
                            mpls_conf->flow_opt[flow_id].stack_level.bottom_up.frst_lvl_before_end.lower = stk_lvl_1 ? stk_lvl_1->range[0].min : 0;
                            mpls_conf->flow_opt[flow_id].stack_level.bottom_up.frst_lvl_before_end.upper = stk_lvl_1 ? stk_lvl_1->range[0].max : 0;
                        }
                    }

                    if (has_stk_lvl_2) {
                        if (mpls_conf->flow_opt[flow_id].stack_ref_point == VTSS_PHY_TS_MPLS_STACK_REF_POINT_TOP) {
                            mpls_conf->flow_opt[flow_id].stack_level.top_down.snd_lvl_after_top.lower = stk_lvl_2 ? stk_lvl_2->range[0].min : 0;
                            mpls_conf->flow_opt[flow_id].stack_level.top_down.snd_lvl_after_top.upper = stk_lvl_2 ? stk_lvl_2->range[0].max : 0;
                        } else {
                            mpls_conf->flow_opt[flow_id].stack_level.bottom_up.snd_lvl_before_end.lower = stk_lvl_2 ? stk_lvl_2->range[0].min : 0;
                            mpls_conf->flow_opt[flow_id].stack_level.bottom_up.snd_lvl_before_end.upper = stk_lvl_2 ? stk_lvl_2->range[0].max : 0;
                        }
                    }

                    if (has_stk_lvl_3) {
                        if (mpls_conf->flow_opt[flow_id].stack_ref_point == VTSS_PHY_TS_MPLS_STACK_REF_POINT_TOP) {
                            mpls_conf->flow_opt[flow_id].stack_level.top_down.thrd_lvl_after_top.lower = stk_lvl_3 ? stk_lvl_3->range[0].min : 0;
                            mpls_conf->flow_opt[flow_id].stack_level.top_down.thrd_lvl_after_top.upper = stk_lvl_3 ? stk_lvl_3->range[0].max : 0;
                        } else {
                            mpls_conf->flow_opt[flow_id].stack_level.bottom_up.thrd_lvl_before_end.lower = stk_lvl_3 ? stk_lvl_3->range[0].min : 0;
                            mpls_conf->flow_opt[flow_id].stack_level.bottom_up.thrd_lvl_before_end.upper = stk_lvl_3 ? stk_lvl_3->range[0].max : 0;
                        }
                    }

                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed!\n");
                    VTSS_FREE(eng_conf);
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(eng_conf);
    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    }
}

void misc_icli_deb_phy_ts_engine_ach_comm_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u16 ach_ver, u16 channel_type, u16 proto_id)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_eng_init_conf_t *eng_conf = NULL, *conf_eng;
    vtss_phy_ts_ach_conf_t *ach_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    eng_conf = (vtss_phy_ts_eng_init_conf_t *)VTSS_MALLOC(sizeof(vtss_phy_ts_eng_init_conf_t));
    if (eng_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        VTSS_FREE(flow_conf);
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                conf_ptr = flow_conf;
                conf_eng = eng_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_init_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_eng);
                    if (rc != VTSS_RC_OK) {
                        ICLI_PRINTF("Could not get engine init config\n");
                        VTSS_FREE(eng_conf);
                        VTSS_FREE(flow_conf);
                        return;
                    }

                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_init_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_eng);
                    if (rc != VTSS_RC_OK) {
                        ICLI_PRINTF("Could not get engine init config\n");
                        VTSS_FREE(eng_conf);
                        VTSS_FREE(flow_conf);
                        return;
                    }

                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Engine configuration does not exist port=%d engine=%d\n", port_no, eng_id);
                    VTSS_FREE(eng_conf);
                    VTSS_FREE(flow_conf);
                    return;
                }

                if (rc == VTSS_RC_OK) {
                    if (eng_conf->encap_type == VTSS_PHY_TS_ENCAP_ETH_MPLS_ACH_OAM) {
                        ach_conf = &flow_conf->flow_conf.oam.ach_opt;
                    } else {
                        ach_conf = &flow_conf->flow_conf.ptp.ach_opt;
                    }

                    ach_conf->comm_opt.version.value = ach_ver;
                    ach_conf->comm_opt.version.mask = 0xF;
                    ach_conf->comm_opt.channel_type.value = channel_type;
                    ach_conf->comm_opt.channel_type.mask = 0xFFFF;
                    ach_conf->comm_opt.proto_id.value = proto_id;
                    ach_conf->comm_opt.proto_id.mask = 0xFFFF;
                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed!\n");
                    VTSS_FREE(eng_conf);
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(eng_conf);
    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    }
}

void misc_icli_deb_phy_ts_engine_generic_comm_conf( u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u32 next_proto_offset, u8 flow_offset)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_gen_conf_t *gen_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                gen_conf = &flow_conf->flow_conf.gen.gen_opt;
                conf_ptr = flow_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc == VTSS_RC_OK) {
                    gen_conf->comm_opt.next_prot_offset = next_proto_offset;
                    gen_conf->comm_opt.flow_offset = flow_offset;
                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed\n");
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_deb_phy_ts_generic_flow_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 flow_id, BOOL enable, u32 data_upper, u32 mask_upper, u32 data_upper_mid, u32 mask_upper_mid, u32 data_lower_mid, u32 mask_lower_mid, u32 data_lower, u32 mask_lower)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_gen_conf_t *gen_conf;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                gen_conf = &flow_conf->flow_conf.gen.gen_opt;
                conf_ptr = flow_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc == VTSS_RC_OK) {
                    gen_conf->flow_opt[flow_id].flow_en = enable;
                    gen_conf->flow_opt[flow_id].data[3] = data_upper;
                    gen_conf->flow_opt[flow_id].data[2] = data_upper_mid;
                    gen_conf->flow_opt[flow_id].data[1] = data_lower_mid;
                    gen_conf->flow_opt[flow_id].data[0] = data_lower;
                    gen_conf->flow_opt[flow_id].mask[3] = mask_upper;
                    gen_conf->flow_opt[flow_id].mask[2] = mask_upper_mid;
                    gen_conf->flow_opt[flow_id].mask[1] = mask_lower_mid;
                    gen_conf->flow_opt[flow_id].mask[0] = mask_lower;
                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, flow_conf);
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Failed\n");
                    VTSS_FREE(flow_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(flow_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    } else {
        ICLI_PRINTF("Failed\n");
    }
}

void misc_icli_ts_engine_add_action(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 action_id, u8 channel_map, BOOL has_ptp, u8 clk_mode, u8 delaym, u8 domain_lower, u8 domain_upper, BOOL has_delay_req_ts, BOOL has_y1731, BOOL has_ietf, u8 ietf_ds, BOOL has_generic, u8 flow_id, u32 gen_data_upper, u32 gen_data_lower, u32 gen_mask_upper, u32 gen_mask_lower, u8 ts_type, u8 ts_offset)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_action_t *action_conf = NULL, *conf_ptr;
    vtss_phy_ts_ptp_engine_action_t *ptp_action;
    vtss_phy_ts_oam_engine_action_t *oam_action;
    vtss_phy_ts_generic_action_t    *gen_action;
    mesa_rc rc = VTSS_RC_OK;

    action_conf = (vtss_phy_ts_engine_action_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_action_t));
    if (action_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                conf_ptr = action_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_action_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                } else {
                    rc = vtss_phy_ts_egress_engine_action_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                }

                if (rc == VTSS_RC_OK) {
                    if (action_conf->action_ptp == TRUE && has_ptp == TRUE) {
                        if (action_id < 2) {
                            ptp_action = &action_conf->action.ptp_conf[action_id];
                            ptp_action->enable = TRUE;
                            ptp_action->channel_map = channel_map;
                            ptp_action->clk_mode = (vtss_phy_ts_ptp_clock_mode_t)clk_mode;
                            ptp_action->delaym_type = (vtss_phy_ts_ptp_delaym_type_t)delaym;
                            ptp_action->ptp_conf.range_en = TRUE;
                            ptp_action->ptp_conf.domain.range.lower = domain_lower;
                            ptp_action->ptp_conf.domain.range.upper = domain_upper;
                            ptp_action->delay_req_recieve_timestamp = has_delay_req_ts ? TRUE : FALSE;
                        } else {
                            rc = VTSS_RC_ERROR;
                        }
                    } else if (action_conf->action_ptp == FALSE && has_y1731 == TRUE) {
                        if (action_id < 6) {
                            oam_action = &action_conf->action.oam_conf[action_id];
                            oam_action->enable = TRUE;
                            oam_action->channel_map = channel_map;
                            oam_action->version     = 0;
                            oam_action->y1731_en    = TRUE;
                            oam_action->oam_conf.y1731_oam_conf.delaym_type = (vtss_phy_ts_y1731_oam_delaym_type_t)delaym;
                            oam_action->oam_conf.y1731_oam_conf.range_en = TRUE;
                            oam_action->oam_conf.y1731_oam_conf.meg_level.range.lower = domain_lower;
                            oam_action->oam_conf.y1731_oam_conf.meg_level.range.upper = domain_upper;
                        } else {
                            rc = VTSS_RC_ERROR;
                        }
                    }  else if (action_conf->action_ptp == FALSE && has_ietf == TRUE) {
                        if (action_id < 6) {
                            oam_action = &action_conf->action.oam_conf[action_id];
                            oam_action->enable = TRUE;
                            oam_action->channel_map = channel_map;
                            oam_action->version     = 0;
                            oam_action->y1731_en    = FALSE;
                            oam_action->oam_conf.ietf_oam_conf.delaym_type = (vtss_phy_ts_ietf_mpls_ach_oam_delaym_type_t)delaym;
                            oam_action->oam_conf.ietf_oam_conf.ts_format = VTSS_PHY_TS_IETF_MPLS_ACH_OAM_TS_FORMAT_PTP;
                            oam_action->oam_conf.ietf_oam_conf.ds = ietf_ds;
                        } else {
                            rc = VTSS_RC_ERROR;
                        }
                    } else if (action_conf->action_ptp == FALSE && has_generic == TRUE) {
                        if (action_id < 6 && flow_id < 6) {
                            gen_action = &action_conf->action.gen_conf[action_id];
                            gen_action->enable = TRUE;
                            gen_action->flow_id = flow_id;
                            gen_action->channel_map = channel_map;
                            gen_action->data[0] = gen_data_lower;
                            gen_action->data[1] = gen_data_upper;
                            gen_action->mask[0] = gen_mask_lower;
                            gen_action->mask[1] = gen_mask_upper;
                            gen_action->ts_type = (vtss_phy_ts_action_format)ts_type;
                            gen_action->ts_offset = ts_offset;
                        } else {
                            rc = VTSS_RC_ERROR;
                        }
                    }

                    if (rc == VTSS_RC_OK) {
                        if (ingress) {
                            rc = vtss_phy_ts_ingress_engine_action_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, action_conf);
                        } else {
                            rc = vtss_phy_ts_egress_engine_action_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, action_conf);
                        }
                    }
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Action config failed on port %d engine %d\n", uport, eng_id);
                    VTSS_FREE(action_conf);
                    return;
                }
            }
        }
    }

    VTSS_FREE(action_conf);
    if (rc == VTSS_RC_OK) {
        ICLI_PRINTF("Success\n");
    }
}

void misc_icli_deb_phy_ts_engine_action_del(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, u8 action_id)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_action_t *action_conf = NULL, *conf_ptr;
    mesa_rc rc;
    vtss_phy_ts_ptp_engine_action_t *ptp_action;
    vtss_phy_ts_oam_engine_action_t *oam_action;
    vtss_phy_ts_generic_action_t *gen_action;

    if (!v_port_type_list) {
        ICLI_PRINTF("No ports specified\n");
        return;
    }

    action_conf = (vtss_phy_ts_engine_action_t *)VTSS_MALLOC(sizeof(vtss_phy_ts_engine_action_t));
    if (action_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }

    for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
        for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
            uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

            port_no = uport2iport(uport);
            conf_ptr = action_conf;
            if (ingress) {
                rc = vtss_phy_ts_ingress_engine_action_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
            } else {
                rc = vtss_phy_ts_egress_engine_action_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
            }

            if (rc != VTSS_RC_OK) {
                ICLI_PRINTF("Unable to get engine action\n");
                goto do_exit;
            }

            if (action_conf->action_ptp) {
                if (action_id < 2) {
                    ptp_action = &action_conf->action.ptp_conf[action_id];
                    ptp_action->enable = FALSE;
                    ptp_action->channel_map = 0;
                    ptp_action->clk_mode = (vtss_phy_ts_ptp_clock_mode_t)0;
                    ptp_action->delaym_type = (vtss_phy_ts_ptp_delaym_type_t)0;
                    ptp_action->ptp_conf.range_en = FALSE;
                    ptp_action->ptp_conf.domain.range.lower = 0;
                    ptp_action->ptp_conf.domain.range.upper = 0;
                } else {
                    ICLI_PRINTF("Invalid PTP Action ID (%u)", action_id);
                    goto do_exit;
                }
            } else  if (!action_conf->action_gen) {
                if (action_id < 6) {
                    oam_action = &action_conf->action.oam_conf[action_id];
                    oam_action->enable = FALSE;
                    oam_action->channel_map = 0;
                    oam_action->oam_conf.y1731_oam_conf.delaym_type = (vtss_phy_ts_y1731_oam_delaym_type_t)0;
                    oam_action->oam_conf.y1731_oam_conf.range_en = FALSE;
                    oam_action->oam_conf.y1731_oam_conf.meg_level.range.lower = 0;
                    oam_action->oam_conf.y1731_oam_conf.meg_level.range.upper = 0;
                } else {
                    ICLI_PRINTF("Invalid OAM Action ID (%u)", action_id);
                    goto do_exit;
                }
            } else {
                if (action_id < 6) {
                    gen_action = &action_conf->action.gen_conf[action_id];
                    gen_action->enable = FALSE;
                    gen_action->channel_map = 0;
                    gen_action->data[0] = 0;
                    gen_action->data[1] = 0;
                    gen_action->mask[0] = 0;
                    gen_action->mask[1] = 0;
                    gen_action->ts_type = (vtss_phy_ts_action_format)0;
                    gen_action->ts_offset = 0;
                } else {
                    ICLI_PRINTF("Invalid Gen Action ID (%u)", action_id);
                    goto do_exit;
                }
            }

            if (ingress) {
                rc = vtss_phy_ts_ingress_engine_action_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, action_conf);
            } else {
                rc = vtss_phy_ts_egress_engine_action_set(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, action_conf);
            }

            if (rc != VTSS_RC_OK) {
                ICLI_PRINTF("Failed!\n");
                goto do_exit;
            }
        }
    }

    ICLI_PRINTF("Success\n");

do_exit:
    VTSS_FREE(action_conf);
}

void misc_icli_show_phy_ts_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, u8 eng_id, BOOL ingress, BOOL has_eng_init, u8 flow_id, BOOL has_comm_conf, BOOL has_eth1, BOOL has_eth2, BOOL has_ip1, BOOL has_ip2, BOOL has_mpls, BOOL has_ach, BOOL has_gen_ts, BOOL has_flow_conf, BOOL has_eth1_1, BOOL has_eth2_1, BOOL has_ip1_1, BOOL has_ip2_1, BOOL has_mpls_1, BOOL has_gen_ts_1, BOOL has_action, BOOL has_ptp, BOOL has_oam, BOOL has_generic)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_phy_ts_engine_flow_conf_t *flow_conf = NULL, *conf_ptr;
    vtss_phy_ts_eng_init_conf_t *eng_conf = NULL, *conf_eng;
    vtss_phy_ts_engine_action_t *action_conf = NULL, *conf_action;
    vtss_phy_ts_eth_conf_t *eth_conf = NULL;
    vtss_phy_ts_ip_conf_t *ip_conf = NULL;
    vtss_phy_ts_mpls_conf_t *mpls_conf = NULL;
    vtss_phy_ts_ach_conf_t *ach_conf = NULL;
    vtss_phy_ts_gen_conf_t *gen_conf = NULL;
    vtss_phy_ts_ptp_engine_action_t *ptp_action;
    vtss_phy_ts_oam_engine_action_t *oam_action;
    vtss_phy_ts_generic_action_t *gen_action;
    u8 i = 0;
    mesa_rc rc = VTSS_RC_OK;

    flow_conf = (vtss_phy_ts_engine_flow_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_flow_conf_t));
    if (flow_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        return;
    }
    memset(flow_conf, 0, sizeof(vtss_phy_ts_engine_flow_conf_t));

    eng_conf = (vtss_phy_ts_eng_init_conf_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_eng_init_conf_t));
    if (eng_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        VTSS_FREE(flow_conf);
        return;
    }
    memset(eng_conf, 0, sizeof(vtss_phy_ts_eng_init_conf_t));

    action_conf = (vtss_phy_ts_engine_action_t *) VTSS_MALLOC(sizeof(vtss_phy_ts_engine_action_t));
    if (action_conf == NULL) {
        ICLI_PRINTF("Failed!\n");
        VTSS_FREE(flow_conf);
        VTSS_FREE(eng_conf);
        return;
    }
    memset(action_conf, 0, sizeof(vtss_phy_ts_engine_action_t));

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                port_no = uport2iport(uport);
                conf_eng = eng_conf;
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_init_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_eng);
                } else {
                    rc = vtss_phy_ts_egress_engine_init_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_eng);
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Could not get engine init config\n");
                    goto do_exit;
                }

                conf_action = action_conf;  /* using alt ptr to engine_action_get, otherwise Lint complains on custody problem i.e. error 429 */
                if (ingress) {
                    rc = vtss_phy_ts_ingress_engine_action_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_action);
                } else {
                    rc = vtss_phy_ts_egress_engine_action_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_action);
                }

                if (rc != VTSS_RC_OK) {
                    ICLI_PRINTF("Unable to get engine action\n");
                    goto do_exit;
                }

                if (has_eng_init) {
                    ICLI_PRINTF("port_no = %u, ingress = %u, engine-id = %u, encapsulation = %d, flow-start-index = %d, flow-end-index = %d, flow-match-mode = %d\n",
                                port_no, ingress, eng_id, eng_conf->encap_type, eng_conf->flow_st_index, eng_conf->flow_end_index,
                                eng_conf->flow_match_mode);
                } else if (has_comm_conf) {
                    conf_ptr = flow_conf;
                    if (ingress) {
                        rc = vtss_phy_ts_ingress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                    } else {
                        rc = vtss_phy_ts_egress_engine_conf_get(API_INST_DEFAULT, port_no, (vtss_phy_ts_engine_t)eng_id, conf_ptr);
                    }

                    if (rc != VTSS_RC_OK) {
                        ICLI_PRINTF("Could not get flow conf\n");
                        goto do_exit;
                    }

                    if (has_eth1) {
                        if (action_conf->action_ptp) {
                            eth_conf = &flow_conf->flow_conf.ptp.eth1_opt;
                        } else if (!action_conf->action_ptp && !action_conf->action_gen) {
                            eth_conf = &flow_conf->flow_conf.oam.eth1_opt;
                        } else {
                            eth_conf = &flow_conf->flow_conf.gen.eth1_opt;
                        }

                        ICLI_PRINTF("ETH1 conf: pbb = %d, etype = 0x%x, tpid = 0x%x\n",
                                    eth_conf->comm_opt.pbb_en, eth_conf->comm_opt.etype,
                                    eth_conf->comm_opt.tpid);
                    } else if (has_eth2) {
                        if (action_conf->action_ptp) {
                            eth_conf = &flow_conf->flow_conf.ptp.eth2_opt;
                        } else if (!action_conf->action_ptp && !action_conf->action_gen) {
                            eth_conf = &flow_conf->flow_conf.oam.eth2_opt;
                        } else {
                            ICLI_PRINTF("ETH2 configuration cannot be retrieved\n");
                            continue;
                        }

                        ICLI_PRINTF("ETH2 conf: pbb = %d, etype = 0x%x, tpid = 0x%x\n",
                                    eth_conf->comm_opt.pbb_en, eth_conf->comm_opt.etype,
                                    eth_conf->comm_opt.tpid);
                    } else if (has_ip1) {
                        if (action_conf->action_ptp) {
                            ip_conf = &flow_conf->flow_conf.ptp.ip1_opt;
                        } else {
                            ICLI_PRINTF("ip configuration cannot be retrieved\n");
                            continue;
                        }

                        ICLI_PRINTF("IP1 conf: mode = %d, sport_val = %d, mask = 0x%x, dport_val = %d, mask = 0x%x\n",
                                    ip_conf->comm_opt.ip_mode,
                                    ip_conf->comm_opt.sport_val, ip_conf->comm_opt.sport_mask,
                                    ip_conf->comm_opt.dport_val, ip_conf->comm_opt.dport_mask);
                    } else if (has_ip2) {
                        if (action_conf->action_ptp) {
                            ip_conf = &flow_conf->flow_conf.ptp.ip2_opt;
                        } else {
                            ICLI_PRINTF("ip configuration cannot be retrieved\n");
                            continue;
                        }

                        ICLI_PRINTF("IP2 conf: mode = %d, sport_val = %d, mask = 0x%x, dport_val = %d, mask = 0x%x\n",
                                    ip_conf->comm_opt.ip_mode,
                                    ip_conf->comm_opt.sport_val, ip_conf->comm_opt.sport_mask,
                                    ip_conf->comm_opt.dport_val, ip_conf->comm_opt.dport_mask);
                    } else if (has_mpls) {
                        if (action_conf->action_ptp) {
                            mpls_conf = &flow_conf->flow_conf.ptp.mpls_opt;
                        } else {
                            mpls_conf = &flow_conf->flow_conf.oam.mpls_opt;
                        }

                        ICLI_PRINTF("MPLS Control Word: enable = %d\n", mpls_conf->comm_opt.cw_en);
                    } else if (has_ach) {
                        if (action_conf->action_ptp) {
                            ach_conf = &flow_conf->flow_conf.ptp.ach_opt;
                        } else {
                            ach_conf = &flow_conf->flow_conf.oam.ach_opt;
                        }

                        ICLI_PRINTF("ACH conf: ver = %d, chan_type = %d, proto_id = %d\n",
                                    ach_conf->comm_opt.version.value,
                                    ach_conf->comm_opt.channel_type.value,
                                    ach_conf->comm_opt.proto_id.value);
                    } else if (has_gen_ts) {
                        if (action_conf->action_gen) {
                            gen_conf = &flow_conf->flow_conf.gen.gen_opt;
                            ICLI_PRINTF("Generic common conf: next protocol offset=%d flow offset=%d \n",
                                        gen_conf->comm_opt.next_prot_offset, gen_conf->comm_opt.flow_offset);
                        }
                    }
                } else if (has_flow_conf) {
                    if (has_eth1_1) {
                        if (action_conf->action_ptp) {
                            eth_conf = &flow_conf->flow_conf.ptp.eth1_opt;
                        } else if (!action_conf->action_ptp && !action_conf->action_gen) {
                            eth_conf = &flow_conf->flow_conf.oam.eth1_opt;
                        } else {
                            eth_conf = &flow_conf->flow_conf.gen.eth1_opt;
                        }

                        ICLI_PRINTF("ETH1 flow conf: enable = %d, match_mode = %d, mac = 0x%x-%x-%x-%x-%x-%x\n",
                                    eth_conf->flow_opt[flow_id].flow_en,
                                    eth_conf->flow_opt[flow_id].addr_match_mode,
                                    eth_conf->flow_opt[flow_id].mac_addr[0],
                                    eth_conf->flow_opt[flow_id].mac_addr[1],
                                    eth_conf->flow_opt[flow_id].mac_addr[2],
                                    eth_conf->flow_opt[flow_id].mac_addr[3],
                                    eth_conf->flow_opt[flow_id].mac_addr[4],
                                    eth_conf->flow_opt[flow_id].mac_addr[5]);
                        ICLI_PRINTF("match_select = %d, vlan_chk = %d, num_tag = %d, range_mode = %d\n",
                                    eth_conf->flow_opt[flow_id].addr_match_select,
                                    eth_conf->flow_opt[flow_id].vlan_check,
                                    eth_conf->flow_opt[flow_id].num_tag,
                                    eth_conf->flow_opt[flow_id].tag_range_mode);
                        ICLI_PRINTF("tag1_type = %d, tag2_type = %d, tag1_lower = %d, upper = %d, tag2_lower = %d, upper = %d\n",
                                    eth_conf->flow_opt[flow_id].outer_tag_type,
                                    eth_conf->flow_opt[flow_id].inner_tag_type,
                                    eth_conf->flow_opt[flow_id].outer_tag.range.lower,
                                    eth_conf->flow_opt[flow_id].outer_tag.range.upper,
                                    eth_conf->flow_opt[flow_id].inner_tag.range.lower,
                                    eth_conf->flow_opt[flow_id].inner_tag.range.upper);
                    } else if (has_eth2_1) {
                        if (action_conf->action_ptp) {
                            eth_conf = &flow_conf->flow_conf.ptp.eth2_opt;
                        } else if (!action_conf->action_ptp && !action_conf->action_gen) {
                            eth_conf = &flow_conf->flow_conf.oam.eth2_opt;
                        } else {
                            ICLI_PRINTF("ETH2 configuration cannot be retrieved\n");
                            continue;
                        }

                        ICLI_PRINTF("ETH2 flow conf: enable = %d, match_mode = %d, mac = 0x%x-%x-%x-%x-%x-%x\n",
                                    eth_conf->flow_opt[flow_id].flow_en,
                                    eth_conf->flow_opt[flow_id].addr_match_mode,
                                    eth_conf->flow_opt[flow_id].mac_addr[0],
                                    eth_conf->flow_opt[flow_id].mac_addr[1],
                                    eth_conf->flow_opt[flow_id].mac_addr[2],
                                    eth_conf->flow_opt[flow_id].mac_addr[3],
                                    eth_conf->flow_opt[flow_id].mac_addr[4],
                                    eth_conf->flow_opt[flow_id].mac_addr[5]);
                        ICLI_PRINTF("match_select = %d, vlan_chk = %d, num_tag = %d, range_mode = %d\n",
                                    eth_conf->flow_opt[flow_id].addr_match_select,
                                    eth_conf->flow_opt[flow_id].vlan_check,
                                    eth_conf->flow_opt[flow_id].num_tag,
                                    eth_conf->flow_opt[flow_id].tag_range_mode);
                        ICLI_PRINTF("tag1_type = %d, tag2_type = %d, tag1_lower = %d, upper = %d, tag2_lower = %d, upper = %d\n",
                                    eth_conf->flow_opt[flow_id].outer_tag_type,
                                    eth_conf->flow_opt[flow_id].inner_tag_type,
                                    eth_conf->flow_opt[flow_id].outer_tag.range.lower,
                                    eth_conf->flow_opt[flow_id].outer_tag.range.upper,
                                    eth_conf->flow_opt[flow_id].inner_tag.range.lower,
                                    eth_conf->flow_opt[flow_id].inner_tag.range.upper);
                    } else if (has_ip1_1) {
                        if (action_conf->action_ptp) {
                            ip_conf = &flow_conf->flow_conf.ptp.ip1_opt;
                        } else {
                            ICLI_PRINTF("ip configuration cannot be retrieved\n");
                            continue;
                        }

                        if (ip_conf->comm_opt.ip_mode == VTSS_PHY_TS_IP_VER_4) {
                            ICLI_PRINTF("IP1 flow_conf: enable = %d, match_mode = %d, addr = 0x%x, mask = 0x%x\n",
                                        ip_conf->flow_opt[flow_id].flow_en,
                                        ip_conf->flow_opt[flow_id].match_mode,
                                        (unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv4.addr,
                                        (unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv4.mask);
                        } else {
                            ICLI_PRINTF("IP1 flow_conf: enable = %d, match_mode = %d, addr = %x:%x:%x:%x:%x:%x:%x:%x, mask = %x:%x:%x:%x:%x:%x:%x:%x\n",
                                        ip_conf->flow_opt[flow_id].flow_en,
                                        ip_conf->flow_opt[flow_id].match_mode,
                                        (unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[0] & 0xFFFF,
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[0] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[1] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[1] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[2] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[2] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[3] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[3] >> 16) & 0xFFFF),
                                        (unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[0] & 0xFFFF,
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[0] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[1] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[1] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[2] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[2] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[3] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[3] >> 16) & 0xFFFF));
                        }

                    } else if (has_ip2_1) {
                        if (action_conf->action_ptp) {
                            ip_conf = &flow_conf->flow_conf.ptp.ip2_opt;
                        } else {
                            ICLI_PRINTF("ip configuration cannot be retrieved\n");
                            continue;
                        }

                        if (ip_conf->comm_opt.ip_mode == VTSS_PHY_TS_IP_VER_4) {
                            ICLI_PRINTF("IP2 flow_conf: enable = %d, match_mode = %d, addr = 0x%x, mask = 0x%x\n",
                                        ip_conf->flow_opt[flow_id].flow_en,
                                        ip_conf->flow_opt[flow_id].match_mode,
                                        (unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv4.addr,
                                        (unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv4.mask);
                        } else {
                            ICLI_PRINTF("IP2 flow_conf: enable = %d, match_mode = %d, addr = %x:%x:%x:%x:%x:%x:%x:%x, mask = %x:%x:%x:%x:%x:%x:%x:%x\n",
                                        ip_conf->flow_opt[flow_id].flow_en,
                                        ip_conf->flow_opt[flow_id].match_mode,
                                        (unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[0] & 0xFFFF,
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[0] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[1] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[1] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[2] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[2] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[3] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.addr[3] >> 16) & 0xFFFF),
                                        (unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[0] & 0xFFFF,
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[0] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[1] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[1] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[2] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[2] >> 16) & 0xFFFF),
                                        ((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[3] & 0xFFFF),
                                        (((unsigned int)ip_conf->flow_opt[flow_id].ip_addr.ipv6.mask[3] >> 16) & 0xFFFF));
                        }
                    } else if (has_mpls_1) {
                        if (action_conf->action_ptp) {
                            mpls_conf = &flow_conf->flow_conf.ptp.mpls_opt;
                        } else {
                            mpls_conf = &flow_conf->flow_conf.oam.mpls_opt;
                        }

                        ICLI_PRINTF("MPLS flow_conf: enable = %d, stack_depth = %d, ref_point = %d\n",
                                    mpls_conf->flow_opt[flow_id].flow_en,
                                    mpls_conf->flow_opt[flow_id].stack_depth,
                                    mpls_conf->flow_opt[flow_id].stack_ref_point);
                        if (mpls_conf->flow_opt[flow_id].stack_ref_point == VTSS_PHY_TS_MPLS_STACK_REF_POINT_TOP) {
                            ICLI_PRINTF("level_0 = %u - %u, level_1 = %u - %u, level_2 = %u - %u, level_3 = %u - %u\n",
                                        mpls_conf->flow_opt[flow_id].stack_level.top_down.top.lower,
                                        mpls_conf->flow_opt[flow_id].stack_level.top_down.top.upper,
                                        mpls_conf->flow_opt[flow_id].stack_level.top_down.frst_lvl_after_top.lower,
                                        mpls_conf->flow_opt[flow_id].stack_level.top_down.frst_lvl_after_top.upper,
                                        mpls_conf->flow_opt[flow_id].stack_level.top_down.snd_lvl_after_top.lower,
                                        mpls_conf->flow_opt[flow_id].stack_level.top_down.snd_lvl_after_top.upper,
                                        mpls_conf->flow_opt[flow_id].stack_level.top_down.thrd_lvl_after_top.lower,
                                        mpls_conf->flow_opt[flow_id].stack_level.top_down.thrd_lvl_after_top.upper);
                        } else {
                            ICLI_PRINTF("level_0 = %u - %u, level_1 = %u - %u, level_2 = %u - %u, level_3 = %u - %u\n",
                                        mpls_conf->flow_opt[flow_id].stack_level.bottom_up.end.lower,
                                        mpls_conf->flow_opt[flow_id].stack_level.bottom_up.end.upper,
                                        mpls_conf->flow_opt[flow_id].stack_level.bottom_up.frst_lvl_before_end.lower,
                                        mpls_conf->flow_opt[flow_id].stack_level.bottom_up.frst_lvl_before_end.upper,
                                        mpls_conf->flow_opt[flow_id].stack_level.bottom_up.snd_lvl_before_end.lower,
                                        mpls_conf->flow_opt[flow_id].stack_level.bottom_up.snd_lvl_before_end.upper,
                                        mpls_conf->flow_opt[flow_id].stack_level.bottom_up.thrd_lvl_before_end.lower,
                                        mpls_conf->flow_opt[flow_id].stack_level.bottom_up.thrd_lvl_before_end.upper);
                        }
                    } else if (has_gen_ts_1) {
                        if (!action_conf->action_ptp && action_conf->action_gen) {
                            gen_conf = &flow_conf->flow_conf.gen.gen_opt;
                            ICLI_PRINTF("Generic flow conf: flow_id=%d data=%x%x%x%x mask=%x%x%x%x \n", flow_id,
                                        gen_conf->flow_opt[flow_id].data[3], gen_conf->flow_opt[flow_id].data[2], gen_conf->flow_opt[flow_id].data[1],
                                        gen_conf->flow_opt[flow_id].data[0], gen_conf->flow_opt[flow_id].mask[3], gen_conf->flow_opt[flow_id].mask[2],
                                        gen_conf->flow_opt[flow_id].mask[1], gen_conf->flow_opt[flow_id].mask[0]);
                        }
                    }
                } else if (has_action) {
                    if (has_ptp && action_conf->action_ptp) {
                        ICLI_PRINTF("***PTP action***\n");
                        for (i = 0; i < 2; i++) {
                            ptp_action = &action_conf->action.ptp_conf[i];
                            ICLI_PRINTF("id = %d, enable = %d, channel_map = %d, clk_mode = %d, delaym = %d\n",
                                        i, ptp_action->enable, ptp_action->channel_map,
                                        ptp_action->clk_mode, ptp_action->delaym_type);
                            ICLI_PRINTF("domain range_en = %d, domain range lower = %d, upper = %d\n\n",
                                        ptp_action->ptp_conf.range_en,
                                        ptp_action->ptp_conf.domain.range.lower,
                                        ptp_action->ptp_conf.domain.range.upper);
                        }
                    } else if (has_oam && action_conf->action_gen == FALSE) {
                        ICLI_PRINTF("***OAM action***\n");
                        for (i = 0; i < 6; i++) {
                            oam_action = &action_conf->action.oam_conf[i];
                            ICLI_PRINTF("id = %d, enable = %d, channel_map = %d, delaym = %d\n",
                                        i, oam_action->enable,
                                        oam_action->channel_map,
                                        oam_action->oam_conf.y1731_oam_conf.delaym_type);
                            ICLI_PRINTF("MEG range_en = %d, MEG range lower = %d, upper = %d\n\n",
                                        oam_action->oam_conf.y1731_oam_conf.range_en,
                                        oam_action->oam_conf.y1731_oam_conf.meg_level.range.lower,
                                        oam_action->oam_conf.y1731_oam_conf.meg_level.range.upper);
                        }
                    } else if ((has_generic && action_conf->action_gen == TRUE)) {
                        ICLI_PRINTF("***Generic action***\n");
                        for (i = 0; i < 6; i++) {
                            gen_action = &action_conf->action.gen_conf[i];
                            ICLI_PRINTF("id = %d, enable =%d, channel_map =%d flow_id = %d\n",
                                        i, gen_action->enable, gen_action->channel_map, gen_action->flow_id);
                            ICLI_PRINTF("data = %x%x mask = %x%x ts_type = %d ts_offset =%d\n",
                                        gen_action->data[1], gen_action->data[0], gen_action->mask[1], gen_action->mask[0],
                                        gen_action->ts_type, gen_action->ts_offset);
                        }
                    } else {
                        ICLI_PRINTF("Action configuration cannot be retrieved\n");
                    }
                }
            }
        }
    }

do_exit:
    VTSS_FREE(eng_conf);
    VTSS_FREE(flow_conf);
    VTSS_FREE(action_conf);
}

static uint8_t hex2bin(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    return 0xff;
}

mesa_rc misc_icli_wis_tx_overhead_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL  has_set,
                                       BOOL has_all, BOOL has_sec_oh, BOOL has_line_oh, BOOL has_path_oh, BOOL has_d1_d3,
                                       BOOL has_sec_ord, BOOL has_suc, BOOL  has_res_sg, BOOL has_d4_d12, BOOL has_line_ord,
                                       BOOL has_aps_rdil, BOOL has_sync, BOOL has_res_lg, BOOL has_c2pl, BOOL has_puc,
                                       BOOL has_ptcm, BOOL has_res_pg, const char *oh_value_1)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_ewis_tx_oh_t  tx_oh;
    u8      wis_oh_val[27];
    u8      *oh_value;
    u32     length = 0;
    u8      val, val1, i = 0, j;
    mesa_rc rc = VTSS_RC_ERROR;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                memset(wis_oh_val, 0, sizeof(wis_oh_val));
                port_no =  uport2iport(uport);
                memset(&tx_oh, 0, sizeof(vtss_ewis_tx_oh_t));
                if (VTSS_RC_OK != (rc = vtss_ewis_tx_oh_get(misc_phy_inst_get(), port_no, &tx_oh))) {
                    ICLI_PRINTF("Could not perform vtss_ewis_tx_oh_get(): %u operation\n", uport);
                    continue;
                }

                if (has_set) {
                    length = strlen(oh_value_1);
                    if ((length > 54) || (length < 1)) {
                        ICLI_PRINTF("Overhead value error\n");
                        return VTSS_RC_INV_STATE;
                    }

                    if (!strncmp(oh_value_1, "0x", 2)) {
                        i = 2;
                    }

                    for (j = 0; i < length; i += 2, j++) {
                        val = hex2bin(oh_value_1[i]);
                        if (val == 0xFF) {
                            ICLI_PRINTF("No Valid input\n");
                            return VTSS_RC_INV_STATE;
                        }

                        val1 = hex2bin(oh_value_1[i + 1]);
                        if (val1 == 0xFF) {
                            ICLI_PRINTF("No Valid input\n");
                            return VTSS_RC_INV_STATE;
                        }

                        val = (val << 4) | val1;
                        wis_oh_val[j] = val;
                    }

                    oh_value = wis_oh_val;
                    if (has_all) {
                        memcpy((u8 *)(tx_oh.tx_dcc_s), oh_value, 3);
                        i = i + 3;
                        tx_oh.tx_e1       = *(oh_value + i++);
                        tx_oh.tx_f1       = *(oh_value + i++);
                        memcpy((u8 *)(tx_oh.tx_dcc_l), (oh_value + i), 9);
                        i = i + 9;
                        tx_oh.tx_e2 = *(oh_value + i++);
                        memcpy((u8 *)(&tx_oh.tx_k1_k2), (oh_value + i), 2);
                        i = i + 2;
                        tx_oh.tx_s1 = *(oh_value + i++);
                        memcpy((u8 *)(&tx_oh.tx_z1_z2), (oh_value + i), 2);
                        i = i + 2;
                        tx_oh.tx_c2 = *(oh_value + i++);
                        tx_oh.tx_f2 = *(oh_value + i++);
                        tx_oh.tx_n1 = *(oh_value + i++);
                        memcpy((u8 *)(&tx_oh.tx_z3_z4), (oh_value + i), 2);

                    } else if (has_sec_oh) {
                        memcpy((u8 *)(tx_oh.tx_dcc_s), oh_value, 3);
                        i = i + 3;
                        tx_oh.tx_e1       = *(oh_value + i++);
                        tx_oh.tx_f1       = *(oh_value + i++);

                    } else if (has_line_oh) {
                        memcpy((u8 *) (tx_oh.tx_dcc_l), oh_value, 9);
                        i = i + 9;
                        tx_oh.tx_e2 = *(oh_value + i++);
                        memcpy((u8 *) (&tx_oh.tx_k1_k2), (oh_value + i), 2);
                        i = i + 2;
                        tx_oh.tx_s1 = *(oh_value + i++);
                        memcpy((u8 *)(&tx_oh.tx_z1_z2), (oh_value + i), 2);

                    } else if (has_path_oh) {
                        tx_oh.tx_c2 = *(oh_value + i++);
                        tx_oh.tx_f2 = *(oh_value + i++);
                        tx_oh.tx_n1 = *(oh_value + i++);
                        memcpy((u8 *) (&tx_oh.tx_z3_z4), (oh_value + i), 2);

                    } else if (has_d1_d3) {
                        memcpy((u8 *) (tx_oh.tx_dcc_s), oh_value, 3);

                    } else if (has_sec_ord) {
                        tx_oh.tx_e1       = oh_value[0];

                    } else if (has_suc) {
                        tx_oh.tx_f1       = oh_value[0];

                    } else if (has_res_sg) {
                        //ignore

                    } else if (has_d4_d12) {
                        memcpy((u8 *) (tx_oh.tx_dcc_l), oh_value, 9);

                    } else if (has_line_ord) {
                        tx_oh.tx_e2 = oh_value[0];

                    } else if (has_aps_rdil) {
                        memcpy((u8 *) (&tx_oh.tx_k1_k2), oh_value, 2);

                    } else if (has_sync) {
                        tx_oh.tx_s1 = oh_value[0];

                    } else if (has_res_lg) {
                        memcpy((u8 *) (&tx_oh.tx_z1_z2), oh_value, 2);

                    } else if (has_c2pl) {
                        tx_oh.tx_c2 = oh_value[0];

                    } else if (has_puc) {
                        tx_oh.tx_f2 = oh_value[0];

                    } else if (has_ptcm) {
                        tx_oh.tx_n1 = oh_value[0];

                    } else if (has_res_pg) {
                        memcpy((u8 *) (&tx_oh.tx_z3_z4), oh_value, 2);
                    }

                    if (VTSS_RC_OK != (rc = vtss_ewis_tx_oh_set(misc_phy_inst_get(), port_no, &tx_oh))) {
                        ICLI_PRINTF("Could not perform vtss_ewis_tx_oh_set() for port %u\n", uport);
                        continue;
                    }
                } else {
                    if (VTSS_RC_OK != (rc = vtss_ewis_tx_oh_get(misc_phy_inst_get(), port_no, &tx_oh))) {
                        ICLI_PRINTF("Could not perform vtss_ewis_tx_oh_get(): %u operation\n", uport);
                        continue;
                    }

                    ICLI_PRINTF("Tx Overhead for Port: %u\n", uport);
                    ICLI_PRINTF("------------------------\n");
                    ICLI_PRINTF("D1-D3     :  0x%x%x%x \n", tx_oh.tx_dcc_s[0], tx_oh.tx_dcc_s[1], tx_oh.tx_dcc_s[2]);
                    ICLI_PRINTF("SEC-ORD   :  0x%x\n", tx_oh.tx_e1);
                    ICLI_PRINTF("SUC       :  0x%x \n", tx_oh.tx_f1);
                    ICLI_PRINTF("D4-D12    :  0x%x%x%x%x%x%x%x%x%x \n", tx_oh.tx_dcc_l[0], tx_oh.tx_dcc_l[1], tx_oh.tx_dcc_l[2],
                                tx_oh.tx_dcc_l[3], tx_oh.tx_dcc_l[4], tx_oh.tx_dcc_l[5],
                                tx_oh.tx_dcc_l[6], tx_oh.tx_dcc_l[7], tx_oh.tx_dcc_l[8]);
                    ICLI_PRINTF("LINE-ORD  :  0x%x \n", tx_oh.tx_e2);
                    ICLI_PRINTF("APS-RDIL  :  0x%x \n", tx_oh.tx_e2);
                    ICLI_PRINTF("SYNC      :  0x%x \n", tx_oh.tx_k1_k2);
                    ICLI_PRINTF("RES-LG    :  0x%x \n", tx_oh.tx_z1_z2);
                    ICLI_PRINTF("C2PL      :  0x%x \n", tx_oh.tx_c2);
                    ICLI_PRINTF("PUC       :  0x%x \n", tx_oh.tx_f2);
                    ICLI_PRINTF("PTCM      :  0x%x \n", tx_oh.tx_n1);
                    ICLI_PRINTF("RES-PG    :  0x%x \n\n", tx_oh.tx_z3_z4);
                }
            }
        }
    }

    return rc;
}

mesa_rc misc_cli_wis_tx_perf_thr_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_set,
                                      u32 wis_n_ebc_thr_s, u32 wis_n_ebc_thr_l, u32 wis_f_ebc_thr_l,
                                      u32 wis_n_ebc_thr_p, u32 wis_f_ebc_thr_p)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_ewis_counter_threshold_t   threshold;
    mesa_rc rc = VTSS_RC_ERROR;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                port_no =  uport2iport(uport);
                if (has_set) {
                    threshold.n_ebc_thr_s = wis_n_ebc_thr_s;
                    threshold.n_ebc_thr_l = wis_n_ebc_thr_l;
                    threshold.f_ebc_thr_l = wis_f_ebc_thr_l;
                    threshold.n_ebc_thr_p = wis_n_ebc_thr_p;
                    threshold.f_ebc_thr_p = wis_f_ebc_thr_p;
                    if (VTSS_RC_OK != (rc = vtss_ewis_counter_threshold_set(misc_phy_inst_get(), port_no, &threshold))) {
                        ICLI_PRINTF("Could not perform vtss_ewis_counter_threshold_set() for port %u\n", uport);
                        continue;
                    } else {
                        ICLI_PRINTF("Success: ewis counter threshold set\n");
                    }
                } else {
                    if (VTSS_RC_OK != (rc = vtss_ewis_counter_threshold_get(misc_phy_inst_get(), port_no, &threshold))) {
                        ICLI_PRINTF("Could not perform vtss_ewis_counter_threshold_get(): %u operation\n", uport);
                        continue;
                    }

                    ICLI_PRINTF("Threshold conf for Port: %u \n", uport);
                    ICLI_PRINTF("---------------------------\n");
                    ICLI_PRINTF("Section error count (B1) threshold        :%-12u \n", threshold.n_ebc_thr_s);
                    ICLI_PRINTF("Near end line error count (B2) threshold  :%-12u \n", threshold.n_ebc_thr_l );
                    ICLI_PRINTF("Far end line error count threshold        :%-12u \n", threshold.f_ebc_thr_l );
                    ICLI_PRINTF("Path block error count (B3) threshold     :%-12u \n", threshold.n_ebc_thr_p );
                    ICLI_PRINTF("Far end path error count threshold        :%-12u \n\n", threshold.f_ebc_thr_p );

                }
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_wis_test_status(u32 session_id, icli_stack_port_range_t *v_port_type_list)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_ewis_test_status_t test_status;
    mesa_rc rc = VTSS_RC_ERROR;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                port_no =  uport2iport(uport);
                if (VTSS_RC_OK != (rc = vtss_ewis_test_counter_get(misc_phy_inst_get(), port_no, &test_status))) {
                    ICLI_PRINTF("Could not perform vtss_ewis_test_counter_get() operation for Port: %u \n", uport);
                    continue;
                }

                ICLI_PRINTF("WIS Test Status Port: %u\n", uport);
                ICLI_PRINTF("------------------------\n");
                ICLI_PRINTF("%-30s %-12d\n", "  PRBS31 test pattern error counter:", test_status.tstpat_cnt);
                ICLI_PRINTF("%-30s %-12s\n", "  PRBS31 test Analyzer status      :", (test_status.ana_sync) ? "Sync" : "Not Synced");
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_wis_test_mode(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_loopback,
                                BOOL has_no_loopback, BOOL has_gen_dis, BOOL has_gen_sqr,
                                BOOL has_gen_prbs31, BOOL has_gen_mix, BOOL has_ana_dis, BOOL has_ana_prbs31,
                                BOOL has_ana_mix)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_ewis_test_conf_t  test_mode;
    mesa_rc rc = VTSS_RC_ERROR;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                port_no =  uport2iport(uport);
                if (VTSS_RC_OK != (rc = vtss_ewis_test_mode_get(misc_phy_inst_get(), port_no, &test_mode))) {
                    ICLI_PRINTF("Could not perform vtss_ewis_test_mode_get() operation");
                    continue;
                }

                if (has_loopback || has_no_loopback || has_gen_dis || has_gen_sqr || has_gen_prbs31 || has_gen_mix ||
                    has_ana_dis || has_ana_prbs31 || has_ana_mix) {
                    if (has_gen_dis) {
                        test_mode.test_pattern_gen = VTSS_WIS_TEST_MODE_DISABLE;
                    }

                    if (has_gen_sqr) {
                        test_mode.test_pattern_gen = VTSS_WIS_TEST_MODE_SQUARE_WAVE;
                    }

                    if (has_gen_prbs31) {
                        test_mode.test_pattern_gen = VTSS_WIS_TEST_MODE_PRBS31;
                    }

                    if (has_gen_mix) {
                        test_mode.test_pattern_gen = VTSS_WIS_TEST_MODE_MIXED_FREQUENCY;
                    }

                    if (has_ana_dis) {
                        test_mode.test_pattern_ana = VTSS_WIS_TEST_MODE_DISABLE;
                    }

                    if (has_ana_prbs31) {
                        test_mode.test_pattern_ana = VTSS_WIS_TEST_MODE_PRBS31;
                    }

                    if (has_ana_mix) {
                        test_mode.test_pattern_ana = VTSS_WIS_TEST_MODE_MIXED_FREQUENCY;
                    }

                    if (VTSS_RC_OK != (rc = vtss_ewis_test_mode_set(misc_phy_inst_get(), port_no, &test_mode))) {
                        ICLI_PRINTF("Could not perform vtss_ewis_test_mode_set() operation for port %u\n", uport);
                        continue;
                    }
                } else {
                    ICLI_PRINTF("Port: %u\n", uport);
                    ICLI_PRINTF("--------\n");
                    ICLI_PRINTF("Loopback :%-12s  Test Pattern generator  : %u   Test pattern analyzer  :%u\n",
                                test_mode.loopback ? "Yes" : "No",
                                test_mode.test_pattern_gen, test_mode.test_pattern_ana);
                }
            }
        }
    }

    return rc;
}

mesa_rc misc_cli_wis_prbs31_err_inj_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list,
                                         BOOL has_single_erro, BOOL has_sat_erro)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_ewis_prbs31_err_inj_t  err_inj;
    mesa_rc rc = VTSS_RC_ERROR;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                port_no =  uport2iport(uport);
                if (has_sat_erro) {
                    err_inj = VTSS_EWIS_PRBS31_SAT_ERR;
                } else if (has_single_erro) {
                    err_inj = VTSS_EWIS_PRBS31_SINGLE_ERR;
                }

                if (VTSS_RC_OK != (rc = vtss_ewis_prbs31_err_inj_set(misc_phy_inst_get(), port_no, &err_inj))) {
                    ICLI_PRINTF("Could not perform vtss_ewis_prbs31_err_inj_set(): %u operation\n", uport);
                    continue;
                }
            }
        }
    }

    return rc;
}

void misc_icli_wis_defects(u32 session_id, icli_stack_port_range_t *v_port_type_list)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t          port_no, uport;
    vtss_ewis_defects_t     defects;
    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;

                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                port_no = uport2iport(uport);

                if (vtss_ewis_defects_get(misc_phy_inst_get(), port_no, &defects) != VTSS_RC_OK) {
                    ICLI_PRINTF("Could not perform vtss_ewis_defects_get()  operation for Port: %u\n", uport);
                    continue;
                }

                ICLI_PRINTF("WIS Defects:\n\n");
                ICLI_PRINTF("Port: %u\n", uport);
                ICLI_PRINTF("--------\n");
                ICLI_PRINTF("%-30s %-12s\n", "  Loss of signal                      :", defects.dlos_s ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Loss of frame                       :", defects.dlof_s ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Line alarm indication signal        :", defects.dais_l ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Line remote defect indication       :", defects.drdi_l ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Path alarm indication signal        :", defects.dais_p ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Loss of pointer                     :", defects.dlop_p ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Path Unequipped                     :", defects.duneq_p ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Path Remote Defect Indication       :", defects.drdi_p ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Path loss of code-group delineation :", defects.dlcd_p ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Path label Mismatch                 :", defects.dplm_p ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Far-end AIS-P or LOP-P              :", defects.dfais_p ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Far-end PLM-P or LCD-P defect       :", defects.dfplm_p ? "Yes" : "No");
                ICLI_PRINTF("%-30s %-12s\n", "  Far End Path Unequipped             :", defects.dfuneq_p ? "Yes" : "No");
            }
        }
    }
}

void misc_icli_wis_counters(u32 session_id, icli_stack_port_range_t *v_port_type_list)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_ewis_counter_t     counter;
    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                port_no = uport2iport(uport);
                if (vtss_ewis_counter_get(misc_phy_inst_get(), port_no, &counter) != VTSS_RC_OK) {
                    ICLI_PRINTF("Could not perform vtss_phy_ewis_counter_get()  operation for Port: %u\n", uport);
                    continue;
                }

                ICLI_PRINTF("WIS Error Counters:\n\n");
                ICLI_PRINTF("Port: %u\n", uport);
                ICLI_PRINTF("--------\n");
                ICLI_PRINTF("%-30s %-12u\n", "  Section BIP error count               :", counter.pn_ebc_s);
                ICLI_PRINTF("%-30s %-12u\n", "  Near end line block (BIP) error count:", counter.pn_ebc_l);
                ICLI_PRINTF("%-30s %-12u\n", "  Far end line block (BIP) error count :", counter.pf_ebc_l);
                ICLI_PRINTF("%-30s %-12u\n", "  Path block error count                :", counter.pn_ebc_p);
                ICLI_PRINTF("%-30s %-12u\n", "  Far end path block error count        :", counter.pf_ebc_p);
            }
        }
    }
}

mesa_rc misc_icli_wis_conse_act(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_enable,
                                u8 wis_aisl, u8 wis_rdil, u16 wis_fault)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    mesa_rc rc = VTSS_RC_ERROR;

    vtss_ewis_cons_act_t cons_act;

    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                port_no =  uport2iport(uport);
                if (VTSS_RC_OK != (rc = vtss_ewis_cons_act_get(misc_phy_inst_get(), port_no, &cons_act))) {
                    ICLI_PRINTF("Could not perform vtss_ewis_cons_act_get() for port %u\n", uport);
                    continue;
                }

                if (has_enable) {
                    cons_act.aisl.ais_on_los         = wis_aisl & 1;
                    cons_act.aisl.ais_on_lof         = (wis_aisl >> 1) & 1;

                    cons_act.rdil.rdil_on_los        = wis_rdil & 1;
                    cons_act.rdil.rdil_on_lof        = (wis_rdil >> 1) & 1;
                    cons_act.rdil.rdil_on_lopc       = (wis_rdil >> 2) & 1;
                    cons_act.rdil.rdil_on_ais_l      = (wis_rdil >> 3) & 1;

                    cons_act.fault.fault_on_feplmp   = wis_fault & 1;
                    cons_act.fault.fault_on_feaisp   = (wis_fault >> 1) & 1;
                    cons_act.fault.fault_on_rdil     = (wis_fault >> 2) & 1;
                    cons_act.fault.fault_on_sef      = (wis_fault >> 3) & 1;
                    cons_act.fault.fault_on_lof      = (wis_fault >> 4) & 1;
                    cons_act.fault.fault_on_los      = (wis_fault >> 5) & 1;
                    cons_act.fault.fault_on_aisl     = (wis_fault >> 6) & 1;
                    cons_act.fault.fault_on_lcdp     = (wis_fault >> 7) & 1;
                    cons_act.fault.fault_on_plmp     = (wis_fault >> 8) & 1;
                    cons_act.fault.fault_on_aisp     = (wis_fault >> 9) & 1;
                    cons_act.fault.fault_on_lopp     = (wis_fault >> 10) & 1;

                    if (VTSS_RC_OK != (rc = vtss_ewis_cons_act_set(misc_phy_inst_get(), port_no, &cons_act))) {
                        ICLI_PRINTF("Could not perform vtss_ewis_cons_act_set() for port %u\n", uport);
                        continue;
                    }
                } else {
                    ICLI_PRINTF("Port: %u\n", uport);
                    ICLI_PRINTF("--------\n");
                    ICLI_PRINTF("AIS-L insertion on LOS           :  %-6d \n", cons_act.aisl.ais_on_los);
                    ICLI_PRINTF("AIS-L insertion on LOF           :  %-6d \n\n", cons_act.aisl.ais_on_lof);
                    ICLI_PRINTF("RDI-L back reporting on LOS      :  %-6d \n", cons_act.rdil.rdil_on_los);
                    ICLI_PRINTF("RDI-L back reporting on LOF      :  %-6d \n", cons_act.rdil.rdil_on_lof);
                    ICLI_PRINTF("RDI-L back reporting on LOPC     :  %-6d \n", cons_act.rdil.rdil_on_lopc);
                    ICLI_PRINTF("RDI-L back reporting on AIS_L    :  %-6d \n\n", cons_act.rdil.rdil_on_ais_l);
                    ICLI_PRINTF("Fault condition on far-end PLM-P :  %-6d \n", cons_act.fault.fault_on_feplmp);
                    ICLI_PRINTF("Fault condition on far-end AIS-P :  %-6d \n", cons_act.fault.fault_on_feaisp);
                    ICLI_PRINTF("Fault condition on RDI-L         :  %-6d \n", cons_act.fault.fault_on_rdil);
                    ICLI_PRINTF("Fault condition on SEF           :  %-6d \n", cons_act.fault.fault_on_sef);
                    ICLI_PRINTF("Fault condition on LOF           :  %-6d \n", cons_act.fault.fault_on_lof);
                    ICLI_PRINTF("Fault condition on LOS           :  %-6d \n", cons_act.fault.fault_on_los);
                    ICLI_PRINTF("Fault condition on AIS-L         :  %-6d \n", cons_act.fault.fault_on_aisl);
                    ICLI_PRINTF("Fault condition on LCD-P         :  %-6d \n", cons_act.fault.fault_on_lcdp);
                    ICLI_PRINTF("Fault condition on PLM-P         :  %-6d \n", cons_act.fault.fault_on_plmp);
                    ICLI_PRINTF("Fault condition on AIS-P         :  %-6d \n", cons_act.fault.fault_on_aisp);
                    ICLI_PRINTF("Fault condition on LOP-P         :  %-6d \n", cons_act.fault.fault_on_lopp);
                }
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_wis_force_conf( u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL set,
                                  u8  wis_line_rx, u8 wis_line_tx, u8 wis_force_events)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    vtss_ewis_force_mode_t  force_conf;
    mesa_rc rc = VTSS_RC_ERROR;
    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                port_no =  uport2iport(uport);
                if (set) {
                    force_conf.line_rx_force.force_ais =  wis_line_rx & 1;
                    force_conf.line_rx_force.force_rdi = (wis_line_rx >> 1) & 1;
                    force_conf.line_tx_force.force_ais =  wis_line_tx & 1;
                    force_conf.line_tx_force.force_rdi = (wis_line_tx >> 1) & 1;
                    force_conf.path_force.force_uneq   =  wis_force_events & 1;;
                    force_conf.path_force.force_rdi    = (wis_force_events >> 1) & 1;;

                    if (VTSS_RC_OK != (rc = vtss_ewis_force_conf_set(misc_phy_inst_get(), port_no, &force_conf)))  {
                        ICLI_PRINTF("Could not perform vtss_ewis_force_conf_set() for port %u\n", uport);
                        continue;
                    } else {
                        ICLI_PRINTF("Ewis force conf set success\nn");
                    }
                } else {
                    if (VTSS_RC_OK != (rc = vtss_ewis_force_conf_get(misc_phy_inst_get(), port_no, &force_conf)))  {
                        ICLI_PRINTF("Could not perform vtss_ewis_force_conf_get() for port %u\n", uport);
                        continue;
                    }

                    ICLI_PRINTF("Port: %u\n", uport);
                    ICLI_PRINTF("--------\n");
                    ICLI_PRINTF("Line rx force ais :%-6s \n", force_conf.line_rx_force.force_ais ? "Yes" : "No");
                    ICLI_PRINTF("Line rx force rdi :%-6s \n", force_conf.line_rx_force.force_rdi ? "Yes" : "No");
                    ICLI_PRINTF("Line tx force ais :%-6s \n", force_conf.line_tx_force.force_ais ? "Yes" : "No");
                    ICLI_PRINTF("Line tx force rdi :%-6s \n", force_conf.line_tx_force.force_rdi ? "Yes" : "No");
                    ICLI_PRINTF("path force uneq   :%-6s \n", force_conf.path_force.force_uneq ? "Yes" : "No");
                    ICLI_PRINTF("path force rdi   :%-6s \n", force_conf.path_force.force_rdi ? "Yes" : "No");
                }
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_wis_event_force_conf(u32 session_id, icli_stack_port_range_t *v_port_type_list, vtss_ewis_event_t ev_force,
                                       BOOL has_enable, BOOL  has_disable)
{
    u32 range_idx, cnt_idx;
    mesa_port_no_t uport, port_no;
    mesa_rc rc = VTSS_RC_ERROR;
    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                //Ignore if it is not 10g port
                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), uport2iport(uport))) {
                    continue;
                }

                port_no =  uport2iport(uport);
                if (has_enable || has_disable) {
                    if (has_enable) {
                        if (VTSS_RC_OK != (rc = vtss_ewis_event_force(misc_phy_inst_get(), port_no, TRUE, ev_force))) {
                            ICLI_PRINTF("Could not perform vtss_ewis_events_force(): %u operation\n", uport);
                            continue;
                        } else {
                            ICLI_PRINTF("Ewis event force success\n");
                        }
                    } else {
                        if (VTSS_RC_OK != (rc = vtss_ewis_event_force(misc_phy_inst_get(), port_no, FALSE, ev_force)))  {
                            ICLI_PRINTF("Could not perform vtss_ewis_events_force(): %u operation\n", uport);
                            continue;
                        } else {
                            ICLI_PRINTF("Ewis event force disabled\n");
                        }
                    }
                } else {
                    ICLI_PRINTF(" WIS event force get not applicable \n");
                    return VTSS_RC_OK;
                }
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_wis_status(u32 session_id, icli_stack_port_range_t *v_port_type_list)
{
    u8                   range_idx = 0, cnt_idx = 0;
    mesa_port_no_t       uport = 0, port_no = 0;
    vtss_ewis_status_t   wis_status;
    mesa_rc              rc = VTSS_RC_ERROR;
    if (v_port_type_list) {
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                port_no = uport2iport(uport);

                if (!vtss_phy_10G_is_valid(misc_phy_inst_get(), port_no)) {
                    continue;
                }

                if (VTSS_RC_OK != (rc = vtss_ewis_status_get(misc_phy_inst_get(), port_no, &wis_status))) {
                    ICLI_PRINTF("Could not perform vtss_ewis_status_get() operation for Port: %u\n", uport);
                    continue;
                }

                ICLI_PRINTF("WIS Status Port: %u\n", uport);
                ICLI_PRINTF("-------------------\n");
                ICLI_PRINTF("%-30s %-12s\n", "  Fault condition       :", wis_status.fault ? "Fault" : "Normal");
                ICLI_PRINTF("%-30s %-12s\n", "  Link status condition :", wis_status.link_stat ? "Up" : "Down");
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_wis_mode(u32 session_id, BOOL enable, BOOL disable, icli_stack_port_range_t *plist)
{
    u8               range_idx = 0, cnt_idx = 0;
    vtss_ewis_mode_t wis_mode;
    mesa_port_no_t   uport = 0, port_no = 0;
    mesa_rc          rc = VTSS_RC_ERROR;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;
                port_no = uport2iport(uport);

                if (VTSS_RC_OK != (rc = vtss_ewis_mode_get(misc_phy_inst_get(), port_no, &wis_mode))) {
                    ICLI_PRINTF("Could not perform vtss_ewis_mode_get() for port %u\n", uport);
                    continue;
                }

                if (enable || disable) {
                    if (enable) {
                        wis_mode = VTSS_WIS_OPERMODE_WIS_MODE;
                    } else {
                        wis_mode = VTSS_WIS_OPERMODE_DISABLE;
                    }

                    if (VTSS_RC_OK != (rc = vtss_ewis_mode_set(misc_phy_inst_get(), port_no, &wis_mode))) {
                        ICLI_PRINTF("Could not perform vtss_ewis_mode_set() for port %u\n", uport);
                        continue;
                    }
                }

                ICLI_PRINTF("Port: %u\n", uport);
                ICLI_PRINTF("--------\n");
                ICLI_PRINTF("WIS Mode :%-12s  \n", wis_mode ? "Enabled" : "Disabled");
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_wis_perf_counters_get(u32 session_id, icli_stack_port_range_t *plist)
{
    u8               range_idx = 0, cnt_idx = 0;
    mesa_port_no_t   uport = 0, port_no = 0;
    mesa_rc          rc = VTSS_RC_ERROR;
    vtss_ewis_perf_t perf_pre;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;
                port_no = uport2iport(uport);

                if (VTSS_RC_OK != (rc = vtss_ewis_perf_get(misc_phy_inst_get(), port_no, &perf_pre))) {
                    ICLI_PRINTF("Could not perform vtss_ewis_perf_get(): %u operation\n", uport);
                    continue;
                }

                ICLI_PRINTF("WIS performance primitives:\n\n");
                ICLI_PRINTF("Port: %u\n", uport);
                ICLI_PRINTF("--------\n");
                ICLI_PRINTF("Section BIP error count               : %-12u\n", perf_pre.pn_ebc_s);
                ICLI_PRINTF("Near end line block (BIP) error count : %-12u\n", perf_pre.pn_ebc_l);
                ICLI_PRINTF("Far end line block (BIP) error count  : %-12u\n", perf_pre.pf_ebc_l);
                ICLI_PRINTF("Path block error count                : %-12u\n", perf_pre.pn_ebc_p);
                ICLI_PRINTF("Far end path block error count        : %-12u\n", perf_pre.pf_ebc_p);
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_wis_atti(u32 session_id, BOOL has_overhead, u8 overhead_value, icli_stack_port_range_t *plist)
{
    u8              range_idx = 0, cnt_idx = 0;
    mesa_port_no_t  uport = 0, port_no = 0;
    mesa_rc         rc = VTSS_RC_ERROR;
    vtss_ewis_tti_t wis_section_acti, wis_path_acti;
    BOOL            sec = 0, path = 0;

    if (plist) {
        memset(&wis_section_acti, 0, sizeof(vtss_ewis_tti_t));
        memset(&wis_path_acti, 0, sizeof(vtss_ewis_tti_t));
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;
                port_no = uport2iport(uport);

                if (has_overhead) {
                    if (overhead_value == 1) {
                        path = 1;
                        sec = 0;
                    } else if (overhead_value == 0) {
                        path = 0;
                        sec = 1;
                    }
                } else {
                    sec = 1;
                    path = 1;
                }

                if (sec) {
                    if (VTSS_RC_OK != (rc = vtss_ewis_section_acti_get(misc_phy_inst_get(), port_no, &wis_section_acti))) {
                        ICLI_PRINTF("Could not perform vtss_ewis_section_acti_get() operation for port %u\n", uport);
                        continue;
                    }

                    ICLI_PRINTF("Port: %u\n", uport);
                    ICLI_PRINTF("--------\n");
                    ICLI_PRINTF("Section received TTI :\n");
                    ICLI_PRINTF("%-20s: %-4u\n", "Mode", wis_section_acti.mode);

                    if (wis_section_acti.valid != FALSE) {
                        ICLI_PRINTF("%-20s: %-64s\n", "TTI", wis_section_acti.tti);
                    } else {
                        ICLI_PRINTF("Invalid TxTI configuration :: Transmitter and Receiver Section TxTI modes are not matching\n");
                    }
                }

                if (path) {
                    if (VTSS_RC_OK != (rc = vtss_ewis_path_acti_get(misc_phy_inst_get(), port_no, &wis_path_acti))) {
                        ICLI_PRINTF("Could not perform vtss_ewis_section_acti_get() operation for port %u\n", uport);
                        continue;
                    }

                    ICLI_PRINTF("Port: %u\n", uport);
                    ICLI_PRINTF("--------\n");
                    ICLI_PRINTF("Path received TTI :\n");
                    ICLI_PRINTF("%-20s: %-4u\n", "Mode", wis_path_acti.mode);
                    if (wis_path_acti.valid != FALSE) {
                        ICLI_PRINTF("%-20s: %-64s\n", "TTI", wis_path_acti.tti);
                    } else {
                        ICLI_PRINTF("Invalid TxTI configuration :: Transmitter and Receiver Path TxTI modes are not matching\n");
                    }
                }
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_wis_reset(u32 session_id, icli_stack_port_range_t *plist)
{
    u8             range_idx = 0, cnt_idx = 0;
    mesa_port_no_t uport = 0, port_no = 0;
    mesa_rc        rc = VTSS_RC_ERROR;

    if (plist) {
        // Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;
                port_no = uport2iport(uport);

                if (VTSS_RC_OK != (rc = vtss_ewis_reset(misc_phy_inst_get(), port_no))) {
                    ICLI_PRINTF("Could not perform vtss_ewis_reset() for port %u\n", uport);
                    continue;
                }
            }
        }
    }

    return rc;
}

mesa_rc misc_icli_wis_txtti(u32 session_id, BOOL has_set, u8 overhead, vtss_ewis_tti_mode_t tti_mode, const char *tti, icli_stack_port_range_t *plist)
{
    u8              range_idx = 0, cnt_idx = 0;
    mesa_port_no_t  uport = 0, port_no = 0;
    mesa_rc         rc = VTSS_RC_ERROR;
    vtss_ewis_tti_t tx_tti_path, tx_tti_section;
    u32             i;
    u32             explen = 0, length = 0;

    if (plist) {
        //  Loop through all switches in a stack
        for (range_idx = 0; range_idx < plist->cnt; range_idx++) {
            // Loop through ports in a switch
            for (cnt_idx = 0; cnt_idx < plist->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = plist->switch_range[range_idx].begin_uport + cnt_idx;
                port_no = uport2iport(uport);

                if (has_set) {
                    if (overhead == 0) {
                        if (VTSS_RC_OK != (rc = vtss_ewis_section_txti_get(misc_phy_inst_get(), port_no, &tx_tti_section))) {
                            ICLI_PRINTF("Could not perform vtss_ewis_section_txti_get() for port %u\n", uport);
                            continue;
                        }

                        length = strlen(tti);
                        if (tti_mode == VTSS_EWIS_TTI_MODE_1) {
                            explen = 1;
                        } else if (tti_mode == VTSS_EWIS_TTI_MODE_16) {
                            explen = 16;
                        } else if (tti_mode == VTSS_EWIS_TTI_MODE_64) {
                            explen = 64;
                        } else {
                            ICLI_PRINTF("TTI Mode mismatch\n");
                        }

                        if (length > explen) {
                            ICLI_PRINTF("TTI Length exceeds\n");
                            rc = VTSS_RC_ERROR;
                            continue;
                        }

                        tx_tti_section.mode = tti_mode;
                        if (tx_tti_section.mode == VTSS_EWIS_TTI_MODE_16) {
                            memcpy(tx_tti_section.tti, tti, 15);
                            tx_tti_section.tti[15] = 0x89; /* MSB set to 1 */
                        } else if (tx_tti_section.mode == VTSS_EWIS_TTI_MODE_64) {
                            memcpy(&tx_tti_section.tti[2], tti, 62);
                            tx_tti_section.tti[0] = 13; /* CR */
                            tx_tti_section.tti[1] = 10; /* LF */
                        } else if (tx_tti_section.mode == VTSS_EWIS_TTI_MODE_1) {
                            tx_tti_section.tti[0] = tti[0];
                        }

                        if (VTSS_RC_OK != (rc = vtss_ewis_section_txti_set(misc_phy_inst_get(), port_no, &tx_tti_section))) {
                            ICLI_PRINTF("Could not perform vtss_ewis_section_txti_set() for port %u\n", uport);
                            continue;
                        }
                    }

                    if (overhead == 1) {
                        if (VTSS_RC_OK != (rc = vtss_ewis_path_txti_get(misc_phy_inst_get(), port_no, &tx_tti_path))) {
                            ICLI_PRINTF("Could not perform vtss_ewis_path_txti_get() for port %u\n", uport);
                            continue;
                        }
                        // check the length of tti here and continue with assigning error status to rc variable.
                        length = strlen(tti);
                        if (tti_mode == VTSS_EWIS_TTI_MODE_1) {
                            explen = 1;
                        } else if (tti_mode == VTSS_EWIS_TTI_MODE_16) {
                            explen = 16;
                        } else if (tti_mode == VTSS_EWIS_TTI_MODE_64) {
                            explen = 64;
                        } else {
                            ICLI_PRINTF("TTI Mode mismatch\n");
                        }

                        if (length > explen) {
                            ICLI_PRINTF("TTI Length exceeds\n");
                            rc = VTSS_RC_ERROR;
                            continue;
                        }

                        tx_tti_path.mode = tti_mode;
                        if (tx_tti_path.mode == VTSS_EWIS_TTI_MODE_16) {
                            memcpy(tx_tti_path.tti, tti, 15);
                            tx_tti_path.tti[15] = 0x89; /* MSB set to 1 */
                        } else if (tx_tti_path.mode == VTSS_EWIS_TTI_MODE_64) {
                            memcpy(&tx_tti_path.tti[2], tti, 62);
                            tx_tti_path.tti[0] = 13; /* CR */
                            tx_tti_path.tti[1] = 10; /* LF */
                        } else if (tx_tti_path.mode == VTSS_EWIS_TTI_MODE_1) {
                            tx_tti_path.tti[0] = tti[0];
                        }

                        if (VTSS_RC_OK != (rc = vtss_ewis_path_txti_set(misc_phy_inst_get(), port_no, &tx_tti_path))) {
                            ICLI_PRINTF("Could not perform vtss_ewis_path_txti_set() for port %u\n", uport);
                            continue;
                        }
                    }
                } else {
                    if (VTSS_RC_OK != (rc = vtss_ewis_section_txti_get(misc_phy_inst_get(), port_no, &tx_tti_section))) {
                        ICLI_PRINTF("Could not perform vtss_ewis_section_txti_get() for port %u\n", uport);
                        continue;
                    }

                    ICLI_PRINTF("Port: %u\n", uport);
                    ICLI_PRINTF("--------\n");
                    ICLI_PRINTF("Section TTI Mode :%-12d \nSection TTI:", tx_tti_section.mode);
                    if (tx_tti_section.mode == 0) {
                        ICLI_PRINTF("%c", tx_tti_section.tti[0]);
                    } else if (tx_tti_section.mode == 1) {
                        /* 15th Char is Static charecter filled by API */
                        for (i = 0; i < 15; i++) {
                            ICLI_PRINTF("%c", tx_tti_section.tti[i]);
                        }
                    } else if (tx_tti_section.mode == 2) {
                        /* First two bytes are  filled by API */
                        for (i = 2; i < 64; i++) {
                            ICLI_PRINTF("%c", tx_tti_section.tti[i]);
                        }
                    }

                    ICLI_PRINTF("\n");
                    if (VTSS_RC_OK != (rc = vtss_ewis_path_txti_get(misc_phy_inst_get(), port_no, &tx_tti_path))) {
                        ICLI_PRINTF("Could not perform vtss_ewis_test_mode_get() operation");
                        continue;
                    }

                    ICLI_PRINTF("Port: %u\n", uport);
                    ICLI_PRINTF("--------\n");
                    ICLI_PRINTF("Path TTI Mode :%-12d \nPath TTI:", tx_tti_path.mode);
                    if (tx_tti_path.mode == 0) {
                        ICLI_PRINTF("%c", tx_tti_path.tti[0]);
                    } else if (tx_tti_path.mode == 1) {
                        /* 15th Char is Static charecter filled by API */
                        for (i = 0; i < 15; i++) {
                            ICLI_PRINTF("%c", tx_tti_path.tti[i]);
                        }
                    } else if (tx_tti_path.mode == 2) {
                        /* First two bytes are  filled by API */
                        for (i = 2; i < 64; i++) {
                            ICLI_PRINTF("%c", tx_tti_path.tti[i]);
                        }
                    }

                    ICLI_PRINTF("\n");
                }
            }
        }
    }

    return rc;
}

static mesa_rc MISC_ICLI_perm_chk(u32 session_id, const char *perm_filter, u32 index, const char *allowed)
{
    int i;

    for (i = 0; i < strlen(allowed); i++) {
        if (allowed[i] == perm_filter[index]) {
            return VTSS_RC_OK;
        }
    }

    if (session_id != ICLI_SESSION_ID_NONE) {
        ICLI_PRINTF("%% %d%s character of permission filter must be one of \"%s\"\n", index + 1, index == 0 ? "st" : index == 1 ? "nd" : index == 2 ? "rd" : "th", allowed);
    }

    return VTSS_RC_ERROR;
}

static BOOL MISC_ICLI_perm_filter_match(const char *perm_filter, const char *perm)
{
    int i;

    for (i = 0; i < strlen(perm_filter); i++) {
        if (perm_filter[i] == '.') {
            // Don't care.
            continue;
        }

        if (perm_filter[i] != perm[i]) {
            return FALSE; // No match
        }
    }

    return TRUE;
}

typedef struct {
    char pathname[200];
    char perm[200];
    u32  from;
    u32  to;
    u32  virt_size;
    u32  phys_size;
} pagemap_t;

static mesa_rc MISC_ICLI_pagemap_details_show(u32 session_id, int pid, char *section, const char *perm_filter)
{
    FILE      *maps_file /* It's a text file, so easier to use a FILE */;
    int       pagemap_fd = -1, res;
    char      maps_filename[60], pagemap_filename[60], line[200];
    u64       data;
    u32       offset, cnt, mapped, hits = 0, page_cnt;
    pagemap_t new_pagemap;
    mesa_rc   rc;

    if (session_id == ICLI_SESSION_ID_NONE) {
        T_E("Invoked with invalid session ID");
        return VTSS_RC_ERROR;
    }

    sprintf(maps_filename,    "/proc/%d/maps",    pid);
    sprintf(pagemap_filename, "/proc/%d/pagemap", pid);

    if ((maps_file = fopen(maps_filename, "r")) == NULL) {
        ICLI_PRINTF("%% Unable to open %s in R/O mode. Error: %s\n", maps_filename, strerror(errno));
        rc = VTSS_RC_ERROR;
        goto do_exit;
    }

    if ((pagemap_fd = open(pagemap_filename, O_RDONLY)) < 0) {
        ICLI_PRINTF("%% Unable to open %s in R/O mode. Error: %s\n", pagemap_filename, strerror(errno));
        rc = VTSS_RC_ERROR;
        goto do_exit;
    }

    ICLI_PRINTF("Section Name                             Page Idx   Virt Addr  Phys Addr  Raw Value\n");
    ICLI_PRINTF("---------------------------------------- ---------- ---------- ---------- ------------------\n");

    while (fgets(line, sizeof(line), maps_file) != NULL) {
        T_N("%s", line);

        memset(&new_pagemap, 0, sizeof(new_pagemap));

        // Format of a line in "maps":
        // address           perm offset   dev   inode      pathname
        // 00400000-015cb000 r-xp 00000000 01:00 321        /usr/bin/switch_app
        res = sscanf(line, "%x-%x %s %*s %*s %*s %s", &new_pagemap.from, &new_pagemap.to, new_pagemap.perm, new_pagemap.pathname);
        if (res != 3 && res != 4) {
            // pathname is not always sscanf'ed.
            ICLI_PRINTF("%% Unable to scan first two hex numbers and permissions from %s\n", line);
            continue;
        }

        if (strlen(new_pagemap.perm) != 4) {
            ICLI_PRINTF("%% Read perm = \"%s\" from file. Expected something of length 4\n", new_pagemap.perm);
            continue;
        }

        if (section && section[0] != '\0' && strcmp(new_pagemap.pathname, section) != 0) {
            continue;
        }

        // Check if user wants this section filtered based on permissions
        if (!MISC_ICLI_perm_filter_match(perm_filter, new_pagemap.perm)) {
            continue;
        }

#define PAGE_SIZE 4096

        offset = sizeof(u64) * (new_pagemap.from / PAGE_SIZE) /* page size */;
        page_cnt = (new_pagemap.to - new_pagemap.from) / PAGE_SIZE;

        if (lseek(pagemap_fd, offset, SEEK_SET) == -1) {
            ICLI_PRINTF("%% Unable to seek %u bytes into %s for pagemap entry \"%s\"\n", offset, pagemap_filename, line);
            continue;
        }

        T_D("%s: from = 0x%08x, to = 0x%08x, page_cnt = %u, offset = 0x%x (line = %s)", new_pagemap.pathname, new_pagemap.from, new_pagemap.to, page_cnt, offset, line);

        mapped = 0;
        for (cnt = 0; cnt < page_cnt; cnt++) {
            if (read(pagemap_fd, &data, sizeof(data)) == sizeof(data)) {
                // Bits 0-55  page frame number (PFN) if present
                // Bits 0-4   swap type if swapped
                // Bits 5-55  swap offset if swapped
                // Bits 55-60 page shift (page size = 1<<page shift)
                // Bit  61    reserved for future use
                // Bit  62    page swapped
                // Bit  63    page present
                if (data & 0x8000000000000000LLU) {
                    // Page is mapped
                    ICLI_PRINTF("%-40s %10u 0x%08x 0x%08x 0x" VPRI64x "\n", mapped == 0 ? new_pagemap.pathname : "", cnt, new_pagemap.from + (cnt * PAGE_SIZE), (u32)((data & 0x00FFFFFFFFFFFFFFLLU) * PAGE_SIZE), data);
                    mapped++;
                }
            } else {
                ICLI_PRINTF("%% Unable to read " VPRIz " bytes from %s at offset %u. Error = %s\n", sizeof(data), pagemap_filename, offset, strerror(errno));
            }
        }

#undef PAGE_SIZE

        if (mapped == 0) {
            ICLI_PRINTF("%-40s <No physically mapped pages>\n", new_pagemap.pathname);
        }

        hits++;
    }

    if (hits == 0) {
        ICLI_PRINTF("%-40s <No matches>\n", "");
    }

    if (section && section[0] != '\0' && hits != 1) {
        rc = VTSS_RC_ERROR;
    } else {
        rc = VTSS_RC_OK;
    }

do_exit:
    if (maps_file != NULL) {
        (void)fclose(maps_file);
    }

    if (pagemap_fd >= 0) {
        (void)close(pagemap_fd);
    }

    return rc;
}

static mesa_rc MISC_ICLI_pagemap_update(u32 session_id, const char *smaps_filename, pagemap_t *new_pagemap, BOOL changes_only, BOOL size_seen, BOOL rss_seen, u32 *physical_used, u32 *virt_total, u32 *phys_total)
{
    // Holds the values from previous invocation of this function.
    // Keyed by virtual start address.
    static vtss::Map<u32, pagemap_t> old_pagemaps;
    pagemap_t old_pagemap;

    if (!size_seen || !rss_seen) {
        if (session_id != ICLI_SESSION_ID_NONE) {
            ICLI_PRINTF("%% During parsing of section \"%s\" of %s: \"%s\" not found\n", new_pagemap->pathname, smaps_filename, size_seen ? "Rss" : "Size");
        }

        return VTSS_RC_ERROR;
    }

    if (physical_used) {
        *physical_used = new_pagemap->phys_size;
    }

    if (session_id != ICLI_SESSION_ID_NONE) {
        auto itr = old_pagemaps.find(new_pagemap->from);
        if (itr == old_pagemaps.end()) {
            // Not found before
            memset(&old_pagemap, 0, sizeof(old_pagemap));
        } else {
            old_pagemap = itr->second;
        }

#define PLUS_MINUS_STR(_n_, _o_) (strcmp(_n_, _o_) ? '*' : ' ')
#define PLUS_MINUS_NUM(_n_, _o_) ((_n_) < (_o_) ? '-' : (_n_) > (_o_) ? '+' : ' ')
        if (!changes_only || memcmp(new_pagemap, &old_pagemap, sizeof(*new_pagemap)) != 0) {
            ICLI_PRINTF("%-40s %4s%c 0x%08x%c 0x%08x%c %10u%c %10u%c\n",
                        new_pagemap->pathname,
                        new_pagemap->perm,      PLUS_MINUS_STR(new_pagemap->perm,      old_pagemap.perm),
                        new_pagemap->from,      PLUS_MINUS_NUM(new_pagemap->from,      old_pagemap.from),
                        new_pagemap->to,        PLUS_MINUS_NUM(new_pagemap->to,        old_pagemap.to),
                        new_pagemap->virt_size, PLUS_MINUS_NUM(new_pagemap->virt_size, old_pagemap.virt_size),
                        new_pagemap->phys_size, PLUS_MINUS_NUM(new_pagemap->phys_size, old_pagemap.phys_size));

            old_pagemaps.set(new_pagemap->from, *new_pagemap);
        }
#undef PLUS_MINUS_STR
#undef PLUS_MINUS_NUM

        *virt_total += new_pagemap->virt_size;
        *phys_total += new_pagemap->phys_size;
    }

    return VTSS_RC_OK;
}

static mesa_rc MISC_ICLI_pagemap_get(u32 session_id, char *section, u32 *physical_used, int pid, const char *permissions, BOOL details, BOOL changes_only)
{
    static u32 old_virt_total, old_phys_total;
    FILE       *smaps_file /* It's a text file, so easier to use a FILE */;
    int        res;
    char       smaps_filename[60], line[200], perm_filter[5];
    u32        hits = 0, virt_total = 0, phys_total = 0;
    BOOL       started = FALSE, size_seen = FALSE, rss_seen = FALSE;
    pagemap_t  new_pagemap;
    mesa_rc    rc;
    int        our_pid = getpid();

    // Check the permissions filter.
    if (permissions) {
        if (strlen(permissions) != 4) {
            if (session_id != ICLI_SESSION_ID_NONE) {
                ICLI_PRINTF("%% Permissions filter must be exactly 4 chars long\n");
            }

            return VTSS_RC_ERROR;
        }

        strcpy(perm_filter, permissions);
    } else {
        // Default: Print all (by using '.' as wildcard).
        strcpy(perm_filter, "....");
    }

    VTSS_RC(MISC_ICLI_perm_chk(session_id, perm_filter, 0, "r-."));
    VTSS_RC(MISC_ICLI_perm_chk(session_id, perm_filter, 1, "w-."));
    VTSS_RC(MISC_ICLI_perm_chk(session_id, perm_filter, 2, "x-."));
    VTSS_RC(MISC_ICLI_perm_chk(session_id, perm_filter, 3, "ps-."));

    if (details) {
        return MISC_ICLI_pagemap_details_show(session_id, pid, section, perm_filter);
    }

    sprintf(smaps_filename, "/proc/%d/smaps", pid);

    if ((smaps_file = fopen(smaps_filename, "r")) == NULL) {
        if (session_id != ICLI_SESSION_ID_NONE) {
            ICLI_PRINTF("%% Unable to open %s in R/O mode. Error: %s\n", smaps_filename, strerror(errno));
        }

        rc = VTSS_RC_ERROR;
        goto do_exit;
    }

    if (session_id != ICLI_SESSION_ID_NONE) {
        ICLI_PRINTF("Section Name                             Perm  Virt Start  Virt End    Virt Size   Phys Size\n");
        ICLI_PRINTF("---------------------------------------- ----  ----------  ----------  ----------  ----------\n");
    }

    // Parse file
    while (fgets(line, sizeof(line), smaps_file) != NULL) {
        char str1[200], str2[200];
        u32  u1, u2;

        // Format of first line of each section in "smaps":
        // address           perm offset   dev   inode      pathname
        // 00400000-015cb000 r-xp 00000000 01:00 321        /usr/bin/switch_app

        // Format of subsequent lines in "smaps":
        // MemoryType            size kB
        // Rss:                  12 kB

        str2[0] = '\0';
        res = sscanf(line, "%x-%x %s %*s %*s %*s %s", &u1, &u2, str1, str2);
        if (res == 3 || res == 4) {
            // We're at a section header. res is 3 when pathname is not available.

            if (started) {
                // Output previous section info
                if (MISC_ICLI_pagemap_update(session_id, smaps_filename, &new_pagemap, changes_only, size_seen, rss_seen, physical_used, &virt_total, &phys_total) == VTSS_RC_OK) {
                    hits++;
                }

                started = size_seen = rss_seen = FALSE;
            }

            if (strlen(str1) != 4) {
                if (session_id != ICLI_SESSION_ID_NONE) {
                    ICLI_PRINTF("%% Read perm = \"%s\" from %s. Expected something of length 4\n", str1, smaps_filename);
                }

                continue;
            }

            // Check if user wants this section filtered based on permissions
            if (!MISC_ICLI_perm_filter_match(perm_filter, str1)) {
                continue;
            }

            if (pid == our_pid && str2[0] == '\0') {
                int tid;
                // We are asked for the page map for our own application and the
                // section name is (currently) empty. Check to see if one of the
                // thread stacks are using this area, and if so, give it a name.
                // On uclibc, the thread stack is already given a name, like
                // [stack:82], which says: Thread ID 82 is using this section as
                // stack, but glibc doesn't provide it.
                if ((tid = vtss_thread_id_from_thread_stack_range_get((void *)(uintptr_t)u1, (void *)(uintptr_t)u2)) != 0) {
                    snprintf(str2, sizeof(str2), "[stack:%d]", tid);
                }
            }

            // Check if user wants this section.
            if (section && section[0] != '\0' && strcmp(section, str2) != 0) {
                continue;
            }

            memset(&new_pagemap, 0, sizeof(new_pagemap));
            new_pagemap.from = u1;
            new_pagemap.to   = u2;
            strcpy(new_pagemap.perm, str1);
            strcpy(new_pagemap.pathname, str2);
            started = TRUE;
        } else if (started) {
            // Attempt to read a subsequent line.
            res = sscanf(line, "%s %u kB", str1, &u1);
            if (res != 1 && res != 2) {
                if (session_id != ICLI_SESSION_ID_NONE) {
                    ICLI_PRINTF("%% Expected \"<string>: [<number> kB]\", got \"%s\" when reading from %s. Parsed args = %d\n", line, smaps_filename, res);
                }

                continue;
            }

            if (res == 1) {
                // Only parsed one arg, so silently skip across lines like this:
                // "VmFlags: rd wr mr mw me ac"
                continue;
            }

            // We're interested in two numbers from each section, namely "Size" and "Rss".
            u1 *= 1024; // It's in kBytes in the file and we report in bytes
            if (strcmp(str1, "Size:") == 0) {
                size_seen = TRUE;
                new_pagemap.virt_size = u1;
            } else if (strcmp(str1, "Rss:") == 0) {
                rss_seen = TRUE;
                new_pagemap.phys_size = u1;
            }
        }
    }

    if (started) {
        // Output last section info
        if (MISC_ICLI_pagemap_update(session_id, smaps_filename, &new_pagemap, changes_only, size_seen, rss_seen, physical_used, &virt_total, &phys_total) == VTSS_RC_OK) {
            hits++;
        }
    }

#define PLUS_MINUS_NUM(_n_, _o_) ((_n_) < (_o_) ? '-' : (_n_) > (_o_) ? '+' : ' ')
    if (session_id != ICLI_SESSION_ID_NONE) {
        if (hits == 0) {
            ICLI_PRINTF("%-40s <No matches>\n", "");
        } else {
            ICLI_PRINTF("---------------------------------------- ----  ----------  ----------  ----------  ----------\n");
            ICLI_PRINTF("%-70s %10u%c %10u%c\n", "Total", virt_total, PLUS_MINUS_NUM(virt_total, old_virt_total), phys_total, PLUS_MINUS_NUM(phys_total, old_phys_total));

            old_virt_total = virt_total;
            old_phys_total = phys_total;
        }
    }

#undef PLUS_MINUS_STR
#undef PLUS_MINUS_NUM

    if (section && section[0] != '\0' && hits == 0) {
        rc = VTSS_RC_ERROR;
    } else {
        rc = VTSS_RC_OK;
    }

do_exit:
    if (smaps_file != NULL) {
        (void)fclose(smaps_file);
    }

    return rc;
}

mesa_rc misc_icli_pagemap_show(u32 session_id, char *section, int pid, const char *perm, BOOL details, BOOL changes_only)
{
    return MISC_ICLI_pagemap_get(session_id, section, NULL, pid, perm, details, changes_only);
}

/******************************************************************************/
// misc_icli_vmstat_show()
/******************************************************************************/
mesa_rc misc_icli_vmstat_show(u32 session_id, BOOL changes_only)
{
    vtss_proc_vmstat_t        vmstat;
    mesa_rc                   rc;
    int                       i;
    static vtss_proc_vmstat_t *prev_vmstat;     // Demand-alloc RAM upon use, never free

    if ((rc = misc_proc_vmstat_parse(&vmstat)) != VTSS_RC_OK) {
        ICLI_PRINTF("%% Unable to parse proc/vmstat\n");
        return rc;
    }

    ICLI_PRINTF("%-*s %14s %9s %9s\n", vmstat.max_name_wid, "Field", "Current", "Previous", "Delta");

    for (i = 0; i < ARRSZ(vmstat.tlv); i++) {
        if (vmstat.tlv[i].name[0] == '\0') {
            break;
        }

        if (!prev_vmstat) {
            // no prev data yet
            ICLI_PRINTF("%-*s %14lu\n", vmstat.max_name_wid, vmstat.tlv[i].name, vmstat.tlv[i].value);
        } else {
            // Have prev data -- here we assume that the set of fields is constant over time, also wrt. ordering. Hope it's a safe assumption...
            signed long delta = vmstat.tlv[i].value - prev_vmstat->tlv[i].value;
            if (!changes_only || (changes_only && delta != 0)) {
                ICLI_PRINTF("%-*s %14lu %9lu %9ld\n", vmstat.max_name_wid, vmstat.tlv[i].name, vmstat.tlv[i].value, prev_vmstat->tlv[i].value, delta);
            }
        }
    }

    if (!prev_vmstat) {
        prev_vmstat = (vtss_proc_vmstat_t *)VTSS_MALLOC(sizeof(*prev_vmstat));
    }

    if (prev_vmstat) {
        *prev_vmstat = vmstat;
    }

    return VTSS_RC_OK;
}

#if defined(VTSS_SW_OPTION_THREAD_LOAD_MONITOR)
/******************************************************************************/
// misc_icli_pgfaults_show()
/******************************************************************************/
mesa_rc misc_icli_pgfaults_show(u32 session_id)
{
    thread_load_monitor_page_faults_t page_faults;
    bool                              threadload_started;
    mesa_rc                           rc;

    if ((rc = thread_load_monitor_page_faults_get(page_faults, threadload_started)) != VTSS_RC_OK) {
        (void)ICLI_PRINTF("%% Unable to get page faults. Error = %s\n", error_txt(rc));
    }

    ICLI_PRINTF("Last 1sec  Last 10sec Total\n");
    ICLI_PRINTF("---------- ---------- ----------\n");

    if (threadload_started) {
        ICLI_PRINTF("%10u %10u %10u\n", page_faults.one_sec, page_faults.ten_sec, page_faults.total);
    } else {
        ICLI_PRINTF("<Use 'debug thread load-monitor' to enable counting of page faults>\n");
    }

    return VTSS_RC_OK;
}
#endif

/******************************************************************************/
// misc_icli_thread_stack_size_get()
/******************************************************************************/
u32 misc_icli_thread_stack_size_get(int tid)
{
    char section[32];
    u32  result;

    sprintf(section, "[stack:%d]", tid);

    if (MISC_ICLI_pagemap_get(ICLI_SESSION_ID_NONE, section, &result, getpid(), NULL, FALSE, FALSE) != VTSS_RC_OK) {
        printf("Error: No hits on %s\n", section);
        return 0;
    }

    return result;
}

mesa_rc misc_icli_slabinfo_show(u32 session_id, BOOL changes_only)
{
    typedef struct {
        u32  active_objs;
        u32  objsize;
        bool updated;
    } slabinfo_t;

    static     vtss::Map<std::string, slabinfo_t> old_slab_sizes; // Holds the values from previous time.
    static     u32 old_total;

    FILE       *slabinfo_file /* It's a text file, so easier to use a FILE */;
    int        res;
    const char *filename = "/proc/slabinfo";
    char       slabname[33], line[300];
    u32        new_size, new_total = 0, line_cnt = 0;
    slabinfo_t old_slab_size, new_slab_size;
    mesa_rc    rc;

#define PLUS_MINUS(_n_, _o_) ((_n_) < (_o_) ? '-' : (_n_) > (_o_) ? '+' : ' ')

    if ((slabinfo_file = fopen(filename, "r")) == NULL) {
        ICLI_PRINTF("%% Unable to open %s in R/O mode. Error: %s\n", filename, strerror(errno));
        rc = VTSS_RC_ERROR;
        goto do_exit;
    }

    for (auto itr = old_slab_sizes.begin(); itr != old_slab_sizes.end(); ++itr) {
        itr->second.updated = false;
    }

    ICLI_PRINTF("Slab Name                        Actv. Objs  Obj. Size   Total\n");
    ICLI_PRINTF("-------------------------------- ----------  ----------  ----------\n");

    while (fgets(line, sizeof(line), slabinfo_file) != NULL) {
        T_D("%s", line);
        line_cnt++;

        // Format of a line in "slabinfo":
        // # name            <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab> : tunables <limit> <batchcount> <sharedfactor> : slabdata <active_slabs> <num_slabs> <sharedavail>
        // ubifs_inode_slab      10     10    384   10
        // 00400000-015cb000 r-xp 00000000 01:00 321        /usr/bin/switch_app
        if ((res = sscanf(line, "%s %u %*u %u", slabname, &new_slab_size.active_objs, &new_slab_size.objsize)) != 3) {
            if (line_cnt > 2) {
                // The first two lines are headers. Don't print warnings for them.
                ICLI_PRINTF("%% Unable to scan a string and three u32s from %s\n", line);
            }

            continue;
        }

        new_slab_size.updated = true;
        slabname[sizeof(slabname) - 1] = '\0';

        new_size  = new_slab_size.active_objs * new_slab_size.objsize;
        new_total += new_size;

        auto itr = old_slab_sizes.find(slabname);
        if (itr == old_slab_sizes.end()) {
            // Not found before
            memset(&old_slab_size, 0, sizeof(old_slab_size));
        } else {
            old_slab_size = itr->second;
        }

        if (!changes_only || new_slab_size.active_objs != old_slab_size.active_objs || new_slab_size.objsize != old_slab_size.objsize) {
            ICLI_PRINTF("%-32s %10u%c %10u%c %10u%c\n",
                        slabname,
                        new_slab_size.active_objs, PLUS_MINUS(new_slab_size.active_objs, old_slab_size.active_objs),
                        new_slab_size.objsize,     PLUS_MINUS(new_slab_size.objsize,     old_slab_size.objsize),
                        new_size,                  PLUS_MINUS(new_size,                  old_slab_size.active_objs * old_slab_size.objsize));
        }

        old_slab_sizes.set(slabname, new_slab_size);
    }

    ICLI_PRINTF("-------------------------------- ----------  ----------  ----------\n");
    ICLI_PRINTF("%-56s %10u%c\n", "Total", new_total, PLUS_MINUS(new_total, old_total));

    old_total = new_total;

    rc = VTSS_RC_OK;

do_exit:
    for (auto itr = old_slab_sizes.begin(); itr != old_slab_sizes.end();) {
        if (!itr->second.updated) {
            old_slab_sizes.erase(itr++);
        } else {
            ++itr;
        }
    }

    if (slabinfo_file != NULL) {
        (void)fclose(slabinfo_file);
    }

    return rc;
}

mesa_rc misc_icli_debug_port_state(u32 session_id, icli_stack_port_range_t *plist, BOOL enable)
{
    port_iter_t   pit;

    VTSS_RC(icli_port_iter_init(&pit, 1, PORT_ITER_FLAGS_FRONT | PORT_ITER_FLAGS_NPI));
    while (icli_port_iter_getnext(&pit, plist)) {
        (void)misc_debug_port_state(pit.iport, enable);
    }

    return VTSS_RC_OK;
}

mesa_rc vtss_vscope_scan_icli(u32 session_id, icli_stack_port_range_t *v_port_type_list, BOOL has_fast_scan,
                              u32 v_0_to_10000, BOOL has_enable, BOOL has_disable, BOOL has_xy_scan, BOOL has_enable_1,
                              u32 v_0_to_127, u32 v_0_to_63, u32 v_0_to_127_1, u32 v_0_to_63_1, u32 v_0_to_10,
                              u32 v_0_to_10_1, u32 v_0_to_64, u32 v_0_to_10000_1, BOOL has_disable_1)
{
    u32 range_idx = 0, cnt_idx = 0;
    i32 i, j;
    mesa_port_no_t uport;
    mesa_vscope_conf_t conf = {};
    mesa_vscope_scan_conf_t conf_scan = {};
    mesa_vscope_scan_status_t  conf1 = {};
    mesa_rc rc = VTSS_RC_ERROR;

    if (v_port_type_list) { //at least one range input
        for (range_idx = 0; range_idx < v_port_type_list->cnt; range_idx++) {
            for (cnt_idx = 0; cnt_idx < v_port_type_list->switch_range[range_idx].port_cnt; cnt_idx++) {
                uport = v_port_type_list->switch_range[range_idx].begin_uport + cnt_idx;
                if (has_fast_scan) {
                    conf.scan_type = MESA_VSCOPE_FAST_SCAN;

                    if (has_enable) {
                        conf.enable = TRUE;
                        conf.error_thres = v_0_to_10000;
                    } else {
                        conf.enable = FALSE;
                    }
                }

                if (has_xy_scan) {
                    conf.scan_type = MESA_VSCOPE_FULL_SCAN;

                    if (has_enable_1) {
                        conf.error_thres = v_0_to_10000_1;
                        conf.enable = TRUE;

                        conf_scan.x_start = v_0_to_127;
                        conf_scan.y_start = v_0_to_63;
                        conf_scan.x_count = v_0_to_127_1;
                        conf_scan.y_count = v_0_to_63_1;
                        conf_scan.x_incr = v_0_to_10;
                        conf_scan.y_incr = v_0_to_10_1;
                        conf_scan.ber = v_0_to_64;
                        conf1.scan_conf = conf_scan;
                    } else {
                        conf.enable = FALSE;
                    }
                }

                if (has_enable_1) {
                    if (conf_scan.x_count + conf_scan.x_start > 127) {
                        ICLI_PRINTF("The points exceed bounds, make sure that x-count + x-start <= 127\n");
                        return VTSS_RC_ERROR;
                    }

                    if (conf_scan.y_count + conf_scan.y_start > 63) {
                        ICLI_PRINTF("The points exceed bounds, make sure that y-count + y-start <= 63\n");
                        return VTSS_RC_ERROR;
                    }
                }

                if ((rc = mesa_vscope_conf_set(NULL, uport2iport(uport), &conf)) != VTSS_RC_OK) {
                    ICLI_PRINTF("ERROR: VSCOPE configuration failed\n");
                    return VTSS_RC_ERROR;
                } else {
                    ICLI_PRINTF("VSCOPE successfully configured\n");
                }

                if (has_enable | has_enable_1) {
                    if ((rc = mesa_vscope_scan_status_get(NULL, uport2iport(uport), &conf1)) != VTSS_RC_OK) {
                        ICLI_PRINTF("ERROR: Fetching of VSCOPE data failed\n");
                        return VTSS_RC_ERROR;
                    } else {
                        if (has_fast_scan) {
                            ICLI_PRINTF("error free x points: %d\n", conf1.error_free_x);
                            ICLI_PRINTF("error free y points: %d\n", conf1.error_free_y);
                            ICLI_PRINTF("amplitude range: %d\n", conf1.amp_range);
                        } else if (has_xy_scan) {
                            for (j = conf_scan.y_start; j < (conf_scan.y_start + conf_scan.y_count); j = j + conf_scan.y_incr + 1) {
                                if (conf1.errors[0][j] == 0) {
                                    j++;
                                }
                                ICLI_PRINTF("\n");
                                for (i = conf_scan.x_start; i < (conf_scan.x_start + conf_scan.x_count) ; i = i + conf_scan.x_incr + 1) {
                                    if (conf1.errors[i][j] == 0) {
                                        ICLI_PRINTF("X");
                                    } else {
                                        ICLI_PRINTF(".");
                                    }
                                }
                            }
                            ICLI_PRINTF("\n");
                        } else {
                            ICLI_PRINTF("Scan Type not supported\n");
                        }
                    }
                }
            }
        }
    }
    return rc;
}
