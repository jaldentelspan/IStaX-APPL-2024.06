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

/*
 Microchip is aware that some terminology used in this technical document is
 antiquated and inappropriate. As a result of the complex nature of software
 where seemingly simple changes have unpredictable, and often far-reaching
 negative results on the software's functionality (requiring extensive retesting
 and revalidation) we are unable to make the desired changes in all legacy
 systems without compromising our product or our clients' products.
*/

#ifndef _VTSS_PTP_API_H_
#define _VTSS_PTP_API_H_
/**
 * \file vtss_ptp_api.h
 * \brief PTP protocol engine main API header file
 *
 * This file contain the definitions of API functions and associated
 * types.
 *
 */

#include "vtss_ptp_packet_callout.h"
#include "vtss/appl/ptp.h"
#include "vtss_ptp_ms_servo.h"
#include "vtss_ptp_clock.h"
#include "vtss_ptp_port.h"

#define SW_OPTION_BASIC_PTP_SERVO // Basic Servo needs to be encluded for the purpose of calibration

#ifdef SW_OPTION_BASIC_PTP_SERVO
#include "vtss_ptp_basic_servo.h"
#endif // SW_OPTION_BASIC_PTP_SERVO

/*
 * Trace group numbers
 */
#ifdef __cplusplus
        extern "C" {
#endif

#define VTSS_TRACE_GRP_PTP_BASE_TIMER        1
#define VTSS_TRACE_GRP_PTP_BASE_PACK_UNPACK  2
#define VTSS_TRACE_GRP_PTP_BASE_MASTER       3
#define VTSS_TRACE_GRP_PTP_BASE_SLAVE        4
#define VTSS_TRACE_GRP_PTP_BASE_STATE        5
#define VTSS_TRACE_GRP_PTP_BASE_FILTER       6
#define VTSS_TRACE_GRP_PTP_BASE_PEER_DELAY   7
#define VTSS_TRACE_GRP_PTP_BASE_TC           8
#define VTSS_TRACE_GRP_PTP_BASE_BMCA         9
#define VTSS_TRACE_GRP_PTP_BASE_CLK_STATE    10
#define VTSS_TRACE_GRP_PTP_BASE_802_1AS      11
#define VTSS_TRACE_GRP_PTP_CNT               11  /* must be = number of BASE trace groups */

/**
 * \brief Static part of Clock Default Data Set structure.
 * The contents of this structure is defined at clock init time.
 */
typedef struct ptp_init_clock_ds_t {
    vtss_appl_ptp_clock_config_default_ds_t cfg;
    i16 max_foreign_records;
    i16 max_outstanding_records;  /* max number of simultaneous outstanding Delay requests in a E2E transparent clock */
    u16 numberPorts;
    u16 numberEtherPorts;
    vtss_appl_clock_identity clockIdentity;
    bool afi_announce_enable;   // true if Automatic announce frame injection is used
    bool afi_sync_enable;       // true if Automatic sync frame injection is used
} ptp_init_clock_ds_t;

/**
 * \brief Foreign Master Data Set structure
 */
typedef struct ptp_foreign_ds_t {
    vtss_appl_ptp_port_identity foreignmasterIdentity;
    vtss_appl_clock_quality     foreignmasterClockQuality;
    u8                          foreignmasterPriority1;
    u8                          foreignmasterPriority2;
    u8                          foreignmasterLocalPriority;
    bool                        best;              /* true if evaluated as best master on the port */
    bool                        qualified;         /* true if the foreign master is qualified */
} ptp_foreign_ds_t;

/**
 * \brief Unicast slave configuration/state structure
 */
typedef struct vtss_ptp_unicast_slave_config_state_t {
    u32  duration;
    u32  ip_addr;
    i8   log_msg_period; // the granted sync interval
    u8   comm_state;
} vtss_ptp_unicast_slave_config_state_t;

/**
 * \brief Clock Slave Data Set structure
 */

const char * vtss_ptp_slave_state_2_text(vtss_appl_ptp_slave_clock_state_t s);
char *vtss_ptp_ho_state_2_text(bool stable, i64 adj, char *str, bool details);


/**
 * \brief Create PTP instance.
 *
 * The (maximum) number of ports in the clock is defined in the rtOpts
 * structure.
 * This defines the valid port numbers for this instance to lie in.
 * Ports are specifically enabled/disabled with vtss_ptp_port_ena() and
 * vtss_ptp_port_dis() interfaces.
 *
 * \param config [IN]  pointer to a structure containing the default parameters
 * to the clock.
 *
 * \return (opaque) instance data reference or NULL.
 */
ptp_clock_t *vtss_ptp_clock_add(ptp_init_clock_ds_t *clock_init,
                                vtss_appl_ptp_clock_timeproperties_ds_t *time_prop,
                                vtss_appl_ptp_config_port_ds_t *port_config,
                                ptp_servo *servo,
                                int localClockId);

/**
 * \brief Remove PTP instance.
 * The memory allocated for the clock are free'ed
 *
 * \param ptp_clock_t *The PTP clock instance handle.
 *
 */
void vtss_ptp_clock_remove(ptp_clock_t *ptp);

/**
 * \brief create (initialize) clock .
 * The instance has been created by vtss_ptp_clock_add.
 * the internal clock structures are initialized.
 * Performs the INITIALIZE event (P1588 9.2.6.3)
 *
 * \param ptp The PTP instance data.
 *
 * \return nothing
 */
void vtss_ptp_clock_create(ptp_clock_t *ptpClock);

/**
 * \brief Change offset filter and servo.
 *
 * \param ptp [IN/OUT] The PTP instance data.
 * \param delay_filt The filter/servo instance
 *
 * \return true
 */
bool vtss_ptp_clock_servo_set(ptp_clock_t *ptp, ptp_servo *servo);

/**
 * \brief Definition of a pointer to a callback funtion used for updating the PTP UDP filter.
 */
typedef mesa_rc (*vtss_ptp_udp_rx_filter_update_cb_t)(uint clockId, bool usesUDP);

/**
 * \brief Function for installing the pointer to the rx filter callback function.
 * 
 * \param cb [IN]  A pointer to the callback function.
 */
void vtss_ptp_install_udp_rx_filter_update_cb(const vtss_ptp_udp_rx_filter_update_cb_t cb);

/**
 * \brief enable clock port.
 * Performs the DESIGNATED_ENABLED event (P1588 9.2.6.4)
 *
 * \param ptp [IN/OUT] The PTP instance data.
 *
 * \param portnum The clock port to enable
 *
 * \return true if the operation succeeded. The operation will fail
 * if the given port number is < 1 or > the maximum allowed port
 * number (see vtss_ptp_create_instance()).
 */
mesa_rc vtss_ptp_port_ena(ptp_clock_t *ptp, uint portnum);
mesa_rc vtss_ptp_port_ena_virtual(ptp_clock_t *ptp, uint portnum);

/**
 * \brief disable clock port.
 * Performs the DESIGNATED_DISABLED event (P1588 9.2.6.5)
 *
 * \param ptp The PTP instance data.
 *
 * \param portnum The clock port to disable
 *
 * \return true if the operation succeeded. The operation will fail
 * if the given port number is < 1 or > the maximum allowed port
 * number (see vtss_ptp_create_instance()).
 */
mesa_rc vtss_ptp_port_dis(ptp_clock_t *ptp, uint portnum);

/**
 * \brief set link state for a clock port.
 *
 * \param ptp [IN/OUT] The PTP instance data.
 *
 * \param portnum The clock port
 *
 * \param enable true = Link Up
 *               false = Link down
 *
 * \return VTSS_RC_OK if the operation succeeded. The operation will fail
 * if the given port number is < 1 or > the maximum allowed port
 * number (see vtss_ptp_create_instance()).
 */
mesa_rc vtss_ptp_port_linkstate(ptp_clock_t *ptp, uint portnum, bool enable);

/**
 * \brief set link state for a internal TC clock port.
 *
 * \param ptp [IN/OUT] The PTP instance data.
 *
 * \param portnum The clock port
 *
 * \return VTSS_RC_OK if the operation succeeded. The operation will fail
 * if the given port number is < 1 or > the maximum allowed port
 * number (see vtss_ptp_create_instance()).
 */
mesa_rc vtss_ptp_port_internal_linkstate(ptp_clock_t *ptp, uint portnum);

/**
 * \brief set p2p state for a clock port.
 *
 * \param ptp [IN/OUT] The PTP instance data.
 * \param portnum The clock port
 * \param enable true  = P2P state Up
 *               false = P2P state down
 *
 * \return VTSS_RC_OK if the operation succeeded. The operation will fail
 * if the given port number is < 1 or > the maximum allowed port
 * number (see vtss_ptp_create_instance()).
 */
mesa_rc vtss_ptp_p2p_state(ptp_clock_t *ptp, uint portnum, bool enable);

/**
 * \brief  Read config part of clock default data set
 *
 * Purpose: To obtain information regarding the clock's default data set
 *
 * \param ptp The PTP instance data.
 *
 * \param status The clock default data set
 */
mesa_rc vtss_ptp_get_clock_default_ds_cfg(const ptp_clock_t *ptp, vtss_appl_ptp_clock_config_default_ds_t *default_ds_cfg);

/**
 * \brief Set config part of clock default data set
 *
 * Purpose: To set information regarding the Clock's Default data
 *
 * \param ptp The PTP instance data.
 *
 * \param default_ds The Default Data Set
 */
mesa_rc vtss_ptp_set_clock_default_ds_cfg(ptp_clock_t *ptp, const vtss_appl_ptp_clock_config_default_ds_t *default_ds_cfg);

/**
 * \brief Read status part of clock default data set
 *
 * Purpose: To obtain information regarding the clock's default data set
 *
 * \param ptp The PTP instance data.
 *
 * \param status The clock default data set
 */
mesa_rc vtss_ptp_get_clock_default_ds_status(const ptp_clock_t *ptp, vtss_appl_ptp_clock_status_default_ds_t *default_ds_status);

/**
* \brief Set Clock Quality
*
* Purpose: To set information regarding the Clock's Quality level
*
* \param ptp The PTP instance data.
*
* \param quality The Quality level
*/
mesa_rc vtss_ptp_set_clock_quality(ptp_clock_t *ptp, const vtss_appl_clock_quality *quality);

/**
 * \brief Read clock current data set
 *
 * Purpose: To obtain information regarding the clock's current data set
 *
 * \param ptp The PTP instance data.
 *
 * \param status clock current data set
 */
void vtss_ptp_get_clock_current_ds(const ptp_clock_t *ptp, vtss_appl_ptp_clock_current_ds_t *status);

/**
 * \brief Read Clock parent data set.
 *
 * Purpose: To obtain information regarding the Clock's parent data set
 *
 * \param ptp The PTP instance data.
 *
 * \param status clock parent data set
 */
void vtss_ptp_get_clock_parent_ds(const ptp_clock_t *ptp, vtss_appl_ptp_clock_parent_ds_t *status);

/**
 * \brief Read Clock slave data set.
 *
 * Purpose: To obtain information regarding the Clock's slave state data set
 *
 * \param ptp The PTP instance data.
 *
 * \param status clock slave data set
 */
void vtss_ptp_get_clock_slave_ds(const ptp_clock_t *ptp, vtss_appl_ptp_clock_slave_ds_t *slave_ds);

/**
 * Read Clock Time Properties Data Set
 *
 * Purpose: To obtain information regarding the Clock's Time Properties data
 * Protocol Entity.
 *
 * \param ptp [IN] The PTP instance data.
 *
 * \param timeproperties_ds [OUT] The Time properties Data Set
 */
void vtss_ptp_get_clock_timeproperties_ds(const ptp_clock_t *ptp, vtss_appl_ptp_clock_timeproperties_ds_t *timeproperties_ds);

/**
 * Write Clock Time Properties Data Set
 *
 * Purpose: To set information regarding the Clock's Time Properties data
 * Protocol Entity.
 *
 * \param ptp [OUT]The PTP instance data.
 *
 * \param timeproperties_ds [IN] The Time properties Data Set
 */
void vtss_ptp_set_clock_timeproperties_ds(ptp_clock_t *ptp, const vtss_appl_ptp_clock_timeproperties_ds_t *timeproperties_ds);

/**
 * \brief Read port data set
 *
 * Purpose: To obtain information regarding the clock's port
 *
 * \param ptp The PTP instance data.
 *
 * \param portnum The PTP port number.
 *
 * \param status port data set
 *
 * \return true if valid portnum, otherwise false
 */
mesa_rc vtss_ptp_get_port_status(const ptp_clock_t *ptp, uint portnum, vtss_appl_ptp_status_port_ds_t *status);

/**
 * \brief Read port parameter statistics
 *
 * Purpose: To obtain the clock's port parameter statistics
 *
 * \param ptp The PTP instance data.
 * \param portnum The PTP port number.
 * \param statistics  port parameter statistics
 * \param clear [in]  if true:: Clear after read.
 * \return true if valid portnum, otherwise false
 */
mesa_rc vtss_ptp_get_port_statistics(const ptp_clock_t *ptp, uint portnum, vtss_appl_ptp_status_port_statistics_t *port_statistics, BOOL clear);

/**
 * \brief Set Port Data Set
 *
 * Purpose: To set information regarding the Port's data
 *
 * \param ptp The PTP instance data.
 *
 * \param portnum The PTP port number.
 *
 * \param port_ds The Port Data Set
 *
 * \return true if valid portnum, otherwise false
 */
mesa_rc vtss_ptp_set_port_cfg(ptp_clock_t *ptp, uint portnum, const vtss_appl_ptp_config_port_ds_t *port_ds);

/**
 * \brief Get Port Foreign master Data Set
 *
 * Purpose: To get information regarding the Port's foreign masters
 *
 * \param ptp The PTP instance data.
 * \param portnum The PTP port number.
 * \param ix index in the list of foreign data.
 * \param foreign_ds The Foreign master Data Set
 *
 * \return true if valid portnum and ix, otherwise false
 */
bool vtss_ptp_get_port_foreign_ds(ptp_clock_t *ptp, uint portnum, i16 ix, ptp_foreign_ds_t *f_ds);

/**
 * \brief Set Unicast Slave Configuration
 *
 * Purpose: To set information regarding the Unicast slave
 *
 * \param ptp The PTP instance data.
 *
 * \param c The Unicast Slave Configuration
 *
 * \return true if valid portnum, otherwise false
 */
void vtss_ptp_uni_slave_conf_set(ptp_clock_t *ptp, int ix, const vtss_appl_ptp_unicast_slave_config_t *c);

/**
 * \brief Get Unicast Slave Configuration
 *
 * Purpose: To get information regarding the Unicast slave
 *
 * \param ptp The PTP instance data.
 *
 * \param c The Unicast Slave Configuration
 *
 * \return true if valid portnum, otherwise false
 */
void vtss_ptp_uni_slave_conf_get(ptp_clock_t *ptp, int ix, vtss_appl_ptp_unicast_slave_config_t *c);

/**
 * \brief Tick PTP state-event machines.
 * This shall be called every 500msec by the host system.
 *
 * \param ptp The PTP instance data.
 *
 * The function drives state for the PTP instance.
 */

#define PTP_LOG_TICKS_PR_SEC 7 /* 0 = 1 tick/s, 1 = 2 tick/s, 2 = 4 ticks/s etc. 7 = 128 ticks pr sec*/
void vtss_ptp_tick(ptp_clock_t *ptp);

/**
 * \brief BPPDU event message receive.
 * This is called when a port receives an event PDU for the clock.
 *
 * \param ptp The PTP instance data.
 * \param portnum The physical port on which the frames was received.
 * \param buf_handle Handle to the received PTP PDU.
 * \param sender sender protocol address.
 *
 * \return true if buffers is used for reply or forwarding
 *         false if buffers is returned to platform
 */
bool vtss_ptp_event_rx(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, uint portnum, ptp_tx_buffer_handle_t *buf_handle, vtss_appl_ptp_protocol_adr_t *sender);

/**
 * \brief BPPDU general message receive.
 *
 * This is called when a port receives a general PDU for the clock.
 * \param ptp The PTP instance data.
 * \param portnum The physical port on which the frames was received.
 * \param buf_handle Handle to the received PTP PDU.
 * \param sender sender protocol address.
 *
 * \return true if buffers is used for reply or forwarding
 *         false if buffers is returned to platform
 */
bool vtss_ptp_general_rx(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, uint portnum, ptp_tx_buffer_handle_t *buf_handle, vtss_appl_ptp_protocol_adr_t *sender);

/* Set debug_mode. */
bool vtss_ptp_debug_mode_set(ptp_clock_t *ptp, int debug_mode, BOOL has_log_to_file, BOOL has_control, u32 log_time);

bool vtss_ptp_cmlds_messages(ptp_cmlds_port_ds_s *port_cmlds, ptp_tx_buffer_handle_t *buf_handle);

typedef struct {
    int debug_mode;
    BOOL file_open;
    BOOL keep_control;
    u32  log_time;
    u32  time_left;
} vtss_ptp_logmode_t;

/* Get debug_mode. */
bool vtss_ptp_debug_mode_get(ptp_clock_t *ptp, vtss_ptp_logmode_t *log_mode);


/* Delete the PTP log file */
bool vtss_ptp_log_delete(ptp_clock_t *ptp);

/*
 * Enable/disable the wireless variable tx delay feature for a port.
 */
bool vtss_ptp_port_wireless_delay_mode_set(ptp_clock_t *ptp, bool enable, int portnum);
bool vtss_ptp_port_wireless_delay_mode_get(ptp_clock_t *ptp, bool *enable, int portnum);

typedef struct vtss_ptp_delay_cfg_s {
    mesa_timeinterval_t base_delay;      /* wireless base delay in scaled ns */
    mesa_timeinterval_t incr_delay;      /* wireless incremental delay pr packet byte in scaled ns */
} vtss_ptp_delay_cfg_t;

/*
 * Pre notification sent from the wireless modem transmitter before the delay is changed.
 */
bool vtss_ptp_port_wireless_delay_pre_notif(ptp_clock_t *ptp, int portnum);

/*
 * Set the delay configuration, sent from the wireless modem transmitter whenever the delay is changed.
 */
bool vtss_ptp_port_wireless_delay_set(ptp_clock_t *ptp, const vtss_ptp_delay_cfg_t *delay_cfg, int portnum);

/*
 * Get the delay configuration.
 */
bool vtss_ptp_port_wireless_delay_get(ptp_clock_t *ptp, vtss_ptp_delay_cfg_t *delay_cfg, int portnum);


/**
 * \brief Other protocol timestamp receive.
 * This is called when a port receives forwarding timestamps from a non PTP protocol.
 *
 * \param ptp The PTP instance data.
 * \param portnum The physical port on which the frames was received.
 * \param ts timestamps.
 *
 * \return void
 */

typedef struct {
    mesa_timestamp_t tx_ts;
    mesa_timestamp_t rx_ts;
    mesa_timeinterval_t corr;
} vtss_ptp_timestamps_t;

void vtss_non_ptp_slave_t1_t2_rx(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptp, int clock_inst, vtss_ptp_timestamps_t *ts, u8 clock_class, u8 log_repeat_interval, bool virt_port);
void vtss_non_ptp_slave_t3_t4_rx(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptp, int clock_inst, vtss_ptp_timestamps_t *ts);
void vtss_non_ptp_slave_timeout_rx(ptp_clock_t *ptp);
void *vtss_non_ptp_slave_check_port(ptp_clock_t *ptp);
u32 vtss_non_ptp_slave_check_port_no(ptp_clock_t *ptp);
void vtss_non_ptp_slave_init(ptp_clock_t *ptp, int portnum);
void vtss_virtual_ptp_announce_rx(CapArray<ptp_clock_t *, VTSS_APPL_CAP_PTP_CLOCK_CNT> &ptpClock, int clock_inst, uint portnum, ClockDataSet *clock_ds, vtss_appl_ptp_clock_timeproperties_ds_t *time_prop);
void vtss_ptp_virtual_port_master_state_init(ptp_clock_t *ptpClock, uint portnum);

void defaultDSChanged(ptp_clock_t *ptpClock);

void vtss_ptp_state_set(u8, ptp_clock_t *, PtpPort_t *);
void vtss_ptp_recommended_state(vtss_ptp_bmc_recommended_state_t rec_state, ptp_clock_t *ptpClock, PtpPort_t *ptpPort);

ptp_tag_conf_t *get_tag_conf(ptp_clock_t *ptpClock, PtpPort_t *ptpPort);

void vtss_ptp_update_vid_pkt_buf(ptp_clock_t *ptpClock, uint32_t port);

void ptp_802_1as_set_current_message_interval(PtpPort_t *ptpPort, i8 rxAnv, i8 rxSyv, i8 rxMpr);
void ptp_802_1as_set_gptp_current_message_interval(PtpPort_t *ptpPort, i8 rxGptp);
void vtss_ptp_apply_profile_defaults_to_port_ds(vtss_appl_ptp_config_port_ds_t *port_ds, vtss_appl_ptp_profile_t profile);

#if defined (VTSS_SW_OPTION_P802_1_AS)
void vtss_ptp_cmlds_set_pdelay_interval(ptp_cmlds_port_ds_t *port_cmlds, i8 rxMpr, bool signalling);
/*
 * Add a new cmlds port instance.
 */
ptp_cmlds_port_ds_t * vtss_ptp_cmlds_port_inst_add(uint port_num);

/*
 * Start/Stop the CMLDS on a port
 */
void vtss_ptp_cmlds_peer_delay_update(ptp_cmlds_port_ds_s *port_cmlds, vtss_appl_clock_identity clock_id, const vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t *conf, uint portnum, bool enable, bool conf_modified);
/* Update CMLDS status on clock instances */
void vtss_ptp_cmlds_clock_inst_status_notify(ptp_clock_t *ptpClock, uint portnum, const vtss_appl_ptp_802_1as_cmlds_status_port_ds_t *status);
#endif
#ifdef __cplusplus
}
#endif

#endif /* _VTSS_PTP_API_H_ */
