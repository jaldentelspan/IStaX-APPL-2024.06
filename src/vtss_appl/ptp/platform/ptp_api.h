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

#ifndef _PTP_API_H_
#define _PTP_API_H_

#include "ptp_constants.h"
#include "vtss_ptp_api.h"
#include "vtss/appl/ptp.h"

typedef enum
{
    PTP_IO_PIN_USAGE_NONE,      // IO PIN is not used
    PTP_IO_PIN_USAGE_MAIN,      // IO PIN is used by the main ptp application for E.g. PHY control or station clock output
    PTP_IO_PIN_USAGE_RS422,     // IO PIN is used by the RS422 feature
    PTP_IO_PIN_USAGE_IO_PIN,    // IO PIN is used by the general IO_PIN function
} ptp_io_pin_usage_t;

typedef struct {
    vtss_appl_ptp_ext_io_mode_t   io_pin;               // actual pin HW configuration
    meba_event_t                  source_id;            // interrupt event associated with the io_pin
    int                           ptp_inst;             // ptp instance that uses the io_pin, = -1 is not used by any PTP instance
    meba_ptp_io_cap_t             board_assignments;    // possible assignments of the pin for the actual board
    ptp_io_pin_usage_t            usage;                // actual use of the pin
} ptp_io_pin_mode_t;

#define MAX_VTSS_TS_IO_ARRAY_SIZE 8
#define VTSS_PTP_VIRTUAL_PORT_DEFAULT_DELAY 5 // pps delay approximated for 1m length.
extern  CapArray<ptp_io_pin_mode_t, MESA_CAP_TS_IO_CNT> ptp_io_pin;

#define VTSS_IS_TIME_IF_IN(val)     ((val) & MEBA_PTP_IO_CAP_TIME_IF_IN)
#define VTSS_IS_TIME_IF_OUT(val)    ((val) & MEBA_PTP_IO_CAP_TIME_IF_OUT)
#define VTSS_IS_IO_PIN_IN(val)      ((val) & MEBA_PTP_IO_CAP_PIN_IN)
#define VTSS_IS_IO_PIN_OUT(val)     ((val) & MEBA_PTP_IO_CAP_PIN_OUT)
#define VTSS_IS_PHY_SYNC(val)       ((val) & MEBA_PTP_IO_CAP_PHY_SYNC)
#define PTP_IO_PIN_UNUSED 0x80  // pin number range is always less than 10.
#define ONE_SEC_TIMER_INIT_VALUE 128
#define PTP_VIRTUAL_PORT_ALARM_TIME 1280 //10 seconds interval. 1 second => 128 ticks, 10 seconds =>1280 ticks.
#define PTP_VIRTUAL_PORT_SUB_DELAY 19 // 19ns
#define MAX_VTSS_PTP_INSTANCES 4

typedef struct {
    int32_t sync_rate;
    int32_t srcClkDomain;
    vtss_ptp_sys_timer_t internal_sync_timer;
} ptp_internal_mode_config_t;

#define DEFAULT_INTERNAL_MODE_SYNC_RATE -3

#ifdef __cplusplus
extern "C" {
#endif

void send_snmp_ptp_clock_state_trap(ptp_slave_t *slave);
void send_snmp_ptp_unicast_comm_state_trap(vtss_appl_ptp_unicast_comm_state_t  state);
void send_snmp_ptp_uc_master_announcement_trap(u32 ip, BOOL new_slave);

#define PTP_SKIP_REDUNDANT_TRAPS(type, value)  do       \
{                                                       \
    static BOOL is_first_call = TRUE;                   \
    static type prev_value;                             \
    if ((!memcmp(&value, &prev_value,sizeof(type))) &&  \
        (!is_first_call)) {                             \
        return;                                         \
    }                                                   \
    prev_value    = value;                              \
    is_first_call = FALSE;                              \
} while (0)

#define SEND_PTP_SLAVE_CLOCK_STATE_CHANGE_TRAP(slave_ptr) do {  \
    send_snmp_ptp_clock_state_trap(slave_ptr);                  \
} while(0)

#define SEND_PTP_UNICAST_COMM_STATE_TRAP(comm_state) do {\
    send_snmp_ptp_unicast_comm_state_trap(comm_state);   \
} while(0)

#define SEND_PTP_UC_MASTER_ANNOUCEMENT_TRAP(ip, created)        do {\
    send_snmp_ptp_uc_master_announcement_trap(ip, created);         \
} while(0)

#define PTP_MAX_INSTANCES_PR_PORT 2 /* Number og clock instances that are allowed to enable the same port */


int extract_date(const char* s, u16* date);

mesa_rc ptp_ifindex_to_port(vtss_ifindex_t i, u32 *v);

mesa_rc ptp_init(vtss_init_data_t *data);

mesa_rc ptp_clock_clockidentity_set(uint instance, vtss_appl_clock_identity *clockIdentity);

void ptp_apply_profile_defaults_to_port_ds(vtss_appl_ptp_config_port_ds_t *port_ds, vtss_appl_ptp_profile_t profile);

void ptp_apply_profile_defaults_to_default_ds(vtss_appl_ptp_clock_config_default_ds_t *default_ds_cfg, vtss_appl_ptp_profile_t profile);
void ptp_get_default_clock_default_ds(vtss_appl_ptp_clock_status_default_ds_t *default_ds_status,
                                      vtss_appl_ptp_clock_config_default_ds_t *default_ds_cfg);

void ptp_clock_default_timeproperties_ds_get(vtss_appl_ptp_clock_timeproperties_ds_t *timeproperties_ds);

void vtss_appl_ptp_clock_config_default_virtual_port_config_get(vtss_appl_ptp_virtual_port_config_t *const c);

bool ptp_get_port_foreign_ds(ptp_foreign_ds_t *f_ds, int portnum, i16 ix, uint instance);

void vtss_ptp_clock_slave_config_set(ptp_servo *servo, const vtss_appl_ptp_clock_slave_config_t *cfg);

mesa_rc ptp_set_virtual_port_clock_class(uint instance, u8 ptp_class);
mesa_rc ptp_set_virtual_port_clock_accuracy(uint instance, u8 ptp_accuracy);
mesa_rc ptp_set_virtual_port_clock_variance(uint instance, u16 ptp_variance);
mesa_rc ptp_set_virtual_port_local_priority(uint instance, u8 local_priority);
mesa_rc ptp_set_virtual_port_priority1(uint instance, u8 priority1);
mesa_rc ptp_set_virtual_port_priority2(uint instance, u8 priority2);
mesa_rc ptp_set_virtual_port_io_pin(uint instance, u16 io_pin, BOOL enable);
mesa_rc ptp_set_virtual_port_time_property(uint instance, const vtss_appl_ptp_clock_timeproperties_ds_t *prop);
mesa_rc ptp_get_virtual_port_time_property(uint instance, vtss_appl_ptp_clock_timeproperties_ds_t *const prop);
mesa_rc ptp_set_virtual_port_clock_identity(uint instance, char *clock_identity, bool enable);
mesa_rc ptp_set_virtual_port_steps_removed(uint instance, uint16_t steps_removed, bool enable);

mesa_rc ptp_clear_virtual_port_clock_class(uint instance);
mesa_rc ptp_clear_virtual_port_clock_accuracy(uint instance);
mesa_rc ptp_clear_virtual_port_clock_variance(uint instance);
mesa_rc ptp_clear_virtual_port_local_priority(uint instance);
mesa_rc ptp_clear_virtual_port_priority1(uint instance);
mesa_rc ptp_clear_virtual_port_priority2(uint instance);
mesa_rc ptp_virtual_port_alarm_set(uint instance, mesa_bool_t enable);
mesa_rc ptp_clock_config_virtual_port_config_get(uint instance, vtss_appl_ptp_virtual_port_config_t *const c);
mesa_port_no_t ptp_get_virtual_port_number();
mesa_rc ptp_1pps_sma_calibrate_virtual_port(uint32_t instance, bool enable);
uint32_t ptp_1pps_calibrated_delay_get();

mesa_rc ptp_set_clock_class(uint instance, u8 ptp_class);
mesa_rc ptp_set_clock_accuracy(uint instance, u8 ptp_accuracy);
mesa_rc ptp_set_clock_variance(uint instance, u16 ptp_variance);

mesa_rc ptp_clear_clock_class(uint instance);
mesa_rc ptp_clear_clock_accuracy(uint instance);
mesa_rc ptp_clear_clock_variance(uint instance);

mesa_rc ptp_clock_send_unicast_cancel(uint instance, uint slave_index, u8 msg_type);

/**
 * \brief Get default filter parameters for a PTP filter.
 *
 */
void vtss_appl_ptp_filter_default_parameters_get(vtss_appl_ptp_clock_filter_config_t *c, vtss_appl_ptp_profile_t profile);

mesa_rc vtss_appl_ptp_clock_servo_clear(uint instance);

void vtss_appl_ptp_clock_servo_default_parameters_get(vtss_appl_ptp_clock_servo_config_t *c, vtss_appl_ptp_profile_t profile);

/**
 * \brief Get servo status parameters for a Default PTP servo instance.
 *
 * \param s [OUT]  pointer to a structure containing the status for
 *                the servo
 * \param instance clock instance number.
 * \return true if success
 */
mesa_rc vtss_appl_ptp_clock_servo_status_get(uint instance, vtss_ptp_servo_status_t *s);

/* latency observed in onestep tx timestamping */
typedef struct observed_egr_lat_t {
    mesa_timeinterval_t max;
    mesa_timeinterval_t min;
    mesa_timeinterval_t mean;
    u32 cnt;
} observed_egr_lat_t;

/**
 * \brief Get observed egress latency.
 * \param c [OUT]  pointer to a structure containing the latency.
 * \return void
 */
void ptp_clock_egress_latency_get(observed_egr_lat_t *lat);

/**
 * \brief Clear observed egress latency.
 * \return void
 */
void ptp_clock_egress_latency_clear(void);

/* external clock output configuration */

/**
 * \brief Get external clock output default configuration.
 * \param mode [OUT]  pointer to a structure containing the configuration.
 * \return void
 */

void vtss_ext_clock_out_default_get(vtss_appl_ptp_ext_clock_mode_t *mode);

typedef enum  {
    VTSS_PTP_RS422_DISABLE,     /* RS422 mode not used */
    VTSS_PTP_RS422_MAIN_AUTO,   /* RS422 main auto mode i.e 1 pps master */
    VTSS_PTP_RS422_SUB,         /* RS422 sub mode i.e 1 pps slave */
    VTSS_PTP_RS422_MAIN_MAN,    /* RS422 main man mode i.e 1 pps master */
    VTSS_PTP_RS422_CALIB,       /* RS422 calibration mode. Same as main auto mode except RS422_1588_SLVOEn = 0 and RS422_1588_MSTOEn = 1 (both have been inverted) */
} ptp_rs422_mode_t;

typedef enum  {
    VTSS_PTP_RS422_PROTOCOL_SER_POLYT, /* use serial protocol with NMEA proprietary POLYT format */
    VTSS_PTP_RS422_PROTOCOL_SER_ZDA,   /* use serial protocol with NMEA ZDA format */
    VTSS_PTP_RS422_PROTOCOL_SER_GGA,   /* use serial protocol with NMEA GGA format */
    VTSS_PTP_RS422_PROTOCOL_SER_RMC,   /* use serial protocol with NMEA RMC format */
    VTSS_PTP_RS422_PROTOCOL_PIM,       /* use PIM protocol */
} ptp_rs422_protocol_t;


/* RS422 PTP configuration */
typedef struct vtss_ptp_rs422_conf_t {
    ptp_rs422_mode_t     mode;   /* Select rs422 mode:
                                    DISABLE : rs422 not in use
                                    MAIN: main module function
                                    SUB: sub module function */
    u32                  delay;  /* in MAIN mode: read only, measured turn around delay in ns.
                                    in SUB mode: reload value used to compensate for path delay (in ns) */
    ptp_rs422_protocol_t proto;  /* Selected protocol */
    uint32_t             instance; /* instance currently using RS-422 interface. */
    mesa_port_no_t       port;   /* Switch port used for the PIM protocol */
} vtss_ptp_rs422_conf_t;

/**
 * \brief Get serval rs424 external clock protocol configuration.
 * \param mode [OUT]  pointer to a variable to receive the protocol value
 * \return void
 */
void vtss_ext_clock_rs422_protocol_get(ptp_rs422_protocol_t *proto);

/**
 * \brief Get serval rs424 external clock configuration.
 * \param mode [OUT]  pointer to a structure containing the configuration.
 * \return void
 */
void vtss_ext_clock_rs422_conf_get(vtss_ptp_rs422_conf_t *mode);

void vtss_ext_clock_rs422_default_conf_get(vtss_ptp_rs422_conf_t *mode);

/**
 * \brief Set serval rs422 external clock configuration.
 * \param mode [IN]  pointer to a structure containing the configuration.
 * \return void
 */
void vtss_ext_clock_rs422_conf_set(const vtss_ptp_rs422_conf_t *mode);

/**
 * \brief Set serval rs422 time at next 1PPS.
 * \param t [IN]  time at nexr 1PPS.
 * \return void
 */
void vtss_ext_clock_rs422_time_set(const mesa_timestamp_t *t);
void ptp_virtual_port_timestamp_rx(const mesa_timestamp_t *t);
void ptp_virtual_port_alarm_rx(mesa_bool_t alarm);

/* G.8275 Holdover Spec PTP configuration */
typedef struct vtss_appl_ho_spec_conf_t {
    u32                  cat1;  /* Holdover spec time in sec */
    u32                  cat2;  /* Holdover spec time in sec */
    u32                  cat3;  /* Holdover spec time in sec */
} vtss_ho_spec_conf_t;

/**
 * \brief Get Holdover spec configuration.
 * \param conf [OUT]  pointer to a structure containing the configuration.
 * \return void
 */
void vtss_ho_spec_conf_get(vtss_ho_spec_conf_t *spec);


/**
 * \brief Set Holdover spec configuration.
 * \param spec [IN]  pointer to a structure containing the configuration.
 * \return void
 */
void vtss_ho_spec_conf_set(const vtss_ho_spec_conf_t *spec);

// Update phy correction field as per different phys.
// In Gen2 phys with Jaguar-2 platform, delay request packet is sent by filling ltc in orgin timestamp. Switch or Mac would update nano second part in resered bytes. PHY would update correction field by subtracting time derived from reserved bytes from its current ltc.
// Gen2 phys or Gen 3C (lan8814 Rev C)which are not used along with switch timestamping need to use TC mode A for correction field update in delay request messages. Delay-req packet is sent by inserting nano seconds part of LTC in reserved bytes and phy takes care of updating correction field using TC mode A.
// In Gen3(Lan8814 rev B), Gen2(except vsc8574) Gen3C(Indy rev C), for TC mode C, delay request packet is sent by subtracting ltc from correction field.
// But, in Gen3, seconds field has to be placed in leftmost 18 bits and nano seconds part in next 30 bits. In Gen3C, Gen2 phys, seconds + nanoseconds together are placed in left most 48 bits.
typedef enum {
    PHY_CORR_GEN_2 = 0,
    PHY_CORR_GEN_2A,
    PHY_CORR_GEN_3,
    PHY_CORR_GEN_3C
} vtss_ptp_phy_corr_type_t;

vtss_ptp_phy_corr_type_t vtss_port_phy_delay_corr_upd(uint port);
bool vtss_port_has_lan8814_phy(uint port);

void vtss_ptp_port_crc_update(uint32_t inst, uint32_t port);

typedef struct vtss_ptp_port_link_state_t {
    bool link_state;        /* true if link is up */
    bool in_sync_state;     /* true if port local timer is in sync (only relevant for ports with PHY timestaming */
    bool forw_state;        /* true if port filter does not indicate 'discard'  */
    bool phy_timestamper;   /* true if port uses phy timestamp feature */
} vtss_ptp_port_link_state_t;

/**
 * \brief Get port state.
 * \param ds [OUT]  pointer to a structure containing the port status.
 * \return true if valid port number
 */
mesa_rc ptp_get_port_link_state(uint instance, int portnum, vtss_ptp_port_link_state_t *ds);

bool ptp_debug_mode_set(int debug_mode, uint instance, BOOL has_log_to_file, BOOL has_control, u32 log_time);
bool ptp_debug_mode_get(uint instance, vtss_ptp_logmode_t *log_mode);
bool ptp_log_delete(uint instance);

/**
 * \brief Get afi_mode.
 * \param inst    [IN] PTP instance
 * \param ann     [IN] true if Announce AFI mode, false if Sync AFI mode.
 * \param enable [OUT] true if mode is enabled
 * \return error code
 */
mesa_rc ptp_afi_mode_get(uint instance, bool ann, bool* enable);

/**
 * \brief Set afi_mode.
 * \param inst   [IN] PTP instance
 * \param ann    [IN] true if Announce AFI mode, false if Sync AFI mode.
 * \param enable [IN] true if mode is enabled
 * \return error code
 */
mesa_rc ptp_afi_mode_set(uint instance, bool ann, bool enable);

/**
* \brief Execute a one-pps external clock input action.
* \param action [IN]  action:   [0]    Dump statistics
                                [1]    Clear statistics
                                [2]    Enable offset logging
                                [3]    Disable offset logging
* \return void
*/

typedef struct vtss_ptp_one_pps_statistic_t {
    i32 min;
    i32 max;
    i32 mean;
    bool dLos;
} vtss_ptp_one_pps_statistic_t;

/**
 * \brief Enable/disable the wireless variable tx delay feature for a port.
 * \param enable [IN]    true => enable vireless, false => disable vireles feature.
 * \param portnum [IN]   ptp port number.
 * \param instance [IN]  ptp instance number.
 * \return true if success, false if invalid portnum or instance.
 */
bool ptp_port_wireless_delay_mode_set(bool enable, int portnum, uint instance);

/**
 * \brief Get the Enable/disable mode for a port.
 * \param enable [OUT]   true => wireless enabled, false => wireles disabled.
 * \param portnum [IN]   ptp port number.
 * \param instance [IN]  ptp instance number.
 * \return true if success, false if invalid portnum or instance.
 */
bool ptp_port_wireless_delay_mode_get(bool *enable, int portnum, uint instance);

/**
 * \brief Pre notification sent from the wireless modem transmitter before the delay is changed.
 * \param portnum [IN]  ptp port number
 * \param instance [IN]  ptp instance number.
 * \return true if success, false in invalid portnum or instance
 */
bool ptp_port_wireless_delay_pre_notif(int portnum, uint instance);

/**
 * \brief Set the delay configuration, sent from the wireless modem transmitter whenever the delay is changed.
 * \Note: the wireless delay for a packet equals: base_delay + packet_length*incr_delay.
 * \param delay_cfg [IN]  delay configuration.
 * \param portnum [IN]  ptp port number.
 * \param instance [IN]  ptp instance number.
 * \return true if success, false in invalid portnum or instance
 */
bool ptp_port_wireless_delay_set(const vtss_ptp_delay_cfg_t *delay_cfg, int portnum, uint instance);

/**
 * \brief Get the delay configuration.
 * \param delay_cfg [OUT]  delay configuration.
 * \param portnum [IN]  ptp port number.
 * \param instance [IN]  ptp instance number.
 * \return true if success, false in invalid portnum or instance
 */
bool ptp_port_wireless_delay_get(vtss_ptp_delay_cfg_t *delay_cfg, int portnum, uint instance);

void vtss_appl_ptp_clock_slave_default_config_get(vtss_appl_ptp_clock_slave_config_t *cfg);

mesa_rc vtss_appl_ptp_io_pin_conf_default_get(u32 pin_idx, vtss_appl_ptp_ext_io_mode_t *const mode);

void ptp_1pps_ptp_slave_t1_t2_rx(int inst, mesa_port_no_t port_no, vtss_ptp_timestamps_t *ts);

mesa_rc ptp_clock_slave_statistics_enable(int instance, bool enable);

mesa_rc ptp_clock_slave_statistics_get(int instance, vtss_ptp_slave_statistics_t *statistics, bool clear);

mesa_rc ptp_clock_path_trace_get(int instance, ptp_path_trace_t *trace);

mesa_rc ptp_clock_path_802_1as_status_get(int instance, vtss_ptp_clock_802_1as_bmca_t *status);

mesa_rc ptp_port_path_802_1as_status_get(int instance, mesa_port_no_t port_no, vtss_ptp_port_802_1as_bmca_t *status);

typedef struct {
    u32                 tod_cnt;
    u32                 one_pps_cnt;
    u32                 missed_one_pps_cnt;
    u32                 missed_tod_rx_cnt;
} vtss_ptp_one_pps_tod_statistics_t;

mesa_rc ptp_clock_one_pps_tod_statistics_get(vtss_ptp_one_pps_tod_statistics_t *statistics, bool clear);

void ptp_local_clock_time_set(mesa_timestamp_t *t, u32 domain);

u32 ptp_instance_2_timing_domain(int instance);
const char *ptp_error_txt(mesa_rc rc); // Convert Error code to text

typedef enum  {
    VTSS_PTP_SYNCE_NONE,     /* No SyncE source is selected */
    VTSS_PTP_SYNCE_ELEC,     /* Electrical SyncE source is selected */
    VTSS_PTP_SYNCE_PAC       /* Packet SyncE source is selected */
} vtss_ptp_synce_src_type_t;

typedef struct {
    vtss_ptp_synce_src_type_t   type;
    int                         ref;
} vtss_ptp_synce_src_t;

mesa_rc ptp_set_selected_src(vtss_ptp_synce_src_t *src);
mesa_rc ptp_get_selected_src(vtss_ptp_synce_src_t *src);
const char *sync_src_type_2_txt(vtss_ptp_synce_src_type_t s);

typedef enum  {
    VTSS_PTP_SERVO_NONE,        /* No servo is active */
    VTSS_PTP_SERVO_HYBRID,      /* Servo in hybrid mode */
    VTSS_PTP_SERVO_ELEC,        /* Servo in Electrical mode */
    VTSS_PTP_SERVO_PAC,         /* Servo in Packet mode */
    VTSS_PTP_SERVO_HOLDOVER,    /* Servo in Holdover mode */
} vtss_ptp_servo_mode_t;

typedef struct {
    vtss_ptp_servo_mode_t   mode;
    int                     ref;
} vtss_ptp_servo_mode_ref_t;

mesa_rc vtss_ptp_get_servo_mode_ref(int inst, vtss_ptp_servo_mode_ref_t *mode_ref);
const char *sync_servo_mode_2_txt(vtss_ptp_servo_mode_t s);

mesa_rc vtss_ptp_switch_to_packet_mode(int instance);
mesa_rc vtss_ptp_switch_to_hybrid_mode(int instance);
mesa_rc vtss_ptp_get_hybrid_mode_state(int instance, bool *hybrid_mode);

mesa_rc vtss_ptp_set_active_ref(int stream);
mesa_rc vtss_ptp_set_active_electrical_ref(int input);

mesa_rc vtss_ptp_set_hybrid_transient(vtss_ptp_hybrid_transient state);

mesa_rc vtss_ptp_force_holdover_set(int instance, BOOL enable);
mesa_rc vtss_ptp_force_holdover_get(int instance, BOOL *enable);

mesa_rc vtss_ptp_set_1pps_virtual_reference(int inst, bool enable, bool warm_start);
mesa_rc vtss_ptp_set_servo_device_1pps_virtual_port(bool enable);
/**
 * \brief Get the holdover OK status of a PTP instance.
 * \param inst [IN] instance number of the PTP instance
 * \return false if the PTP instance has not aquired a stable holdover value
 *         true  if the PTP instance has aquired a stable holdover value
 */
bool vtss_ptp_servo_get_holdover_status(int inst);

/**
 * \brief Get Default PTP CMLDS port Configuration.
 * \param conf[OUT]         A pointer to structure through configuration is obtained.
 */
void vtss_appl_ptp_cmlds_conf_defaults_get(vtss_appl_ptp_802_1as_cmlds_conf_port_ds_t *const conf);

uint32_t ptp_meba_cap_synce_dpll_mode_dual();
uint32_t ptp_meba_cap_synce_dpll_mode_single();
uint32_t ptp_mesa_cap_ts_separate_domain();
bool ptp_cap_sub_nano_second();
#ifdef __cplusplus
}
#endif

// Set auto delay response enabling/disabling for each clock domain.
bool ptp_auto_delay_resp_set(uint32_t clock_domain, bool set, bool enable);

// Configure internal mode.
bool ptp_internal_mode_config_set(uint32_t instance, ptp_internal_mode_config_t in_cfg);
bool ptp_internal_mode_config_get(uint32_t instance, ptp_internal_mode_config_t *const in_cfg);

extern bool calib_1pps_initiate;
extern bool calib_1pps_enabled;

#endif // _PTP_API_H_

